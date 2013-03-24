
#include <winsock2.h>

typedef SOCKET webby_socket_t;

#define WB_ALIGN(x) __declspec(align(x))
#define WB_INVALID_SOCKET INVALID_SOCKET
#define snprintf _snprintf

typedef int webby_socklen_t;

static int wb_socket_error(void)
{
  return WSAGetLastError();
}

static int strcasecmp(const char *a, const char *b)
{
  return _stricmp(a, b);
}

static int strncasecmp(const char *a, const char *b, size_t len)
{
  return _strnicmp(a, b, len);
}

static int wb_set_blocking(webby_socket_t socket, int blocking)
{
  u_long val = !blocking;
  return ioctlsocket(socket, FIONBIO, &val);
}

static int wb_valid_socket(webby_socket_t socket)
{
  return INVALID_SOCKET != socket;
}

static void wb_close_socket(webby_socket_t socket)
{
  closesocket(socket);
}

static int wb_is_blocking_error(int error)
{
  return WSAEWOULDBLOCK == error;
}
