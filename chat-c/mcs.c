//===================================================== file = mclient.c =====
//=  A multicast client to receive multicast datagrams                       =
//============================================================================
//=  Notes:                                                                  =
//=    1) This program receives on a multicast address and outputs the       =
//=       received buffer to the screen.                                     =
//=    2) Conditionally compiles for Winsock and BSD sockets by setting the  =
//=       initial #define to WIN or BSD as appropriate.                      =
//=    3) The multicast group address is GROUP_ADDR.                         =
//=--------------------------------------------------------------------------=
//=  Build: bcc32 mclient.c or cl mclient.c wsock32.lib for Winsock          =
//=         gcc mclient.c -lsocket -lnsl for BSD                             =
//=--------------------------------------------------------------------------=
//=  History:  JNS (07/11/02) - Genesis                                      =
//============================================================================             // WIN for Winsock and BSD for BSD sockets

//----- Include files -------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <stdlib.h>         // Needed for memcpy()
#if defined (_WIN32)
  #include <winsock.h>      // Needed for all Windows stuff
#endif
#ifdef BSD
  #include <sys/types.h>    // Needed for system defined identifiers.
  #include <netinet/in.h>   // Needed for internet address structure.
  #include <sys/socket.h>   // Needed for socket(), bind(), etc...
  #include <arpa/inet.h>    // Needed for inet_ntoa()
  #include <fcntl.h>
  #include <netdb.h>
#endif

//----- Defines -------------------------------------------------------------
#define PORT_NUM         4444             // Port number used
#define GROUP_ADDR "225.1.1.1"            // Address of the multicast group

//===== Main program ========================================================
void main(void)
{
#ifdef WIN
  WORD wVersionRequested = MAKEWORD(1,1); // Stuff for WSA functions
  WSADATA wsaData;                        // Stuff for WSA functions
#endif
  unsigned int         multi_server_sock; // Multicast socket descriptor
  struct ip_mreq       mreq;              // Multicast group structure
  struct sockaddr_in   client_addr;       // Client Internet address
  unsigned int         addr_len;          // Internet address length
  unsigned char        buffer[256];       // Datagram buffer
  int                  retcode;           // Return code

#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif

  // Create a multicast socket and fill-in multicast address information
  multi_server_sock=socket(AF_INET, SOCK_DGRAM,0);
  mreq.imr_multiaddr.s_addr = inet_addr(GROUP_ADDR);
  mreq.imr_interface.s_addr = INADDR_ANY;

  // Create client address information and bind the multicast socket
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = INADDR_ANY;
  client_addr.sin_port = htons(PORT_NUM);
  retcode = bind(multi_server_sock,(struct sockaddr *)&client_addr,
                 sizeof(struct sockaddr));
  if (retcode < 0)
  {
    printf("*** ERROR - bind() failed with retcode = %d \n", retcode);
    return;
  }

  // Have the multicast socket join the multicast group
  retcode = setsockopt(multi_server_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
             (char*)&mreq, sizeof(mreq)) ;
  if (retcode < 0)
  {
    printf("*** ERROR - setsockopt() failed with retcode = %d \n", retcode);
    return;
  }

  // Set addr_len
  addr_len = sizeof(client_addr);

  while(1)
  {
    // Receive a datagram from the multicast server
    if( (retcode = recvfrom(multi_server_sock, buffer, sizeof(buffer), 0,
      (struct sockaddr *)&client_addr, &addr_len)) < 0){
        printf("*** ERROR - recvfrom() failed \n");
        exit(1);
      }

    // Output the received buffer to the screen as a string
    printf("%s\n",buffer);
  }

  // Close and clean-up
#ifdef WIN
  closesocket(multi_server_sock);
  WSACleanup();
#endif
#ifdef BSD
  close(multi_server_sock);
#endif
}