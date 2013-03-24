#include "stdafx.h"
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

void WriteStandardFooter(WebDebugConnection * connection)
{
	connection->WriteString(
		"<script src=\"js/bootstrap.min.js\"></script>\n"
		"</body>\n"
		"</html>\n"
	);
}
