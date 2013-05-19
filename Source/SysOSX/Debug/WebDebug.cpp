#include "stdafx.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include "WebDebug.h"
#include "WebDebugTemplate.h"
#include <webby.h>

#ifdef DAEDALUS_W32
#include <winsock2.h>
#endif

#ifdef DAEDALUS_OSX
#include <unistd.h>
#endif

#include <string>
#include <vector>

#include "Debug/DBGConsole.h"
#include "Math/MathUtil.h"
#include "System/Paths.h"
#include "Utility/IO.h"
#include "Utility/Macros.h"
#include "Utility/StringUtil.h"
#include "Utility/Thread.h"


enum
{
	MAX_WSCONN = 8
};

struct WebDebugHandlerEntry
{
	const char * 	Request;
	WebDebugHandler	Handler;
	void *			Arg;
};

struct StaticResource
{
	std::string		Resource;
	std::string		FullPath;
};

static void *				gServerMemory = NULL;
static struct WebbyServer * gServer       = NULL;
static volatile bool		gKeepRunning  = false;
static ThreadHandle 		gThread       = kInvalidThreadHandle;

static int 									ws_connection_count;
static struct WebbyConnection *				ws_connections[MAX_WSCONN];
static std::vector<WebDebugHandlerEntry>	gHandlers;
static std::vector<StaticResource>			gStaticResources;

const char * const kApplicationJavascript = "application/javascript";
const char * const kApplicationJSON       = "application/json";
const char * const kImagePng              = "image/png";
const char * const kTextCSS               = "text/css";
const char * const kTextHTML              = "text/html";
const char * const kTextPlain             = "text/plain";

void WebDebug_Register(const char * request, WebDebugHandler handler, void * arg)
{
	WebDebugHandlerEntry entry = { request, handler, arg };
	gHandlers.push_back(entry);
}

static void test_log(const char* text)
{
	printf("[debug] %s\n", text);
	fflush(stdout);
}

WebDebugConnection::WebDebugConnection(WebbyConnection * connection)
:	mConnection(connection)
,	mState(kUnresponded)
,	mBytesExpected(0)
,	mBytesWritten(0)
{
	if (const char * params = connection->request.query_params)
	{
		std::vector<ConstStringRef> args;
		Split(params, '&', &args);

		mQueryParams.reserve(args.size());
		for (size_t i = 0; i < args.size(); ++i)
		{
			Param param;
			SplitAt(args[i], '=', &param.Key, &param.Value);

			mQueryParams.push_back(param);
		}
	}
}

const char * WebDebugConnection::GetQueryString() const
{
	return mConnection->request.query_params;
}

void WebDebugConnection::BeginResponse(int code, int content_length, const char * content_type)
{
	DAEDALUS_ASSERT(mState == kUnresponded, "Already responded to this request");

	mBytesExpected = content_length >= 0 ? content_length : 0;

	WebbyHeader headers[] = {
		{ "Content-Type", content_type },
	};

	WebbyBeginResponse(mConnection, code, content_length, headers, ARRAYSIZE(headers));
	mState = kResponding;
}

size_t WebDebugConnection::Write(const void * p, size_t len)
{
	mBytesWritten += len;
	return WebbyWrite(mConnection, p, len);
}

void WebDebugConnection::Flush()
{
	// Ignore this, it's not useful for network connections.
}

void WebDebugConnection::WriteString(const char * str)
{
	DAEDALUS_ASSERT(mState == kResponding, "Should be in Responding state");

	Write(str, strlen(str));
}

void WebDebugConnection::WriteF(const char * format, ...)
{
	static const u32 kBufferLen = 1024;
	char buffer[kBufferLen];

	va_list va;
	va_start(va, format);
	vsnprintf( buffer, kBufferLen, format, va );

	// This should be guaranteed...
	buffer[kBufferLen-1] = 0;
	va_end(va);

	Write(buffer, strlen(buffer));
}

void WebDebugConnection::EndResponse()
{
	DAEDALUS_ASSERT(mState == kResponding, "Should be in Responding state");
	DAEDALUS_ASSERT(mBytesExpected == 0 || mBytesWritten == mBytesExpected,
		"Promised %d bytes, but only wrote %d",
		mBytesExpected, mBytesWritten);

	WebbyEndResponse(mConnection);
	mState = kResponded;
}

void Generate404(WebDebugConnection * connection, const char * request)
{
	connection->BeginResponse(404, -1, kTextHTML );

	WriteStandardHeader(connection, "404 - Page Not Found");

	connection->WriteString(
		"<div class=\"container\">\n"
		"	<div class=\"row\">\n"
		"		<div class=\"span12\">\n"
	);
	connection->WriteString("<h1>404 - Page Not Found</h1>\n");
	connection->WriteF("<div>%s was not found. Boo.</div>", request);
	connection->WriteString(
		"		</div>\n"
		"	</div>\n"
		"</div>\n"
	);

	WriteStandardFooter(connection);
	connection->EndResponse();
}

void Generate500(WebDebugConnection * connection, const char * message)
{
	connection->BeginResponse(500, -1, kTextHTML );

	WriteStandardHeader(connection, "404 - Page Not Found");

	connection->WriteString(
		"<div class=\"container\">\n"
		"	<div class=\"row\">\n"
		"		<div class=\"span12\">\n"
	);
	connection->WriteString("<h1>500 - Epic Fail</h1>\n");
	connection->WriteF("<div>%s. Sad Panda :(</div>", message);
	connection->WriteString(
		"		</div>\n"
		"	</div>\n"
		"</div>\n"
	);

	WriteStandardFooter(connection);
	connection->EndResponse();
}

static const char * GetContentTypeForFilename(const char * filename)
{
	const char * ext = strrchr(filename, '.');
	if (ext)
	{
		if (strcmp(ext, ".js") == 0)	return kApplicationJavascript;
		if (strcmp(ext, ".css") == 0)	return kTextCSS;
		if (strcmp(ext, ".html") == 0)	return kTextHTML;
		if (strcmp(ext, ".png") == 0)	return kImagePng;
	}

	DBGConsole_Msg(0, "Unknown filetype [C%s]", filename);
	return kTextPlain;
}

static void ServeFile(WebDebugConnection * connection, const char * filename)
{
	FILE * fh = fopen(filename, "rb");
	if (!fh)
	{
		Generate404(connection, filename);
		return;
	}

	connection->BeginResponse(200, -1, GetContentTypeForFilename(filename));

	static const size_t kBufSize = 1024;
	size_t len_read;
	char buf[kBufSize];
	do
	{
		len_read = fread(buf, 1, kBufSize, fh);
		if (len_read > 0)
			connection->Write(buf, len_read);
	}
	while (len_read == kBufSize);

	connection->EndResponse();
	fclose(fh);
}

bool ServeResource(WebDebugConnection * connection, const char * resource_path)
{
	for (size_t i = 0; i < gStaticResources.size(); ++i)
	{
		const StaticResource & resource = gStaticResources[i];

		if (strcmp(resource_path, resource.Resource.c_str()) == 0)
		{
			ServeFile(connection, resource.FullPath.c_str());
			return true;
		}
	}

	return false;
}

static int WebDebugDispatch(struct WebbyConnection *connection)
{
	WebDebugConnection dbg_connection(connection);

	// Check dynamic handlers.
	for (size_t i = 0; i < gHandlers.size(); ++i)
	{
		const WebDebugHandlerEntry & entry = gHandlers[i];
		if (strcmp(connection->request.uri, entry.Request) == 0)
		{
			entry.Handler(entry.Arg, &dbg_connection);

			DAEDALUS_ASSERT(dbg_connection.GetState() == WebDebugConnection::kResponded, "Failed to handle the response");

			// Return success if we handled the connection.
			return 0;
		}
	}

	// Check static resources.
	if (ServeResource(&dbg_connection, connection->request.uri))
		return 0;

	DBGConsole_Msg(0, "404 [R%s]", connection->request.uri);
	Generate404(&dbg_connection, connection->request.uri);
	return 1;
}

static int test_ws_connect(struct WebbyConnection *connection)
{
	/* Allow websocket upgrades on /wstest */
	if (0 == strcmp(connection->request.uri, "/wstest") && ws_connection_count < MAX_WSCONN)
		return 0;
	else
		return 1;
}

static void test_ws_connected(struct WebbyConnection *connection)
{
	printf("WebSocket connected\n");
	ws_connections[ws_connection_count++] = connection;
}

static void test_ws_closed(struct WebbyConnection *connection)
{
	int i;
	printf("WebSocket closed\n");

	for (i = 0; i < ws_connection_count; i++)
	{
		if (ws_connections[i] == connection)
		{
			int remain = ws_connection_count - i;
			memmove(ws_connections + i, ws_connections + i + 1, remain * sizeof(struct WebbyConnection *));
			--ws_connection_count;
			break;
		}
	}
}

static int test_ws_frame(struct WebbyConnection *connection, const struct WebbyWsFrame *frame)
{
	int i = 0;

	printf("WebSocket frame incoming\n");
	printf("  Frame OpCode: %d\n", frame->opcode);
	printf("  Final frame?: %s\n", (frame->flags & WEBBY_WSF_FIN) ? "yes" : "no");
	printf("  Masked?     : %s\n", (frame->flags & WEBBY_WSF_MASKED) ? "yes" : "no");
	printf("  Data Length : %d\n", (int) frame->payload_length);

	while (i < frame->payload_length)
	{
		unsigned char buffer[16];
		int remain = frame->payload_length - i;
		size_t read_size = remain > sizeof buffer ? sizeof buffer : (size_t) remain;
		size_t k;

		printf("%08x ", (int) i);

		if (0 != WebbyRead(connection, buffer, read_size))
			break;

		for (k = 0; k < read_size; ++k)
			printf("%02x ", buffer[k]);

		for (k = read_size; k < 16; ++k)
			printf("   ");

		printf(" | ");

		for (k = 0; k < read_size; ++k)
			printf("%c", isprint(buffer[k]) ? buffer[k] : '?');

		printf("\n");

		i += read_size;
	}

	return 0;
}

static u32 DAEDALUS_THREAD_CALL_TYPE WebDebugThread(void * arg)
{
	WebbyServer * server = static_cast<WebbyServer*>(arg);
	int frame_counter = 0;

	while (gKeepRunning)
	{
		WebbyServerUpdate(server);

		/* Push some test data over websockets */
		// if (0 == (frame_counter & 0x7f))
		// {
		// 	for (int i = 0; i < ws_connection_count; ++i)
		// 	{
		// 		WebbyBeginSocketFrame(ws_connections[i], WEBBY_WS_OP_TEXT_FRAME);
		// 		WebbyPrintf(ws_connections[i], "Hello world over websockets!\n");
		// 		WebbyEndSocketFrame(ws_connections[i]);
		// 	}
		// }

		ThreadSleepMs(10);

		++frame_counter;
	}

	return 0;
}

static void AddStaticContent(const char * dir, const char * root)
{
	IO::FindHandleT find_handle;
	IO::FindDataT   find_data;

	if (IO::FindFileOpen(dir, &find_handle, find_data))
	{
		do
		{
			IO::Filename full_path;
			IO::Path::Combine(full_path, dir, find_data.Name);

			std::string resource_path = root;
			resource_path += '/';
			resource_path += find_data.Name;

			if (IO::Directory::IsDirectory(full_path))
			{
				AddStaticContent(full_path, resource_path.c_str());
			}
			else if (IO::File::Exists(full_path))
			{
				StaticResource resource;
				resource.Resource = resource_path;
				resource.FullPath = full_path;

				DBGConsole_Msg(0, " adding [M%s] -> [C%s]",
					resource.Resource.c_str(), resource.FullPath.c_str());

				gStaticResources.push_back(resource);
			}
		}
		while (IO::FindFileNext(find_handle, find_data));

		IO::FindFileClose(find_handle);
	}
}

bool WebDebug_Init()
{
#if defined(DAEDALUS_W32)
	{
		WORD wsa_version = MAKEWORD(2, 2);
		WSADATA wsa_data;
		if (0 != WSAStartup( wsa_version, &wsa_data ))
		{
			fprintf(stderr, "WSAStartup failed\n");
			return false;
		}
	}
#endif

	struct WebbyServerConfig config;
	memset(&config, 0, sizeof config);
	config.bind_address        = "127.0.0.1";
	config.listening_port      = 8081;
	config.flags               = WEBBY_SERVER_WEBSOCKETS;
	config.connection_max      = 16;		// Chrome and Firefox open lots of connections simultaneously.
	config.request_buffer_size = 2048;
	config.io_buffer_size      = 8192;
	config.dispatch            = &WebDebugDispatch;
	config.log                 = &test_log;
	config.ws_connect          = &test_ws_connect;
	config.ws_connected        = &test_ws_connected;
	config.ws_closed           = &test_ws_closed;
	config.ws_frame            = &test_ws_frame;

	if (0)
		config.flags = WEBBY_SERVER_LOG_DEBUG;

	int memory_size = WebbyServerMemoryNeeded(&config);
	gServerMemory = malloc(memory_size);
	gServer = WebbyServerInit(&config, gServerMemory, memory_size);

	if (!gServer)
	{
		fprintf(stderr, "failed to init server\n");
		return false;
	}

	IO::Filename data_path;
	IO::Path::Combine(data_path, gDaedalusExePath, "Web");
	DBGConsole_Msg(0, "Looking for static resource in [C%s]", data_path);
	AddStaticContent(data_path, "");

	gKeepRunning = true;
	gThread      = CreateThread( "WebDebug", &WebDebugThread, gServer );

	return true;
}

void WebDebug_Fini()
{
	if (gThread != kInvalidThreadHandle)
	{
		gKeepRunning = false;
		JoinThread(gThread, -1);
		gThread = kInvalidThreadHandle;
	}

	WebbyServerShutdown(gServer);
	free(gServerMemory);

#if defined(DAEDALUS_W32)
	WSACleanup();
#endif
}
#endif //DAEDALUS_DEBUG_DISPLAYLIST
