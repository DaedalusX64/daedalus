#ifndef WEBDEBUG_H_
#define WEBDEBUG_H_

#include "Utility/DataSink.h"

struct WebbyConnection;

class WebDebugConnection : public DataSink
{
public:
	explicit WebDebugConnection(WebbyConnection * connection);

	enum State
	{
		kUnresponded,
		kResponding,
		kResponded,
	};

	const char * 	GetQueryParams() const;
	State			GetState() const				{ return mState; }

	void 			BeginResponse(int code, int content_length, const char * content_type);
	void			EndResponse();

	void			WriteString(const char * str);
	void			WriteF(const char * format, ...);

	// DataSink interface
	virtual size_t	Write(const void * p, size_t len);
	virtual void	Flush();
private:
	struct WebbyConnection * 	mConnection;
	State						mState;

	size_t						mBytesExpected;
	size_t						mBytesWritten;
};

typedef void (*WebDebugHandler)(void * arg, WebDebugConnection * connection);
void WebDebug_Register(const char * request, WebDebugHandler handler, void * arg);

bool WebDebug_Init();
void WebDebug_Fini();

#endif // WEBDEBUG_H_
