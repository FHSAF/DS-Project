#include "server_s.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define PORT "8081"
#define MAX_CONNECTION 10
#define BUFFER_SIZE 4096

struct ClientInfo {
	int id;
	int socket;
	void *addr;
	struct sockaddr_storage address;
};

struct ClientInfo clients[MAX_CONNECTION];
int client_count = 0;

int getRadomId(int min, int max) {
    // Seed the random number generator with the current time
    srand((unsigned int)time(NULL));

    // Generate a random number between min and max
    int randomId = rand() % (max - min + 1) + min;

    return randomId;
}

int main()
{
	struct addrinfo hints;
	struct addrinfo *bind_address;
	SOCKET socket_listen;
	SOCKET socket_max;
	fd_set master;

    printf("Configuring local address...\n");
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(0, PORT, &hints, &bind_address);

    printf("Creating socket...\n");
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
	{
		fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
		return (1);
	}
	
    printf("Binding socket to local address...\n");
	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
	{
		fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
		return (1);
	}
	freeaddrinfo(bind_address);

    printf("Listening...\n");
	if (listen(socket_listen, 10))
	{
		fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
		return (1);
	}

	FD_ZERO(&master);
	FD_SET(socket_listen, &master);
	socket_max = socket_listen;

    printf("Waiting for connections...\n");

	while (1)
	{
		fd_set reads;
		reads = master;
		if (select(socket_max + 1, &reads, 0, 0, 0) < 0)
		{
			fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
			return (1);
		}

		SOCKET i;
		for (i = 1; i <= socket_max; ++i)
		{
			if (FD_ISSET(i, &reads)) 
			{
				if (i == socket_listen)
				{
					struct sockaddr_storage client_address;
					socklen_t client_len = sizeof(client_address);
					SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
					if (!ISVALIDSOCKET(socket_client))
					{
						fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
						return (1);
					}

					FD_SET(socket_client, &master);
					if (socket_client > socket_max)
					{
						socket_max = socket_client;
					}

					char address_buffer[100];
					getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
					printf("New connection from %s\n", address_buffer);
					//
					int client_id = getRadomId(10, 1000)*(++client_count);
        			printf("Client id is (%d).\n", client_id);
					struct ClientInfo *client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
					// printf("Malloced %p \n", client_info);
					client_info->id = client_id;
					client_info->socket = socket_client;
					client_info->address = client_address;
					client_info->addr = client_info;
					clients[client_count - 1] = *client_info;
        			send(socket_client, &client_id, sizeof(client_id), 0);
        			printf("Assigned ID (%d) to (%d)\n", client_id, client_count);
					//
				} else {
					char read[4096];
					int byte_received = recv(i, read, 4096, 0);
					if (byte_received < 1)
					{
						//
						for (int ci = 0; ci <= client_count; ++ci)
						{
							if (clients[ci].socket == i)
							{
								printf("Client with (%d) disconnected.\n", clients[ci].id);
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
						//
						FD_CLR(i, &master);
						CLOSESOCKET(i);
						continue;
					}
					
					int dest_id;
					char message[4096];
					read[byte_received] = '\0';
					if (sscanf(read, "%d %[^\n]", &dest_id, message) == 2)
					{
						printf("message (%s) (%lu), (%d)\n", read, sizeof(read), byte_received);
						struct ClientInfo *dest_client = NULL;
						for (int ci = 0; ci < client_count; ++ci)
						{
							if (clients[ci].id == dest_id)
							{
								dest_client = &clients[ci];
            					printf("Received from ID (%d), forwarding to ID (%d).\n", i, dest_client->id);
								break;
							}
						}

						if (dest_client != NULL)
						{
							int bytes_sent = send(dest_client->socket, read, byte_received, 0);
            				printf("Sent %d bytes to (%d)\n", bytes_sent, dest_client->id);
						} else {
							printf("Client not found (%d).\n", dest_id);
						}
					} else {

						SOCKET j;
						for (j = 1; j <= socket_max; ++j)
						{
							if (FD_ISSET(j, &master)) {
								if (j == socket_listen || j == i)
									continue;
								else
									send(j, read, byte_received, 0);
							}
						}
					}
				}
			}
		}
	}
	printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}