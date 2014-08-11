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
    This programs will monitor WAN protocols 
===================================================================
*/

#include <stdio.h>
#include <sys/time.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <linux/ip.h>
#include <sys/syslog.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>
#include <ulog.h>

// for PF_PACKET
#include <features.h>
#if __GLIBC__ >=2 && __GLIBC_MINOR__ >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif

#include <syscfg.h>
#include <sysevent/sysevent.h>

/* prototypes */
int route_add(char *name, int metric, char *dst, char *gateway, char *genmask);
int route_del(char *name, int metric, char *dst, char *gateway, char *genmask);
int SE_msg_receive(int fd, char *replymsg, unsigned int *replymsg_size, token_t *who);

/* enums */
enum {LISTEN_FAIL, LISTEN_ERROR, LISTEN_SUCCESS};
enum {EVENT_ERROR=-1, EVENT_OK, EVENT_TIMEOUT, EVENT_PPP_PREUP, EVENT_PPP_UP, EVENT_PPP_DOWN};
enum {W_INIT, W_EVENTUPWAIT, W_EVENTWAIT, W_LISTEN, W_RESTART, W_EXIT};

/*
 * Macros
 */
/*
#define DEBUG(...) do {;} while(0)
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define DEBUG(args...) fprintf(stderr, ## args)
#define DEBUG(args...) syslog(LOG_INFO, ## args)
*/
#define DEBUG(args...) ulogf(ULOG_WAN, UL_WMON, ## args)

/* Print to the console */
#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt, ## args); \
                fclose(fp); \
        } \
} while (0)

/*
 * Structs
 */
struct eth_packet {
    u_int8_t dst_mac[6];
    u_int8_t src_mac[6];
    u_int8_t type[2];
    //struct iphdr ip;      //size=20
    u_int8_t version;
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int8_t saddr[4];
    u_int8_t daddr[4];
    u_int8_t data[1500-20];
};

/*
 * Local variables
 */
int current_state;
int next_state;
static int se_fd1 = 0; 
static int se_fd2 = 0;
char wanproto[32], wanconnmethod[32];
token_t token1, token2;
async_id_t async_id;
static short server_port;
static char  server_ip[19];
static char *state_str[6] =
    {"W_INIT", "W_EVENTUPWAIT", "W_EVENTWAIT", "W_LISTEN", "W_RESTART", "W_EXIT"};
static char *listen_str[3] = {"LISTEN_FAIL", "LISTEN_ERROR", "LISTEN_SUCCESS"};

/*
 * Initialize syscfg
 *   return 0 if good, or -1 if fail.
 */
int syscfg_inits()
{
    int ret = 0;
    /*
     * Initialize syscfg so as to get system persistant values in this code.
     */
    if (syscfg_init()) {
       DEBUG("syscfg unable to initialize with syscfg context.\n");
       ret = -1;
    }
    DEBUG("syscfg initialized with syscfg context.\n");

    return ret;
}

/*
 * Initialize sysevnt 
 *   return 0 if success and -1 if failture.
 */
int event_inits(char *event)
{
    int rc;

    snprintf(server_ip, sizeof(server_ip), "127.0.0.1");
    server_port = SE_SERVER_WELL_KNOWN_PORT;

    se_fd1 = sysevent_open(server_ip, server_port, SE_VERSION, "wmon1", &token1);
    if (!se_fd1) {
        DEBUG("Unable to register with sysevent daemon.\n");
        return(EVENT_ERROR);
    }
    sysevent_set_options(se_fd1, token1, event, TUPLE_FLAG_SERIAL);
    rc = sysevent_setnotification(se_fd1, token1, event, &async_id);
    if (rc) {
       DEBUG("cannot set request for client %s events %s\n", "wmon1", event);
       DEBUG("                Reason (%d) %s\n", rc, SE_strerror(rc));
       return(EVENT_ERROR);
    } else {
       DEBUG("event_inits: Ready to receive updates to %s\n", "wmon1");
    }

    se_fd2 = sysevent_open(server_ip, server_port, SE_VERSION, "wmon2", &token2);
    if (!se_fd2) {
       DEBUG("cannot set request for client %s\n", "wmon2");
       return(EVENT_ERROR);
    }

    return(EVENT_OK);
}

int event_close()
{
    /* we are done with this notification, so unregister it using async_id provided earlier */
    sysevent_rmnotification(se_fd1, token1, async_id);

    /* close this session with syseventd */
    sysevent_close(se_fd1, token1);
    sysevent_close(se_fd2, token2);

    return (EVENT_OK);
}

/*
 * Listen on ppp_status event
 *   return EVENT_TIMEOUT if no event messages, or 
 *          EVENT_PPP_UP if ppp_status is up, or 
 *          EVENT_PPP_DOWN if ppp_status is down.
 */
int event_listen(char *event)
{
    (void) event;
    int ret=EVENT_TIMEOUT;
    fd_set rfds;
    struct timeval tv;
    int retval;


        tv.tv_sec = 30;
        tv.tv_usec=0;
        FD_ZERO(&rfds);
        FD_SET(se_fd1, &rfds);

        DEBUG("Waiting for event ... \n");
        retval=select(se_fd1+1, &rfds, NULL, NULL, &tv);

        if(retval) {
            se_buffer            msg_buffer;
            se_notification_msg *msg_body = (se_notification_msg *)msg_buffer;
            unsigned int         msg_size;
            token_t              from;
            int                  msg_type;

            msg_size  = sizeof(msg_buffer);
            msg_type = SE_msg_receive(se_fd1, msg_buffer, &msg_size, &from);
            // if not a notification message then ignore it
            if (SE_MSG_NOTIFICATION == msg_type) {
               // extract the name and value from the return message data
              int   name_bytes;
              int   value_bytes;
              char *name_str;
              char *value_str;
              char *data_ptr;

              data_ptr   = (char *)&(msg_body->data);
              name_str   = (char *)SE_msg_get_string(data_ptr, &name_bytes);
              data_ptr  += name_bytes;
              value_str =  (char *)SE_msg_get_string(data_ptr, &value_bytes);

              DEBUG("Received event <%s %s>\n", name_str, value_str);
              if (!strncmp(value_str, "up", 2)) 
                  ret = EVENT_PPP_UP;
              else if (!strncmp(value_str, "preup", 5)) 
                  ret = EVENT_PPP_PREUP;
              else if (!strncmp(value_str, "down", 4)) 
                  ret = EVENT_PPP_DOWN;
            } else {
               DEBUG("Received msg that is not a SE_MSG_NOTIFICATION (%d)\n", msg_type);
            }
        } else {
           DEBUG("Received no event retval=%d\n", retval);
        }
    return ret;
}

int raw_socket(int ifindex)
{
        int fd;
        struct sockaddr_ll sock;

        DEBUG("Opening raw socket on ifindex %d\n", ifindex);
        if ((fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
                DEBUG("socket call failed: \n");
                return -1;
        }

        sock.sll_family = AF_PACKET;
        sock.sll_protocol = htons(ETH_P_ALL);
        sock.sll_ifindex = ifindex;
        if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
                DEBUG("bind call failed: \n");
                close(fd);
                return -1;
        }

        return fd;
}
/*
 *
 */
int intf_read(const char *interface, int *ifindex, u_int32_t *addr, unsigned char *arp)
{
    int fd;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    memset(&ifr, 0, sizeof(struct ifreq));
    if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
        ifr.ifr_addr.sa_family = AF_INET;
        strcpy(ifr.ifr_name, interface);

        if (addr) {
            if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
                sin = (struct sockaddr_in *) &ifr.ifr_addr;
                *addr = sin->sin_addr.s_addr;
                DEBUG("%s (our ip) = %s \n", ifr.ifr_name, inet_ntoa(sin->sin_addr));
            } else {
                DEBUG("SIOCGIFADDR failed!: \n");
                return -1;
            }
        }

        if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
            DEBUG("adapter index %d \n", ifr.ifr_ifindex);
            *ifindex = ifr.ifr_ifindex;
        } else {
            DEBUG("SIOCGIFINDEX failed!: \n");
            return -1;
        }
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
            memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
            DEBUG("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x \n",
                   arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
        } else {
            DEBUG("SIOCGIFHWADDR failed!: \n");
            return -1;
        }
    } else {
        DEBUG("socket failed!: \n");
        return -1;
    }
    close(fd);
    return 0;
}

/*
 * listen on a network interface, e.g. eth0, brlan0, etc.
 */
int intf_listen(const char *interface)
{
        int ifindex=0;
        fd_set rfds;
        struct eth_packet packet;
        struct timeval tv;
        int retval;
        unsigned char mac[6];
        static int fd;
        int ret = LISTEN_FAIL;
        int bytes;
        struct in_addr ipaddr, netmask, wanipaddr;
        char protostr[256], lan_addrstr[256], wan_addrstr[256], maskstr[256];
        u_int32_t val1, val2, val3, val4;

        /*
         * Get MAC address on the designated interface, e.g. eth0, brlan0, etc.
         */
        if (intf_read(interface, &ifindex, NULL, mac) < 0 ){
            ret = LISTEN_ERROR;
            goto Exit;
        }

        if (0 != syscfg_get(NULL, "wan_proto", protostr, sizeof(protostr))) goto Exit;
        DEBUG("syscfg_get returns wan_proto=%s\n", protostr);

        if (0 != syscfg_get(NULL, "lan_ipaddr", lan_addrstr, sizeof(lan_addrstr))) goto Exit;
        DEBUG("syscfg_get returns lan_ipaddr=%s\n", lan_addrstr);

        if (0 != syscfg_get(NULL, "lan_netmask", maskstr, sizeof(maskstr))) goto Exit;
        DEBUG("syscfg_get returns lan_netmask=%s\n", maskstr);

        if (0 != sysevent_get(se_fd2, token2, "wan_default_gateway", wan_addrstr, sizeof(wan_addrstr)) ) {  
           goto Exit;
        }
        DEBUG("sysevent returns wan_default_gateway=%s\n", wan_addrstr);

        for (;;) {
            tv.tv_sec = 100000;
            tv.tv_usec=0;
            FD_ZERO(&rfds);
            fd=raw_socket(ifindex);
            if (fd<0) {
                DEBUG("FATAL: couldn't listen on socket\n");
                return LISTEN_ERROR;
            }
            if (fd>=0) FD_SET(fd, &rfds);
            if (tv.tv_sec >0) {
                DEBUG("Waiting for traffic ... \n");
                retval=select(fd+1, &rfds, NULL, NULL, &tv);
            } else retval=0;

            if (retval==0) {
                DEBUG("no packet recieved! \n\n");
            } else {
                memset(&packet, 0, sizeof(struct eth_packet));
                bytes = read(fd, &packet, sizeof(struct eth_packet));
                if (bytes < 0) {
                    DEBUG("couldn't read on raw listening socket -- ignoring\n");
                    usleep(500000); /* possible down interface, looping condition */
                    ret = LISTEN_FAIL;
                    goto Exit;
                }

                if (bytes < (int) (sizeof(struct iphdr) )) {
                    DEBUG( "message too short, ignoring\n");
                    ret = LISTEN_FAIL;
                    goto Exit;
                }

                /* check dst mac if listen on brlan0 */
                if (!strncmp(interface, "brlan0", 3)) {
                    if (strncmp((char *)mac, (char *)packet.dst_mac, 6)) {
                        DEBUG( "dst mac not the router\n");
                        ret = LISTEN_FAIL;
                        goto Exit;
                    }
                }

                DEBUG("ip: src %d.%d.%d.%d, dst %d.%d.%d.%d\n",
                      packet.saddr[0],packet.saddr[1],packet.saddr[2],packet.saddr[3], 
                      packet.daddr[0],packet.daddr[1],packet.daddr[2],packet.daddr[3]); 

                //for (i=0; i<34;i++) {
                //      if (i%16==0) printf("\n");
                //      printf("%02x ",*( ( (u_int8_t *)packet)+i) );
                //}
                //printf ("\n");
                DEBUG("mac: src %02X:%02X:%02X:%02X:%02X:%02X, dst %02X:%02X:%02X:%02X:%02X:%02X, type:%02X%02X\n",
                     packet.src_mac[0], packet.src_mac[1], packet.src_mac[2],
                     packet.src_mac[3], packet.src_mac[4], packet.src_mac[5],
                     packet.dst_mac[0], packet.dst_mac[1], packet.dst_mac[2],
                     packet.dst_mac[3], packet.dst_mac[4], packet.dst_mac[5],
                     packet.type[0],packet.type[1]);

                // DEBUG("ip.version = %x ", packet.version);
                // DEBUG("ip.tos = %x ", packet.tos);
                // DEBUG("ip.tot_len = %x ", packet.tot_len);
                // DEBUG("ip.id = %x ", packet.id);
                // DEBUG("ip.ttl= %x ", packet.ttl);
                // DEBUG("ip.protocol= %x ", packet.protocol);
                // DEBUG("ip.check=%04x\n", packet.check);
                // DEBUG("ip.saddr=%08x ", *(u_int32_t *)&(packet.saddr));
                // DEBUG("ip.daddr=%08x\n", *(u_int32_t *)&(packet.daddr));

                if (*(u_int16_t *)packet.type == 0x0800) {
                    DEBUG( "not ip protocol");
                    ret = LISTEN_FAIL;
                    goto Exit;
                }

                /* ignore any extra garbage bytes */
                bytes = ntohs(packet.tot_len);

                DEBUG( "got a packet!\n");

                inet_aton(lan_addrstr, &ipaddr);
                inet_aton(maskstr, &netmask);
                inet_aton(wan_addrstr, &wanipaddr);

                DEBUG("lan_gateway=%08x netmask=%08x\n", ipaddr.s_addr, netmask.s_addr);
                val1=ipaddr.s_addr & netmask.s_addr;
                val2=wanipaddr.s_addr & netmask.s_addr;
                val3=(*(u_int32_t *)&(packet.saddr)) & netmask.s_addr;
                val4=(*(u_int32_t *)&(packet.daddr)) & netmask.s_addr;
                DEBUG("lan=%08x, wan=%08x, pkt_saddr=%08x\n", val1, val2, val3);

                /*
                 * Return fail if both source and destination IP addresses are of LAN network
                 */
                if (val3==val4 && val2 != val4) {
                    ret = LISTEN_FAIL;
                    goto Exit;
                }
                /*
                 * Return success if source IP address is of router's LAN or WAN network
                 */
                if (val1==val3 || val2==val3) {
                    ret = LISTEN_SUCCESS;
                    goto Exit;
                }
            break;
            }
        } /* end for loop */

Exit:
    if(fd) close(fd);

    DEBUG("intf_listern: return %s\n", listen_str[ret]);

    return ret;
}

static int pppoe_start()
{
    char pppoe_start[64];

    snprintf(pppoe_start, sizeof(pppoe_start), "/usr/sbin/pppd call utopia-pppoe &");
    DEBUG("Starting PPPoE: %s\n", pppoe_start);
    system(pppoe_start);
    return 0;
}

static inline void pppoe_stop ()
{
    DEBUG("Stopping PPPoE\n");
    DEBUG("Sleep 1 before kill pppd\n");
    sleep(1);
    system("killall -9 pppd");
}

static int pptp_start()
{
    char pptp_start[64];
    snprintf(pptp_start, sizeof(pptp_start), "/usr/sbin/pppd call utopia-pptp");
    DEBUG("Starting PPTP: %s\n", pptp_start);
    system(pptp_start);
    return 0;
}

static inline void pptp_stop()
{
    DEBUG("Stopping PPTP\n");
    DEBUG("Sleep 1 before kill PPTP\n");
    sleep(1);
    system("killall -9 pppd");
    system("killall -9 pptp");
}

static int l2tp_start()
{
    char l2tp_cmd[64], addrstr[32];

    if (syscfg_get(NULL, "wan_proto_server_address", addrstr, sizeof(addrstr))) return -1;
    snprintf(l2tp_cmd, sizeof(l2tp_cmd), "/usr/sbin/l2tpd -d 65535");
    DEBUG("Starting L2TP: %s\n", l2tp_cmd);
    system(l2tp_cmd);
    sleep(1);
    snprintf(l2tp_cmd, sizeof(l2tp_cmd), "/usr/sbin/l2tp-control \"start-session %s\"", addrstr);
    DEBUG("Starting L2TP: %s\n", l2tp_cmd);
    system(l2tp_cmd);
    return 0;
}

static inline void l2tp_stop()
{
    DEBUG("Stopping L2TP\n");
    /*
     * The delay is needed for redial mode, not on-demand mode.
     */
    if (!strncmp(wanconnmethod, "redial", 6)) {
       /*
        * The delay would allow l2tp to have time reply to ack StopCCN message; 
        *   otherwise some qacafe tests may fail. 
        */
        DEBUG("Sleep 5 before kill L2TP\n");
        sleep(5);
    }
    system("killall -9 pppd");
    system("killall -9 l2tpd");
}

#if 0
static int start_bpalogin()
{
    char bpalogin[64];
    snprintf(bpalogin, sizeof(bpalogin), "/usr/sbin/bpalogin -c /tmp/bpalogin.conf");
    DEBUG("Starting BPALogin: %s\n", bpalogin);

    return 0;
}
#endif
 
/*
 * To be called prior to starting WAN protocol
 */
static int ppp_pre_route_change (const char *wanproto)
{
    char wan_ifname[8];

    sysevent_get(se_fd2, token2, "wan_ifname", wan_ifname, sizeof(wan_ifname));
    DEBUG("ppp_pre_route_change: sysevent_get returned wan_ifname %s\n", wan_ifname);

    if ((!strncmp(wanproto, "l2tp", 4)) || (!strncmp(wanproto, "pptp", 4)))
        while (route_del(wan_ifname, 0, NULL, NULL, NULL) == 0);

    return 0;
}

/*
 * To be called when WAN protocol is pre-up to fix some routes
 */
static int ppp_preup_route_change (const char *wanproto)
{
    (void) wanproto;

    return 0;
}

/*
 * To be called when WAN protocol is up to fix some routes
 */
static int ppp_up_route_change (const char *wanproto)
{
    char wan_ifname[8], ppp_remote_addr[16], ppp_local_addr[16];

    sysevent_get(se_fd2, token2, "wan_ifname", wan_ifname, sizeof(wan_ifname));
    DEBUG("ppp_up_route_change: sysevent_get returned wan_ifname %s\n", wan_ifname);

    sysevent_get(se_fd2, token2, "ppp_local_ipaddr", ppp_local_addr, sizeof(ppp_local_addr));
    DEBUG("ppp_up_route_change: sysevent_get returned ppp_local_ipaddr %s\n", ppp_local_addr);

    sysevent_get(se_fd2, token2, "ppp_remote_ipaddr", ppp_remote_addr, sizeof(ppp_remote_addr));
    DEBUG("ppp_up_route_change: sysevent_get returned ppp_remote_ipaddr %s\n", ppp_remote_addr);

    if ((!strncmp(wanproto, "l2tp", 4)) || (!strncmp(wanproto, "pptp", 4))) {
        route_del("ppp0", 0, ppp_remote_addr, NULL, "255.255.255.255");
        route_del("ppp0", 0, "0.0.0.0", "0.0.0.0", "0.0.0.0");
        route_del("vlan2", 0, "0.0.0.0", "0.0.0.0", "255.255.255.255");

        // Add a host
        route_add("ppp0", 0, ppp_local_addr, NULL, "255.255.255.255");
        // Add a gateway
        route_add("ppp0", 0, "0.0.0.0", ppp_local_addr, "0.0.0.0");
        // Adding the following route will fail CDRouter L2TP-PPP tests but it's in WRT610n.
        // route_add("vlan2", 0, "0.0.0.0", ppp_remote_addr, "0.0.0.0");
        // Add a host
        // route_add("vlan2", 0, ppp_remote_addr, "0.0.0.0", "255.255.255.255");
    }

    return 0;
}

static int enable_ip_forward (void)
{
   /* Enable ip_forward */
   FILE *fp = fopen ("/proc/sys/net/ipv4/ip_forward", "w");
   if (NULL != fp) {
      fprintf(fp, "1");
      fclose(fp);
   }
   return 0;
}

static int set_system_preup_parameters (void)
{
   // transfer network parameters from local sysevent variables to
   // system sysevent variables
   int rc;
   char element[256];
   rc = sysevent_get(se_fd2, token2, "pppd_current_wan_subnet", element, sizeof(element));
   if (0 != rc || '\0' == element[0]) {
      sysevent_set(se_fd2, token2, "current_wan_subnet", "255.255.255.255", 0);
      DEBUG("sysevent set current_wan_subnet %s\n", "255.255.255.255");
   } else {
      sysevent_set(se_fd2, token2, "current_wan_subnet", element, 0);
      DEBUG("sysevent set current_wan_subnet %s\n", element);
   }
   rc = sysevent_get(se_fd2, token2, "pppd_current_wan_ifname", element, sizeof(element));
   if (0 != rc || '\0' == element[0]) {
      sysevent_set(se_fd2, token2, "current_wan_ifname", "ppp0", 0);
      DEBUG("sysevent set current_wan_ifname %s\n", "ppp0");
   } else {
      sysevent_set(se_fd2, token2, "current_wan_ifname", element, 0);
      DEBUG("sysevent set current_wan_ifname %s\n", element);
   }

   rc = sysevent_get(se_fd2, token2, "pppd_current_wan_ipaddr", element, sizeof(element));
   if (0 != rc || '\0' == element[0]) {
      sysevent_set(se_fd2, token2, "current_wan_ipaddr", "10.64.64.64", 0);
      DEBUG("sysevent set current_wan_ipaddr %s\n", "10.64.64.64");
   } else {
      sysevent_set(se_fd2, token2, "current_wan_ipaddr", element, 0);
      DEBUG("sysevent set current_wan_ipaddr %s\n", element);
   }

   sysevent_set(se_fd2, token2, "firewall-restart", "", 0);

   enable_ip_forward();

   return 0;
}

static int set_system_up_parameters (void)
{
   // transfer network parameters from local sysevent variables to
   // system sysevent variables
   int rc;
   char element[256];
   rc = sysevent_get(se_fd2, token2, "pppd_current_wan_subnet", element, sizeof(element));
   if (0 != rc || '\0' == element[0]) {
      sysevent_set(se_fd2, token2, "current_wan_subnet", "255.255.255.255", 0);
      DEBUG("sysevent set current_wan_subnet %s\n", "255.255.255.255");
   } else {
      sysevent_set(se_fd2, token2, "current_wan_subnet", element, 0);
      DEBUG("sysevent set current_wan_subnet %s\n", element);
   }
   rc = sysevent_get(se_fd2, token2, "pppd_current_wan_ifname", element, sizeof(element));
   if (0 != rc || '\0' == element[0]) {
      sysevent_set(se_fd2, token2, "current_wan_ifname", "ppp0", 0);
      DEBUG("sysevent set current_wan_ifname %s\n", "ppp0");
   } else {
      sysevent_set(se_fd2, token2, "current_wan_ifname", element, 0);
      DEBUG("sysevent set current_wan_ifname %s\n", element);
   }

   rc = sysevent_get(se_fd2, token2, "pppd_current_wan_ipaddr", element, sizeof(element));
   if (0 != rc || '\0' == element[0]) {
      sysevent_set(se_fd2, token2, "current_wan_ipaddr", "0.0.0.0", 0);
      DEBUG("sysevent set current_wan_ipaddr %s\n", "0.0.0.0");
   } else {
      sysevent_set(se_fd2, token2, "current_wan_ipaddr", element, 0);
      DEBUG("sysevent set current_wan_ipaddr %s\n", element);
   }

   sysevent_set(se_fd2, token2, "firewall-restart", "", 0);

   enable_ip_forward();

   DEBUG("current_ipv4_wan_state up\n");
   sysevent_set(se_fd2, token2, "current_ipv4_wan_state", "up", 0);
   DEBUG("current_wan_state up\n");
   sysevent_set(se_fd2, token2, "current_wan_state", "up", 0);
   DEBUG("wan-status started\n");
   sysevent_set(se_fd2, token2, "wan-status", "started", 0);
   return 0;
}

static int set_system_down_parameters (void)
{
   /*
    * Stop ip forwarding 
    */
   FILE *fp = fopen ("/proc/sys/net/ipv4/ip_forward", "w");
   if (NULL != fp) {
      fprintf(fp, "0");
      fclose(fp);
   }

   DEBUG("current_ipv4_wan_state down\n");
   sysevent_set(se_fd2, token2, "current_ipv4_wan_state", "down", 0);
   DEBUG("current_wan_state down\n");
   sysevent_set(se_fd2, token2, "current_wan_state", "down", 0);
   DEBUG("wan-status stopped\n");
   sysevent_set(se_fd2, token2, "wan-status", "stopped", 0);
   return 0;
}

/*
 * To be called after stoppng WAN protocol
 */
static int ppp_post_route_change (const char *wanproto)
{
    char wan_ifname[8], default_router[16];

    DEBUG("ppp_post_route_change: called with %s\n", wanproto);

    sysevent_get(se_fd2, token2, "wan_ifname", wan_ifname, sizeof(wan_ifname));
    DEBUG("ppp_post_route_change: sysevent_get returned wan_ifname %s\n", wan_ifname);

    sysevent_get(se_fd2, token2, "default_router", default_router, sizeof(default_router));
    DEBUG("ppp_post_route_change: sysevent_get returned default_router %s\n", default_router);

    if ((!strncmp(wanproto, "l2tp", 4)) || (!strncmp(wanproto, "pptp", 4)))
        route_add(wan_ifname, 0, NULL, default_router, NULL);

    return 0;
}

static void start_wan_proto (const char *wanproto)
{
    ppp_pre_route_change(wanproto);

    DEBUG("started %s\n", wanproto);
    char wan_status[64];
    wan_status[0] = '\0';
    if (0 != sysevent_get(se_fd2, token2, "wan-status", wan_status, sizeof(wan_status)) ) {  
       DEBUG("started %s. Current wan-status is %s\n", wanproto, wan_status);
    } else {
       DEBUG("started %s. Current wan-status is %s\n", wanproto, "unknown");
    }
    if (!strncmp(wanproto, "l2tp", 4)) {
        if (0 != strcmp("starting", wan_status)) {
           sysevent_set(se_fd2, token2, "wan-status", "starting", 0);
        }
        l2tp_start();
    }
    else if (!strncmp(wanproto, "pppoe", 5)) {
        if (0 != strcmp("starting", wan_status)) {
           sysevent_set(se_fd2, token2, "wan-status", "starting", 0);
        }
        pppoe_start();
    }
    else if (!strncmp(wanproto, "pptp", 4)) {
        if (0 != strcmp("starting", wan_status)) {
           sysevent_set(se_fd2, token2, "wan-status", "starting", 0);
        }
        pptp_start();
    }
    else 
        DEBUG("no protocal started\n");
}

static void stop_wan_proto (const char *wanproto)
{
    ppp_post_route_change(wanproto);

    if (!strncmp(wanproto, "l2tp", 4))
        l2tp_stop();
    else if (!strncmp(wanproto, "pppoe", 5))
        pppoe_stop();
    else if (!strncmp(wanproto, "pptp", 4)) 
        pptp_stop();
}

static void wmon_signal_handler (int signum)
{
    DEBUG("Handling SIGQUIT/SIGTERM (%d)\n", signum);
    stop_wan_proto(wanproto);
    set_system_down_parameters();
    event_close();
    exit(0);
}

static int wmon_daemon_main (const char *interface)
{
    char *ppp_event ="ppp_status";
    struct sigaction act;

    if ((syscfg_inits() == -1) || (event_inits(ppp_event) == EVENT_ERROR)) {
        printf("syscfg or sysevent init failed\n");
        return 1; // Error 
    }

    syscfg_get(NULL, "wan_proto", wanproto, sizeof(wanproto));
    DEBUG("syscfg_get wan_proto %s\n", wanproto);

    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = wmon_signal_handler;

    if (-1 == sigaction(SIGQUIT, &act, NULL)) {
        printf("signal handler set failed\n");
        return 1; // Error 
    }
    if (-1 == sigaction(SIGTERM, &act, NULL)) {
        printf("signal handler set failed\n");
        return 1; // Error 
    }


      // kickstart state m/c
      next_state = W_INIT;
      for(;;) {

          DEBUG("current state is %s\n", state_str[next_state]);

          switch(next_state) {
            case W_INIT:

                syscfg_get(NULL, "wan_proto", wanproto, sizeof(wanproto));
                DEBUG("syscfg_get wan_proto %s\n", wanproto);

                syscfg_get(NULL, "ppp_conn_method", wanconnmethod, sizeof(wanconnmethod)); 
                DEBUG("syscfg_get ppp_conn_method %s\n", wanconnmethod);

                start_wan_proto(wanproto);

                next_state = W_EVENTUPWAIT;

                break;

            case W_EVENTUPWAIT:
                /*
                 * if ppp is down, take actions.
                 * if ppp is up, keep listing on event.
                 */
                switch (event_listen("ppp_event")) {
                case EVENT_PPP_PREUP:
                    // wan protocol startup success
                    ppp_preup_route_change(wanproto);
                    if (!strncmp(wanconnmethod, "demand", 6) && strncmp(wanproto, "l2tp", 4)) {
                        set_system_preup_parameters();
                    }
                    next_state = W_EVENTWAIT;
                    break;
                case EVENT_PPP_UP:
                    ppp_up_route_change(wanproto);
                    set_system_up_parameters();
                    next_state = W_EVENTWAIT;
                    break;
                case EVENT_TIMEOUT:
                case EVENT_PPP_DOWN:
                default:
                    next_state = W_RESTART;
                    break;
                }

                break;

            case W_EVENTWAIT:
                /*
                 * if ppp is down, take actions.
                 * if ppp is up, change routes as needed and keep listing event.
                 */
                switch (event_listen("ppp_event")) {
                case EVENT_PPP_UP:
                    ppp_up_route_change(wanproto);
                    set_system_up_parameters();
                    break;
                case EVENT_PPP_DOWN:
                  /*
                   *    if demand, we listen on interface for lan to wan traffic;
                   *    if redial, we start pppd.
                   */
                  /* 
                   * Go to W_RESTART state if ppp_conn_method = redial
                   * go to W_LISTEN state if ppp_conn_method = demand
                   * else, stay in the same state
                   */
                    if (!strncmp(wanconnmethod, "redial", 6)) {
                      next_state = W_RESTART;
                    } else if (!strncmp(wanconnmethod, "demand", 6) && !strncmp(wanproto, "l2tp", 4)) {
                      next_state = W_LISTEN;
                    } 
                    break;
                case EVENT_TIMEOUT:
                default:
                    // stay in the same state
                    break;
                } /* end case switch */

                break;

            case W_LISTEN:
                next_state = W_LISTEN;
                if (intf_listen(interface) == LISTEN_SUCCESS)
                  next_state = W_RESTART;
                break;

            case W_RESTART:
                stop_wan_proto(wanproto);
                set_system_down_parameters();
                start_wan_proto(wanproto);

                next_state = W_EVENTUPWAIT;
                break;

            case W_EXIT:
                event_close();
                exit(1);
                break;

            default:
                break;
          } /* end switch on next_state */

          DEBUG("next state is %s\n", state_str[next_state]);
    } /* end for loop */

    DEBUG("Leaving wmon_daemon_main\n");
    event_close();
}

int main(int argc, char *argv[])
{

    char *interface=argv[1];
    pid_t pid;
    int rc = 0;

    if (argc <2) {
        printf("Usage: %s <interface>\n",argv[0]);
        return 0;
    }

    ulog_init();

    DEBUG("Starting LAN to WAN monitoring on %s\n",interface);

    /*
     * Real work starts here.
     */
    pid = fork();
    switch(pid)
    {
        case -1:
          perror("fork failed");
          exit(1);
        case 0:
          rc = wmon_daemon_main(interface);
          exit(rc);
        default:
          _exit(0);
          break;
    } /* end switch on pid */
}


