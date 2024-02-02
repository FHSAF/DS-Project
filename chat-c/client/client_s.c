#include "client.h"

char Buffer[BUFFER_SIZE];
char message[BUFFER_SIZE];
char userInput[BUFFER_SIZE - 100];

char GROUP_IP[16];
char GROUP_PORT[6];
int mode = 0;
int GROUP_ID = 0;
int SELF_ID = 0;
char *clean_message = NULL;


// Multicast
int clk_index = 0;
int T[MAX_GROUP_SIZE];
// 

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

    fd_set master;
    FD_ZERO(&master);

    SOCKET socket_peer = connect_toserver(argv[1], argv[2]);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "connect_toserver() failed.\n");
        return 1;
    }
    FD_SET(socket_peer, &master);
    
    memset(message, 0, BUFFER_SIZE);
    sprintf(message, "CLIENT:%d:%d:GET_ID\n\n", 0, 0);

    memset(Buffer, 'x', sizeof(Buffer)-1);
    Buffer[sizeof(Buffer) - 1] = '\0';
    memcpy(Buffer, message, strlen(message));

    printf("Sending ID request...\n");

    if (send(socket_peer, Buffer, strlen(Buffer), 0) < 1) {
        printf("Sending ID request failed.\n");
        CLOSESOCKET(socket_peer);
        #if defined(_WIN32)
            WSACleanup();
        #endif
        return (0);
    }
    printf("ID request (%lu) (%s) sent.\n", strlen(Buffer), message);

    memset(message, 0, BUFFER_SIZE);
    memset(Buffer, 0, sizeof(Buffer));
    int received_bytes = 0;

    if ((received_bytes = recv(socket_peer, Buffer, BUFFER_SIZE-1, 0)) < 1)
    {
        printf("Receing ID failed.\n");
        CLOSESOCKET(socket_peer);
        #if defined(_WIN32)
            WSACleanup();
        #endif
        return (0);
    }

	char *end = strstr(Buffer, "\n\n");
	if (end == NULL)
	{
		printf("[main] end of message not found\n");
		return (0);
	}
	memcpy(message, Buffer, end - Buffer);
	printf("[main] message recieved (%lu) Bytes: (%s)\n", strlen(Buffer), message);
    int mPORT;
    if (sscanf(message, "ORDER:%d:ID:%d:GROUP_ID:%d:GROUP_IP:%15[^:]:GROUP_PORT:%d", 
                    &clk_index, &SELF_ID, &GROUP_ID, GROUP_IP, &mPORT) != 5)
    {
        printf("Receing ID failed.\n");
        CLOSESOCKET(socket_peer);
        #if defined(_WIN32)
            WSACleanup();
        #endif
        return (0);
    }
    sprintf(GROUP_PORT, "%d", mPORT);
    // Getting GROUP socket
    SOCKET group_socket = join_multicast(GROUP_IP, GROUP_PORT);
    FD_SET(group_socket, &master);
    // END Getting GROUP socket

    if (!(ISVALIDSOCKET(group_socket))) {
        fprintf(stderr, "join_multicast() failed. (%d)\n", GETSOCKETERRNO());
        CLOSESOCKET(socket_peer);
        #if defined(_WIN32)
            WSACleanup();
        #endif
        return (0);
    }

    #if !defined(_WIN32)
        FD_SET(0, &master);
    #endif

    printf("==>[main] Connected with assigned ID: %d\n", SELF_ID);
    printf("==>[main] GROUP ID: %d\n", GROUP_ID);
    printf("==>[main] GROUP IP: %s:%s\n", GROUP_IP, GROUP_PORT);

    printf("[USAGE] Enter <id> <message> to send 1 for multicast (empty line to quit):\n----->");
    fflush(stdout);
    SOCKET socket_max = socket_peer;
    if (group_socket > socket_max) socket_max = group_socket;
    while(1) {

        fd_set reads;
        FD_ZERO(&reads);
        reads = master;

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
        if (select(socket_max+1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }
        if (FD_ISSET(group_socket, &reads)) {

            printf("\n=> => [NOTIFICATION] Multicast message received.\n");
            handle_group_receive(group_socket, SELF_ID);
            if (mode == 0)
                printf("[USAGE] Enter <id> <message> to send 1 for multicast (empty line to quit):\n----->");
            else 
                printf("[USAGE] Enter message to multicast, 0 for Unicast (empty line to quit):\n----->");
            fflush(stdout);
        }

        if (FD_ISSET(socket_peer, &reads)) {
            memset(Buffer, 0, sizeof(Buffer));
            int bytes_received = recv(socket_peer, Buffer, BUFFER_SIZE, 0);
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            }
            clean_message = clear_message(Buffer);
            printf("\n\t[%lu Bytes]: %s\n\n",strlen(Buffer), clean_message);
            if (mode == 0)
                printf("[USAGE] Enter <id> <message> to send 1 for multicast (empty line to quit):\n----->");
            else 
                printf("[USAGE] Enter message to multicast, 0 for Unicast (empty line to quit):\n----->");
            fflush(stdout);
        }
        

    #if defined(_WIN32)
            if(_kbhit()) {
    #else
            if(FD_ISSET(0, &reads)) {
    #endif      
                if (mode == 0){
                memset(userInput, 0, sizeof(userInput));
                if (!fgets(userInput, BUFFER_SIZE-100, stdin)) break;
                if (strlen(userInput) <= 1) break;
                if ((userInput[0] == '1') && (strlen(userInput) == 2)){ 
                    mode = 1;
                    printf("==> Mode changed to multicast.\n");
                    printf("[USAGE] Enter message to multicast, 0 for Unicast (empty line to quit):\n----->");
                    fflush(stdout);
                    continue;}
                sprintf(message, "CLIENT:%d:%s\n\n", SELF_ID, userInput);
                // BUFFER_SIZE-1 bytes should be sent, and be null-terminated
                memset(Buffer, 'x', BUFFER_SIZE-1);
                Buffer[BUFFER_SIZE - 1] = '\0';
                memcpy(Buffer, message, strlen(message));
                // 
                printf("Sending: %lu bytes %s.", strlen(Buffer), message);
                int bytes_sent = send(socket_peer, Buffer, strlen(Buffer), 0);
                printf("Sent %d bytes.\n", bytes_sent);
                printf("[USAGE] Enter <id> <message> to send 1 for multicast (empty line to quit):\n----->");
                fflush(stdout);
            } else if (mode == 1)
            {
                memset(userInput, 0, sizeof(userInput));
                if (!fgets(userInput, BUFFER_SIZE-100, stdin)) break;
                if (strlen(userInput) <= 1) break;

                if ((userInput[0] == '0') && (strlen(userInput) == 2)){ 
                    mode = 0;
                    printf("==> Mode changed to Unicast.\n");
                    printf("[USAGE] Enter <id> <message> to send 1 for multicast (empty line to quit):\n----->");
                    fflush(stdout);
                    continue;}

                sprintf(message, "GROUP_ID:%d:SENDER_ID:%d:SENDER_CLK_INDEX:%d:MESSAGE:%s\n\n", 
                                                        GROUP_ID, SELF_ID, clk_index, userInput);
                // BUFFER_SIZE-1 bytes should be sent, and be null-terminated
                memset(Buffer, 'x', BUFFER_SIZE-1);
                Buffer[BUFFER_SIZE - 1] = '\0';
                memcpy(Buffer, message, strlen(message));
                // 
                
                printf("Sending: %lu bytes %s", strlen(Buffer), message);

                // FD_CLR(group_socket, &master);
                group_multicast(&group_socket, GROUP_IP, Buffer);
                // FD_SET(group_socket, &master);

                printf("[USAGE] Enter message to multicast, 0 for Unicast (empty line to quit):\n----->");
                fflush(stdout);
            }
        }
    } //end while(1)

    printf("Closing socket...\n");
    CLOSESOCKET(group_socket);
    CLOSESOCKET(socket_peer);

    #if defined(_WIN32)
        WSACleanup();
    #endif

    printf("Finished.\n");
    return 0;
}

SOCKET connect_toserver(const char *host, const char *port)
{
    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(host, port, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s:%s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    printf("Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    freeaddrinfo(peer_address);

    return (socket_peer);
}

void handle_group_receive(SOCKET group_socket, int self_id)
{
    int DEST_ID = 0;
    int SENDER_ID = 0;
    int SENDER_CLK_INDEX = 0;
    char content[BUFFER_SIZE];
    memset(content, 0, sizeof(content));

    struct sockaddr_storage sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    char sender_ip[INET6_ADDRSTRLEN];
    char sender_port[6]; // max length of port number string

    int bytes_received = recvfrom(group_socket, Buffer, BUFFER_SIZE, 0, (struct sockaddr*)&sender_addr, &sender_addr_len);

    if (bytes_received == -1) {
        fprintf(stderr, "[handle_group_receive] recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return;
    }

    // Get sender's IP and port
    int res = getnameinfo((struct sockaddr*)&sender_addr, sender_addr_len, 
                            sender_ip, sizeof(sender_ip), sender_port, 
                            sizeof(sender_port), NI_NUMERICHOST | NI_NUMERICSERV);

    if (res != 0) {
        fprintf(stderr, "[handle_group_receive] getnameinfo() failed: %s\n", gai_strerror(res));
        return;
    }

    clean_message = clear_message(Buffer);
    if (clean_message == NULL)
    {
        printf("[handle_group_receive] clean_message is NULL\n");
        return;
    }
    if (sscanf(clean_message, "GROUP_ID:%d:SENDER_ID:%d:SENDER_CLK_INDEX:%d:MESSAGE:%s", 
                &DEST_ID, &SENDER_ID, &SENDER_CLK_INDEX, content) != 4)
    {
        printf("[handle_group_receive] sscanf() failed.\n");
        return;
    }
    printf("My ID: %d\n", self_id);
    printf("[handle_group_receive] \n\n");
    printf("\tReceived %d bytes\n", bytes_received);
    printf("\tSender IP: %s\n", sender_ip);
    printf("\t[Message] %s(%d Bytes).\n\n",clean_message, bytes_received);
}

char * clear_message(char *not_clean_message)
{
    memset(message, 0, BUFFER_SIZE);
    char *end = strstr(not_clean_message, "\n\n");
    if (end == NULL)
    {
        printf("[clear_message] end of message not found\n");
        return (0);
    }
    memcpy(message, not_clean_message, end - not_clean_message);
    return (message);
}


SOCKET group_multicast(SOCKET *mc_socket, char *multicast_ip, char * msg) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(multicast_ip, GROUP_PORT, &hints, &res)) != 0) {
        fprintf(stderr, "[group_multicast] getaddrinfo: %s\n", gai_strerror(status));
        return (error_return);
    }
	printf("==> [group_multicast] (%lu) sending (%s)...\n", strlen(msg), multicast_ip);

    // char message[32];
	// sprintf(message, "%d:%s:%d", head->ID, head->addr, head->leader);

    if (sendto(*mc_socket, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "[group_multicast] sendto failed: %s\n", strerror(errno));
		return (error_return);
    }

	printf("==> [group_multicast] sent (%lu bytes)\n", strlen(msg));
	// int yes = 1;
	// if (setsockopt(*mc_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	// 	fprintf(stderr, "[group_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
	// 	// handle error
	// }

	// CLOSESOCKET(*mc_socket);
	// *mc_socket = join_multicast(multicast_ip, GROUP_PORT);
    freeaddrinfo(res); // free the linked list
    return 0;
}



SOCKET join_multicast(char *multicast_ip, char * mPORT)
{
    struct addrinfo hints, *bind_addr;
    SOCKET mc_socket;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, mPORT, &hints, &bind_addr))
    {
        fprintf(stderr, "[join_multicast] getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }
	mc_socket = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol);
    if (!(ISVALIDSOCKET(mc_socket))){
        fprintf(stderr, "[join_multicast] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }

    if (bind(mc_socket, bind_addr->ai_addr, bind_addr->ai_addrlen) == -1){
        fprintf(stderr, "[join_multicast] bind() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	#if defined(_WIN32)
		if (setsockopt(mc_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0)	
	#else
    if (setsockopt(mc_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	#endif
    {
        fprintf(stderr, "[join_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }
	// char multicast_loop = 1;
	// if (setsockopt(mc_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop)) < 0) {
	// 	fprintf(stderr, "[join_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
	// 	return (error_return);
	// }
	// unsigned char ttl = 1; // Set the TTL to 1
	// if (setsockopt(mc_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
	// 	perror("setsockopt");
	// 	// handle error
	// }
    freeaddrinfo(bind_addr); // Free the linked-list
	printf("\n==> [join_multicast] joined multicast group (%s:%s)\n\n", multicast_ip, mPORT);
    return (mc_socket);
}



// Path: chat-c/client/client_s.c