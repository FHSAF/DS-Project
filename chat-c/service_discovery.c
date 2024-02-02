#include "server_s.h"

SOCKET service_discovery(SOCKET *mc_socket, SOCKET *successor_socket, SOCKET tcp_socket, struct serverInfo *head)
{
	fd_set master;
    FD_ZERO(&master);
    FD_SET(tcp_socket, &master);

    SOCKET socket_max = tcp_socket;

	char message[BUFFER_SIZE-3]; // \n\n\0
	memset(message, 0, sizeof(message));
    
    sprintf(message, "LEADER_DISCOVERY:%d:%d\n\n", head->ID, head->port);
	SOCKET socket_client;
	char address_buffer[100];
	char service_buffer[100];

	memset(sendBuf, 'x', sizeof(sendBuf)-1);
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	memcpy(sendBuf, message, strlen(message));

	printf("\t[service_discovery] broadcasting (%lu) bytes, attempt(%d)...\n", strlen(sendBuf), 1);
	if (!ISVALIDSOCKET(do_multicast(mc_socket, MULTICAST_IP, sendBuf)))
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
						char readBuf[BUFFER_SIZE+1];
						int bytes_received = recv(i, readBuf, BUFFER_SIZE, 0);
						if (bytes_received < 1)
						{
							fprintf(stderr, "[service_discovery] recv() failed. (%d)\n", GETSOCKETERRNO());
							return (error_return);
						}
						readBuf[bytes_received] = '\0'; 

						char buf[BUFFER_SIZE+1];
						memset(buf, 0, sizeof(buf));

						char *end = strstr(readBuf, "\n\n");
						if (end == NULL)
						{
							printf("[service_discovery] end of message not found\n");
							return (error_return);
						}
						memcpy(buf, readBuf, end - readBuf);

						printf("[handle_mcast_receive] Received (%d) bytes: (%s).\n", bytes_received, buf);

						int ID, IDs, mPORT;
						char successorIP[16];
						if (sscanf(buf, "%d:%15[^:]:%d:%d\n\n", &ID, successorIP, &mPORT, &IDs) == 4){

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