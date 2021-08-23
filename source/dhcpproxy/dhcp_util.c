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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "dhcp_util.h"
#include "safec_lib_common.h"

/**
 * @brief Send a DHCP Message
 */
void dhcp_send(int s, const unsigned char *msg, unsigned int size, struct sockaddr_in dest_addr)
{
   unsigned char *msg_data = (unsigned char*)msg;

   if (sendto(s, msg_data, size, 0,
              (const struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0)
   {
      perror("sendto");
   }
}

/**
 * @brief Insert ARP record, required if DHCP message is unicast to a host
 *        that has not finished IP initialization
 */
void insert_arp_record(int s, const char *device_name,
                       struct sockaddr_in ip_addr,
                       ui8 htype, ui8 hlen, ui8* chaddr)
{
   struct arpreq req;
   errno_t  rc  = -1;

   *((struct sockaddr_in *)&req.arp_pa) = ip_addr;
   req.arp_ha.sa_family = htype;
   memcpy(req.arp_ha.sa_data, chaddr, hlen);
   rc = strcpy_s(req.arp_dev, sizeof(req.arp_dev), device_name);
   ERR_CHK(rc);

   req.arp_flags = ATF_COM;

   if (ioctl(s, SIOCSARP, &req) < 0)
   {
      perror("ioctl-SIOCSARP");
   }
}

/**
 * @brief Create a DHCP socket, bind to port and device
 */
int dhcp_create_socket(int port, const char* device_name)
{
   int sock;
   struct sockaddr_in serv_addr;
   memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero)); //CID 163232: Uninitialized scalar variable
   const int flag_one = 0x00; /*RDKB-7146, CID-33246, initialize before use*/

   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0)
   {
      perror("socket");
      return -1;
   }

   if (device_name)
   {
      if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, device_name, strlen(device_name)+1) < 0)
      {
         perror("setsockopt-SOL_SOCKET-SO_BINDTODEVICE");
         close(sock);
         return -1;
      }
   }

   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag_one, sizeof(flag_one)) < 0)
   {
      perror("setsockopt-SOL_SOCKET-SO_REUSEADDR");
      close(sock);
      return -1;
   }

   if (setsockopt(sock, SOL_IP, IP_PKTINFO, &flag_one, sizeof(flag_one)) < 0)
   {
      perror("setsockopt-SOL_IP-IP_PKTINFO");
      close(sock);
      return -1;
   }

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(port);
   serv_addr.sin_addr.s_addr = INADDR_ANY;

   if (bind(sock, (const struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
   {
      perror("bind");
      close(sock);
      return -1;
   }

   return sock;
}

/**
 * @brief Receive packet from DHCP socket with source and destination
 *        IP address
 */
int dhcp_recvfrom_to(int s, void *buf, size_t len, int flags,
                     struct sockaddr_in *from,
                     struct in_addr *to)
{
   struct msghdr msg = {0};
   struct iovec iov = {0};
   unsigned char msg_control[100] = {0};
   ssize_t size;

   memset(&msg, 0, sizeof(msg)); /*RDKB-7146, CID-33564, initialize before use*/

   iov.iov_base = buf;
   iov.iov_len = len;

   msg.msg_name = from;
   msg.msg_namelen = sizeof(*from);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = msg_control;
   msg.msg_controllen = sizeof(msg_control);

   size = recvmsg(s, &msg, flags);
   if (size >= 0)
   {
      struct cmsghdr *p_cmsg;

      p_cmsg = (struct cmsghdr*)msg_control;
      if (p_cmsg->cmsg_len > 0 &&
          p_cmsg->cmsg_level == SOL_IP &&
          p_cmsg->cmsg_type == IP_PKTINFO)
      {
         struct in_pktinfo* p_pktinfo;

         p_pktinfo = (struct in_pktinfo*)&msg_control[sizeof(struct cmsghdr)];

         to->s_addr = p_pktinfo->ipi_addr.s_addr;
      }
   }
   else
   {
      perror("recvmsg");
   }

   return size;
}

/**
 * The following functions are for diagnostic purposes only
 */

/**
 * @brief Dump Binary Data
 */
void dump_data(const unsigned char *data, size_t size)
{
   unsigned int i, j;
   for (i=0;i<size;i+=16)
   {
      printf("%04x: ", i);
      for (j=i;j<i+16;j++)
      {
         if (j < size) printf(" %02x", data[j]);
         else printf("   ");
      }
      printf("   ");
      for (j=i;j<i+16;j++)
      {
         if (j < size)
         {
            if (isprint(data[j])) printf("%c", data[j]);
            else printf(".");
         }
         else break;
      }
      printf("\n");
   }
}

/**
 * @brief Dump Binary Data in short form
 */
void dump_data_short(const unsigned char *data, size_t size)
{
   unsigned int i;
   for (i=0;i<size;i++) printf(" %02x", data[i]);
   printf(" -ASCII- ");
   for (i=0;i<size;i++) printf("%c", isprint(data[i]) ? data[i] : '.');
   printf("\n");
}

/**
 * @brief Dump MAC address
 */
void dump_mac_addr(const void *param)
{
   const unsigned char *data = (const unsigned char*)param;
   printf("%02x:%02x:%02x:%02x:%02x:%02x", data[0], data[1], data[2], data[3], data[4], data[5]);
}

/**
 * @brief Dump IP address
 */
void dump_ip_addr(const void *param)
{
   const unsigned char *data = (const unsigned char*)param;
   printf("%d.%d.%d.%d", data[0], data[1], data[2], data[3]);
}

/**
 * @brief Dump a list of IP address
 */
void dump_ip_list(const unsigned char *data, size_t size)
{
   unsigned i;
   for (i=0;i<size;i++)
   {
      if (i%4!=0) printf(".");
      else if (i!=0) printf(", ");
      printf("%d", data[i]);
   }
}

/**
 * @brief Dump DHCP Option
 */
void dump_dhcp_option(const struct dhcp_option *p_option)
{
   switch (p_option->code)
   {
      case OPTION_ROUTER:
         printf("router(s)= ");
         dump_ip_list(p_option->data, p_option->len);
         printf("\n");
         break;
      case OPTION_DNS:
         printf("dns server(s)= ");
         dump_ip_list(p_option->data, p_option->len);
         printf("\n");
         break;
      case OPTION_LEASETIME:
         printf("lease time = ");
         printf("%d seconds\n", p_option->data[0] << 24 | p_option->data[1] << 16 |
                 p_option->data[2] << 8 | p_option->data[3]);
         break;
      case OPTION_MSGTYPE:
         printf("DHCP Message Type: %d(%s)\n", p_option->data[0],
                p_option->data[0] == DHCPDISCOVER ? "DHCPDISCOVER" :
                p_option->data[0] == DHCPOFFER ? "DHCPOFFER" :
                p_option->data[0] == DHCPREQUEST ? "DHCPREQUEST" :
                p_option->data[0] == DHCPDECLINE ? "DHCPDECLINE" :
                p_option->data[0] == DHCPACK ? "DHCPACK" :
                p_option->data[0] == DHCPNAK ? "DHCPNAK" :
                p_option->data[0] == DHCPRELEASE ? "DHCPRELEASE" :
                p_option->data[0] == DHCPINFORM ? "DHCPINFORM" : "???");
         break;
      case OPTION_SERVERID:
         printf("Server Identifier: ");
         if (p_option->len == 4)
         {
            dump_ip_addr(p_option->data);
         }
         else
         {
            dump_data_short(p_option->data, p_option->len);
         }
         printf("\n");
         break;
      case OPTION_CLIENTID:
         printf("Client Identifier: ");
         if (p_option->data[0] == 1 && p_option->len == 7)
         {
            printf("ether ");
            dump_mac_addr(p_option->data+1);
         }
         else
         {
            dump_data_short(p_option->data, p_option->len);
         }
         printf("\n");
         break;
      default:
         printf("option %d: ", p_option->code);
         if (p_option->len <= 16) dump_data_short(p_option->data, p_option->len);
         else
         {
            printf("\n");
            dump_data(p_option->data, p_option->len);
         }
         break;
   }
}

/**
 * @brief Dump DHCP Option List
 */
void dump_dhcp_option_list(const struct dhcp_option *option_list)
{
   const struct dhcp_option *p_option;
   for (p_option = option_list; p_option; p_option = p_option->next)
   {
      dump_dhcp_option(p_option);
   }
}

/**
 * @brief Dump DHCP message
 */
void dump_dhcp_msg(const struct dhcp_msg *msg, const struct dhcp_option_info *opts)
{
   printf("op=%d htype=%d hlen=%d hops=%d xid=0x%08x secs=%d flags=0x%04x\n",
          msg->hdr.op, msg->hdr.htype, msg->hdr.hlen, msg->hdr.hops,
          htonl(msg->hdr.xid), htonl(msg->hdr.secs), htonl(msg->hdr.flags));
   printf("ciaddr=%s\n", inet_ntoa(msg->hdr.ciaddr));
   printf("yiaddr=%s\n", inet_ntoa(msg->hdr.yiaddr));
   printf("siaddr=%s\n", inet_ntoa(msg->hdr.siaddr));
   printf("giaddr=%s\n", inet_ntoa(msg->hdr.giaddr));
   printf("chaddr=");
   if (msg->hdr.htype == 1 && msg->hdr.hlen == 6) dump_mac_addr(msg->hdr.chaddr);
   else dump_data_short(msg->hdr.chaddr, msg->hdr.hlen);
   printf("\n");
   if (msg->sname[0] && (!opts || !opts->sname_option_list)) printf("sname=%s\n", msg->sname);
   if (msg->file[0] && (!opts || !opts->file_option_list)) printf("file=%s\n", msg->file);

   if (opts)
   {
      dump_dhcp_option_list(opts->option_list);
      dump_dhcp_option_list(opts->file_option_list);
      dump_dhcp_option_list(opts->sname_option_list);
   }
}
