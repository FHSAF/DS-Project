#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "6161"

int main() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    char message[] = "Hello, broadcast message!";

    // Set up hints for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // Datagram socket
    hints.ai_flags = AI_PASSIVE;    // Fill in my IP for me

    // Get address information using getaddrinfo
    if (getaddrinfo("192.168.0.255", PORT, &hints, &servinfo) != 0) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    // Iterate through all the results and create a socket
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket creation failed");
            continue;
        }

        // Broadcast the message
        if (sendto(sockfd, message, strlen(message), 0, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Broadcast failed");
            close(sockfd);
            freeaddrinfo(servinfo);
            exit(EXIT_FAILURE);
        }

        break; // If we're here, we successfully sent the broadcast
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to create socket and send broadcast\n");
    } else {
        printf("Broadcasted message: %s\n", message);
    }

    // Clean up
    close(sockfd);
    freeaddrinfo(servinfo);

    return 0;
}
