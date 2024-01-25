#include "server_s.h"

#if defined(_WIN32)
SOCKET error_return = INVALID_SOCKET;
#else
SOCKET error_return = -1;
#endif
int GROUP_ID;

struct ClientInfo clients[MAX_CONNECTION];
struct ClientInfo *tempClient;

int client_count = 0;
int participant = 0;

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
	GROUP_ID = getRadomId(20000, 1000000);
	printf("[main] Group ID (%d).\n", GROUP_ID);

    SOCKET socket_listen = setup_tcp_socket();
    if (!ISVALIDSOCKET(socket_listen))
        return(1);

    struct serverInfo *connected_peers = NULL;
	append_server(&connected_peers, getRadomId(1, 10000), (void *)SERVER_IP, atoi(PORT), leader, socket_listen);

	printf("[main] setting up select...\n");
	FD_ZERO(&master);
	FD_SET(socket_listen, &master);
	socket_max = socket_listen;

	SOCKET udp_socket = error_return; //setup_udp_socket(NULL, BROADCAST_PORT);
	// if (udp_socket!=-1)
	// {
	// 	FD_SET(udp_socket, &master);
	// 	if (udp_socket > socket_max)
	// 		socket_max = udp_socket;
	// }

	// mc_socket the socket receiving multicast messages
	SOCKET mc_socket = join_multicast(MULTICAST_IP, MULTICAST_PORT);
	SOCKET successor_socket = error_return;
	
	if (!ISVALIDSOCKET(mc_socket))
		return (1);
	
	ltcp_socket = service_discovery(&mc_socket, &successor_socket, socket_listen, connected_peers);
	if (ISVALIDSOCKET(ltcp_socket)) {
		
		FD_SET(ltcp_socket, &master);
		if (ltcp_socket > socket_max)
			socket_max = ltcp_socket;
		if (successor_socket != error_return) {
			FD_SET(successor_socket, &master);
			if (successor_socket > socket_max)
				socket_max = successor_socket;
		}
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
        // if ((int)difftime(end_t, start_t) == 10)
        // {
		// 	FD_CLR(mc_socket, &master);
		// 	if (connected_peers->leader == 1)
		// 		do_multicast(&mc_socket, MULTICAST_IP, msg1);
		// 	else
        //     	do_multicast(&mc_socket, MULTICAST_IP, msg1);
		// 	if (mc_socket > socket_max)
		// 		socket_max = mc_socket;
		// 	FD_SET(mc_socket, &master);
        //     printf("%d \n", (int)difftime(end_t, start_t));
        //     time(&start_t);
        // }

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
					printf("[main] New connection from %s\n", address_buffer);
					assign_client_info(socket_client, client_address, 0);
				} else if (i == mc_socket) {
					SOCKET peer_socket = handle_mcast_receive(mc_socket, connected_peers);
					if (ISVALIDSOCKET(peer_socket)){
						printf("[main] The peer socket (%d)...\n", peer_socket);
						FD_SET(peer_socket, &master);
						if (peer_socket > socket_max)
							socket_max = peer_socket;
					}
				} else {
					char read[1024];
					memset(read, 0, sizeof(read));
					int byte_received = recv(i, read, 1024, 0);
					if (byte_received < 1)
					{
						handle_disconnection(connected_peers, i, udp_socket, mc_socket, ltcp_socket, successor_socket);
						if (i == ltcp_socket)
							ltcp_socket = error_return;
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
					char message[1024];
					memset(message, 0, sizeof(message));
					char keyword[10];
					memset(keyword, 0, sizeof(keyword));
					int pred_id;
					read[byte_received] = '\0';
					printf("[main] read (%s) (%d) bytes: %.*s\n", read, byte_received, byte_received, read);
					if (i == ltcp_socket){
						printf("[main] read on ltcpsocket...\n");
						int sID, sPORT;
						char sIP[16];
						if (sscanf(read, "%15[^:]:%d:%d", sIP, &sID, &sPORT) == 3)
						{
							SOCKET new_successor_socket = setup_tcp_client(sIP, PORT);
							if (!(ISVALIDSOCKET(new_successor_socket))){
								fprintf(stderr, "[main] setup_tcp_client() for new successor failed. (%d)\n", GETSOCKETERRNO());
								return (1);
							}
							if (connected_peers->next->next != NULL)
							{
								delete_server(connected_peers, connected_peers->next->next->ID);
								append_server(&connected_peers, sID, (void *)sIP, sPORT, 0, new_successor_socket);
								FD_CLR(successor_socket, &master);
								CLOSESOCKET(successor_socket);
							} else {
								append_server(&connected_peers, sID, (void *)sIP, sPORT, 0, new_successor_socket);
							}
							successor_socket = new_successor_socket;
							FD_SET(successor_socket, &master);
							if (successor_socket > socket_max)
								socket_max = successor_socket;
						}
					} else if (sscanf(read, "client:%d %[^\n]", &dest_id, message) == 2)
					{
						#if defined(_WIN32)
							printf("[main] message (%s) (%d), (%d)\n", read, strlen(read), byte_received);
						#else
							printf("[main] message (%s) (%lu), (%d)\n", read, sizeof(read), byte_received);
						#endif
						struct ClientInfo *dest_client = NULL;
						// check if the dest_id is group id
						if ((dest_id > 20000) && (dest_id < 1000000))
						{
							message_to_group(i, dest_id, message, connected_peers);
							continue;
						}
						for (int ci = 0; ci < client_count; ++ci)
						{
							if (clients[ci].id == dest_id)
							{
								dest_client = &clients[ci];
            					printf("[main] Received from ID (%d), forwarding to ID (%d).\n", i, dest_client->id);
								break;
							}
						}

						if (dest_client != NULL)
						{
							int bytes_sent = send(dest_client->socket, read, byte_received, 0);
            				printf("[main] Sent %d bytes to (%d)\n", bytes_sent, dest_client->id);
						} else {
							printf("[main] Client not found (%d).\n", dest_id);
						}
					} else if (sscanf(read, "%9[^:]:%d", keyword, &pred_id) == 2) {
						int value;
						if (sscanf(read, "%*[^L]LEADERRR:%d", &value) == 1)
						{
							sprintf(keyword, "LEADERRR");
							pred_id = value;
						}
						if (lcr_election(keyword, pred_id, connected_peers, i, &mc_socket) == connected_peers->next->ID) {
							FD_CLR(connected_peers->next->next->tcp_socket, &master);
							CLOSESOCKET(connected_peers->next->next->tcp_socket);
							display_server(connected_peers);
							delete_server(connected_peers, connected_peers->next->next->ID);
						}
						fflush(stdout);
					} else {
						int sID, sPORT;
						char sIP[16];
						if (sscanf(read, "%15[^:]:%d:%d", sIP, &sID, &sPORT) == 3){
							
							printf("[main] update leader tcp socket: %.*s\n", byte_received, read);
							connected_peers->next->tcp_socket = i;
							remove_client_from_list(i);
							ltcp_socket = i;
						} else {
						printf("[main] read on socket (%d) %s...\n", i, read);
						continue;
						}

						// SOCKET j;
						// for (j = 1; j <= socket_max; ++j)
						// {
						// 	if (FD_ISSET(j, &master)) {
						// 		if (j == socket_listen || j == i)
						// 			continue;
						// 		else
						// 			send(j, read, byte_received, 0);
						// 	}
						// }
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
	socklen_t addr_len = sizeof(client_address);
	char client_ip[NI_MAXHOST];

	int res = getnameinfo((struct sockaddr*)&client_address, addr_len, client_ip, sizeof(client_ip), NULL, 0, NI_NUMERICHOST);
	if (res != 0) {
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(res));
		return;
	}

	printf("Peer IP address: %s\n", client_ip);

	if (client_count == MAX_CONNECTION)
	{
		printf("Client limit reached. Rejecting client.\n");
		CLOSESOCKET(socket_client);
		return;
	}
	int client_id = getRadomId(2000000, 10000000)*(++client_count);
	struct ClientInfo *client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
	if (client_info == NULL)
	{
		printf("[]Memory allocation failed\n");
		exit(1);
	}
	printf("Client id is (%d).\n", client_id);
	client_info->id = client_id;
	client_info->socket = socket_client;
	client_info->addr = client_info;
	memcpy(client_info->ip_addr, client_ip, sizeof(client_info->ip_addr));
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
        return (error_return);
    }
	mc_socket = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol);
    if (!(ISVALIDSOCKET(mc_socket))){
        fprintf(stderr, "[join_multicast] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }

    if (bind(mc_socket, bind_addr->ai_addr, bind_addr->ai_addrlen) == -1){
        fprintf(stderr, "[join_multicast] bind() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
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
        return (error_return);
    }
	char multicast_loop = 0;
	if (setsockopt(mc_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop)) < 0) {
		fprintf(stderr, "[join_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
		return (error_return);
	}
	// unsigned char ttl = 1; // Set the TTL to 1
	// if (setsockopt(mc_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
	// 	perror("setsockopt");
	// 	// handle error
	// }
    freeaddrinfo(bind_addr); // Free the linked-list

    return (mc_socket);
}

SOCKET do_multicast(SOCKET *mc_socket, char *multicast_ip, char * msg) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(multicast_ip, MULTICAST_PORT, &hints, &res)) != 0) {
        fprintf(stderr, "[do_multicast] getaddrinfo: %s\n", gai_strerror(status));
        return (error_return);
    }
	printf("[do_multicast] %s %s sending (%s)...\n",multicast_ip, MULTICAST_PORT, msg);

    // char message[32];
	// sprintf(message, "%d:%s:%d", head->ID, head->addr, head->leader);

    if (sendto(*mc_socket, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "[do_multicast] sendto failed: %s\n", strerror(errno));
		return (error_return);
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

SOCKET service_discovery(SOCKET *mc_socket, SOCKET *successor_socket, SOCKET tcp_socket, struct serverInfo *head)
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

	printf("\t[service_discovery] broadcasting ID (%s), attempt(%d)...\n", msg, 1);
	if (!ISVALIDSOCKET(do_multicast(mc_socket, MULTICAST_IP, msg)))
		return (error_return);
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        fd_set reads;
        reads = master;

        struct timeval timeout;
        timeout.tv_sec = 1; // Wait for 3 seconds
        timeout.tv_usec = 0;

        int activity = select(socket_max + 1, &reads, NULL, NULL, &timeout);

        if (activity == -1) {
            fprintf(stderr, "[service_discovery] select() failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
        } else if (activity == 0) {
            printf("[service_discovery] No response received within 3 seconds.\n");
			// printf("\t[service_discovery] broadcasting ID (%s), attempt(%d)...\n\n", msg, attempt+2);
			// if (!ISVALIDSOCKET(do_multicast(mc_socket, MULTICAST_IP, msg)))
			// 	return (error_return);
			return (error_return);
        } else {
            // A response was received. Process it...
            for (SOCKET i = 0; i <= socket_max; i++) {
                if (FD_ISSET(i, &reads)) {
					if (i==tcp_socket) {

						struct sockaddr_storage client_address;
						socklen_t client_len = sizeof(client_address);

						socket_client = accept(tcp_socket, (struct sockaddr*)&client_address, &client_len);
						if (!ISVALIDSOCKET(socket_client))
						{
							fprintf(stderr, "[service_discover] accept() failed. (%d)\n", GETSOCKETERRNO());
							return (error_return);
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
							return (error_return);
						}
						int ID, IDs, mPORT;
						char successorIP[16];
						if (sscanf(buf, "%d:%15[^:]:%d:%d", &ID, successorIP, &mPORT, &IDs) == 4){
							printf("[service_discovery] Received: (%d) bytes: %.*s\n", numBytes, numBytes, buf);
							printf("[service_discovery] Leader found.\n");
							head->leader = 0;
							append_server(&head, ID, (void *)address_buffer, head->port, 1, socket_client);
							if (strlen(successorIP) >= 8)
							{
								char port[6];
								sprintf(port, "%d", mPORT);
								SOCKET socket_successor = setup_tcp_client(successorIP, port);
								if (!(ISVALIDSOCKET(socket_successor))) {
									fprintf(stderr, "[service_discovery] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
									CLOSESOCKET(socket_client);
									return (error_return);
								}
								*successor_socket = socket_successor;
								append_server(&head, IDs, (void *)successorIP, mPORT, 0, socket_successor);
							}

							return (socket_client);
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
	return (error_return);
}


SOCKET handle_mcast_receive(SOCKET mc_socket, struct serverInfo * connected_peers) {
	
    char buf[1024];
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    int bytes_received = recvfrom(mc_socket, buf, sizeof(buf), 0, (struct sockaddr*)&sender_addr, &sender_addr_len);

    if (bytes_received == -1) {
        fprintf(stderr, "[handle_mcast_receive] recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return error_return;
    }

    buf[bytes_received] = '\0'; // Null-terminate the string

    int received_id;
	if (connected_peers->leader == 1)
	{
		SOCKET peer_socket = peer_mcast_receive(connected_peers, buf, sender_addr);
		if (peer_socket == error_return)
		{
			fprintf(stderr, "[handle_mcast_receive] peer_mcast_receive() failed. (%d)\n", GETSOCKETERRNO());
			return error_return;}
		
		return (peer_socket);
	} else if (sscanf(buf, "%d", &received_id) == 1) {
        int leaderId = connected_peers->next->ID; // Get the next peer
		if (received_id == leaderId) {
			printf("[handle_mcast_receive] Leader <Ok> ID: %d\n", received_id);
			return error_return;
		} else if (connected_peers->next->next != NULL) {
			if (connected_peers->next->next->ID == received_id) {
				printf("[handle_mcast_receive] Successor <Ok> ID: %d\n", received_id);
				return error_return;
			}
		}
    }
	return error_return;
}

SOCKET peer_mcast_receive(struct serverInfo * connected_peers, char *buf, struct sockaddr_in sender_addr) {

    int new_peer_id, new_peer_port;
    if (sscanf(buf, "%d:%d", &new_peer_id, &new_peer_port) == 2) {
		char port[6];
		sprintf(port, "%d", new_peer_port);
		SOCKET ctcp_socket = setup_tcp_client(inet_ntoa(sender_addr.sin_addr), port);
		if (!(ISVALIDSOCKET(ctcp_socket)))
		{
			fprintf(stderr, "[peer_mcast_receive] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}
        char message[1024];
		if (connected_peers->next)
		{
			ServerInfo *successor = get_successor(new_peer_id, connected_peers);
			sprintf(message, "%d:%s:%d:%d", connected_peers->ID, successor->addr, successor->port, successor->ID);
		}else
			snprintf(message, sizeof(message), "%d:%s:%d:%d", connected_peers->ID, "0.0.0.0", '0', connected_peers->leader);
		
        if (send(ctcp_socket, message, strlen(message), 0) == -1) {
			fprintf(stderr, "[peer_mcast_receive] send() failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}
		// send the new peer ip, id, port to the last peer
		if (connected_peers->next){
			snprintf(message, sizeof(message), "%s:%d:%d", inet_ntoa(sender_addr.sin_addr), new_peer_id, new_peer_port);
			// SOCKET last_peer_socket = get_last_peer_socket(connected_peers);
			SOCKET pred_socket = get_pred_socket(new_peer_id, connected_peers);
			if (send(pred_socket, message, strlen(message), 0) == -1) {
				fprintf(stderr, "[peer_mcast_receive] send() to last peer failed. (%d)\n", GETSOCKETERRNO());
				return (error_return);
			}
		}
		append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
        return (ctcp_socket);
    } else if (sscanf(buf, "%d", &new_peer_id) == 1) {
		if (server_info_exist(new_peer_id, connected_peers) != 0)
			printf("[peer_mcast_receive] peer ID (%d) <Ok>.\n", new_peer_id);
		else
			printf("[peer_mcast_receive] ID (%d) Not known.\n", new_peer_id);

	} else {
		#if defined(_WIN32)
			fprintf(stderr, "[peer_mcast_receive] Unknow message fomrat: (%d) bytes: %.*s\n", strlen(buf), (int)strlen(buf), buf);
		#else
			printf("[peer_mcast_receive] Unknow message fomrat: (%lu) bytes: %.*s\n", strlen(buf), (int)strlen(buf), buf);
		#endif
	}
   return (error_return);
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
        return (error_return);
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
        return (error_return);
    }


    printf("[setup_tcp_client] Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "[setup_tcp_client] connect() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }
    freeaddrinfo(peer_address);

	return (socket_peer);
}

void handle_disconnection(struct serverInfo * head, SOCKET i, SOCKET udp_socket, SOCKET mc_socket, SOCKET ltcp_socket, SOCKET successor_socket)
{
	if (i == udp_socket) {
		printf("[handle_disconnection] udp_client disconnect...\n");
	} else if (i == mc_socket) {
		printf("[handle_disconnection] mc_client disconnect...\n");
	} else if (i == ltcp_socket) {
		printf("[handle_disconnection] leader disconnected...\n");
		printf("[handle_disconnection] Leader election required...\n");
		// head->leader = 1;
		// delete_server(head, head->next->ID);
		send_ele_msg(head);
	} else if ((i == successor_socket) && (head->leader != 1)) {
		printf("[handle_disconnection] successor disconnected...\n");
		delete_server(head, head->next->next->ID);
	} else if(ist_peer_server(i, head) != NULL) {
		ServerInfo *pred_i = ist_peer_server(i, head);
		printf("[handle_disconnection] Peer (%d) disconnected...\n", pred_i->next->ID);
		delete_server(head, pred_i->next->ID);
		if (pred_i == head)
			return;
		if (update_ring(pred_i)==-1) {
			printf("[handle_disconnection] update_ring() failed.\n");
			exit(1);
		}
	} else {
		remove_client_from_list(i);
	}
}

void send_ele_msg(struct serverInfo *head)
{
	if (head->next->next == NULL)
	{
		printf("[send_ele_msg] No successor found I'm the leader.\n");
		head->leader = 1;
		delete_server(head, head->next->ID);
		return;
	}
	char msg[32];
	sprintf(msg, "%s:%d", "ELECTION", head->ID);
	if (send(head->next->next->tcp_socket, msg, strlen(msg), 0) == -1)
	{
		fprintf(stderr, "[send_ele_msg] send() failed. (%d)\n", GETSOCKETERRNO());
		return;
	}
}

int update_ring(struct serverInfo *head)
{
	char update[1024];
	printf("[update_ring] updating %d ring...\n", head->ID);
	if (head->next != NULL)
	{
		snprintf(update, sizeof(update), "%d:%s:%d", head->next->ID, head->next->addr, head->next->port);
		if (send(head->tcp_socket, update, strlen(update), 0) == -1)
		{
			fprintf(stderr, "[update_ring] send() failed. (%d)\n", GETSOCKETERRNO());
			return (-1);
		}
	}
	return (0);
}

int message_to_group(SOCKET sender_socket, int group_id, char *msg, struct serverInfo *head)
{

	if (group_id == GROUP_ID)
	{
		printf("[message_to_group] group (%d) found locally (%d).\n", group_id, head->ID);
		char message[1024];
		int sender_id = get_client_id(sender_socket);
		sprintf(message, "%d:%s\n", sender_id, msg);
		for (int ci = 0; ci < client_count; ++ci)
		{
			if (clients[ci].id != sender_id)
			{
				int bytes_sent = send(clients[ci].socket, message, strlen(message), 0);
				printf("[message_to_group] Sent %d bytes to (%d)\n", bytes_sent, clients[ci].id);
			}
		}
		return (1);
	} else {
		// Handle if group is not found in self server
		printf("[message_to_group] group (%d) (%d)not found locally.\n", group_id, GROUP_ID);
		// TODO: forward to successor
		return (1);
	}
	
	return (1);
}

int get_client_id(SOCKET socket)
{
	for (int ci = 0; ci < client_count; ++ci)
	{
		if (clients[ci].socket == socket)
			return (clients[ci].id);
	}
	return (-1);
}

int lcr_election(char *keyword, int pred_id, struct serverInfo *connected_peers, SOCKET i, SOCKET *mc_socket)
{
	printf("[lcr_election] keyword (%s) pred_id (%d) my ID (%d).\n", keyword, pred_id, connected_peers->ID);
	if (strcmp(keyword, "ELECTION") == 0)
	{
		printf("[lcr_election] ID (%d) received.\n", pred_id);
		if (pred_id > connected_peers->ID)
		{
			participant = 1;
			printf("[lcr_election] ID (%d) is greater than my ID (%d).\n", pred_id, connected_peers->ID);
			char msg[32];
			memset(msg, 0, sizeof(msg));
			sprintf(msg, "ELECTION:%d", pred_id);
			if (send(connected_peers->next->next->tcp_socket, msg, strlen(msg), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (0);
			}
			
		} else if(pred_id < connected_peers->ID){
			participant = 1;
			if (connected_peers->leader == 1)
				return (0);
			printf("[lcr_election] ID (%d) is less than my ID (%d).\n", pred_id, connected_peers->ID);
			char msg[32];
			memset(msg, 0, sizeof(msg));
			sprintf(msg, "ELECTION:%d", connected_peers->ID);
			if (send(connected_peers->next->next->tcp_socket, msg, strlen(msg), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (0);
			}
		} else if(connected_peers->ID == pred_id) {
			// I receive my message ELECTION:ID back so I'm the leader
			// I send LEADER:ID to my successor
			if (connected_peers->leader == 1)
				return (0);
			participant = 1;
			connected_peers->leader = 1;
			printf("[lcr_election] ID (%d) is equal to my ID (%d).\n", pred_id, connected_peers->ID);
			char msg2[64];
			memset(msg2, 0, sizeof(msg2));
			sprintf(msg2, "LEADERRR:%d", connected_peers->ID);
			printf("[lcr_election] sending (%s) to (%d) socket (%d)...\n", msg2, connected_peers->next->ID, connected_peers->next->next->tcp_socket);
			if (send(connected_peers->next->next->tcp_socket, msg2, strlen(msg2), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (1);
			}
			delete_server(connected_peers, connected_peers->next->ID);
		}
	} else if (strcmp(keyword, "LEADERRR") == 0) {
		if (pred_id == connected_peers->ID)
		{
			participant = 0;
			printf("[lcr_election] I'm leader (%d)", i);
			char message[64];
			memset(message, 0, sizeof(message));
			snprintf(message, sizeof(message), "%s:%d:%d", connected_peers->next->addr, connected_peers->next->ID, connected_peers->next->port);
			// SOCKET last_peer_socket = get_last_peer_socket(connected_peers);
			SOCKET pred_socket = get_pred_socket(connected_peers->next->ID, connected_peers);
			printf("[lcr_election] sending (%s) to (%d) socket (%d)...\n", message, connected_peers->next->ID, pred_socket);
			if (send(pred_socket, message, strlen(message), 0) == -1) {
				fprintf(stderr, "[lcr_election] send() to last peer failed. (%d)\n", GETSOCKETERRNO());
				return (error_return);
			}
			// TODO: I receive my messaeg LEADER:ID back updating ring
		} else {
			// the leader is found
			printf("[lcr_election] Leader found (%d).\n", pred_id);
			participant = 0;
			connected_peers->next->ID = pred_id;
			connected_peers->next->leader = 1;
			
			char msg3[64];
			memset(msg3, 0, sizeof(msg3));
			sprintf(msg3, "LEADERRR:%d", pred_id);
			if (send(connected_peers->next->next->tcp_socket, msg3, strlen(msg3), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (0);
			}
			memset(msg3, 0, sizeof(msg3));
    		sprintf(msg3, "%d:%d", connected_peers->ID, 4041);
			do_multicast(mc_socket, MULTICAST_IP, msg3);
			if (connected_peers->next->next->ID == pred_id)
			{
				connected_peers->next->next->leader = 1;
				return (pred_id);
			}
		}
	}

	return (0);
}

void remove_client_from_list(SOCKET sockfd)
{
	for (int ci = 0; ci < client_count; ++ci)
	{
		if (clients[ci].socket == sockfd)
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