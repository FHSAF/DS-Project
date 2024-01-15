#include "../chat-c/server_s.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define PORT "4040"
#define MAX_CONNECTION 10
#define BUFFER_SIZE 4096
#define SERVER_IP "192.168.0.101"
#define NEXT_SERVER_IP "127.0.0.1"
#define NEXT_SERVER_PORT "6970"
#define BROADCAST_ADDRESS "192.168.0.255"
#define BROADCAST_PORT "3939"



SOCKET setup_udp_socket(char * sock_ip, char *sock_port)
{
	printf("[UDP] Configuring local address...\n");
    struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo *udp_bind_address;
    
    if (getaddrinfo(sock_ip, sock_port, &hints, &udp_bind_address) != 0) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    };

    printf("[UDP] Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(udp_bind_address->ai_family, udp_bind_address->ai_socktype, udp_bind_address->ai_protocol);
    if (!(ISVALIDSOCKET(socket_listen)))
    {
        fprintf(stderr, "[UDP] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (1);
    }
	printf("Remote address is: ");
        char address_buffer[100];
        char service_buffer[100];
        getnameinfo(udp_bind_address->ai_addr, udp_bind_address->ai_addrlen,
                address_buffer, sizeof(address_buffer),
                service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST);
        printf("%s %s\n", address_buffer, service_buffer);
    #if defined(_WIN32)
    char broadcastEnable = '1';
    #else
    int broadcastEnable = 1;
    #endif
	printf("[UDP] enabling broadcast...\n");
    if (setsockopt(socket_listen, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1)
    {
        fprintf(stderr, "[UDP] setsocktopt() failed. (%d)\n", GETSOCKETERRNO());
		CLOSESOCKET(socket_listen);
        return (-1);
    }
	
	free(udp_bind_address);
	return(socket_listen);
}

void udp_broadcast(char *msg)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo *rcv_addr;
    
	SOCKET brd_socket = setup_udp_socket(BROADCAST_ADDRESS, BROADCAST_PORT);
	if (brd_socket != -1)
	{
		getaddrinfo(BROADCAST_ADDRESS, BROADCAST_PORT, &hints, &rcv_addr);
		char address_buffer[100];
		char service_buffer[100];
		getnameinfo(rcv_addr->ai_addr,
				rcv_addr->ai_addrlen,
				address_buffer, sizeof(address_buffer),
				service_buffer, sizeof(service_buffer),
				NI_NUMERICHOST | NI_NUMERICSERV);
		printf("[udp_broadcast] address: %s %s\n", address_buffer, service_buffer);
		if (sendto(brd_socket, msg, sizeof(msg), 0, rcv_addr->ai_addr, rcv_addr->ai_addrlen) == -1)
			fprintf(stderr, "[udp_broadcast] sendto() failed. (%d)\n", GETSOCKETERRNO());
		free(rcv_addr);
		CLOSESOCKET(brd_socket);
	}
}

int main()
{
	time_t start_t, end_t;
	time(&start_t);
	int count = 0;
	while (1){
	    time(&end_t);
		char *msg = "[Leader] Ok.";
        if ((int)difftime(end_t, start_t) == 5)
        {
			printf("%d \n", count);

			udp_broadcast(msg);
			count += 1;
            time(&start_t);
        }
	}
}