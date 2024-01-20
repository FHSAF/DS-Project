#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MULTICAST_ADDR "239.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 1024

int main() {
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
	addr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR);
	addr.sin_port = htons(PORT);

	// Set up select timeout
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	while (1) {
		// Send multicast message
		strcpy(buffer, "Hello, multicast!");
		if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			perror("sendto");
			exit(EXIT_FAILURE);
		}

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
	close(sockfd);

	return 0;
}
