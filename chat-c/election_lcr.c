#include "server_s.h"

int participant = 0;

int lcr_election(char *keyword, int pred_id, struct serverInfo *connected_peers, SOCKET i)
{
	printf("\n=>=> [INFO][lcr_election] \n");

	int bytes_sent = 0;
	
	if (strcmp(keyword, "VOTE") == 0)
	{
		if (pred_id > connected_peers->ID)
		{
			if (connected_peers->next == NULL)
				return (0);
			// participant = 1;
			printf("[INFO][lcr_election] ID (%d) is greater than my ID (%d).\n", pred_id, connected_peers->ID);
			
			memset(sendBuf, 0, sizeof(sendBuf));
			sprintf(sendBuf, "ELECTION:VOTE:%d\n\n", pred_id);

			printf("==> [PROGRESS][lcr_election] sending (%lu Bytes) to (%d)...\n", strlen(sendBuf), connected_peers->next->next->ID);
			if ((bytes_sent = send(connected_peers->next->next->tcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (-1);
			}
			printf("==> [PROGRESS][lcr_election] sent (%d Bytes) %s", bytes_sent, sendBuf);
					
		} else if(pred_id < connected_peers->ID){
			if (connected_peers->next == NULL)
				return (0);
			if (participant == 1)
			{
				printf("[INFO][lcr_election] Participated ID (%d) is less than my ID (%d).\n", pred_id, connected_peers->ID);
				return (0);
			}
			participant = 1;
			
			printf("[INFO][lcr_election] ID (%d) is less than my ID (%d).\n", pred_id, connected_peers->ID);

			memset(sendBuf, 0, sizeof(sendBuf));
			sprintf(sendBuf, "ELECTION:VOTE:%d\n\n", connected_peers->ID);

			printf("==> [PROGRESS][lcr_election] sending (%lu Bytes) to (%d)...\n", strlen(sendBuf), connected_peers->next->next->ID);
			if ((bytes_sent = send(connected_peers->next->next->tcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (-1);
			}
			printf("==> [PROGRESS][lcr_election] sent (%d Bytes) %s", bytes_sent, sendBuf);

		} else if(connected_peers->ID == pred_id) {
			// I receive my message ELECTION:ID back so I'm the leader
			// I send LEADER:ID to my successor
			if (connected_peers->leader == 1)
				return (0);
			participant = 1;
			connected_peers->leader = 1;
			printf("[lcr_election] ID (%d) is equal to my ID (%d).\n", pred_id, connected_peers->ID);
			
			memset(sendBuf, 0, sizeof(sendBuf));
			sprintf(sendBuf, "ELECTION:LEADER:%d\n\n", connected_peers->ID);

			printf("==> [PROGRESS][lcr_election] sending (%lu Bytes) to (%d)...\n", strlen(sendBuf), connected_peers->next->next->ID);
			if ((bytes_sent = send(connected_peers->next->next->tcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (-1);
			}
			printf("==> [PROGRESS][lcr_election] sent (%d Bytes) %s", bytes_sent, sendBuf);

			delete_server(connected_peers, connected_peers->next->ID);

			return (pred_id);
			
		}
	} else if (strcmp(keyword, "LEADER") == 0) {
		if (pred_id == connected_peers->ID)
		{
			participant = 0;
			printf("[INFO][lcr_election] I'm leader (%d)\n.", i);
		} else {
			if (connected_peers->next == NULL)
				return (0);
			printf("[INFO][lcr_election] Leader found (%d).\n", pred_id);
			participant = 0;
			connected_peers->next->ID = pred_id;
			connected_peers->next->leader = 1;
			
			memset(sendBuf, 0, sizeof(sendBuf));
			sprintf(sendBuf, "ELECTION:LEADER:%d\n\n", pred_id);

			printf("==> [PROGRESS][lcr_election] sending (%lu Bytes) to (%d)...\n", strlen(sendBuf), connected_peers->next->next->ID);
			if ((bytes_sent = send(connected_peers->next->next->tcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1)
			{
				fprintf(stderr, "[lcr_election] send() failed. (%d)\n", GETSOCKETERRNO());
				return (0);
			}
			printf("==> [PROGRESS][lcr_election] sent (%d Bytes) %s", bytes_sent, sendBuf);
			return (1);
		}
	}

	return (0);
}

void send_ele_msg(struct serverInfo *head)
{
	printf("\n=>=> [INFO][send_ele_msg] \n");

	/// sleep(3);
	
	int bytes_sent = 0;
	if (head->next->next == NULL)
	{
		printf("[send_ele_msg] No successor found I'm the leader.\n");
		head->leader = 1;
		delete_server(head, head->next->ID);
		return;
	}
	
	memset(sendBuf, 0, sizeof(sendBuf));
	sprintf(sendBuf, "%s:%d\n\n", "ELECTION:VOTE", head->ID);
	if (participant == 1)
		return;
	printf("==> [PROGRESS][send_ele_msg] sending (%lu Bytes) to (%d)...\n", strlen(sendBuf), head->next->next->ID);
	if ((bytes_sent = send(head->next->next->tcp_socket, sendBuf, BUFFER_SIZE, 0)) == -1)
	{
		fprintf(stderr, "[send_ele_msg] send() failed. (%d)\n", GETSOCKETERRNO());
		return;
	}
	participant = 1;
	printf("==> [PROGRESS][send_ele_msg] sent (%d Bytes) %s", bytes_sent, sendBuf);
}


int leader_found(char *message)
{
	// printf("\n=>=> [INFO][leader_found] \n");

	char *found = strstr(message, "LEADER:");
	int value = 0;

	if (found != NULL) {
		char *endptr;
		value = strtol(found + strlen("LEADER:"), &endptr, 10);
		if (endptr != found + strlen("LEADER:")) {
			// printf("[leader_found] Found LEADER with ID %d\n", value);
			return (value);
		} else {
			printf("[leader_found] No integer found after LEADER:\n");
		}
	} 
	return (value);
}