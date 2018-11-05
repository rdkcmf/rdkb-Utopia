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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/filter.h>

#include "packet_util.h"

#define MAX_INTERFACES 20

int g_packet_socket;
struct packet_intf g_packet_interfaces[MAX_INTERFACES];
int g_num_interfaces = 0;

/**
 * @brief create packet socket
 */
int packet_init_socket()
{
   g_packet_socket = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
   if (g_packet_socket < 0)
   {
      perror("socket");
   }
   return g_packet_socket;
}

/**
 * @brief Attach socket filter
 */
int packet_attach_filter(struct sock_filter *filter, int filter_len)
{
   int err;
   struct sock_fprog fprog;

   fprog.len = filter_len;
   fprog.filter = filter;

   err = setsockopt(g_packet_socket, SOL_SOCKET, SO_ATTACH_FILTER, &fprog, sizeof(fprog));
   if (err < 0) 
   {
      perror("setsockop-SOL_SOCKET-SO_ATTACH_FILTER");
   }

   return err;
}

/**
 * @brief get IfIndex by interface name
 */
int get_ifindex(const char *device_name)
{
   int err;
   struct ifreq ifr;

   memset(&ifr, 0, sizeof(ifr));
   strncpy(ifr.ifr_name, device_name, sizeof(ifr.ifr_name)-1);

   err = ioctl(g_packet_socket, SIOCGIFINDEX, &ifr);

   if (err < 0)
   {
      perror("ioctl-SIOCGIFINDEX");
      return -1;
   }
   else
   {
      return ifr.ifr_ifindex;
   }
}

/**
 * @brief add an IP interface to packet socket
 *
 * @param ifname interface name
 * @param iftype interface type, opaque to packet util
 *
 * @return 0 if successful
 * @return -1 if too many interfaces or invalid interface
 */
int packet_add_interface(const char *ifname, int iftype)
{
   int ifindex;

   if (g_num_interfaces >= MAX_INTERFACES)
   {
      printf("Too many interfaces: %d >= %d\n",
             g_num_interfaces, MAX_INTERFACES);
      return -1;
   }

   if ((ifindex = get_ifindex(ifname)) < 0)
   {
      return -1;
   }

   printf("add interface %d %s %d %d\n",
          g_num_interfaces, ifname, iftype, ifindex);

   strncpy(g_packet_interfaces[g_num_interfaces].ifname, ifname, IFNAMSIZ-1);
   g_packet_interfaces[g_num_interfaces].ifname[IFNAMSIZ-1] = '\0';

   g_packet_interfaces[g_num_interfaces].ifindex = ifindex;
   g_packet_interfaces[g_num_interfaces].iftype = iftype;

   g_num_interfaces++;

   return ifindex;
}

/**
 * @brief Bind packet to IP protocol
 */
int packet_bind_socket()
{
   int err;
   struct sockaddr_ll bind_addr;

   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sll_family = AF_PACKET;
   bind_addr.sll_protocol = htons(ETH_P_ALL);
   // bind_addr.sll_ifindex = 0;

   err = bind(g_packet_socket, (const struct sockaddr*)&bind_addr, sizeof(bind_addr));
   if (err < 0)
   {
      perror("bind");
   }

   return err;
}


/**
 * @brief Receive data from packet socket
 * @param buf  buffer provided by caller
 * @param len  buffer size
 * @param flags 
 * @param from_addr used to return socket addrss
 * @param from_intr used to return interface
 * @param timeout   optional timeout value
 * @return -1 if error
 * @return 0 if timeout
 * @return size if data received
 */
int packet_recvfrom(void *buf, size_t len, int flags,
                    struct sockaddr_ll *from_addr,
                    struct packet_intf **from_intf,
                    struct timeval *timeout)
{
   ssize_t size;
   socklen_t recv_addr_len;

   if (timeout)
   {
      int rc;
      fd_set rfds;

      FD_ZERO(&rfds);
      FD_SET(g_packet_socket, &rfds);

      rc = select(FD_SETSIZE, &rfds, 0, 0, timeout);

      if (rc < 0)
      {
         if (errno != EINTR) perror("select");
         return rc;
      }
      else if (rc == 0)
      {
         return 0; // timeout
      }
   }

   recv_addr_len = sizeof(struct sockaddr_ll);

   size = recvfrom(g_packet_socket, buf, len, flags, 
                   (struct sockaddr*)from_addr, &recv_addr_len);

   if (size < 0)
   {
      perror("recvfrom");
      return size;
   }

   if (from_intf && from_addr)
   {
      int i;

      *from_intf = NULL;

      for (i=0;i<g_num_interfaces;i++)
      {
         if (g_packet_interfaces[i].ifindex == from_addr->sll_ifindex)
         {
            *from_intf = &g_packet_interfaces[i];
            break;
         }
      }
   }

   return size;
}


/**
 * @brief Set data to packet socket
 * @param ifindex IfIndex of the interface used to send packet
 * @param buf     data to be sent
 * @param len     data size
 * @param flags
 * @return -1 error
 * @return size of data sent
 */
int packet_sendto(int ifindex, const void *buf, size_t len, int flags)
{
   struct sockaddr_ll to_addr;
   ssize_t rc;

   memset(&to_addr, 0, sizeof(to_addr));
   to_addr.sll_family   = AF_PACKET;
   to_addr.sll_ifindex  = ifindex;
   to_addr.sll_halen    = MAC_ADDRESS_LEN;
   // First 6 bytes of Ethernet MAC Header is Destination MAC Address
   memcpy(to_addr.sll_addr, buf, MAC_ADDRESS_LEN); 

   rc = sendto(g_packet_socket, buf, len, flags,
               (const struct sockaddr*)&to_addr, sizeof(to_addr));

   if (rc < 0)
   {
      perror("rc");
   }

   return rc;
}

/**
 * @brief Validate and Parse Ethernet Packet
 * @param packet ethernet packet
 * @param size ethernet packet size
 * @param machdr used to return mac header
 * @param iphdr used to return ip header
 * @param udphdr used to return udp header
 * @param udp_payload_size used to return UDP payload size
 * @return NULL if parse/validation fails
 * @return pointer to UDP payload
 */
void *parse_and_validate_ethernet_packet(void *packet, unsigned int size,
                                         struct mac_header **machdr,
                                         struct ip_header **iphdr,
                                         struct udp_header **udphdr,
                                         unsigned int *udp_payload_size)
{
   *machdr = (struct mac_header*)packet;
   *iphdr = get_ip_header(*machdr, size);
   if (*iphdr == NULL || is_packet_truncated(*iphdr, size-MAC_HEADER_SIZE))
   {
      printf("No IPHDR\n");
      return NULL;
   }
   *udphdr = get_udp_header(*iphdr, size-MAC_HEADER_SIZE);
   if (*udphdr == NULL || is_ip_fragmented(*iphdr, *udphdr))
   {
      printf("NO UDP Header or fragmented\n");
      return NULL;
   }
   *udp_payload_size = ntohs((*udphdr)->len) - UDP_HEADER_SIZE;
   return get_udp_payload(*udphdr);
}

