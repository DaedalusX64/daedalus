#ifndef WEBDEBUGTEMPLATE_H_
#define WEBDEBUGTEMPLATE_H_

#include <stdlib.h>

class WebDebugConnection;

void WriteStandardHeader(WebDebugConnection * connection, const char * title);
void WriteStandardFooter(WebDebugConnection * connection, const char * user_script = NULL);

#endif // WEBDEBUGTEMPLATE_H_
