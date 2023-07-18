#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define TAP_NET_DEV "tap-right"

struct arp_ipv4 {
    struct arphdr ah;
    unsigned char arp_sha[ETH_ALEN];
    unsigned char arp_spa[4];
    unsigned char arp_tha[ETH_ALEN];
    unsigned char arp_tpa[4];
};

int tap_sock_open(const char* ifname) {
    int sock;
    int res;
    struct sockaddr_ll sock_ll;

    int ifindex = if_nametoindex(ifname);

    if (ifindex == 0) {
        return -1;
    }

    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        return sock;
    }

    memset((void*)&sock_ll, 0x00, sizeof(sock_ll));
    sock_ll.sll_family = AF_PACKET;
    sock_ll.sll_ifindex = ifindex;
    // sock_ll.sll_protocol = htons(ETH_P_ALL);

    res = bind(sock, (struct sockaddr*)&sock_ll, sizeof(sock_ll));
    if (res < 0) {
        perror("bind");
        return res;
    }

    return sock;
}

// int main() {
//     int tap_fd;
//     int bytes;
//     char buf[sizeof(struct ether_header) + sizeof(struct arp_ipv4)] = {0};

//     tap_fd = tap_sock_open(TAP_NET_DEV);
//     if (tap_fd > 0) {
//         printf("TAP %s opened!\n", TAP_NET_DEV);
//     } else {
//         printf("TAP %s failed!\n", TAP_NET_DEV);
//         return 1;
//     }

//     while (1) {
//         // bytes = write(tap_fd, buf, sizeof(buf));
//         // bytes = sendto(tap_fd, buf, sizeof(buf), 0, NULL, 0);
//         bytes = recvfrom(tap_fd, buf, sizeof(buf), 0, NULL, 0);
//         // Display all elements of the buffer
//         printf("Rx %d bytes\n", bytes);
//         for (int i = 0; i < sizeof(buf); i++) {
//             printf("%02x ", buf[i]);
//         }
//     }
//     close(tap_fd);
// }

int main() {
    int fd1, len;
    char buf[32];

    fd1 = tap_sock_open("tap-right");
    if (fd1 > 0) {
        printf("interface %s opened!\n", TAP_NET_DEV);
    } else {
        printf("interface %s failed!\n", TAP_NET_DEV);
        return 1;
    }
    memset(buf, 0, sizeof(buf));

    while (1) {
        len = read(fd1, buf, sizeof(buf));
        if (len > 0) {
            printf("Rx %d bytes\n", len);
            for (int i = 0; i < len; i++) {
                printf("%02x ", buf[i]);
            }
            printf("\n");
        }
        len = 0;
        memset(buf, 0, sizeof(buf));
    }
    close(fd1);
}