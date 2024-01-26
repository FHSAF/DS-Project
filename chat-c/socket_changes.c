#include "server_s.h"

void handle_socket_change(fd_set *master, SOCKET i, SOCKET udp_socket, SOCKET *mc_socket, SOCKET ltcp_socket, SOCKET successor_socket, SOCKET *socket_max, ServerInfo *connected_peers)
{
	char read[1024];
	memset(read, 0, sizeof(read));
	printf("[main] read buffer before recv(%d): %s\n", i, read);
	int byte_received = recv(i, read, 1024, 0);
	if (byte_received < 1)
	{
		handle_disconnection(connected_peers, i, udp_socket, *mc_socket, ltcp_socket, successor_socket);
		if (i == ltcp_socket)
			ltcp_socket = error_return;
		FD_CLR(i, master);
		CLOSESOCKET(i);
		return;
	}
	printf("[main] message recieved(%d): %s\n", i, read);
	if (i == udp_socket)
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
	if (i == ltcp_socket){
		printf("[main] read on ltcpsocket...\n");
		int sID, sPORT;
		char sIP[16];
		if (sscanf(read, "UPDATE_FROM_LEADER:%15[^:]:%d:%d", sIP, &sID, &sPORT) == 3)
		{
			SOCKET new_successor_socket = setup_tcp_client(sIP, PORT);
			if (!(ISVALIDSOCKET(new_successor_socket))){
				fprintf(stderr, "[main] setup_tcp_client() for new successor failed. (%d)\n", GETSOCKETERRNO());
				exit(1);
				return;
			}
			if (connected_peers->next->next != NULL)
			{
				delete_server(connected_peers, connected_peers->next->next->ID);
				append_server(&connected_peers, sID, (void *)sIP, sPORT, 0, new_successor_socket);
				FD_CLR(successor_socket, master);
				CLOSESOCKET(successor_socket);
			} else {
				append_server(&connected_peers, sID, (void *)sIP, sPORT, 0, new_successor_socket);
			}
			successor_socket = new_successor_socket;
			FD_SET(successor_socket, master);
			if (successor_socket > *socket_max)
				*socket_max = successor_socket;
		}
	} else if (sscanf(read, "CLIENT:%d:%d %[^\n]", &sender_id, &dest_id, message) == 3)
	{
		handle_client_message(sender_id, dest_id, message, connected_peers);
	} else if (sscanf(read, "ELECTION:%7[^:]:%d", keyword, &pred_id) == 2) {
		printf("[main] keyword (%s) (%d)\n", read, pred_id);
		int value = leader_found(read);
		if (value != 0)
		{
			memset(keyword, 0, sizeof(keyword));
			printf("[main] ELECTION rejected LEADER received\n");
			sprintf(keyword, "LEADER");
			pred_id = value;
		}
		if (lcr_election(keyword, pred_id, connected_peers, i, mc_socket) == connected_peers->next->ID) {
			FD_CLR(connected_peers->next->next->tcp_socket, master);
			CLOSESOCKET(connected_peers->next->next->tcp_socket);
			display_server(connected_peers);
			delete_server(connected_peers, connected_peers->next->next->ID);
		}
		fflush(stdout);
	} else {
		int sID, sPORT;
		char sIP[16];
		if (sscanf(read, "UPDATE_FROM_LEADER:%15[^:]:%d:%d", sIP, &sID, &sPORT) == 3){
			
			printf("[main] update leader tcp socket: %.*s\n", byte_received, read);
			connected_peers->next->tcp_socket = i;
			remove_client_from_list(i);
			ltcp_socket = i;
		} else {
		printf("[main] read on socket (%d) %s...\n", i, read);
		return;
		}
	}
}