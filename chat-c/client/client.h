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
#define SERVICE_MULTICAST_IP "239.255.255.250"
#define SERVICE_MULTICAST_PORT "12345"

typedef struct holdBackQueue
{
    int id;
    int clk_index;
    char content[BUFFER_SIZE];
    int dependency[MAX_GROUP_SIZE];
    struct holdBackQueue *next;
} HoldBackQueue;

extern char message[BUFFER_SIZE];
extern char Buffer[BUFFER_SIZE];

SOCKET connect_toserver(const char *host, const char *port);
HoldBackQueue * handle_group_receive(SOCKET group_socket, HoldBackQueue *head);
char * clear_message(char *not_clean_message);
SOCKET group_multicast(SOCKET *mc_socket, char *multicast_ip, char *multicast_port, char * msg);
SOCKET join_multicast(char *multicast_ip, char * mPORT);
int array_compare(int *arr1, int *arr2);
HoldBackQueue * deliver_messages(HoldBackQueue *head, char *clean_message);
char * get_service_info(const char *host, const char *port, const char *device_ip);
// Gettting service info from multicast or tcp
char * get_service_info_mcast(const char *host, const char *port, const char *device_ip, const char *device_port);
SOCKET get_tcp_socket(char *address, char *port);

void append_to_holdback_queue(HoldBackQueue **head, char *clean_message);
void print_holdback_queue(HoldBackQueue *head);
void remove_from_holdback_queue(HoldBackQueue **head, int clk_index);
void free_holdback_queue(HoldBackQueue *head);
void get_deps_str(int *int_array, int size, char *str_array);
void get_deps_int(char *str_array, int *int_array);



#endif // CLIENT_H