#include "stdafx.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include "WebDebugTemplate.h"
#include "WebDebug.h"

void WriteStandardHeader(WebDebugConnection * connection, const char * title)
{
	connection->WriteString(
	"<!DOCTYPE html>\n"
	"<html lang=\"en\">\n"
	"	<head>\n"
	"		<meta charset=\"utf-8\">\n"
	"		<link href=\"css/bootstrap.min.css\" rel=\"stylesheet\">\n"
	);
	connection->WriteF("		<title>%s</title>\n", title);
	connection->WriteString("	</head><body>\n");
}

void WriteStandardFooter(WebDebugConnection * connection, const char * user_script)
{
	connection->WriteString(
		"<script src=\"js/bootstrap.min.js\"></script>\n"
		"<script src=\"js/jquery-1.9.1.min.js\"></script>\n"
	);

	// If there's a user script, append it after the other scripts.
	if (user_script)
		connection->WriteF("<script src=\"%s\"></script>\n", user_script);

	connection->WriteString(
		"</body>\n"
		"</html>\n"
	);
}
#endif