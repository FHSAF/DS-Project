#include "../chat-c/server_s.h"
#include <time.h>

#if defined(_WIN32)
#include <conio.h>
#endif

int main(int argc, char *argv[]) {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);


    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    sendto(socket_peer, "Hello world", 11, 0, peer_address->ai_addr, peer_address->ai_addrlen);
    printf("Closing socket...\n");
    
    char read[1024];
    struct sockaddr_storage sender_address;
    socklen_t sender_len;
    int bytes_received = recvfrom(socket_peer, read, 1024, 0,
            (struct sockaddr*)&sender_address, &sender_len);
    if (bytes_received < 1)
    {
        fprintf(stderr, "Connection closed. (%d)\n", GETSOCKETERRNO());
        return (1);
    }
    printf("Reaceived (%d bytes) %.*s\n", bytes_received, bytes_received, read);
    printf("Remote address is: \n");

    getnameinfo(((struct sockaddr*)&sender_address),
            sender_len,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);
    free(peer_address);
    CLOSESOCKET(socket_peer);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

