#include "server_s.h"

// Extern variables
int GROUP_ID;
char sendBuf[BUFFER_SIZE];
int client_count = 0;
char PORT[6];
char SERVER_IP[16];

void clear_input_stream() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
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

	memset(sendBuf, 'x', sizeof(sendBuf)-1);
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	
	memset(PORT, 0, sizeof(PORT));
	memset(SERVER_IP, 0, sizeof(SERVER_IP));
	char temp[25];
	memset(temp, 0, sizeof(temp));

	printf("Enter IP:PORT: number: ");
	fgets(temp, sizeof(temp)-1, stdin);
	sscanf(temp, "%[^:]:%s", SERVER_IP, PORT);
	printf("Port: (%s)\n", PORT);
	printf("IP: (%s)\n", SERVER_IP);

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
		
		// sprintf(sendBuf, "%d\n\n", connected_peers->ID);
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
						printf("[main] The peer socket (%d)\n", peer_socket);
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
    free(connected_peers);
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