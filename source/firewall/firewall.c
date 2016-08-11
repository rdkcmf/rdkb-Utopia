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
 * Copyright (c) 2009 by Cisco Systems, Inc. All Rights Reserved.
 *
 * This work is subject to U.S. and international copyright laws and
 * treaties. No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged, condensed,
 * expanded, collected, compiled, linked, recast, transformed or adapted
 * without the prior written consent of Cisco Systems, Inc. Any use or
 * exploitation of this work without authorization could subject the
 * perpetrator to criminal and civil liability.
 */

/*
 ============================================================================

 Introduction to IPv4 Firewall
 -------------------------------
 
 The firewall is based on iptables. It uses the mangle, nat, and filters tables,
 and for each of these, it add several subtables.

 The reason for using subtables is that a subtable represents a block of rules
 which can be erased (using -F), and reconstituted using syscfg and sysevent, 
 without affecting the rest of the firewall. That makes its easier to organize
 a complex firewall into smaller functional groups. 

 The main tables, INPUT OUTPUT, and FORWARD, contain jumps to subtables that better represent
 a Utopia firewall: wan2self, lan2self, lan2wan, wan2wan. Each of these subtables
 further specifies the order of rules and jumps to further subtables. 
 
 As mentioned earlier, the firewall is iptables based. There are two ways to use iptables:
 iptables-restore using an input file, or issuing a series of iptables commands. Using iptables-restore
 disrupts netfilters connection tracking which causes established connections to appear to be invalid.
 Using iptables is slower, and it requires that Utopia firewall table structure already exists. This means
 that it cannot be used to initially structure the firewall. 

 The behavior of firewall.c is to check whether the iptables file (/tmp/.ipt)
 exists. If it doesn't exist, then a new one is created and instantiated via iptables-restore.
 On the other hand if .ipt already exists, then all subtables are flushed and reconstituted
 using iptables rules. 

 Here is a list of subtables and how each subtable is populated:
 Note that some syscfg/sysevent tuples are used to populate more than one subtable

 raw
 ---
   prerouting_ephemeral:
   output_ephemeral:
      Rules are made from:
         -sysevent RawFirewallRule

   prerouting_raw:
   output_raw:
      Rules are made from:
         - syscfg set RawTableFirewallRule

   prerouting_nowan:
   output_nowan:
      Rules are made when current_wan_ipaddr is 0.0.0.0

 mangle
 -----
   prerouting_trigger:
      Rules are made from:
           - syscfg PortRangeTrigger_x

   prerouting_qos:
   postrouting_qos:
     Rules are made from:
      - syscfg QoSPolicy_x
      - syscfg QoSUserDefinedPolicy_x
      - syscfg QoSDefinedPolicy_x
      - syscfg QoSMacAddr_x
      - syscfg QoSVoiceDevice_x
  
   postrouting_lan2lan
      Rules are made from:
         - syscfg block_nat_redirection

 nat
 ---
   prerouting_fromwan:
        - syscfg SinglePortForward_x
        - syscfg PortRangeFoward_x
        - syscfg WellKnownPortForward_x
        - sysevent portmap_dyn_pool

   prerouting_mgmt_override:
        - syscfg mgmt_httpaccess
        - syscfg mgmt_httpsaccess
        - syscfg http_admin_port

   prerouting_plugins:
      Root of subtables used by plugins such as parental control which use a local logic
      to provision its subtables

   prerouting_fromwan_todmz:
        - syscfg dmz_enabled

   prerouting_fromlan:
   postrouting_tolan:
     Rules are made from:
        - syscfg SinglePortForward_x
        - syscfg PortRangeFoward_x
        - syscfg WellKnownPortForward_x
        - sysevent portmap_dyn_pool

   postrouting_towan:
     Rules are made from:
       - syscfg nat_enabled

   prerouting_ephemeral:
   postrouting_ephemeral:
     powerful rules that are run early on the PREROUTING|POSTROUTING chain
     Rules are made from:
        - sysevent NatFirewallRule

   xlog_drop_lanattack:
      attacks from lan

   postrouting_plugins:
      Root of subtables used by plugins such as parental control which use a local logic
      to provision its subtables


 filter
 ------
   The filter table splits traffic into chains based on the incoming interface and the destination.
   Traffic specific chains are:
      lan2wan 
      wan2lan
      lan2self
      wan2self 
   Each chain further classifies traffic, and acts upon the traffic that fits the rule's criterea


   general_input:
   general_output:
   general_forward:
      powerful rules that are run early on the INPUT/OUTPUT/FORWARD chains 
      Rules are made from:
         - syscfg GeneralPurposeFirewallRule_x
         - sysevent GeneralPurposeFirewallRule
         - a DNAT trigger (via GeneralPurposeFirewallRule) 

   lan2wan:
     used to jump to other sub tables that are interested in traffic from lan to wan
      lan2wan_disable:
        Rules ase made from:
            - If nat is disable all lan to wan traffic dorped

      lan2wan_misc:
         Rules are made from:
            - sysevent get current_wan_ipaddr. If the current_wan_ipaddr is 0.0.0.0 then 
              there is no lan to wan traffic allowed
            - sysevent ppp_clamp_mtu

      lan2wan_triggers:
        Rules are made from:
           - syscfg PortRangeTrigger_x

      lan2wan_webfilters:
        Rules are made from:
           - syscfg block_webproxy
           - syscfg block_java
           - syscfg block_activex
           - syscfg block_cookies

      lan2wan_iap :
        Rules are made from:
           - syscfg InternetAccessPolicy_x
           This subtable is used to hold Internet Access Policy subtables
               * namespace_classification
               * namespace_rules

      lan2wan_plugins :
         Root of subtables used by plugins such as parental control which use a local logic
         to provision its subtables

   wan2lan:
     used to jump to other tables that are interested in traffic from wan to lan

      wan2lan_disabled:
         Rules are made from:
            - sysevent get current_wan_ipaddr. If the current_wan_ipaddr is 0.0.0.0 then 
              there is no wan to lan traffic allowed

      wan2lan_forwarding_accept:
        Rules are made from:
           - syscfg SinglePortForward_x
           - syscfg PortRangeFoward_x
           - syscfg WellKnownPortForward_x
           - sysevent portmap_dyn_pool
           - syscfg StaticRoute_x

      wan2lan_misc:
        Rules are made from:
           - syscfg W2LFirewallRule_x
           - syscfg W2LWellKnownFirewallRule_x
           - sysevent ppp_clamp_mtu

      wan2lan_accept:
         Rules are accept multicast

      wan2lan_nonat:
        When nat is disabled then firewall doesn't block forwarding to lan hosts
        Rules are made from:
           - syscfg nat_enabled (if not enabled)

      wan2lan_plugins:
         Root of subtables used by plugins such as parental control which use a local logic
         to provision its subtables

      wan2lan_dmz:
        Rules are made from:
           - syscfg dmz_enabled
     

   lan2self:
     used to jump to other tables that are interested in traffic from lan to utopia
     These tables are:
        lan2self_mgmt
        lan2self_attack
        host_detect

      lan2self_mgmt:
        Rules are made from:
           - syscfg mgmt_wifi_access

      lanattack:
        Rules are made from well known rules to protect from attacks on our trusted interface

      host_detect:
          Rules are made dynamically as lan host are discovered

      lan2self_plugins:
         Root of subtables used by plugins such as parental control which use a local logic
         to provision its subtables

   self2lan:
      used to jump to other tables that are interested in traffic from utopia to the lan
      These tables are:
         self2lan_plugins

      self2lan_plugins:
         Root of subtables used by plugins such as parental control which use a local logic
         to provision its subtables

   wan2self:
     used to jump to other tables that are interested in traffic from wan to utopia
     These tables are:
        wan2self_ports
        wan2self_mgmt
        wan2self_attack

      wan2self_mgmt:
         Rules are made from:
            - syscfg mgmt_wan_access

      wan2self_ports:
         powerful port control for packets from wan to our untrusted interface. They are examined early
         and allow accept/deny priviledges for ports/protocols etc. 
         Rules are made from:
            syscfg rip_enabled
            syscfg firewall_development_override
            syscfg block_ping
            syscfg block_multicast
            syscfg block_ident

      wanattack:
        Rules are made from well known rules to protect from attacks on our untrusted interface

   terminal rules:
      Many rules end with a jump to the appropriate log. In these rules, if logging is turned on, then a 
      log will be emitted. Otherwise no log is emitted, but the packet will be either accepted or dropped.

      xlog_accept_lan2wan:
      xlog_accept_wan2lan:
      xlog_accept_wan2self:
      xlog_drop_wan2lan:
      xlog_drop_lan2wan:
      xlog_drop_wan2self:
      xlog_drop_wanattack:
      xlog_drop_lanattack:
      xlogdrop:
      xlogreject:
        Rules are to log and drop/accept/reject

NOTES:
1) Port Range Triggering requires the userspace process "trigger" to be included in the image


Author: enright@cisco.com

Defines used to control conditional compilation 
-----------------------------------------------
CONFIG_BUILD_TRIGGER:
   Port Range Triggering built in. This requires the userspace process "trigger" to 
   be built into the image

OBSOLETE:
NOT_DEF:
   Not used code, but not yet removed

 ============================================================================
*/
#include "autoconf.h"
//zqiu: Need to enable kernel flag CONFIG_KERNEL_NF_TRIGGER_SUPPORT
#ifdef CONFIG_INTEL_NF_TRIGGER_SUPPORT
#define CONFIG_KERNEL_NF_TRIGGER_SUPPORT CONFIG_INTEL_NF_TRIGGER_SUPPORT
#endif

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <ulog/ulog.h>
#include "syscfg/syscfg.h"
#include "sysevent/sysevent.h"
#define __USE_GNU
#include <string.h>   // strcasestr needs __USE_GNU
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//#include "utapi.h"
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"
#include "firewall_custom.h"

#define CCSP_SUBSYS "eRT."
#define PORTMAPPING_2WAY_PASSTHROUGH
#define MAX_URL_LEN 1024 
#define IF_IPV6ADDR_MAX 16

#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
#define PARCON_WALLED_GARDEN_HTTP_PORT_SITEBLK "18080" // the same as the port in lighttpd.conf
#define PARCON_WALLED_GARDEN_HTTPS_PORT_SITEBLK "10443" // the same as the port in lighttpd.conf
//#define DNS_QUERY_QUEUE_NUM 5

#define DNS_RES_QUEUE_NUM_START 6 //should be the same range as system_defaults-xxx
#define DNS_RES_QUEUE_NUM_END 8

#define DNSV6_RES_QUEUE_NUM_START 9 //should be the same range as system_defaults-xxx
#define DNSV6_RES_QUEUE_NUM_END 10

#define HTTP_GET_QUEUE_NUM_START 11 
#define HTTP_GET_QUEUE_NUM_END 12

#define HTTPV6_GET_QUEUE_NUM_START 13 
#define HTTPV6_GET_QUEUE_NUM_END 14


#if (HTTP_GET_QUEUE_NUM_END == HTTP_GET_QUEUE_NUM_START)
#define __IPT_GET_QUEUE_CONFIG__(x) "--queue-num " #x
#define _IPT_GET_QUEUE_CONFIG_(x) __IPT_GET_QUEUE_CONFIG__(x)
#define HTTP_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(DNS_RES_QUEUE_NUM_START)
#define DNSR_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(HTTP_GET_QUEUE_NUM_START)
#define HTTPV6_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(DNSV6_RES_QUEUE_NUM_START)
#define DNSV6R_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(HTTPV6_GET_QUEUE_NUM_START)
#else
#define __IPT_GET_QUEUE_CONFIG__(s,e) "--queue-balance " #s ":" #e 
#define _IPT_GET_QUEUE_CONFIG_(s,e) __IPT_GET_QUEUE_CONFIG__(s,e)
#define HTTP_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(HTTP_GET_QUEUE_NUM_START, HTTP_GET_QUEUE_NUM_END) 
#define DNSR_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(DNS_RES_QUEUE_NUM_START, DNS_RES_QUEUE_NUM_END) 
#define HTTPV6_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(HTTPV6_GET_QUEUE_NUM_START, HTTPV6_GET_QUEUE_NUM_END) 
#define DNSV6R_GET_QUEUE_CONFIG _IPT_GET_QUEUE_CONFIG_(DNSV6_RES_QUEUE_NUM_START, DNSV6_RES_QUEUE_NUM_END) 
#endif

#endif

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
#define PARCON_ALLOW_LIST "/var/.parcon_allow_list"
#define PARCON_IP_URL "/var/parcon"

#define PARCON_WALLED_GARDEN_HTTP_PORT_SITEBLK "18080" // the same as the port in lighttpd.conf
#define PARCON_WALLED_GARDEN_HTTPS_PORT_SITEBLK "10443" // the same as the port in lighttpd.conf
#define PARCON_WALLED_GARDEN_HTTP_PORT_TIMEBLK "38080" // the same as the port in lighttpd.conf
#define PARCON_WALLED_GARDEN_HTTPS_PORT_TIMEBLK "30443" // the same as the port in lighttpd.conf

#define DNS_QUERY_QUEUE_NUM 5

#define DNS_RES_QUEUE_NUM_START 6 //should be the same range as system_defaults-xxx
#define DNS_RES_QUEUE_NUM_END 8

#define HTTP_GET_QUEUE_NUM_START 11 
#define HTTP_GET_QUEUE_NUM_END 12
#endif
#define FW_DEBUG 1

FILE *firewallfp = NULL;

//#define CONFIG_BUILD_TRIGGER 1
/*
 * Service template declarations & definitions
 */
static char *service_name = "firewall";

const char* const firewall_component_id = "ccsp.firewall";
static void* bus_handle = NULL;
pthread_mutex_t firewall_check;
#define SERVICE_EV_COUNT 4
enum{
    NAT_DISABLE = 0,
    NAT_DHCP,
    NAT_STATICIP,
    NAT_DISABLE_STATICIP,
};

#ifdef CISCO_CONFIG_TRUE_STATIC_IP 
#define MAX_TS_ASN_COUNT 64
typedef struct{
    char ip[20];
    char mask[20];
}staticip_subnet_t;

static char wan_staticip_status[20];       // wan_service-status

static char current_wan_static_ipaddr[20];//ipv4 static ip address 
static char current_wan_static_mask[20];//ipv4 static ip mask 
static char firewall_true_static_ip_enable[20];
static char firewall_true_static_ip_enablev6[20];
static int isWanStaticIPReady;
static int isFWTS_enable = 0;
static int isFWTS_enablev6 = 0;

static int StaticIPSubnetNum = 0;
staticip_subnet_t StaticIPSubnet[MAX_TS_ASN_COUNT]; 

#endif
typedef enum {
    SERVICE_EV_UNKNOWN,
    SERVICE_EV_START,
    SERVICE_EV_STOP,
    SERVICE_EV_RESTART,
    // add custom events here
    SERVICE_EV_SYSLOG_STATUS,
} service_ev_t;

/* iptables module name
 * Note: when get priorty from sysevent failed, it will use the default priority order
 * the default priority is IPT_PRI_XXXXX.
 * 1 is the highest priorty
 */ 
enum{
   IPT_PRI_NONEED = 0,
#ifdef CISCO_CONFIG_TRUE_STATIC_IP   
   IPT_PRI_STATIC_IP,
#endif
   IPT_PRI_PORTMAPPING,
   IPT_PRI_PORTTRIGGERING,
   IPT_PRI_FIREWALL,
   IPT_PRI_DMZ,   
   IPT_PRI_MAX = IPT_PRI_DMZ,
};

/*
 * Service event mapping table
 */
struct {
    service_ev_t ev;
    char         *ev_string;
} service_ev_map[SERVICE_EV_COUNT] = 
    {
        { SERVICE_EV_START,   "firewall-start" },
        { SERVICE_EV_STOP,    "firewall-stop" },
        { SERVICE_EV_RESTART, "firewall-restart" },
        // add entries for custom events here
        { SERVICE_EV_SYSLOG_STATUS, "syslog-status" },
    } ;



static int  lock_fd = -1;

static int            sysevent_fd = -1;
static char          *sysevent_name = "firewall";
static token_t        sysevent_token;
static unsigned short sysevent_port;
static char           sysevent_ip[19];

static char default_wan_ifname[50]; // name of the regular wan interface
static char current_wan_ifname[50]; // name of the ppp interface or the regular wan interface if no ppp
static char ecm_wan_ifname[20];
static char emta_wan_ifname[20];
static char wan6_ifname[20];
static char wan_service_status[20];       // wan_service-status

static int ecm_wan_ipv6_num = 0;
static char ecm_wan_ipv6[IF_IPV6ADDR_MAX][40];

static int current_wan_ipv6_num = 0;
static char current_wan_ipv6[IF_IPV6ADDR_MAX][40];

static char current_wan_ipaddr[20]; // ipv4 address of the wan interface, whether ppp or regular
static char lan_ifname[50];       // name of the lan interface
static char lan_ipaddr[20];       // ipv4 address of the lan interface
static char lan_netmask[20];      // ipv4 netmask of the lan interface
static char lan_3_octets[17];     // first 3 octets of the lan ipv4 address
static char iot_ifName[50];       // IOT interface
static char iot_primaryAddress[50]; //IOT primary IP address
#if defined(_COSA_BCM_MIPS_)
static char lan0_ipaddr[20];       // ipv4 address of the lan0 interface used to access web ui in bridge mode
#endif
static char rip_enabled[20];      // is rip enabled
static char rip_interface_wan[20];  // if rip is enabled, then is it enabled on the wan interface
static char nat_enabled[20];      // is nat enabled
static char dmz_enabled[20];      // is dmz enabled
static char firewall_enabled[20]; // is the firewall enabled
static char bridge_mode[20];      // is system in bridging mode
static char log_level[5];         // if logging is enabled then this is the log level
static int  log_leveli;           // an integer version of the above
static int  syslog_level;         // syslog equivalent of above
static char reserved_mgmt_port[10];  // mgmt port of utopia
static char transparent_cache_state[10]; // state of the transparent http cache
static char byoi_bridge_mode[10]; // whether or not byoi is in bridge mode
static char cmdiag_enabled[20];   // If eCM diagnostic Interface Enabled
static char firewall_level[20];   // None, Low, Medium, High, or Custom
static char firewall_levelv6[20];   // None, High, or Custom
static char cmdiag_ifname[20];       // name of the lan interface
static int isNatReady;
static char natip4[20];
static char captivePortalEnabled[50]; //to ccheck captive portal is enabled or not
static char redirectionFlag[50]; //Captive portal mode flag

static char iptables_pri_level[IPT_PRI_MAX];
//static int  portmapping_pri;        
//static int  porttriggering_pri;
//static int  firewall_pri;
//static int  dmz_pri;

static int isFirewallEnabled;
static int isHairpin;
static int isWanReady;

static int isWanServiceReady;
static int isBridgeMode;
static int isRFC1918Blocked;
static int isRipEnabled;
static int isRipWanEnabled;
static int isNatEnabled;

static int isDmzEnabled;
static int isLogEnabled;
static int isLogSecurityEnabled;
static int isLogIncomingEnabled;
static int isLogOutgoingEnabled;
static int isCronRestartNeeded;
static int isPingBlocked;
static int isPingBlockedV6;
static int isIdentBlocked;
static int isIdentBlockedV6;
static int isMulticastBlocked;
static int isMulticastBlockedV6;
static int isNatRedirectionBlocked;
static int isPortscanDetectionEnabled;
static int isDevelopmentOverride;
static int isNtpFinished                 = 0;
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
static int isTriggerMonitorRestartNeeded = 0;
#endif
static int isLanHostTracking             = 0;
static int isDMZbyMAC                    = 0;   // DMZ is known by MAC address
static int isRawTableUsed                = 0; 
static int isCacheActive                 = 0;
static int isCmDiagEnabled;
static int isHttpBlocked;                       // Block incoming HTTP/HTTPS traffic
static int isHttpBlockedV6;
static int isP2pBlocked;                        // Block incoming P2P traffic
static int isP2pBlockedV6;

static int flush = 0;

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
static int isGuestNetworkEnabled;
static char guest_network_ipaddr[20];
static char guest_network_mask[20];
#endif

static int ppFlushNeeded = 0;

/*
 * For timed internet access rules we use cron 
 */
#define crontab_dir  "/var/spool/cron/crontabs/"
#define crontab_filename  "firewall"
#define cron_everyminute_dir "/etc/cron/cron.everyminute"
/*
 * For tracking lan hosts
 */
#define lan_hosts_dir "/tmp/lanhosts"
#define hosts_filename "lanhosts"
/*
 * various files that we use to make well known name to rule mappings
 * This allows User Interface and Firewall to refer to the rules by name.
 */
#define qos_classification_file_dir "/etc/"
#define qos_classification_file "qos_classification_rules"
#define wellknown_ports_file_dir "/etc/"
#define wellknown_ports_file "services"
#define otherservices_dir "/etc/"
#define otherservices_file "otherservices"

/*
 * triggers use this well known namespace within iptables LOGs.
 * keep this in sync with trigger_monitor.sh 
 */
#define LOG_TRIGGER_PREFIX "UTOPIA.TRIGGER"

/*
 * For simplicity purposes we cap the number of syscfg entries within a
 * specific namespace. This cap is controlled by MAX_SYSCFG_ENTRIES
 */
#define MAX_SYSCFG_ENTRIES 100

#define MAX_QUERY 256
#define MAX_NAMESPACE 64

/*
 * For URL blocking,
 * The string lengths of "http://" and "https://"
 */
#define STRLEN_HTTP_URL_PREFIX  (7)
#define STRLEN_HTTPS_URL_PREFIX (8)

/*
 * local date and time
 */
static struct tm local_now;

/*
 * iptables priority level 
 */

inline void SET_IPT_PRI_DEFAULT(void){
  iptables_pri_level[IPT_PRI_PORTMAPPING -1 ]= IPT_PRI_PORTMAPPING; 
  iptables_pri_level[IPT_PRI_PORTTRIGGERING -1]= IPT_PRI_PORTTRIGGERING; 
  iptables_pri_level[IPT_PRI_DMZ-1]= IPT_PRI_DMZ; 
  iptables_pri_level[IPT_PRI_FIREWALL -1]= IPT_PRI_FIREWALL; 
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
  iptables_pri_level[IPT_PRI_STATIC_IP -1]= IPT_PRI_STATIC_IP; 
#endif
}


inline int SET_IPT_PRI_MODULD(char *s){
    if(strcmp(s, "portmapping") == 0)
       return IPT_PRI_PORTMAPPING;
    else if(strcmp(s, "porttriggering") == 0)
       return IPT_PRI_PORTTRIGGERING;
    else if(strcmp(s, "dmz") == 0)
       return IPT_PRI_DMZ;
    else if(strcmp(s, "firewall") == 0)
       return IPT_PRI_FIREWALL;
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
    else if(strcmp(s, "staticip") == 0)
       return IPT_PRI_STATIC_IP;
#endif
   else
       return 0; 
} 

/* 
 * Get PSM value 
 */
 #ifdef CISCO_CONFIG_TRUE_STATIC_IP
#define PSM_NAME_TRUE_STATIC_IP_ADDRESS "dmsb.truestaticip.Ipaddress"
#define PSM_NAME_TRUE_STATIC_IP_NETMASK "dmsb.truestaticip.Subnetmask"
#define PSM_NAME_TRUE_STATIC_IP_ENABLE  "dmsb.truestaticip.Enable"
#define PSM_NAME_TRUE_STATIC_ASN        "dmsb.truestaticip.Asn."
#define PSM_NAME_TRUE_STATIC_ASN_IP     "Ipaddress"
#define PSM_NAME_TRUE_STATIC_ASN_MASK   "Subnetmask"
#define PSM_NAME_TRUE_STATIC_ASN_ENABLE "Enable"
#endif

#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(bus_handle, CCSP_SUBSYS, name, NULL, &(str)) 
#define PSM_VALUE_GET_INS(name, pIns, ppInsArry) PsmGetNextLevelInstances(bus_handle, CCSP_SUBSYS, name, pIns, ppInsArry)

/* 
 */
#define REMOTE_ACCESS_IP_RANGE_MAX_RULE 20

/* DSCP val for gre*/
#define PSM_NAME_GRE_DSCP_VALUE "dmsb.hotspot.tunnel.1.DSCPMarkPolicy"
int greDscp = 44; // Default initialized to 44

/* Configure WiFi flag for captive Portal*/
#define PSM_NAME_CP_NOTIFY_VALUE "eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges"
/*
 =================================================================
                     utilities
 =================================================================
 */
/*
 * Procedure     : trim
 * Purpose       : trims a string
 * Parameters    :
 *    in         : A string to trim
 * Return Value  : The trimmed string
 * Note          : This procedure will change the input sting in situ
 */
static char *trim(char *in)
{
  //               FIREWALL_DEBUG("Entering *trim\n");         
   // trim the front of the string
   if (NULL == in) {
      return(NULL);
   }
   char *start = in;
   while(isspace(*start)) {
      start++;
   }
   // trim the end of the string

   char *end = start+strlen(start);
   end--;
   while(isspace(*end)) {
      *end = '\0';
      end--;
   }
    //             FIREWALL_DEBUG("Exiting *trim\n");         
   return(start);
}


/*
 * Procedure     : token_get
 * Purpose       : given the start of a string 
 *                 containing a delimiter,
 *                 return the end of the string
 * Parameters    :
 *    in         : The start of the string containing a token
 *    delim      : A character used as delimiter
 *
 * Return Value  : The start of the next possible token
 *                 NULL if end
 * Note          : This procedure will change the input sting in situ by breaking
 *                 it up into substrings at the delimiter.
 */
static char *token_get(char *in, char delim)
{
      //           FIREWALL_DEBUG("Entering *token_get\n");         
   char *end = strchr(in, delim);
   if (NULL != end) {
      *end = '\0';
      end++;
   }
        //         FIREWALL_DEBUG("Exiting *token_get\n");         
   return(end);   
}

/*
 *  Procedure     : time_delta
 *  Purpose       : how much time between two times in different formats
 *  Parameters    :
 *    time1          : time in struct tm format
 *    time2          : time in hh:mm format (24 hour time)
 *    hours          : hours between time1 and time2
 *    mins           : mins between time1 and time2
 *  Return Value:
 *     0       : if time2 was greater than time1
 *    -1       : if time 1 was greater than time2
 *  Note        :
 *   If return value is -1 then hours and mins will be 0
 */
static int time_delta(struct tm *time1, const char *time2, int *hours, int *mins)
{
   int t2h;
   int t2m;
   sscanf(time2,"%d:%d", &t2h, &t2m);
  // FIREWALL_DEBUG("Entering time_delta\n");         
  if (time1->tm_hour > t2h) {
      *hours = *mins = 0;
      return(-1);
   } else if (time1->tm_hour == t2h && time1->tm_min >= t2m) { 
      *hours = *mins = 0;
      return(-1);
   } else {
      *hours = t2h-time1->tm_hour;
      if (time1->tm_min <= t2m) {
         *mins = t2m -time1->tm_min;
      } else {
         *hours -= 1;
         *mins = (60 - (time1->tm_min-t2m));
      }
      return(0);
   }
  // FIREWALL_DEBUG("Exiting time_delta\n");         
}
 
/*
 *  Procedure     : substitute
 *  Purpose       : Change all occurances of a token to another token 
 *  Paramenters   :
 *    in_str         : A string that might contain the from token
 *    out_str        : Memory to place the string with from string replaced by to token
 *    size           : The size of out_str
 *    from           : The token to look for
 *    to             : The token to replace to
 * Return Values  :
 *    The number of substitutions
 *    -1   : ERROR
 */
static int substitute(char *in_str, char *out_str, const int size, char *from, char *to)
{
    char *in_str_p    = in_str;
    char *out_str_p   = out_str;
    char *in_str_end  = in_str + strlen(in_str);
    char *out_str_end = out_str + size;
    int   num_subst   = 0;
   // FIREWALL_DEBUG("Entering substitute\n");         
    while (in_str_p < in_str_end && out_str_p < out_str_end) {
       char *from_p;
       from_p = strstr(in_str_p, from);
       if (NULL != from_p) {
          /*
           * we found an instance of the token to replace.
           * First copy the bytes upto the token
           */
          int num_bytes = from_p-in_str_p;
          if (out_str_p + num_bytes < out_str_end) {
             memmove(out_str_p, in_str_p, num_bytes);
             out_str_p += num_bytes;
          } else {
            out_str[size-1] = '\0';
            return(-1);
          }
          /*
           * now substitute the token
           */
          if (out_str_p + strlen(to) < out_str_end) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", to);
             in_str_p = from_p + strlen(from);
             num_subst++;
          } else {
             out_str[size-1] = '\0';
             return(-1);
          }
       } else {
          /*
           * no more instances of the token are found
           * so copy in the rest of the input string and return
           */
          int num_bytes = strlen(in_str_p) + 1;
          if (out_str_p + num_bytes < out_str_end) {
             memmove(out_str_p, in_str_p, num_bytes);
             out_str_p += num_bytes;
             out_str[size-1] = '\0';
             return(num_subst);
          } else {
            out_str[size-1] = '\0';
            return(-1);
          }
       }
    }

    out_str[size-1] = '\0';
   // FIREWALL_DEBUG("Exiting substitute\n");         
    return(num_subst);
}

/*
 *  Procedure     : make_substitutions
 *  Purpose       : Change any well-known symbols in a string to the running/configured values
 *  Paramenters   :
 *    in_str         : A string that might contain well-known symbols
 *    out_str        : Memory to place the string with symbols converted to values
 *    size           : The size of out_str
 * Return Values  :
 *    A pointer to out_str on success
 *    Otherwise NULL
 * Notes:
 *   Currently we handle $WAN_IPADDR, $WAN_IFNAME, $LAN_IFNAME, $LAN_IPADDR, $LAN_NETMASK
 *                       $ACCEPT $DROP $REJECT and 
 *   QoS classes $HIGH, $MEDIUM, $NORMAL, $LOW
 */
static char *make_substitutions(char *in_str, char *out_str, const int size)
{
    char *in_str_p = in_str;
    char *out_str_p = out_str;
    char *in_str_end = in_str + strlen(in_str);
    char *out_str_end = out_str + size;
   // FIREWALL_DEBUG("Entering *make_substitutions\n");         
    while (in_str_p < in_str_end && out_str_p < out_str_end) {
       char token[50];
       if ('$' == *in_str_p) {
          sscanf(in_str_p, "%50s", token); 
          in_str_p += strlen(token);
          if (0 == strcmp(token, "$WAN_IPADDR")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", current_wan_ipaddr);
          } else if (0 == strcmp(token, "$WAN_IFNAME")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", current_wan_ifname);
          } else if (0 == strcmp(token, "$LAN_IFNAME")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", lan_ifname);
          } else if (0 == strcmp(token, "$LAN_IPADDR")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", lan_ipaddr);
          } else if (0 == strcmp(token, "$LAN_NETMASK")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", lan_netmask);
          } else if (0 == strcmp(token, "$ACCEPT")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", "ACCEPT");
          } else if (0 == strcmp(token, "$DROP")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", "DROP");
          } else if (0 == strcmp(token, "$REJECT")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", "REJECT --reject-with tcp-reset");
         } else if (0 == strcmp(token, "$HIGH")) {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", "EF");
         } else  if (0 == strcmp(token, "$MEDIUM")) { 
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", "AF11");
         } else  if (0 == strcmp(token, "$NORMAL")) { 
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", "AF22");
         } else  if (0 == strcmp(token, "$LOW")) { 
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", "BE");
         } else {
             out_str_p += snprintf(out_str_p, out_str_end-out_str_p, "%s", token);
         }
       } else {
          *out_str_p = *in_str_p;
          out_str_p++;
          in_str_p++;
       }
       if ('\0' == *in_str_p) {
          *out_str_p = *in_str_p;
          break;
       } 
   }

    out_str[size-1] = '\0';
   // FIREWALL_DEBUG("Exiting *make_substitutions\n");         
    return(out_str);
}
 
/*
 *  Procedure     : match_keyword
 *  Purpose       : given an open file with format 
 *                  keyword [delim] .......
 *                  return the first line matching that keyword
 *  Parameters    :
 *     fp              : An open file to search
 *     keyword         : The keyword to search for in the first field
 *     delim           : The delimiter 
 *     line            : A string in which to place the found line
 *     size            : the size of line
 * Return Values  : 
 *    A pointer to the start of the rest of the line after the delimiter
 *    NULL if keyword is not found
 * 
 */
static char *match_keyword(FILE *fp, char *keyword, char delim, char *line, int size)
{
  // FIREWALL_DEBUG("Entering *match_keyword\n");         
   while (NULL != fgets(line, size, fp) ) {
      char *keyword_candidate = NULL;
      char *next;
      /*
       * handle space differently
       */
      if (' ' == delim) {
         char local_name[50];
         local_name[0] = '\0';
         sscanf(line, "%50s ", local_name); 
         next = line + strlen(local_name);
         if (next-line > size) {
              continue;
         } else {
            *next = '\0';
            next++;
            keyword_candidate = trim(line);
         }
      } else {
         keyword_candidate = line;
         next = token_get(keyword_candidate, delim);
         if (NULL != keyword_candidate) {
            keyword_candidate = trim(keyword_candidate);
         } else {
            continue;
         }
      }

      if (0 == strcasecmp(keyword, keyword_candidate)) {
         return(next);
      } 
   }
  // FIREWALL_DEBUG("Exiting *match_keyword\n");         
   return(NULL);
}


/*
 *  Procedure     : to_syslog_level
 *  Purpose       : convert syscfg log_level to syslog level
 */
static int to_syslog_level (int log_leveli)
{
   switch (log_leveli) {
   case 0:
      return LOG_WARNING;
   case 2:
      return LOG_INFO;
   case 3:
      return LOG_DEBUG;
   case 1:
   default:
      return LOG_NOTICE;
   }
}

#define IPV6_ADDR_SCOPE_MASK    0x00f0U
#define _PROCNET_IFINET6  "/proc/net/if_inet6"
int get_ip6address (char * ifname, char ipArry[][40], int * p_num)
{
    FILE * fp = NULL;
	char addr6p[8][5];
	int plen, scope, dad_status, if_idx;    
	char addr6[40], devname[20];
	struct sockaddr_in6 sap;
    int    i = 0;
   // FIREWALL_DEBUG("Entering get_ip6address\n");         
    if (!ifname && !ipArry && !p_num)
        return -1;
    fp = fopen(_PROCNET_IFINET6, "r");
    if (!fp)
        return -1;
    
	while (fscanf
		   (fp, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
			addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
			addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
			&dad_status, devname) != EOF
          )
    {
        if (!strcmp(ifname, devname))
        {
            sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
            
            /* Global address */ 
            if(0 == (scope & IPV6_ADDR_SCOPE_MASK)){
                strncpy(ipArry[i], addr6, 40);
                i++;
                if(i == IF_IPV6ADDR_MAX)
                    break;
			}
        }
    }
    *p_num = i;

    fclose(fp);
   // FIREWALL_DEBUG("Exiting get_ip6address\n");         
    return 0;
}

/*
 *  Procedure     : prepare_globals_from_configuration
 *  Purpose       : use syscfg and sysevent to prepare information such as
 *                  wan interface name, wan ip address, etc
 */
static int prepare_globals_from_configuration(void)
{
   int rc;
   char tmp[100];
   char *pStr = NULL;
   int i;
   FIREWALL_DEBUG("Entering prepare_globals_from_configuration\n");       
   tmp[0] = '\0';
   // use wan protocol determined wan interface name if possible, else use default (the name used by the OS)
   default_wan_ifname[0] = '\0';
   current_wan_ifname[0] = '\0';
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
   current_wan_static_ipaddr[0] = '\0';
   current_wan_static_mask[0] = '\0';
   wan_staticip_status[0] = '\0';
#endif
   lan_ipaddr[0] = '\0';
   transparent_cache_state[0] = '\0';
   wan_service_status[0] = '\0';
#if defined(_COSA_BCM_MIPS_)
   lan0_ipaddr[0] = '\0';
   sysevent_get(sysevent_fd, sysevent_token, "lan0_ipaddr", lan0_ipaddr, sizeof(lan0_ipaddr));
#endif
    
   sysevent_get(sysevent_fd, sysevent_token, "wan_ifname", default_wan_ifname, sizeof(default_wan_ifname));
   sysevent_get(sysevent_fd, sysevent_token, "current_wan_ifname", current_wan_ifname, sizeof(current_wan_ifname));
   if ('\0' == current_wan_ifname[0]) {
      snprintf(current_wan_ifname, sizeof(current_wan_ifname), default_wan_ifname);
   }
   sysevent_get(sysevent_fd, sysevent_token, "current_wan_ipaddr", current_wan_ipaddr, sizeof(current_wan_ipaddr));
   sysevent_get(sysevent_fd, sysevent_token, "current_lan_ipaddr", lan_ipaddr, sizeof(lan_ipaddr));
   
#if defined(CONFIG_CISCO_FEATURE_CISCOCONNECT) || defined(CONFIG_CISCO_PARCON_WALLED_GARDEN) 
   FILE *gwIpFp = fopen("/var/.gwip", "w");
   fprintf(gwIpFp, "%s", lan_ipaddr);
   fclose(gwIpFp);
#endif

   sysevent_get(sysevent_fd, sysevent_token, "transparent_cache_state", transparent_cache_state, sizeof(transparent_cache_state));
   sysevent_get(sysevent_fd, sysevent_token, "byoi_bridge_mode", byoi_bridge_mode, sizeof(byoi_bridge_mode));
   sysevent_get(sysevent_fd, sysevent_token, "wan_service-status", wan_service_status, sizeof(wan_service_status));
   isWanServiceReady = (0 == strcmp("started", wan_service_status)) ? 1 : 0;

   char pri_level_name[20];
   memset(iptables_pri_level, 0, sizeof(iptables_pri_level));
   for(i = 0; i < IPT_PRI_MAX; i++)
   {
      sprintf(pri_level_name, "ipt_pri_level_%d", i+1);
      tmp[0] = '\0';
      rc =  sysevent_get(sysevent_fd, sysevent_token, pri_level_name, tmp, sizeof(tmp));
      if(rc != 0 || tmp[0] == '\0')
      {
          SET_IPT_PRI_DEFAULT();
          break;
      }
      iptables_pri_level[i] = SET_IPT_PRI_MODULD(tmp);
      if(iptables_pri_level[i] == 0){
          SET_IPT_PRI_DEFAULT();
          break;
      }
   }

   memset(lan_ifname, 0, sizeof(lan_ifname));
   memset(cmdiag_ifname, 0, sizeof(cmdiag_ifname));
//   memset(lan_ipaddr, 0, sizeof(lan_ipaddr));
   memset(lan_netmask, 0, sizeof(lan_netmask));
   memset(lan_3_octets, 0, sizeof(lan_3_octets));
   memset(rip_enabled, 0, sizeof(rip_enabled));
   memset(rip_interface_wan, 0, sizeof(rip_interface_wan));
   memset(nat_enabled, 0, sizeof(nat_enabled));
   memset(dmz_enabled, 0, sizeof(dmz_enabled));
   memset(firewall_enabled, 0, sizeof(firewall_enabled));
   memset(bridge_mode, 0, sizeof(bridge_mode));
   memset(log_level, 0, sizeof(log_level));


   syscfg_get(NULL, "lan_ifname", lan_ifname, sizeof(lan_ifname));
   //syscfg_get(NULL, "lan_ipaddr", lan_ipaddr, sizeof(lan_ipaddr));
   syscfg_get(NULL, "lan_netmask", lan_netmask, sizeof(lan_netmask)); 
   syscfg_get(NULL, "rip_enabled", rip_enabled, sizeof(rip_enabled)); 
   syscfg_get(NULL, "rip_interface_wan", rip_interface_wan, sizeof(rip_interface_wan)); 
   syscfg_get(NULL, "nat_enabled", nat_enabled, sizeof(nat_enabled)); 
   syscfg_get(NULL, "dmz_enabled", dmz_enabled, sizeof(dmz_enabled)); 
   syscfg_get(NULL, "firewall_enabled", firewall_enabled, sizeof(firewall_enabled)); 
   //mipieper - change for pseudo bridge
   //syscfg_get(NULL, "bridge_mode", bridge_mode, sizeof(bridge_mode)); 
   sysevent_get(sysevent_fd, sysevent_token, "bridge_mode", bridge_mode, sizeof(bridge_mode));
   syscfg_get(NULL, "log_level", log_level, sizeof(log_level)); 
   if ('\0' == log_level[0]) {
      sprintf(log_level, "1");
   } 
   log_leveli = atoi(log_level);
   syslog_level = to_syslog_level(log_leveli);

   // the first 3 octets of the lan ip address
   snprintf(lan_3_octets, sizeof(lan_3_octets), "%s", lan_ipaddr);
   char *p;
   for (p=lan_3_octets+strlen(lan_3_octets); p >= lan_3_octets; p--) {
      if (*p == '.') {
         *p = '\0';
         break;
      } else {
         *p = '\0';
      }
   }

   syscfg_get(NULL, "cmdiag_ifname", cmdiag_ifname, sizeof(cmdiag_ifname));
   syscfg_get(NULL, "cmdiag_enabled", cmdiag_enabled, sizeof(cmdiag_enabled));

   syscfg_get(NULL, "firewall_level", firewall_level, sizeof(firewall_level));
   syscfg_get(NULL, "firewall_levelv6", firewall_levelv6, sizeof(firewall_levelv6));

   syscfg_get(NULL, "ecm_wan_ifname", ecm_wan_ifname, sizeof(ecm_wan_ifname));
   syscfg_get(NULL, "emta_wan_ifname", emta_wan_ifname, sizeof(emta_wan_ifname));
   get_ip6address(ecm_wan_ifname, ecm_wan_ipv6, &ecm_wan_ipv6_num);
   //get_ip6address(current_wan_ifname, current_wan_ipv6, &current_wan_ipv6_num);

   isCacheActive     = (0 == strcmp("started", transparent_cache_state)) ? 1 : 0;
   isFirewallEnabled = (0 == strcmp("0", firewall_enabled)) ? 0 : 1;  
   isWanReady        = (0 == strcmp("0.0.0.0", current_wan_ipaddr)) ? 0 : 1;
   //isBridgeMode        = (0 == strcmp("1", bridge_mode)) ? 1 : (0 == strcmp("1", byoi_bridge_mode)) ? 1 : 0;
   isBridgeMode        = (0 == strcmp("0", bridge_mode)) ? 0 : 1;
   isRipEnabled      = (0 == (strcmp("1", rip_enabled))) ? 1 : 0;
   isRipWanEnabled   = ((isRipEnabled) && (0 == (strcmp("1", rip_interface_wan)))) ? 1 : 0;
   isDmzEnabled      = (0 == strcmp("1", dmz_enabled)) ? 1 : 0;
   /* nat_enabled(0): disable  (1) DHCP (2)StaticIP (others) disable */
   isNatEnabled      = atoi(nat_enabled);
   #ifdef CISCO_CONFIG_TRUE_STATIC_IP
   isNatEnabled      = (isNatEnabled > NAT_STATICIP ? NAT_DISABLE : isNatEnabled);
   #else
   isNatEnabled      = (isNatEnabled = NAT_DISABLE ? NAT_DISABLE : NAT_DHCP);
   #endif
   isLogEnabled      = (log_leveli > 1) ? 1 : 0;
   isLogSecurityEnabled = (isLogEnabled && log_leveli > 1) ? 1 : 0;
#if 0
   isLogIncomingEnabled = (isLogEnabled && log_leveli > 1) ? 1 : 0;
   isLogOutgoingEnabled = (isLogEnabled && log_leveli > 1) ? 1 : 0;
#endif
   //todo add syscfg to config log flag 
   isLogIncomingEnabled = 0;
   isLogOutgoingEnabled = 0;
   isCmDiagEnabled   = (0 == strcmp("1", cmdiag_enabled)) ? 1 : 0;

#ifdef CISCO_CONFIG_TRUE_STATIC_IP
   /* get true static IP info */
   sysevent_get(sysevent_fd, sysevent_token, "wan_staticip-status", wan_staticip_status, sizeof(wan_staticip_status));
   isWanStaticIPReady = (0 == strcmp("started", wan_staticip_status)) ? 1 : 0; 
   /* Get Ture Static IP Enable/Disable */
   if(bus_handle != NULL && isWanStaticIPReady){
       isWanStaticIPReady = 0;
       rc = PSM_VALUE_GET_STRING(PSM_NAME_TRUE_STATIC_IP_ENABLE, pStr);
       if(rc == CCSP_SUCCESS && pStr != NULL){
          if(strcmp("1", pStr) == 0){
              isWanStaticIPReady = 1;
          }
          Ansc_FreeMemory_Callback(pStr);
       }   
   }
   /* get value from PSM */
   if(bus_handle != NULL && isWanStaticIPReady)
   {
      /* Get True static Ip information */ 
      rc = PSM_VALUE_GET_STRING(PSM_NAME_TRUE_STATIC_IP_ADDRESS,pStr);
      if(rc == CCSP_SUCCESS && pStr != NULL){
         strcpy(current_wan_static_ipaddr, pStr);
         Ansc_FreeMemory_Callback(pStr);
         pStr = NULL;
      }
      
      rc = PSM_VALUE_GET_STRING(PSM_NAME_TRUE_STATIC_IP_NETMASK ,pStr);
      if(rc == CCSP_SUCCESS && pStr != NULL){
         strcpy(current_wan_static_mask, pStr);
         Ansc_FreeMemory_Callback(pStr);
         pStr = NULL;
      }
      
      strncpy(StaticIPSubnet[0].ip, current_wan_static_ipaddr, sizeof(StaticIPSubnet[0].ip));
      strncpy(StaticIPSubnet[0].mask, current_wan_static_mask, sizeof(StaticIPSubnet[0].mask));
      StaticIPSubnetNum = 1;

      unsigned int ts_asn_count = 0;
      unsigned int *ts_asn_ins = NULL;
      rc = PSM_VALUE_GET_INS(PSM_NAME_TRUE_STATIC_ASN, &ts_asn_count, &ts_asn_ins);
      if(rc == CCSP_SUCCESS && ts_asn_count != 0){
          if(MAX_TS_ASN_COUNT -1  < ts_asn_count){
             fprintf(stderr, "[Firewall] ERROR too many Ture static subnet\n");
             ts_asn_count = MAX_TS_ASN_COUNT -1;
          }
          for(i = 0; i < (int)ts_asn_count ; i++){
             sprintf(tmp,"%s%d.%s", PSM_NAME_TRUE_STATIC_ASN, ts_asn_ins[i], PSM_NAME_TRUE_STATIC_ASN_ENABLE);
             rc = PSM_VALUE_GET_STRING(tmp, pStr) - CCSP_SUCCESS;
             if(rc == 0 && pStr != NULL){
                if(atoi(pStr) != 1){
                    Ansc_FreeMemory_Callback(pStr);
                    pStr = NULL;
                    continue;
                }
                Ansc_FreeMemory_Callback(pStr);
                pStr = NULL;
             }
 
             sprintf(tmp,"%s%d.%s", PSM_NAME_TRUE_STATIC_ASN, ts_asn_ins[i], PSM_NAME_TRUE_STATIC_ASN_IP);
             rc |= PSM_VALUE_GET_STRING(tmp, pStr) - CCSP_SUCCESS;
             if(rc == 0 && pStr != NULL){
                strncpy(StaticIPSubnet[StaticIPSubnetNum].ip, pStr, sizeof(StaticIPSubnet[StaticIPSubnetNum].ip));
                Ansc_FreeMemory_Callback(pStr);
                pStr = NULL;
             }
 
             sprintf(tmp,"%s%d.%s", PSM_NAME_TRUE_STATIC_ASN, ts_asn_ins[i], PSM_NAME_TRUE_STATIC_ASN_MASK);
             rc |= PSM_VALUE_GET_STRING(tmp, pStr) - CCSP_SUCCESS;
             if(rc == 0 && pStr != NULL){
                strncpy(StaticIPSubnet[StaticIPSubnetNum].mask, pStr, sizeof(StaticIPSubnet[StaticIPSubnetNum].mask));
                Ansc_FreeMemory_Callback(pStr);
                pStr = NULL;
             }

             if(rc == 0){
                printf("%d : ip %s mask %s \n", StaticIPSubnetNum, StaticIPSubnet[StaticIPSubnetNum].ip, StaticIPSubnet[StaticIPSubnetNum].mask);
                StaticIPSubnetNum++;
             }  
          }
          Ansc_FreeMemory_Callback(ts_asn_ins);
      }else{
        printf("%d GET INSTERR\n", __LINE__);
      }
   }

   if(isWanReady && isNatEnabled == 1){
       isNatReady = 1;
       strcpy(natip4, current_wan_ipaddr);
   }else if(isWanReady && isNatEnabled == 2 && isWanStaticIPReady ){
       isNatReady = 1;
       strcpy(natip4, current_wan_static_ipaddr); 
   }else 
       isNatReady = 0;

   memset(firewall_true_static_ip_enable, 0, sizeof(firewall_true_static_ip_enable));
   syscfg_get(NULL, "firewall_true_static_ip_enable", firewall_true_static_ip_enable,sizeof(firewall_true_static_ip_enable));
   isFWTS_enable = (0 == strcmp("1", firewall_true_static_ip_enable) ? 1 : 0);
	   
#else
    strcpy(natip4, current_wan_ipaddr);
    isNatReady = isWanReady; 
#endif
   char wan_proto[20];
   wan_proto[0] = '\0';

   char temp[20];

   temp[0] = '\0';
   sysevent_get(sysevent_fd, sysevent_token, "pp_flush", temp, sizeof(temp));
   if ('\0' == temp[0]) {
       ppFlushNeeded = 0;
   } else if (0 == strcmp("0", temp)) {
       ppFlushNeeded  = 0;
   } else {
       ppFlushNeeded  = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_ping", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isPingBlocked = 1;
   } else if (0 == strcmp("0", temp)) {
      isPingBlocked = 0;
   } else {
     isPingBlocked = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_pingv6", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isPingBlockedV6 = 1;
   } else if (0 == strcmp("0", temp)) {
      isPingBlockedV6 = 0;
   } else {
     isPingBlockedV6 = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_multicast", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isMulticastBlocked = 0;
   } else if (0 == strcmp("0", temp)) {
      isMulticastBlocked = 0;
   } else {
     isMulticastBlocked = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_multicastv6", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isMulticastBlockedV6 = 0;
   } else if (0 == strcmp("0", temp)) {
      isMulticastBlockedV6 = 0;
   } else {
     isMulticastBlockedV6 = 1;
   }

   temp[0] = '\0';
   isNatRedirectionBlocked = 0;
   rc = syscfg_get(NULL, "block_nat_redirection", temp, sizeof(temp));
   if (0 == rc && '\0' != temp[0]) {
      if (0 == strcmp("1", temp)){
         isNatRedirectionBlocked = 1;
      }
   }

   temp[0] = '\0';
   isHairpin = 0;
   rc = syscfg_get(NULL, "nat_hairping_enable", temp, sizeof(temp));
   if(0 == rc || '\0' != temp[0]){
       if(!strcmp("1", temp))
          isHairpin = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_ident", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isIdentBlocked = 1;
   } else if (0 == strcmp("0", temp)) {
      isIdentBlocked = 0;
   } else {
     isIdentBlocked = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_identv6", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isIdentBlockedV6 = 1;
   } else if (0 == strcmp("0", temp)) {
      isIdentBlockedV6 = 0;
   } else {
     isIdentBlockedV6 = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_rfc1918", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isRFC1918Blocked = 0;
   } else if (0 == strcmp("0", temp)) {
      isRFC1918Blocked = 0;
   } else {
     isRFC1918Blocked = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_http", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isHttpBlocked = 0;
   } else if (0 == strcmp("0", temp)) {
      isHttpBlocked = 0;
   } else {
     isHttpBlocked = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_httpv6", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isHttpBlockedV6 = 0;
   } else if (0 == strcmp("0", temp)) {
      isHttpBlockedV6 = 0;
   } else {
     isHttpBlockedV6 = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_p2p", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isP2pBlocked = 0;
   } else if (0 == strcmp("0", temp)) {
      isP2pBlocked = 0;
   } else {
     isP2pBlocked = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "block_p2pv6", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isP2pBlockedV6 = 0;
   } else if (0 == strcmp("0", temp)) {
      isP2pBlockedV6 = 0;
   } else {
     isP2pBlockedV6 = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "firewall_development_override", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isDevelopmentOverride = 0;
   } else if (0 == strcmp("0", temp)) {
      isDevelopmentOverride = 0;
   } else {
     isDevelopmentOverride = 1;
   }

   temp[0] = '\0';
   rc = syscfg_get(NULL, "portscan_enabled", temp, sizeof(temp));
   if (0 != rc || '\0' == temp[0]) {
      isPortscanDetectionEnabled = 1;
   } else if (0 == strcmp("0", temp)) {
      isPortscanDetectionEnabled = 0;
   } else {
     isPortscanDetectionEnabled = 1;
   }

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   temp[0] = '\0';
   sysevent_get(sysevent_fd, sysevent_token, "ciscoconnect_guest_enable", temp, sizeof(temp));
   if ('\0' == temp[0]) {
      isGuestNetworkEnabled = 0;
   } else if (0 == strcmp("0", temp)) {
       isGuestNetworkEnabled  = 0;
   } else {
       isGuestNetworkEnabled  = 1;
   }

   char guest_network_ins[8];
   char guestnet_ip_name[32];
   char guestnet_mask_name[32];

   sysevent_get(sysevent_fd, sysevent_token, "ciscoconnect_guest_l3net", guest_network_ins, sizeof(guest_network_ins));

   snprintf(guestnet_ip_name, sizeof(guestnet_ip_name), "ipv4_%s-ipv4addr", guest_network_ins);
   sysevent_get(sysevent_fd, sysevent_token, guestnet_ip_name, guest_network_ipaddr, sizeof(guest_network_ipaddr));

   FILE *f = fopen("/var/.guestnetip", "w");
   fprintf(f, "%s", guest_network_ipaddr);
   fclose(f);

   snprintf(guestnet_mask_name, sizeof(guestnet_mask_name), "ipv4_%s-ipv4subnet", guest_network_ins);
   sysevent_get(sysevent_fd, sysevent_token, guestnet_mask_name, guest_network_mask, sizeof(guest_network_mask));
#endif

   /*
    * for development we all ping and rfc 1918 addresses on wan
    */
   if (isDevelopmentOverride) {
      isRFC1918Blocked = 0;
      isPingBlocked = 0;
   }

   /*
    * Is the system clock trustable
    */
   temp[0] = '\0';
   sysevent_get(sysevent_fd, sysevent_token, "ntpclient-status", temp, sizeof(temp));
   if ( '\0' != temp[0] && 0 == strcmp("started", temp)) {      
      isNtpFinished=1;
   }

   if (isFirewallEnabled) {
      /*
       * Are we tracking bandwidth usage on a per host basis
       */
      temp[0] = '\0';
      rc = syscfg_get(NULL, "lanhost_tracking_enabled", temp, sizeof(temp));
      if (0 == rc && '\0' != temp[0]) {
         if (0 != strcmp("0", temp)) {
            isLanHostTracking              = 1;
         }
      } else {
         temp[0] = '\0';
         sysevent_get(sysevent_fd, sysevent_token, "lanhost_tracking_enabled", temp, sizeof(temp));
         if ( '\0' != temp[0] && 0 != strcmp("0", temp)) {      
            isLanHostTracking              = 1;
         }
      }

      /*
       * Are we using DMZ based on mac address
       */
      temp[0] = '\0';
      if (isDmzEnabled) {
         rc = syscfg_get(NULL, "dmz_dst_ip_addr", temp, sizeof(temp));
         if (0 != rc || '\0' == temp[0]) {
            temp[0] = '\0';
            rc = syscfg_get(NULL, "dmz_dst_mac_addr", temp, sizeof(temp));
            if (0 == rc && '\0' != temp[0]) {
               isDMZbyMAC  = 1;
            }
         }
      }
   }

   reserved_mgmt_port[0] = '\0';
   rc = syscfg_get(NULL, "http_admin_port", reserved_mgmt_port, sizeof(reserved_mgmt_port));
   if (0 != rc || '\0' == reserved_mgmt_port[0]) {
      snprintf(reserved_mgmt_port, sizeof(reserved_mgmt_port), "80");
   } 
   
   /* Get DSCP value for gre */
   if(bus_handle != NULL){
       rc = PSM_VALUE_GET_STRING(PSM_NAME_GRE_DSCP_VALUE, pStr);
       if(rc == CCSP_SUCCESS && pStr != NULL){
          greDscp = atoi(pStr);
          Ansc_FreeMemory_Callback(pStr);
          pStr = NULL;
       }   
   }
    FIREWALL_DEBUG("Exiting prepare_globals_from_configuration\n");       
   return(0);
}

/*
 ****************************************************************
 *               IPv4 Firewall                                  *
 ****************************************************************
 */

/*
 =================================================================
             Logging
 =================================================================
 */

/*
 *  Procedure     : do_raw_logs
 *  Purpose       : prepare the iptables-restore statements with statements for logging
 *                  the raw table
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *                  protocol.
 *  Return Values :
 *     0               : done
 */
 int do_raw_logs(FILE *fp)
{
   char str[MAX_QUERY];
  // FIREWALL_DEBUG("Entering do_raw_logs\n");       
 if (isLogEnabled) {
      if (isLogSecurityEnabled) {
         snprintf(str, sizeof(str),
                  "-A xlog_drop_lanattack -j LOG --log-prefix \"UTOPIA: FW.LANATTACK DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);
      }
   }
   snprintf(str, sizeof(str),
            "-A xlog_drop_lanattack -j DROP");
   fprintf(fp, "%s\n", str);
  // FIREWALL_DEBUG("Exiting do_raw_logs\n");       
   return(0);
}

/*
 *  Procedure     : do_logs
 *  Purpose       : prepare the iptables-restore statements with statements for logging
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *                  protocol.
 *  Return Values :
 *     0               : done
 */
 int do_logs(FILE *fp)
{
   char str[MAX_QUERY];
  // FIREWALL_DEBUG("Entering do_logs\n");       
   /*
    * Aside from the general idea that logging is enabled,
    * we can turn on/off certain logs according to whether
    * they are security related, incoming, or outgoing
    */
   if (isLogEnabled) {
      if (isLogOutgoingEnabled) {
            snprintf(str, sizeof(str),
                     "-A xlog_accept_lan2wan -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.LAN2WAN ACCEPT \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
            fprintf(fp, "%s\n", str);
      }

      if (isLogIncomingEnabled) {
         snprintf(str, sizeof(str),
                  "-A xlog_accept_wan2lan -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.WAN2LAN ACCEPT \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlog_accept_wan2self -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.WAN2SELF ACCEPT \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlog_drop_wan2lan -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.WAN2LAN DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlog_drop_wan2self -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.WAN2SELF DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlogdrop -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlogreject -j LOG --log-prefix \"UTOPIA: FW.REJECT \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);
      }

      if (isLogSecurityEnabled) {
         snprintf(str, sizeof(str),
                  "-A xlog_drop_wanattack -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.WANATTACK DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlog_drop_lanattack -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.LANATTACK DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlog_drop_lan2self -m state --state NEW -j LOG --log-prefix \"UTOPIA: FW.LAN2SELF DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A xlog_drop_lan2wan -j LOG --log-prefix \"UTOPIA: FW.LAN2WAN DROP \" --log-level %d --log-tcp-sequence --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", syslog_level);
         fprintf(fp, "%s\n", str);
      }

   }

   snprintf(str, sizeof(str),
            "-A xlog_accept_lan2wan -j ACCEPT");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlog_accept_wan2lan -j ACCEPT");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlog_accept_wan2self -j ACCEPT");
   fprintf(fp, "%s\n", str);
#if !(defined INTEL_PUMA7) && !(defined _COSA_BCM_ARM_)
   snprintf(str, sizeof(str),
            "-A xlog_drop_wan2lan -j DROP");
   fprintf(fp, "%s\n", str);
#endif
   snprintf(str, sizeof(str),
            "-A xlog_drop_wan2self -j DROP");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlog_drop_wanattack -j DROP");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlog_drop_lanattack -j DROP");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlog_drop_lan2self -j DROP");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlog_drop_lan2wan -j DROP");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlogdrop -j DROP");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A xlogreject -p tcp -m tcp -j REJECT --reject-with tcp-reset");
   fprintf(fp, "%s\n", str);
   // for non tcp
   snprintf(str, sizeof(str),
            "-A xlogreject -j DROP");
   fprintf(fp, "%s\n", str);
    //       FIREWALL_DEBUG("Exiting do_logs\n");       
   return(0);
}

/*
 =================================================================
              Port Forwarding
 =================================================================
 */
/*
 *  Procedure     : do_single_port_forwarding
 *  Purpose       : prepare the iptables-restore statements for single port forwarding
 *  Parameters    : 
 *     nat_fp          : An open file for nat table writes
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int do_single_port_forwarding(FILE *nat_fp, FILE *filter_fp, int iptype, FILE *filter_fp_v6)
{
   /*
    * syscfg tuple SinglePortForward_x, where x is a digit
    * keeps track of the syscfg namespace of a defined port forwarding rule.
    * We iterate through these tuples until we dont find another instance in syscfg.
    */ 

   int idx;
   char namespace[MAX_NAMESPACE];
   char query[MAX_QUERY];
   int  rc;
   int  count;
   char *tmp = NULL;
           FIREWALL_DEBUG("Entering do_single_port_forwarding\n");       
   query[0] = '\0';
   rc = syscfg_get(NULL, "SinglePortForwardCount", query, sizeof(query)); 
   if (0 != rc || '\0' == query[0]) {
      goto SinglePortForwardNext;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto SinglePortForwardNext;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }
   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "SinglePortForward_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }
   
      // is the rule enabled
      query[0] = '\0';
      rc = syscfg_get(namespace, "enabled", query, sizeof(query));
      if (0 != rc || '\0' == query[0]) {
         continue;
      } else if (0 == strcmp("0", query)) {
        continue;
      }
   
      // what is the ip address to forward to
      char toip[40];
      toip[0] = '\0';
      char toipv6[64];
      toipv6[0] = '\0';
      
      if(iptype == AF_INET){
          rc = syscfg_get(namespace, "to_ip", toip, sizeof(toip));
          /* some time user only need IPv6 forwarding, in this case 255.255.255.255 will be set as toip. 
          * so we needn't do anything about those entry */
          if (0 != rc || '\0' == toip[0] || !strcmp("255.255.255.255", toip)) {
             continue;
          }
      }else{
        rc = syscfg_get(namespace, "to_ipv6", toipv6, sizeof(toipv6));
        if (0 != rc || '\0' == toipv6[0] || strcmp("x", toipv6) == 0) {
            continue;
        }
      }

      // what is the destination port for the protocol we are forwarding
      char external_port[10];
      external_port[0] = '\0';
      rc = syscfg_get(namespace, "external_port", external_port, sizeof(external_port));
      if (0 != rc || '\0' == external_port[0]) {
         continue;
      }

      // what is the forwarded destination port for the protocol
      char internal_port[10];
      internal_port[0] = '\0';
      rc = syscfg_get(namespace, "internal_port", internal_port, sizeof(internal_port));
      if (0 != rc || '\0' == internal_port[0]) {
         snprintf(internal_port, sizeof(internal_port), "%s", external_port);
      }
      char port_modifier[10];
      if ('\0' == internal_port[0] || 0 == strcmp(internal_port, external_port) || 0 == strcmp(internal_port, "0") ) {
        port_modifier[0] = '\0';
      } else {
        snprintf(port_modifier, sizeof(port_modifier), ":%s", internal_port);
      }

      // what is the forwarded protocol
      char prot[10];
      prot[0] = '\0';
      rc = syscfg_get(namespace, "protocol", prot, sizeof(prot));
      if (0 != rc || '\0' == prot[0]) {
         snprintf(prot, sizeof(prot), "%s", "both");
      }

      //PortForwarding in IPv6 is to overwrite the Firewall wan2lan rules
      if(iptype == AF_INET6) {
          if (0 == strcmp("both", prot) || 0 == strcmp("tcp", prot)) {
              fprintf(filter_fp_v6, "-A wan2lan -p tcp -m tcp -d %s --dport %s -j ACCEPT\n", toipv6, external_port);
          }

          if (0 == strcmp("both", prot) || 0 == strcmp("udp", prot)) {
              fprintf(filter_fp_v6, "-A wan2lan -p udp -m udp -d %s --dport %s -j ACCEPT\n", toipv6, external_port);
          }

          continue;
      }
      
      char str[MAX_QUERY];
      
      if (0 == strcmp("both", prot) || 0 == strcmp("tcp", prot)) {
         if (isNatReady) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromwan -p tcp -m tcp -d %s --dport %s -j DNAT --to-destination %s%s",
                     natip4, external_port, toip, port_modifier);
            fprintf(nat_fp, "%s\n", str);
         }
         
         if(isHairpin){
             if (isNatReady) {
               snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p tcp -m tcp -d %s --dport %s -j DNAT --to-destination %s%s",
                        natip4, external_port, toip, port_modifier);
               fprintf(nat_fp, "%s\n", str);
 
                if(strcmp(internal_port, "0")){
                    tmp = internal_port; 
                }else{
                    tmp = external_port;
                }
                snprintf(str, sizeof(str),
                    "-A postrouting_tolan -s %s.0/%s -p tcp -m tcp -d %s --dport %s -j SNAT --to-source %s", 
                     lan_3_octets, lan_netmask, toip, tmp, natip4);
                fprintf(nat_fp, "%s\n", str);
            }
         }else if (!isNatRedirectionBlocked) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromlan -p tcp -m tcp -d %s --dport %s -j DNAT --to-destination %s%s",
                     lan_ipaddr, external_port, toip, port_modifier);
            fprintf(nat_fp, "%s\n", str);
         
            if (isNatReady) {
               snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p tcp -m tcp -d %s --dport %s -j DNAT --to-destination %s%s",
                        natip4, external_port, toip, port_modifier);
               fprintf(nat_fp, "%s\n", str);
            }

            if(strcmp(internal_port, "0")){
                snprintf(str, sizeof(str),
                         "-A postrouting_tolan -s %s.0/%s -p tcp -m tcp -d %s --dport %s -j SNAT --to-source %s", 
                        lan_3_octets, lan_netmask, toip, internal_port, lan_ipaddr);
                fprintf(nat_fp, "%s\n", str);
            }else{
                snprintf(str, sizeof(str),
                         "-A postrouting_tolan -s %s.0/%s -p tcp -m tcp -d %s --dport %s -j SNAT --to-source %s", 
                        lan_3_octets, lan_netmask, toip, external_port, lan_ipaddr);
                fprintf(nat_fp, "%s\n", str);
            }
         }
         if (filter_fp) {
            if(strcmp(internal_port, "0")){
                snprintf(str, sizeof(str),
                        "-A wan2lan_forwarding_accept -p tcp -m tcp -d %s --dport %s -j xlog_accept_wan2lan", 
                        toip, internal_port);
                fprintf(filter_fp, "%s\n", str);
#ifdef PORTMAPPING_2WAY_PASSTHROUGH
            snprintf(str, sizeof(str),
                 "-A lan2wan_forwarding_accept -p tcp -m tcp -s %s --sport %s -j xlog_accept_lan2wan", 
                 toip, internal_port);
            fprintf(filter_fp, "%s\n", str);
#endif
         }else{
            snprintf(str, sizeof(str),
                      "-A wan2lan_forwarding_accept -p tcp -m tcp -d %s --dport %s -j xlog_accept_wan2lan", 
                     toip, external_port);
            fprintf(filter_fp, "%s\n", str);
#ifdef PORTMAPPING_2WAY_PASSTHROUGH
            snprintf(str, sizeof(str),
                 "-A lan2wan_forwarding_accept -p tcp -m tcp -s %s --sport %s -j xlog_accept_lan2wan", 
                 toip, external_port);
            fprintf(filter_fp, "%s\n", str);
#endif
         }

            fprintf(filter_fp, "%s\n", str);
         }
      }
      if (0 == strcmp("both", prot) || 0 == strcmp("udp", prot)) {
         if (isNatReady) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromwan -p udp -m udp -d %s --dport %s -j DNAT --to-destination %s%s",
                     natip4, external_port, toip, port_modifier);
            fprintf(nat_fp, "%s\n", str);
         }
         if(isHairpin){
             if (isNatReady) {
               snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p udp -m udp -d %s --dport %s -j DNAT --to-destination %s%s",
                        natip4, external_port, toip, port_modifier);
               fprintf(nat_fp, "%s\n", str);
 
                if(strcmp(internal_port, "0")){
                    tmp = internal_port; 
                }else{
                    tmp = external_port;
                }
                snprintf(str, sizeof(str),
                    "-A postrouting_tolan -s %s.0/%s -p udp -m udp -d %s --dport %s -j SNAT --to-source %s", 
                     lan_3_octets, lan_netmask, toip, tmp, natip4);
                fprintf(nat_fp, "%s\n", str);
            }
         }else if (!isNatRedirectionBlocked) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromlan -p udp -m udp -d %s --dport %s -j DNAT --to-destination %s%s",
                     lan_ipaddr, external_port, toip, port_modifier);
            fprintf(nat_fp, "%s\n", str);

            if (isNatReady) {
               snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p udp -m udp -d %s --dport %s -j DNAT --to-destination %s%s",
                        natip4, external_port, toip, port_modifier);
               fprintf(nat_fp, "%s\n", str);
            }

            if(strcmp(internal_port, "0")){
                snprintf(str, sizeof(str),
                     "-A postrouting_tolan -s %s.0/%s -p udp -m udp -d %s --dport %s -j SNAT --to-source %s", 
                    lan_3_octets, lan_netmask, toip, internal_port, lan_ipaddr);
                fprintf(nat_fp, "%s\n", str);
            }else{
                snprintf(str, sizeof(str),
                     "-A postrouting_tolan -s %s.0/%s -p udp -m udp -d %s --dport %s -j SNAT --to-source %s", 
                    lan_3_octets, lan_netmask, toip, external_port, lan_ipaddr);
                fprintf(nat_fp, "%s\n", str);
            }
         }
         if (filter_fp) {
            if(strcmp(internal_port, "0")){
                snprintf(str, sizeof(str),
                    "-A wan2lan_forwarding_accept -p udp -m udp -d %s --dport %s -j xlog_accept_wan2lan", 
                    toip, internal_port);
                fprintf(filter_fp, "%s\n", str);
#ifdef PORTMAPPING_2WAY_PASSTHROUGH
            snprintf(str, sizeof(str),
                 "-A lan2wan_forwarding_accept -p udp -m udp -s %s --sport %s -j xlog_accept_lan2wan", 
                 toip, internal_port);
            fprintf(filter_fp, "%s\n", str);
#endif
         }else{
            snprintf(str, sizeof(str),
                 "-A wan2lan_forwarding_accept -p udp -m udp -d %s --dport %s -j xlog_accept_wan2lan", 
                 toip, external_port);
            fprintf(filter_fp, "%s\n", str);
#ifdef PORTMAPPING_2WAY_PASSTHROUGH
            snprintf(str, sizeof(str),
                 "-A lan2wan_forwarding_accept -p udp -m udp -s %s --sport %s -j xlog_accept_lan2wan", 
                 toip, external_port);
            fprintf(filter_fp, "%s\n", str);
#endif
            }
         }
      }
#ifndef PORTMAPPING_2WAY_PASSTHROUGH
            if (filter_fp) {
                snprintf(str, sizeof(str),
                    "-A lan2wan_forwarding_accept -m conntrack --ctstate DNAT -j xlog_accept_lan2wan", 
                    toip, internal_port);
                fprintf(filter_fp, "%s\n", str);
            }
#endif
   }
SinglePortForwardNext:
           FIREWALL_DEBUG("Exiting do_single_port_forwarding\n");       
   return(0);
}

/*
 *  Procedure     : do_port_range_forwarding
 *  Purpose       : prepare the iptables-restore statements for port range forwarding
 *  Parameters    : 
 *     nat_fp          : An open file for nat table writes
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int do_port_range_forwarding(FILE *nat_fp, FILE *filter_fp, int iptype, FILE *filter_fp_v6)
{
   int idx;
   char namespace[MAX_NAMESPACE];
   char query[MAX_QUERY];
   int  rc;

   char str[MAX_QUERY];
   int count;
   query[0] = '\0';
           FIREWALL_DEBUG("Entering do_port_range_forwarding\n");       
   rc = syscfg_get(NULL, "PortRangeForwardCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto PortRangeForwardNext;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto PortRangeForwardNext;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "PortRangeForward_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }
   
      // is the rule enabled
      query[0] = '\0';
      rc = syscfg_get(namespace, "enabled", query, sizeof(query));
      if (0 != rc || '\0' == query[0]) {
         continue;
      } else if (0 == strcmp("0", query)) {
        continue;
      }
       
      // what is the ip address to forward to
      char toip[40];
      toip[0] = '\0';
      char toipv6[64];
      toipv6[0] = '\0';
      char public_ip[40]; 
      public_ip[0] = '\0';

      /* seting IPv4 rule */
      if(iptype == AF_INET){ 
          rc = syscfg_get(namespace, "to_ip", toip, sizeof(toip));
          /* some time user only need IPv6 forwarding, in this case 255.255.255.255 will be set as toip. 
           * so we needn't do anything about those entry */
          if ( 0 != rc || '\0' == toip[0] || strcmp("255.255.255.255", toip) == 0 ) {
             continue;
          }

          rc = syscfg_get(namespace, "public_ip", public_ip, sizeof(public_ip));
          /* In older version public ip field is not exist.
           * so if it get failed, keep doing the next step */ 
          if(0 == rc && '\0' != public_ip[0] ){
              // do one-2-one nat 
              if( 0 != strcmp("0.0.0.0", public_ip)){
/* if TRUE static IP not be configed , skip one 2 one nat */
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
                 if (isWanReady && isWanStaticIPReady) {
                     snprintf(str, sizeof(str),
                         "-A prerouting_fromwan -d %s -j DNAT --to-destination %s",
                         public_ip, toip);
                    fprintf(nat_fp, "%s\n", str);
                    snprintf(str, sizeof(str), 
                        "-A postrouting_towan -s %s -j SNAT --to-source %s", 
                        toip, public_ip);
                    fprintf(nat_fp, "%s\n", str);
                    if (filter_fp) {
                        snprintf(str, sizeof(str),
                            "-A wan2lan_forwarding_accept  -d %s -j xlog_accept_wan2lan", 
                            toip);
                        fprintf(filter_fp, "%s\n", str);
                        /* one 2 one should work even nat disable */ 
                        if(!isNatReady){
                            snprintf(str, sizeof(str), 
                                "-I lan2wan_disable -s %s -j xlog_accept_lan2wan", toip);
                            fprintf(filter_fp, "%s\n", str);
                            snprintf(str, sizeof(str), 
                                "-I wan2lan_disabled -d %s -j xlog_accept_wan2lan", toip);
                            fprintf(filter_fp, "%s\n", str);
                        }

#ifdef PORTMAPPING_2WAY_PASSTHROUGH
                        snprintf(str, sizeof(str),
                            "-A lan2wan_forwarding_accept -s %s -j xlog_accept_lan2wan", 
                            toip);
                        fprintf(filter_fp, "%s\n", str);
#endif
                    }
                 }
#endif
                 continue;
              }
          }
      /* setting IPv6 rule */
      }else{
          rc = syscfg_get(namespace, "to_ipv6", toipv6, sizeof(toipv6));
          if (0 != rc || toipv6[0] == '\0' || strcmp("x", toipv6) == 0 || strcmp("0", toipv6) == 0) {
             continue;
          }
      }

      // what is the destination port for the protocol we are forwarding
      char sdport[10];
      char edport[10];
      char portrange[30];
      portrange[0]='\0';
      sdport[0] = '\0';
      edport[0] = '\0';
      rc = syscfg_get(namespace, "external_port_range", portrange, sizeof(portrange));
      if (0 != rc || '\0' == portrange[0]) {
         continue;
      } else {
         if (2 != sscanf(portrange, "%10s %10s", sdport, edport)) {
            continue;
         }
      }

      // how long is the port range
      int s = atoi(sdport);
      int e = atoi(edport);
      int range= e-s;
      if (0 > range) {
         range=0;
      }

      // what is the forwarded destination port for the protocol
      char toport[10];
      int internal_port = 0;
      toport[0] = '\0';
      rc = syscfg_get(namespace, "internal_port", toport, sizeof(toport));
      if (0 == rc && '\0' != toport[0]) {
         internal_port = atoi(toport);
         if (internal_port < 0) internal_port = 0;
      }

      // what is the last port of the destination range
      char rangesize[10] = "";
      int internal_port_range_size = 0;
      rc = syscfg_get(namespace, "internal_port_range_size", rangesize, sizeof(rangesize));
      if (0 == rc && '\0' != rangesize[0]) {
         internal_port_range_size = atoi(rangesize);
         if (internal_port_range_size < 0) internal_port_range_size = 0;
      }

      // what is the forwarded protocol
      char prot[10];
      prot[0] = '\0';
      rc = syscfg_get(namespace, "protocol", prot, sizeof(prot));
      if (0 != rc || '\0' == prot[0]) {
         snprintf(prot, sizeof(prot), "%s", "both");
      }

      char target_internal_port[40] = "";
      char match_internal_port[40] = "";

      if (internal_port)
      {
         if (internal_port_range_size)
         {
            // range -> range, random port translation
            snprintf(match_internal_port, sizeof(match_internal_port), "%d:%d", internal_port, internal_port+internal_port_range_size);
            snprintf(target_internal_port, sizeof(target_internal_port), ":%d-%d", internal_port, internal_port+internal_port_range_size);
         }
         else
         {
            // range -> one port translation
            snprintf(match_internal_port, sizeof(match_internal_port), "%d", internal_port);
            snprintf(target_internal_port, sizeof(target_internal_port), ":%d", internal_port);
         }
      }
      else
      {
         // no port translation
         snprintf(match_internal_port, sizeof(match_internal_port), "%s:%s", sdport, edport);
      }

      //PortForwarding in IPv6 is to overwrite the Firewall wan2lan rules
      if(iptype == AF_INET6) {
          if (0 == strcmp("both", prot) || 0 == strcmp("tcp", prot)) {
              fprintf(filter_fp_v6, "-A wan2lan -p tcp -m tcp -d %s --dport %s:%s -j ACCEPT\n", toipv6, sdport, edport);
          }

          if (0 == strcmp("both", prot) || 0 == strcmp("udp", prot)) {
              fprintf(filter_fp_v6, "-A wan2lan -p udp -m udp -d %s --dport %s:%s -j ACCEPT\n", toipv6, sdport, edport);
          }

          continue;
      }
      
      char str[MAX_QUERY];
      
      if (0 == strcmp("both", prot) || 0 == strcmp("tcp", prot)) {
         if (isNatReady) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromwan -p tcp -m tcp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                     natip4, sdport, edport, toip, target_internal_port);
            fprintf(nat_fp, "%s\n", str);
         }

         if(isHairpin){
             if (isNatReady) {
                snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p tcp -m tcp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                        natip4, sdport, edport, toip, target_internal_port);
                fprintf(nat_fp, "%s\n", str);
 
                snprintf(str, sizeof(str),
                    "-A postrouting_tolan -s %s.0/%s -p tcp -m tcp -d %s --dport %s -j SNAT --to-source %s", 
                     lan_3_octets, lan_netmask, toip, match_internal_port, natip4);
                fprintf(nat_fp, "%s\n", str);
            }
         }else if (!isNatRedirectionBlocked) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromlan -p tcp -m tcp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                     lan_ipaddr, sdport, edport, toip, target_internal_port);
            fprintf(nat_fp, "%s\n", str);

            if (isNatReady) {
               snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p tcp -m tcp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                        natip4, sdport, edport, toip, target_internal_port);
               fprintf(nat_fp, "%s\n", str);
            }

            snprintf(str, sizeof(str),
                     "-A postrouting_tolan -s %s.0/%s -p tcp -m tcp -d %s --dport %s -j SNAT --to-source %s", 
                    lan_3_octets, lan_netmask, toip, match_internal_port, lan_ipaddr);
            fprintf(nat_fp, "%s\n", str);
         }

         if (filter_fp) {
            snprintf(str, sizeof(str),
                    "-A wan2lan_forwarding_accept -p tcp -m tcp -d %s --dport %s -j xlog_accept_wan2lan", 
                    toip, match_internal_port);
            fprintf(filter_fp, "%s\n", str);

#ifdef PORTMAPPING_2WAY_PASSTHROUGH
            snprintf(str, sizeof(str),
                    "-A lan2wan_forwarding_accept -p tcp -m tcp -s %s --sport %s -j xlog_accept_lan2wan", 
                    toip, match_internal_port);
            fprintf(filter_fp, "%s\n", str);
#endif
         }
      }
      if (0 == strcmp("both", prot) || 0 == strcmp("udp", prot)) {
         if (isNatReady) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromwan -p udp -m udp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                     natip4, sdport, edport, toip, target_internal_port);
            fprintf(nat_fp, "%s\n", str);
         }
 
         if(isHairpin){
             if (isNatReady) {
                snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p udp -m udp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                        natip4, sdport, edport, toip, target_internal_port);
                fprintf(nat_fp, "%s\n", str);
 
                snprintf(str, sizeof(str),
                    "-A postrouting_tolan -s %s.0/%s -p udp -m udp -d %s --dport %s -j SNAT --to-source %s", 
                     lan_3_octets, lan_netmask, toip, match_internal_port, natip4);
                fprintf(nat_fp, "%s\n", str);
            }
         }else if (!isNatRedirectionBlocked) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromlan -p udp -m udp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                     lan_ipaddr, sdport, edport, toip, target_internal_port);
            fprintf(nat_fp, "%s\n", str);

            if (isNatReady) {
               snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p udp -m udp -d %s --dport %s:%s -j DNAT --to-destination %s%s",
                        natip4, sdport, edport, toip, target_internal_port);
               fprintf(nat_fp, "%s\n", str);
            }

            snprintf(str, sizeof(str),
                    "-A postrouting_tolan -s %s.0/%s -p udp -m udp -d %s --dport %s -j SNAT --to-source %s", 
                    lan_3_octets, lan_netmask, toip, match_internal_port, lan_ipaddr);
            fprintf(nat_fp, "%s\n", str);
        }

        if(filter_fp){
            snprintf(str, sizeof(str),
                    "-A wan2lan_forwarding_accept -p udp -m udp -d %s --dport %s -j xlog_accept_wan2lan", 
                    toip, match_internal_port);
            fprintf(filter_fp, "%s\n", str);

#ifdef PORTMAPPING_2WAY_PASSTHROUGH
         snprintf(str, sizeof(str),
                  "-A lan2wan_forwarding_accept -p udp -m udp -s %s --sport %s -j xlog_accept_lan2wan", 
                  toip, match_internal_port);
         fprintf(filter_fp, "%s\n", str);
#endif
        }
      }
#ifndef PORTMAPPING_2WAY_PASSTHROUGH
    if(filter_fp) {
            snprintf(str, sizeof(str),
                 "-A lan2wan_forwarding_accept -m conntrack --ctstate DNAT -j xlog_accept_lan2wan", 
                 toip, internal_port);
            fprintf(filter_fp, "%s\n", str);
    }
#endif

   }
PortRangeForwardNext:
           FIREWALL_DEBUG("Exiting do_port_range_forwarding\n");       
   return(0);
}

/*
 *  Procedure     : do_wellknown_ports_forwarding
 *  Purpose       : prepare the iptables-restore statements for port forwarding based on
 *                  lookups to /etc/services
 *  Parameters    : 
 *     nat_fp          : An open file for nat table writes
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int do_wellknown_ports_forwarding(FILE *nat_fp, FILE *filter_fp)
{
   int idx;
   char *filename = wellknown_ports_file_dir"/"wellknown_ports_file;
   FILE *wkp_fp = NULL;
   char namespace[MAX_NAMESPACE];
   char query[MAX_QUERY];
   int  rc;

           FIREWALL_DEBUG("Entering do_wellknown_ports_forwarding\n");       
   wkp_fp = fopen(filename, "r");
   if (NULL == wkp_fp) {
      return(-1);
   }

   query[0] = '\0';
   int count;
   rc = syscfg_get(NULL, "WellKnownPortForwardCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto WellKnownPortForwardNext;
   } else {
      count = atoi(query);
      ulogf(ULOG_FIREWALL, UL_INFO, "\n @@@@@ WellKnownCount = %d \n", count);
      if (0 == count) {
         goto WellKnownPortForwardNext;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "WellKnownPortForward_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }
   
      // is the rule enabled
      query[0] = '\0';
      rc = syscfg_get(namespace, "enabled", query, sizeof(query));
      ulogf(ULOG_FIREWALL, UL_INFO, "\n @@@@@ WellKnown::Enabled = %d \n", atoi(query));
      if (0 != rc || '\0' == query[0]) {
         continue;
      } else if (0 == strcmp("0", query)) {
        continue;
      }

      // what is the last octet of the ip address to forward to
      char toip[10];
      toip[0] = '\0';
      rc = syscfg_get(namespace, "to_ip", toip, sizeof(toip));
      ulogf(ULOG_FIREWALL, UL_INFO, "\n @@@@@ WellKnown::to_ip = %s \n", toip);
      if (0 != rc || '\0' == toip[0]) {
         continue;
      }
   
      // what is the name of the well known port
      char name[50];
      name[0] = '\0';
      rc = syscfg_get(namespace, "name", name, sizeof(name));
      if (0 != rc || '\0' == name[0]) {
         continue;
      } 

      // what is the forwarded destination port for the protocol
      char toport[10];
      toport[0] = '\0';
      syscfg_get(namespace, "internal_port", toport, sizeof(toport));
      ulogf(ULOG_FIREWALL, UL_INFO, "\n @@@@@ WellKnown::port = %s \n", toport);

      // what is the destination port for the protocol we are forwarding based on its name
      char *next_token;
      char *port_prot;
      char *port_val;
      char  line[MAX_QUERY];

      while (NULL != (next_token = match_keyword(wkp_fp, name, ' ', line, sizeof(line))) ) {
         char port_str[50];
         sscanf(next_token, "%50s ", port_str);
         port_val = port_str;
         port_prot = strchr(port_val, '/');
         if (NULL != port_prot) {
            *port_prot = '\0';
            port_prot++;
         } else {
            continue;
         }
         char port_modifier[10];
         if ('\0' == toport[0] || 0 == strcmp(toport, port_val) ) {
           port_modifier[0] = '\0';
         } else {
           snprintf(port_modifier, sizeof(port_modifier), ":%s", toport);
         }

         char str[MAX_QUERY];
         if (isWanReady) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromwan -p %s -m %s -d %s --dport %s -j DNAT --to-destination %s.%s%s",
                     port_prot, port_prot, current_wan_ipaddr, port_val, lan_3_octets, toip, port_modifier);
            fprintf(nat_fp, "%s\n", str);
         }

         if (!isNatRedirectionBlocked) {
            snprintf(str, sizeof(str),
                  "-A prerouting_fromlan -p %s -m %s -d %s --dport %s -j DNAT --to-destination %s.%s%s",
                  port_prot, port_prot, lan_ipaddr, port_val, lan_3_octets, toip, port_modifier);
            fprintf(nat_fp, "%s\n", str);

            if (isWanReady) {
               snprintf(str, sizeof(str),
                     "-A prerouting_fromlan -p %s -m %s -d %s --dport %s -j DNAT --to-destination %s.%s%s",
                     port_prot, port_prot, current_wan_ipaddr, port_val, lan_3_octets, toip, port_modifier);
               fprintf(nat_fp, "%s\n", str);
            }

            snprintf(str, sizeof(str),
                  "-A postrouting_tolan -s %s.0/%s -p %s -m %s -d %s.%s --dport %s -j SNAT --to-source %s", 
                 lan_3_octets, lan_netmask, port_prot, port_prot, lan_3_octets, toip, '\0' == toport[0] ? port_val : toport, lan_ipaddr);
            fprintf(nat_fp, "%s\n", str);
         }

         if(filter_fp) {
            snprintf(str, sizeof(str),
                    "-A wan2lan_forwarding_accept -p %s -m %s -d %s.%s --dport %s -j xlog_accept_wan2lan", port_prot, port_prot, lan_3_octets, toip, '\0' == toport[0] ? port_val : toport);
            fprintf(filter_fp, "%s\n", str);
         }
      }
   }
WellKnownPortForwardNext:
   fclose(wkp_fp);
           FIREWALL_DEBUG("Exiting do_wellknown_ports_forwarding\n");       
   return(0);
}

/*
 *  Procedure     : do_ephemeral_port_forwarding
 *  Purpose       : prepare the iptables-restore statements for port forwarding statements
 *                  defined in sysevent
 *  Parameters    :
 *     nat_fp          : An open file for nat table writes
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */

static int do_ephemeral_port_forwarding(FILE *nat_fp, FILE *filter_fp)
{
   unsigned int iterator;
   char          name[MAX_QUERY];
   char          rule[MAX_QUERY];
   char          in_rule[MAX_QUERY];
   char          subst[MAX_QUERY];
           FIREWALL_DEBUG("Entering do_ephemeral_port_forwarding\n");       
   iterator = SYSEVENT_NULL_ITERATOR;
   do {
      name[0] = rule[0] = subst[0] = '\0';
      memset(in_rule, 0, sizeof(in_rule));
      sysevent_get_unique(sysevent_fd, sysevent_token,
                          "portmap_dyn_pool", &iterator,
                          name, sizeof(name), in_rule, sizeof(in_rule));
      if ('\0' != in_rule[0]) {
         // the rule we have looks like enabled|disabled,external_ip,external_port,internal_ip,internal_port,protocol,...
         char *next;
         char *token = in_rule;
         next = token_get(token, ',');
         if (NULL == token || NULL == next || 0 == strcmp(token, "disabled")) {
            continue;
         }
         char *fromip;
         fromip = next;
         next = token_get(fromip, ',');
         if (NULL == fromip || NULL == next) {
            continue;
         }
         char *fromport;
         fromport = next;
         next = token_get(fromport, ',');
         if (NULL == fromport || NULL == next) {
            continue;
         }
         char *toip;
         toip = next;
         next = token_get(toip, ',');
         if (NULL == toip || NULL == next) {
            continue;
         }
         char *dport;
         dport = next;
         next = token_get(dport, ',');
         if (NULL == dport || NULL == next) {
            continue;
         }
         char *prot;
         prot = next;
         next = token_get(prot, ',');
         if (NULL == prot) {
            continue;
         }

         char external_ip[50];
         char external_dest_port[50];
         external_ip[0] ='\0';
         external_dest_port[0] ='\0';
         if (0 != strcmp("none", fromip)) {
            snprintf(external_ip, sizeof(external_ip), "-s %s", fromip); 
         } 
         if (0 != strcmp("none", fromport)) {
            snprintf(external_dest_port, sizeof(external_dest_port), "--dport %s", fromport);
         } 

         char port_modifier[10];
         if ('\0' == dport[0] || 0 == strcmp(dport, fromport) ) {
           port_modifier[0] = '\0';
         } else {
           snprintf(port_modifier, sizeof(port_modifier), ":%s", dport);
         }

         
         char str[MAX_QUERY];
         if (0 == strcmp("both", prot) || 0 == strcmp("tcp", prot)) {
            if (isNatReady) {
               snprintf(str, sizeof(str),
                        "-A prerouting_fromwan -p tcp -m tcp -d %s %s %s -j DNAT --to-destination %s%s",
                        natip4, external_dest_port, external_ip, toip, port_modifier);
               fprintf(nat_fp, "%s\n", str);
            }

            if (!isNatRedirectionBlocked) {
               if (0 == strcmp("none", fromip)) {
                  snprintf(str, sizeof(str),
                        "-A prerouting_fromlan -p tcp -m tcp -d %s %s %s -j DNAT --to-destination %s%s",
                        lan_ipaddr, external_dest_port, external_ip, toip, port_modifier);
                  fprintf(nat_fp, "%s\n", str);

                  if (isNatReady) {
                     snprintf(str, sizeof(str),
                           "-A prerouting_fromlan -p tcp -m tcp -d %s %s %s -j DNAT --to-destination %s%s",
                           natip4, external_dest_port, external_ip, toip, port_modifier);
                     fprintf(nat_fp, "%s\n", str);
                  }

                  snprintf(str, sizeof(str),
                        "-A postrouting_tolan -s %s.0/%s -p tcp -m tcp -d %s --dport %s -j SNAT --to-source %s", 
                       lan_3_octets, lan_netmask, toip, dport, lan_ipaddr);
                  fprintf(nat_fp, "%s\n", str);
               }
            }

            if(filter_fp) {
                snprintf(str, sizeof(str),
                        "-A wan2lan_forwarding_accept -p tcp -m tcp %s -d %s --dport %s -j xlog_accept_wan2lan", 
                            external_ip, toip, dport);
                fprintf(filter_fp, "%s\n", str);
            }
         }
         if (0 == strcmp("both", prot) || 0 == strcmp("udp", prot)) {
            if (isNatReady) {
               snprintf(str, sizeof(str),
                       "-A prerouting_fromwan -p udp -m udp -d %s %s %s -j DNAT --to-destination %s%s",
                       natip4, external_dest_port, external_ip, toip, port_modifier);
               fprintf(nat_fp, "%s\n", str);
            }

            if (!isNatRedirectionBlocked) {
               if (0 == strcmp("none", fromip)) {
                  snprintf(str, sizeof(str),
                    "-A prerouting_fromlan -p udp -m udp -d %s %s %s -j DNAT --to-destination %s%s",
                    lan_ipaddr, external_dest_port, external_ip, toip, port_modifier);
                  fprintf(nat_fp, "%s\n", str);

                  if (isNatReady) {
                     snprintf(str, sizeof(str),
                       "-A prerouting_fromlan -p udp -m udp -d %s %s %s -j DNAT --to-destination %s%s",
                       natip4, external_dest_port, external_ip, toip, port_modifier);
                     fprintf(nat_fp, "%s\n", str);
                  }

                  snprintf(str, sizeof(str),
                        "-A postrouting_tolan -s %s.0/%s -p udp -m udp -d %s --dport %s -j SNAT --to-source %s", 
                          lan_3_octets, lan_netmask, toip, dport, lan_ipaddr);
                  fprintf(nat_fp, "%s\n", str);
               }
            }

            if(filter_fp) {
                snprintf(str, sizeof(str),
                        "-A wan2lan_forwarding_accept -p udp -m udp %s -d %s --dport %s -j xlog_accept_wan2lan", 
                            external_ip, toip, dport);
                fprintf(filter_fp, "%s\n", str);
            }
         }
      }
   } while (SYSEVENT_NULL_ITERATOR != iterator);
           FIREWALL_DEBUG("Exiting do_ephemeral_port_forwarding\n");       
   return(0);
}

/*
 *  Procedure     : do_static_route_forwarding
 *  Purpose       : prepare the iptables-restore statements for port forwarding statements
 *                  to allow wan to reach static routes in lan
 *  Parameters    :
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int do_static_route_forwarding(FILE *filter_fp)
{
   char namespace[MAX_NAMESPACE];
   char query[MAX_QUERY];
   int  rc;
   int idx;
           FIREWALL_DEBUG("Entering do_static_route_forwarding\n");       
   query[0] = '\0';
   int count;
   rc = syscfg_get(NULL, "StaticRouteCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto StaticRouteForwardDone;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto StaticRouteForwardDone;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "StaticRoute_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }

      // is the rule for the lan interface
      query[0] = '\0';
      rc = syscfg_get(namespace, "interface", query, sizeof(query));
      if (0 != rc || '\0' == query[0]) {
         continue;
      } else if (0 != strcmp("lan", query)) {
        continue;
      }

      // extract the dest network
      char dest[MAX_QUERY];
      char netmask[MAX_QUERY];
      dest[0] = '\0';
      rc = syscfg_get(namespace, "dest", dest, sizeof(dest));
      if (0 != rc || '\0' == dest[0]) {
         continue;
      }
      netmask[0] = '\0';
      rc = syscfg_get(namespace, "netmask", netmask, sizeof(netmask));
      if (0 != rc || '\0' == netmask[0]) {
         continue;
      }

      char str[MAX_QUERY];
      snprintf(str, sizeof(str),
                  "-A wan2lan_forwarding_accept -d %s/%s -j xlog_accept_wan2lan",
                  dest, netmask);
       fprintf(filter_fp, "%s\n", str);
    }
StaticRouteForwardDone:
           FIREWALL_DEBUG("Exiting do_static_route_forwarding\n");       
   return(0);
}


/*
 *  Procedure     : do_port_forwarding
 *  Purpose       : prepare the iptables-restore statements for forwarding incoming packets to a lan host
 *  Parameters    : 
 *     nat_fp          : An open file for nat table writes
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 */
static int do_port_forwarding(FILE *nat_fp, FILE *filter_fp)
{

   /*
    * For each type of port forwarding (single_port, port_range etc) there are two distinct iptables rules:
    *   a PREROUTING DNAT rule
    *   an ACCEPT rule
    */
      //     FIREWALL_DEBUG("Entering do_port_forwarding\n");       
   do_single_port_forwarding(nat_fp, filter_fp, AF_INET, NULL);
   do_port_range_forwarding(nat_fp, filter_fp, AF_INET, NULL);
   do_wellknown_ports_forwarding(nat_fp, filter_fp);
   do_ephemeral_port_forwarding(nat_fp, filter_fp);
   if (filter_fp)
    do_static_route_forwarding(filter_fp);
        //   FIREWALL_DEBUG("Exiting do_port_forwarding\n");       
   return(0);
}

/*
 =================================================================
              No NAT
 =================================================================
 */
/*
 *  Procedure     : do_nonat
 *  Purpose       : prepare the iptables-restore statements for forwarding incoming packets to a lan hosts
 *  Parameters    :
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 */
static int do_nonat(FILE *filter_fp)
{

   if (isNatEnabled == NAT_DISABLE) {
      return(0);
   } 
           FIREWALL_DEBUG("Entering do_nonat\n");       
   if (strncasecmp(firewall_level, "High", strlen("High")) != 0)
   {
      // if we are not doing nat, restrict wan to lan traffic per security settings
      if (strncasecmp(firewall_level, "Medium", strlen("Medium")) == 0)
      {
         fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 113 -j RETURN\n"); // IDENT

         fprintf(filter_fp, "-A wan2lan_nonat -p icmp --icmp-type 8 -j RETURN\n"); // ICMP PING

         fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 1214 -j RETURN\n"); // Kazaa
         fprintf(filter_fp, "-A wan2lan_nonat -p udp --dport 1214 -j RETURN\n"); // Kazaa
         fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 6881:6999 -j RETURN\n"); // Bittorrent
         fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 6346 -j RETURN\n"); // Gnutella
         fprintf(filter_fp, "-A wan2lan_nonat -p udp --dport 6346 -j RETURN\n"); // Gnutella
         fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 49152:65534 -j RETURN\n"); // Vuze
      }
      else if (strncasecmp(firewall_level, "Low", strlen("Low")) == 0)
      {
         fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 113 -j RETURN\n"); // IDENT
      }
      else
      {
         if (isHttpBlocked)
         {
            fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 80 -j RETURN\n"); // HTTP
            fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 443 -j RETURN\n"); // HTTPS
         }
         if (isIdentBlocked)
         {
            fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 113 -j RETURN\n"); // IDENT
         }
         if (isPingBlocked)
         {
            fprintf(filter_fp, "-A wan2lan_nonat -p icmp --icmp-type 8 -j RETURN\n"); // ICMP PING
         }
         if (isP2pBlocked)
         {
            fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 1214 -j RETURN\n"); // Kazaa
            fprintf(filter_fp, "-A wan2lan_nonat -p udp --dport 1214 -j RETURN\n"); // Kazaa
            fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 6881:6999 -j RETURN\n"); // Bittorrent
            fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 6346 -j RETURN\n"); // Gnutella
            fprintf(filter_fp, "-A wan2lan_nonat -p udp --dport 6346 -j RETURN\n"); // Gnutella
            fprintf(filter_fp, "-A wan2lan_nonat -p tcp --dport 49152:65534 -j RETURN\n"); // Vuze
         }
      }

      char str[MAX_QUERY];
      snprintf(str, sizeof(str),
               "-A wan2lan_nonat -d %s/%s -j xlog_accept_wan2lan", lan_ipaddr, lan_netmask);
      fprintf(filter_fp, "%s\n", str);
   }
           FIREWALL_DEBUG("Exiting do_nonat\n");       
   return(0);
}

/*
 =================================================================
              DMZ
 =================================================================
 */
/*
 *  Procedure     : do_dmz
 *  Purpose       : prepare the iptables-restore statements for forwarding incoming packets to a dmz lan host
 *  Parameters    : 
 *     nat_fp          : An open file for nat table writes
 *     filter_fp       : An open file for filter table writes
 *  Return Values :
 *     0               : done
 */
static int do_dmz(FILE *nat_fp, FILE *filter_fp)
{


   int rc;
   int  src_type = 0; // 0 is all networks, 1 is an ip[/netmask], 2 is ip range
           FIREWALL_DEBUG("Entering do_dmz\n");       
   if (!isDmzEnabled) {
      return(0);
   } 
 
   // what is the src ip address to forward to our dmz
   char src_str[64];
   src_str[0] = '\0';
   rc = syscfg_get(NULL, "dmz_src_addr_range", src_str, sizeof(src_str));
   if (0 != rc || '\0' == src_str[0]) {
      src_type = 0;
   } else {
      if (0 == strcmp("*", src_str)) {
          src_type = 0;
      } else if (strchr(src_str, '-')) {
         src_type = 2;
      } else {
         src_type = 1;
      }
   }

   // what is the dmz host 
   char tohost[50];
   tohost[0] = '\0';

   rc = syscfg_get(NULL, "dmz_dst_ip_addr", tohost, sizeof(tohost));
   if (0 != rc || '\0' == tohost[0]) {
      // there is no statement for a dmz host found by ip address
      // so check if there is a statement for dmz host found by mac address
      tohost[0] = '\0';
      char mac_addr[50];
      mac_addr[0] = '\0';
      rc = syscfg_get(NULL, "dmz_dst_mac_addr", mac_addr, sizeof(mac_addr));
      if (0 != rc || '\0' == mac_addr[0]) {
         return(0);
      } else {
         // look for the mac address in the dnsmasq file
         FILE *fp2 = fopen("/etc/dnsmasq.leases","r");
         if (NULL != fp2) {
            char line[512];
            while (NULL != fgets(line, sizeof(line), fp2)) {
               if (NULL != strcasestr(line, mac_addr)) {
                  char field1[50];
                  char field2[50];
                  char field3[50];
                  char field4[50];
                  char field5[50];
                  sscanf(line, "%50s %50s %50s %50s %50s", field1, field2, field3, field4, field5);
                  // ip address is in field 3
                  // extract last octet of ip addr
                  char *idx = strrchr(field3, '.');
                  if (NULL != idx) {
                     snprintf(tohost, sizeof(tohost), "%s", idx+1);
                  }
               }
            }
            fclose(fp2);
         }

         if ('\0' == tohost[0]) {
            // we couldnt find the host in the dhcp server file, so try the discovered hosts file
            FILE *kh_fp = fopen(lan_hosts_dir"/"hosts_filename, "r");
            char buf[1024];
            if (NULL != kh_fp) {
                while (NULL != fgets(buf, sizeof(buf), kh_fp)) {
                   char ip[50];
                   char mac[50];
                   sscanf(buf, "%50s %50s", ip, mac);
                   if (0 == strcasecmp(mac, mac_addr)) {
                      // extract last octet of ip address
                      char *idx = strrchr(ip, '.');
                      if (NULL != idx) {
                         snprintf(tohost, sizeof(tohost), "%s", idx+1);
                      }
                     break;
                   }
                }
                fclose(kh_fp);
             }
         }

         if ('\0' == tohost[0]) {
            return(0);
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
         } else {
            isTriggerMonitorRestartNeeded = 1;
#endif
         }
      }
   }

   if ('\0' == tohost[0]) {
      return(0);
   }
      
   char dst_str[64];
   //snprintf(dst_str, sizeof(dst_str), "--to-destination %s.%s ", lan_3_octets, tohost);
   /* tohost is now a full ip address */
   snprintf(dst_str, sizeof(dst_str), "--to-destination %s ", tohost);

   switch (src_type) {
      char str[MAX_QUERY];
      case(0):
         if (isNatReady &&
             strcmp(tohost, "0.0.0.0") != 0) { /* 0.0.0.0 stands for disable in SA-RG-MIB */
            snprintf(str, sizeof(str),
               "-A prerouting_fromwan_todmz --dst %s -p tcp -j DNAT %s", natip4, dst_str);
            fprintf(nat_fp, "%s\n", str);
            
            snprintf(str, sizeof(str),
               "-A prerouting_fromwan_todmz --dst %s -p udp -j DNAT %s", natip4, dst_str);
            fprintf(nat_fp, "%s\n", str);
         }

         /*snprintf(str, sizeof(str),
                  "-A wan2lan_dmz -d %s.%s -j xlog_accept_wan2lan", lan_3_octets, tohost);*/
         snprintf(str, sizeof(str),
                  "-A wan2lan_dmz -d %s -j xlog_accept_wan2lan", tohost);
         fprintf(filter_fp, "%s\n", str);
         snprintf(str, sizeof(str),
                  "-A lan2wan_dmz_accept -s %s -j xlog_accept_wan2lan", tohost);
         fprintf(filter_fp, "%s\n", str);

         break;
      case(1):
         if (isNatReady) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromwan_todmz --src %s --dst %s -j DNAT %s", src_str, natip4, dst_str);
            fprintf(nat_fp, "%s\n", str);
         }

         /*snprintf(str, sizeof(str),
                  "-A wan2lan_dmz -s %s -d %s.%s -j xlog_accept_wan2lan", src_str, lan_3_octets, tohost);*/
         snprintf(str, sizeof(str),
                  "-A wan2lan_dmz -s %s -d %s -j xlog_accept_wan2lan", src_str, tohost);
         fprintf(filter_fp, "%s\n", str);
#ifdef PORTMAPPING_2WAY_PASSTHROUGH
         snprintf(str, sizeof(str),
                  "-A lan2wan_dmz_accept -d %s -s %s -j xlog_accept_lan2wan", src_str, tohost);
         fprintf(filter_fp, "%s\n", str);
#endif
         break;
      case(2):
         if (isNatReady) {
            snprintf(str, sizeof(str),
                     "-A prerouting_fromwan_todmz --dst %s -m iprange --src-range %s -j DNAT %s", natip4, src_str,  dst_str);
            fprintf(nat_fp, "%s\n", str);
         }

         /*snprintf(str, sizeof(str),
                  "-A wan2lan_dmz -m iprange --src-range %s -d %s.%s -j xlog_accept_wan2lan", src_str, lan_3_octets, tohost);*/
         snprintf(str, sizeof(str),
                  "-A wan2lan_dmz -m iprange --src-range %s -d %s -j xlog_accept_wan2lan", src_str, tohost);
         fprintf(filter_fp, "%s\n", str);

#ifdef PORTMAPPING_2WAY_PASSTHROUGH
         snprintf(str, sizeof(str),
                  "-A lan2wan_dmz_accept -m iprange --dst-range %s -s %s -j xlog_accept_lan2wan", src_str, tohost);
         fprintf(filter_fp, "%s\n", str);
#endif
         break;
      default:
         break;
   }
           FIREWALL_DEBUG("Exiting do_dmz\n");       
   return(0);
}

/*
 =================================================================
              QoS
 =================================================================
 */

/*
 *  Procedure     : write_qos_classification_statement
 *  Purpose       : prepare the iptables-restore statements with all qos marking rules for a particular
 *                  protocol as known in the file known as qos_fp.
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *     qos_fp          : An open file containing qos rules in the format
 *                         rule name | friendly name | type | match | iptables hook 
 *                            where type is application | game
 *                       eg.  name | Name Protocol | application | -p tcp -m tcp --dport 22 | PREROUTING |
 *     name            : name of the qos rule.  
 *     class           : DSCP class to mark packets fitting the rule
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 *  Note          : prerouting statements will be put in the mangle table, prerouting_qos subtable
 *  Note          : postrouting statements will be put in the mangle table, postrouting_qos subtable
 *
 */
static int write_qos_classification_statement (FILE *fp, FILE *qos_fp, char *name, char *class)
{
   rewind(qos_fp);
   char line[512];
   char *next_token;
           FIREWALL_DEBUG("Entering write_qos_classification_statement\n");       
   while (NULL != (next_token = match_keyword(qos_fp, name, '|', line, sizeof(line))) ) {

      char *friendly_name = next_token;
      next_token = token_get(friendly_name, '|');
      if (NULL == next_token || NULL == friendly_name) {
         continue;
      }

      char *type = next_token;
      next_token = token_get(type, '|');
      if (NULL == next_token || NULL == type) {
         continue;
      }

      char *match = next_token;
      next_token = token_get(match, '|');
      if (NULL == match) {
         continue;
      }

      char *hook = next_token;
      next_token = token_get(hook, '|');
      char subst_hook[MAX_QUERY];
      if (NULL == next_token || NULL == hook) {
         continue;
      } else {
         if (0 == strcasestr(hook, "PREROUTING")) {
            sprintf(subst_hook, "prerouting_qos");
         } else if (0 == strcasestr(hook, "POSTROUTING") ) {
            sprintf(subst_hook, "postrouting_qos");
         } else {
            continue;
         }
      } 

      char subst[MAX_QUERY];
      char subst2[MAX_QUERY];
      char str[MAX_QUERY];
      snprintf(str, sizeof(str),
               "-A %s %s -j DSCP --set-dscp-class %s", 
              subst_hook, make_substitutions(match,subst,sizeof(subst)), make_substitutions(class, subst2,sizeof(subst2)));
      fprintf(fp, "%s\n", str);
   }
           FIREWALL_DEBUG("Exiting write_qos_classification_statement\n");       
   return(0); 
}

/*
 *  Procedure     : add_qos_marking_statements
 *  Purpose       : prepare the iptables-restore statements for marking packets with DSCP
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int add_qos_marking_statements(FILE *fp)
{
   if (NULL == fp) {
      return(-1);
   }

   int rc;
   char query[MAX_QUERY];
           FIREWALL_DEBUG("Entering add_qos_marking_statements\n");       
   // is the qos enabled
   query[0] = '\0';
   rc = syscfg_get(NULL, "qos_enable", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      return(0);
   } else if (0 == strcmp("0", query)) {
      return(0);
   }
   int count;

   query[0] = '\0';
   rc = syscfg_get(NULL, "QoSPolicyCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto QoSUserDefinedPolicies;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto QoSUserDefinedPolicies;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   /* 
    * syscfg tuple QoSPolicy_x, where x is a digit
    * We iterate through these tuples until we dont find an instance in syscfg.
    */
   int idx;
   char namespace[MAX_NAMESPACE];

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "QoSPolicy_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }

      char rule[MAX_QUERY];
      int i;
      for (i=1; i<MAX_SYSCFG_ENTRIES ; i++) {
         snprintf(query, sizeof(query), "qos_rule_%d", i);
         rc = syscfg_get(namespace, query, rule, sizeof(rule));
         if (0 != rc || '\0' == rule[0]) {
            break;
         } else {
            char subst[MAX_QUERY];
            char str[MAX_QUERY];
            if (NULL != make_substitutions(rule, subst, sizeof(subst))) {
               if (1 == substitute(subst, str, sizeof(str), "PREROUTING", "prerouting_qos") ||
                  (1 == substitute(subst, str, sizeof(str), "POSTROUTING", "postrouting_qos")) ) {
                  fprintf(fp, "%s\n", str);
               }
            }
         }
      }
   }

QoSUserDefinedPolicies:
   query[0] = '\0';
   rc = syscfg_get(NULL, "QoSUserDefinedPolicyCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto QoSDefinedPolicies;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto QoSDefinedPolicies;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "QoSUserDefinedPolicy_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }

      char prot[10];
      int  proto;
      char portrange[30];
      char sdport[10];
      char edport[10];
      int i;
      for (i=1; i<MAX_SYSCFG_ENTRIES ; i++) {
         proto = 0; // 0 is both, 1 is tcp, 2 is udp
         snprintf(query, sizeof(query), "protocol_%d", i);
         rc = syscfg_get(namespace, query, prot, sizeof(prot));
         if (0 != rc || '\0' == prot[0]) {
            proto = 0;
         } else if (0 == strcmp("tcp", prot)) {
            proto = 1;
         } else   if (0 == strcmp("udp", prot)) {
            proto = 2;
         }

         portrange[0]= '\0';
         sdport[0]   = '\0';
         edport[0]   = '\0';
         snprintf(query, sizeof(query), "port_range_%d", i);
         rc = syscfg_get(namespace, query, portrange, sizeof(portrange));
         if (0 != rc || '\0' == portrange[0]) {
            break;
         } else {
            int r = 0;
            if (2 != (r = sscanf(portrange, "%10s %10s", sdport, edport))) {
               if (1 == r) {
                  snprintf(edport, sizeof(edport), "%s", sdport);
               } else {
                  break;
               }
            }
         }

         char class[MAX_QUERY];
         class[0] = '\0';
         snprintf(query, sizeof(query), "class_%d", i);
         rc = syscfg_get(namespace, query, class, sizeof(class));
         if (0 != rc || '\0' == class[0]) {
            break;
         }

         char rule[MAX_QUERY];
         char subst[MAX_QUERY];
         if (0 == proto || 1 ==  proto) {
            snprintf(rule, sizeof(rule), 
                      "-A prerouting_qos -p tcp -m tcp --dport %s:%s -j DSCP --set-dscp-class %s",
                      sdport, edport, class);
            char  str[MAX_QUERY];
            snprintf(str, sizeof(str),
                     "%s", make_substitutions(rule, subst, sizeof(subst)));
            fprintf(fp, "%s\n", str);

         }

         if (0 == proto || 2 ==  proto) {
            snprintf(rule, sizeof(rule), 
                      "-A prerouting_qos -p udp -m udp --dport %s:%s -j DSCP --set-dscp-class %s",
                      sdport, edport, class);
            char  str[MAX_QUERY];
            snprintf(str, sizeof(str),
                     "%s", make_substitutions(rule, subst, sizeof(subst)));
            fprintf(fp, "%s\n", str);
         }
      }
   }

QoSDefinedPolicies:
   /*
    * syscfg tuple QoSDefinedPolicy_x, where x is a digit
    * keeps track of the names of policies which are defined
    * in an external file with format
    *    name | user friendly name | type | iptables hook | match criterea |
    *  eg foo | Foo Tool and Game | game | PREROUTING | -p tcp --destination-port 666 |
    */
   query[0] = '\0';
   rc = syscfg_get(NULL, "QoSDefinedPolicyCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto QoSMacAddrs;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto QoSMacAddrs;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }
   char *filename = qos_classification_file_dir"/"qos_classification_file;
   FILE *qos_fp = fopen(filename, "r"); 
   if (NULL != qos_fp) {
      for (idx=1 ; idx<=count ; idx++) {
         namespace[0] = '\0';
         snprintf(query, sizeof(query), "QoSDefinedPolicy_%d", idx);
         rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
         if (0 != rc || '\0' == namespace[0]) {
           continue; 
         }

         char name[MAX_QUERY];
         char class[MAX_QUERY];
      
         name[0] = '\0';
         class[0] = '\0';
         rc = syscfg_get(namespace, "name", name, sizeof(name));
         if (0 != rc || '\0' == name[0]) {
            break;
         } else {
            rc = syscfg_get(namespace, "class", class, sizeof(class));
            if (0 != rc || '\0' == class[0]) {
               break;
            } else {
               char subst[MAX_QUERY];
               write_qos_classification_statement(fp, qos_fp, name, make_substitutions(class, subst, sizeof(subst))); 
           }
         }
      }
      fclose(qos_fp);
   }

QoSMacAddrs:
   query[0] = '\0';
   rc = syscfg_get(NULL, "QoSMacAddrCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto QoSVoiceDevices;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto QoSVoiceDevices;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "QoSMacAddr_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }

      char mac[MAX_QUERY];
      char class[MAX_QUERY];
      mac[0]   = '\0';
      class[0] = '\0';
      rc = syscfg_get(namespace, "mac", mac, sizeof(mac));
      if (0 != rc || '\0' == mac[0]) {
         break;
      } else {
         rc = syscfg_get(namespace, "class", class, sizeof(class));
         if (0 != rc || '\0' == class[0]) {
            break;
         } else {
            char subst[MAX_QUERY];
            char str[MAX_QUERY];
            snprintf(str, sizeof(str),
                     "-A prerouting_qos -m mac --mac-source %s -j DSCP --set-dscp-class %s", 
                     mac, make_substitutions(class, subst, sizeof(subst)));
            fprintf(fp, "%s\n", str);
        }
      }
   }

QoSVoiceDevices:
   query[0] = '\0';
   rc = syscfg_get(NULL, "QoSVoiceDeviceCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto QoSDone;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto QoSDone;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "QoSVoiceDevice_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }

      char mac[MAX_QUERY];
      char class[MAX_QUERY];
      mac[0]   = '\0';
      class[0] = '\0';
      rc = syscfg_get(namespace, "mac", mac, sizeof(mac));
      if (0 != rc || '\0' == mac[0]) {
         break;
      } else {
         rc = syscfg_get(namespace, "class", class, sizeof(class));
         if (0 != rc || '\0' == class[0]) {
            break;
         } else {
            char subst[MAX_QUERY];
            char str[MAX_QUERY];
            snprintf(str, sizeof(str),
                     "-A prerouting_qos -m mac --mac-source %s -j DSCP --set-dscp-class %s", 
                     mac,  make_substitutions(class, subst, sizeof(subst)));
            fprintf(fp, "%s\n", str);
        }
      }
   }

QoSDone:
           FIREWALL_DEBUG("Exiting add_qos_marking_statements\n");       
   return(0);
}

/*
 =================================================================
            Misc Postrouting NAT 
 =================================================================
 */

/*
 *  Procedure     : do_nat_ephemeral
 *  Purpose       : prepare the iptables-restore statements for nat statements gleaned from the sysevent
 *                  NatFirewallRule pool
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 *  Notes         : These rules will be placed into the iptables nat table, and use target
 *                     prerouting_ephemeral for PREROUTING statements, or
 *                     postrouting_ephemeral for POSTROUTING statements
 */
static int do_nat_ephemeral(FILE *fp)
{

   unsigned  int iterator;
   char      name[MAX_QUERY];
   char      in_rule[MAX_QUERY];
   char      subst[MAX_QUERY];
   char      str[MAX_QUERY];
           FIREWALL_DEBUG("Entering do_nat_ephemeral\n");       
   iterator = SYSEVENT_NULL_ITERATOR;
   do {
      name[0] = subst[0] = '\0';
      memset(in_rule, 0, sizeof(in_rule));
      sysevent_get_unique(sysevent_fd, sysevent_token,
                          "NatFirewallRule", &iterator,
                          name, sizeof(name), in_rule, sizeof(in_rule));
      if ('\0' != in_rule[0]) {
         /*
          * the rule we just got could contain variables that we need to substitute
          * for runtime/configuration values
          */
        if (NULL != make_substitutions(in_rule, subst, sizeof(subst))) {
           if (1 == substitute(subst, str, sizeof(str), "PREROUTING", "prerouting_ephemeral") ||
              (1 == substitute(subst, str, sizeof(str), "POSTROUTING", "postrouting_ephemeral")) ) {
              fprintf(fp, "%s\n", str);
           }
        }
      }
   } while (SYSEVENT_NULL_ITERATOR != iterator);
           FIREWALL_DEBUG("Exiting do_nat_ephemeral\n");       
   return(0);
}

/*
 *  Procedure     : do_wan_nat_lan_clients
 *  Purpose       : prepare the iptables-restore statements for natting the outgoing packets from lan
 *                  to the filter table 
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int do_wan_nat_lan_clients(FILE *fp)
{
   if (!isNatReady) {
      return(0);
   }
  char str[MAX_QUERY];
           FIREWALL_DEBUG("Entering do_wan_nat_lan_clients\n");       
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
  //do not do SNAT on public ip
  int i;
  for(i = 0; i < StaticIPSubnetNum ;i++ ){
    snprintf(str, sizeof(str), 
              "-A postrouting_towan -s %s/%s -j RETURN", StaticIPSubnet[i].ip, StaticIPSubnet[i].mask);
    fprintf(fp, "%s\n",str);
  }
  //do not do SNAT if packet is come from erouter0
  snprintf(str, sizeof(str), 
           "-A postrouting_towan -s %s -j RETURN", current_wan_ipaddr);
  fprintf(fp, "%s\n",str); 
#endif 
#if defined(_COSA_BCM_MIPS_)
  if(!isBridgeMode) {       
#endif
  snprintf(str, sizeof(str),
           "-A postrouting_towan  -j SNAT --to-source %s", natip4);
#if defined(_COSA_BCM_MIPS_)
  }
#endif
  fprintf(fp, "%s\n", str);
  
   if (isCacheActive) {
      char rule[MAX_QUERY];
      snprintf(rule, sizeof(rule),
               "-A PREROUTING -i %s -p tcp --dport 80 -j DNAT --to %s:%s", lan_ifname, lan_ipaddr, "3128");
      fprintf(fp, "%s\n", rule);
   }
           FIREWALL_DEBUG("Exiting do_wan_nat_lan_clients\n");       
   return(0);
}

/*
 ==========================================================================
                     lan2self
 ==========================================================================
 */
/*
 *  Procedure     : do_lan2self_attack
 *  Purpose       : detect attacks from the lan side
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */
static int do_lan2self_attack(FILE *fp)
{
   /* LAND ATTACK */
   char str[MAX_QUERY];
// TODO: Add for each lan ip
           FIREWALL_DEBUG("Entering do_lan2self_attack\n");       
 snprintf(str, sizeof(str),
            "-A lanattack -s %s -d %s -j xlog_drop_lanattack", lan_ipaddr, lan_ipaddr);
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A lanattack -s 127.0.0.1 -j xlog_drop_lanattack");
   fprintf(fp, "%s\n", str);

   snprintf(str, sizeof(str),
            "-A lanattack -d 127.0.0.1 -j xlog_drop_lanattack");
   fprintf(fp, "%s\n", str);
           FIREWALL_DEBUG("Exiting do_lan2self_attack\n");       
   return(0);
}

/*
 * enable / disable telnet and ssh from lan side
 */
static int lan_telnet_ssh(FILE *fp, int family)
{
   int rc;
   char query[MAX_QUERY];
   query[0] = '\0';
           FIREWALL_DEBUG("Entering lan_telnet_ssh\n");     
   //telnet access control for lan side
   memset(query, 0, MAX_QUERY);
   rc = syscfg_get(NULL, "mgmt_lan_telnetaccess", query, sizeof(query));
   if (rc != 0 || (rc == 0 && '\0' != query[0] && 0 == strncmp(query, "0", sizeof(query))) ) {

       if(family == AF_INET6) {
           if(!isBridgeMode) //brlan0 exists
               fprintf(fp, "-A %s -i %s -p tcp --dport 23 -j DROP\n", "INPUT", lan_ifname);

           fprintf(fp, "-A %s -i %s -p tcp --dport 23 -j DROP\n", "INPUT", cmdiag_ifname); //lan0 always exist
       }
       else {
           fprintf(fp, "-A %s -p tcp --dport 23 -j DROP\n", "lan2self_mgmt");
       }

   }
   else if(family == AF_INET && isFirewallEnabled && !isBridgeMode && isWanServiceReady){ //only valid in router mode when wan is ready
       fprintf(fp, "-I %s -i %s -p tcp --dport 23 -j ACCEPT\n", "general_input", cmdiag_ifname);
   }

   //ssh access control for lan side
   memset(query, 0, MAX_QUERY);
   rc = syscfg_get(NULL, "mgmt_lan_sshaccess", query, sizeof(query));
   if (rc != 0 || (rc == 0 && '\0' != query[0] && 0 == strncmp(query, "0", sizeof(query))) ) {

       if(family == AF_INET6) {
           if(!isBridgeMode) //brlan0 exists
               fprintf(fp, "-A %s -i %s -p tcp --dport 22 -j DROP\n", "INPUT", lan_ifname);

           fprintf(fp, "-A %s -i %s -p tcp --dport 22 -j DROP\n", "INPUT", cmdiag_ifname); //lan0 always exist
       }
       else {
           fprintf(fp, "-A %s -p tcp --dport 22 -j DROP\n", "lan2self_mgmt");
       }
   }
           FIREWALL_DEBUG("Exiting lan_telnet_ssh\n");     
   return 0;
}

static int do_lan2self_by_wanip6(FILE *filter_fp)
{
           FIREWALL_DEBUG("Entering do_lan2self_by_wanip6\n");     
    int i;
    for(i = 0; i < ecm_wan_ipv6_num; i++){
        fprintf(filter_fp, "-A INPUT -i %s -d %s -p tcp --match multiport --dports 23,22,80,443,161 -j LOG_INPUT_DROP\n", lan_ifname, ecm_wan_ipv6[i]);
    }
           FIREWALL_DEBUG("Exiting do_lan2self_by_wanip6\n");     
}

static int do_lan2self_by_wanip(FILE *filter_fp, int family)
{
   //As requested, we don't allow SNMP/HTTP/HTTPs/Ping
   char httpport[64], tmpQuery[64];
   char httpsport[64];
   char port[64];
   int rc = 0, ret;
   httpport[0] = '\0';
   httpsport[0] = '\0';
           FIREWALL_DEBUG("Entering do_lan2self_by_wanip\n");     
   fprintf(filter_fp, "-A lan2self_by_wanip -d %s -j RETURN\n", current_wan_ipaddr); //eRouter address doesn't have any restrictions
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
   if(isWanStaticIPReady){
         int i;
//       fprintf(filter_fp, "-A lan2self_by_wanip -d %s -j RETURN \n", current_wan_static_ipaddr); //true static ip doesn't have any restrictions
         for(i = 0; i < StaticIPSubnetNum ;i++ )
            fprintf(filter_fp, "-A lan2self_by_wanip -d %s -j RETURN \n", StaticIPSubnet[i].ip); //true static ip doesn't have any restrictions
   }
#endif
   //Setting wan_mgmt_httpport/httpsport to 12368 will block ATOM dbus connection. Add exception to avoid this situation.
   // TODO: REMOVE THIS EXCEPTION SINCE DBUS WILL BE ON PRIVATE NETWORK
   fprintf(filter_fp, "-A lan2self_by_wanip -s 192.168.100.3 -d 192.168.100.1 -j RETURN\n");

   rc = syscfg_get(NULL, "mgmt_wan_httpport", httpport, sizeof(httpport));
#if defined(CONFIG_CCSP_WAN_MGMT_PORT)
   tmpQuery[0] = '\0';
   ret = syscfg_get(NULL, "mgmt_wan_httpport_ert", tmpQuery, sizeof(tmpQuery));
   if(ret == 0)
       strcpy(httpport, tmpQuery);
#endif

   if (rc == 0 && httpport[0] != '\0' && atoi(httpport) != 80 && (atoi(httpport) >= 0 && atoi(httpport) <= 65535 ))
       fprintf(filter_fp, "-A lan2self_by_wanip -p tcp --dport %s -j xlog_drop_lan2self\n", httpport); //GUI on mgmt_wan port

   rc = syscfg_get(NULL, "mgmt_wan_httpsport", httpsport, sizeof(httpsport));
   if (rc == 0 && httpsport[0] != '\0' && atoi(httpsport) != 443 && (atoi(httpsport) >= 0 && atoi(httpsport) <= 65535 ))
       fprintf(filter_fp, "-A lan2self_by_wanip -p tcp --dport %s -j xlog_drop_lan2self\n", httpsport); //GUI on mgmt_wan port

   fprintf(filter_fp, "-A lan2self_by_wanip -p tcp -m multiport --dports 80,443 -j xlog_drop_lan2self\n"); //GUI on standard ports
   fprintf(filter_fp, "-A lan2self_by_wanip -p udp --dport 161 -j xlog_drop_lan2self\n"); //SNMP
   fprintf(filter_fp, "-A lan2self_by_wanip -p icmp --icmp-type 8 -j xlog_drop_lan2self\n"); // ICMP PING request
           FIREWALL_DEBUG("Exiting do_lan2self_by_wanip\n");     
}
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
/*
 *  Procedure     : do_lan2wan_staticip
 *  Purpose       : allow or deny true static subnet  access
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */
static void do_lan2wan_staticip(FILE *filter_fp){
    int i;
           FIREWALL_DEBUG("Entering do_lan2wan_staticip\n");     
    if(isWanStaticIPReady && isFWTS_enable ){
        for(i = 0; i < StaticIPSubnetNum; i++){
            fprintf(filter_fp, "-A lan2wan_staticip -s %s/%s -j ACCEPT\n", StaticIPSubnet[i].ip, StaticIPSubnet[i].mask);
        }
    }
           FIREWALL_DEBUG("Exiting do_lan2wan_staticip\n");     
}
#endif
/*
 * Disable LAN http access while CmWebAccessUserIfLevel.all-users.lan = off(0)
 */
void lan_http_access(FILE *fp) {
    char lan_ip_webaccess[2];
           FIREWALL_DEBUG("Entering lan_http_access\n");     
    if (0 == sysevent_get(sysevent_fd, sysevent_token, "lan_ip_webaccess", lan_ip_webaccess, sizeof(lan_ip_webaccess))) {
       if(lan_ip_webaccess[0]!='\0' && strcmp(lan_ip_webaccess, "0")==0) {          
           if(!isBridgeMode) //brlan0 exists
               fprintf(fp, "-A %s -i %s -p tcp --dport %s -j xlog_drop_lan2self\n", "lan2self_mgmt", lan_ifname, reserved_mgmt_port);

           fprintf(fp, "-A %s -i %s -p tcp --dport %s -j xlog_drop_lan2self\n", "lan2self_mgmt", cmdiag_ifname, reserved_mgmt_port); //lan0 always exist
       }
   } 
           FIREWALL_DEBUG("Exiting lan_http_access\n");     
}
/*
 *  Procedure     : do_lan2self_mgmt
 *  Purpose       : allow or deny local access
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */
static int do_lan2self_mgmt(FILE *fp)
{
   int rc;
   char query[MAX_QUERY];
         //  FIREWALL_DEBUG("Entering do_lan2self_mgmt\n");     
 query[0] = '\0';
   rc = syscfg_get(NULL, "mgmt_wifi_access", query, sizeof(query));
   if (0 == rc && '\0' != query[0] && 0 == strncmp(query, "0", sizeof(query)) ) {
      /* disallow wifi access */
      query[0] = '\0';
      rc = syscfg_get(NULL, "lan_wl_physical_ifnames", query, sizeof(query));
      if (0 == rc && '\0' != query[0]) {
         char *q = trim(query);
         char *next_token;
         while (NULL != q)  {
            next_token = token_get(q, ' ');
            if ( 0 != strcmp(q, "") ) {
                char str[MAX_QUERY];
                snprintf(str, sizeof(str), /* TODO: is this used / accurate? */
                        "-A lan2self_mgmt -p tcp  -m tcp --dport %s -m physdev --physdev-in %s -j DROP",  reserved_mgmt_port, q);
               fprintf(fp, "%s\n", str);

            }
            q = next_token;
         } 
      }
   }

   lan_telnet_ssh(fp, AF_INET);

#if defined(CONFIG_CCSP_LAN_HTTP_ACCESS)
   lan_http_access(fp);
#endif
          // FIREWALL_DEBUG("Exiting do_lan2self_mgmt\n");     
   return(0);
}
 
/*
 *  Procedure     : do_lan2self
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic 
 *                  from the lan to utopia
 *  Parameters    :
 *    fp             : An open file to write lan2self rules to
 * Return Values  :
 *    0              : Success
 *
 */
static int do_lan2self(FILE *fp)
{
        // FIREWALL_DEBUG("Entering do_lan2self\n");     
   if(isWanReady)
       do_lan2self_by_wanip(fp, AF_INET);

   do_lan2self_attack(fp);
   do_lan2self_mgmt(fp);
        // FIREWALL_DEBUG("Exiting do_lan2self\n");     
   return(0);
}

/*
 ==========================================================================
                     wan2self
 ==========================================================================
 */

/*
 *  Procedure     : do_wan2self_attack
 *  Purpose       : prepare the iptables-restore statements with
 *                  counter measures against well known attacks
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *  Note          : wanattack table is immediately followed by a rule to drop all packets.
 *                  Therefore the only reason to detect wan attacks is for security logging purposes.
 *                  If another subtable is inserted between wanattack and drop all, then you MUST
 *                  enable wanattack for all packets so that you have a chance at avoiding attacks
 *                  before interpreting the packets.
 */
static int do_wan2self_attack(FILE *fp)
{
   if (!isLogEnabled) { 
      return(0);
   }

   char str[MAX_QUERY];
   char *logRateLimit = "-m limit --limit 6/h --limit-burst 1";
        // FIREWALL_DEBUG("Entering do_wan2self_attack\n");     
   /*
    * Log probable DoS attack
    */
   //Smurf attack, actually the below rules are to prevent us from being the middle-man host
   fprintf(fp, "-A wanattack -p icmp -m icmp --icmp-type address-mask-request %s -j ULOG --ulog-prefix \"DoS Attack - Smurf Attack\" --ulog-cprange 50\n", logRateLimit);
   fprintf(fp, "-A wanattack -p icmp -m icmp --icmp-type address-mask-request -j xlog_drop_wanattack\n");
   fprintf(fp, "-A wanattack -p icmp -m icmp --icmp-type timestamp-request %s -j ULOG --ulog-prefix \"DoS Attack - Smurf Attack\" --ulog-cprange 50\n", logRateLimit);
   fprintf(fp, "-A wanattack -p icmp -m icmp --icmp-type timestamp-request -j xlog_drop_wanattack\n");

   //ICMP Flooding. Mark traffic bit rate > 5/s as attack and limit 6 log entries per hour
   fprintf(fp, "-A wanattack -p icmp -m limit --limit 5/s --limit-burst 10 -j RETURN\n"); //stop traveling the rest of the wanattack chain
   fprintf(fp, "-A wanattack -p icmp %s -j ULOG --ulog-prefix \"DoS Attack - ICMP Flooding\" --ulog-cprange 50\n", logRateLimit);
   fprintf(fp, "-A wanattack -p icmp -j xlog_drop_wanattack\n");

   //TCP SYN Flooding
   fprintf(fp, "-A wanattack -p tcp --syn -m limit --limit 10/s --limit-burst 20 -j RETURN\n");
   fprintf(fp, "-A wanattack -p tcp --syn %s -j ULOG --ulog-prefix \"DoS Attack - TCP SYN Flooding\" --ulog-cprange 50\n", logRateLimit);
   fprintf(fp, "-A wanattack -p tcp --syn -j xlog_drop_wanattack\n");

   //LAND Aattack - sending a spoofed TCP SYN pkt with the target host's IP address to an open port as both source and destination
   if(isWanReady) {
       /* Allow multicast packet through */
       fprintf(fp, "-A wanattack -p udp -s %s -d 224.0.0.0/8 -j RETURN\n", current_wan_ipaddr);
       fprintf(fp, "-A wanattack -s %s %s -j ULOG --ulog-prefix \"DoS Attack - LAND Attack\" --ulog-cprange 50\n", current_wan_ipaddr, logRateLimit);
       fprintf(fp, "-A wanattack -s %s -j xlog_drop_wanattack\n", current_wan_ipaddr);
   }

   /*
    * Reject packets from RFC1918 class networks (i.e., spoofed)
    */
   if (isRFC1918Blocked) {
      snprintf(str, sizeof(str),
               "-A wanattack -s 10.0.0.0/8 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -s 169.254.0.0/16 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -s 172.16.0.0/12 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -s 192.168.0.0/16 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -i ! lo -s 127.0.0.0/8 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -s 224.0.0.0/4 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -d 224.0.0.0/4 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -s 240.0.0.0/5 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -d 240.0.0.0/5 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -s 0.0.0.0/8 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -d 0.0.0.0/8 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -d 239.255.255.0/24 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack -d 255.255.255.255  -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);
   }

   /*
    * TCP reset attack
    *  Drop excessive RST packets to avoid SMURF attacks, by given the
    *  next real data packet in the sequence a better chance to arrive first.
    */
   snprintf(str, sizeof(str),
            "-A wanattack -p tcp -m tcp --tcp-flags RST RST -m limit --limit 2/second --limit-burst 2 -j xlog_accept_wan2lan");
   fprintf(fp, "%s\n", str);

   /*
    * SYN Flood
    * Protect against SYN floods by rate limiting the number of new
    * connections from any host to 60 per second.  This does *not* do rate
    * limiting overall, because then someone could easily shut us down by
    * saturating the limit.
    */
    //snprintf(str, sizeof(str), "-A wanattack -m state --state NEW -p tcp -m tcp --syn -m recent --name synflood --set");
    //fprintf(fp, "%s\n", str);
    //snprintf(str, sizeof(str), "-A wanattack -m state --state NEW -p tcp -m tcp --syn -m recent --name synflood --update --seconds 1 --hitcount 60 -j xlog_drop_wanattack");
    //fprintf(fp, "%s\n", str);

    // since some wan protocols have an ip address for a connection to the isp
    // and a different one for the wan itself. We make sure to protect the isp
    // connection as well 
    char isp_connection[MAX_QUERY];
    isp_connection[0] = '\0';
    sysevent_get(sysevent_fd, sysevent_token, "ipv4_wan_ipaddr", isp_connection, sizeof(isp_connection));
    if ('\0' != isp_connection[0] && 
        0 != strcmp("0.0.0.0", isp_connection) && 
        0 != strcmp(isp_connection, current_wan_ipaddr)) {
       snprintf(str, sizeof(str),
                "-A wanattack -s %s -j xlog_drop_wanattack", isp_connection);
       fprintf(fp, "%s\n", str);
    }

    snprintf(str, sizeof(str),
             "-A wanattack -s %s -j xlog_drop_wanattack", lan_ipaddr);
    fprintf(fp, "%s\n", str);

    snprintf(str, sizeof(str),
             "-A wanattack -d %s -j xlog_drop_wanattack", lan_ipaddr);
    fprintf(fp, "%s\n", str);

    snprintf(str, sizeof(str),
             "-A wanattack -s 127.0.0.1 -j xlog_drop_wanattack");
    fprintf(fp, "%s\n", str);

    snprintf(str, sizeof(str),
             "-A wanattack -d 127.0.0.1 -j xlog_drop_wanattack");
    fprintf(fp, "%s\n", str);

   /*
    * Port Scanning
    * This technique relies on looking for connection attempts to port 139 (Windows File Sharing)
    * so it is unsuitable for situations where we want windows file sharing on the wan side.
    */
   if (isPortscanDetectionEnabled) {

      // Anyone who tried to portscan us is locked out for an entire day.
      snprintf(str, sizeof(str),
               "-A wanattack -m recent --name portscan --rcheck --seconds 86400 -j xlog_drop_wanattack");
      fprintf(fp, "%s\n", str);


      // Once the day has passed, remove them from the portscan list
      snprintf(str, sizeof(str),
               "-A wanattack   -m recent --name portscan --remove");
      fprintf(fp, "%s\n", str);


      // These rules add scanners to the portscan list, and log the attempt.
      snprintf(str, sizeof(str),
               "-A wanattack  -i %s -p tcp -m tcp --dport 139  -m recent --name portscan --set -j LOG --log-prefix \"Portscan:\" -m limit --limit 1/minute --limit-burst 1", current_wan_ifname);
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A wanattack  -i %s -p tcp -m tcp --dport 139 -m recent --name portscan --set -j xlog_drop_wanattack", current_wan_ifname);
      fprintf(fp, "%s\n", str);

   }
        // FIREWALL_DEBUG("Exiting do_wan2self_attack\n");     
   return(0);
}

/*
 *  Procedure     : do_mgmt_override
 *  Purpose       : ensure that mgmt port access cannot be taken by forward/dmz
 *  Parameters    :
 *     nat_fp
 *  Return Values :
 *     0               : done
 */
static int do_mgmt_override(FILE *nat_fp)
{
   char str[MAX_QUERY];
       //  FIREWALL_DEBUG("Entering do_mgmt_override\n");     
  snprintf(str, sizeof(str),
            "-I prerouting_mgmt_override 1 -s %s/%s -d %s -p tcp  -m tcp --dport %s -j ACCEPT",
              lan_ipaddr, lan_netmask, lan_ipaddr, reserved_mgmt_port);
   fprintf(nat_fp, "%s\n", str);
        // FIREWALL_DEBUG("Exiting do_mgmt_override\n");     
   return(0);
}

/*
 *  Procedure     : remote_access_set_proto
 *  Purpose       : allow or deny remote access
 *  Parameters    :
 *     nat_fp
 *     filter_fp
 *  Return Values :
 *     0               : done
 */

static int remote_access_set_proto(FILE *filt_fp, FILE *nat_fp, const char *port, const char *src, int family, const char *interface)
{
         FIREWALL_DEBUG("Entering remote_access_set_proto\n");    
    if (family == AF_INET) {
        fprintf(filt_fp, "-A wan2self_mgmt -i %s %s -p tcp -m tcp --dport %s -j ACCEPT\n", interface, src, port);
    } else {
        fprintf(filt_fp, "-A INPUT -i %s %s -p tcp -m tcp --dport %s -j ACCEPT\n", interface, src, port);
    }
         FIREWALL_DEBUG("Exiting remote_access_set_proto\n");    
    return 0;
}

#define IPRANGE_UTKEY_PREFIX "mgmt_wan_iprange_"
static int do_remote_access_control(FILE *nat_fp, FILE *filter_fp, int family)
{
    int rc, ret;
    char query[MAX_QUERY], tmpQuery[MAX_QUERY];
    char srcaddr[MAX_QUERY];
    char iprangeAddr[REMOTE_ACCESS_IP_RANGE_MAX_RULE][MAX_QUERY] = {'\0'};
    unsigned long count, i;
    char countStr[16];
    char startip[64];
    char endip[64];
    char httpport[64];
    char httpsport[64];
    char port[64];
    char utKey[64];
    unsigned char srcany = 0, validEntry = 0, noIPv6Entry = 0;
    char cm_ip_webaccess[2];
    char rg_ip_webaccess[2];
         FIREWALL_DEBUG("Entering do_remote_access_control\n");    
    httpport[0] = '\0';
    httpsport[0] = '\0';
    cm_ip_webaccess[0] = '\0';
    rg_ip_webaccess[0] = '\0';

    /* global flag */
    rc = syscfg_get(NULL, "mgmt_wan_access", query, sizeof(query));
    if (rc != 0 || atoi(query) != 1) {
        return 0;
    }
    
    srcaddr[0] = '\0';

#if defined(CONFIG_CCSP_CM_IP_WEBACCESS)
#if defined(_COSA_BCM_MIPS_)
    // XF3 only has this interface available on IPv6 erouter0
    if (family == AF_INET6)
    {
#endif
       remote_access_set_proto(filter_fp, nat_fp, "80", srcaddr, family, ecm_wan_ifname);
       remote_access_set_proto(filter_fp, nat_fp, "443", srcaddr, family, ecm_wan_ifname);
#if defined(_COSA_BCM_MIPS_)
    }
#endif
#endif


    rc = syscfg_get(NULL, IPRANGE_UTKEY_PREFIX"count", countStr, sizeof(countStr));
    if(rc == 0)
        count = strtoul(countStr, NULL, 10);
    else
        count = 0;

    rc = syscfg_get(NULL, "mgmt_wan_srcany", query, sizeof(query));

    if (rc == 0 && (srcany = atoi(query)) == 1)
        ;
    else {
        if (family == AF_INET) {
            //get iprange src IP address first
            for(i = 0; i < count; i++) {
                snprintf(utKey, sizeof(utKey), IPRANGE_UTKEY_PREFIX"%u_startIP", i);
                syscfg_get(NULL, utKey, startip, sizeof(startip));
                snprintf(utKey, sizeof(utKey), IPRANGE_UTKEY_PREFIX"%u_endIP", i);
                syscfg_get(NULL, utKey, endip, sizeof(endip));

                if (strcmp(startip, endip) == 0)
                    snprintf(iprangeAddr[i], sizeof(iprangeAddr[i]), "-s %s", startip);
                else
                    snprintf(iprangeAddr[i], sizeof(iprangeAddr[i]), "-m iprange --src-range %s-%s", startip, endip);
            }

            //this is the remote access IP address on GUI
            rc = syscfg_get(NULL, "mgmt_wan_srcstart_ip", startip, sizeof(startip));
            rc |= syscfg_get(NULL, "mgmt_wan_srcend_ip", endip, sizeof(endip));
            if (rc != 0)
                fprintf(stderr, "[REMOTE ACCESS] fail to get mgmt ip address\n");
            else {
                if (strcmp(startip, endip) == 0) {
                    snprintf(srcaddr, sizeof(srcaddr), "-s %s", startip);
                } else {
                    snprintf(srcaddr, sizeof(srcaddr), "-m iprange --src-range %s-%s", startip, endip);
                }
            }
        }
        else {
            startip[0] = endip[0] = '\0';
            rc = syscfg_get(NULL, "mgmt_wan_srcstart_ipv6", startip, sizeof(startip));
            rc |= syscfg_get(NULL, "mgmt_wan_srcend_ipv6", endip, sizeof(endip));
            if (rc != 0 || startip[0] == '\0' || endip[0] == '\0') {
                noIPv6Entry = 1;
            }
            else {
                if (strcmp(startip, endip) == 0) {
                    snprintf(srcaddr, sizeof(srcaddr), "-s %s", startip);
                } else {
                    snprintf(srcaddr, sizeof(srcaddr), "-m iprange --src-range %s-%s", startip, endip);
                }
            }
        }
    }

   // 255.255.255.255 or x to cover the use case of leave either IPv4 or IPv6 address blank on GUI
   if( srcany == 1 
        || (family == AF_INET && strcmp("255.255.255.255", startip) != 0 && strcmp("255.255.255.255", endip) != 0) \
        || (family == AF_INET6 && strcmp("x", startip) != 0 && strcmp("x", endip) != 0 && noIPv6Entry != 1) \
     ) {
       validEntry = 1;
   }

   /* HTTP access */
   // CM-IP: wan0
#if defined(CONFIG_CCSP_CM_IP_WEBACCESS)
/*
   if(validEntry)
       remote_access_set_proto(filter_fp, nat_fp, "80", srcaddr, family, ecm_wan_ifname);

   for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
       remote_access_set_proto(filter_fp, nat_fp, "80", iprangeAddr[i], family, ecm_wan_ifname);
*/
#else
   if (0 == sysevent_get(sysevent_fd, sysevent_token, "cm_ip_webaccess", cm_ip_webaccess, sizeof(cm_ip_webaccess))) {
       if(cm_ip_webaccess[0]!='\0' && strcmp(cm_ip_webaccess, "1")==0) {
           if(validEntry)
               remote_access_set_proto(filter_fp, nat_fp, "80", srcaddr, family, ecm_wan_ifname);

           for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
               remote_access_set_proto(filter_fp, nat_fp, "80", iprangeAddr[i], family, ecm_wan_ifname);
       }
   }
#endif
   // RG-IP: erouter0
#if defined(CONFIG_CCSP_WAN_MGMT)
   rc = syscfg_get(NULL, "mgmt_wan_httpaccess", query, sizeof(query));
#if defined(CONFIG_CCSP_WAN_MGMT_ACCESS)
   tmpQuery[0] = '\0';
   ret = syscfg_get(NULL, "mgmt_wan_httpaccess_ert", tmpQuery, sizeof(tmpQuery));
   if(ret == 0)
       strcpy(query, tmpQuery);
#endif

   rc |= syscfg_get(NULL, "mgmt_wan_httpport", httpport, sizeof(httpport));
#if defined(CONFIG_CCSP_WAN_MGMT_PORT)
   tmpQuery[0] = '\0';
   ret = syscfg_get(NULL, "mgmt_wan_httpport_ert", tmpQuery, sizeof(tmpQuery));
   if(ret == 0)
       strcpy(httpport, tmpQuery);
#endif

   if (rc == 0 && atoi(query) == 1)
   {
       // allows remote GUI access on erouter0 interface
       if(validEntry)
           remote_access_set_proto(filter_fp, nat_fp, httpport, srcaddr, family, current_wan_ifname);

       for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
           remote_access_set_proto(filter_fp, nat_fp, httpport, iprangeAddr[i], family, current_wan_ifname);
   }
#else
   rc = syscfg_get(NULL, "mgmt_wan_httpport", httpport, sizeof(httpport));
   if(rc == 0) {
       if (0 == sysevent_get(sysevent_fd, sysevent_token, "rg_ip_webaccess", rg_ip_webaccess, sizeof(rg_ip_webaccess))) {
           if(rg_ip_webaccess[0]!='\0' && strcmp(rg_ip_webaccess, "1")==0) {
              if(validEntry)
                  remote_access_set_proto(filter_fp, nat_fp, httpport, srcaddr, family, current_wan_ifname);

              for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
                  remote_access_set_proto(filter_fp, nat_fp, httpport, iprangeAddr[i], family, current_wan_ifname);
           }
       }
   }   
#endif

   /* HTTPS access */
   // CM-IP: wan0
#if defined(CONFIG_CCSP_CM_IP_WEBACCESS)
/*
   if(validEntry)
       remote_access_set_proto(filter_fp, nat_fp, "443", srcaddr, family, ecm_wan_ifname);

   for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
       remote_access_set_proto(filter_fp, nat_fp, "443", iprangeAddr[i], family, ecm_wan_ifname);
*/
#else
   if (0 == sysevent_get(sysevent_fd, sysevent_token, "cm_ip_webaccess", cm_ip_webaccess, sizeof(cm_ip_webaccess))) {
       if(cm_ip_webaccess[0]!='\0' && strcmp(cm_ip_webaccess, "1")==0) {
           if(validEntry)
               remote_access_set_proto(filter_fp, nat_fp, "443", srcaddr, family, ecm_wan_ifname);

           for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
               remote_access_set_proto(filter_fp, nat_fp, "443", iprangeAddr[i], family, ecm_wan_ifname);
       }
   }
#endif

   // RG-IP: erouter0
#if defined(CONFIG_CCSP_WAN_MGMT)
   rc = syscfg_get(NULL, "mgmt_wan_httpsaccess", query, sizeof(query));
   rc |= syscfg_get(NULL, "mgmt_wan_httpsport", httpsport, sizeof(httpsport));
   if (rc == 0 && atoi(query) == 1)
   {

       if(validEntry)
           remote_access_set_proto(filter_fp, nat_fp, httpsport, srcaddr, family, current_wan_ifname);

       for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
           remote_access_set_proto(filter_fp, nat_fp, httpsport, iprangeAddr[i], family, current_wan_ifname);
   }
#else
   rc = syscfg_get(NULL, "mgmt_wan_httpsport", httpsport, sizeof(httpsport));
   if(rc == 0) {
       if (0 == sysevent_get(sysevent_fd, sysevent_token, "rg_ip_webaccess", rg_ip_webaccess, sizeof(rg_ip_webaccess))) {
           if(rg_ip_webaccess[0]!='\0' && strcmp(rg_ip_webaccess, "1")==0) {
              if(validEntry)
                  remote_access_set_proto(filter_fp, nat_fp, httpsport, srcaddr, family, current_wan_ifname);

              for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
                  remote_access_set_proto(filter_fp, nat_fp, httpsport, iprangeAddr[i], family, current_wan_ifname);
           }
       }
   }  
#endif

   /* eCM SSH access */
   rc = syscfg_get(NULL, "mgmt_wan_sshaccess", query, sizeof(query));
   rc |= syscfg_get(NULL, "mgmt_wan_sshport", port, sizeof(port));
   if (rc == 0 && atoi(query) == 1) {
       if(validEntry)
           remote_access_set_proto(filter_fp, nat_fp, port, srcaddr, family, ecm_wan_ifname);

       for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
           remote_access_set_proto(filter_fp, nat_fp, port, iprangeAddr[i], family, ecm_wan_ifname);
   }

   /* eCM Telnet access */
   rc = syscfg_get(NULL, "mgmt_wan_telnetaccess", query, sizeof(query));
   rc |= syscfg_get(NULL, "mgmt_wan_telnetport", port, sizeof(port));
   if (rc == 0 && atoi(query) == 1) {
       if(validEntry)
           remote_access_set_proto(filter_fp, nat_fp, port, srcaddr, family, ecm_wan_ifname);

       for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
           remote_access_set_proto(filter_fp, nat_fp, port, iprangeAddr[i], family, ecm_wan_ifname);
   }

   /* eMTA SSH access */
   rc = syscfg_get(NULL, "mgmt_mta_sshaccess", query, sizeof(query));
   rc |= syscfg_get(NULL, "mgmt_wan_sshport", port, sizeof(port));
   if (rc == 0 && atoi(query) == 1) {
       if(validEntry)
           remote_access_set_proto(filter_fp, nat_fp, port, srcaddr, family, emta_wan_ifname);

       for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
           remote_access_set_proto(filter_fp, nat_fp, port, iprangeAddr[i], family, emta_wan_ifname);
   }

   /* eMTA Telnet access */
   rc = syscfg_get(NULL, "mgmt_mta_telnetaccess", query, sizeof(query));
   rc |= syscfg_get(NULL, "mgmt_wan_telnetport", port, sizeof(port));
   if (rc == 0 && atoi(query) == 1) {
       if(validEntry)
           remote_access_set_proto(filter_fp, nat_fp, port, srcaddr, family, emta_wan_ifname);

       for(i = 0; i < count && family == AF_INET && srcany == 0; i++)
           remote_access_set_proto(filter_fp, nat_fp, port, iprangeAddr[i], family, emta_wan_ifname);
   }

    if(httpport[0] == '\0' || atoi(httpport) < 0 || atoi(httpport) > 65535)
        strcpy(httpport, "80");
    if(httpsport[0] == '\0' || atoi(httpsport) < 0 || atoi(httpsport) > 65535)
        strcpy(httpsport, "443");

    if(strcmp(httpport, "80") != 0)
        strcat(httpport, ",80");
    if(strcmp(httpsport, "443") != 0)
        strcat(httpsport, ",443");

    //remote management is only available on eCM interface if it is enabled
    if(family == AF_INET)
        fprintf(filter_fp, "-A wan2self_mgmt -p tcp -m multiport --dports 23,%s,%s -j xlog_drop_wan2self\n", httpport, httpsport);
    else
        fprintf(filter_fp, "-A INPUT ! -i %s -p tcp -m multiport --dports 23,%s,%s -j DROP\n", isBridgeMode == 0 ? lan_ifname : cmdiag_ifname, httpport, httpsport);
         FIREWALL_DEBUG("Exiting do_remote_access_control\n");    
    return 0;
}

/*
 *  Procedure     : do_wan2self_ports
 *  Purpose       : allow or deny access to some ports
 *  Parameters    :
 *     mangle_fp     
 *     nat_fp     
 *     filter_fp
 *  Return Values :
 *     0               : done
 */
static int do_wan2self_ports(FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{

   char str[MAX_QUERY];
        // FIREWALL_DEBUG("Entering do_wan2self_ports\n");    
   // since connection tracking is turned of if current_wan_ipaddr = 0.0.0.0
   // we need to explicitly allow dns
   if (!isWanReady) {
      snprintf(str, sizeof(str), 
         "-A wan2self_ports -i %s -p udp -m udp --sport 53 -j xlog_accept_wan2self", default_wan_ifname);
      fprintf(filter_fp, "%s\n", str);
   }

   /*
    * if we are doing rip on the wan
    * if rip is enabled then we need to allow multicast udp port 520 for version2
    * as well as unicast
    */
   if (isRipWanEnabled) {
      snprintf(str, sizeof(str),
               "-A wan2self_ports -d 224.0.0.9 -p udp -m udp --dport 520 -j xlog_accept_wan2self");
      fprintf(filter_fp, "%s\n", str);
      if (isDmzEnabled) {
         snprintf(str, sizeof(str),
                  "-I prerouting_fromwan_todmz 1 -d 224.0.0.9 -p udp -m udp --dport 520 -j ACCEPT");
         fprintf(nat_fp, "%s\n", str);
      }

      snprintf(str, sizeof(str),
               "-A wan2self_ports -p udp -m udp --dport 520 -j xlog_accept_wan2self");
      fprintf(filter_fp, "%s\n", str);
      if (isDmzEnabled) {
         snprintf(str, sizeof(str),
                  "-I prerouting_fromwan_todmz 1 --dport 520  -j ACCEPT");
         fprintf(nat_fp, "%s\n", str);
      }

      // set QoS DSCP markings
     snprintf(str, sizeof(str),
              "-A postrouting_qos -p udp -m udp --dport 520 -j DSCP --set-dscp-class cs6");
     fprintf(mangle_fp, "%s\n", str);
   }

   /*
    * if the development override switch is enabled then allow ssh, http, https, from wan side
    */
   if (isDevelopmentOverride) {
      snprintf(str, sizeof(str),
               "-A wan2self_ports -p tcp -m tcp --dport 22 -j xlog_accept_wan2self");
      fprintf(filter_fp, "%s\n", str);
      if (isDmzEnabled) {
         snprintf(str, sizeof(str),
                  "-I prerouting_fromwan_todmz 1 -p tcp -m tcp --dport 22  -j ACCEPT");
         fprintf(nat_fp, "%s\n", str);
      }

      snprintf(str, sizeof(str),
               "-A wan2self_ports -p tcp -m tcp --dport 80 -j xlog_accept_wan2self");
      fprintf(filter_fp, "%s\n", str);
      if (isDmzEnabled) {
         snprintf(str, sizeof(str),
                  "-I prerouting_fromwan_todmz 1 -p tcp -m tcp --dport 80  -j ACCEPT");
         fprintf(nat_fp, "%s\n", str);
      }

      snprintf(str, sizeof(str),
               "-A wan2self_ports -p tcp -m tcp --dport 443 -j xlog_accept_wan2self");
      fprintf(filter_fp, "%s\n", str);
      if (isDmzEnabled) {
         snprintf(str, sizeof(str),
                  "-I prerouting_fromwan_todmz 1 -p tcp -m tcp --dport 443  -j ACCEPT");
         fprintf(nat_fp, "%s\n", str);
      }

   }

   if (strncasecmp(firewall_level, "High", strlen("High")) != 0)
   {
      if (strncasecmp(firewall_level, "Medium", strlen("Medium")) == 0)
      {
         fprintf(filter_fp, "-A wan2self_ports -p tcp --dport 113 -j xlog_drop_wan2self\n"); // IDENT
         fprintf(filter_fp, "-A wan2self_ports -p icmp --icmp-type 8 -j xlog_drop_wan2self\n"); // Drop ICMP PING
      }
      else if (strncasecmp(firewall_level, "Low", strlen("Low")) == 0)
      {
         fprintf(filter_fp, "-A wan2self_ports -p tcp --dport 113 -j xlog_drop_wan2self\n"); // IDENT
      #if defined(CONFIG_CCSP_DROP_ICMP_PING)
         fprintf(filter_fp, "-A wan2self_ports -p icmp --icmp-type 8 -j xlog_drop_wan2self\n"); // Drop ICMP PING
      #else
         fprintf(filter_fp, "-A wan2self_ports -p icmp --icmp-type 8 -m limit --limit 3/second -j xlog_accept_wan2self\n"); // Allow ICMP PING with limited rate
      #endif
      }
      else if (strncasecmp(firewall_level, "Custom", strlen("Custom")) == 0)
      {
         /*Since the WebGUI only indicates the wan2lan traffic should be blocked, wan2self http traffic should be controled by wan2self_mgmt
         if (isHttpBlocked)
         {
            fprintf(filter_fp, "-A wan2self_ports -p tcp --dport 80 -j xlog_drop_wan2self\n"); // HTTP
            fprintf(filter_fp, "-A wan2self_ports -p tcp --dport 443 -j xlog_drop_wan2self\n"); // HTTPs
         }
         */
         fprintf(filter_fp, "-A wan2self_ports -p tcp --dport 113 -j %s\n", isIdentBlocked ? "xlog_drop_wan2self" : "xlog_accept_wan2self"); // IDENT
         if(isPingBlocked) {
             fprintf(filter_fp, "-A wan2self_ports -p icmp --icmp-type 8 -j %s\n", "xlog_drop_wan2self"); // ICMP PING
         }
         else {
             fprintf(filter_fp, "-A wan2self_ports -p icmp --icmp-type 8 -m limit --limit 3/second -j %s\n", "xlog_accept_wan2self"); // ICMP PING
         }
      }
      else //None
      {
         fprintf(filter_fp, "-A wan2self_ports -p icmp --icmp-type 8 -m limit --limit 3/second -j xlog_accept_wan2self\n"); // ACCEPT ICMP PING if Firewall is disabled
      }

      // we still need to protect against other icmp besides ping
      fprintf(filter_fp, "-A wan2self_ports -p icmp -m limit --limit 1/second -j xlog_accept_wan2self\n");
      //rule for IGMP(protocol num is 2)
      fprintf(filter_fp, "-A wan2self_ports -p 2 -j %s\n", "xlog_accept_wan2self");
   }
   else //High Level
   {
       fprintf(filter_fp, "-A wan2self_ports -p tcp --dport 113 -j xlog_drop_wan2self\n"); // IDENT
       fprintf(filter_fp, "-A wan2self_ports -p icmp --icmp-type 8 -j xlog_drop_wan2self\n"); // DROP ICMP PING
   }
        // FIREWALL_DEBUG("Exiting do_wan2self_ports\n");    
   return(0);
}
/*
 *  Procedure     : do_wan2self_allow
 *  Purpose       :   
 *  Parameters    :
 *    filter_fp     : An open file to write filter rules to
 * Return Values  :
 *    0              : Success
 */
static int do_wan2self_allow(FILE *filter_fp)
{
        // FIREWALL_DEBUG("Entering do_wan2self_allow\n");    
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
   int i;
   //always allow ping if disable true static on firewall
   if(isFWTS_enable && isWanStaticIPReady){
//      fprintf(filter_fp, "-A wan2self_allow -d %s -p icmp --icmp-type 8 -m limit --limit 3/second -j %s\n", current_wan_static_ipaddr,"xlog_accept_wan2self");
      for(i = 0; i < StaticIPSubnetNum ;i++ )
         fprintf(filter_fp, "-A wan2self_allow -d %s -p icmp --icmp-type 8 -m limit --limit 3/second -j %s\n",  StaticIPSubnet[i].ip,"xlog_accept_wan2self");
  }

#endif
//	FIREWALL_DEBUG("Exiting do_wan2self_allow\n");	  
}
/*
 *  Procedure     : do_wan2self
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic
 *                  from the wan to utopia
 *  Parameters    :
 *    mangle_fp     : An open file to write mangle rules to
 *    nat_fp        : An open file to write nat rules to
 *    filter_fp     : An open file to write filter rules to
 * Return Values  :
 *    0              : Success
 */
static int do_wan2self(FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
  //       FIREWALL_DEBUG("Entering do_wan2self\n");    
   do_wan2self_allow(filter_fp);
   do_wan2self_attack(filter_fp);
   do_wan2self_ports(mangle_fp, nat_fp, filter_fp);
   do_mgmt_override(nat_fp);
   do_remote_access_control(nat_fp, filter_fp, AF_INET);
    //     FIREWALL_DEBUG("Exiting do_wan2self\n");    
   return(0);
}

/*
 ==========================================================================
                     lan2wan
 ==========================================================================
 */

/*
 *  Procedure     : write_block_application_statement
 *  Purpose       : prepare the iptables-restore statements with statements for dropping packets from well known applications
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *                  protocol.
 *     wkp_fp          : An open file containing well known port mappings
 *                       /etc/services is a standard linux format
 *     table           : table rule belongs to
 *     name            : name of the service  
 *  Return Values :
 *     0               : done
 */
static int write_block_application_statement (FILE *fp, FILE *wkp_fp, char *table, char *name)
{
   rewind(wkp_fp);
   char line[512];

   char *next_token;
   char *port_prot;
   char *port_val;
         FIREWALL_DEBUG("Entering write_block_application_statement\n");    
   while (NULL != (next_token = match_keyword(wkp_fp, name, ' ', line, sizeof(line))) ) {
      char port_str[50];
      sscanf(next_token, "%50s ", port_str);
      port_val = port_str;
      port_prot = strchr(port_val, '/');
      if (NULL != port_prot) {
         *port_prot = '\0';
         port_prot++;
      } else {
         continue;
      }
      
      char str[MAX_QUERY];
      snprintf(str, sizeof(str),
               "-A %s -p %s -m %s --dport %s -j xlogreject", table, port_prot, port_prot, port_val);
      fprintf(fp, "%s\n", str);

   }
         FIREWALL_DEBUG("Exiting write_block_application_statement\n");    
   return(0); 
}

/*
 *  Procedure     : do_lan2wan_webfilters
 *  Purpose       : prepare the iptables-restore statements for all
 *                  syscfg defined web filters
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */
static int do_lan2wan_webfilters(FILE *filter_fp)
{
   char str[MAX_QUERY];
   char webfilter_enable[4];
         FIREWALL_DEBUG("Entering do_lan2wan_webfilters\n");    
if ( 0 == syscfg_get(NULL, "block_webproxy", webfilter_enable, sizeof(webfilter_enable)) ) {
      if ('\0' != webfilter_enable[0] && 0 != strncmp("0", webfilter_enable, sizeof(webfilter_enable)) ) {
         snprintf(str, sizeof(str),
                  "-A lan2wan_webfilters -p tcp -m tcp --dport 80 -m httpurl --match-proxy -j xlogreject");
         fprintf(filter_fp, "%s\n", str);
      }
   }
   if ( 0 == syscfg_get(NULL, "block_java", webfilter_enable, sizeof(webfilter_enable)) ) {
      if ('\0' != webfilter_enable[0] && 0 != strncmp("0", webfilter_enable, sizeof(webfilter_enable)) ) {
         snprintf(str, sizeof(str),
                  "-A lan2wan_webfilters -p tcp -m tcp --dport 80 -m httpurl --match-java -j xlogreject");
         fprintf(filter_fp, "%s\n", str);
      }
   }
   if ( 0 == syscfg_get(NULL, "block_activex", webfilter_enable, sizeof(webfilter_enable)) ) {
      if ('\0' != webfilter_enable[0] && 0 != strncmp("0", webfilter_enable, sizeof(webfilter_enable)) ) {
         snprintf(str, sizeof(str),
                  "-A lan2wan_webfilters -p tcp -m tcp --dport 80 -m httpurl --match-activex -j xlogreject");
         fprintf(filter_fp, "%s\n", str);
      }
   }
   if ( 0 == syscfg_get(NULL, "block_cookies", webfilter_enable, sizeof(webfilter_enable)) ) {
      if ('\0' != webfilter_enable[0] && 0 != strncmp("0", webfilter_enable, sizeof(webfilter_enable)) ) {
         snprintf(str, sizeof(str),
                  "-A lan2wan_webfilters -p tcp -m tcp --dport 80 -m httpcookie --block-cookies");
         fprintf(filter_fp, "%s\n", str);
      }
   }
         FIREWALL_DEBUG("Exiting do_lan2wan_webfilters\n");    
   return(0);
}

/*
 *  Procedure     : set_lan_access_restriction_start_stop
 *  Purpose       : set cron wakeups for the start and stop of a policy
 *  Parameters    :
 *     fp              : An open file where we write cron rules
 *     days            : A bit field describing the days that this policy is active
 *     start           : A string describing the start policy
 *     stop            : A string describing the stop policy
 *     h24             : 1 if this policy is for 24 hours, else 0
 *  Return Values :
 *     0               : done
 *    -1               : error
 */
static int set_lan_access_restriction_start_stop(FILE *fp, int days, char *start, char *stop, int h24)
{
   int sh;
   int sm;
   int eh;
   int em;
      FIREWALL_DEBUG("Entering set_lan_access_restriction_start_stop\n");    
   /*
    * If the policy is for 7 days per week and NO start/stop times, then we dont need
    * a cron rule because the policy is always active
    */
   if (0x7F == days && 0 != h24 ) {
      return(0);
   }

   if (h24) {
      sh = sm = eh = em = 0;   
   } else {
      if ('\0' != start[0]) {
         if (2 != sscanf(start, "%d:%d", &sh, &sm)) {
            return(-1);
         } else {
            if (2 != sscanf(stop, "%d:%d", &eh, &em)) {
               return(-1);
            }
         }
      }
   }

   char str[MAX_QUERY];
   char *strp;
   int   bytes;
   int   rc;


   // 1) set a wakeup for starting 
   strp = str;
   bytes = sizeof(str);
   str[0] = '\0';

   // minutes field
   rc = snprintf(strp, bytes, " %d", sm);
   strp += rc;
   bytes -= rc;

   // hours field
   rc = snprintf(strp, bytes, " %d", sh);
   strp += rc;
   bytes -= rc;

   // days of month, and month of year
   rc = snprintf(strp, bytes, " %s", "* * ");
   strp += rc;
   bytes -= rc;
           
   // day of the week
   int first = 0;
   int day;

   int mask;
   for (day=0; day<=6; day++) {
      mask = 1 << day;
      if (days & mask) {
         rc = snprintf(strp, bytes, "%s%d", (0 == first ? "" : ","), day);
         first=1;
         strp += rc;
         bytes -= rc;
      }
   }
   *strp = '\0';
   fprintf(fp, " %s %s\n", str, "sysevent set pp_flush 1 && sysevent set firewall-restart");

   // 2) set a wakeup for stopping 
   strp = str;
   bytes = sizeof(str);
   str[0] = '\0';
   rc = snprintf(strp, bytes, " %d", em);
   strp += rc;
   bytes -= rc;

   // hours field
   rc = snprintf(strp, bytes, " %d", eh);
   strp += rc;
   bytes -= rc;

   // days of month, and month of year
   rc = snprintf(strp, bytes, " %s", "* * ");
   strp += rc;
   bytes -= rc;
           
   // day of the week
   first = 0;
   for (day=0; day<=6; day++) {
      mask = 1 << day;
      if (days & mask) {
         // if this is a 24 hour policy then the stop time is the next day
         rc = snprintf(strp, bytes, "%s%d", (0 == first ? "" : ","), (0 == h24 ? day : 6 == day ? 0 : day+1) );
         first=1;
         strp += rc;
         bytes -= rc;
      }
   }
   *strp = '\0';
   fprintf(fp, " %s %s\n", str, "sysevent set pp_flush 1 && sysevent set firewall-restart");
      FIREWALL_DEBUG("Exiting set_lan_access_restriction_start_stop\n");    
   return(0);
}

static int set_lan_access_restriction_start(FILE *fp, int days, char *start, int h24)
{
   int sh;
   int sm;
      FIREWALL_DEBUG("Entering set_lan_access_restriction_start\n");    
   sscanf(start, "%d:%d", &sh, &sm);

   char str[MAX_QUERY];
   char *strp;
   int   bytes;
   int   rc;

   // 1) set a wakeup for starting 
   strp = str;
   bytes = sizeof(str);
   str[0] = '\0';

   // minutes field
   rc = snprintf(strp, bytes, " %d", sm);
   strp += rc;
   bytes -= rc;

   // hours field
   rc = snprintf(strp, bytes, " %d", sh);
   strp += rc;
   bytes -= rc;

   // days of month, and month of year
   rc = snprintf(strp, bytes, " %s", "* * ");
   strp += rc;
   bytes -= rc;
           
   // day of the week
   int first = 0;
   int day;

   int mask;
   for (day=0; day<=6; day++) {
      mask = 1 << day;
      if (days & mask) {
         rc = snprintf(strp, bytes, "%s%d", (0 == first ? "" : ","), day);
         first=1;
         strp += rc;
         bytes -= rc;
      }
   }
   *strp = '\0';
   fprintf(fp, " %s %s\n", str, "sysevent set firewall-restart");
      FIREWALL_DEBUG("Exiting set_lan_access_restriction_start\n");    
   return(0);
}

static int set_lan_access_restriction_stop(FILE *fp, int days, char *stop, int h24)
{
   int eh;
   int em;
   // FIREWALL_DEBUG("Entering set_lan_access_restriction_stop\n");  
   sscanf(stop, "%d:%d", &eh, &em);

   char str[MAX_QUERY];
   char *strp;
   int   bytes;
   int   rc;

   // 2) set a wakeup for stopping 
   strp = str;
   bytes = sizeof(str);
   str[0] = '\0';
   rc = snprintf(strp, bytes, " %d", em);
   strp += rc;
   bytes -= rc;

   // hours field
   rc = snprintf(strp, bytes, " %d", eh);
   strp += rc;
   bytes -= rc;

   // days of month, and month of year
   rc = snprintf(strp, bytes, " %s", "* * ");
   strp += rc;
   bytes -= rc;
           
   // day of the week
   int first = 0;
   int day;
   int mask;

   for (day=0; day<=6; day++) {
      mask = 1 << day;
      if (days & mask) {
         // if this is a 24 hour policy then the stop time is the next day
         rc = snprintf(strp, bytes, "%s%d", (0 == first ? "" : ","), (0 == h24 ? day : 6 == day ? 0 : day+1) );
         first=1;
         strp += rc;
         bytes -= rc;
      }
   }
   *strp = '\0';
   fprintf(fp, " %s %s\n", str, "sysevent set firewall-restart");
   // FIREWALL_DEBUG("Exiting set_lan_access_restriction_stop\n");  
   return(0);
}



/*
 *  Procedure     : do_lan_access_restrictions
 *  Purpose       : prepare the iptables-restore statements for all
 *                  syscfg defined Internet Access Restrictions lists
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */
static int do_lan_access_restrictions(FILE *fp, FILE *nat_fp)
{

   char *cron_file = crontab_dir"/"crontab_filename;
   FILE *cron_fp; // the crontab file we use to set wakeups for timed firewall events
   cron_fp = fopen(cron_file, "w");
      
    FIREWALL_DEBUG("Entering do_lan_access_restrictions\n");  
   /*
    * syscfg tuple InternetAccessPolicy_x, where x is a digit
    * keeps track of the syscfg namespace of a user defined internet access policy.
    * We iterate through these tuples until we dont find an instance in syscfg.
    */ 
   int idx;
   int  rc;
   char namespace[MAX_QUERY];
   char query[MAX_QUERY];

   isCronRestartNeeded = 0;

   int count;
   query[0] = '\0';
   rc = syscfg_get(NULL, "InternetAccessPolicyCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      return(0);
   } else {
      count = atoi(query);
      if (0 == count) {
         return(0);
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      query[0] = '\0';
      snprintf(query, sizeof(query), "InternetAccessPolicy_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }
   
      // is this Internet Access Control list enabled
      query[0] = '\0';
      rc = syscfg_get(namespace, "enabled", query, sizeof(query));
      if (0 != rc || '\0' == query[0]) {
         continue;
      } else if (0 == strcmp("0", query)) {
        continue;
      }
   
      // Determine enforcement schedule and whether we are within the enforcement schedule right now
      int within_policy_start_stop = 0;

      query[0] = '\0';
      rc = syscfg_get(namespace, "enforcement_schedule", query, sizeof(query));
      if (0 != rc || '\0' == query[0]) {
         within_policy_start_stop = 1;
      } else  {
         int   policy_days = 127;
         char  policy_time_start[25];
         char  policy_time_stop[25];
 
         policy_time_start[0] = policy_time_stop[0] = '\0';
         sscanf(query, "%d %25s %25s", &policy_days, policy_time_start, policy_time_stop); 

         int h24 = 0;  // not 24 hours
         if ('\0' == policy_time_start[0] || '\0' == policy_time_stop[0]) {
            snprintf(policy_time_start, sizeof(policy_time_start), "0:0");
            snprintf(policy_time_stop, sizeof(policy_time_stop), "0:0");
            h24 = 1;
         }

         /*
          * Figure out if we are within the start/stop policy times
          * and prepare cron times to reevaluate the firewall based on policy changes
          * Note that if policy is not 24/7 and there is no reliable NTP, then we dont
          * apply policies
          */

         if (0x7F == policy_days && 1 == h24 ) {
            within_policy_start_stop = 1;
         } else if (isNtpFinished) {
            if (NULL != cron_fp) {
               set_lan_access_restriction_start_stop(cron_fp, policy_days, policy_time_start, policy_time_stop, h24);
               isCronRestartNeeded = 1;
            }

            /*
             * local time field wday provides the number of days since sunday
             * we convert that to a bit field where:
             *    1  = sunday
             *    2  = monday
             *    4  = tuesday
             *    8  = wednesday
             *   16  = thursday
             *   32  = friday
             *   64  = saturday
             */
            int today_bits = 0;
            today_bits = (1 << local_now.tm_wday);
            if(!(today_bits & policy_days)) {
            } else {
               if (1 == h24) {
                  within_policy_start_stop = 1;
               } else {
                  int hours, mins; 
                  if ( (-1 == time_delta(&local_now, policy_time_start, &hours, &mins)) ||
                         (hours == 0 && mins == 0) ) {
                     // if return from time_delta is -1 then we are past the policy_time_start.
                     // if return is 0 and hours = mins = 0, then we are at the policy_time_start
                     if (0 == time_delta(&local_now, policy_time_stop, &hours, &mins)) {
                        // we are before the end time too
                        within_policy_start_stop = 1;
                     }
                  }
               }
            }
         } else {
            continue;
         } 
      }

      // is this an allow or deny group
      int access_mode = 0; // deny
      char access[20];
      access[0] = '\0';
      rc = syscfg_get(namespace, "access", access, sizeof(access));
      if (0 == rc && 0 == strcmp("allow", access)) {
         access_mode = 1; 
      }

      /*
       * If this is a deny policy and we are not within the start/stop period
       * then ignore it
       */
      if (0 == access_mode && 0 == within_policy_start_stop) {
         continue;
      }

      char classification_table[50];
      char rules_table[50];
      snprintf(classification_table, sizeof(classification_table), "%s_classification", namespace);
      snprintf(rules_table, sizeof(rules_table), "%s_rules", namespace);


      // is there a list of local hosts to which this policy is applied
      char namespace2[MAX_QUERY];
      namespace2[0] = '\0';
      rc = syscfg_get(namespace, "local_host_list", namespace2, sizeof(namespace2));
      if (0 != rc || '\0' == namespace2[0]) {
         continue;
      }

      char str[MAX_QUERY];
      snprintf(str, sizeof(str),
               ":%s - [0:0]", classification_table); 
      fprintf(fp, "%s\n", str);
      snprintf(str, sizeof(str),
               "-N %s", classification_table); 
      snprintf(str, sizeof(str),
               "-F %s", classification_table); 

      snprintf(str, sizeof(str),
               ":%s - [0:0]", rules_table); 
      fprintf(fp, "%s\n", str);
      snprintf(str, sizeof(str),
               "-N %s", rules_table); 
      snprintf(str, sizeof(str),
               "-F %s", rules_table); 

      snprintf(str, sizeof(str),
               "-A lan2wan_iap -j %s", classification_table); 
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str),
               "-A lan2wan_iap -j %s", rules_table); 
      fprintf(fp, "%s\n", str);

      char block_device[MAX_QUERY];
      char block_site[MAX_QUERY];
      char block_service[MAX_QUERY];

      snprintf(block_device, sizeof(block_device), "LOG_DeviceBlocked_%d_DROP", idx);
      snprintf(block_site, sizeof(block_site), "LOG_SiteBlocked_%d_DROP", idx);
      snprintf(block_service, sizeof(block_service), "LOG_ServiceBlocked_%d_DROP", idx);

      fprintf(fp, ":%s: - [0:0]\n", block_device);
      fprintf(fp, "-N %s\n", block_device);
      fprintf(fp, "-F %s\n", block_device);

      fprintf(fp, ":%s: - [0:0]\n", block_site);
      fprintf(fp, "-N %s\n", block_site);
      fprintf(fp, "-F %s\n", block_site);

      fprintf(fp, ":%s: - [0:0]\n", block_service);
      fprintf(fp, "-N %s\n", block_service);
      fprintf(fp, "-F %s\n", block_service);

      if (isLogEnabled)
      {
         fprintf(fp, "-A %s -m limit --limit 1/minute --limit-burst 1 -j LOG --log-prefix %s\n", block_device, block_device);
         fprintf(fp, "-A %s -m limit --limit 1/minute --limit-burst 1 -j LOG --log-prefix %s\n", block_site, block_site);
         fprintf(fp, "-A %s -m limit --limit 1/minute --limit-burst 1 -j LOG --log-prefix %s\n", block_service, block_service);
      }

      fprintf(fp, "-A %s -j DROP\n", block_device);
      fprintf(fp, "-A %s -j DROP\n", block_site);
      fprintf(fp, "-A %s -j DROP\n", block_service);
         
      char query2[100];
      query2[0] = '\0';
      int count2;

      rc = syscfg_get(namespace2, "InternetAccessPolicyIPHostCount", query2, sizeof(query2));
      if (0 != rc || '\0' == query2[0]) {
         goto InternetAccessPolicyNext;
      } else {
         count2 = atoi(query2);
         if (0 == count2) {
            goto InternetAccessPolicyNext;
         }
         if (MAX_SYSCFG_ENTRIES < count2) {
            count2 = MAX_SYSCFG_ENTRIES;
         }
      }

      int i;
      int srcNum = 0;
      char src_ip[10][25];
      // add ip hosts to classification table
      for (i=1; i<=count2; i++) {
         char tstr[50];
         char ip[25];
         ip[0] = '\0';
         snprintf(tstr, sizeof(tstr), "ip_%d", i);
         rc = syscfg_get(namespace2, tstr, ip, sizeof(ip));
         srcNum = i-1;
         sprintf(src_ip[srcNum], "%s.%s", lan_3_octets, ip);
         if (0 != rc || '\0' == ip[0]) {
            continue;
         } else {
            snprintf(str, sizeof(str), 
                     "-A %s -s %s.%s -j %s", 
                     classification_table, lan_3_octets, ip,  
                     ( (0 == access_mode) || (1 == access_mode && 0 == within_policy_start_stop)) ? block_device : rules_table);
           fprintf(fp, "%s\n", str);

         }   
      }

InternetAccessPolicyNext:
      query2[0] = '\0';
      rc = syscfg_get(namespace2, "InternetAccessPolicyIPRangeCount", query2, sizeof(query2));
      if (0 != rc || '\0' == query2[0]) {
         goto InternetAccessPolicyNext2;
      } else {
         count2 = atoi(query2);
         if (0 == count2) {
            goto InternetAccessPolicyNext2;
         }
         if (MAX_SYSCFG_ENTRIES < count2) {
            count2 = MAX_SYSCFG_ENTRIES;
         }
      }

      // add ip ranges to classification table
      for (i=1; i<=count2 ; i++) {
         char tstr[100];
         char ip[25];
         ip[0] = '\0';
         snprintf(tstr, sizeof(tstr), "ip_range_%d", i);
         rc = syscfg_get(namespace2, tstr, ip, sizeof(ip));
         if (0 != rc || '\0' == ip[0]) {
            continue;
         } else {
            int first, last;
            if (2 == sscanf(ip, "%d %d", &first, &last)) {
               snprintf(str, sizeof(str), 
                        "-A %s -m iprange --src-range %s.%d-%s.%d -j %s", 
                        classification_table, lan_3_octets, first, lan_3_octets, last, 
                        ( (0 == access_mode) || (1 == access_mode && 0 == within_policy_start_stop)) ? block_device : rules_table);
              fprintf(fp, "%s\n", str);
            }
         }   
      }

InternetAccessPolicyNext2:
      query2[0] = '\0';
      rc = syscfg_get(namespace2, "InternetAccessPolicyMacCount", query2, sizeof(query2));
      if (0 != rc || '\0' == query2[0]) {
         goto InternetAccessPolicyNext3;
      } else {
         count2 = atoi(query2);
         if (0 == count2) {
            goto InternetAccessPolicyNext3;
         }
         if (MAX_SYSCFG_ENTRIES < count2) {
            count2 = MAX_SYSCFG_ENTRIES;
         }
      }

      // add mac hosts to classification table
      for (i=1; i<=count2; i++) {
         char tstr[100];
         char mac[25];
         mac[0] = '\0';
         snprintf(tstr, sizeof(tstr), "mac_%d", i);
         rc = syscfg_get(namespace2, tstr, mac, sizeof(mac));
         if (0 != rc || '\0' == mac[0]) {
            continue;
         } else {
            snprintf(str, sizeof(str), 
                     "-A %s -m mac --mac-source %s -j %s", 
                     classification_table, mac,
                     ( (0 == access_mode) || (1 == access_mode && 0 == within_policy_start_stop)) ? block_device : rules_table);
            fprintf(fp, "%s\n", str);
         }   
      }

InternetAccessPolicyNext3:
   if (1 == access_mode && 1 == within_policy_start_stop) {
         int count;
         char count_str[MAX_QUERY];
         //char dst_host[MAX_QUERY] ={'\0'};
         char block_page[MAX_QUERY] ={'\0'};
         char blockPage[MAX_QUERY] ={'\0'};
         char *ptr = NULL;
         int host_name_offset = 0; 
         //syscfg_get(NULL, "lan_ipaddr", dst_host, sizeof(dst_host));
         
         /*Currently the redirection is happening only to an IP*/ 
         rc = syscfg_get(NULL, "blocked_error_url", blockPage, sizeof(blockPage));
         if(rc !=0 || '\0' != blockPage[0]){
             strcpy(block_page, "/cosa/cpehelp/");
         }else{
             sprintf(block_page, "%sPolicy=%d", blockPage, idx);
         }
         //ulogf(ULOG_FIREWALL, UL_INFO, "dst_host=%s, block_page=%s@@@\n", dst_host, block_page);

         // block urls
         count_str[0] = '\0';
         rc = syscfg_get(namespace, "BlockUrlCount", count_str, sizeof(count_str));
         if (0 == rc && '\0' != count_str[0]) {
            count = atoi(count_str);
         } else { 
            count = 0;
         }
         if (MAX_SYSCFG_ENTRIES < count) {
            count = MAX_SYSCFG_ENTRIES;
         }

         for (i=1; i<=count ; i++) {
            char tstr[100];
            int src_cnt = 0;
            //char cmd[512] = {'\0'};
            char url[MAX_QUERY];
            url[0] = '\0';
            snprintf(tstr, sizeof(tstr), "BlockUrl_%d", i);
            rc = syscfg_get(namespace, tstr, url, sizeof(url));
            if (0 != rc || '\0' == url[0]) {
               continue;
            } else {
               host_name_offset = 0;
               
               // Strip http:// or https:// from the beginning of the URL
               // string so that only the host name is passed in
               if (0 == strncmp(url, "http://", STRLEN_HTTP_URL_PREFIX)) {
                  host_name_offset = STRLEN_HTTP_URL_PREFIX;
               }
               else if (0 == strncmp(url, "https://", STRLEN_HTTPS_URL_PREFIX)) {
                  host_name_offset = STRLEN_HTTPS_URL_PREFIX;
               }

               snprintf(str, sizeof(str), 
                        "-A %s -p tcp -m tcp --dport 80 -m httphost --host \"%s\" -j %s",
                        rules_table, url + host_name_offset, block_site);
               fprintf(fp, "%s\n", str);

               for(src_cnt; src_cnt <= srcNum; src_cnt++){
                   snprintf(str, sizeof(str),
                            "-A prerouting_fromlan -p tcp -s %s -d %s --dport 80 -j REDIRECT --to-port 81",
                                                                src_ip[src_cnt], url + host_name_offset);
                   fprintf(nat_fp, "%s\n", str);
                   //sprintf(cmd, "iptables %s", str);
                   //system(cmd);
                   ulogf(ULOG_FIREWALL, UL_INFO, "Hitting Policy-%d, HTTP request for %s from client %s", idx, url, src_ip[src_cnt]);
                   memset(str, 0, sizeof(str));
                   //memset(cmd, 0, sizeof(cmd));
	       }
            }   
         }

         // block keywords in http GET
         count_str[0] = '\0';
         rc = syscfg_get(namespace, "BlockKeywordCount", count_str, sizeof(count_str));
         if (0 == rc && '\0' != count_str[0]) {
            count = atoi(count_str);
         } else { 
            count = 0;
         }
         if (MAX_SYSCFG_ENTRIES < count) {
            count = MAX_SYSCFG_ENTRIES;
         }
         for (i=1; i<=count ; i++) {
            char tstr[100];
            char url[MAX_QUERY];
            url[0] = '\0';
            snprintf(tstr, sizeof(tstr), "BlockKeyword_%d", i);
            rc = syscfg_get(namespace, tstr, url, sizeof(url));
            if (0 != rc || '\0' == url[0]) {
               continue;
            } else {
               snprintf(str, sizeof(str), 
                        "-A %s -p tcp -m tcp  -m string --string \"%s\" --algo kmp -j %s", 
                        rules_table, url, block_site);
               fprintf(fp, "%s\n", str);
            }   
         }

         // block normalized applications
         count_str[0] = '\0';
         rc = syscfg_get(namespace, "BlockApplicationCount", count_str, sizeof(count_str));
         if (0 == rc && '\0' != count_str[0]) {
            count = atoi(count_str);
         } else {
            count = 0;
         }
         if (MAX_SYSCFG_ENTRIES < count) {
            count = MAX_SYSCFG_ENTRIES;
         }

         for (i=1;i<=count; i++) {
            char tstr[100];
            snprintf(tstr, sizeof(tstr), "BlockApplication_%d", i);
            char namespace2[MAX_QUERY];
            namespace2[0] = '\0';
            rc = syscfg_get(namespace, tstr, namespace2, sizeof(namespace2));
            if (0 != rc || '\0' == namespace2[0]) {
               continue;
            } else {
               char prot[10];
               int  proto;
               char portrange[30];
               char sdport[10];
               char edport[10];

               proto = 0; // 0 is both, 1 is tcp, 2 is udp
               prot[0] = '\0';
               rc = syscfg_get(namespace2, "protocol", prot, sizeof(prot));
               if (0 != rc || '\0' == prot[0]) {
                  proto = 0;
               } else if (0 == strcmp("tcp", prot)) {
                  proto = 1;
               } else   if (0 == strcmp("udp", prot)) {
                  proto = 2;
               }

               portrange[0]= '\0';
               sdport[0]   = '\0';
               edport[0]   = '\0';
               rc = syscfg_get(namespace2, "port_range", portrange, sizeof(portrange));
               if (0 != rc || '\0' == portrange[0]) {
                  continue;
               } else {
                  int r = 0;
                  if (2 != (r = sscanf(portrange, "%10s %10s", sdport, edport))) {
                     if (1 == r) {
                        snprintf(edport, sizeof(edport), "%s", sdport);
                     } else {
                        continue;
                     }
                  }
               }
               if (0 == proto || 1 ==  proto) {
                  snprintf(str, sizeof(str), 
                           "-A %s -p tcp -m tcp --dport %s:%s -j %s",
                           rules_table, sdport, edport, block_service);
                  fprintf(fp, "%s\n", str);

               }

               if (0 == proto || 2 ==  proto) {
                  snprintf(str, sizeof(str), 
                           "-A %s -p udp -m udp --dport %s:%s -j %s",
                      rules_table, sdport, edport, block_service);
                  fprintf(fp, "%s\n", str);
               }
            }
         }

         // block applications
         count_str[0] = '\0';
         rc = syscfg_get(namespace, "BlockApplicationRuleCount", count_str, sizeof(count_str));
         if (0 == rc && '\0' != count_str[0]) {
            count = atoi(count_str);
         } else {
            count = 0;
         }
         if (MAX_SYSCFG_ENTRIES < count) {
            count = MAX_SYSCFG_ENTRIES;
         }

         for (i=1;i<=count; i++) {
            char tstr[100];
            char app[1025];
            app[0] = '\0';
            snprintf(tstr, sizeof(tstr), "BlockApplicationRule_%d", i);
            rc = syscfg_get(namespace, tstr, app, sizeof(app));
            if (0 != rc || '\0' == app[0]) {
               continue;
            } else {
               char str[MAX_QUERY];
               snprintf(str, sizeof(str),
                        "-A %s %s -j %s", rules_table, app, block_service);
               fprintf(fp, "%s\n", str);

            }   
         }

         // block well-known applications
         count_str[0] = '\0';
         rc = syscfg_get(namespace, "BlockWellknownApplicationCount", count_str, sizeof(count_str));
         if (0 == rc && '\0' != count_str[0]) {
            count = atoi(count_str);
         } else {
            count = 0;
         }
         if (MAX_SYSCFG_ENTRIES < count) {
            count = MAX_SYSCFG_ENTRIES;
         }
         if (0 < count) {
            char *filename = wellknown_ports_file_dir"/"wellknown_ports_file;
            FILE *wkp_fp = fopen(filename, "r");
            if (NULL != wkp_fp) {
               for (i=1; i<=count; i++) {
                  char tstr[100];
                  char name[100];
                  name[0] = '\0';
                  snprintf(tstr, sizeof(tstr), "BlockWellknownApplication_%d", i);
                  rc = syscfg_get(namespace, tstr, name, sizeof(name));
                  if (0 != rc || '\0' == name[0]) {
                     continue;
                  } else {
                     write_block_application_statement(fp, wkp_fp, rules_table, name); 
                  }
               }   
               fclose(wkp_fp);
            }
         }
      }

      if (1 == access_mode && 1 == within_policy_start_stop) {
         // block special application - ping
         char tstr[100];
         tstr[0] = '\0';
         rc = syscfg_get(namespace, "BlockPing", tstr, sizeof(tstr));
         if (0 == rc && 0 == strcmp(tstr, "1")) {
            snprintf(str, sizeof(str), 
                     "-A %s -p icmp --icmp-type 8 -j xlogreject", rules_table);
            fprintf(fp, "%s\n", str);
         }
      }
   }


   fclose(cron_fp);
   if (!isCronRestartNeeded) {
      unlink(cron_file);
   }
    FIREWALL_DEBUG("Exiting do_lan_access_restrictions\n");  
   return(0);
}

// Determine enforcement schedule and whether we are within the enforcement schedule right now
static int determine_enforcement_schedule(FILE *cron_fp, const char *namespace) 
{
   int rc;
   char query[MAX_QUERY];
    FIREWALL_DEBUG("Entering determine_enforcement_schedule\n");  
   int always = 1;
   query[0] = '\0';
   rc = syscfg_get(namespace, "always", query, sizeof(query));
   if (0 != rc || '\0' == query[0] || query[0] == '0') always = 0;

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   if (always) return 0;
#else
   if (always) return 1;
#endif

   int policy_days = 0;
   char policy_time_start[25], policy_time_stop[25];

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   char timeStr[sizeof("00:00,12:00")];
   char policy_time_start_weekends[sizeof("00:00")], policy_time_stop_weekends[sizeof("00:00")];

   rc = syscfg_get(namespace, "end_time", timeStr, sizeof(timeStr));
   if (rc != 0 || timeStr[0] == '\0') strcpy(timeStr, "0:0,0:0");

   char *pch = strchr(timeStr, ',');
   *pch = '\0';

   strcpy(policy_time_start, timeStr);
   strcpy(policy_time_start_weekends, pch+1);

   rc = syscfg_get(namespace, "start_time", timeStr, sizeof(timeStr));
   if (rc != 0 || timeStr[0] == '\0') strcpy(timeStr, "0:0,0:0");

   pch = strchr(timeStr, ',');
   *pch = '\0';

   strcpy(policy_time_stop, timeStr);
   strcpy(policy_time_stop_weekends, pch+1);
#else
   rc = syscfg_get(namespace, "start_time", policy_time_start, sizeof(policy_time_start));
   if (rc != 0 || policy_time_start[0] == '\0') strcpy(policy_time_start, "0:0");

   rc = syscfg_get(namespace, "end_time", policy_time_stop, sizeof(policy_time_stop));
   if (rc != 0 || policy_time_stop[0] == '\0') strcpy(policy_time_stop, "0:0");
#endif

   query[0] = '\0';
   rc = syscfg_get(namespace, "days", query, sizeof(query));

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   if(strcasecmp(query, "never") == 0 || query[0] == 0)
       return 1;
#endif

   if (rc == 0)
   {
      char *ptr, *next_ptr;
      for (ptr = query; ptr && *ptr;ptr = next_ptr)
      {
         next_ptr = strchr(ptr, ',');
         if (next_ptr) *(next_ptr++) = '\0';
         if (strncasecmp(ptr, "SUN", 3) == 0) policy_days |= 1<<0;
         else if (strncasecmp(ptr, "MON", 3) == 0) policy_days |= 1<<1;
         else if (strncasecmp(ptr, "TUE", 3) == 0) policy_days |= 1<<2;
         else if (strncasecmp(ptr, "WED", 3) == 0) policy_days |= 1<<3;
         else if (strncasecmp(ptr, "THU", 3) == 0) policy_days |= 1<<4;
         else if (strncasecmp(ptr, "FRI", 3) == 0) policy_days |= 1<<5;
         else if (strncasecmp(ptr, "SAT", 3) == 0) policy_days |= 1<<6;
      }
   }

   int h24 = 0;  // not 24 hours

   if (cron_fp)
   {
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
      set_lan_access_restriction_start(cron_fp, 0x3e, policy_time_start, 0); //Mon ~ Fri
      set_lan_access_restriction_stop(cron_fp, 0x1F, policy_time_stop, 0); //Sun ~ Thu
      set_lan_access_restriction_start(cron_fp, 0x41, policy_time_start_weekends, 0); //Sat, Sun
      set_lan_access_restriction_stop(cron_fp, 0x60, policy_time_stop_weekends, 0); //Fri, Sat
      policy_days = 0x7f;
#else
      set_lan_access_restriction_start_stop(cron_fp, policy_days, policy_time_start, policy_time_stop, h24);
#endif
      isCronRestartNeeded = 1;
   }

   int within_policy_start_stop = 0;

   int today_bits = 0;
   today_bits = (1 << local_now.tm_wday);
   if(!(today_bits & policy_days)) {
   } else {
      if (1 == h24) {
         within_policy_start_stop = 1;
      } else {
         int startPassedHours, startPassedMins;
         int stopPassedHours, stopPassedMins;
         int startPass, stopPass;
         int sh, sm, eh, em;

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
         if(today_bits & 0x3e) { //Mon ~ Fri
             sscanf(policy_time_start, "%d:%d", &sh, &sm);
             startPass = time_delta(&local_now, policy_time_start, &startPassedHours, &startPassedMins);
         }

         if(today_bits & 0x1F) { //Sun ~ Thu
             sscanf(policy_time_stop, "%d:%d", &eh, &em);
             stopPass = time_delta(&local_now, policy_time_stop, &stopPassedHours, &stopPassedMins);
         }

         if(today_bits & 0x41) { //Sat, Sun
             sscanf(policy_time_start_weekends, "%d:%d", &sh, &sm);
             startPass = time_delta(&local_now, policy_time_start_weekends, &startPassedHours, &startPassedMins);
         }

         if(today_bits & 0x60) { //Fri, Sat
             sscanf(policy_time_stop_weekends, "%d:%d", &eh, &em);
             stopPass = time_delta(&local_now, policy_time_stop_weekends, &stopPassedHours, &stopPassedMins);
         }
#else
         sscanf(policy_time_start, "%d:%d", &sh, &sm);
         sscanf(policy_time_stop, "%d:%d", &eh, &em);

         startPass = time_delta(&local_now, policy_time_start, &startPassedHours, &startPassedMins);
         stopPass = time_delta(&local_now, policy_time_stop, &stopPassedHours, &stopPassedMins);
#endif
         //start time > stop time
         if(sh > eh || (sh == eh && sm >= em)) {
             if(!((stopPass == -1 || (stopPass == 0 && stopPassedHours == 0 && stopPassedMins == 0))
                 && startPass == 0))
               within_policy_start_stop = 1;
         }
         else { //start time < stop time
             //printf("today is %d, start time is %d, stop time is %d\n", today_bits, sh, eh);
             if((startPass == -1 || (startPass == 0 && startPassedHours == 0 && startPassedMins == 0))
                 && stopPass == 0) {
               within_policy_start_stop = 1;
             }
         }
       }
   }
    FIREWALL_DEBUG("Exiting determine_enforcement_schedule\n");  
   return within_policy_start_stop;
}

static int determine_enforcement_schedule2(FILE *cron_fp, const char *namespace) 
{
   int rc;
   char query[MAX_QUERY];
    FIREWALL_DEBUG("Entering determine_enforcement_schedule2\n");  
   int always = 1;
   query[0] = '\0';
   rc = syscfg_get(namespace, "always", query, sizeof(query));
   if (0 != rc || '\0' == query[0] || query[0] == '0') always = 0;

   if (always) return 1;

   int policy_days = 0;
   char policy_time_start[25], policy_time_stop[25];

   rc = syscfg_get(namespace, "start_time", policy_time_start, sizeof(policy_time_start));
   if (rc != 0 || policy_time_start[0] == '\0') strcpy(policy_time_start, "0:0");

   rc = syscfg_get(namespace, "end_time", policy_time_stop, sizeof(policy_time_stop));
   if (rc != 0 || policy_time_stop[0] == '\0') strcpy(policy_time_stop, "0:0");

   query[0] = '\0';
   rc = syscfg_get(namespace, "days", query, sizeof(query));

   if (rc == 0)
   {
      char *ptr, *next_ptr;
      for (ptr = query; ptr && *ptr;ptr = next_ptr)
      {
         next_ptr = strchr(ptr, ',');
         if (next_ptr) *(next_ptr++) = '\0';
         if (strncasecmp(ptr, "SUN", 3) == 0) policy_days |= 1<<0;
         else if (strncasecmp(ptr, "MON", 3) == 0) policy_days |= 1<<1;
         else if (strncasecmp(ptr, "TUE", 3) == 0) policy_days |= 1<<2;
         else if (strncasecmp(ptr, "WED", 3) == 0) policy_days |= 1<<3;
         else if (strncasecmp(ptr, "THU", 3) == 0) policy_days |= 1<<4;
         else if (strncasecmp(ptr, "FRI", 3) == 0) policy_days |= 1<<5;
         else if (strncasecmp(ptr, "SAT", 3) == 0) policy_days |= 1<<6;
      }
   }

   int h24 = 0;  // not 24 hours

   if (cron_fp)
   {
      set_lan_access_restriction_start_stop(cron_fp, policy_days, policy_time_start, policy_time_stop, h24);
      isCronRestartNeeded = 1;
   }

   int within_policy_start_stop = 0;

   int today_bits = 0;
   today_bits = (1 << local_now.tm_wday);
   if(!(today_bits & policy_days)) {
   } else {
      if (1 == h24) {
         within_policy_start_stop = 1;
      } else {
         int startPassedHours, startPassedMins;
         int stopPassedHours, stopPassedMins;
         int startPass, stopPass;
         int sh, sm, eh, em;


         sscanf(policy_time_start, "%d:%d", &sh, &sm);
         sscanf(policy_time_stop, "%d:%d", &eh, &em);

         startPass = time_delta(&local_now, policy_time_start, &startPassedHours, &startPassedMins);
         stopPass = time_delta(&local_now, policy_time_stop, &stopPassedHours, &stopPassedMins);
         
         //start time > stop time
         if(sh > eh || (sh == eh && sm >= em)) {
             if(!((stopPass == -1 || (stopPass == 0 && stopPassedHours == 0 && stopPassedMins == 0))
                 && startPass == 0))
               within_policy_start_stop = 1;
         }
         else { //start time < stop time
             //printf("today is %d, start time is %d, stop time is %d\n", today_bits, sh, eh);
             if((startPass == -1 || (startPass == 0 && startPassedHours == 0 && startPassedMins == 0))
                 && stopPass == 0) {
               within_policy_start_stop = 1;
             }
         }
       }
   }
    FIREWALL_DEBUG("Exiting determine_enforcement_schedule2\n");  
   return within_policy_start_stop;
}

/*
 *  Procedure     : do_parental_control_allow_trusted
 *  Purpose       : prepare the iptables-restore statements for parental control trusted user
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *     iptype          : 4 or 6
 *     list_name       : syscfg name for user list
 *     table_name      : iptable name for rules
 *  Return Values :
 *     0               : done
 */
static int do_parental_control_allow_trusted(FILE *fp, int iptype, const char* list_name, const char* table_name)
{
   int count=0, idx, rc;
   int count_v4=0, count_v6=0;
   char query[MAX_QUERY], result[MAX_QUERY];
    FIREWALL_DEBUG("Entering do_parental_control_allow_trusted\n");  
   snprintf(query, sizeof(query), "%sCount", list_name);

   result[0] = '\0';
   rc = syscfg_get(NULL, query, result, sizeof(result)); 
   if (rc == 0 && result[0] != '\0') count = atoi(result);
   if (count < 0) count = 0;
   if (count > MAX_SYSCFG_ENTRIES) count = MAX_SYSCFG_ENTRIES;
   for (idx = 1; idx <= count; idx++)
   {
      char namespace[MAX_QUERY];
      snprintf(query, sizeof(query), "%s_%d", list_name, idx);
      namespace[0] = '\0';
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }

      int trusted = 1;
      query[0] = '\0';
      rc = syscfg_get(namespace, "trusted", query, sizeof(query));
      if (0 != rc || '\0' == query[0] || query[0] == '0') trusted = 0;

      if (!trusted) continue;

      int this_iptype = 4;
      query[0] = '\0';
      rc = syscfg_get(namespace, "ip_type", query, sizeof(query)); 
      if (rc == 0 && query[0] != '\0') this_iptype = atoi(query);
      if (this_iptype != 6) this_iptype = 4;

      this_iptype == 4 ? ++count_v4 : ++count_v6;

      if (this_iptype == iptype)
      {
         query[0] = '\0';
         rc = syscfg_get(namespace, "ip_addr", query, sizeof(query)); 
         if (rc == 0 && query[0] != '\0')
         {
            fprintf(fp, "-A %s -s %s -j RETURN\n", table_name, query);
         }
      }
   }
    FIREWALL_DEBUG("Exiting do_parental_control_allow_trusted\n");  
   return iptype == 4 ? count_v4 : count_v6;
}
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
void block_url_by_ipaddr(FILE *fp, char *url, char *dropLog, int ipver, char *insNum, const char *nstdPort)
{
    //IPv4, NAT table will REDIRECT those ip, so needn't add rule in filter table 
//    if(ipver == 6){
//         if(nstdPort[0] == '\0')
//         {
//            fprintf(fp, "-A lan2wan_pc_site  -p tcp -m tcp --dport 80 -m set --match-set %s_v6 dst -m comment --comment \"host match %s \" -j %s\n", insNum, url, dropLog);
//            fprintf(fp, "-A lan2wan_pc_site  -p tcp -m tcp --dport 443 -m set --match-set %s_v6 dst -m comment --comment \"host match %s \" -j %s\n", insNum, url, dropLog);
//         }
//         else
//            fprintf(fp, "-A lan2wan_pc_site  -p tcp -m tcp --dport %s -m set --match-set %s_v6 dst -m comment --comment \"host match %s \" -j %s\n", nstdPort, insNum, url, dropLog);
//    }
}
#else
void block_url_by_ipaddr(FILE *fp, char *url, char *dropLog, int ipver, char *insNum, const char *nstdPort)
{
    char filePath[256];
    char ipAddr[40];
    FILE *ipRecords;
    int len;
    int dnsResponse = 0;
    FIREWALL_DEBUG("Entering block_url_by_ipaddr\n");  
    if(ipver == 6)
        snprintf(filePath, sizeof(filePath), "/var/.pc_url2ipv6_%s", insNum);
    else
        snprintf(filePath, sizeof(filePath), "/var/.pc_url2ip_%s", insNum);

    ipRecords = fopen(filePath, "r");

    if(ipRecords == NULL) {
        ipRecords = fopen(filePath, "w+");

        struct addrinfo hints, *res, *p;
        int status;

        memset(&hints, 0, sizeof(hints));
        if(ipver == 6)
            hints.ai_family = AF_INET6;
        else
            hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if ((status = getaddrinfo(url, NULL, &hints, &res)) != 0) {
            goto EXIT;
        }

        for(p = res;p != NULL; p = p->ai_next) {
            if((ipver == 6 && p->ai_family == AF_INET) || \
               (ipver == 4 && p->ai_family == AF_INET6))
                continue;

            void *addr;
            if(ipver == 4){
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
                addr = &(ipv4->sin_addr);
            }else{
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
                addr = &(ipv6->sin6_addr);
            }
            inet_ntop(p->ai_family, addr, ipAddr, sizeof(ipAddr));
            fprintf(ipRecords, "%s\n", ipAddr);
        }

        freeaddrinfo(res);
        rewind(ipRecords);
    }

    memset(ipAddr, 0, sizeof(ipAddr));

    while(fgets(ipAddr, sizeof(ipAddr), ipRecords) != NULL) {
        dnsResponse = 1;
        len = strlen(ipAddr);
        if(len > 0 && ipAddr[len-1] == '\n')
            ipAddr[len-1] = '\0';
 
        if(nstdPort[0] == '\0')
        {
            fprintf(fp, "-A lan2wan_pc_site -d %s -p tcp -m tcp --dport 80 -m comment --comment \"host match %s \" -j %s\n", ipAddr, url, dropLog);
            fprintf(fp, "-A lan2wan_pc_site -d %s -p tcp -m tcp --dport 443 -m comment --comment \"host match %s \" -j %s\n", ipAddr, url, dropLog);
        }
        else
            fprintf(fp, "-A lan2wan_pc_site -d %s -p tcp -m tcp --dport %s -m comment --comment \"host match %s \" -j %s\n", ipAddr, nstdPort, url, dropLog);
    }

EXIT:
    fclose(ipRecords);

    if(dnsResponse == 0)
        unlink(filePath);
    FIREWALL_DEBUG("Exiting block_url_by_ipaddr\n");  
    return;
}
#endif

static char *convert_url_to_hex_fmt(const char *url, char *dnsHexUrl)
{
    char s1[256], s2[512 + 32];
    char *p = s1, *tok;
    int i, len;
    FIREWALL_DEBUG("Entering convert_url_to_hex_fm\n"); 
    if(strlen(url) > 255) {
        fprintf(stderr, "firewall: %s - maxium length of url is 255\n", __FUNCTION__);
        return NULL;
    }

    strncpy(s1, url, sizeof(s1));
    *dnsHexUrl = '\0';

    while ((tok = strsep(&p, ".")) != NULL) {
        len = strlen(tok);
        sprintf(s2, "%02d", len);

        for(i = 0; i < len; i++) {
            sprintf(&s2[2*(i+1)], "%2x", tok[i]);
        }
        strcat(dnsHexUrl, s2);
    }
    FIREWALL_DEBUG("Exiting convert_url_to_hex_fm\n"); 
    return dnsHexUrl;
}

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
/*
 * Device based parental control for Rogers Cisco Connect. Used in conjunction with Walled Garden.
 */
static void do_device_based_parcon_allow(FILE *fp)
{
    FILE *allowList = fopen(PARCON_ALLOW_LIST, "r");
    char line[256];
    char *t;
    FIREWALL_DEBUG("Entering do_device_based_parcon_allow\n"); 
    if(!allowList)
        return;

    //each line in PARCON_ALLOW_LIST file should looks exactly like "00:11:22:33:44:55,XXXXXXXX"
    while(fgets(line, sizeof(line), allowList) != NULL) {
        if((t = strchr(line, ',')) != NULL) {
            *t = '\0';
            fprintf(fp, "-A parcon_allow_list -m mac --mac-source %s -j ACCEPT\n", line);
        }
    }
    FIREWALL_DEBUG("Exiting do_device_based_parcon_allow\n"); 
    return;
}

static int do_device_based_parcon(FILE *natFp, FILE* filterFp)
{
   char *cron_file = crontab_dir"/"crontab_filename;
   FILE *cron_fp = NULL; // the crontab file we use to set wakeups for timed firewall events
   FILE *fp;
   char ipAddr[256];
   int len;
   char filePath[256];
    FIREWALL_DEBUG("Entering do_device_based_parcon\n"); 
   cron_fp = fopen(cron_file, "a+");

   int rc;
   char query[1024]; //max url list length is 1024

   query[0] = '\0';
   rc = syscfg_get(NULL, "managedsites_enabled", query, sizeof(query)); 
   if (rc == 0 && query[0] != '\0' && query[0] != '0') // managed site list enabled
   {
      int count = 0, idx, ruleIndex = 0;

      // first, we let traffic from allow list get through
      // bypass ipset redirect rules in nat PREROUTING
      do_device_based_parcon_allow(natFp);
      // bypass HTTP GET spoof rules in filter FORWARD
      do_device_based_parcon_allow(filterFp);

      query[0] = '\0';
      rc = syscfg_get(NULL, "ManagedSiteBlockCount", query, sizeof(query)); 
      if (rc == 0 && query[0] != '\0') count = atoi(query);
      if (count < 0) count = 0;
      if (count > MAX_SYSCFG_ENTRIES) count = MAX_SYSCFG_ENTRIES;
      
      for (idx = 1; idx <= count; idx++)
      {
         char namespace[MAX_QUERY];
         snprintf(query, sizeof(query), "ManagedSiteBlock_%d", idx);

         namespace[0] = '\0';
         rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
         if (0 != rc || '\0' == namespace[0]) {
            continue;
         }

         char ins_num[16] = "";
         rc = syscfg_get(namespace, "ins_num", ins_num, sizeof(ins_num));
         if (0 != rc || '\0' == ins_num[0]) continue;

         query[0] = '\0';
         rc = syscfg_get(namespace, "days", query, sizeof(query)); 
         if (0 != rc || strcmp("disable", query) == 0) continue;

         query[0] = '\0';
         rc = syscfg_get(namespace, "mac", query, sizeof(query)); 
         if (0 != rc || '\0' == query[0]) continue;

         fprintf(natFp, ":device_%s - [0:0]\n", ins_num);
         fprintf(natFp, "-A parcon_walled_garden -m mac --mac-source %s -j device_%s\n", query, ins_num);

         // parental control walled garden SFS
         // -> device try to access a website
         // -> check if current time is blocked (if blocked then walled garden)
         // -> check if this site is blocked (if blocked then walled garden)
         // -> if no violation then grant access, else goto walled garden
         // -> if password is correct then grant access to all websites including blocked sites
         int within_policy_start_stop = determine_enforcement_schedule(cron_fp, namespace);
         if (!within_policy_start_stop){
             fprintf(natFp, "-A device_%s -p tcp --dport 80 -j REDIRECT --to-port %s\n", \
                                                        ins_num, PARCON_WALLED_GARDEN_HTTP_PORT_TIMEBLK);
             fprintf(natFp, "-A device_%s -p tcp --dport 443 -j REDIRECT --to-port %s\n", \
                                                        ins_num, PARCON_WALLED_GARDEN_HTTPS_PORT_TIMEBLK);
             continue;
         }
         
         fprintf(natFp, "-A device_%s -p tcp --dport 80 -m set --match-set %s dst -j REDIRECT --to-port %s\n", \
                                                        ins_num, ins_num, PARCON_WALLED_GARDEN_HTTP_PORT_SITEBLK);
         fprintf(natFp, "-A device_%s -p tcp --dport 443 -m set --match-set %s dst -j REDIRECT --to-port %s\n", \
                                                        ins_num, ins_num, PARCON_WALLED_GARDEN_HTTPS_PORT_SITEBLK);

         fprintf(filterFp, ":device_%s_container - [0:0]\n", ins_num);
         fprintf(filterFp, "-A wan2lan_dns_intercept -j device_%s_container\n", ins_num);

         //lan2wan dns query interception per device
         fprintf(filterFp, ":lan2wan_dnsq_nfqueue_%s - [0:0]\n", ins_num);
         //lan2wan http interception per device
         fprintf(filterFp, ":lan2wan_http_nfqueue_%s - [0:0]\n", ins_num);

         do_device_based_pp_disabled_appendrule(filterFp, ins_num, lan_ifname, query);

         //these rules are for http and dns query interception per device
         fprintf(filterFp, "-A lan2wan_httpget_intercept -m mac --mac-source %s -j lan2wan_http_nfqueue_%s\n", query, ins_num);
         fprintf(filterFp, "-A lan2wan_dnsq_intercept -m mac --mac-source %s -m limit --limit 2/m --limit-burst 2 -j lan2wan_dnsq_nfqueue_%s\n", query, ins_num);
         fprintf(filterFp, "-A lan2wan_dnsq_nfqueue_%s -j MARK --set-mark %s\n", ins_num, ins_num);
         fprintf(filterFp, "-A lan2wan_dnsq_nfqueue_%s -j NFQUEUE --queue-num %d\n", ins_num, DNS_QUERY_QUEUE_NUM);

         //these rules are for disable wan2lan pp
         fprintf(filterFp, ":wan2lan_dnsr_nfqueue_%s - [0:0]\n", ins_num);

         snprintf(filePath, sizeof(filePath), PARCON_IP_URL"/%s", query);
         FILE *mac2Ip = fopen(filePath, "r");
         if(mac2Ip != NULL) {
             fgets(ipAddr, sizeof(ipAddr), mac2Ip);
             fprintf(filterFp, "-A device_%s_container -d %s -m limit --limit 1/s --limit-burst 1 -j wan2lan_dnsr_nfqueue_%s\n", ins_num, ipAddr, ins_num);
             do_device_based_pp_disabled_ip_appendrule(filterFp, ins_num, ipAddr);
             fclose(mac2Ip);
         }

         query[0] = '\0';
         rc = syscfg_get(namespace, "site", query, sizeof(query));
         if (0 != rc || '\0' == query[0]) continue;

         char *token, *str = query;
         int index = 0;
         char hexUrl[512 + 32];

         uint32_t insNum = (uint32_t)atoi(ins_num);
         uint32_t siteIndex = 0;

         //these rules are for dns response interception of all blocked sites
         while((token = strsep(&str, ",")) != NULL) {
             siteIndex++;
             if(convert_url_to_hex_fmt(token, hexUrl) != NULL) {

                 //if Host: XXX is found, DROP and send spoof HTTP redirect in nfqueue handler
                 fprintf(filterFp, "-A lan2wan_http_nfqueue_%s -m webstr --url %s -j MARK --set-mark 0x%x\n", ins_num, token, insNum);

                 if(HTTP_GET_QUEUE_NUM_START == HTTP_GET_QUEUE_NUM_END)
                     fprintf(filterFp, "-A lan2wan_http_nfqueue_%s -m webstr --url %s -j NFQUEUE --queue-num %d\n", \
                                                                ins_num, token, HTTP_GET_QUEUE_NUM_START);
                 else
                     fprintf(filterFp, "-A lan2wan_http_nfqueue_%s -m webstr --url %s -j NFQUEUE --queue-balance %d:%d\n", \
                                                                ins_num, token, HTTP_GET_QUEUE_NUM_START, HTTP_GET_QUEUE_NUM_END);

                 fprintf(filterFp, "-A wan2lan_dnsr_nfqueue_%s -m string --algo kmp --hex-string \"|%s|\" -j MARK --set-mark 0x%x\n", \
                                                                ins_num, hexUrl, insNum);

                 if(DNS_RES_QUEUE_NUM_START == DNS_RES_QUEUE_NUM_END)
                     fprintf(filterFp, "-A wan2lan_dnsr_nfqueue_%s -m string --algo kmp --hex-string \"|%s|\" -j NFQUEUE --queue-num %d\n", \
                                                                ins_num, hexUrl, DNS_RES_QUEUE_NUM_START);
                 else
                     fprintf(filterFp, "-A wan2lan_dnsr_nfqueue_%s -m string --algo kmp --hex-string \"|%s|\" -j NFQUEUE --queue-balance %d:%d\n", \
                                                                ins_num, hexUrl, DNS_RES_QUEUE_NUM_START, DNS_RES_QUEUE_NUM_END);
             }
         }

         //TO DO: log needed?
         //char drop_log[40];
         //snprintf(drop_log, sizeof(drop_log), "LOG_SiteBlocked_%d_DROP", idx);
         //fprintf(fp, ":%s - [0:0]\n", drop_log);
         //fprintf(fp, "-A %s -j LOG --log-prefix %s --log-level %d\n", drop_log, drop_log, syslog_level);
         //fprintf(fp, "-A %s -j DROP\n", drop_log);
      }
   }

   fclose(cron_fp);
    FIREWALL_DEBUG("Exiting do_device_based_parcon\n"); 
   return(0);
}
#endif

/*
 *  Procedure     : do_parental_control
 *  Purpose       : prepare the iptables-restore statements for all
 *                  syscfg defined Parental Control Rules
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */

static int do_parcon_mgmt_device(FILE *fp, int iptype, FILE *cron_fp);
static int do_parcon_mgmt_service(FILE *fp, int iptype, FILE *cron_fp);
static int do_parcon_mgmt_site_keywd(FILE *fp, FILE *nat_fp, int iptype, FILE *cron_fp);

static int do_parental_control(FILE *fp,FILE *nat_fp, int iptype) {

    char *cron_file = crontab_dir"/"crontab_filename;
    FILE *cron_fp = NULL; // the crontab file we use to set wakeups for timed firewall events
    FIREWALL_DEBUG("Entering do_parental_control\n"); 
    //only do cron configuration once   
    if (iptype == 4) {
        cron_fp = fopen(cron_file, "a+");
    }
	if (iptype == 4)
	{
    	do_parcon_mgmt_device(nat_fp, iptype, cron_fp);
	}
	else
	{
		do_parcon_mgmt_device(nat_fp,iptype, NULL);
	}
#ifndef CONFIG_CISCO_FEATURE_CISCOCONNECT
    do_parcon_mgmt_site_keywd(fp,nat_fp, iptype, cron_fp);
#endif
    do_parcon_mgmt_service(fp, iptype, cron_fp);

    if (iptype == 4) {
        fclose(cron_fp);
    }
    FIREWALL_DEBUG("Exiting do_parental_control\n"); 
    return 0;
}

/*
 * add parental control managed device rules
 */
static int do_parcon_mgmt_device(FILE *fp, int iptype, FILE *cron_fp)
{
   int rc;
   char query[MAX_QUERY];
   FIREWALL_DEBUG("Entering do_parcon_mgmt_device\n"); 
   query[0] = '\0';
   rc = syscfg_get(NULL, "manageddevices_enabled", query, sizeof(query)); 
   if (rc == 0 && query[0] != '\0' && query[0] != '0') { // managed device list enabled
      int allow_all = 0;
      query[0] = '\0';
      rc = syscfg_get(NULL, "manageddevices_allow_all", query, sizeof(query)); 
      if (rc == 0 && query[0] != '\0' && query[0] != '0') allow_all = 1;

      int count = 0;
      query[0] = '\0';
      rc = syscfg_get(NULL, "ManagedDeviceCount", query, sizeof(query)); 
      if (rc == 0 && query[0] != '\0') count = atoi(query);
      if (count < 0) count = 0;
      if (count > MAX_SYSCFG_ENTRIES) count = MAX_SYSCFG_ENTRIES;

      int idx;
      for (idx = 1; idx <= count; idx++) {
         char namespace[MAX_QUERY];
         snprintf(query, sizeof(query), "ManagedDevice_%d", idx);
         namespace[0] = '\0';
         rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
         if (0 != rc || '\0' == namespace[0]) {
            continue;
         }

         int block = 1;
         query[0] = '\0';
         rc = syscfg_get(namespace, "block", query, sizeof(query));
         if (0 != rc || '\0' == query[0] || query[0] == '0') block = 0;

         if (allow_all != block) continue;

		 if (iptype == 4){
	        int within_policy_start_stop = determine_enforcement_schedule2(cron_fp, namespace);

         	if (!within_policy_start_stop) continue;
		 }
		 

         query[0] = '\0';
         rc = syscfg_get(namespace, "mac_addr", query, sizeof(query));
         if (0 != rc || '\0' == query[0]) continue;

         fprintf(fp, "-A prerouting_devices -p tcp -m mac --mac-source %s -j prerouting_redirect\n",query);
      }

      if (!allow_all) {
        fprintf(fp, "-A prerouting_devices -p tcp -j prerouting_redirect\n");
      }
   }
   FIREWALL_DEBUG("Exiting do_parcon_mgmt_device\n"); 
   return(0);
}

/*
 * add parental control managed service(ports) rules
 */
static int do_parcon_mgmt_service(FILE *fp, int iptype, FILE *cron_fp)
{
   FIREWALL_DEBUG("Entering do_parcon_mgmt_service\n"); 
   int rc;
   char query[MAX_QUERY];

   query[0] = '\0';
   rc = syscfg_get(NULL, "managedservices_enabled", query, sizeof(query)); 
   if (rc == 0 && query[0] != '\0' && query[0] != '0') {// managed site list enabled
      int count=0, idx;

      // first, we let traffic from trusted user get through
      do_parental_control_allow_trusted(fp, iptype, "ManagedServiceTrust", "lan2wan_pc_service");

#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
     /* only ipv4 has nat table, so cannot use nat table to redirect service port */ 
     fprintf(fp, "%s\n", ":parcon_service_nfq - [0:0]");
     fprintf(fp, "-A parcon_service_nfq -p tcp --syn -j ACCEPT\n");
     fprintf(fp, "-A parcon_service_nfq -j MARK --set-mark 0x00\n");
     fprintf(fp, "-A parcon_service_nfq -m string --string \"HTTP\" --algo kmp  -m string --string \"GET\" --algo kmp -j NFQUEUE "HTTP_GET_QUEUE_CONFIG "\n");
     fprintf(fp, "-A parcon_service_nfq -p tcp --tcp-flag FIN FIN -j REJECT --reject-with tcp-reset\n");
     fprintf(fp, "-A parcon_service_nfq -j DROP\n");
#endif
      query[0] = '\0';
      rc = syscfg_get(NULL, "ManagedServiceBlockCount", query, sizeof(query)); 
      if (rc == 0 && query[0] != '\0') count = atoi(query);
      if (count < 0) count = 0;
      if (count > MAX_SYSCFG_ENTRIES) count = MAX_SYSCFG_ENTRIES;
      for (idx = 1; idx <= count; idx++) {
         char namespace[MAX_QUERY];
         snprintf(query, sizeof(query), "ManagedServiceBlock_%d", idx);
         namespace[0] = '\0';
         rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
         if (0 != rc || '\0' == namespace[0]) {
            continue;
         }

         int within_policy_start_stop = determine_enforcement_schedule2(cron_fp, namespace);
         if (!within_policy_start_stop) continue;

         char prot[10];
         int  proto;
         char sdport[10];
         char edport[10];

         proto = 0; // 0 is both, 1 is tcp, 2 is udp
         prot[0] = '\0';
         rc = syscfg_get(namespace, "proto", prot, sizeof(prot));
         if (0 != rc || '\0' == prot[0]) {
            proto = 0;
         } else if (0 == strncasecmp("tcp", prot, 3)) {
            proto = 1;
         } else if (0 == strncasecmp("udp", prot, 3)) {
            proto = 2;
         }

         sdport[0]   = '\0';
         rc = syscfg_get(namespace, "start_port", sdport, sizeof(sdport));
         if (0 != rc || '\0' == sdport[0]) {
            continue;
         }

         edport[0]   = '\0';
         rc = syscfg_get(namespace, "end_port", edport, sizeof(edport));
         if (0 != rc || '\0' == edport[0]) {
            continue;
         }

         char drop_log[40];
         snprintf(drop_log, sizeof(drop_log), "LOG_ServiceBlocked_%d_DROP", idx);
         fprintf(fp, ":%s - [0:0]\n", drop_log);
         fprintf(fp, "-A %s -m limit --limit 1/minute --limit-burst 1 -j LOG --log-prefix %s --log-level %d\n", drop_log, drop_log, syslog_level);
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN

         fprintf(fp, "-A %s -p tcp -m multiport --dports 80,8080 -j parcon_service_nfq\n", drop_log);
		 /* if we dorp the tcp SYN packet without any FIN or RST, some client will retry many times*/ 
         fprintf(fp, "-A %s -j DROP\n", drop_log);
//         fprintf(fp, "-A %s -p tcp -j REJECT --reject-with tcp-reset\n", drop_log);
//         if(iptype == 4)
//            fprintf(fp, "-A %s -p udp -j REJECT --reject-with icmp-port-unreachable\n", drop_log);
//         else
//            fprintf(fp, "-A %s -p udp -j REJECT --reject-with icmp6-port-unreachable\n", drop_log);
#else
         fprintf(fp, "-A %s -j DROP\n", drop_log);
#endif
         if (0 == proto || 1 ==  proto) {
            fprintf(fp, "-A lan2wan_pc_service -p tcp -m tcp --dport %s:%s -j %s\n", sdport, edport, drop_log);
         }

         if (0 == proto || 2 ==  proto) {
            fprintf(fp, "-A lan2wan_pc_service -p udp -m udp --dport %s:%s -j %s\n", sdport, edport, drop_log);
         }
      }
   }
   FIREWALL_DEBUG("Exiting do_parcon_mgmt_service\n"); 
   return(0);
}

/*
 * add parental control managed site/keyword rules
 */
static int do_parcon_mgmt_site_keywd(FILE *fp, FILE *nat_fp, int iptype, FILE *cron_fp)
{
    int rc;
    char query[MAX_QUERY];
    int isHttps = 0; 
   FIREWALL_DEBUG("Entering do_parcon_mgmt_site_keywd\n"); 
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
    if(iptype == 4)
        fprintf(nat_fp, "-A prerouting_fromlan -j managedsite_based_parcon\n");
#endif

    query[0] = '\0';
    rc = syscfg_get(NULL, "managedsites_enabled", query, sizeof(query)); 
    if (rc == 0 && query[0] != '\0' && query[0] != '0') // managed site list enabled
    {
        int count = 0, idx, ruleIndex = 0;

        // first, we let traffic from trusted user get through
        ruleIndex = do_parental_control_allow_trusted(fp, iptype, "ManagedSiteTrust", "lan2wan_pc_site");
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
        if(iptype == 4){
            ruleIndex = do_parental_control_allow_trusted(nat_fp, iptype, "ManagedSiteTrust", "managedsite_based_parcon");
            fprintf(nat_fp, "-A managedsite_based_parcon -j parcon_walled_garden\n");
        }
#endif

        query[0] = '\0';
        rc = syscfg_get(NULL, "ManagedSiteBlockCount", query, sizeof(query)); 
        if (rc == 0 && query[0] != '\0') count = atoi(query);
        if (count < 0) count = 0;
        if (count > MAX_SYSCFG_ENTRIES) count = MAX_SYSCFG_ENTRIES;

#if !defined(_COSA_BCM_MIPS_)
        ruleIndex += do_parcon_mgmt_lan2wan_pc_site_appendrule(fp);
#endif

        for (idx = 1; idx <= count; idx++)
        {
            char namespace[MAX_QUERY];
            snprintf(query, sizeof(query), "ManagedSiteBlock_%d", idx);
            namespace[0] = '\0';
            rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
            if (0 != rc || '\0' == namespace[0]) {
                continue;
            }

            char method[20] = "";
            rc = syscfg_get(namespace, "method", method, sizeof(method));
            if (0 != rc || '\0' == method[0]) continue;

            char ins_num[16] = "";
            rc = syscfg_get(namespace, "ins_num", ins_num, sizeof(ins_num));
            if (0 != rc || '\0' == ins_num[0]) continue;

            query[0] = '\0';
            rc = syscfg_get(namespace, "site", query, sizeof(query)); 
            if (0 != rc || '\0' == query[0]) continue;
            
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
            if (strncasecmp(method, "URL", 3)==0)
            {
                char hexUrl[MAX_URL_LEN * 2 + 32];
                int host_name_offset = 0;
                isHttps = 0;
                if (0 == strncmp(query, "http://", STRLEN_HTTP_URL_PREFIX)) {
                    host_name_offset = STRLEN_HTTP_URL_PREFIX;
                }
                else if (0 == strncmp(query, "https://", STRLEN_HTTPS_URL_PREFIX)) {
                    host_name_offset = STRLEN_HTTPS_URL_PREFIX;
                    isHttps = 1;
                }
                char *tmp;
                int is_dnsr_nfq = 1;
                if(strncmp(query+host_name_offset, "[", 1) == 0){ /* if this is a ipv6 address stop monitor dns */
                    is_dnsr_nfq = 0;
                }else if((tmp = strstr(query+host_name_offset, ":")) != NULL ){ 
                    /* remove the port */
                    *tmp = '\0';
                    is_dnsr_nfq = 2;
                }
                if( is_dnsr_nfq >= 1 && NULL != convert_url_to_hex_fmt(query+host_name_offset, hexUrl)){
                    strncat(hexUrl, "00",sizeof(hexUrl));
                    if(is_dnsr_nfq == 2 )
                        *tmp=':';
                    fprintf(fp, ":wan2lan_dnsr_nfqueue_%s - [0:0]\n", ins_num);
                    fprintf(fp, "-A wan2lan_dnsr_nfqueue -m string --algo kmp --hex-string \"|%s|\" -j wan2lan_dnsr_nfqueue_%s \n", hexUrl, ins_num);   
                    fprintf(fp, "-A wan2lan_dnsr_nfqueue_%s  -j MARK --set-mark 0x%x\n", ins_num, atoi(ins_num));
                    if(iptype == 4)
                        fprintf(fp, "-A wan2lan_dnsr_nfqueue_%s  -m limit --limit 1/minute --limit-burst 1 -j NFQUEUE " DNSR_GET_QUEUE_CONFIG "\n",ins_num); 
                    else
                        fprintf(fp, "-A wan2lan_dnsr_nfqueue_%s  -m limit --limit 1/minute --limit-burst 1 -j NFQUEUE " DNSV6R_GET_QUEUE_CONFIG "\n",ins_num); 
                }
            } 
#endif
            int within_policy_start_stop = determine_enforcement_schedule2(cron_fp, namespace);
            if (!within_policy_start_stop) continue;

            char drop_log[40];
            snprintf(drop_log, sizeof(drop_log), "LOG_SiteBlocked_%d_DROP", idx);
            fprintf(fp, ":%s - [0:0]\n", drop_log);
            fprintf(fp, "-A %s -m limit --limit 1/minute --limit-burst 1  -j LOG --log-prefix %s --log-level %d\n", drop_log, drop_log, syslog_level);
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
            fprintf(fp, "-A %s -j MARK --set-mark 0x%x\n", drop_log, atoi(ins_num));
            if(iptype==4){
                fprintf(fp, "-A %s -j NFQUEUE "HTTP_GET_QUEUE_CONFIG "\n", drop_log);
                fprintf(nat_fp, ":%s - [0:0]\n", drop_log);
                fprintf(nat_fp, "-A %s -m limit --limit 1/minute --limit-burst 1  -j LOG --log-prefix %s --log-level %d\n", drop_log, drop_log, syslog_level);
                //if(isHttps)
                //    fprintf(nat_fp, "-A %s -m tcp -p tcp -j REDIRECT --to-port %s\n\n", drop_log, PARCON_WALLED_GARDEN_HTTPS_PORT_SITEBLK);
                //else
                //    fprintf(nat_fp, "-A %s -m tcp -p tcp -j REDIRECT --to-port %s\n\n", drop_log, PARCON_WALLED_GARDEN_HTTP_PORT_SITEBLK);
            }else    
                fprintf(fp, "-A %s -j NFQUEUE "HTTPV6_GET_QUEUE_CONFIG "\n", drop_log);
#else
            fprintf(fp, "-A %s -j DROP\n", drop_log);
#endif
            if (strncasecmp(method, "URL", 3)==0)
            {
                int host_name_offset = 0;

                // Strip http:// or https:// from the beginning of the URL
                // string so that only the host name is passed in
                if (0 == strncmp(query, "http://", STRLEN_HTTP_URL_PREFIX)) {
                    host_name_offset = STRLEN_HTTP_URL_PREFIX;
                }
                else if (0 == strncmp(query, "https://", STRLEN_HTTPS_URL_PREFIX)) {
                    host_name_offset = STRLEN_HTTPS_URL_PREFIX;
                }

                char nstdPort[8] = {'\0'};
                char *pch;

                //We only need host name in url, so eliminate everything comes after '/'
                //This can also eliminate the '/' postfix in some url e.g. "www.google.com/"
                pch = strstr(query+host_name_offset, "/");
                if(pch != NULL)
                    *pch = '\0';

                enum urlType_t {TEXT_URL, IPv4_URL, IPv6_URL} urlType = IPv4_URL;
                char *urlStart = query + host_name_offset;
                int pos = 0, len = strlen(urlStart);

                if(*urlStart == '[')
                    urlType = IPv6_URL;
                else
                    while(pos < len)
                    {
                        if(urlStart[pos] == ':')
                            break;

                        if(!isdigit(urlStart[pos]) && urlStart[pos] != '.')
                        {
                            urlType = TEXT_URL;
                            break;
                        }

                        ++pos;
                    }

                if(urlType == IPv6_URL && iptype == 4)
                    continue;

                if(urlType == IPv4_URL && iptype == 6)
                    continue;

                if(urlType == IPv6_URL)
                    pch = strstr(query+host_name_offset, "]:");
                else
                    pch = strstr(query+host_name_offset, ":");

                if(pch != NULL)
                {
                    strncpy(nstdPort, urlType == IPv6_URL ? pch+2 : pch+1, sizeof(nstdPort));
                    if(urlType == IPv6_URL)
                        *(pch+1) = '\0';
                    else
                        *pch = '\0';

                    fprintf(fp, "-A lan2wan_pc_site -p tcp -m tcp --dport %s -m httphost --host \"%s:%s\" -j %s\n", nstdPort, query + host_name_offset, nstdPort, drop_log);
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
                    if(iptype == 4){
                        if(isHttps){
                            fprintf(nat_fp, "-A parcon_walled_garden -p tcp --dport %s -m set --match-set %s dst -m comment --comment \"host match %s \"  -j %s\n",\
                                    nstdPort, ins_num, query, drop_log);
                            fprintf(nat_fp, "-A %s -m tcp -p tcp -j REDIRECT --to-port %s\n\n", drop_log, PARCON_WALLED_GARDEN_HTTPS_PORT_SITEBLK);
                        }else{
                            fprintf(nat_fp, "-A parcon_walled_garden -p tcp --dport %s -m set --match-set %s dst -m comment --comment \"host match %s \"  -j %s\n",\
                                    nstdPort, ins_num, query, drop_log);
                            fprintf(nat_fp, "-A %s -m tcp -p tcp -j REDIRECT --to-port %s\n\n", drop_log, PARCON_WALLED_GARDEN_HTTP_PORT_SITEBLK);
                        }
                    }
                    
#endif
#if !defined(_COSA_BCM_MIPS_)
                    do_parcon_mgmt_lan2wan_pc_site_insertrule(fp, ruleIndex, nstdPort);
#endif
                }
                else
                {
                    fprintf(fp, "-A lan2wan_pc_site -p tcp -m tcp --dport 80 -m httphost --host \"%s\" -j %s\n", query + host_name_offset, drop_log);
                    fprintf(fp, "-A lan2wan_pc_site -p tcp -m tcp --dport 443 -m httphost --host \"%s\" -j %s\n", query + host_name_offset, drop_log);
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
                    if(iptype == 4)
                    {
                        fprintf(nat_fp, "-A parcon_walled_garden -p tcp --dport 80 -m set --match-set %s dst -m comment --comment \"host match %s \"  -j  %s\n", \
                                ins_num, query, drop_log);
                        fprintf(nat_fp, "-A parcon_walled_garden -p tcp --dport 443 -m set --match-set %s dst -m comment --comment \"host match %s \" -j  %s\n", \
                                ins_num, query, drop_log);
                        fprintf(nat_fp, "-A %s -m tcp -p tcp --dport 443 -j REDIRECT --to-port %s\n\n", drop_log, PARCON_WALLED_GARDEN_HTTPS_PORT_SITEBLK);
                        fprintf(nat_fp, "-A %s -m tcp -p tcp --dport 80 -j REDIRECT --to-port %s\n\n", drop_log, PARCON_WALLED_GARDEN_HTTP_PORT_SITEBLK);
                    }
#endif
                }

                block_url_by_ipaddr(fp, query + host_name_offset, drop_log, iptype, ins_num, nstdPort);
            }
            else if (strncasecmp(method, "KEYWD", 5)==0)
            {
                // consider the case that user input whole url.
                if(strstr(query, "://") != 0) {
                    fprintf(fp, "-A lan2wan_pc_site -m string --string \"%s\" --algo kmp --icase -j %s\n", strstr(query, "://") + 3, drop_log);
                } else {
                    fprintf(fp, "-A lan2wan_pc_site -m string --string \"%s\" --algo kmp --icase -j %s\n", query, drop_log);
                }
            }
        }
    }
   FIREWALL_DEBUG("Exiting do_parcon_mgmt_site_keywd\n"); 
    return(0);
}



#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
#define GUEST_ALLOW_LIST "/var/.guest_allow_list"
static void do_allowed_guest(FILE *natFp)
{
    FILE *allowList = fopen(GUEST_ALLOW_LIST, "r");
    char line[256];
    char *t;
    int len; 
   FIREWALL_DEBUG("Entering do_allowed_guest\n"); 
    if(!allowList)
        return;

    while(fgets(line, sizeof(line), allowList) != NULL) {
        if((t = strrchr(line, ',')) != NULL) {
            t++;
            len = strlen(t);
            if(t[len-1] == '\n')
                t[len-1] = '\0';
            fprintf(natFp, "-A guestnet_allow_list -s %s -j ACCEPT\n", t);
        }
    }
   FIREWALL_DEBUG("Exiting do_allowed_guest\n"); 
    return;
}

static void do_guestnet_walled_garden(FILE *natFp)
{
    do_allowed_guest(natFp);
   FIREWALL_DEBUG("Entering do_guestnet_walled_garden\n"); 
    fprintf(natFp, "-A guestnet_walled_garden -d %s -p tcp --dport 80 -j ACCEPT\n", guest_network_ipaddr);
    fprintf(natFp, "-A guestnet_walled_garden -d %s -p tcp --dport 443 -j ACCEPT\n", guest_network_ipaddr);
    fprintf(natFp, "-A guestnet_walled_garden -p tcp --dport 80 -j REDIRECT --to-port 28080\n");
    fprintf(natFp, "-A guestnet_walled_garden -p tcp --dport 443 -j REDIRECT --to-port 20443\n");
   FIREWALL_DEBUG("Exiting do_guestnet_walled_garden\n"); 
}
#endif

#ifdef CONFIG_BUILD_TRIGGER
/*
 *  Procedure     : do_prepare_port_range_triggers
 *  Purpose       : prepare the iptables-restore statements for triggers
 *  Parameters    :
 *     mangle_fp              : An open file that will be used for iptables-restore
 *     filter_fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
static int do_prepare_port_range_triggers(FILE *nat_fp, FILE *filter_fp)
#else
static int do_prepare_port_range_triggers(FILE *mangle_fp, FILE *filter_fp)
#endif
{
   FIREWALL_DEBUG("Entering do_prepare_port_range_triggers\n"); 
   int idx;
   int  rc;
   char namespace[MAX_NAMESPACE];
   char query[MAX_QUERY];

   int count;

   query[0] = '\0';
   rc = syscfg_get(NULL, "PortRangeTriggerCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto end_do_prepare_port_range_triggers;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto end_do_prepare_port_range_triggers;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "PortRangeTrigger_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }
  
      // is the rule enabled
      query[0] = '\0';
      rc = syscfg_get(namespace, "enabled", query, sizeof(query));
      if (0 != rc || '\0' == query[0]) {
         continue;
      } else if (0 == strcmp("0", query)) {
        continue;
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
      } else {
         isTriggerMonitorRestartNeeded = 1;
#endif
      }

      // what is the trigger id
      char id[10];
      id[0] = '\0';
      rc = syscfg_get(namespace, "trigger_id", id, sizeof(id));
      if (0 != rc || '\0' == id[0]) {
         continue;
      } 

      // what is the triggering protocol
      char prot[10];
      prot[0] = '\0';
      rc = syscfg_get(namespace, "trigger_protocol", prot, sizeof(prot));
      if (0 != rc || '\0' == prot[0]) {
         snprintf(prot, sizeof(prot), "%s", "both");
      }

      // what is the triggering port range
      char portrange[30];
      char sdport[10];
      char edport[10];
      portrange[0]= '\0';
      sdport[0]   = '\0';
      edport[0]   = '\0';
      rc = syscfg_get(namespace, "trigger_range", portrange, sizeof(portrange));
      if (0 != rc || '\0' == portrange[0]) {
         continue;
      } else {
         int r = 0;
         if (2 != (r = sscanf(portrange, "%10s %10s", sdport, edport))) {
            if (1 == r) {
               snprintf(edport, sizeof(edport), "%s", sdport);
            } else {
               continue;
            }
         }
      }

#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
      // what is the forward protocol
      char fprot[10];
      fprot[0] = '\0';
      rc = syscfg_get(namespace, "forward_protocol", fprot, sizeof(fprot));
      if (0 != rc || '\0' == fprot[0]) {
         snprintf(fprot, sizeof(fprot), "%s", "both");
      }

      // what is the forwarding port range
      char sfport[10];
      char efport[10];
      portrange[0]= '\0';
      sfport[0]   = '\0';
      efport[0]   = '\0';
      rc = syscfg_get(namespace, "forward_range", portrange, sizeof(portrange));
      if (0 != rc || '\0' == portrange[0]) {
         continue;
      } else {
         int r = 0;
         if (2 != (r = sscanf(portrange, "%10s %10s", sfport, efport))) {
            if (1 == r) {
               snprintf(efport, sizeof(efport), "%s", sfport);
            } else {
               continue;
            }
         }
      }
#endif

      if (0 == strcmp("both", prot) || 0 == strcmp("tcp", prot)) {
         char str[MAX_QUERY];
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
         snprintf(str, sizeof(str), 
                  "-A prerouting_fromlan_trigger -p tcp -m tcp --dport %s:%s -j TRIGGER --trigger-type out --trigger-proto %s --trigger-match %s:%s --trigger-relate %s:%s",
                      sdport, edport, fprot, sdport, edport, sfport, efport);
         fprintf(nat_fp, "%s\n", str);
         snprintf(str, sizeof(str), 
                  "-A lan2wan_triggers -p tcp -m tcp --dport %s:%s -j xlog_accept_lan2wan",
                      sdport, edport);
         fprintf(filter_fp, "%s\n", str);
         snprintf(str, sizeof(str), 
                  "-A lan2wan_triggers -p tcp -m tcp --sport %s:%s -j xlog_accept_lan2wan",
                      sfport, efport);
         fprintf(filter_fp, "%s\n", str);

#else
         snprintf(str, sizeof(str), 
                  "-A prerouting_trigger -p tcp -m tcp --dport %s:%s -j MARK --set-mark %d",
                      sdport, edport, atoi(id));
         fprintf(mangle_fp, "%s\n", str);

         snprintf(str, sizeof(str), 
                  "-A lan2wan_triggers -p tcp -m tcp --dport %s:%s -j NFQUEUE --queue-num 22",
                      sdport, edport);
         fprintf(filter_fp, "%s\n", str);
#endif
      }
  
      if (0 == strcmp("both", prot) || 0 == strcmp("udp", prot)) {
         char str[MAX_QUERY];
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
         snprintf(str, sizeof(str), 
                  "-A prerouting_fromlan_trigger -p udp -m udp --dport %s:%s -j TRIGGER --trigger-type out --trigger-proto %s --trigger-match %s:%s --trigger-relate %s:%s",
                      sdport, edport, fprot, sdport, edport, sfport, efport);
         fprintf(nat_fp, "%s\n", str);
         snprintf(str, sizeof(str), 
                  "-A lan2wan_triggers -p udp -m udp --dport %s:%s -j xlog_accept_lan2wan",
                      sdport, edport);
         fprintf(filter_fp, "%s\n", str);
         snprintf(str, sizeof(str), 
                  "-A lan2wan_triggers -p udp -m udp --sport %s:%s -j xlog_accept_lan2wan",
                      sfport, efport);
         fprintf(filter_fp, "%s\n", str);

#else
         snprintf(str, sizeof(str), 
                  "-A prerouting_trigger -p udp -m udp --dport %s:%s -j MARK --set-mark %d",
                      sdport, edport, atoi(id));
         fprintf(mangle_fp, "%s\n", str);

         snprintf(str, sizeof(str), 
                   "-A lan2wan_triggers -p udp -m udp --dport %s:%s -j NFQUEUE --queue-num 22",
                      sdport, edport);
         fprintf(filter_fp, "%s\n", str);
#endif
      }

#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
      if (0 == strcmp("both", fprot) || 0 == strcmp("tcp", fprot)) {
         char str[MAX_QUERY];
         snprintf(str, sizeof(str), 
                  "-A prerouting_fromwan_trigger -p tcp -m tcp --dport %s:%s -j TRIGGER --trigger-type dnat", sfport, efport);
         fprintf(nat_fp, "%s\n", str);
         snprintf(str, sizeof(str), 
                  "-A wan2lan_trigger -p tcp -m tcp --dport %s:%s -j TRIGGER --trigger-type in", sfport, efport);
         fprintf(filter_fp, "%s\n", str);
      }
      if (0 == strcmp("both", fprot) || 0 == strcmp("udp", fprot)) {
         char str[MAX_QUERY];
         snprintf(str, sizeof(str), 
                  "-A prerouting_fromwan_trigger -p udp -m udp --dport %s:%s -j TRIGGER --trigger-type dnat", sfport, efport);
         fprintf(nat_fp, "%s\n", str);
         snprintf(str, sizeof(str), 
                  "-A wan2lan_trigger -p udp -m udp --dport %s:%s -j TRIGGER --trigger-type in", sfport, efport);
         fprintf(filter_fp, "%s\n", str);
      }
#endif
   }

end_do_prepare_port_range_triggers:
   FIREWALL_DEBUG("Exiting do_prepare_port_range_triggers\n"); 
   return(0);
}
#endif // CONFIG_BUILD_TRIGGER

/*
 *  Procedure     : prepare_host_detect
 *  Purpose       : prepare the iptables-restore statements for detecting when new hosts join the lan
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 */
static int prepare_host_detect(FILE * fp)
{
   FIREWALL_DEBUG("Entering prepare_host_detect\n"); 
   if (isLanHostTracking || isDMZbyMAC) {
      char str[MAX_QUERY];
      /*
       * add all known hosts and have them be accepted, but if not, then the last statement is the new host
       */
      FILE *kh_fp = fopen(lan_hosts_dir"/"hosts_filename, "r");
      char buf[1024];
      if (NULL != kh_fp) { 
         while (NULL != fgets(buf, sizeof(buf), kh_fp)) {
            char ip[20];
            char mac[20];
            sscanf(buf, "%20s %20s", ip, mac);
            snprintf(str, sizeof(str),
                     // "-A host_detect -i %s -s %s -m state --state NEW -j RETURN", lan_ifname, ip);
                     "-A host_detect -i %s -s %s -j RETURN", lan_ifname, ip);
           fprintf(fp, "%s\n", str);
         }
         fclose(kh_fp);
      }
      snprintf(str, sizeof(str),
               "-A host_detect -m state --state NEW -j LOG --log-level 1 --log-prefix \"%s.NEWHOST \" --log-tcp-options --log-ip-options -m limit --limit 1/minute --limit-burst 1", 
                 LOG_TRIGGER_PREFIX);
      fprintf(fp, "%s\n", str);
   }
   FIREWALL_DEBUG("Exiting prepare_host_detect\n"); 
   return(0);
}

#ifdef OBSOLETE
/*
 *  Procedure     : prepare_lan_bandwidth_tracking
 *  Purpose       : prepare the iptables-restore statements for tracking bandwidth usage of hosts
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 * Note:   This will place a file in cron.everyminute
 * Note:   This is not efficient, and also no longer necessary
 *         iptables-restore -c maintains the counters, so it is not
 *         necessary to run a script every minute to save the counters
 */
static int prepare_lan_bandwidth_tracking(FILE *fp)
{
   FIREWALL_DEBUG("Entering prepare_lan_bandwidth_tracking\n"); 
   int hosts = 0;
   FILE *kh_fp = fopen(lan_hosts_dir"/"hosts_filename, "r");
   char buf[1024];
   if (NULL != kh_fp) {
      while (NULL != fgets(buf, sizeof(buf), kh_fp)) {
         char ip[20];
         char mac[20];
         sscanf(buf, "%20s %20s", ip, mac);
         char str[MAX_QUERY];
         snprintf(str, sizeof(str),
             ":bandwidth_%s - [0:0]", ip);
         fprintf(fp, "%s\n", str);
         snprintf(str, sizeof(str),
               "-N bandwidth_%s", ip); 

         snprintf(str, sizeof(str),
                  "-A bandwidth_%s -j RETURN", ip);
         fprintf(fp, "%s\n", str);

         snprintf(str, sizeof(str),
                  "-A lan2wan_bandwidth -s %s -o %s -j bandwidth_%s", ip, current_wan_ifname, ip);
         fprintf(fp, "%s\n", str);

         hosts++;
      }
      fclose(kh_fp);
   }
   FIREWALL_DEBUG("Exiting prepare_lan_bandwidth_tracking\n"); 
   return(0);
}
#endif
/*
 *  Procedure     : do_lan2wan_misc
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  IoT firewall rules pertaining to traffic
 *                  from the lan to the wan
 *  Parameters    :
 *    filter_fp             : An open file to write lan2wan rules to
 * Return Values  :
 *    0              : Success
 */
static int do_lan2wan_IoT_Allow(FILE *filter_fp)
{
   FIREWALL_DEBUG("Entering do_lan2wan_IoT_Allow\n"); 
   /*
    * if the wan is currently unavailable, then drop any packets from lan to wan
    */
      fprintf(filter_fp, "-A lan2wan_iot_allow -p udp --dport 53 -j ACCEPT\n");
      fprintf(filter_fp, "-A lan2wan_iot_allow -d ntp01.cmc.co.denver.comcast.net -j ACCEPT\n");
      fprintf(filter_fp, "-A lan2wan_iot_allow -d ntp.ccp.xcal.tv -j ACCEPT\n");
      //fprintf(filter_fp, "-A lan2wan_iot_allow -d cpentp.services.cox.net -j ACCEPT\n");
      //fprintf(filter_fp, "-A lan2wan_iot_allow -d cpentp.services.coxlab.net -j ACCEPT\n");
      fprintf(filter_fp, "-A lan2wan_iot_allow -d fkps.ccp.xcal.tv -j ACCEPT\n");
      fprintf(filter_fp, "-A lan2wan_iot_allow -d xacs.ccp.xcal.tv -j ACCEPT\n");
      //fprintf(filter_fp, "-A lan2wan_iot_allow -d x1.xcal.tv -j ACCEPT\n");
      fprintf(filter_fp, "-A lan2wan_iot_allow -d decider.r53.xcal.tv -j ACCEPT\n");
      fprintf(filter_fp, "-A lan2wan_iot_allow -j REJECT\n");
   FIREWALL_DEBUG("Exiting do_lan2wan_IoT_Allow\n"); 
   return(0);
}

//zqiu:R5337
static int do_wan2lan_IoT_Allow(FILE *filter_fp)
{
   FIREWALL_DEBUG("Entering do_wan2lan_IoT_Allow\n"); 
      //Low firewall
      fprintf(filter_fp, "-A wan2lan_iot_allow -p tcp --dport 113 -j RETURN\n"); // IDENT
      fprintf(filter_fp, "-A wan2lan_iot_allow -j ACCEPT\n");
   FIREWALL_DEBUG("Exiting do_wan2lan_IoT_Allow\n"); 
   return(0);
}

/*
 *  Procedure     : do_lan2wan_disable
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic
 *                  from the lan to the wan for disable case 
 *  Parameters    :
 *    filter_fp             : An open file to write lan2wan rules to
 * Return Values  :
 *    0              : Success
 */
static void do_lan2wan_disable(FILE *filter_fp)
{
   char str[MAX_QUERY];
   FIREWALL_DEBUG("Entering do_lan2wan_disable\n"); 
   /* if nat is disable or
     * wan is not ready or
     * nat is not ready
     * all private packet from lan to wan should be blocked
     * public packet should be allowed 
     */
    if(!isNatReady){
         snprintf(str, sizeof(str),
                  "-A lan2wan_disable -s %s/%s -j DROP", lan_ipaddr, lan_netmask);
         fprintf(filter_fp, "%s\n", str);
    }
   FIREWALL_DEBUG("Exiting do_lan2wan_disable\n"); 
}

/*
 *  Procedure     : do_lan2wan_misc
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic
 *                  from the lan to the wan for misc cases
 *  Parameters    :
 *    filter_fp             : An open file to write lan2wan rules to
 * Return Values  :
 *    0              : Success
 */
static int do_lan2wan_misc(FILE *filter_fp)
{
   char str[MAX_QUERY];
   FIREWALL_DEBUG("Entering do_lan2wan_misc\n");
   /*
    * if the wan is currently unavailable, then drop any packets from lan to wan
    */
   if (!isWanReady) {
      snprintf(str, sizeof(str),
               "-I lan2wan_misc 1 -j DROP");
      fprintf(filter_fp, "%s\n", str);
   }
   char mtu[26];
   int tcp_mss_limit;
   if ( 0 == sysevent_get(sysevent_fd, sysevent_token, "ppp_clamp_mtu", mtu, sizeof(mtu)) ) {
      if ('\0' != mtu[0] && 0 != strncmp("0", mtu, sizeof(mtu)) ) {
         tcp_mss_limit=atoi(mtu) + 1;
         snprintf(str, sizeof(str),
                  "-A lan2wan_misc -p tcp --tcp-flags SYN,RST SYN -m tcpmss --mss %d: -j TCPMSS --set-mss %s", tcp_mss_limit, mtu);
         fprintf(filter_fp, "%s\n", str);
      }
   }

#if defined(CONFIG_CCSP_VPN_PASSTHROUGH)
   char query[2] = {'\0'};

   if(isWanReady) {
       if((0==syscfg_get(NULL, "IPSecPassthrough", query, sizeof(query))) && (atoi(query)==0))
           fprintf(filter_fp, "-A lan2wan_misc -p udp --dport 500  -j DROP\n"); // block IPSec

       query[0] = '\0';
       if((0==syscfg_get(NULL, "PPTPPassthrough", query, sizeof(query))) && (atoi(query)==0))
           fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 1723 -j DROP\n"); // block PPTP
   }
#endif

   if (isWanReady && strncasecmp(firewall_level, "High", strlen("High")) == 0)
   {
      // enforce high security - per requirement
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 80   -j RETURN\n"); // HTTP
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 443  -j RETURN\n"); // HTTPS
      fprintf(filter_fp, "-A lan2wan_misc -p udp --dport 53   -j RETURN\n"); // DNS
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 53   -j RETURN\n"); // DNS
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 119  -j RETURN\n"); // NTP
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 123  -j RETURN\n"); // NTP
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 25   -j RETURN\n"); // EMAIL
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 110  -j RETURN\n"); // EMAIL
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 143  -j RETURN\n"); // EMAIL
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 465  -j RETURN\n"); // EMAIL
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 587  -j RETURN\n"); // EMAIL
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 993  -j RETURN\n"); // EMAIL
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 995  -j RETURN\n"); // EMAIL
      fprintf(filter_fp, "-A lan2wan_misc -p gre              -j RETURN\n"); // GRE
      fprintf(filter_fp, "-A lan2wan_misc -p udp --dport 500  -j RETURN\n"); // VPN
      //zqiu>> cisco vpn
      fprintf(filter_fp, "-A lan2wan_misc -p udp --dport 4500  -j RETURN\n"); // VPN
      fprintf(filter_fp, "-A lan2wan_misc -p udp --dport 62515  -j RETURN\n"); // VPN
      //zqiu<<
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 1723 -j RETURN\n"); // VPN
      fprintf(filter_fp, "-A lan2wan_misc -p tcp --dport 3689 -j RETURN\n"); // ITUNES
      fprintf(filter_fp, "-A lan2wan_misc -m state --state RELATED,ESTABLISHED -j RETURN\n");
      fprintf(filter_fp, "-A lan2wan_misc -j DROP\n");
   }
   FIREWALL_DEBUG("Exiting do_lan2wan_misc\n");
   return(0);
}

/*
 *  Procedure     : do_lan2wan
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic
 *                  from the lan to the wan
 *  Parameters    :
 *    mangle_fp             : An open file to write lan2wan rules to
 *    filter_fp             : An open file to write lan2wan rules to
 * Return Values  :
 *    0              : Success
 */
static int do_lan2wan(FILE *mangle_fp, FILE *filter_fp, FILE *nat_fp)
{
   FIREWALL_DEBUG("Entering do_lan2wan\n");
   do_lan2wan_misc(filter_fp);
   //do_lan2wan_IoT_Allow(filter_fp);
   //Not used in USGv2
   //do_lan2wan_webfilters(filter_fp);
   //do_lan_access_restrictions(filter_fp, nat_fp);
   do_lan2wan_disable(filter_fp);
   do_parental_control(filter_fp, nat_fp, 4);
   #ifdef CISCO_CONFIG_TRUE_STATIC_IP
   do_lan2wan_staticip(filter_fp);
   #endif
#ifdef CONFIG_BUILD_TRIGGER
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   do_prepare_port_range_triggers(nat_fp, filter_fp);
#else
   do_prepare_port_range_triggers(mangle_fp, filter_fp);
#endif
#endif

   if (isLanHostTracking || isDMZbyMAC) {
      prepare_host_detect(filter_fp);
   }

#ifdef OBSOLETE
   if (isLanHostTracking) {
      prepare_lan_bandwidth_tracking(filter_fp);
   }
#endif

   FIREWALL_DEBUG("Exiting do_lan2wan\n"); 

   return(0);
}



static void add_usgv2_wan2lan_general_rules(FILE *fp)
{
   FIREWALL_DEBUG("Entering add_usgv2_wan2lan_general_rules\n"); 
    fprintf(fp, "-A wan2lan_misc -m state --state RELATED,ESTABLISHED -j ACCEPT\n");

    if (strncasecmp(firewall_level, "High", strlen("High")) == 0) {
        
        fprintf(fp, "-A wan2lan_misc -j xlog_drop_wan2lan\n");

    } else if (strncasecmp(firewall_level, "Medium", strlen("Medium")) == 0) {

        fprintf(fp, "-A wan2lan_misc -p tcp --dport 113 -j xlog_drop_wan2lan\n"); // IDENT
        fprintf(fp, "-A wan2lan_misc -p icmp --icmp-type 8 -j xlog_drop_wan2lan\n"); // ICMP PING

        fprintf(fp, "-A wan2lan_misc -p tcp --dport 1214 -j xlog_drop_wan2lan\n"); // Kazaa
        fprintf(fp, "-A wan2lan_misc -p udp --dport 1214 -j xlog_drop_wan2lan\n"); // Kazaa
        fprintf(fp, "-A wan2lan_misc -p tcp --dport 6881:6999 -j xlog_drop_wan2lan\n"); // Bittorrent
        fprintf(fp, "-A wan2lan_misc -p tcp --dport 6346 -j xlog_drop_wan2lan\n"); // Gnutella
        fprintf(fp, "-A wan2lan_misc -p udp --dport 6346 -j xlog_drop_wan2lan\n"); // Gnutella
        fprintf(fp, "-A wan2lan_misc -p tcp --dport 49152:65534 -j xlog_drop_wan2lan\n"); // Vuze

    } else if (strncasecmp(firewall_level, "Low", strlen("Low")) == 0) {

        fprintf(fp, "-A wan2lan_misc -p tcp --dport 113 -j xlog_drop_wan2lan\n"); // IDENT

    } else if (strncasecmp(firewall_level, "Custom", strlen("Custom")) == 0) {
        
        if (isHttpBlocked) {
            fprintf(fp, "-A wan2lan_misc -p tcp --dport 80 -j xlog_drop_wan2lan\n"); // HTTP
            fprintf(fp, "-A wan2lan_misc -p tcp --dport 443 -j xlog_drop_wan2lan\n"); // HTTPS
        }

        if (isIdentBlocked) {
            fprintf(fp, "-A wan2lan_misc -p tcp --dport 113 -j xlog_drop_wan2lan\n"); // IDENT
        }

        if (isPingBlocked) {
            fprintf(fp, "-A wan2lan_misc -p icmp --icmp-type 8 -j xlog_drop_wan2lan\n"); // ICMP PING
        }

        if (isP2pBlocked) {
            fprintf(fp, "-A wan2lan_misc -p tcp --dport 1214 -j xlog_drop_wan2lan\n"); // Kazaa
            fprintf(fp, "-A wan2lan_misc -p udp --dport 1214 -j xlog_drop_wan2lan\n"); // Kazaa
            fprintf(fp, "-A wan2lan_misc -p tcp --dport 6881:6999 -j xlog_drop_wan2lan\n"); // Bittorrent
            fprintf(fp, "-A wan2lan_misc -p tcp --dport 6346 -j xlog_drop_wan2lan\n"); // Gnutella
            fprintf(fp, "-A wan2lan_misc -p udp --dport 6346 -j xlog_drop_wan2lan\n"); // Gnutella
            fprintf(fp, "-A wan2lan_misc -p tcp --dport 49152:65534 -j xlog_drop_wan2lan\n"); // Vuze
        }

        if(isMulticastBlocked) {
            fprintf(fp, "-A wan2lan_misc -p 2 -j xlog_drop_wan2lan\n"); // IGMP
        }
    }
   FIREWALL_DEBUG("Exiting add_usgv2_wan2lan_general_rules\n"); 
}

/*
 ==========================================================================
                     wan2lan
 ==========================================================================
 */

/*
 *  Procedure     : do_wan2lan_misc
 *  Purpose       : prepare the iptables-restore statements for forwarding incoming packets to a lan host
 *  Parameters    : 
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int do_wan2lan_misc(FILE *fp) 
{
   if (NULL == fp) {
      return(-1);
   }
   FIREWALL_DEBUG("Entering do_wan2lan_misc\n"); 
   add_usgv2_wan2lan_general_rules(fp);
   /*
    * syscfg tuple W2LFirewallRule_, where x is a digit
    * keeps track of the syscfg namespace of a user defined forwarding rule.
    * We iterate through these tuples until we dont find an instance in syscfg.
    */ 
   int idx;
   int  rc;
   char namespace[MAX_NAMESPACE];
   char query[MAX_QUERY];

   int count;

   query[0] = '\0';
   rc = syscfg_get(NULL, "W2LFirewallRuleCount", query, sizeof(query));
   if (0 != rc || '\0' == query[0]) {
      goto FirewallRuleNext;
   } else {
      count = atoi(query);
      if (0 == count) {
         goto FirewallRuleNext;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   for (idx=1 ; idx<=count ; idx++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "W2LFirewallRule_%d", idx);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == query[0]) {
         continue;
      } 

      char match[MAX_QUERY];
      match[0] = '\0';
      rc = syscfg_get(namespace, "match", match, sizeof(match));
      if (0 != rc || '\0' == match[0]) {
         continue;
      } 
      char result[26];
      result[0] = '\0';
      rc = syscfg_get(namespace, "result", result, sizeof(result));
      if (0 != rc || '\0' == result[0]) {
         continue;
      } 

      char subst[MAX_QUERY];
      char str[MAX_QUERY];
      snprintf(str, sizeof(str),
               "-A wan2lan_misc %s -j %s", 
               match, make_substitutions(result, subst, sizeof(subst)));
      fprintf(fp, "%s\n", str);

   }
FirewallRuleNext:

   count = 0;
   /*
    * syscfg tuple W2LWellKnownFirewallRule_x, where x is a digit
    * keeps track of the syscfg namespace of a user defined forwarding rule.
    * We iterate through these tuples until we dont find an instance in syscfg.
    */ 

   char *filename = otherservices_dir"/"otherservices_file;
   FILE *os_fp = fopen(filename, "r");
   if (NULL != os_fp) {

      query[0] = '\0';
      rc = syscfg_get(NULL, "W2LWellKnownFirewallRuleCount", query, sizeof(query));
      if (0 != rc || '\0' == query[0]) {
         goto FirewallRuleNext2;
      } else {
         count = atoi(query);
         if (0 == count) {
            goto FirewallRuleNext2;
         }
         if (MAX_SYSCFG_ENTRIES < count) {
            count = MAX_SYSCFG_ENTRIES;
         }
      }

      for (idx=1 ; idx<=count ; idx++) {
         namespace[0] = '\0';
         snprintf(query, sizeof(query), "W2LWellKnownFirewallRule_%d", idx);
         rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
         if (0 != rc || '\0' == namespace[0]) {
            continue;
         } 

         char name[56];
         name[0] = '\0';
         rc = syscfg_get(namespace, "name", name, sizeof(name));
         if (0 != rc || '\0' == name[0]) {
            continue;
         } 
         char result[26];
         result[0] = '\0';
         rc = syscfg_get(namespace, "result", result, sizeof(result));
         if (0 != rc || '\0' == result[0]) {
            continue;
         } 

         /*
          * look up the rules for this service
          */
         rewind(os_fp);
         char line[512];
         char *next_token;

         while (NULL != (next_token = match_keyword(os_fp, name, '|', line, sizeof(line))) ) {
            char *friendly_name = next_token;

            next_token = token_get(friendly_name, '|');
            if (NULL == next_token || NULL == friendly_name) {
               continue;
            }

            char *match = next_token;
            next_token = token_get(match, '|');
            if (NULL == match) {
               continue;
            }

            char subst[MAX_QUERY];
            /*
             * The wan2lan iptables chain contains packets from wan destined to lan.
             * The wan2lan chain is linked to from the FORWARD chain
             */
            char str[MAX_QUERY];
            snprintf(str, sizeof(str),
                     "-A wan2lan_misc %s -j %s", match, make_substitutions(result, subst, sizeof(subst)));
            fprintf(fp, "%s\n", str);
         }
      }
FirewallRuleNext2:

      fclose(os_fp);
   }

  // mtu clamping
   char mtu[26];
   int tcp_mss_limit;
   if ( 0 == sysevent_get(sysevent_fd, sysevent_token, "ppp_clamp_mtu", mtu, sizeof(mtu)) ) {
     char str[MAX_QUERY];
      if ('\0' != mtu[0] && 0 != strncmp("0", mtu, sizeof(mtu)) ) {
         tcp_mss_limit=atoi(mtu) + 1;
         snprintf(str, sizeof(str),
                  "-A wan2lan_misc -p tcp --tcp-flags SYN,RST SYN -m tcpmss --mss %d: -j TCPMSS --set-mss %s", tcp_mss_limit, mtu);
         fprintf(fp, "%s\n", str);
      }
   }

   FIREWALL_DEBUG("Exiting do_wan2lan_misc\n"); 
   return(0);
}



/*
 *  Procedure     : do_wan2lan_disabled
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic
 *                  from the wan to the lan for the case where lan2wan traffic is disabled
 *  Parameters    :
 *    fp             : An open file to write wan2lan rules to
 * Return Values  :
 *    0              : Success
 */
static int do_wan2lan_disabled(FILE *fp)
{
   FIREWALL_DEBUG("Entering do_wan2lan_disabled\n"); 
   char str[MAX_QUERY];

   /*
    * if the wan is currently unavailable, then drop any packets from wan to lan
    */
   if (!isNatReady ) {
      snprintf(str, sizeof(str),
               "-A wan2lan_disabled -d %s/%s -j DROP\n", lan_ipaddr, lan_netmask);
      fprintf(fp, "%s\n", str);
   }
   FIREWALL_DEBUG("Exiting do_wan2lan_disabled\n"); 
   return(0);
}

/*
 *  Procedure     : do_wan2lan_accept
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic
 *                  from the wan to the lan for which we are allowing
 *  Parameters    :
 *    fp             : An open file to write wan2lan rules to
 * Return Values  :
 *    0              : Success
 */
static int do_wan2lan_accept(FILE *fp)
{

   char str[MAX_QUERY];
   FIREWALL_DEBUG("Entering do_wan2lan_accept\n"); 

   if (!isMulticastBlocked) {
      // accept multicast from our wan
      snprintf(str, sizeof(str),
         "-A wan2lan_accept -p udp -m udp --destination 224.0.0.0/4 -j ACCEPT");
      fprintf(fp, "%s\n", str);
   }
   FIREWALL_DEBUG("Exiting do_wan2lan_accept\n"); 
   return(0);
}

#ifdef CISCO_CONFIG_TRUE_STATIC_IP
/*
 *  Procedure     : do_wan2lan_staticip
 *  Purpose       : Accept or deny static ip subnet packet from wan to lan
 *  Parameters    :
 *    fp             : An open file to write wan2lan rules to
 * Return Values  :
 *    0              : Success
 */
static void do_wan2lan_staticip(FILE *filter_fp)
{
    int i;
   FIREWALL_DEBUG("Entering do_wan2lan_staticip\n"); 	  
    if(isWanStaticIPReady && isFWTS_enable){
        for(i = 0; i < StaticIPSubnetNum; i++){
            fprintf(filter_fp, "-A wan2lan_staticip -d %s/%s -j ACCEPT\n", StaticIPSubnet[i].ip, StaticIPSubnet[i].mask);
        }
    }
    
    for(i = 0; i < StaticIPSubnetNum; i++){
        fprintf(filter_fp, "-A wan2lan_staticip_post -d %s/%s -j ACCEPT\n", StaticIPSubnet[i].ip, StaticIPSubnet[i].mask);
    }
   FIREWALL_DEBUG("Exiting do_wan2lan_staticip\n"); 	  
}

//Add True Static IP port mgmt rules
#define PT_MGMT_PREFIX "tsip_pm_"
static void do_wan2lan_tsip_pm(FILE *filter_fp)
{
    int rc, i, count;
    char query[MAX_QUERY], countStr[16], utKey[64];
    char startIP[sizeof("255.255.255.255")], endIP[sizeof("255.255.255.255")];
    char startPort[sizeof("65535")], endPort[sizeof("65535")];
    unsigned char type;
   FIREWALL_DEBUG("Entering do_wan2lan_tsip_pm\n"); 	  
    query[0] = '\0';
    rc = syscfg_get(NULL, PT_MGMT_PREFIX"enabled", query, sizeof(query));

    //if the key tsip_pm_enabled doesn't exist, e.g. rc != 0 we treat it as enabled.
    if ((rc == 0 && atoi(query) != 1) || !isWanStaticIPReady) { //key exists and the value is 0 or tsi not ready
        return ;
    }

    query[0] = '\0';
    rc = syscfg_get(NULL, PT_MGMT_PREFIX"type", query, sizeof(query));

    if (rc != 0)
        type = 0; //if the key doesn't exist, the default mode is white list
    else if(strcmp("white", query) == 0)
        type = 0;
    else if(strcmp("black", query) == 0)
        type = 1;

    rc = syscfg_get(NULL, PT_MGMT_PREFIX"count", countStr, sizeof(countStr));
    if(rc == 0)
        count = strtoul(countStr, NULL, 10);
    else
        count = 0;
    
    //allow the return traffic of lan initiated traffic
    fprintf(filter_fp, "-A wan2lan_staticip_pm -p tcp -m state --state ESTABLISHED -j ACCEPT\n");
    fprintf(filter_fp, "-A wan2lan_staticip_pm -p udp -m state --state ESTABLISHED -j ACCEPT\n");

    for(i = 0; i < count; i++) {
        snprintf(utKey, sizeof(utKey), PT_MGMT_PREFIX"%u_enabled", i);
        query[0] = '\0';
        rc = syscfg_get(NULL, utKey, query, sizeof(query));
        if(rc != 0 || atoi(query) != 1)
            continue;

        snprintf(utKey, sizeof(utKey), PT_MGMT_PREFIX"%u_protocol", i);
        query[0] = '\0';
        syscfg_get(NULL, utKey, query, sizeof(query));
        
        snprintf(utKey, sizeof(utKey), PT_MGMT_PREFIX"%u_startIP", i);
        syscfg_get(NULL, utKey, startIP, sizeof(startIP));
        snprintf(utKey, sizeof(utKey), PT_MGMT_PREFIX"%u_endIP", i);
        syscfg_get(NULL, utKey, endIP, sizeof(endIP));
        
        snprintf(utKey, sizeof(utKey), PT_MGMT_PREFIX"%u_startPort", i);
        syscfg_get(NULL, utKey, startPort, sizeof(startPort));
        snprintf(utKey, sizeof(utKey), PT_MGMT_PREFIX"%u_endPort", i);
        syscfg_get(NULL, utKey, endPort, sizeof(endPort));

        if(strcmp("tcp", query) == 0 || strcmp("both", query) == 0)
            fprintf(filter_fp, "-A wan2lan_staticip_pm -p tcp -m tcp --dport %s:%s -m iprange --dst-range %s-%s -j %s\n", startPort, endPort, startIP, endIP, type == 0 ? "ACCEPT" : "DROP");
        if(strcmp("udp", query) == 0 || strcmp("both", query) == 0)
            fprintf(filter_fp, "-A wan2lan_staticip_pm -p udp -m udp --dport %s:%s -m iprange --dst-range %s-%s -j %s\n", startPort, endPort, startIP, endIP, type == 0 ? "ACCEPT" : "DROP");
    }

    for(i = 0; i < StaticIPSubnetNum; i++) {
        fprintf(filter_fp, "-A wan2lan_staticip_pm -p tcp -d %s/%s -j %s\n", StaticIPSubnet[i].ip, StaticIPSubnet[i].mask, type == 0 ? "DROP" : "ACCEPT");
        fprintf(filter_fp, "-A wan2lan_staticip_pm -p udp -d %s/%s -j %s\n", StaticIPSubnet[i].ip, StaticIPSubnet[i].mask, type == 0 ? "DROP" : "ACCEPT");
    }
   FIREWALL_DEBUG("Exiting do_wan2lan_tsip_pm\n"); 	  
}
#endif

/*
 *  Procedure     : do_wan2lan
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules pertaining to traffic
 *                  from the wan to the lan
 *  Parameters    :
 *    fp             : An open file to write wan2lan rules to
 * Return Values  :
 *    0              : Success
 */
static int do_wan2lan(FILE *fp)
{
   FIREWALL_DEBUG("Entering do_wan2lan\n"); 	  
   do_wan2lan_disabled(fp);
   do_wan2lan_misc(fp);
   do_wan2lan_accept(fp);
   #ifdef CISCO_CONFIG_TRUE_STATIC_IP
   do_wan2lan_staticip(fp);
   do_wan2lan_tsip_pm(fp);
   #endif
   FIREWALL_DEBUG("Exiting do_wan2lan\n"); 	  
   return(0);
}

/*
 ==========================================================================
              Ephemeral filter rules
 ==========================================================================
 */

/*
 *  Procedure     : do_filter_table_general_rules
 *  Purpose       : prepare the iptables-restore statements for syscfg/syseventstatements that are applied directly 
 *                  to the filter table 
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 */
static int do_filter_table_general_rules(FILE *fp)
{
   // add rules from syscfg
   int  idx;
   char rule_query[MAX_QUERY];
   int  count = 1;

   char      rule[MAX_QUERY];
   char      in_rule[MAX_QUERY];
   char      subst[MAX_QUERY];
   FIREWALL_DEBUG("Entering do_filter_table_general_rules\n"); 	  
   in_rule[0] = '\0';
   syscfg_get(NULL, "GeneralPurposeFirewallRuleCount", in_rule, sizeof(in_rule));
   if ('\0' == in_rule[0]) {
      goto GPFirewallRuleNext;
   } else {
      count = atoi(in_rule);
      if (0 == count) {
         goto GPFirewallRuleNext;
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   memset(in_rule, 0, sizeof(in_rule));
   for (idx=1; idx<=count; idx++) {
      snprintf(rule_query, sizeof(rule_query), "GeneralPurposeFirewallRule_%d", idx);
      syscfg_get(NULL, rule_query, in_rule, sizeof(in_rule));
      if ('\0' == in_rule[0]) {
         continue;
      } else {
         /*
          * the rule we just got could contain variables that we need to substitute
          * for runtime/configuration values
          */
         char str[MAX_QUERY];
         if (NULL != make_substitutions(in_rule, subst, sizeof(subst))) {
            if ((1 == substitute(subst, str, sizeof(str), "INPUT", "general_input")) ||
                (1 == substitute(subst, str, sizeof(str), "OUTPUT", "general_output")) ||
                (1 == substitute(subst, str, sizeof(str), "FORWARD", "general_forward")) ) {
               fprintf(fp, "%s\n", str);
            }
         }
      }
      memset(in_rule, 0, sizeof(in_rule));
   }

GPFirewallRuleNext:

{};// this statement is just to keep the compiler happy. otherwise it has  a problem with the lable:
   // add rules from sysevent
   unsigned int iterator; 
   char          name[MAX_QUERY];

   iterator = SYSEVENT_NULL_ITERATOR;
   do {
      name[0] = rule[0] = '\0';
      sysevent_get_unique(sysevent_fd, sysevent_token, 
                                  "GeneralPurposeFirewallRule", &iterator, 
                                  name, sizeof(name), rule, sizeof(rule));
      if ('\0' != rule[0]) {
         /*
          * the rule we just got could contain variables that we need to substitute
          * for runtime/configuration values
          */
         char str[MAX_QUERY];
         if (NULL != make_substitutions(rule, subst, sizeof(subst))) {
            if ((1 == substitute(subst, str, sizeof(str), "INPUT", "general_input"))   ||
               (1 == substitute(subst, str, sizeof(str), "OUTPUT", "general_output"))  ||
               (1 == substitute(subst, str, sizeof(str), "FORWARD", "general_forward")) ) {
                    fprintf(fp, "%s\n", str);
            }
         }
      }

   } while (SYSEVENT_NULL_ITERATOR != iterator);
                FIREWALL_DEBUG("Exiting do_filter_table_general_rules\n"); 	  
   return (0);
}

// TODO: ALL THESE THINGSZ
static int prepare_multinet_prerouting_nat(FILE *nat_fp) {
    return 0;
}

static int prepare_multinet_postrouting_nat(FILE *nat_fp) {
    return 0;
}

static void prepare_ipc_filter(FILE *filter_fp) {
                FIREWALL_DEBUG("Entering prepare_ipc_filter\n"); 	  
    // TODO: fix this hard coding
    fprintf(filter_fp, "-I OUTPUT -o %s -j ACCEPT\n", "l2sd0.500");
    fprintf(filter_fp, "-I INPUT -i %s -j ACCEPT\n", "l2sd0.500");
//zqiu>>
//  make sure rpc channel are not been blocked
    fprintf(filter_fp, "-I OUTPUT -o %s -j ACCEPT\n", "l2sd0.4093");
    fprintf(filter_fp, "-I INPUT -i %s -j ACCEPT\n", "l2sd0.4093");
//zqiu<<
                FIREWALL_DEBUG("Exiting prepare_ipc_filter\n"); 	  
}

static int prepare_multinet_filter_input(FILE *filter_fp) {
    
                FIREWALL_DEBUG("Entering prepare_multinet_filter_input\n"); 	  
    //Allow GRE tunnel traffic
    // TODO: Read sysevent enable flag
    fprintf(filter_fp, "-I INPUT -i %s -p gre -j ACCEPT\n", current_wan_ifname);
    
                 FIREWALL_DEBUG("Exiting prepare_multinet_filter_input\n"); 	 
    
}

static int prepare_multinet_filter_output(FILE *filter_fp) {
   
    return 0;
}

static int prepare_multinet_filter_forward(FILE *filter_fp) {
     unsigned int instanceCnt = 0;
    unsigned int *instanceList = NULL;
    unsigned int i = 0;
    unsigned int func_ret = CCSP_SUCCESS;
    char* tok = NULL;
    
    char net_query[40];
    net_query[0] = '\0';
    char net_resp[MAX_QUERY];
    char inst_resp[MAX_QUERY];
    inst_resp[0] = '\0';
    //char net_ip[MAX_QUERY];
    char primary_inst[MAX_QUERY];
    primary_inst[0] = '\0';
    char ip[20];
          FIREWALL_DEBUG("Entering prepare_multinet_filter_forward\n"); 	 
    //L3 rules
    snprintf(net_query, sizeof(net_query), "ipv4-instances");
    sysevent_get(sysevent_fd, sysevent_token, net_query, inst_resp, sizeof(inst_resp));
    
    snprintf(net_query, sizeof(net_query), "primary_lan_l3net");
    sysevent_get(sysevent_fd, sysevent_token, net_query, primary_inst, sizeof(inst_resp));
    
    tok = strtok(inst_resp, " ");
    
    if (tok) do {
        // TODO: IGNORING Primary INSTANCE FOR NOW
        if (strcmp(primary_inst,tok) == 0) 
            continue;
        snprintf(net_query, sizeof(net_query), "ipv4_%s-status", tok);
        sysevent_get(sysevent_fd, sysevent_token, net_query, net_resp, sizeof(net_resp));
        if (strcmp("up", net_resp) != 0)
            continue;
        
        snprintf(net_query, sizeof(net_query), "ipv4_%s-ifname", tok);
        sysevent_get(sysevent_fd, sysevent_token, net_query, net_resp, sizeof(net_resp));
        
        snprintf(net_query, sizeof(net_query), "ipv4_%s-ipv4addr", tok);
        sysevent_get(sysevent_fd, sysevent_token, net_query, ip, sizeof(ip));
        
//         snprintf(net_query, sizeof(net_query), "ipv4_%s-ipv4addr", tok);
//         sysevent_get(sysevent_fd, sysevent_token, net_query, net_ip, sizeof(net_resp));
        
//         fprintf(filter_fp, "-I INPUT -i %s -d %s -j ACCEPT\n", net_resp, net_ip);
        fprintf(filter_fp, "-A INPUT -i %s -d %s -j ACCEPT\n", net_resp, ip);
        fprintf(filter_fp, "-A INPUT -i %s -m pkttype ! --pkt-type unicast -j ACCEPT\n", net_resp);
        fprintf(filter_fp, "-A FORWARD -i %s -o %s -j ACCEPT\n", net_resp, current_wan_ifname);
        fprintf(filter_fp, "-A FORWARD -i %s -o %s -j ACCEPT\n", current_wan_ifname, net_resp);
        
        //fprintf(filter_fp, "-A OUTPUT -o %s -j ACCEPT\n", net_resp);
        
    } while ((tok = strtok(NULL, " ")) != NULL);
    
    snprintf(net_query, sizeof(net_query), "multinet-instances");
    sysevent_get(sysevent_fd, sysevent_token, net_query, inst_resp, sizeof(inst_resp));
    
    tok = strtok(inst_resp, " ");
    
    if (tok) do {
        snprintf(net_query, sizeof(net_query), "multinet_%s-localready", tok);
        sysevent_get(sysevent_fd, sysevent_token, net_query, net_resp, sizeof(net_resp));
        if (strcmp("1", net_resp) != 0)
            continue;
        
        snprintf(net_query, sizeof(net_query), "multinet_%s-name", tok);
        sysevent_get(sysevent_fd, sysevent_token, net_query, net_resp, sizeof(net_resp));
        
        fprintf(filter_fp, "-A FORWARD -i %s -o %s -j ACCEPT\n", net_resp, net_resp);
        
    } while ((tok = strtok(NULL, " ")) != NULL);
      FIREWALL_DEBUG("Exiting prepare_multinet_filter_forward\n"); 	 
    return 0;
}

static int prepare_multinet_mangle(FILE *mangle_fp) {
        unsigned int iterator; 
   char          name[MAX_QUERY];
   FILE* fp = mangle_fp;
   char      rule[MAX_QUERY];
   char      subst[MAX_QUERY];
   FIREWALL_DEBUG("Entering prepare_multinet_mangle\n"); 	
   iterator = SYSEVENT_NULL_ITERATOR;
   do {
      name[0] = rule[0] = '\0';
      sysevent_get_unique(sysevent_fd, sysevent_token, 
                                  "GeneralPurposeMangleRule", &iterator, 
                                  name, sizeof(name), rule, sizeof(rule));
      if ('\0' != rule[0]) {
         /*
          * the rule we just got could contain variables that we need to substitute
          * for runtime/configuration values
          */
         
         if (NULL != make_substitutions(rule, subst, sizeof(subst))) {
            fprintf(fp, "%s\n", subst);
         }
      }

   } while (SYSEVENT_NULL_ITERATOR != iterator);
      FIREWALL_DEBUG("Exiting prepare_multinet_mangle\n"); 	
}


//Captive Portal
static int isInCaptivePortal()
{
   //Captive Portal
   int retCode = 0;
   int retPsm = 0;
   char *pVal = NULL;
   int isNotifyDefault=0;
   int isRedirectionDefault=0;
   int isResponse204=0;
   FILE *responsefd;
   char responseCode[10];
   int iresCode;
   char *networkResponse = "/var/tmp/networkresponse.txt";
   FIREWALL_DEBUG("Entering isInCaptivePortal\n"); 	
   /* Get the syscfg DB value to check if we are in CP redirection mode*/


   retCode=syscfg_get(NULL, "CaptivePortal_Enable", captivePortalEnabled, sizeof(captivePortalEnabled));  
   if (0 != retCode || '\0' == captivePortalEnabled[0])
   {
	printf("%s Syscfg read failed to get CaptivePortal_Enable value\n", __FUNCTION__);
 	     	FIREWALL_DEBUG("%s Syscfg read failed to get CaptivePortal_Enable value\n" COMMA __FUNCTION__); 		
   }
   else
   {
        // Set a flag which we can check later to add DNS redirection
        if(!strcmp("false", captivePortalEnabled))
        {
 	     printf("CaptivePortal is disabled : Return 0\n"); 
 	     	FIREWALL_DEBUG("CaptivePortal is disabled : Return 0\n"); 	
      	     return 0;
        }
   }

   retCode=syscfg_get(NULL, "redirection_flag", redirectionFlag, sizeof(redirectionFlag));  
   if (0 != retCode || '\0' == redirectionFlag[0])
   {
	printf("CP DNS Redirection %s, Syscfg read failed\n", __FUNCTION__);
	FIREWALL_DEBUG("CP DNS Redirection %s, Syscfg read failed\n"COMMA __FUNCTION__); 	
   }
   else
   {
        // Set a flag which we can check later to add DNS redirection
        if(!strcmp("true", redirectionFlag))
        {
            isRedirectionDefault = 1;
        }
   }

   // Check the PSM value to check we are having default WiFi configuration
   if(bus_handle != NULL)
   {
       retPsm = PSM_VALUE_GET_STRING(PSM_NAME_CP_NOTIFY_VALUE, pVal);
       if(retPsm == CCSP_SUCCESS && pVal != NULL)
       {
          /* If value is true then we are in default onfiguration mode */
          if(strcmp("true", pVal) == 0)
          {
             isNotifyDefault = 1;
          }
          Ansc_FreeMemory_Callback(pVal);
          pVal = NULL;
       }  
   }  
   //Check the reponse code received from Web Service
   if((responsefd = fopen(networkResponse, "r")) != NULL) 
   {

       if(fgets(responseCode, sizeof(responseCode), responsefd) != NULL)
       {
		  iresCode = atoi(responseCode);
          if( iresCode == 204 ) 
          {
            isResponse204=1;
            fclose(responsefd);
          }
       }
   }

   FIREWALL_DEBUG("Exiting isInCaptivePortal\n");	 

   if((isRedirectionDefault) && (isNotifyDefault) && (isResponse204))
   {
      printf("CP DNS Redirection : Return 1\n"); 
         FIREWALL_DEBUG("CP DNS Redirection : Return 1\n"); 	
      return 1;
   }
   else
   {
      printf("CP DNS Redirection : Return 0\n"); 
       FIREWALL_DEBUG("CP DNS Redirection : Return 0\n"); 
      return 0;
   }
}
/*
 ==========================================================================
              IPv4 Firewall 
 ==========================================================================
 */

/*
 *  Procedure     : prepare_subtables
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules with the table/subtable structure
 *  Parameters    :
 *    raw_fp         : An open file for raw subtables
 *    mangle_fp      : An open file for mangle subtables
 *    nat_fp         : An open file for nat subtables
 *    filter_fp      : An open file for filter subtables
 * Return Values  :
 *    0              : Success
 */
static int prepare_subtables(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
   FIREWALL_DEBUG("Entering prepare_subtables \n"); 	
   int i; 
   /*
    * raw
    */
   fprintf(raw_fp, "%s\n", "*raw");
   fprintf(raw_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(raw_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
   fprintf(raw_fp, "%s\n", ":xlog_drop_lanattack  - [0:0]");
   fprintf(raw_fp, "%s\n", ":prerouting_ephemeral - [0:0]");
   fprintf(raw_fp, "%s\n", ":output_ephemeral - [0:0]");
   fprintf(raw_fp, "%s\n", ":prerouting_raw - [0:0]");
   fprintf(raw_fp, "%s\n", ":output_raw - [0:0]");
   fprintf(raw_fp, "%s\n", ":prerouting_nowan - [0:0]");
   fprintf(raw_fp, "%s\n", ":output_nowan - [0:0]");
   fprintf(raw_fp, "-A PREROUTING -j prerouting_ephemeral\n");
   fprintf(raw_fp, "-A OUTPUT -j output_ephemeral\n");
   fprintf(raw_fp, "-A PREROUTING -j prerouting_raw\n");
   fprintf(raw_fp, "-A OUTPUT -j output_raw\n");
   fprintf(raw_fp, "-A PREROUTING -j prerouting_nowan\n");
   fprintf(raw_fp, "-A OUTPUT -j output_nowan\n");

   /*
    * mangle
    */
   fprintf(mangle_fp, "%s\n", "*mangle");
   fprintf(mangle_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(mangle_fp, "%s\n", ":POSTROUTING ACCEPT [0:0]");
   fprintf(mangle_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
#ifdef CONFIG_BUILD_TRIGGER
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   fprintf(mangle_fp, "%s\n", ":prerouting_trigger - [0:0]");
#endif
#endif
   fprintf(mangle_fp, "%s\n", ":prerouting_qos - [0:0]");
   fprintf(mangle_fp, "%s\n", ":postrouting_qos - [0:0]");
   fprintf(mangle_fp, "%s\n", ":postrouting_lan2lan - [0:0]");

  /*ADDED TO SUPPORT XCONF SERVER REACHABILITY
   * All egress traffic from the erouter0 interface 
   * is marked with AF22 
   */
   
   //zqiu: RDKB-4519 
   //fprintf(mangle_fp, "-A FORWARD -j DSCP --set-dscp 0x0\n");
#if defined(_COSA_BCM_MIPS_)
   fprintf(mangle_fp, "-A FORWARD -m physdev --physdev-in emta0 -j ACCEPT\n");
#endif
   fprintf(mangle_fp, "-A FORWARD -m state --state NEW -j DSCP --set-dscp-class af22\n");
#if ! defined (INTEL_PUMA7) && ! defined (_COSA_BCM_ARM_)
   fprintf(mangle_fp, "-A FORWARD -m state ! --state NEW -j DSCP  --set-dscp 0x0\n");
#endif
   fprintf(mangle_fp, "-A OUTPUT -o erouter0 -j DSCP --set-dscp-class af22\n");
   fprintf(mangle_fp, "-A POSTROUTING -o erouter0 -p gre -j DSCP --set-dscp %d \n",greDscp);
   
   fprintf(mangle_fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class af32 -j CONNMARK --set-mark 0xA\n");
   fprintf(mangle_fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class cs1 -j CONNMARK --set-mark 0xB\n");
   fprintf(mangle_fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class cs5 -j CONNMARK --set-mark 0xC\n");
   fprintf(mangle_fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class af22 -j CONNMARK --set-mark 0xD\n");
   fprintf(mangle_fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xA  -j DSCP --set-dscp-class af32\n");
   fprintf(mangle_fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xB -j DSCP --set-dscp-class cs1\n");
   fprintf(mangle_fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xC -j DSCP --set-dscp-class cs5\n");
   fprintf(mangle_fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xD -j DSCP --set-dscp-class af22\n");
   
   /*XCONF RULES END*/

#ifdef CONFIG_BUILD_TRIGGER
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   fprintf(mangle_fp, "-A PREROUTING -j prerouting_trigger\n");
#endif
#endif
   fprintf(mangle_fp, "-A PREROUTING -j prerouting_qos\n");
   fprintf(mangle_fp, "-A POSTROUTING -j postrouting_qos\n");
   fprintf(mangle_fp, "-A POSTROUTING -j postrouting_lan2lan\n");

   /*
    * nat
    */
   fprintf(nat_fp, "%s\n", "*nat");
   fprintf(nat_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(nat_fp, "%s\n", ":POSTROUTING ACCEPT [0:0]");
   fprintf(nat_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
#if defined(_COSA_BCM_MIPS_)
   fprintf(nat_fp, "-A PREROUTING -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
   fprintf(nat_fp, "-A PREROUTING -m physdev --physdev-out %s -j ACCEPT\n", emta_wan_ifname);
#endif
   fprintf(nat_fp, "%s\n", ":prerouting_ephemeral - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_fromwan - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_mgmt_override - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_plugins - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_fromwan_todmz - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_fromlan - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_devices - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_redirect - [0:0]");
#ifdef CONFIG_BUILD_TRIGGER
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   fprintf(nat_fp, "%s\n", ":prerouting_fromlan_trigger - [0:0]");
   fprintf(nat_fp, "%s\n", ":prerouting_fromwan_trigger - [0:0]");
#endif
#endif

   fprintf(nat_fp, "%s\n", ":postrouting_towan - [0:0]");
   fprintf(nat_fp, "%s\n", ":postrouting_tolan - [0:0]");
   fprintf(nat_fp, "%s\n", ":postrouting_plugins - [0:0]");
   fprintf(nat_fp, "%s\n", ":postrouting_ephemeral - [0:0]");
#if defined(_COSA_BCM_MIPS_)
   fprintf(nat_fp, "-A POSTROUTING -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
   fprintf(nat_fp, "-A POSTROUTING -m physdev --physdev-out %s -j ACCEPT\n", emta_wan_ifname);
#endif
   fprintf(nat_fp, "-A PREROUTING -j prerouting_ephemeral\n");
   fprintf(nat_fp, "-A PREROUTING -j prerouting_mgmt_override\n");
   fprintf(nat_fp, "-A PREROUTING -i %s -j prerouting_fromlan\n", lan_ifname);
   fprintf(nat_fp, "-A PREROUTING -i %s -j prerouting_devices\n", lan_ifname);    
   char IPv4[17] = "0"; 
      
   syscfg_get(NULL, "HTTP_Server_IP", IPv4, sizeof(IPv4));
   fprintf(nat_fp, "-A prerouting_redirect -p tcp --dport 80 -j DNAT --to-destination %s:80\n",IPv4);
  

   IPv4[0] = '\0';
   syscfg_get(NULL, "HTTPS_Server_IP", IPv4, sizeof(IPv4));
   fprintf(nat_fp, "-A prerouting_redirect -p tcp --dport 443 -j DNAT --to-destination %s:443\n",IPv4);

   IPv4[0] = '\0';
   syscfg_get(NULL, "Default_Server_IP", IPv4, sizeof(IPv4));
   fprintf(nat_fp, "-A prerouting_redirect -p tcp -j DNAT --to-destination %s:443\n",IPv4);
   fprintf(nat_fp, "-A prerouting_redirect -p udp ! --dport 53 -j DNAT --to-destination %s:80\n",IPv4);
   
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   if(isGuestNetworkEnabled) {
       fprintf(nat_fp, "%s\n", ":guestnet_walled_garden - [0:0]");
       fprintf(nat_fp, "%s\n", ":guestnet_allow_list - [0:0]");
       fprintf(nat_fp, "-A guestnet_walled_garden -j guestnet_allow_list\n");
       fprintf(nat_fp, "-A PREROUTING -s %s/%s -j guestnet_walled_garden\n", guest_network_ipaddr, guest_network_mask);
   }
   
   fprintf(nat_fp, "%s\n", ":device_based_parcon - [0:0]");
   fprintf(nat_fp, "%s\n", ":parcon_allow_list - [0:0]");
   fprintf(nat_fp, "%s\n", ":parcon_walled_garden - [0:0]");
   fprintf(nat_fp, "-A device_based_parcon -j parcon_allow_list\n");
   fprintf(nat_fp, "-A device_based_parcon -j parcon_walled_garden\n");
   fprintf(nat_fp, "-A prerouting_fromlan -j device_based_parcon\n");
#endif
#ifdef  CONFIG_CISCO_PARCON_WALLED_GARDEN
   fprintf(nat_fp, "%s\n", ":managedsite_based_parcon - [0:0]");
//   fprintf(nat_fp, "%s\n", ":parcon_allow_list - [0:0]");
   fprintf(nat_fp, "%s\n", ":parcon_walled_garden - [0:0]");
//   fprintf(nat_fp, "-A device_based_parcon -j parcon_allow_list\n");
//   fprintf(nat_fp, "-A device_based_parcon -j parcon_walled_garden\n");
#endif

   fprintf(nat_fp, "-A PREROUTING -i %s -j prerouting_fromwan\n", current_wan_ifname);
   prepare_multinet_prerouting_nat(nat_fp);
#ifdef CONFIG_BUILD_TRIGGER
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   fprintf(nat_fp, "-A prerouting_fromlan -j prerouting_fromlan_trigger\n");
   fprintf(nat_fp, "-A prerouting_fromwan -j prerouting_fromwan_trigger\n");
#endif
#endif
   fprintf(nat_fp, "-A PREROUTING -j prerouting_plugins\n");
   fprintf(nat_fp, "-A PREROUTING -i %s -j prerouting_fromwan_todmz\n", current_wan_ifname);
   fprintf(nat_fp, "-A POSTROUTING -j postrouting_ephemeral\n");
// This breaks emta DNS routing on XF3. We may need some special rule here.
   fprintf(nat_fp, "-A POSTROUTING -o %s -j postrouting_towan\n", current_wan_ifname);
   fprintf(nat_fp, "-A POSTROUTING -o %s -j postrouting_tolan\n", lan_ifname);
   prepare_multinet_postrouting_nat(nat_fp);
   fprintf(nat_fp, "-A POSTROUTING -j postrouting_plugins\n");

   /*
    * filter
    */
   fprintf(filter_fp, "%s\n", "*filter");
   fprintf(filter_fp, "%s\n", ":INPUT DROP [0:0]");
   fprintf(filter_fp, "%s\n", ":FORWARD ACCEPT [0:0]");
   fprintf(filter_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");

#if defined (INTEL_PUMA7) || defined (_COSA_BCM_ARM_)
   //Avoid blocking packets at the Intel NIL layer
   fprintf(filter_fp, "-A FORWARD -i a-mux -j ACCEPT\n");
#endif

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   fprintf(filter_fp, "%s\n", ":pp_disabled - [0:0]");
   if(isGuestNetworkEnabled) {
       fprintf(filter_fp, "-A pp_disabled -s %s/%s -p tcp -m state --state ESTABLISHED -m connbytes --connbytes 0:5 --connbytes-dir original --connbytes-mode packets -j GWMETA --dis-pp\n", guest_network_ipaddr, guest_network_mask);
       fprintf(filter_fp, "-A pp_disabled -d %s/%s -p tcp -m state --state ESTABLISHED -m connbytes --connbytes 0:5 --connbytes-dir reply --connbytes-mode packets -j GWMETA --dis-pp\n", guest_network_ipaddr, guest_network_mask);
   }

   fprintf(filter_fp, "-A pp_disabled -p udp --dport 53 -j GWMETA --dis-pp\n");
   fprintf(filter_fp, "-A pp_disabled -p udp --sport 53 -j GWMETA --dis-pp\n");
   fprintf(filter_fp, "-A FORWARD -j pp_disabled\n");
#endif

   fprintf(filter_fp, "%s\n", ":lan2wan - [0:0]");
   
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   fprintf(filter_fp, "%s\n", ":lan2wan_dnsq_intercept - [0:0]"); //dns query nfqueue handler rules
   fprintf(filter_fp, "%s\n", ":lan2wan_httpget_intercept - [0:0]"); //http nfqueue handler rules
   fprintf(filter_fp, "%s\n", ":parcon_allow_list  - [0:0]");
   fprintf(filter_fp, "-A lan2wan_httpget_intercept -j parcon_allow_list\n");
   fprintf(filter_fp, "-A lan2wan -p tcp --dport 80 -j lan2wan_httpget_intercept\n");
   fprintf(filter_fp, "-A lan2wan -p udp --dport 53 -j lan2wan_dnsq_intercept\n");
#endif

   fprintf(filter_fp, "%s\n", ":lan2wan_misc - [0:0]");
#ifdef CONFIG_BUILD_TRIGGER
   fprintf(filter_fp, "%s\n", ":lan2wan_triggers - [0:0]");
#endif
   fprintf(filter_fp, "%s\n", ":lan2wan_disable - [0:0]");
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
   fprintf(filter_fp, "%s\n", ":lan2wan_staticip - [0:0]");
#endif
   fprintf(filter_fp, "%s\n", ":lan2wan_forwarding_accept - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2wan_dmz_accept - [0:0]");
   //fprintf(filter_fp, "%s\n", ":lan2wan_webfilters - [0:0]");
   //fprintf(filter_fp, "%s\n", ":lan2wan_iap - [0:0]");
   //fprintf(filter_fp, "%s\n", ":lan2wan_plugins - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2wan_pc_device - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2wan_pc_site - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2wan_pc_service - [0:0]");
   //fprintf(filter_fp, "%s\n", ":lan2wan_iot_allow - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan - [0:0]");
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN 
//    fprintf(filterFp, ":lan2wan_dnsq_nfqueue - [0:0]\n");
//    fprintf(fp, ":lan2wan_http_nfqueue - [0:0]\n");
    fprintf(filter_fp, ":wan2lan_dnsr_nfqueue - [0:0]\n");
#endif

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   fprintf(filter_fp, "%s\n", ":wan2lan_dns_intercept - [0:0]");
#endif

   fprintf(filter_fp, "%s\n", ":wan2lan_disabled - [0:0]");
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
   fprintf(filter_fp, "%s\n", ":wan2lan_staticip_pm - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_staticip - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_staticip_post - [0:0]");
#endif
   fprintf(filter_fp, "%s\n", ":wan2lan_forwarding_accept - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_misc - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_accept - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_plugins - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_nonat - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_dmz - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2lan_iot_allow - [0:0]");
#ifdef CONFIG_BUILD_TRIGGER
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   fprintf(filter_fp, "%s\n", ":wan2lan_trigger - [0:0]");
#endif
#endif
   fprintf(filter_fp, "%s\n", ":lan2self - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2self_by_wanip - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2self_mgmt - [0:0]");
   fprintf(filter_fp, "%s\n", ":host_detect - [0:0]");
   fprintf(filter_fp, "%s\n", ":lanattack - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2self_plugins - [0:0]");
   fprintf(filter_fp, "%s\n", ":self2lan - [0:0]");
   fprintf(filter_fp, "%s\n", ":self2lan_plugins - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2self - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2self_mgmt - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2self_ports - [0:0]");
   fprintf(filter_fp, "%s\n", ":wanattack - [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2self_allow - [0:0]");
   fprintf(filter_fp, "%s\n", ":lanhosts - [0:0]");
   fprintf(filter_fp, "%s\n", ":general_input - [0:0]");
   fprintf(filter_fp, "%s\n", ":general_output - [0:0]");
   fprintf(filter_fp, "%s\n", ":general_forward - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_accept_lan2wan - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_accept_wan2lan - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_accept_wan2self - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_wan2lan - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_lan2wan - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_wan2self - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_wanattack - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_lan2self - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_lanattack - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlogdrop - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlogreject - [0:0]");

   // Allow local loopback traffic 
   fprintf(filter_fp, "-A INPUT -i lo -s 127.0.0.0/8 -j ACCEPT\n");
   if (isWanReady) {
      fprintf(filter_fp, "-A INPUT -p tcp -i lo -s %s -d %s -j ACCEPT\n", current_wan_ipaddr, current_wan_ipaddr);
   }
   // since some protocols have a different ip address for the connection to the isp, and the wan
   // accept loopback to isp
    char isp_connection[MAX_QUERY];
    isp_connection[0] = '\0';
    sysevent_get(sysevent_fd, sysevent_token, "ipv4_wan_ipaddr", isp_connection, sizeof(isp_connection));
    if ('\0' != isp_connection[0] &&
        0 != strcmp("0.0.0.0", isp_connection) &&
        0 != strcmp(isp_connection, current_wan_ipaddr)) {
      fprintf(filter_fp, "-A INPUT -p tcp -i lo -s %s -d %s -j ACCEPT\n", isp_connection, isp_connection);
   }

   fprintf(filter_fp, "-A INPUT -i lo -m state --state NEW -j ACCEPT\n");
   fprintf(filter_fp, "-A INPUT -j general_input\n");
   fprintf(filter_fp, "-A INPUT -i %s -j wan2self_mgmt\n", ecm_wan_ifname);
   fprintf(filter_fp, "-A INPUT -i %s -j wan2self_mgmt\n", current_wan_ifname);
   fprintf(filter_fp, "-A INPUT -i %s -j wan2self_mgmt\n", emta_wan_ifname);
   fprintf(filter_fp, "-A INPUT -i %s -j lan2self\n", lan_ifname);
   fprintf(filter_fp, "-A INPUT -i %s -j wan2self\n", current_wan_ifname);
   if ('\0' != default_wan_ifname[0] && 0 != strlen(default_wan_ifname) && 0 != strcmp(default_wan_ifname, current_wan_ifname)) {
      // even if current_wan_ifname is ppp we still want to consider default wan ifname as an interface
      // but dont duplicate
      char str[MAX_QUERY];
      snprintf(str, sizeof(str),
               "-A INPUT -i %s -j wan2self\n", default_wan_ifname);
      fprintf(filter_fp, "%s\n", str);
   }

   //Add wan2self restrictions to other wan interfaces
   //ping is allowed to cm and mta inferfaces regardless the firewall level
   fprintf(filter_fp, "-A INPUT -i %s -p icmp --icmp-type 8 -m limit --limit 3/second -j %s\n", ecm_wan_ifname, "xlog_accept_wan2self"); // ICMP PING
   fprintf(filter_fp, "-A INPUT -i %s -j wan2self_ports\n", ecm_wan_ifname);
   fprintf(filter_fp, "-A INPUT -i %s -p icmp --icmp-type 8 -m limit --limit 3/second -j %s\n", emta_wan_ifname, "xlog_accept_wan2self"); // ICMP PING
   fprintf(filter_fp, "-A INPUT -i %s -j wan2self_ports\n", emta_wan_ifname);
   fprintf(filter_fp, "-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n");

   if (isCmDiagEnabled)
   {
      fprintf(filter_fp, "-A INPUT -i %s -d 192.168.100.1 -j ACCEPT\n", cmdiag_ifname);
   }

#ifdef _COSA_BCM_MIPS_
    // Allow all traffic to the private interface priv0
    fprintf(filter_fp, "-A INPUT -i priv0 -j ACCEPT\n");
#endif

   //Captive Portal 
   /* If both PSM and syscfg values are true then SET the DNS redirection to GW*/    
   if(isInCaptivePortal()==1)
   {   
      fprintf(nat_fp, "-I PREROUTING -i %s -p udp --dport 53 -j DNAT --to %s\n", lan_ifname, lan_ipaddr);
      fprintf(nat_fp, "-I PREROUTING -i %s -p tcp --dport 53 -j DNAT --to %s\n", lan_ifname, lan_ipaddr);
   }
   

   if (ecm_wan_ifname[0])  // spare eCM wan interface from Utopia firewall
   {
      //block port 53/67/514
      fprintf(filter_fp, "-A INPUT -i %s -p udp --dport 53 -j DROP\n", ecm_wan_ifname);
      fprintf(filter_fp, "-A INPUT -i %s -p udp --dport 67 -j DROP\n", ecm_wan_ifname);
      fprintf(filter_fp, "-A INPUT -i %s -p udp --dport 514 -j DROP\n", ecm_wan_ifname);

      fprintf(filter_fp, "-A INPUT -i %s -j ACCEPT\n", ecm_wan_ifname);
   }

   if (emta_wan_ifname[0]) // spare eMTA wan interface from Utopia firewall
   {
      fprintf(filter_fp, "-A INPUT -i %s -p udp --dport 80 -j DROP\n", emta_wan_ifname);
      fprintf(filter_fp, "-A INPUT -i %s -p udp --dport 443 -j DROP\n", emta_wan_ifname);

      fprintf(filter_fp, "-A INPUT -i %s -j ACCEPT\n", emta_wan_ifname);
   }
#if defined (INTEL_PUMA7) || defined (_COSA_BCM_ARM_)

   fprintf(filter_fp, "-A INPUT -i gmac5 -j ACCEPT\n");

#endif
   prepare_multinet_filter_input(filter_fp);
   prepare_ipc_filter(filter_fp);
   //fprintf(filter_fp, "-A OUTPUT -m state --state INVALID -j DROP\n");

   /*
    * if the wan is currently unavailable, then drop any packets from lan to wan
    * except for DHCP (broadcast)
    */
   char str[MAX_QUERY];
   //snprintf(str, sizeof(str), "-I OUTPUT 1 -s 0.0.0.0 ! -d 255.255.255.255 -o %s -j DROP", current_wan_ifname);
   //fprintf(filter_fp, "%s\n", str);
#if defined(_COSA_BCM_MIPS_)
   fprintf(filter_fp, "-A OUTPUT -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
#endif
   fprintf(filter_fp, "-A OUTPUT -j general_output\n");
   fprintf(filter_fp, "-A OUTPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n");
   fprintf(filter_fp, "-A OUTPUT -o %s -j self2lan\n", lan_ifname);
   prepare_multinet_filter_output(filter_fp);
   fprintf(filter_fp, "-A self2lan -j self2lan_plugins\n");
   fprintf(filter_fp, "-A OUTPUT -m state --state NEW -j ACCEPT\n");

#if defined(_COSA_BCM_MIPS_)
   fprintf(filter_fp, "-A FORWARD -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
   fprintf(filter_fp, "-A FORWARD -m physdev --physdev-out %s -j ACCEPT\n", emta_wan_ifname);
#endif

   fprintf(filter_fp, "-A FORWARD -j general_forward\n");
   fprintf(filter_fp, "-A FORWARD -i %s -o %s -j wan2lan\n", current_wan_ifname, lan_ifname);
   fprintf(filter_fp, "-A FORWARD -i %s -o %s -j lan2wan\n", lan_ifname, current_wan_ifname);
   // need br0 to br0 for virtual services)
   fprintf(filter_fp, "-A FORWARD -i %s -o %s -j ACCEPT\n", lan_ifname, lan_ifname);
   prepare_multinet_filter_forward(filter_fp);
   fprintf(filter_fp, "-A FORWARD -j xlog_drop_wan2lan\n");
   
   fprintf(filter_fp, "-I FORWARD 2 -i l2sd0.4090 -o %s -j ACCEPT\n", current_wan_ifname);
   fprintf(filter_fp, "-I FORWARD 3 -i %s -o l2sd0.4090 -j ACCEPT\n", current_wan_ifname);

   // RDKB-4826 - IOT rules for DHCP
   static char iot_enabled[20];
   memset(iot_enabled, 0, sizeof(iot_enabled));
   syscfg_get(NULL, "lost_and_found_enable", iot_enabled, sizeof(iot_enabled));
  
   if(0==strcmp("true",iot_enabled))
   {
      printf("IOT_LOG : Adding iptable rules for IOT\n");
      memset(iot_ifName, 0, sizeof(iot_ifName));
      syscfg_get(NULL, "iot_ifname", iot_ifName, sizeof(iot_ifName));
      memset(iot_primaryAddress, 0, sizeof(iot_primaryAddress));
      syscfg_get(NULL, "iot_ipaddr", iot_primaryAddress, sizeof(iot_primaryAddress));

      fprintf(filter_fp,"-A INPUT -d %s/24 -i %s -j ACCEPT\n",iot_primaryAddress,iot_ifName);
      fprintf(filter_fp,"-A INPUT -i %s -m pkttype ! --pkt-type unicast -j ACCEPT\n",iot_ifName);
      //fprintf(filter_fp,"-A FORWARD -i %s -o %s -j ACCEPT\n",iot_ifName,iot_ifName);
      //fprintf(filter_fp, "-I FORWARD 2 -i %s -o %s -j lan2wan_iot_allow\n", iot_ifName,current_wan_ifname);
      fprintf(filter_fp, "-I FORWARD 2 -i %s -o %s -j ACCEPT\n", iot_ifName,current_wan_ifname);
      fprintf(filter_fp, "-I FORWARD 3 -i %s -o %s -j wan2lan_iot_allow\n", current_wan_ifname, iot_ifName);
      //zqiu: R5337
      //do_lan2wan_IoT_Allow(filter_fp);
      do_wan2lan_IoT_Allow(filter_fp);
   }


   /***********************
    * set lan to wan subrule by order 
    * *********************/
   fprintf(filter_fp, "-A lan2wan -j lan2wan_disable\n");
   //fprintf(filter_fp, "-A lan2wan -i l2sd0.106 -j lan2wan_iot_allow\n");
   for(i=0; i< IPT_PRI_MAX; i++)
   {
      switch(iptables_pri_level[i]){
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
          case IPT_PRI_STATIC_IP:
             fprintf(filter_fp, "-A lan2wan -j lan2wan_staticip\n");
             break;
#endif
#ifdef CONFIG_BUILD_TRIGGER
          case IPT_PRI_PORTTRIGGERING:
             fprintf(filter_fp, "-A lan2wan -j lan2wan_triggers\n");
             break;
#endif
        case IPT_PRI_FIREWALL:
             //   fprintf(filter_fp, "-A lan2wan -m state --state INVALID -j xlog_drop_lan2wan\n");
             fprintf(filter_fp, "-A lan2wan -j lan2wan_misc\n");
             //fprintf(filter_fp, "-A lan2wan -j lan2wan_webfilters\n");
            //fprintf(filter_fp, "-A lan2wan -j lan2wan_iap\n");
            //fprintf(filter_fp, "-A lan2wan -j lan2wan_plugins\n");
            fprintf(filter_fp, "-A lan2wan -j lan2wan_pc_device\n");
            fprintf(filter_fp, "-A lan2wan -j lan2wan_pc_site\n");
            fprintf(filter_fp, "-A lan2wan -j lan2wan_pc_service\n");
            fprintf(filter_fp, "-A lan2wan -j host_detect\n");
            fprintf(filter_fp, "-A lan2wan -m state --state NEW -j xlog_accept_lan2wan\n");
            fprintf(filter_fp, "-A lan2wan -m state --state RELATED,ESTABLISHED -j xlog_accept_lan2wan\n");
            fprintf(filter_fp, "-A lan2wan -j xlog_accept_lan2wan\n");
            break;

        case IPT_PRI_PORTMAPPING:
            fprintf(filter_fp, "-A lan2wan -j lan2wan_forwarding_accept\n");
            break;

        case IPT_PRI_DMZ:
            fprintf(filter_fp, "-A lan2wan -j lan2wan_dmz_accept\n");
            break;

        default:
            break;
      }
   }

   //fprintf(filter_fp, "-A lan2self -m state --state INVALID -j DROP\n");
   //Block traffic to lan0. 192.168.100.3 is for ATOM dbus connection.
   if(isWanServiceReady) {
       fprintf(filter_fp, "-A general_input -i lan0 ! -s 192.168.100.3 -d 192.168.100.1 -j xlog_drop_lan2self\n");
       fprintf(filter_fp, "-A general_input -i brlan0 ! -s 192.168.100.3 -d 192.168.100.1 -j xlog_drop_lan2self\n");
   }

   //open port for DHCP
   if(!isBridgeMode) {
       fprintf(filter_fp, "-A general_input -i %s -p udp --dport 68 -j ACCEPT\n", current_wan_ifname);
       fprintf(filter_fp, "-A general_input -i %s -p udp --dport 68 -j ACCEPT\n", ecm_wan_ifname);
       fprintf(filter_fp, "-A general_input -i %s -p udp --dport 68 -j ACCEPT\n", emta_wan_ifname);
   }

   fprintf(filter_fp, "-A lan2self ! -d %s -j lan2self_by_wanip\n", lan_ipaddr);
   fprintf(filter_fp, "-A lan2self -j lan2self_mgmt\n");
   fprintf(filter_fp, "-A lan2self -j lanattack\n");
   fprintf(filter_fp, "-A lan2self -j host_detect\n");
   fprintf(filter_fp, "-A lan2self -j lan2self_plugins\n");
   fprintf(filter_fp, "-A lan2self -m state --state NEW -j ACCEPT\n");
   fprintf(filter_fp, "-A lan2self -m state --state RELATED,ESTABLISHED -j ACCEPT\n");
   fprintf(filter_fp, "-A lan2self -j ACCEPT\n");

//   fprintf(filter_fp, "-A wan2self -m state --state INVALID -j xlog_drop_wan2self\n");
   fprintf(filter_fp, "-A wan2self -j wan2self_allow\n");
   fprintf(filter_fp, "-A wan2self -j wanattack\n");
   fprintf(filter_fp, "-A wan2self -j wan2self_ports\n");
   fprintf(filter_fp, "-A wan2self -m state --state RELATED,ESTABLISHED -j ACCEPT\n");
   //fprintf(filter_fp, "-A wan2self -j wan2self_mgmt\n");
   fprintf(filter_fp, "-A wan2self -j xlog_drop_wan2self\n");

//   fprintf(filter_fp, "-A wan2lan -m state --state INVALID -j xlog_drop_wan2lan\n");

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   fprintf(filter_fp, "-A wan2lan  -p udp --sport 53 -j wan2lan_dns_intercept\n");
#endif

   fprintf(filter_fp, "-A wan2lan  -j wan2lan_disabled\n");
   for(i=0; i< IPT_PRI_MAX; i++)
   {
      switch(iptables_pri_level[i]){
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
          case IPT_PRI_STATIC_IP:
             fprintf(filter_fp, "-A wan2lan -j wan2lan_staticip_pm\n");
             fprintf(filter_fp, "-A wan2lan -j wan2lan_staticip\n");
             break;
#endif
          case IPT_PRI_PORTMAPPING:
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_forwarding_accept\n");
             break;
#ifdef CONFIG_BUILD_TRIGGER
          case IPT_PRI_PORTTRIGGERING:
#ifdef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_trigger\n");
#endif
             break;
#endif
          case IPT_PRI_DMZ:
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_dmz\n");
             break;
        case IPT_PRI_FIREWALL:
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
             fprintf(filter_fp, "-A wan2lan -p udp --sport 53 -j wan2lan_dnsr_nfqueue\n");
             fprintf(filter_fp, "-A wan2lan_dnsr_nfqueue -j GWMETA --dis-pp\n");
#endif
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_misc\n");
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_accept\n");
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_plugins\n");
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_nonat\n");
             fprintf(filter_fp, "-A wan2lan -m state --state RELATED,ESTABLISHED -j ACCEPT\n");
             fprintf(filter_fp, "-A wan2lan  -j host_detect\n");
#ifdef CISCO_CONFIG_TRUE_STATIC_IP
             /* TODO: next time change to USE IPT_PRI flag */
             fprintf(filter_fp, "-A wan2lan  -j wan2lan_staticip_post\n");
#endif
             break;
          default:
              break;
      }
   }
      FIREWALL_DEBUG("Exiting  prepare_subtables \n"); 	
   return(0);
}

/*
 *  Procedure     : prepare_subtables_ext
 *  Purpose       : Add rules to the iptables rules file based on an external file
 *                  This model is used for plugins 
 *  Parameters    :
 *    fname          : The name of the file to consult for extention rules
 *    raw_fp         : An open file for raw subtables
 *    mangle_fp      : An open file for mangle subtables
 *    nat_fp         : An open file for nat subtables
 *    filter_fp      : An open file for filter subtables
 * Return Values  :
 *    0              : Success
 */

static int prepare_subtables_ext(char *fname, FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
	FILE *ext_fp;
	char line[256];
	FILE *fp = NULL;
   FIREWALL_DEBUG("Entering prepare_subtables_ext \n"); 	
	ext_fp = fopen(fname, "r");
	if (ext_fp == NULL) {
		return -1;
	}

	while (fgets(line, sizeof(line), ext_fp) != NULL) {
		if (line[0] == '#') {	// Comment
			continue;
		}
		else if (line[0] == '*') {	// Table
			if (strncmp(line, "*raw", 4) == 0) {
				fp = raw_fp;
			}
			else if (strncmp(line, "*mangle", 7) == 0) {
				fp = mangle_fp;
			}
			else if (strncmp(line, "*nat", 4) == 0) {
				fp = nat_fp;
			}
			else if (strncmp(line, "*filter", 7) == 0) {
				fp = filter_fp;
			}
			continue;
		}
		else if (line[0] == ':') {	// Chain
			if (strncmp(line, ":PREROUTING", 11) == 0 ||
				strncmp(line, ":INPUT", 6) == 0 ||
				strncmp(line, ":OUTPUT", 7) == 0 ||
				strncmp(line, ":POSTROUTING", 12) == 0 ||
				strncmp(line, ":FORWARD", 8) == 0) {
				continue;
			}
		}
		else if (strncmp(line, "COMMIT", 6) == 0) {
			continue;
		}
			
		if (fp != NULL) {
			fprintf(fp, "%s", line);
			printf("%s", line);
		}
	}
   FIREWALL_DEBUG("Exiting prepare_subtables_ext \n"); 	
	return(0);
}

/*
===========================================================================
               raw table
===========================================================================
*/

/*
 *  Procedure     : do_raw_ephemeral
 *  Purpose       : prepare the iptables-restore statements for raw statements gleaned from the sysevent
 *                  RawFirewallRule pool
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 *  Notes         : These rules will be placed into the iptables raw table, and use target
 *                     prerouting_ephemeral for PREROUTING statements, or
 *                     output_ephemeral for OUTPUT statements
 */
static int do_raw_ephemeral(FILE *fp)
{
   unsigned  int iterator;
   char      name[MAX_QUERY];
   char      in_rule[MAX_QUERY];
   char      subst[MAX_QUERY];
   char      str[MAX_QUERY];
   FIREWALL_DEBUG("Entering do_raw_ephemeral \n"); 
   iterator = SYSEVENT_NULL_ITERATOR;
   do {
      name[0] = subst[0] = '\0';
      memset(in_rule, 0, sizeof(in_rule));
      sysevent_get_unique(sysevent_fd, sysevent_token,
                          "RawFirewallRule", &iterator,
                          name, sizeof(name), in_rule, sizeof(in_rule));
      if ('\0' != in_rule[0]) {
         // use the raw table
         isRawTableUsed = 1;
         /*
          * the rule we just got could contain variables that we need to substitute
          * for runtime/configuration values
          */
        if (NULL != make_substitutions(in_rule, subst, sizeof(subst))) {
           if (1 == substitute(subst, str, sizeof(str), "PREROUTING", "prerouting_ephemeral") ||
              (1 == substitute(subst, str, sizeof(str), "OUTPUT", "output_ephemeral")) ) {
              fprintf(fp, "%s\n", str);
           }
        }
      }
   } while (SYSEVENT_NULL_ITERATOR != iterator);

   FIREWALL_DEBUG("Exiting do_raw_ephemeral \n"); 
   return(0);
}

/*
 *  Procedure     : do_raw_table_general_rules
 *  Purpose       : prepare the iptables-restore statements for raw statements gleaned from the syscfg
 *                  RawTableFirewallRule
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 *  Notes         : These rules will be placed into the iptables raw table, and use target
 *                     prerouting_raw for PREROUTING statements, or
 *                     output_raw for OUTPUT statements
 */
static int do_raw_table_general_rules(FILE *fp)
{
FIREWALL_DEBUG("Entering do_raw_table_general_rules \n"); 
   int  idx;
   char rule_query[MAX_QUERY];
   int  count = 1;

   char      in_rule[MAX_QUERY];
   char      subst[MAX_QUERY];
   in_rule[0] = '\0';
   syscfg_get(NULL, "RawTableFirewallRuleCount", in_rule, sizeof(in_rule));
   if ('\0' == in_rule[0]) {
      return(0);
   } else {
      // use the raw table
      isRawTableUsed = 1;
      count = atoi(in_rule);
      if (0 == count) {
         return(0);
      }
      if (MAX_SYSCFG_ENTRIES < count) {
         count = MAX_SYSCFG_ENTRIES;
      }
   }

   memset(in_rule, 0, sizeof(in_rule));
   for (idx=1; idx<=count; idx++) {
      snprintf(rule_query, sizeof(rule_query), "RawTableFirewallRule_%d", idx);
      syscfg_get(NULL, rule_query, in_rule, sizeof(in_rule));
      if ('\0' == in_rule[0]) {
         continue;
      } else {
         /*
          * the rule we just got could contain variables that we need to substitute
          * for runtime/configuration values
          */
         char str[MAX_QUERY];
         if (NULL != make_substitutions(in_rule, subst, sizeof(subst))) {
             if ((1 == substitute(subst, str, sizeof(str), "PREROUTING", "prerouting_raw")) ||
                (1 == substitute(subst, str, sizeof(str), "OUTPUT", "output_raw")) ) {
               fprintf(fp, "%s\n", str);
            }
         }
      }
      memset(in_rule, 0, sizeof(in_rule));
   }
   FIREWALL_DEBUG("Exiting do_raw_table_general_rules \n"); 
   return(0);
}

/*
 *  Procedure     : do_raw_table_nowan
 *  Purpose       : prepare the iptables-restore statements for raw statements used only when there is
 *                  no wan connection yet
 *  Parameters    :
 *     fp              : An open file that will be used for iptables-restore
 *  Return Values :
 *     0               : done
 *    -1               : bad input parameter
 * Notes          :
 *    When there is no wan ip address we turn connection tracking off for packets to/from us
 *    because sometimes a conntrack with 0.0.0.0 is formed, and this lasts a lot time if conntrack
 */
static int do_raw_table_nowan(FILE *fp)
{
	FIREWALL_DEBUG("Entering do_raw_table_nowan \n"); 
   if (!isWanReady) {
      char str[MAX_QUERY];

      //use the raw table
      isRawTableUsed = 1;

      snprintf(str, sizeof(str), "-A prerouting_nowan -i %s -j NOTRACK", default_wan_ifname);
      fprintf(fp, "%s\n", str);

      snprintf(str, sizeof(str), "-A output_nowan -o %s -j NOTRACK",default_wan_ifname);
      fprintf(fp, "%s\n", str);
   }
      FIREWALL_DEBUG("Exiting do_raw_table_nowan \n"); 
   return(0);
}

#ifdef INTEL_PUMA7
static int do_raw_table_puma7(FILE *fp)
{
   FIREWALL_DEBUG("Entering do_raw_table_puma7 \n"); 
	char str[MAX_QUERY];
	fprintf(stderr,"******DO RAW TABLE PUMA7 ****\n");

      	//use the raw table
      	isRawTableUsed = 1;

	snprintf(str, sizeof(str), "-A PREROUTING -i a-mux -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath0 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1000 -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath1 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1001 -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath2 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1002 -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath3 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1003 -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath4 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1004 -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath5 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1005 -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath6 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1006 -j NOTRACK");
	fprintf(fp, "%s\n", str);

	//For ath7 acceleration loop
	snprintf(str, sizeof(str), "-A PREROUTING -i wifilbr0.1007 -j NOTRACK");
	fprintf(fp, "%s\n", str);
   FIREWALL_DEBUG("Exiting do_raw_table_puma7 \n"); 
	return(0);
}
#endif
// static int prepare_multilan_firewall(FILE *nat_fp, FILE *filter_fp)
// {
    //Allow traffic through all psm configured networks.
// }

/*
===========================================================================
              
===========================================================================
*/


/*
 *  Procedure     : prepare_enabled_ipv4_firewall
 *  Purpose       : prepare ipv4 firewall
 *  Parameters    :
 *   raw_fp         : An open file for raw subtables
 *   mangle_fp      : an open file for writing mangle statements
 *   nat_fp         : an open file for writing nat statements
 *   filter_fp      : an open file for writing filter statements
 */
static int prepare_enabled_ipv4_firewall(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
   FIREWALL_DEBUG("Entering prepare_enabled_ipv4_firewall \n"); 
   /*
    * Add all of the tables and subtables that are required for the firewall
    */
   prepare_subtables(raw_fp, mangle_fp, nat_fp, filter_fp);

   /*
    * Add subtables created by plugins
    */
#define GUARDIAN_IPTABLES	"/tmp/guardian.ipt"
   if (access(GUARDIAN_IPTABLES, F_OK) == 0) {
      prepare_subtables_ext(GUARDIAN_IPTABLES, raw_fp, mangle_fp, nat_fp, filter_fp);
   }

   /*
    * Add firewall rules
    */
   //RAW tables is not used in USGv2
   //do_raw_ephemeral(raw_fp);
   //do_raw_table_general_rules(raw_fp);
   //do_raw_table_nowan(raw_fp);
#ifdef INTEL_PUMA7
   do_raw_table_puma7(raw_fp);
#endif
   add_qos_marking_statements(mangle_fp);

   do_port_forwarding(nat_fp, filter_fp);
   do_nonat(filter_fp);
   do_dmz(nat_fp, filter_fp);
   do_nat_ephemeral(nat_fp);
   do_wan_nat_lan_clients(nat_fp);

#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
   if(isGuestNetworkEnabled) {
       do_guestnet_walled_garden(nat_fp);
   }

   do_device_based_parcon(nat_fp, filter_fp);
#endif

   do_lan2self(filter_fp);
   do_wan2self(mangle_fp, nat_fp, filter_fp);
   do_lan2wan(mangle_fp, filter_fp, nat_fp); 
   do_wan2lan(filter_fp);
   do_filter_table_general_rules(filter_fp);
   do_raw_logs(raw_fp);
   do_logs(filter_fp);
   
   prepare_multinet_mangle(mangle_fp);
   
   //do_multinet_patch(mangle_fp, nat_fp, filter_fp);

   fprintf(raw_fp, "%s\n", "COMMIT");
   fprintf(mangle_fp, "%s\n", "COMMIT");
   fprintf(nat_fp, "%s\n", "COMMIT");
   fprintf(filter_fp, "%s\n", "COMMIT");
   FIREWALL_DEBUG("Exiting prepare_enabled_ipv4_firewall \n"); 
   return(0);
}

/*
 *  Procedure     : prepare_disabled_ipv4_firewall
 *  Purpose       : prepare ipv4 firewall in the case it is disabled (qos and nat may be enabled or disabled)
 *  Parameters    :
 *   raw_fp         : an open file for writing raw statements
 *   mangle_fp      : an open file for writing mangle statements
 *   nat_fp         : an open file for writing nat statements
 *   filter_fp      : an open file for writing filter statements
 */
static int prepare_disabled_ipv4_firewall(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
   /*
    * raw
    */
   FIREWALL_DEBUG("Entering prepare_disabled_ipv4_firewall \n"); 
   fprintf(raw_fp, "%s\n", "*raw");
   fprintf(raw_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(raw_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
#ifdef INTEL_PUMA7
   do_raw_table_puma7(raw_fp);
#endif
   fprintf(raw_fp, "%s\n", "COMMIT");

   /*
    * mangle
    */
   fprintf(mangle_fp, "%s\n", "*mangle");
   fprintf(mangle_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(mangle_fp, "%s\n", ":POSTROUTING ACCEPT [0:0]");
   fprintf(mangle_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
#ifdef CONFIG_BUILD_TRIGGER
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   fprintf(mangle_fp, "%s\n", ":prerouting_trigger - [0:0]");
#endif
#endif
   fprintf(mangle_fp, "%s\n", ":prerouting_qos - [0:0]");
   fprintf(mangle_fp, "%s\n", ":postrouting_qos - [0:0]");
   fprintf(mangle_fp, "%s\n", ":postrouting_lan2lan - [0:0]");
#ifdef CONFIG_BUILD_TRIGGER
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   fprintf(mangle_fp, "-A PREROUTING -j prerouting_trigger\n");
#endif
#endif
   fprintf(mangle_fp, "-A PREROUTING -j prerouting_qos\n");
   fprintf(mangle_fp, "-A POSTROUTING -j postrouting_qos\n");
   fprintf(mangle_fp, "-A POSTROUTING -j postrouting_lan2lan\n");
   add_qos_marking_statements(mangle_fp);
   fprintf(mangle_fp, "%s\n", "COMMIT");

   /*
    * nat
    */
   fprintf(nat_fp, "%s\n", "*nat");
   fprintf(nat_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(nat_fp, "%s\n", ":POSTROUTING ACCEPT [0:0]");
   fprintf(nat_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
   fprintf(nat_fp, "%s\n", ":postrouting_towan - [0:0]");
   fprintf(nat_fp, "-A POSTROUTING -o %s -j postrouting_towan\n", current_wan_ifname);
#if defined(_COSA_BCM_MIPS_)
   if(isBridgeMode) {       
       fprintf(nat_fp, "-A PREROUTING -d 10.0.0.1/32 -i %s -p tcp -j DNAT --to-destination %s\n", current_wan_ifname, lan0_ipaddr);
   }
#endif
   do_port_forwarding(nat_fp, NULL);
   do_nat_ephemeral(nat_fp);
   do_wan_nat_lan_clients(nat_fp);
   fprintf(nat_fp, "%s\n", "COMMIT");

   /*
    * filter
    */
   fprintf(filter_fp, "%s\n", "*filter");
   fprintf(filter_fp, "%s\n", ":INPUT ACCEPT [0:0]");
   fprintf(filter_fp, "%s\n", ":wan2self_mgmt - [0:0]");
   fprintf(filter_fp, "%s\n", ":lan2self_mgmt - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_wan2self - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_lan2self - [0:0]");
   fprintf(filter_fp, "-A xlog_drop_wan2self -j DROP\n");
   fprintf(filter_fp, "-A xlog_drop_lan2self -j DROP\n");

   if(isWanServiceReady || isBridgeMode) {
#if defined(_COSA_BCM_MIPS_)
       fprintf(filter_fp, "-A INPUT -p tcp --dport 80 -d %s -j ACCEPT\n",lan0_ipaddr);
#endif
       fprintf(filter_fp, "-A INPUT ! -i %s -j wan2self_mgmt\n", isBridgeMode == 0 ? lan_ifname : cmdiag_ifname);
       do_remote_access_control(NULL, filter_fp, AF_INET);
   }

   if(!isBridgeMode) //brlan0 exists
       fprintf(filter_fp, "-A INPUT -i %s -j lan2self_mgmt\n", lan_ifname);

   fprintf(filter_fp, "-A INPUT -i %s -j lan2self_mgmt\n", cmdiag_ifname); //lan0 always exist

   lan_telnet_ssh(filter_fp, AF_INET);

   #if defined(CONFIG_CCSP_LAN_HTTP_ACCESS)
   lan_http_access(filter_fp);
   #endif

   fprintf(filter_fp, "%s\n", ":FORWARD ACCEPT [0:0]");
   fprintf(filter_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
   fprintf(filter_fp, "%s\n", "COMMIT");
 FIREWALL_DEBUG("Exiting prepare_disabled_ipv4_firewall \n"); 
   return(0);
}

/*
 *  Procedure     : prepare_ipv4_firewall
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules
 *  Parameters    :
 *    fw_file        : The name of the file to which the firewall rules are written
 * Return Values  :
 *    0              : Success
 *   -1              : Bad input parameters
 *   -2              : Could not open firewall file
 * Notes          :
 *   If the fw_file does not exist, it will be created and used as the target of iptables-restore
 *   If the fw_file exists it will be overwritten, but the firewall itself will be updated using a
 *      series of iptables statements.
 *   The syscfg subsystem must be initialized prior to calling this function
 *   The sysevent subsytem must be initializaed prior to calling this function
 */
int prepare_ipv4_firewall(const char *fw_file)
{
 FIREWALL_DEBUG("Inside prepare_ipv4_firewall \n"); 
   /*
    * fw_file is the name of the file that we write firewall statement to.
    * This file is used by iptables-restore to provision the firewall.
    */
   if (NULL == fw_file) {
      return(-1);
   }

   FILE *fp = fopen(fw_file, "w"); 
   if (NULL == fp) {
      return(-2);
   }

   /*
    * We use 4 files to store the intermediary firewall statements.
    * One file is for raw, another is for mangle, another is for 
    * nat tables statements, and the other is for filter statements.
    */
   pid_t ourpid = getpid();
   char  fname[50];

   snprintf(fname, sizeof(fname), "/tmp/raw_%x", ourpid);
   FILE *raw_fp = fopen(fname, "w+");
   if (NULL == raw_fp) {
      fclose(fp);
      return(-2);
   }
   snprintf(fname, sizeof(fname), "/tmp/mangle_%x", ourpid);
   FILE *mangle_fp = fopen(fname, "w+");
   if (NULL == mangle_fp) {
      fclose(fp);
      return(-2);
   }
   snprintf(fname, sizeof(fname), "/tmp/filter_%x", ourpid);
   FILE *filter_fp = fopen(fname, "w+");
   if (NULL == filter_fp) {
      fclose(fp);
      fclose(mangle_fp);
      return(-2);
   }
   snprintf(fname, sizeof(fname), "/tmp/nat_%x", ourpid);
   FILE *nat_fp = fopen(fname, "w+");
   if (NULL == nat_fp) {
      fclose(fp);
      fclose(mangle_fp);
      fclose(filter_fp);
      return(-2);
   }
  
   // TODO: possibly remove bridge mode
   if (isFirewallEnabled && !isBridgeMode ) { fprintf(stderr, "-- prepare_enabled_ipv4_firewall isWanServiceReady=%d\n", isWanServiceReady); //&& isWanServiceReady) {
      prepare_enabled_ipv4_firewall(raw_fp, mangle_fp, nat_fp, filter_fp);
   } else {
      prepare_disabled_ipv4_firewall(raw_fp, mangle_fp, nat_fp, filter_fp);
   }
   
   //prepare_multilan_firewall(nat_fp, filter_fp);

   rewind(raw_fp);
   rewind(mangle_fp);
   rewind(nat_fp);
   rewind(filter_fp);
   char string[MAX_QUERY];
   char *strp;
   /*
    * The raw table is before conntracking and is thus expensive
    * So we dont set it up unless we actually used it
    */
   if (isRawTableUsed) {
      while (NULL != (strp = fgets(string, MAX_QUERY, raw_fp)) ) {
         fprintf(fp, "%s", string);
      }
   } else {
      fprintf(fp, "*raw\n-F\nCOMMIT\n");
   }
   while (NULL != (strp = fgets(string, MAX_QUERY, mangle_fp)) ) {
      fprintf(fp, "%s", string);
   }
   while (NULL != (strp = fgets(string, MAX_QUERY, nat_fp)) ) {
      fprintf(fp, "%s", string);
   }
   while (NULL != (strp = fgets(string, MAX_QUERY, filter_fp)) ) {
      fprintf(fp, "%s", string);
   }
   
   fclose(fp);
   fclose(raw_fp);
   fclose(mangle_fp);
   fclose(nat_fp);
   fclose(filter_fp);

   snprintf(fname, sizeof(fname), "/tmp/raw_%x", ourpid);
   unlink(fname);
   snprintf(fname, sizeof(fname), "/tmp/mangle_%x", ourpid);
   unlink(fname);
   snprintf(fname, sizeof(fname), "/tmp/filter_%x", ourpid);
   unlink(fname);
   snprintf(fname, sizeof(fname), "/tmp/nat_%x", ourpid);
   unlink(fname);
 FIREWALL_DEBUG("Exiting prepare_ipv4_firewall \n"); 
   return(0);
}

/*
 *  Procedure     : prepare_stopped_ipv4_firewall
 *  Purpose       : prepare ipv4 firewall to stop all services (firewall, nat, qos) 
 *                  irrespective of their configuration
 *  Parameters    :
 *   file_fp         : an open file for writing iptables statements
 */
static int prepare_stopped_ipv4_firewall(FILE *file_fp)
{
 FIREWALL_DEBUG("Inside prepare_stopped_ipv4_firewall \n"); 
   /*
    * raw
    */
#ifdef NOTDEF
   fprintf(file_fp, "%s\n", "*raw");
   fprintf(file_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", "COMMIT");
#endif

   /*
    * mangle
    */
   fprintf(file_fp, "%s\n", "*mangle");
   fprintf(file_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", ":POSTROUTING ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", "COMMIT");

   /*
    * nat
    */
   fprintf(file_fp, "%s\n", "*nat");
   fprintf(file_fp, "%s\n", ":PREROUTING ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", ":POSTROUTING ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", "COMMIT");

   /*
    * filter
    */
   fprintf(file_fp, "%s\n", "*filter");
   fprintf(file_fp, "%s\n", ":INPUT ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", ":FORWARD ACCEPT [0:0]");
   fprintf(file_fp, "%s\n", ":OUTPUT ACCEPT [0:0]");

   //Comment out to disable telnet/ssh in stopped firewall
   //fprintf(file_fp, ":lan2self_mgmt - [0:0]\n");
   //fprintf(file_fp, "-A INPUT -j lan2self_mgmt\n");
   //lan_telnet_ssh(file_fp, AF_INET);

   fprintf(file_fp, "%s\n", "COMMIT");
 FIREWALL_DEBUG("Exiting prepare_stopped_ipv4_firewall \n"); 
   return(0);
}

//Hardcoded support for cm and erouter should be generalized.
static char * ifnames[] = { wan6_ifname, ecm_wan_ifname, emta_wan_ifname};
static int numifs = sizeof(ifnames) / sizeof(*ifnames);

static void do_ipv6_sn_filter(FILE* fp) {
 FIREWALL_DEBUG("Inside do_ipv6_sn_filter \n"); 
    int i;
    char mcastAddrStr[64];
    char ifIpv6AddrKey[64];
    fprintf(fp, "*mangle\n");
    
    
    
    for (i = 0; i < numifs; ++i) {
        snprintf(ifIpv6AddrKey, sizeof(ifIpv6AddrKey), "ipv6_%s_dhcp_solicNodeAddr", ifnames[i]);
        sysevent_get(sysevent_fd, sysevent_token, ifIpv6AddrKey, mcastAddrStr, sizeof(mcastAddrStr));
        if (mcastAddrStr[0] != '\0')
            fprintf(fp, "-A PREROUTING -i %s -d %s -p ipv6-icmp -m icmp6 --icmpv6-type 135 -m limit --limit 20/sec -j ACCEPT\n", ifnames[i], mcastAddrStr);
        
        snprintf(ifIpv6AddrKey, sizeof(ifIpv6AddrKey), "ipv6_%s_ll_solicNodeAddr", ifnames[i]);
        sysevent_get(sysevent_fd, sysevent_token, ifIpv6AddrKey, mcastAddrStr, sizeof(mcastAddrStr));
        if (mcastAddrStr[0] != '\0')
            fprintf(fp, "-A PREROUTING -i %s -d %s -p ipv6-icmp -m icmp6 --icmpv6-type 135 -m limit --limit 20/sec -j ACCEPT\n", ifnames[i], mcastAddrStr);
        
        fprintf(fp, "-A PREROUTING -i %s -d ff00::/8 -p ipv6-icmp -m icmp6 --icmpv6-type 135 -m limit --limit 20/sec -j ACCEPT\n", ifnames[i]);
        fprintf(fp, "-A PREROUTING -i %s -d ff00::/8 -p ipv6-icmp -m icmp6 --icmpv6-type 135 -j DROP\n", ifnames[i]);
    }
	//Commenting out all DSCP rules : RDKB-5254
	//zqiu: XCONF >>  RDKB-4519
		//fprintf(fp, "-A FORWARD -m state --state NEW -j DSCP --set-dscp-class af22\n");
		//fprintf(fp, "-A FORWARD -m state ! --state NEW -j DSCP  --set-dscp 0x0\n");
		//fprintf(fp, "-A OUTPUT -o erouter0 -j DSCP --set-dscp-class af22\n");
		//fprintf(fp, "-A POSTROUTING -o erouter0 -p gre -j DSCP --set-dscp %d \n",greDscp);
		//fprintf(fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class af32 -j CONNMARK --set-mark 0xA\n");
		//fprintf(fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class cs1 -j CONNMARK --set-mark 0xB\n");
		//fprintf(fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class cs5 -j CONNMARK --set-mark 0xC\n");
		//fprintf(fp, "-I PREROUTING -i erouter0 -m dscp --dscp-class af22 -j CONNMARK --set-mark 0xD\n");
		//fprintf(fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xA  -j DSCP --set-dscp-class af32\n");
		//fprintf(fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xB -j DSCP --set-dscp-class cs1\n");
		//fprintf(fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xC -j DSCP --set-dscp-class cs5\n");
		//fprintf(fp, "-A POSTROUTING -o erouter0 -m connmark --mark 0xD -j DSCP --set-dscp-class af22\n");	
	//zqiu: XCONF <<
	
     FIREWALL_DEBUG("Exiting do_ipv6_sn_filter \n"); 
}
static void do_ipv6_nat_table(FILE* fp)
{
 FIREWALL_DEBUG("Inside do_ipv6_nat_table \n");
    char mcastAddrStr[64];
    char IPv6[64] = "0";
	int rc;
    fprintf(fp, "*nat\n");
	
   fprintf(fp, "%s\n", ":prerouting_devices - [0:0]");
   fprintf(fp, "%s\n", ":prerouting_redirect - [0:0]");

 
   syscfg_get(NULL, "HTTP_Server_IPv6", IPv6, sizeof(IPv6));
   fprintf(fp, "-A prerouting_redirect -p tcp --dport 80 -j DNAT --to-destination [%s]:80\n",IPv6);
 	

   IPv6[0] = '\0';
   syscfg_get(NULL, "HTTPS_Server_IPv6", IPv6, sizeof(IPv6));
   fprintf(fp, "-A prerouting_redirect -p tcp --dport 443 -j DNAT --to-destination [%s]:443\n",IPv6);
      
   IPv6[0] = '\0';  
   syscfg_get(NULL, "Default_Server_IPv6", IPv6, sizeof(IPv6));
   fprintf(fp, "-A prerouting_redirect -p tcp -j DNAT --to-destination [%s]:443\n",IPv6);
   fprintf(fp, "-A prerouting_redirect -p udp ! --dport 53 -j DNAT --to-destination [%s]:80\n",IPv6);
   
    FIREWALL_DEBUG("Exiting do_ipv6_nat_table \n");
}

static void do_ipv6_filter_table(FILE *fp);

/*
 ****************************************************************
 *               IPv6 Firewall                                  *
 ****************************************************************
 */

/*
 *  Procedure     : prepare_ipv6_firewall
 *  Purpose       : prepare the ip6tables-restore file that establishes all
 *                  ipv6 firewall rules
 *  Paramenters   :
 *    fw_file        : The name of the file to which the firewall rules are written
 * Return Values  :
 *    0              : Success
 *   -1              : Bad input parameters
 *   -2              : Could not open firewall file
 * Notes          :
 *   If the fw_file exists it will be overwritten.
 *   The syscfg subsystem must be initialized prior to calling this function
 *   The sysevent subsytem must be initializaed prior to calling this function
 */
int prepare_ipv6_firewall(const char *fw_file)
{
 FIREWALL_DEBUG("Inside prepare_ipv6_firewall \n");
   if (NULL == fw_file) {
      return(-1);
   }
   FILE *fp = fopen(fw_file, "w");
   if (NULL == fp) {
      return(-2);
   }
   
   sysevent_get(sysevent_fd, sysevent_token, "current_wan_ipv6_interface", wan6_ifname, sizeof(wan6_ifname));
   
   if (wan6_ifname[0] == '\0') 
       strcpy(wan6_ifname, current_wan_ifname);

	int ret=0;
	FILE *raw_fp=NULL;
	FILE *mangle_fp=NULL;
	FILE *filter_fp=NULL;
	FILE *nat_fp=NULL;
    /*
    * We use 4 files to store the intermediary firewall statements.
    * One file is for raw, another is for mangle, another is for 
    * nat tables statements, and the other is for filter statements.
    */
	pid_t ourpid = getpid();
	char  fname[50];
	
	snprintf(fname, sizeof(fname), "/tmp/raw6_%x", ourpid);
	raw_fp = fopen(fname, "w+");
	if (NULL == raw_fp) {
		ret=-2;
		goto clean_up_files;
	}
	
	snprintf(fname, sizeof(fname), "/tmp/mangle6_%x", ourpid);
	mangle_fp = fopen(fname, "w+");
	if (NULL == mangle_fp) {
		ret=-2;
		goto clean_up_files;
	}
	snprintf(fname, sizeof(fname), "/tmp/filter6_%x", ourpid);
	filter_fp = fopen(fname, "w+");
	if (NULL == filter_fp) {
		ret=-2;
		goto clean_up_files;
	}
	snprintf(fname, sizeof(fname), "/tmp/nat6_%x", ourpid);
	nat_fp = fopen(fname, "w+");
	if (NULL == nat_fp) {
		ret=-2;
		goto clean_up_files;
	}

#ifdef INTEL_PUMA7
	fprintf(raw_fp, "*raw\n");
	do_raw_table_puma7(raw_fp);
#endif
   
	do_ipv6_sn_filter(mangle_fp);
	do_ipv6_nat_table(nat_fp);
	
	do_ipv6_filter_table(filter_fp);
	
	do_parental_control(filter_fp,nat_fp, 6);
	
	/*add rules before this*/

	fprintf(raw_fp, "COMMIT\n");
	fprintf(mangle_fp, "COMMIT\n");
	fprintf(nat_fp, "COMMIT\n");
	fprintf(filter_fp, "COMMIT\n");
	
	rewind(raw_fp);
	rewind(mangle_fp);
	rewind(nat_fp);
	rewind(filter_fp);
	char string[MAX_QUERY]={0};
	char *strp=NULL;
	/*
	* The raw table is before conntracking and is thus expensive
	* So we dont set it up unless we actually used it
	*/
	if (isRawTableUsed) {
		while (NULL != (strp = fgets(string, MAX_QUERY, raw_fp)) ) {
		   fprintf(fp, "%s", string);
		}
	} else {
		fprintf(fp, "*raw\n-F\nCOMMIT\n");
	}
	while (NULL != (strp = fgets(string, MAX_QUERY, mangle_fp)) ) {
		fprintf(fp, "%s", string);
	}
	while (NULL != (strp = fgets(string, MAX_QUERY, nat_fp)) ) {
		fprintf(fp, "%s", string);
	}
	while (NULL != (strp = fgets(string, MAX_QUERY, filter_fp)) ) {
		fprintf(fp, "%s", string);
	}

clean_up_files:	 
	if(fp)
		fclose(fp);
	if(raw_fp) {
		fclose(raw_fp);
		snprintf(fname, sizeof(fname), "/tmp/raw6_%x", ourpid);
	 	unlink(fname);
	}
	if(mangle_fp) {
		fclose(mangle_fp);
		snprintf(fname, sizeof(fname), "/tmp/mangle6_%x", ourpid);
	 	unlink(fname);
	}
	if(nat_fp) {
		fclose(nat_fp);
		snprintf(fname, sizeof(fname), "/tmp/filter6_%x", ourpid);
	 	unlink(fname);
	}
	if(filter_fp) {
		fclose(filter_fp);
		snprintf(fname, sizeof(fname), "/tmp/nat6_%x", ourpid);
		unlink(fname);
	}
	FIREWALL_DEBUG("Exiting prepare_ipv6_firewall \n"); 
	return ret;
}

static void do_ipv6_filter_table(FILE *fp){
	FIREWALL_DEBUG("Inside do_ipv6_filter_table \n");
	
   fprintf(fp, "*filter\n");
   fprintf(fp, ":INPUT ACCEPT [0:0]\n");
   fprintf(fp, ":FORWARD ACCEPT [0:0]\n");
   fprintf(fp, ":OUTPUT ACCEPT [0:0]\n");

#ifdef INTEL_PUMA7
   //Avoid blocking packets at the Intel NIL layer
   fprintf(fp, "-A FORWARD -i a-mux -j ACCEPT\n");
#endif

   if (!isFirewallEnabled || isBridgeMode || !isWanServiceReady) {
       if(isBridgeMode || isWanServiceReady)
           do_remote_access_control(NULL, fp, AF_INET6);

       lan_telnet_ssh(fp, AF_INET6);
       goto end_of_ipv6_firewall;
   }

   fprintf(fp, "%s\n", ":LOG_INPUT_DROP - [0:0]");
   fprintf(fp, "%s\n", ":LOG_FORWARD_DROP - [0:0]");
  
   fprintf(fp, "-A LOG_INPUT_DROP -m limit --limit 1/minute -j LOG --log-level %d --log-prefix \"UTOPIA: FW.IPv6 INPUT drop\"\n",syslog_level);
   fprintf(fp, "-A LOG_FORWARD_DROP -m limit --limit 1/minute -j LOG --log-level %d --log-prefix \"UTOPIA: FW.IPv6 FORWARD drop\"\n",syslog_level);
   fprintf(fp, "-A LOG_INPUT_DROP -j DROP\n"); 
   fprintf(fp, "-A LOG_FORWARD_DROP -j DROP\n"); 

   fprintf(fp, "%s\n", ":PING_FLOOD - [0:0]");
   fprintf(fp, "-A PING_FLOOD -m limit --limit 10/sec -j ACCEPT\n");
   fprintf(fp, "-A PING_FLOOD -j DROP\n");

   //ban telnet and ssh from lan side
   lan_telnet_ssh(fp, AF_INET6);

   if (isFirewallEnabled) {
      // Get the current WAN IPv6 interface (which differs from the IPv4 in case of tunnels)
      char query[10],port[10],tmpQuery[10];
      int rc, ret;

      
      // not sure if this is the right thing to do, but if there is no current_wan_ipv6_interface several iptables statements fail
      if ('\0' == wan6_ifname[0]) {
         snprintf(wan6_ifname, sizeof(wan6_ifname), "%s", current_wan_ifname);
      }
      query[0] = '\0';
      port[0] = '\0';
      rc = syscfg_get(NULL, "mgmt_wan_httpaccess", query, sizeof(query));
#if defined(CONFIG_CCSP_WAN_MGMT_ACCESS)
      tmpQuery[0] = '\0';
      ret = syscfg_get(NULL, "mgmt_wan_httpaccess_ert", tmpQuery, sizeof(tmpQuery));
      if(ret == 0)
          strcpy(query, tmpQuery);
#endif
      if (0 == rc && '\0' != query[0] && (0 !=  strncmp(query, "0", sizeof(query))) ) {

          rc = syscfg_get(NULL, "mgmt_wan_httpport", port, sizeof(port));
#if defined(CONFIG_CCSP_WAN_MGMT_PORT)
          tmpQuery[0] = '\0';
          ret = syscfg_get(NULL, "mgmt_wan_httpport_ert", tmpQuery, sizeof(tmpQuery));
          if(ret == 0)
              strcpy(port, tmpQuery);
#endif

          if (0 != rc || '\0' == port[0]) {
            snprintf(port, sizeof(port), "%d", 8080);
         }
      }

      // Accept everything from localhost
      fprintf(fp, "-A INPUT -i lo -j ACCEPT\n");

      // Block the evil routing header type 0
      fprintf(fp, "-A INPUT -m rt --rt-type 0 -j DROP\n");

      fprintf(fp, "-A INPUT -m state --state INVALID -j LOG_INPUT_DROP\n");

#if defined(_COSA_BCM_MIPS_)
      fprintf(fp, "-A INPUT -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
      fprintf(fp, "-A INPUT -m physdev --physdev-out %s -j ACCEPT\n", emta_wan_ifname);
#endif
      // Allow cfgserv through 
      //fprintf(fp, "-A INPUT -p udp -d ff80::114/64 --dport 5555 -j ACCEPT\n");
      //fprintf(fp, "-A INPUT -p tcp --dport 3005 -j ACCEPT\n");

      // Block all packet whose source is mcast
      fprintf(fp, "-A INPUT -s ff00::/8  -j DROP\n");

      // Should include --limit 10/second for most of ICMP
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 1/0 -m limit --limit 10/sec -j ACCEPT\n"); // No route
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 2 -m limit --limit 10/sec -j ACCEPT\n"); // Packet too big
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 3 -m limit --limit 10/sec -j ACCEPT\n"); // Time exceeded
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 4/1 -m limit --limit 10/sec -j ACCEPT\n"); // Unknown header type
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 4/2 -m limit --limit 10/sec -j ACCEPT\n"); // Unknown option

      //ping is allowed for cm and emta regardless whatever firewall level is
      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j PING_FLOOD\n", ecm_wan_ifname); // Echo request
      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m limit --limit 10/sec -j ACCEPT\n", ecm_wan_ifname); // Echo reply

      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j PING_FLOOD\n", emta_wan_ifname); // Echo request
      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m limit --limit 10/sec -j ACCEPT\n", emta_wan_ifname); // Echo reply

      /* not allow ping wan0 from brlan0 */
      int i;
      for(i = 0; i < ecm_wan_ipv6_num; i++){
         fprintf(fp, "-A INPUT -i %s -d %s -p icmpv6 -m icmp6 --icmpv6-type 128  -j LOG_INPUT_DROP\n", lan_ifname, ecm_wan_ipv6[i]);
      }
      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j PING_FLOOD\n", lan_ifname); // Echo request
      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m limit --limit 10/sec -j ACCEPT\n", lan_ifname); // Echo reply

      if ( (isPingBlockedV6 && strncasecmp(firewall_levelv6, "Custom", strlen("Custom")) == 0)
              || strncasecmp(firewall_levelv6, "High", strlen("High")) == 0
              || strncasecmp(firewall_levelv6, "Medium", strlen("Medium")) == 0 )
      {
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j DROP\n", current_wan_ifname); // Echo request
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m state --state NEW,INVALID,RELATED -j DROP\n", current_wan_ifname); // Echo reply

      }
      else if(strncasecmp(firewall_levelv6, "Low", strlen("Low")) == 0) 
      {
      #if defined(CONFIG_CCSP_DROP_ICMP_PING)
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j DROP\n", current_wan_ifname); // Echo request
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m state --state NEW,INVALID,RELATED -j DROP\n", current_wan_ifname); // Echo reply
      #else
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j PING_FLOOD\n", current_wan_ifname); // Echo request
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m limit --limit 10/sec -j ACCEPT\n", current_wan_ifname); // Echo reply
      #endif
      }
      else
      {
          //fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 128 -m limit --limit 10/sec -j ACCEPT\n"); // Echo request
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j PING_FLOOD\n", current_wan_ifname); // Echo request
          fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m limit --limit 10/sec -j ACCEPT\n", current_wan_ifname); // Echo reply
      }

      // Should only come from LINK LOCAL addresses, rate limited except 100/second for NA/NS and RS
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 135 -m limit --limit 100/sec -j ACCEPT\n"); // Allow NS from any type source address
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 136 -m limit --limit 100/sec -j ACCEPT\n"); // NA

      //fprintf(fp, "-A INPUT -s fe80::/64 -d ff02::1/128 -i %s -p icmpv6 -m icmp6 --icmpv6-type 134 -m limit --limit 10/sec -j ACCEPT\n", current_wan_ifname); // periodic RA
      fprintf(fp, "-A INPUT -s fe80::/64 -d ff02::1/128 ! -i %s -p icmpv6 -m icmp6 --icmpv6-type 134 -m limit --limit 10/sec -j ACCEPT\n", lan_ifname); // periodic RA

      if (strcmp(current_wan_ifname, wan6_ifname)) // Also accept from wan6_ifname in case of tunnel
         fprintf(fp, "-A INPUT -s fe80::/64 -d ff02::1/128 -i %s -p icmpv6 -m icmp6 --icmpv6-type 134 -m limit --limit 10/sec -j ACCEPT\n", wan6_ifname); // periodic RA

      //fprintf(fp, "-A INPUT -s fe80::/64 -d fe80::/64 -i %s -p icmpv6 -m icmp6 --icmpv6-type 134 -m limit --limit 10/sec -j ACCEPT\n", current_wan_ifname); // sollicited RA
      fprintf(fp, "-A INPUT -s fe80::/64 -d fe80::/64 ! -i %s -p icmpv6 -m icmp6 --icmpv6-type 134 -m limit --limit 10/sec -j ACCEPT\n", lan_ifname); // sollicited RA

      if (strcmp(current_wan_ifname, wan6_ifname)) // Also accept from wan6_ifname in case of tunnel
         fprintf(fp, "-A INPUT -s fe80::/64 -d fe80::/64 -i %s -p icmpv6 -m icmp6 --icmpv6-type 134 -m limit --limit 10/sec -j ACCEPT\n", wan6_ifname); // sollicited RA

      fprintf(fp, "-A INPUT -s fe80::/64 -i %s -p icmpv6 -m icmp6 --icmpv6-type 133 -m limit --limit 100/sec -j ACCEPT\n", lan_ifname); //RS

      // But can also come from UNSPECIFIED addresses, rate limited 100/second for NS (for DAD) and MLD
      fprintf(fp, "-A INPUT -s ::/128 -p icmpv6 -m icmp6 --icmpv6-type 135 -m limit --limit 100/sec -j ACCEPT\n"); // NS
      fprintf(fp, "-A INPUT -s ::/128 -p icmpv6 -m icmp6 --icmpv6-type 143 -m limit --limit 100/sec -j ACCEPT\n"); // MLD

      // IPV6 Multicast traffic
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 130 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 131 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 132 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 143 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 151 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 152 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 153 -m limit --limit 10/sec -j ACCEPT\n");
      
      // Allow SSDP 
      fprintf(fp, "-A INPUT -i %s -p udp --dport 1900 -j ACCEPT\n", lan_ifname);

      // Normal ports for Management interface
      do_lan2self_by_wanip6(fp);
      fprintf(fp, "-A INPUT -i %s -p tcp -m tcp --dport 80 --tcp-flags FIN,SYN,RST,ACK SYN -m limit --limit 10/sec -j ACCEPT\n", lan_ifname);
      fprintf(fp, "-A INPUT -i %s -p tcp -m tcp --dport 443 --tcp-flags FIN,SYN,RST,ACK SYN -m limit --limit 10/sec -j ACCEPT\n", lan_ifname);
      //if (port[0])
      //   fprintf(fp, "-A INPUT -i %s -p tcp -m tcp --dport %s --tcp-flags FIN,SYN,RST,ACK SYN -m limit --limit 10/sec -j ACCEPT\n", wan6_ifname,port);

      do_remote_access_control(NULL, fp, AF_INET6);

      // Development override
      if (isDevelopmentOverride) {
        fprintf(fp, "-A INPUT -p tcp -m tcp --dport 22 -j ACCEPT\n");
        fprintf(fp, "-A INPUT -p tcp -m tcp --dport 80 -j ACCEPT\n");
        fprintf(fp, "-A INPUT -p tcp -m tcp --dport 443 -j ACCEPT\n");
        fprintf(fp, "-A INPUT -p tcp -m tcp --dport 8080 -j ACCEPT\n");
      }

      // established communication from anywhere is accepted
      fprintf(fp, "-A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT\n");

#if defined(_COSA_BCM_MIPS_)
      fprintf(fp, "-A INPUT -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
      fprintf(fp, "-A INPUT -m physdev --physdev-out %s -j ACCEPT\n", emta_wan_ifname);
#endif

      // for tftp software download to work
      fprintf(fp, "-A INPUT -i %s -p udp --dport 53 -j DROP\n", ecm_wan_ifname);
      fprintf(fp, "-A INPUT -i %s -p udp --dport 67 -j DROP\n", ecm_wan_ifname);
      fprintf(fp, "-A INPUT -i %s -p udp --dport 514 -j DROP\n", ecm_wan_ifname);
      fprintf(fp, "-A INPUT -i %s -j ACCEPT\n", ecm_wan_ifname);

      // Return traffic. The equivalent of IOS 'established'
      fprintf(fp, "-A INPUT -p tcp -m tcp ! --tcp-flags FIN,SYN,RST,ACK SYN -j ACCEPT\n");

      //Captive Portal 
      // Commenting out DROP 53 part as it will create confusion to end user
      //if(isInCaptivePortal()==1)
      //{
           //fprintf(fp, "-I INPUT -i %s -p udp -m udp --dport 53 -j DROP\n", lan_ifname);
           //fprintf(fp, "-I INPUT -i %s -p tcp -m tcp --dport 53 -j DROP\n", lan_ifname);
      //}
      //else
      //{
           // DNS resolver request from client
           // DNS server replies from Internet servers
           fprintf(fp, "-A INPUT -i %s -p udp -m udp --dport 53 -m limit --limit 100/sec -j ACCEPT\n", lan_ifname);
           //fprintf(fp, "-A INPUT -i %s -p udp -m udp --sport 53 -m limit --limit 100/sec -j ACCEPT\n", wan6_ifname);
           fprintf(fp, "-A INPUT ! -i %s -p udp -m udp --sport 53 -m limit --limit 100/sec -j ACCEPT\n", lan_ifname);
      //}


      // NTP request from client
      // NTP server replies from Internet servers
      fprintf(fp, "-A INPUT -i %s -p udp -m udp --dport 123 -m limit --limit 10/sec -j ACCEPT\n", lan_ifname);
      //fprintf(fp, "-A INPUT -i %s -p udp -m udp --sport 123 -m limit --limit 10/sec -j ACCEPT\n", wan6_ifname);
      fprintf(fp, "-A INPUT ! -i %s -p udp -m udp --sport 123 -m limit --limit 10/sec -j ACCEPT\n", lan_ifname);

      // DHCPv6 from inside clients (high rate in case of global reboot)
      fprintf(fp, "-A INPUT -i %s -p udp -m udp --dport 547 -m limit --limit 100/sec -j ACCEPT\n", lan_ifname);

      // DHCPv6 from outside server (low rate as only a couple of potential DHCP servers)
      //fprintf(fp, "-A INPUT -i %s -p udp -m udp --dport 546 -m limit --limit 10/sec -j ACCEPT\n", current_wan_ifname);
      //fprintf(fp, "-A INPUT -i %s -p udp -m udp --dport 546 -m limit --limit 10/sec -j ACCEPT\n", wan6_ifname);
      fprintf(fp, "-A INPUT ! -i %s -p udp -m udp --dport 546 -m limit --limit 10/sec -j ACCEPT\n", lan_ifname);

      // IPv4 in IPv6 (for DS-lite)
      fprintf(fp, "-A INPUT -i %s -p 4 -j ACCEPT\n", wan6_ifname);

      //SNMP
      //fprintf(fp, "-A INPUT -i %s -p udp --dport 161 -j ACCEPT\n", ecm_wan_ifname);
      fprintf(fp, "-A INPUT ! -i %s -p udp --dport 161 -j ACCEPT\n", lan_ifname);

      // add user created rules from syscfg
      int  idx;
      char rule_query[MAX_QUERY];
      char in_rule[MAX_QUERY];
      char subst[MAX_QUERY];
      int  count;

      rule_query[0] = '\0';
      in_rule[0] = '\0'; //TODO addto 1.5.1
      syscfg_get(NULL, "v6GeneralPurposeFirewallRuleCount", in_rule, sizeof(in_rule));
      if ('\0' == in_rule[0]) {
         goto v6GPFirewallRuleNext;
      } else {
         count = atoi(in_rule);
         if (0 == count) {
            goto v6GPFirewallRuleNext;
         }
         if (MAX_SYSCFG_ENTRIES < count) {
            count = MAX_SYSCFG_ENTRIES;
         }
      }

      memset(in_rule, 0, sizeof(in_rule));
      for (idx=1; idx<=count; idx++) {
         snprintf(rule_query, sizeof(rule_query), "v6GeneralPurposeFirewallRule_%d", idx);
         syscfg_get(NULL, rule_query, in_rule, sizeof(in_rule));
         if ('\0' != in_rule[0]) {
            /*
             * the rule we just got could contain variables that we need to substitute
             * for runtime/configuration values
             */
            fprintf(fp,"%s\n", make_substitutions(in_rule, subst, sizeof(subst)));
         }
         memset(in_rule, 0, sizeof(in_rule));
      }

v6GPFirewallRuleNext:

      //fprintf(fp, "-A INPUT -m limit --limit 10/sec -j REJECT --reject-with icmp6-adm-prohibited\n");

      // Open destination port 12368 on wan0 to allow dbus communication between tpg and cns 
      //fprintf(fp, "-A INPUT -i %s -p tcp -m tcp --dport 12368 -j ACCEPT\n", wan6_ifname);
      //fprintf(fp, "-A INPUT -i %s -p udp -m udp --dport 12368 -j ACCEPT\n", wan6_ifname);

      // Open destination port 36367 on wan0 to allow sysevent communication between tpg and cns 
      //fprintf(fp, "-A INPUT -i %s -p tcp -m tcp --dport 36367 -j ACCEPT\n", wan6_ifname);

      // Logging and rejecting politely (rate limiting anyway)
      fprintf(fp, "-A INPUT -j LOG_INPUT_DROP \n");

      // Block the evil routing header type 0
      fprintf(fp, "-A FORWARD -m rt --rt-type 0 -j LOG_FORWARD_DROP \n");

#if defined(_COSA_BCM_MIPS_)
      fprintf(fp, "-A FORWARD -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
      fprintf(fp, "-A FORWARD -m physdev --physdev-out %s -j ACCEPT\n", emta_wan_ifname);
#endif

      // Link local should never be forwarded
      fprintf(fp, "-A FORWARD -s fe80::/64 -j LOG_FORWARD_DROP\n");
      fprintf(fp, "-A FORWARD -d fe80::/64 -j LOG_FORWARD_DROP\n");

      // Block all packet whose source is mcast
      fprintf(fp, "-A FORWARD -s ff00::/8  -j LOG_FORWARD_DROP\n");

      // Block all packet whose destination is mcast with organization scope
      fprintf(fp, "-A FORWARD -d ff08::/16  -j LOG_FORWARD_DROP\n");

      // Block all packet whose source or destination is the deprecated site local
      fprintf(fp, "-A FORWARD -s ec00::/10  -j LOG_FORWARD_DROP\n");
      fprintf(fp, "-A FORWARD -d ec00::/10  -j LOG_FORWARD_DROP\n");
      // Block all packet whose source or destination is IPv4 compatible address
      fprintf(fp, "-A FORWARD -s 0::/4  -j LOG_FORWARD_DROP\n");
      fprintf(fp, "-A FORWARD -d 0::/4  -j LOG_FORWARD_DROP\n");

      // Basic RPF check on the egress & ingress traffic
      char prefix[129];
      sysevent_get(sysevent_fd, sysevent_token, "ipv6_prefix", prefix, sizeof(prefix));
      if ( '\0' != prefix[0] ) {
         //fprintf(fp, "-A FORWARD ! -s %s -i %s -m limit --limit 10/sec -j LOG --log-level %d --log-prefix \"UTOPIA: FW. IPv6 FORWARD anti-spoofing\"\n", prefix, lan_ifname,syslog_level);
         //fprintf(fp, "-A FORWARD ! -s %s -i %s -m limit --limit 10/sec -j REJECT --reject-with icmp6-adm-prohibited\n", prefix, lan_ifname);
         fprintf(fp, "-A FORWARD ! -s %s -i %s -j LOG_FORWARD_DROP\n", prefix, lan_ifname);
         fprintf(fp, "-A FORWARD -s %s -i %s -j LOG_FORWARD_DROP\n", prefix, wan6_ifname);
      }

      fprintf(fp, ":lan2wan - [0:0]\n");
      fprintf(fp, "-A FORWARD -i %s -o %s -j lan2wan\n", lan_ifname, wan6_ifname);
      fprintf(fp, "-A FORWARD -i %s -o %s -j lan2wan\n", lan_ifname, ecm_wan_ifname);
      fprintf(fp, "-A FORWARD -i %s -o %s -j lan2wan\n", lan_ifname, emta_wan_ifname);

      fprintf(fp, ":lan2wan_pc_device - [0:0]\n");
      fprintf(fp, ":lan2wan_pc_site - [0:0]\n");
      fprintf(fp, ":lan2wan_pc_service - [0:0]\n");

      fprintf(fp, "-A lan2wan -j lan2wan_pc_device\n");
      fprintf(fp, "-A lan2wan -j lan2wan_pc_site\n");
      fprintf(fp, "-A lan2wan -j lan2wan_pc_service\n");

#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
      fprintf(fp, "%s\n", ":wan2lan_dnsr_nfqueue - [0:0]");
      fprintf(fp, "-A FORWARD -i %s -p udp --sport 53 -j wan2lan_dnsr_nfqueue\n", wan6_ifname);
#endif


      if (strncasecmp(firewall_levelv6, "High", strlen("High")) == 0)
      {
         /* The following rules are the same as IPv4
         fprintf(fp, "-A lan2wan -p tcp --dport 80   -j ACCEPT\n"); // HTTP
         fprintf(fp, "-A lan2wan -p tcp --dport 443  -j ACCEPT\n"); // HTTPS
         fprintf(fp, "-A lan2wan -p udp --dport 53   -j ACCEPT\n"); // DNS
         fprintf(fp, "-A lan2wan -p tcp --dport 53   -j ACCEPT\n"); // DNS
         fprintf(fp, "-A lan2wan -p tcp --dport 119  -j ACCEPT\n"); // NTP
         fprintf(fp, "-A lan2wan -p tcp --dport 123  -j ACCEPT\n"); // NTP
         fprintf(fp, "-A lan2wan -p tcp --dport 25   -j ACCEPT\n"); // EMAIL
         fprintf(fp, "-A lan2wan -p tcp --dport 110  -j ACCEPT\n"); // EMAIL
         fprintf(fp, "-A lan2wan -p tcp --dport 143  -j ACCEPT\n"); // EMAIL
         fprintf(fp, "-A lan2wan -p tcp --dport 465  -j ACCEPT\n"); // EMAIL
         fprintf(fp, "-A lan2wan -p tcp --dport 587  -j ACCEPT\n"); // EMAIL
         fprintf(fp, "-A lan2wan -p tcp --dport 993  -j ACCEPT\n"); // EMAIL
         fprintf(fp, "-A lan2wan -p tcp --dport 995  -j ACCEPT\n"); // EMAIL
         fprintf(fp, "-A lan2wan -p gre              -j ACCEPT\n"); // GRE
         fprintf(fp, "-A lan2wan -p udp --dport 500  -j ACCEPT\n"); // VPN
         fprintf(fp, "-A lan2wan -p tcp --dport 1723 -j ACCEPT\n"); // VPN
         fprintf(fp, "-A lan2wan -p tcp --dport 3689 -j ACCEPT\n"); // ITUNES
         fprintf(fp, "-A lan2wan -m limit --limit 100/sec -j LOG --log-level %d --log-prefix \"UTOPIA: FW.IPv6 lan2wan drop\"\n",syslog_level);
         fprintf(fp, "-A lan2wan -j DROP\n");
         */

         //Changed GUI and IPv6 firewall now allows all lan2wan traffic
         fprintf(fp, "-A lan2wan -j ACCEPT\n");
      }
      else
      {
         // Everything from inside to Internet is allowed
         fprintf(fp, "-A lan2wan -j ACCEPT\n");
      }

      // established communication from WAN is accepted
      fprintf(fp, "-A FORWARD -i %s -m state --state ESTABLISHED,RELATED -j ACCEPT\n", wan6_ifname);
      fprintf(fp, "-A FORWARD -i %s -m state --state ESTABLISHED,RELATED -j ACCEPT\n", ecm_wan_ifname);
      fprintf(fp, "-A FORWARD -i %s -m state --state ESTABLISHED,RELATED -j ACCEPT\n", emta_wan_ifname);

      // ICMP varies and are rate limited anyway
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 1/0 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 2 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 3 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 4 -m limit --limit 100/sec -j ACCEPT\n");

      // ICMP messages for MIPv6 (assuming mobile node on the inside)
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 145 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 147 -m limit --limit 100/sec -j ACCEPT\n");

      // Traffic WAN to LAN
      fprintf(fp, ":wan2lan - [0:0]\n");
      fprintf(fp, "-A wan2lan -m state --state INVALID -j LOG_FORWARD_DROP\n");

      fprintf(fp, "-A FORWARD -i %s -o %s -j wan2lan\n", wan6_ifname, lan_ifname);
      fprintf(fp, "-A FORWARD -i %s -o %s -j wan2lan\n", ecm_wan_ifname, lan_ifname);
      fprintf(fp, "-A FORWARD -i %s -o %s -j wan2lan\n", emta_wan_ifname, lan_ifname);

      //in IPv6, the DMZ and port forwarding just overwrite the wan2lan rule.
      if(isDmzEnabled) {
          int rc;
          char ipv6host[64] = {'\0'};

          rc = syscfg_get(NULL, "dmz_dst_ip_addrv6", ipv6host, sizeof(ipv6host));
          if(rc == 0 && ipv6host[0] != '\0' && strcmp(ipv6host, "x") != 0) {
              fprintf(fp, "-A wan2lan -d %s -j ACCEPT\n", ipv6host);
          }
      }

      do_single_port_forwarding(NULL, NULL, AF_INET6, fp);
      do_port_range_forwarding(NULL, NULL, AF_INET6, fp);

      if (strncasecmp(firewall_levelv6, "High", strlen("High")) == 0)
      {
         fprintf(fp, "-A wan2lan -j RETURN\n");
      }
      else if (strncasecmp(firewall_levelv6, "Medium", strlen("Medium")) == 0)
      {
         fprintf(fp, "-A wan2lan -p tcp --dport 113 -j RETURN\n"); // IDENT
         fprintf(fp, "-A wan2lan -p icmpv6 --icmpv6-type 128 -j RETURN\n"); // ICMP PING

         fprintf(fp, "-A wan2lan -p tcp --dport 1214 -j RETURN\n"); // Kazaa
         fprintf(fp, "-A wan2lan -p udp --dport 1214 -j RETURN\n"); // Kazaa
         fprintf(fp, "-A wan2lan -p tcp --dport 6881:6999 -j RETURN\n"); // Bittorrent
         fprintf(fp, "-A wan2lan -p tcp --dport 6346 -j RETURN\n"); // Gnutella
         fprintf(fp, "-A wan2lan -p udp --dport 6346 -j RETURN\n"); // Gnutella
         fprintf(fp, "-A wan2lan -p tcp --dport 49152:65534 -j RETURN\n"); // Vuze
         fprintf(fp, "-A wan2lan -j ACCEPT\n");
      }
      else if (strncasecmp(firewall_levelv6, "Low", strlen("Low")) == 0)
      {
         fprintf(fp, "-A wan2lan -p tcp --dport 113 -j RETURN\n"); // IDENT
         fprintf(fp, "-A wan2lan -j ACCEPT\n");
      }
      else if (strncasecmp(firewall_levelv6, "Custom", strlen("Custom")) == 0)
      {
         if (isHttpBlockedV6)
         {
            fprintf(fp, "-A wan2lan -p tcp --dport 80 -j RETURN\n"); // HTTP
            fprintf(fp, "-A wan2lan -p tcp --dport 443 -j RETURN\n"); // HTTPS
         }
         if (isIdentBlockedV6)
         {
            fprintf(fp, "-A wan2lan -p tcp --dport 113 -j RETURN\n"); // IDENT
         }
         if (isPingBlockedV6)
         {
            fprintf(fp, "-A wan2lan -p icmpv6 --icmpv6-type 128 -j RETURN\n"); // ICMP PING
         }
         if (isP2pBlockedV6)
         {
            fprintf(fp, "-A wan2lan -p tcp --dport 1214 -j RETURN\n"); // Kazaa
            fprintf(fp, "-A wan2lan -p udp --dport 1214 -j RETURN\n"); // Kazaa
            fprintf(fp, "-A wan2lan -p tcp --dport 6881:6999 -j RETURN\n"); // Bittorrent
            fprintf(fp, "-A wan2lan -p tcp --dport 6346 -j RETURN\n"); // Gnutella
            fprintf(fp, "-A wan2lan -p udp --dport 6346 -j RETURN\n"); // Gnutella
            fprintf(fp, "-A wan2lan -p tcp --dport 49152:65534 -j RETURN\n"); // Vuze
         }

         if(isMulticastBlockedV6) {
            fprintf(fp, "-A wan2lan -p 2 -j RETURN\n"); // IGMP
         }

         fprintf(fp, "-A wan2lan -j ACCEPT\n");
      }
      else if (strncasecmp(firewall_levelv6, "None", strlen("None")) == 0)
      {
         fprintf(fp, "-A wan2lan -j ACCEPT\n");
      }

      // Accept TCP return traffic (stateless a la IOS 'established')
      // Useless as this kernel has ip6tables with statefull inspection (see ESTABLISHED above)
      //fprintf(fp, "-A FORWARD -i %s -o %s -p tcp -m tcp ! --tcp-flags FIN,SYN,RST,ACK SYN -j ACCEPT\n", wan6_ifname, lan_ifname);
#if 0
      // Accept UDP traffic blindly... to port greater than 1024 in a vain attempt to protect the inside
      fprintf(fp, "-A FORWARD -i %s -o %s -p udp -m udp --dport 1025:65535 -j ACCEPT\n", wan6_ifname, lan_ifname);

      // Accept NTP traffic
      fprintf(fp, "-A FORWARD -i %s -o %s -p udp -m udp --dport 123 -j ACCEPT\n", wan6_ifname, lan_ifname);
#endif

      // Accept blindly ESP/AH/SCTP
      fprintf(fp, "-A FORWARD -i %s -o %s -p esp -j ACCEPT\n", wan6_ifname, lan_ifname);
      fprintf(fp, "-A FORWARD -i %s -o %s -m ah -j ACCEPT\n", wan6_ifname, lan_ifname);
      fprintf(fp, "-A FORWARD -i %s -o %s -p 132 -j ACCEPT\n", wan6_ifname, lan_ifname);

      // Everything else is logged and declined
      //fprintf(fp, "-A FORWARD -m limit --limit 10/sec -j REJECT --reject-with icmp6-adm-prohibited\n");
      fprintf(fp, "-A FORWARD -j LOG_FORWARD_DROP\n");

      // Accept everything from localhost
      fprintf(fp, "-A OUTPUT -o lo -j ACCEPT\n");
      // And accept everything anyway as we trust ourself
      fprintf(fp, "-A OUTPUT -j ACCEPT\n");

#if defined(_COSA_BCM_MIPS_)
      fprintf(fp, "-A OUTPUT -m physdev --physdev-in %s -j ACCEPT\n", emta_wan_ifname);
#endif

   }

end_of_ipv6_firewall:

      FIREWALL_DEBUG("Exiting prepare_ipv6_firewall \n");
   return(0);
}

/*
 ********************************************************************
 *                                                                  *
 ********************************************************************
 */
static void printhelp(char *name) {
   printf ("Usage %s event_name --port sysevent_port --ip sysevent_ip --help\n", name);
   FIREWALL_DEBUG("Usage %s event_name --port sysevent_port --ip sysevent_ip --help\n" COMMA name);
}

/*
 * Procedure     : get_options
 * Purpose       : read commandline parameters and set configurable
 *                 parameters
 * Parameters    :
 *    argc       : The number of input parameters
 *    argv       : The input parameter strings
 * Return Value  :
 *   the index of the first not optional argument
 */
static int get_options(int argc, char **argv)
{
   int c;
   FIREWALL_DEBUG("Inside get_options\n");
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"port", 1, 0, 'p'},
         {"ip", 1, 0, 'i'},
         {"help", 0, 0, 'h'},
         {0, 0, 0, 0}
      };

      // optstring has a leading : to stop debug output
      // p takes an argument
      // i takes an argument
      // h takes no argument
      // w takes no argument
      c = getopt_long (argc, argv, ":p:i:h", long_options, &option_index);
      if (c == -1) {
         break;
      }

      switch (c) {
         case 'p':
            sysevent_port = (0x0000FFFF & (unsigned short) atoi(optarg));
            break;

         case 'i':
            snprintf(sysevent_ip, sizeof(sysevent_ip), optarg);
            break;

         case 'h':
         case '?':
            printhelp(argv[0]);
            return -1;

         default:
            printhelp(argv[0]);
            break;
      }
   }
   FIREWALL_DEBUG("Exiting get_options\n");
   return(optind);
}

/*
 * Service Template Methods
 */

/*
 * Name           :  get_service_event
 * Purpose        :  Utility method to convert string to event id
 * Parameters     :  None
 * Return Values  :
 *    0              : Success
 *    < 0            : Error code
 */
static service_ev_t get_service_event (const char *ev)
{
	FIREWALL_DEBUG("Inside service_ev_t get_service_event\n");
    int i;
    
    for (i = 0; i < SERVICE_EV_COUNT; i++) {
        if (0 == strcmp(ev, service_ev_map[i].ev_string)) {
            return service_ev_map[i].ev;
        }
    }
	FIREWALL_DEBUG("Exiting service_ev_t get_service_event\n");
    return SERVICE_EV_UNKNOWN;
}

/*
 * Name           :  service_init
 * Purpose        :  Initialize resources & retrieve configurations
 *                   required for firewall service
 * Parameters     :
 *    argc        :  Count of arguments (excludes event-name)
 *    argv        :  Array of arguments 
 * Return Values  :
 *    0              : Success
 *    < 0            : Error code
 */
static int service_init (int argc, char **argv)
{
   int rc = 0;
   char* pCfg = CCSP_MSG_BUS_CFG;
   int ret = 0;

   ulog_init();
	FIREWALL_DEBUG("Inside firewall service_init()\n");
   ulogf(ULOG_FIREWALL, UL_INFO, "%s service initializing", service_name);
      	FIREWALL_DEBUG("%s service initializing\n" COMMA service_name);

   isCronRestartNeeded     = 0;

   snprintf(sysevent_ip, sizeof(sysevent_ip), "127.0.0.1");
   sysevent_port = SE_SERVER_WELL_KNOWN_PORT;

   // parse commandline for options and readjust defaults if requested
   //int next_arg = get_options(argc, argv);
   get_options(argc, argv);

   int too_much = 10;
   // we use a O_EXCL file to ensure that only one firewall configuration instance exists
   // for simultaneous calls the others block
   // We also have a failsafe of a limited number of retries in case someone died and left lock on
   mode_t mode = S_IRUSR | S_IWUSR;
   do {
      lock_fd = open("/tmp/firewall_lock", O_CREAT|O_EXCL, mode);    
       if (0 > lock_fd) {
          if (EEXIST == errno) {
             sleep(1);
          } else {
             return(-4);
          }           
        }
      too_much--;
   } while (0 > lock_fd && 0 < too_much);   

   if (0 != (rc = syscfg_init())) {
      rc = -1;
      goto ret_err;
   }
   
   sysevent_fd =  sysevent_open(sysevent_ip, sysevent_port, SE_VERSION, sysevent_name, &sysevent_token);
   if (0 > sysevent_fd) {
      printf("Unable to register with sysevent daemon at %s %u.\n", sysevent_ip, sysevent_port);
      FIREWALL_DEBUG("Unable to register with sysevent daemon at %s %u.\n"COMMA sysevent_ip COMMA sysevent_port);
      rc = -2;
      goto ret_err;
   }
   
#ifdef DBUS_INIT_SYNC_MODE
    ret = CCSP_Message_Bus_Init_Synced(firewall_component_id, pCfg, &bus_handle, Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
#else
    ret = CCSP_Message_Bus_Init(firewall_component_id, pCfg, &bus_handle, Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
#endif
    if ( ret == -1 )
    {
        // Dbus connection error
        // Comment below
        fprintf(stderr, "%d, DBUS connection error\n", CCSP_MESSAGE_BUS_CANNOT_CONNECT);
        FIREWALL_DEBUG("%d, DBUS connection error\n" COMMA CCSP_MESSAGE_BUS_CANNOT_CONNECT);
        fprintf(stderr, "%d", CCSP_MESSAGE_BUS_CANNOT_CONNECT);
        bus_handle = NULL;
        //firewall need work before DBUS started, cannot exit
        //exit(CCSP_MESSAGE_BUS_CANNOT_CONNECT);
    }

   ulog_debugf(ULOG_FIREWALL, UL_INFO, "firewall opening sysevent_fd %d, token %d", sysevent_fd, sysevent_token);
   FIREWALL_DEBUG("firewall opening sysevent_fd %d, token %d\n"COMMA sysevent_fd COMMA sysevent_token);       
   time_t now;
   time(&now);
   if (NULL == localtime_r((&now), (&local_now))) {
      rc = -3;
      goto ret_err;
   }

   prepare_globals_from_configuration();
 
ret_err:
FIREWALL_DEBUG("Exiting firewall service_init()\n");
   return rc;
}

/*
 * Name           :  service_close
 * Purpose        :  Close resources initialized for firewall service
 * Parameters     :
 *    None        :
 * Return Values  :
 *    0              : Success
 *    < 0            : Error code
 */
static int service_close ()
{
FIREWALL_DEBUG("Inside firewall service_close()\n");
   if (0 <= lock_fd) {
       close(lock_fd);
       unlink("/tmp/firewall_lock");
       ulog_debug(ULOG_FIREWALL, UL_INFO, "firewall closing firewall_lock");
       FIREWALL_DEBUG("firewall closing firewall_lock\n");
   }
   if (0 <= sysevent_fd)  {
       ulog_debugf(ULOG_FIREWALL, UL_INFO, "firewall closing sysevent_fd %d, token %d",
                   sysevent_fd, sysevent_token);
       FIREWALL_DEBUG("firewall closing sysevent_fd %d, token %d\n"COMMA
                   sysevent_fd COMMA sysevent_token);
       sysevent_close(sysevent_fd, sysevent_token);
   }
   if (bus_handle != NULL) {
       ulog_debug(ULOG_FIREWALL, UL_INFO, "firewall closing dbus connection");
       FIREWALL_DEBUG("firewall closing dbus connection\n");
        CCSP_Message_Bus_Exit(bus_handle);
   }
   ulog(ULOG_FIREWALL, UL_INFO, "firewall operation completed");
FIREWALL_DEBUG("exiting firewall service_close()\n");
   return 0;
}

/*
 * Name           :  service_start
 * Purpose        :  Start firewall service (including nat & qos)
 * Parameters     :
 *    None        :
 * Return Values  :
 *    0              : Success
 *    < 0            : Error code
 */
static int service_start ()
{
   char *filename1 = "/tmp/.ipt";
   char *filename2 = "/tmp/.ipt_v6";

   //clear content in firewall cron file.
   char *cron_file = crontab_dir"/"crontab_filename;
   FILE *cron_fp = NULL; // the crontab file we use to set wakeups for timed firewall events
   pthread_mutex_lock(&firewall_check);
   FIREWALL_DEBUG("Inside firewall service_start()\n");
   cron_fp = fopen(cron_file, "w");
   if(cron_fp) {
       fclose(cron_fp);
   } else {
       fprintf(stderr,"%s: ### create crontab_file error with %d!!!\n",__FUNCTION__, errno);
       FIREWALL_DEBUG("%s: ### create crontab_file error with %d!!!\n" COMMA __FUNCTION__ COMMA errno);
   } 

   sysevent_set(sysevent_fd, sysevent_token, "firewall-status", "starting", 0);
   ulogf(ULOG_FIREWALL, UL_INFO, "starting %s service", service_name);
      	FIREWALL_DEBUG("starting %s service\n" COMMA service_name);
   /*  ipv4 */
   prepare_ipv4_firewall(filename1);
   system("iptables-restore -c  < /tmp/.ipt");

   //if (!isFirewallEnabled) {
   //   unlink(filename1);
   //}

   /* ipv6 */
   prepare_ipv6_firewall(filename2);
   system("ip6tables-restore < /tmp/.ipt_v6");
   /* start the other process as needed */
#ifdef CONFIG_BUILD_TRIGGER
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   if (isTriggerMonitorRestartNeeded) {
      sysevent_set(sysevent_fd, sysevent_token, "firewall_trigger_monitor-start", NULL, 0);
   }
#endif
#endif
   if (isLanHostTracking) {
      sysevent_set(sysevent_fd, sysevent_token, "firewall_newhost_monitor-start", NULL, 0);
   }

   if (isCronRestartNeeded) {
      sysevent_set(sysevent_fd, sysevent_token, "crond-restart", "1", 0);
   }
   else {
      unlink(cron_file);
   }

   if(ppFlushNeeded == 1) {
       system("echo flush_all_sessions > /proc/net/ti_pp");
       sysevent_set(sysevent_fd, sysevent_token, "pp_flush", "0", 0);
   }

   sysevent_set(sysevent_fd, sysevent_token, "firewall-status", "started", 0);
   ulogf(ULOG_FIREWALL, UL_INFO, "started %s service", service_name);
   FIREWALL_DEBUG("started %s service\n" COMMA service_name);
   pthread_mutex_unlock(&firewall_check);
   FIREWALL_DEBUG("Exiting firewall service_start()\n");
 	return 0;
}

/*
 * Name           :  service_stop
 * Purpose        :  Stop firewall service (including nat & qos)
 * Parameters     :  None
 * Return Values  :
 *    0              : Success
 *    < 0            : Error code
 */
static int service_stop ()
{
   char *filename1 = "/tmp/.ipt";
	pthread_mutex_lock(&firewall_check);
	FIREWALL_DEBUG("Inside firewall service_stop()\n");
   sysevent_set(sysevent_fd, sysevent_token, "firewall-status", "stopping", 0);
   ulogf(ULOG_FIREWALL, UL_INFO, "stopping %s service", service_name);
	FIREWALL_DEBUG("stopping %s service\n" COMMA service_name);
   FILE *fp = fopen(filename1, "w"); 
   if (NULL == fp) {
   pthread_mutex_unlock(&firewall_check);
      return(-2);
   }
   prepare_stopped_ipv4_firewall(fp);
   fclose(fp);

   system("iptables -t filter -F");
   system("iptables -t nat -F");
   system("iptables -t mangle -F");
   system("iptables -t raw -F");

   system("iptables-restore -c  < /tmp/.ipt");

   sysevent_set(sysevent_fd, sysevent_token, "firewall-status", "stopped", 0);
   ulogf(ULOG_FIREWALL, UL_INFO, "stopped %s service", service_name);
   	FIREWALL_DEBUG("stopped %s service\n" COMMA service_name);
 pthread_mutex_unlock(&firewall_check);
 	FIREWALL_DEBUG("Exiting firewall service_stop()\n");
    return 0;
}

/*
 * Name           :  service_restart
 * Purpose        :  Restart the firewall service
 * Parameters     :  None
 * Return Values  :
 *    0              : Success
 *    < 0            : Error code
 */
static int service_restart ()
{
// dont tear down firewall and put it into completly open and unnatted state
// just recalculate the rules - service start does that
//    (void) service_stop();
	FIREWALL_DEBUG("Inside Firewall service_restart () \n");
    return service_start();
}

/*
 * Purpose        : Instantiate ipv4 and ipv6 firewall 
 * Parameters     :
 *    argc          :
 *    argv          :
 * Return Values  :
 *    0              : Success
 *   -1              : Problem with syscfg
 *   -2              : Problem with sysevent
 *   -3              : Couldn't get local time. System error
 *   -4              : Could not open mutex file
 */
int main(int argc, char **argv)
{
   int rc = 0;
   char syslog_status[32];

   ulogf(ULOG_FIREWALL, UL_INFO, "%s called with %s", service_name, (argc > 1) ? argv[1] : "no arg");

   // defualt command if first argument is missing
   service_ev_t event = SERVICE_EV_RESTART;
	   if(firewallfp == NULL) {
		firewallfp = fopen ("/rdklogs/logs/FirewallDebug.txt", "a+");
	} 
        FIREWALL_DEBUG("ENTERED FIREWALL, argc = %d \n" COMMA argc);

   if (argc > 1) {
       if (SERVICE_EV_UNKNOWN == (event = get_service_event(argv[1]))) {
           event = SERVICE_EV_RESTART;
       }
       argc--;
       argv++;
   }

   if (argc > 1) {
      if (!strncmp("flush", argv[1], strlen("flush"))) {
         flush = 1;
         argc--;
         argv++;
      }
   }
	pthread_mutex_init(&firewall_check, NULL);
   rc = service_init(argc, argv);
   if (rc < 0) {
       service_close();
       if(firewallfp)
       fclose(firewallfp);
       return rc;
   }

   switch (event) {
   case SERVICE_EV_START:
       service_start();
       break;
   case SERVICE_EV_STOP:
       service_stop();
       break;
   case SERVICE_EV_RESTART:
       service_restart();
       break;
   /*
    * Handle custom events below here
    */
   case SERVICE_EV_SYSLOG_STATUS:
       sysevent_get(sysevent_fd, sysevent_token, "syslog-status", syslog_status, sizeof(syslog_status));
       ulogf(ULOG_FIREWALL, UL_INFO, "%s handling syslog-status=%s", service_name, syslog_status);
       FIREWALL_DEBUG("%s handling syslog-status=%s\n" COMMA service_name COMMA syslog_status);
       if (0 == strcmp(syslog_status, "started")) {
           service_restart();
       }
       break;
   default:
       ulogf(ULOG_FIREWALL, UL_INFO, "%s received unhandled event", service_name);
        FIREWALL_DEBUG("%s received unhandled event \n" COMMA service_name);
       break;
   }

   service_close();

   if (flush)
      system("conntrack_flush");
        if(firewallfp)
       fclose(firewallfp);
   return(rc);
}
