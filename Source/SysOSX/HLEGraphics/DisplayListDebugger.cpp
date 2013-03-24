#include "stdafx.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "SysOSX/Debug/WebDebug.h"
#include "SysOSX/Debug/WebDebugTemplate.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
bool DLDebugger_IsDebugging()
{
	return false;
}

void DLDebugger_RequestDebug()
{
}

bool DLDebugger_Process()
{
	return false;
}

static void DLDebugHandler(void * arg, WebDebugConnection * connection)
{
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


	connection->WriteString(
		"<script src=\"js/dldebugger.js\"></script>\n"
	);

	WriteStandardFooter(connection);
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
