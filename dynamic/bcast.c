#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "8080"

int main() {
    int sockfd;
    struct addrinfo hints, *myinfo, *p;
    socklen_t addr_len;
    char buffer[1024];

    // Set up hints for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // Datagram socket

    // Get address information using getaddrinfo
    if (getaddrinfo("192.168.0.100", PORT, &hints, &myinfo) != 0) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    // Iterate through all the results and create a socket
    for (p = myinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket creation failed");
            continue;
        }

        // Bind the socket to my address and port
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Bind failed");
            close(sockfd);
            freeaddrinfo(myinfo);
            exit(EXIT_FAILURE);
        }

        break; // If we're here, we successfully bound the socket
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to create socket and bind\n");
        exit(EXIT_FAILURE);
    }

    printf("Listening for broadcasts on port %s...\n", PORT);

    // Receive broadcast messages
    addr_len = p->ai_addrlen;
    if (recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, p->ai_addr, &addr_len) == -1) {
        perror("Receive failed");
        close(sockfd);
        freeaddrinfo(myinfo);
        exit(EXIT_FAILURE);
    }

    buffer[addr_len] = '\0'; // Null-terminate the received message
    printf("Received broadcast message: %s\n", buffer);

    // Clean up
    close(sockfd);
    freeaddrinfo(myinfo);

    return 0;
}
