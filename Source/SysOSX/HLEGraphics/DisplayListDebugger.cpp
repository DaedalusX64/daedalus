#include "stdafx.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "SysOSX/Debug/WebDebug.h"
#include "SysOSX/Debug/WebDebugTemplate.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/PngUtil.h"

#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/DLParser.h"

#include "Utility/StringUtil.h"
#include "Utility/Mutex.h"

#include <GL/glfw.h>


#ifdef DAEDALUS_DEBUG_DISPLAYLIST

static bool gDebugging = false;

// These coordinate requests between the web threads and the main thread (only the main thread can call OpenGL functions)
static GLFWcond gScreenshotCond   = NULL;
static GLFWmutex gScreenshotMutex = NULL;

enum DebugTask
{
	kTaskUndefined,
	kTaskStartDebugging,
	kTaskStopDebugging,
	kTaskScrub,
	kTaskTakeScreenshot,
	kTaskDumpDList,
};

// This mutex is to ensure that only one connection can request a screenshot at a time.
static Mutex 				gMainThreadSync("MainThreadSync");
static WebDebugConnection * gActiveConnection = NULL;
static DebugTask			gDebugTask        = kTaskUndefined;

static u32					gInstructionCountLimit = kUnlimitedInstructionCount;

bool DLDebugger_IsDebugging()
{
	return gDebugging;
}

void DLDebugger_RequestDebug()
{
	gDebugging = true;
}

void DLDebugger_ProcessDebugTask()
{
	// Check if a web request is waiting for a screenshot.
	glfwLockMutex(gScreenshotMutex);
	if (WebDebugConnection * connection = gActiveConnection)
	{
		bool handled = false;
		switch (gDebugTask)
		{
			case kTaskUndefined:
				break;
			case kTaskStartDebugging:
			{
				u32 num_ops = DLParser_Process(kUnlimitedInstructionCount, NULL);
				gInstructionCountLimit = num_ops;

				connection->BeginResponse(200, -1, "text/plain" );
				connection->WriteF("{\"num_ops\":%d}", num_ops);
				connection->EndResponse();
				gDebugging = true;
				handled = true;
				break;
			}
			case kTaskStopDebugging:
			{
				gInstructionCountLimit = kUnlimitedInstructionCount;
				connection->BeginResponse(200, -1, "text/plain");
				connection->WriteString("ok");
				connection->EndResponse();
				gDebugging = false;
				handled = true;
				break;
			}
			case kTaskScrub:
			{
				DLParser_Process(gInstructionCountLimit, NULL);

				connection->BeginResponse(200, -1, "text/plain" );
				connection->WriteString("ok");
				connection->EndResponse();
				handled = true;
				break;
			}
			case kTaskTakeScreenshot:
			{
				connection->BeginResponse(200, -1, "image/png" );

				u32 width;
				u32 height;
				CGraphicsContext::Get()->GetScreenSize(&width, &height);

				// Make the BYTE array, factor of 3 because it's RBG.
				void * pixels = malloc( 4 * width * height );

				glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

				// NB, pass a negative pitch, to render the screenshot the right way up.
				s32 pitch = -(width * 4);

				PngSaveImage(connection, pixels, NULL, TexFmt_8888, pitch, width, height, false);

				free(pixels);

				connection->EndResponse();
				handled = true;
				break;
			}
			case kTaskDumpDList:
			{
				connection->BeginResponse(200, -1, "text/plain");
				DLParser_Process(kUnlimitedInstructionCount, connection);
				connection->EndResponse();
				handled = true;
				break;
			}
		}

		if (!handled)
		{
			Generate500(connection, "Unhandled DebugTask");
		}
		gActiveConnection = NULL;
		gDebugTask = kTaskUndefined;

	}
	glfwUnlockMutex(gScreenshotMutex);
	glfwSignalCond(gScreenshotCond);
}

bool DLDebugger_Process()
{
	DLDebugger_ProcessDebugTask();

	while(gDebugging)
	{
		DLParser_Process(gInstructionCountLimit, NULL);
		DLDebugger_ProcessDebugTask();

		CGraphicsContext::Get()->UpdateFrame( false );
	}

	return false;
}

static void DoTask(WebDebugConnection * connection, DebugTask task)
{
	// Only one request for a screenshot can be serviced at a time.
	MutexLock lock(&gMainThreadSync);

	// Request a screenshot from the main thread.
	glfwLockMutex(gScreenshotMutex);
	gActiveConnection = connection;
	gDebugTask        = task;
	glfwWaitCond(gScreenshotCond, gScreenshotMutex, GLFW_INFINITY);
	glfwUnlockMutex(gScreenshotMutex);
}

static void DLDebugHandler(void * arg, WebDebugConnection * connection)
{
	const WebDebugConnection::QueryParams & params = connection->GetQueryParams();
	if (!params.empty())
	{
		bool ok = false;

		for (size_t i = 0; i < params.size(); ++i)
		{
			if (params[i].Key == "action")
			{
				if (params[i].Value == "stop")
				{
					DoTask(connection, kTaskStartDebugging);
					return;
				}
				else if (params[i].Value == "start")
				{
					DoTask(connection, kTaskStopDebugging);
					return;
				}
			}
			else if (params[i].Key == "scrub")
			{
				gInstructionCountLimit = ParseU32(params[i].Value, 10);
				DoTask(connection, kTaskScrub);
				return;
			}
			else if (params[i].Key == "screen")
			{
				//int cmd = atoi(params[i].Value);
				DoTask(connection, kTaskTakeScreenshot);
				return;
			}
			else if (params[i].Key == "dump")
			{
				//int cmd = atoi(params[i].Value);
				DoTask(connection, kTaskDumpDList);
				return;
			}
		}

		// Fallthrough for handlers that just return 'ok'.
		connection->BeginResponse(ok ? 200 : 400, -1, "text/plain" );
		connection->WriteString(ok ? "ok\n" : "fail\n");
		connection->EndResponse();
		return;
	}

	connection->BeginResponse(200, -1, "text/html" );

	WriteStandardHeader(connection, "Display List");

	connection->WriteString(
		"<div class=\"container\">\n"
		"	<div class=\"row\">\n"
		"		<div class=\"span12\">\n"
	);
	connection->WriteString("<h1 id=\"title\">Display List</h1>\n");
	connection->WriteString(
		"		</div>\n"
		"	</div>\n"
		"</div>\n"
	);

	WriteStandardFooter(connection, "js/dldebugger.js");
	connection->EndResponse();
}

bool DLDebugger_RegisterWebDebug()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	WebDebug_Register( "/dldebugger", &DLDebugHandler, NULL );
#endif
	gScreenshotCond = glfwCreateCond();
	gScreenshotMutex = glfwCreateMutex();
	return true;
}

#endif	//DAEDALUS_DEBUG_DISPLAYLIST
