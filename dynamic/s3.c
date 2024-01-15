#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8083
#define PREV_SERVER_PORT 8082

int main() {
    int serverSocket, prevServerSocket;
    struct sockaddr_in serverAddr, prevServerAddr;
    socklen_t addrLen = sizeof(prevServerAddr);

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
    if (listen(serverSocket, 1) == -1) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    printf("Server 3 listening on port %d...\n", PORT);

    // Accept a connection from the previous server
    prevServerSocket = accept(serverSocket, (struct sockaddr *)&prevServerAddr, &addrLen);
    if (prevServerSocket == -1) {
        perror("Error accepting connection");
        exit(EXIT_FAILURE);
    }

    printf("Server 3: Connection accepted from Server 2\n");

    // Connect to the previous server
    int prevServerClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (prevServerClientSocket == -1) {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    // Initialize previous server address struct
    struct sockaddr_in prevServerClientAddr;
    prevServerClientAddr.sin_family = AF_INET;
    prevServerClientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    prevServerClientAddr.sin_port = htons(PREV_SERVER_PORT);

    // Connect to the previous server
    if (connect(prevServerClientSocket, (struct sockaddr *)&prevServerClientAddr, sizeof(prevServerClientAddr)) == -1) {
        perror("Error connecting to the previous server");
        exit(EXIT_FAILURE);
    }

    printf("Server 3: Connected to Server 2\n");

    // Send "hello" message along with the port number to the next server
    char message[256];
    sprintf(message, "Hello from Server 3! My port is %d", PORT);
    send(prevServerClientSocket, message, strlen(message), 0);

    // Close the sockets
    close(prevServerSocket);
    close(prevServerClientSocket);
    close(serverSocket);

    return 0;
}
