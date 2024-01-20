#include "../chat-c/server_s.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define PORT "4041"
#define MAX_CONNECTION 10
#define BUFFER_SIZE 4096
#define SERVER_IP "192.168.0.100"
#define NEXT_SERVER_IP "127.0.0.1"
#define NEXT_SERVER_PORT "6970"
#define BROADCAST_ADDRESS "192.168.0.255"
#define BROADCAST_PORT "3938"
#define MULTICAST_IP "239.255.255.250"
#define MULTICAST_PORT "12345"

typedef struct serverInfo {
    int ID;
    char *addr;
    int port;
	int leader;
	SOCKET tcp_socket;
	struct serverInfo *next;
} ServerInfo;

void send_server_info(SOCKET dest, ServerInfo *myInfo);
SOCKET setup_tcp_socket();
void assign_client_info(SOCKET socket_client, struct sockaddr_storage client_address, int temp);
SOCKET setup_udp_socket(char * sock_ip, char *sock_port);
int leader_election();
void udp_multicast(char *msg, struct serverInfo *head, SOCKET udp_sockfd);
void udp_broadcast(char *msg, SOCKET udp_sockfd);
void handle_udp_recieve(ServerInfo *connected_peers, int leader, SOCKET udp_socket);
SOCKET join_multicast(char *multicast_ip, char * mPORT);
int do_multicast(SOCKET *mc_socket, char *multicast_ip, char * msg);
SOCKET service_discovery(SOCKET *mc_socket, SOCKET tcp_socket, struct serverInfo *head);
SOCKET handle_mcast_receive(SOCKET mc_socket, struct serverInfo * connected_peers);
SOCKET peer_mcast_receive(struct serverInfo * connected_peers, char *buf, struct sockaddr_in sender_addr);
SOCKET setup_tcp_client(char *address, char *port);
void handle_disconnection(struct serverInfo * head, SOCKET i, SOCKET udp_socket, SOCKET mc_socket, SOCKET ltcp_socket);

// Data structure of servers to keep
int server_info_exist(int id, struct serverInfo *head);
struct serverInfo * create_server(int id, void *address, int port, int leader, SOCKET tcp_socket);
int delete_server(struct serverInfo *head, SOCKET tcp_socket);
void append_server(struct serverInfo **head, int id, void *address, int port, int leader, SOCKET tcp_socket);
void display_server(struct serverInfo *head);
void free_server_storage(struct serverInfo *head);
int ist_peer_server(int id, struct serverInfo *head);


struct ClientInfo {
	int id;
	SOCKET socket;
	void *addr;
	struct sockaddr_storage address;
};

struct ClientInfo clients[MAX_CONNECTION];
struct ClientInfo *tempClient;
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
	SOCKET ltcp_socket;
	fd_set master;

    SOCKET socket_listen = setup_tcp_socket();
    if (socket_listen == -1)
        return(1);

    struct serverInfo *connected_peers = NULL;
	append_server(&connected_peers, getRadomId(1000, 100000), (void *)SERVER_IP, atoi(PORT), leader, socket_listen);

	printf("[main] setting up select...\n");
	FD_ZERO(&master);
	FD_SET(socket_listen, &master);
	socket_max = socket_listen;

	SOCKET udp_socket = -100; //setup_udp_socket(NULL, BROADCAST_PORT);
	// if (udp_socket!=-1)
	// {
	// 	FD_SET(udp_socket, &master);
	// 	if (udp_socket > socket_max)
	// 		socket_max = udp_socket;
	// }

	// mc_socket the socket receiving multicast messages
	SOCKET mc_socket = join_multicast(MULTICAST_IP, MULTICAST_PORT);
	
	if (mc_socket == -1)
		return (1);
	
	ltcp_socket = service_discovery(&mc_socket, socket_listen, connected_peers);
	if (ltcp_socket != -1) {
		
		FD_SET(ltcp_socket, &master);
		if (ltcp_socket > socket_max)
			socket_max = ltcp_socket;
		connected_peers->leader = 0;
	} else {
		printf("[main] I am the leader.\n");
	}

	FD_SET(mc_socket, &master);
	if (mc_socket > socket_max)
		socket_max = mc_socket;

	printf("[main] Waiting for TCP connections...\n");

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
		sprintf(msg1, "%d", connected_peers->ID);
        if ((int)difftime(end_t, start_t) == 10)
        {
			FD_CLR(mc_socket, &master);
			if (connected_peers->leader == 1)
				do_multicast(&mc_socket, MULTICAST_IP, msg1);
			else
            	do_multicast(&mc_socket, MULTICAST_IP, msg1);
			if (mc_socket > socket_max)
				socket_max = mc_socket;
			FD_SET(mc_socket, &master);
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
					assign_client_info(socket_client, client_address, 1);
				} else if (i == mc_socket) {
					SOCKET peer_socket = handle_mcast_receive(mc_socket, connected_peers);
					if (peer_socket > 0){
						printf("[main] The peer socket (%d)...\n", peer_socket);
						FD_SET(peer_socket, &master);
					}
				} else {
					char read[4096];
					int byte_received = recv(i, read, 4096, 0);
					if (byte_received < 1)
					{
						handle_disconnection(connected_peers, i, udp_socket, mc_socket, ltcp_socket);
						FD_CLR(i, &master);
						CLOSESOCKET(i);
						continue;
					}
					if (i == udp_socket)
					{
						printf("[main] read on udpsocket...\n");
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
						} else {
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
    CLOSESOCKET(socket_listen);
	CLOSESOCKET(udp_socket);
	CLOSESOCKET(mc_socket);
    free(connected_peers);
	free_server_storage(connected_peers);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}

void assign_client_info(SOCKET socket_client, struct sockaddr_storage client_address, int temp)
{
	
	if (client_count == MAX_CONNECTION)
	{
		printf("Client limit reached. Rejecting client.\n");
		CLOSESOCKET(socket_client);
		return;
	}
	int client_id = getRadomId(10, 1000)*(++client_count);
	struct ClientInfo *client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
	if (client_info == NULL)
	{
		printf("[]Memory allocation failed\n");
		exit(1);
	}
	printf("Client id is (%d).\n", client_id);
	client_info->id = client_id;
	client_info->socket = socket_client;
	client_info->address = client_address;
	client_info->addr = client_info;
	if (temp == 0)
	{
		clients[client_count - 1] = *client_info;
		send(socket_client, (void *)&client_id, sizeof(client_id), 0);
		printf("Assigned ID (%d) to (%d)\n", client_id, client_count);
	} else {
		tempClient = client_info;
	}
}

void send_server_info(SOCKET dest, ServerInfo *myInfo)
{
    char infoBuf[4096];
    int sendBytes;

    sprintf(infoBuf, "My address {%s}.\nMy ID {%d}.\nMy Port {%d}.\n", myInfo->addr, myInfo->ID, 
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
	int broadcast_loop = 0;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast_loop, sizeof(broadcast_loop)) < 0) {
		fprintf(stderr, "[UDP] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}

    printf("[UDP] Binding socket to local address...\n");
	if (bind(socket_listen, udp_bind_address->ai_addr, udp_bind_address->ai_addrlen))
	{
		fprintf(stderr, "[UDP] bind() failed. (%d)\n", GETSOCKETERRNO());
		CLOSESOCKET(socket_listen);
		return (-1);
	}
	freeaddrinfo(udp_bind_address);
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
				delete_server(connected_peers, 0);
			}
			else
			{
				int ID, leader, mPORT;
				char multicastIP[16];

				if (sscanf(read, "%d:%15[^:]:%d:%d", &ID, multicastIP, &mPORT, &leader) == 4)
				{
					printf("[Leader UDP] Received: (%s) (%d) bytes: %.*s\n", address_buffer, bytes_received, bytes_received, read);
					join_multicast(multicastIP, MULTICAST_PORT);
				} else {
					printf("[Leader UDP] Received: (%s) (%d) bytes: %.*s\n", address_buffer, bytes_received, bytes_received, read);
				}
			}
			
		} else {
			if (bytes_received < 1)
			{
				printf("[NPeer UDP] {%s:%s} disconnected.\n", address_buffer, service_buffer);
			}
			else
			{
				printf("[NPeer UDP] Received: (%d) bytes: %.*s\n", bytes_received, bytes_received, read);
			}

		}
	}
}

int server_info_exist(int id, struct serverInfo *head)
{
	struct serverInfo * current = head;
	while (current!= NULL){
		if (current->ID == id)
			return (1);
		else
			current = current->next;
	}
	return (0);
}

struct serverInfo * create_server(int id, void *address, int port, int leader, SOCKET tcp_socket)
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
	server_info->tcp_socket = tcp_socket;
	server_info->next = NULL;
	
	return (server_info);
}

void append_server(struct serverInfo **head, int id, void *address, int port, int leader, SOCKET tcp_socket)
{
	if (server_info_exist(id, *head))
		return;
	struct serverInfo * new_server = create_server(id, address, port, leader, tcp_socket);
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

int delete_server(struct serverInfo *head, SOCKET tcp_socket)
{
	struct serverInfo * current = head;
	struct serverInfo * temp = NULL;
	while (current != NULL){
		if (current->next->tcp_socket == tcp_socket)
		{
			temp = current->next;
			current->next = current->next->next;
			free(temp);
			display_server(head);
			return (1);
		}
		else
			current = current->next;
	}
	display_server(head);
	return (0);
}

void display_server(struct serverInfo *head)
{
	struct serverInfo *current = head;
	printf("\n=================Servers I know=================\n");
	while (current != NULL)
	{
		printf("\tID: %d, IP: %s:%d, Leader: %d, Socket: %d\n", current->ID, current->addr, current->port, current->leader, current->tcp_socket);
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

SOCKET join_multicast(char *multicast_ip, char * mPORT)
{
    struct addrinfo hints, *bind_addr;
    SOCKET mc_socket;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, mPORT, &hints, &bind_addr))
    {
        fprintf(stderr, "[join_multicast] getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }
    if ((mc_socket = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol)) == -1){
        fprintf(stderr, "[join_multicast] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }

    if (bind(mc_socket, bind_addr->ai_addr, bind_addr->ai_addrlen) == -1){
        fprintf(stderr, "[join_multicast] bind() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	#if defined(_WIN32)
		if (setsockopt(mc_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0)	
	#else
    if (setsockopt(mc_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	#endif
    {
        fprintf(stderr, "[join_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }
	char multicast_loop = 0;
	if (setsockopt(mc_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop)) < 0) {
		fprintf(stderr, "[join_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	// unsigned char ttl = 1; // Set the TTL to 1
	// if (setsockopt(mc_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
	// 	perror("setsockopt");
	// 	// handle error
	// }
    freeaddrinfo(bind_addr); // Free the linked-list

    return (mc_socket);
}

int do_multicast(SOCKET *mc_socket, char *multicast_ip, char * msg) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(multicast_ip, MULTICAST_PORT, &hints, &res)) != 0) {
        fprintf(stderr, "[do_multicast] getaddrinfo: %s\n", gai_strerror(status));
        return (-1);
    }
	printf("[do_multicast] %s %s sending (%s)...\n",multicast_ip, MULTICAST_PORT, msg);

    // char message[32];
	// sprintf(message, "%d:%s:%d", head->ID, head->addr, head->leader);

    if (sendto(*mc_socket, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "[do_multicast] sendto failed: %s\n", strerror(errno));
		return (-1);
    }

	printf("[do_multicast] sent (%s)...\n", msg);
	// int yes = 1;
	// if (setsockopt(*mc_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	// 	fprintf(stderr, "[do_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
	// 	// handle error
	// }
	CLOSESOCKET(*mc_socket);
	*mc_socket = join_multicast(multicast_ip, MULTICAST_PORT);
    freeaddrinfo(res); // free the linked list
    return 0;
}

SOCKET service_discovery(SOCKET *mc_socket, SOCKET tcp_socket, struct serverInfo *head)
{
	fd_set master;
    FD_ZERO(&master);
    FD_SET(tcp_socket, &master);

    SOCKET socket_max = tcp_socket;

    char msg[32];
    sprintf(msg, "%d:%d", head->ID, 4041);
	SOCKET socket_client;
	char address_buffer[100];
	char service_buffer[100];
	time_t start_t, end_t;
	time(&start_t);
	printf("[service_discovery] broadcasting ID (%s), attempt...\n", msg);
	if (do_multicast(mc_socket, MULTICAST_IP, msg) == -1)
		return (-1);
    for (int attempt = 0; attempt < 3; ++attempt)
    {
		time(&end_t);
		if ((int)difftime(end_t, start_t) == 3){
			printf("[service_discovery] broadcasting ID (%s), attempt...\n", msg);
			if (do_multicast(mc_socket, MULTICAST_IP, msg) == -1)
				return (-1);
			time(&start_t);
		}

        fd_set reads;
        reads = master;

        struct timeval timeout;
        timeout.tv_sec = 1; // Wait for 3 seconds
        timeout.tv_usec = 0;

        int activity = select(socket_max + 1, &reads, NULL, NULL, &timeout);

        if (activity == -1) {
            fprintf(stderr, "[service_discovery] select() failed. (%d)\n", GETSOCKETERRNO());
			return (-1);
        } else if (activity == 0) {
            printf("[service_discovery] No response received within 3 seconds.\n");
        } else {
            // A response was received. Process it...
            for (int i = 0; i <= socket_max; i++) {
                if (FD_ISSET(i, &reads)) {
					if (i==tcp_socket) {

						struct sockaddr_storage client_address;
						socklen_t client_len = sizeof(client_address);

						socket_client = accept(tcp_socket, (struct sockaddr*)&client_address, &client_len);
						if (!ISVALIDSOCKET(socket_client))
						{
							fprintf(stderr, "[service_discover] accept() failed. (%d)\n", GETSOCKETERRNO());
							return (1);
						}
						FD_SET(socket_client, &master);
						if (socket_client > socket_max)
							socket_max = socket_client;
						
						
						getnameinfo((struct sockaddr*)&client_address, client_len, 
						address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
						printf("[service_discovery] New connection from %s\n", address_buffer);
					} else if (i==socket_client) {
						// This socket has data. Read it...
						char buf[1024];
						int numBytes = recv(i, buf, sizeof buf, 0);
						if (numBytes == -1)
						{
							fprintf(stderr, "[service_discovery] recv() failed. (%d)\n", GETSOCKETERRNO());
							return (-1);
						}
						int ID, leader, mPORT;
						char successorIP[16];
						if (sscanf(buf, "%d:%15[^:]:%d:%d", &ID, successorIP, &mPORT, &leader) == 4){
							printf("[service_discovery] Received: (%d) bytes: %.*s\n", numBytes, numBytes, buf);
							if (leader == 1)
							{
								printf("[service_discovery] Leader found.\n");
								head->leader = 0;
								append_server(&head, ID, (void *)address_buffer, head->port, leader, socket_client);
								if (strlen(successorIP) >= 8)
								{
									char port[6];
									sprintf(port, "%d", mPORT);
									SOCKET socket_successor = setup_tcp_client(successorIP, port);
									if (socket_successor == -1)
										return (-1);
									append_server(&head, ID, (void *)successorIP, mPORT, leader, socket_successor);
								}

								return (socket_client);
							} else {
								FD_CLR(socket_client, &master);
								CLOSESOCKET(socket_client);
								continue;
							}
						} else {
							FD_CLR(socket_client, &master);
							CLOSESOCKET(socket_client);
							continue;
						}
					}
                }
            }
        }
    }
	return (-1);
}


SOCKET handle_mcast_receive(SOCKET mc_socket, struct serverInfo * connected_peers) {
    char buf[1024];
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    int bytes_received = recvfrom(mc_socket, buf, sizeof(buf), 0, (struct sockaddr*)&sender_addr, &sender_addr_len);

    if (bytes_received == -1) {
        fprintf(stderr, "[handle_mcast_receive] recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }

    buf[bytes_received] = '\0'; // Null-terminate the string

    int received_id;
	if (connected_peers->leader == 1)
	{
		SOCKET peer_socket = peer_mcast_receive(connected_peers, buf, sender_addr);
		if (peer_socket == -1)
		{
			fprintf(stderr, "[handle_mcast_receive] peer_mcast_receive() failed. (%d)\n", GETSOCKETERRNO());
			return (-1);}
		return (peer_socket);
	} else if (sscanf(buf, "%d", &received_id) == 1) {
        int leaderId = connected_peers->next->ID; // Get the next peer
		if (received_id == leaderId) {
			printf("[handle_mcast_receive] Leader <Ok> ID: %d\n", received_id);
			return (0);
		} else if (connected_peers->next->next != NULL) {
			if (connected_peers->next->next->ID == received_id) {
				printf("[handle_mcast_receive] Successor <Ok> ID: %d\n", received_id);
				return (0);
			}
		}
    }
	return (-1);
}

SOCKET peer_mcast_receive(struct serverInfo * connected_peers, char *buf, struct sockaddr_in sender_addr) {

    int new_peer_id, new_peer_port;
    if (sscanf(buf, "%d:%d", &new_peer_id, &new_peer_port) == 2) {
		char port[6];
		sprintf(port, "%d", new_peer_port);
		SOCKET ctcp_socket = setup_tcp_client(inet_ntoa(sender_addr.sin_addr), port);
		if (ctcp_socket == -1)
		{
			fprintf(stderr, "[peer_mcast_receive] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
			return (-1);
		}
        char message[1024];
		if (connected_peers->next != NULL)
        	snprintf(message, sizeof(message), "%d:%s:%d:%d", connected_peers->ID, connected_peers->next->addr, connected_peers->next->port, connected_peers->leader);
		else
			snprintf(message, sizeof(message), "%d:%s:%d:%d", connected_peers->ID, "0.0.0.0", '0', connected_peers->leader);
		// add the new peer to the list
		
        if (send(ctcp_socket, message, strlen(message), 0) == -1) {
			fprintf(stderr, "[peer_mcast_receive] send() failed. (%d)\n", GETSOCKETERRNO());
			return (-1);
		}
		append_server(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
        return (ctcp_socket);
    } else if (sscanf(buf, "%d", &new_peer_id) == 1) {
		if (server_info_exist(new_peer_id, connected_peers) != 0)
			printf("[peer_mcast_receive] peer ID (%d) <Ok>.\n", new_peer_id);
		else
			printf("[peer_mcast_receive] ID (%d) Not known.\n", new_peer_id);

	} else {
		printf("[peer_mcast_receive] Unknow message fomrat: (%lu) bytes: %.*s\n", strlen(buf), (int)strlen(buf), buf);
	}
   return (1);
}

SOCKET setup_tcp_client(char *address, char *port)
{
    printf("[setup_tcp_client] Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(address, port, &hints, &peer_address)) {
        fprintf(stderr, "[setup_tcp_client] getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }


    printf("[setup_tcp_client] Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("[setup_tcp_client] %s %s\n", address_buffer, service_buffer);


    printf("[setup_tcp_client] Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "[setup_tcp_client] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }


    printf("[setup_tcp_client] Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "[setup_tcp_client] connect() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
    }
    freeaddrinfo(peer_address);

	return (socket_peer);
}

int ist_peer_server(int sockfd, struct serverInfo *head) {
	if (head->leader != 1)
		return (0);
    struct serverInfo *current = head;
    while (current != NULL) {
        if (current->tcp_socket == sockfd) {
            return (current->ID); 
        }
        current = current->next;
    }
    return (0);
}

void handle_disconnection(struct serverInfo * head, SOCKET i, SOCKET udp_socket, SOCKET mc_socket, SOCKET ltcp_socket)
{
	if (i == udp_socket) {
		printf("[handle_disconnection] udp_client disconnect...\n");
	} else if (i == mc_socket) {
		printf("[handle_disconnection] mc_client disconnect...\n");
	} else if (i == ltcp_socket) {
		printf("[handle_disconnection] leader disconnected...\n");
		printf("[handle_disconnection] Leader election required...\n");
		head->leader = 1;
		delete_server(head, i);
	} else if(ist_peer_server(i, head) != 0) {
		printf("[handle_disconnection] Peer (%d) disconnected...\n", ist_peer_server(i, head));
		delete_server(head, i);
	} else {
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
	}
}