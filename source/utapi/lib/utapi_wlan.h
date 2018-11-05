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

#ifndef __UTAPI_WLAN_H__
#define __UTAPI_WLAN_H__

#define VERSION 1
#define PASSPHRASE_SZ 64
#define WEP_KEY_SZ 32 
#define WEP_KEY_NUM 4
#define SSID_SIZE (4*32+1)
#define STD_CHAN_NUM 20 
#define MAC_ADDR_SIZE 18 
#define SHAREDKEY_SZ 64

#define WIFI_INTERFACE_COUNT 2

typedef enum wifiInterface {
    FREQ_2_4_GHZ, 
    FREQ_5_GHZ, 
} wifiInterface_t;

typedef struct wifiPlatformSetup {
    wifiInterface_t interface;
    char *syscfg_namespace_prefix;
    char *ifconfig_interface;
} wifiPlatformSetup_t;


#define WIFI_80211_MODE_NUM 7

typedef enum wifiMode {
    MODE_INVALID = -1,
    B_ONLY = 1, 
    G_ONLY, 
    A_ONLY, 
    N_ONLY, 
    B_G_MIXED, 
    B_G_N_MIXED, 
    A_N_MIXED,
} wifiMode_t;

typedef struct wifi80211Mode {
    wifiMode_t val;
    char str[16];
} wifi80211Mode_t;

typedef enum {
    BAND_INVALID = -1, 
    BAND_AUTO = 0, 
    STD_20MHZ, 
    WIDE_40MHZ
} wifiBand_t;

typedef enum {
    SIDEBAND_LOWER, 
    SIDEBAND_UPPER, 
} wifiSideBand_t;


typedef enum wifiWideChannel {
    WC_AUTO, 
    WC_1,  // for 2.4GHz 
    WC_2, 
    WC_3, 
    WC_4, 
    WC_5, 
    WC_6, 
    WC_7, 
    WC_36, // for 5GHz 
    WC_44, 
    WC_52, 
    WC_60, 
    WC_149, 
    WC_157,
} wifiWideChannel_t;

typedef struct wifiWideChannelSetup {
    wifiWideChannel_t val;
    char str[16];
} wifiWideChannelSetup_t;

#define WIDE_CHAN_NUM 25

typedef enum wifiStdChannel {
    SC_AUTO, 
    SC_1, // for 2.4GHz 
    SC_2, 
    SC_3, 
    SC_4, 
    SC_5, 
    SC_6, 
    SC_7, 
    SC_8, 
    SC_9, 
    SC_10, 
    SC_11, 
    SC_36, // for 5GHz 
    SC_40,
    SC_44, 
    SC_48, 
    SC_52, 
    SC_56, 
    SC_60, 
    SC_64, 
    SC_149, 
    SC_153, 
    SC_157, 
    SC_161,
    SC_165,
} wifiStdChannel_t;

typedef struct wifiStdChannelSetup {
    wifiStdChannel_t val;
    char str[16];
} wifiStdChannelSetup_t;


typedef struct wifiRadioInfo {
    wifiInterface_t interface; 
    boolean_t enabled;
    boolean_t ssid_broadcast;
    char ssid[SSID_SIZE];    
    char mac_address[MAC_ADDR_SIZE+1];   // Only get-able, NA in set operation
    wifiMode_t mode;
    wifiBand_t band;
    // wifiWideChannel_t wide_channel;
    // wifiStdChannel_t std_channel;
    wifiSideBand_t sideband;  // applicable only for N-mode(s);
    int channel;
} wifiRadioInfo_t;

/*
 * TEMPORARY: for wlancfg transition to use utapi
 */
#ifndef UTAPI_NO_SEC_MACFILTER

/*
 * Security Settings
 */
#define SECURITY_MODE_NUM 7

typedef enum wifiSecMode {
    WIFI_SECURITY_INVALID = -1,
    WIFI_SECURITY_DISABLED = 0,
    WIFI_SECURITY_WEP, 
    WIFI_SECURITY_WPA_PERSONAL, 
    WIFI_SECURITY_WPA_ENTERPRISE, 
    WIFI_SECURITY_WPA2_PERSONAL, 
    WIFI_SECURITY_WPA2_ENTERPRISE, 
    WIFI_SECURITY_RADIUS,
} wifiSecMode_t;

typedef struct wifiSecModeSetup {
    wifiSecMode_t val;
    char str[64];
} wifiSecModeSetup_t;

typedef enum WPAEncrypt {
    WPA_ENCRYPT_INVALID = -1,
    WPA_ENCRYPT_AES = 0,
    WPA_ENCRYPT_TKIP,
    WPA_ENCRYPT_TKIP_AES,   // a.k.a WPA2-AES
} WPAEncrypt_t;

typedef struct wifiSecurityInfo {
    wifiInterface_t  interface; 
    wifiSecMode_t   mode;
    WPAEncrypt_t    encrypt;    
    char            passphrase[PASSPHRASE_SZ];    
    int             key_renewal_interval;  // default 3600secs
    char            wep_key[WEP_KEY_NUM][WEP_KEY_SZ];
    int             wep_txkey;    // valid 1 - 4
    char            radius_server[IPADDR_SZ];
    int             radius_port;
    char            shared_key[SHAREDKEY_SZ];
} wifiSecurityInfo_t;

/*
 * Mac Filters
 */
typedef enum wifiMacFilterMode {
   MAC_FILTER_DISABLED,
   MAC_FILTER_DENY,
   MAC_FILTER_ALLOW,
} wifiMacFilterMode_t;

#endif // UTAPI_NO_SEC_MACFILTER

typedef enum wifiAuthType {
   WIFI_AUTH_AUTO,
   WIFI_AUTH_OPENSYSTEM,
   WIFI_AUTH_SHAREDKEY,
} wifiAuthType_t;

typedef enum wifiBasicRate {
   WIFI_BASICRATE_DEFAULT, 
   WIFI_BASICRATE_1_2MBPS, 
   WIFI_BASICRATE_ALL,
} wifiBasicRate_t;

#define TX_RATE_NUM 9

typedef enum wifiTXRate {
   WIFI_TX_RATE_AUTO = 0,
   WIFI_TX_RATE_6, 
   WIFI_TX_RATE_9, 
   WIFI_TX_RATE_12,
   WIFI_TX_RATE_18,
   WIFI_TX_RATE_24, 
   WIFI_TX_RATE_36, 
   WIFI_TX_RATE_48, 
   WIFI_TX_RATE_54,
} wifiTXRate_t;

typedef struct wifiTXRateMapping_t {
    wifiTXRate_t val;
    char str[64];
} wifiTXRateMapping_t;

typedef enum wifiNTXRate {
   WIFI_N_TX_RATE_AUTO = -1,
   WIFI_N_TX_RATE_MSC0_6dot5 = 0, 
   WIFI_N_TX_RATE_MSC1_13, 
   WIFI_N_TX_RATE_MSC2_19dot5, 
   WIFI_N_TX_RATE_MSC3_26, 
   WIFI_N_TX_RATE_MSC4_39, 
   WIFI_N_TX_RATE_MSC5_52, 
   WIFI_N_TX_RATE_MSC6_58dot5, 
   WIFI_N_TX_RATE_MSC7_65, 
   WIFI_N_TX_RATE_MSC8_13, 
   WIFI_N_TX_RATE_MSC9_26, 
   WIFI_N_TX_RATE_MSC10_39, 
   WIFI_N_TX_RATE_MSC11_52, 
   WIFI_N_TX_RATE_MSC12_78, 
   WIFI_N_TX_RATE_MSC13_104, 
   WIFI_N_TX_RATE_MSC14_117, 
   WIFI_N_TX_RATE_MSC15_130, 
} wifiNTXRate_t;

typedef enum wifiTXPower {
   TX_POWER_HIGH, 
   TX_POWER_MEDIUM, 
   TX_POWER_LOW,
} wifiTXPower_t;

#define MAX_MACFILTERS 32

#ifndef UTAPI_NO_SEC_MACFILTER

typedef struct wifiMacFiltersInfo {
   int   macFilterMode; 
   int   client_count;
   char  client_mac_list[MAX_MACFILTERS][MACADDR_SZ];
} wifiMacFilterInfo_t;

#endif // UTAPI_NO_SEC_MACFILTER


typedef struct wifiAdvancedInfo {
   wifiInterface_t interface; 
   boolean_t ap_isolation; 
   boolean_t frame_burst; 
   wifiAuthType_t auth_type; 
   wifiBasicRate_t basic_rate; 
   wifiTXRate_t tx_rate; 
   int  n_tx_rate;       // MCS index; range 0 to 15  and -1 being auto 
   wifiTXPower_t tx_power;
   boolean_t auto_cts_protect_mode; 
   int  beacon_interval; // default 100ms; range 1 - 65535 
   int  dtim_interval;   // default 1,     range 1 - 255 
   int  frag_threshold;  // default 2346,  range 256 - 2346 
   int  rts_threshold;   // default 2347,  range 0 - 2347
} wifiAdvancedInfo_t;

typedef struct wifiCounters {
   long tx_frames; 
   long tx_bytes; 
   long tx_retrans; 
   long tx_errors;
   long rx_frames; 
   long rx_bytes;
   long rx_errors;
} wifiCounters_t;

typedef struct wifiStatusInfo {
   char mac_addr[MAC_ADDR_SIZE+1]; 
   char radio_mode[16]; // wifiMode 
   char ssid[SSID_SIZE]; 
   int channel; 
   char security[32];  
   int ssid_broadcast; // 1=enabled, 0=disabled	
   wifiCounters_t counters;
} wifiStatusInfo_t;

typedef struct wifiQoS { 
   boolean_t wmm_support; 
   boolean_t no_acknowledgement;
} wifiQoS_t;		

typedef enum wifiConfigMode {
   WIFI_CONFIG_MANUAL,
   WIFI_CONFIG_WPS
} wifiConfigMode_t;

typedef enum wpsStatus {
   WPS_STOPPED,
   WPS_IN_PROGRESS,
   WPS_SUCCESS,
   WPS_FAILED
} wpsStatus_t;

/*
 * Router/Bridge settings
 */
typedef enum {
    WIFI_BRIDGE_DISABLED    = 0,
    WIFI_BRIDGE_WDS_ENABLED = 1,
    WIFI_BRIDGE_STA_ENABLED = 2
} wifiBridgeMode_t;

typedef struct wifiBridgeInfo {
    wifiBridgeMode_t mode;
    char             bssid[MACADDR_SZ];
} wifiBridgeInfo_t;

/*
 * APIs
 */ 
int Utopia_SetWifiRadioState (UtopiaContext *ctx, wifiInterface_t intf, boolean_t enable);
int Utopia_GetWifiRadioState (UtopiaContext *ctx, wifiInterface_t intf, boolean_t *enable);

int Utopia_GetWifiRadioSettings (UtopiaContext *ctx, wifiInterface_t intf, wifiRadioInfo_t *info);
int Utopia_SetWifiRadioSettings (UtopiaContext *ctx, wifiRadioInfo_t *info);
#ifndef UTAPI_NO_SEC_MACFILTER
int Utopia_GetWifiSecuritySettings (UtopiaContext *ctx, wifiInterface_t intf, wifiSecurityInfo_t *info); 
int Utopia_SetWifiSecuritySettings (UtopiaContext *ctx, wifiSecurityInfo_t *info);
int Utopia_GetWifiMacFilters (UtopiaContext *ctx, wifiMacFilterInfo_t *info);
int Utopia_SetWifiMacFilters (UtopiaContext *ctx, wifiMacFilterInfo_t *info);
#endif
int Utopia_GetWifiAdvancedSettings (UtopiaContext *ctx, wifiInterface_t intf, wifiAdvancedInfo_t *info);
int Utopia_SetWifiAdvancedSettings (UtopiaContext *ctx, wifiAdvancedInfo_t *info);

int Utopia_SetWifiQoSSettings (UtopiaContext *ctx, wifiQoS_t *wifiqos);
int Utopia_GetWifiQoSSettings (UtopiaContext *ctx, wifiQoS_t *wifiqos);

int Utopia_GetWifiConfigMode (UtopiaContext *ctx, wifiConfigMode_t *config_mode);
int Utopia_SetWifiConfigMode (UtopiaContext *ctx, wifiConfigMode_t config_mode);

int Utopia_WPSPushButtonStart (void);
int Utopia_WPSPinStart (int pin);
int Utopia_WPSStop (void);
int Utopia_GetWPSStatus (wpsStatus_t *wps_status);

int Utopia_GetWifiBridgeSettings (UtopiaContext *ctx, wifiBridgeInfo_t *info);
int Utopia_SetWifiBridgeSettings (UtopiaContext *ctx, wifiBridgeInfo_t *info);

#endif // __UTAPI_WLAN_H__
