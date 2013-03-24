#include "stdafx.h"
#include "WebDebug.h"
#include "WebDebugTemplate.h"
#include <webby.h>

#ifdef DAEDALUS_W32
#include <winsock2.h>
#endif

#ifdef DAEDALUS_OSX
#include <unistd.h>
#endif

#include <vector>

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

static void *				gServerMemory = NULL;
static struct WebbyServer * gServer       = NULL;
static volatile bool		gKeepRunning  = false;
static ThreadHandle 		gThread       = kInvalidThreadHandle;

static int 									ws_connection_count;
static struct WebbyConnection *				ws_connections[MAX_WSCONN];
static std::vector<WebDebugHandlerEntry>	gHandlers;

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
{
}

const char * WebDebugConnection::GetQueryParams() const
{
	return mConnection->request.query_params;
}

void WebDebugConnection::BeginResponse(int code, int content_length, const char * content_type)
{
	DAEDALUS_ASSERT(mState == kUnresponded, "Already responded to this request");

	WebbyHeader headers[] = {
		{ "Content-Type", content_type },
	};

	WebbyBeginResponse(mConnection, code, content_length, headers, ARRAYSIZE(headers));
	mState = kResponding;
}

size_t WebDebugConnection::Write(const void * p, size_t len)
{
	return WebbyWrite(mConnection, p, len);
}

void WebDebugConnection::Flush()
{
	// Ignore this, it's not useful for network connections.
}

void WebDebugConnection::WriteString(const char * str)
{
	DAEDALUS_ASSERT(mState == kResponding, "Should be in Responding state");

	WebbyWrite(mConnection, str, strlen(str));
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

	WebbyWrite(mConnection, buffer, strlen(buffer));
}

void WebDebugConnection::EndResponse()
{
	DAEDALUS_ASSERT(mState == kResponding, "Should be in Responding state");
	WebbyEndResponse(mConnection);
	mState = kResponded;
}

static void Generate404(WebDebugConnection * connection, const char * request)
{
	connection->BeginResponse(404, -1, "text/html" );

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

static int WebDebugDispatch(struct WebbyConnection *connection)
{
	for (size_t i = 0; i < gHandlers.size(); ++i)
	{
		const WebDebugHandlerEntry & entry = gHandlers[i];
		if (strcmp(connection->request.uri, entry.Request) == 0)
		{
			WebDebugConnection dbg_connection(connection);

			entry.Handler(entry.Arg, &dbg_connection);

			DAEDALUS_ASSERT(dbg_connection.GetState() != WebDebugConnection::kResponding, "Failed to call EndResponse");

			// Return success if we handled the connection.
			return dbg_connection.GetState() == WebDebugConnection::kResponded;
		}
	}

	WebDebugConnection dbg_connection(connection);
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

static u32 WebDebugThread(void * arg)
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

		ThreadSleepMs(30);

		++frame_counter;
	}

	return 0;
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
	config.connection_max      = 4;
	config.request_buffer_size = 2048;
	config.io_buffer_size      = 8192;
	config.dispatch            = &WebDebugDispatch;
	config.log                 = &test_log;
	config.ws_connect          = &test_ws_connect;
	config.ws_connected        = &test_ws_connected;
	config.ws_closed           = &test_ws_closed;
	config.ws_frame            = &test_ws_frame;

	int memory_size = WebbyServerMemoryNeeded(&config);
	gServerMemory = malloc(memory_size);
	gServer = WebbyServerInit(&config, gServerMemory, memory_size);

	if (!gServer)
	{
		fprintf(stderr, "failed to init server\n");
		return false;
	}

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
