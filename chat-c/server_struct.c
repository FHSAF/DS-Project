#include "server_s.h"


int server_info_exist(int id, struct serverInfo *head)
{
	printf("\n=>=> [INFO][server_info_exist] \n");

	struct serverInfo * current = head;
	while (current!= NULL){
		if (current->ID == id)
			return (1);
		else
			current = current->next;
	}
	return (0);
}

struct serverInfo * create_server(int id, void *address, int port, int leader, SOCKET tcp_socket)
{
	printf("\n=>=> [INFO][create_server] \n");

	struct serverInfo* server_info = (struct serverInfo *)malloc(sizeof(struct serverInfo));
	if (server_info == NULL)
	{
		printf("Memory allocation failed\n");
		exit(1);
	}
	memcpy(server_info->addr, address, sizeof(server_info->addr));
	server_info->ID = id;
	server_info->port = port;
	server_info->leader = leader;
	server_info->tcp_socket = tcp_socket;
	time(&(server_info->last_heartbeat));
	server_info->next = NULL;
	
	return (server_info);
}

void append_server(struct serverInfo **head, int id, void *address, int port, int leader, SOCKET tcp_socket)
{
	printf("\n=>=> [INFO][append_server] \n");

	if (server_info_exist(id, *head))
		return;
	struct serverInfo * new_server = create_server(id, address, port, leader, tcp_socket);
	printf("[append] new sever created...\n");
	if (*head == NULL)
		*head = new_server;
	else{
		struct serverInfo * current = *head;
		while (current->next != NULL)
			current = current->next;
		current->next = new_server;
	}
	
	display_server(*head);
}

int delete_server(struct serverInfo *head, int id)
{
	printf("\n=>=> [INFO][delete_server] \n");

	struct serverInfo * current = head;
	struct serverInfo * temp = NULL;
	while (current != NULL){
		if (current->next->ID == id)
		{
			temp = current->next;
			current->next = current->next->next;
			free(temp);
			display_server(head);
			return (1);
		}
		else
			current = current->next;
	}
	display_server(head);
	return (0);
}

void display_server(struct serverInfo *head)
{
	printf("\n=>=> [INFO][display_server] \n");

	struct serverInfo *current = head;
	printf("\n\t==================== Servers I know ====================\n");
	while (current != NULL)
	{
		printf("\tID: %d, IP: %s:%d, Leader: %d, Socket: %d\n", current->ID, current->addr, current->port, current->leader, current->tcp_socket);
		current = current->next;
	}
	printf("\t==================== Servers I know ====================\n\n");
}

void free_server_storage(struct serverInfo *head)
{
	printf("\n=>=> [INFO][free_server_storage] \n");

	struct serverInfo *current = head;
	struct serverInfo *nextServer;
	while (current != NULL)
	{
		nextServer = current->next;
		free(current);
		current = nextServer;
	}
}

void append_server_sorted(struct serverInfo **head, int id, void *address, int port, int leader, SOCKET tcp_socket)
{
	printf("\n=>=> [INFO][append_server_sorted] \n");

	if (server_info_exist(id, *head))
		return;
	struct serverInfo * new_server = create_server(id, address, port, leader, tcp_socket);
	printf("[append] new sever created...\n");
	if (*head == NULL)
		*head = new_server;
	else{
		struct serverInfo * current = *head;
		struct serverInfo * temp = NULL;
		// if (current->ID < id)
		// {
		// 	new_server->next = current;
		// 	*head = new_server;
		// }
		while (current->next != NULL)
		{
			if (current->next->ID > id)
			{
				temp = current->next;
				current->next = new_server;
				new_server->next = temp;
				break;
			}
			current = current->next;
		}
		current->next = new_server;
	}
	
	display_server(*head);
}

ServerInfo * ist_peer_server(SOCKET sockfd, struct serverInfo *head) 
{
	printf("\n=>=> [INFO][ist_peer_server] \n");

	if (head->leader != 1)
		return (0);
    struct serverInfo *current = head;
    while (current->next != NULL) {
        if (current->next->tcp_socket == sockfd) {
            return (current); 
        }
        current = current->next;
    }
    return (NULL);
}

SOCKET get_pred_socket(int id, struct serverInfo *head)
{
	printf("\n=>=> [INFO][get_pred_socket] \n");

	struct serverInfo * current = head;
	current = current->next;
	while (current->next != NULL){
		if (current->next->ID > id)
			return (current->tcp_socket);
		else
			current = current->next;
	}
	return (head->next->tcp_socket);
}

SOCKET get_last_peer_socket(struct serverInfo *head)
{
	printf("\n=>=> [INFO][get_last_peer_socket] \n");

	struct serverInfo * current = head;
	while (current->next != NULL)
		current = current->next;
	return (current->tcp_socket);
}

ServerInfo * get_successor(int id, struct serverInfo *head)
{
	printf("\n=>=> [INFO][get_successor] \n");

	struct serverInfo * current = head;
	while (current->next != NULL){
		if (current->next->ID > id)
			return (current->next);
		else
			current = current->next;
	}
	return (head->next);
}

ServerInfo * get_predecessor(int id, struct serverInfo *head)
{
	printf("\n=>=> [INFO][get_predecessor] \n");

	struct serverInfo * current = head;
	while (current->next != NULL){
		if (current->next->ID == id)
			return (current);
		else
			current = current->next;
	}
	return (NULL);
}

ServerInfo * get_last_server(struct serverInfo *head)
{
	printf("\n=>=> [INFO][get_last_server] \n");

	struct serverInfo * current = head;
	while (current->next != NULL)
		current = current->next;
	return (current);
}

TcpClient* append_tcp_client(TcpClient* head, SOCKET socket_fd, char* IP, int PORT) {
    TcpClient* newClient = (TcpClient*)malloc(sizeof(TcpClient));
    newClient->socket_fd = socket_fd;
    strncpy(newClient->IP, IP, sizeof(newClient->IP) - 1);
    newClient->IP[sizeof(newClient->IP) - 1] = '\0';  // Ensure null-termination
    newClient->PORT = PORT;
    newClient->next = NULL;

    if (head == NULL) {
        return newClient;
    } else {
        TcpClient* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newClient;
        return head;
    }
}


TcpClient* delete_tcp_client(TcpClient* head, SOCKET socket_fd) {
    TcpClient* current = head;
    TcpClient* prev = NULL;

    while (current != NULL) {
        if (current->socket_fd == socket_fd) {
            if (prev == NULL) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return head;
        }
        prev = current;
        current = current->next;
    }

    return head;  // socket_fd not found
}


TcpClient* search_tcp_client(TcpClient* head, SOCKET socket_fd) {
    TcpClient* current = head;
    while (current != NULL) {
        if (current->socket_fd == socket_fd) {
            return current;
        }
        current = current->next;
    }
    return NULL;  // socket_fd not found
}


void display(TcpClient* head) {
    TcpClient* current = head;
    while (current != NULL) {
        printf("Socket FD: %d, IP: %s, Port: %d\n", current->socket_fd, current->IP, current->PORT);
        current = current->next;
    }
}

void update_heartbeat_timeout(struct serverInfo *head, int id)
{
	// printf("\n=>=> [INFO][update_heartbeat_timeout] \n");
	struct serverInfo * current = head;
	while (current != NULL){
		if (current->ID == id)
		{
			time_t now;
			time(&now);
			fflush(stdout);
			printf("\r[INFO][update_heartbeat_timeout] Server %d is alive since HB(%f).", current->ID, difftime(now, current->last_heartbeat));
			fflush(stdout);
			time(&(current->last_heartbeat));
			break;
		}
		else
			current = current->next;
	}
}
