#include "client.h"

char Buffer[BUFFER_SIZE];
char message[BUFFER_SIZE];
char userInput[BUFFER_SIZE - 127];

char GROUP_IP[16];
char GROUP_PORT[6];
char MY_SERVER_IP[16];
char MY_SERVER_PORT[6];

int mode = 0;
int GROUP_ID = 0;
int SELF_ID = 0;


// Multicast
int CLK_INDEX = 0;
int SEND_SEQ = 0;
int DELIVERED[MAX_GROUP_SIZE];
int DEPENDENCY[MAX_GROUP_SIZE];
char DEPENDENCY_STR[MAX_GROUP_SIZE * 5];


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
    // Multicast
    
    //
    fd_set master;
    FD_ZERO(&master);
    struct holdBackQueue *bufferHead = NULL;

    char *service_info = get_service_info_mcast(SERVICE_MULTICAST_IP, SERVICE_MULTICAST_PORT, argv[1], argv[2]);
    // char *service_info_tcp = get_service_info(argv[1], argv[2], argv[3]);
    if (service_info == NULL)
    {
        printf("[main] get_service_info() failed.\n");
        return (0);
    }
    printf("[main] service_info: %s\n", service_info);
    int sPORT;
    if (sscanf(service_info, "LEADER:YOUR_ID:%d:SERVER_IP:%15[^:]:SERVER_PORT:%d", &SELF_ID, MY_SERVER_IP, &sPORT) != 3)
    {
        printf("[main] sscanf() failed.\n");
        return (0);
    }
    sprintf(MY_SERVER_PORT, "%d", sPORT);
    printf("[main] MY_SERVER_IP: %s\n", MY_SERVER_IP);
    printf("[main] MY_SERVER_PORT: %s\n", MY_SERVER_PORT);
    printf("[main] SELF_ID: %d\n", SELF_ID);

    SOCKET socket_peer = connect_toserver(MY_SERVER_IP, MY_SERVER_PORT);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "connect_toserver() failed.\n");
        return 1;
    }
    FD_SET(socket_peer, &master);
    
    memset(message, 0, BUFFER_SIZE);
    // sprintf(message, "IM_NEW:%d\n\n", SELF_ID);
    sprintf(message, "CLIENT:%d:%d:GET_INDEX\n\n", SELF_ID, 0);

    memset(Buffer, 'x', sizeof(Buffer)-1);
    Buffer[sizeof(Buffer) - 1] = '\0';
    memcpy(Buffer, message, strlen(message));

    printf("Sending ID request...\n");

    if (send(socket_peer, Buffer, BUFFER_SIZE, 0) < 1) {
        printf("Sending ID request failed.\n");
        CLOSESOCKET(socket_peer);
        #if defined(_WIN32)
            WSACleanup();
        #endif
        return (0);
    }
    printf("==>[INFO] INDEX request (%lu) (%s) sent.\n", strlen(Buffer) + 1, message);

    memset(message, 0, BUFFER_SIZE);
    memset(Buffer, 0, sizeof(Buffer));
    int received_bytes = 0;

    if ((received_bytes = recv(socket_peer, Buffer, BUFFER_SIZE, 0)) < 1)
    {
        printf("Receiving ID failed.\n");
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
    if (sscanf(message, "GROUP_IP:%15[^:]:GROUP_PORT:%d:INDEX:%d", 
                    GROUP_IP, &mPORT, &CLK_INDEX) != 3)
    {
        printf("sscanf() failed.\n");
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
            bufferHead = handle_group_receive(group_socket, bufferHead);
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
            char *clean_message = clear_message(Buffer);
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
                if (!fgets(userInput, BUFFER_SIZE-127, stdin)) break;
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
                if (!fgets(userInput, BUFFER_SIZE-127, stdin)) break;
                if (strlen(userInput) <= 1) break;

                if ((userInput[0] == '0') && (strlen(userInput) == 2)){ 
                    mode = 0;
                    printf("==> Mode changed to Unicast.\n");
                    printf("[USAGE] Enter <id> <message> to send 1 for multicast (empty line to quit):\n----->");
                    fflush(stdout);
                    continue;}
                // Getting string representation of dependencies
                memcpy(DEPENDENCY, DELIVERED, sizeof(DEPENDENCY));
                DEPENDENCY[CLK_INDEX] = SEND_SEQ;
                memset(DEPENDENCY_STR, 0, sizeof(DEPENDENCY_STR));
                get_deps_str(DEPENDENCY, MAX_GROUP_SIZE, DEPENDENCY_STR);

                sprintf(message, "\n\tGROUP_ID:%d\n\tSENDER_ID:%d\n\tSENDER_CLK_INDEX:%d\n\tDEPS:%s\n\tMESSAGE:%s\n\n", 
                                                GROUP_ID, SELF_ID, CLK_INDEX, DEPENDENCY_STR, userInput);
                SEND_SEQ++;
                // BUFFER_SIZE-1 bytes should be sent, and be null-terminated
                memset(Buffer, 'x', BUFFER_SIZE-1);
                Buffer[BUFFER_SIZE - 1] = '\0';
                memcpy(Buffer, message, strlen(message));
                // 
                
                printf("Sending: %lu bytes %s", strlen(Buffer), message);

                // FD_CLR(group_socket, &master);

                group_multicast(&group_socket, GROUP_IP, GROUP_PORT, Buffer);
                // FD_SET(group_socket, &master);

                printf("[USAGE] Enter message to multicast, 0 for Unicast (empty line to quit):\n----->");
                fflush(stdout);
            }
        }
    } //end while(1)

    printf("Closing socket...\n");
    CLOSESOCKET(group_socket);
    CLOSESOCKET(socket_peer);
    free_holdback_queue(bufferHead);

    #if defined(_WIN32)
        WSACleanup();
    #endif

    printf("Finished.\n");
    return 0;
}

HoldBackQueue* deliver_messages(HoldBackQueue *head, char *clean_message)
{
    int dest_id = 0;
    int sender_id = 0;
    int sender_clk_index = 0;
    char content[BUFFER_SIZE];
    char sender_dep_str[MAX_GROUP_SIZE+1];
    // int sender_dep[MAX_GROUP_SIZE];
    sender_dep_str[MAX_GROUP_SIZE] = '\0';

    if (sscanf(clean_message, "\n\tGROUP_ID:%d\n\tSENDER_ID:%d\n\tSENDER_CLK_INDEX:%d\n\tDEPS:%s\n\tMESSAGE:%s", 
                &dest_id, &sender_id, &sender_clk_index, sender_dep_str, content) != 5)
    {
        printf("[deliver_messages] sscanf() failed.\n");
        return(0);
    }
    
    HoldBackQueue *current = head;
    if (current->next == NULL)
        printf("[deliver_messages] Queue next empty.\n");
    
    while (current != NULL)
    {
        
        if (array_compare(current->dependency, DELIVERED) <= 0)
        {
            printf("[deliver_messages] Delivering message: \n%s.\n\n", current->content);
            printf("[deliver_messages] Updating DELIVERED array.\n");
            DELIVERED[current->clk_index] += 1;
            int temp_clk = current->clk_index;
            current = current->next;
            remove_from_holdback_queue(&head, temp_clk);
            if (head == NULL)
                printf("[deliver_messages] Queue is empty.\n");
            printf("\n\t==================== HOLD BACK ====================\n");
            //print_holdback_queue(head);
            printf("\t==================== HOLD BACK ====================\n\n");
        } else {
            printf("[deliver_messages]not delivered.\n");
            printf("\n\t==================== HOLD BACK ====================\n");
            print_holdback_queue(head);
            printf("\t==================== HOLD BACK ====================\n\n");
            return(head);
        }
    }

    return(head);
}

char * get_service_info(const char *host, const char *port, const char *device_ip)
{
    printf("[UDP] Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
    struct addrinfo *udp_bind_address;
    if (getaddrinfo(device_ip, "41411", &hints, &udp_bind_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(udp_bind_address->ai_addr, udp_bind_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s:%s\n", address_buffer, service_buffer);

    printf("[UDP] Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(udp_bind_address->ai_family, udp_bind_address->ai_socktype, udp_bind_address->ai_protocol);
    if (!(ISVALIDSOCKET(socket_listen)))
    {
        fprintf(stderr, "[UDP] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }
    
    memset(message, 0, BUFFER_SIZE);
    sprintf(message, "NEW_CLIENT\n\n");

    memset(Buffer, 'x', sizeof(Buffer)-1);
    Buffer[sizeof(Buffer) - 1] = '\0';
    memcpy(Buffer, message, strlen(message));

    printf("Sending ID request...\n");

    printf("[UDP] Configuring remote address...\n");
    struct addrinfo hints2;
    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family = AF_INET;
	hints2.ai_socktype = SOCK_DGRAM;
	hints2.ai_flags = AI_PASSIVE;
    struct addrinfo *remote_address;
    if (getaddrinfo(host, port, &hints2, &remote_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }

    printf("Remote address is: ");
    char address_buffer_remote[100];
    char service_buffer_remote[100];
    getnameinfo(remote_address->ai_addr, remote_address->ai_addrlen,
            address_buffer_remote, sizeof(address_buffer_remote),
            service_buffer_remote, sizeof(service_buffer_remote),
            NI_NUMERICHOST);
    printf("%s:%s\n", address_buffer_remote, service_buffer_remote);

    printf("[UDP] Binding socket to local address...\n");
	if (bind(socket_listen, udp_bind_address->ai_addr, udp_bind_address->ai_addrlen))
	{
		fprintf(stderr, "[UDP] bind() failed. (%d)\n", GETSOCKETERRNO());
		CLOSESOCKET(socket_listen);
		return (0);
	}



    if (sendto(socket_listen, message, BUFFER_SIZE, 0, remote_address->ai_addr, remote_address->ai_addrlen) == -1) {
        fprintf(stderr, "sendto() failed. (%d)\n", GETSOCKETERRNO());
        return (0);}

	struct sockaddr_storage sender_address;
	socklen_t sender_len = sizeof(sender_address);
    
    memset(Buffer, 0, sizeof(Buffer));

    printf("[UDP] Wiating for response form elader...\n");
    if (recvfrom(socket_listen, Buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_address, &sender_len) == -1) {
        fprintf(stderr, "recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }
    char *clean_message = clear_message(Buffer);
    if (clean_message == NULL)
    {
        printf("[get_service_info] clean_message is NULL\n");
        return (0);
    }
	freeaddrinfo(udp_bind_address);
    freeaddrinfo(remote_address);
    CLOSESOCKET(socket_listen);
	return(clean_message);
}



