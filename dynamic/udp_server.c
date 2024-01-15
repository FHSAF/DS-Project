#include "../chat-c/server_s.h"
#include <ctype.h>

#define SERVER_IP "192.168.0.101"

int main()
{
    #if defined(_WIN32)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d))
        {
            fprintf(stderr, "Failed to initialize.\n");
            return (1);
        }
    #endif

    printf("Configuring local address...\n");
    struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
    
    struct addrinfo *bind_address;
    getaddrinfo(SERVER_IP, "3939", &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!(ISVALIDSOCKET(socket_listen)))
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return (1);
    }

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

    printf("Binding socket to local address...\n");
	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
	{
		fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	freeaddrinfo(bind_address);

    // // UDP specific
    // struct sockaddr_storage client_addr;
    // socklen_t client_len = sizeof(client_addr);
    // char read[1024];
    // int bytes_received = recvfrom(socket_listen, read, 1024, 0, 
    //                             (struct sockaddr *)&client_addr, &client_len);
    // printf("Reaceived (%d bytes) %.*s\n", bytes_received, bytes_received, read);
    // printf("Remote address is: \n");
    // char address_buffer[100];
    // char service_buffer[100];
    // getnameinfo(((struct sockaddr*)&client_addr),
    //             client_len,
    //             address_buffer, sizeof(address_buffer),
    //             service_buffer, sizeof(service_buffer),
    //             NI_NUMERICHOST | NI_NUMERICSERV);
    // printf("%s %s\n", address_buffer, service_buffer);

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    while (1) 
    {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0)
        {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return (1);
        }

        if (FD_ISSET(socket_listen, &reads))
        {
            struct sockaddr_storage client_addr;
            socklen_t client_len;
            char read[1024];
            int bytes_received = recvfrom(socket_listen, read, 1024, 0,
                    (struct sockaddr *) &client_addr, &client_len);
            if (bytes_received < 1)
            {
                fprintf(stderr, "Connection closed. (%d)\n", GETSOCKETERRNO());
                return (1);
            }
            printf("Reaceived (%d bytes) %.*s\n", bytes_received, bytes_received, read);
            printf("Remote address is: \n");
            char address_buffer[100];
            char service_buffer[100];
            getnameinfo(((struct sockaddr*)&client_addr),
                    client_len,
                    address_buffer, sizeof(address_buffer),
                    service_buffer, sizeof(service_buffer),
                    NI_NUMERICHOST | NI_NUMERICSERV);
            printf("address: %s %s\n", address_buffer, service_buffer);
            int j=0;
            for (j = 0; j < bytes_received; j++)
                read[j] = toupper(read[j]);
            
            sendto(socket_listen, read, bytes_received, 0,
                (struct sockaddr *)&client_addr, client_len);
        }
    }

    CLOSESOCKET(socket_listen);
    #if defined(_WIN32)
        WSACleanup();
    #endif
    printf("Finished.\n");
}