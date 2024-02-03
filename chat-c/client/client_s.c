#include "client.h"

char Buffer[BUFFER_SIZE];
char message[BUFFER_SIZE];
char userInput[BUFFER_SIZE - 100];

char GROUP_IP[16];
char GROUP_PORT[6];
char MY_SERVER_IP[16];
char MY_SERVER_PORT[6];

int mode = 0;
int GROUP_ID = 0;
int SELF_ID = 0;
char *clean_message = NULL;


// Multicast
int CLK_INDEX = 0;
int SEND_SEQ = 0;
int DELIVERED[MAX_GROUP_SIZE];
int DEPENDENCY[MAX_GROUP_SIZE];
char DEPENDENCY_STR[MAX_GROUP_SIZE+1];

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
    HoldBackQueue *bufferHead = NULL;
    //
    fd_set master;
    FD_ZERO(&master);

    char *service_info = get_service_info(argv[1], argv[2], argv[3]);
    if (service_info == NULL)
    {
        printf("[main] get_service_info() failed.\n");
        return (0);
    }
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
    
    // FD_SET(udp_socket, &master);


    SOCKET socket_peer = connect_toserver(MY_SERVER_IP, MY_SERVER_PORT);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "connect_toserver() failed.\n");
        return 1;
    }
    FD_SET(socket_peer, &master);
    
    memset(message, 0, BUFFER_SIZE);
    sprintf(message, "IM_NEW:%d\n\n", SELF_ID);

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
    if (sscanf(message, "GROUP_IP:%15[^:]:GROUP_PORT:%d:INDEX:%d", 
                    GROUP_IP, &mPORT, &CLK_INDEX) != 3)
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
            handle_group_receive(group_socket, bufferHead);
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
                // Getting string representation of dependencies
                memcpy(DEPENDENCY, DELIVERED, sizeof(DEPENDENCY));
                DEPENDENCY[CLK_INDEX] = SEND_SEQ;
                for (int i = 0; i < MAX_GROUP_SIZE; i++) {
                    DEPENDENCY_STR[i] = DEPENDENCY[i] + '0';
                }
                DEPENDENCY_STR[MAX_GROUP_SIZE] = '\0';

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

void handle_group_receive(SOCKET group_socket, HoldBackQueue *bufferHead)
{
    int dest_id = 0;
    int sender_id = 0;
    int sender_clk_index = 0;
    char content[BUFFER_SIZE];
    char sender_dep_str[MAX_GROUP_SIZE+1];
    // int sender_dep[MAX_GROUP_SIZE];
    sender_dep_str[MAX_GROUP_SIZE] = '\0';

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
    // printf recieved message
    if (sscanf(clean_message, "\n\tGROUP_ID:%d\n\tSENDER_ID:%d\n\tSENDER_CLK_INDEX:%d\n\tDEPS:%s\n\tMESSAGE:%s", 
                &dest_id, &sender_id, &sender_clk_index, sender_dep_str, content) != 5)
        printf("[handle_group_receive] sscanf() failed.\n");
    
    // for (int i = 0; i < MAX_GROUP_SIZE; i++)
    // {
    //     sender_dep[i] = sender_dep_str[i] - '0';
    // }
    // Multicast
    append_to_holdback_queue(&bufferHead, clean_message);
    deliver_messages(bufferHead, clean_message);
    // Multicast
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

void append_to_holdback_queue(HoldBackQueue **head, char *clean_message)
{
    char sender_dep_str[MAX_GROUP_SIZE+1];
    sender_dep_str[MAX_GROUP_SIZE] = '\0';
    char content[BUFFER_SIZE];
    int sender_dep[MAX_GROUP_SIZE];
    memset(content, 0, sizeof(content));
    

    HoldBackQueue *newNode = (HoldBackQueue *)malloc(sizeof(HoldBackQueue));
    if (newNode == NULL)
    {
        printf("[append_to_holdback_queue] malloc() failed.\n");
        return;
    }
    newNode->next = NULL;
    newNode->clk_index = 0;
    newNode->id = 0;
    
    memset(newNode->content, 0, BUFFER_SIZE);
    memset(newNode->dependency, 0, sizeof(newNode->dependency));
    if (sscanf(clean_message, "\n\tGROUP_ID:%d\n\tSENDER_ID:%d\n\tSENDER_CLK_INDEX:%d\n\tDEPS:%s\n\tMESSAGE:%s", 
    &newNode->id, &newNode->id, &newNode->clk_index, sender_dep_str, content) != 5)
    {
        printf("[append_to_holdback_queue] sscanf() failed.\n");
        return;
    }
    for (int i = 0; i < MAX_GROUP_SIZE; i++)
    {
        sender_dep[i] = sender_dep_str[i] - '0';
    }

    memcpy(newNode->dependency, sender_dep, sizeof(sender_dep));
    memcpy(newNode->content, clean_message, strlen(clean_message) + 1);

    if (*head == NULL)
    {
        *head = newNode;
        return;
    }
    HoldBackQueue *current = *head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = newNode;
}

void deliver_messages(HoldBackQueue *head, char *clean_message)
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
        return;
    }
    
    HoldBackQueue *current = head;
    
    while (current != NULL)
    {
        if (array_compare(current->dependency, DELIVERED) <= 0)
        {
            printf("[deliver_messages] Delivering message: \n%s.\n\n", current->content);
            printf("[deliver_messages] Updating DELIVERED array.\n");
            DELIVERED[current->clk_index] += 1;
            remove_from_holdback_queue(&head, current->clk_index);
            printf("\n\t==================== HOLD BACK ====================\n");
            print_holdback_queue(head);
            printf("\t==================== HOLD BACK ====================\n\n");
        } else {
            printf("[deliver_messages] Message (%s) not delivered.\n", current->content);
            return;
        }
        current = current->next;
    }

    return;
}

void remove_from_holdback_queue(HoldBackQueue **head, int clk_index)
{
    if (*head == NULL)
    {
        printf("[remove_from_holdback_queue] Queue is empty.\n");
        return;
    }
    if ((*head)->clk_index == clk_index)
    {
        HoldBackQueue *temp = *head;
        *head = (*head)->next;
        free(temp);
        return;
    }
    HoldBackQueue *current = *head;
    while (current->next != NULL)
    {
        if (current->next->clk_index == clk_index)
        {
            HoldBackQueue *temp = current->next;
            current->next = current->next->next;
            free(temp);
            return;
        }
        current = current->next;
    }
    printf("[remove_from_holdback_queue] Message not found.\n");
    return;
}

int array_compare(int *arr1, int *arr2)
{
    for (int i = 0; i < MAX_GROUP_SIZE; i++)
    {
        if (arr1[i] != arr2[i])
        {
            if (arr1[i] > arr2[i])
                return (1);
        }
    }
    return (0);
}

void print_holdback_queue(HoldBackQueue *head)
{
    HoldBackQueue *current = head;
    while (current != NULL)
    {
        printf("\n%s\n\n", current->content);
        current = current->next;
    }
    return;
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

    // #if defined(_WIN32)
    // char broadcastEnable = '1';
    // #else
    // int broadcastEnable = 1;
    // #endif
	// printf("[UDP] enabling broadcast...\n");
    // if (setsockopt(socket_listen, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1)
    // {
    //     fprintf(stderr, "[UDP] setsocktopt() failed. (%d)\n", GETSOCKETERRNO());
	// 	CLOSESOCKET(socket_listen);
    //     return (0);
    // }

	struct sockaddr_storage sender_address;
	socklen_t sender_len = sizeof(sender_address);
    
    memset(Buffer, 0, sizeof(Buffer));
    char *clean_message = NULL;
    printf("[UDP] Wiating for response form elader...\n");
    if (recvfrom(socket_listen, Buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_address, &sender_len) == -1) {
        fprintf(stderr, "recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }
    clean_message = clear_message(Buffer);
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