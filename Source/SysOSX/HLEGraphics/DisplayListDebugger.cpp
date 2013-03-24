#include "stdafx.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "SysOSX/Debug/WebDebug.h"
#include "SysOSX/Debug/WebDebugTemplate.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/PngUtil.h"

#include "Utility/Mutex.h"

#include <GL/glfw.h>

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

static bool gDebugging = false;

// These coordinate requests between the web threads and the main thread (only the main thread can call OpenGL functions)
static GLFWcond gScreenshotCond   = NULL;
static GLFWmutex gScreenshotMutex = NULL;

// This mutex is to ensure that only one connection can request a screenshot at a time.
static Mutex gSerialiseScreenshots("Screenshots");

static WebDebugConnection * gScreenshotConnection = NULL;

bool DLDebugger_IsDebugging()
{
	return gDebugging;
}

void DLDebugger_RequestDebug()
{
	gDebugging = true;
}

bool DLDebugger_Process()
{
	if (gDebugging)
	{
		printf("Debugging\n");
		gDebugging = false;
	}

	// Check if a web request is waiting for a screenshot.
	glfwLockMutex(gScreenshotMutex);
	if (WebDebugConnection * connection = gScreenshotConnection)
	{
		u32 width;
		u32 height;
		CGraphicsContext::Get()->GetScreenSize(&width, &height);

		// Make the BYTE array, factor of 3 because it's RBG.
		void * pixels = malloc( 4 * width * height );

		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		// NB, pass a negative pitch, to render the screenshot the right way ip.
		s32 pitch = -(width * 4);

		PngSaveImage(connection, pixels, NULL, TexFmt_8888, pitch, width, height, false);

		free(pixels);

		connection->EndResponse();
		gScreenshotConnection = NULL;
	}
	glfwUnlockMutex(gScreenshotMutex);
	glfwSignalCond(gScreenshotCond);

	return false;
}

static void TakeScreenshot(WebDebugConnection * connection)
{
	connection->BeginResponse(200, -1, "image/png" );

	// Only one request for a screenshot can be serviced at a time.
	MutexLock lock(&gSerialiseScreenshots);

	// Request a screenshot from the main thread.
	glfwLockMutex(gScreenshotMutex);
	gScreenshotConnection = connection;
	glfwWaitCond(gScreenshotCond, gScreenshotMutex, GLFW_INFINITY);
	glfwUnlockMutex(gScreenshotMutex);
}

static void DLDebugHandler(void * arg, WebDebugConnection * connection)
{
	const WebDebugConnection::QueryParams & params = connection->GetQueryParams();
	if (!params.empty())
	{
		for (size_t i = 0; i < params.size(); ++i)
		{
			if (params[i].Key == "action")
			{
				if (params[i].Value == "stop")
				{
					DLDebugger_RequestDebug();
				}
			}
			else if (params[i].Key == "setcmd")
			{
				//int cmd = atoi(params[i].Value);
				TakeScreenshot(connection);
				return;
			}
		}

		// Fallthrough for handlers that just return 'ok'.
		connection->BeginResponse(200, -1, "text/plain" );
		connection->WriteString("ok\n");
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
