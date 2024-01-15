#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define MAX_HOST_NAME 50
#define NEXT_SERVER_PORT 8082

typedef struct {
    char name[MAX_HOST_NAME];
    char address[INET_ADDRSTRLEN];
    int port;
} HostInfo;

HostInfo connectedClients[MAX_CLIENTS];
int numClients = 0;

void broadcastHostInfo(int newClientSocket) {
    for (int i = 0; i < numClients; ++i) {
        send(newClientSocket, &connectedClients[i], sizeof(HostInfo), 0);
    }
}

void handleClient(int clientSocket) {
    HostInfo newClientInfo;
    recv(clientSocket, &newClientInfo, sizeof(HostInfo), 0);

    printf("New client connected: %s at %s:%d\n", newClientInfo.name, newClientInfo.address, newClientInfo.port);

    connectedClients[numClients++] = newClientInfo;
    broadcastHostInfo(clientSocket);
}

int main() {
    int serverSocket, clientSocket, nextServerSocket;
    struct sockaddr_in serverAddr, clientAddr, nextServerAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

	nextServerSocket = accept(serverSocket, (struct sockaddr *)&nextServerAddr, &addrLen);
    if (nextServerSocket == -1) {
        perror("Error accepting connection");
        exit(EXIT_FAILURE);
    }

    printf("Server 2: Connection accepted from Server 3\n");

    while (1) {
        // Accept a new connection
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSocket == -1) {
            perror("Error accepting connection");
            continue;
        }

        handleClient(clientSocket);

        // Close the client socket after handling
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
