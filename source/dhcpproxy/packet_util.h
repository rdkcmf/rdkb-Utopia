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

#ifndef _PACKET_UTIL_H_
#define _PACKET_UTIL_H_

#include <netpacket/packet.h>
#include <net/if.h>
#include <linux/filter.h>
#include <arpa/inet.h>
#include <sys/time.h>

/**
 * @brief Network interface
 */
struct packet_intf
{
   char ifname[IFNAMSIZ];
   int  ifindex;
   int  iftype;
};

#define MAC_ADDRESS_LEN 6

/**
 * @brief Ethernet MAC Header
 */
struct mac_header
{
   unsigned char dst[MAC_ADDRESS_LEN];
   unsigned char src[MAC_ADDRESS_LEN];
   unsigned short proto;
};

/**
 * @brief IP header, not including option field
 */
struct ip_header
{
   unsigned char vihl; // Version + IP header length
   unsigned char tos;
   unsigned short len;
   unsigned short id;
   unsigned short frag;
   unsigned char ttl;
   unsigned char proto;
   unsigned short cksum;
   unsigned int src;
   unsigned int dst;
};

/**
 * @brief UDP header
 */
struct udp_header
{
   unsigned short src;
   unsigned short dst;
   unsigned short len;
   unsigned short cksum;
};

#define MAC_HEADER_SIZE sizeof(struct mac_header)
#define MIN_IP_HEADER_SIZE sizeof(struct ip_header)
#define UDP_HEADER_SIZE sizeof(struct udp_header)

#define MF_FLAG 0x2000  // more fragment flag
#define DF_FLAG 0x4000  // don't fragment flag

#ifdef __cplusplus
extern "C" {
#endif

int packet_init_socket();

int get_ifindex(const char *device_name);

int packet_attach_filter(struct sock_filter *filter, int filter_len);

int packet_add_interface(const char *ifname, int iftype);

int packet_bind_socket();

int packet_recvfrom(void *buf, size_t len, int flags,
                    struct sockaddr_ll *from_addr,
                    struct packet_intf **from_intf,
                    struct timeval *timeout);

int packet_sendto(int ifindex, const void *buf, size_t len, int flags);


/**
 * @brief return IP header
 * @param mac_header
 * @param size - ethernet packet size
 */
static inline struct ip_header *get_ip_header(void *mac_header, unsigned int size)
{
   return size < sizeof(struct mac_header)+MIN_IP_HEADER_SIZE ? NULL :
         (struct ip_header*)(mac_header+sizeof(struct mac_header));
}

/**
 * @brief return IP header size
 */
static inline unsigned int get_ip_header_size(const struct ip_header *iph)
{
   return (iph->vihl&0xf)<<2;
}

/**
 * @brief return IP payload pointer
 * @param iph IP header
 * @param size of IP packet
 */
static inline void *get_ip_payload(struct ip_header *iph, unsigned int size)
{
   unsigned int iph_size = get_ip_header_size(iph);

   return (iph_size < MIN_IP_HEADER_SIZE) | (iph_size > size) ? NULL :
          (void*)iph + iph_size;
}

/**
 * @brief return UDP header
 * @param ip_hdr IP header
 * @param size   IP packet size
 */
static inline struct udp_header *get_udp_header(void *ip_hdr, unsigned int size)
{
   struct ip_header *iph = (struct ip_header*)ip_hdr;

   return get_ip_header_size(iph) + UDP_HEADER_SIZE > size ? NULL :
          (struct udp_header *)get_ip_payload(iph, size);
}

/**
 * @param iph IP header
 * @param size data size starting from IP header
 * @return 1 if IP packet is truncated
 */
static inline int is_packet_truncated(const struct ip_header *iph, unsigned int size)
{
   return size < ntohs(iph->len);
}

/**
 * @param iph IP header
 * @param udph UDP header
 * @return 1 if it is a fragmented packet or IP packet does not contain
 *           entire UDP packet
 */
static inline int is_ip_fragmented(const struct ip_header *iph,
                     const struct udp_header *udph)
{
   return (MF_FLAG & ntohs(iph->frag)) ||
          ntohs(udph->len) + get_ip_header_size(iph) > ntohs(iph->len);
}

/**
 * @brief return UDP payload pointer
 * @param udp_hdr  UDP header
 */
static inline void *get_udp_payload(void *udp_hdr)
{
   return udp_hdr + UDP_HEADER_SIZE;
}

void *parse_and_validate_ethernet_packet(void *packet, unsigned int size,
                                         struct mac_header **machdr,
                                         struct ip_header **iphdr,
                                         struct udp_header **udphdr,
                                         unsigned int *udp_payload_size);

#ifdef __cplusplus
}
#endif

#endif // _PACKET_UTIL_H_


