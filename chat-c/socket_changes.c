#include "server_s.h"

void handle_socket_change(fd_set *master, SOCKET i, SOCKET *udp_socket, SOCKET *mc_socket, SOCKET *ltcp_socket, SOCKET *successor_socket, SOCKET *socket_max, ServerInfo *connected_peers)
{
	char readBuf[BUFFER_SIZE];
	memset(readBuf, 0, sizeof(readBuf));

	int byte_received = recv(i, readBuf, BUFFER_SIZE-1, 0);
	if (byte_received < 1)
	{
		handle_disconnection(connected_peers, i, *udp_socket, *mc_socket, *ltcp_socket, *successor_socket);
		if (i == *ltcp_socket)
			*ltcp_socket = error_return;
		FD_CLR(i, master);
		CLOSESOCKET(i);
		printf("[handle_socker_change]] socket (%d) closed\n", i);
		return;
	}
	char read[BUFFER_SIZE];
	memset(read, 0, sizeof(read));

	char *end = strstr(readBuf, "\n\n");
	if (end == NULL)
	{
		printf("[main] end of message not found\n");
		return;
	}
	memcpy(read, readBuf, end - readBuf);
	printf("[main] message recieved read(%lu) bytes: (%s)\n", strlen(readBuf), read);

	if (i == *udp_socket)
	{
		printf("[main] read on udpsocket...\n");
		return;
	}
	int dest_id;
	int sender_id;
	char message[1024];
	memset(message, 0, sizeof(message));
	char keyword[10];
	memset(keyword, 0, sizeof(keyword));
	int pred_id;
	read[byte_received] = '\0';

	int s1ID, s1PORT;
	char s1IP[16];
	if ((i == *ltcp_socket) && (sscanf(read, "UPDATE_FROM_LEADER:%15[^:]:%d:%d", s1IP, &s1ID, &s1PORT) == 3)){
		printf("[main] read on ltcpsocket...\n");
		
		char csPORT[6];
		sprintf(csPORT, "%d", s1PORT);
		SOCKET new_successor_socket = setup_tcp_client(s1IP, csPORT);
		if (!(ISVALIDSOCKET(new_successor_socket))){
			fprintf(stderr, "[main] setup_tcp_client() for new successor failed. (%d)\n", GETSOCKETERRNO());
			exit(1);
			return;
		}
		if (connected_peers->next->next != NULL)
		{
			delete_server(connected_peers, connected_peers->next->next->ID);
			append_server(&connected_peers, s1ID, (void *)s1IP, s1PORT, 0, new_successor_socket);
			FD_CLR(*successor_socket, master);
			CLOSESOCKET(*successor_socket);
		} else {
			append_server(&connected_peers, s1ID, (void *)s1IP, s1PORT, 0, new_successor_socket);
		}
		*successor_socket = new_successor_socket;
		FD_SET(*successor_socket, master);
		if (*successor_socket > *socket_max)
			*socket_max = *successor_socket;

	} else if (sscanf(read, "CLIENT:%d:%d:%[^\n]", &sender_id, &dest_id, message) == 3)
	{
		handle_client_message(sender_id, dest_id, message, connected_peers, i);
	} else if (sscanf(read, "ELECTION:%7[^:]:%d", keyword, &pred_id) == 2) {
		printf("[main] keyword (%s) (%d)\n", read, pred_id);
		int value = leader_found(read);
		int ret_value = 0;
		if (value != 0)
		{
			memset(keyword, 0, sizeof(keyword));
			printf("[main] ELECTION rejected LEADER received\n");
			sprintf(keyword, "LEADER");
			pred_id = value;
		}
		ret_value = lcr_election(keyword, pred_id, connected_peers, i);
		if (ret_value == 1) {
			if (connected_peers->next == NULL)
				return;
			delete_server(connected_peers, connected_peers->next->ID);
			FD_CLR(connected_peers->next->tcp_socket, master);
			CLOSESOCKET(connected_peers->next->tcp_socket);
			delete_server(connected_peers, connected_peers->next->ID);

			sprintf(message, "WHO_IS_LEADER:%d:%d\n\n", connected_peers->ID, connected_peers->port);

			memset(sendBuf, 'x', sizeof(sendBuf)-1);
			sendBuf[sizeof(sendBuf) - 1] = '\0';
			memcpy(sendBuf, message, strlen(message));

			if (ISVALIDSOCKET(*successor_socket)){
				FD_CLR(*successor_socket, master);
				CLOSESOCKET(*successor_socket);
				*successor_socket = error_return;
			}
			do_multicast(mc_socket, MULTICAST_IP, sendBuf);
			
		} else if (connected_peers->ID == ret_value) {
			if (connected_peers->next == NULL)
				return;
			if (ISVALIDSOCKET(*successor_socket)){
				FD_CLR(*successor_socket, master);
				CLOSESOCKET(*successor_socket);
				*successor_socket = error_return;
			}
			FD_CLR(connected_peers->next->tcp_socket, master);
			CLOSESOCKET(connected_peers->next->tcp_socket);
			delete_server(connected_peers, connected_peers->next->ID);
		}
		fflush(stdout);
	} else {
		int nlID, nlPORT, sID, sPORT;
		char nlIP[16], sIP[16];
		if (sscanf(read, "KNOW_NEW_LEADER:%d:%15[^:]:%d:%d:%15[^:]:%d", &nlID, nlIP, &nlPORT, &sID, sIP, &sPORT) == 6){
			
			printf("[main] update leader tcp socket: %.*s\n", byte_received, read);
			if (connected_peers->next == NULL)
				append_server(&connected_peers, nlID, (void *)nlIP, nlPORT, 1, i);
			
			if (sPORT != 0){
				char csPORT[6];
				sprintf(csPORT, "%d", sPORT);
				SOCKET new_successor_socket = setup_tcp_client(sIP, csPORT);
				if (!(ISVALIDSOCKET(new_successor_socket))){
					fprintf(stderr, "[main] setup_tcp_client() for new successor failed. (%d)\n", GETSOCKETERRNO());
					exit(1);
					return;
				}
				if (connected_peers->next->next != NULL)
				{
					delete_server(connected_peers, connected_peers->next->next->ID);
					append_server(&connected_peers, sID, (void *)sIP, sPORT, 0, new_successor_socket);
					FD_CLR(*successor_socket, master);
					CLOSESOCKET(*successor_socket);
				} else {
					append_server(&connected_peers, sID, (void *)sIP, sPORT, 0, new_successor_socket);
				}
				*successor_socket = new_successor_socket;
				FD_SET(*successor_socket, master);
				if (*successor_socket > *socket_max)
					*socket_max = *successor_socket;
			}
			
			//remove_client_from_list(i);
			*ltcp_socket = i;
			display_server(connected_peers);
		} else {
			printf("[main] read on socket (%d) %s...\n", i, read);
			return;
		}
	}

	printf("[socket_chages] end.\n");
}