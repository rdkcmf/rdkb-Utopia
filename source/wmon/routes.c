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
    This programs will manipulate routing tables 
===================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <sys/syslog.h>

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

#define DEBUG(args...) syslog(LOG_INFO, ## args)

int
ifconfig(char *name, int flags, char *addr, char *netmask)
{
        int s;
        struct ifreq ifr;
        struct in_addr in_addr, in_netmask, in_broadaddr;

        DEBUG("ifconfig(): name=[%s] flags=[%s] addr=[%s] netmask=[%s]\n", name, flags == IFUP ? "IFUP" : "0", addr, netmask);

        /* Open a raw socket to the kernel */
        if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                goto err;

        /* Set interface name */
        strncpy(ifr.ifr_name, name, IFNAMSIZ);

        /* Set interface flags */
        ifr.ifr_flags = flags; 
        if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
                goto err;

        /* Set IP address */
        if (addr) {
                inet_aton(addr, &in_addr);
                sin_addr(&ifr.ifr_addr).s_addr = in_addr.s_addr;
                ifr.ifr_addr.sa_family = AF_INET;
                if (ioctl(s, SIOCSIFADDR, &ifr) < 0)
                        goto err;
        }

        /* Set IP netmask and broadcast */
        if (addr && netmask) {
                inet_aton(netmask, &in_netmask);
                sin_addr(&ifr.ifr_netmask).s_addr = in_netmask.s_addr;
                ifr.ifr_netmask.sa_family = AF_INET;
                if (ioctl(s, SIOCSIFNETMASK, &ifr) < 0)
                        goto err;

                in_broadaddr.s_addr = (in_addr.s_addr & in_netmask.s_addr) | ~in_netmask.s_addr;
                sin_addr(&ifr.ifr_broadaddr).s_addr = in_broadaddr.s_addr;
                ifr.ifr_broadaddr.sa_family = AF_INET;
                if (ioctl(s, SIOCSIFBRDADDR, &ifr) < 0)
                        goto err;
        }

        close(s);
        return 0;

 err:
        close(s);
        perror(name);
        return errno;
}
static int
route_main(int cmd, char *name, int metric, char *dst, char *gateway, char *genmask)
{
        int s;
        struct rtentry rt;

        DEBUG("route(): cmd=[%s] name=[%s] ipaddr=[%s] netmask=[%s] gateway=[%s] metric=[%d]\n",cmd == SIOCADDRT ? "ADD" : "DEL",name,dst,genmask,gateway,metric); 

        /* Open a raw socket to the kernel */
        if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                goto err;

        /* Fill in rtentry */
        memset(&rt, 0, sizeof(rt));
        if (dst)
                inet_aton(dst, &sin_addr(&rt.rt_dst));
        if (gateway)
                inet_aton(gateway, &sin_addr(&rt.rt_gateway));
        if (genmask)
                inet_aton(genmask, &sin_addr(&rt.rt_genmask));
        rt.rt_metric = metric;
        rt.rt_flags = RTF_UP;
        if (sin_addr(&rt.rt_gateway).s_addr)
                rt.rt_flags |= RTF_GATEWAY;
        if (sin_addr(&rt.rt_genmask).s_addr == INADDR_BROADCAST)
                rt.rt_flags |= RTF_HOST;
        rt.rt_dev = name;

        /* Force address family to AF_INET */
        rt.rt_dst.sa_family = AF_INET;
        rt.rt_gateway.sa_family = AF_INET;
        rt.rt_genmask.sa_family = AF_INET;

        if (ioctl(s, cmd, &rt) < 0)
                goto err;

        close(s);
        return 0;

 err:
        close(s);
        perror(name);
        return errno;
}

int
route_add(char *name, int metric, char *dst, char *gateway, char *genmask)
{
        return route_main(SIOCADDRT, name, metric, dst, gateway, genmask);
}

int
route_del(char *name, int metric, char *dst, char *gateway, char *genmask)
{
        return route_main(SIOCDELRT, name, metric, dst, gateway, genmask);
}


