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

#ifndef _DHCP_PROXY_H_
#define _DHCP_PROXY_H_

#include <time.h>
#include <netinet/in.h>
#include "dhcp_msg.h"
#include "packet_util.h"

/**
 * @brief DHCP lease structure maintained by proxy
 */
struct dhcp_lease
{
  int state;                       ///< defined below
  int client_ifindex;
  ui8 htype;
  ui8 hlen;
  ui8 chaddr[16];
  ui8 last_msg;                    ///< last received DHCP message type
  ui32 last_xid;                   ///< xid in last received DHCP message
  time_t lastmsgtime;              ///< time when last DHCP message received
  struct in_addr ciaddr;
  struct in_addr yiaddr;
  struct in_addr giaddr;
  struct in_addr server_ip;
  char *hostname;
  struct dhcp_option clientid;
  time_t boundtime;                ///< time when last ACK message received
  ui32 leasetime;                  ///< lease time in last ACK
  time_t expiretime;               ///< lease expiration time or terminated time
  struct dhcp_lease *next;
};

#define DEFAULT_MAX_HOPS 4
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68


/**
 * @brief Proxy State
 */
enum
{
  ST_INITIALIZING = 1,   ///< acquiring lease (after discover, offer, or request)
  ST_BOUND = 2,          ///< lease bound after ack
  ST_TERMINATED = 3,     ///< lease terminated by decline, relase, or nak
  ST_EXPIRED = 4,        ///< lease expired
  ST_STATIC = 5,         ///< static IP (after inform)
};

/**
 * @brief Interface Type to distinguish WAN and LAN internally
 */
enum
{
   WAN_INTERFACE_TYPE = 0,
   LAN_INTERFACE_TYPE = 1,
};

extern struct dhcp_lease *g_lease_list, *g_last_lease;
extern struct in_addr g_my_ip;

#ifdef __cplusplus
extern "C" {
#endif

struct dhcp_lease *dhcp_find_lease(const struct dhcp_msg *msg,
                                   const struct dhcp_option_info *opt_info,
                                   struct dhcp_lease **pprev);

struct dhcp_lease* dhcp_process_msg(struct dhcp_msg *recv_msg, size_t recv_msg_size,
                                    struct dhcp_option_info *opt_info,
                                    int recv_ifindex, int recv_iftype);

void dhcp_relay_message(struct dhcp_lease *lease,
                        void *recv_packet, int udp_header_offset,
                        struct dhcp_msg *recv_msg, size_t recv_msg_size,
                        struct dhcp_option_info *opt_info);

ssize_t dhcp_encode_option_list(const struct dhcp_option *option_list, ui8 *buf, size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif // _DHCP_PROXY_H_

