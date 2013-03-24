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
	"		<link href=\"//netdna.bootstrapcdn.com/twitter-bootstrap/2.3.1/css/bootstrap-combined.min.css\" rel=\"stylesheet\">\n"
	);
	connection->WriteF("		<title>%s</title>\n", title);
	connection->WriteString("	</head><body>\n");
}

void WriteStandardFooter(WebDebugConnection * connection)
{
	connection->WriteString(
		"<script src=\"//netdna.bootstrapcdn.com/twitter-bootstrap/2.3.1/js/bootstrap.min.js\"></script>\n"
		"</body>\n"
		"</html>\n"
	);
}
