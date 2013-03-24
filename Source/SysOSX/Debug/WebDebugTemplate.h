#ifndef WEBDEBUGTEMPLATE_H_
#define WEBDEBUGTEMPLATE_H_

class WebDebugConnection;

void WriteStandardHeader(WebDebugConnection * connection, const char * title);
void WriteStandardFooter(WebDebugConnection * connection);

#endif // WEBDEBUGTEMPLATE_H_
