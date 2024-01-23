#ifndef SERVER_S_H
#define SERVER_S_H


#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
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

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif





#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define PORT "4041"
#define MAX_CONNECTION 10
#define BUFFER_SIZE 4096
#define SERVER_IP "192.168.0.101"
#define NEXT_SERVER_IP "127.0.0.1"
#define NEXT_SERVER_PORT "6970"
#define BROADCAST_ADDRESS "192.168.0.255"
#define BROADCAST_PORT "3938"
#define MULTICAST_IP "239.255.255.250"
#define MULTICAST_PORT "12345"

typedef struct serverInfo {
    int ID;
    char addr[16];
    int port;
	int leader;
	SOCKET tcp_socket;
	struct serverInfo *next;
} ServerInfo;

struct ClientInfo {
	int id;
	SOCKET socket;
	void *addr;
	struct sockaddr_storage address;
};

void send_server_info(SOCKET dest, ServerInfo *myInfo);
SOCKET setup_tcp_socket();
void assign_client_info(SOCKET socket_client, struct sockaddr_storage client_address, int temp);
SOCKET setup_udp_socket(char * sock_ip, char *sock_port);
int leader_election();
void udp_multicast(char *msg, struct serverInfo *head, SOCKET udp_sockfd);
void udp_broadcast(char *msg, SOCKET udp_sockfd);
void handle_udp_recieve(ServerInfo *connected_peers, int leader, SOCKET udp_socket);
SOCKET join_multicast(char *multicast_ip, char * mPORT);
SOCKET do_multicast(SOCKET *mc_socket, char *multicast_ip, char * msg);
SOCKET service_discovery(SOCKET *mc_socket, SOCKET *successor_socket, SOCKET tcp_socket, struct serverInfo *head);
SOCKET handle_mcast_receive(SOCKET mc_socket, struct serverInfo * connected_peers);
SOCKET peer_mcast_receive(struct serverInfo * connected_peers, char *buf, struct sockaddr_in sender_addr);
SOCKET setup_tcp_client(char *address, char *port);
void handle_disconnection(struct serverInfo * head, SOCKET i, SOCKET udp_socket, SOCKET mc_socket, SOCKET ltcp_socket, SOCKET successor_socket);
SOCKET get_last_peer_socket(struct serverInfo *head);
void append_server_sorted(struct serverInfo **head, int id, void *address, int port, int leader, SOCKET tcp_socket);
void send_ele_msg(ServerInfo *head, SOCKET mc_socket);


// Data structure of servers to keep
int server_info_exist(int id, struct serverInfo *head);
struct serverInfo * create_server(int id, void *address, int port, int leader, SOCKET tcp_socket);
int delete_server(struct serverInfo *head, int id);
void append_server(struct serverInfo **head, int id, void *address, int port, int leader, SOCKET tcp_socket);
void display_server(struct serverInfo *head);
void free_server_storage(struct serverInfo *head);
int ist_peer_server(int id, struct serverInfo *head);

#endif 
