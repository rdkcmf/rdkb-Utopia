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
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include "ccsp_custom.h"
extern FILE *firewallfp;
#define FW_DEBUG 1


void do_device_based_pp_disabled_appendrule(FILE *fp, const char *ins_num, const char *lan_ifname, const char *query);
void do_device_based_pp_disabled_ip_appendrule(FILE *fp, const char *ins_num, const char *ipAddr);
int do_parcon_mgmt_lan2wan_pc_site_appendrule(FILE *fp);
void do_parcon_mgmt_lan2wan_pc_site_insertrule(FILE *fp, int index, char *nstdPort);
void firewall_log( char* fmt, ...);
void update_rabid_features_status();
void do_forwardPorts(FILE *filter_fp);
void do_snmp_IpAccessTable(FILE *filt_fp, int family);
void do_ssh_IpAccessTable(FILE *filt_fp, const char *port, int family, const char *interface);
void do_tr69_whitelistTable(FILE *filt_fp, int family);
void filterPortMap(FILE *filt_fp);
void do_openPorts(FILE *filter_fp);
int prepare_xconf_rules(FILE *mangle_fp);
int do_self_heal_rules_v6(FILE *mangle_fp);
int do_qos_output_marking_v6(FILE *mangle_fp);
int do_hub4_mapt_rules_v6(FILE *filter_fp);
int do_hub4_bfd_rules_v6(FILE *filter_fp, FILE *mangle_fp);
int do_hub4_dns_rule_v6(FILE* mangle_fp);
int do_hub4_voice_rules_v6(FILE *filter_fp, FILE *mangle_fp);
int do_self_heal_rules_v4(FILE *mangle_fp);
int do_qos_output_marking_v4(FILE *mangle_fp);
int do_hub4_mapt_rules_v4(FILE *nat_fp, FILE *filter_fp);
int do_hub4_bfd_rules_v4(FILE *nat_fp, FILE *filter_fp, FILE *mangle_fp);
int do_hub4_voice_rules_v4(FILE *filter_fp);
int isInSelfHealMode ();
char *get_lan_ipaddr();
char *get_current_wan_ifname();
void ethwan_mso_gui_acess_rules(FILE *filter_fp,FILE *mangle_fp);
void do_OpenVideoAnalyticsPort (FILE *filter_fp);

#ifdef FW_DEBUG
#define COMMA ,
#define FIREWALL_DEBUG(x) firewall_log(x);
#else 
#define FIREWALL_DEBUG(x)
#endif

#define SHM_MUTEX "FirewallMutex"

typedef enum {
    IP_V4 = 0,
    IP_V6,
}ip_ver_t;

typedef struct fw_shm_mutex {
  pthread_mutex_t *ptr; 
  int fw_shm_create;        
  int fw_shm_fd;       
  char fw_mutex[32];        

} fw_shm_mutex;


fw_shm_mutex fw_shm_mutex_init(char *name);

int fw_shm_mutex_close(fw_shm_mutex mutex);

/*
 *  rdkb_arm is same as 3939/3941
 *
#define CONFIG_CCSP_LAN_HTTP_ACCESS
#define CONFIG_CCSP_VPN_PASSTHROUGH
 */
#if defined (INTEL_PUMA7) || (defined (_COSA_BCM_ARM_) && !defined(_CBR_PRODUCT_REQ_))
#define CONFIG_CCSP_VPN_PASSTHROUGH
#endif

#if defined (INTEL_PUMA7)
#define CONFIG_KERNEL_NETFILTER_XT_TARGET_CT
#endif
#define CONFIG_CCSP_WAN_MGMT
#define CONFIG_CCSP_WAN_MGMT_PORT
//#define CONFIG_CCSP_WAN_MGMT_ACCESS //defined in ccsp_custom.h
#ifndef _HUB4_PRODUCT_REQ_
#define CONFIG_CCSP_CM_IP_WEBACCESS
#endif /* * !_HUB4_PRODUCT_REQ_ */

#endif
