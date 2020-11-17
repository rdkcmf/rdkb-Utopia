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

#ifndef __UTAPI_TR_WLAN_H__
#define __UTAPI_TR_WLAN_H__

#define WLANCFG_RADIO_FULL_FILE "/tmp/wifi.txt"
#define WLANCFG_RADIO_EXTN_FILE "/tmp/wifi_extn.txt"
#define WLANCFG_RADIO_STATS_FILE "/tmp/wifi_stat.txt"
#define WLANCFG_RADIO_FILE "/tmp/wifi_radio.txt"
#define WLANCFG_SSID_FILE "/tmp/wifi_ssid.txt"
#define WLANCFG_SSID_STATS_FILE "/tmp/wifi_ssid_stats.txt"
#define WLANCFG_AP_FILE "/tmp/wifi_ap.txt"
#define WLANCFG_AP_SEC_FILE "/tmp/wifi_ap_sec.txt"
#define WLANCFG_AP_WPS_FILE "/tmp/wifi_ap_wps.txt"
#define WLANCFG_AP_ASSOC_DEV_FILE "/tmp/wifi_ap_assocDev.txt"
#define WIFI_MACFILTER_FILE "/tmp/wifi_mac_filter.txt"

#define WIFI_RADIO_NUM_INSTANCES 2 /* Change this if num of Radios changes */
#define WIFI_SSID_NUM_INSTANCES 8 /* Change this once we implement multi-SSID */
#define WIFI_AP_NUM_INSTANCES 8 /* Change this once we implement multi-SSID */

#define START_SECONDARY_SSID 1 /* Starting of Secondary SSIDs */
#define MAX_SSID_PER_RADIO 8 /* For the time being we allow only 4 per radio */
#define PRIMARY_SSID_COUNT 2 /* Number of non-deletable primary SSID */

#define START_DYNAMIC_SSID 4 /* We configure 3 secondary SSIDs per radio statically */
#define STATIC_SSID_COUNT  8 /* Number of statically configured SSIDs */

#define STR_SZ 32

#define ERR_SSID_NOT_FOUND -100

#define MAX_NUM_INSTANCES 255

#define  IPV4_ADDRESS                                                        \
         union                                                               \
         {                                                                   \
            unsigned char           Dot[4];                                  \
            unsigned long           Value;                                   \
         }

typedef enum wifiInterface {
    FREQ_UNKNOWN = -1,
    FREQ_2_4_GHZ = 0,
    FREQ_5_GHZ
} wifiInterface_t;

typedef struct wifiTRPlatformSetup {
    wifiInterface_t interface;
    char *syscfg_namespace_prefix;
    char *ifconfig_interface;
    char *ssid_name;
    char *ap_name;
} wifiTRPlatformSetup_t;

typedef  enum wifiStandards {
    WIFI_STD_a             = 1,
    WIFI_STD_b             = 2,
    WIFI_STD_g             = 4,
    WIFI_STD_n             = 8
}
wifiStandards_t;

typedef enum wifiTXPower {
   TX_POWER_HIGH = 100,
   TX_POWER_MEDIUM = 50,
   TX_POWER_LOW = 25
} wifiTXPower_t;

typedef enum wifiBasicRate {
   WIFI_BASICRATE_DEFAULT,
   WIFI_BASICRATE_1_2MBPS,
   WIFI_BASICRATE_ALL,
} wifiBasicRate_t;

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
} wifiTxRate_t;

typedef enum wifiSideband {
    SIDEBAND_LOWER = 1,
    SIDEBAND_UPPER
} wifiSideband_t;

typedef enum wifiGuardInterval {
   GI_LONG,
   GI_SHORT,
   GI_AUTO
} wifiGuardInterval_t;

typedef enum wifiBand {
    BAND_AUTO,
    STD_20MHZ,
    WIDE_40MHZ
} wifiBand_t;

typedef enum wifiSecurity {
    WIFI_SECURITY_None                 = 0x00000001,
    WIFI_SECURITY_WEP_64               = 0x00000002,
    WIFI_SECURITY_WEP_128              = 0x00000004,
    WIFI_SECURITY_WPA_Personal         = 0x00000008,
    WIFI_SECURITY_WPA2_Personal        = 0x00000010,
    WIFI_SECURITY_WPA_WPA2_Personal    = 0x00000020,
    WIFI_SECURITY_WPA_Enterprise       = 0x00000040,
    WIFI_SECURITY_WPA2_Enterprise      = 0x00000080,
    WIFI_SECURITY_WPA_WPA2_Enterprise  = 0x00000100
}wifiSecurity_t;

typedef  enum wifiSecurityEncrption {
    WIFI_SECURITY_TKIP    = 1,
    WIFI_SECURITY_AES,
    WIFI_SECURITY_AES_TKIP
}wifiSecurityEncryption_t;

typedef enum wifiWPSMethod {
    WIFI_WPS_METHOD_UsbFlashDrive      = 0x00000001,
    WIFI_WPS_METHOD_Ethernet           = 0x00000002,
    WIFI_WPS_METHOD_ExternalNFCToken   = 0x00000004,
    WIFI_WPS_METHOD_IntgratedNFCToken  = 0x00000008,
    WIFI_WPS_METHOD_NFCInterface       = 0x00000010,
    WIFI_WPS_METHOD_PushButton         = 0x00000020,
    WIFI_WPS_METHOD_Pin                = 0x00000040
}wifiWPSMethod_t;


/*
 *  Config portion of WiFi radio info
 */

typedef  struct
wifiRadioCfg
{
    unsigned long                   InstanceNumber;
    char                            Alias[64];
    unsigned char                   bEnabled;
    wifiInterface_t                 OperatingFrequencyBand;
    unsigned long                   OperatingStandards;
    unsigned long                   Channel;
    unsigned char                   AutoChannelEnable;
    unsigned long                   AutoChannelRefreshPeriod;
    wifiBand_t                      OperatingChannelBandwidth;
    wifiSideband_t                  ExtensionChannel;
    wifiGuardInterval_t             GuardInterval;
    int                             MCS;
    int                             TransmitPower;
    unsigned char                   IEEE80211hEnabled;
    char                            RegulatoryDomain[4];
    /* Below is Cisco Extensions */
    wifiBasicRate_t                 BasicRate;
    wifiTxRate_t                    TxRate;
    unsigned char                   APIsolation;
    unsigned char                   FrameBurst;
    unsigned char                   CTSProtectionMode;
    unsigned long                   BeaconInterval;
    unsigned long                   DTIMInterval;
    unsigned long                   FragmentationThreshold;
    unsigned long                   RTSThreshold;
}wifiRadioCfg_t;

/*
 * Static portion of WiFi radio info
 */

typedef  struct
wifiRadioSinfo
{
    char                            Name[64];
    unsigned char                   bUpstream;
    unsigned long                   MaxBitRate;
    unsigned long                   SupportedFrequencyBands;
    unsigned long                   SupportedStandards;
    char                            PossibleChannels[512];
    unsigned char                   AutoChannelSupported;
    char                            TransmitPowerSupported[64];
    unsigned char                   IEEE80211hSupported;
}wifiRadioSinfo_t;

/*
 * Dynamic portion of WiFi radio info
 */

typedef  struct
wifiRadioDinfo
{
    int                             Status;
    unsigned long                   LastChange;
    char                            ChannelsInUse[512];
}wifiRadioDinfo_t;

/*
 * WiFi Radio Entry 
 */

typedef struct
wifiRadioEntry
{
     wifiRadioCfg_t                  Cfg;
     wifiRadioSinfo_t                StaticInfo;
     wifiRadioDinfo_t                DynamicInfo;

}wifiRadioEntry_t;

/*
 * WiFi Radio Stats 
 */

typedef  struct
wifiRadioStats
{
    unsigned long                   BytesSent;
    unsigned long                   BytesReceived;
    unsigned long                   PacketsSent;
    unsigned long                   PacketsReceived;
    unsigned long                   ErrorsSent;
    unsigned long                   ErrorsReceived;
    unsigned long                   DiscardPacketsSent;
    unsigned long                   DiscardPacketsReceived;

}wifiRadioStats_t;

/*
 * Structure definitions for WiFi SSID
 */

typedef struct
wifiSSIDCfg
{
    unsigned long                   InstanceNumber;
    char                            Alias[64];
    unsigned char                   bEnabled;
    char                            WiFiRadioName[64];
    char                            SSID[32];
}wifiSSIDCfg_t;


/*
 * Static portion of WiFi SSID info
 */

typedef  struct
wifiSSIDSInfo
{
    char                            Name[64];
    unsigned char                   BSSID[6];
    unsigned char                   MacAddress[6];
}wifiSSIDSInfo_t;

/*
 *  *  Dynamic portion of WiFi SSID info
 *   */

typedef  struct
wifiSSIDDInfo
{
    int 			    Status;
    unsigned long                   LastChange;
}wifiSSIDDInfo_t;

/*
 *  *  WiFi SSID Entry
 *   */

typedef struct
wifiSSIDEntry
{
     wifiSSIDCfg_t		    Cfg;
     wifiSSIDSInfo_t                StaticInfo;
     wifiSSIDDInfo_t                DynamicInfo;

}wifiSSIDEntry_t;

typedef struct
wifiSSIDStats
{
    unsigned long                   BytesSent;
    unsigned long                   BytesReceived;
    unsigned long                   PacketsSent;
    unsigned long                   PacketsReceived;
    unsigned long                   ErrorsSent;
    unsigned long                   ErrorsReceived;
    unsigned long                   UnicastPacketsSent;
    unsigned long                   UnicastPacketsReceived;
    unsigned long                   DiscardPacketsSent;
    unsigned long                   DiscardPacketsReceived;
    unsigned long                   MulticastPacketsSent;
    unsigned long                   MulticastPacketsReceived;
    unsigned long                   BroadcastPacketsSent;
    unsigned long                   BroadcastPacketsReceived;
    unsigned long                   UnknownProtoPacketsReceived;
}wifiSSIDStats_t;

typedef struct
wifiAPCfg
{
    unsigned long                   InstanceNumber;
    char                            Alias[64];
    char                            SSID[32];           /* Reference to SSID name */

    unsigned char                   bEnabled;
    unsigned char		    SSIDAdvertisementEnabled;
    unsigned long                   RetryLimit;
    unsigned char                   WMMEnable;
    unsigned char                   UAPSDEnable;
}wifiAPCfg_t;

typedef struct
wifiAPInfo
{
    int				    Status;
    unsigned char                   WMMCapability;
    unsigned char                   UAPSDCapability;
}wifiAPInfo_t;

typedef struct
wifiAPEntry
{
     wifiAPCfg_t                  Cfg;
     wifiAPInfo_t                 Info;

}wifiAPEntry_t;

typedef struct
wifiAPSecCfg
{
    wifiSecurity_t                  ModeEnabled;
    unsigned char                   WEPKeyp[13];
    unsigned char                   PreSharedKey[32];
    unsigned char                   KeyPassphrase[64];
    unsigned long                   RekeyingInterval;
    wifiSecurityEncryption_t        EncryptionMethod;
    IPV4_ADDRESS                    RadiusServerIPAddr;
    unsigned long                   RadiusServerPort;
    char                            RadiusSecret[64];
}wifiAPSecCfg_t;

typedef struct
wifiAPSecInfo
{
    unsigned long                   ModesSupported;     /* Bitmask of wifiSecurity_t*/
}wifiAPSecInfo_t;

typedef struct
wifiAPSecEntry
{
    wifiAPSecCfg_t                  Cfg;
    wifiAPSecInfo_t                 Info;
}wifiAPSecEntry_t;

typedef struct
wifiAPWPSCfg
{
    unsigned char                   bEnabled;
    unsigned long                   ConfigMethodsEnabled;
}wifiAPWPSCfg_t;

typedef struct
wifiAPWPSInfo
{
    unsigned long                   ConfigMethodsSupported;   /* Bitmask of wifiWPSMethod_t */
}wifiAPWPSInfo_t;

typedef struct
wifiAPWPSEntry
{
    wifiAPWPSCfg_t                  Cfg;
    wifiAPWPSInfo_t                 Info;
}wifiAPWPSEntry_t;

typedef struct
wifiAPAssocDevice
{
    unsigned char                   MacAddress[6];
    unsigned char                   AuthenticationState;    
    unsigned long                   LastDataDownlinkRate;
    unsigned long                   LastDataUplinkRate;
    int                             SignalStrength;
    unsigned long                   Retransmissions;    
    unsigned char                   Active;
}wifiAPAssocDevice_t;

/*
 *  * Mac Filter Cfg
 *   */

typedef struct 
wifiMacFilterCfg
{
    unsigned char macFilterEnabled;
    unsigned char macFilterMode;
    unsigned long NumberMacAddrList;
    unsigned char macAddress[6*50];
}wifiMacFilterCfg_t;


/* Function Definitions */
int Utopia_GetWifiRadioInstances();
int Utopia_GetWifiRadioEntry(UtopiaContext *ctx, unsigned long ulIndex, void *pEntry);

int Utopia_GetIndexedWifiRadioCfg(UtopiaContext *ctx, unsigned long ulIndex, void *cfg);
int Utopia_GetWifiRadioCfg(UtopiaContext *ctx,int dummyInstanceNum, void *cfg);
int Utopia_GetWifiRadioSinfo(unsigned long ulIndex, void *sInfo);
int Utopia_GetIndexedWifiRadioDinfo(UtopiaContext *ctx, unsigned long ulIndex, void *dInfo);
int Utopia_GetWifiRadioDinfo(unsigned long ulInstanceNum, void *dInfo);

int Utopia_SetWifiRadioCfg(UtopiaContext *ctx, void *cfg);
int Utopia_WifiRadioSetValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNum, char *pAlias);

int Utopia_GetWifiRadioStats(unsigned long ulInstanceNum, void *stats);

int Utopia_GetWifiSSIDInstances(UtopiaContext *ctx);
int Utopia_GetWifiSSIDEntry(UtopiaContext *ctx, unsigned long ulIndex, void *pEntry);

int Utopia_GetIndexedWifiSSIDCfg(UtopiaContext *ctx, unsigned long ulIndex, void *cfg);
int Utopia_GetWifiSSIDCfg(UtopiaContext *ctx, int dummyInstanceNum, void *cfg);
int Utopia_GetWifiSSIDSInfo(unsigned long ulIndex, void *sInfo);
int Utopia_GetWifiSSIDDInfo(unsigned long ulInstanceNum, void *dInfo);
int Utopia_GetIndexedWifiSSIDDInfo(UtopiaContext *ctx, unsigned long ulIndex, void *dInfo);
int Utopia_GetWifiSSIDDinfo(unsigned long ulInstanceNum, void *dInfo);

int Utopia_SetWifiSSIDCfg(UtopiaContext *ctx, void *cfg);
int Utopia_WifiSSIDSetValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNum, char *pAlias);

int Utopia_GetWifiSSIDStats(unsigned long ulInstanceNum, void *stats);

int Utopia_GetWifiAPInstances(UtopiaContext *ctx);
int Utopia_GetWifiAPEntry(UtopiaContext *ctx, char*pSSID, void *pEntry);

int Utopia_GetIndexedWifiAPCfg(UtopiaContext *ctx, unsigned long ulIndex, void *pCfg);
int Utopia_GetWifiAPCfg(UtopiaContext *ctx, int dummyInstanceNum, void *cfg);
int Utopia_GetWifiAPInfo(UtopiaContext *ctx, char *pSSID, void *info);

int Utopia_SetWifiAPCfg(UtopiaContext *ctx, void *cfg);
int Utopia_WifiAPSetValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNum, char *pAlias);

int Utopia_GetWifiAPSecEntry(UtopiaContext *ctx, char*pSSID, void *pEntry);
int Utopia_GetWifiAPSecCfg(UtopiaContext *ctx, char*pSSID, void *cfg);
int Utopia_GetWifiAPSecInfo(UtopiaContext *ctx, char *pSSID, void *info);

int Utopia_SetWifiAPSecCfg(UtopiaContext *ctx,char *pSSID, void *cfg);

int Utopia_GetWifiAPWPSEntry(UtopiaContext *ctx, char*pSSID, void *pEntry);
int Utopia_GetWifiAPWPSCfg(UtopiaContext *ctx, char*pSSID, void *cfg);

int Utopia_SetWifiAPWPSCfg(UtopiaContext *ctx,char *pSSID, void *cfg);

unsigned long Utopia_GetAssociatedDevicesCount(UtopiaContext *ctx, char *pSSID);
int Utopia_GetAssocDevice(UtopiaContext *ctx, char *pSSID, unsigned long ulIndex, void *assocDev);

unsigned long Utopia_GetWifiAPIndex(UtopiaContext *ctx, char *pSSID);

/*MF */
int Utopia_GetWifiAPMFCfg(UtopiaContext *ctx, char *pSSID, void *cfg);
int Utopia_SetWifiAPMFCfg(UtopiaContext *ctx, char *pSSID, void *cfg);

/* Utility functions */
unsigned long instanceNum_find(unsigned long ulIndex, int *numArray, int numArrayLen);
unsigned long parse_proc_net_dev(char *if_name, int field_to_parse);
int getMacList(char *macList, unsigned char *macAddr, char *tok, unsigned long *numlist);
int setMacList(unsigned char *macAddr, char *macList, unsigned long numMac);
void allocateMultiSSID_Struct(int i);
void freeMultiSSID_Struct(int i);

#endif // __UTAPI_TR_WLAN_H__
