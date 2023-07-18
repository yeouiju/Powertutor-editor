/* The Linux Channel
 * Code for Video Episode: 0x1b0 Linux TUN/TAP interfaces creation via C code -
 * Ep5 Author: Kiran Kankipati Updated: 14-Apr-2019
 */

#include <ctype.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* create via CLI: (if not via this code)
# ip tuntap help
Usage: ip tuntap { add | del } [ dev PHYS_DEV ]
[ mode { tun | tap } ] [ user USER ] [ group GROUP ]
[ one_queue ] [ pi ] [ vnet_hdr ]

Where: USER  := { STRING | NUMBER }
GROUP := { STRING | NUMBER }
*/
/*
   sudo ip addr add 192.168.3.1/24 dev mytuntap1
   sudo ip link set dev mytuntap1 up (or) sudo ifconfig mytuntap1 up
   ping 192.168.3.1
   */

#define TAP_NET_DEV "tap-right"

int tun_tap_alloc(char *name, int type) { // type: IFF_TUN or IFF_TAP
  struct ifreq ifr;
  int fd, ret;

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    printf("error: open()\n");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = type;
  strncpy(ifr.ifr_name, name, IFNAMSIZ);

  if ((ret = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    close(fd);
    return ret;
  }

  return fd;
}

void main() {
  int fd1, len;
  char buf[2000];

  fd1 = tun_tap_alloc(TAP_NET_DEV, IFF_TAP);
  if (fd1 > 0) {
    printf("interface %s created!\n", TAP_NET_DEV);
  } else {
    printf("interface %s creation failed!\n", TAP_NET_DEV);
    return;
  }

  while (1) {
    len = read(fd1, buf, sizeof(buf));

    printf("Rx %d bytes \n", len);
    len = 0;
  }
  close(fd1);
}
