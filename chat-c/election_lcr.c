#include "server_s.h"

int participant = 0;

int lcr_election(char *keyword, int pred_id, struct serverInfo *connected_peers, SOCKET i)
{
	char message[1024];
	memset(message, 0, sizeof(message));
	
	if (strcmp(keyword, "VOTE") == 0)
	{
		if (pred_id > connected_peers->ID)
		{
			if (connected_peers->next == NULL)
				return (0);
			participant = 1;
			printf("[lcr_election] ID (%d) is greater than my ID (%d).\n", pred_id, connected_peers->ID);
			
			sprintf(message, "ELECTION:VOTE:%d\n\n", pred_id);

			memset(sendBuf, 'x', sizeof(sendBuf)-1);
			sendBuf[sizeof(sendBuf) - 1] = '\0';
			memcpy(sendBuf, message, strlen(message));

			if (send(connected_peers->next->next->tcp_socket, sendBuf, strlen(sendBuf), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (-1);
			}
			printf("[send_ele_msg] sent (%lu) bytes (%s) to (%d) socket (%d)\n", strlen(sendBuf), 
					message, connected_peers->next->next->ID, connected_peers->next->next->tcp_socket);
					
		} else if(pred_id < connected_peers->ID){
			if (connected_peers->next == NULL)
				return (0);
			participant = 1;
			
			printf("[lcr_election] ID (%d) is less than my ID (%d).\n", pred_id, connected_peers->ID);
			
			sprintf(message, "ELECTION:VOTE:%d\n\n", connected_peers->ID);

			memset(sendBuf, 'x', sizeof(sendBuf)-1);
			sendBuf[sizeof(sendBuf) - 1] = '\0';
			memcpy(sendBuf, message, strlen(message));

			if (send(connected_peers->next->next->tcp_socket, sendBuf, strlen(sendBuf), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (-1);
			}
			printf("[send_ele_msg] sent (%lu) bytes (%s) to (%d) socket (%d)\n", strlen(sendBuf), 
					message, connected_peers->next->next->ID, connected_peers->next->next->tcp_socket);

		} else if(connected_peers->ID == pred_id) {
			// I receive my message ELECTION:ID back so I'm the leader
			// I send LEADER:ID to my successor
			if (connected_peers->leader == 1)
				return (0);
			participant = 1;
			connected_peers->leader = 1;
			printf("[lcr_election] ID (%d) is equal to my ID (%d).\n", pred_id, connected_peers->ID);
			
			sprintf(message, "ELECTION:LEADER:%d\n\n", connected_peers->ID);
			memset(sendBuf, 'x', sizeof(sendBuf)-1);
			sendBuf[sizeof(sendBuf) - 1] = '\0';
			memcpy(sendBuf, message, strlen(message));
			
			if (send(connected_peers->next->next->tcp_socket, sendBuf, strlen(sendBuf), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (-1);
			}
			printf("[send_ele_msg] sent (%lu) bytes (%s) to (%d) socket (%d)\n", strlen(sendBuf), 
					message, connected_peers->next->next->ID, connected_peers->next->next->tcp_socket);

			delete_server(connected_peers, connected_peers->next->ID);

			return (pred_id);
			
		}
	} else if (strcmp(keyword, "LEADER") == 0) {
		if (pred_id == connected_peers->ID)
		{
			participant = 0;
			printf("[lcr_election] I'm leader (%d)", i);
		} else {
			if (connected_peers->next == NULL)
				return (0);
			printf("[lcr_election] Leader found (%d).\n", pred_id);
			participant = 0;
			connected_peers->next->ID = pred_id;
			connected_peers->next->leader = 1;
			
			sprintf(message, "ELECTION:LEADER:%d\n\n", pred_id);
			memset(sendBuf, 'x', sizeof(sendBuf)-1);
			sendBuf[sizeof(sendBuf) - 1] = '\0';
			memcpy(sendBuf, message, strlen(message));

			if (send(connected_peers->next->next->tcp_socket, sendBuf, strlen(sendBuf), 0) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (0);
			}
			return (1);
		}
	}

	return (0);
}

void send_ele_msg(struct serverInfo *head)
{
	char message[1024];
	if (head->next->next == NULL)
	{
		printf("[send_ele_msg] No successor found I'm the leader.\n");
		head->leader = 1;
		delete_server(head, head->next->ID);
		return;
	}
	
	sprintf(message, "%s:%d\n\n", "ELECTION:VOTE", head->ID);

	memset(sendBuf, 'x', sizeof(sendBuf)-1);
	sendBuf[sizeof(sendBuf) - 1] = '\0';
	memcpy(sendBuf, message, strlen(message));
	
	if (send(head->next->next->tcp_socket, sendBuf, strlen(sendBuf), 0) == -1)
	{
		fprintf(stderr, "[send_ele_msg] send() failed. (%d)\n", GETSOCKETERRNO());
		return;
	}
	printf("[send_ele_msg] sent (%lu) bytes (%s) to (%d) socket (%d)\n", strlen(sendBuf), 
			message, head->next->next->ID, head->next->next->tcp_socket);

}


int leader_found(char *message)
{
	char *found = strstr(message, "LEADER:");
	int value = 0;

	if (found != NULL) {
		char *endptr;
		value = strtol(found + strlen("LEADER:"), &endptr, 10);
		if (endptr != found + strlen("LEADER:")) {
			printf("[leader_found] Found LEADER with ID %d\n", value);
			return (value);
		} else {
			printf("[leader_found] No integer found after LEADER:\n");
		}
	} else {
		printf("[leader_found] LEADER: not found\n");
	}
	return (value);
}