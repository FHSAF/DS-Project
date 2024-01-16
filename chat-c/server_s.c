#include "../chat-c/server_s.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define PORT "4041"
#define MAX_CONNECTION 10
#define BUFFER_SIZE 4096
#define SERVER_IP "192.168.0.101"
#define NEXT_SERVER_IP "127.0.0.1"
#define NEXT_SERVER_PORT "6970"
#define BROADCAST_ADDRESS "192.168.0.255"
#define BROADCAST_PORT "3938"

typedef struct serverInfo {
    int ID;
    char *addr;
    int port;
	int leader;
	struct serverInfo *next;
} ServerInfo;

SOCKET next_server();
void send_server_info(SOCKET dest, ServerInfo *myInfo);
SOCKET setup_tcp_socket();
void assign_client_info(SOCKET socket_client, struct sockaddr_storage client_address);
SOCKET setup_udp_socket(char * sock_ip, char *sock_port);
int leader_election();
void udp_multicast(char *msg, struct serverInfo *head, SOCKET udp_sockfd);
void udp_broadcast(char *msg, SOCKET udp_sockfd);
void handle_udp_recieve(ServerInfo *connected_peers, int leader, SOCKET udp_socket);

// Data structure of servers to keep
int server_info_exist(char *ip_addr, int port, struct serverInfo *head);
struct serverInfo * create_server(int id, void *address, int port, int leader);
int delete_server(char *ip_addr, struct serverInfo *head);
void append_server(struct serverInfo **head, int id, void *address, int port, int leader);
void display_server(struct serverInfo *head);
void free_server_storage(struct serverInfo *head);


struct ClientInfo {
	int id;
	SOCKET socket;
	void *addr;
	struct sockaddr_storage address;
};

struct ClientInfo clients[MAX_CONNECTION];
int client_count = 0;

int getRadomId(int min, int max) {

    srand((unsigned int)time(NULL));
    int randomId = rand() % (max - min + 1) + min;

    return randomId;
}

int main()
{
	#if defined(_WIN32)
		WSADATA d;
		if (WSAStartup(MAKEWORD(2, 2), &d))
		{
			fprintf(stderr, "Failed to initia");
			return (1);
		}
	#endif
    time_t start_t, end_t;
	int leader = 1;
	SOCKET socket_max;
	fd_set master;

    SOCKET socket_listen = setup_tcp_socket();
    if (socket_listen == -1)
        return(1);

    struct serverInfo *connected_peers = NULL;
	append_server(&connected_peers, getRadomId(1000, 100000), (void *)SERVER_IP, atoi(PORT), leader);

	printf("[main] setting up select...\n");
	FD_ZERO(&master);
	FD_SET(socket_listen, &master);
	socket_max = socket_listen;

	SOCKET udp_socket = setup_udp_socket(NULL, BROADCAST_PORT);
	if (udp_socket!=-1)
	{
		FD_SET(udp_socket, &master);
		socket_max = udp_socket;
	}
	// connecto to next server
    printf("connecting to next server...\n");
    SOCKET next_server_socket = next_server();
	if (next_server_socket!=-1)
	{
		FD_SET(next_server_socket, &master);
		socket_max = next_server_socket;
       	send_server_info(next_server_socket, connected_peers);
	}

    printf("Waiting for connections...\n");
    time(&start_t);
	while (1)
	{
		fd_set reads;
		reads = master;

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
		if (select(socket_max + 1, &reads, 0, 0, &timeout) < 0)
		{
			fprintf(stderr, "[main] select() failed. (%d)\n", GETSOCKETERRNO());
			return (1);
		}

        time(&end_t);
		char msg1[12];
		sprintf(msg1, "<%d>(OK)", connected_peers->ID);
		char *msg2 = "NPeer(Ok)";
        if ((int)difftime(end_t, start_t) == 5)
        {
			if (leader == 1)
				udp_multicast(msg1, connected_peers, udp_socket);
			else
            	udp_multicast(msg2, connected_peers, udp_socket);
		
            printf("%d \n", (int)difftime(end_t, start_t));
            time(&start_t);
        }

		SOCKET i;
		for (i = 1; i <= socket_max; ++i)
		{
			if (FD_ISSET(i, &reads)) 
			{
				if (i == socket_listen)
				{
					struct sockaddr_storage client_address;
					socklen_t client_len = sizeof(client_address);
					SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
					if (!ISVALIDSOCKET(socket_client))
					{
						fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
						return (1);
					}

					FD_SET(socket_client, &master);
					if (socket_client > socket_max)
					{
						socket_max = socket_client;
					}
					char address_buffer[100];
					getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
					printf("New connection from %s\n", address_buffer);
					assign_client_info(socket_client, client_address);
				} else if (i == udp_socket)
				{
					printf("[main] read on udpsocket...\n");
					handle_udp_recieve(connected_peers, leader, i);
				}
                else 
                {
					char read[4096];
					int byte_received = recv(i, read, 4096, 0);
					if (byte_received < 1)
					{
						//
						for (int ci = 0; ci <= client_count; ++ci)
						{
							if (clients[ci].socket == i)
							{
								printf("Client with (%d) disconnected.\n", clients[ci].id);
								free(clients[ci].addr);
								// printf("freed %p \n", clients[ci].addr);
								for (int cj = ci; cj < client_count - 1; ++cj)
								{
									clients[cj] = clients[cj + 1];
								}
								client_count--;
								break;
							}
						}
						//
						FD_CLR(i, &master);
						CLOSESOCKET(i);
						continue;
					}
					
					int dest_id;
					char message[4096];
					read[byte_received] = '\0';
					if (sscanf(read, "%d %[^\n]", &dest_id, message) == 2)
					{
						printf("message (%s) (%lu), (%d)\n", read, sizeof(read), byte_received);
						struct ClientInfo *dest_client = NULL;
						for (int ci = 0; ci < client_count; ++ci)
						{
							if (clients[ci].id == dest_id)
							{
								dest_client = &clients[ci];
            					printf("Received from ID (%d), forwarding to ID (%d).\n", i, dest_client->id);
								break;
							}
						}

						if (dest_client != NULL)
						{
							int bytes_sent = send(dest_client->socket, read, byte_received, 0);
            				printf("Sent %d bytes to (%d)\n", bytes_sent, dest_client->id);
						} else if (i == next_server_socket)
                        {
                            printf("From peer server: {%s}\n", read);
                        }
                         else {
							printf("Client not found (%d).\n", dest_id);
						}
					} else {

						SOCKET j;
						for (j = 1; j <= socket_max; ++j)
						{
							if (FD_ISSET(j, &master)) {
								if (j == socket_listen || j == i)
									continue;
								else
									send(j, read, byte_received, 0);
							}
						}
					}
				}
			}
		}
	}
	printf("Closing listening socket...\n");
	CLOSESOCKET(next_server_socket);
    CLOSESOCKET(socket_listen);
	CLOSESOCKET(udp_socket);
    free(connected_peers);
	free_server_storage(connected_peers);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}

void assign_client_info(SOCKET socket_client, struct sockaddr_storage client_address)
{
	
	//
	int client_id = getRadomId(10, 1000)*(++client_count);
	printf("Client id is (%d).\n", client_id);
	struct ClientInfo *client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
	// printf("Malloced %p \n", client_info);
	client_info->id = client_id;
	client_info->socket = socket_client;
	client_info->address = client_address;
	client_info->addr = client_info;
	clients[client_count - 1] = *client_info;
	send(socket_client, (void *)&client_id, sizeof(client_id), 0);
	printf("Assigned ID (%d) to (%d)\n", client_id, client_count);
}
SOCKET next_server()
{
	printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(NEXT_SERVER_IP, NEXT_SERVER_PORT, &hints, &peer_address)) {
        fprintf(stderr, "[next_server] getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }


    printf("[next_server] Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);


    printf("[next_server] Creating socket...\n");

    SOCKET peer_socket = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(peer_socket)) {
        fprintf(stderr, "[next_server] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }


    printf("[next_server] Connecting...\n");
    if (connect(peer_socket,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "[next_server] connect() failed. (%d)\n", GETSOCKETERRNO());
		CLOSESOCKET(peer_socket);
        return (-1);
    }
	freeaddrinfo(peer_address);
	return (peer_socket);
}

void send_server_info(SOCKET dest, ServerInfo *myInfo)
{
    char infoBuf[4096];
    int sendBytes;

    sprintf(infoBuf, "My address {%s}.\nMy ID {%d}.\nMy Port {%d}.\n", myInfo->addr, myInfo->ID, 
    myInfo->port);
    printf("My address {%s}.\nMy ID {%d}.\nMy Port {%d}.\n", myInfo->addr, myInfo->ID, 
    myInfo->port);
    sendBytes = strlen(infoBuf);
    send(dest, infoBuf, sendBytes, 0);

}


SOCKET setup_tcp_socket()
{
	printf("[TCP] Configuring local address...\n");
	SOCKET socket_listen;
	struct addrinfo *bind_address;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(SERVER_IP, PORT, &hints, &bind_address);

    printf("[TCP] Creating socket...\n");
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
	{
		fprintf(stderr, "[TCP] socket() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	
    printf("[TCP] Binding socket to local address...\n");
	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
	{
		fprintf(stderr, "[TCP] bind() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	freeaddrinfo(bind_address);

    printf("[TCP] Listening...\n");
	if (listen(socket_listen, 10))
	{
		fprintf(stderr, "[TCP] listen() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}

	return (socket_listen);
}

SOCKET setup_udp_socket(char * sock_ip, char *sock_port)
{
	printf("[UDP] Configuring local address...\n");
    struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo *udp_bind_address;
    
    getaddrinfo(sock_ip, sock_port, &hints, &udp_bind_address);

    printf("[UDP] Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(udp_bind_address->ai_family, udp_bind_address->ai_socktype, udp_bind_address->ai_protocol);
    if (!(ISVALIDSOCKET(socket_listen)))
    {
        fprintf(stderr, "[UDP] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }

    #if defined(_WIN32)
    char broadcastEnable = '1';
    #else
    int broadcastEnable = 1;
    #endif
	printf("[UDP] enabling broadcast...\n");
    if (setsockopt(socket_listen, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1)
    {
        fprintf(stderr, "[UDP] setsocktopt() failed. (%d)\n", GETSOCKETERRNO());
		CLOSESOCKET(socket_listen);
        return (-1);
    }

    printf("[UDP] Binding socket to local address...\n");
	if (bind(socket_listen, udp_bind_address->ai_addr, udp_bind_address->ai_addrlen))
	{
		fprintf(stderr, "[UDP] bind() failed. (%d)\n", GETSOCKETERRNO());
		CLOSESOCKET(socket_listen);
		return (-1);
	}
	free(udp_bind_address);
	return(socket_listen);
}
void udp_multicast(char *msg, struct serverInfo *head, SOCKET udp_sockfd)
{
	printf("[udp_multicast] multicasting (%d)...\n", head->ID);
	udp_broadcast(msg, udp_sockfd);
}

void udp_broadcast(char *msg, SOCKET udp_sockfd)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo *rcv_addr;
    
	// SOCKET brd_socket = setup_udp_socket(SERVER_IP, BROADCAST_PORT);

	getaddrinfo(BROADCAST_ADDRESS, BROADCAST_PORT, &hints, &rcv_addr);
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(rcv_addr->ai_addr,
			rcv_addr->ai_addrlen,
			address_buffer, sizeof(address_buffer),
			service_buffer, sizeof(service_buffer),
			NI_NUMERICHOST | NI_NUMERICSERV);
	printf("[udp_broadcast] address: %s %s\n", address_buffer, service_buffer);
	if (sendto(udp_sockfd, msg, strlen(msg), 0, rcv_addr->ai_addr, rcv_addr->ai_addrlen) == -1)
		fprintf(stderr, "[udp_broadcast] sendto() failed. (%d)\n", GETSOCKETERRNO());
	free(rcv_addr);

}

void handle_udp_recieve(ServerInfo *connected_peers, int leader, SOCKET udp_socket)
{
	char read[1024];
	struct sockaddr_storage sender_address;
	socklen_t sender_len = sizeof(sender_address);
	int bytes_received = recvfrom(udp_socket, read, 1024, 0, 
				(struct sockaddr *)&sender_address, &sender_len);
	printf("[handle_udp] sender_len %d\n", sender_len);
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(((struct sockaddr *)&sender_address),
			sender_len,
			address_buffer, sizeof(address_buffer),
			service_buffer, sizeof(service_buffer),
			NI_NUMERICHOST | NI_NUMERICSERV);
	if (strncmp(address_buffer, connected_peers->addr, (size_t)sender_len) != 0)
	{
		if (leader == 1)
		{
			if (bytes_received < 1)
			{
				printf("[Leader UDP] {%s:%s} disconnected.\n", address_buffer, service_buffer);
				delete_server(address_buffer, connected_peers);
			}
			else
			{
				printf("[Leader UDP] Received: (%s) (%d) bytes: %.*s\n", address_buffer, bytes_received, bytes_received, read);
				server_info_exist(address_buffer, atoi(service_buffer), connected_peers);
			}
			
		} else {
			if (bytes_received < 1)
			{
				printf("[NPeer UDP] {%s:%s} disconnected.\n", address_buffer, service_buffer);
				delete_server(address_buffer, connected_peers);
			}
			else
			{
				printf("[NPeer UDP] Received: (%d) bytes: %.*s\n", bytes_received, bytes_received, read);
				server_info_exist(address_buffer, atoi(service_buffer), connected_peers);
			}

		}
	}
}

int server_info_exist(char *ip_addr, int port, struct serverInfo *head)
{
	struct serverInfo * current = head;
	while (current!= NULL){
		if (current->addr == ip_addr)
			return (1);
		else
			current = current->next;
	}
	append_server(&head, getRadomId(1000, 1000000), ip_addr, port, 0);
	return (1);
}

struct serverInfo * create_server(int id, void *address, int port, int leader)
{
	struct serverInfo* server_info = (struct serverInfo *)malloc(sizeof(struct serverInfo));
	if (server_info == NULL)
	{
		printf("Memory allocation failed\n");
		exit(1);
	}
	server_info->addr = address;
	server_info->ID = id;
	server_info->port = port;
	server_info->leader = leader;
	server_info->next = NULL;
	
	return (server_info);
}

void append_server(struct serverInfo **head, int id, void *address, int port, int leader)
{
	struct serverInfo * new_server = create_server(id, address, port, leader);
	printf("[append] new sever created...\n");
	if (*head == NULL)
		*head = new_server;
	else{
		struct serverInfo * current = *head;
		while (current->next != NULL)
			current = current->next;
		current->next = new_server;
	}
	
	display_server(*head);
}

int delete_server(char *ip_addr, struct serverInfo *head)
{
	struct serverInfo * current = head;
	struct serverInfo * temp = NULL;
	while (current != NULL){
		if (current->next->addr == ip_addr)
		{
			temp = current->next;
			current->next = current->next->next;
			free(temp);
			return (1);
		}
		else
			current = current->next;
	}

	return (0);
}

void display_server(struct serverInfo *head)
{
	struct serverInfo *current = head;
	printf("\n=================Servers I know=================\n");
	while (current != NULL)
	{
		printf("\tID: %d, IP: %s:%d, Leader: %d\n", current->ID, current->addr, current->port, current->leader);
		current = current->next;
	}
	printf("=================Servers I know=================\n\n");
}

void free_server_storage(struct serverInfo *head)
{
	struct serverInfo *current = head;
	struct serverInfo *nextServer;
	while (current != NULL)
	{
		nextServer = current->next;
		free(current);
		current = nextServer;
	}
}

// end