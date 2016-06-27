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

#ifndef __FIREWALL_CUSTOM_H__
#define __FIREWALL_CUSTOM_H__


#include <stdio.h>
#include<stdlib.h>
#include "ccsp_custom.h"
extern FILE *firewallfp;
#define FW_DEBUG 1


void do_device_based_pp_disabled_appendrule(FILE *fp, const char *ins_num, const char *lan_ifname, const char *query);
void do_device_based_pp_disabled_ip_appendrule(FILE *fp, const char *ins_num, const char *ipAddr);
int do_parcon_mgmt_lan2wan_pc_site_appendrule(FILE *fp);
void do_parcon_mgmt_lan2wan_pc_site_insertrule(FILE *fp, int index, char *nstdPort);

#ifdef FW_DEBUG
#define COMMA ,
#define FIREWALL_DEBUG(x) \
if(firewallfp != NULL){ \
fprintf(firewallfp,"------FIREWALL_DEBUG:" x);}\
else \
printf(" FILE Pointer is NULL \n"); 
#else 
#define FIREWALL_DEBUG(x)
#endif

/*
 *  rdkb_arm is same as 3939/3941
 *
#define CONFIG_CCSP_LAN_HTTP_ACCESS
#define CONFIG_CCSP_VPN_PASSTHROUGH
 */
#define CONFIG_CCSP_WAN_MGMT
#define CONFIG_CCSP_WAN_MGMT_PORT
//#define CONFIG_CCSP_WAN_MGMT_ACCESS //defined in ccsp_custom.h
#define CONFIG_CCSP_CM_IP_WEBACCESS

#endif
