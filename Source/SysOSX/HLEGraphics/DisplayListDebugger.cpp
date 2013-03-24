#include "stdafx.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "SysOSX/Debug/WebDebug.h"
#include "SysOSX/Debug/WebDebugTemplate.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

static bool gDebugging = false;

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
	return false;
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
		}

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
	return true;
}

#endif	//DAEDALUS_DEBUG_DISPLAYLIST
