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

#ifndef _UTAPI_H_
#define _UTAPI_H_

#include <utctx/utctx_api.h>

#ifndef UTAPI_UNITTEST
#include <ulog/ulog.h>
#include <sysevent/sysevent.h>
#else

#define ULOG_CONFIG "config"
#define UL_UTAPI    "utapi"
#define UL_WLANCFG "wlan"
#define ulog(x,y,msg)       printf("%s.%s %s", ULOG_CONFIG, UL_UTAPI, msg);
#define ulog_debug(x,y,msg) printf("%s.%s %s", ULOG_CONFIG, UL_UTAPI, msg);
#define ulog_error(x,y,msg) printf("%s.%s ERROR:%s", ULOG_CONFIG, UL_UTAPI, msg);
#define ulogf(x,y,format,...) printf(format, __VA_ARGS__)
#define ulog_debugf(x,y,format,...) printf(format, __VA_ARGS__)
#define syscfg_getall(x,y,z) printf("Calling syscfg_getall()...\n")

typedef int token_t;
#define SE_SERVER_WELL_KNOWN_PORT 5000
#define sysevent_close(y,z)       printf("Closing sysevent...\n")
#define sysevent_open(v,w,x,y,z)  printf("Initializing sysevent...\n")
#define sysevent_get(v,w,x,y,z)   printf("Sysevent get...\n");
#define sysevent_set(w,x,y,z,c)     printf("Sysevent set...\n");
#define sysevent_set_unique(u,v,w,x,y,z)     printf("Sysevent set unique...\n");
#endif

/*
 * General
 */
 
typedef char* String;
typedef int boolean_t;

#define FALSE 0
#define TRUE  1

/*
 * Administration Settings
 */
#define LANG_SZ 8
#define IPADDR_SZ 40       // IPv4 or IPv6 address
#define MACADDR_SZ 18      // MAC address aa:bb:cc:dd:ee:ff
#define IPHOSTNAME_SZ 128  // hostname foo.domain.com or IP address
#define PORT_SZ    6       // port number 0 - 65535
#define URL_SZ   512       // http://foo.domain.com/blahblah
#define IFNAME_SZ 16       // size of interface names like br0, vlan2, eth3 
#define TOKEN_SZ 128       // generic token 
#define USERNAME_SZ 64
#define PASSWORD_SZ 64
#define WAN_SERVICE_NAME_SZ 64
#define NAME_SZ 64      // generic friendly name size
#define IAP_KEYWORD_SZ    64
#define TR_ALIAS_SZ 65


/*
 * Platform specific settings
 */
#define CONFIG_MTD_DEVICE   "/dev/mtd3"
#define IMAGE_WRITER        "/usr/sbin/image_writer"
#define IMAGE_WRITER_OUTPUT "/tmp/image_writer.out"

#define CFG_MAGIC            0xFEEDDADE
#define CFG_VERSION          0x00000001
#define CFG_RESTORE_TMP_FILE "/tmp/.cfg_restore_tmp"

#define DEFAULT_HTTP_ADMIN_PASSWORD "admin"

#define INCOMING_LOG_TMP_FILE      "/tmp/.incoming_log_tmp"
#define OUTGOING_LOG_TMP_FILE      "/tmp/.outgoing_log_tmp"
#define SECURITY_LOG_TMP_FILE      "/tmp/.security_log_tmp"
#define DHCP_LOG_TMP_FILE          "/tmp/.dhcp_log_tmp"

// Log dumps that can be accessed through the web interface
// If any of these values change, then Makefile.nfsroot needs
// to be updated to reflect the changes
#define INCOMING_LOG_SAVE_FILE     "/tmp/incoming_log.txt"
#define OUTGOING_LOG_SAVE_FILE     "/tmp/outgoing_log.txt"
#define SECURITY_LOG_SAVE_FILE     "/tmp/security_log.txt"
#define DHCP_LOG_SAVE_FILE         "/tmp/dhcp_log.txt"
#define INCOMING_LOG_SAVE_FILE_URL "incoming_log.txt"
#define OUTGOING_LOG_SAVE_FILE_URL "outgoing_log.txt"
#define SECURITY_LOG_SAVE_FILE_URL "security_log.txt"
#define DHCP_LOG_SAVE_FILE_URL     "dhcp_log.txt"

#define PING_LOG_TMP_FILE          "/tmp/.ping_log_tmp"
#define TRACEROUTE_LOG_TMP_FILE    "/tmp/.traceroute_log_tmp"

#define ROUTE_TABLE_TMP_FILE       "/tmp/.route_table_tmp"

typedef struct {
    unsigned int magic;
    unsigned int len;
    unsigned int version; 
    unsigned int crc32;
} config_hdr_t;


/*
 * Error code
 */
typedef enum {
    SUCCESS                       =  0,
    UT_SUCCESS                    =  0,
    ERR_UTCTX_OP                  = -1,
    ERR_INSUFFICIENT_MEM          = -2,
    ERR_ITEM_NOT_FOUND            = -3,
    ERR_INVALID_VALUE             = -4,
    ERR_INVALID_INT_VALUE         = -5,
    ERR_INVALID_BOOL_VALUE        = -6,
    ERR_INVALID_IP                = -7,
    ERR_INVALID_NETMASK           = -8,
    ERR_INVALID_MAC               = -9,
    ERR_INVALID_WAN_TYPE          = -10,
    ERR_INVALID_DDNS_TYPE         = -11,
    ERR_SYSEVENT_CONN             = -12,
    ERR_INVALID_ARGS              = -13,
    ERR_INVALID_PORT_RANGE        = -14,
    ERR_WIFI_INVALID_MODE         = -15,
    ERR_REBOOT_FAILED             = -16,
    ERR_NOT_YET_IMPLEMENTED       = -17,
    ERR_FILE_NOT_FOUND            = -18,
    ERR_UTCTX_INIT                = -19,
    ERR_SYSCFG_FAILED             = -20,
    ERR_INVALID_SYSCFG_FILE       = -21,
    ERR_CFGRESTORE_BAD_MAGIC      = -22,
    ERR_CFGRESTORE_BAD_SIZE       = -23,
    ERR_CFGRESTORE_BAD_VERSION    = -24,
    ERR_CFGRESTORE_BAD_CRC32      = -25,
    ERR_FILE_READ_FAILED          = -26,
    ERR_FILE_WRITE_FAILED         = -27,
    ERR_FW_UPGRADE_LOCK_CONFLICT  = -28,
    ERR_INVALID_BRIDGE_MODE       = -29,
    ERR_WIFI_INVALID_CONFIG_MODE  = -30,
    ERR_WIFI_NO_FREE_SSID         = -31,
    ERR_WIFI_CAN_NOT_DELETE       = -32
} utret_t;

typedef enum {
    INTERFACE_LAN,
    INTERFACE_WAN,
} interface_t; 


/*
 * WAN settings
 */
typedef enum {
    DHCP,
    STATIC,
    PPPOE,
    PPTP,
    L2TP,
    TELSTRA
} wanProto_t;

typedef enum {
    WAN_UNKNOWN,
    WAN_DISCONNECTED,
    WAN_CONNECTING,
    WAN_CONNECTED,
    WAN_DISCONNECTING,
} wanConnStatus_t;

typedef enum {
    WAN_PHY_STATUS_UNKNOWN,
    WAN_PHY_STATUS_DISCONNECTED,
    WAN_PHY_STATUS_CONNECTED,
} wanPhyConnStatus_t;


typedef enum {
    KEEP_ALIVE,
    CONNECT_ON_DEMAND,
} wanConnectMethod_t;


typedef struct wan_static {
    char ip_addr[IPADDR_SZ];
    char subnet_mask[IPADDR_SZ];
    char default_gw[IPADDR_SZ];
    char dns_ipaddr1[IPADDR_SZ];
    char dns_ipaddr2[IPADDR_SZ];
    char dns_ipaddr3[IPADDR_SZ];
}__attribute__ ((__packed__)) wan_static_t;


typedef struct wan_ppp {
    char username[USERNAME_SZ];
    char password[PASSWORD_SZ];
    char service_name[WAN_SERVICE_NAME_SZ];   // for pppoe
    char server_ipaddr[IPADDR_SZ];            // for pptp, l2tp
    wanConnectMethod_t conn_method;
    int max_idle_time;
    int redial_period;
    boolean_t ipAddrStatic;   // for pptp/l2tp: true - use wan_static, false - use dhcp
}__attribute__ ((__packed__)) wan_ppp_t;


typedef struct wanInfo {
    wanProto_t    wan_proto;
    wan_static_t  wstatic;
    wan_ppp_t     ppp;
    char          domainname[IPHOSTNAME_SZ];
    boolean_t     auto_mtu;    // true - automatically picked, false - set it to size specified
    int           mtu_size;
}__attribute__ ((__packed__)) wanInfo_t;
   
/*
 * Router/Bridge settings
 */
typedef enum {
    BRIDGE_MODE_OFF    = 0,
    BRIDGE_MODE_DHCP   = 1,
    BRIDGE_MODE_STATIC = 2,
	BRIDGE_MODE_FULL_STATIC = 3
    
} bridgeMode_t;

typedef struct bridge_static {
    char ip_addr[IPADDR_SZ];
    char subnet_mask[IPADDR_SZ];
    char default_gw[IPADDR_SZ];
    char domainname[IPHOSTNAME_SZ];
    char dns_ipaddr1[IPADDR_SZ];
    char dns_ipaddr2[IPADDR_SZ];
    char dns_ipaddr3[IPADDR_SZ];
} bridge_static_t;

typedef struct bridgeInfo {
    bridgeMode_t  mode;
    bridge_static_t  bstatic;
} bridgeInfo_t;

typedef enum{
    NAPT_MODE_DISABLE_DHCP = 0,
    NAPT_MODE_DHCP,
    NAPT_MODE_STATICIP,
    NAPT_MODE_DISABLE_STATIC
}napt_mode_t;

/*
 * DDNS Settings
 */
#if !defined(DDNS_BROADBANDFORUM)
typedef enum ddnsProvider {
    DDNS_EZIP,
    DDNS_PGPOW,
    DDNS_DHS,
    DDNS_DYNDNS,
    DDNS_DYNDNS_STATIC,
    DDNS_DYNDNS_CUSTOM,
    DDNS_ODS,
    DDNS_TZO,
    DDNS_EASYDNS,
    DDNS_EASYDNS_PARTNER,
    DDNS_GNUDIP,     
    DDNS_JUSTLINUX,     
    DDNS_DYNS,
    DDNS_HN,
    DDNS_ZONEEDIT,
    DDNS_HEIPV6TB
} ddnsProvider_t;

typedef struct ddnsService {
    boolean_t      enabled;
    ddnsProvider_t provider;
    char           username[USERNAME_SZ];
    char           password[PASSWORD_SZ];
    char           hostname[IPHOSTNAME_SZ];
    char           mail_exch[IPHOSTNAME_SZ];
    boolean_t      backup_mx;
    boolean_t      wildcard;
} ddnsService_t;

typedef enum {
    DDNS_STATUS_UNKNOWN,
    DDNS_STATUS_FAILED,
    DDNS_STATUS_FAILED_CONNECT,
    DDNS_STATUS_FAILED_AUTH,
    DDNS_STATUS_SUCCESS,
} ddnsStatus_t;
#endif
/*
 * Route Settings
 */

typedef struct routeRIP {
    boolean_t enabled;
    boolean_t no_split_horizon;
    boolean_t lan_interface;
    boolean_t wan_interface;
    char      wan_md5_password[PASSWORD_SZ];
    char      wan_text_password[PASSWORD_SZ];
} routeRIP_t;

typedef struct routeStatic {
    char         name[NAME_SZ];
    char         dest_lan_ip[IPADDR_SZ];
    char         netmask[IPADDR_SZ];
    char         gateway[IPADDR_SZ];
    interface_t  dest_intf; 
} routeStatic_t;

typedef struct routeEntry {
    char destlanip[IPADDR_SZ];
    char netmask[IPADDR_SZ];
    char gateway[IPADDR_SZ];
    int  hopcount;
    char interface[TOKEN_SZ];
} routeEntry_t;

/*
 * Port Forwarding
 */

typedef enum protocol {
    TCP,
    UDP,
    BOTH_TCP_UDP,
} protocol_t;

typedef struct portFwdSingle {
    char       name[NAME_SZ];
    boolean_t  enabled;
    boolean_t  prevRuleEnabledState;	
    int        rule_id;
    protocol_t protocol;
    int        external_port;
    int        internal_port;
    char       dest_ip[IPADDR_SZ];              
    char       dest_ipv6[64];              
} portFwdSingle_t;

typedef struct portMapDyn {
    char       name[NAME_SZ];
    boolean_t  enabled;
    protocol_t protocol;
    char       external_host[IPADDR_SZ];   // empty for all external hosts
    int        external_port;
    char       internal_host[IPADDR_SZ];   // empty for all internal hosts
    int        internal_port;
    int        lease;
    time_t     last_updated;
} portMapDyn_t;

typedef struct portFwdRange {
    char       name[NAME_SZ];
    boolean_t  enabled;
    boolean_t  prevRuleEnabledState;	
    int        rule_id;
    protocol_t protocol;
    int        start_port;
    int        end_port;
    int        internal_port;
    int        internal_port_range_size;
    char       dest_ip[IPADDR_SZ];              
    char       dest_ipv6[64];    
	char       public_ip[IPADDR_SZ];              
} portFwdRange_t;

typedef struct portRangeTrig {
    char       name[TOKEN_SZ];
    boolean_t  enabled;
    boolean_t  prevRuleEnabledState;	
    int        rule_id;
    protocol_t trigger_proto;
    protocol_t forward_proto;
    int        trigger_start;
    int        trigger_end;
    int        fwd_range_start;
    int        fwd_range_end;
} portRangeTrig_t;

/*
 * Internet Access Policy Settings
 */

#define NUM_IAP_POLICY            5
#define NUM_IAP_BLOCKED_URL       4
#define NUM_IAP_BLOCKED_KEYWORD   4
#define NUM_IAP_BLOCKED_APPS     32
#define NUM_IAP_MACHOSTS         10
#define NUM_IAP_IPHOSTS           6
#define NUM_IAP_IPRANGEHOSTS      4

#define DAY_SUN 0x01
#define DAY_MON 0x02
#define DAY_TUE 0x04
#define DAY_WED 0x08
#define DAY_THU 0x10
#define DAY_FRI 0x20
#define DAY_SAT 0x40
#define DAY_ALL (DAY_SUN | DAY_MON | DAY_TUE | DAY_WED | DAY_THU | DAY_FRI | DAY_SAT)

#define HH_MM_SZ 6

typedef struct iprange {
    int start_ip;   // last octet
    int end_ip;     // last octet
}__attribute__ ((__packed__)) iprange_t;

typedef struct portrange {
    int start;
    int end;
}__attribute__ ((__packed__)) portrange_t;

typedef struct lanHosts {
    int        mac_count;
    char      *maclist;          //  MACADDR_SZ * mac_count buffer
    int        ip_count;
    char      *iplist;           //  IPADDR_SZ * ip_count buffer
    int        iprange_count;
    iprange_t *iprangelist;      //  sizeof(iprange_t) * _count buffer
}__attribute__ ((__packed__)) lanHosts_t;

typedef struct {
    unsigned char day;                     // bitmask of DAY_xyz
    boolean_t     all_day;                 // true if the policy is active
                                           // for the full 24 hours
    char          start_time[HH_MM_SZ];    // 24hr format
    char          stop_time[HH_MM_SZ];
}__attribute__ ((__packed__)) timeofday_t;


typedef struct appentry {
    char        name[NAME_SZ];
    boolean_t   wellknown;
    portrange_t port;
    protocol_t  proto;
}__attribute__ ((__packed__)) appentry_t;

typedef struct {
    int              url_count;
    char            *url_list;          // each of URL_SZ
    unsigned int    *url_tr_inst_num;   // size of url_count
    char            *url_tr_alias;      // each of TR_ALIAS_SZ
    int              keyword_count;
    char            *keyword_list;       // each of IAP_KEYWORD_SZ
    unsigned int    *keyword_tr_inst_num;// size of keyword_count
    char            *keyword_tr_alias;   // each of TR_ALIAS_SZ
    int              app_count;
    appentry_t      *app_list;
    unsigned int    *app_tr_inst_num;   // size of app_count
    char            *app_tr_alias;      // each of TR_ALIAS_SZ
}__attribute__ ((__packed__)) blockentry_t;

typedef struct iap_entry {
    char             policyname[NAME_SZ];
    boolean_t        enabled;
    boolean_t        allow_access;      // allow/deny access during TOD
    timeofday_t      tod;
    boolean_t        lanhosts_set;      // indicates if lanhosts is set
    lanHosts_t       lanhosts;
    blockentry_t     block;
    unsigned int     tr_inst_num;
}__attribute__ ((__packed__)) iap_entry_t;


/*
 * QoS Settings
 */
typedef enum {
    QOS_PRIORITY_DEFAULT,
    QOS_PRIORITY_MEDIUM = QOS_PRIORITY_DEFAULT,
    QOS_PRIORITY_NORMAL,
    QOS_PRIORITY_HIGH,
    QOS_PRIORITY_LOW
} priority_t;

typedef enum {
    QOS_APPLICATION,
    QOS_GAME,
    QOS_MACADDR,
    QOS_VOICE_DEVICE,
    QOS_ETHERNET_PORT,
    QOS_CUSTOM
} qostype_t;

typedef enum {
    QOS_CUSTOM_APP,
    QOS_CUSTOM_GAME
} qoscustomtype_t;

#define MAX_CUSTOM_PORT_ENTRIES 3

typedef struct qosPolicy {
    char            name[NAME_SZ];
    qostype_t       type;
    char            mac[MACADDR_SZ];
    int             hwport;         // 1 to max lan ports (eg 4)
    protocol_t      custom_proto[MAX_CUSTOM_PORT_ENTRIES];
    portrange_t     custom_port[MAX_CUSTOM_PORT_ENTRIES];
    qoscustomtype_t custom_type;
    priority_t      priority;
} qosPolicy_t;

typedef struct qosDefinedPolicy {
    char        name[TOKEN_SZ];
    char        friendly_name[TOKEN_SZ];
    qostype_t   type;
    priority_t  default_priority;
} qosDefinedPolicy_t;

typedef struct qosInfo {
    boolean_t   enable;
    int         policy_count;
    qosPolicy_t *policy_list;
    int         download_speed;
    int         upload_speed;
} qosInfo_t;


/*
typedef struct qosPolicy {
    int  app_count;
    qosAppEntry_t *app;
    int  onlinegame_count;
    qosGameEntry_t *game;
    int  macaddr_count;
    qosMacEntry_t *mac;
    int  etherport_count;
    qosEthPortEntry_t *ethport;
} qosPolicy_t;
*/

int Utopia_GetQoSDefinedPolicyList (int *out_count, qosDefinedPolicy_t const **out_qoslist);
int Utopia_SetQoSSettings (UtopiaContext *ctx, qosInfo_t *qos);
int Utopia_GetQoSSettings (UtopiaContext *ctx, qosInfo_t *qos);
int Utopia_get_lan_host_comments(UtopiaContext *ctx, unsigned char *pMac, unsigned char *pComments);
int Utopia_set_lan_host_comments(UtopiaContext *ctx, unsigned char *pMac, unsigned char *pComments);

/*
 * Log Settings
 */
typedef enum logType {
    INCOMING_LOG,
    OUTGOING_LOG,
    SECURITY_LOG,
    DHCP_CLIENT_LOG
} logtype_t;

typedef enum dhcp_msg_type {
    DHCPDISCOVER,
    DHCPOFFER,
    DHCPREQUEST,
    DHCPACK,
    DHCPNAK,
    DHCPDECLINE,
    DHCPRELEASE,
    DHCPINFORM
} dhcp_msg_type_t;

typedef struct logentry {
    char src[URL_SZ];   // ip address or URL
    char dst[URL_SZ];   // ip address or URL
    char service_port[TOKEN_SZ];  // port or service name
} logentry_t;

typedef struct dhcpclientlog {
    char timestamp[TOKEN_SZ];
    dhcp_msg_type_t msg_type;
    char ipaddr[IPADDR_SZ];
    char macaddr[MACADDR_SZ];
} dhcpclientlog_t;


/*
 * Status Settings
 */

#define NUM_DNS_ENTRIES 3

typedef struct wanConnectionInfo {
    char ip_address[IPADDR_SZ];
    char subnet_mask[IPADDR_SZ];
    char default_gw[IPADDR_SZ];
    char dns[NUM_DNS_ENTRIES][IPHOSTNAME_SZ];
    int  dhcp_lease_time;      // in seconds
    char ifname[IFNAME_SZ];
} wanConnectionInfo_t;

typedef struct wanConnectionStatus {
    wanConnStatus_t   status;
    unsigned int      phylink_up;
    long int          uptime;               // current connection's uptime in seconds (NOT system uptime)
    char              ip_address[IPADDR_SZ];
} wanConnectionStatus_t;

typedef struct wanTrafficInfo {
    unsigned long int          pkts_sent;
    unsigned long int          pkts_rcvd;
    unsigned long int          bytes_sent;
    unsigned long int          bytes_rcvd;
} wanTrafficInfo_t;

typedef struct {
    char ipaddr[IPADDR_SZ];
    char netmask[IPADDR_SZ];
    char domain[IPHOSTNAME_SZ];
    char macaddr[MACADDR_SZ];    // ONLY get-able, not-applicable in set operation
    char ifname[IFNAME_SZ];  // ONLY get-able, not-applicable in set operation
} lanSetting_t;


typedef struct bridgeConnectionInfo {
    char ip_address[IPADDR_SZ];
    char subnet_mask[IPADDR_SZ];
    char default_gw[IPADDR_SZ];
    char dns[NUM_DNS_ENTRIES][IPHOSTNAME_SZ];
    int  dhcp_lease_time;      // in seconds
} bridgeConnectionInfo_t;

/*
 * Lang -
 *     ISO Language Code (ISO-639) - ISO Country Code (ISO-3166)
 *     eg: en-us
 */

typedef struct {
    float gmt_offset;          // GMT offset in hours
    boolean_t is_dst_observed;
    char *dst_on;              // TZ string when DST is on
    char *dst_off;             // TZ string when DST is off
                               // Note: dst_on and dst_off are the same
                               //       when DST is not observed
} timezone_info_t;

typedef enum {
    AUTO_DST_OFF = 0,
    AUTO_DST_ON,
    AUTO_DST_NA,     // not-applicable, for countries that don't have DST
} auto_dst_t;

typedef struct {
    char       hostname[IPHOSTNAME_SZ];
    char       lang[LANG_SZ];
    float      tz_gmt_offset;
    auto_dst_t auto_dst;
} deviceSetting_t;

typedef enum {
    LAN_INTERFACE_WIRED,
    LAN_INTERFACE_WIFI,
} lan_interface_t; 

/*
 * LAN setting
 */
typedef struct DHCPMap {
    char client_name[TOKEN_SZ];
    //int host_ip;                  // just the last octet
    char host_ip[IPADDR_SZ];
    char macaddr[MACADDR_SZ];
} DHCPMap_t;

typedef struct arpHost {
    char ipaddr[IPADDR_SZ];
    char macaddr[MACADDR_SZ];
    char interface[IFNAME_SZ];
    boolean_t is_static;
} arpHost_t;

typedef struct dhcpLANHost {
    char hostname[TOKEN_SZ];
    char ipaddr[IPADDR_SZ];
    char macaddr[MACADDR_SZ];
    char client_id[TOKEN_SZ];
    long leasetime;
    lan_interface_t lan_interface;
} dhcpLANHost_t;

typedef struct dhcpServerInfo {
    boolean_t enabled;
    char DHCPIPAddressStart[IPADDR_SZ];
    char DHCPIPAddressEnd[IPADDR_SZ];
    int  DHCPMaxUsers;
    char DHCPClientLeaseTime[TOKEN_SZ];
    boolean_t StaticNameServerEnabled;
    char StaticNameServer1[IPHOSTNAME_SZ];
    char StaticNameServer2[IPHOSTNAME_SZ];
    char StaticNameServer3[IPHOSTNAME_SZ];
    char WINSServer[IPHOSTNAME_SZ];            
} dhcpServerInfo_t;

typedef struct dmz {
    boolean_t enabled;
    char      source_ip_start[IPADDR_SZ]; // empty string means "any ip address"
    char      source_ip_end[IPADDR_SZ];  
    //int       dest_ip;                    // last octet
    char      dest_ip[IPADDR_SZ];           // full ip
    char      dest_mac[MACADDR_SZ]; 
    char      dest_ipv6[64];
} dmz_t;


/*
 * Public APIs
 */

/*
 * Device Settings
 */

int Utopia_GetDeviceSettings (UtopiaContext *ctx, deviceSetting_t *device);
int Utopia_SetDeviceSettings (UtopiaContext *ctx, deviceSetting_t *device);

/*
 * LAN Settings
 */

int Utopia_GetLanSettings (UtopiaContext *ctx, lanSetting_t *lan);
int Utopia_SetLanSettings(UtopiaContext *ctx, lanSetting_t *lan);


int Utopia_SetDHCPServerSettings (UtopiaContext *ctx, dhcpServerInfo_t *dhcps);      
int Utopia_GetDHCPServerSettings (UtopiaContext *ctx, dhcpServerInfo_t *out_dhcps);


int Utopia_SetDHCPServerStaticHosts (UtopiaContext *ctx, int count, DHCPMap_t *dhcpMap);
int Utopia_GetDHCPServerStaticHosts (UtopiaContext *ctx, int *count, DHCPMap_t **dhcpMap);
int Utopia_GetDHCPServerStaticHostsCount (UtopiaContext *ctx, int *count);
int Utopia_UnsetDHCPServerStaticHosts (UtopiaContext *ctx);

int Utopia_GetARPCacheEntries (UtopiaContext *ctx, int *count, arpHost_t **out_hosts);
int Utopia_GetWLANClients (UtopiaContext *ctx, int *count, char **out_maclist);

// Used for status and display current DHCP leases
int Utopia_GetDHCPServerLANHosts (UtopiaContext *ctx, int *count, dhcpLANHost_t **client_info);
int Utopia_DeleteDHCPServerLANHost (char *ipaddr);


/*
 * WAN Settings
 */

int Utopia_SetWANSettings (UtopiaContext *ctx, wanInfo_t *wan_info);
int Utopia_GetWANSettings (UtopiaContext *ctx, wanInfo_t *wan_info);
#if !defined(DDNS_BROADBANDFORUM)
int Utopia_SetDDNSService (UtopiaContext *ctx, ddnsService_t *ddns);
int Utopia_UpdateDDNSService (UtopiaContext *ctx);
int Utopia_GetDDNSService (UtopiaContext *ctx, ddnsService_t *ddns);
int Utopia_GetDDNSServiceStatus (UtopiaContext *ctx, ddnsStatus_t *ddnsStatus);
#endif
int Utopia_SetMACAddressClone (UtopiaContext *ctx, boolean_t enable, char macaddr[MACADDR_SZ]);
int Utopia_GetMACAddressClone (UtopiaContext *ctx, boolean_t *enable, char macaddr[MACADDR_SZ]);

int Utopia_WANDHCPClient_Release (void);
int Utopia_WANDHCPClient_Renew (void);

int Utopia_WANConnectionTerminate (void);
int Utopia_GetWANConnectionInfo (UtopiaContext *ctx, wanConnectionInfo_t *info);
int Utopia_GetWANConnectionStatus (UtopiaContext *ctx, wanConnectionStatus_t *wan);
int Utopia_GetWANTrafficInfo (wanTrafficInfo_t *wan);

/*
 * Router/Bridge settings
 */
int Utopia_SetBridgeSettings (UtopiaContext *ctx, bridgeInfo_t *bridge_info);
int Utopia_GetBridgeSettings (UtopiaContext *ctx, bridgeInfo_t *bridge_info);
int Utopia_GetBridgeConnectionInfo (UtopiaContext *ctx, bridgeConnectionInfo_t *bridge);

/*
 * Route Settings
 */
int Utopia_SetRouteNAT (UtopiaContext *ctx, napt_mode_t enable);
int Utopia_GetRouteNAT (UtopiaContext *ctx, napt_mode_t *enable);

int Utopia_SetRouteRIP (UtopiaContext *ctx, routeRIP_t *rip); //CID 67860: Big parameter passed by value
int Utopia_GetRouteRIP (UtopiaContext *ctx, routeRIP_t *rip);

int Utopia_FindStaticRoute (int count, routeStatic_t *sroutes, const char *route_name);
int Utopia_DeleteStaticRoute (UtopiaContext *ctx, int index);
int Utopia_DeleteStaticRouteName (UtopiaContext *ctx, const char *route_name);
int Utopia_AddStaticRoute (UtopiaContext *ctx, routeStatic_t *sroute);
int Utopia_EditStaticRoute (UtopiaContext *ctx, int index, routeStatic_t *sroute);
int Utopia_GetStaticRouteCount (UtopiaContext *ctx, int *count);
int Utopia_GetStaticRoutes (UtopiaContext *ctx, int *count, routeStatic_t **out_sroute);
int Utopia_GetStaticRouteTable (int *count, routeStatic_t **out_sroute);

/*
 * Firewall Settings
 */

/*
 * Port Mapping
 */

int Utopia_CheckPortTriggerRange(UtopiaContext *ctx, int new_rule_id, int new_start, int new_end, int new_protocol, int is_trigger);
int Utopia_CheckPortRange(UtopiaContext *ctx, int new_rule_id, int new_start, int new_end, int new_protocol, int is_trigger);

int Utopia_SetPortForwarding (UtopiaContext *ctx, int count, portFwdSingle_t *fwdinfo);
int Utopia_GetPortForwarding (UtopiaContext *ctx, int *count, portFwdSingle_t **fwdinfo);
int Utopia_GetPortForwardingCount (UtopiaContext *ctx, int *count);
int Utopia_FindPortForwarding (int count, portFwdSingle_t *portmap, int external_port, protocol_t proto);
int Utopia_AddPortForwarding (UtopiaContext *ctx, portFwdSingle_t *portmap);
int Utopia_GetPortForwardingByIndex (UtopiaContext *ctx, int index, portFwdSingle_t *fwdinfo);
int Utopia_SetPortForwardingByIndex (UtopiaContext *ctx, int index, portFwdSingle_t *fwdinfo);
int Utopia_DelPortForwardingByIndex (UtopiaContext *ctx, int index);
int Utopia_GetPortForwardingByRuleId (UtopiaContext *ctx, portFwdSingle_t *fwdinfo);
int Utopia_SetPortForwardingByRuleId (UtopiaContext *ctx, portFwdSingle_t *fwdinfo);
int Utopia_DelPortForwardingByRuleId (UtopiaContext *ctx, int rule_id);

int Utopia_AddDynPortMapping (portMapDyn_t *portmap);
int Utopia_UpdateDynPortMapping (int index, portMapDyn_t *pmap);
int Utopia_DeleteDynPortMapping (portMapDyn_t *portmap);
int Utopia_DeleteDynPortMappingIndex (int index);
int Utopia_InvalidateDynPortMappings (void);
int Utopia_ValidateDynPortMapping (int index);
int Utopia_GetDynPortMappingCount (int *count);
int Utopia_GetDynPortMapping (int index, portMapDyn_t *portmap);
int Utopia_FindDynPortMapping(const char *external_host, int external_port, protocol_t proto, portMapDyn_t *pmap, int *index);
int Utopia_IGDConfigAllowed (UtopiaContext *ctx);
int Utopia_IGDInternetDisbleAllowed (UtopiaContext *ctx);

int Utopia_SetPortForwardingRange (UtopiaContext *ctx, int count, portFwdRange_t *fwdinfo);
int Utopia_GetPortForwardingRange (UtopiaContext *ctx, int *count, portFwdRange_t **fwdinfo);
int Utopia_GetPortForwardingRangeCount (UtopiaContext *ctx, int *count);
int Utopia_AddPortForwardingRange (UtopiaContext *ctx, portFwdRange_t *portmap);
int Utopia_GetPortForwardingRangeByIndex (UtopiaContext *ctx, int index, portFwdRange_t *fwdinfo);
int Utopia_SetPortForwardingRangeByIndex (UtopiaContext *ctx, int index, portFwdRange_t *fwdinfo);
int Utopia_DelPortForwardingRangeByIndex (UtopiaContext *ctx, int index);
int Utopia_GetPortForwardingRangeByRuleId (UtopiaContext *ctx, portFwdRange_t *fwdinfo);
int Utopia_SetPortForwardingRangeByRuleId (UtopiaContext *ctx, portFwdRange_t *fwdinfo);
int Utopia_DelPortForwardingRangeByRuleId (UtopiaContext *ctx, int rule_id);

int Utopia_SetPortTrigger (UtopiaContext *ctx, int count, portRangeTrig_t *portinfo);
int Utopia_GetPortTrigger (UtopiaContext *ctx, int *count, portRangeTrig_t **portinfo);
int Utopia_GetPortTriggerCount (UtopiaContext *ctx, int *count);
int Utopia_AddPortTrigger (UtopiaContext *ctx, portRangeTrig_t *portmap);
int Utopia_GetPortTriggerByIndex (UtopiaContext *ctx, int index, portRangeTrig_t *fwdinfo);
int Utopia_SetPortTriggerByIndex (UtopiaContext *ctx, int index, portRangeTrig_t *fwdinfo);
int Utopia_DelPortTriggerByIndex (UtopiaContext *ctx, int index);
int Utopia_GetPortTriggerByRuleId (UtopiaContext *ctx, portRangeTrig_t *portinfo);
int Utopia_SetPortTriggerByRuleId (UtopiaContext *ctx, portRangeTrig_t *portinfo);
int Utopia_DelPortTriggerByRuleId (UtopiaContext *ctx, int trigger_id);

int Utopia_SetDMZSettings (UtopiaContext *ctx, dmz_t *dmz);
int Utopia_GetDMZSettings (UtopiaContext *ctx, dmz_t *out_dmz);

int Utopia_AddInternetAccessPolicy (UtopiaContext *ctx, iap_entry_t *iap);
int Utopia_EditInternetAccessPolicy (UtopiaContext *ctx, int index, iap_entry_t *iap);
int Utopia_AddInternetAccessPolicyLanHosts (UtopiaContext *ctx, const char *policyname, lanHosts_t *lanhosts);
int Utopia_DeleteInternetAccessPolicy (UtopiaContext *ctx, const char *policyname);
int Utopia_GetInternetAccessPolicy (UtopiaContext *ctx, int *out_count, iap_entry_t **out_iap);
int Utopia_FindInternetAccessPolicy (UtopiaContext *ctx, const char *policyname, iap_entry_t *out_iap, int *out_index);
void Utopia_FreeInternetAccessPolicy (iap_entry_t *iap);
/*
 * Returns a null terminated array of strings with network
 * service names like ftp, telnet, dns, etc
 */
int Utopia_GetNetworkServicesList (const char **out_list);

typedef struct firewall {
    boolean_t spi_protection;
    boolean_t filter_anon_req;
    boolean_t filter_anon_req_v6;
    boolean_t filter_multicast;
    boolean_t filter_multicast_v6;
    boolean_t filter_nat_redirect;
    boolean_t filter_ident;
    boolean_t filter_ident_v6;
    boolean_t filter_web_proxy;
    boolean_t filter_web_java;
    boolean_t filter_web_activex;
    boolean_t filter_web_cookies;
    boolean_t allow_ipsec_passthru;
    boolean_t allow_pptp_passthru;
    boolean_t allow_l2tp_passthru;
    boolean_t allow_ssl_passthru;
    boolean_t filter_http_from_wan;
    boolean_t filter_http_from_wan_v6;
    boolean_t filter_p2p_from_wan;
    boolean_t filter_p2p_from_wan_v6;
    boolean_t true_static_ip_enable;
    boolean_t true_static_ip_enable_v6;
    boolean_t smart_pkt_dection_enable;
    boolean_t smart_pkt_dection_enable_v6;
    boolean_t wan_ping_enable;
    boolean_t wan_ping_enable_v6;
} firewall_t;

int Utopia_SetFirewallSettings (UtopiaContext *ctx, firewall_t fw);
int Utopia_GetFirewallSettings (UtopiaContext *ctx, firewall_t *fw);

typedef struct ipv6Prefix {
	char prefix[IPADDR_SZ];
	int size;
}ipv6Prefix_t;

/*
 * IPv6 Settings and Status
 */

typedef struct ipv6Info {
    /* Dynamic information from sysevent mainly */
    char ipv6_connection_state[NAME_SZ];
    char current_lan_ipv6address[IPADDR_SZ];
    char current_lan_ipv6address_ll[IPADDR_SZ];
    char current_wan_ipv6_interface[IFNAME_SZ];
    char current_wan_ipv6address[IPADDR_SZ];
    char current_wan_ipv6address_ll[IPADDR_SZ];
    char ipv6_domain[TOKEN_SZ] ;
    char ipv6_nameserver[TOKEN_SZ] ;
    char ipv6_ntp_server[TOKEN_SZ] ;
    char ipv6_prefix[IPADDR_SZ] ;
    /* Configuration from syscfg mainly */
    int sixrd_enable;
            int sixrd_common_prefix4;
            char sixrd_relay[IPADDR_SZ];
            char sixrd_zone[IPADDR_SZ];
            int sixrd_zone_length;
    int sixtofour_enable;
    int aiccu_enable;
            char aiccu_password[PASSWORD_SZ];
            char aiccu_prefix[IPADDR_SZ];
            char aiccu_tunnel[TOKEN_SZ];
            char aiccu_user[USERNAME_SZ];
    int he_enable;
            char he_client_ipv6[IPADDR_SZ];
            char he_password[PASSWORD_SZ];
            char he_prefix[IPADDR_SZ];
            char he_server_ipv4[IPADDR_SZ];
            char he_tunnel[TOKEN_SZ];
            char he_user[USERNAME_SZ];
    int ipv6_bridging_enable;
    int ipv6_ndp_proxy_enable;
    int dhcpv6c_enable ;
            char dhcpv6c_duid[TOKEN_SZ] ;
    int dhcpv6s_enable ;
            char dhcpv6s_duid[TOKEN_SZ] ;
    int ipv6_static_enable ;
            char ipv6_default_gateway[IPADDR_SZ] ;
            char ipv6_lan_address[IPADDR_SZ] ;
            char ipv6_wan_address[IPADDR_SZ] ;
    int ra_enable ; /* Whether to start Zebra to transmit RA on the LAN side */
    int ra_provisioning_enable ; /* Whether to listen to RA on the WAN side */
} ipv6Info_t ;

int Utopia_SetIPv6Settings (UtopiaContext *ctx, ipv6Info_t *ipv6_info);
int Utopia_GetIPv6Settings (UtopiaContext *ctx, ipv6Info_t *ipv6_info);

/*
 * Administration
 */

typedef struct {
    char      admin_user[NAME_SZ];
    boolean_t http_access;
    boolean_t https_access;
    boolean_t wifi_mgmt_access;
    boolean_t wan_mgmt_access;
    boolean_t wan_http_access;
    boolean_t wan_https_access;
    int       wan_http_port;        // default is 8080
    boolean_t wan_firmware_upgrade;
    boolean_t wan_src_anyip;
    char      wan_src_startip[IPADDR_SZ];
    char      wan_src_endip[IPADDR_SZ];
} webui_t;

typedef struct {
    boolean_t enable;
    boolean_t allow_userconfig;
    boolean_t allow_wandisable;
} igdconf_t;
 
int Utopia_RestoreFactoryDefaults (void);
int Utopia_RestoreConfiguration (char *config_fname);
int Utopia_BackupConfiguration (char *out_config_fname);
int Utopia_FirmwareUpgrade (UtopiaContext *ctx, char *firmware_file);
int Utopia_IsFirmwareUpgradeAllowed (UtopiaContext *ctx, int http_port);
int Utopia_AcquireFirmwareUpgradeLock (int *lock_fd);
int Utopia_ReleaseFirmwareUpgradeLock (int lock_fd);
int Utopia_SystemChangesAllowed (void);
int Utopia_Reboot (void);

int Utopia_SetWebUIAdminPasswd (UtopiaContext *ctx, char *username, char *cleartext_password);
int Utopia_IsAdminDefault (UtopiaContext *ctx, boolean_t *is_admin_default);
int Utopia_SetWebUISettings (UtopiaContext *ctx, webui_t *ui);
int Utopia_GetWebUISettings (UtopiaContext *ctx, webui_t *ui);

int Utopia_SetIGDSettings (UtopiaContext *ctx, igdconf_t *igd);
int Utopia_GetIGDSettings (UtopiaContext *ctx, igdconf_t *igd);

/*
 * Logging and Diagnostics
 */
/*
 * Notes: caller need to free log array
 */
int Utopia_GetIncomingTrafficLog (UtopiaContext *ctx, int *count, logentry_t **ilog);

/*
 * Notes: caller need to free log array
 */
int Utopia_GetOutgoingTrafficLog (UtopiaContext *ctx, int *count, logentry_t **olog);

/*
 * Notes: caller need to free log array
 */
int Utopia_GetSecurityLog (UtopiaContext *ctx, int *count, logentry_t **slog);

int Utopia_GetDHCPClientLog (UtopiaContext *ctx);

int Utopia_SetLogSettings (UtopiaContext *ctx, boolean_t log_enabled, char *log_viewer);
int Utopia_GetLogSettings (UtopiaContext *ctx, boolean_t *log_enabled, char *log_viewer, int sz);

int Utopia_DiagPingTestStart (char *dest, int packet_size, int num_ping);
int Utopia_DiagPingTestStop (void);
int Utopia_DiagPingTestIsRunning (void);
int Utopia_DiagTracerouteTestStart (char *dest);
int Utopia_DiagTracerouteTestStop (void);
int Utopia_DiagTracerouteTestIsRunning (void);

int diagPingTest (UtopiaContext *ctx, String dest, int packet_size, int num_ping);
int diagTraceroute (UtopiaContext *ctx, String dest, char **results_buffer); 

int Utopia_PPPConnect (void);
int Utopia_PPPDisconnect (void);


/* BYOI */
#if 0
typedef struct byoi_wan_ppp {
    char username[USERNAME_SZ];
    char password[PASSWORD_SZ];
    char service_name[WAN_SERVICE_NAME_SZ];   // for pppoe
    wanConnectMethod_t conn_method;
    int max_idle_time;
    int redial_period;
} byoi_wan_ppp_t;

typedef enum {
    BYOI_DHCP,
    BYOI_PPPOE
} byoi_wanProto_t;



typedef struct byoi {
    boolean_t          primary_hsd_allowed;
    byoi_wanProto_t    wan_proto;
    byoi_wan_ppp_t     ppp;
    hsd_type_t         byoi_mode;
    boolean_t          byoi_bridge_mode; 
}byoi_t;
int Utopia_Get_BYOI(UtopiaContext *ctx, byoi_t *byoi);
int Utopia_Set_BYOI(UtopiaContext *ctx, byoi_t *byoi);
*/
#endif

typedef enum hsd_type {
    PRIMARY_PROVIDER_HSD,
    PRIMARY_PROVIDER_RESTRICTED,
    USER_SELECTABLE
} hsd_type_t; 


typedef enum {
    CABLE_PROVIDER_HSD,
    BYOI_PROVIDER_HSD,
    NONE
} hsdStatus_t;

int Utopia_Get_BYOI_allowed(UtopiaContext *ctx,  int *value);
int Utopia_Get_BYOI_Current_Provider(UtopiaContext *ctx,  hsdStatus_t *hsdStatus); 
int Utopia_Set_BYOI_Desired_Provider(UtopiaContext *ctx,  hsdStatus_t hsdStatus);

/* Web Timeout Config value in minutes*/
int Utopia_GetWebTimeout(UtopiaContext *ctx, int *count);
int Utopia_SetWebTimeout(UtopiaContext *ctx, int count);

/*typedef enum {
    ADMIN,
    HOME_USER,
} userType_t;
*/
typedef struct http_user {
    char username[USERNAME_SZ];
    char password[PASSWORD_SZ];
    //userType_t usertype;
} http_user_t;

int Utopia_Get_Http_User(UtopiaContext *ctx,  http_user_t *httpuser);
int Utopia_Set_Http_User(UtopiaContext *ctx, http_user_t *httpuser);
int Utopia_Get_Http_Admin(UtopiaContext *ctx, http_user_t *httpuser);
int Utopia_Set_Http_Admin(UtopiaContext *ctx, http_user_t *httpuser);
int Utopia_Set_Prov_Code(UtopiaContext *ctx, char *val) ;
int Utopia_Get_Prov_Code(UtopiaContext *ctx, char *val) ;
int Utopia_Get_First_Use_Date(UtopiaContext *ctx,  char *val);

/* NTP Functions */
int Utopia_Set_DeviceTime_NTPServer(UtopiaContext *ctx, char *server, int index);
int Utopia_Get_DeviceTime_NTPServer(UtopiaContext *ctx, char *server,int index);
int Utopia_Set_DeviceTime_LocalTZ(UtopiaContext *ctx, char *tz);
int Utopia_Get_DeviceTime_LocalTZ(UtopiaContext *ctx, char *tz);
int Utopia_Set_DeviceTime_Enable(UtopiaContext *ctx, unsigned char enable);
unsigned char Utopia_Get_DeviceTime_Enable(UtopiaContext *ctx);
int Utopia_Get_DeviceTime_Status(UtopiaContext *ctx);
int Utopia_Set_DeviceTime_DaylightEnable(UtopiaContext *ctx, unsigned char enable);
unsigned char Utopia_Get_DeviceTime_DaylightEnable(UtopiaContext *ctx);
int Utopia_Get_DeviceTime_DaylightOffset(UtopiaContext *ctx, int *count);
int Utopia_Set_DeviceTime_DaylightOffset(UtopiaContext *ctx, int count);

int Utopia_Get_Mac_MgWan(UtopiaContext *ctx,  char *val);

int Utopia_GetEthAssocDevices(int unitId, int portId, unsigned char *macAddrList,int *numMacAddr);

int Utopia_GetLanMngmCount(UtopiaContext *ctx, int *val);
int Utopia_SetLanMngmInsNum(UtopiaContext *ctx, unsigned long int val);
int Utopia_GetLanMngmInsNum(UtopiaContext *ctx, unsigned long int *val);
int Utopia_GetLanMngmAlias(UtopiaContext *ctx, char *buf, size_t b_len );
int Utopia_SetLanMngmAlias(UtopiaContext *ctx, const char *val);
//int Utopia_GetLanMngmLanMode(UtopiaContext *ctx, lanMngm_LanMode_t *LanMode);
//int Utopia_SetLanMngmLanMode(UtopiaContext *ctx, lanMngm_LanMode_t LanMode);
int Utopia_GetLanMngmLanNetworksAllow(UtopiaContext *ctx, int* allow);
int Utopia_SetLanMngmLanNetworksAllow(UtopiaContext *ctx, int allow);
int Utopia_GetLanMngmLanNapt(UtopiaContext *ctx, napt_mode_t *enable);
int Utopia_SetLanMngmLanNapt(UtopiaContext *ctx, napt_mode_t enable);

#define DNS_CLIENT_NAMESERVER_CNT 10

typedef struct dns_client{
    char dns_server[DNS_CLIENT_NAMESERVER_CNT][IPADDR_SZ];
}DNS_Client_t;

int Utopia_SetDNSEnable(UtopiaContext *ctx, boolean_t enable);
int Utopia_GetDNSEnable(UtopiaContext *ctx, boolean_t* enable);
int Utopia_GetDNSServer(UtopiaContext *ctx, DNS_Client_t * dns);

int Utopia_IPRule_ephemeral_port_forwarding( portMapDyn_t *pmap, boolean_t isCallForAdd );
int Utopia_privateIpCheck(char *ip_to_check);

#if defined(DDNS_BROADBANDFORUM)
typedef struct DynamicDnsClient
{
   unsigned long  InstanceNumber;
   char           Alias[64];
   int            Status;
   int            LastError;
   char           Server[256];
   char           Interface[256];
   char           Username[256];
   char           Password[256];
   boolean_t      Enable;
}DynamicDnsClient_t;

int Utopia_GetDynamicDnsClientInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins);
int Utopia_GetNumberOfDynamicDnsClient(UtopiaContext *ctx, int *num);
int Utopia_GetDynamicDnsClientByIndex(UtopiaContext *ctx, unsigned long ulIndex, DynamicDnsClient_t *DynamicDnsClient);
int Utopia_SetDynamicDnsClientByIndex(UtopiaContext *ctx, unsigned long ulIndex, const DynamicDnsClient_t *DynamicDnsClient);
int Utopia_SetDynamicDnsClientInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ins, const char *alias);
int Utopia_AddDynamicDnsClient(UtopiaContext *ctx, const DynamicDnsClient_t *DynamicDnsClient);
int Utopia_DelDynamicDnsClient(UtopiaContext *ctx, unsigned long ins);
#endif

#endif // _UTAPI_H_
