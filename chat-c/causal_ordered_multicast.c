#include <stdio.h>
#include <stdlib.h>

// Define the message structure
typedef struct {
	int sender;
	int timestamp;
	char content[100];
} Message;

// Function to send a message
void send(Message message) {
	// TODO: Implement the sending logic here
	printf("Message sent by process %d with timestamp %d\n", message.sender, message.timestamp);
}

// Function to receive a message
void receive(Message message) {
	// TODO: Implement the receiving logic here
	printf("Message received from process %d with timestamp %d\n", message.sender, message.timestamp);
}

// Function to deliver a message
void deliver(Message message) {
	// TODO: Implement the message delivery logic here
	printf("Message delivered from process %d with timestamp %d\n", message.sender, message.timestamp);
}

int main() {
	// TODO: Implement the main logic here
	
	// Example usage:
	Message message1 = {1, 1, "Hello"};
	Message message2 = {2, 2, "World"};
	
	send(message1);
	send(message2);
	
	Message receivedMessage1 = {1, 1, "Hello"};
	Message receivedMessage2 = {2, 2, "World"};
	
	receive(receivedMessage2);
	receive(receivedMessage1);
	
	deliver(receivedMessage1);
	deliver(receivedMessage2);
	
	return 0;
}
