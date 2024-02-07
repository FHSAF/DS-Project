#include "client.h"

HoldBackQueue * handle_group_receive(SOCKET group_socket, HoldBackQueue *bufferHead)
{
    printf("\n=>=>[handle_group_receive] \n");

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
        fprintf(stderr, "=>=>=> [ERROR][handle_group_receive] recvfrom() failed. (%d)\n", GETSOCKETERRNO());
        return(0);
    }

    // Get sender's IP and port
    int res = getnameinfo((struct sockaddr*)&sender_addr, sender_addr_len, 
                            sender_ip, sizeof(sender_ip), sender_port, 
                            sizeof(sender_port), NI_NUMERICHOST | NI_NUMERICSERV);

    if (res != 0) {
        fprintf(stderr, "=>=>=> [ERROR][handle_group_receive] getnameinfo() failed: %s\n", gai_strerror(res));
        return(0);
    }

    char * clean_message = clear_message(Buffer);
    if (clean_message == NULL)
    {
        printf("[handle_group_receive] clean_message is NULL\n");
        return(0);
    }
    // printf recieved message
    if (sscanf(clean_message, "\n\tGROUP_ID:%d\n\tSENDER_ID:%d\n\tSENDER_CLK_INDEX:%d\n\tDEPS:%s\n\tMESSAGE:%s", 
                &dest_id, &sender_id, &sender_clk_index, sender_dep_str, content) != 5)
        printf("[handle_group_receive] sscanf() failed.\n");
    
    // Multicast
    append_to_holdback_queue(&bufferHead, clean_message);
    
    bufferHead = deliver_messages(bufferHead, clean_message);
    // Multicast
    return (bufferHead);
}

