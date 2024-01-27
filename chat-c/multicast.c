#include "server_s.h"

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
	printf("[do_multicast] (%lu) sending (%s)...\n", strlen(msg), multicast_ip);

    // char message[32];
	// sprintf(message, "%d:%s:%d", head->ID, head->addr, head->leader);

    if (sendto(*mc_socket, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "[do_multicast] sendto failed: %s\n", strerror(errno));
		return (error_return);
    }

	printf("[do_multicast] sent (%lu bytes)\n", strlen(msg));
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


SOCKET handle_mcast_receive(SOCKET mc_socket, struct serverInfo * connected_peers) {
	
    char readBuf[BUFFER_SIZE+1];
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    int bytes_received = recvfrom(mc_socket, readBuf, sizeof(readBuf), 0, (struct sockaddr*)&sender_addr, &sender_addr_len);

    if (bytes_received == -1) {
        fprintf(stderr, "[handle_mcast_receive] recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return error_return;
    }

    readBuf[bytes_received] = '\0'; 

	char buf[BUFFER_SIZE];
	memset(buf, 0, sizeof(buf));

	char *end = strstr(readBuf, "\n\n");
	if (end == NULL)
	{
		printf("[main] end of message not found\n");
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

SOCKET peer_mcast_receive(struct serverInfo * connected_peers, char *buf, struct sockaddr_in sender_addr) {

    int new_peer_id, new_peer_port;
	char message[1024];
    if (sscanf(buf, "LEADER_DISCOVERY:%d:%d", &new_peer_id, &new_peer_port) == 2) {
		char port[6];
		sprintf(port, "%d", new_peer_port);
		SOCKET ctcp_socket = setup_tcp_client(inet_ntoa(sender_addr.sin_addr), port);
		if (!(ISVALIDSOCKET(ctcp_socket)))
		{
			fprintf(stderr, "[peer_mcast_receive] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}
        
		if (connected_peers->next != NULL)
		{
			append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
			ServerInfo *successor = get_successor(new_peer_id, connected_peers);
			printf("[----] successor ID (%d)\n", successor->ID);
			sprintf(message, "%d:%s:%d:%d\n\n", connected_peers->ID, successor->addr, successor->port, successor->ID);
		}else{
			append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
			snprintf(message, sizeof(message), "%d:%s:%d:%d\n\n", connected_peers->ID, "0.0.0.0", '0', connected_peers->leader);
		}
		memset(sendBuf, 'x', sizeof(sendBuf));
		sendBuf[sizeof(sendBuf) - 1] = '\0';
		memcpy(sendBuf, message, strlen(message));

		printf("[peer_mcast_receive] sending successor to new server(%d) (%s) ...\n", new_peer_id, sendBuf);
        if (send(ctcp_socket, sendBuf, strlen(sendBuf), 0) == -1) {
			fprintf(stderr, "[peer_mcast_receive] send() failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}
		
		// send the new peer ip, id, port to the last peer
		if (connected_peers->next->next != NULL){
			ServerInfo *predecessor = get_predecessor(new_peer_id, connected_peers);
			if (connected_peers == predecessor)
				predecessor = get_last_server(connected_peers);
			memset(message, 0, sizeof(message));
			snprintf(message, sizeof(message), "UPDATE_FROM_LEADER:%s:%d:%d\n\n", inet_ntoa(sender_addr.sin_addr), new_peer_id, new_peer_port);
			// SOCKET last_peer_socket = get_last_peer_socket(connected_peers);
			memset(sendBuf, 'x', sizeof(sendBuf));
			sendBuf[sizeof(sendBuf) - 1] = '\0';
			memcpy(sendBuf, message, strlen(message));
			printf("[peer_mcast_receive] sending update (%s) to predecessro (%d) ...\n", sendBuf, predecessor->ID);
			if (send(predecessor->tcp_socket, sendBuf, strlen(sendBuf), 0) == -1) {
				fprintf(stderr, "[peer_mcast_receive] send() to last peer failed. (%d)\n", GETSOCKETERRNO());
				return (error_return);
			}
		}
        return (ctcp_socket);
    } else if (sscanf(buf, "SERVER_HEARTBEAT:%d", &new_peer_id) == 1) {
		if (server_info_exist(new_peer_id, connected_peers) != 0)
			printf("[peer_mcast_receive] peer ID (%d) <Ok>.\n", new_peer_id);
		else
			printf("[peer_mcast_receive] ID (%d) Not known.\n", new_peer_id);

	} else if (sscanf(buf, "WHO_IS_LEADER:%d:%d", &new_peer_id, &new_peer_port) == 2) {
		char port[6];
		sprintf(port, "%d", new_peer_port);
		SOCKET ctcp_socket = setup_tcp_client(inet_ntoa(sender_addr.sin_addr), port);
		if (!(ISVALIDSOCKET(ctcp_socket)))
		{
			fprintf(stderr, "[peer_mcast_receive] setup_tcp_client() failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}

		if (connected_peers->next != NULL)
		{
			append_server_sorted(&connected_peers, new_peer_id, (void *)inet_ntoa(sender_addr.sin_addr), new_peer_port, 0, ctcp_socket);
			ServerInfo *successor = get_successor(new_peer_id, connected_peers);
			printf("[----] successor ID (%d)\n", successor->ID);
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

		printf("[peer_mcast_receive] sending successor to new server (%d) (%ld) (%s) ...\n", new_peer_id, sizeof(sendBuf), message);
        if (send(ctcp_socket, sendBuf, strlen(sendBuf), 0) == -1) {
			fprintf(stderr, "[peer_mcast_receive] send() failed. (%d)\n", GETSOCKETERRNO());
			return (error_return);
		}
		
		// send the new peer ip, id, port to the last peer
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

			// SOCKET last_peer_socket = get_last_peer_socket(connected_peers);
			printf("[peer_mcast_receive] sending update (%lu) bytes (%s) to predecessor (%d) ...\n", strlen(sendBuf), message, predecessor->ID);
			if (send(predecessor->tcp_socket, sendBuf, strlen(sendBuf), 0) == -1) {
				fprintf(stderr, "[peer_mcast_receive] send() to last peer failed. (%d)\n", GETSOCKETERRNO());
				return (error_return);
			}
		}
        return (ctcp_socket);
		
	} else {
		#if defined(_WIN32)
			fprintf(stderr, "[peer_mcast_receive] Unknow message fomrat: (%d) bytes: %.*s\n", strlen(buf), (int)strlen(buf), buf);
		#else
			printf("[peer_mcast_receive] Unknow message fomrat: (%lu) bytes: %.*s\n", strlen(buf), (int)strlen(buf), buf);
		#endif
	}
   return (error_return);
}