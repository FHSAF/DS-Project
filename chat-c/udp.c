#include "server_s.h"

void send_server_info(SOCKET dest, ServerInfo *myInfo)
{
    int sendBytes;
	char message[1024];
    sprintf(message, "My address {%s}.\nMy ID {%d}.\nMy Port {%d}.\n\n", myInfo->addr, myInfo->ID, 
    myInfo->port);
	memset(sendBuf, 'x', sizeof(sendBuf)-1);
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	memcpy(sendBuf, message, strlen(message));
    sendBytes = strlen(sendBuf);
    send(dest, sendBuf, sendBytes, 0);
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
	if (sendto(udp_sockfd, msg, sizeof(msg), 0, rcv_addr->ai_addr, rcv_addr->ai_addrlen) == -1)
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