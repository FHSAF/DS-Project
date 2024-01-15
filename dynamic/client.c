#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_ADDRESS "127.0.0.1"
#define PORT 8888
#define MAX_HOST_NAME 50

typedef struct {
    char name[MAX_HOST_NAME];
    char address[INET_ADDRSTRLEN];
    int port;
} HostInfo;

void receiveHostInfo(int clientSocket) {
    HostInfo hostInfo;

    while (recv(clientSocket, &hostInfo, sizeof(HostInfo), 0) > 0) {
        printf("Received host information: %s at %s:%d\n", hostInfo.name, hostInfo.address, hostInfo.port);
    }
}

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serverAddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // Example: Send host information to the server
    HostInfo myHostInfo;
    strcpy(myHostInfo.name, "MyHost");
    strcpy(myHostInfo.address, "127.0.0.1");
    myHostInfo.port = 1234;

    send(clientSocket, &myHostInfo, sizeof(HostInfo), 0);

    // Receive and display host information from the server
    receiveHostInfo(clientSocket);

    // Close the client socket
    close(clientSocket);

    return 0;
}
