#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT "3939"
#define BROADCAST_ADDRESS "192.168.0.255"
#define SERVER_IP "192.168.0.101"

int main() {
    int sockfd;
    struct addrinfo hints, *servinfo;
    int broadcastEnable = 1;
    char *message = "Hello, broadcast message!";

    // Set up hints for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // Datagram socket
    hints.ai_flags = AI_PASSIVE;    // Fill in my IP for me

    // Get address information using getaddrinfo
    if (getaddrinfo(BROADCAST_ADDRESS, PORT, &hints, &servinfo) != 0) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    // Iterate through all the results and create a socket
        if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
            perror("socket creation failed");
        printf("Remote address is: ");
        char address_buffer[100];
        char service_buffer[100];
        getnameinfo(servinfo->ai_addr, servinfo->ai_addrlen,
                address_buffer, sizeof(address_buffer),
                service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST);
        printf("%s %s\n", address_buffer, service_buffer);

        // Enable broadcast
        if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1) {
            perror("setsockopt (SO_BROADCAST) failed");
            close(sockfd);
            freeaddrinfo(servinfo);
            exit(EXIT_FAILURE);
        }
        freeaddrinfo(servinfo);
        getaddrinfo(BROADCAST_ADDRESS, PORT, &hints, &servinfo);
        // Broadcast the message
        if (sendto(sockfd, message, strlen(message), 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            perror("Broadcast failed");
            close(sockfd);
            
            exit(EXIT_FAILURE);
        }




        printf("Broadcasted message: %s\n", message);


    // Clean up
    close(sockfd);
    freeaddrinfo(servinfo);

    return 0;
}
