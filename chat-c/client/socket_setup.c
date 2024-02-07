#include "client.h"

SOCKET connect_toserver(const char *host, const char *port)
{
    printf("\n=>=>[connect_toserver] \n");

    printf("==> [PROGRESS] [Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(host, port, &hints, &peer_address)) {
        fprintf(stderr, "=>=>=> [ERROR] getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    printf("==> [PROGRESS] Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s:%s\n", address_buffer, service_buffer);

    printf("==> [PROGRESS] Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "=>=>=> [ERROR] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    printf("==> [PROGRESS] Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "=>=>=> [ERROR] connect() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    freeaddrinfo(peer_address);

    return (socket_peer);
}

SOCKET get_tcp_socket(char *address, char *port)
{
    printf("\n=>=>[get_tcp_socket] \n");

	printf("[TCP] Configuring local address...\n");
	SOCKET socket_listen;
	struct addrinfo *bind_address;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(address, port, &hints, &bind_address);

    printf("[TCP] Creating socket...\n");
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
	{
		fprintf(stderr, "=>=>=> [ERROR] socket() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	
    printf("[TCP] Binding socket to local address(%s:%s)...\n", address, port);

	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
	{
		fprintf(stderr, "=>=>=> [ERROR] bind() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}
	freeaddrinfo(bind_address);

    printf("[TCP] Listening...\n");
	if (listen(socket_listen, 10))
	{
		fprintf(stderr, "=>=>=> [ERROR] listen() failed. (%d)\n", GETSOCKETERRNO());
		return (-1);
	}

	return (socket_listen);
}