#include <error.h>
#include <stdint.h>
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

#define TAP_NET_DEV "tap-left"

struct arp_ipv4 {
    struct arphdr ah;
    uint8_t arp_sha[ETH_ALEN];
    uint8_t arp_spa[4];
    uint8_t arp_tha[ETH_ALEN];
    uint8_t arp_tpa[4];
};

void make_ether_frame(char* buf) {
    const uint8_t ether_broadcast_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const uint8_t src_tap_addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    const uint8_t dest_tap_addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x66};

    struct ether_header* eh = (struct ether_header*)buf;
    memcpy(eh->ether_dhost, ether_broadcast_addr, ETH_ALEN);
    memcpy(eh->ether_shost, src_tap_addr, ETH_ALEN);
    eh->ether_type = htons(ETHERTYPE_ARP);

    struct arp_ipv4* arp = (struct arp_ipv4*)(buf + sizeof(struct ether_header));
    arp->ah.ar_hrd = htons(ARPHRD_ETHER);
    arp->ah.ar_pro = htons(ETHERTYPE_IP);
    arp->ah.ar_hln = ETH_ALEN;
    arp->ah.ar_pln = 4;
    arp->ah.ar_op = htons(ARPOP_REQUEST);
    memcpy(arp->arp_sha, src_tap_addr, ETH_ALEN);
    memcpy(arp->arp_spa, "\xc0\xa8\x00\x01", 4);
    memcpy(arp->arp_tha, dest_tap_addr, ETH_ALEN);
    memcpy(arp->arp_tpa, "\xc0\xa8\x00\x02", 4);
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

int main() {
    int tap_fd;
    int bytes;
    char buf[sizeof(struct ether_header) + sizeof(struct arp_ipv4)] = {0};

    make_ether_frame(buf);

    tap_fd = tap_sock_open(TAP_NET_DEV);
    if (tap_fd > 0) {
        printf("TAP %s opened!\n", TAP_NET_DEV);
    } else {
        printf("TAP %s failed!\n", TAP_NET_DEV);
        return 1;
    }

    while (1) {
        printf("Trying to send ARP request... %ld\n", sizeof(buf));
        bytes = write(tap_fd, buf, sizeof(buf));
        // bytes = sendto(tap_fd, buf, sizeof(buf), 0, NULL, 0);
        sleep(4);
    }
    close(tap_fd);
}