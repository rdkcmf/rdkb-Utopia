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

#include "autoconf.h"
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utctx_internal.h"
#include "safec_lib_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Utopia transaction node */
typedef struct _UtopiaTransact_Node
{
    UtopiaValue ixUtopia;
    char* pszNamespace;
    char* pszKey;
    char* pszValue;

    struct _UtopiaTransact_Node* pNext;
} UtopiaTransact_Node;

typedef enum _Utopia_Index_For
{
    Utopia_For__NONE__ = 0,
    Utopia_Index_For_Both,
    Utopia_Index_For_NS,
    Utopia_Index_For_Key,
    Utopia_Name_For_NS,
    Utopia_Name_For_Both,
    Utopia_Name_For_Key
} Utopia_Index_For;

typedef enum _Utopia_Static
{
    Utopia_Static__NONE__ = 0,
    Utopia_Static_DeviceType,
    Utopia_Static_ModelDescription,
    Utopia_Static_ModelName,
    Utopia_Static_ModelRevision,
    Utopia_Static_PresentationURL,
    Utopia_Static_VendorName
} Utopia_Static;

typedef enum _Utopia_Type
{
    Utopia_Type__UNKNOWN__ = 0,
    Utopia_Type_Static,
    Utopia_Type_Config,
    Utopia_Type_IndexedConfig,
    Utopia_Type_Indexed2Config,
    Utopia_Type_NamedConfig,
    Utopia_Type_Named2Config,
    Utopia_Type_Event
} Utopia_Type;

/* MACRO for determining if event 'z' is in bitmask 'y' */
#define Utopia_EventSet(y,z) ((y & z) == z)

/* RDKB-7126, CID-33554, out of bound access.
** Macro for defining maximum size of "g_Utopia_Events" 
** if a new event added require to increase the define.
*/
#define MAX_UTOPIA_EVENTS  22

static struct
{
    char* pszEventKey;
    char* pszWaitKey;   /* Key of the sysevent to wait on */
    char* pszWaitValue; /* Value of the sysevent to wait for */
    int   iWaitTimeout; /* Number of seconds to wait for change */
}
    g_Utopia_Events[MAX_UTOPIA_EVENTS] =
{
    /* Utopia_Event__NONE__ */             { 0,                      0, 0, 0 },
    /* Utopia_Event_Cron_Restart */        { "crond-restart",        0, 0, 0 },
    /* Utopia_Event_DHCPClient_Restart */  { "dhcp_client-restart",  0, 0, 0 },
                                                                     /* Setting the pszWaitValue to NULL will cause
                                                                      * execution to continue as soon as the value
                                                                      * changes to anything
                                                                      */
    /* Utopia_Event_DHCPServer_Restart */  { "dhcp_server-restart",  "dhcp_server-status", 0, 5 },
    /* Utopia_Event_DNS_Restart */         { "dns-restart",          0, 0, 0 },
    /* Utopia_Event_Firewall_Restart */    { "firewall-restart",     0, 0, 0 },
    /* Utopia_Event_HTTPServer_Restart */  { "httpd-restart",        0, 0, 0 },
    /* Utopia_Event_LAN_Restart */         { "lan-restart",          "lan-restarting", "1", 5 },
    /* Utopia_Event_MACFilter_Restart */   { "mac_filter_changed",   0, 0, 0 },
    /* Utopia_Event_NTPClient_Restart */   { "ntpclient-restart",    0, 0, 0 },
    /* Utopia_Event_Reboot */              { "system-restart",       0, 0, 0 },
    /* Utopia_Event_WAN_Restart */         { "wan-restart",          "wan-restarting", "0", 30 },
#if !defined(_PLATFORM_IPQ_)
    /* Utopia_Event_WLAN_Restart */        { "wlan-restart",         "wlan-restarting", "1", 5 },
#else
    /* Utopia_Event_WLAN_Restart */        { "wlan-restart",         "wlan-restarting", "1", 40 },
#endif
    /* Utopia_Event_DDNS_Update */         { "ddns-start",           0, 0, 0 },
    /* Utopia_Event_StaticRoute_Restart */ { "staticroute-restart",  0, 0, 0 },
    /* Utopia_Event_SmbServer_Restart */   { "samba_server_restart", 0, 0, 0 },
    /* Utopia_Event_IGD_Restart */         { "igd-restart",          0, 0, 0 },
    /* Utopia_Event_RIP_Restart */         { "ripd-restart",         0, 0, 0 },
    /* Utopia_Event_Syslog_Restart */      { "syslog-restart",          0, 0, 0 },
    /* Utopia_Event_QoS_Restart */         { "qos-restart",             0, 0, 0 },
    /* Utopia_Event_MoCA_Restart */        { "desired_moca_link_state", 0, 0, 0 },
    /* Utopia_Event__LAST__ */             { 0,                      0, 0, 0 }
};

static char* g_Utopia_Statics[] =
{
    /* Utopia_Static__NONE__ */ 0,
    /* Utopia_Static_DeviceType */ "GatewayWithWiFi",
    /* Utopia_Static_ModelDescription */ "Linksys E3000",
#ifdef TEST_DEVICE
    /* Utopia_Static_ModelName */ "E3000-LEGO",
#else
    /* Utopia_Static_ModelName */ "E3000",
#endif
    /* Utopia_Static_ModelRevision */ "1.0",
    /* Utopia_Static_PresentationURL */ "/",
    /* Utopia_Static_VendorName */ "Cisco"
};

/* Utopia key, trigger, type, static index table */
static struct
{
    unsigned char ixType;      /* Utopia_Type */
    unsigned int ixEvent;      /* Utopia_Event */
    unsigned char ixStatic;    /* Utopia_Static */
    unsigned char ixFor;       /* For Indexed, Indexed2, Named, Named2 calls, indicates whether iIndex/pszName is for the key, the namespace, or neither */
    unsigned char fSetAllowed; /* True if UtopiaValue is allowed to be set */
    char* pszKey;              /* The syscfg/sysevent key string */
    unsigned short ixNamespace; /* UtopiaValue - Index for the UtopiaValue to use to look up namespace */
}
    g_Utopia[] =
{
    { Utopia_Type__UNKNOWN__, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue__UNKNOWN__ */

    /* UtopiaValues used by the unittest framework */
#ifdef UTCTX_UNITTEST
    { Utopia_Type__UNKNOWN__,     Utopia_Event__NONE__,            Utopia_Static__NONE__,    Utopia_For__NONE__,    0, 0,                     UtopiaValue__UNKNOWN__ },       /* UtopiaValue__TEST_BEGIN__ */
    { Utopia_Type_Static,         Utopia_Event__NONE__,            Utopia_Static_DeviceType, Utopia_For__NONE__,    0, 0,                     UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Static_TestOne */
    { Utopia_Type_Config,         Utopia_Event_Cron_Restart,       Utopia_Static__NONE__,    Utopia_For__NONE__,    1, "cfg_test_two",        UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Config_TestTwo */
    { Utopia_Type_IndexedConfig,  Utopia_Event_DHCPClient_Restart, Utopia_Static__NONE__,    Utopia_Index_For_Key,  1, "idx_test_one_%d",     UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Indexed_TestOne */
    { Utopia_Type_IndexedConfig,  Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__,    Utopia_Index_For_NS,   1, "idx_test_two",        UtopiaValue_Indexed_TestOne },  /* UtopiaValue_Indexed_TestTwo */
    { Utopia_Type_Indexed2Config, Utopia_Event_DNS_Restart,        Utopia_Static__NONE__,    Utopia_Index_For_Key,  1, "idx2_test_one_%d_%d", UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Indexed2_TestOne */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__,    Utopia_Index_For_Both, 1, "idx2_test_two_%d",    UtopiaValue_Indexed_TestOne },  /* UtopiaValue_Indexed2_TestTwo */
    { Utopia_Type_Indexed2Config, Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__,    Utopia_Index_For_NS,   1, "idx2_test_three",     UtopiaValue_Indexed2_TestOne }, /* UtopiaValue_Indexed2_TestThree */
    { Utopia_Type_NamedConfig,    Utopia_Event_LAN_Restart,        Utopia_Static__NONE__,    Utopia_Name_For_Key,   1, "nmd_test_one_%s",     UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Named_TestOne */
    { Utopia_Type_NamedConfig,    Utopia_Event_MACFilter_Restart,  Utopia_Static__NONE__,    Utopia_Name_For_NS,    1, "nmd_test_two",        UtopiaValue_Named_TestOne },    /* UtopiaValue_Named_TestTwo */
    { Utopia_Type_Named2Config,   Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__,    Utopia_Name_For_Key,   1, "nmd2_test_one_%s_%s", UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Named2_TestOne */
    { Utopia_Type_Named2Config,   Utopia_Event_WAN_Restart,        Utopia_Static__NONE__,    Utopia_Name_For_Both,  1, "nmd2_test_two_%s",    UtopiaValue_Named_TestOne },    /* UtopiaValue_Named2_TestTwo */
    { Utopia_Type_Named2Config,   Utopia_Event_WLAN_Restart,       Utopia_Static__NONE__,    Utopia_Name_For_NS,    1, "nmd2_test_three",     UtopiaValue_Named2_TestOne },   /* UtopiaValue_Named2_TestThree */
    { Utopia_Type_Event,          Utopia_Event_DDNS_Update,        Utopia_Static__NONE__,    Utopia_For__NONE__,    0, "evt_test_one",        UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Event_TestOne */
    { Utopia_Type_Event,          Utopia_Event__NONE__,            Utopia_Static__NONE__,    Utopia_For__NONE__,    1, "evt_test_two",        UtopiaValue__UNKNOWN__ },       /* UtopiaValue_Event_TestOne */
    { Utopia_Type__UNKNOWN__,     Utopia_Event__NONE__,            Utopia_Static__NONE__,    Utopia_For__NONE__,    0, 0,                     UtopiaValue__UNKNOWN__ },       /* UtopiaValue__TEST_LAST__ */
#endif

/* Utopia_Type_Static */

    { Utopia_Type_Static, Utopia_Event__NONE__, Utopia_Static_DeviceType,       Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DeviceType */
    { Utopia_Type_Static, Utopia_Event__NONE__, Utopia_Static_ModelDescription, Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ModelDescription */
    { Utopia_Type_Static, Utopia_Event__NONE__, Utopia_Static_ModelName,        Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ModelName */
    { Utopia_Type_Static, Utopia_Event__NONE__, Utopia_Static_ModelRevision,    Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ModelRevision */
    { Utopia_Type_Static, Utopia_Event__NONE__, Utopia_Static_PresentationURL,  Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue_PresentationURL */
    { Utopia_Type_Static, Utopia_Event__NONE__, Utopia_Static_VendorName,       Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue_VendorName */

/* Utopia_Type_Config */

    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "auto_dst",                       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_AutoDST */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "def_hwaddr",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DefHwAddr */
    //{ Utopia_Type_Config, Utopia_Event_DNS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "hostname",                       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_HostName */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "hostname",                             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_HostName */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "locale",                         UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Locale */
    { Utopia_Type_Config, Utopia_Event_Syslog_Restart,     Utopia_Static__NONE__, Utopia_For__NONE__, 1, "log_level",                      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LogLevel */
    { Utopia_Type_Config, Utopia_Event_Syslog_Restart,     Utopia_Static__NONE__, Utopia_For__NONE__, 1, "log_remote",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LogRemote */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "nat_enabled",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NATEnabled */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "samba_server_enabled",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_SambaServerEnabled */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "TZ",                             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_TZ */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_enable",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Enable */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_service",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Service */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_update_days",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_UpdateDays */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_last_update",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_LastUpdate */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_hostname",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Hostname */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_username",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Username */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_password",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Password */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_mx",                        UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Mx */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_mx_backup",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_MxBackup */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_wildcard",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Wildcard */
    { Utopia_Type_Config, Utopia_Event_DDNS_Update,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ddns_server",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DDNS_Server */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_server_enabled",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_ServerEnabled */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_start",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_Start */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_end",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_End */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_num",                       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_Num */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_lease_time",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_LeaseTime */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_nameserver_enabled",        UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_Nameserver_Enabled */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_nameserver_1",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_Nameserver1 */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_nameserver_2",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_Nameserver2 */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_nameserver_3",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_Nameserver3 */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_wins_server",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_WinsServer */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_num_static_hosts",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_NumStaticHosts */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dmz_enabled",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DMZ_Enabled */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dmz_src_addr_range",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DMZ_SrcAddrRange */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dmz_dst_ip_addr",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DMZ_DstIpAddr */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dmz_dst_ip_addrv6",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DMZ_DstIpAddrV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dmz_dst_mac_addr",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DMZ_DstMacAddr */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "admin_pw",                       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_User_AdminPassword */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "root_pw",                        UtopiaValue__UNKNOWN__ }, /* UtopiaValue_User_RootPassword */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "http_admin_password",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_HTTP_AdminPassword */
    { Utopia_Type_Config, Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "http_admin_port",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_HTTP_AdminPort */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "http_admin_user",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_HTTP_AdminUser */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "http_admin_is_default",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_HTTP_AdminIsDefault */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_http_enable",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_HTTPAccess */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_https_enable",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_HTTPSAccess */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wifi_access",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WIFIAccess */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_access",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANAccess */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_httpaccess",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANHTTPAccess */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_httpsaccess",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANHTTPSAccess */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_httpport",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANHTTPPort */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_HTTPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_httpsport",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANHTTPSPort */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_fwupgrade",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANFWUpgrade */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_srcany",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANSrcAny */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_srcstart_ip",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANSrcStartIP */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_srcstart_ipv6",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANSrcStartIPV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_srcend_ip",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANSrcEndIP */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_srcend_ipv6",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANSrcEndIPV6 */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_wan_iprang_count", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANIPrangeCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "mgmt_wan_iprang_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_WANIPrange*/
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_NS, 1, "srcend_ip", UtopiaValue_Mgmt_WANIPrange}, /* UtopiaValue_Mgmt_WANIPrange_SrcEndIP */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_NS, 1, "srcstart_ip", UtopiaValue_Mgmt_WANIPrange}, /* UtopiaValue_Mgmt_WANIPRange_SrcStartIP */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_NS, 1, "desp", UtopiaValue_Mgmt_WANIPrange}, /* UtopiaValue_Mgmt_WANIPrange_Desp */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_NS, 1, "ins_num", UtopiaValue_Mgmt_WANIPrange}, /* UtopiaValue_Mgmt_WANIPrange_InsNum */

    { Utopia_Type_Config, Utopia_Event_IGD_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "upnp_igd_enabled",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_IGDEnabled */
    { Utopia_Type_Config, Utopia_Event_IGD_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "igd_allow_userconfig",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_IGDUserConfig */
    { Utopia_Type_Config, Utopia_Event_IGD_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "igd_allow_wandisable",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_IGDWANDisable */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_mso_access",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_MsoAccess */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "mgmt_cusadmin_access",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Mgmt_CusadminAccess */
    { Utopia_Type_Config, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "lan_domain",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_Domain */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 0, "lan_ethernet_physical_ifnames",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_EthernetPhysicalIfNames */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 0, "lan_ethernet_virtual_ifnums",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_EthernetVirtualIfNum */
    { Utopia_Type_Config, Utopia_Event_LAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "lan_ipaddr",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_IPAddr */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 0, "lan_ifname",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_IfName */
    { Utopia_Type_Config, Utopia_Event_LAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "lan_netmask",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_Netmask */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_server1",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Server1 */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_server2",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Server2 */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_server3",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Server3 */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_server4",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Server4 */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_server5",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Server5 */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_daylightenable",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Daylight */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_daylightoffset",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_DayOffset */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_RIP_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "rip_enabled",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_RIP_Enabled */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_RIP_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "rip_no_split_horizon",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_RIP_NoSplitHorizon */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_RIP_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "rip_interface_lan",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_RIP_InterfaceLAN */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_RIP_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "rip_interface_wan",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_RIP_InterfaceWAN */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_RIP_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "rip_md5_passwd",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_RIP_MD5Passwd */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart |
                          Utopia_Event_RIP_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "rip_text_passwd",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_RIP_TextPasswd */
    { Utopia_Type_Config, Utopia_Event_StaticRoute_Restart,Utopia_Static__NONE__, Utopia_For__NONE__, 1, "StaticRouteCount",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_StaticRoute_Count */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_proto",                      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_Proto */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_ipaddr",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_IPAddr */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_netmask",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_Netmask */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_default_gateway",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_DefaultGateway */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "staticdns_enable",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_EnableStaticNameServer*/
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "nameserver1",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_NameServer1 */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "nameserver2",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_NameServer2 */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "nameserver3",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_NameServer3 */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_proto_username",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_ProtoUsername */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_proto_password",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_ProtoPassword */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ppp_conn_method",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_PPPConnMethod */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ppp_keepalive_interval",         UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_PPPKeepAliveInterval */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ppp_idle_time",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_PPPIdleTime */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_proto_remote_name",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_ProtoRemoteName */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_domain",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_ProtoAuthDomain */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "pppoe_service_name",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_PPPoEServiceName */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "pppoe_access_concentrator_name", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_PPPoEAccessConcentratorName */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_proto_server_address",       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_ProtoServerAddress */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "pptp_address_static",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_PPTPAddressStatic */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "l2tp_address_static",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_L2TPAddressStatic */
    { Utopia_Type_Config, Utopia_Event_WAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "telstra_server",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_TelstraServer */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_mtu",                        UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_MTU */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 0, "wan_physical_ifname",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_PhysicalIfName */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 0, "wan_virtual_ifnum",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_VirtualIfNum */
    { Utopia_Type_Config, Utopia_Event_LAN_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "lan_dhcp_client",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_DHCPClient */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_mode",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_Mode */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_ipaddr",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_IPAddress */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_netmask",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_Netmask */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_default_gateway",         UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_DefaultGateway */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_domain",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_Domain */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_nameserver1",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_NameServer1 */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_nameserver2",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_NameServer2 */
    { Utopia_Type_Config, Utopia_Event_Reboot,             Utopia_Static__NONE__, Utopia_For__NONE__, 1, "bridge_nameserver3",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Bridge_NameServer3 */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_config_mode",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_ConfigMode */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_mac_filter",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_MACFilter */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_access_restriction",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AccessRestriction */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_client_list",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_ClientList */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 0, "lan_wl_physical_ifnames",        UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_PhysicalIfNames */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_wmm_support",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_WMMSupport */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_no_acknowledgement",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_NoAcknowledgement */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wifi_bridge_mode",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_BridgeMode */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wifi_bridge_bssid",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_BridgeSSID */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "qos_enable",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_Enable */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSDefinedPolicyCount",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_DefPolicyCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSUserDefinedPolicyCount",      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_UserDefPolicyCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSPolicyCount",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_PolicyCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSMacAddrCount",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_MacAddrCount */
    { Utopia_Type_Config, Utopia_Event_QoS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSEthernetPortCount",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_EthernetPortCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSVoiceDeviceCount",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_VoiceDeviceCount */
    { Utopia_Type_Config, Utopia_Event_QoS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSEthernetPort_1",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_EthernetPort1 */
    { Utopia_Type_Config, Utopia_Event_QoS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSEthernetPort_2",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_EthernetPort2 */
    { Utopia_Type_Config, Utopia_Event_QoS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSEthernetPort_3",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_EthernetPort3 */
    { Utopia_Type_Config, Utopia_Event_QoS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "QoSEthernetPort_4",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_EthernetPort4 */
    { Utopia_Type_Config, Utopia_Event_QoS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_download_speed",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_WanDownloadSpeed */
    { Utopia_Type_Config, Utopia_Event_QoS_Restart,        Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_upload_speed",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_QoS_WanUploadSpeed */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_enabled",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_Enabled */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_ping",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockPing */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_pingv6",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockPingV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_multicast",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockMulticast */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_multicastv6",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockMulticastV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_nat_redirection",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockNatRedir */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_ident",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockIdent */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_identv6",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockIdentV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_webproxy",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockWebProxy */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_java",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockJava */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_activex",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockActiveX */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_cookies",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockCookies */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_http",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockHttp */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_httpv6",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockHttpV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_p2p",                      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockP2p */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "block_p2pv6",                      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_BlockP2pV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_level", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_Level */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_levelv6", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_LevelV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_true_static_ip_enable", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_TrueStaticIpEnable */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_true_static_ip_enablev6", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_TrueStaticIpEnableV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_smart_enable", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_SmartEnable */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_smart_enablev6", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_SmartEnableV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_disable_wan_ping", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_WanPingEnable */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "firewall_disable_wan_pingv6", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_WanPingEnableV6 */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "SinglePortForwardCount",         UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_SPFCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "PortRangeForwardCount",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_PFRCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "PortRangeTriggerCount",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_PRTCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "InternetAccessPolicyCount",      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_IAPCount */
    { Utopia_Type_Config, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "W2LWellKnownFirewallRuleCount",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Firewall_W2LWKRuleCount */
    { Utopia_Type_Config, Utopia_Event_SmbServer_Restart,  Utopia_Static__NONE__, Utopia_For__NONE__, 1, "SharedFolderCount",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NAS_SFCount */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_device_name",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_DeviceName */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_device_pin",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_DevicePin */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_manufacturer",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_Manufacturer */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_method",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_Method */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_mode",                       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_Mode */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_model_name",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_ModelName */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_model_number",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_ModelNumber */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_station_pin",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_StationPin */
    { Utopia_Type_Config, Utopia_Event__NONE__,            Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wps_uuid",                       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WPS_UUID */

/* Utopia_Type_IndexedConfig */

    { Utopia_Type_IndexedConfig, Utopia_Event_DHCPServer_Restart, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dhcp_static_host_%d",              UtopiaValue__UNKNOWN__ },           /* UtopiaValue_DHCP_StaticHost */
    { Utopia_Type_IndexedConfig, Utopia_Event_StaticRoute_Restart,  Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "StaticRoute_%d",                 UtopiaValue__UNKNOWN__ },           /* UtopiaValue_StaticRoute */
    { Utopia_Type_IndexedConfig, Utopia_Event_StaticRoute_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                           UtopiaValue_StaticRoute },          /* UtopiaValue_SR_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_StaticRoute_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "dest",                           UtopiaValue_StaticRoute },          /* UtopiaValue_SR_Dest */
    { Utopia_Type_IndexedConfig, Utopia_Event_StaticRoute_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "netmask",                        UtopiaValue_StaticRoute },          /* UtopiaValue_SR_Netmask */
    { Utopia_Type_IndexedConfig, Utopia_Event_StaticRoute_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "gw",                             UtopiaValue_StaticRoute },          /* UtopiaValue_SR_Gateway */
    { Utopia_Type_IndexedConfig, Utopia_Event_StaticRoute_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "interface",                      UtopiaValue_StaticRoute },          /* UtopiaValue_SR_Interface */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "firewall_rule%d",                  UtopiaValue__UNKNOWN__ },           /* UtopiaValue_FirewallRule */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "W2LWellKnownFirewallRule_%d",      UtopiaValue__UNKNOWN__ },           /* UtopiaValue_FW_W2LWellKnown */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_FW_W2LWellKnown },      /* UtopiaValue_FW_W2LWK_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "result",                           UtopiaValue_FW_W2LWellKnown },      /* UtopiaValue_FW_W2LWK_Result */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "InternetAccessPolicy_%d",          UtopiaValue__UNKNOWN__ },           /* UtopiaValue_InternetAccessPolicy */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "enabled",                          UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_Enabled */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "enforcement_schedule",             UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_EnforcementSchedule */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "access",                           UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_Access */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "tr_inst_num",                           UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_TR_INST_NUM */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "BlockUrlCount",                    UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockUrlCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "BlockKeywordCount",                UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockKeywordCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "BlockApplicationCount",            UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockApplicationCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "BlockWellknownApplicationCount",   UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockWellknownApplicationCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "BlockApplicationRuleCount",        UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockApplicationRuleCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "BlockPing",                        UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockPing */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "local_host_list",                  UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_LocalHostList */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "InternetAccessPolicyIPHostCount",  UtopiaValue_IAP_LocalHostList },    /* UtopiaValue_IAP_IPHostCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "InternetAccessPolicyIPRangeCount", UtopiaValue_IAP_LocalHostList },    /* UtopiaValue_IAP_IPRangeCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "InternetAccessPolicyMacCount",     UtopiaValue_IAP_LocalHostList },    /* UtopiaValue_IAP_MACCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "SinglePortForward_%d",             UtopiaValue__UNKNOWN__ },           /* UtopiaValue_SinglePortForward */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "enabled",                          UtopiaValue_SinglePortForward },    /* UtopiaValue_SPF_Enabled */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_SinglePortForward },    /* UtopiaValue_SPF_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "protocol",                         UtopiaValue_SinglePortForward },    /* UtopiaValue_SPF_Protocol */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "external_port",                    UtopiaValue_SinglePortForward },    /* UtopiaValue_SPF_ExternalPort */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "internal_port",                    UtopiaValue_SinglePortForward },    /* UtopiaValue_SPF_InternalPort */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "to_ip",                            UtopiaValue_SinglePortForward },    /* UtopiaValue_SPF_ToIp */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "to_ipv6",                          UtopiaValue_SinglePortForward },    /* UtopiaValue_SPF_ToIpV6 */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "PortRangeForward_%d",              UtopiaValue__UNKNOWN__ },           /* UtopiaValue_PortRangeForward */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "enabled",                          UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_Enabled */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "protocol",                         UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_Protocol */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "external_port_range",              UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_ExternalPortRange */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "internal_port",                    UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_InternalPort */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "internal_port_range_size",         UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_InternalPortRangeSize */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "to_ip",                            UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_ToIp */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "public_ip",                        UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_PublicIp */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "to_ipv6",                          UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_ToIpV6 */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "QoSPolicy_%d",                     UtopiaValue__UNKNOWN__ },           /* UtopiaValue_QoSPolicy */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "QoSDefinedPolicy_%d",              UtopiaValue__UNKNOWN__ },           /* UtopiaValue_QoSDefinedPolicy */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_QoSDefinedPolicy },     /* UtopiaValue_QDP_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "class",                            UtopiaValue_QoSDefinedPolicy },     /* UtopiaValue_QDP_Class */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "QoSUserDefinedPolicy_%d",          UtopiaValue__UNKNOWN__ },           /* UtopiaValue_QoSUserDefinedPolicy */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_QoSUserDefinedPolicy }, /* UtopiaValue_QUP_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "portrange_count",                  UtopiaValue_QoSUserDefinedPolicy }, /* UtopiaValue_QUP_PortRangeCount */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "type",                             UtopiaValue_QoSUserDefinedPolicy }, /* UtopiaValue_QUP_Type */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "QoSMacAddr_%d",                    UtopiaValue__UNKNOWN__ },           /* UtopiaValue_QoSMacAddr */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "mac",                              UtopiaValue_QoSMacAddr },           /* UtopiaValue_QMA_Mac */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_QoSMacAddr },           /* UtopiaValue_QMA_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "class",                            UtopiaValue_QoSMacAddr },           /* UtopiaValue_QMA_Class */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "QoSVoiceDevice_%d",                UtopiaValue__UNKNOWN__ },           /* UtopiaValue_QoSVoiceDevice */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "mac",                              UtopiaValue_QoSVoiceDevice },       /* UtopiaValue_QVD_Mac */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_QoSVoiceDevice },       /* UtopiaValue_QVD_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "class",                            UtopiaValue_QoSVoiceDevice },       /* UtopiaValue_QVD_Class */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "PortRangeTrigger_%d",              UtopiaValue__UNKNOWN__ },           /* UtopiaValue_PortRangeTrigger */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "enabled",                          UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_Enabled */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "trigger_id",                       UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_TriggerID */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "trigger_protocol",                 UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_TriggerProtocol */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "trigger_range",                    UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_TriggerRange */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "forward_protocol",                 UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_ForwardProtocol */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "forward_range",                    UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_ForwardRange */
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "lifetime",                         UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_Lifetime */
    { Utopia_Type_IndexedConfig, Utopia_Event_SmbServer_Restart,  Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "SharedFolder_%d",                  UtopiaValue__UNKNOWN__ },           /* UtopiaValue_NAS_SharedFolder */
    { Utopia_Type_IndexedConfig, Utopia_Event_SmbServer_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "name",                             UtopiaValue_NAS_SharedFolder },     /* UtopiaValue_NSF_Name */
    { Utopia_Type_IndexedConfig, Utopia_Event_SmbServer_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "folder",                           UtopiaValue_NAS_SharedFolder },     /* UtopiaValue_NSF_Folder */
    { Utopia_Type_IndexedConfig, Utopia_Event_SmbServer_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "drive",                            UtopiaValue_NAS_SharedFolder },     /* UtopiaValue_NSF_Drive */
    { Utopia_Type_IndexedConfig, Utopia_Event_SmbServer_Restart,  Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "readonly",                         UtopiaValue_NAS_SharedFolder },     /* UtopiaValue_NSF_ReadOnly */


/* Utopia_Type_Indexed2Config */

    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockUrl_%d",                    UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockURL */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockUrl_tr_inst_num_%d",                    UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockURL_TR_INST_NUM */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockUrl_tr_alias_%d",                    UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockURL_TR_ALIAS */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockKeyword_%d",                UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockKeyword */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockKeyword_tr_inst_num_%d",                UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockKeyword_TR_INST_NUM */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockKeyword_tr_alias_%d",                UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockKeyword_TR_ALIAS */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockApplication_%d",            UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockApplication */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockApplication_tr_inst_num_%d",            UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockApplication_TR_INST_NUM */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockWellknownApplication_%d",   UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockWKApplication */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockWellknownApplication_tr_inst_num_%d",   UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockWKApplication_TR_INST_NUM */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "BlockApplicationRule_%d",        UtopiaValue_InternetAccessPolicy }, /* UtopiaValue_IAP_BlockApplicationRule */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "ip_%d",                          UtopiaValue_IAP_LocalHostList },    /* UtopiaValue_IAP_IP */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "ip_range_%d",                    UtopiaValue_IAP_LocalHostList },    /* UtopiaValue_IAP_IPRange */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "mac_%d",                         UtopiaValue_IAP_LocalHostList },    /* UtopiaValue_IAP_MAC */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "qos_rule_%d",                    UtopiaValue_QoSPolicy },            /* UtopiaValue_QSP_Rule */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "protocol_%d",                    UtopiaValue_QoSUserDefinedPolicy }, /* UtopiaValue_QUP_Protocol */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "port_range_%d",                  UtopiaValue_QoSUserDefinedPolicy }, /* UtopiaValue_QUP_PortRange */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_Both, 1, "class_%d",                       UtopiaValue_QoSUserDefinedPolicy }, /* UtopiaValue_QUP_Class */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_NS,   1, "name",                           UtopiaValue_IAP_BlockApplication }, /* UtopiaValue_IAP_BlockName */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_NS,   1, "protocol",                       UtopiaValue_IAP_BlockApplication }, /* UtopiaValue_IAP_BlockProto */
    { Utopia_Type_Indexed2Config, Utopia_Event_Firewall_Restart, Utopia_Static__NONE__, Utopia_Index_For_NS,   1, "port_range",                     UtopiaValue_IAP_BlockApplication }, /* UtopiaValue_IAP_BlockPortRange */


/* Utopia_Type_NamedConfig */

    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_description",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Description */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_network_mode",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_NetworkMode */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_ssid",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SSID */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_ssid_broadcast",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SSIDBroadcast */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_radio_band",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_RadioBand */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wide_channel",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_WideChannel */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_standard_channel",        UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_StandardChannel */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_channel",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Channel */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_sideband",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SideBand */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_state",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_State */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_security_mode",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SecurityMode */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_passphrase",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Passphrase */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_key_0",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Key0 */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_key_1",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Key1 */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_key_2",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Key2 */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_key_3",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Key3 */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_tx_key",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_TxKey */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_radius_server",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_RadiusServer */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_radius_port",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_RadiusPort */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_shared_key",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Shared */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_encryption",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Encryption */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_key_renewal",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_KeyRenewal */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_ap_isolation",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_ApIsolation */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_frame_burst",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_FrameBurst */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_authentication_type",     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AuthenticationType */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_basic_rate",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_BasicRate */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_transmission_rate",       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_TransmissionRate */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_n_transmission_rate",     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_N_TransmissionRate */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_transmission_power",      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_TransmissionPower */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_cts_protection_mode",     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_CTSProtectionMode */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_beacon_interval",         UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_BeaconInterval */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_dtim_interval",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_DTIMInterval */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_fragmentation_threshold", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_FragmentationThreshold */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_rts_threshold",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_RTSThreshold */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wps_mode",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_WPSMode */

/* Utopia_Type_Named2Config */


/* Utopia_Type_Event */

    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "firmware_version",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_FirmwareVersion */
    { Utopia_Type_Event, Utopia_Event_NTPClient_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_ntp_server1",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_DhcpServer1 */
    { Utopia_Type_Event, Utopia_Event_NTPClient_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_ntp_server2",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_DhcpServer2 */
    { Utopia_Type_Event, Utopia_Event_NTPClient_Restart, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_ntp_server3",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_DhcpServer3 */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "lan-status",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_CurrentState */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "usb_device_state",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_USB_DeviceState */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "usb_device_mount_pt", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_USB_DeviceMountPt */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "vlan2_mac",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_VLan2Mac */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_wan_ipaddr",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_CurrentIPAddr */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_wan_state",   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_CurrentState */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "default_router",      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_DefaultRouter */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "lan-restarting",      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_Restarting */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "wlan-restarting",     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Restarting */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "wan-restarting",      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WAN_Restarting */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "SN",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_SerialNumber */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_lan_ipaddr",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_LAN_CurrentIPAddr */

    /* Begin of IPv6 */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "ipv6_connection_state",    	UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Connection_State */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_lan_ipv6address",    	UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Current_Lan_IPv6_Address */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_lan_ipv6address_ll",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Current_Lan_IPv6_Address_ll */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_wan_ipv6address",    	UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Current_Wan_IPv6_Address */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_wan_ipv6address_ll",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Current_Wan_IPv6_Address_ll */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_wan_ipv6_interface",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Current_Wan_IPv6_Interface */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "ipv6_domain",    		UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Domain */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "ipv6_nameserver",   		UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Nameserver */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "ipv6_ntp_server",    		UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Ntp_Server */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "ipv6_prefix",    		UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Prefix */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "6rd_common_prefix4",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_6rd_Common_Prefix4, */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "6rd_enable",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_6rd_Enable, */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "6rd_relay",	   		UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_6rd_Relay */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "6rd_zone",                      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_6rd_Zone */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "6rd_zone_length",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_6rd_Zone_Length */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "6to4_enable",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_6to4_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "aiccu_enable",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Aiccu_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "aiccu_password",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Aiccu_Password */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "aiccu_prefix",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Aiccu_Prefix */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "aiccu_tunnel",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Aiccu_Tunnel */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "aiccu_user",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Aiccu_User */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcpv6c_duid",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_DHCPv6c_DUID */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcpv6c_enable",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_DHCPv6c_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcpv6s_duid",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_DHCPv6s_DUID */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcpv6s_enable",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_DHCPv6s_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "he_client_ipv6",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_HE_Client_IPv6 */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "he_enable",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_HE_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "he_password",                   UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_HE_Password */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "he_prefix",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_HE_Prefix */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "he_server_ipv4",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_HE_Server_IPv4 */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "he_tunnel",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_HE_Tunnel */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "he_user",                       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_HE_User */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ipv6_bridging_enable",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Bridging_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ipv6_ndp_proxy_enable",         UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Ndp_Proxy_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ipv6_static_enable",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Static_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "lan_ipv6addr",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Lan_Address */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "router_adv_enable",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_RA_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "router_adv_provisioning_enable",UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_RA_Provisioning_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_ipv6_default_gateway",      UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Default_Gateway */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_ipv6addr",                  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Wan_Address */
    /* End of IPv6 */
    /*Start of BYOI Parameters*/
    //The BYOI parameters are held her for future use. We are not allowing them to be set at this point. 
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 0, "primary_HSD_allowed",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Primary_HSD_Allowed */
    { Utopia_Type_Event,  Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "desired_hsd_mode",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Desired_HSD_Mode */
    { Utopia_Type_Event,  Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 0, "current_hsd_mode",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Current_HSD_Mode */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "last_configured_hsd_mode",        UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Last_Configured_HSD_Mode */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 0, "byoi_bridge_mode",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_BYOI_Bridge_Mode */
    /* End of BYOI Parameters*/
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "web_timeout",                     UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Web_Timeout */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "web_username",                    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Web_Username */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "web_userpassword",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Web_Password */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "tr_prov_code",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_TR_Prov_Code */
    { Utopia_Type_Config, Utopia_Event_NTPClient_Restart,          Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ntp_enabled",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Enabled */
    { Utopia_Type_Config, Utopia_Event__NONE__,          Utopia_Static__NONE__, Utopia_For__NONE__, 0, "ntp_status",                UtopiaValue__UNKNOWN__ }, /* UtopiaValue_NTP_Status */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 0, "device_first_use_date", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Device_FirstuseDate*/

 /*MoCA Intf*/
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "moca_Enable", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_Enable */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "moca_Alias", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_Alias */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "PreferredNC", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_PreferredNC */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "PrivacyEnabledSetting", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_PrivEnabledSet */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "FreqCurrentMaskSetting", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_FreqCurMaskSet */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "KeyPassphrase", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_KeyPassPhrase */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "TxPowerLimit", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_TxPowerLimit */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "PowerCntlPhyTarget", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_PwrCntlPhyTarget */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "BeaconPowerLimit", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_Moca_BeaconPwrLimit */
  
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_alias",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Alias*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_network_mode",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Network_Mode*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_autochannel_cycle",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AutoChannelCycle*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_guard_interval",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_GuardInterval*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_regulatory_mode",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Regulatory_Mode*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_regulatory_domain",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Regulatory_Domain*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_alias",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SSID_Alias*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_instance_num",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SSID_Instance_Num*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_alias",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AP_Alias*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_instance_num",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AP_Instance_Num*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wmm",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AP_WMM*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wmm_apsd",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AP_WMM_UAPSD*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wmm_no_ack",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AP_WMM_No_Ack*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_retry_limit",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AP_Retry_Limit*/
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "macmgwan", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_MAC_MgWan */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_instance_num", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Radio_Instance_Num*/
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wps", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_WPS_Enabled */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wps_pin",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_WPS_PIN_Method */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_wps_pbc",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_WPS_PBC_Method */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_server_pool_insNum", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_ServerPool_InsNum */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "dhcp_server_pool_alias", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_ServerPool_Alias */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dhcp_static_host_insNum_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_StaticHost_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dhcp_static_host_alias_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DHCP_StaticHost_Alias */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "user_count", UtopiaValue__UNKNOWN__ },     /* UtopiaValue_User_Count */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "user_insNum_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_UserIndx_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "user_name_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_UserName */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "user_password_%d", UtopiaValue__UNKNOWN__ },  /* UtopiaValue_Password */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "user_enabled_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_User_Enabled */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "user_remote_access_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_User_RemoteAccess */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "user_language_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_User_Language */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "user_access_perm_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_User_Access_Permissions */

    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_enable_mac_filter", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_Enable_MACFilter */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_num_mac_filter", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_NUM_MACFilter */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wl_num_ssid", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SSID_Num */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "wl_radio_index_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_SSID_Radio */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "%s_ap_state", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WLAN_AP_State */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_enabled", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_EmailEnabled */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_sendto", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_EmailSendTo */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_server", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_EmailServer */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_username", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_EmailUsername */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_password", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_EmailPassword */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_fromaddr", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_EmailFromAddr */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_sendlogs", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_SendLogs */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_firewallbreach", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_FirewallBreach */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_parentalcontrolbreach", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_ParentalControlBreach */
    { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "email_notification_alertswarning", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_Security_AlertsWarnings */

    /* Start of Parental Control */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "managedsites_enabled",               UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedSites_Enabled */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ManagedSiteBlockCount",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedSiteBlockedCount */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "ManagedSiteBlock_%d",       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ins_num",                   UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "alias",                     UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_Alias  */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "method",                    UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_Method */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "site",                      UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_Site   */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "always",                    UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_Always */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "start_time",                UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_StartTime */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "end_time",                  UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_EndTime */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "days",                      UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_Days   */
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "mac",                       UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_MAC */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "device_name",               UtopiaValue_ParentalControl_ManagedSiteBlocked }, /* UtopiaValue_ParentalControl_ManagedSiteBlocked_DeviceName */
#endif
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ManagedSiteTrustCount",              UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedSiteTrustedCount */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "ManagedSiteTrust_%d",       UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ins_num",                   UtopiaValue_ParentalControl_ManagedSiteTrusted }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "alias",                     UtopiaValue_ParentalControl_ManagedSiteTrusted }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted_Alias  */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "desc",                      UtopiaValue_ParentalControl_ManagedSiteTrusted }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted_Desc   */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ip_type",                   UtopiaValue_ParentalControl_ManagedSiteTrusted }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted_IpType  */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ip_addr",                   UtopiaValue_ParentalControl_ManagedSiteTrusted }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted_IPAddr  */
#ifdef _HUB4_PRODUCT_REQ_
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "mac_addr",                   UtopiaValue_ParentalControl_ManagedSiteTrusted }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted_MacAddr  */
#endif
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "trusted",                   UtopiaValue_ParentalControl_ManagedSiteTrusted }, /* UtopiaValue_ParentalControl_ManagedSiteTrusted_Trusted  */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "managedservices_enabled",            UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedServices_Enabled */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ManagedServiceBlockCount",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedServiceBlockedCount */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "ManagedServiceBlock_%d",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ins_num",                   UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "alias",                     UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_Alias  */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "desc",                      UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_Desc */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "proto",                     UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_Proto */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "start_port",                UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_StartPort */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "end_port",                  UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_EndPort */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "always",                    UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_Always */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "start_time",                UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_StartTime */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "end_time",                  UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_EndTime */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "days",                      UtopiaValue_ParentalControl_ManagedServiceBlocked }, /* UtopiaValue_ParentalControl_ManagedServiceBlocked_Days   */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ManagedServiceTrustCount",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedServiceTrustedCount */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "ManagedServiceTrust_%d",    UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ins_num",                   UtopiaValue_ParentalControl_ManagedServiceTrusted }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "alias",                     UtopiaValue_ParentalControl_ManagedServiceTrusted }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted_Alias  */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "desc",                      UtopiaValue_ParentalControl_ManagedServiceTrusted }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted_Desc   */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ip_type",                   UtopiaValue_ParentalControl_ManagedServiceTrusted }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted_IpType  */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ip_addr",                   UtopiaValue_ParentalControl_ManagedServiceTrusted }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted_IpAddr  */
#ifdef _HUB4_PRODUCT_REQ_
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "mac_addr",                   UtopiaValue_ParentalControl_ManagedServiceTrusted }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted_MacAddr  */
#endif
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "trusted",                   UtopiaValue_ParentalControl_ManagedServiceTrusted }, /* UtopiaValue_ParentalControl_ManagedServiceTrusted_Trusted  */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "manageddevices_enabled",             UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedDevices_Enabled */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "manageddevices_allow_all",           UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedDevices_AllowAll */
    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "ManagedDeviceCount",                 UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedDeviceCount */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "ManagedDevice_%d",          UtopiaValue__UNKNOWN__ }, /* UtopiaValue_ParentalControl_ManagedDevice */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ins_num",                   UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "alias",                     UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_Alias */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "block",                     UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_Block */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "desc",                      UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_Desc */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "mac_addr",                  UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_MacAddr */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "always",                    UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_Always */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "start_time",                UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_StartTime */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "end_time",                  UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_EndTime */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "days",                      UtopiaValue_ParentalControl_ManagedDevice }, /* UtopiaValue_ParentalControl_ManagedDevice_Days */
    /* End of parental control */
#if defined(DDNS_BROADBANDFORUM)

{ Utopia_Type_Config,        Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__,   1, "DynamicDnsClientCount", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DynamicDnsClientCount */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "DynamicDnsClient_%d", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_DynamicDnsClient */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "ins_num",             UtopiaValue_DynamicDnsClient }, /* UtopiaValue_DynamicDnsClient_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "alias",               UtopiaValue_DynamicDnsClient }, /* UtopiaValue_DynamicDnsClient_Alias */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "enable",              UtopiaValue_DynamicDnsClient }, /* UtopiaValue_DynamicDnsClient_Enable */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "Username",            UtopiaValue_DynamicDnsClient }, /* UtopiaValue_DynamicDnsClient_Username */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "Password",            UtopiaValue_DynamicDnsClient }, /* UtopiaValue_DynamicDnsClient_Password */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "Server",            UtopiaValue_DynamicDnsClient }, /* UtopiaValue_DynamicDnsClient_Server */
    /* End of Dynamic DNS */

#endif

    { Utopia_Type_Config, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_For__NONE__, 1, "lan_clients_count", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_USGv2_Lan_Clients_Count */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "lan_clients_%d", UtopiaValue__UNKNOWN__ },  /* UtopiaValue_USGv2_Lan_Clients */
    { Utopia_Type_NamedConfig, Utopia_Event__NONE__,   Utopia_Static__NONE__, Utopia_Name_For_Key, 1, "lan_clients_mac_%s", UtopiaValue__UNKNOWN__ }, /* UtopiaValue_USGv2_Lan_Clients_Mac */
    { Utopia_Type_Event, Utopia_Event__NONE__,           Utopia_Static__NONE__, Utopia_For__NONE__, 0, "pnm-status",    		UtopiaValue__UNKNOWN__ }, /* UtopiaValue_IPv6_Prefix */  
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,	Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "prev_rule_enabled_state",			UtopiaValue_SinglePortForward },	/* UtopiaValue_SPF_PrevRuleEnabledState */	  
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "prev_rule_enabled_state",          UtopiaValue_PortRangeForward },     /* UtopiaValue_PFR_PrevRuleEnabledState */        
    { Utopia_Type_IndexedConfig, Utopia_Event_Firewall_Restart,   Utopia_Static__NONE__, Utopia_Index_For_NS,  1, "prev_rule_enabled_state",          UtopiaValue_PortRangeTrigger },     /* UtopiaValue_PRT_PrevRuleEnabledState */
    	{ Utopia_Type_IndexedConfig, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "hash_password_%d", UtopiaValue__UNKNOWN__ },  /* UtopiaValue_HashPassword */
#if defined(_WAN_MANAGER_ENABLED_)
      { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_mode",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WanMode */
      { Utopia_Type_Config, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 1, "wan_conn_enabled",  UtopiaValue__UNKNOWN__ }, /* UtopiaValue_WanConnEnabled */
#endif /*_WAN_MANAGER_ENABLED_*/
#ifdef DSLITE_FEATURE_SUPPORT
    { Utopia_Type_Config,        Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_For__NONE__,   1, "dslite_enable",             UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Enable */
    { Utopia_Type_Config,        Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_For__NONE__,   1, "dslite_count",              UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Count */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_InsNum_%d",          UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_InsNum */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_active_%d",          UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Active */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,              Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_status_%d",          UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Status */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,              Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_alias_%d",           UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Alias */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_mode_%d",            UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Mode */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_addr_type_%d",       UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Addr_Type */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,              Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_addr_inuse_%d",      UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Addr_Inuse */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_addr_fqdn_%d",       UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Addr_Fqdn */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_addr_ipv6_%d",       UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Addr_IPv6 */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,              Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_origin_%d",          UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Origin */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,              Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_tunnel_interface_%d",    UtopiaValue__UNKNOWN__ },      /* UtopiaValue_Dslite_Tunnel_Interface */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,              Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_tunneled_interface_%d",  UtopiaValue__UNKNOWN__ },      /* UtopiaValue_Dslite_Tunneled_Interface */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_mss_clamping_enable_%d", UtopiaValue__UNKNOWN__ },      /* UtopiaValue_Dslite_Mss_Clamping_Enable */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_tcpmss_%d",          UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Tcpmss */
    { Utopia_Type_IndexedConfig, Utopia_Event_DSLite_Restart,       Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_ipv6_frag_enable_%d", UtopiaValue__UNKNOWN__ },         /* UtopiaValue_Dslite_IPv6_Frag_Enable */
    { Utopia_Type_IndexedConfig, Utopia_Event__NONE__,              Utopia_Static__NONE__, Utopia_Index_For_Key, 1, "dslite_tunnel_v4addr_%d",   UtopiaValue__UNKNOWN__ },          /* UtopiaValue_Dslite_Tunneled_Interface */
#endif
    { Utopia_Type__UNKNOWN__, Utopia_Event__NONE__, Utopia_Static__NONE__, Utopia_For__NONE__, 0, 0, UtopiaValue__UNKNOWN__ }, /* UtopiaValue__LAST__ */
};

#define Utopia_ToKey(x)              g_Utopia[(int)x].pszKey
#define Utopia_ToEvent(x)            (Utopia_Event)g_Utopia[(int)x].ixEvent
#define Utopia_ToFor(x)              (Utopia_Index_For)g_Utopia[(int)x].ixFor
#define Utopia_ToNamespace(x)        (UtopiaValue)g_Utopia[(int)x].ixNamespace
#define Utopia_ToType(x)             (Utopia_Type)g_Utopia[(int)x].ixType

#define Utopia_IsConfig(x)            (Utopia_ToType(x) == Utopia_Type_Config)
#define Utopia_IsIndexedConfig(x)     (Utopia_ToType(x) == Utopia_Type_IndexedConfig)
#define Utopia_IsIndexed2Config(x)    (Utopia_ToType(x) == Utopia_Type_Indexed2Config)
#define Utopia_IsNamedConfig(x)       (Utopia_ToType(x) == Utopia_Type_NamedConfig)
#define Utopia_IsNamed2Config(x)      (Utopia_ToType(x) == Utopia_Type_Named2Config)
#define Utopia_IsEvent(x)             (Utopia_ToType(x) == Utopia_Type_Event)
#define Utopia_IsStatic(x)            (Utopia_ToType(x) == Utopia_Type_Static)

#define Utopia_IsIndexForBoth(x)      (Utopia_ToFor(x) == Utopia_Index_For_Both)
#define Utopia_IsIndexForKey(x)       (Utopia_ToFor(x) == Utopia_Index_For_Key)
#define Utopia_IsIndexForNS(x)        (Utopia_ToFor(x) == Utopia_Index_For_NS)
#define Utopia_IsNameForBoth(x)        (Utopia_ToFor(x) == Utopia_Name_For_Both)
#define Utopia_IsNameForKey(x)        (Utopia_ToFor(x) == Utopia_Name_For_Key)
#define Utopia_IsNameForNS(x)         (Utopia_ToFor(x) == Utopia_Name_For_NS)

#define Utopia_SetAllowed(x)          (g_Utopia[(int)x].fSetAllowed != 0)


/* Utopia auth file update helper function */
static void s_Utopia_AuthFileUpdate(UtopiaContext* pCtx)
{
#ifndef UTCTX_UNITTEST
    char* pszHttpdSh = "/etc/init.d/service_httpd/httpd_util.sh";
    char pszArgs[UTOPIA_BUF_SIZE] = {'\0'};
    errno_t rc = -1;
    char* ppszEnv[] =
        {
            "PATH=/bin:/sbin:/usr/sbin:/usr/bin",
            "SHELL=/bin/sh",
            (char*)0
        };

    pid_t pID;

    /* Get the HTTP username\password */
    char pszDevicePassword[UTOPIA_MAX_PASSWD_LENGTH + 1] = {'\0'};
    char pszDeviceUsername[UTOPIA_MAX_USERNAME_LENGTH + 1] = {'\0'};

    /* Get the username and password stored in the device */
    Utopia_Get(pCtx, UtopiaValue_HTTP_AdminPassword, pszDevicePassword, UTOPIA_MAX_PASSWD_LENGTH);
    Utopia_Get(pCtx, UtopiaValue_HTTP_AdminUser, pszDeviceUsername, UTOPIA_MAX_USERNAME_LENGTH);
       
    /* Format the arguments */
    rc = sprintf_s(pszArgs, sizeof(pszArgs), "generate_authfile %s %s", pszDeviceUsername, pszDevicePassword);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }

    /* Fork and execute */
    pID = fork();

    /* If child process, execute command */
    if (pID == 0)
    {
        execle(pszHttpdSh, "", pszArgs, (char*)0, ppszEnv);
        /* If we get here, it means the call failed, so just exit the child process */
        exit(0);
    }
#else
    /* Unused parameter */
    (void) pCtx;
#endif
    UTCTX_LOG_DBG1("%s: Updating HTTPD auth file...\n", __FUNCTION__);
}

/* Utopia namespace/key formatting helper function */
static int s_UtopiaValue_FormatNamespaceKey(UtopiaContext* pCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2, int iIndex1, int iIndex2,
                                            char* pszNamespace, unsigned int cbNamespace, char* pszKey, unsigned int cbKey)
{
    unsigned int iKeyLength;
    errno_t      rc = -1;

    /* Validate our indexes and prefix depending on UtopiaValue type */
    if (Utopia_IsIndexedConfig(ixUtopia) && iIndex1 < 1)
    {
        return 0;
    }
    else if (Utopia_IsIndexed2Config(ixUtopia) && (iIndex1 < 1 || iIndex2 < 1))
    {
        return 0;
    }
    else if (Utopia_IsNamedConfig(ixUtopia) && pszName1 == 0)
    {
        return 0;
    }
    else if (Utopia_IsNamed2Config(ixUtopia) && (pszName1 == 0 || pszName2 == 0))
    {
        return 0;
    }

    /* Make sure there's enough room, if so, format key */
    iKeyLength = strlen(Utopia_ToKey(ixUtopia));
    if ((Utopia_IsConfig(ixUtopia) || Utopia_IsEvent(ixUtopia) ||
         ((Utopia_IsIndexedConfig(ixUtopia) || Utopia_IsIndexed2Config(ixUtopia)) && Utopia_IsIndexForNS(ixUtopia)) ||
         ((Utopia_IsNamedConfig(ixUtopia) || Utopia_IsNamed2Config(ixUtopia)) && Utopia_IsNameForNS(ixUtopia))) &&
        (iKeyLength + 1 <= cbKey))
    {
        rc = strcpy_s(pszKey, cbKey, Utopia_ToKey(ixUtopia));
        ERR_CHK(rc);
    }
    else if (Utopia_IsIndexedConfig(ixUtopia) && Utopia_IsIndexForKey(ixUtopia) &&
             (iIndex1 < 1000) && (iKeyLength + 2 <= cbKey))
    {
        rc = sprintf_s(pszKey, cbKey, Utopia_ToKey(ixUtopia), iIndex1);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    else if (Utopia_IsIndexed2Config(ixUtopia) && Utopia_IsIndexForBoth(ixUtopia) &&
             (iIndex2 < 1000) && (iKeyLength + 2 <= cbKey))
    {
        rc = sprintf_s(pszKey, cbKey, Utopia_ToKey(ixUtopia), iIndex2);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    else if (Utopia_IsIndexed2Config(ixUtopia) && Utopia_IsIndexForKey(ixUtopia) &&
             (iIndex1 < 1000) && (iIndex2 < 1000) && (iKeyLength + 3 <= cbKey))
    {
        rc = sprintf_s(pszKey, cbKey, Utopia_ToKey(ixUtopia), iIndex1, iIndex2);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    else if (Utopia_IsNamedConfig(ixUtopia) && Utopia_IsNameForKey(ixUtopia) &&
             (iKeyLength + strlen(pszName1) - 1 <= cbKey))
    {
        rc = sprintf_s(pszKey, cbKey, Utopia_ToKey(ixUtopia), pszName1);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    else if (Utopia_IsNamed2Config(ixUtopia) && Utopia_IsNameForBoth(ixUtopia) &&
             (iKeyLength + strlen(pszName2) - 1 <= cbKey))
    {
        rc = sprintf_s(pszKey, cbKey, Utopia_ToKey(ixUtopia), pszName2);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    else if (Utopia_IsNamed2Config(ixUtopia) && Utopia_IsNameForKey(ixUtopia) &&
             (iKeyLength + strlen(pszName1) + strlen(pszName2) - 3 <= cbKey))
    {
        rc = sprintf_s(pszKey, cbKey, Utopia_ToKey(ixUtopia), pszName1, pszName2);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    else
    {
        return 0;
    }

    /* Now format the namespace */
    if (Utopia_ToNamespace(ixUtopia) != UtopiaValue__UNKNOWN__)
    {
        if (Utopia_IsIndexed2Config(ixUtopia) && Utopia_IsIndexForNS(ixUtopia))
        {
            if (Utopia_GetIndexed2(pCtx, Utopia_ToNamespace(ixUtopia), iIndex1, iIndex2, pszNamespace, cbNamespace) == 0)
            {
                return 0;
            }
        }
        else if (Utopia_IsIndexForNS(ixUtopia) || Utopia_IsIndexForBoth(ixUtopia))
        {
            if (Utopia_GetIndexed(pCtx, Utopia_ToNamespace(ixUtopia), iIndex1, pszNamespace, cbNamespace) == 0)
            {
                return 0;
            }
        }
        else if (Utopia_IsNamed2Config(ixUtopia) && Utopia_IsNameForNS(ixUtopia))
        {
            if (Utopia_GetNamed2(pCtx, Utopia_ToNamespace(ixUtopia), pszName1, pszName2, pszNamespace, cbNamespace) == 0)
            {
                return 0;
            }
        }
        else if (Utopia_IsNameForNS(ixUtopia) || Utopia_IsNameForBoth(ixUtopia))
        {
            if (Utopia_GetNamed(pCtx, Utopia_ToNamespace(ixUtopia), pszName1, pszNamespace, cbNamespace) == 0)
            {
                return 0;
            }
        }
    }
    else if (Utopia_IsNameForNS(ixUtopia) || Utopia_IsNameForBoth(ixUtopia))
    {
        if (strlen(pszName1) >= cbNamespace)
        {
            return 0;
        }
        rc = strcpy_s(pszNamespace, cbNamespace, pszName1);
        ERR_CHK(rc);
    }
    return 1;
}

/* Helper function to translate from Utopia_Event enum to index */
static int s_UtopiaEvent_EnumToIndex(Utopia_Event event)
{
    int ix = (int)event;

    /* Translate Utopia_Event bitfield into table index (log base2)*/
    if (event > 1)
    {
        int i;
        ix = 1;
        for (i = (int)event; i > 1; i /= 2) ++ix;
    }

    return ix;
}

/* Utopia system event get helper function */
static int s_UtopiaEvent_Get(UtopiaContext* pUtopiaCtx, char* pszKey, char* pszValue, unsigned int ccbBuf)
{
    /* Initialize the event handle if it's not yet been happened */
    if (pUtopiaCtx->iEventHandle == 0 &&
        ((pUtopiaCtx->iEventHandle = SysEvent_Open(UTCTX_EVENT_ADDRESS, UTCTX_EVENT_PORT, UTCTX_EVENT_VERSION, UTCTX_EVENT_NAME, (token_t *)&pUtopiaCtx->uiEventToken)) == 0))
    {
        return 0;
    }

    return SysEvent_Get(pUtopiaCtx->iEventHandle, pUtopiaCtx->uiEventToken, pszKey, pszValue, ccbBuf);
}

/* Utopia system event set helper function */
static int s_UtopiaEvent_Set(UtopiaContext* pUtopiaCtx, char* pszKey, char* pszValue)
{
    /* Initialize the event handle if it's not yet been happened */
    if (pUtopiaCtx->iEventHandle == 0 &&
        ((pUtopiaCtx->iEventHandle = SysEvent_Open(UTCTX_EVENT_ADDRESS, UTCTX_EVENT_PORT, UTCTX_EVENT_VERSION, UTCTX_EVENT_NAME, (token_t *)&pUtopiaCtx->uiEventToken)) == 0))
    {
        return 0;
    }

    UTCTX_LOG_CFG3("%s: set key %s, value %s\n", __FUNCTION__, pszKey, pszValue);
    return SysEvent_Set(pUtopiaCtx->iEventHandle, pUtopiaCtx->uiEventToken, pszKey, pszValue, 0);
}

/* Utopia system event listener helper function */
static int s_UtopiaEvent_Wait(UtopiaContext* pUtopiaCtx, char* pszEventName, char* pszEventValue, int iTimeout)
{
    if (pszEventName == 0)
    {
        return 0;
    }

    if (pszEventValue == 0)
    {
        UTCTX_LOG_DBG3("%s: Waiting %d seconds for sysevent %s value to change...\n", __FUNCTION__, iTimeout, pszEventName);
    }
    else
    {
        UTCTX_LOG_DBG4("%s: Waiting %d seconds for sysevent %s value to change to %s...\n", __FUNCTION__, iTimeout, pszEventName, pszEventValue);
    }
#ifndef UTCTX_UNITTEST
    /* Loop until we either time out on a select call, or the desired event is occurs */
    for (;;)
    {
        fd_set fdSet;
        struct timeval tVal;
        int iReturn;

        FD_ZERO(&fdSet);
        FD_SET(pUtopiaCtx->iEventHandle, &fdSet);

        tVal.tv_sec = iTimeout;
        tVal.tv_usec = 0;

        if ((iReturn = select(pUtopiaCtx->iEventHandle + 1, &fdSet, NULL, NULL, &tVal)) != 0)
        {
            int msgType;
            se_buffer msgBuffer;
            se_notification_msg* pmsgBody = (se_notification_msg *)msgBuffer;
            token_t token;
            unsigned int msgSize = sizeof(msgBuffer);

            msgType = SE_msg_receive(pUtopiaCtx->iEventHandle, msgBuffer, &msgSize, &token);

            /* If not a notification message then ignore it */
            if (SE_MSG_NOTIFICATION == msgType)
            {
                /* Extract the name and value from the return message data */
                int iNameBytes;
                int iValueBytes;
                char* pszName;
                char* pszValue;
                char* pData;

                pData = (char*)&(pmsgBody->data);
                pszName = (char*)SE_msg_get_string(pData, &iNameBytes);
                pData += iNameBytes;
                pszValue = (char*)SE_msg_get_string(pData, &iValueBytes);

                if (pszName != 0 &&
                    strcmp(pszName, pszEventName) == 0)
                {
                    if (pszValue == 0)
                    {
                        return 0;
                    }
                    else if (pszEventValue == 0 ||
                             strcmp(pszValue, pszEventValue) == 0)
                    {
                        return 1;
                    }
                }
            }
        }
        else
        {
            /* Event timed out */
            return 0;
        }
    }
    /* Should never get here */
    return 0;
#else
    (void)iTimeout;
    (void)pUtopiaCtx;
    return 1;
#endif
}

/* Utopia event set notification helper function */
static void s_UtopiaEvent_SetNotify(UtopiaContext* pUtopiaCtx, char* pszWaitKey)
{
#ifndef UTCTX_UNITTEST
    async_id_t aID;
    int rc = sysevent_setnotification(pUtopiaCtx->iEventHandle, pUtopiaCtx->uiEventToken, pszWaitKey, &aID);
    (void)rc;
#else
    (void)pszWaitKey;
    (void)pUtopiaCtx;
#endif
    UTCTX_LOG_DBG2("%s: Setting notify on sysevent %s...\n", __FUNCTION__, pszWaitKey);
}

/* Utopia system event trigger helper function */
static void s_UtopiaEvent_Trigger(UtopiaContext* pUtopiaCtx)
{
    /* Handle auth file update */
    if ((pUtopiaCtx->bfEvents & Utopia_Event_HTTPServer_Restart) == Utopia_Event_HTTPServer_Restart)
    {
        s_Utopia_AuthFileUpdate(pUtopiaCtx);
    }

    /* Initialize the event handle if we need to if it's not yet happened */
    if (pUtopiaCtx->bfEvents != Utopia_Event__NONE__ &&
        pUtopiaCtx->iEventHandle == 0 &&
        ((pUtopiaCtx->iEventHandle = SysEvent_Open(UTCTX_EVENT_ADDRESS, UTCTX_EVENT_PORT, UTCTX_EVENT_VERSION, UTCTX_EVENT_NAME, (token_t *)&pUtopiaCtx->uiEventToken)) == 0))
    {
        return;
    }

    /* Reboot overides all */
    if ((pUtopiaCtx->bfEvents & Utopia_Event_Reboot) == Utopia_Event_Reboot)
    {
        UTCTX_LOG_DBG1("%s: Rebooting...\n", __FUNCTION__);

        /* Signal that a reboot is occurring */
        SysEvent_Trigger(pUtopiaCtx->iEventHandle, pUtopiaCtx->uiEventToken, g_Utopia_Events[s_UtopiaEvent_EnumToIndex(Utopia_Event_Reboot)].pszEventKey, 0);
    }
    else
    {
        unsigned int i;

        /* Loop over each of the events */
        for (i = 1; i < Utopia_Event__LAST__; i = i << 1)
        {
            if (Utopia_EventSet(pUtopiaCtx->bfEvents, i))
            {
                /* If this is a DHCP server restart and we are doing a lan restart, then skip */
                if (((Utopia_Event)i != Utopia_Event_DHCPServer_Restart ||
                     !Utopia_EventSet(pUtopiaCtx->bfEvents, Utopia_Event_LAN_Restart)))
                {
                    unsigned int ix = s_UtopiaEvent_EnumToIndex((Utopia_Event)i);

                    /* RDKB-7126, CID-33554, Out-of-bounds read
                    ** Perform boundary check before passing index.
                    ** this is to limit array access.defined for "g_Utopia_Events"
                    ** with maximum "MAX_UTOPIA_EVENTS"
                    */
                    if(ix > MAX_UTOPIA_EVENTS-1)
                    {
                        ix = MAX_UTOPIA_EVENTS-1;
                    }
                    else if(ix < 0)
                    {
                        ix = 0;
                    }
                    else
                    {
                        ix =ix;
                    }

                    /* Set the event notification if we need to */
                    if (g_Utopia_Events[ix].pszWaitKey != 0)
                    {
                        s_UtopiaEvent_SetNotify(pUtopiaCtx, g_Utopia_Events[ix].pszWaitKey);
                    }

                    /* Trigger the event */
                    UTCTX_LOG_CFG2("%s: Trigger %s\n", __FUNCTION__, g_Utopia_Events[ix].pszEventKey);
                    SysEvent_Trigger(pUtopiaCtx->iEventHandle, pUtopiaCtx->uiEventToken, g_Utopia_Events[ix].pszEventKey, 0);

                    /* Wait for a sysevent value to change if needed */
                    if (g_Utopia_Events[ix].pszWaitKey != 0)
                    {
                        s_UtopiaEvent_Wait(pUtopiaCtx, g_Utopia_Events[ix].pszWaitKey, g_Utopia_Events[ix].pszWaitValue, g_Utopia_Events[ix].iWaitTimeout);
                    }
                }
            }
        }
    }
}

/* Helper function for adding utopia transaction node */
static UtopiaTransact_Node* s_UtopiaTransact_Add(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszNamespace, char* pszKey, char* pszValue)
{
    UtopiaTransact_Node* pNode;

    if (pszKey == 0)
    {
        return 0;
    }

    /* Allocate a transaction node */
    if ((pNode = (UtopiaTransact_Node*)calloc(1, sizeof(UtopiaTransact_Node))) == 0)
    {
        return 0;
    }


    if ((pNode->pszKey = strdup(pszKey)) == NULL) {
        free(pNode);
        return NULL;
    }

    if (pszNamespace && (pNode->pszNamespace = strdup(pszNamespace)) == NULL) //CID:56805
    {
	free(pNode->pszKey);
        free(pNode);
        return NULL;
    }
    if (pszValue && (pNode->pszValue = strdup(pszValue)) == NULL) // CID:56805 : Wrong sizeof argument
    {
        free(pNode->pszKey);
        if (pNode->pszNamespace)
        {
            free(pNode->pszNamespace);
        }
        free(pNode);
        return NULL;
    }

    /* Set the utopia index, namespace, key, and value */
    pNode->ixUtopia = ixUtopia;
    /* Add the node to the transaction list */
    pNode->pNext = pUtopiaCtx->pHead;
    pUtopiaCtx->pHead = pNode;

    return pNode;
}

/* Helper function for getting utopia transaction node */
static UtopiaTransact_Node* s_UtopiaTransact_Find(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszNamespace, char* pszKey)
{
    UtopiaTransact_Node* pNode;

    (void)ixUtopia;

    if (pszKey == 0)
    {
        UTCTX_LOG_ERR2("%s: pszKey is NULL, ixUtopia= %d\n", __FUNCTION__, (int)ixUtopia);
        return 0;
    }
    if (pUtopiaCtx == 0)
    {
        UTCTX_LOG_ERR2("%s: pUtopiaCtx is NULL, ixUtopia= %d\n", __FUNCTION__, (int)ixUtopia);
        return 0;
    }

    /* Iterate the transaction list and return node if it's there */
    for (pNode = pUtopiaCtx->pHead; pNode; pNode = pNode->pNext)
    {
        if (pNode->pszKey != 0  && strcmp(pNode->pszKey, pszKey) == 0 &&
            (pNode->pszNamespace == 0 || pszNamespace == 0 || strcmp(pNode->pszNamespace, pszNamespace) == 0))
        {
            return pNode;
        }
    }

    return 0;
}

/* Helper function for getting a value from the transaction list */
static int UtopiaTransact_Get(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszNamespace, char* pszKey,
                       char* pszValue, unsigned int ccbBuf)
{
    UtopiaTransact_Node* pNode;

    /* ensure output buffer is initialised in all cases, including errors */
    if (pszValue)
    {
        *pszValue = 0;
    }

    if (pUtopiaCtx == 0)
    {
        UTCTX_LOG_ERR2("%s: pUtopiaCtx is NULL, ixUtopia= %d\n", __FUNCTION__, (int)ixUtopia);
        return 0;
    }
    if (pszKey == 0)
    {
        UTCTX_LOG_ERR2("%s: pszKey is NULL, ixUtopia= %d\n", __FUNCTION__, (int)ixUtopia);
        return 0;
    }
    if (pszValue == 0)
    {
        UTCTX_LOG_ERR2("%s: pszValue is NULL, ixUtopia= %d\n", __FUNCTION__, (int)ixUtopia);
        return 0;
    }

    /* Check the transaction list for value */
    if ((pNode = s_UtopiaTransact_Find(pUtopiaCtx, ixUtopia, pszNamespace, pszKey)) != 0)
    {
        /* If the value is 'unset' then we need to fail */
        if (pNode->pszValue == 0)
        {
            return 0;
        }

        strncpy(pszValue, pNode->pszValue, ccbBuf - 1);
    }
    else
    {
        /* Acquire read lock */
        if (UtopiaRWLock_ReadLock(&pUtopiaCtx->rwLock) == 0)
        {
            return 0;
        }

        /* If this is an event, then call the SysEvent helper function, otherwise call SysCfg get */
        if (Utopia_IsEvent(ixUtopia))
        {
            if (s_UtopiaEvent_Get(pUtopiaCtx, pszKey, pszValue, ccbBuf) == 0)
            {
                return 0;
            }
        }
        else
        {
            if (SysCfg_Get(pszNamespace, pszKey, pszValue, ccbBuf) == 0)
            {
                return 0;
            }
        }
    }
    return 1;
}

/* Helper function for setting a value into the transaction list */
static int UtopiaTransact_Set(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszNamespace, char* pszKey,
                       char* pszValue)
{
    UtopiaTransact_Node* pNode;

    if (pszNamespace != 0)
    {
        UTCTX_LOG_CFG4("%s: key %s::%s, value %s\n", __FUNCTION__, pszNamespace, pszKey, pszValue);
    }
    else
    {
        UTCTX_LOG_CFG3("%s: key %s, value %s\n", __FUNCTION__, pszKey, pszValue);
    }

    /* See if the value is in the transaction list */
    if ((pNode = s_UtopiaTransact_Find(pUtopiaCtx, ixUtopia, pszNamespace, pszKey)) != 0)
    {
        /* Free the old value */
        if (pNode->pszValue)
        {
            free(pNode->pszValue);
        }

        /* Allocate memory and copy value if needed */
        if (pszValue)
        {
            pNode->pszValue = strdup(pszValue); //CID 62572: Wrong sizeof argument
	    if (! pNode->pszValue)
            {
                return 0;
            }
        }
        else
        {
            pNode->pszValue = 0;
        }
    }
    else
    {
        char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

        /* Check to see if the value is changing before adding it to transaction list */
        if ((Utopia_IsEvent(ixUtopia) &&
             ((s_UtopiaEvent_Get(pUtopiaCtx, pszKey, pszBuffer, sizeof(pszBuffer)) == 0 && pszValue != 0) ||
              (s_UtopiaEvent_Get(pUtopiaCtx, pszKey, pszBuffer, sizeof(pszBuffer)) != 0 && pszValue == 0) ||
              (pszValue != 0 && strcmp(pszValue, pszBuffer) != 0))) ||
            ((SysCfg_Get(pszNamespace, pszKey, pszBuffer, sizeof(pszBuffer)) == 0 && pszValue != 0) ||
             (SysCfg_Get(pszNamespace, pszKey, pszBuffer, sizeof(pszBuffer)) != 0 && pszValue == 0) ||
             (pszValue != 0 && strcmp(pszValue, pszBuffer) != 0)))
        {
            /* Acquire write lock */
            if (UtopiaRWLock_WriteLock(&pUtopiaCtx->rwLock) == 0)
            {
                return 0;
            }

            if ((pNode = s_UtopiaTransact_Add(pUtopiaCtx, ixUtopia, pszNamespace, pszKey, pszValue)) == 0)
            {
                return 0;
            }

            /* Update the events bitfield */
            pUtopiaCtx->bfEvents |= Utopia_ToEvent(pNode->ixUtopia);
        }
    }
    return 1;
}

/* Helper function for getting all of the values from syscfg and the transact list, merging them, then returning */
static int s_UtopiaTransact_GetAll(UtopiaContext* pUtopiaCtx, char* pszBuffer, unsigned int ccbBuf)
{
    char* pPtr;
    char pState[UTOPIA_STATE_SIZE];
    int iSize;
    UtopiaTransact_Node* pNode;
    errno_t rc = -1;

    /* Zero out the buffer */
    memset(pszBuffer, 0, ccbBuf);

    /* First off, copy all of the values in the transaction list into buffer */
    for (pNode = pUtopiaCtx->pHead; pNode; pNode = pNode->pNext)
    {
        int j;

        /* Skip if we're unsetting */
        if (pNode->pszValue == 0)
        {
            continue;
        }

        /* Make sure there's enough room in the buffer */
        if ((strlen(pNode->pszKey) + strlen(pNode->pszValue) + (pNode->pszNamespace == 0 ? 0 : strlen(pNode->pszNamespace) + 2) + 3) > ccbBuf)
        {
            return 0;
        }

        if (pNode->pszNamespace != 0)
        {
            rc = sprintf_s(pszBuffer, ccbBuf, "%s::%s=%s\n", pNode->pszNamespace, pNode->pszKey, pNode->pszValue);
            if(rc < EOK)
            {
                ERR_CHK(rc);
                return 0;
            }
            j = rc;
        }
        else
        {
            rc = sprintf_s(pszBuffer, ccbBuf, "%s=%s\n", pNode->pszKey, pNode->pszValue);
            if(rc < EOK)
            {
                ERR_CHK(rc);
                return 0;
            }
            j = rc;
        }

        pszBuffer += j;
        ccbBuf -= j;
    }

    /* Next, call syscfg get all */
    if (SysCfg_GetAll(pState, sizeof(pState), &iSize) == 0)
    {
        return 0;
    }

    pPtr = pState;

    /* Copy state into buffer, skipping any values that were already copied from the transaction list */
    while(iSize > 0)
    {
        char* pszNamespace = 0;
        char* pszKey;
        char* pszValue;
        int iLen = strlen(pPtr);

        /* Parse the namespace */
        if ((pszKey = strchr(pPtr, ':')) != 0 && *(pPtr + 1) == ':')
        {
            pszNamespace = pPtr;

            /* Null terminate namespace and move pointer */
            *pszKey = '\0';
            pszKey += 2;
        }
        else
        {
            pszKey = pPtr;
        }
        /* Parse the key and value */
        if ((pszValue = strchr(pszKey, '=')) != 0)
        {
            int j;
            UtopiaTransact_Node* pNode;
            UtopiaValue ixUtopia = UtopiaValue__UNKNOWN__;

            /* Null terminate key and move pointer */
            *pszValue = '\0';
            pszValue++;

            /* See if the value is in the transaction list */
            if ((pNode = s_UtopiaTransact_Find(pUtopiaCtx, ixUtopia, pszNamespace, pszKey)) == 0)
            {
                /* Make sure there's enough room in the buffer */
                if ((strlen(pszKey) + strlen(pszValue) + (pszNamespace == 0 ? 0 : strlen(pszNamespace) + 2) + 3) > ccbBuf)
                {
                    return 0;
                }

                if (pszNamespace != 0)
                {
                    rc = sprintf_s(pszBuffer, ccbBuf, "%s::%s=%s\n", pszNamespace, pszKey, pszValue);
                    if(rc < EOK)
                    {
                        ERR_CHK(rc);
                        return 0;
                    }
                    j = rc;
                }
                else
                {
                    rc = sprintf_s(pszBuffer, ccbBuf, "%s=%s\n", pszKey, pszValue);
                    if(rc < EOK)
                    {
                        ERR_CHK(rc);
                        return 0;
                    }
                    j = rc;
                }

                pszBuffer += j;
                ccbBuf -= j;
            }
        }

        /* Move the buffer pointer and update size */
        pPtr = pPtr + iLen + 1;
        iSize -= iLen + 1;
    }
    return 1;
}

/* Helper function for unsetting all of the old syscfg values and setting new ones from pszBuffer */
static int s_UtopiaTransact_SetAll(UtopiaContext* pUtopiaCtx, char* pszBuffer, int fUnset)
{
    char* pszEnd;

    /* Iterate over the input buffer, parsing values and setting them into the transaction list */
    while((pszEnd = strchr(pszBuffer, '\n')) != 0)
    {
        char* pszKey;
        char* pszNamespace = 0;
        char* pszValue = 0;
        char pszLine[(UTOPIA_KEY_NS_SIZE * 2) + UTOPIA_BUF_SIZE + 4] = {'\0'};

        strncpy(pszLine, pszBuffer, (size_t)(pszEnd - pszBuffer));

        /* Parse the namespace */
        if ((pszKey = strchr(pszLine, ':')) != 0 && *(pszKey + 1) == ':')
        {
            pszNamespace = pszLine;
            *pszKey = '\0';
            pszKey += 2;
        }
        else
        {
            pszKey = pszLine;
        }
        /* Parse the key and value */
        if ((pszValue = strchr(pszKey, '=')) != 0)
        {
            UtopiaValue ixUtopia = UtopiaValue__UNKNOWN__;

            *pszValue = '\0';
            pszValue = (fUnset == 1 ? 0 : pszValue + 1);

            if (UtopiaTransact_Set(pUtopiaCtx, ixUtopia, pszNamespace, pszKey, pszValue) == 0)
            {
                return 0;
            }
        }
        pszBuffer = pszEnd + 1;
    }
    return 1;
}

/* Utopia system config get key helper function */
static int s_UtopiaValue_GetKey(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2,
                                int iIndex1, int iIndex2, char* pszValue, unsigned int ccbBuf)
{
    char pszKey[UTOPIA_KEY_NS_SIZE] = {'\0'};
    char pszNS[UTOPIA_KEY_NS_SIZE] = {'\0'};
    errno_t rc = -1;

    if (pUtopiaCtx == 0 || pszValue == 0)
    {
        return 0;
    }

    /* Zero out the buffer */
    memset(pszValue, '\0', ccbBuf);

    /* Format the Utopia namespace/key */
    if (s_UtopiaValue_FormatNamespaceKey(pUtopiaCtx, ixUtopia, pszName1, pszName2, iIndex1, iIndex2,
                                         pszNS, sizeof(pszNS), pszKey, sizeof(pszKey)) == 0)
    {
        return 0;
    }

    if (((*pszNS == '\0' ? 0 : strlen(pszNS) + 2) + strlen(pszKey) + 1) > ccbBuf)
    {
        return 0;
    }

    if (*pszNS != '\0')
    {
        rc = sprintf_s(pszValue, ccbBuf, "%s::%s", pszNS, pszKey);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    else
    {
        rc = sprintf_s(pszValue, ccbBuf, "%s", pszKey);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
    }
    return 1;
}

/* Utopia system config get helper function */
static int s_UtopiaValue_Get(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2,
                             int iIndex1, int iIndex2, char* pszValue, unsigned int ccbBuf)
{
    char pszKey[UTOPIA_KEY_NS_SIZE] = {'\0'};
    char pszNS[UTOPIA_KEY_NS_SIZE] = {'\0'};
    char* pszNamespace = 0;

    if (pUtopiaCtx == 0 || pszValue == 0)
    {
        return 0;
    }

    /* Zero out the buffer */
    memset(pszValue, '\0', ccbBuf);

    /* Format the Utopia namespace/key */
    if (s_UtopiaValue_FormatNamespaceKey(pUtopiaCtx, ixUtopia, pszName1, pszName2, iIndex1, iIndex2,
                                         pszNS, sizeof(pszNS), pszKey, sizeof(pszKey)) == 0)
    {
        return 0;
    }

    /* If the namespace buffer is empty set to null */
    if (*pszNS != '\0')
    {
        pszNamespace = pszNS;
    }

    return UtopiaTransact_Get(pUtopiaCtx, ixUtopia, pszNamespace, pszKey, pszValue, ccbBuf);
}

/* Utopia system config set helper function */
static int s_UtopiaValue_Set(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2,
                             int iIndex1, int iIndex2, char* pszValue)
{
    char pszKey[UTOPIA_KEY_NS_SIZE] = {'\0'};
    char pszNS[UTOPIA_KEY_NS_SIZE] = {'\0'};
    char* pszNamespace = 0;

    if (pUtopiaCtx == 0)
    {
        return 0;
    }

    /* Return an error if this value does not allow setting */
    if (Utopia_SetAllowed(ixUtopia) == 0)
    {
        return 0;
    }

    /* Format the Utopia namespace/key */
    if (s_UtopiaValue_FormatNamespaceKey(pUtopiaCtx, ixUtopia, pszName1, pszName2, iIndex1, iIndex2,
                                         pszNS, sizeof(pszNS), pszKey, sizeof(pszKey)) == 0)
    {
        return 0;
    }

    /* If the namespace buffer is empty set to null */
    if (*pszNS != '\0')
    {
        pszNamespace = pszNS;
    }

    return UtopiaTransact_Set(pUtopiaCtx, ixUtopia, pszNamespace, pszKey, pszValue);
}


/*
 * utctx_api.h
 */

int Utopia_GetKey(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsConfig(ixUtopia) || Utopia_IsEvent(ixUtopia))
    {
        return s_UtopiaValue_GetKey(pUtopiaCtx, ixUtopia, 0, 0, -1, -1, pszBuffer, ccbBuf);
    }
    return 0;
}

int Utopia_GetIndexedKey(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsIndexedConfig(ixUtopia))
    {
        return s_UtopiaValue_GetKey(pUtopiaCtx, ixUtopia, 0, 0, iIndex, -1, pszBuffer, ccbBuf);
    }
    return 0;
}

int Utopia_GetIndexed2Key(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex1, int iIndex2, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsIndexed2Config(ixUtopia))
    {
        return s_UtopiaValue_GetKey(pUtopiaCtx, ixUtopia, 0, 0, iIndex1, iIndex2, pszBuffer, ccbBuf);
    }
    return 0;
}

int Utopia_GetNamedKey(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsNamedConfig(ixUtopia))
    {
        return s_UtopiaValue_GetKey(pUtopiaCtx, ixUtopia, pszName, 0, -1, -1, pszBuffer, ccbBuf);
    }
    return 0;
}

int Utopia_GetNamed2Key(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsNamed2Config(ixUtopia))
    {
        return s_UtopiaValue_GetKey(pUtopiaCtx, ixUtopia, pszName1, pszName2, -1, -1, pszBuffer, ccbBuf);
    }
    return 0;
}

int Utopia_Get(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszBuffer, unsigned int ccbBuf)
{
    if ((Utopia_IsConfig(ixUtopia) || Utopia_IsEvent(ixUtopia)) &&
        s_UtopiaValue_Get(pUtopiaCtx, ixUtopia, 0, 0, -1, -1, pszBuffer, ccbBuf) != 0)
    {
        return 1;
    }
    else if (Utopia_IsStatic(ixUtopia))
    {
        strncpy(pszBuffer, g_Utopia_Statics[g_Utopia[(int)ixUtopia].ixStatic], ccbBuf - 1);
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_GetAll(UtopiaContext* pUtopiaCtx, char* pszBuffer, unsigned int ccbBuf)
{
    if (s_UtopiaTransact_GetAll(pUtopiaCtx, pszBuffer, ccbBuf) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR1("%s: Failed\n", __FUNCTION__);
    return 0;
}

int Utopia_GetIndexed(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsIndexedConfig(ixUtopia) &&
        s_UtopiaValue_Get(pUtopiaCtx, ixUtopia, 0, 0, iIndex, -1, pszBuffer, ccbBuf))
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_GetIndexed2(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex1, int iIndex2, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsIndexed2Config(ixUtopia) &&
        s_UtopiaValue_Get(pUtopiaCtx, ixUtopia, 0, 0, iIndex1, iIndex2, pszBuffer, ccbBuf))
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_GetNamed(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsNamedConfig(ixUtopia) &&
        s_UtopiaValue_Get(pUtopiaCtx, ixUtopia, pszName, 0, -1, -1, pszBuffer, ccbBuf) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_GetNamed2(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2, char* pszBuffer, unsigned int ccbBuf)
{
    if (Utopia_IsNamed2Config(ixUtopia) &&
        s_UtopiaValue_Get(pUtopiaCtx, ixUtopia, pszName1, pszName2, -1, -1, pszBuffer, ccbBuf) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_RawGet(UtopiaContext* pUtopiaCtx, char* pszNamespace, char* pszKey, char* pszBuffer, unsigned int ccbBuf)
{
    return UtopiaTransact_Get(pUtopiaCtx, UtopiaValue__UNKNOWN__, pszNamespace, pszKey, pszBuffer, ccbBuf);
}

int Utopia_RawSet(UtopiaContext* pUtopiaCtx, char* pszNamespace, char* pszKey, char* pszValue)
{
    return UtopiaTransact_Set(pUtopiaCtx, UtopiaValue__UNKNOWN__, pszNamespace, pszKey, pszValue);
}

int Utopia_Set(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszValue)
{
    if ((Utopia_IsConfig(ixUtopia) || Utopia_IsEvent(ixUtopia)) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, 0, 0, -1, -1, pszValue) !=0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_SetAll(UtopiaContext* pUtopiaCtx, char* pszBuffer)
{
    char pszState[UTOPIA_STATE_SIZE];

    /* First need to do a get all and unset each value */
    if (s_UtopiaTransact_GetAll(pUtopiaCtx, pszState, sizeof(pszState)) != 0 &&
        s_UtopiaTransact_SetAll(pUtopiaCtx, pszState, 1) != 0 &&
        /* Now, we can do a set all, if the buffer's not null */
        (pszBuffer == 0 ||
         s_UtopiaTransact_SetAll(pUtopiaCtx, pszBuffer, 0) != 0))
    {
        return 1;
    }
    UTCTX_LOG_ERR1("%s: Failed\n", __FUNCTION__);
    return 0;
}

int Utopia_SetIndexed(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex, char* pszValue)
{
    if (Utopia_IsIndexedConfig(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, 0, 0, iIndex, -1, pszValue) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_SetIndexed2(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex1, int iIndex2, char* pszValue)
{
    if (Utopia_IsIndexed2Config(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, 0, 0, iIndex1, iIndex2, pszValue) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_SetNamed(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName, char* pszValue)
{
    if (Utopia_IsNamedConfig(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, pszName, 0, -1, -1, pszValue) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_SetNamed2(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2, char* pszValue)
{
    if (Utopia_IsNamed2Config(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, pszName1, pszName2, -1, -1, pszValue) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_Unset(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia)
{
    if ((Utopia_IsConfig(ixUtopia) || Utopia_IsEvent(ixUtopia)) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, 0, 0, -1, -1, 0) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_UnsetIndexed(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex)
{
    if (Utopia_IsIndexedConfig(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, 0, 0, iIndex, -1, 0) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_UnsetIndexed2(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, int iIndex1, int iIndex2)
{
    if (Utopia_IsIndexed2Config(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, 0, 0, iIndex1, iIndex2, 0) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_UnsetNamed(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName)
{
    if (Utopia_IsNamedConfig(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, pszName, 0, -1, -1, 0) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_UnsetNamed2(UtopiaContext* pUtopiaCtx, UtopiaValue ixUtopia, char* pszName1, char* pszName2)
{
    if (Utopia_IsNamed2Config(ixUtopia) &&
        s_UtopiaValue_Set(pUtopiaCtx, ixUtopia, pszName1, pszName2, -1, -1, 0) != 0)
    {
        return 1;
    }
    UTCTX_LOG_ERR2("%s: Failed on index %d\n", __FUNCTION__, (int)ixUtopia);
    return 0;
}

int Utopia_SetEvent(UtopiaContext* pUtopiaCtx, Utopia_Event event)
{
    if (pUtopiaCtx)
    {
        pUtopiaCtx->bfEvents |= event;
        return 1;
    }
    else
    {
        UTCTX_LOG_ERR2("%s: Failed event %d\n", __FUNCTION__, (int)event);
        return 0;
    }
}


/*
 * utctx.h
 */

int Utopia_Init(UtopiaContext* pUtopiaCtx)
{
    UTCTX_LOG_INIT();

    UTCTX_LOG_DBG1("%s: Initializing\n", __FUNCTION__);

    pUtopiaCtx->bfEvents = Utopia_Event__NONE__;
    pUtopiaCtx->iEventHandle = 0;
    pUtopiaCtx->uiEventToken = 0;
    pUtopiaCtx->pHead = 0;

    /* Initialize the UtopiaRWLock and syscfg system */
    return (UtopiaRWLock_Init(&pUtopiaCtx->rwLock) && SysCfg_Init());
}

void Utopia_Free(UtopiaContext* pUtopiaCtx, int fCommit)
{
    UtopiaTransact_Node* pNode;
    UtopiaTransact_Node* pNext;
    int fValuesSet = 0;

    UTCTX_LOG_DBG1("%s: Freeing\n", __FUNCTION__);

    if (fCommit)
    {
        /* For anything that will disrupt the lan, we need to immediately set the lan-restarting flag */
        if (Utopia_EventSet(pUtopiaCtx->bfEvents, Utopia_Event_Reboot) ||
            Utopia_EventSet(pUtopiaCtx->bfEvents, Utopia_Event_LAN_Restart) ||
            Utopia_EventSet(pUtopiaCtx->bfEvents, Utopia_Event_WLAN_Restart))
        {
            /* Initialize the event handle if it's not yet been happened */
            if (pUtopiaCtx->iEventHandle == 0)
            {
                pUtopiaCtx->iEventHandle = SysEvent_Open(UTCTX_EVENT_ADDRESS, UTCTX_EVENT_PORT,
                                                         UTCTX_EVENT_VERSION, UTCTX_EVENT_NAME,
                                                         (token_t *)&pUtopiaCtx->uiEventToken);
		/* CID 58554 : Improper use of negative value */
		if (pUtopiaCtx->iEventHandle < 0)
		{
		    UTCTX_LOG_DBG1("%s: EventHandle can't be negative\n",__FUNCTION__);
	            return;
	        }
            }

            SysEvent_Trigger(pUtopiaCtx->iEventHandle, pUtopiaCtx->uiEventToken, Utopia_ToKey(UtopiaValue_LAN_Restarting), 0);
        }
    }

    /* Set and free the transact list */
    pNode = pUtopiaCtx->pHead;
    while (pNode)
    {
        /* Only set values if we did't have an error */
        if (fCommit)
        {
            fValuesSet = 1;

            if (pNode->pszValue == 0)
            {
                SysCfg_Unset(pNode->pszNamespace, pNode->pszKey);
            }
            else
            {
                if (Utopia_IsEvent(pNode->ixUtopia))
                {
                    s_UtopiaEvent_Set(pUtopiaCtx, pNode->pszKey, pNode->pszValue);
                }
                else
                {
                    SysCfg_Set(pNode->pszNamespace, pNode->pszKey, pNode->pszValue);
                }
            }
        }

        /* Free */
        pNext = pNode->pNext;
        if (pNode->pszNamespace)
        {
            free(pNode->pszNamespace);
        }
        if (pNode->pszKey)
        {
            free(pNode->pszKey);
        }
        if (pNode->pszValue)
        {
            free(pNode->pszValue);
        }
        free(pNode);
        pNode = pNext;
    }

    /* Commit the syscfg values if any where set */
    if (fValuesSet)
    {
        SysCfg_Commit();
    }
    else
    {
        SysCfg_Free();
    }

    /* Free up the UtopiaRWLock */
    UtopiaRWLock_Free(&pUtopiaCtx->rwLock);

    /* Trigger events if we didn't have an error */
    if (fCommit)
    {
        s_UtopiaEvent_Trigger(pUtopiaCtx);
    }

    /* Close the SysEvent handle if we need to */
    if (pUtopiaCtx->iEventHandle)
    {
        SysEvent_Close(pUtopiaCtx->iEventHandle, pUtopiaCtx->uiEventToken);
    }
}


