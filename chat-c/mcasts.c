#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #define sleep(x) Sleep(x * 1000)
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
#endif

#define MULTICAST_ADDR "239.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 1024

int main() {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "WSAStartup failed.\n");
            exit(EXIT_FAILURE);
        }
    #endif

    int sockfd;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    struct timeval timeout;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up multicast address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Receive multicast from any interface
    addr.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Join the multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR); // Multicast address
    mreq.imr_interface.s_addr = htonl(INADDR_ANY); // Any interface
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    unsigned char ttl = 1; // TTL of 1 keeps the multicast in the same subnet
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    int loopback = 0;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loopback, sizeof(loopback)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Set up select timeout
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    while (1) {
        // Send multicast message
        strcpy(buffer, "Hello, multicast! from ME");
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
        sleep(5);

        // Set up file descriptor set for select
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        // Wait for multicast messages using select
        if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check if there is data to be read
        if (FD_ISSET(sockfd, &readfds)) {
            // Receive multicast message
            memset(buffer, 0, sizeof(buffer));
            if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) < 0) {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }

            // Process received multicast message
            printf("Received multicast message: %s\n", buffer);
        }

        // Reset timeout for next iteration
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
    }

    // Close socket
    #ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
    #else
        close(sockfd);
    #endif

    return 0;
}