#ifndef CLIENT_H
#define CLIENT_H


#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#endif

#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())
#define error_return INVALID_SOCKET
#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#define error_return -1

#endif


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 256
#define MAX_GROUP_SIZE 10

SOCKET connect_toserver(const char *host, const char *port);
void handle_group_receive(SOCKET group_socket, int self_id);
char * clear_message(char *not_clean_message);
SOCKET group_multicast(SOCKET *mc_socket, char *multicast_ip, char * msg);
SOCKET join_multicast(char *multicast_ip, char * mPORT);

#endif // CLIENT_H