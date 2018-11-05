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

#ifndef _DHCP_UTIL_H_
#define _DHCP_UTIL_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "dhcp_msg.h"

void dhcp_send(int s, const unsigned char *msg, unsigned int size, struct sockaddr_in dest_addr);

void insert_arp_record(int s, const char *device_name,
                       struct sockaddr_in ip_addr,
                       ui8 htype, ui8 hlen, ui8* chaddr);

int dhcp_create_socket(int port, const char* device_name);

int dhcp_recvfrom_to(int s, void *buf, size_t len, int flags,
                     struct sockaddr_in *from,
                     struct in_addr *to);


void dump_data(const unsigned char *data, size_t size);

void dump_data_short(const unsigned char *data, size_t size);

void dump_mac_addr(const void *param);

void dump_ip_addr(const void *param);

void dump_ip_list(const unsigned char *data, size_t size);

void dump_dhcp_option(const struct dhcp_option *p_option);

void dump_dhcp_option_list(const struct dhcp_option *option_list);

void dump_dhcp_msg(const struct dhcp_msg *msg, const struct dhcp_option_info *opts);

#endif // _DHCP_UTIL_H_
