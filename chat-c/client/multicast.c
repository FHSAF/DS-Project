#include "client.h"

SOCKET group_multicast(SOCKET *mc_socket, char *multicast_ip, char *multicast_port, char * msg) 
{
    printf("\n=>=>[group_multicast] \n");

    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = AI_PASSIVE;
    printf("==> [PROGRESS][group_multicast] setting up getaddrinfo(%s:%s)...\n", multicast_ip, multicast_port);
    if ((status = getaddrinfo(multicast_ip, multicast_port, &hints, &res)) != 0) {
        fprintf(stderr, "=>=>=> [ERROR][group_multicast] getaddrinfo: %s\n", gai_strerror(status));
        return (error_return);
    }
	printf("==> [PROGRESS][group_multicast] sending (%lu Bytes) to (%s:%s)...\n", strlen(msg), multicast_ip, multicast_port);

    int bytes_sent;
    if ((bytes_sent = sendto(*mc_socket, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen)) == -1) {
        fprintf(stderr, "=>=>=> [ERROR][group_multicast] sendto failed: %s\n", strerror(errno));
		return (error_return);
    }

	printf("==> [PROGRESS][group_multicast] sent (%d bytes).\n", bytes_sent);

    freeaddrinfo(res); // free the linked list
    return 0;
}



SOCKET join_multicast(char *multicast_ip, char * mPORT)
{
    printf("\n=>=>[join_multicast] \n");

    struct addrinfo hints, *bind_addr;
    SOCKET mc_socket;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    printf("==> [PROGRESS][join_multicast] setting up getaddrinfo(%s:%s)...\n", multicast_ip, mPORT);

    if (getaddrinfo(NULL, mPORT, &hints, &bind_addr))
    {
        fprintf(stderr, "=>=>=> [ERROR][join_multicast] getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }
	mc_socket = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol);
    if (!(ISVALIDSOCKET(mc_socket))){
        fprintf(stderr, "=>=>=> [ERROR][join_multicast] socket() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }

    if (bind(mc_socket, bind_addr->ai_addr, bind_addr->ai_addrlen) == -1){
        fprintf(stderr, "=>=>=> [ERROR][join_multicast] bind() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }

    printf("==> [PROGRESS][join_multicast] joining to (%s:%s)...\n", multicast_ip, mPORT);

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	#if defined(_WIN32)
		if (setsockopt(mc_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0)	
	#else
    if (setsockopt(mc_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	#endif
    {
        fprintf(stderr, "=>=>=> [ERROR][join_multicast] setsockopt() failed. (%d)\n", GETSOCKETERRNO());
        return (error_return);
    }
    freeaddrinfo(bind_addr); // Free the linked-list
	printf("==> [PROGRESS][join_multicast] joined multicast group (%s:%s).\n", multicast_ip, mPORT);
    return (mc_socket);
}

