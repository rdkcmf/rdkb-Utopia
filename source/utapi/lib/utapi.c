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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/reboot.h>
#include <syscfg/syscfg.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_product.h"
#include "utapi_util.h"
#include "utapi_wlan.h"
#include "DM_TR181.h"
#include "secure_wrapper.h"
#define UTOPIA_TR181_PARAM_SIZE 64
#define UTOPIA_TR181_PARAM_SIZE1        256
#define UTOPIA_TR181_PARAM_SIZE2        1024

#define CRC32_TABLE_SIZE 256
#define POLYNOMIAL 0xEDB88320L
#define CALCULATE_CRC32_TABLE_ENTRY(X) (((X) & 1) ? (POLYNOMIAL^ ((X) >> 1)) : ((X) >> 1))

/*
 * utapi.c -
 */

/* WanType table */
static EnumString_Map g_WanTypeMap[] =
{
    { "dhcp", DHCP },
    { "static", STATIC },
    { "pppoe", PPPOE },
    { "pptp", PPTP },
    { "l2tp", L2TP },
    { "telstra", TELSTRA },
    { 0, 0 }
};

static EnumString_Map g_WanConnMap[] =
{
    { "redial", KEEP_ALIVE },
    { "demand", CONNECT_ON_DEMAND },
    { 0, 0 }
};

static EnumString_Map g_ProtocolMap[] =
{
    { "tcp", TCP },
    { "udp", UDP },
    { "both", BOTH_TCP_UDP },
    { 0, 0 }
};

static EnumString_Map g_QoSClassMap[] = 
{
    { "$HIGH",   QOS_PRIORITY_HIGH   },
    { "$MEDIUM", QOS_PRIORITY_MEDIUM },
    { "$NORMAL", QOS_PRIORITY_NORMAL },
    { "$LOW",    QOS_PRIORITY_LOW    },
    { 0, 0 }
};

static EnumString_Map g_QoSCustomTypeMap[] = 
{
    { "app",  QOS_CUSTOM_APP  },
    { "game", QOS_CUSTOM_GAME },
    { 0, 0 }
};

static EnumString_Map g_DDNSProviderMap[] =
{
    { "dyndns",           DDNS_DYNDNS },
    { "tzo",              DDNS_TZO           },
    { "dyndns-static",    DDNS_DYNDNS_STATIC },
    { "dyndns-custom",    DDNS_DYNDNS_CUSTOM },
    { "ezip",             DDNS_EZIP   },
    { "pgpow",            DDNS_PGPOW  },
    { "dhs",              DDNS_DHS    },
    { "ods",              DDNS_ODS           },
    { "easydns",          DDNS_EASYDNS       },
    { "easydns-partner",  DDNS_EASYDNS_PARTNER},
    { "gnudip",           DDNS_GNUDIP},
    { "justlinux",        DDNS_JUSTLINUX},
    { "dyns",             DDNS_DYNS},
    { "hn",               DDNS_HN},
    { "zoneedit",         DDNS_ZONEEDIT},
    { "heipv6tb",         DDNS_HEIPV6TB},
    { 0, 0 }
};

static EnumString_Map g_DDNSStatus[] =
{
    { "success",       DDNS_STATUS_SUCCESS },
    { "error-connect", DDNS_STATUS_FAILED_CONNECT },
    { "error-auth",    DDNS_STATUS_FAILED_AUTH },
    { "error",         DDNS_STATUS_FAILED },
    { 0, 0 }
};


const char *g_NetworkServices[] = 

    { "ftp", 
      "telnet",
      "ssh",
      "sftp",
      "domain",
      "finger",
      "tftp",
      "www",
      "http-alt",
      "https",
      "pop3",
      "smtp",
      "nntp",
      "ntp",
      "nameserver",
      "netstat",
      "netbios-nm",
      "sunrpc",
      "snmp",
      "irc",
      "imap2",
      "ldap",
      "isakmp",
      "rtsp",
      0 };

// Based on the timezone database at:
// http://www.di-mgt.com.au/src/wclocktz-20090424T1313.zip
static timezone_info_t tz_list[] = {
    {12,    FALSE,  "MHT12",                                        "MHT12"},
    {11,    FALSE,  "WST11",                                        "WST11"},
    {10,    FALSE,  "HST10",                                        "HST10"},
    {9,     TRUE,   "AKST9AKDT,M3.2.0/02:00,M11.1.0/02:00",         "AKST9"},
    {8,     TRUE,   "PST8PDT,M3.2.0/02:00,M11.1.0/02:00",           "PST8"},
    {7,     FALSE,  "MST7",                                         "MST7"},
    {7,     TRUE,   "MST7MDT,M3.2.0/02:00,M11.1.0/02:00",           "MST7"},
    {6,     FALSE,  "CST6",                                         "CST6"},
    {6,     TRUE,   "CST6CDT,M3.2.0/02:00,M11.1.0/02:00",           "CST6"},
    {5,     FALSE,  "EST5",                                         "EST5"},
    {5,     TRUE,   "EST5EDT,M3.2.0/02:00,M11.1.0/02:00",           "EST5"},
    {4,     FALSE,  "VET4",                                         "VET4"},
    {4,     TRUE,   "CLT4CLST,M10.2.6/04:00,M3.2.6/03:00",          "CLT4"},
    {3.5,   TRUE,   "NST03:30NDT,M3.2.0/0:01,M11.1.0/0:01",         "NST03:30"},
    {3,     FALSE,  "ART3",                                         "ART3"},
    {3,     TRUE,   "BRT3BRST,M10.3.0/0:01,M2.3.0/0:01",            "BRT3"},
    {2,     FALSE,  "MAT2",                                         "MAT2"},
    {1,     TRUE,   "AZOT1AZOST,M3.5.0/00:00,M10.5.0/01:00",        "AZOT1"},
    {0,     FALSE,  "GMT0",                                         "GMT0"},
    {0,     TRUE,   "GMT0BST,M3.5.0/01:00,M10.5.0/02:00",           "GMT0"},
    {-1,    FALSE,  "CET-1",                                        "CET-1"},
    {-1,    TRUE,   "CET-1CEST,M3.5.0/02:00,M10.5.0/03:00",         "CET-1"},
    {-2,    FALSE,  "SAST-2",                                       "SAST-2"},
    {-2,    TRUE,   "EET-2EEST,M3.5.0/03:00,M10.5.0/04:00",         "EET-2"},
    {-3,    FALSE,  "AST-3",                                        "AST-3"},
    {-4,    FALSE,  "GST-4",                                        "GST-4"},
    {-5,    FALSE,  "PKT-5",                                        "PKT-5"},
    {-5.5,  FALSE,  "IST-05:30",                                    "IST-05:30"},
    {-6,    FALSE,  "ALMT-6",                                       "ALMT-6"},
    {-7,    FALSE,  "ICT-7",                                        "ICT-7"},
    {-8,    FALSE,  "HKT-8",                                        "HKT-8"},
    {-8,    FALSE,  "SGT-8",                                        "SGT-8"},
    {-9,    FALSE,  "JST-9",                                        "JST-9"},
    {-10,   FALSE,  "GST-10",                                       "GST-10"},
    {-10,   TRUE,   "AEST-10AEDT-11,M10.1.0/00:00,M4.1.0/00:00",    "AEST-10"},
    {-11,   FALSE,  "SBT-11",                                       "SBT-11"},
    {-12,   FALSE,  "FJT-12",                                       "FJT-12"},
    {-12,   TRUE,   "NZST-12NZDT,M9.5.0/02:00,M4.1.0/03:00",        "NZST-12"},
    {0,     0,      NULL,                                           NULL}
};


/* 
   PRIMARY_PROVIDER_RESTRICTED:This means the Cable provider is not providing any internet and not allowing you to get internet from 3rd party and will give you a standard wall gardened page saying you need to call customer service for internet service
   PRIMARY_PROVIDER_HSD: This means you are served with internet by your primary(cable service) provider
   BYOI: This means you are free to use the Router as you wish and you can connect to a 3rd party internet service provider.
*/
static EnumString_Map g_byoi_Mode[] =
{
    { "docsis",PRIMARY_PROVIDER_HSD },  
    { "non-docsis", PRIMARY_PROVIDER_RESTRICTED },
    { "user-selectable",USER_SELECTABLE},
    { 0, 0 }
};

static EnumString_Map g_hsdStatus[] =
{
    { "primary", CABLE_PROVIDER_HSD},
    { "byoi",    BYOI_PROVIDER_HSD },
    { "none",    NONE },
    { 0, 0 }
};

/*
static EnumString_Map g_byoi_WanTypeMap[] =

{
    { "dhcp", DHCP },
    { "pppoe", PPPOE },
    { 0, 0 }
};

*/
/*
 * ------------------------------------------------------------------------------ 
 */

/*
 * Device Settings
 */
int Utopia_GetDeviceSettings (UtopiaContext *ctx, deviceSetting_t *device)
{
    if (NULL == ctx || NULL == device) {
        return ERR_INVALID_ARGS;
    }

    bzero(device, sizeof(deviceSetting_t));

    Utopia_Get(ctx, UtopiaValue_HostName, device->hostname, IPHOSTNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_Locale, device->lang, LANG_SZ);
    Utopia_GetInt(ctx, UtopiaValue_AutoDST, (int *)&device->auto_dst);
    Utopia_Get(ctx, UtopiaValue_TZ, s_tokenbuf, TOKEN_SZ);

    timezone_info_t *tz;

    for (tz = tz_list; tz->dst_on != NULL && tz->dst_off != NULL; tz++) {
        if ((tz->is_dst_observed
             && device->auto_dst == AUTO_DST_OFF
             && !strcmp(tz->dst_off, s_tokenbuf))
            ||
            (tz->is_dst_observed
             && device->auto_dst == AUTO_DST_ON
             && !strcmp(tz->dst_on, s_tokenbuf))
            ||
            (!tz->is_dst_observed
             && device->auto_dst == AUTO_DST_NA
             && !strcmp(tz->dst_off, s_tokenbuf))
           )
        {
            device->tz_gmt_offset = tz->gmt_offset;
            break;
        }
    }

    return SUCCESS;
}


int Utopia_SetDeviceSettings (UtopiaContext *ctx, deviceSetting_t *device)
{
    if (NULL == ctx || NULL == device) {
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SET(ctx, UtopiaValue_HostName, device->hostname);
    UTOPIA_SET(ctx, UtopiaValue_Locale, device->lang);
    
    timezone_info_t *tz;

    for (tz = tz_list; tz->dst_on != NULL && tz->dst_off != NULL; tz++) {
        if (tz->gmt_offset == device->tz_gmt_offset) {
            if (device->auto_dst == AUTO_DST_OFF && tz->is_dst_observed) {
                UTOPIA_SET(ctx, UtopiaValue_TZ, tz->dst_off);
                UTOPIA_SETINT(ctx, UtopiaValue_AutoDST, device->auto_dst);
                break;
            }
            else if (device->auto_dst == AUTO_DST_ON && tz->is_dst_observed) {
                UTOPIA_SET(ctx, UtopiaValue_TZ, tz->dst_on);
                UTOPIA_SETINT(ctx, UtopiaValue_AutoDST, device->auto_dst);
                break;
            }
            else if (device->auto_dst == AUTO_DST_NA && !tz->is_dst_observed) {
                UTOPIA_SET(ctx, UtopiaValue_TZ, tz->dst_off);
                UTOPIA_SETINT(ctx, UtopiaValue_AutoDST, device->auto_dst);
                break;
            }
        }
    }

    return SUCCESS;
}

int Utopia_GetDeviceInfo (UtopiaContext *ctx, deviceInfo_t *info)
{
    if (NULL == ctx || NULL == info) {
        return ERR_INVALID_ARGS;
    }

    bzero(info, sizeof(deviceInfo_t));

    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    sysevent_get(se_fd, se_token, "firmware_version", info->firmware_version, TOKEN_SZ);
    sysevent_get(se_fd, se_token, "model_name", info->model_name, NAME_SZ);

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    ctime_r(&cur_time.tv_sec, info->current_time);

    char     ifname[IFNAME_SZ];
    sysevent_get(se_fd, se_token, "wan_ifname", ifname, IFNAME_SZ);
    s_get_interface_mac(ifname, info->wan_mac_address, MACADDR_SZ);

    FILE *fp;
    if ((fp = fopen(FIRMWARE_REVISION_FILE, "r"))) {
        fgets(info->firmware_revision, NAME_SZ, fp);
        fclose(fp);
    }
    if ((fp = fopen(FIRMWARE_BUILDDATE_FILE, "r"))) {
        fgets(info->firmware_builddate, NAME_SZ, fp);
        fclose(fp);
    }
    // TODO: wan domain 
    
    return SUCCESS; 
}

void get_dhcp_wan_domain(unsigned char *pDomain)
{
    FILE *f;
    char *pos;
    
    pDomain[0] = 0;
    f = popen("sysevent get dhcp_domain","r");
    if(f==NULL){
        return;
    }
    fgets(pDomain,IPHOSTNAME_SZ,f);
    /* remove trailing newline */
    if((pos = strrchr(pDomain, '\n')) != NULL)
        *pos = '\0';
    pclose(f);
}

/*
 * LAN Settings
 */
int Utopia_GetLanSettings (UtopiaContext *ctx, lanSetting_t *lan)
{
    if (NULL == ctx || NULL == lan) {
        return ERR_INVALID_ARGS;
    }

    bzero(lan, sizeof(lanSetting_t));

    UTOPIA_GET(ctx, UtopiaValue_LAN_IPAddr, lan->ipaddr, IPADDR_SZ);
    UTOPIA_GET(ctx, UtopiaValue_LAN_Netmask, lan->netmask, IPADDR_SZ);
    /*just for USGv2*/
    //get_dhcp_wan_domain(lan->domain);
   // if(lan->domain[0]==0)
    Utopia_Get(ctx, UtopiaValue_LAN_Domain, lan->domain, IPHOSTNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_LAN_IfName, lan->ifname, IFNAME_SZ);
    s_get_interface_mac(lan->ifname, lan->macaddr, MACADDR_SZ);

    return SUCCESS;
}

int Utopia_SetLanSettings (UtopiaContext *ctx, lanSetting_t *lan)
{
    if (NULL == ctx || NULL == lan) {
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SETIP(ctx, UtopiaValue_LAN_IPAddr, lan->ipaddr);
    UTOPIA_VALIDATE_SET(ctx, UtopiaValue_LAN_Netmask, lan->netmask, IsValid_Netmask, ERR_INVALID_NETMASK);
    UTOPIA_SET(ctx, UtopiaValue_LAN_Domain, lan->domain);

    return SUCCESS;
}

int Utopia_SetDHCPServerSettings (UtopiaContext *ctx, dhcpServerInfo_t *dhcps)
{
    if (NULL == ctx || NULL == dhcps) {
        return ERR_INVALID_ARGS;
    }

    // TODO: validate max users based on netmask
    
    snprintf(s_intbuf, sizeof(s_intbuf), "%d", dhcps->enabled);
    UTOPIA_SETBOOL(ctx, UtopiaValue_DHCP_ServerEnabled, dhcps->enabled);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_Start, dhcps->DHCPIPAddressStart);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_End, dhcps->DHCPIPAddressEnd);
    snprintf(s_intbuf, sizeof(s_intbuf), "%d", dhcps->DHCPMaxUsers);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_Num, s_intbuf);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_LeaseTime, dhcps->DHCPClientLeaseTime);
    UTOPIA_SETBOOL(ctx, UtopiaValue_DHCP_Nameserver_Enabled, dhcps->StaticNameServerEnabled);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_Nameserver1, dhcps->StaticNameServer1);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_Nameserver2, dhcps->StaticNameServer2);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_Nameserver3, dhcps->StaticNameServer3);
    UTOPIA_SET(ctx, UtopiaValue_DHCP_WinsServer, dhcps->WINSServer);

    return SUCCESS;
}

int Utopia_GetDHCPServerSettings (UtopiaContext *ctx, dhcpServerInfo_t *dhcps)
{
    if (NULL == ctx || NULL == dhcps) {
        return ERR_INVALID_ARGS;
    }

    UTOPIA_GETBOOL(ctx, UtopiaValue_DHCP_ServerEnabled, &dhcps->enabled);
    UTOPIA_GET(ctx, UtopiaValue_DHCP_Start, dhcps->DHCPIPAddressStart, IPADDR_SZ);
    UTOPIA_GET(ctx, UtopiaValue_DHCP_End, dhcps->DHCPIPAddressEnd, IPADDR_SZ);
    UTOPIA_GETINT(ctx, UtopiaValue_DHCP_Num, &dhcps->DHCPMaxUsers);
    UTOPIA_GET(ctx, UtopiaValue_DHCP_LeaseTime, dhcps->DHCPClientLeaseTime, TOKEN_SZ);
    UTOPIA_GETBOOL(ctx, UtopiaValue_DHCP_Nameserver_Enabled, &dhcps->StaticNameServerEnabled);
    Utopia_Get(ctx, UtopiaValue_DHCP_Nameserver1, dhcps->StaticNameServer1, IPHOSTNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_DHCP_Nameserver2, dhcps->StaticNameServer2, IPHOSTNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_DHCP_Nameserver3, dhcps->StaticNameServer3, IPHOSTNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_DHCP_WinsServer, dhcps->WINSServer, IPHOSTNAME_SZ);

    return SUCCESS;
}

#define MAX_ITERATION 200
#define DELIM_CHAR ','

int Utopia_UnsetDHCPServerStaticHosts (UtopiaContext *ctx)
{
    int count = 0;

    if (NULL == ctx) {
        return ERR_INVALID_ARGS;
    }

    Utopia_GetInt(ctx, UtopiaValue_DHCP_NumStaticHosts, &count);
    if (count > 0) {
        int i;
        for (i = 0; i < count; i++) {
            Utopia_UnsetIndexed(ctx, UtopiaValue_DHCP_StaticHost, i + 1);
        }
        UTOPIA_SETINT(ctx, UtopiaValue_DHCP_NumStaticHosts, 0);
    }

    return SUCCESS;
}


int Utopia_SetDHCPServerStaticHosts (UtopiaContext *ctx, int count, DHCPMap_t *dhcpMap)
{
    int i;
    char tbuf[512];

    if (NULL == ctx) {
        return ERR_INVALID_ARGS;
    }

    Utopia_UnsetDHCPServerStaticHosts(ctx);

    for (i = 0; i < count; i++) {
        if (!IsValid_MACAddr(dhcpMap[i].macaddr)  ||
            //!IsValid_IPAddrLastOctet(dhcpMap[i].host_ip)) {
            !IsValid_IPAddr(dhcpMap[i].host_ip)){
            return ERR_INVALID_VALUE;
        }
        /*snprintf(tbuf, sizeof(tbuf), "%s%c%d%c%s", dhcpMap[i].macaddr, DELIM_CHAR,
                                                   dhcpMap[i].host_ip, DELIM_CHAR,
                                                   dhcpMap[i].client_name);*/
        snprintf(tbuf, sizeof(tbuf), "%s%c%s%c%s", dhcpMap[i].macaddr, DELIM_CHAR,
                                                   dhcpMap[i].host_ip, DELIM_CHAR,
                                                   dhcpMap[i].client_name);
        UTOPIA_SETINDEXED(ctx, UtopiaValue_DHCP_StaticHost, i + 1, tbuf);
    }
    UTOPIA_SETINT(ctx, UtopiaValue_DHCP_NumStaticHosts, count);

    return SUCCESS;
}

int Utopia_GetDHCPServerStaticHostsCount (UtopiaContext *ctx, int *count)
{
    *count = 0;
    Utopia_GetInt(ctx, UtopiaValue_DHCP_NumStaticHosts, count);
    return SUCCESS;
}

/*
 * Caller should free dhcpMap
 * Caller-need-to-free
 * dhcp_static_host_x must be
 *          either none (for a hole in the array)
 *          or mac_address,last_octet_of_ip_address,host_name
 *          eg. 11:22:33:44:55:66,99,Bob's mac
 */
int Utopia_GetDHCPServerStaticHosts (UtopiaContext *ctx, int *count, DHCPMap_t **dhcpMap)
{
    char *p, *n, tbuf[512];

    if (NULL == ctx || NULL == count || NULL == dhcpMap) {
        return ERR_INVALID_ARGS;
    }

    Utopia_GetInt(ctx, UtopiaValue_DHCP_NumStaticHosts, count);
    if (*count > 0) {
        *dhcpMap = (DHCPMap_t *) malloc(sizeof(DHCPMap_t) * (*count));
        if (*dhcpMap) {
            int ct, i;
            bzero(*dhcpMap, sizeof(DHCPMap_t) * (*count));
            for (ct = 0, i = 0; ct < (*count) && (i < MAX_ITERATION) ; i++) {
                if (Utopia_GetIndexed(ctx, UtopiaValue_DHCP_StaticHost, i + 1, tbuf, sizeof(tbuf))) {
                    ct++;
                    p = tbuf;
                    if (NULL == (n = chop_str(p, ','))) {
                        continue;
                    }
                    strncpy((*dhcpMap)[i].macaddr, p, MACADDR_SZ);

                    p = n;
                    if (NULL == (n = chop_str(p, ','))) {
                        continue;
                    }
                    /*
                    if (FALSE == IsInteger(p)) {
                        // log err?
                        continue; 
                    }
                    (*dhcpMap)[i].host_ip = atoi(p);
                    */
                    strncpy((*dhcpMap)[i].host_ip, p, IPADDR_SZ);
                      
                    p = n;
                    strncpy((*dhcpMap)[i].client_name, p, TOKEN_SZ);
                }
            }
        } else {
            return ERR_INSUFFICIENT_MEM;
        }
    }

    return SUCCESS;
}

#define DHCP_SERVER_LEASE_FILE "/tmp/dnsmasq.leases"

static void s_parse_dhcp_lease (char *line, dhcpLANHost_t *client_info)
{
    char lease[64];

    sscanf(line, "%s %s %s %s %s", 
           lease,
           client_info->macaddr,
           client_info->ipaddr,
           client_info->hostname,
           client_info->client_id);
    client_info->leasetime = atol(lease);
    client_info->lan_interface = LAN_INTERFACE_WIRED; // TODO
}

int Utopia_GetDHCPServerLANHosts (UtopiaContext *ctx, int *count, dhcpLANHost_t **client_info)
{
    char line[512];
    int ct = 0;

    // Unused
    (void) ctx;

    *client_info = NULL;

    dhcpLANHost_t *lan_hosts = NULL;

    FILE *fp = fopen(DHCP_SERVER_LEASE_FILE, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
           lan_hosts = (dhcpLANHost_t *) realloc(lan_hosts, sizeof(dhcpLANHost_t) * (ct+1));
           if (NULL == lan_hosts) {
               fclose(fp); /*RDKB-7128, CID-33073, free unused resources before exit */
               return ERR_INSUFFICIENT_MEM;
           }
           s_parse_dhcp_lease(line, &lan_hosts[ct]);
           ct++;
        }
        fclose(fp);
    }

    *count = ct;
    *client_info = lan_hosts;

    return SUCCESS;
}

int Utopia_DeleteDHCPServerLANHost (char *ipaddr)
{
    v_secure_system("/etc/init.d/service_dhcp_server.sh delete_lease %s", ipaddr);
    
    token_t se_token;
    int se_fd = s_sysevent_connect(&se_token);
    
    if (0 > se_fd) {
        printf("Unable to register with sysevent daemon\n");
        return ERR_SYSEVENT_CONN;
    }
    
    sysevent_set(se_fd, se_token, "dhcp_server-restart", "", 0);
    
    return SUCCESS;
}

#define ARP_CACHE_FILE "/tmp/.tmp_arp_cache"

static void s_parse_arp_cache (char *line, arpHost_t *host_info)
{
    char stub[64], is_static[32];

    /*
     * Sample line
     *   192.168.1.113 dev br0 lladdr 00:23:32:c8:28:d8 REACHABLE
     */
    sscanf(line, "%s %s %s %s %s %s", 
                 host_info->ipaddr,
                 stub,
                 host_info->interface,
                 stub,
                 host_info->macaddr,
                 is_static);
    host_info->is_static = ( 0 == strcmp(is_static, "PERMANENT")) ? TRUE : FALSE;
}

int Utopia_GetARPCacheEntries (UtopiaContext *ctx, int *count, arpHost_t **out_hosts)
{
    char line[512];
    int ct = 0;

    // Unused
    (void) ctx;

    *out_hosts = NULL;

    arpHost_t *hosts = NULL;

    unlink(ARP_CACHE_FILE);
    snprintf(line, sizeof(line), "ip neigh show > %s", ARP_CACHE_FILE);
    system(line);

    FILE *fp = fopen(ARP_CACHE_FILE, "r");
    if (fp) {
       
        while (fgets(line, sizeof(line), fp)) {
           hosts = (arpHost_t *) realloc(hosts, sizeof(arpHost_t) * (ct+1));
           if (NULL == hosts) {
               fclose(fp); /*RDKB-7128, CID-33193, free unused resources before exit */
               unlink(ARP_CACHE_FILE);
               return ERR_INSUFFICIENT_MEM;
           }
           s_parse_arp_cache(line, &hosts[ct]);
           ct++;
        }
        fclose(fp);
    }
    unlink(ARP_CACHE_FILE);

    *count = ct;
    *out_hosts = hosts;

    return UT_SUCCESS;
}

#define WLAN_CLIENTLIST_TMP "/tmp/.tmp_wlan_clients"
int Utopia_GetWLANClients (UtopiaContext *ctx, int *count, char **out_maclist)
{
    char line[512];
    int ct = 0;

    // Unused
    (void) ctx;

    *out_maclist = NULL;

    char *maclist = NULL;

    unlink(WLAN_CLIENTLIST_TMP);
    snprintf(line, sizeof(line), "wlancfg eth0 clientlist > %s", WLAN_CLIENTLIST_TMP);
    system(line);
    snprintf(line, sizeof(line), "wlancfg eth1 clientlist >> %s", WLAN_CLIENTLIST_TMP);
    system(line);

    FILE *fp = fopen(WLAN_CLIENTLIST_TMP, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
           maclist = (char *) realloc(maclist, MACADDR_SZ * (ct+1));
           if (NULL == maclist) {
               fclose(fp); /*RDKB-7128, CID-33326, free unused resources before exit*/
               unlink(WLAN_CLIENTLIST_TMP);
               return ERR_INSUFFICIENT_MEM;
           }
           char *p;
           if ((p = strrchr(line, '\n'))) {
               *p = '\0';
           }
           strncpy(maclist + (MACADDR_SZ *ct), line, MACADDR_SZ);
           ulogf(ULOG_CONFIG, UL_UTAPI, "%s: maclist[%d] = %s", __FUNCTION__, ct, maclist + (MACADDR_SZ * ct));
           ct++;
        }
        fclose(fp);
    }
    // unlink(WLAN_CLIENTLIST_TMP);

    *count = ct;
    *out_maclist = maclist;

    return UT_SUCCESS;
}


/*
 * WAN Settings
 */
static int s_SetWANStaticSetting (UtopiaContext *ctx, wan_static_t wstatic)
{
    UTOPIA_SETIP(ctx, UtopiaValue_WAN_IPAddr, wstatic.ip_addr);
    UTOPIA_VALIDATE_SET(ctx, UtopiaValue_WAN_Netmask, wstatic.subnet_mask, IsValid_Netmask, ERR_INVALID_NETMASK);
    UTOPIA_SETIP(ctx, UtopiaValue_WAN_DefaultGateway, wstatic.default_gw);
    UTOPIA_SETIP(ctx, UtopiaValue_WAN_NameServer1, wstatic.dns_ipaddr1);
    UTOPIA_SETIP(ctx, UtopiaValue_WAN_NameServer2, wstatic.dns_ipaddr2);
    UTOPIA_SETIP(ctx, UtopiaValue_WAN_NameServer3, wstatic.dns_ipaddr3);

    return SUCCESS;
}

static int s_SetWANPPPSetting (UtopiaContext *ctx, wanProto_t wan_proto, wanInfo_t *wan)
{
    int rc = SUCCESS;
    /*
     * PPP common settings
     */
    UTOPIA_SET(ctx, UtopiaValue_WAN_ProtoUsername, wan->ppp.username);
    UTOPIA_SET(ctx, UtopiaValue_WAN_ProtoPassword, wan->ppp.password);

    char *p = s_EnumToStr(g_WanConnMap, wan->ppp.conn_method);
    if (NULL == p) {
        return ERR_INVALID_VALUE;
    } 
    UTOPIA_SET(ctx, UtopiaValue_WAN_PPPConnMethod, p);
    UTOPIA_SETINT(ctx, UtopiaValue_WAN_PPPKeepAliveInterval, wan->ppp.redial_period);
    UTOPIA_SETINT(ctx, UtopiaValue_WAN_PPPIdleTime, wan->ppp.max_idle_time);
    UTOPIA_SETINT(ctx, UtopiaValue_WAN_Max_MRU, wan->ppp.max_mru);
    UTOPIA_SETBOOL(ctx, UtopiaValue_WAN_PPPIPCP, wan->ppp.ipcp);
    UTOPIA_SETBOOL(ctx, UtopiaValue_WAN_PPPIPV6CP, wan->ppp.ipv6cp);

    if (PPPOE == wan_proto) {
        UTOPIA_SET(ctx, UtopiaValue_WAN_PPPoEServiceName, wan->ppp.service_name);
    }

    if (PPTP == wan_proto || L2TP == wan_proto || TELSTRA == wan_proto) {
        UTOPIA_SET(ctx, UtopiaValue_WAN_ProtoServerAddress, wan->ppp.server_ipaddr);
        UTOPIA_SETBOOL(ctx, UtopiaValue_WAN_PPTPAddressStatic, wan->ppp.ipAddrStatic);
        UTOPIA_SETBOOL(ctx, UtopiaValue_WAN_L2TPAddressStatic, wan->ppp.ipAddrStatic);
        if (wan->ppp.ipAddrStatic) {
            rc = s_SetWANStaticSetting (ctx, wan->wstatic);
        }
    }
    return rc;
}

int Utopia_SetWANSettings (UtopiaContext *ctx, wanInfo_t *wan_info)
{
    int rc = SUCCESS;
    if (NULL == ctx || NULL == wan_info) {
        return ERR_INVALID_ARGS;
    }

    /*
     * WAN Procotol
     */

    char *p = s_EnumToStr(g_WanTypeMap, wan_info->wan_proto);
    if (NULL == p) {
        return ERR_INVALID_WAN_TYPE;
    }

      UTOPIA_SET(ctx, UtopiaValue_WAN_Proto, p);
    /*
     * MTU
     */
    if (TRUE == wan_info->auto_mtu) {
        UTOPIA_SETINT(ctx, UtopiaValue_WAN_MTU, 0);
    } else {
        UTOPIA_SETINT(ctx, UtopiaValue_WAN_MTU, wan_info->mtu_size);
    } 

    UTOPIA_SET(ctx, UtopiaValue_WAN_ProtoAuthDomain, wan_info->domainname);

    switch (wan_info->wan_proto) {
    case DHCP:
        break;
    case STATIC:
        rc = s_SetWANStaticSetting (ctx, wan_info->wstatic);
        break;
    case PPPOE:
    case PPTP:
    case L2TP:
    case TELSTRA:
        rc = s_SetWANPPPSetting (ctx, wan_info->wan_proto, wan_info);
        break;
    default:
        return ERR_INVALID_WAN_TYPE;
    }
    return rc;
}

static int s_GetWANStaticSetting (UtopiaContext *ctx, wan_static_t *wstatic)
{
    Utopia_Get(ctx, UtopiaValue_WAN_IPAddr, wstatic->ip_addr, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_Netmask, wstatic->subnet_mask, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_DefaultGateway, wstatic->default_gw, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_NameServer1, wstatic->dns_ipaddr1, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_NameServer2, wstatic->dns_ipaddr2, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_NameServer3, wstatic->dns_ipaddr3, IPADDR_SZ);

    return SUCCESS;
}

static int s_GetWANPPPSetting (UtopiaContext *ctx, wanProto_t wan_proto, wanInfo_t *wan)
{
    int rc = SUCCESS;

    /*
     * PPP common settings
     */
    Utopia_Get(ctx, UtopiaValue_WAN_ProtoUsername, wan->ppp.username, USERNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_ProtoPassword, wan->ppp.password, PASSWORD_SZ);
    Utopia_GetInt(ctx, UtopiaValue_WAN_Max_MRU, &wan->ppp.max_mru);
    Utopia_GetBool(ctx, UtopiaValue_WAN_PPPIPCP, &wan->ppp.ipcp);
    Utopia_GetBool(ctx, UtopiaValue_WAN_PPPIPV6CP, &wan->ppp.ipv6cp);
    Utopia_Get(ctx, UtopiaValue_WAN_PPPConnMethod, s_tokenbuf, sizeof(s_tokenbuf));
    wan->ppp.conn_method = s_StrToEnum(g_WanConnMap, s_tokenbuf);

    if (CONNECT_ON_DEMAND == wan->ppp.conn_method) {
        Utopia_GetInt(ctx, UtopiaValue_WAN_PPPIdleTime, &wan->ppp.max_idle_time);
    } else {
        Utopia_GetInt(ctx, UtopiaValue_WAN_PPPKeepAliveInterval, &wan->ppp.redial_period);
    }

    if (PPPOE == wan_proto) {
        Utopia_Get(ctx, UtopiaValue_WAN_PPPoEServiceName, wan->ppp.service_name, WAN_SERVICE_NAME_SZ);
    } else {
        Utopia_Get(ctx, UtopiaValue_WAN_ProtoServerAddress, wan->ppp.server_ipaddr, IPADDR_SZ);
    }

    if (PPTP == wan_proto) {
        Utopia_GetBool(ctx, UtopiaValue_WAN_PPTPAddressStatic, &wan->ppp.ipAddrStatic);
        if (wan->ppp.ipAddrStatic) {
            rc = s_GetWANStaticSetting (ctx, &wan->wstatic);
        }
    } else if (L2TP == wan_proto) {
        Utopia_GetBool(ctx, UtopiaValue_WAN_L2TPAddressStatic, &wan->ppp.ipAddrStatic);
        if (wan->ppp.ipAddrStatic) {
            rc = s_GetWANStaticSetting (ctx, &wan->wstatic);
        }
    }

    return rc;
}

int Utopia_GetWANSettings (UtopiaContext *ctx, wanInfo_t *wan_info)
{
    int rc = SUCCESS;

    if (NULL == ctx || NULL == wan_info) {
        return ERR_INVALID_ARGS;
    }

    memset(wan_info,0,sizeof(wanInfo_t));

    UTOPIA_GET(ctx, UtopiaValue_WAN_Proto, s_tokenbuf, sizeof(s_tokenbuf));
    wan_info->wan_proto = s_StrToEnum(g_WanTypeMap, s_tokenbuf);

    Utopia_Get(ctx, UtopiaValue_WAN_ProtoAuthDomain, wan_info->domainname, IPHOSTNAME_SZ);
    UTOPIA_GETINT(ctx, UtopiaValue_WAN_MTU, &wan_info->mtu_size);
    wan_info->auto_mtu = (wan_info->mtu_size == 0)? TRUE:FALSE;

    switch (wan_info->wan_proto) {
    case DHCP:
    case STATIC:
    case PPPOE:
    case PPTP:
    case L2TP:
    case TELSTRA:
        rc = s_GetWANStaticSetting (ctx, &wan_info->wstatic);
        
        if (rc == SUCCESS) {
            rc = s_GetWANPPPSetting (ctx, wan_info->wan_proto, wan_info);
        }
        break;
    default:
        return ERR_INVALID_WAN_TYPE;
    }

    return rc;
}

#define DHCP_CLIENT_LOG_FILE "/tmp/udhcp.log"

static void s_getWANDHCPConnectionInfo (UtopiaContext *ctx, int se_fd, token_t se_token, wanConnectionInfo_t *wan)
{
    char dns[512] = {0};
    char lease[64] = {0};

    // Unused
    (void) ctx;

    sysevent_get(se_fd, se_token, "current_wan_subnet", wan->subnet_mask, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "default_router", wan->default_gw, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "wan_dhcp_lease", lease, sizeof(lease));
    wan->dhcp_lease_time = atoi(lease);
    sysevent_get(se_fd, se_token, "wan_dhcp_dns", dns, sizeof(dns));
    if (strlen(dns) > 0) {
        char *token, *saveptr;
        int count = 0;

        token = strtok_r(dns, " ", &saveptr);
        while (token && count < NUM_DNS_ENTRIES) {
            strncpy(wan->dns[count], token, IPHOSTNAME_SZ);
            token = strtok_r(NULL, " ", &saveptr);
            count++;
        }
    }
}

#define PPP_DNS_RESOLVE_FILE "/tmp/ppp/resolve.conf"

static void s_getWANPPPConnectionInfo (UtopiaContext *ctx, int se_fd, token_t se_token, wanConnectionInfo_t *wan)
{
    char dns[512] = {0};

    // Unused
    (void) ctx;

    sysevent_get(se_fd, se_token, "current_wan_subnet", wan->subnet_mask, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "wan_default_gateway", wan->default_gw, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "wan_ppp_dns", dns, sizeof(dns));
    if (strlen(dns) > 0) {
        char *token, *saveptr;
        int count = 0;

        token = strtok_r(dns, " ", &saveptr);
        while (token && count < NUM_DNS_ENTRIES) {
            strncpy(wan->dns[count], token, IPHOSTNAME_SZ);
            token = strtok_r(NULL, " ", &saveptr);
            count++;
        }
    }
}

static void s_getWANStaticConnectionInfo (UtopiaContext *ctx, wanConnectionInfo_t *wan)
{
    Utopia_Get(ctx, UtopiaValue_WAN_IPAddr, wan->ip_address, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_Netmask, wan->subnet_mask, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_DefaultGateway, wan->default_gw, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_NameServer1, wan->dns[0], IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_NameServer2, wan->dns[1], IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_WAN_NameServer3, wan->dns[2], IPADDR_SZ);
}

int Utopia_GetWANConnectionInfo (UtopiaContext *ctx, wanConnectionInfo_t *wan)
{
    token_t        se_token;

    if (NULL == ctx || NULL == wan) {
        return ERR_INVALID_ARGS;
    }

    bzero(wan, sizeof(wanConnectionInfo_t));

    int se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    sysevent_get(se_fd, se_token, "current_wan_ipaddr", wan->ip_address, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "current_wan_ifname", wan->ifname, IFNAME_SZ);

    UTOPIA_GET(ctx, UtopiaValue_WAN_Proto, s_tokenbuf, sizeof(s_tokenbuf));
    wanProto_t wan_proto = s_StrToEnum(g_WanTypeMap, s_tokenbuf);

    switch (wan_proto) {
    case DHCP:
        s_getWANDHCPConnectionInfo(ctx, se_fd, se_token, wan);
        break;
    case PPPOE:
    case PPTP:
    case L2TP:
    case TELSTRA:
        s_getWANPPPConnectionInfo(ctx, se_fd, se_token, wan);
        break;
    case STATIC:
        s_getWANStaticConnectionInfo(ctx, wan);
        break;
    default:
        break;
    }

    return SUCCESS;
}

int Utopia_GetWANConnectionStatus (UtopiaContext *ctx, wanConnectionStatus_t *wan)
{
    token_t        se_token;
    char wan_status[32];

    if (NULL == ctx || NULL == wan) {
        return ERR_INVALID_ARGS;
    }

    bzero(wan, sizeof(wanConnectionStatus_t));

    int se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    /*
     * WAN Connection Status
     */
    sysevent_get(se_fd, se_token, "wan-status", wan_status, sizeof(wan_status));

    if (0 == strcmp(wan_status, "started")) {
        /*
         * WAN Connection IP Address
         */
        sysevent_get(se_fd, se_token, "current_wan_ipaddr", wan->ip_address, IPADDR_SZ);
        if (0 == strcmp(wan->ip_address, "0.0.0.0")) {
            wan->status = WAN_DISCONNECTED;
        } else {
            wan->status = WAN_CONNECTED;
        }
    } else 
    if (0 == strcmp(wan_status, "starting")) {
        wan->status = WAN_CONNECTING;
    } else 
    if (0 == strcmp(wan_status, "stopping")) {
        sysevent_get(se_fd, se_token, "current_wan_ipaddr", wan->ip_address, IPADDR_SZ);
        wan->status = WAN_DISCONNECTING;
    } else 
    if (0 == strcmp(wan_status, "stopped")) {
        wan->status = WAN_DISCONNECTED;
    } else {
        wan->status = WAN_UNKNOWN;
    }

    /*
     * WAN Physical Link Status
     */
    sysevent_get(se_fd, se_token, "phylink_wan_state", wan_status, sizeof(wan_status));
    wan->phylink_up = (0 == strcmp(wan_status, "up")) ? 1 : 0;
 
    /*
     * WAN Connection uptime
     */
    if (WAN_CONNECTED == wan->status) {
        // this is a difference between system-uptime from /proc/uptime
        //   and wan_start_time sysevent which got recorded using /proc/uptime value when
        //   wan link came up or restarted
        char wanup_buf[64], sysup_buf[64];
        long wan_start_time = 0, sys_uptime = 0;
    
        sysevent_get(se_fd, se_token, "wan_start_time", wanup_buf, sizeof(wanup_buf));
        // ulogf(ULOG_CONFIG, UL_UTAPI, "%s: wan_start_time %s", __FUNCTION__, wanup_buf);
        if (strlen(wanup_buf) > 0) {
            FILE *fp = fopen("/proc/uptime","r");
            if (fp) {
                if (fgets(sysup_buf, sizeof(sysup_buf), fp)) {
                    // ulogf(ULOG_CONFIG, UL_UTAPI, "%s: proc uptime %s", __FUNCTION__, sysup_buf);
                    char *p = chop_str(sysup_buf, '.');
                    if (p) {
                        wan_start_time = atol(wanup_buf);
                        sys_uptime = atol(sysup_buf);
                        // ulogf(ULOG_CONFIG, UL_UTAPI, "%s: diff wan start %ld  sys uptime %ld", __FUNCTION__, wan_start_time, sys_uptime);
                        wan->uptime = sys_uptime - wan_start_time;
                    }
                }
                fclose(fp);
            }
        }
    }

    return SUCCESS;
}

static void s_getBridgeDHCPConnectionInfo (int se_fd, token_t se_token, bridgeConnectionInfo_t *bridge)
{
    char dns[512] = {0};
    char lease[64] = {0};

    sysevent_get(se_fd, se_token, "bridge_ipv4_ipaddr", bridge->ip_address, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "bridge_ipv4_subnet", bridge->subnet_mask, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "bridge_default_router", bridge->default_gw, IPADDR_SZ);
    sysevent_get(se_fd, se_token, "bridge_dhcp_lease", lease, sizeof(lease));
    bridge->dhcp_lease_time = atoi(lease);
    sysevent_get(se_fd, se_token, "bridge_dhcp_dns", dns, sizeof(dns));
    if (strlen(dns) > 0) {
        char *token, *saveptr;
        int count = 0;

        token = strtok_r(dns, " ", &saveptr);
        while (token && count < NUM_DNS_ENTRIES) {
            strncpy(bridge->dns[count], token, IPHOSTNAME_SZ);
            token = strtok_r(NULL, " ", &saveptr);
            count++;
        }
    }
}

static void s_getBridgeStaticConnectionInfo (UtopiaContext *ctx, bridgeConnectionInfo_t *bridge)
{
    Utopia_Get(ctx, UtopiaValue_Bridge_IPAddress, bridge->ip_address, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_Netmask, bridge->subnet_mask, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_DefaultGateway, bridge->default_gw, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_NameServer1, bridge->dns[0], IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_NameServer2, bridge->dns[1], IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_NameServer3, bridge->dns[2], IPADDR_SZ);
}

int Utopia_GetBridgeConnectionInfo (UtopiaContext *ctx, bridgeConnectionInfo_t *bridge)
{
    token_t      se_token;
    int          se_fd;
    bridgeMode_t bridge_mode;

    if (NULL == ctx || NULL == bridge) {
        return ERR_INVALID_ARGS;
    }

    bzero(bridge, sizeof(bridgeConnectionInfo_t));

    // Return a zeroed out struct if bridge mode is off
    if (SUCCESS != Utopia_GetInt(ctx, UtopiaValue_Bridge_Mode, (int *)&bridge_mode) ||
        bridge_mode == BRIDGE_MODE_OFF)
    {
        return SUCCESS;
    }

    switch (bridge_mode) {
        case BRIDGE_MODE_DHCP:
            se_fd = s_sysevent_connect(&se_token);
            if (0 > se_fd) {
                return ERR_SYSEVENT_CONN;
            }
            s_getBridgeDHCPConnectionInfo(se_fd, se_token, bridge);
            break;
        case BRIDGE_MODE_STATIC:
        case BRIDGE_MODE_FULL_STATIC:			
            s_getBridgeStaticConnectionInfo(ctx, bridge);
            break;
        default:
            return ERR_INVALID_BRIDGE_MODE;
    }

    return SUCCESS;
}

/*
 * Parse the /proc/net/dev file that looks like,
 * Inter-|   Receive                                                |  Transmit
 * face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
  * vlan2:56771141  268709    0    0    0     0          0     27161 56801729  222747    0    0    0     0       0          0
 */
#define PROC_NET_DEV "/proc/net/dev"

int Utopia_GetWANTrafficInfo (wanTrafficInfo_t *wan)
{
    token_t        se_token;
    char wan_ifname[32] = {0};

    if (NULL == wan) {
        return ERR_INVALID_ARGS;
    }

    bzero(wan, sizeof(wanTrafficInfo_t));

    int se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    sysevent_get(se_fd, se_token, "current_wan_ifname", wan_ifname, sizeof(wan_ifname));

    FILE *fp = fopen(PROC_NET_DEV, "r");
    if (!fp) {
        return ERR_FILE_READ_FAILED;
    }

    char line_buf[512];
    int  line_num = 0, ct;

    while (NULL != fgets(line_buf, sizeof(line_buf), fp)) {
        line_num++;
        // Skip first two lines
        if (line_num <= 2) {
            continue;
        }
        if (strstr(line_buf, wan_ifname)) {
            char *tok, *saveptr;

            if (NULL == (tok = strchr(line_buf, ':'))) {
                continue;
            }
            tok++;
            if (NULL == (tok = strtok_r(tok, " ", &saveptr))) {
                continue;
            }
            wan->bytes_rcvd = atol(tok);
            if (NULL == (tok = strtok_r(NULL, " ", &saveptr))) {
                continue;
            }
            wan->pkts_rcvd = atol(tok);

            // Skp next six fields
            for (ct = 0; ct < 6; ct++) {
                strtok_r(NULL, " ", &saveptr);
            }
            if (NULL == (tok = strtok_r(NULL, " ", &saveptr))) {
                continue;
            }
            wan->bytes_sent = atol(tok);

            if (NULL == (tok = strtok_r(NULL, " ", &saveptr))) {
                continue;
            }
            wan->pkts_sent = atol(tok);

            ulogf(ULOG_CONFIG, UL_UTAPI, "traffic stats: bytes sent %ld, rcvd %ld, pkts sent %ld, rcvd %ld\n",
                   wan->bytes_sent,
                   wan->bytes_rcvd,
                   wan->pkts_sent,
                   wan->pkts_rcvd);
            break;

        }
    }
    fclose(fp);

    return UT_SUCCESS;
}

typedef enum {
    DHCP_RELEASE,
    DHCP_RENEW,
    DHCP_RESTART,
} dhcpClientOp_t;

/*
 * Utility to set appropriate sysevent
 * this operation valid only if wan-proto is dhcp
 */
static int s_WAN_DHCPClient_control (dhcpClientOp_t oper)
{
    char buf[32];
    token_t  se_token;
    UtopiaContext ctx;

    if (!Utopia_Init(&ctx)) {
        return ERR_UTCTX_INIT;
    }
    Utopia_Get(&ctx, UtopiaValue_WAN_Proto, buf, sizeof(buf));
    wanProto_t wan_proto = s_StrToEnum(g_WanTypeMap, buf);
    Utopia_Free(&ctx, 0);

    if (DHCP != wan_proto) {
        return ERR_INVALID_WAN_TYPE;
    } 

    int se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        printf("Unable to register with sysevent daemon\n");
        return ERR_SYSEVENT_CONN;
    }

    switch (oper) {
    case DHCP_RELEASE:
        sysevent_set(se_fd, se_token, "dhcp_client-release", "", 0);
        break;
    case DHCP_RENEW:
        sysevent_set(se_fd, se_token, "dhcp_client-renew", "", 0);
        break;
    case DHCP_RESTART:
        sysevent_set(se_fd, se_token, "dhcp_client-restart", "", 0);
        break;
    }

    return UT_SUCCESS;
}

int Utopia_WANDHCPClient_Release (void)
{
    return s_WAN_DHCPClient_control(DHCP_RELEASE);
}

int Utopia_WANDHCPClient_Renew (void)
{
    return s_WAN_DHCPClient_control(DHCP_RENEW);
}

int Utopia_WANConnectionTerminate (void)
{
    token_t        se_token;

    int se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        printf("Unable to register with sysevent daemon\n");
        return ERR_SYSEVENT_CONN;
    }
    sysevent_set(se_fd, se_token, "wan-stop", "", 0);
    // sysevent_close(se_fd, se_token);

    return SUCCESS;
}

/*
 * Router/Bridge settings
 */

static int s_GetBridgeStaticSetting (UtopiaContext *ctx, bridge_static_t *bstatic)
{
    Utopia_Get(ctx, UtopiaValue_Bridge_IPAddress, bstatic->ip_addr, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_Netmask, bstatic->subnet_mask, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_DefaultGateway, bstatic->default_gw, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_Domain, bstatic->domainname, IPHOSTNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_NameServer1, bstatic->dns_ipaddr1, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_NameServer2, bstatic->dns_ipaddr2, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_Bridge_NameServer3, bstatic->dns_ipaddr3, IPADDR_SZ);

    return SUCCESS;
}

static int s_SetBridgeStaticSetting (UtopiaContext *ctx, bridge_static_t bstatic)
{
    UTOPIA_SETIP(ctx, UtopiaValue_Bridge_IPAddress, bstatic.ip_addr);
    UTOPIA_VALIDATE_SET(ctx, UtopiaValue_Bridge_Netmask, bstatic.subnet_mask, IsValid_Netmask, ERR_INVALID_NETMASK);
    UTOPIA_SETIP(ctx, UtopiaValue_Bridge_DefaultGateway, bstatic.default_gw);
    UTOPIA_SET(ctx, UtopiaValue_Bridge_Domain, bstatic.domainname);
    UTOPIA_SETIP(ctx, UtopiaValue_Bridge_NameServer1, bstatic.dns_ipaddr1);
    UTOPIA_SETIP(ctx, UtopiaValue_Bridge_NameServer2, bstatic.dns_ipaddr2);
    UTOPIA_SETIP(ctx, UtopiaValue_Bridge_NameServer3, bstatic.dns_ipaddr3);

    return SUCCESS;
}

int Utopia_SetBridgeSettings (UtopiaContext *ctx, bridgeInfo_t *bridge_info)
{
    int rc = SUCCESS;

    if (NULL == ctx || NULL == bridge_info) {
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SETINT(ctx, UtopiaValue_Bridge_Mode, bridge_info->mode);

    switch (bridge_info->mode) {
    case BRIDGE_MODE_OFF:
    case BRIDGE_MODE_DHCP:
        break;
    case BRIDGE_MODE_STATIC:
	case BRIDGE_MODE_FULL_STATIC:
        rc = s_SetBridgeStaticSetting (ctx, bridge_info->bstatic);
        break;
    default:
        return ERR_INVALID_BRIDGE_MODE;
    }
    return rc;
}

int Utopia_GetBridgeSettings (UtopiaContext *ctx, bridgeInfo_t *bridge_info)
{
    int rc = SUCCESS;

    if (NULL == ctx || NULL == bridge_info) {
        return ERR_INVALID_ARGS;
    }

    if (SUCCESS != Utopia_GetInt(ctx, UtopiaValue_Bridge_Mode, (int *)&bridge_info->mode)) {
        bridge_info->mode = BRIDGE_MODE_OFF;
    }

    switch (bridge_info->mode) {
    case BRIDGE_MODE_OFF:
    case BRIDGE_MODE_DHCP:
    case BRIDGE_MODE_STATIC:
	case BRIDGE_MODE_FULL_STATIC:
        rc = s_GetBridgeStaticSetting (ctx, &bridge_info->bstatic);
        break;
    default:
        return ERR_INVALID_BRIDGE_MODE;
    }

    return rc;
}

static int ddns_status_reset ()
{
    char *val = "";

    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    sysevent_set(se_fd, se_token, "ddns_return_status", val, 0);

    return UT_SUCCESS;
}

int Utopia_SetDDNSService (UtopiaContext *ctx, ddnsService_t *ddns)
{
    UTOPIA_SETBOOL(ctx, UtopiaValue_DDNS_Enable, ddns->enabled)
    if (TRUE == ddns->enabled) {
        char *p = s_EnumToStr(g_DDNSProviderMap, ddns->provider);
        if (NULL == p) {
            return ERR_INVALID_DDNS_TYPE;
        }
        UTOPIA_SET(ctx, UtopiaValue_DDNS_Service, p);
        UTOPIA_SET(ctx, UtopiaValue_DDNS_Hostname, ddns->hostname);
        UTOPIA_SET(ctx, UtopiaValue_DDNS_Username, ddns->username);
        UTOPIA_SET(ctx, UtopiaValue_DDNS_Password, ddns->password);
        UTOPIA_SET(ctx, UtopiaValue_DDNS_Mx, ddns->mail_exch);
        UTOPIA_SETBOOL(ctx, UtopiaValue_DDNS_MxBackup, ddns->backup_mx)
        UTOPIA_SETBOOL(ctx, UtopiaValue_DDNS_Wildcard, ddns->wildcard)
    } 
    UTOPIA_UNSET(ctx, UtopiaValue_DDNS_LastUpdate);
    ddns_status_reset();
    return SUCCESS;
}

int Utopia_UpdateDDNSService (UtopiaContext *ctx)
{
    UTOPIA_UNSET(ctx, UtopiaValue_DDNS_LastUpdate);
    ddns_status_reset();
    return SUCCESS;
}

int Utopia_GetDDNSService (UtopiaContext *ctx, ddnsService_t *ddns)
{
    bzero(ddns, sizeof(ddns));

    Utopia_GetBool(ctx, UtopiaValue_DDNS_Enable, &ddns->enabled);
    if (TRUE == ddns->enabled) {
        UTOPIA_GET(ctx, UtopiaValue_DDNS_Service, s_tokenbuf, sizeof(s_tokenbuf));
        ddns->provider = s_StrToEnum(g_DDNSProviderMap, s_tokenbuf);
        UTOPIA_GET(ctx, UtopiaValue_DDNS_Hostname, ddns->hostname, IPHOSTNAME_SZ);
        UTOPIA_GET(ctx, UtopiaValue_DDNS_Username, ddns->username, USERNAME_SZ);
        UTOPIA_GET(ctx, UtopiaValue_DDNS_Password, ddns->password, PASSWORD_SZ);
        Utopia_Get(ctx, UtopiaValue_DDNS_Mx, ddns->mail_exch, IPHOSTNAME_SZ);
        Utopia_GetBool(ctx, UtopiaValue_DDNS_MxBackup, &ddns->backup_mx);
        Utopia_GetBool(ctx, UtopiaValue_DDNS_Wildcard, &ddns->wildcard);
    }
    return SUCCESS;
}

int Utopia_GetDDNSServiceStatus (UtopiaContext *ctx, ddnsStatus_t *ddnsStatus)
{
    char buf[TOKEN_SZ];

    if (NULL == ctx || NULL == ddnsStatus) {
        return ERR_INVALID_ARGS;
    }

    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    sysevent_get(se_fd, se_token, "ddns_return_status", buf, sizeof(buf));

    int status = s_StrToEnum(g_DDNSStatus, buf);
    *ddnsStatus = (status < 0) ?  DDNS_STATUS_UNKNOWN : status;
    
    return UT_SUCCESS;
}

int Utopia_SetMACAddressClone (UtopiaContext *ctx, boolean_t enable, char macaddr[MACADDR_SZ])
{
    if (TRUE == enable) {
        UTOPIA_VALIDATE_SET(ctx, UtopiaValue_DefHwAddr, macaddr, IsValid_MACAddr, ERR_INVALID_MAC);
    } else {
        Utopia_Unset(ctx, UtopiaValue_DefHwAddr);
    }
    return SUCCESS;
}

int Utopia_GetMACAddressClone (UtopiaContext *ctx, boolean_t *enable, char macaddr[MACADDR_SZ])
{
    if (NULL == ctx || NULL == enable || NULL == macaddr) {
        return ERR_INVALID_ARGS;
    }

    *enable = FALSE;
    if (Utopia_Get(ctx, UtopiaValue_DefHwAddr, macaddr, MACADDR_SZ)) {
        *enable = TRUE;
    }
    return SUCCESS;
}

/*
 * Route Settings
 */
int Utopia_SetRouteNAT (UtopiaContext *ctx, napt_mode_t enable)
{
    UTOPIA_SETINT(ctx, UtopiaValue_NATEnabled, enable);
    return SUCCESS;
}

int Utopia_GetRouteNAT (UtopiaContext *ctx, napt_mode_t *enable)
{
    Utopia_GetInt(ctx, UtopiaValue_NATEnabled, enable);
    return SUCCESS;
}

int Utopia_SetRouteRIP (UtopiaContext *ctx, routeRIP_t rip)
{
    UTOPIA_SETBOOL(ctx, UtopiaValue_RIP_Enabled, rip.enabled);
    if (TRUE == rip.enabled) {
        UTOPIA_SETBOOL(ctx, UtopiaValue_RIP_NoSplitHorizon, rip.no_split_horizon);
        UTOPIA_SETBOOL(ctx, UtopiaValue_RIP_InterfaceLAN, rip.lan_interface);
        UTOPIA_SETBOOL(ctx, UtopiaValue_RIP_InterfaceWAN, rip.wan_interface);
        UTOPIA_SET(ctx, UtopiaValue_RIP_MD5Passwd, rip.wan_md5_password);
        UTOPIA_SET(ctx, UtopiaValue_RIP_TextPasswd, rip.wan_text_password);
    } 

    return SUCCESS;
}

int Utopia_GetRouteRIP (UtopiaContext *ctx, routeRIP_t *rip)
{
    bzero(rip, sizeof(routeRIP_t));

    Utopia_GetBool(ctx, UtopiaValue_RIP_Enabled, &rip->enabled);
    if (TRUE == rip->enabled) {
        Utopia_GetBool(ctx, UtopiaValue_RIP_NoSplitHorizon, &rip->no_split_horizon);
        Utopia_GetBool(ctx, UtopiaValue_RIP_InterfaceLAN, &rip->lan_interface);
        Utopia_GetBool(ctx, UtopiaValue_RIP_InterfaceWAN, &rip->wan_interface);
        Utopia_Get(ctx, UtopiaValue_RIP_MD5Passwd, rip->wan_md5_password, PASSWORD_SZ);
        Utopia_Get(ctx, UtopiaValue_RIP_TextPasswd, rip->wan_text_password, PASSWORD_SZ);
    }

    return SUCCESS;
}

/*
 * Static Routing
 */

static int s_get_staticroute (UtopiaContext *ctx, int index, routeStatic_t *sroute)
{
    char buf[128];

    Utopia_GetIndexed(ctx, UtopiaValue_StaticRoute, index, buf, sizeof(buf));
    if (0 == (strcmp(buf, "none"))) {
        return UT_SUCCESS;
    }

    Utopia_GetIndexed(ctx, UtopiaValue_SR_Name, index, sroute->name, NAME_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_SR_Dest, index, sroute->dest_lan_ip, IPADDR_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_SR_Netmask, index, sroute->netmask, IPADDR_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_SR_Gateway, index, sroute->gateway, IPADDR_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_SR_Interface, index, buf, sizeof(buf));
    sroute->dest_intf = (0 == strcasecmp(buf, "wan")) ?  INTERFACE_WAN : INTERFACE_LAN;

    return UT_SUCCESS;
}

static int s_set_staticroute (UtopiaContext *ctx, int index, routeStatic_t *sroute)
{
    char buf[128];

    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "sroute_%d", index);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_StaticRoute, index, s_tokenbuf);

    UTOPIA_SETINDEXED(ctx, UtopiaValue_SR_Name, index, sroute->name);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SR_Dest, index, sroute->dest_lan_ip);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SR_Netmask, index, sroute->netmask);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SR_Gateway, index, sroute->gateway);

    strncpy(buf, (INTERFACE_WAN == sroute->dest_intf) ? "wan" : "lan", sizeof(buf));
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SR_Interface, index, buf);

    return UT_SUCCESS;
}

/*
 * Remove 'index' entry and move-up 'index' to 'count' entries
 */
static int s_delete_staticroute (UtopiaContext *ctx, int index)
{
    int i, count = 0;

    Utopia_GetInt(ctx, UtopiaValue_StaticRoute_Count, &count);

    if (index < 1 || index > count) {
        return ERR_INVALID_VALUE;
    }

    // delete 'index' entry
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_StaticRoute, index);
    
    if (index < count) {
        for (i = index; i < count ; i++) {
            routeStatic_t sroute;

            bzero(&sroute, sizeof(routeStatic_t));
            s_get_staticroute(ctx, i+1, &sroute); 
            s_set_staticroute(ctx, i, &sroute); 
        }
        // delete last vacated entry at index=count
        UTOPIA_UNSETINDEXED(ctx, UtopiaValue_StaticRoute, count);
    }

    UTOPIA_SETINT(ctx, UtopiaValue_StaticRoute_Count, count-1);

    return UT_SUCCESS;
}

/*
 * Utility routine to find a static-route entry given an array of static routes
 * and a friendly name
 *
 * Returns -1 if entry not found
 *         [1 to count], index of entry found
 */
int Utopia_FindStaticRoute (int count, routeStatic_t *sroutes, const char *route_name)
{
    int i;

    for (i = 0; i < count; i++) {
        if (0 == strcasecmp(sroutes[i].name, route_name)) {
            return i+1;
        }
    }
    return -1;
}

int Utopia_DeleteStaticRoute (UtopiaContext *ctx, int index)
{

    if (NULL == ctx) {
        return ERR_INVALID_ARGS;
    }

    int count;
    Utopia_GetInt(ctx, UtopiaValue_StaticRoute_Count, &count);

    if (index < 1 || index > count) {
        return ERR_ITEM_NOT_FOUND;
    }

    return s_delete_staticroute(ctx, index);
}

int Utopia_DeleteStaticRouteName (UtopiaContext *ctx, const char *route_name)
{
    if (NULL == ctx || NULL == route_name) {
        return ERR_INVALID_ARGS;
    }

    int rc, count = 0, index;
    routeStatic_t *sroutes = NULL;

    if (UT_SUCCESS == (rc = Utopia_GetStaticRoutes(ctx, &count, &sroutes))) {
        if (-1 != (index = Utopia_FindStaticRoute(count, sroutes, route_name))) {
            rc = s_delete_staticroute(ctx, index);
        } else {
            rc = ERR_ITEM_NOT_FOUND;
        }
        if (sroutes) {
            free(sroutes);
        }
    } 

    return rc;
}


int Utopia_AddStaticRoute (UtopiaContext *ctx, routeStatic_t *sroute)
{
    int rc, count = 0;

    if (NULL == ctx || NULL == sroute) {
        return ERR_INVALID_ARGS;
    }

    Utopia_GetInt(ctx, UtopiaValue_StaticRoute_Count, &count);

    // add new static route entry as (count+1) index
    if (SUCCESS != (rc = s_set_staticroute(ctx, count+1, sroute))) {
        return rc;
    }

    UTOPIA_SETINT(ctx, UtopiaValue_StaticRoute_Count, count+1);

    return UT_SUCCESS;
}

int Utopia_EditStaticRoute (UtopiaContext *ctx, int index, routeStatic_t *sroute)
{
    if (NULL == ctx || index < 1 || NULL == sroute) {
        return ERR_INVALID_ARGS;
    }
    
    int rc, count = 0;
    
    Utopia_GetInt(ctx, UtopiaValue_StaticRoute_Count, &count);
    
    if (index > count) {
        return ERR_INVALID_ARGS;
    }
    
    if (SUCCESS != (rc = s_set_staticroute(ctx, index, sroute))) {
        return rc;
    }

    return UT_SUCCESS;
}

int Utopia_GetStaticRouteCount (UtopiaContext *ctx, int *count)
{
    if (NULL == ctx || NULL == count) {
        return ERR_INVALID_ARGS;
    }

    *count = 0; /*RDKB-7128, CID-33530, null check before use */

    Utopia_GetInt(ctx, UtopiaValue_StaticRoute_Count, count);
    return UT_SUCCESS;
}

/*
 * Caller-need-to-free
 * Since we malloc buffer here, don't use macro get method (it won't free buffer)
 */
int Utopia_GetStaticRoutes (UtopiaContext *ctx, int *count, routeStatic_t **out_sroute)
{
    int i;
    routeStatic_t *sroute;

    if (NULL == ctx || NULL == count || NULL == out_sroute) {
        return ERR_INVALID_ARGS;
    }

    Utopia_GetInt(ctx, UtopiaValue_StaticRoute_Count, count);
    if (*count <= 0) {
        return UT_SUCCESS;
    }

    sroute = (routeStatic_t *) malloc(sizeof(routeStatic_t) * (*count));
    if (NULL == sroute) {
        return ERR_INSUFFICIENT_MEM;
    }
    bzero(sroute, sizeof(routeStatic_t) * (*count));

    for (i = 0; i < *count; i++) {
        int rc;
        if (SUCCESS != (rc = s_get_staticroute(ctx, i+1, &sroute[i]))) {
            free(sroute);
            *out_sroute = NULL;
            return rc;
        }
    }
    
    *out_sroute = sroute;
    return UT_SUCCESS;
}

int Utopia_GetStaticRouteTable (int *count, routeStatic_t **out_sroute)
{
    int i;
    int j;
    routeStatic_t *sroute;
    char cmd[512];
    char line_buf[512];
    int line_count;
    token_t se_token;
    char wan_ifname[IFNAME_SZ];

    if (NULL == count || NULL == out_sroute) {
        return ERR_INVALID_ARGS;
    }
    
    int se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    
    *count = 0;
    
    snprintf(cmd, sizeof(cmd), "route -en | grep -v \"^127.0.0\" > %s", ROUTE_TABLE_TMP_FILE);
    system(cmd);
    
    FILE *fp = fopen(ROUTE_TABLE_TMP_FILE, "r");
    
    if (!fp) {
        return ERR_FILE_NOT_FOUND;
    }
    
    line_count = 0;
    while (NULL != fgets(line_buf, sizeof(line_buf), fp)) {
        line_count++;
    }
    
    *count = line_count - 2; // Skip the first two lines
    if (*count <= 0) {
        fclose(fp);
        return UT_SUCCESS;
    }

    sroute = (routeStatic_t *) malloc(sizeof(routeStatic_t) * (*count));
    if (NULL == sroute) {
        fclose(fp);/*RDKB-7128, CID-33470, free unused resources before exit*/
        return ERR_INSUFFICIENT_MEM;
    }
    bzero(sroute, sizeof(routeStatic_t) * (*count));
    
    // Seek to beginning of file
    fseek(fp, 0, SEEK_SET);
    
    // Read past the first two lines
    for (i = 0; i < 2; i++) {
        if (NULL == fgets(line_buf, sizeof(line_buf), fp)) {
    	    break;
        }
    }
    
    sysevent_get(se_fd, se_token, "current_wan_ifname", wan_ifname, IFNAME_SZ);
    
    for (i = 0; i < *count && fgets(line_buf, sizeof(line_buf), fp) != NULL; i++) {
        char *start;
        int len;
        
        // Set the destination LAN IP
        start = line_buf;
        len = strcspn(start, " ");
        if (len >= IPADDR_SZ) {
            len = IPADDR_SZ - 1;
        }
        strncpy(sroute[i].dest_lan_ip, start, len);
        sroute[i].dest_lan_ip[len] = 0;
        
        // Set the gateway
        start += len;
        start += strspn(start, " ");
        len = strcspn(start, " ");
        if (len >= IPADDR_SZ) {
            len = IPADDR_SZ - 1;
        }
        strncpy(sroute[i].gateway, start, len);
        sroute[i].gateway[len] = 0;
        
        // Set the netmask
        start += len;
        start += strspn(start, " ");
        len = strcspn(start, " ");
        if (len >= IPADDR_SZ) {
            len = IPADDR_SZ - 1;
        }
        strncpy(sroute[i].netmask, start, len);
        sroute[i].netmask[len] = 0;
        
        // Skip the next four columns
        for (j = 0; j < 4; j++) {
            start += len;
	        start += strspn(start, " ");
	        len = strcspn(start, " ");
        }
        
        // Lookup the interface
        start += len;
        start += strspn(start, " ");
        
        // Set the route's interface
        if (!strncmp(start, wan_ifname, strlen(wan_ifname))) {
            sroute[i].dest_intf = INTERFACE_WAN;
        }
        else {
            sroute[i].dest_intf = INTERFACE_LAN;
        }
    }
    
    fclose(fp);
    
    *out_sroute = sroute;
    return UT_SUCCESS;
}

/*
 * Firewall Settings
 */

/*
 * String literals
 */
#define FW_W2L_BLOCK_IPSEC_RULE    1
#define FW_W2L_BLOCK_PPTP_RULE     2
#define FW_W2L_BLOCK_L2TP_RULE     3

/*
 * String literals
 */

static const char *s_blockipsec =  "blockipsec";
static const char *s_blockpptp =  "blockpptp";
static const char *s_blockl2tp =  "blockl2tp";
static const char *s_blockssl = "blockssl";

static int setFWBlockingRule (UtopiaContext *ctx, int w2l_rule_index,
                              const char *ns, const char *name, const char *result)
{
     Utopia_SetIndexed(ctx, UtopiaValue_FW_W2LWellKnown, w2l_rule_index, (char *) ns);
     Utopia_SetIndexed(ctx, UtopiaValue_FW_W2LWK_Name,   w2l_rule_index, (char *) name);
     Utopia_SetIndexed(ctx, UtopiaValue_FW_W2LWK_Result, w2l_rule_index, (char *) result);

     return SUCCESS;
}

#if 0
static int unsetFWBlockingRule (UtopiaContext *ctx, int w2l_rule_index)
{
     Utopia_UnsetIndexed(ctx, UtopiaValue_FW_W2LWK_Name,   w2l_rule_index);
     Utopia_UnsetIndexed(ctx, UtopiaValue_FW_W2LWK_Result, w2l_rule_index);
     Utopia_UnsetIndexed(ctx, UtopiaValue_FW_W2LWellKnown, w2l_rule_index);

     return SUCCESS;
}

/*
 * treat presence of an entry in the W2LWellKnownFirewallRule_x as rule is "set"
 */
static boolean_t getFWBlockingRule (UtopiaContext *ctx, int w2l_rule_index)
{
     return Utopia_GetIndexed(ctx, UtopiaValue_FW_W2LWellKnown, w2l_rule_index, s_tokenbuf, sizeof(s_tokenbuf));
}
#endif

int Utopia_SetFirewallSettings (UtopiaContext *ctx, firewall_t fw)
{
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_Enabled, fw.spi_protection);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockPing, fw.filter_anon_req);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockMulticast, fw.filter_multicast);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockNatRedir, fw.filter_nat_redirect);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockIdent, fw.filter_ident);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockWebProxy, fw.filter_web_proxy);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockJava, fw.filter_web_java);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockActiveX, fw.filter_web_activex);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockCookies, fw.filter_web_cookies);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockHttp, fw.filter_http_from_wan);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockP2p, fw.filter_p2p_from_wan);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockRFC1918, fw.filter_rfc1918_from_wan);

     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockPingV6, fw.filter_anon_req_v6);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockMulticastV6, fw.filter_multicast_v6);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockIdentV6, fw.filter_ident_v6);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockHttpV6, fw.filter_http_from_wan_v6);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_BlockP2pV6, fw.filter_p2p_from_wan_v6);

     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_TrueStaticIpEnable, fw.true_static_ip_enable);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_TrueStaticIpEnableV6, fw.true_static_ip_enable_v6);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_SmartEnable, fw.smart_pkt_dection_enable);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_SmartEnableV6, fw.smart_pkt_dection_enable_v6);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_WanPingEnable, fw.wan_ping_enable);
     UTOPIA_SETBOOL(ctx, UtopiaValue_Firewall_WanPingEnableV6, fw.wan_ping_enable_v6);

     int rule_count = 0;

         FALSE == fw.allow_ipsec_passthru? setFWBlockingRule(ctx, ++rule_count, s_blockipsec, "ipsec", "$DROP") : setFWBlockingRule(ctx, ++rule_count, s_blockipsec, "ipsec", "$ACCEPT");

      FALSE == fw.allow_pptp_passthru? setFWBlockingRule(ctx, ++rule_count, s_blockpptp, "pptp", "$DROP") : setFWBlockingRule(ctx, ++rule_count, s_blockpptp, "pptp", "$ACCEPT");

     FALSE == fw.allow_l2tp_passthru? setFWBlockingRule(ctx, ++rule_count, s_blockl2tp, "l2tp", "$DROP") : setFWBlockingRule(ctx, ++rule_count, s_blockl2tp, "l2tp", "$ACCEPT");

      FALSE == fw.allow_ssl_passthru? setFWBlockingRule(ctx, ++rule_count, s_blockssl, "ssl", "$DROP") : setFWBlockingRule(ctx, ++rule_count, s_blockssl, "ssl", "$ACCEPT");


     UTOPIA_SETINT(ctx, UtopiaValue_Firewall_W2LWKRuleCount, rule_count);

     return SUCCESS;
}

int Utopia_GetFirewallSettings (UtopiaContext *ctx, firewall_t *fw)
{
    int rule_count;
    int i;
    char buf[8];
    
    bzero(fw, sizeof(firewall_t));

    Utopia_GetBool(ctx, UtopiaValue_Firewall_Enabled, &fw->spi_protection);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockPing, &fw->filter_anon_req);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockMulticast, &fw->filter_multicast);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockNatRedir, &fw->filter_nat_redirect);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockIdent, &fw->filter_ident);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockP2p, &fw->filter_p2p_from_wan);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockHttp, &fw->filter_http_from_wan);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockP2pV6, &fw->filter_rfc1918_from_wan);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockRFC1918, &fw->filter_rfc1918_from_wan);

    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockPingV6, &fw->filter_anon_req_v6);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockMulticastV6, &fw->filter_multicast_v6);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockIdentV6, &fw->filter_ident_v6);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockP2pV6, &fw->filter_p2p_from_wan_v6);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockHttpV6, &fw->filter_http_from_wan_v6);

    Utopia_GetBool(ctx, UtopiaValue_Firewall_TrueStaticIpEnable, &fw->true_static_ip_enable);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_TrueStaticIpEnableV6, &fw->true_static_ip_enable_v6);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_SmartEnable, &fw->smart_pkt_dection_enable);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_SmartEnableV6, &fw->smart_pkt_dection_enable_v6);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_WanPingEnable, &fw->wan_ping_enable);
    Utopia_GetBool(ctx, UtopiaValue_Firewall_WanPingEnableV6, &fw->wan_ping_enable_v6);

    if (SUCCESS != Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockWebProxy, &fw->filter_web_proxy)) {
        fw->filter_web_proxy = FALSE;
    }
    if (SUCCESS != Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockJava, &fw->filter_web_java)) {
        fw->filter_web_java = FALSE;
    }
    if (SUCCESS != Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockActiveX, &fw->filter_web_activex)) {
        fw->filter_web_activex = FALSE;
    }
    if (SUCCESS != Utopia_GetBool(ctx, UtopiaValue_Firewall_BlockCookies, &fw->filter_web_cookies)) {
        fw->filter_web_cookies = FALSE;
    }

    if (SUCCESS != Utopia_GetInt(ctx, UtopiaValue_Firewall_W2LWKRuleCount, &rule_count)) {
        rule_count = 0;
    }

    fw->allow_ipsec_passthru = TRUE;
    fw->allow_pptp_passthru = TRUE;
    fw->allow_l2tp_passthru = TRUE;
    fw->allow_ssl_passthru = TRUE;

     memset(buf, 0, sizeof(buf));
    syscfg_get(NULL, "blockssl::result", buf, sizeof(buf));
    if (0 == strcmp("$DROP",buf))
    {
           fw->allow_ssl_passthru = FALSE;
    }

    memset(buf, 0, sizeof(buf));
    syscfg_get(NULL, "blockipsec::result", buf, sizeof(buf));
    if (0 == strcmp("$DROP",buf))
    {
           fw->allow_ipsec_passthru = FALSE;
    }

    memset(buf, 0, sizeof(buf));
    syscfg_get(NULL, "blockl2tp::result", buf, sizeof(buf));
    if (0 == strcmp("$DROP",buf))
    {
           fw->allow_l2tp_passthru = FALSE;
    }

    memset(buf, 0, sizeof(buf));
    syscfg_get(NULL, "blockpptp::result", buf, sizeof(buf));
    if (0 == strcmp("$DROP",buf))
    {
           fw->allow_pptp_passthru = FALSE;
     }

    return SUCCESS;
}
 
/*
 * Port Mapping Settings
 */

/*
 * @brief removeConntrackRules() - Function to remove conntrack rules for PortMapping & DMZ based on the reconfiguration done by the user during runtime.
 */
static int removeConntrackRules(protocol_t proto, int internalport, char *destination)
{
    char buf[256] = {0};
    char toip[256] = {0};
    FILE *lanClients = NULL;

    if (proto == -1) { // DMZ
        memset(buf, 0, sizeof(buf));
        v_secure_system("conntrack -D -r %s", destination);
        sprintf(ulog_msg, "%s: Existing conntrack rules for DMZ host %s removed due to reconfiguration.", __FUNCTION__, destination);
        ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return SUCCESS;
    }
    else { // PortForwarding
        if (fopen("/tmp/lanClients", "r" ) == NULL) {  // connected hosts info available in '/tmp/lanClients' file
            sprintf(ulog_msg, "%s: File /tmp/lanClients (which gives connected hosts info) not present.", __FUNCTION__);
            ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            return ERR_FILE_NOT_FOUND;
        }
        snprintf(buf, sizeof(buf), "cat /tmp/lanClients | grep -w %s | awk '{print $3}'| tail -1", destination);
        if (!(lanClients = popen(buf, "r"))) {
            sprintf(ulog_msg, "%s: Lan client - %s not present in /tmp/lanClients file.", __FUNCTION__, destination);
            ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            return ERR_ITEM_NOT_FOUND;
        }
        fgets(toip, sizeof(toip), lanClients);
        toip[strlen(toip) - 1] = '\0';
        pclose(lanClients);
        if (proto == BOTH_TCP_UDP ) {
            memset(buf, 0, sizeof(buf));
            v_secure_system("conntrack -D -p tcp --reply-port-src %d -r %s", internalport, toip);
            memset(buf, 0, sizeof(buf));
            v_secure_system("conntrack -D -p udp --reply-port-src %d -r %s", internalport, toip);
            sprintf(ulog_msg, "%s: Existing conntrack rules for PortForwarding client %s (for both protocols - TCP & UDP) removed due to reconfiguration.", __FUNCTION__, toip);
            ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        }
        else {
            v_secure_system("conntrack -D -p %s --reply-port-src %d -r %s", ((proto == TCP)? "tcp" : "udp"), internalport, toip);
            sprintf(ulog_msg, "%s: Existing conntrack rules for PortForwarding client %s removed due to reconfiguration.", __FUNCTION__, toip);
            ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        }
        return SUCCESS;
    }
}

static int s_getportfwd_ruleid (UtopiaContext *ctx, int index)
{
    int rule_id = -1;
    UTOPIA_GETINDEXED(ctx, UtopiaValue_SinglePortForward, index, s_tokenbuf, sizeof(s_tokenbuf));
    if (sscanf(s_tokenbuf, "spf_%d", &rule_id) == 1)
    {
        return rule_id;
    }
    else
    {
        return -1;
    }
}

static int s_setportfwd_ruleid (UtopiaContext *ctx, int index, int rule_id)
{
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "spf_%d", rule_id);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SinglePortForward, index, s_tokenbuf);
    return SUCCESS;
}

static int s_getportfwd_index (UtopiaContext *ctx, int rule_id, int count)
{
    int i;
    for (i = 1; i <= count; i++)
    {
        if (s_getportfwd_ruleid(ctx, i)==rule_id)
        {
            return i;
        }
    }
    return -1;
}

static int s_getportfwd (UtopiaContext *ctx, int index, portFwdSingle_t *portmap)
{
	boolean_t TempprevRuleEnabledState = FALSE;
	
    Utopia_GetIndexedBool(ctx, UtopiaValue_SPF_Enabled, index, &portmap->enabled);

	/* 
	  * Check whether previous enable state variable available in syscfg DB 
	  * file or not. If not available then create DB instance store this value
	  */
	if( SUCCESS == Utopia_GetIndexedBool(ctx, UtopiaValue_SPF_PrevRuleEnabledState, index, &TempprevRuleEnabledState) )
	{
		portmap->prevRuleEnabledState = TempprevRuleEnabledState;
	}
	else
	{
	   portmap->prevRuleEnabledState = portmap->enabled;
	   UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_SPF_PrevRuleEnabledState, index, portmap->prevRuleEnabledState);
	}

    Utopia_GetIndexed(ctx, UtopiaValue_SPF_Name, index, portmap->name, NAME_SZ);
    Utopia_GetIndexedInt(ctx, UtopiaValue_SPF_ExternalPort, index, &portmap->external_port);
    Utopia_GetIndexedInt(ctx, UtopiaValue_SPF_InternalPort, index, &portmap->internal_port);
//    Utopia_GetIndexedInt(ctx, UtopiaValue_SPF_ToIp, index, &portmap->dest_ip);
    Utopia_GetIndexed(ctx, UtopiaValue_SPF_ToIp, index, &portmap->dest_ip, DEST_NAME_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_SPF_ToIpV6, index, &portmap->dest_ipv6, IPADDR_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_SPF_Protocol, index, s_tokenbuf, sizeof(s_tokenbuf));
    Utopia_GetIndexed(ctx, UtopiaValue_SPF_RemoteHost, index, &portmap->remotehost, IPADDR_SZ);
    portmap->protocol = s_StrToEnum(g_ProtocolMap, s_tokenbuf);
    
    return SUCCESS;
}

static int s_setportfwd (UtopiaContext *ctx, int index, portFwdSingle_t *portmap)
{
    UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_SPF_Enabled, index, portmap->enabled);
    UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_SPF_PrevRuleEnabledState, index,portmap->prevRuleEnabledState );	
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SPF_Name, index, portmap->name);
    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_SPF_ExternalPort, index, portmap->external_port);
    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_SPF_InternalPort, index, portmap->internal_port);
//    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_SPF_ToIp, index, portmap->dest_ip);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SPF_ToIp, index,portmap->dest_ip);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SPF_ToIpV6, index,portmap->dest_ipv6);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SPF_RemoteHost, index,portmap->remotehost);
    char *p = s_EnumToStr(g_ProtocolMap, portmap->protocol);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_SPF_Protocol, index, p);
    
    return SUCCESS;
}

static int s_unsetportfwd (UtopiaContext *ctx, int index)
{
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_Enabled, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_PrevRuleEnabledState, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_Name, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_ExternalPort, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_InternalPort, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_ToIp, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_ToIpV6, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_Protocol, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SPF_RemoteHost, index);

    return SUCCESS;
}

int Utopia_SetPortForwarding (UtopiaContext *ctx, int count, portFwdSingle_t *fwdinfo)
{
    int i, old_count;

    for (i=0;i<count;i++)
    {
        if (fwdinfo[i].rule_id==0)
        {
            fwdinfo[i].rule_id = i+1; // maintain legacy behavior for backward compatibility
        }
        if (fwdinfo[i].rule_id < 0)
        {
            return ERR_INVALID_VALUE;
        }
       /* if(!IsValid_IPAddr(fwdinfo[i].dest_ip))
        {
            return ERR_INVALID_IP;       
        }*/
    }

    Utopia_GetPortForwardingCount(ctx, &old_count);

    for (i = 0; i < old_count; i++)
    {
        s_unsetportfwd(ctx, i+1);
        UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SinglePortForward, i+1);
    }

    for (i = 0; i < count; i++) {
        s_setportfwd_ruleid(ctx, i+1, fwdinfo[i].rule_id);
        s_setportfwd(ctx, i+1, &fwdinfo[i]);
    }
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_SPFCount, count);

    return SUCCESS;
}

int Utopia_AddPortForwarding (UtopiaContext *ctx, portFwdSingle_t *portmap)
{
    int count = 0;

    Utopia_GetInt(ctx, UtopiaValue_Firewall_SPFCount, &count);

    if (portmap->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }
    /*else if(!IsValid_IPAddr(portmap->dest_ip))
    {
        return ERR_INVALID_IP;       
    }*/
    else if (portmap->rule_id == 0)
    {
        s_setportfwd_ruleid(ctx, count+1, count+1); // maintain legacy behavior for backware compatibility
        s_setportfwd(ctx, count+1, portmap);
    }
    else
    {
        int i, j;
        for (i=0;i<count;i++)
        {
            int rule_id = s_getportfwd_ruleid(ctx, i+1);
            if (rule_id > portmap->rule_id) break; // find the place to insert
            if (rule_id == portmap->rule_id)
            {
                return ERR_INVALID_VALUE; // rule_id must be unique
            }
        }

        for (j=count-1;j>=i;j--)
        { 
            // shift rule to next spot to make room for new rule
            int rule_id = s_getportfwd_ruleid(ctx, j+1);
            s_setportfwd_ruleid(ctx, j+2, rule_id);
        }

        s_setportfwd_ruleid(ctx, i+1, portmap->rule_id);
        s_setportfwd(ctx, i+1, portmap);
    }

    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_SPFCount, count+1);

    return SUCCESS;
}

int Utopia_GetPortForwardingCount (UtopiaContext *ctx, int *count)
{
    *count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_SPFCount, count);
    return SUCCESS;
}

/*
 * Caller-need-to-free
 * Since we malloc buffer here, don't use macro get method (it won't free buffer)
 */
int Utopia_GetPortForwarding (UtopiaContext *ctx, int *count, portFwdSingle_t **fwdinfo)
{
    int i;
    portFwdSingle_t *portmap;

    Utopia_GetInt(ctx, UtopiaValue_Firewall_SPFCount, count);
    if (*count <= 0) {
        return SUCCESS;
    }

    portmap = (portFwdSingle_t *) malloc(sizeof(portFwdSingle_t) * (*count));
    if (NULL == portmap) {
        return ERR_INSUFFICIENT_MEM;
    }
    bzero(portmap, sizeof(portFwdSingle_t) * (*count));

    for (i = 0; i < *count; i++) {
        portmap[i].rule_id = s_getportfwd_ruleid(ctx, i+1);
        s_getportfwd(ctx, i+1, &portmap[i]);
    }
    
    *fwdinfo = portmap;
    return SUCCESS;
}

/*
 * Utility routine to find a portmap entry given external port and protocol
 * Returns -1 if entry not found
 *         0 - count-1, index of entry found
 */
int Utopia_FindPortForwarding (int count, portFwdSingle_t *portmap, int external_port, protocol_t proto)
{
    int i;

    for (i = 0; i < count; i++) {
        if (portmap[i].external_port == external_port &&
            portmap[i].protocol == proto) {
            return i;
        }
    }
    return -1;
}

int Utopia_GetPortForwardingByIndex (UtopiaContext *ctx, int index, portFwdSingle_t *fwdinfo)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_SPFCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }
    fwdinfo->rule_id = s_getportfwd_ruleid(ctx, index+1);
    s_getportfwd(ctx, index+1, fwdinfo);
    return SUCCESS;
}

int Utopia_SetPortForwardingByIndex (UtopiaContext *ctx, int index, portFwdSingle_t *fwdinfo)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_SPFCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }
    s_setportfwd(ctx, index+1, fwdinfo);
    return SUCCESS;
}

int Utopia_DelPortForwardingByIndex (UtopiaContext *ctx, int index)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_SPFCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_unsetportfwd(ctx, index+1);
    for (;index+1<count;index++)
    {
        // shift to fill the empty spot
        int tmp_rule_id = s_getportfwd_ruleid(ctx, index+2);
        s_setportfwd_ruleid(ctx, index+1, tmp_rule_id);
    }

    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SinglePortForward, count);
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_SPFCount, count-1);

    return SUCCESS;
}

int Utopia_GetPortForwardingByRuleId (UtopiaContext *ctx, portFwdSingle_t *fwdinfo)
{
    int index, count = 0;

    if (fwdinfo->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }

    Utopia_GetPortForwardingCount (ctx, &count);
    if ((index = s_getportfwd_index (ctx, fwdinfo->rule_id, count)) < 0)
    {
        return ERR_INVALID_VALUE;
    }

    s_getportfwd(ctx, index, fwdinfo);

    return SUCCESS;
}

int Utopia_SetPortForwardingByRuleId (UtopiaContext *ctx, portFwdSingle_t *fwdinfo)
{
    int index, count=0;

    if (fwdinfo->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }

    Utopia_GetPortForwardingCount (ctx, &count);
    if ((index = s_getportfwd_index (ctx, fwdinfo->rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_setportfwd(ctx, index, fwdinfo);

    return SUCCESS;
}

int Utopia_DelPortForwardingByRuleId (UtopiaContext *ctx, int rule_id)
{
    int index, count = 0;
    portFwdSingle_t  singleInfo = {0};
    if (rule_id < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    Utopia_GetPortForwardingCount (ctx, &count);

    if ((index = s_getportfwd_index (ctx, rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    // found
    singleInfo.rule_id = rule_id;
    Utopia_GetPortForwardingByRuleId(ctx, &singleInfo);
    if (removeConntrackRules(singleInfo.protocol, singleInfo.internal_port, singleInfo.dest_ip) != SUCCESS) {
        sprintf(ulog_msg, "%s: removeConntrackRules() returned failure.", __FUNCTION__);
        ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    s_unsetportfwd(ctx, index);
    for (;index<count;index++)
    {
        // shift to fill the empty spot
        int tmp_rule_id = s_getportfwd_ruleid(ctx, index+1);
        s_setportfwd_ruleid(ctx, index, tmp_rule_id);
    }
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_SinglePortForward, count);
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_SPFCount, count-1);
    return SUCCESS;
}

/*
 * Dynamic Port Forwarding
 *   doesn't goes into persistent system configuration (i.e. doesn't survive reboot)
 *   aged out by lease-time
 *
 * sysevent space layout for dynamic port forwarding
 *
 * sysevent set portmap_dyn_count n
 * unique=`sysevent set portmap_dyn_pool  "<enabled|disabled>,externalhost-ip,external_port,internalhost-ip,internal-port,protocol,leaseduration-in-secs,last_update_in_epoch_time,friendly-name"`
 * sysevent set portmap_dyn_x $unique
 *
 * Usage:
 *     1 - to - n entries of portmap_dyn_x, no holes due to delete or edit
 *     setunique portmap_dyn_pool is used for firewall convenience
 *     internally maintain a map of portmap_dyn_x <> setunique tuple name
 *     "none" is used for optional empty parameters (e.g. externalhost-ip)
 *     leaseduration is in secs; 0 indicates the mapping is valid until next reboot or explicitly removed
 */

static void ulog_pmap (const char *prefix, portMapDyn_t *pmap)
{
    char value[1024];
    snprintf(value, sizeof(value), "%s: %s,%s,%d,%s,%d,%s,%d,%ld,%s", 
                               prefix,
                               (TRUE == pmap->enabled) ? "enabled" : "disabled",
                               (strlen(pmap->external_host) == 0) ? "none" : pmap->external_host,
                               pmap->external_port,
                               pmap->internal_host,
                               pmap->internal_port,
                               (TCP == pmap->protocol) ? "tcp" : "udp",
                               pmap->lease,
                               (long)(pmap->last_updated),
                               pmap->name);
    ulog(ULOG_CONFIG, UL_UTAPI, value);
}

static int s_get_portmapdyn_count ()
{
    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    int count = 0;
    char buf[128] = {0};
    sysevent_get(se_fd, se_token, "portmap_dyn_count", buf, sizeof(buf));
    if (*buf) {
        count = atoi(buf);
    }

    return count;
}

static int s_set_portmapdyn_count (int count)
{
    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    char val[32];
    snprintf(val, sizeof(val), "%d", count);
    sysevent_set(se_fd, se_token, "portmap_dyn_count", val, 0);

    sprintf(ulog_msg, "%s: set count %d", __FUNCTION__, count);
    ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);

    return UT_SUCCESS;
}

// Add dynamic port mapping at "index" slot
static int s_add_portmapdyn (int index, portMapDyn_t *pmap)
{
    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    char value[1024], param[128], unique[128];
    time(&pmap->last_updated);
    snprintf(value, sizeof(value), "%s,%s,%d,%s,%d,%s,%d,%ld,%s", 
                               (TRUE == pmap->enabled) ? "enabled" : "disabled",
                               (strlen(pmap->external_host) == 0) ? "none" : pmap->external_host,
                               pmap->external_port,
                               pmap->internal_host,
                               pmap->internal_port,
                               (TCP == pmap->protocol) ? "tcp" : "udp",
                               pmap->lease,
                               (long)(pmap->last_updated),
                               pmap->name);

#if 0
    sysevent_get(se_fd, se_token, param, unique, sizeof(unique));

    if (0 == *unique || 0 == strcasecmp(unique, "none")) {
        // add new unique entry
        sysevent_set_unique(se_fd, se_token, "portmap_dyn_pool", value, unique, sizeof(unique));
        sysevent_set(se_fd, se_token, param, unique, 0);
        ulogf(ULOG_CONFIG, UL_UTAPI, "%s: add entry (index %d): create new unique entry %s", __FUNCTION__, index, unique);
    } else {
        // overwrite existing unique entry
        ulogf(ULOG_CONFIG, UL_UTAPI, "%s: add entry (index %d): overwrite unique entry %s", __FUNCTION__, index, unique);
        sysevent_set(se_fd, se_token, unique, value, 0);
    }
#else
    snprintf(param, sizeof(param), "portmap_dyn_%d", index);
	sysevent_set(se_fd, se_token, param, value, 0);
	ulogf(ULOG_CONFIG, UL_UTAPI, "%s: add entry (index %d): add/overwrite entry param %s value:%s", __FUNCTION__, index, 
							param,
							value);
#endif /* 0 */

    return UT_SUCCESS;
}

static int s_del_portmapdyn (int index)
{
    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    ulogf(ULOG_CONFIG, UL_UTAPI, "%s: entry %d", __FUNCTION__, index);

    char buf[1024], param[128], unique[128] = {0};
    int i, count;

    count = s_get_portmapdyn_count();
    if (index < 1 || index > count) {
        return ERR_INVALID_ARGS;
    }

    // deleting 'index' entries' referenced value
    snprintf(param, sizeof(param), "portmap_dyn_%d", index);
    sysevent_get(se_fd, se_token, param, unique, sizeof(unique));
    if (strlen(unique) > 0 && 0 != strcasecmp(unique, "none")) {
		sysevent_set(se_fd, se_token, param, "none", 0);
    }

    // bubble swap the references after 'index'th entry
    if (index < count) {
        for (i = index; i < count ; i++) {
            snprintf(param, sizeof(param), "portmap_dyn_%d", i+1);
            sysevent_get(se_fd, se_token, param, buf, sizeof(buf));
            snprintf(param, sizeof(param), "portmap_dyn_%d", i);
            sysevent_set(se_fd, se_token, param, buf, 0);
            ulogf(ULOG_CONFIG, UL_UTAPI, "%s: copy from portmap_dyn_%d to portmap_dyn_%d rule entry %s", __FUNCTION__, i+1, i, buf);
        }
    }

    // deleting last entry
    snprintf(param, sizeof(param), "portmap_dyn_%d", count);
    sysevent_set(se_fd, se_token, param, "none", 0);
    ulogf(ULOG_CONFIG, UL_UTAPI, "%s: delete entry %s", __FUNCTION__, param);

    return UT_SUCCESS;
}



static int s_get_portmapdyn (int index, portMapDyn_t *portmap)
{
    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    char buf[1024], param[128], unique[128] = {0}, *p, *next;

    snprintf(param, sizeof(param), "portmap_dyn_%d", index);
#if 0
    sysevent_get(se_fd, se_token, param, unique, sizeof(unique));
    if (0 == *unique || 0 == strcasecmp(unique, "none")) {
        return ERR_ITEM_NOT_FOUND;
    }
#endif

    sysevent_get(se_fd, se_token, param, buf, sizeof(buf));
    // ulogf(ULOG_CONFIG, UL_UTAPI, "%s: retrieving entry (index %d) using unique %s", __FUNCTION__, index, unique);
    // ulogf(ULOG_CONFIG, UL_UTAPI, "%s: processing entry (index %d) %s", __FUNCTION__, index, buf);

    // parse portmap rule into data structure
    p = buf;
    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    portmap->enabled = (0 == strcasecmp(p,"enabled")) ? TRUE : FALSE;
    p = next;

    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    if (0 != strcasecmp(p, "none")) {
        strncpy(portmap->external_host, p, IPADDR_SZ);
    }
    p = next;

    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    portmap->external_port = atoi(p);
    p = next;

    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    strncpy(portmap->internal_host, p, IPADDR_SZ);
    p = next;

    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    portmap->internal_port = atoi(p);
    p = next;

    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    portmap->protocol = (0 == strcasecmp(p,"tcp")) ? TCP : UDP;
    p = next;

    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    portmap->lease = atoi(p);
    p = next;

    if (NULL == (next = chop_str(p,','))) {
        return ERR_INVALID_VALUE;
    }
    portmap->last_updated = (time_t)atol(p);
    p = next;

    // last entry doesn't have delimiter, nothing to chop; use as-is
    strncpy(portmap->name, p, NAME_SZ);

    ulog_debugf(ULOG_CONFIG, UL_UTAPI, "enabled %d ext host %s ext port %d int host %s int port %d lease %d name %s last update %ld",
                portmap->enabled,
                portmap->external_host,
                portmap->external_port,
                portmap->internal_host,
                portmap->internal_port,
                portmap->lease,
                portmap->name,
                portmap->last_updated);

    return UT_SUCCESS;
}

/*
 * Returns 0 if not found
 *         1 -to- count, index of matched entry if found
 */
static int s_find_portmapdyn (const char *external_host, int external_port, protocol_t proto)
{
    int i, count = s_get_portmapdyn_count();

    ulog_debugf(ULOG_CONFIG, UL_UTAPI, "%s: count %d", __FUNCTION__, count);

    for (i = 1; i <= count; i++) {
        portMapDyn_t pmap;

        bzero(&pmap, sizeof(pmap));

        if (UT_SUCCESS == s_get_portmapdyn(i, &pmap)) {
            if (external_port == pmap.external_port &&
                proto == pmap.protocol &&
                (0 == strcasecmp(external_host, pmap.external_host))) {
                sprintf(ulog_msg, "%s: found dynamic port map: entry %d (%s:%d<->%s:%d)",
                        __FUNCTION__, i, pmap.external_host, pmap.external_port, pmap.internal_host, pmap.internal_port);
                ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
                return i;
            }
        }
    }

    return 0;
}

static int s_firewall_restart ()
{
    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    ulog(ULOG_CONFIG, UL_UTAPI, "restarting firewall...");

    printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
    int rc = sysevent_set(se_fd, se_token, "firewall-restart", NULL, 0);

    ulogf(ULOG_CONFIG, UL_UTAPI, "firewall restart mesg sent, (rc %d)", rc);

    sleep(1);

    ulog(ULOG_CONFIG, UL_UTAPI, "firewall hold off done");
    return UT_SUCCESS;
}

/*
 * Returns 
 *    1 - if IGD user config is allowed
 *    0 - if disallowed
 */
int Utopia_IGDConfigAllowed (UtopiaContext *ctx)
{
    int igd_config_allow = 0;

    if (NULL == ctx) {
        return 0;
    }
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_IGDUserConfig, &igd_config_allow);

    return igd_config_allow;
}

/*
 * Returns 
 *    1 - if IGD internet-disable (using force-termination) is allowed
 *    0 - if disallowed
 */
int Utopia_IGDInternetDisbleAllowed (UtopiaContext *ctx)
{
    int igd_wandisable_allow = 0;

    if (NULL == ctx) {
        return 0;
    }
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_IGDWANDisable, &igd_wandisable_allow);

    return igd_wandisable_allow;
}


int Utopia_AddDynPortMapping (portMapDyn_t *pmap)
{
    int count, rc;

    if (NULL == pmap) {
        return ERR_INVALID_ARGS;
    }

    ulog_pmap(__FUNCTION__, pmap);

    count = s_get_portmapdyn_count();
    if (UT_SUCCESS == (rc = s_add_portmapdyn(count+1, pmap))) {
        s_set_portmapdyn_count(count+1);

/* Instead  firewall restart we have to add explicit rule */
#if 0
        s_firewall_restart();
#else
		Utopia_IPRule_ephemeral_port_forwarding( pmap, TRUE );
#endif /* 0 */
        return UT_SUCCESS;
    }

    return rc;
}

int Utopia_UpdateDynPortMapping_WithoutFirewallRestart (int index, portMapDyn_t *pmap)
{


    int count, rc;

    ulog_pmap(__FUNCTION__, pmap);

    count = s_get_portmapdyn_count();
    if (index < 1 || index > count) {
        return ERR_INVALID_ARGS;
    }

    if (UT_SUCCESS == (rc = s_add_portmapdyn(index, pmap))) {
        return UT_SUCCESS;
    }

    return rc;
}

int Utopia_UpdateDynPortMapping (int index, portMapDyn_t *pmap)
{
    int count, rc;

    ulog_pmap(__FUNCTION__, pmap);

    count = s_get_portmapdyn_count();
    if (index < 1 || index > count) {
        return ERR_INVALID_ARGS;
    }

    if (UT_SUCCESS == (rc = s_add_portmapdyn(index, pmap))) {
        // note, don't increment count, this is just an "update" to existing entry
/* Instead firewall restart we have to delete and add explicit rule */
#if 0
		s_firewall_restart();
#else
		Utopia_IPRule_ephemeral_port_forwarding( pmap, FALSE );
		Utopia_IPRule_ephemeral_port_forwarding( pmap, TRUE );
#endif /* 0 */

        return UT_SUCCESS;
    }

    return rc;
}


int Utopia_DeleteDynPortMappingIndex (int index)
{
    int count, rc, rc_del;
    portMapDyn_t pmap;

    ulogf(ULOG_CONFIG, UL_UTAPI, "%s: at index %d", __FUNCTION__, index);

    count = s_get_portmapdyn_count();
    if (index < 1 || index > count) {
        return ERR_INVALID_ARGS;
    }

	/* Get the corresponding port mapping information */
    bzero(&pmap, sizeof(pmap));
    rc_del = s_get_portmapdyn(index, &pmap); 

    if (UT_SUCCESS == (rc = s_del_portmapdyn(index))) {
        s_set_portmapdyn_count(count-1);

/* Instead firewall restart we have to delete explicit rule */
#if 0
		s_firewall_restart();
#else
		if( UT_SUCCESS == rc_del ) 
		{
			Utopia_IPRule_ephemeral_port_forwarding( &pmap, FALSE );
		}
#endif /* 0 */
    }

    return rc;
}

int Utopia_DeleteDynPortMapping (portMapDyn_t *pmap)
{
    int index, rc;

    ulog_pmap(__FUNCTION__, pmap);

    if ((index = s_find_portmapdyn(pmap->external_host, pmap->external_port, pmap->protocol))) {
        ulog_debugf(ULOG_CONFIG, UL_UTAPI, "%s: found entry at index %d", __FUNCTION__, index);
        if (UT_SUCCESS == (rc = Utopia_DeleteDynPortMappingIndex(index))) {
            // Note: Utopia_DeleteDynPortMappingIndex  does a firewall_restart
            //       no need to so firewall-restart here
	    ulog_debugf(ULOG_CONFIG, UL_UTAPI, "%s: delete entry at index %d success!", __FUNCTION__, index);
            return rc;
        }
        ulog_debugf(ULOG_CONFIG, UL_UTAPI, "%s: delete entry at index %d failed (rc %d)", __FUNCTION__, index, rc);
    } else {
        rc = ERR_ITEM_NOT_FOUND;
    }

    return rc;
}

int Utopia_GetDynPortMappingCount (int *count)
{
    *count = s_get_portmapdyn_count();
    return UT_SUCCESS;
}

int Utopia_GetDynPortMapping (int index, portMapDyn_t *portmap)
{
    return s_get_portmapdyn(index, portmap);
}

int Utopia_FindDynPortMapping(const char *external_host, int external_port, protocol_t proto, portMapDyn_t *pmap, int *index)
{
   if ((*index = s_find_portmapdyn(external_host, external_port, proto))) {
       s_get_portmapdyn(*index, pmap);
       return UT_SUCCESS;
   }

   return ERR_ITEM_NOT_FOUND;
}

/*
 * Update last_updated time
 */
int Utopia_ValidateDynPortMapping (int index)
{
    portMapDyn_t pmap;

    bzero(&pmap, sizeof(pmap));
    if (UT_SUCCESS == s_get_portmapdyn(index, &pmap)) {
        time(&pmap.last_updated);
        s_add_portmapdyn(index, &pmap);
    }

    return UT_SUCCESS;
}

// Remove entries whose lease time expired
// note, this check is valid only on entries that have leasetime > 0
// if leastime = 0, the entry is left indefinitely until it is 
// explicitly deleted or system reboots
int Utopia_InvalidateDynPortMappings (void)
{
    int i, count = s_get_portmapdyn_count();
    for (i = 1; i <= count; i++) {
        portMapDyn_t pmap;
        bzero(&pmap, sizeof(pmap));
        if (UT_SUCCESS == s_get_portmapdyn(i, &pmap)) {
// Lease is not unlimited , commenting out below check
/*            if (0 == pmap.lease) {
                continue;
            } */
            // ulogf(ULOG_CONFIG, UL_UTAPI, "dynamic port map: time check(%d): last_update+lease %ld, curtime %ld", i, (long)(pmap.last_updated + pmap.lease), (long)curtime);

            pmap.lease -= 3600; /*30 - Modified invalidate iterations once in one hour instead previous iteration */
            if (pmap.lease <= 0){
                ulogf(ULOG_CONFIG, UL_UTAPI, "dynamic port map: lease time expired for entry %d (%s:%d<->%s:%d)",
                        i, pmap.external_host, pmap.external_port, pmap.internal_host, pmap.internal_port);
            printf("dynamic port map: lease time expired for entry %d (%s:%d<->%s:%d)\n",
	i, pmap.external_host, pmap.external_port, pmap.internal_host, pmap.internal_port);

                Utopia_DeleteDynPortMappingIndex(i);
                //s_firewall_restart();
            }else
                s_add_portmapdyn(i, &pmap);    /* decrement leaseDuration every second */
        }
    }

    return UT_SUCCESS;
}

/*
 * Port Forwarding by range
 */
static int s_getportfwdrange_ruleid (UtopiaContext *ctx, int index)
{
    int rule_id = -1;
    UTOPIA_GETINDEXED(ctx, UtopiaValue_PortRangeForward, index, s_tokenbuf, sizeof(s_tokenbuf));
    if (sscanf(s_tokenbuf, "pfr_%d", &rule_id) == 1)
    {
        return rule_id;
    }
    else
    {
        return -1;
    }
}

static int s_setportfwdrange_ruleid (UtopiaContext *ctx, int index, int rule_id)
{
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "pfr_%d", rule_id);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PortRangeForward, index, s_tokenbuf);
    return SUCCESS;
}

static int s_getportfwdrange_index (UtopiaContext *ctx, int rule_id, int count)
{
    int i;
    for (i = 1; i <= count; i++)
    {
        if (s_getportfwdrange_ruleid(ctx, i)==rule_id)
        {
            return i;
        }
    }
    return -1;
}

static int s_getportfwdrange (UtopiaContext *ctx, int index, portFwdRange_t *portmap)
{
    char *last, port_range[32];
	boolean_t TempprevRuleEnabledState = FALSE;

    portmap->internal_port_range_size = 0;

    Utopia_GetIndexedBool(ctx, UtopiaValue_PFR_Enabled, index, &portmap->enabled);

	/* 
	  * Check whether previous enable state variable available in syscfg DB 
	  * file or not. If not available then create DB instance store this value
	  */
	if( SUCCESS == Utopia_GetIndexedBool(ctx, UtopiaValue_PFR_PrevRuleEnabledState, index, &TempprevRuleEnabledState) )
	{
	   portmap->prevRuleEnabledState = TempprevRuleEnabledState;
	}
	else
	{
	   portmap->prevRuleEnabledState = portmap->enabled;
	   UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_PFR_PrevRuleEnabledState, index, portmap->prevRuleEnabledState);
	}

    Utopia_GetIndexed(ctx, UtopiaValue_PFR_Name, index, portmap->name, NAME_SZ);
    Utopia_GetIndexedInt(ctx, UtopiaValue_PFR_InternalPort, index, &portmap->internal_port);
    Utopia_GetIndexedInt(ctx, UtopiaValue_PFR_InternalPortRangeSize, index, &portmap->internal_port_range_size);
    //Utopia_GetIndexedInt(ctx, UtopiaValue_PFR_ToIp, index, &portmap->dest_ip);
    Utopia_GetIndexed(ctx, UtopiaValue_PFR_ToIp, index, &portmap->dest_ip, DEST_NAME_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_PFR_ToIpV6, index, &portmap->dest_ipv6, IPADDR_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_PFR_PublicIp, index, &portmap->public_ip, IPADDR_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_PFR_Protocol, index, s_tokenbuf, sizeof(s_tokenbuf));
    Utopia_GetIndexed(ctx, UtopiaValue_PFR_RemoteHost, index, &portmap->remotehost, IPADDR_SZ);

    portmap->protocol = s_StrToEnum(g_ProtocolMap, s_tokenbuf);

    *port_range = '\0';
    Utopia_GetIndexed(ctx, UtopiaValue_PFR_ExternalPortRange, index, port_range, sizeof(port_range));
    if ('\0' != *port_range) {
        if (NULL == (last = chop_str(port_range,' '))) {
            return ERR_INVALID_PORT_RANGE;
        }
        portmap->start_port = atoi(port_range);
        portmap->end_port = atoi(last);
    }
    
    return SUCCESS;
}

static int s_setportfwdrange (UtopiaContext *ctx, int index, portFwdRange_t *portmap)
{
    char port_range[32];

    UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_PFR_Enabled, index, portmap->enabled);
    UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_PFR_PrevRuleEnabledState, index, portmap->prevRuleEnabledState );
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PFR_Name, index, portmap->name);
    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_PFR_InternalPort, index, portmap->internal_port);
    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_PFR_InternalPortRangeSize, index, portmap->internal_port_range_size);
    //UTOPIA_SETINDEXEDINT(c, so we have to get the value each ti*/
    //  tx, UtopiaValue_PFR_ToIp, index, portmap->dest_ip);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PFR_ToIp, index, portmap->dest_ip);
    if(portmap->public_ip[0] == 0)
    {
        UTOPIA_SETINDEXED(ctx,UtopiaValue_PFR_PublicIp, index, "0.0.0.0");
    }
    else
    {
        UTOPIA_SETINDEXED(ctx, UtopiaValue_PFR_PublicIp, index, portmap->public_ip);
    }
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PFR_ToIpV6, index, portmap->dest_ipv6);
    char *p = s_EnumToStr(g_ProtocolMap, portmap->protocol);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PFR_Protocol, index, p);
    snprintf(port_range, sizeof(port_range), "%d %d", portmap->start_port, portmap->end_port);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PFR_ExternalPortRange, index, port_range);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PFR_RemoteHost, index,portmap->remotehost);
    
    return SUCCESS;
}

static int s_unsetportfwdrange (UtopiaContext *ctx, int index)
{
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_Enabled, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_PrevRuleEnabledState, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_Name, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_InternalPort, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_InternalPortRangeSize, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_ToIp, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_PublicIp, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_ToIpV6, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_Protocol, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_ExternalPortRange, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PFR_RemoteHost, index);
    
    return SUCCESS;
}

int Utopia_SetPortForwardingRange (UtopiaContext *ctx, int count, portFwdRange_t *fwdinfo)
{
    int i, old_count;

    for (i=0; i<count; i++)
    {
        if (fwdinfo[i].rule_id == 0)
        {
            fwdinfo[i].rule_id = i+1;
        }
        else if (fwdinfo[i].rule_id < 0)
        {
            return ERR_INVALID_VALUE;
        }
    /*    if(!IsValid_IPAddr(fwdinfo[i].dest_ip))
        {
            return ERR_INVALID_IP;       
        }
        if(fwdinfo[i].public_ip[0] != 0 && !IsValid_IPAddr(fwdinfo[i].public_ip))
        {
            return ERR_INVALID_IP;       
        }*/
    }

    Utopia_GetInt(ctx, UtopiaValue_Firewall_PFRCount, &old_count);

    for (i=0; i<old_count; i++)
    {
        s_unsetportfwdrange(ctx, i+1);
        UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PortRangeForward, i+1);
    }

    for(i = 0; i < count; i++) {
        s_setportfwdrange_ruleid(ctx, i+1, fwdinfo[i].rule_id);
        s_setportfwdrange(ctx, i+1, &fwdinfo[i]);
    }

    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PFRCount, count);

    return SUCCESS;
}

int Utopia_GetPortForwardingRangeCount (UtopiaContext *ctx, int *count)
{
    *count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PFRCount, count);
    return SUCCESS;
}

/*
 * Caller-need-to-free
 * Since we malloc buffer here, don't use macro get method (it won't free buffer)
 */
int Utopia_GetPortForwardingRange (UtopiaContext *ctx, int *count, portFwdRange_t **fwdinfo)
{
    int i;
    portFwdRange_t *portmap;

    Utopia_GetInt(ctx, UtopiaValue_Firewall_PFRCount, count);
    if (*count <= 0) {
        return SUCCESS;
    }

    portmap = (portFwdRange_t *) malloc(sizeof(portFwdRange_t) * (*count));
    if (NULL == portmap) {
        return ERR_INSUFFICIENT_MEM;
    }
    bzero(portmap, sizeof(portFwdRange_t) * (*count));

    for (i = 0; i < *count; i++) {
        portmap[i].rule_id = s_getportfwdrange_ruleid(ctx, i+1);
        s_getportfwdrange(ctx, i+1, &portmap[i]);
    }
    
    *fwdinfo = portmap;
    return SUCCESS;
}

int Utopia_AddPortForwardingRange (UtopiaContext *ctx, portFwdRange_t *portmap)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PFRCount, &count);

    if (portmap->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }
    else if (portmap->rule_id == 0)
    {
        s_setportfwdrange_ruleid(ctx, count+1, count+1);
        s_setportfwdrange(ctx, count+1, portmap);
    }
    /*else if(!IsValid_IPAddr(portmap->dest_ip))
    {
        return ERR_INVALID_IP;       
    }
    else if(portmap->public_ip[0] != 0 && !IsValid_IPAddr(portmap->dest_ip))
    {
        return ERR_INVALID_IP;       
    }*/
    else
    {
        int i, j;
        for (i=0;i<count;i++)
        {
            int rule_id = s_getportfwdrange_ruleid(ctx, i+1);
            if (rule_id > portmap->rule_id) break; // find the place to insert
            if (rule_id == portmap->rule_id)
            {
                return ERR_INVALID_VALUE; // rule_id must be unique
            }
        }

        for (j=count-1;j>=i;j--)
        { 
            // shift rule to next spot to make room for new rule
            int rule_id = s_getportfwdrange_ruleid(ctx, j+1);
            s_setportfwdrange_ruleid(ctx, j+2, rule_id);
        }

        s_setportfwdrange_ruleid(ctx, i+1, portmap->rule_id);
        s_setportfwdrange(ctx, i+1, portmap);
    }

    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PFRCount, count+1);

    return SUCCESS;
}

int Utopia_GetPortForwardingRangeByIndex (UtopiaContext *ctx, int index, portFwdRange_t *fwdinfo)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PFRCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }
    fwdinfo->rule_id = s_getportfwdrange_ruleid(ctx, index+1);
    s_getportfwdrange(ctx, index+1, fwdinfo);
    return SUCCESS;
}

int Utopia_SetPortForwardingRangeByIndex (UtopiaContext *ctx, int index, portFwdRange_t *fwdinfo)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PFRCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }
    s_setportfwdrange(ctx, index+1, fwdinfo);
    return SUCCESS;
}

int Utopia_DelPortForwardingRangeByIndex (UtopiaContext *ctx, int index)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PFRCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_unsetportfwdrange(ctx, index+1);
    for (;index+1<count;index++)
    {
        // shift to fill the empty spot
        int tmp_rule_id = s_getportfwdrange_ruleid(ctx, index+2);
        s_setportfwdrange_ruleid(ctx, index+1, tmp_rule_id);
    }

    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PortRangeForward, count);
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PFRCount, count-1);

    return SUCCESS;
}

int Utopia_GetPortForwardingRangeByRuleId (UtopiaContext *ctx, portFwdRange_t *fwdinfo)
{
    int index, count = 0;

    if (fwdinfo->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }

    Utopia_GetPortForwardingRangeCount(ctx, &count);
    if ((index = s_getportfwdrange_index(ctx, fwdinfo->rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_getportfwdrange(ctx, index, fwdinfo);

    return SUCCESS;
}

int Utopia_SetPortForwardingRangeByRuleId (UtopiaContext *ctx, portFwdRange_t *fwdinfo)
{
    int index, count = 0;

    if (fwdinfo->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }

    Utopia_GetPortForwardingRangeCount(ctx, &count);
    if ((index = s_getportfwdrange_index(ctx, fwdinfo->rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_setportfwdrange(ctx, index, fwdinfo);

    return SUCCESS;
}


int Utopia_DelPortForwardingRangeByRuleId (UtopiaContext *ctx, int rule_id)
{
    int index, count = 0;
    portFwdRange_t  rangeInfo = {0};
    if (rule_id < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    Utopia_GetPortForwardingRangeCount(ctx, &count);
    if ((index = s_getportfwdrange_index(ctx, rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    // found
    rangeInfo.rule_id = rule_id;
    Utopia_GetPortForwardingRangeByRuleId(ctx, &rangeInfo);
    if (removeConntrackRules(rangeInfo.protocol, rangeInfo.internal_port, rangeInfo.dest_ip) != SUCCESS) {
        sprintf(ulog_msg, "%s: removeConntrackRules() returned failure.", __FUNCTION__);
        ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    s_unsetportfwdrange(ctx, index);
    for (;index<count;index++)
    {
        // shift to fill the empty spot
        int tmp_rule_id = s_getportfwdrange_ruleid(ctx, index+1);
        s_setportfwdrange_ruleid(ctx, index, tmp_rule_id);
    }
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PortRangeForward, count);
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PFRCount, count-1);
    return SUCCESS;
}

static int s_getporttrigger_ruleid (UtopiaContext *ctx, int index)
{
    int rule_id = -1;
    UTOPIA_GETINDEXED(ctx, UtopiaValue_PortRangeTrigger, index, s_tokenbuf, sizeof(s_tokenbuf));
    if (sscanf(s_tokenbuf, "prt_%d", &rule_id) == 1)
    {
        return rule_id;
    }
    else
    {
        return -1;
    }
}

static int s_setporttrigger_ruleid (UtopiaContext *ctx, int index, int rule_id)
{
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "prt_%d", rule_id);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PortRangeTrigger, index, s_tokenbuf);
    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_PRT_TriggerID, index, rule_id);
    return SUCCESS;
}

static int s_getporttrigger_index (UtopiaContext *ctx, int rule_id, int count)
{
    int i;
    for (i = 1; i <= count; i++)
    {
        if (s_getporttrigger_ruleid(ctx, i)==rule_id)
        {
            return i;
        }
    }
    return -1;
}

static int s_getporttrigger (UtopiaContext *ctx, int index, portRangeTrig_t *map)
{
    char *last, port_range[32];
    boolean_t  TempprevRuleEnabledState = FALSE;	

    Utopia_GetIndexedBool(ctx, UtopiaValue_PRT_Enabled, index, &map->enabled);

	/* 
	  * Check whether previous enable state variable available in syscfg DB 
	  * file or not. If not available then create DB instance store this value
	  */
	if( SUCCESS == Utopia_GetIndexedBool(ctx, UtopiaValue_PRT_PrevRuleEnabledState, index, &TempprevRuleEnabledState) )
	{
	   map->prevRuleEnabledState = TempprevRuleEnabledState;
	}
	else
	{
	   map->prevRuleEnabledState = map->enabled;
	   UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_PRT_PrevRuleEnabledState, index, map->prevRuleEnabledState);
	}

    Utopia_GetIndexed(ctx, UtopiaValue_PRT_Name, index, map->name, NAME_SZ);

    Utopia_GetIndexed(ctx, UtopiaValue_PRT_TriggerProtocol, index, s_tokenbuf, sizeof(s_tokenbuf));
    map->trigger_proto = s_StrToEnum(g_ProtocolMap, s_tokenbuf);

    Utopia_GetIndexed(ctx, UtopiaValue_PRT_ForwardProtocol, index, s_tokenbuf, sizeof(s_tokenbuf));
    map->forward_proto = s_StrToEnum(g_ProtocolMap, s_tokenbuf);

    *port_range = '\0';
    Utopia_GetIndexed(ctx, UtopiaValue_PRT_TriggerRange, index, port_range, sizeof(port_range));
    if ('\0' != *port_range) {
        if (NULL == (last = chop_str(port_range,' '))) {
            return ERR_INVALID_PORT_RANGE;
        }
        map->trigger_start = atoi(port_range);
        map->trigger_end = atoi(last);
    }

    *port_range = '\0';
    Utopia_GetIndexed(ctx, UtopiaValue_PRT_ForwardRange, index, port_range, sizeof(port_range));
    if ('\0' != *port_range) {
        if (NULL == (last = chop_str(port_range,' '))) {
            return ERR_INVALID_PORT_RANGE;
        }
        map->fwd_range_start = atoi(port_range);
        map->fwd_range_end = atoi(last);
    }
    
    return SUCCESS;
}

static int s_setporttrigger (UtopiaContext *ctx, int index, portRangeTrig_t *map)
{
    char *p, port_range[32];
	boolean_t TempprevRuleEnabledState;

    UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_PRT_Enabled, index, map->enabled);
    UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_PRT_PrevRuleEnabledState, index, map->prevRuleEnabledState);
	UTOPIA_SETINDEXED(ctx, UtopiaValue_PRT_Name, index, map->name);
    snprintf(port_range, sizeof(port_range), "%d %d", map->trigger_start,
                                                      map->trigger_end);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PRT_TriggerRange, index, port_range);
    snprintf(port_range, sizeof(port_range), "%d %d", map->fwd_range_start,
                                                      map->fwd_range_end);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PRT_ForwardRange, index, port_range);
    p = s_EnumToStr(g_ProtocolMap, map->trigger_proto);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PRT_TriggerProtocol, index, p);
    p = s_EnumToStr(g_ProtocolMap, map->forward_proto);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_PRT_ForwardProtocol, index, p);

    return SUCCESS;
}

static int s_unsetporttrigger (UtopiaContext *ctx, int index)
{
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_TriggerID, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_Enabled, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_PrevRuleEnabledState, index);
	UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_Name, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_TriggerRange, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_ForwardRange, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_TriggerProtocol, index);
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PRT_ForwardProtocol, index);

    return SUCCESS;
}

int Utopia_SetPortTrigger (UtopiaContext *ctx, int count, portRangeTrig_t *porttrigger)
{
    int i, old_count;

    for (i=0; i<count; i++)
    {
        if (porttrigger[i].rule_id < 0)
        {
            return ERR_INVALID_VALUE;
        }
        else if (porttrigger[i].rule_id == 0)
        {
            porttrigger[i].rule_id = i+1;
        }
    }

    Utopia_GetInt(ctx, UtopiaValue_Firewall_PRTCount, &old_count);
    for (i=0; i<old_count; i++)
    {
        s_unsetporttrigger(ctx, i+1);
        UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PortRangeTrigger, i+1);
    }

    for(i = 0; i < count; i++) {
        s_setporttrigger_ruleid(ctx, i+1, porttrigger[i].rule_id);
        s_setporttrigger(ctx, i+1, &porttrigger[i]);
    }
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PRTCount, count);

    return SUCCESS;
}


int Utopia_GetPortTrigger (UtopiaContext *ctx, int *count, portRangeTrig_t **porttrigger)
{
    int i;
    portRangeTrig_t *triggermap;

    Utopia_GetInt(ctx, UtopiaValue_Firewall_PRTCount, count);
    if (*count <= 0) {
        return SUCCESS;
    }

    triggermap = (portRangeTrig_t *) malloc(sizeof(portRangeTrig_t) * (*count));
    if (NULL == triggermap) {
        return ERR_INSUFFICIENT_MEM;
    }
    bzero(triggermap, sizeof(portRangeTrig_t) * (*count));

    for (i = 0; i < *count; i++) {
        triggermap[i].rule_id = s_getporttrigger_ruleid(ctx, i+1);
        s_getporttrigger(ctx, i+1, &triggermap[i]);
    }
    
    *porttrigger = triggermap;
    return SUCCESS;
}

int Utopia_GetPortTriggerCount (UtopiaContext *ctx, int *count)
{
    *count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PRTCount, count);
    return SUCCESS;
}

int Utopia_AddPortTrigger (UtopiaContext *ctx, portRangeTrig_t *portmap)
{
    int count = 0;

    Utopia_GetInt(ctx, UtopiaValue_Firewall_PRTCount, &count);

    if (portmap->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }
    else if (portmap->rule_id == 0)
    {
        s_setporttrigger_ruleid(ctx, count+1, count+1);
        s_setporttrigger(ctx, count+1, portmap);
    }
    else
    {
        int i, j;
        for (i=0;i<count;i++)
        {
            int rule_id = s_getporttrigger_ruleid(ctx, i+1);
            if (rule_id > portmap->rule_id) break; // find the place to insert
            else if (rule_id == portmap->rule_id)
            {
                return ERR_INVALID_VALUE; // rule_id must be unique
            }
        }

        for (j=count-1;j>=i;j--)
        { 
            // shift rule to next spot to make room for new rule
            int rule_id = s_getporttrigger_ruleid(ctx, j+1);
            s_setporttrigger_ruleid(ctx, j+2, rule_id);
        }

        s_setporttrigger_ruleid(ctx, i+1, portmap->rule_id);
        s_setporttrigger(ctx, i+1, portmap);
    }

    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PRTCount, count+1);

    return SUCCESS;
}

int Utopia_GetPortTriggerByIndex (UtopiaContext *ctx, int index, portRangeTrig_t *fwdinfo)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PRTCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }
    fwdinfo->rule_id = s_getporttrigger_ruleid(ctx, index+1);
    s_getporttrigger(ctx, index+1, fwdinfo);
    return SUCCESS;
}

int Utopia_SetPortTriggerByIndex (UtopiaContext *ctx, int index, portRangeTrig_t *fwdinfo)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PRTCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }
    s_setporttrigger(ctx, index+1, fwdinfo);
    return SUCCESS;
}

int Utopia_DelPortTriggerByIndex (UtopiaContext *ctx, int index)
{
    int count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Firewall_PRTCount, &count);
    if (index < 0 || index >= count)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_unsetporttrigger(ctx, index+1);
    for (;index+1<count;index++)
    {
        // shift to fill the empty spot
        int tmp_rule_id = s_getporttrigger_ruleid(ctx, index+2);
        s_setporttrigger_ruleid(ctx, index+1, tmp_rule_id);
    }

    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PortRangeTrigger, count);
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PRTCount, count-1);

    return SUCCESS;
}

int Utopia_GetPortTriggerByRuleId (UtopiaContext *ctx, portRangeTrig_t *porttrigger)
{
    int index, count = 0;

    if (porttrigger->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }

    Utopia_GetPortTriggerCount(ctx, &count);
    if ((index = s_getporttrigger_index(ctx, porttrigger->rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_getporttrigger(ctx, index, porttrigger);

    return SUCCESS;
}

int Utopia_SetPortTriggerByRuleId (UtopiaContext *ctx, portRangeTrig_t *porttrigger)
{
    int index, count = 0;

    if (porttrigger->rule_id < 0)
    {
        return ERR_INVALID_VALUE;
    }

    Utopia_GetPortTriggerCount(ctx, &count);
    if ((index = s_getporttrigger_index(ctx, porttrigger->rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    s_setporttrigger(ctx, index, porttrigger);

    return SUCCESS;
}

int Utopia_DelPortTriggerByRuleId (UtopiaContext *ctx, int rule_id)
{
    int index, count = 0;

    if (rule_id < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    Utopia_GetPortTriggerCount(ctx, &count);
    if ((index = s_getporttrigger_index(ctx, rule_id, count)) < 0)
    {
        return ERR_ITEM_NOT_FOUND;
    }

    // found
    s_unsetporttrigger(ctx, index);
    for (;index<count;index++)
    {
        // shift to fill the empty spot
        int tmp_rule_id = s_getporttrigger_ruleid(ctx, index+1);
        s_setporttrigger_ruleid(ctx, index, tmp_rule_id);
    }
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_PortRangeTrigger, count);
    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_PRTCount, count-1);
    return SUCCESS;
}

#define PORT_OVER_RANGE(n_s, n_e, s, e) ((((n_s) > (e)) || ((n_e) < (s))) ? 0 : 1)

int _check_port_range( UtopiaContext *ctx, int new_rule_id, int new_start, int new_end, int new_protocol, UtopiaValue utopia[5], int is_trigger){
    int count;
    int protocol;
    int start_port;
    int end_port;
    int i;
    char port_range[32];
    boolean_t enabled;
    char *last;
    char rule_id = 0;
    int (*get_ruleid_func)(UtopiaContext *ctx, int index);

    if(utopia[4] == UtopiaValue_PortRangeForward){
        get_ruleid_func = s_getportfwdrange_ruleid;
        if( is_trigger == 1)
            new_rule_id = 0;
    }else if(utopia[4] == UtopiaValue_PortRangeTrigger){
        get_ruleid_func = s_getporttrigger_ruleid;
        if( is_trigger == 0 )
            new_rule_id = 0;
    }else{
        return 1;
    }

    Utopia_GetInt(ctx, utopia[0], &count);
    for(i = 1; i <= count; i++){

        rule_id = get_ruleid_func(ctx, i);
        if(rule_id == new_rule_id)
            continue;
/* no matter whether this rule is enabled or disabled , always check the port range */
#if 0
        Utopia_GetIndexedBool(ctx, utopia[1], i, &enabled);
        if(enabled == FALSE)
            continue;
#endif
        Utopia_GetIndexed(ctx, utopia[2], i, s_tokenbuf, sizeof(s_tokenbuf));
        protocol = s_StrToEnum(g_ProtocolMap, s_tokenbuf);
        if(new_protocol == BOTH_TCP_UDP || protocol == BOTH_TCP_UDP || new_protocol == protocol ){ 
            *port_range = '\0';
            Utopia_GetIndexed(ctx, utopia[3], i, port_range, sizeof(port_range));
            if ('\0' != *port_range &&
                NULL != (last = chop_str(port_range,' '))) {
                start_port = atoi(port_range);
                end_port = atoi(last);

                if(PORT_OVER_RANGE(new_start, new_end, start_port,end_port))
                    return 1;
            }
        }
    }
    return 0;
}

int _check_single_port_range( UtopiaContext *ctx, int new_rule_id, int new_start, int new_end, int new_protocol, UtopiaValue utopia[5], int is_trigger){
    int count;
    int i;
    boolean_t enabled;
    int protocol;
    int port;
    char rule_id = 0;
    int (*get_ruleid_func)(UtopiaContext *ctx, int index);

    if(utopia[4] == UtopiaValue_SinglePortForward){
        get_ruleid_func = s_getportfwd_ruleid;
        if( is_trigger == 1){
            new_rule_id = 0;
        }    
    }else{
        return 1;
    }
    Utopia_GetInt(ctx, utopia[0] , &count);
    for(i = 1; i <= count; i++){
        rule_id = get_ruleid_func(ctx, i);
        if(rule_id == new_rule_id)
            continue;
/* no matter whether this rule is enabled or disabled , always check the port range */
#if 0
        Utopia_GetIndexedBool(ctx, utopia[1], i, &enabled);
        if(enabled == FALSE)
            continue;
#endif
        Utopia_GetIndexed(ctx, utopia[2], i, s_tokenbuf, sizeof(s_tokenbuf));
        protocol = s_StrToEnum(g_ProtocolMap, s_tokenbuf);
        
        if(new_protocol == BOTH_TCP_UDP || protocol == BOTH_TCP_UDP || new_protocol == protocol ){ 
            Utopia_GetIndexedInt(ctx, utopia[3], i, &port);
            if(PORT_OVER_RANGE(new_start, new_end, port, port)){
                return 1;
            }
        }
    }
    return 0;
}

int Utopia_CheckPortRange(UtopiaContext *ctx, int new_rule_id, int new_start, int new_end, int new_protocol, int is_trigger)
{
    UtopiaValue utopia[5];
    
    /* check range port forwarding port */
    utopia[0] = UtopiaValue_Firewall_PFRCount;
    utopia[1] = UtopiaValue_PFR_Enabled;
    utopia[2] = UtopiaValue_PFR_Protocol;
    utopia[3] = UtopiaValue_PFR_ExternalPortRange;
    utopia[4] = UtopiaValue_PortRangeForward;
    if(_check_port_range(ctx, new_rule_id, new_start, new_end, new_protocol, utopia, is_trigger)){
        return FALSE;
    }
      
    /* check single port forwarding port */        
    utopia[0] = UtopiaValue_Firewall_SPFCount;
    utopia[1] = UtopiaValue_SPF_Enabled;
    utopia[2] = UtopiaValue_SPF_Protocol;
    utopia[3] = UtopiaValue_SPF_ExternalPort;
    utopia[4] = UtopiaValue_SinglePortForward; 
    if(_check_single_port_range(ctx, new_rule_id, new_start, new_end, new_protocol, utopia, is_trigger)){
        return FALSE;
    }

    /* check port triggering Forwarding port */
    utopia[0] = UtopiaValue_Firewall_PRTCount;
    utopia[1] = UtopiaValue_PRT_Enabled;
    utopia[2] = UtopiaValue_PRT_ForwardProtocol;
    utopia[3] = UtopiaValue_PRT_ForwardRange;
    utopia[4] = UtopiaValue_PortRangeTrigger;
    if(_check_port_range(ctx, new_rule_id, new_start, new_end, new_protocol, utopia, is_trigger)){
        return FALSE;
    }


    return TRUE;
}

int Utopia_CheckPortTriggerRange(UtopiaContext *ctx, int new_rule_id, int new_start, int new_end, int new_protocol, int is_trigger)
{
    UtopiaValue utopia[5];

    /* check port triggering trigger port */
    utopia[0] = UtopiaValue_Firewall_PRTCount;
    utopia[1] = UtopiaValue_PRT_Enabled;
    utopia[2] = UtopiaValue_PRT_TriggerProtocol;
    utopia[3] = UtopiaValue_PRT_TriggerRange;
    utopia[4] = UtopiaValue_PortRangeTrigger;
    if(_check_port_range(ctx, new_rule_id, new_start, new_end, new_protocol, utopia, is_trigger)){
        return FALSE;
    }
    return TRUE;
}

int Utopia_SetDMZSettings (UtopiaContext *ctx, dmz_t dmz)
{
    if (NULL == ctx) {
        return ERR_INVALID_ARGS;
    }
    dmz_t tmpDmz = {0};
    if (Utopia_GetDMZSettings(ctx, &tmpDmz) == SUCCESS) {
        if (removeConntrackRules(-1, 0, tmpDmz.dest_ip) != SUCCESS) {
            sprintf(ulog_msg, "%s: removeConntrackRules() returned failure.", __FUNCTION__);
            ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        }
    }
    UTOPIA_SETBOOL(ctx, UtopiaValue_DMZ_Enabled, dmz.enabled);
    if (TRUE == dmz.enabled) {
        if (IsValid_IPAddr(dmz.source_ip_start) && 
            IsValid_IPAddr(dmz.source_ip_end)) {
            char src_addr_range[512];
            snprintf(src_addr_range, sizeof(src_addr_range), "%s-%s", dmz.source_ip_start, dmz.source_ip_end);
            UTOPIA_SET(ctx, UtopiaValue_DMZ_SrcAddrRange, src_addr_range);
        } else {
            UTOPIA_SET(ctx, UtopiaValue_DMZ_SrcAddrRange, "*");
        }
        UTOPIA_SET(ctx, UtopiaValue_DMZ_DstIpAddr, dmz.dest_ip);
        UTOPIA_SET(ctx, UtopiaValue_DMZ_DstMacAddr, dmz.dest_mac);
        UTOPIA_SET(ctx, UtopiaValue_DMZ_DstIpAddrV6, dmz.dest_ipv6);
    }

    return SUCCESS;
}

int Utopia_GetDMZSettings (UtopiaContext *ctx, dmz_t *out_dmz)
{
    char src_addr_range[512];

    if (NULL == ctx || NULL == out_dmz) {
        return ERR_INVALID_ARGS;
    }

    bzero(out_dmz, sizeof(dmz_t));

    Utopia_GetBool(ctx, UtopiaValue_DMZ_Enabled, &out_dmz->enabled);
    if (TRUE == out_dmz->enabled) {
        Utopia_Get(ctx, UtopiaValue_DMZ_DstIpAddrV6, out_dmz->dest_ipv6, IPADDR_SZ);
        Utopia_Get(ctx, UtopiaValue_DMZ_DstIpAddr, out_dmz->dest_ip, IPADDR_SZ);
        Utopia_Get(ctx, UtopiaValue_DMZ_DstMacAddr, out_dmz->dest_mac, MACADDR_SZ);
        Utopia_Get(ctx, UtopiaValue_DMZ_SrcAddrRange, src_addr_range, sizeof(src_addr_range));
        if (0 == strcmp(src_addr_range, "*") ||
            0 == strcmp(src_addr_range, "")) {
            // means any source addr/network
        } else if (strchr(src_addr_range, '-')) {
            char *end_addr = chop_str(src_addr_range, '-');
            if (end_addr) {
                strncpy(out_dmz->source_ip_start, src_addr_range, IPADDR_SZ);
                strncpy(out_dmz->source_ip_end, end_addr, IPADDR_SZ);
            }
        }
    }

    return SUCCESS;
}

/*
 * Internet Access Policy
 */



static boolean_t isWellKnownService (const char *name)
{
    int i = 0;

    while (g_NetworkServices[i]) {
        if (0 == strcasecmp(name, g_NetworkServices[i])) {
            return TRUE;
        }
        i++;   
    }
    return FALSE;
}

static int s_getiaphosts (UtopiaContext *ctx, int index, lanHosts_t *lanhosts)
{
    char buf[128];

    /*
     * LAN hosts
     */
    int i, count = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_IPHostCount, index, &count);
    if (count > 0) {
        char *ipaddr;
        lanhosts->iplist = ipaddr = (char *) malloc(IPADDR_SZ * count);
        if (ipaddr) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_IP, index, i, ipaddr, IPADDR_SZ);
                ipaddr += IPADDR_SZ;
            }
            lanhosts->ip_count = count;
        }
    }

    count = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_MACCount, index, &count);
    if (count > 0) {
        char *macaddr;
        lanhosts->maclist = macaddr = (char *) malloc(MACADDR_SZ * count);
        if (macaddr) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_MAC, index, i, macaddr, MACADDR_SZ);
                macaddr += MACADDR_SZ;
            }
            lanhosts->mac_count = count;
        }
    }

    count = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_IPRangeCount, index, &count);
    if (count > 0) {
        iprange_t *iprange;
        char *end_ip;
        lanhosts->iprangelist = iprange = (iprange_t *) malloc(sizeof(iprange_t) * count);
        if (iprange) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_IPRange, index, i, buf, sizeof(buf));
                if ((end_ip = chop_str(buf, ' '))) {
                    iprange->start_ip = atoi(buf);
                    iprange->end_ip = atoi(end_ip);
                }
                iprange++;
            }
            lanhosts->iprange_count = count;
        }
    }

    return UT_SUCCESS;
}

static int s_getiap (UtopiaContext *ctx, int index, iap_entry_t *iap)
{
    char buf[128], *sched, *stop_time;
    int i, j, count = 0;

    Utopia_GetIndexed(ctx, UtopiaValue_InternetAccessPolicy, index, buf, sizeof(buf));
    if (0 == (strcmp(buf, "none"))) {
        return SUCCESS;
    }

    Utopia_GetIndexedBool(ctx, UtopiaValue_IAP_Enabled, index, &iap->enabled);
    Utopia_GetIndexed(ctx, UtopiaValue_IAP_Name, index, iap->policyname, NAME_SZ);
    Utopia_GetIndexed(ctx, UtopiaValue_IAP_Access, index, buf, sizeof(buf));
    iap->allow_access = TRUE;
    if (0 == strcmp(buf, "deny")) {
        iap->allow_access = FALSE;
    } 
    Utopia_GetIndexed(ctx, UtopiaValue_IAP_TR_INST_NUM, index, buf, sizeof(buf));
    iap->tr_inst_num = IsInteger(buf) ? atoi(buf) : 0;

    /*
     * TOD schedule
     */
    Utopia_GetIndexed(ctx, UtopiaValue_IAP_EnforcementSchedule, index, buf, sizeof(buf));
    if (NULL == (sched = chop_str(buf,' '))) {
        iap->tod.all_day = TRUE;
        iap->tod.day = atoi(buf);
    } else {
        iap->tod.all_day = FALSE;
        iap->tod.day = atoi(buf);
        if ((stop_time = chop_str(sched, ' '))) {
            strncpy(iap->tod.start_time, sched, HH_MM_SZ);
            strncpy(iap->tod.stop_time, stop_time, HH_MM_SZ);
        }
    }

    s_getiaphosts(ctx, index, &iap->lanhosts);

    iap->lanhosts_set = TRUE;

#ifdef ZERO
    /*
     * LAN hosts
     */
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_IPHostCount, index, &count);
    if (count > 0) {
        char *ipaddr;
        iap->lanhosts.iplist = ipaddr = (char *) malloc(IPADDR_SZ * count);
        if (ipaddr) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_IP, index, i, ipaddr, IPADDR_SZ);
                ipaddr += IPADDR_SZ;
            }
            iap->lanhosts.ip_count = count;
        }
    }

    count = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_MACCount, index, &count);
    if (count > 0) {
        char *macaddr;
        iap->lanhosts.maclist = macaddr = (char *) malloc(MACADDR_SZ * count);
        if (macaddr) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_MAC, index, i, macaddr, MACADDR_SZ);
                macaddr += MACADDR_SZ;
            }
            iap->lanhosts.mac_count = count;
        }
    }

    count = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_IPRangeCount, index, &count);
    if (count > 0) {
        iprange_t *iprange;
        char *end_ip;
        iap->lanhosts.iprangelist = iprange = (iprange_t *) malloc(sizeof(iprange_t) * count);
        if (iprange) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_IPRange, index, i, buf, sizeof(buf));
                if ((end_ip = chop_str(buf, ' '))) {
                    iprange->start_ip = atoi(buf);
                    iprange->end_ip = atoi(end_ip);
                }
                iprange++;
            }
            iap->lanhosts.iprange_count = count;
        }
    }
#endif

    /*
     * Blocked url/keyword/apps
     */
    count = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_BlockUrlCount, index, &count);
    if (count > 0) {
        char *url;
        iap->block.url_list = url = (char *) malloc(URL_SZ * count);
        if (url) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockURL, index, i, url, URL_SZ);
                url += URL_SZ;
            }
            iap->block.url_count = count;
        }

        iap->block.url_tr_inst_num = (unsigned int *) malloc(sizeof(unsigned int)*count);
        if (iap->block.url_tr_inst_num) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockURL_TR_INST_NUM, index, i, buf, sizeof(buf));
                iap->block.url_tr_inst_num[i-1] = IsInteger(buf) ? atoi(buf):0;
            } 
        }
        char * alias = NULL;
        iap->block.url_tr_alias = alias = (char *) malloc(TR_ALIAS_SZ * count);
        if (iap->block.url_tr_alias) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockURL_TR_ALIAS, index, i, alias, TR_ALIAS_SZ);
                alias += TR_ALIAS_SZ;
            }
        }
    }

    count = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_BlockKeywordCount, index, &count);
    if (count > 0) {
        char *keyword;
        iap->block.keyword_list = keyword = (char *) malloc(IAP_KEYWORD_SZ * count);
        if (keyword) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockKeyword, index, i, keyword, IAP_KEYWORD_SZ);
                keyword += IAP_KEYWORD_SZ;
            }
            iap->block.keyword_count = count;
        }

        iap->block.keyword_tr_inst_num = (unsigned int *) malloc(sizeof(unsigned int) * count);
        if (iap->block.keyword_tr_inst_num) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockKeyword_TR_INST_NUM, index, i, buf, sizeof(buf));
                iap->block.keyword_tr_inst_num[i-1] = IsInteger(buf) ? atoi(buf):0;
            }
        }

        char * alias = NULL;
        iap->block.keyword_tr_alias = alias = (char *) malloc(TR_ALIAS_SZ * count);
        if (alias) {
            for (i = 1; i <= count; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockKeyword_TR_ALIAS, index, i, alias, TR_ALIAS_SZ);
                alias += TR_ALIAS_SZ;
            }
        }
    }

    int appcount = 0, wkappcount = 0;

    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_BlockApplicationCount, index, &appcount);
    Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_BlockWellknownApplicationCount, index, &wkappcount);

    count = appcount + wkappcount;

    int is_ping_blocked = 0;
    if (SUCCESS == Utopia_GetIndexedInt(ctx, UtopiaValue_IAP_BlockPing, index, &is_ping_blocked)
        && is_ping_blocked == 1)
    {
        count++;
    }

    if (count > 0) {
        appentry_t *app;
        iap->block.app_list = app = (appentry_t *) malloc(sizeof(appentry_t) * count);

        iap->block.app_tr_inst_num = (unsigned int *)malloc(sizeof(int)*count);

        if (app && iap->block.app_tr_inst_num ) {
        	j = 0;
            for (i = 0; i < wkappcount; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockWKApplication, index, i+1, app[j].name, NAME_SZ);
                app[j].wellknown = TRUE;

                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockWKApplication_TR_INST_NUM, index, i+1, buf, sizeof(buf));
                iap->block.app_tr_inst_num[j] = IsInteger(buf) ? atoi(buf):0;

                j++;
            }
            for (i = 0; i < appcount; i++) {
                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockName, index, i+1, app[j].name, NAME_SZ);

                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockApplication_TR_INST_NUM, index, i+1, buf, sizeof(buf));
                iap->block.app_tr_inst_num[j] = IsInteger(buf) ? atoi(buf):0;

                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockProto, index, i+1, buf, sizeof(buf));
                app[j].proto = s_StrToEnum(g_ProtocolMap, buf);

                Utopia_GetIndexed2(ctx, UtopiaValue_IAP_BlockPortRange, index, i+1, buf, sizeof(buf));
                char sport[10], eport[10];
                if (2 == (sscanf(buf, "%10s %10s", sport, eport))) {
                    app[j].port.start = atoi(sport);
                    app[j].port.end = atoi(eport);
                }

                app[j].wellknown = FALSE;
                j++;
            }
            if (is_ping_blocked) {
                strcpy(app[j].name, "Ping");
                app[j].proto = -1;
                app[j].port.start = 0;
                app[j].port.end = 0;
                app[j].wellknown = FALSE;
                j++;
            }
            iap->block.app_count = count;
        }
    }

    return SUCCESS;
}

/*
 * Set LAN hosts
 */
static int s_setiaphosts (UtopiaContext *ctx, int index, lanHosts_t *lanhosts)
{
    char buf[128];
    int i;

    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "iap_%d", index);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_InternetAccessPolicy, index, s_tokenbuf);

    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "iap_localhost_%d", index);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_IAP_LocalHostList, index, s_tokenbuf);

    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_IAP_IPHostCount, index, lanhosts->ip_count);
    if (lanhosts->ip_count > 0) {
        char *ipaddr = lanhosts->iplist;
        if (ipaddr) {
            for (i = 1; i <= lanhosts->ip_count; i++) {
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_IP, index, i, ipaddr);
                ipaddr += IPADDR_SZ;
            }
        }
    }

    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_IAP_MACCount, index, lanhosts->mac_count);
    if (lanhosts->mac_count > 0) {
        char *mac = lanhosts->maclist;
        if (mac) {
            for (i = 1; i <= lanhosts->mac_count; i++) {
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_MAC, index, i, mac);
                mac += MACADDR_SZ;
            }
        }
    }
    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_IAP_IPRangeCount, index, lanhosts->iprange_count);
    if (lanhosts->iprange_count > 0) {
        iprange_t *iprange = lanhosts->iprangelist;
        if (iprange) {
            for (i = 1; i <= lanhosts->iprange_count; i++) {
                snprintf(buf, sizeof(buf), "%d %d", iprange->start_ip, iprange->end_ip);
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_IPRange, index, i, buf);
                iprange++;
            }
        } 
    }

    return UT_SUCCESS;
}

static int s_setiap (UtopiaContext *ctx, int index, iap_entry_t *iap)
{
    char buf[128];
    int i;

    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "iap_%d", index);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_InternetAccessPolicy, index, s_tokenbuf);

    UTOPIA_SETINDEXEDBOOL(ctx, UtopiaValue_IAP_Enabled, index, iap->enabled);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_IAP_Name, index, iap->policyname);
    strncpy(buf, (iap->allow_access) ? "allow" : "deny", sizeof(buf));
    UTOPIA_SETINDEXED(ctx, UtopiaValue_IAP_Access, index, buf);
    
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf)-1, "%d", iap->tr_inst_num );
    UTOPIA_SETINDEXED(ctx, UtopiaValue_IAP_TR_INST_NUM, index, buf);

    if (iap->tod.all_day) {
        snprintf(buf, sizeof(buf), "%d", iap->tod.day);
    }
    else {
        snprintf(buf, sizeof(buf), "%d %s %s", iap->tod.day, iap->tod.start_time, iap->tod.stop_time);
    }
    UTOPIA_SETINDEXED(ctx, UtopiaValue_IAP_EnforcementSchedule, index, buf);

    if (TRUE == iap->lanhosts_set) {
        (void) s_setiaphosts(ctx, index, &iap->lanhosts);
    }

    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_IAP_BlockUrlCount, index, iap->block.url_count);
    if (iap->block.url_count > 0) {
        char *url = iap->block.url_list;
        if (url) {
            for (i = 1; i <= iap->block.url_count; i++) {
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockURL, index, i, url);
                
                memset(buf, 0, sizeof(buf));
                snprintf(buf, sizeof(buf)-1, "%d",  iap->block.url_tr_inst_num[i-1]);
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockURL_TR_INST_NUM, index, i, buf);

                char * alias = iap->block.url_tr_alias + (i-1)*TR_ALIAS_SZ;
                memset(buf, 0, sizeof(buf));
                snprintf(buf, sizeof(buf)-1, "%s", alias);
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockURL_TR_ALIAS, index, i, buf);
                
                url += URL_SZ;
            }
        }
    }

    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_IAP_BlockKeywordCount, index, iap->block.keyword_count);
    if (iap->block.keyword_count > 0) {
        char *keyword = iap->block.keyword_list;
        if (keyword) {
            for (i = 1; i <= iap->block.keyword_count; i++) {
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockKeyword, index, i, keyword);
                
                memset(buf, 0, sizeof(buf));
                snprintf(buf, sizeof(buf)-1, "%d", iap->block.keyword_tr_inst_num[i-1]);
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockKeyword_TR_INST_NUM, index, i, buf);

                char * alias = iap->block.keyword_tr_alias + (i-1)*TR_ALIAS_SZ;
                memset(buf, 0, sizeof(buf));
                snprintf(buf, sizeof(buf)-1, "%s", alias);
                Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockKeyword_TR_ALIAS, index, i, buf);

                keyword += IAP_KEYWORD_SZ;
            }
        }
    }

    int wkappcount = 0, appcount = 0;

    Utopia_UnsetIndexed(ctx, UtopiaValue_IAP_BlockPing, index);
    if (iap->block.app_count > 0) {
        appentry_t *app = iap->block.app_list;
        if (app) {
            int index_wk_app = 0;
            int index_app = 0;
            for (i = 0; i < iap->block.app_count; i++) {
                /*since we read WKApplication&Application at separate loop, we need to maintain their instanceNumbers in 2 domains */
                if (isWellKnownService(app[i].name)) {
                    Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockWKApplication, index, wkappcount+1, app[i].name);

                    memset(buf, 0, sizeof(buf));
                    snprintf(buf, sizeof(buf)-1, "%d", iap->block.app_tr_inst_num[i]);
                    Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockWKApplication_TR_INST_NUM, index, index_wk_app+1, buf);

                    index_wk_app++;
                    wkappcount++;
                }
                else if ((0 == strcasecmp("ping", app[i].name)) && ((int)app[i].proto == -1)) {
                    Utopia_SetIndexedInt(ctx, UtopiaValue_IAP_BlockPing, index, 1);
                } else {
                    snprintf(buf, sizeof(buf), "iap_blockapp_%d_%d", index, appcount+1);
                    Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockApplication, index, appcount+1, buf);

                    memset(buf, 0, sizeof(buf));
                    snprintf(buf, sizeof(buf)-1, "%d", iap->block.app_tr_inst_num[i]);
                    Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockApplication_TR_INST_NUM, index, index_app+1, buf);

                    Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockName, index, appcount+1, app[i].name);
                    char *proto = s_EnumToStr(g_ProtocolMap, app[i].proto);
                    Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockProto, index, appcount+1, proto);
                    snprintf(buf, sizeof(buf), "%d %d", app[i].port.start, app[i].port.end);
                    Utopia_SetIndexed2(ctx, UtopiaValue_IAP_BlockPortRange, index, appcount+1, buf);

                    index_app++;
                    appcount++;
                }
            }
        }
    }

    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_IAP_BlockWellknownApplicationCount, index, wkappcount);
    UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_IAP_BlockApplicationCount, index, appcount);

    return SUCCESS;
}

static void s_freeiap (iap_entry_t *iap)
{
    if (iap->lanhosts.iplist) {
        free(iap->lanhosts.iplist);
    }
    iap->lanhosts.iplist = NULL;
    if (iap->lanhosts.maclist) {
        free(iap->lanhosts.maclist);
    }
    iap->lanhosts.maclist = NULL;
    if (iap->lanhosts.iprangelist) {
        free(iap->lanhosts.iprangelist);
    }
    iap->lanhosts.iprangelist = NULL;
    if (iap->block.url_list) {
        free(iap->block.url_list);
    }
    iap->block.url_list = NULL;
    if (iap->block.url_tr_inst_num) {
        free(iap->block.url_tr_inst_num);
    }
    iap->block.url_tr_inst_num = NULL;
    if (iap->block.url_tr_alias) {
        free(iap->block.url_tr_alias);
    }
    iap->block.url_tr_alias = NULL;    
    if (iap->block.keyword_list) {
        free(iap->block.keyword_list);
    }
    iap->block.keyword_list = NULL;
    if (iap->block.keyword_tr_inst_num) {
        free(iap->block.keyword_tr_inst_num);
    }
    iap->block.keyword_tr_inst_num = NULL;
    if (iap->block.keyword_tr_alias) {
        free(iap->block.keyword_tr_alias);
    }
    iap->block.keyword_tr_alias = NULL;
    if (iap->block.app_list) {
        free(iap->block.app_list);
    }
    iap->block.app_list = NULL;
    if (iap->block.app_tr_inst_num) {
        free(iap->block.app_tr_inst_num);
    }
    iap->block.app_tr_inst_num = NULL;
}


/*
 * Remove 'index' entry and move-up 'index' to 'count' entries
 */
static int s_deleteiap (UtopiaContext *ctx, int index)
{
    int i, count = 0;

    Utopia_GetInt(ctx, UtopiaValue_Firewall_IAPCount, &count);

    if (index > count) {
        return ERR_INVALID_VALUE;
    }

    // delete 'index' entry
    UTOPIA_UNSETINDEXED(ctx, UtopiaValue_InternetAccessPolicy, index);
    
    if (index < count) {
        for (i = index; i < count ; i++) {
            iap_entry_t old_iap;

            bzero(&old_iap, sizeof(iap_entry_t));
            s_getiap(ctx, i+1, &old_iap); 
            s_setiap(ctx, i, &old_iap); 
            s_freeiap(&old_iap);
        }
    }

    UTOPIA_SETINT(ctx, UtopiaValue_Firewall_IAPCount, count-1);

    return UT_SUCCESS;
}

/*
 * Returns a null terminated array of strings with network
 * service names like ftp, telnet, dns, etc
 */
int Utopia_GetNetworkServicesList (const char **out_list)
{
    out_list = g_NetworkServices;
    return UT_SUCCESS;
}

void Utopia_FreeInternetAccessPolicy (iap_entry_t *iap)
{
    return s_freeiap(iap);
}

int Utopia_AddInternetAccessPolicy (UtopiaContext *ctx, iap_entry_t *iap)
{
    int rc, count = 0, old_index = 0;
    iap_entry_t oldiap;

    if (NULL == ctx || NULL == iap) {
        return ERR_INVALID_ARGS;
    }

    if (UT_SUCCESS != Utopia_FindInternetAccessPolicy(ctx, iap->policyname, &oldiap, &old_index)) {
        /*
         * Add new IAP entry as (count+1) index & 
         * update total policy count
         */
        Utopia_GetInt(ctx, UtopiaValue_Firewall_IAPCount, &count);
        if (UT_SUCCESS != (rc = s_setiap(ctx, count+1, iap))) {
            return rc;
        }
        UTOPIA_SETINT(ctx, UtopiaValue_Firewall_IAPCount, count+1);
    } else {
        /*
         * Edit existing IAP entry as-is
         */
        return s_setiap(ctx, old_index, iap);
    }

    return SUCCESS;
}

int Utopia_EditInternetAccessPolicy (UtopiaContext *ctx, int index, iap_entry_t *iap)
{
    int count = 0;

    if (NULL == ctx || NULL == iap) {
        return ERR_INVALID_ARGS;
    }
    
    Utopia_GetInt(ctx, UtopiaValue_Firewall_IAPCount, &count);
    
    if (index < 1 || index > count) {
        return ERR_INVALID_ARGS;
    }
    
    /*
     * Edit existing IAP entry as-is
     */
    return s_setiap(ctx, index, iap);
}

int Utopia_AddInternetAccessPolicyLanHosts (UtopiaContext *ctx, const char *policyname, lanHosts_t *lanhosts)
{
    int rc, count = 0, old_index = 0;
    iap_entry_t oldiap;

    if (UT_SUCCESS != Utopia_FindInternetAccessPolicy(ctx, policyname, &oldiap, &old_index)) {
        /*
         * Add new IAP entry as (count+1) index, set just lan hosts settings
         * update total policy count
         */
        Utopia_GetInt(ctx, UtopiaValue_Firewall_IAPCount, &count);
        if (UT_SUCCESS != (rc = s_setiaphosts(ctx, count+1, lanhosts))) {
            return rc;
        }
        UTOPIA_SETINDEXED(ctx, UtopiaValue_IAP_Name, count+1, (char *)policyname);
        UTOPIA_SETINT(ctx, UtopiaValue_Firewall_IAPCount, count+1);
    } else {
        /*
         * Edit existing IAP entry's lan hosts settings
         */
        return s_setiaphosts(ctx, old_index, lanhosts);
    }

    return UT_SUCCESS;
}

int Utopia_DeleteInternetAccessPolicy (UtopiaContext *ctx, const char *policyname)
{
    int old_index = 0;
    iap_entry_t oldiap;

    if (NULL == ctx || NULL == policyname) {
        return ERR_INVALID_ARGS;
    }

    if (UT_SUCCESS == Utopia_FindInternetAccessPolicy(ctx, policyname, &oldiap, &old_index)) {
        return s_deleteiap(ctx, old_index);
    }

    return ERR_INVALID_VALUE;
}

int Utopia_GetInternetAccessPolicy (UtopiaContext *ctx, int *out_count, iap_entry_t **out_iap)
{
    int i;

    if (NULL == ctx || NULL == out_count || NULL == out_iap) {
        return ERR_INVALID_ARGS;
    }

    Utopia_GetInt(ctx, UtopiaValue_Firewall_IAPCount, out_count);
    if (*out_count <= 0) {
        return SUCCESS;
    }

    iap_entry_t *iap = (iap_entry_t *) malloc(sizeof(iap_entry_t) * (*out_count));
    if (NULL == iap) {
        return ERR_INSUFFICIENT_MEM;
    }
    bzero(iap, sizeof(iap_entry_t) * (*out_count));

    for (i = 0; i < *out_count; i++) {
        int rc;
        if (SUCCESS != (rc = s_getiap(ctx, i+1, &iap[i]))) {
            // TODO: free internal buffers
            free(iap);
            *out_iap = NULL;
            return rc;
        }
    }
    
    *out_iap = iap;

    return SUCCESS;
}

int Utopia_FindInternetAccessPolicy (UtopiaContext *ctx, const char *policyname, iap_entry_t *out_iap, int *out_index)
{
    int count = 0, i;

    if (NULL == ctx || NULL == policyname || NULL == out_iap) {
        return ERR_INVALID_ARGS;
    }

    Utopia_GetInt(ctx, UtopiaValue_Firewall_IAPCount, &count);
    for (i = 1; i <= count; i++) {
        bzero(out_iap, sizeof(iap_entry_t));
        if (SUCCESS == s_getiap(ctx, i, out_iap)) {
            if (0 == strcmp(policyname, out_iap->policyname)) {
                if (out_index) {
                    *out_index = i;
                }
                return UT_SUCCESS;
            }
        }
    }

    bzero(out_iap, sizeof(iap_entry_t));
    return ERR_ITEM_NOT_FOUND;
}

/*
 * QoS
 */

#define QOS_MAX_DEFINED_POLICY 256
#define QOS_DEFINED_POLICY_FILE "/etc/qos_classification_rules"
#define QOS_ETHERNET_PORT_1_POLICY_NAME "Ethernet Port 1"
#define QOS_ETHERNET_PORT_2_POLICY_NAME "Ethernet Port 2"
#define QOS_ETHERNET_PORT_3_POLICY_NAME "Ethernet Port 3"
#define QOS_ETHERNET_PORT_4_POLICY_NAME "Ethernet Port 4"

// Singleton
qosDefinedPolicy_t g_qosDefinedPolicyList[QOS_MAX_DEFINED_POLICY];
boolean_t          g_qosDefinedPolicyCount = 0;
boolean_t          g_qosDefinedPolicyInit = FALSE;

static boolean_t findQosDefinedPolicyEntry(qosDefinedPolicy_t qoslist[], int count, char *name)
{
    int i;
    for(i = 0; i < count; i++) {
        if (0 == strcmp(name, qoslist[i].name)) {
            return TRUE;
        }
    }
    return FALSE;
}

int Utopia_GetQoSDefinedPolicyList (int *out_count, qosDefinedPolicy_t const **out_qoslist)
{

    if (TRUE == g_qosDefinedPolicyInit) {
        *out_count = g_qosDefinedPolicyCount;
        *out_qoslist = g_qosDefinedPolicyList;
        return UT_SUCCESS;
    }

    FILE *fp = fopen(QOS_DEFINED_POLICY_FILE, "r");
    if (fp) {
        char *saveptr;
        char *name, *friendly, *type;

        int count = 0;
        while (count < QOS_MAX_DEFINED_POLICY && fgets(s_tokenbuf, sizeof(s_tokenbuf), fp)) {
            name = strtok_r(s_tokenbuf, "|", &saveptr);
            if (NULL == name) {
                continue;
            }
            if (TRUE == findQosDefinedPolicyEntry(g_qosDefinedPolicyList, count, name)) {
                continue;
            }
            friendly = strtok_r(NULL, "|", &saveptr);
            if (NULL == friendly) {
                continue;
            }
            type = strtok_r(NULL, "|", &saveptr);
            if (NULL == type) {
                continue;
            }
            strncpy(g_qosDefinedPolicyList[count].name, name, NAME_SZ);
            strncpy(g_qosDefinedPolicyList[count].friendly_name, friendly, TOKEN_SZ);
            if (0 == strcasecmp("game", type)) {
                g_qosDefinedPolicyList[count].type = QOS_GAME;
            } else {
                g_qosDefinedPolicyList[count].type = QOS_APPLICATION;
            }
            count++;
        }
        fclose(fp);

        g_qosDefinedPolicyInit = TRUE;
        *out_count = g_qosDefinedPolicyCount = count;
        *out_qoslist = g_qosDefinedPolicyList;

        return UT_SUCCESS;
    }   

    return ERR_FILE_NOT_FOUND;
}

int Utopia_SetQoSSettings (UtopiaContext *ctx, qosInfo_t *qos)
{
    if (NULL == ctx || NULL == qos || (TRUE == qos->enable && qos->policy_count > 0 && NULL == qos->policy_list)) {
        return ERR_INVALID_ARGS;
    }

    int i;

    UTOPIA_SETINT(ctx, UtopiaValue_QoS_WanDownloadSpeed, qos->download_speed);
    UTOPIA_SETINT(ctx, UtopiaValue_QoS_WanUploadSpeed, qos->upload_speed);

    // Assume no ethernet port policies are active. If any of them are active,
    // they'll be set in the for loop below.
    // (We need to unset them here to ensure inactive policies are removed from
    //  syscfg correctly.)
    UTOPIA_UNSET(ctx, UtopiaValue_QoS_EthernetPort1);
    UTOPIA_UNSET(ctx, UtopiaValue_QoS_EthernetPort2);
    UTOPIA_UNSET(ctx, UtopiaValue_QoS_EthernetPort3);
    UTOPIA_UNSET(ctx, UtopiaValue_QoS_EthernetPort4);

    if (FALSE == qos->enable) {
        UTOPIA_SETBOOL(ctx, UtopiaValue_QoS_Enable, FALSE);
        return UT_SUCCESS;
    }
    UTOPIA_SETBOOL(ctx, UtopiaValue_QoS_Enable, TRUE);

    int dpcount = 0, maccount = 0, ethcount = 0, vdcount = 0, customct = 0;
    char *token;
    int port_range_idx;
    for (i = 0; i < qos->policy_count; i++) {
        switch (qos->policy_list[i].type) {
        case QOS_APPLICATION:
        case QOS_GAME:
            snprintf(s_tokenbuf, sizeof(s_tokenbuf), "qdp_%d", dpcount+1);
            UTOPIA_SETINDEXED(ctx, UtopiaValue_QoSDefinedPolicy, dpcount+1, s_tokenbuf);

            UTOPIA_SETINDEXED(ctx, UtopiaValue_QDP_Name, dpcount+1, qos->policy_list[i].name);
            token = s_EnumToStr(g_QoSClassMap, qos->policy_list[i].priority);
            if (token) {
                UTOPIA_SETINDEXED(ctx, UtopiaValue_QDP_Class, dpcount+1, token);
            }
            dpcount++;
            break;
        case QOS_MACADDR:
            snprintf(s_tokenbuf, sizeof(s_tokenbuf), "qma_%d", maccount+1);
            UTOPIA_SETINDEXED(ctx, UtopiaValue_QoSMacAddr, maccount+1, s_tokenbuf);

            UTOPIA_SETINDEXED(ctx, UtopiaValue_QMA_Name, maccount+1, qos->policy_list[i].name);
            UTOPIA_SETINDEXED(ctx, UtopiaValue_QMA_Mac, maccount+1, qos->policy_list[i].mac);
            token = s_EnumToStr(g_QoSClassMap, qos->policy_list[i].priority);
            if (token) {
                UTOPIA_SETINDEXED(ctx, UtopiaValue_QMA_Class, maccount+1, token);
            }
            maccount++;
            break;
        case QOS_VOICE_DEVICE:
            snprintf(s_tokenbuf, sizeof(s_tokenbuf), "qvd_%d", vdcount+1);
            UTOPIA_SETINDEXED(ctx, UtopiaValue_QoSVoiceDevice, vdcount+1, s_tokenbuf);

            UTOPIA_SETINDEXED(ctx, UtopiaValue_QVD_Name, vdcount+1, qos->policy_list[i].name);
            UTOPIA_SETINDEXED(ctx, UtopiaValue_QVD_Mac, vdcount+1, qos->policy_list[i].mac);
            token = s_EnumToStr(g_QoSClassMap, qos->policy_list[i].priority);
            if (token) {
                UTOPIA_SETINDEXED(ctx, UtopiaValue_QVD_Class, vdcount+1, token);
            }
            vdcount++;
            break;
        case QOS_CUSTOM:
            snprintf(s_tokenbuf, sizeof(s_tokenbuf), "qup_%d", customct+1);
            UTOPIA_SETINDEXED(ctx, UtopiaValue_QoSUserDefinedPolicy, customct+1, s_tokenbuf);

            UTOPIA_SETINDEXED(ctx, UtopiaValue_QUP_Name, customct+1, qos->policy_list[i].name);
            UTOPIA_SETINDEXEDINT(ctx, UtopiaValue_QUP_PortRangeCount, customct+1, qos->policy_list[i].hwport);
            
            // Retrieve the class/priority of the policy
            token = s_EnumToStr(g_QoSClassMap, qos->policy_list[i].priority);
            
            for (port_range_idx = 0; port_range_idx < qos->policy_list[i].hwport; port_range_idx++) {
                // Set the custom proto
                char *proto = s_EnumToStr(g_ProtocolMap, qos->policy_list[i].custom_proto[port_range_idx]);
                Utopia_SetIndexed2(ctx, UtopiaValue_QUP_Protocol, customct+1, port_range_idx+1, proto);
                
                // Set the custom port range
                snprintf(s_tokenbuf, sizeof(s_tokenbuf), "%d %d",
                         qos->policy_list[i].custom_port[port_range_idx].start,
                         qos->policy_list[i].custom_port[port_range_idx].end);
                // Add 1 to the index since syscfg is one-indexed instead of
                // zero-indexed
                Utopia_SetIndexed2(ctx, UtopiaValue_QUP_PortRange, customct+1, port_range_idx+1, s_tokenbuf);
                
                // Set the class/priority
                if (token) {
                    Utopia_SetIndexed2(ctx, UtopiaValue_QUP_Class, customct+1, port_range_idx+1, token);
                }
            }
            
            token = s_EnumToStr(g_QoSCustomTypeMap, qos->policy_list[i].custom_type);
            if (token) {
                UTOPIA_SETINDEXED(ctx, UtopiaValue_QUP_Type, customct+1, token);
            }
            customct++;
            break;
        case QOS_ETHERNET_PORT:
            token = s_EnumToStr(g_QoSClassMap, qos->policy_list[i].priority);
            if (NULL == token) {
                continue;
            }
            
            if (0 == strcmp(qos->policy_list[i].name, QOS_ETHERNET_PORT_1_POLICY_NAME)) {
                UTOPIA_SET(ctx, UtopiaValue_QoS_EthernetPort1, token);
                ethcount++;
            }
            else if (0 == strcmp(qos->policy_list[i].name, QOS_ETHERNET_PORT_2_POLICY_NAME)) {
                UTOPIA_SET(ctx, UtopiaValue_QoS_EthernetPort2, token);
                ethcount++;
            }
            else if (0 == strcmp(qos->policy_list[i].name, QOS_ETHERNET_PORT_3_POLICY_NAME)) {
                UTOPIA_SET(ctx, UtopiaValue_QoS_EthernetPort3, token);
                ethcount++;
            }
            else if (0 == strcmp(qos->policy_list[i].name, QOS_ETHERNET_PORT_4_POLICY_NAME)) {
                UTOPIA_SET(ctx, UtopiaValue_QoS_EthernetPort4, token);
                ethcount++;
            }
            break;
         }
    }
    UTOPIA_SETINT(ctx, UtopiaValue_QoS_DefPolicyCount, dpcount);
    UTOPIA_SETINT(ctx, UtopiaValue_QoS_MacAddrCount, maccount);
    UTOPIA_SETINT(ctx, UtopiaValue_QoS_EthernetPortCount, ethcount);
    UTOPIA_SETINT(ctx, UtopiaValue_QoS_VoiceDeviceCount, vdcount);
    UTOPIA_SETINT(ctx, UtopiaValue_QoS_UserDefPolicyCount, customct);

    return UT_SUCCESS;
}

int Utopia_GetQoSSettings (UtopiaContext *ctx, qosInfo_t *qos)
{
    if (NULL == ctx || NULL == qos) {
        return ERR_INVALID_ARGS;
    }

    int i;
    int j;

    Utopia_GetInt(ctx, UtopiaValue_QoS_WanDownloadSpeed, &qos->download_speed);
    Utopia_GetInt(ctx, UtopiaValue_QoS_WanUploadSpeed, &qos->upload_speed);

    UTOPIA_GETBOOL(ctx, UtopiaValue_QoS_Enable, &qos->enable);
    if (FALSE == qos->enable) {
        qos->policy_count = 0;
        return UT_SUCCESS;
    }

    int dpcount = 0, maccount = 0, ethcount = 0, vdcount = 0, customct = 0;

    Utopia_GetInt(ctx, UtopiaValue_QoS_DefPolicyCount, &dpcount);
    Utopia_GetInt(ctx, UtopiaValue_QoS_MacAddrCount, &maccount);
    Utopia_GetInt(ctx, UtopiaValue_QoS_EthernetPortCount, &ethcount);
    Utopia_GetInt(ctx, UtopiaValue_QoS_VoiceDeviceCount, &vdcount);
    Utopia_GetInt(ctx, UtopiaValue_QoS_UserDefPolicyCount, &customct);
    qos->policy_count = (dpcount + maccount + ethcount + vdcount + customct);

    qosPolicy_t *policy_list = (qosPolicy_t *) malloc(sizeof(qosPolicy_t) * (qos->policy_count));
    if (NULL == policy_list) {
        return ERR_INSUFFICIENT_MEM;
    }

    j = 0;

    for (i = 0; i < dpcount; i++) {
        Utopia_GetIndexed(ctx, UtopiaValue_QDP_Name, i + 1, policy_list[j].name, NAME_SZ);
        Utopia_GetIndexed(ctx, UtopiaValue_QDP_Class, i + 1, s_tokenbuf, sizeof(s_tokenbuf));
        policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
        policy_list[j].type = QOS_APPLICATION; // for both game & app is same
        j++;
    }

    for (i = 0; i < maccount; i++) {
        Utopia_GetIndexed(ctx, UtopiaValue_QMA_Name, i + 1, policy_list[j].name, NAME_SZ);
        Utopia_GetIndexed(ctx, UtopiaValue_QMA_Mac, i + 1, policy_list[j].mac, MACADDR_SZ);
        Utopia_GetIndexed(ctx, UtopiaValue_QMA_Class, i + 1, s_tokenbuf, sizeof(s_tokenbuf));
        policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
        policy_list[j].type = QOS_MACADDR;
        j++;
    }


    if (Utopia_Get(ctx, UtopiaValue_QoS_EthernetPort1, s_tokenbuf, sizeof(s_tokenbuf))) {
        strncpy(policy_list[j].name, QOS_ETHERNET_PORT_1_POLICY_NAME, NAME_SZ);
        policy_list[j].name[NAME_SZ - 1] = '\0';
        policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
        policy_list[j].type = QOS_ETHERNET_PORT;
        j++;
    }

	if (Utopia_Get(ctx, UtopiaValue_QoS_EthernetPort2, s_tokenbuf, sizeof(s_tokenbuf))) {
        strncpy(policy_list[j].name, QOS_ETHERNET_PORT_2_POLICY_NAME, NAME_SZ);
        policy_list[j].name[NAME_SZ - 1] = '\0';
        policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
        policy_list[j].type = QOS_ETHERNET_PORT;
        j++;
    }

	if (Utopia_Get(ctx, UtopiaValue_QoS_EthernetPort3, s_tokenbuf, sizeof(s_tokenbuf))) {
        strncpy(policy_list[j].name, QOS_ETHERNET_PORT_3_POLICY_NAME, NAME_SZ);
        policy_list[j].name[NAME_SZ - 1] = '\0';
        policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
        policy_list[j].type = QOS_ETHERNET_PORT;
        j++;
    }

	if (Utopia_Get(ctx, UtopiaValue_QoS_EthernetPort4, s_tokenbuf, sizeof(s_tokenbuf))) {
        strncpy(policy_list[j].name, QOS_ETHERNET_PORT_4_POLICY_NAME, NAME_SZ);
        policy_list[j].name[NAME_SZ - 1] = '\0';
        policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
        policy_list[j].type = QOS_ETHERNET_PORT;
        j++;
    }

    for (i = 0; i < vdcount; i++) {
        Utopia_GetIndexed(ctx, UtopiaValue_QVD_Name, i + 1, policy_list[j].name, NAME_SZ);
        Utopia_GetIndexed(ctx, UtopiaValue_QVD_Mac, i + 1, policy_list[j].mac, MACADDR_SZ);
        Utopia_GetIndexed(ctx, UtopiaValue_QVD_Class, i + 1, s_tokenbuf, sizeof(s_tokenbuf));
        policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
        policy_list[j].type = QOS_VOICE_DEVICE;
        j++;
    }
    
    for (i = 0; i < customct; i++) {
        int port_range_idx;
        Utopia_GetIndexed(ctx, UtopiaValue_QUP_Name, i + 1, policy_list[j].name, NAME_SZ);
        Utopia_GetIndexedInt(ctx, UtopiaValue_QUP_PortRangeCount, i + 1, &policy_list[i].hwport);
        
        for (port_range_idx = 0; port_range_idx < policy_list[i].hwport; port_range_idx++) {
            // Get the custom protocol
            Utopia_GetIndexed2(ctx, UtopiaValue_QUP_Protocol, i + 1, port_range_idx + 1, s_tokenbuf, sizeof(s_tokenbuf));
            policy_list[j].custom_proto[port_range_idx] = s_StrToEnum(g_ProtocolMap, s_tokenbuf);
            
            // Get the custom port range
            Utopia_GetIndexed2(ctx, UtopiaValue_QUP_PortRange, i + 1, port_range_idx + 1, s_tokenbuf, sizeof(s_tokenbuf));
            if (sscanf(s_tokenbuf, "%d %d",
                       &qos->policy_list[j].custom_port[port_range_idx].start,
                       &qos->policy_list[j].custom_port[port_range_idx].end) != 2)
            {
                policy_list[j].custom_port[port_range_idx].start = 0;
                policy_list[j].custom_port[port_range_idx].end = 0;
            }
            
            // Get the class/priority
            // Since each set of protocol/port range will have the same class,
            // just retrieve the class from the first one
            if (port_range_idx == 0) {
                Utopia_GetIndexed2(ctx, UtopiaValue_QUP_Class, i + 1, port_range_idx + 1, s_tokenbuf, sizeof(s_tokenbuf));
                policy_list[j].priority = s_StrToEnum(g_QoSClassMap, s_tokenbuf);
            }
        }
        // Fill in the remaining custom port ranges with default values
        for (; port_range_idx < MAX_CUSTOM_PORT_ENTRIES; port_range_idx++) {
            policy_list[j].custom_proto[port_range_idx] = BOTH_TCP_UDP;
            policy_list[j].custom_port[port_range_idx].start = 0;
            policy_list[j].custom_port[port_range_idx].end = 0;
        }
        
        policy_list[j].type = QOS_CUSTOM;
        
        Utopia_GetIndexed(ctx, UtopiaValue_QUP_Type, i + 1, s_tokenbuf, sizeof(s_tokenbuf));
        policy_list[j].custom_type = s_StrToEnum(g_QoSCustomTypeMap, s_tokenbuf);
        
        j++;
    }

    qos->policy_list = policy_list;
    return UT_SUCCESS;
}

/*
 * IPv6
 */


int Utopia_SetIPv6Settings (UtopiaContext *ctx, ipv6Info_t *ipv6_info)
{
    if (NULL == ctx || NULL == ipv6_info) {
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_6rd_Enable, ipv6_info->sixrd_enable);
        UTOPIA_SETINT(ctx, UtopiaValue_IPv6_6rd_Common_Prefix4, ipv6_info->sixrd_common_prefix4);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_6rd_Relay, ipv6_info->sixrd_relay);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_6rd_Zone, ipv6_info->sixrd_zone);
        UTOPIA_SETINT(ctx, UtopiaValue_IPv6_6rd_Zone_Length, ipv6_info->sixrd_zone_length);

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_6to4_Enable, ipv6_info->sixtofour_enable);

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_Aiccu_Enable, ipv6_info->aiccu_enable);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_Aiccu_Password, ipv6_info->aiccu_password);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_Aiccu_Prefix, ipv6_info->aiccu_prefix);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_Aiccu_Tunnel, ipv6_info->aiccu_tunnel);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_Aiccu_User, ipv6_info->aiccu_user);

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_HE_Enable, ipv6_info->he_enable);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_HE_Client_IPv6, ipv6_info->he_client_ipv6);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_HE_Password, ipv6_info->he_password);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_HE_Prefix, ipv6_info->he_prefix);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_HE_Server_IPv4, ipv6_info->he_server_ipv4);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_HE_Tunnel, ipv6_info->he_tunnel);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_HE_User, ipv6_info->he_user);

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_Bridging_Enable, ipv6_info->ipv6_bridging_enable);
    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_Ndp_Proxy_Enable, ipv6_info->ipv6_ndp_proxy_enable);

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_DHCPv6c_Enable, ipv6_info->dhcpv6c_enable);
    UTOPIA_SET(ctx, UtopiaValue_IPv6_DHCPv6c_DUID, ipv6_info->dhcpv6c_duid);
    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_DHCPv6s_Enable, ipv6_info->dhcpv6s_enable);
    UTOPIA_SET(ctx, UtopiaValue_IPv6_DHCPv6s_DUID, ipv6_info->dhcpv6s_duid);

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_Static_Enable, ipv6_info->ipv6_static_enable);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_Default_Gateway, ipv6_info->ipv6_default_gateway);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_Lan_Address, ipv6_info->ipv6_lan_address);
        UTOPIA_SET(ctx, UtopiaValue_IPv6_Wan_Address, ipv6_info->ipv6_wan_address);

    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_RA_Enable, ipv6_info->ra_enable);
    UTOPIA_SETINT(ctx, UtopiaValue_IPv6_RA_Provisioning_Enable, ipv6_info->ra_provisioning_enable);

    return SUCCESS;
}

int Utopia_GetIPv6Settings (UtopiaContext *ctx, ipv6Info_t *out_ipv6)
{
    if (NULL == ctx || NULL == out_ipv6) {
        return ERR_INVALID_ARGS;
    }

    bzero(out_ipv6, sizeof(ipv6Info_t));

    /* Live data from sysevent */
    Utopia_Get(ctx, UtopiaValue_IPv6_Current_Lan_IPv6_Address, out_ipv6->current_lan_ipv6address, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Current_Lan_IPv6_Address_Ll, out_ipv6->current_lan_ipv6address_ll, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Current_Wan_IPv6_Interface, out_ipv6->current_wan_ipv6_interface, IFNAME_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Current_Wan_IPv6_Address, out_ipv6->current_wan_ipv6address, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Current_Wan_IPv6_Address_Ll, out_ipv6->current_wan_ipv6address_ll, IPADDR_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Connection_State, out_ipv6->ipv6_connection_state, NAME_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Domain, out_ipv6->ipv6_domain, TOKEN_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Nameserver, out_ipv6->ipv6_nameserver, TOKEN_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Ntp_Server, out_ipv6->ipv6_ntp_server, TOKEN_SZ);
    Utopia_Get(ctx, UtopiaValue_IPv6_Prefix, out_ipv6->ipv6_prefix, IPADDR_SZ);

	/* Configuration data syscfg */
    Utopia_GetInt(ctx, UtopiaValue_IPv6_6rd_Enable, &out_ipv6->sixrd_enable);

	Utopia_GetInt(ctx, UtopiaValue_IPv6_6rd_Common_Prefix4, &out_ipv6->sixrd_common_prefix4) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_6rd_Relay, out_ipv6->sixrd_relay, IPADDR_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_6rd_Zone, out_ipv6->sixrd_zone, IPADDR_SZ) ;
	Utopia_GetInt(ctx, UtopiaValue_IPv6_6rd_Zone_Length, &out_ipv6->sixrd_zone_length) ;

    Utopia_GetInt(ctx, UtopiaValue_IPv6_6to4_Enable, &out_ipv6->sixtofour_enable);

    Utopia_GetInt(ctx, UtopiaValue_IPv6_Aiccu_Enable, &out_ipv6->aiccu_enable);
	Utopia_Get(ctx, UtopiaValue_IPv6_Aiccu_Password, out_ipv6->aiccu_password, PASSWORD_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_Aiccu_Prefix, out_ipv6->aiccu_prefix, IPADDR_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_Aiccu_Tunnel, out_ipv6->aiccu_tunnel, TOKEN_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_Aiccu_User, out_ipv6->aiccu_user, USERNAME_SZ) ;


    Utopia_GetInt(ctx, UtopiaValue_IPv6_HE_Enable, &out_ipv6->he_enable);
	Utopia_Get(ctx, UtopiaValue_IPv6_HE_Client_IPv6, out_ipv6->he_client_ipv6, IPADDR_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_HE_Password, out_ipv6->he_password, PASSWORD_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_HE_Prefix, out_ipv6->he_prefix, IPADDR_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_HE_Server_IPv4, out_ipv6->he_server_ipv4, IPADDR_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_HE_Tunnel, out_ipv6->he_tunnel, TOKEN_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_HE_User, out_ipv6->he_user, USERNAME_SZ) ;

    Utopia_GetInt(ctx, UtopiaValue_IPv6_Bridging_Enable, &out_ipv6->ipv6_bridging_enable);
    Utopia_GetInt(ctx, UtopiaValue_IPv6_Ndp_Proxy_Enable, &out_ipv6->ipv6_ndp_proxy_enable);

    Utopia_GetInt(ctx, UtopiaValue_IPv6_DHCPv6c_Enable, &out_ipv6->dhcpv6c_enable);
	Utopia_Get(ctx, UtopiaValue_IPv6_DHCPv6c_DUID, out_ipv6->dhcpv6c_duid, TOKEN_SZ) ;
    Utopia_GetInt(ctx, UtopiaValue_IPv6_DHCPv6s_Enable, &out_ipv6->dhcpv6s_enable);
	Utopia_Get(ctx, UtopiaValue_IPv6_DHCPv6s_DUID, out_ipv6->dhcpv6s_duid, TOKEN_SZ) ;

    Utopia_GetInt(ctx, UtopiaValue_IPv6_Static_Enable, &out_ipv6->ipv6_static_enable);
	Utopia_Get(ctx, UtopiaValue_IPv6_Default_Gateway, out_ipv6->ipv6_default_gateway, IPADDR_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_Lan_Address, out_ipv6->ipv6_lan_address, IPADDR_SZ) ;
	Utopia_Get(ctx, UtopiaValue_IPv6_Wan_Address, out_ipv6->ipv6_wan_address, IPADDR_SZ) ;

    Utopia_GetInt(ctx, UtopiaValue_IPv6_RA_Enable, &out_ipv6->ra_enable);
    Utopia_GetInt(ctx, UtopiaValue_IPv6_RA_Provisioning_Enable, &out_ipv6->ra_provisioning_enable);

    return SUCCESS ;
}


/*
 * Logging and Diagnostics
 * logviewer - could be hostname or ipaddr
 */
#define LOG_LEVEL_DEFAULT 1
#define LOG_LEVEL_ENABLED 2
int Utopia_SetLogSettings (UtopiaContext *ctx, boolean_t log_enabled, char *log_viewer)
{
    UTOPIA_SETINT(ctx, UtopiaValue_LogLevel, log_enabled ? LOG_LEVEL_ENABLED : LOG_LEVEL_DEFAULT);
    if (log_viewer && *log_viewer) {
        UTOPIA_SET(ctx, UtopiaValue_LogRemote, log_viewer);
    }

    return SUCCESS;
}

/*
 * logviewerip - buffer should be IPHOSTNAME_SZ
 */
int Utopia_GetLogSettings (UtopiaContext *ctx, boolean_t *log_enabled, char *log_viewer, int sz)
{
    int log_level;

    *log_enabled = FALSE;
    log_viewer[0] = '\0';

    Utopia_GetInt(ctx, UtopiaValue_LogLevel, &log_level);
    if (log_level > LOG_LEVEL_DEFAULT) {
        *log_enabled = TRUE;
        Utopia_Get(ctx, UtopiaValue_LogRemote, log_viewer, sz);
    }

    return SUCCESS;
}


int Utopia_RestoreFactoryDefaults (void)
{
    char cmd[512];

    snprintf(cmd, sizeof(cmd), "syscfg_format -d %s", CONFIG_MTD_DEVICE);
    system(cmd);

    return SUCCESS;
}


static unsigned long *crc32_table = NULL;

static boolean_t crc32_table_initialized()
{
	if (NULL != crc32_table)
	   return FALSE;

	crc32_table= (unsigned long *) malloc(CRC32_TABLE_SIZE * sizeof(unsigned long));
	if (NULL == crc32_table) {
	   perror("malloc");
	   return FALSE;
	}

	return TRUE;
}

static void prepare_crc32_table()
{
   unsigned long crc_value;

   int loop_count, bit_value;

   if(!crc32_table_initialized())
      return;

   for (loop_count = 0; loop_count < CRC32_TABLE_SIZE; loop_count++)
   {
      crc_value = (unsigned long) loop_count;

      for (bit_value = 0; bit_value < 8; bit_value++)
         crc_value = CALCULATE_CRC32_TABLE_ENTRY(crc_value);

      crc32_table[loop_count] = crc_value;
   }
}

static unsigned int get_crc32_value(char *buffer, size_t length)
{
   unsigned int crc_value = 0xFFFFFFFF;
   while(length)
   {
      crc_value = crc32_table[(crc_value ^ *buffer) & 0xff] ^ (crc_value >> 8);
      length--;
      buffer++;
   }
   return crc_value;
}

int Utopia_BackupConfiguration (char *out_config_fname)
{
    if (out_config_fname && *out_config_fname) {
        ulogf(ULOG_CONFIG, UL_UTAPI, "backup system configuration to %s", out_config_fname);
        int syscfg_len = 0;
        char *buf = NULL;

        if (NULL == (buf = malloc(SYSCFG_SZ))) {
            return ERR_INSUFFICIENT_MEM;
        }

        /*
         * get the whole syscfg configuration
         * and replace terminating '\0' with '\n' for backup config
         */
        if (0 == syscfg_getall(buf, SYSCFG_SZ, &syscfg_len)) {
            char *p = buf;
            int len, sz = syscfg_len;
            while (sz > 0) {
                len = strlen(p);
                *(p+len) = '\n';
                p = p + len + 1;
                sz -= len + 1;
            }
        }

        prepare_crc32_table();
                
        config_hdr_t hdr;
    
        hdr.magic = CFG_MAGIC;
        hdr.len = syscfg_len;
        hdr.version = CFG_VERSION;
        hdr.crc32 = get_crc32_value(buf, syscfg_len);
    
        ulogf(ULOG_CONFIG, UL_UTAPI, "backup crc32 0x%x", hdr.crc32);

        unlink(out_config_fname);
        FILE *fp = fopen(out_config_fname, "w");
        if (NULL == fp) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI,
                        "backup: file open to write failed %s", out_config_fname);
            free(buf);
            return ERR_FILE_WRITE_FAILED;
        }

        /*
         * Write the header
         */
        int sz = fwrite(&hdr, 1, sizeof(config_hdr_t), fp);
        ulogf(ULOG_CONFIG, UL_UTAPI, "backup: wrote hdr of size %d", sz);
        if (sz != sizeof(config_hdr_t)) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI,
                        "backup hdr size didn't match: expected %d, real %d",
                        sizeof(config_hdr_t), sz);
        }
        /*
         * Write the configuration
         */
        sz = fwrite(buf, 1, syscfg_len, fp);
        ulogf(ULOG_CONFIG, UL_UTAPI, "backup: wrote buffer of size %d", sz);
        if (sz != syscfg_len) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "backup content size didn't match: expected %d, real %d", syscfg_len, sz);
        }
        free(buf); /*RDKB-7128, CID-33025, free unused resources before exit*/
        fclose(fp);
        return UT_SUCCESS;
    }

    return ERR_INVALID_ARGS;
}

int Utopia_RestoreConfiguration (char *config_fname)
{
    char *buf, cmd[512];
    config_hdr_t hdr;
    unsigned int sz, config_sz;
    FILE *fp;

    if (NULL == config_fname) {
        return ERR_INVALID_ARGS;
    }

    if ((fp = fopen(config_fname, "r"))) {
        ulogf(ULOG_CONFIG, UL_UTAPI, "restore system configuration from %s", config_fname);
        sz = fread(&hdr, 1, sizeof(config_hdr_t), fp);
        if (sz != sizeof(config_hdr_t)) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: hdr size didn't match: expected %d, real %d", sizeof(config_hdr_t), sz);
        }
        if ((buf = malloc(SYSCFG_SZ))) {
            config_sz = fread(buf, 1, SYSCFG_SZ, fp);
        } else {
            fclose(fp);
            return ERR_INSUFFICIENT_MEM;
        }
        fclose(fp);

        prepare_crc32_table();

        /*
         * Header check
         */
        if (hdr.magic != CFG_MAGIC) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: config hdr magic didn't match: expected 0x%x, actual 0x%x", CFG_MAGIC, hdr.magic);
            free(buf);
            return ERR_CFGRESTORE_BAD_MAGIC;
        }
        if (hdr.len != config_sz) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: config hdr len didn't match: expected %d, actual %d", hdr.len, config_sz);
            free(buf);
            return ERR_CFGRESTORE_BAD_SIZE;
        }
        if (hdr.version != CFG_VERSION) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: config hdr version didn't match: expected 0x%x, actual 0x%x", CFG_VERSION, hdr.version);
            free(buf);
            return ERR_CFGRESTORE_BAD_VERSION;
        }
        unsigned int expected_crc32 = get_crc32_value(buf, config_sz);
        if (hdr.crc32 != expected_crc32) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: config hdr crc32 didn't match: expected 0x%x, actual 0x%x", hdr.crc32, expected_crc32);
            free(buf);
            return ERR_CFGRESTORE_BAD_CRC32;
        } else {
            ulogf(ULOG_CONFIG, UL_UTAPI, "restore: config hdr crc32 MATCHED!!: actual 0x%x", expected_crc32);
        }
        /*
         * Header check passed!!
         */

        unlink(CFG_RESTORE_TMP_FILE);
        FILE *fpw = fopen(CFG_RESTORE_TMP_FILE, "w");
        if (NULL == fpw) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: config write to temp file failed");
            free(buf);
            return ERR_INVALID_ARGS;
        }
        sz = fwrite(buf, 1, config_sz, fpw);
        fclose(fpw);

        if (sz != config_sz) {
            ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: written config size didn't match: expected %d, real %d", config_sz, sz);
        }
 
        snprintf(cmd, sizeof(cmd), "syscfg_format -d %s -f %s", CONFIG_MTD_DEVICE, CFG_RESTORE_TMP_FILE);
        ulogf(ULOG_CONFIG, UL_UTAPI, "restore: running cmd [%s]", cmd);
        system(cmd);
        free(buf);
    } else {
        ulog_errorf(ULOG_CONFIG, UL_UTAPI, "restore: couldn't open config file %s", config_fname);
        return ERR_INVALID_SYSCFG_FILE;
    }

    return UT_SUCCESS;
}

#define FW_UPGRADE_LOCK_FILE	"/tmp/.fw_upgrade_lock"

int Utopia_AcquireFirmwareUpgradeLock (int *lock_fd)
{
    if (lock_fd == NULL) {
        return ERR_INVALID_ARGS;
    }
    
    // Initialize an exclusive write-lock
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();
    
    // Open the lock file
    int fd = open(FW_UPGRADE_LOCK_FILE, O_WRONLY|O_CREAT, 0644);
    
    if (fd < 0) {
        return ERR_FILE_NOT_FOUND;
    }
    
    // Return an error if someone else has the lock
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        fclose(fd); /*RDKB-7128, CID-32972, free unused resources before exit*/
        return ERR_FW_UPGRADE_LOCK_CONFLICT;
    }
    
    // Save the lock's file descriptor
    *lock_fd = fd;
    
    return SUCCESS;
}

int Utopia_ReleaseFirmwareUpgradeLock (int lock_fd)
{
    if (lock_fd < 0) {
        return ERR_INVALID_ARGS;
    }
    
    // Initialize a file lock for unlocking
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();
    
    // Release the lock on the given lock file
    fcntl(lock_fd, F_SETLK, &fl);
    
    // Close the file
    close(lock_fd);
    
    return SUCCESS;
}

/*
 * Returns 
 *   1 - if system changes allowed
 *   0 - if disallowed
 */
int Utopia_SystemChangesAllowed (void)
{
    // Initialize an exclusive write-lock
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = 0;
    
    // Open the lock file
    int fd = open(FW_UPGRADE_LOCK_FILE, O_WRONLY);
    
    // If there's no lock file, then assume the
    // system isn't locked
    if (fd < 0) {
        return 1;
    }
    
    // Retrieve the information on the lock
    fcntl(fd, F_GETLK, &fl);
    close(fd);
    
    // If another process is holding onto the lock,
    // disallow system changes
    if (fl.l_pid > 0) {
        return 0;
    }
    
    // Otherwise, allow system changes
    return 1;
}   

/*
 * Description
 *   Check if remote mgmt user is allowed to do firmware upgrades
 * Returns: 
 *   1 - if firmware upgrade is allowed for this [http-port, http-src-ip] tuple
 *   0 - if disallowed
 * Notes:
 *   Checks if the http request is coming from Remote Management (WAN) side
 *   by looking at http-port (to see if it is the configured wan mgmt port)
 */
int Utopia_IsFirmwareUpgradeAllowed (UtopiaContext *ctx, int http_port)
{
    int wan_http_port, wan_upgrade = FALSE;

    if (NULL == ctx) {
        return 0;
    }

    Utopia_GetInt(ctx, UtopiaValue_Mgmt_WANHTTPPort, &wan_http_port);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_WANFWUpgrade, &wan_upgrade);

    if (http_port == wan_http_port && FALSE == wan_upgrade) {
        return 0;
    }

    return 1;
}

int Utopia_FirmwareUpgrade (UtopiaContext *ctx, char *firmware_file)
{
    char cmd[512];
    struct stat fstat;

    // Unused
    (void) ctx;

    if (firmware_file && 0 == stat(firmware_file, &fstat)) {
        int fw_upgrade_lock_fd;
        int exit_status;
        
        // Try to acquire firmware upgrade lock
        if (SUCCESS != Utopia_AcquireFirmwareUpgradeLock(&fw_upgrade_lock_fd)) {
            return ERR_FW_UPGRADE_LOCK_CONFLICT;
        }
        
        // TODO: use a sysevent mutex to check if we are alreadying rebooting
        ulogf(ULOG_CONFIG, UL_UTAPI, "upgrade firmware using file %s", firmware_file);
        
        snprintf(cmd, sizeof(cmd), "%s %s > %s 2>&1",
                 IMAGE_WRITER, firmware_file, IMAGE_WRITER_OUTPUT);
        exit_status = system(cmd);
        
        // Release the firmware upgrade lock
        Utopia_ReleaseFirmwareUpgradeLock(fw_upgrade_lock_fd);
        
        if (0 != exit_status) {
            return ERR_FILE_WRITE_FAILED;
        }
        
        return SUCCESS;
    }
    return ERR_INVALID_ARGS;
}

int Utopia_Reboot (void) {
    int exit_status = system("/sbin/reboot");
    return (exit_status == 0) ? SUCCESS : ERR_REBOOT_FAILED;
}

/*
 * Management APIs
 */

int Utopia_SetWebUISettings (UtopiaContext *ctx, webui_t *ui)
{
    if (NULL == ctx || NULL == ui) {
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_HTTPAccess,     ui->http_access);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_HTTPSAccess,    ui->https_access);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_WIFIAccess,     ui->wifi_mgmt_access);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_WANAccess,      ui->wan_mgmt_access);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_WANHTTPAccess,  ui->wan_http_access);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_WANHTTPSAccess, ui->wan_https_access);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_WANFWUpgrade,   ui->wan_firmware_upgrade);
    UTOPIA_SETINT(ctx,  UtopiaValue_Mgmt_WANHTTPPort,    ui->wan_http_port);
    UTOPIA_SETINT(ctx,  UtopiaValue_Mgmt_WANHTTPSPort,   ui->wan_http_port);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_WANSrcAny,      ui->wan_src_anyip);
    UTOPIA_SET(ctx,     UtopiaValue_Mgmt_WANSrcStartIP,  ui->wan_src_startip);
    UTOPIA_SET(ctx,     UtopiaValue_Mgmt_WANSrcEndIP,    ui->wan_src_endip);

    return UT_SUCCESS;
}

int Utopia_GetWebUISettings (UtopiaContext *ctx, webui_t *ui)
{
    if (NULL == ctx || NULL == ui) {
        return ERR_INVALID_ARGS;
    }

    Utopia_Get(ctx,     UtopiaValue_HTTP_AdminUser,      ui->admin_user, NAME_SZ);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_HTTPAccess,     &ui->http_access);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_HTTPSAccess,    &ui->https_access);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_WIFIAccess,     &ui->wifi_mgmt_access);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_WANAccess,      &ui->wan_mgmt_access);
    Utopia_GetInt(ctx,  UtopiaValue_Mgmt_WANHTTPPort,    &ui->wan_http_port);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_WANHTTPAccess,  &ui->wan_http_access);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_WANHTTPSAccess, &ui->wan_https_access);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_WANFWUpgrade,   &ui->wan_firmware_upgrade);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_WANSrcAny,      &ui->wan_src_anyip);
    Utopia_Get(ctx,     UtopiaValue_Mgmt_WANSrcStartIP,  ui->wan_src_startip, IPADDR_SZ);
    Utopia_Get(ctx,     UtopiaValue_Mgmt_WANSrcEndIP,    ui->wan_src_endip, IPADDR_SZ);

    return UT_SUCCESS;
}

int Utopia_SetWebUIAdminPasswd (UtopiaContext *ctx, char *username, char *cleartext_password)
{
    if (NULL == ctx || NULL == username || NULL == cleartext_password) {
        return ERR_INVALID_ARGS;
    }
    
    FILE *fp = NULL;
    char hashed_password[PASSWORD_SZ];

    fp = v_secure_popen("r", "/etc/init.d/service_httpd/httpd_util.sh generate_passwd \"%s\" \"%s\"",
                        username, cleartext_password);
    
    if (NULL == fp) {
        return ERR_FILE_NOT_FOUND;
    }
    
    if (NULL == fgets(hashed_password, PASSWORD_SZ, fp)) {
        v_secure_pclose(fp);
        return ERR_INVALID_VALUE;
    }
    
    int len = strlen(hashed_password);
    
    if (len <= 0) {
        v_secure_pclose(fp);
        return ERR_INVALID_VALUE;
    }
    
    // Remove a trailing newline
    if (hashed_password[len - 1] == '\n') {
        hashed_password[len - 1] = 0;
    }

    v_secure_pclose(fp); /*RDKB-7128, CID-33462; free unused resources*/
    fp = NULL;

    UTOPIA_SET(ctx, UtopiaValue_HTTP_AdminPassword, hashed_password);

    UTOPIA_SETBOOL(ctx, UtopiaValue_HTTP_AdminIsDefault, !strcmp(cleartext_password, DEFAULT_HTTP_ADMIN_PASSWORD));


    fp = v_secure_popen("r", "/etc/init.d/password_functions.sh admin_pw \"%s\"", cleartext_password);
   
    if (NULL == fp) {
        return ERR_FILE_NOT_FOUND;
    }
   
    if (NULL == fgets(hashed_password, PASSWORD_SZ, fp)) {
        v_secure_pclose(fp);
        return ERR_INVALID_VALUE;
    }
   
    len = strlen(hashed_password);
   
    if (len <= 0) {
        v_secure_pclose(fp);
        return ERR_INVALID_VALUE;
    }
   
    // Remove a trailing newline
    if (hashed_password[len - 1] == '\n') {
        hashed_password[len - 1] = 0;
    }

    v_secure_pclose(fp); /*RDKB-7128, CID-33462; free unused resources*/

    UTOPIA_SET(ctx, UtopiaValue_User_AdminPassword, hashed_password);

    fp = v_secure_popen("r", "/etc/init.d/password_functions.sh root_pw \"%s\"", cleartext_password);
  
    if (NULL == fp) {
        return ERR_FILE_NOT_FOUND;
    }
    
    if (NULL == fgets(hashed_password, PASSWORD_SZ, fp)) {
        v_secure_pclose(fp); 
        return ERR_INVALID_VALUE;
    }
  
    len = strlen(hashed_password);
  
    if (len <= 0) {
        v_secure_pclose(fp);
        return ERR_INVALID_VALUE;
    }
  
    // Remove a trailing newline
    if (hashed_password[len - 1] == '\n') {
        hashed_password[len - 1] = 0;
    }

    v_secure_pclose(fp);/*RDKB-7128, CID-33462; free unused resources*/

    UTOPIA_SET(ctx, UtopiaValue_User_RootPassword, hashed_password);

    return UT_SUCCESS;
}

int Utopia_IsAdminDefault (UtopiaContext *ctx, boolean_t *is_admin_default)
{
    if (NULL == ctx || NULL == is_admin_default) {
        return ERR_INVALID_ARGS;
    }
    
    Utopia_GetBool(ctx, UtopiaValue_HTTP_AdminIsDefault, is_admin_default);
    return UT_SUCCESS;
}

int Utopia_SetIGDSettings (UtopiaContext *ctx, igdconf_t *igd)
{
    if (NULL == ctx || NULL == igd) {
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_IGDEnabled,     igd->enable);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_IGDUserConfig, igd->allow_userconfig);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Mgmt_IGDWANDisable, igd->allow_wandisable);

    return UT_SUCCESS;
}

int Utopia_GetIGDSettings (UtopiaContext *ctx, igdconf_t *igd)
{
    if (NULL == ctx || NULL == igd) {
        return ERR_INVALID_ARGS;
    }

    Utopia_GetBool(ctx, UtopiaValue_Mgmt_IGDEnabled,     &igd->enable);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_IGDUserConfig, &igd->allow_userconfig);
    Utopia_GetBool(ctx, UtopiaValue_Mgmt_IGDWANDisable, &igd->allow_wandisable);

    return UT_SUCCESS;
}

#define ETC_SERVICES_FILE   "/etc/services"

// Returns 1 if a service is found with a matching port and protocol
// Returns 0 otherwise
// If a match is found, the name of the service will be copied into return buf
static int s_get_service_name (int port, protocol_t protocol, char *return_buf, int buf_len)
{
    if (port <= 0 || (protocol != TCP && protocol != UDP) || return_buf == NULL || buf_len <= 0) {
        return 0;
    }
    
    FILE *fp = fopen(ETC_SERVICES_FILE, "r");

    if (fp == NULL) {
        return 0;
    }

    char line_buf[256];
    char needle_str[32];
    int was_a_match_found = 0;

    while (fgets(line_buf, sizeof(line_buf), fp)) {
        if (protocol == TCP) {
            snprintf(needle_str, sizeof(needle_str), "\t%d/tcp", port);
        }
        else if (protocol == UDP) {
            snprintf(needle_str, sizeof(needle_str), "\t%d/udp", port);
        }
        if (strstr(line_buf, needle_str)) {
            snprintf(return_buf, buf_len, "%.*s", strcspn(line_buf, "\t"), line_buf);
            was_a_match_found = 1;
            break;
        }
    }

    fclose(fp);
    
    return was_a_match_found;
}

// Parses the log string and populates the logentry_t data structure
// Returns TRUE if the string was successfully parsed
// Returns FALSE otherwise
static boolean_t s_parse_log_entry (logentry_t *log_entry,
                                    char *log_str,
                                    boolean_t do_reverse_dns_lookup,
                                    boolean_t do_service_name_lookup)
{
    if (NULL == log_entry || NULL == log_str) {
        return FALSE;
    }
    
    protocol_t proto;
    char *str_pos;
    
    // Determine the network protocol for this log entry
    str_pos = strstr(log_str, "PROTO=");
    
    if (NULL == str_pos) {
        return FALSE;
    }
    
    str_pos += strlen("PROTO=");
    if (!strncmp(str_pos, "TCP", strlen("TCP"))) {
        proto = TCP;
    }
    else if (!strncmp(str_pos, "UDP", strlen("UDP"))) {
        proto = UDP;
    }
    else {
        // Skip this log entry if the protocol isn't TCP or UDP
        return FALSE;
    }
    
    bzero(log_entry, sizeof(logentry_t));
    
    str_pos = strstr(log_str, "SRC=");
    if (str_pos) {
        str_pos += strlen("SRC=");
        snprintf(log_entry->src, URL_SZ, "%.*s", strcspn(str_pos, " "), str_pos);
    }
    
    str_pos = strstr(log_str, "DST=");
    if (str_pos) {
        str_pos += strlen("DST=");
        snprintf(log_entry->dst, URL_SZ, "%.*s", strcspn(str_pos, " "), str_pos);
        
        if (do_reverse_dns_lookup) {
            // Do a reverse DNS lookup
            in_addr_t ip_addr = inet_addr(log_entry->dst);
            struct hostent *host_data = gethostbyaddr(&ip_addr, sizeof(in_addr_t), AF_INET);
            
            // If there is a known domain for this IP address, use the domain name instead
            if (NULL != host_data) {
                strncpy(log_entry->dst, host_data->h_name, URL_SZ);
                log_entry->dst[URL_SZ - 1] = 0;
            }
        }
    }
    
    str_pos = strstr(log_str, "DPT=");
    if (str_pos) {
        str_pos += strlen("DPT=");
        snprintf(log_entry->service_port, TOKEN_SZ, "%.*s", strcspn(str_pos, " "), str_pos);
        
        if (do_service_name_lookup) {
            if (1 != s_get_service_name(atoi(log_entry->service_port), proto, log_entry->service_port, TOKEN_SZ)) {
                snprintf(log_entry->service_port, TOKEN_SZ, "%d", atoi(log_entry->service_port));
            }
        }
    }
    
    return TRUE;
}

// Allocates and populates an array of logentry_t structs from a file
// containing a newline-separated list of log entries
// If there are no errors, this will return the final count of log entries
// If there is an error, it will return the error code,
// which should be a NEGATIVE value
static int s_populate_log_entry_array (logentry_t **log_array,
                                       char *log_filename,
                                       boolean_t do_reverse_dns_lookup,
                                       boolean_t do_service_name_lookup)
{
    FILE *log_file = fopen(log_filename, "r");
    
    if (NULL == log_file) {
        return ERR_FILE_NOT_FOUND;
    }
    
    logentry_t *_log_array = NULL;
    int count = 0;
    char line_buf[512];
    
    while (fgets(line_buf, sizeof(line_buf), log_file)) {
        logentry_t next_log_entry;
        
        if (FALSE == s_parse_log_entry(&next_log_entry,
                                       line_buf,
                                       do_reverse_dns_lookup,
                                       do_service_name_lookup))
        {
            continue;
        }
        
        // If the next log entry is different from the previous entry,
        // add it to the array. Otherwise, ignore consecutive duplicate
        // entries.
        if (count == 0
            || (strcmp(next_log_entry.src, _log_array[count - 1].src)
                || strcmp(next_log_entry.dst, _log_array[count - 1].dst)
                || strcmp(next_log_entry.service_port, _log_array[count - 1].service_port)))
        {
            _log_array = realloc(_log_array, sizeof(logentry_t) * (count + 1));
            _log_array[count] = next_log_entry;
            count++;
        }
    }
    
    fclose(log_file);
    
    *log_array = _log_array;
    
    return count;
}

// Generates a saveable log file based on the log type
static void s_generate_saveable_log (char *output_filename,
                                     logtype_t log_type,
                                     int count,
                                     logentry_t *log_array)
{
    FILE *output_file = fopen(output_filename, "w");
    int i;
    
    if (NULL == output_file) {
        return;
    }
    
    for (i = 0; i < count; i++) {
        switch (log_type) {
        case INCOMING_LOG:
            fprintf(output_file, "%s to port %s is accepted\n",
                    log_array[i].src, log_array[i].service_port);
            break;
        case OUTGOING_LOG:
            fprintf(output_file, "%s to %s:%s is accepted\n",
                    log_array[i].src, log_array[i].dst, log_array[i].service_port);
            break;
        case SECURITY_LOG:
            fprintf(output_file, "%s to %s:%s is dropped\n",
                    log_array[i].src, log_array[i].dst, log_array[i].service_port);
            break;
        default:
            break;
        }
    }
    
    fclose(output_file);
}

// Parses the DHCP log message and populates the dhcpclientlog_t data structure
// Returns TRUE if the string was successfully parsed
// Returns FALSE otherwise
static boolean_t s_parse_dhcp_log_msg (dhcpclientlog_t *dhcp_data, char *dhcp_log_msg)
{
    if (NULL == dhcp_data || NULL == dhcp_log_msg) {
        return FALSE;
    }
    
    char *str_pos;
    int i;
    int spaces_seen;
    int msg_len = strlen(dhcp_log_msg);
    
    bzero(dhcp_data, sizeof(dhcpclientlog_t));
    
    // The log message starts with the date, which is formatted like this:
    // Jan 1 12:00:00
    // Copy the log message into the timestamp field up to the third space
    for (i = 0, spaces_seen = 0; i < msg_len && i < TOKEN_SZ - 1 && spaces_seen < 3; i++) {
        switch (dhcp_log_msg[i]) {
        case ' ':
            spaces_seen++;
            if (spaces_seen >= 3) {
                break;
            }
        default:
            dhcp_data->timestamp[i] = dhcp_log_msg[i];
            break;
        }
    }
    
    // Determine the type of DHCP message
    str_pos = strstr(dhcp_log_msg, "DHCP");
    
    if (NULL == str_pos) {
        return FALSE;
    }
    
    str_pos += strlen("DHCP");
    
    if (!strncmp(str_pos, "DISCOVER", strlen("DISCOVER"))) {
        dhcp_data->msg_type = DHCPDISCOVER;
    }
    else if (!strncmp(str_pos, "OFFER", strlen("OFFER"))) {
        dhcp_data->msg_type = DHCPOFFER;
    }
    else if (!strncmp(str_pos, "REQUEST", strlen("REQUEST"))) {
        dhcp_data->msg_type = DHCPREQUEST;
    }
    else if (!strncmp(str_pos, "ACK", strlen("ACK"))) {
        dhcp_data->msg_type = DHCPACK;
    }
    else if (!strncmp(str_pos, "NAK", strlen("NAK"))) {
        dhcp_data->msg_type = DHCPNAK;
    }
    else if (!strncmp(str_pos, "DECLINE", strlen("DECLINE"))) {
        dhcp_data->msg_type = DHCPDECLINE;
    }
    else if (!strncmp(str_pos, "RELEASE", strlen("RELEASE"))) {
        dhcp_data->msg_type = DHCPRELEASE;
    }
    else if (!strncmp(str_pos, "INFORM", strlen("INFORM"))) {
        dhcp_data->msg_type = DHCPINFORM;
    }
    else {
        return FALSE;
    }
    
    switch (dhcp_data->msg_type) {
    case DHCPOFFER:
    case DHCPREQUEST:
    case DHCPACK:
    case DHCPNAK:
    case DHCPDECLINE:
    case DHCPRELEASE:
    case DHCPINFORM:
        // The next "word" is the IP address
        
        // Move the string position pointer to the start of the DHCP data
        str_pos = strchr(str_pos, ' ');
        
        if (NULL == str_pos) {
            return FALSE;
        }
        
        str_pos++;
        
        // Copy the IP address up to a space or newline
        snprintf(dhcp_data->ipaddr, IPADDR_SZ, "%.*s", strcspn(str_pos, " \n"), str_pos);
        dhcp_data->ipaddr[IPADDR_SZ - 1] = 0;
        
        // Fall through
    case DHCPDISCOVER:
        // The next "word" is the MAC address
        
        // Move the string position pointer to the start of the DHCP data
        str_pos = strchr(str_pos, ' ');
        
        if (NULL == str_pos) {
            return FALSE;
        }
        
        str_pos++;
        
        // Copy the MAC address up to a space or newline
        snprintf(dhcp_data->macaddr, MACADDR_SZ, "%.*s", strcspn(str_pos, " \n"), str_pos);
        dhcp_data->macaddr[MACADDR_SZ - 1] = 0;
        break;
    default:
        break;
    }
    
    return TRUE;
}

// Allocates and populates an array of dhcpclientlog_t structs from a file
// containing a newline-separated list of DHCP log messages
// If there are no errors, this will return the final count of messages
// If there is an error, it will return the error code,
// which should be a NEGATIVE value
static int s_populate_dhcp_log_array (dhcpclientlog_t **log_array,
                                      char *log_filename)
{
    FILE *log_file = fopen(log_filename, "r");
    
    if (NULL == log_file) {
        return ERR_FILE_NOT_FOUND;
    }
    
    dhcpclientlog_t *_log_array = NULL;
    int count = 0;
    char line_buf[512];
    
    while (fgets(line_buf, sizeof(line_buf), log_file)) {
        dhcpclientlog_t next_log_entry;
        
        if (FALSE == s_parse_dhcp_log_msg(&next_log_entry, line_buf)) {
            continue;
        }
        
        _log_array = realloc(_log_array, sizeof(dhcpclientlog_t) * (count + 1));
        _log_array[count] = next_log_entry;
        count++;
    }
    
    fclose(log_file);
    
    *log_array = _log_array;
    
    return count;
}

// Generates a saveable DHCP client log file
static void s_generate_saveable_dhcp_client_log (char *output_filename,
                                                 int count,
                                                 dhcpclientlog_t *log_array)
{
    FILE *output_file = fopen(output_filename, "w");
    int i;
    
    if (NULL == output_file) {
        return;
    }
    
    for (i = 0; i < count; i++) {
        switch (log_array[i].msg_type) {
        case DHCPDISCOVER:
            fprintf(output_file, "%s received DISCOVER from %s\n",
                    log_array[i].timestamp, log_array[i].macaddr);
            break;
        case DHCPOFFER:
            fprintf(output_file, "%s sending OFFER to 255.255.255.255 with %s\n",
                    log_array[i].timestamp, log_array[i].ipaddr);
            break;
        case DHCPREQUEST:
            fprintf(output_file, "%s received REQUEST from %s\n",
                    log_array[i].timestamp, log_array[i].macaddr);
            break;
        case DHCPACK:
            fprintf(output_file, "%s sending ACK to %s\n",
                    log_array[i].timestamp, log_array[i].ipaddr);
            break;
        case DHCPNAK:
            fprintf(output_file, "%s sending NAK to %s\n",
                    log_array[i].timestamp, log_array[i].ipaddr);
            break;
        case DHCPDECLINE:
            fprintf(output_file, "%s received DECLINE from %s\n",
                    log_array[i].timestamp, log_array[i].macaddr);
            break;
        case DHCPRELEASE:
            fprintf(output_file, "%s received RELEASE from %s\n",
                    log_array[i].timestamp, log_array[i].macaddr);
            break;
        case DHCPINFORM:
            fprintf(output_file, "%s received INFORM from %s\n",
                    log_array[i].timestamp, log_array[i].macaddr);
            break;
        default:
            break;
        }
    }
    
    fclose(output_file);
}

#define LOG_SOURCE "/var/log/messages"

#define INCOMING_LOG_PREFIX_REGEX           "UTOPIA: FW.WAN2\\(LAN\\|SELF\\) ACCEPT"
#define MAX_INCOMING_LOG_ENTRIES_TO_DISPLAY (64)

int Utopia_GetIncomingTrafficLog (UtopiaContext *ctx, int *count, logentry_t **ilog)
{
    // Unused
    (void) ctx;
    
    if (NULL == count || NULL == ilog) {
        return ERR_INVALID_ARGS;
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "grep \"%s\" %s | tac | head -n %d > %s",
             INCOMING_LOG_PREFIX_REGEX, LOG_SOURCE, MAX_INCOMING_LOG_ENTRIES_TO_DISPLAY, INCOMING_LOG_TMP_FILE);
    system(cmd);
    
    *count = s_populate_log_entry_array(ilog, INCOMING_LOG_TMP_FILE, FALSE, FALSE);
    
    // If there was an error, return the error code
    if (*count < 0) {
    	return *count;
    }
    
    s_generate_saveable_log(INCOMING_LOG_SAVE_FILE, INCOMING_LOG, *count, *ilog);
    
    return SUCCESS;
}

#define OUTGOING_LOG_PREFIX_REGEX           "UTOPIA: FW.LAN2WAN ACCEPT"
#define MAX_OUTGOING_LOG_ENTRIES_TO_DISPLAY (64)

int Utopia_GetOutgoingTrafficLog (UtopiaContext *ctx, int *count, logentry_t **olog)
{
    // Unused
    (void) ctx;
    
    if (NULL == count || NULL == olog) {
        return ERR_INVALID_ARGS;
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "grep \"%s\" %s | tac | head -n %d > %s",
             OUTGOING_LOG_PREFIX_REGEX, LOG_SOURCE, MAX_OUTGOING_LOG_ENTRIES_TO_DISPLAY, OUTGOING_LOG_TMP_FILE);
    system(cmd);
    
    *count = s_populate_log_entry_array(olog, OUTGOING_LOG_TMP_FILE, TRUE, TRUE);
    
    // If there was an error, return the error code
    if (*count < 0) {
    	return *count;
    }
    
    s_generate_saveable_log(OUTGOING_LOG_SAVE_FILE, OUTGOING_LOG, *count, *olog);
    
    return SUCCESS;
}

#define SECURITY_LOG_PREFIX_REGEX           "UTOPIA: FW.WAN\\(2SELF\\|ATTACK\\) DROP"
#define MAX_SECURITY_LOG_ENTRIES_TO_DISPLAY (64)

int Utopia_GetSecurityLog (UtopiaContext *ctx, int *count, logentry_t **slog)
{
    // Unused
    (void) ctx;
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "grep \"%s\" %s | tac | head -n %d > %s",
             SECURITY_LOG_PREFIX_REGEX, LOG_SOURCE, MAX_SECURITY_LOG_ENTRIES_TO_DISPLAY, SECURITY_LOG_TMP_FILE);
    system(cmd);
    
    *count = s_populate_log_entry_array(slog, SECURITY_LOG_TMP_FILE, FALSE, TRUE);
    
    // If there was an error, return the error code
    if (*count < 0) {
    	return *count;
    }
    
    s_generate_saveable_log(SECURITY_LOG_SAVE_FILE, SECURITY_LOG, *count, *slog);
    
    return SUCCESS;
}



#define DHCP_LOG_PREFIX_REGEX           "DHCP\\(DISCOVER\\|OFFER\\|REQUEST\\|ACK\\|NAK\\|DECLINE\\|RELEASE\\|INFORM\\)"
#define MAX_DHCP_LOG_ENTRIES_TO_DISPLAY (64)

int Utopia_GetDHCPClientLog (UtopiaContext *ctx)
{
    // Unused
    (void) ctx;
    
    int count;
    dhcpclientlog_t *dlog;
    char cmd[512];
    
    snprintf(cmd, sizeof(cmd), "grep \"%s\" %s | tac | head -n %d > %s",
             DHCP_LOG_PREFIX_REGEX, LOG_SOURCE, MAX_DHCP_LOG_ENTRIES_TO_DISPLAY, DHCP_LOG_TMP_FILE);
    system(cmd);
    
    count = s_populate_dhcp_log_array(&dlog, DHCP_LOG_TMP_FILE);
    
    // If there was an error, return the error code
    if (count < 0) {
    	return count;
    }
    
    s_generate_saveable_dhcp_client_log(DHCP_LOG_SAVE_FILE, count, dlog);
    
    free(dlog);
    
    return SUCCESS;
}

// Forks a child to run a singleton process. The given lock file ensures
// that only one lock owner will be able to run at a time.
// The arguments bin and argv are passed to the system call execv to
// execute the child process.
// The output of the child can be optionally piped to a file, specified
// by the output_filename argument.
// Additionally, an optional string output_header can be written to the
// output file before execv is called.
// Returns the PID of the child, or -1 on error.
static pid_t s_spawn_async_process(const char *lock_filename,
                                   const char *bin,
                                   char *argv[],
                                   const char *output_filename,
                                   char *output_header)
{
    if (NULL == lock_filename || NULL == bin || NULL == argv) {
        return -1;
    }
    
    // Fork a child process
    pid_t child_pid = fork();
    
    if (child_pid == 0) {
        int lock_fd;     // File descriptor for lock file
        struct flock fl; // Lock settings
        
        // Open the lock file
        lock_fd = open(lock_filename, O_WRONLY|O_CREAT);
        
        if (lock_fd < 0) {
            exit(1);
        }
        
        // Initialize the lock settings
        fl.l_type   = F_WRLCK;  // Exclusive lock
        fl.l_whence = SEEK_SET;
        fl.l_start  = 0;
        fl.l_len    = 0;
        fl.l_pid    = getpid();
        
        // If we obtain the lock,
        if (fcntl(lock_fd, F_SETLK, &fl) != -1) {
            int output_fd = -1; // File descriptor for output file
            
            if (NULL == output_filename) {
                // Close unneeded file descriptors
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
            }
            else {
                // Open the file into which the output will be piped
                output_fd = open(output_filename, O_WRONLY|O_CREAT, 0644);
                
                if (output_fd < 0) {
                    fl.l_type = F_UNLCK;
                    fcntl(lock_fd, F_SETLK, &fl);
                    close(lock_fd);
                    exit(1);
                }
                
                // Clear the previous contents of the file
                ftruncate(output_fd, 0);
                
                // If an output header was provided,
                if (NULL != output_header) {
                    // Write the header to the output file
                    write(output_fd, output_header, strlen(output_header));
                }
                
                // Pipe stdout to the output file
                close(STDOUT_FILENO);
                dup2(output_fd, STDOUT_FILENO);
                
                // Pipe stderr to the output file
                close(STDERR_FILENO);
                dup2(output_fd, STDERR_FILENO);
                
                // Close unneeded stdin
                close(STDIN_FILENO);
            }
            
            // Execute the specified binary with the given arguments
            execv(bin, argv);
            
            // Execution will only reach this point on an error.
            // If this happens, release the lock, close both files, and exit.
            if (NULL != output_filename) {
                close(output_fd);
            }
            fl.l_type = F_UNLCK;
            fcntl(lock_fd, F_SETLK, &fl);
            close(lock_fd);
            exit(1);
        }
        
        // If the child doesn't perform an exec, exit here just in case.
        exit(1);
    }
    
    // Parent resumes execution asynchronously
    
    // Return child PID
    return child_pid;
}

// Checks whether a process is holding the given lock file.
// Optionally interrupts the lock owner and forces the lock
// to be released.
// Returns 1 if a process is holding the lock.
// Returns 0 otherwise.
static int s_has_lock_been_acquired(const char *lock_filename,
                                    int interrupt_lock_owner)
{
    int lock_fd;
    struct flock fl;
    int has_been_acquired = 0;
    
    lock_fd = open(lock_filename, O_WRONLY);
    
    if (lock_fd < 0) {
        return 0;
    }
    
    // Initialize the file lock
    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    fl.l_pid    = 0;
    
    // Retrieve the info associated with the lock file
    fcntl(lock_fd, F_GETLK, &fl);
    close(lock_fd);
    
    // If a process is holding the lock,
    if (fl.l_pid > 0) {
        has_been_acquired = 1;
        
        // If the interrupt option is set,
        if (interrupt_lock_owner) {
            // Interrupt the process that owns the lock
            kill(fl.l_pid, SIGINT);
            
            // Release the lock
            fl.l_type = F_UNLCK;
            fcntl(lock_fd, F_SETLK, &fl);
        }
    }
    
    return has_been_acquired;
}

// Interrupts the owner of the given lock and forces the lock
// to be released.
static void s_interrupt_spawned_process(const char *lock_filename)
{
    s_has_lock_been_acquired(lock_filename, 1);
}

#define PING_PATHNAME "/bin/ping"
#define DIAG_PING_LOCK_FILE "/tmp/.diag_ping_lock"

int Utopia_DiagPingTestStart (char *dest, int packet_size, int num_ping)
{
    if (NULL == dest || packet_size <= 0 || num_ping < 0) {
        return ERR_INVALID_ARGS;
    }
    
    char *argv[7];
    char packet_size_buf[16];
    char num_ping_buf[8];
    char header_buf[256];
    
    // Convert the packet size into a string stored in packet_size_buf
    snprintf(packet_size_buf, sizeof(packet_size_buf), "%d", packet_size);
    packet_size_buf[sizeof(packet_size_buf) - 1] = 0;
    
    // Set up the first 3 arguments for execv
    argv[0] = "ping";
    argv[1] = "-s";
    argv[2] = packet_size_buf;
    
    if (num_ping == 0) { // Keep pinging indefinitely
    	// The -c option isn't necessary in this case
    	
    	// Set up the rest of the arguments for execv
        argv[3] = dest;
        argv[4] = (char *)0;
    }
    else {
    	// Convert the ping count into a string stored in num_ping_buf
        snprintf(num_ping_buf, sizeof(num_ping_buf), "%d", num_ping);
        num_ping_buf[sizeof(num_ping_buf) - 1] = 0;
        
        // Set up the rest of the arguments for execv
        argv[3] = "-c";
        argv[4] = num_ping_buf;
        argv[5] = dest;
        argv[6] = (char *)0;
    }
    
    // Create a header for the output file
    snprintf(header_buf, sizeof(header_buf),
             "PING %s (%s): %d data bytes\n",
             dest, dest, packet_size);
    header_buf[sizeof(header_buf) - 1] = 0;
    
    // Interrupt the previous ping instance, if there is one
    s_interrupt_spawned_process(DIAG_PING_LOCK_FILE);
    
    // Spawn a ping process
    s_spawn_async_process(DIAG_PING_LOCK_FILE,
                          PING_PATHNAME,
                          argv,
                          PING_LOG_TMP_FILE,
                          header_buf);
    
    return SUCCESS;
}

int Utopia_DiagPingTestStop (void)
{
    s_interrupt_spawned_process(DIAG_PING_LOCK_FILE);
    return SUCCESS;
}

int Utopia_DiagPingTestIsRunning (void)
{
    return s_has_lock_been_acquired(DIAG_PING_LOCK_FILE, 0);
}

#define TRACEROUTE_PATHNAME "/usr/bin/traceroute"
#define DIAG_TRACEROUTE_LOCK_FILE "/tmp/.diag_traceroute_lock"

int Utopia_DiagTracerouteTestStart (char *dest)
{
    if (NULL == dest) {
        return ERR_INVALID_ARGS;
    }
    
    int fd;
    char buf[256];
    
    // Interrupt the previous traceroute instance, if there is one
    s_interrupt_spawned_process(DIAG_PING_LOCK_FILE);
    
    // Check for two invalid corner cases:
    // 1. 0.0.0.0
    // 2. 255.255.255.255
    if (0 == strcmp(dest, "0.0.0.0") ||
        0 == strcmp(dest, "255.255.255.255"))
    {
        // Open the log file
        fd = open(TRACEROUTE_LOG_TMP_FILE, O_WRONLY|O_CREAT);
        
        if (fd < 0) {
            return ERR_FILE_NOT_FOUND;
        }
        
        // Clear the previous contents of the file
        ftruncate(fd, 0);
        
        // Write an error message to the file
        snprintf(buf, sizeof(buf), "traceroute: bad address '%s'\n", dest);
        buf[sizeof(buf) - 1] = 0;
        write(fd, buf, strlen(buf));
        
        close(fd);
    }
    else {
        char *argv[3];
        argv[0] = "traceroute";
        argv[1] = dest;
        argv[2] = (char *)0;
        
        // Create a header for the output file
        snprintf(buf, sizeof(buf), "traceroute to %s (%s)\n", dest, dest);
        buf[sizeof(buf) - 1] = 0;
        
        // Spawn a traceroute process
        s_spawn_async_process(DIAG_TRACEROUTE_LOCK_FILE,
                              TRACEROUTE_PATHNAME,
                              argv,
                              TRACEROUTE_LOG_TMP_FILE,
                              buf);
    }
    
    return SUCCESS;
}

int Utopia_DiagTracerouteTestStop (void)
{
    s_interrupt_spawned_process(DIAG_TRACEROUTE_LOCK_FILE);
    return SUCCESS;
}

int Utopia_DiagTracerouteTestIsRunning (void)
{
    return s_has_lock_been_acquired(DIAG_TRACEROUTE_LOCK_FILE, 0);
}

int Utopia_PPPConnect (void)
{
    token_t se_token;
    int se_fd = s_sysevent_connect(&se_token);
    
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    
    sysevent_set(se_fd, se_token, "wan-start", "", 0);
    
    return SUCCESS;
}

int Utopia_PPPDisconnect (void)
{
    token_t se_token;
    int se_fd = s_sysevent_connect(&se_token);
    
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    
    sysevent_set(se_fd, se_token, "wan-stop", "", 0);
    
    return SUCCESS;
}


int Utopia_SetWebTimeout(UtopiaContext *ctx, int count)
{
   UTOPIA_SETINT(ctx, UtopiaValue_Web_Timeout, count);
   return SUCCESS;
}
int Utopia_GetWebTimeout (UtopiaContext *ctx, int *count)
{
    *count = 0;
    Utopia_GetInt(ctx, UtopiaValue_Web_Timeout, count);
    return SUCCESS;
}

int Utopia_Set_Http_Admin(UtopiaContext *ctx, http_user_t *httpuser)
{
    UTOPIA_SET(ctx,UtopiaValue_HTTP_AdminUser,httpuser->username);
    UTOPIA_SET(ctx,UtopiaValue_HTTP_AdminPassword,httpuser->password);
    return SUCCESS;
}

int Utopia_Get_Http_Admin(UtopiaContext *ctx, http_user_t *httpuser)
{
    if (NULL == ctx || NULL == httpuser) {
        return ERR_UTCTX_INIT;
    }
     bzero(httpuser, sizeof(http_user_t));
     Utopia_Get(ctx,UtopiaValue_HTTP_AdminUser,httpuser->username, PASSWORD_SZ);
     Utopia_Get(ctx,UtopiaValue_HTTP_AdminPassword,httpuser->password, PASSWORD_SZ);
     return SUCCESS;
}

int Utopia_Set_Http_User(UtopiaContext *ctx, http_user_t *httpuser)
{
    UTOPIA_SET(ctx,UtopiaValue_Web_Username,httpuser->username);
    UTOPIA_SET(ctx,UtopiaValue_Web_Password,httpuser->password);
    return SUCCESS;
}
int Utopia_Get_Http_User(UtopiaContext *ctx,  http_user_t *httpuser)
{ 

    if (NULL == ctx || NULL == httpuser) {
        return ERR_UTCTX_INIT;
    }

     bzero(httpuser, sizeof(http_user_t));
     Utopia_Get(ctx,UtopiaValue_Web_Username,httpuser->username, PASSWORD_SZ);
     Utopia_Get(ctx,UtopiaValue_Web_Password,httpuser->password, PASSWORD_SZ);
     return SUCCESS;
}

/*
 * BYOI API 
 */

/* This API is used to control to UI pages display
    if the status is docsis - no UI page for internet setting
    if the status is non-docsis- UI page with WAN option(byoi) and None( no internet)
    if the status is user - UI page with Cable(primary provider), WAN(byoi) and None(no internet) will be provided*/
int Utopia_Get_BYOI_allowed(UtopiaContext *ctx,  int *byoi_state)
{  

    char buf[TOKEN_SZ];

    if (NULL == ctx) {
        return ERR_UTCTX_INIT;
    }

    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    sysevent_get(se_fd, se_token, "primary_HSD_allowed", buf, sizeof(buf));

    int status = s_StrToEnum(g_byoi_Mode, buf);

    if(-1 == status) {
	 status = USER_SELECTABLE;
     }

     *byoi_state = status;

    sprintf(ulog_msg, "%s: hsd_allowed %d", __FUNCTION__, *byoi_state);
    ulog(ULOG_CONFIG, UL_UTAPI, ulog_msg);

    return UT_SUCCESS;

}

/* This API controls the display of the current mode that the user is having*/
int Utopia_Get_BYOI_Current_Provider(UtopiaContext *ctx,  hsdStatus_t *hsdStatus)
{  
    char buf[TOKEN_SZ];

    if (NULL == ctx || NULL == hsdStatus) {
        return ERR_UTCTX_INIT;
    }

    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    sysevent_get(se_fd, se_token, "current_hsd_mode", buf, sizeof(buf));

    int status = s_StrToEnum(g_hsdStatus, buf);
    *hsdStatus = status;
    
    return UT_SUCCESS;
}


int Utopia_Set_BYOI_Desired_Provider(UtopiaContext *ctx,  hsdStatus_t hsdStatus)
{  
    char *val = "UNKNOWN";
    if (NULL == ctx) {
        return ERR_UTCTX_INIT;
    }
    val = s_EnumToStr(g_hsdStatus, hsdStatus);

    UTOPIA_SET(ctx,UtopiaValue_Last_Configured_HSD_Mode,val);
    token_t  se_token;
    int      se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }
    sysevent_set(se_fd, se_token, "desired_hsd_mode", val, 0);

    
    return UT_SUCCESS;
}

/* Separate these functions into a new TR181 UTAPI code files*/


int Utopia_Set_Prov_Code(UtopiaContext *ctx, char *val)
{
    UTOPIA_SET(ctx,UtopiaValue_TR_Prov_Code,val);
    return SUCCESS;
}

int Utopia_Get_Prov_Code(UtopiaContext *ctx,  char *val)
{ 

    if (NULL == ctx || NULL == val) {
        return ERR_INVALID_ARGS;
    }
     bzero(val, NAME_SZ);
     Utopia_Get(ctx,UtopiaValue_TR_Prov_Code, val,NAME_SZ);
     return SUCCESS;
}

int Utopia_Get_First_Use_Date(UtopiaContext *ctx,  char *val)
{

    if (NULL == ctx || NULL == val) {
        return ERR_INVALID_ARGS;
    }
    bzero(val, NAME_SZ);
    Utopia_Get(ctx,UtopiaValue_Device_FirstuseDate, val,NAME_SZ);
    return SUCCESS;

}

int Utopia_Set_DeviceTime_NTPServer(UtopiaContext *ctx, char *server, int index)
{
  if (NULL == ctx || NULL == server)
  {
     return ERR_UTCTX_INIT;
  }

  char tmp_server[UTOPIA_TR181_PARAM_SIZE] = {0};

  if('\0' == server[0])
  {
      strncpy(tmp_server, "no_ntp_address", UTOPIA_TR181_PARAM_SIZE);
  }
  else
      strncpy(tmp_server, server, UTOPIA_TR181_PARAM_SIZE);


  switch(index)
  {
  case 1:
      UTOPIA_SET(ctx, UtopiaValue_NTP_Server1, tmp_server);
      break;
  case 2:
      UTOPIA_SET(ctx, UtopiaValue_NTP_Server2, tmp_server);
      break;
  case 3:
      UTOPIA_SET(ctx, UtopiaValue_NTP_Server3, tmp_server);
      break;
  case 4:
      UTOPIA_SET(ctx, UtopiaValue_NTP_Server4, tmp_server);
      break;
  case 5:
      UTOPIA_SET(ctx, UtopiaValue_NTP_Server5, tmp_server);
      break;
  }

  return SUCCESS;
}

int Utopia_Get_DeviceTime_NTPServer(UtopiaContext *ctx, char *server,int index)
{
  int rc = -1;
  if (NULL == ctx || NULL == server)
  {
     return ERR_UTCTX_INIT;
  }
  bzero(server, UTOPIA_TR181_PARAM_SIZE);
  
  switch(index)
  {
  case 1:
      rc = Utopia_Get(ctx, UtopiaValue_NTP_Server1, server, UTOPIA_TR181_PARAM_SIZE);
      break;
  case 2:
      rc = Utopia_Get(ctx, UtopiaValue_NTP_Server2, server, UTOPIA_TR181_PARAM_SIZE);
      break;
  case 3:
      rc = Utopia_Get(ctx, UtopiaValue_NTP_Server3, server, UTOPIA_TR181_PARAM_SIZE);
      break;
  case 4:
      rc = Utopia_Get(ctx, UtopiaValue_NTP_Server4, server, UTOPIA_TR181_PARAM_SIZE);
      break;
  case 5:
      rc = Utopia_Get(ctx, UtopiaValue_NTP_Server5, server, UTOPIA_TR181_PARAM_SIZE);
      break;
  }
  if(rc)
  {
      if(0 == strncmp(server, "no_ntp_address", strlen("no_ntp_address")))
      {
          memset(server, 0, UTOPIA_TR181_PARAM_SIZE);
      }

      return SUCCESS;
  }

  /* Syscfg failed */
  return ERR_SYSCFG_FAILED;

}

int Utopia_Set_DeviceTime_LocalTZ(UtopiaContext *ctx, char *tz)
{
  if (NULL == ctx || NULL == tz)
  {
     return ERR_UTCTX_INIT;
  }

  UTOPIA_SET(ctx, UtopiaValue_TZ, tz);
  sprintf(ulog_msg, "%s: entered ", __FUNCTION__);
  ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
  return SUCCESS;
}

int Utopia_Get_DeviceTime_LocalTZ(UtopiaContext *ctx, char *tz)
{
  if (NULL == ctx || NULL == tz)
  {
     return ERR_UTCTX_INIT;
  }

  bzero(tz, UTOPIA_TR181_PARAM_SIZE1);
  if(Utopia_Get(ctx, UtopiaValue_TZ, tz, UTOPIA_TR181_PARAM_SIZE1))
  {
      return SUCCESS;
  }

  /* Syscfg failed */
  return ERR_SYSCFG_FAILED;
}

int Utopia_Set_DeviceTime_Enable(UtopiaContext *ctx, unsigned char enable)
{
  int val = -1;
  if (NULL == ctx)
  {
     return ERR_UTCTX_INIT;
  }
  val = (FALSE == enable) ? 0 : 1 ;
  Utopia_SetInt(ctx,UtopiaValue_NTP_Enabled,val);
  return SUCCESS;
}

unsigned char Utopia_Get_DeviceTime_Enable(UtopiaContext *ctx)
{
  unsigned char enbl;
  int val = -1;
  if(NULL == ctx)
  {
    return FALSE;
  }
  Utopia_GetInt(ctx,UtopiaValue_NTP_Enabled,&val);
  enbl = (0 == val) ? FALSE : TRUE ;
  return enbl;
}

int Utopia_Set_DeviceTime_DaylightEnable(UtopiaContext *ctx, unsigned char enable)
{
  int val = -1;
  if (NULL == ctx)
  {
     return ERR_UTCTX_INIT;
  }
  val = (FALSE == enable) ? 0 : 1 ;
  Utopia_SetInt(ctx,UtopiaValue_NTP_DaylightEnable,val);
  return SUCCESS;
}

unsigned char Utopia_Get_DeviceTime_DaylightEnable(UtopiaContext *ctx)
{
  unsigned char enbl;
  int val = -1;
  if(NULL == ctx)
  {
    return FALSE;
  }
  Utopia_GetInt(ctx,UtopiaValue_NTP_DaylightEnable,&val);
  enbl = (0 == val) ? FALSE : TRUE ;
  return enbl;
}

int Utopia_Set_DeviceTime_DaylightOffset(UtopiaContext *ctx, int offset)
{
   UTOPIA_SETINT(ctx, UtopiaValue_NTP_DaylightOffset, offset);
   return SUCCESS;
}

int Utopia_Get_DeviceTime_DaylightOffset(UtopiaContext *ctx, int *offset)
{
    *offset = 0;
    Utopia_GetInt(ctx, UtopiaValue_NTP_DaylightOffset, offset);
    return SUCCESS;
}

int Utopia_Get_DeviceTime_Status(UtopiaContext *ctx)
{

  int status = 0;
  Utopia_GetInt(ctx,UtopiaValue_NTP_Status, &status);
  return status;
}

int Utopia_Get_Mac_MgWan(UtopiaContext *ctx,  char *val)
{

    if (NULL == ctx || NULL == val) {
        return ERR_INVALID_ARGS;
    }
     bzero(val, MACADDR_SZ);
     Utopia_Get(ctx,UtopiaValue_MAC_MgWan,val,MACADDR_SZ);
     return SUCCESS;
}

int Utopia_GetEthAssocDevices(int unitId, int portId ,unsigned char *macAddrList,int *numMacAddr)
{
    char cmd[BUF_SZ];
    char line[LINE_SZ];
    char tok[] = "=";
    char *mac = NULL;
    char *name = NULL;
    unsigned char hexMac[MAC_SZ] = {'\0'};
    int index = 0;
    FILE *fp = NULL;
    int numAssocDev = 0;

    sprintf(cmd,"switchcfg -a %d \"l2 show\" | grep Learned | grep \"DestPort(s): %d\" | cut -d'|' -f3 | awk '{print $1$2}' > %s", unitId, portId, ETHERNET_ASSOC_DEVICE_FILE);
    system(cmd);
    if((fp = fopen(ETHERNET_ASSOC_DEVICE_FILE, "r"))== NULL ) {
        sprintf(ulog_msg, "%s: Error in File Open !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_FILE_OPEN_FAIL;
    }
    while((fgets(line, sizeof(line), fp)) != NULL)
    {
        if(line[0] == ' ' || line[0] == '\n')
            continue;
        if(strstr(line, "MAC") != NULL){
            name = strtok(line,tok);
            mac = strtok(NULL,tok);
        }else{
            continue;
        }
        /*RDKB-7128, CID-33504, NULL check before use */
        if(mac)
        {
            mac[MACADDR_SZ-1] = '\0'; /*We need only 17 chars */
        }
        getHexGeneric(mac,hexMac,MAC_SZ);

        /*we don't collect multicast MAC address*/
        if (hexMac[0] & 0x01)
            continue;
        
        memcpy((macAddrList+index),hexMac,MAC_SZ);
        numAssocDev += 1;
        index += MAC_SZ;
        memset(hexMac,0,MAC_SZ);
    }
    *numMacAddr = numAssocDev;

    if(fclose(fp) != SUCCESS){
        sprintf(ulog_msg, "%s: File Close Error !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_FILE_CLOSE_FAIL;
    }

    if(macAddrList)
        return SUCCESS;

    return ERR_NO_NODES;
}

#define UTOPIA_LANMNG_INSNUM "lanmng_insnum"
#define UTOPIA_LANMNG_ALISA "lanmng_alisa"

int Utopia_GetLanMngmCount(UtopiaContext *ctx, int *val){
    /* Not use */
    (void)ctx;

    /* Till now only support single lan */
    /* TODO: Add the count value into UtopiaValues when support mutli lan interface */
    *val = 1;
    return SUCCESS;
}

int Utopia_GetLanMngmInsNum(UtopiaContext *ctx, unsigned long int *val){
    char buf[64] = {0};
 
    /* Not use */
    (void)ctx;


    if(1 == Utopia_RawGet(ctx, NULL, UTOPIA_LANMNG_INSNUM, buf, sizeof(buf))){
        *val = atoi(buf);
    }else
        *val = 0;

    return SUCCESS;
}
    
int Utopia_SetLanMngmInsNum(UtopiaContext *ctx, unsigned long int val){
    char buf[64] = {0};
    /* Not use */
    (void)ctx;

    sprintf(buf, "%ld", val);
    if(Utopia_RawSet(ctx, NULL, UTOPIA_LANMNG_INSNUM, buf)){
	return SUCCESS;
    }else
        return ERR_INVALID_VALUE;
}

int Utopia_GetLanMngmAlias(UtopiaContext *ctx, char *buf, size_t b_len ){
     /* Not use */
    (void)ctx;

    if( buf == NULL )
        return ERR_INVALID_VALUE;
    
    if(Utopia_RawGet(ctx, NULL, UTOPIA_LANMNG_ALISA, buf, b_len)){
        return SUCCESS;
    }else{
        /* if alisa not exsist, create the alisa from ifname */
        Utopia_Get(ctx, UtopiaValue_LAN_IfName, buf, b_len);
        Utopia_RawSet(ctx, NULL, UTOPIA_LANMNG_ALISA, buf);
        return SUCCESS;
    }
}

int Utopia_SetLanMngmAlias(UtopiaContext *ctx, const char *val){
     /* Not use */
    (void)ctx;

    if( val == NULL )
        return ERR_INVALID_VALUE;
    
    if(Utopia_RawSet(ctx, NULL, UTOPIA_LANMNG_ALISA, val)){
        return SUCCESS;
    }else
        return ERR_INVALID_VALUE;
}

#if 0
int Utopia_GetLanMngmLanMode(UtopiaContext *ctx, lanMngm_LanMode_t *LanMode){
    //TO-DO
    //get Lan Mode
    return SUCCESS;
}


int Utopia_SetLanMngmLanMode(UtopiaContext *ctx, lanMngm_LanMode_t LanMode){
    boolean_t enable;
    if(LanMode == LANMODE_BRIDGE){
        Utopia_GetLanMngmLanNapt(ctx, &enable);
        /* disbale NAPT when change to bridge mode */
        if(enable){
            Utopia_SetRouteNAT(ctx, FALSE);
        }
        // To-DO 
        // Set Lan mode 
    }else{
        Utopia_GetLanMngmLanNapt(ctx, &enable);
        /* Enable NAPT when change to router mode */
        Utopia_SetRouteNAT(ctx, TRUE);
        // TO-Do
        // Set Lan mode 
    }

    return SUCCESS;
}
#endif

int Utopia_GetLanMngmLanNetworksAllow(UtopiaContext *ctx, int* allow){
    (void)ctx;
    *allow = 0;
    return SUCCESS;
}

int Utopia_SetLanMngmLanNetworksAllow(UtopiaContext *ctx, int allow){
    /* Not use */
    (void)ctx; 
  
    /* USGv2 not support */
    if(allow != 0)
        return ERR_INVALID_VALUE;
    else
        return SUCCESS;
}
/* To-Do: need be changed to support mutil-lan interface */
int Utopia_GetLanMngmLanNapt(UtopiaContext *ctx, napt_mode_t *enable){
    /* Not use */
    (void)ctx; 
  
    return Utopia_GetRouteNAT (ctx, enable);
}

/* To-Do: need be changed to support mutil-lan interface */
int Utopia_SetLanMngmLanNapt(UtopiaContext *ctx, napt_mode_t enable){
    /* Not use */
    (void)ctx; 

    return Utopia_SetRouteNAT (ctx, enable);
}

/*
 * Procedure     : trim
 * Purpose       : trims a string
 * Parameters    :
 *    in         : A string to trim
 * Return Value  : The trimmed string
 * Note          : This procedure will change the input sting in this situation
 */
static char *trim(char *in)
{
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

   return(start);
}

#define DNS_RESOLV_FILE "/etc/resolv.conf"
static int resolve_dns_server(char* line, char* dns_server){
    char *p;
    char *q;
      
    if(line == NULL || dns_server == NULL)
        return -1;
    /* remove the '\n' at the end of line */
    line[strlen(line) - 1] = '\0';

    if(NULL != (p = strstr(line, "nameserver"))){
        p += strlen("nameserver");
        while(*p == ' '){
            p++;
        }
        //Needs to remove leading and traling spce from string
        q=trim(p);
    	memcpy(dns_server, q, strlen(q)+1);
        return 0;
    }

    return -1;
}

int Utopia_SetDNSEnable(UtopiaContext *ctx, boolean_t enable){
     /* Not use */
    (void)ctx; 

     /* don't allow disable DNS client */
    if(!enable)
        return ERR_INVALID_VALUE;
    else
        return SUCCESS;
}

int Utopia_GetDNSEnable(UtopiaContext *ctx, boolean_t* enable){
    /* Not use */
    (void)ctx;   
 
    /* don't allow disable DNS client */
    *enable = TRUE;
    return SUCCESS;
}

int Utopia_GetDNSServer(UtopiaContext *ctx, DNS_Client_t * dns){
    FILE* fp;
    char line[256];
    int i = 0;
 
    /* Not use */
    (void)ctx;   
    
    if(dns == NULL)
        return ERR_INVALID_VALUE;
    
    if(NULL == (fp = fopen(DNS_RESOLV_FILE, "r"))){
        return ERR_FILE_NOT_FOUND;
    }
    memset(dns, 0, sizeof(DNS_Client_t));
    while(fgets(line, sizeof(line), fp) && i < DNS_CLIENT_NAMESERVER_CNT){
        if(0 == resolve_dns_server(line, &(dns->dns_server[i][0])))
            i++;
    }

    fclose(fp);

    return SUCCESS;
}

static int get_lan_host_index_and_comments(UtopiaContext *ctx, char *macStr,int *pIndex, char *pComments)
{
	char buffer[256];

	buffer[0] = 0;
	pComments[0] = 0;
	*pIndex = 0;
	Utopia_GetNamed(ctx, UtopiaValue_USGv2_Lan_Clients_Mac, macStr, buffer, sizeof(buffer));
	if(buffer[0]){
		char *p;

		*pIndex = atoi(buffer);
		p = strchr(buffer, '+');
		if(p!=NULL){
			strcpy(pComments,p+1);
		}
	}
	return(SUCCESS);
}

int Utopia_get_lan_host_comments(UtopiaContext *ctx, unsigned char *pMac, unsigned char *pComments)
{
	int index;
	char macStr[18];

	if((ctx==NULL)||(pMac==NULL)||(pComments==NULL))
		return(ERR_INVALID_ARGS);
	sprintf(macStr,"%02x:%02x:%02x:%02x:%02x:%02x",
		pMac[0],pMac[1],pMac[2],pMac[3],pMac[4],pMac[5]);
	get_lan_host_index_and_comments(ctx,macStr,&index,pComments);
	return(SUCCESS);
}

int Utopia_set_lan_host_comments(UtopiaContext *ctx, unsigned char *pMac, unsigned char *pComments)
{
	int count = 0, index1=0, index2=0;
	char macStr1[18], macStr2[18];
	unsigned char comments1[64],comments2[64], buffer[128];
	
	if((ctx==NULL)||(pMac==NULL)||(pComments==NULL))
		return(ERR_INVALID_ARGS);
	Utopia_GetInt(ctx, UtopiaValue_USGv2_Lan_Clients_Count, &count);
	sprintf(macStr1,"%02x:%02x:%02x:%02x:%02x:%02x",
		pMac[0],pMac[1],pMac[2],pMac[3],pMac[4],pMac[5]);
	get_lan_host_index_and_comments(ctx,macStr1,&index1,comments1);
	if(pComments[0]==0){/*remove comments for a client*/
		if(index1 <=0 )
			return(SUCCESS);
		if( count <=0 ){
			Utopia_UnsetNamed(ctx, UtopiaValue_USGv2_Lan_Clients_Mac, macStr1);
			return(SUCCESS);
		}
		if(index1 == count){
			Utopia_UnsetNamed(ctx, UtopiaValue_USGv2_Lan_Clients_Mac, macStr1);
			Utopia_UnsetIndexed(ctx, UtopiaValue_USGv2_Lan_Clients, index1);
		}else{/*user the last one to replace index1*/
			macStr2[0] = 0;
			Utopia_GetIndexedKey(ctx, UtopiaValue_USGv2_Lan_Clients, count, macStr2, sizeof(macStr2));
			if(macStr2[0] == 0)
				return(ERR_INVALID_VALUE);
			get_lan_host_index_and_comments(ctx,macStr2,&index2,comments2);
			if(index2 <= 0)
				return(ERR_INVALID_VALUE);
			Utopia_UnsetNamed(ctx, UtopiaValue_USGv2_Lan_Clients_Mac, macStr1);
			snprintf(buffer, sizeof(buffer),"%d+%s",index1,comments2);
			Utopia_SetNamed(ctx, UtopiaValue_USGv2_Lan_Clients_Mac, macStr2, buffer);
			Utopia_SetIndexed(ctx, UtopiaValue_USGv2_Lan_Clients, index1, macStr2);
			Utopia_UnsetIndexed(ctx, UtopiaValue_USGv2_Lan_Clients, count);
		}
		Utopia_SetInt(ctx, UtopiaValue_USGv2_Lan_Clients_Count, count-1);
	}else{
		if(strlen(pComments) >= 64)
			return(ERR_INVALID_ARGS);
		if(index1 <= 0){/*a new one*/
			Utopia_SetIndexed(ctx, UtopiaValue_USGv2_Lan_Clients, count+1, macStr1);
			snprintf(buffer, sizeof(buffer),"%d+%s",count+1,pComments);
			Utopia_SetNamed(ctx, UtopiaValue_USGv2_Lan_Clients_Mac, macStr1, buffer);
			Utopia_SetInt(ctx, UtopiaValue_USGv2_Lan_Clients_Count, count+1);
		}else{
			snprintf(buffer, sizeof(buffer),"%d+%s",index1,pComments);
			Utopia_SetNamed(ctx, UtopiaValue_USGv2_Lan_Clients_Mac, macStr1, buffer);
		}
	}
	
	return SUCCESS;
}

int Utopia_privateIpCheck(char *ip_to_check)
{
    struct in_addr l_sIpValue, l_sDhcpStart, l_sDhcpEnd;
    long int l_iIpValue, l_iDhcpStart, l_iDhcpEnd;
	char l_cDhcpStart[16] = {0}, l_cDhcpEnd[16] = {0};
	int l_iRes;

	if (NULL == ip_to_check || 0 == ip_to_check[0])
	{
		return 1;
	}

	syscfg_get(NULL, "dhcp_start", l_cDhcpStart, sizeof(l_cDhcpStart));
	syscfg_get(NULL, "dhcp_end", l_cDhcpEnd, sizeof(l_cDhcpEnd));

    l_iRes = inet_pton(AF_INET, ip_to_check, &l_sIpValue);
    l_iRes &= inet_pton(AF_INET, l_cDhcpStart, &l_sDhcpStart);
    l_iRes &= inet_pton(AF_INET, l_cDhcpEnd, &l_sDhcpEnd);

    l_iIpValue = (long int)l_sIpValue.s_addr;
    l_iDhcpStart = (long int)l_sDhcpStart.s_addr;
    l_iDhcpEnd = (long int)l_sDhcpEnd.s_addr;

	switch(l_iRes) 
	{
    	case 1:
        	if (l_iIpValue <= l_iDhcpEnd && l_iIpValue >= l_iDhcpStart)
		  	{	
				return 1;
			}
          	else
			{
          		return 0;
			}
       case 0:
          return 1;
       default:
          return 1;
    }
}

int Utopia_IPRule_ephemeral_port_forwarding( portMapDyn_t *pmap, boolean_t isCallForAdd )
{
	token_t	se_token;
	int		se_fd 		 = s_sysevent_connect( &se_token ),
			isBridgeMode = FALSE,
			isWanReady   = FALSE,
			isNatReady	 = FALSE,
			isNatRedirectionBlocked = FALSE,
			rc = 0;
	char 	external_ip[ 64 ],
			external_dest_port[ 64 ],
			str[ 512 ],
			port_modifier[ 10 ],
			event_string[ 64 ],
			natip4[ 64 ],
			fromip[ 64 ],
			fromport[ 16 ],
			toip[ 64 ],
			dport[ 16 ],
			lan_ipaddr[ 64 ],
			lan_3_octets[ 32 ],
			*p,
			lan_netmask[ 32 ],
			ciptableOprationCode = 'D';

	if ( 0 > se_fd ) 
	{
	   return ERR_SYSEVENT_CONN;
	}

	/* port mapping is not enabled so no need to do anything */
	if( FALSE == pmap->enabled )
	{
		return ERR_INVALID_VALUE;
	}

	/* Get Bridge-Mode value */
	memset( event_string, 0, sizeof( event_string ) );
    sysevent_get(se_fd, se_token, "bridge_mode", event_string, sizeof( event_string ) - 1 );
	isBridgeMode		= ( 0 == strcmp( "0", event_string ) ) ? 0 : 1;

	/* NAT Ip4  & NAT, WAN Ready */
	memset( natip4, 0, sizeof( natip4 ) );
    sysevent_get(se_fd, se_token, "current_wan_ipaddr", natip4, sizeof( natip4 ) - 1 );
	isWanReady		  	= ( 0 == strcmp( "0.0.0.0", natip4 ) ) ? 0 : 1;
	isNatReady		  	= isWanReady;

	/* Lan IP Address, Octets and LAN Net mask */
	memset( lan_ipaddr, 0, sizeof( lan_ipaddr ) );
    sysevent_get(se_fd, se_token, "current_lan_ipaddr", lan_ipaddr, sizeof( lan_ipaddr ) - 1 );

	// the first 3 octets of the lan ip address
	memset( lan_3_octets, 0, sizeof( lan_3_octets ) );
	snprintf( lan_3_octets, sizeof( lan_3_octets ), "%s", lan_ipaddr );
	for ( p = lan_3_octets + strlen( lan_3_octets ); p >= lan_3_octets; p-- ) 
	{
	   if ( *p == '.' ) 
	   {
		  *p = '\0';
		  break;
	   } 
	   else 
	   {
		  *p = '\0';
	   }
	}

	memset( lan_netmask, 0, sizeof( lan_netmask ) );
	syscfg_get( NULL, "lan_netmask", lan_netmask, sizeof( lan_netmask ) ); 

	/* NatRedirectionBlocked status */
	memset( event_string, 0, sizeof( event_string ) );
	rc = syscfg_get( NULL, "block_nat_redirection", event_string, sizeof( event_string ));
	if ( ( 0 == rc ) && ('\0' != event_string[ 0 ] ) ) 
	{
	   if ( 0 == strcmp( "1", event_string))
	   {
		  isNatRedirectionBlocked = 1;
	   }
	}

	/* 
	 * 1 - isCallForAdd, for Add so needs to append Operation Code as 'A'
 	 * 0 - isCallForAdd, for Delete so needs to append Operation Code as 'D'
	 */
	if ( TRUE == isCallForAdd )
	{
		ciptableOprationCode = 'A';
	}
	else
	{
		ciptableOprationCode = 'D';
	}
		

	/* external & destination IP-Host */
	memset( external_ip, 0, sizeof( external_ip ) );
	memset( external_dest_port, 0, sizeof( external_dest_port ) );	
	memset( fromip, 0, sizeof( fromip ) );
	memset( fromport, 0, sizeof( fromport ) );	
	memset( toip, 0, sizeof( toip ) );
	memset( dport, 0, sizeof( dport ) );	

	sprintf( fromip, "%s", (strlen(pmap->external_host) == 0) ? "none" : pmap->external_host );
	sprintf( fromport, "%d", pmap->external_port );

	sprintf( toip, "%s", pmap->internal_host );
	sprintf( dport, "%d", pmap->internal_port );

	if ( 0 != strcmp( "none", fromip ) ) 
	{
		snprintf( external_ip, sizeof( external_ip ), "-s %s", fromip ); 
	} 

	if ( 0 != strcmp( "none", fromport ) ) 
	{
		snprintf( external_dest_port, sizeof( external_dest_port ), "--dport %s", fromport );
	} 

	if ( ('\0' == dport[ 0 ] ) || ( 0 == strcmp( fromport, dport ) ) ) 
	{
		port_modifier[ 0 ] = '\0';
	} 
	else 
	{
		snprintf( port_modifier, sizeof( port_modifier ), ":%s", dport );
	}

	     
	if ( ( ( BOTH_TCP_UDP == pmap->protocol ) || ( TCP == pmap->protocol ) ) && \
		 ( !( isBridgeMode && Utopia_privateIpCheck( pmap->internal_host ) ) )
		)
	{
		if ( isNatReady ) 
		{
			memset( str, 0, sizeof( str ) );
			snprintf(str, sizeof(str), 
				"iptables -t nat -%c prerouting_fromwan -p tcp -m tcp -d %s %s %s -j DNAT --to-destination %s%s",
				ciptableOprationCode,natip4, external_dest_port, external_ip, toip, port_modifier);
			system( str );
		}

		if ( !isNatRedirectionBlocked ) 
		{
			if (0 == strcmp("none", fromip)) 
			{
				memset( str, 0, sizeof( str ) );
				snprintf(str, sizeof(str),
					"iptables -t nat -%c prerouting_fromlan -p tcp -m tcp -d %s %s %s -j DNAT --to-destination %s%s",
					ciptableOprationCode,lan_ipaddr, external_dest_port, external_ip, toip, port_modifier);
				system( str );

				if ( isNatReady )
				{
					memset( str, 0, sizeof( str ) );
					snprintf(str, sizeof(str),
						"iptables -t nat -%c prerouting_fromlan -p tcp -m tcp -d %s %s %s -j DNAT --to-destination %s%s",
						ciptableOprationCode,natip4, external_dest_port, external_ip, toip, port_modifier);
					system( str );
				}

				memset( str, 0, sizeof( str ) );
				snprintf(str, sizeof(str),
					"iptables -t nat -%c postrouting_tolan -s %s.0/%s -p tcp -m tcp -d %s --dport %s -j SNAT --to-source %s", 
					ciptableOprationCode,lan_3_octets, lan_netmask, toip, dport, lan_ipaddr);
				system( str );
			}
		}

		/*  it will applicable during router mode */
		if( 0 == isBridgeMode )
		{
			memset( str, 0, sizeof( str ) );
			snprintf(str, sizeof(str),
				"iptables -t filter -%c wan2lan_forwarding_accept -p tcp -m tcp %s -d %s --dport %s -j xlog_accept_wan2lan", 
				ciptableOprationCode,external_ip, toip, dport);
			system( str );
		}
	}

     if ( ( ( BOTH_TCP_UDP == pmap->protocol ) || ( UDP == pmap->protocol ) ) && \
		  ( !( isBridgeMode && Utopia_privateIpCheck( pmap->internal_host ) ) )
		)
	 {
		if (isNatReady) 
		{
		   memset( str, 0, sizeof( str ) );
           snprintf(str, sizeof(str),
                   "iptables -t nat -%c prerouting_fromwan -p udp -m udp -d %s %s %s -j DNAT --to-destination %s%s",
                   ciptableOprationCode,natip4, external_dest_port, external_ip, toip, port_modifier);
		   system( str );
        }

        if ( !isNatRedirectionBlocked ) 
		{
           if (0 == strcmp("none", fromip)) 
		   {
			  memset( str, 0, sizeof( str ) );
              snprintf(str, sizeof(str),
                "iptables -t nat -%c prerouting_fromlan -p udp -m udp -d %s %s %s -j DNAT --to-destination %s%s",
                ciptableOprationCode,lan_ipaddr, external_dest_port, external_ip, toip, port_modifier);
			  system( str );

              if ( isNatReady ) 
			  {
				 memset( str, 0, sizeof( str ) );
                 snprintf(str, sizeof(str),
                   "iptables -t nat -%c prerouting_fromlan -p udp -m udp -d %s %s %s -j DNAT --to-destination %s%s",
                   ciptableOprationCode,natip4, external_dest_port, external_ip, toip, port_modifier);
				 system( str );
              }

			  memset( str, 0, sizeof( str ) );
              snprintf(str, sizeof(str),
                    "iptables -t nat -%c postrouting_tolan -s %s.0/%s -p udp -m udp -d %s --dport %s -j SNAT --to-source %s", 
                      ciptableOprationCode,lan_3_octets, lan_netmask, toip, dport, lan_ipaddr);
			  system( str );
           }
        }

		/*  it will applicable during router mode */
		if( 0 == isBridgeMode )
		{
			memset( str, 0, sizeof( str ) );
			snprintf(str, sizeof(str),
					"iptables -t filter -%c wan2lan_forwarding_accept -p udp -m udp %s -d %s --dport %s -j xlog_accept_wan2lan", 
						ciptableOprationCode,external_ip, toip, dport);
			system( str );
		}
     }
 
  return SUCCESS;
}

int Utopia_GetDynamicDnsClientInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins)
{
    return Utopia_GetIndexedInt(ctx, UtopiaValue_DynamicDnsClient_InsNum, uIndex+1, ins);
}

static int g_DynamicDnsClientCount = 0;

int Utopia_GetNumberOfDynamicDnsClient(UtopiaContext *ctx, int *num)
{
    int rc = SUCCESS;

    if(g_DynamicDnsClientCount == 0)
    {
        Utopia_GetInt(ctx, UtopiaValue_DynamicDnsClientCount, &g_DynamicDnsClientCount);
    }

    *num = g_DynamicDnsClientCount;
    return rc;
}


int Utopia_GetDynamicDnsClientByIndex(UtopiaContext *ctx, unsigned long ulIndex, DynamicDnsClient_t *DynamicDnsClient)
{
    int index = ulIndex + 1;
    int ins_num = 0;

    Utopia_GetIndexedInt(ctx, UtopiaValue_DynamicDnsClient_InsNum, index, &ins_num); DynamicDnsClient->InstanceNumber = ins_num;
    Utopia_GetIndexedBool(ctx, UtopiaValue_DynamicDnsClient_Enable, index, &DynamicDnsClient->Enable);
    Utopia_GetIndexed(ctx, UtopiaValue_DynamicDnsClient_Alias, index, DynamicDnsClient->Alias, sizeof(DynamicDnsClient->Alias));
    Utopia_GetIndexed(ctx, UtopiaValue_DynamicDnsClient_Username, index, DynamicDnsClient->Username, sizeof(DynamicDnsClient->Username));
    Utopia_GetIndexed(ctx, UtopiaValue_DynamicDnsClient_Password, index, DynamicDnsClient->Password, sizeof(DynamicDnsClient->Password));
    Utopia_GetIndexed(ctx, UtopiaValue_DynamicDnsClient_Server, index, DynamicDnsClient->Server, sizeof(DynamicDnsClient->Server));

    return 0;
}

int Utopia_SetDynamicDnsClientByIndex(UtopiaContext *ctx, unsigned long ulIndex, const DynamicDnsClient_t *DynamicDnsClient)
{
    int index = ulIndex + 1;
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "arddnsclient_%d", index);
    Utopia_SetIndexed(ctx, UtopiaValue_DynamicDnsClient, index, s_tokenbuf);

    Utopia_SetIndexedInt(ctx, UtopiaValue_DynamicDnsClient_InsNum, index, DynamicDnsClient->InstanceNumber);
    Utopia_SetIndexed(ctx, UtopiaValue_DynamicDnsClient_Alias, index, (char*)DynamicDnsClient->Alias);
    Utopia_SetIndexedBool(ctx, UtopiaValue_DynamicDnsClient_Enable, index, DynamicDnsClient->Enable);
    Utopia_SetIndexed(ctx, UtopiaValue_DynamicDnsClient_Username, index, (char *)DynamicDnsClient->Username);
    Utopia_SetIndexed(ctx, UtopiaValue_DynamicDnsClient_Password, index, (char *)DynamicDnsClient->Password);
    Utopia_SetIndexed(ctx, UtopiaValue_DynamicDnsClient_Server, index, (char *)DynamicDnsClient->Server);

    return 0;
}

int Utopia_SetDynamicDnsClientInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ins, const char *alias)
{
    int index = ulIndex+1;
    Utopia_SetIndexedInt(ctx, UtopiaValue_DynamicDnsClient_InsNum, index, ins);
    Utopia_SetIndexed(ctx, UtopiaValue_DynamicDnsClient_Alias, index, (char*)alias);
    return 0;
}

int Utopia_AddDynamicDnsClient(UtopiaContext *ctx, const DynamicDnsClient_t *DynamicDnsClient)
{
    int index;

    Utopia_GetNumberOfDynamicDnsClient(ctx, &index);

    g_DynamicDnsClientCount++;
    Utopia_SetInt(ctx, UtopiaValue_DynamicDnsClientCount, g_DynamicDnsClientCount);

    Utopia_SetDynamicDnsClientByIndex(ctx, index, DynamicDnsClient);

    return 0;
}


int Utopia_DelDynamicDnsClient(UtopiaContext *ctx, unsigned long ins)
{
    int count, index;

    Utopia_GetNumberOfDynamicDnsClient(ctx, &count);
    for (index = 0; index < count; index++)
    {
        int ins_num;
        Utopia_GetDynamicDnsClientInsNumByIndex(ctx, index, &ins_num);
        if (ins_num == (int)ins)
       {
            break;
        }
    }

    if (index >= count)
    {
        return -1;
    }

    if (index < count-1)
    {
        for (;index < count-1; index++)
        {
            DynamicDnsClient_t DynamicDnsClient;
            Utopia_GetDynamicDnsClientByIndex(ctx, index+1, &DynamicDnsClient);
            Utopia_SetDynamicDnsClientByIndex(ctx, index, &DynamicDnsClient);
        }
    }
    Utopia_UnsetIndexed(ctx, UtopiaValue_DynamicDnsClient_InsNum, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_DynamicDnsClient_Enable, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_DynamicDnsClient_Alias, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_DynamicDnsClient_Username, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_DynamicDnsClient_Password, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_DynamicDnsClient_Server, count);
    g_DynamicDnsClientCount--;
    Utopia_SetInt(ctx, UtopiaValue_DynamicDnsClientCount, g_DynamicDnsClientCount);
    return 0;
}

int Utopia_GetVLANTerminationInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins)
{
    return Utopia_GetIndexedInt(ctx, UtopiaValue_VLANTermination_InsNum, uIndex+1, ins);
}

static int g_VLANTerminationCount = 0;

int Utopia_GetNumberOfVLANTermination(UtopiaContext *ctx, int *num)
{
    int rc = SUCCESS;

    if(g_VLANTerminationCount == 0)
    {
        Utopia_GetInt(ctx, UtopiaValue_VLANTerminationCount, &g_VLANTerminationCount);
    }

    *num = g_VLANTerminationCount;
    return rc;
}

int Utopia_GetVLANTerminationByIndex(UtopiaContext *ctx, unsigned long ulIndex, vlantermination_t *vt)
{
    int index = ulIndex + 1;
    int ins_num = 0;
    Utopia_GetIndexedInt(ctx, UtopiaValue_VLANTermination_InsNum, index, &ins_num);
    vt->InstanceNumber = ins_num;
    Utopia_GetIndexedBool(ctx, UtopiaValue_VLANTermination_Enable, index, &vt->Enable);
    Utopia_GetIndexed(ctx, UtopiaValue_VLANTermination_Alias, index, vt->Alias, sizeof(vt->Alias));
    Utopia_GetIndexed(ctx,UtopiaValue_VLANTermination_Name , index, vt->Name, sizeof(vt->Name));
    Utopia_GetIndexed(ctx,UtopiaValue_VLANTermination_LowerLayer , index, vt->LowerLayer, sizeof(vt->LowerLayer));
    Utopia_GetIndexed(ctx,UtopiaValue_VLANTermination_EthLinkName , index, vt->EthLinkName, sizeof(vt->EthLinkName));
    Utopia_GetIndexedInt(ctx, UtopiaValue_VLANTermination_VLANID, index,&vt->VLANID);
    Utopia_GetIndexedInt(ctx, UtopiaValue_VLANTermination_TPID, index,&vt->TPID);
    return 0;
}
int Utopia_SetVLANTerminationByIndex(UtopiaContext *ctx, unsigned long ulIndex, const vlantermination_t *vt)
{
    int index = ulIndex + 1;
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "ardvlantermination_%d", index);
    Utopia_SetIndexed(ctx, UtopiaValue_VLANTermination, index, s_tokenbuf);

    Utopia_SetIndexedInt(ctx, UtopiaValue_VLANTermination_InsNum, index, vt->InstanceNumber);
    Utopia_SetIndexed(ctx, UtopiaValue_VLANTermination_Alias, index, (char*)vt->Alias);
    Utopia_SetIndexedBool(ctx, UtopiaValue_VLANTermination_Enable, index, vt->Enable);
    Utopia_SetIndexed(ctx, UtopiaValue_VLANTermination_Name, index, (char *)vt->Name);
    Utopia_SetIndexed(ctx, UtopiaValue_VLANTermination_LowerLayer, index, (char *)vt->LowerLayer);
    Utopia_SetIndexed(ctx, UtopiaValue_VLANTermination_EthLinkName, index,(char *)vt->EthLinkName);
    Utopia_SetIndexedInt(ctx, UtopiaValue_VLANTermination_TPID, index, vt->TPID);
    Utopia_SetIndexedInt(ctx, UtopiaValue_VLANTermination_VLANID, index, vt->VLANID);

    return 0;
}

int Utopia_SetVLANTerminationInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ins, const char *alias)
{
    int index = ulIndex+1;
    Utopia_SetIndexedInt(ctx, UtopiaValue_VLANTermination_InsNum, index, ins);
    Utopia_SetIndexed(ctx, UtopiaValue_VLANTermination_Alias, index, (char*)alias);
    return 0;
}
int Utopia_AddVLANTermination(UtopiaContext *ctx, const vlantermination_t *vt)
{
    int index = 0;

    Utopia_GetNumberOfVLANTermination(ctx, &index);

    g_VLANTerminationCount++;
    Utopia_SetInt(ctx, UtopiaValue_VLANTerminationCount, g_VLANTerminationCount);

    Utopia_SetVLANTerminationByIndex(ctx, index, vt);

    return 0;
}

int Utopia_DelVLANTermination(UtopiaContext *ctx, unsigned long ins)
{
    int count = 0, index = 0;

    Utopia_GetNumberOfVLANTermination(ctx, &count);
    for (index = 0; index < count; index++)
    {
        int ins_num = 0;
        Utopia_GetVLANTerminationInsNumByIndex(ctx, index, &ins_num);
        if (ins_num == (int)ins)
       {
            break;
        }
    }

    if (index >= count)
    {
        return -1;
    }

    if (index < count-1)
    {
        for (;index < count-1; index++)
{
            vlantermination_t vt;
            Utopia_GetVLANTerminationByIndex(ctx, index+1, &vt);
            Utopia_SetVLANTerminationByIndex(ctx, index, &vt);
        }
    }
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_InsNum, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_Alias, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_LowerLayer, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_Name, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_TPID, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_VLANID, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_Enable, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_VLANTermination_EthLinkName, count);
    g_VLANTerminationCount--;
    Utopia_SetInt(ctx, UtopiaValue_VLANTerminationCount, g_VLANTerminationCount);

    Utopia_GetNumberOfVLANTermination(ctx, &count);
    for (index = 0; index < count; index++)
    {
        vlantermination_t vt;
        Utopia_GetVLANTerminationByIndex(ctx, index, &vt);
    }
    return 0;
}
