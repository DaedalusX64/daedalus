#include "stdafx.h"
#include <webby.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef DAEDALUS_W32
#include <winsock2.h>
#endif

#ifdef DAEDALUS_OSX
#include <unistd.h>
#endif

#include "Utility/Thread.h"

static int quit = 0;

enum
{
	MAX_WSCONN = 8
};

static int ws_connection_count;
static struct WebbyConnection *ws_connections[MAX_WSCONN];

static void test_log(const char* text)
{
	printf("[debug] %s\n", text);
}

static int test_dispatch(struct WebbyConnection *connection)
{
	if (0 == strcmp("/foo", connection->request.uri))
	{
		WebbyBeginResponse(connection, 200, 14, NULL, 0);
		WebbyWrite(connection, "Hello, world!\n", 14);
		WebbyEndResponse(connection);
		return 0;
	}
	else if (0 == strcmp("/bar", connection->request.uri))
	{
		WebbyBeginResponse(connection, 200, -1, NULL, 0);
		WebbyWrite(connection, "Hello, world!\n", 14);
		WebbyWrite(connection, "Hello, world?\n", 14);
		WebbyEndResponse(connection);
		return 0;
	}
	else if (0 == strcmp(connection->request.uri, "/quit"))
	{
		WebbyBeginResponse(connection, 200, -1, NULL, 0);
		WebbyPrintf(connection, "Goodbye, cruel world\n");
		WebbyEndResponse(connection);
		quit = 1;
		return 0;
	}
	else
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

static void *				gServerMemory = NULL;
static struct WebbyServer * gServer       = NULL;
static volatile bool		gKeepRunning  = false;
static ThreadHandle 		gThread       = kInvalidThreadHandle;

static u32 WebDebugThread(void * arg)
{
	WebbyServer * server = static_cast<WebbyServer*>(arg);
	int frame_counter = 0;

	while (gKeepRunning)
	{
		WebbyServerUpdate(server);

		/* Push some test data over websockets */
		if (0 == (frame_counter & 0x7f))
		{
			for (int i = 0; i < ws_connection_count; ++i)
			{
				WebbyBeginSocketFrame(ws_connections[i], WEBBY_WS_OP_TEXT_FRAME);
				WebbyPrintf(ws_connections[i], "Hello world over websockets!\n");
				WebbyEndSocketFrame(ws_connections[i]);
			}
		}

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
	config.dispatch            = &test_dispatch;
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
