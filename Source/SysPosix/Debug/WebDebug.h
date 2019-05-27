#ifndef SYSOSX_DEBUG_WEBDEBUG_H_
#define SYSOSX_DEBUG_WEBDEBUG_H_

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include "Utility/DataSink.h"
#include "Utility/String.h"

#include <vector>

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

	struct Param
	{
		ConstStringRef Key;
		ConstStringRef Value;
	};

	typedef std::vector<Param> QueryParams;

	const char * 	GetQueryString() const;
	State			GetState() const				{ return mState; }

	const QueryParams & GetQueryParams() const		{ return mQueryParams; }

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

	QueryParams					mQueryParams;
};

extern const char * const kApplicationJavascript;
extern const char * const kApplicationJSON;
extern const char * const kImagePng;
extern const char * const kTextCSS;
extern const char * const kTextHTML;
extern const char * const kTextPlain;


bool ServeResource(WebDebugConnection * connection, const char * resource_path);
void Generate404(WebDebugConnection * connection, const char * request);
void Generate500(WebDebugConnection * connection, const char * message);

typedef void (*WebDebugHandler)(void * arg, WebDebugConnection * connection);
void WebDebug_Register(const char * request, WebDebugHandler handler, void * arg);

bool WebDebug_Init();
void WebDebug_Fini();
#endif // DAEDALUS_DEBUG_DISPLAYLIST
#endif // SYSOSX_DEBUG_WEBDEBUG_H_
