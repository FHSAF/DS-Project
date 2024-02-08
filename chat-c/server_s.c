#include "server_s.h"

// Extern variables
int GROUP_ID;
char sendBuf[BUFFER_SIZE];
int client_count = 0;
int clk_index = 0;
char PORT[6];
char SERVER_IP[16];

void clear_input_stream() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(int argc, char *argv[])
{
	#if defined(_WIN32)
		WSADATA d;
		if (WSAStartup(MAKEWORD(2, 2), &d))
		{
			fprintf(stderr, "Failed to initia");
			return (1);
		}
	#endif
	if (argc < 3)
	{
		fprintf(stderr, "[ERROR] usage: chat_server hostname port\n");
		return (1);
	}

	memset(sendBuf, 'x', sizeof(sendBuf)-1);
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	
	memset(PORT, 0, sizeof(PORT));
	memset(SERVER_IP, 0, sizeof(SERVER_IP));

	sscanf(argv[1], "%s", SERVER_IP);
	sscanf(argv[2], "%s", PORT);

	printf("Port: (%s)\n", PORT);
	printf("IP: (%s)\n", SERVER_IP);

    time_t start_t, end_t;
	int leader = 1;
	SOCKET socket_max;
	SOCKET ltcp_socket;
	fd_set master;
	GROUP_ID = getRadomId(20000, 1000000);
	printf("[INFO][main] Group ID (%d).\n", GROUP_ID);
	// TcpClient* tcp_clients = NULL;

    SOCKET socket_listen = setup_tcp_socket();
    if (!ISVALIDSOCKET(socket_listen))
        return(1);

    struct serverInfo *connected_peers = NULL;
	append_server(&connected_peers, getRadomId(1, 10000), (void *)SERVER_IP, atoi(PORT), leader, socket_listen);

	printf("==> [PROGRESS][main] setting up select...\n");
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
		if (ClientArray[1].socket != 0) {
			FD_SET(ClientArray[1].socket, &master);
			if (ClientArray[1].socket > socket_max)
				socket_max = ClientArray[1].socket;
		}
	} else {
		printf("[INFO][main] I am the leader.\n");
	}

	FD_SET(mc_socket, &master);
	if (mc_socket > socket_max)
		socket_max = mc_socket;

	printf("[INFO][main] Waiting for TCP connections...\n");

    time(&start_t);
	printf("[INFO][main] Enter 'exit' to exit: ");
	fflush(stdout);
	while (1)
	{
		fd_set reads;
		FD_SET(0, &master);
		reads = master;

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
		if (select(socket_max + 1, &reads, 0, 0, &timeout) < 0)
		{
			fprintf(stderr, "[ERROR][main] select() failed. (%d)\n", GETSOCKETERRNO());
			return (1);
		}

        time(&end_t);
		int bytes_sent = 0;
		memset(sendBuf, 0, sizeof(sendBuf));
		sprintf(sendBuf, "HEARTBEAT:%d\n\n", connected_peers->ID);
        if ((int)difftime(end_t, start_t) >= HEARTBEAT_TIMEOUT)
        {
			if (connected_peers->leader == 1){
				if (connected_peers->next != NULL){
					do_multicast(&mc_socket, MULTICAST_IP, sendBuf);
					check_heartbeat_timeout(connected_peers, 2*HEARTBEAT_TIMEOUT);
				}
			}else{
				ServerInfo * current = connected_peers->next;
				while (current != NULL) {
					bytes_sent = send(current->tcp_socket, sendBuf, sizeof(sendBuf), 0);
					if (bytes_sent < 0) {
						printf("[ERROR][main] send() failed.\n");
						current = current->next;
					} else {
						fflush(stdout);
						printf("\r[INFO][main] Heartbeat sent(%d bytes) to (%d).", bytes_sent, current->ID);
						fflush(stdout);
						current = current->next;
					}
					check_leader_timeout(connected_peers, 2*HEARTBEAT_TIMEOUT);
				}
			}
            time(&start_t);
        }

		#if defined(_WIN32)
			if kb_hit() {
		#else
			if (FD_ISSET(0, &reads)) {
		#endif
				if (exit_keyword() == 1) {
					break;
				}
			}

		SOCKET i;
		for (i = 1; i <= socket_max; ++i)
		{
			if (FD_ISSET(i, &reads)) 
			{
				if (i == socket_listen)
				{
					accept_new_client(socket_listen, &master, &socket_max);
				} else if (i == mc_socket) {

					SOCKET peer_socket = handle_mcast_receive(mc_socket, connected_peers);
					if (ISVALIDSOCKET(peer_socket)){
						printf("[INFOR][main] new peer socket (%d).\n", peer_socket);
						FD_SET(peer_socket, &master);
						if (peer_socket > socket_max)
							socket_max = peer_socket;
					}
					
				} else {
					handle_socket_change(&master, i, &udp_socket, &mc_socket, &ltcp_socket, &successor_socket, &socket_max, connected_peers);
					
				}
			}
		}
	}
	printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);
	CLOSESOCKET(udp_socket);
	CLOSESOCKET(mc_socket);
	CLOSESOCKET(ltcp_socket);
	CLOSESOCKET(successor_socket);
	printf("Closing connected sockets...\n");
	for (int i = 0; i < client_count; ++i)
	{
		CLOSESOCKET(ClientArray[i].socket);
		printf("Freeing client storage...\n");
		free(ClientArray[i].addr);
	}
	printf("Freeing server storage...\n");
	free_server_storage(connected_peers);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}

int getRadomId(int min, int max) {

    srand((unsigned int)time(NULL));
    int randomId = rand() % (max - min + 1) + min;

    return randomId;
}

void accept_new_client(SOCKET socket_listen, fd_set *master, SOCKET *socket_max)
{
	printf("\n=>=> [INFO][accept_new_client] \n");
	struct sockaddr_storage client_address;
	socklen_t client_len = sizeof(client_address);
	SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
	if (!ISVALIDSOCKET(socket_client))
	{
		fprintf(stderr, "[ERROR] accept() failed. (%d)\n", GETSOCKETERRNO());
		return;
	}

	FD_SET(socket_client, master);
	if (socket_client > *socket_max)
	{
		*socket_max = socket_client;
	}
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(((struct sockaddr *)&client_address),
			client_len,
			address_buffer, sizeof(address_buffer),
			service_buffer, sizeof(service_buffer),
			NI_NUMERICHOST | NI_NUMERICSERV);
	printf("=>=> [INFO] [accept_new_client] New connection from %s:%s, socket_fd: %d.\n", address_buffer, service_buffer, socket_client);
	assign_client_info(socket_client, client_address);
}

int exit_keyword() 
{
    char userInput[6];
    int ch;

    memset(userInput, 0, sizeof(userInput));

    if (fgets(userInput, sizeof(userInput), stdin)) {
        // If the newline character is not found in the input, it means the input was too long.
        // We need to consume the rest of the characters until we find a newline.
        if (strchr(userInput, '\n') == NULL) {
            while ((ch = getchar()) != '\n' && ch != EOF);
        }
    } else {
        printf("[INFO] [exit_keyword] Enter 'exit' to exit: ");
        fflush(stdout);
        return(0);
    }

    if (strlen(userInput) <= 1) {
        printf("[INFO] [exit_keyword] Enter 'exit' to exit: ");
        fflush(stdout);
        return(0);
    }

    if (strcmp(userInput, "exit\n") == 0) {
        printf("[INFO] [exit_keyword] Exiting...\n");
        return (1);
    }

    printf("[INFO] [exit_keyword] Enter 'exit' to exit: ");
    fflush(stdout);
    return (0);
}