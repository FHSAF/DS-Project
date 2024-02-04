#include "client.h"

SOCKET group_multicast(SOCKET *mc_socket, char *multicast_ip, char *multicast_port, char * msg) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = AI_PASSIVE;
    printf("==> [group_multicast] setting up getaddrinfo().. (%s:%s)\n", multicast_ip, multicast_port);
    if ((status = getaddrinfo(multicast_ip, multicast_port, &hints, &res)) != 0) {
        fprintf(stderr, "[group_multicast] getaddrinfo: %s\n", gai_strerror(status));
        return (error_return);
    }
	printf("==> [group_multicast] (%lu) sending (%s)...\n", strlen(msg), multicast_ip);

    if (sendto(*mc_socket, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "[group_multicast] sendto failed: %s\n", strerror(errno));
		return (error_return);
    }

	printf("==> [group_multicast] sent (%lu bytes)\n", strlen(msg));

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

    printf("==> [join_multicast] setting up getaddrinfo().. (%s:%s)\n", multicast_ip, mPORT);

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

    printf("==> [join_multicast] joining to (%s:%s)\n", multicast_ip, mPORT);

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
    freeaddrinfo(bind_addr); // Free the linked-list
	printf("\n==> [join_multicast] joined multicast group (%s:%s)\n\n", multicast_ip, mPORT);
    return (mc_socket);
}

