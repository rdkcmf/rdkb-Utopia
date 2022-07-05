/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

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
 ============================================================================

 Introduction to BYOI_BRIDGE_MODE
 -------------------------------

 When DRG is in BYOI BRIDGE MODE, NP is briding LAN ports and BYOI WAN port.
 Internet traffic from LAN CPEs will be bridged to BYOI WAN port by NP.
 Managed traffic should be routed to managed service provider through AP and
 CM. To solved this dilemma, 
 1) NP will obtain its WAN IP address, WAN router address, and WAN DNS server
    addresses, either from upstream DHCP server through BYOI WAN port, or
    static configured through syscfg.
 2) NP will function as default router for all LAN CPEs. For traffic destination
    that does not reside in local LAN, NP will forward traffic based on the
    destination IP address:
    a) If destination IP address is in one of managed subnet, forward traffic
       to AP.
    b) Otherwise, forward traffic to upstream router. 
 3) NP will function as DNS server/forwarder for all LAN CPEs. For domain
    postfix that matches one of the managed domain, NP will forward DNS request
    to WAN DNS servers, otherwise it will forward DNS request to AP.

 Since WAN DHCP server is still serving DHCP requests for all LAN CPEs, NP will
 need to modify DHCP offer and ACK from WAN DHCP server to make itself as 
 the default router and DNS server for all LAN CPEs. Therefore we have a DHCP
 Proxy.
 
 Introduction to DHCP Proxy
 -------------------------------

 DHCP requests from LAN CPEs to WAN DHCP server will pass through NP bridge. It
 may be one of the following messages:

 1. DHCP Discover for initial discovery of DHCP server
 2. DHCP Request to obtain, renew, or rebind a leased IP address
 3. DHCP Release to relinguish DHCP server
 4. DHCP Decline to reject a leased IP address when DAD returns failure

 DHCP request will be sent from port 68 (DHCP client port) to port 67 (DHCP server
 port), except when it is from a DHCP relay agent, the source port will also
 67. DHCP request could be a broadcast packet (most likely), or an unicast
 packet (renewing).

 DHCP response from WAN DHCP server to LAN CPEs will also pass through NP bridge.
 It may be one of the following messages:

 1. DHCP Offer
 2. DHCP ACK to assigned a leased IP address or extend an existing lease

 DHCP response will be sent from port 67 (DHCP server port) to port 68 (DHCP
 client port), except when it is sent to a relay agent, the destination port
 will also be 68. DHCP response is usually an unicast packet directly to CPEs
 (most likely) or to relay agent. But it can also be a broadcast packet to
 CPE if CPE set broadcast flag in request message. (Most DHCP clients do not
 do this)

 In order to modify DHCP response transparently and efficiently, the following
 changes have been made to NP:

 1. Set EBTABLE rules to block DHCP traffic (IPv4 UDP destination Port 67 and 68)
    from BYOI WAN port to LAN ports. This is mainly to block DHCP response from
    WAN DHCP server to LAN CPEs. Port 68 is also blocked for a) DHCP relay
    agent at LAN side (not likely), and b) DHCP requests from WAN CPEs (also not
    likely)

 2. DHCP Proxy to snoop at DHCP requests from LAN ports. This is mainly for
    a. Associated ongoing DHCP session with a specific LAN port.
    b. Maintained the state of DHCP leases by monitoring DHCP decline and release
       messages.

 3. DHCP Proxy to receive DHCP response from WAN ports. 
    a) DHCP proxy will associated received DHCP response with an ongoing DHCP
       session using DHCP Client Identifier, Client Hardware (MAC) address and
       transaction id (xid).
    b) DHCP will maintained the state of DHCP leases.
    c) DHCP Proxy will pass DHCP NACK messages to associated LAN port as is.
    d) DHCP Proxy will modify DHCP OFFER and ACK messages to replace a)
       Router Option, b) DNS server option with its own WAN IP address and then
       pass ir to associated LAN port.

 4. DHCP Proxy will maintain and update a lease file for all existing DHCP leases
    associated with LAN CPEs.

 DHCP proxy use packet socket to snoop at LAN/WAN interfaces directly to get
 DHCP messages. This brings certain overhead for packet processing.  Linux Socket
 Filtering is used to minize the overhead (see below). CPU time to bridge a single
 packet between LAN and WAN ports will increase by about 7% to 8%.

 ============================================================================
*/
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <linux/filter.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "dhcp_msg.h"
#include "dhcp_proxy.h"
#include "dhcp_util.h"
#include "packet_util.h"

#define DEFAULT_MAX_HOPS 4
#define DEFAULT_LEASE_FILE "/tmp/dhcp_proxy.leases"

/**
 * @brief List of DHCP lease maintained.
 */
struct dhcp_lease *g_lease_list = NULL, *g_last_lease=NULL;
struct in_addr g_my_ip;
int g_max_hops = DEFAULT_MAX_HOPS;
const char* g_lease_file = DEFAULT_LEASE_FILE;

extern void dhcp_free_lease(struct dhcp_lease *lease);
extern void dhcp_update_lease_file();

/**
 * The following magic codes are Linux Socket Filter (LSF) codes. They are
 * executed in kernel space to keep unwanted packets out. The following
 * (slightly) human readable codes is generated using tcpdump:
 *
 *    sudo tcpdump -i eth0 -d -s 1514 ip and udp dst portrange 67-68
 *
 * It means we are only interested in IPv4 UDP packet with destination
 * port from 67(dhcps) to 68(dhcpc). That is for DHCP only. We will get
 * up to 1514 bytes of data per packet, which should be enough for 14 bytes
 * Ethernet header plus 1500 bytes of IP packet.
 *
 * We could have used libpcap instead, but for our little purpose, this
 * appears sufficient:
 *
 * (000) ldh      [12]
 * (001) jeq      #0x800           jt 2    jf 11
 * (002) ldb      [23]
 * (003) jeq      #0x11            jt 4    jf 11
 * (004) ldh      [20]
 * (005) jset     #0x1fff          jt 11   jf 6
 * (006) ldxb     4*([14]&0xf)
 * (007) ldh      [x + 16]
 * (008) jge      #0x43            jt 9    jf 11
 * (009) jgt      #0x44            jt 11   jf 10
 * (010) ret      #1514
 * (011) ret      #0
 *
 * It means:
 * (000) load half-word from offset 12 (Protocol Field in Ethernet Header)
 * (001) if it is 0x800 (IP Protocol) then goto to (002) goto (011) 
 * (002) load one byte from offset 23 (Protocol Filter in IP header)
 * (003) if it is 0x11 (UDP) then goto to (004) else goto (011)
 * (004) load half-word from offset 20 (fragment offset field in IP header)
 * (005) if 0x1fff is set (not first fragment), goto to (011), else goto (006)
 * (006) load IP header size
 * (007) load half-word 2(16-14) bytes after IP header, (UDP destination port)
 * (008) if >= 67 goto (009) else goto (011)
 * (009) if <= 68 goto (010) else goto (011)
 * (010) return 1514 bytes to user space
 * (011) return nothing to user space
 *
 * The following are filter codes in C struct, generated using tcpdump:
 *
 *    sudo tcpdump -i eth0 -dd -s 1514 ip and udp dst portrange 67-68
 *
 */
struct sock_filter dhcp_packet_filter[] = {
{ 0x28, 0, 0, 0x0000000c }, // We don't need this since we already bound to
{ 0x15, 0, 9, 0x00000800 }, // ETH_P_IP 
{ 0x30, 0, 0, 0x00000017 },
{ 0x15, 0, 7, 0x00000011 },
{ 0x28, 0, 0, 0x00000014 },
{ 0x45, 5, 0, 0x00001fff },
{ 0xb1, 0, 0, 0x0000000e },
{ 0x48, 0, 0, 0x00000010 },
{ 0x35, 0, 2, 0x00000043 },
{ 0x25, 1, 0, 0x00000044 },
{ 0x6, 0, 0, 0x000005ea },
{ 0x6, 0, 0, 0x00000000 },
};

#define SIZE_OF_PACKET_FILTER(filter) (sizeof(filter)/sizeof(struct sock_filter))
#define DHCP_PACKET_FILTER_SIZE SIZE_OF_PACKET_FILTER(dhcp_packet_filter)

/**
 * @brief Print Usage
 */
void usage(const char* program)
{
   printf("Usage: %s [-w wan_interface_name] [-l lan_interface_name] [-s my_ip_address] [-f lease_file]\n",
          program);
   exit(-1);
}

/**
 * @brief Main function for DHCP Proxy
 */
int main(int argc, char** argv)
{
   int opt;

   packet_init_socket();

   while ((opt = getopt(argc, argv, "w:l:s:f:")) != -1)
   {
      switch (opt)
      {
         case 'w':
            packet_add_interface(optarg, WAN_INTERFACE_TYPE);
            break;
         case 'l':
            packet_add_interface(optarg, LAN_INTERFACE_TYPE);
            break;
         case 's':
            g_my_ip.s_addr = inet_addr(optarg);
            break;
         case 'f':
            g_lease_file = optarg;
            break;
         default:
            usage(argv[0]);
            break;
      }
   }

   if (g_my_ip.s_addr == INADDR_ANY || g_my_ip.s_addr == INADDR_BROADCAST)
   {
      printf("Invalid my IP address: %s\n", inet_ntoa(g_my_ip));
      usage(argv[0]);
   }

   // Filter to get rid of unwanted packets, we are interested in DHCP packets only
   if (packet_attach_filter(dhcp_packet_filter, DHCP_PACKET_FILTER_SIZE) < 0)
   {
      return -1;
   }

   if (packet_bind_socket() < 0)
   {
      return -1;
   }

   for (;;)
   {
      unsigned char recv_buf[1514];
      struct sockaddr_ll from_addr;
      struct packet_intf *from_intf;
      ssize_t recv_size;

      recv_size = packet_recvfrom(recv_buf, sizeof(recv_buf), 0, &from_addr, &from_intf, 0);

      if (recv_size < 0)
      {
         if (errno == EINTR) continue;
         else break;
      }
      else if (recv_size == 0)
      {
         continue; 
      }
      else
      {
         unsigned int recv_dhcp_msg_size;
         struct dhcp_msg *recv_dhcp_msg;
         struct mac_header *mac_hdr;
         struct ip_header *ip_hdr;
         struct udp_header *udp_hdr;

         if (from_addr.sll_pkttype == PACKET_OUTGOING)
         {
            printf("Discard %ld bytes outgoing message from interface %d\n\n",
                   (long int)recv_size, from_addr.sll_ifindex);
            continue;
         }

         if (!from_intf)
         {
            printf("Discard %ld bytes message from unknown interface %d\n\n",
                   (long int)recv_size, from_addr.sll_ifindex);
            continue;
         }

         printf("Receive %ld bytes from interface=%s %d\n",
                (long int)recv_size, from_intf->ifname, from_intf->iftype);

         recv_dhcp_msg = (struct dhcp_msg*) parse_and_validate_ethernet_packet(
                               recv_buf, recv_size,
                               &mac_hdr, &ip_hdr, &udp_hdr,
                               &recv_dhcp_msg_size);
         if (recv_dhcp_msg)
         {
            struct dhcp_option_info dhcp_opts;
            struct dhcp_lease *lease;

            dhcp_parse_msg(&dhcp_opts, recv_dhcp_msg, recv_dhcp_msg_size);

            lease = dhcp_process_msg(recv_dhcp_msg, recv_dhcp_msg_size, &dhcp_opts,
                                     from_intf->ifindex, from_intf->iftype);

            if (lease)
            {
                dhcp_relay_message(lease, recv_buf, (unsigned char*)udp_hdr-recv_buf,
                                   recv_dhcp_msg, recv_dhcp_msg_size, &dhcp_opts);
            }

            dhcp_clear_option_info(&dhcp_opts);
         }
         else
         {
            printf("Discard invalid message\n");
         }

         printf("\n");
      } // if data received from socket
   } // forever until fatal error

   return -1;
}

/**
 * @brief Lookup of DHCP lease 1) use client ID, 2) if not available, 
 *        use chaddr
 * @param pprev return pointer to previous lease, make it easier to delete lease
 */
struct dhcp_lease *dhcp_find_lease(const struct dhcp_msg *msg,
                                   const struct dhcp_option_info *opt_info,
                                   struct dhcp_lease **pprev)
{
   struct dhcp_lease *found = NULL, *prev = NULL;
   for (found = g_lease_list; found; prev = found, found = found->next)
   {
      if (opt_info->opt_clientid)
      {
          if (compare_option(opt_info->opt_clientid, &found->clientid) == 0) break;
      } else if (msg->hdr.htype == found->htype && msg->hdr.hlen == found->hlen &&
                    memcmp(msg->hdr.chaddr, found->chaddr, found->hlen) == 0) break;
   }
   if (found) *pprev = prev;
   return found;
}


/**
 * @brief Process received DHCP message
 *
 * @param recv_msg      received DHCP message
 * @param recv_msg_size size of received DHCP message
 * @param opt_info      contains list of DHCP options in received DHCP message
 * @param recv_ifindex  Index of Interface the message is received from
 * @param recv_iftype   type of Interface the message is received from (WAN or LAN)
 *
 * @return DHCP lease this messsage associated with
 */
struct dhcp_lease *dhcp_process_msg(struct dhcp_msg *recv_msg, size_t recv_msg_size,
                                    struct dhcp_option_info *opt_info,
                                    int recv_ifindex, int recv_iftype)
{
   struct dhcp_lease *lease, *prev_lease;

   if (dhcp_validate_msg(recv_msg, opt_info) < 0)
   {
      printf("Invalid DHCP message.\n");
      return NULL;
   }

   if (recv_iftype == WAN_INTERFACE_TYPE && recv_msg->hdr.op == BOOTREQUEST)
   {
      printf("Ignore DHCP request message from WAN side\n");
      return NULL;
   }

   if (recv_iftype == LAN_INTERFACE_TYPE && recv_msg->hdr.op == BOOTREPLY)
   {
      printf("Ignore DHCP reply message from LAN side\n");
      return NULL;
   }

   // extra precausion: use hops to avoid infinite forward loop
   if (recv_msg->hdr.hops >= g_max_hops)
   {
      printf("hops %d > %d, discard DHCP message.\n", recv_msg->hdr.hops, g_max_hops);
      return NULL;
   }

   dump_dhcp_msg(recv_msg, opt_info);

   lease = dhcp_find_lease(recv_msg, opt_info, &prev_lease);

   if (!lease) {
      // for request, create new lease
      if (recv_msg->hdr.op == BOOTREQUEST && recv_msg->cookie == htonl(DHCP_COOKIE)) {

         lease = (struct dhcp_lease*)malloc(sizeof(struct dhcp_lease));
         memset(lease, 0, sizeof(struct dhcp_lease));

         lease->client_ifindex = recv_ifindex;
         lease->last_msg = opt_info->msgtype;
         lease->last_xid = recv_msg->hdr.xid;
         lease->lastmsgtime = time(0);
         lease->htype = recv_msg->hdr.htype;
         lease->hlen = recv_msg->hdr.hlen;
         memcpy(lease->chaddr, recv_msg->hdr.chaddr, lease->hlen);

         switch (opt_info->msgtype)
         {
            case DHCPDISCOVER: case DHCPREQUEST:
               lease->state = ST_INITIALIZING;
               break;
            case DHCPDECLINE: case DHCPRELEASE:
               lease->state = ST_TERMINATED;
               break;
            case DHCPINFORM: lease->state = ST_STATIC; break;
         }

         lease->ciaddr = recv_msg->hdr.ciaddr;
         lease->yiaddr = recv_msg->hdr.yiaddr;
         lease->giaddr = recv_msg->hdr.giaddr;
         lease->server_ip = opt_info->server_ip;

         if (opt_info->opt_hostname)
         {
            lease->hostname = (char*)malloc(opt_info->opt_hostname->len+1);
            memcpy(lease->hostname, opt_info->opt_hostname->data, opt_info->opt_hostname->len);
            lease->hostname[opt_info->opt_hostname->len] = '\0';
         }

         if (opt_info->opt_clientid) dhcp_copy_option(&lease->clientid, opt_info->opt_clientid);

         if (g_lease_list)
         {
            g_last_lease->next = lease;
            prev_lease = g_last_lease;
            g_last_lease = lease;
         }
         else
         {
            g_lease_list = g_last_lease = lease;
            prev_lease = NULL;
         }
      }
      else
      {
         // discard message
         printf("DHCP reply received without request record.\n");
         return NULL;
      }
   }
   else
   {
      if (recv_msg->hdr.op == BOOTREPLY)
      {
         if (recv_msg->hdr.xid != lease->last_xid)
         {
            // xid mismatch, discard
            printf("XID mismatch, expect 0x%08x, find 0x%08x, discard message.\n", lease->last_xid, recv_msg->hdr.xid);
            return NULL;
         }
      }
      else
      {
         lease->giaddr = recv_msg->hdr.giaddr; // remember original giaddr
         lease->client_ifindex = recv_ifindex;
      }

      // update lease record
      lease->last_msg = opt_info->msgtype;
      lease->last_xid = recv_msg->hdr.xid;
      lease->lastmsgtime = time(0);
      lease->ciaddr = recv_msg->hdr.ciaddr;
      lease->yiaddr = recv_msg->hdr.yiaddr;
      if (opt_info->server_ip.s_addr != INADDR_ANY) lease->server_ip = opt_info->server_ip;

      switch (opt_info->msgtype)
      {
         case DHCPDISCOVER: case DHCPREQUEST:
            lease->state = ST_INITIALIZING;
            break;
         case DHCPDECLINE: case DHCPRELEASE:
            if (lease->state == ST_BOUND) lease->expiretime = time(0);
            lease->state = ST_TERMINATED;
            break;
         case DHCPINFORM: lease->state = ST_STATIC; break;
         case DHCPOFFER:
            lease->state = ST_INITIALIZING;
            break;
         case DHCPACK:
            lease->state = ST_BOUND;
            lease->boundtime = time(0);
            lease->leasetime = opt_info->leasetime;
            if (lease->leasetime == INFINITE_LEASETIME) lease->expiretime = INFINITE_LEASETIME;
            else lease->expiretime = lease->boundtime + lease->leasetime;
            break;
      }

   }

   dhcp_update_lease_file();

   return lease;
}

/**
 * @brief Relay DHCP reply from WAN to LAN
 *
 * @param lease DHCP        lease this message related to
 * @param recv_packet       received DHCP reply packet, from Ethernet header
 * @param udp_header_offset offset to UDP header from ethernet header for the DHCP reply packet
 *                          use to locate UDP length field and UDP header checksum field
 * @param recv_msg          received DHCP message (UDP payload)
 * @param recv_msg_size     received DHCP message size
 * @param opt_info          received DHCP message option list
 *
 * For DHCP OFFER and ACK, this function will update Router option and DNS server option.
 * It will also update IP length field, UDP length field, as well as IP checksum field accordingly.
 * UDP checksum field will be zero out. (Not necessary in a LAN environment).
 */
void dhcp_relay_message(struct dhcp_lease *lease,
                        void *recv_packet, int udp_header_offset,
                        struct dhcp_msg *recv_msg, size_t recv_msg_size,
                        struct dhcp_option_info *opt_info)
{
   unsigned int all_header_size; // MAC+IP+UDP header size

   all_header_size = (void*)recv_msg - recv_packet;

   if (recv_msg->hdr.op == BOOTREPLY)
   {
      printf("Relay to ifIndex %d\n", lease->client_ifindex);

      if (opt_info->msgtype == DHCPOFFER || opt_info->msgtype == DHCPACK)
      {
         ui8 relay_packet_buffer[1514];
         ui8 *relay_udp_payload;
         ui32 max_udp_payload_size;
         struct dhcp_msg *relay_msg;
         struct ip_header *relay_iphdr;
         struct udp_header *relay_udphdr;
         unsigned short new_udp_len;
         unsigned short new_ip_len;

         relay_iphdr = (struct ip_header*)(relay_packet_buffer+MAC_HEADER_SIZE);
         relay_udphdr= (struct udp_header*)(relay_packet_buffer+udp_header_offset);

         relay_udp_payload = relay_packet_buffer + all_header_size;
         max_udp_payload_size = sizeof(relay_packet_buffer)-all_header_size;

         memcpy(relay_packet_buffer, recv_packet, all_header_size);

         relay_msg = (struct dhcp_msg*)relay_udp_payload;
         memcpy(relay_msg, recv_msg, sizeof(struct dhcp_msg_hdr));

         relay_msg->hdr.hops++;

         memset(relay_udp_payload+sizeof(struct dhcp_msg_hdr), 0,
                max_udp_payload_size-sizeof(struct dhcp_msg_hdr));

         // now this is the fun part: insert our own
         // Router and DNS Server options
         relay_msg->cookie = recv_msg->cookie;
         size_t option_size = 0;
         size_t max_option_size = max_udp_payload_size - sizeof(struct dhcp_msg);

         // Insert Router option if not found
         if (!opt_info->opt_router)
            option_size += dhcp_add_ipaddr_option(OPTION_ROUTER, g_my_ip,
                                                  relay_msg->option_data+option_size,
                                                  max_option_size-option_size);

         // Insert DNS option if not found
         if (!opt_info->opt_dns)
            option_size += dhcp_add_ipaddr_option(OPTION_DNS, g_my_ip,
                                                  relay_msg->option_data+option_size,
                                                  max_option_size-option_size);

         option_size += dhcp_encode_option_list(opt_info->option_list,
                                                relay_msg->option_data+option_size,
                                                max_option_size-option_size);

         if (opt_info->file_option_list)
         {
            dhcp_encode_option_list(opt_info->file_option_list,
                                    relay_msg->file, sizeof(relay_msg->file));
         }
         else
         {
            memcpy(relay_msg->file, recv_msg->file, sizeof(recv_msg->file));
         }

         if (opt_info->sname_option_list)
         {
            dhcp_encode_option_list(opt_info->sname_option_list,
                                    relay_msg->sname, sizeof(relay_msg->sname));
         }
         else
         {
            memcpy(relay_msg->sname, recv_msg->sname, sizeof(recv_msg->sname));
         }

         new_udp_len = UDP_HEADER_SIZE+sizeof(struct dhcp_msg)+option_size;
         new_ip_len = new_udp_len + udp_header_offset - MAC_HEADER_SIZE;

         // update UDP header
         relay_udphdr->len = htons(new_udp_len);
         relay_udphdr->cksum = 0; // do not calculate checksum for now

         // update IP header
         {
            unsigned int short old_ip_cksum, new_ip_cksum;

            old_ip_cksum = ~ntohs(relay_iphdr->cksum);

            new_ip_cksum = old_ip_cksum - ntohs(relay_iphdr->len) + new_ip_len;

	    /* CID 60037 : Bad bit shift operation
	     * These 2 lines of code have no effect.
	     * The value of old_ip_cksum is never used again, 
            old_ip_cksum = (old_ip_cksum>>16) + (old_ip_cksum&0xffff);
            old_ip_cksum = (old_ip_cksum>>16) + (old_ip_cksum&0xffff); */

            relay_iphdr->len = htons(new_ip_len);
            relay_iphdr->cksum = htons(~new_ip_cksum);
         }

         packet_sendto(lease->client_ifindex, relay_packet_buffer,
                       all_header_size+sizeof(struct dhcp_msg)+option_size, 0);
      }
      else
      {
         packet_sendto(lease->client_ifindex, recv_packet,
                       all_header_size+recv_msg_size, 0);
      }
   }
   else
   {
      printf("do not forward dhcp request message.\n");
   } 
}

/**
 * @brief Encode option list, for Router and DND Server option, replace it with
 *        my IP address.
 */
ssize_t dhcp_encode_option_list(const struct dhcp_option *option_list, ui8 *buf, size_t bufsize)
{
   ssize_t size;

   const struct dhcp_option *p_option;

   for (size = 0, p_option = option_list; p_option; p_option = p_option->next)
   {
      switch (p_option->code)
      {
         case OPTION_ROUTER:
         case OPTION_DNS:
            size += dhcp_add_ipaddr_option(p_option->code, g_my_ip, buf+size, bufsize-size);
            break;
         default:
            buf[size++] = p_option->code;
            buf[size++] = p_option->len;
            memcpy(buf+size, p_option->data, p_option->len);
            size += p_option->len;
      }
   }

   buf[size++] = OPTION_END;
   return size;
}

/**
 * @brief Free expired lease
 */
void dhcp_free_lease(struct dhcp_lease *lease)
{
   if (lease)
   {
      if (lease->hostname) free(lease->hostname);
      dhcp_cleanup_option(&lease->clientid);

      free(lease);
   }
}

/**
 * @brief Update Lease File
 *
 * Update lease file to the latest as well as purged expired/terminated lease from record.
 */
void dhcp_update_lease_file()
{
   struct dhcp_lease *this_lease, *prev_lease, *next_lease;
   FILE *fp;
   time_t now;

   fp = fopen(g_lease_file, "w");
   if (fp == NULL)
   {
      printf("Fail to open lease file %s for writing\n", g_lease_file);
      return;
   }

   now = time(0);

   for (this_lease = g_lease_list, prev_lease = NULL;
        this_lease;
        prev_lease = this_lease, this_lease = next_lease)
   {
      unsigned int i;

      next_lease = this_lease->next;

      if (this_lease->state == ST_BOUND)
      {
         if (now > this_lease->expiretime)
         {
            this_lease->state = ST_EXPIRED;
         }
      }

      if (this_lease->state == ST_INITIALIZING)
      {
         // clean up lease info if it remains in initializing state for over 600 seconds
         if (now > this_lease->lastmsgtime + 600)
         {
            this_lease->state = ST_TERMINATED;
         }
      }

      if (this_lease->state == ST_EXPIRED || this_lease->state == ST_TERMINATED)
      {
         // cleanup expired or terminated lease
         // remove lease record from linked list and free it
         if (prev_lease) prev_lease->next = next_lease;
         else g_lease_list = next_lease;
         if (this_lease == g_last_lease) g_last_lease = prev_lease;

         dhcp_free_lease(this_lease);
         this_lease = prev_lease;

         continue;
      }

      // First Column: Lease Time
      if (this_lease->state == ST_BOUND)
      {
         fprintf(fp, "%u ", this_lease->leasetime);
      }
      else if (this_lease->state == ST_STATIC)
      {
         fprintf(fp, "* ");
      }
      else
      {
         continue;
      }

      // Second Column: Hardware Address
      if (this_lease->htype != HTYPE_ETHERNET || this_lease->hlen == 0)
         fprintf(fp, "%.2x-", this_lease->htype);

      for (i=0;i<this_lease->hlen;i++)
      {
         if (i) fprintf(fp, ":");
         fprintf(fp, "%.2x", this_lease->chaddr[i]);
      }

      // Third Column: Leased IP Address
      fprintf(fp, " %s ", inet_ntoa(this_lease->yiaddr));

      // Fourth Column: Host Name
      if (this_lease->hostname)
      {
         fprintf(fp, "%s ", this_lease->hostname);
      }
      else
      {
         fprintf(fp, "* ");
      }

      // Fifth Column: Client ID
      if (this_lease->clientid.code == OPTION_CLIENTID)
      {
         for (i=0;i<this_lease->clientid.len;i++)
         {
            if (i) fprintf(fp, ":");
            fprintf(fp, "%.2x", this_lease->clientid.data[i]);
         }
      }
      else
      {
         fprintf(fp, "*");
      }

      fprintf(fp, "\n");
   }

   fclose(fp);

}
