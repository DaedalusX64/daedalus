#include "stdafx.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "SysOSX/Debug/WebDebug.h"
#include "SysOSX/Debug/WebDebugTemplate.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/PngUtil.h"

#include "HLEGraphics/BaseRenderer.h"
#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/RDP.h"
#include "HLEGraphics/RDPStateManager.h"

#include "Utility/StringUtil.h"
#include "Utility/Mutex.h"

#include <GL/glfw.h>


#ifdef DAEDALUS_DEBUG_DISPLAYLIST

static bool gDebugging = false;

// These coordinate requests between the web threads and the main thread (only the main thread can call OpenGL functions)
static GLFWcond  gMainThreadCond  = NULL;
static GLFWmutex gMainThreadMutex = NULL;

enum DebugTask
{
	kTaskUndefined,
	kBreakExecution,
	kResumeExecution,
	kTaskScrub,
	kTaskTakeScreenshot,
	kTaskDumpDList,
};

static WebDebugConnection * gActiveConnection = NULL;
static DebugTask			gDebugTask        = kTaskUndefined;
static u32					gInstructionCountLimit = kUnlimitedInstructionCount;


class HTMLDebugOutput : public DLDebugOutput
{
public:
	explicit HTMLDebugOutput(WebDebugConnection * connection)
	:	Connection(connection)
	{
	}

	virtual size_t Write(const void * p, size_t len)
	{
		return Connection->Write(p, len);
	}

	virtual void BeginInstruction(u32 idx, u32 cmd0, u32 cmd1, const char * name)
	{
		Print("<span class=\"hle-instr\" id=\"I%d\">", idx);
		Print("%05d %08x%08x %-10s\n", idx, cmd0, cmd1, name);
		Print("<span class=\"hle-detail\" style=\"display:none\">");
	}

	virtual void EndInstruction()
	{
		Print("</span></span>");
	}

	WebDebugConnection * Connection;
};

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
	glfwLockMutex(gMainThreadMutex);
	if (WebDebugConnection * connection = gActiveConnection)
	{
		bool handled = false;
		switch (gDebugTask)
		{
			case kTaskUndefined:
				break;
			case kBreakExecution:
			{
				u32 num_ops = DLParser_Process(kUnlimitedInstructionCount, NULL);
				gInstructionCountLimit = num_ops;

				connection->BeginResponse(200, -1, kApplicationJSON);
				connection->WriteF("{\"num_ops\":%d}", num_ops);
				connection->EndResponse();
				gDebugging = true;
				handled = true;
				break;
			}
			case kResumeExecution:
			{
				gInstructionCountLimit = kUnlimitedInstructionCount;
				connection->BeginResponse(200, -1, kTextPlain);
				connection->WriteString("ok");
				connection->EndResponse();
				gDebugging = false;
				handled = true;
				break;
			}
			case kTaskScrub:
			{
				DLParser_Process(gInstructionCountLimit, NULL);

				u64 mux = gRenderer->GetMux();
				u32 mux_hi = mux >> 32;
				u32 mux_lo = mux & 0xffffffff;

				connection->BeginResponse(200, -1, kApplicationJSON);
				connection->WriteString("{\n");
				connection->WriteF("\t\"mux\": {\"hi\": \"0x%08x\", \"lo\": \"0x%08x\"},\n",
					mux_hi, mux_lo);
				connection->WriteString("\t\"tiles\": [\n");
				for (u32 i = 0; i < 8; ++i)
				{
					if (i > 0)
						connection->WriteString(",\n");

					const RDP_Tile &     tile      = gRDPStateManager.GetTile(i);
					const RDP_TileSize & tile_size = gRDPStateManager.GetTileSize(i);
					connection->WriteString("\t{\n");
					connection->WriteF("\t\t\"format\": %d,\n",   tile.format);
					connection->WriteF("\t\t\"size\": %d,\n",     tile.size);
					connection->WriteF("\t\t\"line\": %d,\n",     tile.line);
					connection->WriteF("\t\t\"tmem\": %d,\n",     tile.tmem);
					connection->WriteF("\t\t\"palette\": %d,\n",  tile.palette);
					connection->WriteF("\t\t\"clamp_s\": %d,\n",  tile.clamp_s);
					connection->WriteF("\t\t\"mirror_s\": %d,\n", tile.mirror_s);
					connection->WriteF("\t\t\"mask_s\": %d,\n",   tile.mask_s);
					connection->WriteF("\t\t\"shift_s\": %d,\n",  tile.shift_s);
					connection->WriteF("\t\t\"clamp_t\": %d,\n",  tile.clamp_t);
					connection->WriteF("\t\t\"mirror_t\": %d,\n", tile.mirror_t);
					connection->WriteF("\t\t\"mask_t\": %d,\n",   tile.mask_t);
					connection->WriteF("\t\t\"shift_t\": %d,\n",  tile.shift_t);
					connection->WriteF("\t\t\"left\": %d,\n",     tile_size.left);
					connection->WriteF("\t\t\"top\": %d,\n",      tile_size.top);
					connection->WriteF("\t\t\"right\": %d,\n",    tile_size.right);
					connection->WriteF("\t\t\"bottom\": %d\n",    tile_size.bottom);
					connection->WriteString("\t}");
				}
				connection->WriteString("]");
				connection->WriteString("}");
				connection->EndResponse();
				handled = true;
				break;
			}
			case kTaskTakeScreenshot:
			{
				connection->BeginResponse(200, -1, kImagePng);

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
				connection->BeginResponse(200, -1, kTextPlain);

				HTMLDebugOutput		dl_output(connection);

				DLParser_Process(kUnlimitedInstructionCount, &dl_output);
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
	glfwUnlockMutex(gMainThreadMutex);
	glfwSignalCond(gMainThreadCond);
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
	// Request a screenshot from the main thread.
	glfwLockMutex(gMainThreadMutex);
	gActiveConnection = connection;
	gDebugTask        = task;
	glfwWaitCond(gMainThreadCond, gMainThreadMutex, GLFW_INFINITY);
	glfwUnlockMutex(gMainThreadMutex);
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
				if (params[i].Value == "break")
				{
					DoTask(connection, kBreakExecution);
					return;
				}
				else if (params[i].Value == "resume")
				{
					DoTask(connection, kResumeExecution);
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
		connection->BeginResponse(ok ? 200 : 400, -1, kTextPlain);
		connection->WriteString(ok ? "ok\n" : "fail\n");
		connection->EndResponse();
		return;
	}

	if (!ServeResource(connection, "/html/dldebugger.html"))
	{
		Generate500(connection, "Couldn't load html/debugger.html");
	}
}

bool DLDebugger_RegisterWebDebug()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	WebDebug_Register( "/dldebugger", &DLDebugHandler, NULL );
#endif
	gMainThreadCond = glfwCreateCond();
	gMainThreadMutex = glfwCreateMutex();
	return true;
}

#endif	//DAEDALUS_DEBUG_DISPLAYLIST
