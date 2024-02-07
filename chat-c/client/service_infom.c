#include "client.h"

char * get_service_info_mcast(const char *host, const char *port, const char *device_ip, const char *device_port)
{
    printf("\n=>=>[get_service_info_mcast] \n");

    char mCastIP[16];
    memset(mCastIP, 0, sizeof(mCastIP));
    char mCastPort[6];
    memset(mCastPort, 0, sizeof(mCastPort));
    char localIP[16];
    memset(localIP, 0, sizeof(localIP));
    char localPort[6];
    memset(localPort, 0, sizeof(localPort));

    memcpy(mCastIP, host, strlen(host));
    memcpy(mCastPort, port, strlen(port));
    memcpy(localIP, device_ip, strlen(device_ip));
    memcpy(localPort, device_port, strlen(device_port));
    
    SOCKET socket_listen = join_multicast(mCastIP, mCastPort);
    if (!(ISVALIDSOCKET(socket_listen)))
    {
        fprintf(stderr, "=>=>=> [ERROR] join_multicast() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }
    memset(Buffer, 'x', sizeof(Buffer));
    Buffer[sizeof(Buffer) - 1] = '\0';
    memset(message, 0, BUFFER_SIZE);
    sprintf(message, "NEW_CLIENT:%s\n\n", localPort);
    memcpy(Buffer, message, strlen(message));

    group_multicast(&socket_listen, mCastIP, mCastPort, Buffer);

    CLOSESOCKET(socket_listen);
    SOCKET tcp_socket = get_tcp_socket(localIP, localPort);
    if (!(ISVALIDSOCKET(tcp_socket)))
    {
        fprintf(stderr, "=>=>=> [ERROR] get_tcp_socket() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }
    SOCKET new_socket = accept(tcp_socket, NULL, NULL);
    if (new_socket == -1)
    {
        fprintf(stderr, "=>=>=> [ERROR] accept() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }
    memset(Buffer, 0, sizeof(Buffer));
    if (recv(new_socket, Buffer, BUFFER_SIZE, 0) == -1)
    {
        fprintf(stderr, "=>=>=> [ERROR] recv() failed. (%d)\n", GETSOCKETERRNO());
        return (0);
    }
    char *clean_message = clear_message(Buffer);
    if (clean_message == NULL)
    {
        printf("[get_service_info] clean_message is NULL\n");
        return (0);
    }
    CLOSESOCKET(new_socket);
    CLOSESOCKET(tcp_socket);
    return (clean_message);
}