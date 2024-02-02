#include "server_s.h"

struct ClientInfo clients[MAX_CONNECTION];
struct ClientInfo *tempClient;
int randomPort = 6969;
void assign_client_info(SOCKET socket_client, struct sockaddr_storage client_address)
{
	socklen_t addr_len = sizeof(client_address);
	char client_ip[NI_MAXHOST];

	int res = getnameinfo((struct sockaddr*)&client_address, addr_len, client_ip, sizeof(client_ip), NULL, 0, NI_NUMERICHOST);
	if (res != 0) {
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(res));
		return;
	}

	printf("Peer IP address: %s\n", client_ip);

	if (client_count == MAX_CONNECTION)
	{
		printf("Client limit reached. Rejecting client.\n");
		CLOSESOCKET(socket_client);
		return;
	}
	
	struct ClientInfo *client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
	if (client_info == NULL)
	{
		printf("[]Memory allocation failed\n");
		exit(1);
	}
	printf("Client id is (%d).\n", 0);
	client_info->id = 0;
	client_info->socket = socket_client;
	client_info->addr = client_info;
	memcpy(client_info->ip_addr, client_ip, sizeof(client_info->ip_addr));
	// if (temp == 0)
	// {
	// 	clients[client_count - 1] = *client_info;
	// 	send(socket_client, (void *)&client_id, sizeof(client_id), 0);
	// 	printf("Assigned ID (%d) to (%d)\n", client_id, client_count);
	// } else {
	// 	tempClient = client_info;
	// }
}

int handle_client_message(int sender_id, int dest_id, char *message, struct serverInfo *head, SOCKET i)
{
	
	printf("[handle_client_message] Sender (%d), Dest (%d), Message (%s)\n", sender_id, dest_id, message);
	if ((dest_id == sender_id) && (strcmp(message, "GET_ID") == 0))
	{
		char msg[BUFFER_SIZE];
		memset(msg, 0, sizeof(msg));
		memset(msg, 0, BUFFER_SIZE);
		int client_id = getRadomId(1000, 10000)*(++client_count);
		for (int ci = 0; ci < client_count; ++ci)
		{
			if (clients[ci].socket == i)
			{
				clients[ci].id = client_id;
			}
		}
		
		sprintf(msg, "ID:%d:GROUP_ID:%d:GROUP_IP:%s:GROUP_PORT:%d\n\n", client_id, GROUP_ID, MULTICAST_IP, randomPort);

		memset(sendBuf, 'x', sizeof(sendBuf)-1);
		sendBuf[sizeof(sendBuf) - 1] = '\0';
		memcpy(sendBuf, msg, strlen(msg));
		printf("[handle_client_msg] Client id is (%d).\n", client_id);
		if (send(i, sendBuf, strlen(sendBuf), 0) < 1)
		{
			printf("[handle_client_msg] Sending ID failed.\n");
			CLOSESOCKET(i);
			return (-1);
		}
		printf("[handle_client_msg] ID request (%lu) Bytes (%s) sent.\n", strlen(sendBuf), msg);
		return (0);
	}

	struct ClientInfo *dest_client = NULL;
	for (int ci = 0; ci < client_count; ++ci)
	{
		if (clients[ci].id == dest_id)
		{
			dest_client = &clients[ci];
			printf("[handle_client_message] Destination found localy(%d).\n",dest_id);
			break;
		}
	}

	if (dest_client != NULL)
	{
		sprintf(sendBuf, "[SENDER]: %d\n\t[MESSAGE]: %s\n\n", sender_id, message);
		int bytes_sent = send(dest_client->socket, sendBuf, strlen(sendBuf), 0);
		printf("[handle_client_message] Sent %d bytes to (%d)\n", bytes_sent, dest_client->id);
	} else {
		printf("[handle_client_message] Client not found (%d).\n", dest_id);
	}
	if (dest_id == GROUP_ID)
	{
		printf("[handle_client_message] group (%d) found locally (%d).\n", dest_id, head->ID);
		
		sprintf(sendBuf, "[SENDER]: %d\n\t[GROUP]: %d\n\t[MESSAGE]: %s\n\n",sender_id, GROUP_ID, message);
		for (int ci = 0; ci < client_count; ++ci)
		{
			if (clients[ci].id != sender_id)
			{
				int bytes_sent = send(clients[ci].socket, sendBuf, strlen(sendBuf), 0);
				printf("[handle_client_message] Sent %d bytes to (%d)\n", bytes_sent, clients[ci].id);
			}
		}
		return (1);
	} else {
		// Handle if group is not found in self server
		printf("[handle_client_message] group (%d) (%d)not found locally.\n", dest_id, GROUP_ID);
	}
	return (0);
}

int message_to_group(SOCKET sender_socket, int group_id, char *msg, struct serverInfo *head)
{

	if (group_id == GROUP_ID)
	{
		printf("[message_to_group] group (%d) found locally (%d).\n", group_id, head->ID);
		
		int sender_id = get_client_id(sender_socket);
		snprintf(sendBuf, sizeof(sendBuf), "%d:%s\n", sender_id, msg);
		for (int ci = 0; ci < client_count; ++ci)
		{
			if (clients[ci].id != sender_id)
			{
				int bytes_sent = send(clients[ci].socket, sendBuf, strlen(sendBuf), 0);
				printf("[message_to_group] Sent %d bytes to (%d)\n", bytes_sent, clients[ci].id);
			}
		}
		return (1);
	} else {
		// Handle if group is not found in self server
		printf("[message_to_group] group (%d) (%d)not found locally.\n", group_id, GROUP_ID);
		// TODO: forward to successor
		return (1);
	}
	
	return (1);
}

int get_client_id(SOCKET socket)
{
	for (int ci = 0; ci < client_count; ++ci)
	{
		if (clients[ci].socket == socket)
			return (clients[ci].id);
	}
	return (-1);
}



void remove_client_from_list(SOCKET sockfd)
{
	for (int ci = 0; ci < client_count; ++ci)
	{
		if (clients[ci].socket == sockfd)
		{
			printf("Client with ID:(%d) disconnected.\n", clients[ci].id);
			free(clients[ci].addr);
			// printf("freed %p \n", clients[ci].addr);
			for (int cj = ci; cj < client_count - 1; ++cj)
			{
				clients[cj] = clients[cj + 1];
			}
			client_count--;
			break;
		}
	}
}