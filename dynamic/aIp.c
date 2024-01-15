#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <netinet/udp.h>
////#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>

//#include <dhcpclient/dhcp.h>

void print_ip(struct in_addr addr) {
    printf("IP Address: %s\n", inet_ntoa(addr));
}

int main() {
    int sockfd;
    struct sockaddr_in listen_addr;
    char buffer[1024];

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up listener address
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(0); // Let the system choose a random port

    // Bind the socket to the specified address and port
    if (bind(sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Configure DHCP client
    struct dhcp_config config;
    dhcp_init_config(&config);
    config.client_mac = (uint8_t*)"\x00\x01\x02\x03\x04\x05"; // Replace with your MAC address
    config.socket = sockfd;
    config.timeout = 10; // Timeout for DHCP lease (in seconds)

    // Run DHCP client
    struct in_addr ip_address;
    if (dhcp_run_client(&config, &ip_address) != DHCP_SUCCESS) {
        fprintf(stderr, "DHCP client failed\n");
        exit(EXIT_FAILURE);
    }

    // Print the obtained IP address
    print_ip(ip_address);

    // Now you can proceed with your socket code

    // ...

    close(sockfd);
    return 0;
}
