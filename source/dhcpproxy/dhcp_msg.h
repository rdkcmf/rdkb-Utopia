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

#ifndef _DHCP_MSG_H_
#define _DHCP_MSG_H_

#include <netinet/in.h>

typedef unsigned char ui8;
typedef unsigned short ui16;
typedef unsigned int ui32;

/**
 * @brief DHCP Message header, exclude sname and file
 */
struct dhcp_msg_hdr
{
  ui8 op;
  ui8 htype;
  ui8 hlen;
  ui8 hops;
  ui32 xid;
  ui16 secs;
  ui16 flags;
  struct in_addr ciaddr;
  struct in_addr yiaddr;
  struct in_addr siaddr;
  struct in_addr giaddr;
  ui8 chaddr[16];
};

/**
 * @brief BOOTP Operation Type
 */
enum {
  BOOTREQUEST=1,
  BOOTREPLY=2
};
#define HTYPE_ETHERNET 1                   ///< htype = ethernet
#define BROADCAST_FLAG 0x8000              ///< Broadcast Flag

/**
 * @brief DHCP Message
 */
struct dhcp_msg
{
  struct dhcp_msg_hdr hdr;
  ui8 sname[64];
  ui8 file[128];
  ui32 cookie;
  ui8 option_data[];
};
#define DHCP_COOKIE 0x63825363             ///< DHCP Magic Cookie

/**
 * @brief DHCP option in TLV format
 */
struct dhcp_option
{
  ui8 code;
  ui8 len;
  ui8 *data;
  struct dhcp_option *next;
};

/**
 * @brief DHCP option code
 */
enum {
  OPTION_PAD=0, 
  OPTION_ROUTER=3,
  OPTION_DNS=6,
  OPTION_HOSTNAME=12,
  OPTION_LEASETIME=51,
  OPTION_OVERLOAD=52,
  OPTION_MSGTYPE=53,
  OPTION_SERVERID=54,
  OPTION_CLIENTID=61,
  OPTION_END=255
};

/**
 * @brief Additional DHCP Message Info from options
 */
struct dhcp_option_info
{
   ui8 msgtype;                             ///< DHCP message type (option 53)
   int overload;                            ///< DHCP overload flag (option 52)
   struct in_addr server_ip;                ///< DHCP server IP (option 54)
   ui32 leasetime;                          ///< DHCP lease time (option 51)
   struct dhcp_option *opt_hostname;        ///< hostname (option  12)
   struct dhcp_option *opt_clientid;        ///< client ID (option 61)
   struct dhcp_option *opt_router;          ///< Routers (option 3)
   struct dhcp_option *opt_dns;             ///< DNS servers (option 6)
   struct dhcp_option *option_list;         ///< List of option in option field
   struct dhcp_option *file_option_list;    ///< List of option in file field
   struct dhcp_option *sname_option_list;  ///< List of option in sname field
};

#define OPTION_IN_FILE 0x01                ///< file field is used to store additional options
#define OPTION_IN_SNAME 0x02               ///< sname field is used to store additional options

/**
 * @brief DHCP message type (option 53)
 */
enum {
  DHCPDISCOVER=1,
  DHCPOFFER=2,
  DHCPREQUEST=3,
  DHCPDECLINE=4,
  DHCPACK=5,
  DHCPNAK=6,
  DHCPRELEASE=7,
  DHCPINFORM=8
};

#define INFINITE_LEASETIME 0xFFFFFFFF

#ifdef __cplusplus
extern "C" {
#endif

int compare_option(const struct dhcp_option *opt1, const struct dhcp_option *opt2);

void dhcp_copy_option(struct dhcp_option *dst, const struct dhcp_option *src);

void dhcp_cleanup_option(struct dhcp_option *opt);

void dhcp_clear_option_info(struct dhcp_option_info *opt_info);

struct dhcp_option *dhcp_parse_options(struct dhcp_option_info *opt_info,
                                       ui8* opt_data, size_t size);

void dhcp_parse_msg(struct dhcp_option_info *opt_info, struct dhcp_msg *msg, size_t size);

int dhcp_validate_msg(const struct dhcp_msg *msg, const struct dhcp_option_info *opt_info);

size_t dhcp_add_ipaddr_option(ui8 code, struct in_addr ipaddr, ui8* buf, size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif // _DHCP_MSG_H_

