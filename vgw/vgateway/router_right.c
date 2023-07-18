#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define TAP_NET_DEV "tap-right"

struct arp_ipv4 {
    struct arphdr ah;
    uint8_t arp_sha[ETH_ALEN];
    uint8_t arp_spa[4];
    uint8_t arp_tha[ETH_ALEN];
    uint8_t arp_tpa[4];
};

void make_ether_frame_arp_reply(uint8_t* buf) {
    // const uint8_t ether_broadcast_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const uint8_t src_tap_addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x66};
    const uint8_t dest_tap_addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

    struct ether_header* eh = (struct ether_header*)buf;
    memcpy(eh->ether_shost, src_tap_addr, ETH_ALEN);
    memcpy(eh->ether_dhost, dest_tap_addr, ETH_ALEN);
    eh->ether_type = htons(ETHERTYPE_ARP);

    struct arp_ipv4* arp = (struct arp_ipv4*)(buf + sizeof(struct ether_header));
    arp->ah.ar_hrd = htons(ARPHRD_ETHER);
    arp->ah.ar_pro = htons(ETHERTYPE_IP);
    arp->ah.ar_hln = ETH_ALEN;
    arp->ah.ar_pln = 4;
    arp->ah.ar_op = htons(ARPOP_REPLY);
    memcpy(arp->arp_sha, src_tap_addr, ETH_ALEN);
    memcpy(arp->arp_spa, "\x0a\x01\x01\x02", 4);
    memcpy(arp->arp_tha, dest_tap_addr, ETH_ALEN);
    memcpy(arp->arp_tpa, "\x0a\x01\x01\x01", 4);
}

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

void dump_hex(const void* data, size_t size) {
    for (int i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char*)data)[i]);

        if ((i + 1) % 8 == 0 || i + 1 == size) {
            printf(" ");
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
    }
    printf("\n");
}

int main() {
    int tap_fd;
    int bytes;
    uint8_t buf[sizeof(struct ether_header) + sizeof(struct arp_ipv4)] = {0};

    tap_fd = tap_sock_open(TAP_NET_DEV);
    if (tap_fd > 0) {
        printf("TAP %s opened!\n", TAP_NET_DEV);
    } else {
        printf("TAP %s failed!\n", TAP_NET_DEV);
        return 1;
    }

    struct in_addr addr;
    while (1) {
        // bytes = sendto(tap_fd, buf, sizeof(buf), 0, NULL, 0);
        bytes = recvfrom(tap_fd, buf, sizeof(buf), 0, NULL, 0);

        if (bytes < 0)
            continue;

        // Display all elements of the buffer
        struct arp_ipv4* arp = (struct arp_ipv4*)(buf + sizeof(struct ether_header));
        addr.s_addr = *(uint32_t*)arp->arp_spa;
        printf("\nRx %d bytes from %s\n", bytes, inet_ntoa(addr));
        dump_hex(buf, sizeof(buf));
        fflush(stdout);

        // Make ARP reply
        printf("Trying to send ARP reply... %ld bytes\n", sizeof(buf));
        memset(buf, 0, sizeof(buf));
        make_ether_frame_arp_reply(buf);
        bytes = sendto(tap_fd, buf, sizeof(buf), 0, NULL, 0);
    }
    close(tap_fd);
}
