/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/*
 * Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
 *
 * This work is subject to U.S. and international copyright laws and
 * treaties. No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged, condensed,
 * expanded, collected, compiled, linked, recast, transformed or adapted
 * without the prior written consent of Cisco Systems, Inc. Any use or
 * exploitation of this work without authorization could subject the
 * perpetrator to criminal and civil liability.
 */

/*
===================================================================
    This program setsa a MAC address on WAN interface. 
===================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>

/*
 * Defines and static variables
 */

/* #define ETHER_ADDR_LEN 6 */
/* #define ARPHRD_ETHER 1 */

/*
#define DEBUG(args...) fprintf(stderr, ## args)
*/
#define DEBUG(args...) syslog(LOG_INFO, ## args)

char command[32];

/*
 * Ethernet address string to binary conversion
 * strEth       string in xx:xx:xx:xx:xx:xx notation
 * binEth       binary data
 * return      TRUE if conversion was successful and FALSE otherwise
 */
int
ether_atoe(const char *strEth, unsigned char *binEth)
{
    char *c = (char *) strEth;
    int i;

    memset(binEth, 0, ETHER_ADDR_LEN);
    for (i = 0; (*c) && i < ETHER_ADDR_LEN; ++i,++c) {
        binEth[i] = (unsigned char) strtoul(c, &c, 16);
    }
    return (i == ETHER_ADDR_LEN);
}

int addr_set(const char *intf, const char *addr)
{
    int fd;
    struct ifreq ifr;

    if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        return -1;

    /*
     * Setting MAC address to interface.
     */ 
    snprintf(command, sizeof(command), "ifconfig %s down", intf);
    system(command); /* bring down interface */

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, intf, 8);

    ether_atoe(addr, ifr.ifr_hwaddr.sa_data);

    DEBUG("macclone: set %02x:%02x:%02x:%02x:%02x:%02x to %s\n",
            (int) ((unsigned char *) ifr.ifr_hwaddr.sa_data)[0], 
            (int) ((unsigned char *) ifr.ifr_hwaddr.sa_data)[1], 
            (int) ((unsigned char *) ifr.ifr_hwaddr.sa_data)[2], 
            (int) ((unsigned char *) ifr.ifr_hwaddr.sa_data)[3], 
            (int) ((unsigned char *) ifr.ifr_hwaddr.sa_data)[4], 
            (int) ((unsigned char *) ifr.ifr_hwaddr.sa_data)[5], 
            ifr.ifr_name);

    if (memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", ETHER_ADDR_LEN)) {
        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
        ioctl(fd, SIOCSIFHWADDR, &ifr);
    } else {
        DEBUG("macclone: invalid hardware address\n");
    }
    snprintf(command, sizeof(command), "ifconfig %s up", intf);
    system(command); /* bring up interface */

    close(fd);

    return 0;
}

int main(int argc, char *argv[])
{

    char *interface=argv[1];
    char *mac_addr=argv[2];

    if (argc <3) {
        printf("Usage: %s <interface>  <hw addr>\n",argv[0]);
        return 0;
    }
    addr_set((const char *)interface, (const char *)mac_addr);

    return 1;
}
