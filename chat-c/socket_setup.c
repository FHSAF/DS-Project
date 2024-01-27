#include "server_s.h"

SOCKET setup_tcp_client(char *address, char *port)
{
    printf("[setup_tcp_client] Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(address, port, &hints, &peer_address)) {
        fprintf(stderr, "[setup_tcp_client] getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }


    printf("[setup_tcp_client] Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("[setup_tcp_client] %s %s\n", address_buffer, service_buffer);


    printf("[setup_tcp_client] Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "[setup_tcp_client] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }


    printf("[setup_tcp_client] Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "[setup_tcp_client] connect() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }
    freeaddrinfo(peer_address);

	return (socket_peer);
}

SOCKET setup_udp_socket(char * sock_ip, char *sock_port)
{
	printf("[UDP] Configuring local address...\n");
    struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo *udp_bind_address;
    
    getaddrinfo(sock_ip, sock_port, &hints, &udp_bind_address);

    printf("[UDP] Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(udp_bind_address->ai_family, udp_bind_address->ai_socktype, udp_bind_address->ai_protocol);
    if (!(ISVALIDSOCKET(socket_listen)))
    {
        fprintf(stderr, "[UDP] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (-1);
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
	int broadcast_loop = 0;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast_loop, sizeof(broadcast_loop)) < 0) {
		fprintf(stderr, "[UDP] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}

    printf("[UDP] Binding socket to local address...\n");
	if (bind(socket_listen, udp_bind_address->ai_addr, udp_bind_address->ai_addrlen))
	{
		fprintf(stderr, "[UDP] bind() failed. (%d)\n", GETSOCKETERRNO());
		CLOSESOCKET(socket_listen);
		return (-1);
	}
	freeaddrinfo(udp_bind_address);
	return(socket_listen);
}

SOCKET setup_tcp_socket()
{
	printf("[TCP] Configuring local address...\n");
	SOCKET socket_listen;
	struct addrinfo *bind_address;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(SERVER_IP, PORT, &hints, &bind_address);

    printf("[TCP] Creating socket...\n");
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
	{
		fprintf(stderr, "[TCP] socket() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	
    printf("[TCP] Binding socket to local address...\n");
	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
	{
		fprintf(stderr, "[TCP] bind() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	freeaddrinfo(bind_address);

    printf("[TCP] Listening...\n");
	if (listen(socket_listen, 10))
	{
		fprintf(stderr, "[TCP] listen() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}

	return (socket_listen);
}
