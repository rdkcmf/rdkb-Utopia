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

#ifndef __FIREWALL_H__
#define __FIREWALL_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <string.h>   // strcasestr needs __USE_GNU

#include <errno.h>

#include "safec_lib_common.h"

#include "syscfg/syscfg.h"
#include "sysevent/sysevent.h"

#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"
#include "firewall_custom.h"
#include "secure_wrapper.h"
#include "safec_lib_common.h"

int do_logs(FILE *fp);
int do_wan2self_attack(FILE *fp,char* wan_ip);
int prepare_ipv4_firewall(const char *fw_file);
int prepare_ipv6_firewall(const char *fw_file);

#define CCSP_SUBSYS "eRT."

#define IF_IPV6ADDR_MAX 16

#define IPV6_ADDR_SCOPE_MASK    0x00f0U
#define IPV6_ADDR_SCOPE_GLOBAL  0
#define IPV6_ADDR_SCOPE_LINKLOCAL     0x0020U
#define _PROCNET_IFINET6  "/proc/net/if_inet6"
#define MAX_INET6_PROC_CHARS 200

extern void* bus_handle ;
#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(bus_handle, CCSP_SUBSYS, name, NULL, &(str)) 

int get_ip6address (char * ifname, char ipArry[][40], int * p_num, unsigned int scope_in);

#ifdef WAN_FAILOVER_SUPPORTED

void  redirect_dns_to_extender(FILE *nat_fp,int family);

typedef enum {
    ROUTER =0,
    EXTENDER_MODE,
} Dev_Mode;

unsigned int Get_Device_Mode() ;

char* get_iface_ipaddr(const char* iface_name);

#endif

#ifdef RDKB_EXTENDER_ENABLED

void add_if_mss_clamping(FILE *mangle_fp,int family);
int service_start_ext_mode () ;

int prepare_ipv4_rule_ex_mode(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp);
int prepare_ipv6_rule_ex_mode(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp);
int isExtProfile();
#endif
#endif