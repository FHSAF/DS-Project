#include "server_s.h"

SOCKET join_multicast(char *multicast_ip, char * mPORT)
{
	printf("\n=>=> [INFO][join_multicast]\n");

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

    freeaddrinfo(bind_addr); // Free the linked-list
	printf("\n[join_multicast] joined multicast group (%s:%s)\n\n", multicast_ip, mPORT);
    return (mc_socket);
}

SOCKET do_multicast(SOCKET *mc_socket, char *multicast_ip, char * msg) 
{
	printf("\n=>=> [INFO][do_multicast]\n");

    struct addrinfo hints, *res;
    int status;
	int bytes_sent = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(multicast_ip, MULTICAST_PORT, &hints, &res)) != 0) {
        fprintf(stderr, "[do_multicast] getaddrinfo: %s\n", gai_strerror(status));
        return (error_return);
    }
	printf("==> [PROGRESS][do_multicast] sending (%lu bytes) to (%s:%s)...\n", strlen(msg), multicast_ip, MULTICAST_PORT);
    if ((bytes_sent = sendto(*mc_socket, msg, BUFFER_SIZE, 0, res->ai_addr, res->ai_addrlen)) == -1) {
        fprintf(stderr, "[do_multicast] sendto failed: %s\n", strerror(errno));
		return (error_return);
    }
	// Clear the message
	char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    char *end = strstr(msg, "\n\n");
    if (end == NULL)
    {
        printf("[clear_message] end of message not found\n");
        return (0);
    }
    memcpy(message, msg, end - msg);
	// Clear the message

	printf("==> [PROGRESS][do_multicast] sent (%d bytes) %s", bytes_sent, message);

	CLOSESOCKET(*mc_socket);
	*mc_socket = join_multicast(multicast_ip, MULTICAST_PORT);
    freeaddrinfo(res); // free the linked list
    return 0;
}


SOCKET handle_mcast_receive(SOCKET mc_socket, struct serverInfo * connected_peers) 
{
	printf("\n=>=>[INFO][handle_mcast_receive]\n");

    char readBuf[BUFFER_SIZE];
	memset(readBuf, 0, sizeof(readBuf));
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    int bytes_received = recvfrom(mc_socket, readBuf, sizeof(readBuf), 0, (struct sockaddr*)&sender_addr, &sender_addr_len);

    if (bytes_received == -1) {
        fprintf(stderr, "[handle_mcast_receive] recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return error_return;
    }

    readBuf[bytes_received-1] = '\0'; 

	char buf[BUFFER_SIZE];
	memset(buf, 0, sizeof(buf));

	char *end = strstr(readBuf, "\n\n");
	if (end == NULL)
	{
		printf("[ERROR][handle_mcast_receive] end of message not found\n");
		printf("[ERROR][handle_mcast_receive] readBuf: (%s)\n", readBuf);
		return(error_return);
	}
	memcpy(buf, readBuf, end - readBuf);

	printf("[handle_mcast_receive] Received (%d) bytes: (%s).\n", bytes_received, buf);

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

SOCKET peer_mcast_receive(struct serverInfo * connected_peers, char *buf, struct sockaddr_in sender_addr) 
{
	printf("\n=>=>[INFO][peer_mcast_receive]\n");

    int new_peer_id, new_peer_port;
	int new_client_tcp_port;

    if (sscanf(buf, "LEADER_DISCOVERY:%d:%d", &new_peer_id, &new_peer_port) == 2) {
		// New server service discovery
		return (new_server(connected_peers, new_peer_id, new_peer_port, sender_addr));
    } 
	else if (sscanf(buf, "WHO_IS_LEADER:%d:%d", &new_peer_id, &new_peer_port) == 2) {
		// The message for new leader for ring reconstruction
		return (ring_reconstruction(connected_peers, new_peer_id, new_peer_port, sender_addr));
	} 
	else if (sscanf(buf, "NEW_CLIENT:%d", &new_client_tcp_port) == 1){
		// New client service discovery
		return (new_client(connected_peers, new_client_tcp_port, sender_addr));
	}
	else if (sscanf(buf, "SERVER_HEARTBEAT:%d", &new_peer_id) == 1) {
		if (server_info_exist(new_peer_id, connected_peers) != 0)
			printf("[peer_mcast_receive] peer ID (%d) <Ok>.\n", new_peer_id);
		else
			printf("[peer_mcast_receive] ID (%d) Not known.\n", new_peer_id);
	} 
	else {
		#if defined(_WIN32)
			fprintf(stderr, "[peer_mcast_receive] Unknow message fomrat: (%d) bytes: %.*s\n", strlen(buf), (int)strlen(buf), buf);
		#else
			printf("[peer_mcast_receive] Unknow message fomrat: (%lu) bytes: %.*s\n", strlen(buf), (int)strlen(buf), buf);
		#endif
	}
   return (error_return);
}

SOCKET new_server(ServerInfo *connected_peers, int new_peer_id, int new_peer_port, struct sockaddr_in sender_addr)
{
	printf("\n=>=>[INFO][new_server]\n");

	char port[6];
	char message[1024];
	int bytes_send;

	sprintf(port, "%d", new_peer_port);
	SOCKET ctcp_socket = setup_tcp_client(inet_ntoa(sender_addr.sin_addr), port);
	if (!(ISVALIDSOCKET(ctcp_socket)))
	{
		fprintf(stderr, "|=>=>[new_server] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
		return (error_return);
	}
	
	if (connected_peers->next != NULL)
	{
		append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
		ServerInfo *successor = get_successor(new_peer_id, connected_peers);
		sprintf(message, "%d:%s:%d:%d\n\n", connected_peers->ID, successor->addr, successor->port, successor->ID);
	}else{
		append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
		snprintf(message, sizeof(message), "%d:%s:%d:%d\n\n", connected_peers->ID, "0.0.0.0", '0', connected_peers->leader);
	}
	memset(sendBuf, 'x', sizeof(sendBuf));
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	memcpy(sendBuf, message, strlen(message));

	printf("==> [PROGRESS][new_server] sending(%lu Bytes) to new server(%d)...\n", strlen(sendBuf), new_peer_id);
	if ((bytes_send = send(ctcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1) {
		fprintf(stderr, "|=>=>[new_server] send() failed. (%d)\n", GETSOCKETERRNO());
		return (error_return);
	}
	printf("==> [PROGRESS][new_server] Sent(%d) %s", bytes_send, message);
	
	if (connected_peers->next->next != NULL){
		ServerInfo *predecessor = get_predecessor(new_peer_id, connected_peers);
		if (connected_peers == predecessor)
			predecessor = get_last_server(connected_peers);

		memset(message, 0, sizeof(message));
		snprintf(message, sizeof(message), "UPDATE_FROM_LEADER:%s:%d:%d\n\n", inet_ntoa(sender_addr.sin_addr), new_peer_id, new_peer_port);

		memset(sendBuf, 'x', sizeof(sendBuf));
		sendBuf[sizeof(sendBuf) - 1] = '\0';
		memcpy(sendBuf, message, strlen(message));

		printf("==> [PROGRESS][new_server] sending (%lu Bytes) to predecessor (%d) ...\n", strlen(sendBuf), predecessor->ID);
		if ((bytes_send = send(predecessor->tcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1) {
			fprintf(stderr, "|=>=>[new_server] send() to last peer failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}
		printf("==> [PROGRESS][new_server] Sent(%d) %s", bytes_send, message);
	}
	return (ctcp_socket);
}

SOCKET ring_reconstruction(ServerInfo *connected_peers, int new_peer_id, int new_peer_port, struct sockaddr_in sender_addr) 
{
	printf("\n=>=>[INFO][ring_reconstruction]\n");

	char port[6];
	char message[BUFFER_SIZE-3];
	int bytes_send;

	sprintf(port, "%d", new_peer_port);
	SOCKET ctcp_socket = setup_tcp_client(inet_ntoa(sender_addr.sin_addr), port);
	if (!(ISVALIDSOCKET(ctcp_socket)))
	{
		fprintf(stderr, "|=>=>[ring_reconstruction] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
		return (error_return);
	}

	if (connected_peers->next != NULL)
	{
		append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
		ServerInfo *successor = get_successor(new_peer_id, connected_peers);
		memset(message, 0, sizeof(message));
		sprintf(message, "KNOW_NEW_LEADER:%d:%s:%d:%d:%s:%d\n\n", 
		connected_peers->ID, connected_peers->addr, connected_peers->port,
		successor->ID, successor->addr, successor->port);
	} else {
		append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
		memset(message, 0, sizeof(message));
		sprintf(message, "KNOW_NEW_LEADER:%d:%s:%d:%d:%s:%d\n\n", 
		connected_peers->ID, connected_peers->addr, connected_peers->port,
		0, "0.0.0.0", 0);
	}

	memset(sendBuf, 'x', sizeof(sendBuf));
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	memcpy(sendBuf, message, strlen(message));

	printf("==> [PROGRESS][ring_reconstruction] sending(%lu Bytes) to new server(%d)...\n", strlen(sendBuf), new_peer_id);
	if ((bytes_send = send(ctcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1) {
		fprintf(stderr, "|=>=>[ring_reconstruction] send() failed. (%d)\n", GETSOCKETERRNO());
		return (error_return);
	}
	printf("==> [PROGRESS][ring_reconstruction] Sent(%d) %s", bytes_send, message);
	
	if (connected_peers->next->next != NULL){
		ServerInfo *predecessor = get_predecessor(new_peer_id, connected_peers);
		if (connected_peers == predecessor)
			predecessor = get_last_server(connected_peers);
		snprintf(message, sizeof(message), "KNOW_NEW_LEADER:%d:%s:%d:%d:%s:%d\n\n", 
		connected_peers->ID, connected_peers->addr, connected_peers->port,
		new_peer_id, inet_ntoa(sender_addr.sin_addr), new_peer_port);

		memset(sendBuf, 'x', sizeof(sendBuf)-1);
		sendBuf[sizeof(sendBuf) - 1] = '\0';
		memcpy(sendBuf, message, strlen(message));

		printf("==> [PROGRESS][ring_reconstruction] sending(%lu Bytes) to new server(%d)...\n", strlen(sendBuf), predecessor->ID);
		if ((bytes_send = send(predecessor->tcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1) {
			fprintf(stderr, "|=>=>[ring_reconstruction] send() to last peer failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}
		printf("==> [PROGRESS][ring_reconstruction] Sent(%d) %s", bytes_send, message);
	}
	return (ctcp_socket);
}

SOCKET new_client(ServerInfo * connected_peers, int new_client_tcp_port, struct sockaddr_in sender_addr)
{
	printf("\n=>=>[INFO][new_client] \n");

	char port[6];
	char message[1024];
	int bytes_send;

	sprintf(port, "%d", new_client_tcp_port);
	SOCKET tcp_socket_to_client = setup_tcp_client(inet_ntoa(sender_addr.sin_addr), port);
	if (!(ISVALIDSOCKET(tcp_socket_to_client)))
	{
		fprintf(stderr, "|=>=>[new_client] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
		return (error_return);
	}
	memset(message, 0, sizeof(message));

	// for now, we are just sending the leader's info
	int client_id = getRadomId(100, 1000);
	sprintf(message, "LEADER:YOUR_ID:%d:SERVER_IP:%s:SERVER_PORT:%d\n\n", 
			client_id, connected_peers->addr, connected_peers->port);

	memset(sendBuf, 'x', sizeof(sendBuf)-1);
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	memcpy(sendBuf, message, strlen(message));

	printf("==> [PROGRESS][new_client] sending(%lu Bytes) to new client(%d)...\n", strlen(sendBuf), client_id);
	if ((bytes_send = send(tcp_socket_to_client, sendBuf, BUFFER_SIZE, 0)) == -1) {
		fprintf(stderr, "|=>=>[new_client] send() failed. (%d)\n", GETSOCKETERRNO());
		return (error_return);
	}
	printf("==> [PROGRESS][new_client] Sent(%d) %s", bytes_send, message);

	CLOSESOCKET(tcp_socket_to_client);
	return (error_return);
}