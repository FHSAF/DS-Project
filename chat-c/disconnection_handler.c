#include "server_s.h"

void handle_disconnection(struct serverInfo * head, SOCKET i, SOCKET udp_socket, SOCKET mc_socket, SOCKET ltcp_socket, SOCKET successor_socket)
{
	printf("\n=>=> [INFO][handle_disconnection] \n");

	if (i == udp_socket) {
		printf("[handle_disconnection] udp_client disconnect...\n");
	} else if (i == mc_socket) {
		printf("[handle_disconnection] mc_client disconnect...\n");
	} else if (i == ltcp_socket) {
		printf("[handle_disconnection] leader disconnected...\n");
		printf("[handle_disconnection] Leader election required...\n");
		// head->leader = 1;
		// delete_server(head, head->next->ID);
		// send_ele_msg(head);
		return;
	} else if ((i == successor_socket) && (head->leader != 1)) {
		printf("[handle_disconnection] successor disconnected...\n");
		delete_server(head, head->next->next->ID);
	} else if(ist_peer_server(i, head) != NULL) {
		return;
		// ring_status(i, head);
	} else {
		printf("[handle_disconnection] Client (%d) disconnected...\n", i);
		
		remove_client_from_list(i);
	}
}

void check_heartbeat_timeout(struct serverInfo *head, int timeout)
{
	// printf("\n=>=> [INFO][check_heartbeat_timeout] \n");

	struct serverInfo * current = head->next;
	time_t now;
	time(&now);
	while (current != NULL){
		if (difftime(now, current->last_heartbeat) >= timeout)
		{
			printf("[INFO] No HB since(%f) Server %d is dead\n", difftime(now, current->last_heartbeat), current->ID);
			SOCKET sock_i = current->tcp_socket;
			current = current->next;
			ring_status(sock_i, head);
		}
		else if (current != NULL)
			current = current->next;
	}
}

void check_leader_timeout(struct serverInfo *head, int timeout)
{
	// printf("\n=>=> [INFO][chekc_leader_timeout] \n");

	time_t now;
	time(&now);
	if (difftime(now, head->next->last_heartbeat) >= timeout)
	{
		printf("[INFO] No HB since(%f) Leader %d is dead\n", difftime(now, head->next->last_heartbeat), head->next->ID);
		// head->leader = 1;
		// delete_server(head, head->next->ID);
		send_ele_msg(head);
	}
}

void ring_status(SOCKET i, ServerInfo * head)
{
	ServerInfo *pred_i = ist_peer_server(i, head);
	if (pred_i == head){
		printf("[handle_disconnection] first element.\n");
		printf("[handle_disconnection] Peer (%d) disconnected...\n", pred_i->next->ID);
		pred_i = get_last_server(head);
		delete_server(head, head->next->ID);
		if (head->next == NULL)
		{
			printf("[handle_disconnection] No peer left.\n");
			return;
		}
		if (head->next->next == NULL)
		{
			printf("[handle_disconnection] One peer left.\n");
			return;
		}
		if (update_ring(head->next, pred_i, head)==-1) {
			printf("[handle_disconnection] update_ring() failed.\n");
			exit(1);
		}
	} else {
		delete_server(head, pred_i->next->ID);
		if (head->next == NULL)
		{
			printf("[handle_disconnection] No peer left.\n");
			return;
		}
		if (head->next->next == NULL)
		{
			printf("[handle_disconnection] One peer left.\n");
			return;
		}
		if (update_ring(NULL, pred_i, head)==-1) {
			printf("[handle_disconnection] update_ring() failed.\n");
			exit(1);
		}
	}
}

int update_ring(struct serverInfo *head, ServerInfo *pred_i, ServerInfo *connected_peers)
{
	printf("\n=>=> [INFO][update_ring] \n");
	
	char message[1024];
	printf("\n[update_ring] updating %d ring...\n", pred_i->ID);
	if (head != NULL)
		snprintf(message, sizeof(message), "UPDATE_FROM_LEADER:%s:%d:%d\n\n", head->addr, head->ID, head->port);
	else{
		if (pred_i->next == NULL)
			snprintf(message, sizeof(message), "UPDATE_FROM_LEADER:%s:%d:%d\n\n", connected_peers->next->addr, connected_peers->next->ID, connected_peers->next->port);
		else
			snprintf(message, sizeof(message), "UPDATE_FROM_LEADER:%s:%d:%d\n\n", pred_i->next->addr, pred_i->next->ID, pred_i->next->port);
	}

	memset(sendBuf, 'x', sizeof(sendBuf)-1);
	sendBuf[(int)sizeof(sendBuf) - 1] = '\0';
	memcpy(sendBuf, message, strlen(message));
	printf("[update_ring] sending to (%d) on socket (%d): %s", pred_i->tcp_socket, pred_i->tcp_socket, message);

	if (send(pred_i->tcp_socket, sendBuf, BUFFER_SIZE, 0) == -1)
	{
		fprintf(stderr, "[update_ring] send() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	return (0);
}