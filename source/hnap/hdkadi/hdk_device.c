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

#include "hdk_device.h"
#include "secure_wrapper.h"
#include "hdk_util.h"
#include "hdk_srv.h"
#include "hnap12.h"
#include "hotspot.h"
#include "utctx_api.h"
#include "safec_lib_common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef HNAP_DEBUG
#define UTOPIA_NW_ARP_FILE          "/proc/net/arp"
#define UTOPIA_NW_DEVICE_FILE       "/proc/net/dev"
#define UTOPIA_DNS_LEASE_FILE       "/tmp/dnsmasq.leases"
#else
#define UTOPIA_NW_ARP_FILE          HNAP_DEBUG_ARP_FILE
#define UTOPIA_NW_DEVICE_FILE       HNAP_DEBUG_DEV_FILE
#define UTOPIA_DNS_LEASE_FILE       HNAP_DEBUG_DNS_FILE
#endif


/* UT610N Lan/Wan interfaces */
#define UTOPIA_LAN_INTERFACE        "vlan1"
#define UTOPIA_WAN_INTERFACE        "vlan2"


/* <crypt.h> Function prototype required to compile as ISO C */
extern char* crypt( const char* key, const char* setting );

/* From mini_httpd/htpasswd.c */
static unsigned char itoa64[] =         /* 0 ... 63 => ascii - 64 */
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void to64(register char *s, register long v, register int n)
{
    while (--n >= 0)
    {
        *s++ = itoa64[v&0x3f];
        v >>= 6;
    }
}

/* UT610N wireless capabilies */
static struct
{
    char* pszRadioID;
    int   iFrequency;
    char* pszSysCfgPrefix;
    char* pszEthInterface;
}
    g_WiFiRadios[] =
{
    /* { RadioID, Frequency, Prefix, Interface } */
    { "RADIO_2.4GHz", 2, "wl0", "eth0" },
    { "RADIO_5GHz", 5,  "wl1", "eth1" }
};

#define WIFI_RADIO_NUM sizeof(g_WiFiRadios) / sizeof(*g_WiFiRadios)

#define WIFI_RADIO_ID(ix) g_WiFiRadios[ix].pszRadioID
#define WIFI_RADIO_FREQ(ix) g_WiFiRadios[ix].iFrequency
#define WIFI_RADIO_PREFIX(ix) g_WiFiRadios[ix].pszSysCfgPrefix
#define WIFI_RADIO_ETHIF(ix) g_WiFiRadios[ix].pszEthInterface

/* Supported Channels for each radio */
static int g_Channels[][WIFI_RADIO_NUM] =
{
    /* { RADIO_2.4GHz, RADIO_5GHz } */
    { 0,  0   },
    { 1,  36  },
    { 2,  40  },
    { 3,  44  },
    { 4,  48  },
    { 5,  149 },
    { 6,  153 },
    { 7,  157 },
    { 8,  161 },
    { 9,  -1  },
    { 10, -1  },
    { 11, -1  },
    { -1, -1  }
};

/* Supported WideChannels for each radio */
static int g_WideChannels[][WIFI_RADIO_NUM] =
{
    /* { RADIO_2.4GHz, RADIO_5GHz } */
    { 0,  0   },
    { 3,  38  },
    { 4,  46  },
    { 5,  151 },
    { 6,  159 },
    { 7,  -1  },
    { 8,  -1  },
    { 9,  -1  },
    { -1, -1  }
};

/* Supported WiFiModes for each radio */
static HNAP12_Enum_PN_WiFiMode g_WiFiModes[][WIFI_RADIO_NUM] =
{
    /* { RADIO_2.4GHz, RADIO_5GHz } */
    { HNAP12_Enum_PN_WiFiMode_802_11b, HNAP12_Enum_PN_WiFiMode_802_11a },
    { HNAP12_Enum_PN_WiFiMode_802_11g, HNAP12_Enum_PN_WiFiMode_802_11n },
    { HNAP12_Enum_PN_WiFiMode_802_11n, HNAP12_Enum_PN_WiFiMode_802_11an },
    { HNAP12_Enum_PN_WiFiMode_802_11bg, HNAP12_Enum_PN_WiFiMode__UNKNOWN__ },
    { HNAP12_Enum_PN_WiFiMode_802_11bgn, HNAP12_Enum_PN_WiFiMode__UNKNOWN__ },
    { HNAP12_Enum_PN_WiFiMode__UNKNOWN__, HNAP12_Enum_PN_WiFiMode__UNKNOWN__ }
};

/* Security capability is the same for all radios */
typedef struct _WiFiSecurityMap
{
    HNAP12_Enum_PN_WiFiSecurity eWiFiSecurity;
    HNAP12_Enum_PN_WiFiEncryption eWiFiEncryptions[2];
} WiFiSecurityMap;

static WiFiSecurityMap g_WiFiSecurityMaps[] =
{
    { HNAP12_Enum_PN_WiFiSecurity_WPA_AUTO_PSK, { HNAP12_Enum_PN_WiFiEncryption_AES, HNAP12_Enum_PN_WiFiEncryption_TKIP }},
    { HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS, { HNAP12_Enum_PN_WiFiEncryption_AES, HNAP12_Enum_PN_WiFiEncryption_TKIPORAES }}
};

/* Month abreviations for month parsing */
static char* g_ppszMonthAbbrev[] =
{
    "Jan","Feb","Mar","Apr",
    "May","Jun","Jul","Aug",
    "Sep","Oct","Nov","Dec"
};

/* RRP Supported locales */
static char* g_ppszLocales[] =
{
    "en-us"
};

/* Map UTC to/from RRP timezone format */
static struct
{
    char* pszUTC;
    char* pszTZ;
    char* pszDT;
}
    g_TimezoneMap[] =
{
    {"UTC-12:00", "MHT12",     0 },
    {"UTC-11:00", "WST11",     0 },
    {"UTC-10:00", "HST10",     0 },
    {"UTC-09:00", "AKST9",     "AKDT,M3.2.0/02:00,M11.1.0/02:00" },
    {"UTC-08:00", "PST8",      "PDT,M3.2.0/02:00,M11.1.0/02:00" },
    {"UTC-07:00", "MST7",      "MDT,M3.2.0/02:00,M11.1.0/02:00" },
    {"UTC-06:00", "CST6",      "CDT,M3.2.0/02:00,M11.1.0/02:00" },
    {"UTC-05:00", "EST5",      "EDT,M3.2.0/02:00,M11.1.0/02:00" },
    {"UTC-04:00", "VET4",      0 },
    {"UTC-04:00", "CLT4",      "CLST,M10.2.6/04:00,M3.2.6/03:00" },
    {"UTC-03:30", "NST03:30",  "NDT,M3.2.0/0:01,M11.1.0/0:01" },
    {"UTC-03:00", "ART3",      0 },
    {"UTC-03:00", "BRT3",      "BRST,M10.3.0/0:01,M2.3.0/0:01" },
    {"UTC-02:00", "MAT2",      0 },
    {"UTC-01:00", "AZOT1",     "AZOST,M3.5.0/00:00,M10.5.0/01:00" },
    {"UTC",       "GMT0",      "BST,M3.5.0/01:00,M10.5.0/02:00" },
    {"UTC+01:00", "CET-1",     "CEST,M3.5.0/02:00,M10.5.0/03:00" },
    {"UTC+02:00", "SAST-2",    0 },
    {"UTC+02:00", "EET-2",     "EEST,M3.5.0/03:00,M10.5.0/04:00" },
    {"UTC+03:00", "AST-3",     0 },
    {"UTC+04:00", "GST-4",     0 },
    {"UTC+05:00", "PKT-5",     0 },
    {"UTC+05:30", "IST-05:30", 0 },
    {"UTC+06:00", "ALMT-6",    0 },
    {"UTC+07:00", "ICT-7",     0 },
    {"UTC+08:00", "HKT-8",     0 },
    {"UTC+09:00", "JST-9",     0 },
    {"UTC+10:00", "GST-10",    0 },
    {"UTC+10:00", "AEST-10",   "AEDT-11,M10.1.0/00:00,M4.1.0/00:00" },
    {"UTC+11:00", "SBT-11",    0 },
    {"UTC+12:00", "FJT-12",    0 },
    {"UTC+12:00", "NZST-12",   "NZDT,M9.5.0/02:00,M4.1.0/03:00" }
};

#define TIMEZONE_NUM sizeof(g_TimezoneMap) / sizeof(*g_TimezoneMap)

/* Supported WAN Types */
static HNAP12_Enum_PN_WANType g_peWANTypes[] =
{
    HNAP12_Enum_PN_WANType_DHCP,
    HNAP12_Enum_PN_WANType_Static,
    HNAP12_Enum_PN_WANType_DHCPPPPoE,
    HNAP12_Enum_PN_WANType_DynamicPPTP,
    HNAP12_Enum_PN_WANType_StaticPPTP,
    HNAP12_Enum_PN_WANType_DynamicL2TP,
    HNAP12_Enum_PN_WANType_StaticL2TP
};

/* Generic struct used to map between the various HNAP12_Enums and their RRP string representations */
typedef struct _EnumString_Map
{
    char* pszStr;
    int iEnum;
} EnumString_Map;

/* WanType table */
static EnumString_Map g_WanTypeMap[] =
{
    { "dhcp", HNAP12_Enum_PN_WANType_DHCP },
    { "static", HNAP12_Enum_PN_WANType_Static },
    { "pppoe", HNAP12_Enum_PN_WANType_DHCPPPPoE },
    { "pptp", HNAP12_Enum_PN_WANType_DynamicPPTP },
    { "pptp", HNAP12_Enum_PN_WANType_StaticPPTP },
    { "l2tp", HNAP12_Enum_PN_WANType_DynamicL2TP },
    { "l2tp", HNAP12_Enum_PN_WANType_StaticL2TP },
    { 0, 0 }
};

/* WiFiEncryption table */
static EnumString_Map g_WiFiEncMap[] =
{
    { "aes", HNAP12_Enum_PN_WiFiEncryption_AES },
    { "tkip", HNAP12_Enum_PN_WiFiEncryption_TKIP },
    { "tkip+aes", HNAP12_Enum_PN_WiFiEncryption_TKIPORAES },
    { "", HNAP12_Enum_PN_WiFiEncryption_ },
    { 0, 0 }
};

/* WiFiSecurity table */
static EnumString_Map g_WiFiSecTypeMap[] =
{
    { "wpa2-personal", HNAP12_Enum_PN_WiFiSecurity_WPA_AUTO_PSK },
    { "wpa2-enterprise", HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS },
    { "disabled", HNAP12_Enum_PN_WiFiSecurity_ },
    { 0, 0 }
};

/* WiFiMode table */
static EnumString_Map g_WiFiModeMap[] =
{
    { "11a", HNAP12_Enum_PN_WiFiMode_802_11a },
    { "11b", HNAP12_Enum_PN_WiFiMode_802_11b },
    { "11g", HNAP12_Enum_PN_WiFiMode_802_11g },
    { "11n", HNAP12_Enum_PN_WiFiMode_802_11n },
    { "11a 11n", HNAP12_Enum_PN_WiFiMode_802_11an },
    { "11b 11g", HNAP12_Enum_PN_WiFiMode_802_11bg },
    { "11b 11g 11n", HNAP12_Enum_PN_WiFiMode_802_11bgn },
    { 0, 0 }
};

/* Helper function to map from HDK_Enum to string */
static char* s_EnumToStr(EnumString_Map* pMap, int iEnum)
{
    if (pMap)
    {
        for( ;pMap->pszStr; ++pMap)
        {
            if (iEnum == pMap->iEnum)
            {
                return pMap->pszStr;
            }
        }
    }

    return 0;
}

/* Helper function to map from string to HNAP12_Enum */
static int s_StrToEnum(EnumString_Map* pMap, char* pszStr)
{
    if (pMap && pszStr)
    {
        for ( ;pMap->pszStr; ++pMap)
        {
            if (strcmp(pszStr, pMap->pszStr) == 0)
            {
                return pMap->iEnum;
            }
        }
    }

    return 0;
}

/* Helper function to map from RadioID string to syscfg index */
static char* s_RadioIDToPrefix(char* pszRadioID)
{
    unsigned int i;

    if (pszRadioID)
    {
        for (i = 0; i < WIFI_RADIO_NUM; ++i)
        {
            if(!strcmp(pszRadioID, g_WiFiRadios[i].pszRadioID))
            {
                /* Found the RadioID! */
                return g_WiFiRadios[i].pszSysCfgPrefix;
            }
        }
    }

    return 0;
}

/* Helper function to map from WiFiMode to LANConnection */
static HNAP12_Enum_PN_LANConnection s_WiFiModeToLANConnection(HNAP12_Enum_PN_WiFiMode eMode)
{
    switch(eMode)
    {
        case HNAP12_Enum_PN_WiFiMode_802_11a:
            return HNAP12_Enum_PN_LANConnection_WLAN_802_11a;

        case HNAP12_Enum_PN_WiFiMode_802_11b:
            return HNAP12_Enum_PN_LANConnection_WLAN_802_11b;

        case HNAP12_Enum_PN_WiFiMode_802_11g:
        case HNAP12_Enum_PN_WiFiMode_802_11bg:
            return HNAP12_Enum_PN_LANConnection_WLAN_802_11g;

        case HNAP12_Enum_PN_WiFiMode_802_11n:
        case HNAP12_Enum_PN_WiFiMode_802_11bn:
        case HNAP12_Enum_PN_WiFiMode_802_11bgn:
        case HNAP12_Enum_PN_WiFiMode_802_11gn:
        case HNAP12_Enum_PN_WiFiMode_802_11an:
            return HNAP12_Enum_PN_LANConnection_WLAN_802_11n;

        default:
            return HNAP12_Enum_PN_LANConnection__UNKNOWN__;
    }
}

/* Helper function to parse lease string to minutes */
static int s_LeaseStrToMinutes(char* pszLeaseTime)
{
    char cUnit;
    int iLeaseTime = 0;

    if (pszLeaseTime)
    {
        if (sscanf(pszLeaseTime, "%d%1c", &iLeaseTime, &cUnit) == 2)
        {
            if (cUnit == 'h')
            {
                /* Convert hours to minutes */
                iLeaseTime *= 60;
            }
        }
        else if (sscanf(pszLeaseTime, "%d", &iLeaseTime) == 1)
        {
            /* If not 0 and less than 60 seconds, set it to 1 minute */
            if (iLeaseTime != 0 && iLeaseTime < 60)
            {
                iLeaseTime = 1;
            }
            else
            {
                /* Convert seconds to minutes */
                iLeaseTime /= 60;
            }
        }
        /* Default value of 0 is 1 day */
        if (iLeaseTime == 0)
        {
            iLeaseTime = 1440;
        }
    }
    return iLeaseTime;
}

/* Helper function to get connected clients */
static HDK_XML_Member* s_GetConnectedClients(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Member* pMember = 0;

    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    char pszMAC[18];
    char pszIP[16];
    FILE* fdARP;
    FILE* fdDNS;
    HDK_XML_IPAddress ip = {0,0,0,0};
    HDK_XML_MACAddress mac;
    HDK_XML_MACAddress* pMAC;
    HDK_XML_Member* pmClient;
    HDK_XML_Struct* psClient;
    HDK_XML_Struct* psClients;
    int iScanned;

    if ((psClients = HDK_XML_Set_Struct(pStruct, element)) != 0)
    {
        /* DHCP lease table */
        if ((fdDNS = fopen(UTOPIA_DNS_LEASE_FILE, "r")) != 0)
        {
            char pszHostname[256];
            char pszClientID[256];
            int iLeaseSecs;
            int iLeaseExp;

            Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_LeaseTime, pszBuffer, sizeof(pszBuffer));
            iLeaseSecs = s_LeaseStrToMinutes(pszBuffer) * 60;

            /*
             * Parse the dnsmasq leases file:
             *     '1247687872 00:19:bb:e2:dc:44 192.168.1.118 richard-ibex *'
             */
            while ((iScanned = fscanf(fdDNS, "%d %s %s %s %s\n", &iLeaseExp, pszMAC, pszIP, pszHostname, pszClientID)) > 0)
            {
                if (iScanned != 5 ||
                    HDK_Util_StrToMAC(&mac, pszMAC) == 0)
                {
                    continue;
                }

                /* Don't care if it failed to parse */
                HDK_Util_StrToIP(&ip, pszIP);

                /* Add the connected client */
                if ((psClient = HDK_XML_Append_Struct(psClients, HNAP12_Element_PN_ConnectedClient)) != 0)
                {
                    HDK_XML_Set_DateTime(psClient, HNAP12_Element_PN_ConnectTime, iLeaseExp - iLeaseSecs);
                    HDK_XML_Set_MACAddress(psClient, HNAP12_Element_PN_MacAddress, &mac);
                    HDK_XML_Set_IPAddress(psClient, HNAP12_Element_PN_IPAddress, &ip);
                    HDK_XML_Set_String(psClient, HNAP12_Element_PN_DeviceName, (*pszHostname == '*' ? "" : pszHostname));
                    HDK_XML_Set_Bool(psClient, HNAP12_Element_PN_Active, 1);
                    HNAP12_Set_PN_LANConnection(psClient, HNAP12_Element_PN_PortName, HNAP12_Enum_PN_LANConnection_LAN);
                    HDK_XML_Set_Bool(psClient, HNAP12_Element_PN_Wireless, 0);
                }
            }
            fclose(fdDNS);
        }

        /* ARP table */
        if ((fdARP = fopen(UTOPIA_NW_ARP_FILE, "r")) != 0)
        {
            char pszDevice[16]; /* Defined as char[16] in 'linux-2.6.22-ut610n/include/linux/if_arp.h' */
            char pszFlags[7];   /* Defined as an int in 'linux-2.6.22-ut610n/include/linux/if_arp.h', so need room for 0xFFFF' */
            char pszHeader[128];
            char pszHWType[5];  /* ARP HW Type has 16 bits, so need room for 0xFF */
            char pszLanIf[6];
            char pszMask[16];

            /* Skip the header */
            if (fgets(pszHeader, sizeof(pszHeader), fdARP) != 0)
            {
                /* Get the LAN interface name */
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_IfName, pszLanIf, sizeof(pszLanIf));

                /*
                 * Parse the arp file:
                 *     'IP address       HW type     Flags       HW address            Mask     Device'
                 *     '192.168.1.120    0x1         0x2         00:0C:29:29:89:9F     *        br0'
                 */
                while ((iScanned = fscanf(fdARP, "%s %s %s %s %s %s", pszIP, pszHWType, pszFlags, pszMAC, pszMask, pszDevice)) > 0)
                {
                    if (iScanned != 6 ||
                        HDK_Util_StrToMAC(&mac, pszMAC) == 0 ||
                        /* If it's not on the LAN, then skip */
                        strcmp(pszDevice, pszLanIf) != 0)
                    {
                        continue;
                    }

                    /* Make sure this client is not already in the list */
                    for (pmClient = psClients->pHead; pmClient; pmClient = pmClient->pNext)
                    {
                        if ((pMAC = HDK_XML_Get_MACAddress(HDK_XML_GetMember_Struct(pmClient), HNAP12_Element_PN_MacAddress)) != 0 &&
                            pMAC->a == mac.a && pMAC->b == mac.b &&
                            pMAC->c == mac.c && pMAC->d == mac.d &&
                            pMAC->e == mac.e && pMAC->f == mac.f)
                        {
                            break;
                        }
                    }

                    /* If we didn't find it, then create a new entry for it */
                    if (pmClient == 0 &&
                        (psClient = HDK_XML_Append_Struct(psClients, HNAP12_Element_PN_ConnectedClient)) != 0)
                    {
                        HDK_XML_Set_DateTime(psClient, HNAP12_Element_PN_ConnectTime, 0);
                        HDK_XML_Set_MACAddress(psClient, HNAP12_Element_PN_MacAddress, &mac);
                        HDK_XML_Set_String(psClient, HNAP12_Element_PN_DeviceName, "");
                        HDK_XML_Set_Bool(psClient, HNAP12_Element_PN_Active, 1);
                        HNAP12_Set_PN_LANConnection(psClient, HNAP12_Element_PN_PortName, HNAP12_Enum_PN_LANConnection_LAN);
                        HDK_XML_Set_Bool(psClient, HNAP12_Element_PN_Wireless, 0);
                    }
                }
            }
            fclose(fdARP);
        }

        /* Wireless Association */
#ifndef HNAP_DEBUG
        {
            unsigned int ixRadio;

            /* Loop over the radios */
            for (ixRadio = 0; ixRadio < WIFI_RADIO_NUM; ++ixRadio)
            {
                char pszCmd[26];
                FILE* fdWlAssoc;
                HNAP12_Enum_PN_LANConnection lanConn;

                /* Figure out the LANConnection */
                Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_NetworkMode,
                                WIFI_RADIO_PREFIX(ixRadio), pszBuffer, sizeof(pszBuffer));

                lanConn = s_WiFiModeToLANConnection((HNAP12_Enum_PN_WiFiMode)s_StrToEnum(g_WiFiModeMap, pszBuffer));
                if (lanConn == HNAP12_Enum_PN_LANConnection__UNKNOWN__)
                {
                    continue;
                }

                /* Spawn a process with the wl command */
                if ((fdWlAssoc = v_secure_popen("r", "wl -i %s assoclist", WIFI_RADIO_ETHIF(ixRadio))) == 0)
                {
                    continue;
                }

                /*
                 * Parse the wireless association list:
                 *     'assoclist 00:1E:C2:C2:5D:31\n'
                 */
                while ((iScanned = fscanf(fdWlAssoc, "assoclist %s\n", pszMAC)) > 0)
                {
                    if (iScanned != 1 ||
                        HDK_Util_StrToMAC(&mac, pszMAC) == 0)
                    {
                        continue;
                    }

                    /* If the client is already in the list, then update the LANConnection and wireless elements */
                    for (pmClient = psClients->pHead; pmClient; pmClient = pmClient->pNext)
                    {
                        if ((pMAC = HDK_XML_Get_MACAddress(HDK_XML_GetMember_Struct(pmClient), HNAP12_Element_PN_MacAddress)) != 0 &&
                            pMAC->a == mac.a && pMAC->b == mac.b &&
                            pMAC->c == mac.c && pMAC->d == mac.d &&
                            pMAC->e == mac.e && pMAC->f == mac.f)
                        {
                            HNAP12_Set_PN_LANConnection(HDK_XML_GetMember_Struct(pmClient), HNAP12_Element_PN_PortName, lanConn);
                            HDK_XML_Set_Bool(HDK_XML_GetMember_Struct(pmClient), HNAP12_Element_PN_Wireless, 1);

                            break;
                        }
                    }

                    /* If we didn't find it, then create a new entry for it */
                    if (pmClient == 0 &&
                        (psClient = HDK_XML_Append_Struct(psClients, HNAP12_Element_PN_ConnectedClient)) != 0)
                    {
                        HDK_XML_Set_DateTime(psClient, HNAP12_Element_PN_ConnectTime, 0);
                        HDK_XML_Set_MACAddress(psClient, HNAP12_Element_PN_MacAddress, &mac);
                        HDK_XML_Set_String(psClient, HNAP12_Element_PN_DeviceName, "");
                        HDK_XML_Set_Bool(psClient, HNAP12_Element_PN_Active, 1);
                        HNAP12_Set_PN_LANConnection(psClient, HNAP12_Element_PN_PortName, lanConn);
                        HDK_XML_Set_Bool(psClient, HNAP12_Element_PN_Wireless, 1);
                    }
                }
                v_secure_pclose(fdWlAssoc);
            }
        }
#endif
        pMember = (HDK_XML_Member*)psClients;
    }

    return pMember;
}

/* WLanInfo get/set ADI value function type */
typedef HDK_XML_Member* (*HNAP12_WLan_GetFn)(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                             char* pszPrefix);

typedef int (*HNAP12_WLan_SetFn)(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                 char* pszPrefix);

static HDK_XML_Member* s_HNAP12_WLan_Get_SecurityEnabled(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                         char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HDK_XML_Set_Bool(pStruct, HNAP12_Element_PN_Enabled, (strcmp(pszBuffer, "disabled")));
}

static int s_HNAP12_WLan_Set_SecurityEnabled(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                             char* pszPrefix)
{
    int* pfEnabled;

    if ((pfEnabled = HDK_XML_Get_Bool(pStruct, HNAP12_Element_PN_Enabled)) != 0)
    {
        /*
         * If we're disabling, set to "disable", otherwise just set it to a default value
         * because it will get set by WLanType.
         */
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix,
                               *pfEnabled == 0 ? "disabled" : "wpa2-personal");
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_Type(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                              char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HNAP12_Set_PN_WiFiSecurity(pStruct, HNAP12_Element_PN_Type, s_StrToEnum(g_WiFiSecTypeMap, pszBuffer));
}

static int s_HNAP12_WLan_Set_Type(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                  char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    char* pszSecurity;
    HNAP12_Enum_PN_WiFiSecurity* peType;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    /* If we're disabling then just set disabled string */
    if (strcmp(pszBuffer, "disabled") == 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer);
    }
    else if ((peType = HNAP12_Get_PN_WiFiSecurity(pStruct, HNAP12_Element_PN_Type)) != 0 &&
             (pszSecurity = s_EnumToStr(g_WiFiSecTypeMap, *peType)) != 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszSecurity);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_Encryption(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                    char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Encryption, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HNAP12_Set_PN_WiFiEncryption(pStruct, HNAP12_Element_PN_Encryption, s_StrToEnum(g_WiFiEncMap, pszBuffer));
}

static int s_HNAP12_WLan_Set_Encryption(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                        char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    char* pszWLanEnc;
    HNAP12_Enum_PN_WiFiEncryption* peEncryption;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    /* If we're disabling then just set disabled string */
    if (strcmp(pszBuffer, "disabled") == 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer);
    }
    else if ((peEncryption = HNAP12_Get_PN_WiFiEncryption(pStruct, HNAP12_Element_PN_Encryption)) != 0 &&
             (pszWLanEnc = s_EnumToStr(g_WiFiEncMap, *peEncryption)) != 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Encryption, pszPrefix, pszWLanEnc);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_Key(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                             char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    HNAP12_Enum_PN_WiFiSecurity eWLanType;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    eWLanType = (HNAP12_Enum_PN_WiFiSecurity)s_StrToEnum(g_WiFiSecTypeMap, pszBuffer);
    if (eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA_PSK ||
        eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA2_PSK ||
        eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA_AUTO_PSK)
    {
        Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Passphrase, pszPrefix, pszBuffer, sizeof(pszBuffer));
    }
    else
    {
        *pszBuffer = '\0';
    }

    return HDK_XML_Set_String(pStruct, HNAP12_Element_PN_Key, pszBuffer);
}

static int s_HNAP12_WLan_Set_Key(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                 char* pszPrefix)
{
    char* pszKey;
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    HNAP12_Enum_PN_WiFiSecurity eType;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));
    eType = (HNAP12_Enum_PN_WiFiSecurity)s_StrToEnum(g_WiFiSecTypeMap, pszBuffer);

    if ((pszKey = HDK_XML_Get_String(pStruct, HNAP12_Element_PN_Key)) != 0)
    {
        if (eType == HNAP12_Enum_PN_WiFiSecurity_WPA_PSK ||
            eType == HNAP12_Enum_PN_WiFiSecurity_WPA2_PSK ||
            eType == HNAP12_Enum_PN_WiFiSecurity_WPA_AUTO_PSK)
        {
            return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Passphrase, pszPrefix, pszKey);
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_KeyRenewal(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                    char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    HNAP12_Enum_PN_WiFiSecurity eWLanType;
    int iKeyRenewal = 0;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    eWLanType = (HNAP12_Enum_PN_WiFiSecurity)s_StrToEnum(g_WiFiSecTypeMap, pszBuffer);
    if (eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA_PSK ||
        eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA2_PSK ||
        eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA_AUTO_PSK ||
        eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA_RADIUS ||
        eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS)
    {
        Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_KeyRenewal, pszPrefix, pszBuffer, sizeof(pszBuffer));
        iKeyRenewal = atoi(pszBuffer);
    }

    return HDK_XML_Set_Int(pStruct, HNAP12_Element_PN_KeyRenewal, iKeyRenewal);
}

static int s_HNAP12_WLan_Set_KeyRenewal(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                        char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    char pszRenewal[20] = {'\0'};
    int* piRenewal;
    errno_t safec_rc = -1;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    piRenewal = HDK_XML_Get_Int(pStruct, HNAP12_Element_PN_KeyRenewal);
    if (! piRenewal) {
        return 0;
    }
    safec_rc = sprintf_s(pszRenewal, sizeof(pszRenewal),"%d", *piRenewal);
    if(safec_rc < EOK)
    {
       ERR_CHK(safec_rc);
       return 0;
    }

    if ((*piRenewal >= 600 && *piRenewal <= 7200) ||
         (strcmp(pszBuffer, "disabled") == 0 && *piRenewal < 600))
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_KeyRenewal, pszPrefix, pszRenewal);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_RadiusIP1(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                   char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    HNAP12_Enum_PN_WiFiSecurity eWLanType;
    HDK_XML_IPAddress ipAddress = {0,0,0,0};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    eWLanType = (HNAP12_Enum_PN_WiFiSecurity)s_StrToEnum(g_WiFiSecTypeMap, pszBuffer);
    if (eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS)
    {
        Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadiusServer, pszPrefix, pszBuffer, sizeof(pszBuffer));
        HDK_Util_StrToIP(&ipAddress, pszBuffer);
    }

    return HDK_XML_Set_IPAddress(pStruct, HNAP12_Element_PN_RadiusIP1, &ipAddress);
}

static int s_HNAP12_WLan_Set_RadiusIP1(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                              char* pszPrefix)
{
    char pszIP[16] = {'\0'};

    if (HDK_Util_IPToStr(pszIP, HDK_XML_Get_IPAddress(pStruct, HNAP12_Element_PN_RadiusIP1)) != 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadiusServer, pszPrefix, pszIP);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_RadiusPort1(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                     char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    HNAP12_Enum_PN_WiFiSecurity eWLanType;
    int iPort = 0;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    eWLanType = (HNAP12_Enum_PN_WiFiSecurity)s_StrToEnum(g_WiFiSecTypeMap, pszBuffer);
    if (eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS)
    {
        Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadiusPort, pszPrefix, pszBuffer, sizeof(pszBuffer));
        iPort = atoi(pszBuffer);
    }

    return HDK_XML_Set_Int(pStruct, HNAP12_Element_PN_RadiusPort1, iPort);
}

static int s_HNAP12_WLan_Set_RadiusPort1(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                              char* pszPrefix)
{
    char pszPort[12] = {'\0'};
    int* piPort;
    errno_t safec_rc = -1;

    piPort = HDK_XML_Get_Int(pStruct, HNAP12_Element_PN_RadiusPort1);
    if (! piPort) {
        return 0;
    }
    safec_rc = sprintf_s(pszPort, sizeof(pszPort),"%d", *piPort);
    if(safec_rc < EOK)
    {
       ERR_CHK(safec_rc);
       return 0;
    }

    if (*piPort >= 0 && *piPort <= 65535)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadiusPort, pszPrefix, pszPort);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_RadiusSecret1(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                       char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    HNAP12_Enum_PN_WiFiSecurity eWLanType;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SecurityMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    eWLanType = (HNAP12_Enum_PN_WiFiSecurity)s_StrToEnum(g_WiFiSecTypeMap, pszBuffer);
    if (eWLanType == HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS)
    {
        Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Passphrase, pszPrefix, pszBuffer, sizeof(pszBuffer));
    }
    else
    {
        *pszBuffer = '\0';
    }

    return HDK_XML_Set_String(pStruct, HNAP12_Element_PN_RadiusSecret1, pszBuffer);
}

static int s_HNAP12_WLan_Set_RadiusSecret1(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                           char* pszPrefix)
{
    char* pszSecret;

    if ((pszSecret = HDK_XML_Get_String(pStruct, HNAP12_Element_PN_RadiusSecret1)) != 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Passphrase, pszPrefix, pszSecret);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_Enabled(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                 char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_State, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HDK_XML_Set_Bool(pStruct, HNAP12_Element_PN_Enabled, (strcmp(pszBuffer, "up") == 0));
}

static int s_HNAP12_WLan_Set_Enabled(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                     char* pszPrefix)
{
    int* pfEnabled;

    if ((pfEnabled = HDK_XML_Get_Bool(pStruct, HNAP12_Element_PN_Enabled)) != 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_State, pszPrefix, *pfEnabled == 0 ? "down" : "up");
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_Mode(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                              char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_NetworkMode, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HNAP12_Set_PN_WiFiMode(pStruct, HNAP12_Element_PN_Mode, s_StrToEnum(g_WiFiModeMap, pszBuffer));
}

static int s_HNAP12_WLan_Set_Mode(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                  char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_State, pszPrefix, pszBuffer, sizeof(pszBuffer));

    /* We only need to change this if we're enabling */
    if (strcmp(pszBuffer, "up") == 0)
    {
        char* pszMode;
        HNAP12_Enum_PN_WiFiMode* peMode;

        if ((peMode = HNAP12_Get_PN_WiFiMode(pStruct, HNAP12_Element_PN_Mode)) != 0 &&
            (pszMode = s_EnumToStr(g_WiFiModeMap, *peMode)) != 0)
        {
            return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_NetworkMode, pszPrefix, pszMode);
        }
    }
    else
    {
        return 1;
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_MacAddress(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                    char* pszPrefix)
{
    HDK_XML_MACAddress macAddress = {10,10,10,10,10,10};

    /* TODO: Implement WLan interface macaddress get */
    return HDK_XML_Set_MACAddress(pStruct, HNAP12_Element_PN_MacAddress, &macAddress);

    /* Unused variables */
    (void) pMethodCtx;
    (void) pszPrefix;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_SSID(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                              char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SSID, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HDK_XML_Set_String(pStruct, HNAP12_Element_PN_SSID, pszBuffer);
}

static int s_HNAP12_WLan_Set_SSID(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                  char* pszPrefix)
{
    char* pszSSID;
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_State, pszPrefix, pszBuffer, sizeof(pszBuffer));

    if ((pszSSID = HDK_XML_Get_String(pStruct, HNAP12_Element_PN_SSID)) != 0 &&
        (strlen(pszSSID) > 0 || strcmp(pszBuffer, "up") != 0) &&
        strlen(pszSSID) <= 32)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SSID, pszPrefix, pszSSID);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_SSIDBroadcast(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                       char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SSIDBroadcast, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HDK_XML_Set_Bool(pStruct, HNAP12_Element_PN_SSIDBroadcast, (strcmp(pszBuffer, "0") != 0));
}

static int s_HNAP12_WLan_Set_SSIDBroadcast(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                           char* pszPrefix)
{
    int* pfBroadcast;

    if ((pfBroadcast = HDK_XML_Get_Bool(pStruct, HNAP12_Element_PN_SSIDBroadcast)) != 0)
    {
        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SSIDBroadcast, pszPrefix, *pfBroadcast ? "1" : "0");
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_ChannelWidth(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                      char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    int iChannelWidth = 0;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadioBand, pszPrefix, pszBuffer, sizeof(pszBuffer));

    if (strcmp(pszBuffer, "auto") == 0)
    {
        iChannelWidth = 0;
    }
    else if (strcmp(pszBuffer, "standard") == 0)
    {
        iChannelWidth = 20;
    }
    else if (strcmp(pszBuffer, "wide") == 0)
    {
        iChannelWidth = 40;
    }

    return HDK_XML_Set_Int(pStruct, HNAP12_Element_PN_ChannelWidth, iChannelWidth);
}

static int s_HNAP12_WLan_Set_ChannelWidth(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                          char* pszPrefix)
{
    int* piChannelWidth;

    if ((piChannelWidth = HDK_XML_Get_Int(pStruct, HNAP12_Element_PN_ChannelWidth)) != 0)
    {
        if (*piChannelWidth == 0)
        {
            return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadioBand, pszPrefix, "auto");
        }
        else if (*piChannelWidth == 20)
        {
            return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadioBand, pszPrefix, "standard");
        }
        else if (*piChannelWidth == 40)
        {
            return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadioBand, pszPrefix, "wide");
        }
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_Channel(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                 char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Channel, pszPrefix, pszBuffer, sizeof(pszBuffer));

    return HDK_XML_Set_Int(pStruct, HNAP12_Element_PN_Channel,
                           (strcmp(pszBuffer, "auto") == 0 ? 0 : atoi(pszBuffer)));
}

static int s_HNAP12_WLan_Set_Channel(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                     char* pszPrefix)
{
    char pszChannel[12] = {'\0'};
    int* piChannel;
    errno_t safec_rc = -1;

    if ((piChannel = HDK_XML_Get_Int(pStruct, HNAP12_Element_PN_Channel)) != 0)
    {
        if (*piChannel == 0)
        {
            safec_rc = strcpy_s(pszChannel, sizeof(pszChannel),"auto");
            if(safec_rc != EOK)
            {
               ERR_CHK(safec_rc);
               return 0;
            }
        }
        else
        {
            safec_rc = sprintf_s(pszChannel, sizeof(pszChannel),"%d", *piChannel);
            if(safec_rc < EOK)
            {
               ERR_CHK(safec_rc);
               return 0;
            }
        }

        return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Channel, pszPrefix, pszChannel);
    }

    return 0;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_SecondaryChannel(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                          char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    int iSecChannel = 0;

    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadioBand, pszPrefix, pszBuffer, sizeof(pszBuffer));

    if (strcmp(pszBuffer, "auto") == 0 || strcmp(pszBuffer, "wide") == 0)
    {
        Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Channel, pszPrefix, pszBuffer, sizeof(pszBuffer));

        iSecChannel = atoi(pszBuffer);

        Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SideBand,
                        pszPrefix, pszBuffer, sizeof(pszBuffer));

        if (iSecChannel != 0)
        {
            if (strcmp(pszBuffer, "lower") == 0)
            {
                iSecChannel -= 2;
            }
            else
            {
                iSecChannel += 2;
            }
        }
    }

    return HDK_XML_Set_Int(pStruct, HNAP12_Element_PN_SecondaryChannel, iSecChannel);
}

static int s_HNAP12_WLan_Set_SecondaryChannel(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                              char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    int iChannel;
    int iChannelWidth;
    int* piSecChannel;

    /* Get the current channel */
    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_Channel, pszPrefix, pszBuffer, sizeof(pszBuffer));
    iChannel = (strcmp(pszBuffer, "auto") == 0 ? 0 : atoi(pszBuffer));

    /* Get the current channel width */
    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_RadioBand, pszPrefix, pszBuffer, sizeof(pszBuffer));
    iChannelWidth = (strcmp(pszBuffer, "auto") == 0 ? 0 :
                     (strcmp(pszBuffer, "standard") == 0 ? 20 : 40));

    if ((piSecChannel = HDK_XML_Get_Int(pStruct, HNAP12_Element_PN_SecondaryChannel)) != 0)
    {
        /* Only set the secondary channel for auto and wide channel width */
        if (iChannelWidth == 0 || iChannelWidth == 40)
        {
            return Utopia_SetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_SideBand, pszPrefix,
                                   (*piSecChannel > iChannel ? "upper" : "lower"));
        }
        else
        {
            return 1;
        }
    }

    return 1;
}

static HDK_XML_Member* s_HNAP12_WLan_Get_QoS(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                             char* pszPrefix)
{
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WLAN_WMMSupport, pszBuffer, sizeof(pszBuffer));

    return HDK_XML_Set_Bool(pStruct, HNAP12_Element_PN_QoS, (strcmp(pszBuffer, "enabled") == 0));

    /* Unused variables */
    (void) pszPrefix;
}

/* Helper function for populating WLan ADI values into WLan*Info arrays */
#define WLan_ADIGet(pMethodCtx, pStruct, element, wlanElement)                \
    s_HNAP12_WLanInfo_Array_ADIGet(pMethodCtx, pStruct, element,              \
                                   HNAP12_Element_PN_WLan##wlanElement##Info, \
                                   s_HNAP12_WLan_Get_##wlanElement)

static HDK_XML_Member* s_HNAP12_WLanInfo_Array_ADIGet(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                                      HDK_XML_Element element, HDK_XML_Element arrayElement,
                                                      HNAP12_WLan_GetFn pfnWLanGet)
{
    HDK_XML_Struct* psWLanInfos;

    /* Iterate over the radios obtaining each value */
    if ((psWLanInfos = HDK_XML_Set_Struct(pStruct, element)) != 0)
    {
        unsigned int ixRadio;
        for (ixRadio = 0; ixRadio < WIFI_RADIO_NUM; ++ixRadio)
        {
            HDK_XML_Struct* psTemp;
            if ((psTemp = HDK_XML_Append_Struct(psWLanInfos, arrayElement)) != 0)
            {
                /* Set the RadioID key element */
                HDK_XML_Set_String(psTemp, HNAP12_Element_PN_RadioID, WIFI_RADIO_ID(ixRadio));

                /* Call the WLan get function pointer to get the ADI value */
                pfnWLanGet(pMethodCtx, psTemp, WIFI_RADIO_PREFIX(ixRadio));
            }
        }
    }

    return (HDK_XML_Member*)psWLanInfos;
}

/* Helper function for pulling WLan ADI value out of WLan*Info array and setting */
#define WLan_ADISet(pMethodCtx, pStruct, element, wlanElement)                \
    s_HNAP12_WLanInfo_Array_ADISet(pMethodCtx, pStruct, element,              \
                                   s_HNAP12_WLan_Set_##wlanElement)

static int s_HNAP12_WLanInfo_Array_ADISet(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct,
                                          HDK_XML_Element element, HNAP12_WLan_SetFn pfnWLanSet)
{
    int fSuccess = 1;
    HDK_XML_Struct* psWLanInfos;

    if ((psWLanInfos = HDK_XML_Get_Struct(pStruct, element)) != 0)
    {
        HDK_XML_Member* pmWLanInfo = 0;

        /* Iterate over the wlan infos, setting each one */
        for (pmWLanInfo = psWLanInfos->pHead; fSuccess && pmWLanInfo; pmWLanInfo = pmWLanInfo->pNext)
        {
            char* pszRadioID;

            if ((pszRadioID = HDK_XML_Get_String(HDK_XML_GetMember_Struct(pmWLanInfo), HNAP12_Element_PN_RadioID)) != 0)
            {
                /* Call the WLan set function pointer to set the ADI value */
                fSuccess &= pfnWLanSet(pMethodCtx, HDK_XML_GetMember_Struct(pmWLanInfo), s_RadioIDToPrefix(pszRadioID));
            }
        }
    }
    return fSuccess;
}

/* Device ADIGet implementation */
static HDK_XML_Member* s_HNAP12_SRV_Device_ADIGet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                                                  HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Member* pMember = 0;

    /* Variables used by a number of cases, declared here to save typing */
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    HDK_XML_IPAddress ipAddress = {0,0,0,0};
    errno_t safec_rc = -1;

    switch(value)
    {
        case HNAP12_ADI_PN_AdminPassword:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_HTTP_AdminPassword, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_IsAdminPasswordDefault:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_HTTP_AdminPassword, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, strcmp(crypt(UTOPIA_DEFAULT_ADMIN_PASSWD,
                                                                          pszBuffer), pszBuffer) == 0);
            }
            break;

        case HNAP12_ADI_PN_UsernameSupported:
            {
                /* RRP does not allow the username to be set */
                pMember = HDK_XML_Set_Bool(pStruct, element, 0);
            }
            break;

        case HNAP12_ADI_PN_Username:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_HTTP_AdminUser, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_DeviceType:
            {
                HNAP12_Enum_PN_DeviceType eDevType = HNAP12_Enum_PN_DeviceType__UNKNOWN__;
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DeviceType, pszBuffer, sizeof(pszBuffer));

                if (strcmp(pszBuffer, "Gateway") == 0)
                {
                    eDevType = HNAP12_Enum_PN_DeviceType_Gateway;
                }
                else if (strcmp(pszBuffer, "GatewayWithWiFi") == 0)
                {
                    eDevType = HNAP12_Enum_PN_DeviceType_GatewayWithWiFi;
                }
                else if (strcmp(pszBuffer, "WiFiAccessPoint") == 0)
                {
                    eDevType = HNAP12_Enum_PN_DeviceType_WiFiAccessPoint;
                }

                pMember = HNAP12_Set_PN_DeviceType(pStruct, element, eDevType);
            }
            break;

        case HNAP12_ADI_PN_DeviceName:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_HostName, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_VendorName:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_VendorName, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_ModelDescription:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_ModelDescription, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_ModelName:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_ModelName, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_ModelRevision:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_ModelRevision, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_FirmwareVersion:
            {
                char pszVersion[UTOPIA_BUF_SIZE] = {'\0'};
                int x,y,z;

                /* Format as w.x.yy Build zzz, where w.x is hardware revision */
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_ModelRevision, pszBuffer, sizeof(pszBuffer));
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_FirmwareVersion, pszVersion, sizeof(pszBuffer));

                if (sscanf(pszVersion, "%d.%d.%d", &x, &y, &z) == 3 )
                {
                    safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%s.%02d build %d%d", pszBuffer, x, y, z);
                    if(safec_rc < EOK)
                    {
                       ERR_CHK(safec_rc);
                    }
                }
                if(safec_rc > 0 )
                {
                    pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_PresentationURL:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_PresentationURL, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_SubDeviceURLs:
            {
                pMember = (HDK_XML_Member*)HDK_XML_Set_Struct(pStruct, element);
            }
            break;

        case HNAP12_ADI_PN_TaskExtensions:
            {
                HDK_XML_Struct* psTasks;
                HDK_XML_Struct* psTaskExtension;

                if ((psTasks = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    /* Three task extensions for the RRP */
                    if ((psTaskExtension = HDK_XML_Append_Struct(psTasks, HNAP12_Element_PN_TaskExtension)) != 0)
                    {
                        HDK_XML_Set_String(psTaskExtension, HNAP12_Element_PN_Name, "Status Page");
                        HDK_XML_Set_String(psTaskExtension, HNAP12_Element_PN_URL, "/Status_Router.asp");
                        HNAP12_Set_PN_TaskExtType(psTaskExtension, HNAP12_Element_PN_Type, HNAP12_Enum_PN_TaskExtType_Browser);
                    }
                    if ((psTaskExtension = HDK_XML_Append_Struct(psTasks, HNAP12_Element_PN_TaskExtension)) != 0)
                    {
                        HDK_XML_Set_String(psTaskExtension, HNAP12_Element_PN_Name, "Basic Wireless Settings");
                        HDK_XML_Set_String(psTaskExtension, HNAP12_Element_PN_URL, "/Wireless_Basic.asp");
                        HNAP12_Set_PN_TaskExtType(psTaskExtension, HNAP12_Element_PN_Type, HNAP12_Enum_PN_TaskExtType_Browser);
                    }
                    if ((psTaskExtension = HDK_XML_Append_Struct(psTasks, HNAP12_Element_PN_TaskExtension)) != 0)
                    {
                        HDK_XML_Set_String(psTaskExtension, HNAP12_Element_PN_Name, "UT610N");
                        HDK_XML_Set_String(psTaskExtension, HNAP12_Element_PN_URL, "http://www.linksys.com");
                        HNAP12_Set_PN_TaskExtType(psTaskExtension, HNAP12_Element_PN_Type, HNAP12_Enum_PN_TaskExtType_Browser);
                    }
                    pMember = (HDK_XML_Member*)psTasks;
                }
            }
            break;

        case HNAP12_ADI_PN_SerialNumber:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_SerialNumber, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_TimeZoneSupported:
            {
                /* RRP does allow the timezone to be set */
                pMember = HDK_XML_Set_Bool(pStruct, element, 1);
            }
            break;

        case HNAP12_ADI_PN_TimeZone:
            {
                unsigned int i;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_TZ, pszBuffer, sizeof(pszBuffer));

                for (i = 0; i < TIMEZONE_NUM; ++i)
                {
                    /* Look for the matching timezone */
                    if (strstr(pszBuffer, g_TimezoneMap[i].pszTZ) != 0)
                    {
                        pMember = HDK_XML_Set_String(pStruct, element, g_TimezoneMap[i].pszUTC);
                        break;
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_AutoAdjustDST:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_AutoDST, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, (strcmp(pszBuffer, "0") != 0));
            }
            break;

        case HNAP12_ADI_PN_Locale:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Locale, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_SupportedLocales:
            {
                HDK_XML_Struct* psLocales;

                if ((psLocales = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    char** ppszLocale;
                    char** ppszEnd = g_ppszLocales + sizeof(g_ppszLocales) / sizeof(*g_ppszLocales);

                    for (ppszLocale = g_ppszLocales; ppszLocale != ppszEnd; ++ppszLocale)
                    {
                        HDK_XML_Append_String(psLocales, HNAP12_Element_PN_string, *ppszLocale);
                    }
                }
                    pMember = (HDK_XML_Member*)psLocales;
            }
            break;

        case HNAP12_ADI_PN_SSL:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_HTTPSAccess, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, (strcmp(pszBuffer, "1") == 0));
            }
            break;

        case HNAP12_ADI_PN_IsDeviceReady:
            {
                /* Acquire write lock, which will allow all current reads or current write to finish */
                if (UtopiaRWLock_ReadLock(&pUTCtx(pMethodCtx)->rwLock) != 0)
                {
                    /* Check to make sure the lan is not restarting */
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_Restarting, pszBuffer, sizeof(pszBuffer));
                    pMember = HDK_XML_Set_Bool(pStruct, element, strcmp(pszBuffer, "1") != 0);
                }
            }
            break;

        case HNAP12_ADI_PN_RebootTrigger:
            {
                /* Acquire write lock, which will allow all current reads or current write to finish */
                if (UtopiaRWLock_WriteLock(&pUTCtx(pMethodCtx)->rwLock) != 0)
                {
                    /* Set the reboot event */
                    Utopia_SetEvent(pUTCtx(pMethodCtx), Utopia_Event_Reboot);
                    pMember = (HDK_XML_Member*)1;
                }
            }
            break;

        case HNAP12_ADI_PN_FirmwareDate:
            {
                int iYear = 0;
                int iMonth = 0;
                int iMDay = 0;
                int iHour = 0;
                int iMin = 0;
                int iSec = 0;
                char pszMonth[6] = {'\0'};
                struct tm sTime;

                /* Scan __DATE__ for year, month, and day */
                if ((sscanf(__DATE__, "%s %02d %d", pszMonth, &iMDay, &iYear) == 3) &&
                    (iMDay <= 31 && iMDay > 0))
                {
                    unsigned int i;

                    if (iYear > 1900)
                    {
                        iYear -= 1900;
                    }

                    for (i = 0; i < sizeof(g_ppszMonthAbbrev)/sizeof(*g_ppszMonthAbbrev); ++i)
                    {
                        if (strncasecmp(pszMonth, g_ppszMonthAbbrev[i], 3) == 0)
                        {
                            iMonth = i;
                            break;
                        }
                    }
                }

                /* Scan __TIME__ for hour, min, and sec */
                sscanf(__TIME__, "%02d:%02d:%02d", &iHour, &iMin, &iSec);

                /* Populate time struct */
                sTime.tm_year = iYear;
                sTime.tm_mon = iMonth;
                sTime.tm_mday = iMDay;
                sTime.tm_hour = iHour;
                sTime.tm_min = iMin;
                sTime.tm_sec = iSec;
                sTime.tm_isdst = -1;

#ifndef HNAP_DEBUG
                pMember = HDK_XML_Set_DateTime(pStruct, element, mktime(&sTime));
#else
                pMember = HDK_XML_Set_DateTime(pStruct, element, 0);
#endif
            }
            break;

        case HNAP12_ADI_PN_UpdateMethods:
            {
                HDK_XML_Struct* psMethods = HDK_XML_Set_Struct(pStruct, element);

                if (psMethods)
                {
                    HNAP12_Append_PN_UpdateMethod(psMethods, HNAP12_Element_PN_string, HNAP12_Enum_PN_UpdateMethod_HNAP_UPLOAD);
                }

                pMember = (HDK_XML_Member*)psMethods;
            }
            break;

        case HNAP12_ADI_PN_ClientStats:
            {
                HDK_XML_Member* pmConnectedClient;
                HDK_XML_Struct* psConnectedClients;
                HDK_XML_Struct* psClientStats;
                HDK_XML_Struct sTemp;

                if ((psClientStats = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    /* Get the connected clients */
                    HDK_XML_Struct_Init(&sTemp);
                    s_HNAP12_SRV_Device_ADIGet(pMethodCtx, HNAP12_ADI_PN_ConnectedClients, &sTemp, HNAP12_Element_PN_ConnectedClients);
                    if ((psConnectedClients = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_ConnectedClients)) == 0)
                    {
                        HDK_XML_Struct_Free(&sTemp);
                        break;
                    }

                    /* Iterate over the connected clients and add them to the client stat struct */
                    for (pmConnectedClient = psConnectedClients->pHead; pmConnectedClient; pmConnectedClient = pmConnectedClient->pNext)
                    {
                        int iRssi = 0;
                        int iLinkSpeed = 0;
                        int* pfWireless;
                        HNAP12_Enum_PN_LANConnection* pLanConn;
                        HDK_XML_MACAddress* pMAC;
                        HDK_XML_Struct* psClientStat;

                        /* Get the connected client data we need */
                        if ((pLanConn = HNAP12_Get_PN_LANConnection(HDK_XML_GetMember_Struct(pmConnectedClient), HNAP12_Element_PN_PortName)) == 0 ||
                            (pfWireless = HDK_XML_Get_Bool(HDK_XML_GetMember_Struct(pmConnectedClient), HNAP12_Element_PN_Wireless)) == 0 ||
                            (pMAC = HDK_XML_Get_MACAddress(HDK_XML_GetMember_Struct(pmConnectedClient), HNAP12_Element_PN_MacAddress)) == 0)
                        {
                            continue;
                        }

                        /* If this is a wireless connection, we need to compute signal strength */
                        if (*pfWireless)
#ifndef HNAP_DEBUG
                        {
                            unsigned int ixRadio;
                            /* The client may be connected to any wifi interface, so loop over them */
                            for (ixRadio = 0; ixRadio < WIFI_RADIO_NUM; ++ixRadio)
                            {
                                char pszCmd[48] = {'\0'};
                                FILE* fdWlRssi;

                                /* Format the wl command */
                                safec_rc = sprintf_s(pszCmd, sizeof(pszCmd),"wl -i %s rssi ", WIFI_RADIO_ETHIF(ixRadio));
                                if(safec_rc < EOK)
                                {
                                   ERR_CHK(safec_rc);
                                   continue;
                                }
                                HDK_Util_MACToStr(pszCmd + strlen(pszCmd), pMAC);

                                /* Spawn a process with the wl command */
                                if ((fdWlRssi = popen(pszCmd, "r")) == 0)
                                {
                                    continue;
                                }

                                if (fscanf(fdWlRssi, "%d", &iRssi) == 1)
                                {
                                    /*
                                     * Convert dBm to a percentage with following formula:
                                     *
                                     *    % = 100 - 80 * (MAX_RSSI - iRssi) / (MAX_RSSI - MIN_RSSI),
                                     *
                                     *    where MAX_RSSI = -40, MIN_RSSI = -85
                                     *
                                     */
                                    iRssi = 100 - 80 * (-40 - iRssi) / (-40 - (-85));
                                    iRssi = (iRssi < 0 ? 0 : (iRssi > 100 ? 100 : iRssi));
                                    pclose(fdWlRssi);
                                    break;
                                }
                                pclose(fdWlRssi);
                            }
                        }
#endif

                        /* Now compute the link speed */
                        switch (*pLanConn)
                        {
                            case HNAP12_Enum_PN_LANConnection_LAN:
                                iLinkSpeed = 1000;
                                break;

                            case HNAP12_Enum_PN_LANConnection_WLAN_802_11b:
                                iLinkSpeed = 11;
                                break;

                            case HNAP12_Enum_PN_LANConnection_WLAN_802_11a:
                            case HNAP12_Enum_PN_LANConnection_WLAN_802_11g:
                                iLinkSpeed = 54;
                                break;

                            case HNAP12_Enum_PN_LANConnection_WLAN_802_11n:
                                iLinkSpeed = 130;
                                break;

                            default:
                                break;
                        }

                        /* Append the client stat */
                        if ((psClientStat = HDK_XML_Append_Struct(psClientStats, HNAP12_Element_PN_ClientStat)) != 0)
                        {
                            HDK_XML_Set_MACAddress(psClientStat, HNAP12_Element_PN_MacAddress, pMAC);
                            HDK_XML_Set_Bool(psClientStat, HNAP12_Element_PN_Wireless, *pfWireless);
                            HDK_XML_Set_Int(psClientStat, HNAP12_Element_PN_LinkSpeedIn, iLinkSpeed);
                            HDK_XML_Set_Int(psClientStat, HNAP12_Element_PN_LinkSpeedOut, iLinkSpeed);
                            HDK_XML_Set_Int(psClientStat, HNAP12_Element_PN_SignalStrength, iRssi);
                        }
                    }
                    HDK_XML_Struct_Free(&sTemp);
                    pMember = (HDK_XML_Member*)psClientStats;
                }
            }
            break;

        case HNAP12_ADI_PN_ConnectedClients:
            {
                HDK_XML_Struct* psClients1;
                HDK_XML_Struct* psClients2;
                HDK_XML_Struct sTemp;

                /* Initialize the temporary struct */
                HDK_XML_Struct_Init(&sTemp);

                if (s_GetConnectedClients(pMethodCtx, &sTemp, element) != 0 &&
                    (psClients1 = HDK_XML_Get_Struct(&sTemp, element)) != 0 &&
                    (psClients2 = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    HDK_XML_Member* pmClient;

                    for (pmClient = psClients1->pHead; pmClient; pmClient = pmClient->pNext)
                    {
                        HDK_XML_Struct* psClient1 = HDK_XML_GetMember_Struct(pmClient);
                        HDK_XML_Struct* psClient2;

                        if ((psClient2 = HDK_XML_Append_Struct(psClients2, HNAP12_Element_PN_ConnectedClient)) != 0)
                        {
                            HDK_XML_Set_DateTime(psClient2, HNAP12_Element_PN_ConnectTime, *HDK_XML_Get_DateTime(psClient1, HNAP12_Element_PN_ConnectTime));
                            HDK_XML_Set_MACAddress(psClient2, HNAP12_Element_PN_MacAddress, HDK_XML_Get_MACAddress(psClient1, HNAP12_Element_PN_MacAddress));
                            HDK_XML_Set_String(psClient2, HNAP12_Element_PN_DeviceName, HDK_XML_Get_String(psClient1, HNAP12_Element_PN_DeviceName));
                            HDK_XML_Set_Bool(psClient2, HNAP12_Element_PN_Active, *HDK_XML_Get_Bool(psClient1, HNAP12_Element_PN_Active));
                            HNAP12_Set_PN_LANConnection(psClient2, HNAP12_Element_PN_PortName, *HNAP12_Get_PN_LANConnection(psClient1, HNAP12_Element_PN_PortName));
                            HDK_XML_Set_Bool(psClient2, HNAP12_Element_PN_Wireless, *HDK_XML_Get_Bool(psClient1, HNAP12_Element_PN_Wireless));
                        }
                    }
                    pMember = (HDK_XML_Member*)psClients2;
                }

                /* Free the temporary struct */
                HDK_XML_Struct_Free(&sTemp);
            }
            break;

        case HNAP12_ADI_PN_MFEnabled:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WLAN_AccessRestriction, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, (strcmp(pszBuffer, "deny") == 0 ||
                                                              strcmp(pszBuffer, "permit") == 0));
            }
            break;

        case HNAP12_ADI_PN_MFIsAllowList:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WLAN_AccessRestriction, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, (strcmp(pszBuffer, "permit") == 0));
            }
            break;

        case HNAP12_ADI_PN_MFMACList:
            {
                char* pszMACList;
                char* pszEnd = 0;
                HDK_XML_MACAddress macAddr;
                HDK_XML_Struct* psMacInfo;
                HDK_XML_Struct* psMacList;

                if ((psMacList = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WLAN_MACFilter, pszBuffer, sizeof(pszBuffer));
                    pszMACList = pszBuffer;

                    /*
                     * Parse the following mac filters string into mac filters struct:
                     *
                     * "00:00:00:00:00:01 00:00:00:00:00:02 00:00:00:00:00:03"
                     *
                     */
                    while ((pszEnd = strchr(pszMACList, ' ')))
                    {
                        char pszMacAddr[18] = {'\0'};

                        strncpy(pszMacAddr, pszMACList, (size_t)(pszEnd - pszMACList));
                        if (HDK_Util_StrToMAC(&macAddr, pszMacAddr) &&
                            (psMacInfo = HDK_XML_Append_Struct(psMacList, HNAP12_Element_PN_MACInfo)) != 0)
                        {
                            HDK_XML_Set_MACAddress(psMacInfo, HNAP12_Element_PN_MacAddress, &macAddr);
                            HDK_XML_Set_String(psMacInfo, HNAP12_Element_PN_DeviceName, "");
                        }

                        /* Move begin pointer ahead of ' ' */
                        pszMACList = pszEnd + 1;
                    }

                    /* Copy the last element */
                    if (*pszMACList &&
                        HDK_Util_StrToMAC(&macAddr, pszMACList) &&
                        (psMacInfo = HDK_XML_Append_Struct(psMacList, HNAP12_Element_PN_MACInfo)) != 0)
                    {
                        HDK_XML_Set_MACAddress(psMacInfo, HNAP12_Element_PN_MacAddress, &macAddr);
                        HDK_XML_Set_String(psMacInfo, HNAP12_Element_PN_DeviceName, "");
                    }

                    pMember = (HDK_XML_Member*)psMacList;
                }
            }
            break;

        case HNAP12_ADI_PN_DeviceNetworkStats:
            {
                char pszLine[256];
                FILE* fd;
                HDK_XML_Struct* ppsNetworkStats[WIFI_RADIO_NUM] = {0};
                HDK_XML_Struct* psStats;

                if ((psStats = HDK_XML_Set_Struct(pStruct, element)) != 0 &&
                    (fd = fopen(UTOPIA_NW_DEVICE_FILE, "r")) != 0)
                {
                    /*
                     * Parse the network stats from the network device file:
                     *   'Inter-|Receive                                                   |Transmit'
                     *   'face  |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed'
                     *   'eth0:  161488    1423    0    0    0     0          0       154   144254     975    0    0    0     0       0          0'
                     */
                    while (fgets(pszLine, sizeof(pszLine) - 1, fd) != 0)
                    {
                        char* pszColon;
                        char* pszInterface = pszLine;
                        HNAP12_Enum_PN_LANConnection lanConn = HNAP12_Enum_PN_LANConnection__UNKNOWN__;
                        HDK_XML_Struct* psNetworkStats = 0;
                        long long int liPackRec;
                        long long int liPackSent;
                        long long int liByteRec;
                        long long int liByteSent;
                        long long int liUnused;

                        /* Parse the interface name */
                        if ((pszColon = strchr(pszLine, ':')) == 0)
                        {
                            continue;
                        }

                        /* Replace the ':' with '\0' */
                        *pszColon++ = '\0';

                        if (sscanf(pszColon, "%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
                                   &liByteRec, &liPackRec, &liUnused, &liUnused, &liUnused, &liUnused, &liUnused, &liUnused,
                                   &liByteSent, &liPackSent, &liUnused, &liUnused, &liUnused, &liUnused, &liUnused, &liUnused) != 16)
                        {
                            continue;
                        }

                        /* Skip leading whitespace */
                        for ( ; *pszInterface == ' ' && *pszInterface != '\0'; ++pszInterface);

                        if (strcmp(pszInterface, UTOPIA_LAN_INTERFACE) == 0)
                        {
                            lanConn = HNAP12_Enum_PN_LANConnection_LAN;
                        }
                        else if (strcmp(pszInterface, UTOPIA_WAN_INTERFACE) == 0)
                        {
                            lanConn = HNAP12_Enum_PN_LANConnection_WAN;
                        }

                        /* LAN/WAN Interfaces */
                        if (lanConn != HNAP12_Enum_PN_LANConnection__UNKNOWN__)
                        {
                            if ((psNetworkStats = HDK_XML_Append_Struct(psStats, HNAP12_Element_PN_NetworkStats)) != 0)
                            {
                                HNAP12_Set_PN_LANConnection(psNetworkStats, HNAP12_Element_PN_PortName, lanConn);
                                HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_PacketsReceived, liPackRec);
                                HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_PacketsSent, liPackSent);
                                HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_BytesReceived, liByteRec);
                                HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_BytesSent, liByteSent);
                            }
                        }
                        /* WLAN Interfaces */
                        else
                        {
                            HNAP12_Enum_PN_LANConnection* pLanConn;
                            HDK_XML_Struct* psNetworkStats;
                            unsigned int ixRadio, ixStats;

                            /* Loop over the radios */
                            for (ixRadio = 0; ixRadio < WIFI_RADIO_NUM; ++ixRadio)
                            {
                                if (strcmp(pszInterface, WIFI_RADIO_ETHIF(ixRadio)) == 0)
                                {
                                    Utopia_GetNamed(pUTCtx(pMethodCtx), UtopiaValue_WLAN_NetworkMode,
                                                    WIFI_RADIO_PREFIX(ixRadio), pszBuffer, sizeof(pszBuffer));

                                    lanConn = s_WiFiModeToLANConnection((HNAP12_Enum_PN_WiFiMode)s_StrToEnum(g_WiFiModeMap, pszBuffer));
                                    if (lanConn == HNAP12_Enum_PN_LANConnection__UNKNOWN__)
                                    {
                                        continue;
                                    }

                                    /* Find the network stats to update */
                                    psNetworkStats = 0;
                                    for (ixStats = 0; ppsNetworkStats[ixStats] && ixStats < WIFI_RADIO_NUM; ++ixStats)
                                    {
                                        pLanConn = HNAP12_Get_PN_LANConnection(ppsNetworkStats[ixStats], HNAP12_Element_PN_PortName);

                                        if (*pLanConn == lanConn)
                                        {
                                            long long int* pliPackRec = HDK_XML_Get_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_PacketsReceived);
                                            long long int* pliPackSent = HDK_XML_Get_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_PacketsSent);
                                            long long int* pliByteRec = HDK_XML_Get_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_BytesReceived);
                                            long long int* pliByteSent = HDK_XML_Get_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_BytesSent);

                                            HDK_XML_Set_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_PacketsReceived, *pliPackRec + liPackRec);
                                            HDK_XML_Set_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_PacketsSent, *pliPackSent + liPackSent);
                                            HDK_XML_Set_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_BytesReceived, *pliByteRec + liByteRec);
                                            HDK_XML_Set_Long(ppsNetworkStats[ixStats], HNAP12_Element_PN_BytesSent, *pliByteSent + liByteSent);

                                            psNetworkStats = ppsNetworkStats[ixStats];
                                            break;
                                        }
                                    }

                                    /* Append the network stats if this is a new lan connection */
                                    if (psNetworkStats == 0 &&
                                        (psNetworkStats = HDK_XML_Append_Struct(psStats, HNAP12_Element_PN_NetworkStats)) != 0)
                                    {
                                        HNAP12_Set_PN_LANConnection(psNetworkStats, HNAP12_Element_PN_PortName, lanConn);
                                        HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_PacketsReceived, liPackRec);
                                        HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_PacketsSent, liPackSent);
                                        HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_BytesReceived, liByteRec);
                                        HDK_XML_Set_Long(psNetworkStats, HNAP12_Element_PN_BytesSent, liByteSent);
                                        ppsNetworkStats[ixStats] = psNetworkStats;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    fclose(fd);
                    pMember = (HDK_XML_Member*)psStats;
                }
            }
            break;

        case HNAP12_ADI_PN_PortMappings:
            {
                HDK_XML_Struct* psPortMappings;
                int iPMCount = 0;
                int i;

                if ((psPortMappings = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_IPAddr, pszBuffer, sizeof(pszBuffer));
                    HDK_Util_StrToIP(&ipAddress, pszBuffer);

                    /* Get the port mapping count */
                    if (Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Firewall_SPFCount, pszBuffer, sizeof(pszBuffer)) != 0)
                    {
                        iPMCount = atoi(pszBuffer);
                    }

                    /* Loop over port mappings */
                    for (i = 1; i <= iPMCount; ++i)
                    {
                        HNAP12_Enum_PN_IPProtocol eIPProtocol;
                        HDK_XML_Struct* psMapping;
                        int fBothProt = 0;
                        int iExtPort;
                        int iIntPort;
                        int iIpOct;

                        /* If it's not enabled, skip to the next one */
                        pszBuffer[0] = '\0';
                        Utopia_GetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Enabled, i, pszBuffer, sizeof(pszBuffer));
                        if (strcmp(pszBuffer, "1") != 0)
                        {
                            continue;
                        }

                        pszBuffer[0] = '\0';
                        Utopia_GetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Protocol, i, pszBuffer, sizeof(pszBuffer));
                        if (strcmp(pszBuffer, "tcp") == 0)
                        {
                            eIPProtocol = HNAP12_Enum_PN_IPProtocol_TCP;
                        }
                        else if (strcmp(pszBuffer, "udp") == 0)
                        {
                            eIPProtocol = HNAP12_Enum_PN_IPProtocol_UDP;
                        }
                        else if (strcmp(pszBuffer, "both") == 0)
                        {
                            eIPProtocol = HNAP12_Enum_PN_IPProtocol_TCP;
                            fBothProt = 1;
                        }
                        else
                        {
                            continue;
                        }

                        pszBuffer[0] = '\0';
                        Utopia_GetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_ExternalPort, i, pszBuffer, sizeof(pszBuffer));
                        iExtPort = atoi(pszBuffer);

                        pszBuffer[0] = '\0';
                        Utopia_GetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_InternalPort, i, pszBuffer, sizeof(pszBuffer));
                        iIntPort = atoi(pszBuffer);

                        pszBuffer[0] = '\0';
                        Utopia_GetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_ToIp, i, pszBuffer, sizeof(pszBuffer));
                        iIpOct = atoi(pszBuffer);

                        pszBuffer[0] = '\0';
                        Utopia_GetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Name, i, pszBuffer, sizeof(pszBuffer));

                        if ((psMapping = HDK_XML_Append_Struct(psPortMappings, HNAP12_Element_PN_PortMapping)) != 0)
                        {
                            ipAddress.d = (unsigned int)iIpOct;

                            HDK_XML_Set_String(psMapping, HNAP12_Element_PN_PortMappingDescription, pszBuffer);
                            HDK_XML_Set_IPAddress(psMapping, HNAP12_Element_PN_InternalClient, &ipAddress);
                            HNAP12_Set_PN_IPProtocol(psMapping, HNAP12_Element_PN_PortMappingProtocol, eIPProtocol);
                            HDK_XML_Set_Int(psMapping, HNAP12_Element_PN_ExternalPort, iExtPort);
                            HDK_XML_Set_Int(psMapping, HNAP12_Element_PN_InternalPort, iIntPort);
                        }

                        if (fBothProt &&
                            (psMapping = HDK_XML_Append_Struct(psPortMappings, HNAP12_Element_PN_PortMapping)) != 0)
                        {
                            HDK_XML_Set_String(psMapping, HNAP12_Element_PN_PortMappingDescription, pszBuffer);
                            HDK_XML_Set_IPAddress(psMapping, HNAP12_Element_PN_InternalClient, &ipAddress);
                            HNAP12_Set_PN_IPProtocol(psMapping, HNAP12_Element_PN_PortMappingProtocol, HNAP12_Enum_PN_IPProtocol_UDP);
                            HDK_XML_Set_Int(psMapping, HNAP12_Element_PN_ExternalPort, iExtPort);
                            HDK_XML_Set_Int(psMapping, HNAP12_Element_PN_InternalPort, iIntPort);
                        }
                    }
                    pMember = (HDK_XML_Member*)psPortMappings;
                }
            }
            break;

        case HNAP12_ADI_PN_LanIPAddress:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_IPAddr, pszBuffer, sizeof(pszBuffer));
                HDK_Util_StrToIP(&ipAddress, pszBuffer);

                pMember = HDK_XML_Set_IPAddress(pStruct, element, &ipAddress);
            }
            break;

        case HNAP12_ADI_PN_LanSubnetMask:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_Netmask, pszBuffer, sizeof(pszBuffer));
                HDK_Util_StrToIP(&ipAddress, pszBuffer);

                pMember = HDK_XML_Set_IPAddress(pStruct, element, &ipAddress);
            }
            break;

        case HNAP12_ADI_PN_DHCPServerEnabled:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_ServerEnabled, pszBuffer, sizeof(pszBuffer));

                pMember = HDK_XML_Set_Bool(pStruct, element, (strcmp(pszBuffer, "1") == 0));
            }
            break;

        case HNAP12_ADI_PN_DHCPIPAddressFirst:
            {
                int iFirstIPOctet;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_Start, pszBuffer, sizeof(pszBuffer));
                iFirstIPOctet = atoi(pszBuffer);

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_IPAddr, pszBuffer, sizeof(pszBuffer));
                HDK_Util_StrToIP(&ipAddress, pszBuffer);
                ipAddress.d = (unsigned char)iFirstIPOctet;

                pMember = HDK_XML_Set_IPAddress(pStruct, element, &ipAddress);
            }
            break;

        case HNAP12_ADI_PN_DHCPIPAddressLast:
            {
                int iFirstIPOctet;
                int iNumIPs;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_Start, pszBuffer, sizeof(pszBuffer));
                iFirstIPOctet = atoi(pszBuffer);

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_Num, pszBuffer, sizeof(pszBuffer));
                iNumIPs = atoi(pszBuffer);

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_IPAddr, pszBuffer, sizeof(pszBuffer));
                HDK_Util_StrToIP(&ipAddress, pszBuffer);
                ipAddress.d = (unsigned char)(iFirstIPOctet + iNumIPs - 1);

                pMember = HDK_XML_Set_IPAddress(pStruct, element, &ipAddress);
            }
            break;

        case HNAP12_ADI_PN_DHCPLeaseTime:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_LeaseTime, pszBuffer, sizeof(pszBuffer));

                pMember = HDK_XML_Set_Int(pStruct, element, s_LeaseStrToMinutes(pszBuffer));
            }
            break;

        case HNAP12_ADI_PN_DHCPReservationsSupported:
            {
                /* RRP currently allows DHCP reservations */
                pMember = HDK_XML_Set_Bool(pStruct, element, 1);
            }
            break;

        case HNAP12_ADI_PN_DHCPReservations:
            {
                HDK_XML_Struct* psDHCPs;

                if ((psDHCPs = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    int i, iLeaseNum;
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_NumStaticHosts, pszBuffer, sizeof(pszBuffer));
                    iLeaseNum = atoi(pszBuffer);

                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_IPAddr, pszBuffer, sizeof(pszBuffer));
                    HDK_Util_StrToIP(&ipAddress, pszBuffer);

                    for (i = 1; i <= iLeaseNum; ++i)
                    {
                        char* pszComma;
                        char pszMacAddress[20] = {'\0'};
                        HDK_XML_MACAddress macAddress = {0,0,0,0,0,0};
                        HDK_XML_Struct* psDHCP;
                        int iLastIPOctet;

                        Utopia_GetIndexed(pUTCtx(pMethodCtx), UtopiaValue_DHCP_StaticHost, i, pszBuffer, sizeof(pszBuffer));

                        if ((pszComma = strchr(pszBuffer, ',')) == 0)
                        {
                            continue;
                        }

                        strncpy(pszMacAddress, pszBuffer, (size_t)(pszComma - pszBuffer));

                        if (HDK_Util_StrToMAC(&macAddress, pszMacAddress) == 0 ||
                            sscanf(++pszComma, "%d", &iLastIPOctet) != 1)
                        {
                            continue;
                        }

                        if ((psDHCP = HDK_XML_Append_Struct(psDHCPs, HNAP12_Element_PN_DHCPReservation)) != 0)
                        {
                            ipAddress.d = iLastIPOctet;

                            HDK_XML_Set_String(psDHCP, HNAP12_Element_PN_DeviceName, "");
                            HDK_XML_Set_MACAddress(psDHCP, HNAP12_Element_PN_MacAddress, &macAddress);
                            HDK_XML_Set_IPAddress(psDHCP, HNAP12_Element_PN_IPAddress, &ipAddress);
                        }
                    }
                }
                pMember = (HDK_XML_Member*)psDHCPs;
            }
            break;

        case HNAP12_ADI_PN_RemoteManagementSupported:
            {
                pMember = HDK_XML_Set_Bool(pStruct, element, 1);
            }
            break;

        case HNAP12_ADI_PN_ManageRemote:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WANAccess, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, strcmp(pszBuffer, "1") == 0);
            }
            break;

        case HNAP12_ADI_PN_ManageWireless:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WIFIAccess, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, strcmp(pszBuffer, "1") == 0);
            }
            break;

        case HNAP12_ADI_PN_RemotePort:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WANHTTPPort, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Int(pStruct, element, atoi(pszBuffer));
            }
            break;

        case HNAP12_ADI_PN_ManageViaSSLSupported:
            {
                pMember = HDK_XML_Set_Bool(pStruct, element, 1);
            }
            break;

        case HNAP12_ADI_PN_ManageOnlyViaSSL:
            {
                pMember = HDK_XML_Set_Bool(pStruct, element, 0);
            }
            break;

        case HNAP12_ADI_PN_RemoteSSLNeedsSSL:
            {
                pMember = HDK_XML_Set_Bool(pStruct, element, 0);
            }
            break;

        case HNAP12_ADI_PN_RemoteSSL:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WANHTTPSAccess, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, strcmp(pszBuffer, "1") == 0);
            }
            break;

        case HNAP12_ADI_PN_DomainNameChangeSupported:
            {
                /* RRP does allow domain name change SSL */
                pMember = HDK_XML_Set_Bool(pStruct, element, 1);
            }
            break;

        case HNAP12_ADI_PN_DomainName:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_LAN_Domain, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_WiredQoSSupported:
            {
                /* RRP does allow wired QoS */
                pMember = HDK_XML_Set_Bool(pStruct, element, 1);
            }
            break;

        case HNAP12_ADI_PN_WiredQoS:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_QoS_Enable, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_Bool(pStruct, element, (strcmp(pszBuffer, "1") == 0));
            }
            break;

        case HNAP12_ADI_PN_WPSPin:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WPS_DevicePin, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_WLanSecurityEnabled:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, SecurityEnabled);
            }
            break;

        case HNAP12_ADI_PN_WLanType:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, Type);
            }
            break;

        case HNAP12_ADI_PN_WLanEncryption:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, Encryption);
            }
            break;

        case HNAP12_ADI_PN_WLanKey:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, Key);
            }
            break;

        case HNAP12_ADI_PN_WLanKeyRenewal:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, KeyRenewal);
            }
            break;

        case HNAP12_ADI_PN_WLanRadiusIP1:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, RadiusIP1);
            }
            break;

        case HNAP12_ADI_PN_WLanRadiusPort1:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, RadiusPort1);
            }
            break;

        case HNAP12_ADI_PN_WLanRadiusSecret1:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, RadiusSecret1);
            }
            break;

        case HNAP12_ADI_PN_Radius2Supported:
            {
                /* RRP does not support Radius2 values */
                pMember = HDK_XML_Set_Bool(pStruct, element, 0);
            }
            break;

        case HNAP12_ADI_PN_WLanEnabled:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, Enabled);
            }
            break;

        case HNAP12_ADI_PN_WLanMode:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, Mode);
            }
            break;

        case HNAP12_ADI_PN_WLanMacAddress:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, MacAddress);
            }
            break;

        case HNAP12_ADI_PN_WLanSSID:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, SSID);
            }
            break;

        case HNAP12_ADI_PN_WLanSSIDBroadcast:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, SSIDBroadcast);
            }
            break;

        case HNAP12_ADI_PN_WLanChannelWidth:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, ChannelWidth);
            }
            break;

        case HNAP12_ADI_PN_WLanChannel:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, Channel);
            }
            break;

        case HNAP12_ADI_PN_WLanSecondaryChannel:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, SecondaryChannel);
            }
            break;

        case HNAP12_ADI_PN_WLanQoS:
            {
                pMember = WLan_ADIGet(pMethodCtx, pStruct, element, QoS);
            }
            break;

        case HNAP12_ADI_PN_WLanRadioInfos:
            {
                HDK_XML_Struct* psRadios;
                unsigned int ixRadio;

                if ((psRadios = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    /* Iterate over the radios */
                    for (ixRadio = 0; ixRadio < WIFI_RADIO_NUM; ++ixRadio)
                    {
                        HDK_XML_Struct* psRadio;
                        HDK_XML_Struct* psSupportedModes;
                        HDK_XML_Struct* psChannels;
                        HDK_XML_Struct* psWideChannels;
                        HDK_XML_Struct* psSupportedSecurity;
                        unsigned int ixChan;
                        WiFiSecurityMap* pValue;
                        WiFiSecurityMap* pEnd;

                        if ((psRadio = HDK_XML_Append_Struct(psRadios, HNAP12_Element_PN_RadioInfo)) == 0)
                        {
                            continue;
                        }

                        /* Set the RadioID and Frequency */
                        HDK_XML_Set_String(psRadio, HNAP12_Element_PN_RadioID, WIFI_RADIO_ID(ixRadio));
                        HDK_XML_Set_Int(psRadio, HNAP12_Element_PN_Frequency, WIFI_RADIO_FREQ(ixRadio));

                        /* Set the supported WiFiModes */
                        if ((psSupportedModes = HDK_XML_Set_Struct(psRadio, HNAP12_Element_PN_SupportedModes)) != 0)
                        {
                            for (ixChan = 0; g_WiFiModes[ixChan][ixRadio] != HNAP12_Enum_PN_WiFiMode__UNKNOWN__; ++ixChan)
                            {
                                HNAP12_Append_PN_WiFiMode(psSupportedModes, HNAP12_Element_PN_string, g_WiFiModes[ixChan][ixRadio]);
                            }
                        }

                        /* Set the Channels */
                        if ((psChannels = HDK_XML_Set_Struct(psRadio, HNAP12_Element_PN_Channels)) != 0)
                        {
                            for (ixChan = 0; g_Channels[ixChan][ixRadio] != -1; ++ixChan)
                            {
                                HDK_XML_Append_Int(psChannels, HNAP12_Element_PN_int, g_Channels[ixChan][ixRadio]);
                            }
                        }

                        /* Set the WideChannels */
                        if ((psWideChannels = HDK_XML_Set_Struct(psRadio, HNAP12_Element_PN_WideChannels)) != 0)
                        {
                            for (ixChan = 0; g_WideChannels[ixChan][ixRadio] != -1; ++ixChan)
                            {
                                HDK_XML_Struct* psSecondaryChannels;
                                HDK_XML_Struct* psWideChannel;

                                /* Create wide channel and add to wide channels array */
                                if ((psWideChannel = HDK_XML_Append_Struct(psWideChannels, HNAP12_Element_PN_WideChannel)) != 0)
                                {
                                    /* Set the channel */
                                    HDK_XML_Set_Int(psWideChannel, HNAP12_Element_PN_Channel, g_WideChannels[ixChan][ixRadio]);

                                    /*
                                     * There are exactly two secondary channels in each wide channel,
                                     * wide channel + 2, wide channel - 2, except for the case that channel == 0
                                     */
                                    if ((psSecondaryChannels = HDK_XML_Set_Struct(psWideChannel, HNAP12_Element_PN_SecondaryChannels)) != 0)
                                    {
                                        if (g_WideChannels[ixChan][ixRadio] == 0)
                                        {
                                            HDK_XML_Append_Int(psSecondaryChannels, HNAP12_Element_PN_int, 0);
                                        }
                                        else
                                        {
                                            HDK_XML_Append_Int(psSecondaryChannels, HNAP12_Element_PN_int, g_WideChannels[ixChan][ixRadio] + 2);
                                            HDK_XML_Append_Int(psSecondaryChannels, HNAP12_Element_PN_int, g_WideChannels[ixChan][ixRadio] - 2);
                                        }
                                    }
                                }
                            }
                        }

                        /* Set the SupportedSecurity */
                        if ((psSupportedSecurity = HDK_XML_Set_Struct(psRadio, HNAP12_Element_PN_SupportedSecurity)) != 0)
                        {
                            pEnd = g_WiFiSecurityMaps + sizeof(g_WiFiSecurityMaps) / sizeof(*g_WiFiSecurityMaps);
                            for (pValue = g_WiFiSecurityMaps; pValue != pEnd; ++pValue)
                            {
                                HDK_XML_Struct* psEncryptions;
                                HDK_XML_Struct* psSecurityInfo;

                                /* Create security info and add to supported security array */
                                if((psSecurityInfo = HDK_XML_Append_Struct(psSupportedSecurity, HNAP12_Element_PN_SecurityInfo)) != 0)
                                {
                                    /* Set the security type */
                                    HNAP12_Set_PN_WiFiSecurity(psSecurityInfo, HNAP12_Element_PN_SecurityType, pValue->eWiFiSecurity);

                                    /* There are two encryptions for each security type */
                                    if ((psEncryptions = HDK_XML_Set_Struct(psSecurityInfo, HNAP12_Element_PN_Encryptions)) != 0)
                                    {
                                        HNAP12_Append_PN_WiFiEncryption(psEncryptions, HNAP12_Element_PN_string, pValue->eWiFiEncryptions[0]);
                                        HNAP12_Append_PN_WiFiEncryption(psEncryptions, HNAP12_Element_PN_string, pValue->eWiFiEncryptions[1]);
                                    }
                                }
                            }
                        }
                    }
                    pMember = (HDK_XML_Member*)psRadios;
                }
            }
            break;

        case HNAP12_ADI_PN_WanSupportedTypes:
            {
                HDK_XML_Struct* psTypes;

                if ((psTypes = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    HNAP12_Enum_PN_WANType* peValue;
                    HNAP12_Enum_PN_WANType* peEnd = g_peWANTypes + sizeof(g_peWANTypes) / sizeof(*g_peWANTypes);

                    for (peValue = g_peWANTypes; peValue != peEnd; ++peValue)
                    {
                        HNAP12_Append_PN_WANType(psTypes, HNAP12_Element_PN_string, *peValue);
                    }
                }

                pMember = (HDK_XML_Member*)psTypes;
            }
            break;

        case HNAP12_ADI_PN_WanType:
        case HNAP12_ADI_PN_WanAutoDetectType:
            {
                HNAP12_Enum_PN_WANType eWanType;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_Proto, pszBuffer, sizeof(pszBuffer));
                eWanType = s_StrToEnum(g_WanTypeMap, pszBuffer);

                if (strcmp(pszBuffer, "pptp") == 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPTPAddressStatic, pszBuffer, sizeof(pszBuffer));

                    if (strcmp(pszBuffer, "1") == 0)
                    {
                        eWanType = HNAP12_Enum_PN_WANType_StaticPPTP;
                    }
                }
                else if (strcmp(pszBuffer, "l2tp") == 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_L2TPAddressStatic, pszBuffer, sizeof(pszBuffer));

                    if (strcmp(pszBuffer, "1") == 0)
                    {
                        eWanType = HNAP12_Enum_PN_WANType_StaticL2TP;
                    }
                }

                pMember = HNAP12_Set_PN_WANType(pStruct, element, eWanType);
            }
            break;

        case HNAP12_ADI_PN_WanStatus:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_CurrentState, pszBuffer, sizeof(pszBuffer));

                pMember = HNAP12_Set_PN_WANStatus(pStruct, element, (strcmp(pszBuffer, "up") == 0 ?
                                                                     HNAP12_Enum_PN_WANStatus_CONNECTED :
                                                                     HNAP12_Enum_PN_WANStatus_DISCONNECTED));
            }
            break;

        case HNAP12_ADI_PN_WanUsername:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_ProtoUsername, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_WanPassword:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_ProtoPassword, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_WanMaxIdleTime:
            {
                int iMaxIdleTime;
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPPIdleTime, pszBuffer, sizeof(pszBuffer));
                iMaxIdleTime = atoi(pszBuffer);

                /* Convert to seconds */
                iMaxIdleTime *= 60;

                pMember = HDK_XML_Set_Int(pStruct, element, iMaxIdleTime);
            }
            break;

        case HNAP12_ADI_PN_WanAutoReconnect:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPPConnMethod, pszBuffer, sizeof(pszBuffer));

                pMember = HDK_XML_Set_Bool(pStruct, element, (strcmp(pszBuffer, "demand") != 0));
            }
            break;

        case HNAP12_ADI_PN_WanAuthService:
            {
                pMember = HDK_XML_Set_String(pStruct, element, "");
            }
            break;

        case HNAP12_ADI_PN_WanPPPoEService:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPPoEServiceName, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_WanLoginService:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_ProtoServerAddress, pszBuffer, sizeof(pszBuffer));
                pMember = HDK_XML_Set_String(pStruct, element, pszBuffer);
            }
            break;

        case HNAP12_ADI_PN_WanIPAddress:
            {
                HNAP12_Enum_PN_WANType eWanType;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_Proto, pszBuffer, sizeof(pszBuffer));
                eWanType = s_StrToEnum(g_WanTypeMap, pszBuffer);

                if (strcmp(pszBuffer, "pptp") == 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPTPAddressStatic, pszBuffer, sizeof(pszBuffer));

                    if (strcmp(pszBuffer, "1") == 0)
                    {
                        eWanType = HNAP12_Enum_PN_WANType_StaticPPTP;
                    }
                }
                else if (strcmp(pszBuffer, "l2tp") == 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_L2TPAddressStatic, pszBuffer, sizeof(pszBuffer));

                    if (strcmp(pszBuffer, "1") == 0)
                    {
                        eWanType = HNAP12_Enum_PN_WANType_StaticL2TP;
                    }
                }

                if (eWanType == HNAP12_Enum_PN_WANType_Static || eWanType == HNAP12_Enum_PN_WANType_StaticPPTP ||
                    eWanType == HNAP12_Enum_PN_WANType_StaticL2TP)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_IPAddr, pszBuffer, sizeof(pszBuffer));
                }
                else
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_CurrentIPAddr, pszBuffer, sizeof(pszBuffer));
                }

                HDK_Util_StrToIP(&ipAddress, pszBuffer);
                pMember = HDK_XML_Set_IPAddress(pStruct, element, &ipAddress);
            }
            break;

        case HNAP12_ADI_PN_WanGateway:
            {
                HNAP12_Enum_PN_WANType eWanType;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_Proto, pszBuffer, sizeof(pszBuffer));
                eWanType = s_StrToEnum(g_WanTypeMap, pszBuffer);

                if (strcmp(pszBuffer, "pptp") == 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPTPAddressStatic, pszBuffer, sizeof(pszBuffer));

                    if (strcmp(pszBuffer, "1") == 0)
                    {
                        eWanType = HNAP12_Enum_PN_WANType_StaticPPTP;
                    }
                }
                else if (strcmp(pszBuffer, "l2tp") == 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_L2TPAddressStatic, pszBuffer, sizeof(pszBuffer));

                    if (strcmp(pszBuffer, "1") == 0)
                    {
                        eWanType = HNAP12_Enum_PN_WANType_StaticL2TP;
                    }
                }

                if (eWanType == HNAP12_Enum_PN_WANType_Static || eWanType == HNAP12_Enum_PN_WANType_StaticPPTP ||
                    eWanType == HNAP12_Enum_PN_WANType_StaticL2TP)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_DefaultGateway, pszBuffer, sizeof(pszBuffer));
                }
                else
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_DefaultRouter, pszBuffer, sizeof(pszBuffer));
                }

                HDK_Util_StrToIP(&ipAddress, pszBuffer);
                pMember = HDK_XML_Set_IPAddress(pStruct, element, &ipAddress);
            }
            break;

        case HNAP12_ADI_PN_WanSubnetMask:
            {
                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_Netmask, pszBuffer, sizeof(pszBuffer));
                HDK_Util_StrToIP(&ipAddress, pszBuffer);

#ifndef HNAP_DEBUG
                if (!(ipAddress.a || ipAddress.b || ipAddress.c || ipAddress.d))
                {
                    char pszCmd[64] = {'\0'};
                    char pszLine[UTOPIA_BUF_SIZE] = {'\0'};
                    HDK_XML_IPAddress wanIP;
                    FILE* fp;

                    /* We need the WAN IPAddr */
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_CurrentIPAddr, pszBuffer, sizeof(pszBuffer));
                    HDK_Util_StrToIP(&wanIP, pszBuffer);

                    if (wanIP.a || wanIP.b || wanIP.c || wanIP.d)
                    {
                        safec_rc = sprintf_s(pszCmd, sizeof(pszCmd),"/sbin/route -n | grep %u.%u.%u 2> /dev/null", wanIP.a, wanIP.b, wanIP.c);
                        if(safec_rc < EOK)
                        {
                           ERR_CHK(safec_rc);
                        }
                        /* Spawn process to get the netmask */
                        fp = popen(pszCmd, "r");
                        if (fp != 0)
                        {
                            /* Read the output a line at a time, parsing the dest, gateway, and subnet ips */
                            while (fgets(pszLine, sizeof(pszLine) - 1, fp) != 0)
                            {
                                char pszDest[16] = {'\0'};
                                char pszGateway[16] = {'\0'};
                                char pszNetmask[16] = {'\0'};

                                if (sscanf(pszLine, "%s %s %s", pszDest, pszGateway, pszNetmask) == 3)
                                {
                                    HDK_XML_IPAddress destIP;

                                    HDK_Util_StrToIP(&destIP, pszDest);
                                    if (destIP.a == wanIP.a && destIP.b == wanIP.b && destIP.c == wanIP.c)
                                    {
                                        HDK_Util_StrToIP(&ipAddress, pszNetmask);
                                    }
                                }
                            }
                            pclose(fp);
                        }
                    }
                }
#endif
                pMember = HDK_XML_Set_IPAddress(pStruct, element, &ipAddress);
            }
            break;

        case HNAP12_ADI_PN_WanDNS:
            {
                char pszPrimary[18] = {'\0'};
                char pszSecondary[18] = {'\0'};
                char pszTertiary[18] = {'\0'};
                HDK_XML_IPAddress ipDefault = {0,0,0,0};
                HDK_XML_Struct* psDNS;

                if ((psDNS = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer1, pszPrimary, sizeof(pszPrimary) - 1);
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer2, pszSecondary, sizeof(pszSecondary) - 1);
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer3, pszTertiary, sizeof(pszTertiary) - 1);

                    if (HDK_Util_StrToIP(&ipAddress, pszPrimary))
                    {
                        HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Primary, &ipAddress);

                        if (HDK_Util_StrToIP(&ipAddress, pszSecondary))
                        {
                            HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Secondary, &ipAddress);

                            if (HDK_Util_StrToIP(&ipAddress, pszTertiary))
                            {
                                HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Tertiary, &ipAddress);
                            }
                            else
                            {
                                HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Tertiary, &ipDefault);
                            }
                        }
                        else
                        {
                            HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Secondary, &ipDefault);
                            HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Tertiary, &ipDefault);
                        }
                    }
                    else
                    {
                        HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Primary, &ipDefault);
                        HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Secondary, &ipDefault);
                        HDK_XML_Set_IPAddress(psDNS, HNAP12_Element_PN_Tertiary, &ipDefault);
                    }
                }
                pMember = (HDK_XML_Member*)psDNS;
            }
            break;

        case HNAP12_ADI_PN_WanAutoMTUSupported:
            {
                /* RRP allows auto MTU */
                pMember = HDK_XML_Set_Bool(pStruct, element, 1);
            }
            break;

        case HNAP12_ADI_PN_WanMTU:
            {
                int iMTU;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_MTU, pszBuffer, sizeof(pszBuffer));
                iMTU = atoi(pszBuffer);

                /* Need to return the correct MTU value if it's set to auto (0) */
                if(iMTU == 0)
                {
                    HNAP12_Enum_PN_WANType eWanType;

                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WAN_Proto, pszBuffer, sizeof(pszBuffer));
                    eWanType = s_StrToEnum(g_WanTypeMap, pszBuffer);

                    switch(eWanType)
                    {
                        case HNAP12_Enum_PN_WANType_BigPond:
                        case HNAP12_Enum_PN_WANType_DHCP:
                        case HNAP12_Enum_PN_WANType_Static:
                            {
                                iMTU = 1500;
                            }
                            break;

                        case HNAP12_Enum_PN_WANType_DHCPPPPoE:
                            {
                                iMTU = 1492;
                            }
                            break;

                        case HNAP12_Enum_PN_WANType_DynamicPPTP:
                        case HNAP12_Enum_PN_WANType_StaticPPTP:
                            {
                                iMTU = 1452;
                            }
                            break;

                        case HNAP12_Enum_PN_WANType_DynamicL2TP:
                        case HNAP12_Enum_PN_WANType_StaticL2TP:
                            {
                                iMTU = 1460;
                            }
                            break;

                        default:
                            break;
                    }
                }
                pMember = HDK_XML_Set_Int(pStruct, element, iMTU);
            }
            break;

        case HNAP12_ADI_PN_WanMacAddress:
            {
                HDK_XML_MACAddress macAddress = {0,0,0,0,0,0};

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DefHwAddr, pszBuffer, sizeof(pszBuffer));

                if (strcmp(pszBuffer, "") == 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_VLan2Mac, pszBuffer, sizeof(pszBuffer));
                }

                HDK_Util_StrToMAC(&macAddress, pszBuffer);
                pMember = HDK_XML_Set_MACAddress(pStruct, element, &macAddress);
            }
            break;

        case HNAP12_ADI_PN_ConfigBlob:
            {
                char pszState[UTOPIA_STATE_SIZE] = {'\0'};

                if (Utopia_GetAll(pUTCtx(pMethodCtx), pszState, sizeof(pszState)) != 0 &&
                    HDK_XML_Set_Blob(pStruct, element, pszState, strlen(pszState)) != 0)
                {
                    pMember = (HDK_XML_Member*)1;
                }
            }
            break;

        case HNAP12_ADI_PN_FactoryRestoreTrigger:
            {
                char psSalt[3];

                /* Encrypt the password */
                srandom((int)time(0));
                to64(&psSalt[0], random(), 2);

                /* Doing a set all with a null buffer results in the factory settings being restored */
                if (Utopia_SetAll(pUTCtx(pMethodCtx), 0) != 0 &&
                    /* Need to set the username/password to defaults */
                    Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_HTTP_AdminPassword, crypt(UTOPIA_DEFAULT_ADMIN_PASSWD, psSalt)) != 0 &&
                    Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_HTTP_AdminUser, UTOPIA_DEFAULT_ADMIN_USER) != 0)
                {
                    /* Set the reboot event */
                    Utopia_SetEvent(pUTCtx(pMethodCtx), Utopia_Event_Reboot);
                    pMember = (HDK_XML_Member*)1;
                }
            }
            break;

        default:
            break;
    }

    return pMember;
}

/* Device HNAP12 ADISet implementation */
static int s_HNAP12_SRV_Device_ADISet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                                      HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    /* No auto-provisioning for HNAP12, so return defaults to failure */
    int iReturn = 0;

    /* Variables used by a number of cases, declared here to save typing */
    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    char* pszStr;
    errno_t safec_rc = -1;

    switch(value)
    {
        case HNAP12_ADI_PN_DeviceName:
            {
                if ((pszStr = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_HostName, pszStr);
                }
            }
            break;

        case HNAP12_ADI_PN_AdminPassword:
            {
                if ((pszStr = HDK_XML_Get_String(pStruct, element)) != 0 &&
                     strlen(pszStr) >= UTOPIA_MIN_PASSWD_LENGTH &&
                     strlen(pszStr) <= UTOPIA_MAX_PASSWD_LENGTH)
                {
                    char psSalt[3];

                    /* Encrypt the password */
                    srandom((int)time(0));
                    to64(&psSalt[0], random(), 2);
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_HTTP_AdminPassword, crypt(pszStr, psSalt));
                }
            }
            break;

        case HNAP12_ADI_PN_Username:
            {
                if ((pszStr = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    /* The RRP does not currently support username */
                    iReturn = 1;
                }
            }
            break;

        case HNAP12_ADI_PN_TimeZone:
            {
                if ((pszStr = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    unsigned int i;

                    for (i = 0; i < TIMEZONE_NUM; ++i)
                    {
                        /* Look for the matching timezone */
                        if (strcmp(pszStr, g_TimezoneMap[i].pszUTC) == 0)
                        {
                            iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_TZ, g_TimezoneMap[i].pszTZ);
                            break;
                        }
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_AutoAdjustDST:
            {
                int* pfAutoDST;

                if ((pfAutoDST = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    unsigned int i;

                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_TZ, pszBuffer, sizeof(pszBuffer));

                    for (i = 0; i < TIMEZONE_NUM; ++i)
                    {
                        /* Look for the matching timezone */
                        if (strstr(pszBuffer, g_TimezoneMap[i].pszTZ) != 0)
                        {
                            /* AutoDST string */
                            if (*pfAutoDST &&
                                g_TimezoneMap[i].pszDT != 0)
                            {
                                safec_rc = strcat_s(pszBuffer, sizeof(pszBuffer),g_TimezoneMap[i].pszDT);
                                ERR_CHK(safec_rc);
                            }

                            iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_TZ, pszBuffer) &&
                                Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_AutoDST,
                                           (*pfAutoDST == 0 ? "0" :
                                            (g_TimezoneMap[i].pszDT != 0 ? "2" : "1")));

                            break;
                        }
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_Locale:
            {
                if ((pszStr = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    char** ppszLocale;
                    char** ppszEnd = g_ppszLocales + sizeof(g_ppszLocales) / sizeof(*g_ppszLocales);

                    /* Search for locale in supported RRP supported locales */
                    for (ppszLocale = g_ppszLocales; ppszLocale != ppszEnd; ++ppszLocale)
                    {
                        if (strcmp(pszStr, *ppszLocale) == 0)
                        {
                            iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_Locale, *ppszLocale);
                            break;
                        }
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_SSL:
            {
                int* pfSSL;

                if ((pfSSL = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_HTTPSAccess,
                                         (*pfSSL ? "1" : "0"));
                }
            }
            break;

        case HNAP12_ADI_PN_PortMappings:
            {
                HDK_XML_Struct* psPortMappings;

                if ((psPortMappings = HDK_XML_Get_Struct(pStruct, element)) != 0)
                {
                    HDK_XML_Member* pmPortMapping;
                    int iPMCount = 0;
                    int i;

                    /* Get the port mapping count */
                    if (Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_Firewall_SPFCount, pszBuffer, sizeof(pszBuffer)) != 0)
                    {
                        iPMCount = atoi(pszBuffer);
                    }

                    /* Loop over current port mappings, deleting them */
                    for (i = 1; i <= iPMCount; ++i)
                    {
                        /* Unset each element of a single port forward */
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Enabled, i);
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_PrevRuleEnabledState, i);
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Name, i);
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Protocol, i);
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_ExternalPort, i);
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_InternalPort, i);
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_ToIp, i);
                        Utopia_UnsetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SinglePortForward, i);
                    }

                    i = 0;

                    /* Iterate over the PortMappings, and serialize to syscfg */
                    iReturn = 1;
                    for (pmPortMapping = psPortMappings->pHead; iReturn && pmPortMapping; pmPortMapping = pmPortMapping->pNext)
                    {
                        char* pszDescription;
                        HNAP12_Enum_PN_IPProtocol* pEnum;
                        HDK_XML_IPAddress* pIPAddress;
                        HDK_XML_Struct* psPortMapping;
                        int* piExtPort;
                        int* piIntPort;

                        psPortMapping = HDK_XML_GetMember_Struct(pmPortMapping);
                        piExtPort = HDK_XML_Get_Int(psPortMapping, HNAP12_Element_PN_ExternalPort);
                        piIntPort = HDK_XML_Get_Int(psPortMapping, HNAP12_Element_PN_InternalPort);
                        pszDescription = HDK_XML_Get_String(psPortMapping, HNAP12_Element_PN_PortMappingDescription);
                        pEnum = HNAP12_Get_PN_IPProtocol(psPortMapping, HNAP12_Element_PN_PortMappingProtocol);
                        pIPAddress = HDK_XML_Get_IPAddress(psPortMapping, HNAP12_Element_PN_InternalClient);

                        if (pszDescription && pIPAddress && pEnum && piExtPort && piIntPort &&
                            /* Ports must be > 0 */
                            *piExtPort > 0 && *piIntPort > 0 &&
                            *pEnum != HNAP12_Enum_PN_IPProtocol__UNKNOWN__ &&
                            ++i <= 10)
                        {
                            /* Create a unique namespace key */
                            safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"spf_%d", i);
                            if(safec_rc < EOK)
                            {
                               ERR_CHK(safec_rc);
                            }
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SinglePortForward, i, pszBuffer);

                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Enabled, i, "1");
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_PrevRuleEnabledState, i, "1");
						
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Name, i, pszDescription);
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_Protocol, i, (*pEnum == HNAP12_Enum_PN_IPProtocol_TCP ? "tcp" : "udp"));

                            safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%d", *piExtPort);
                            if(safec_rc < EOK)
                            {
                               ERR_CHK(safec_rc);
                            }
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_ExternalPort, i, pszBuffer);

                            safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%d", *piIntPort);
                            if(safec_rc < EOK)
                            {
                               ERR_CHK(safec_rc);
                            }
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_InternalPort, i, pszBuffer);

                            safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%u", pIPAddress->d);
                            if(safec_rc < EOK)
                            {
                               ERR_CHK(safec_rc);
                            }
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_SPF_ToIp, i, pszBuffer);
                        }
                        else
                        {
                            iReturn = 0;
                        }
                    }

                    if (iReturn)
                    {
                        /* Update the SinglePortForwardCount */
                        safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%d", i);
                        if(safec_rc < EOK)
                        {
                          ERR_CHK(safec_rc);
                        }
                        iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_Firewall_SPFCount, pszBuffer);
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_MFEnabled:
            {
                int* pfEnabled;

                if ((pfEnabled = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WLAN_AccessRestriction, *pfEnabled ? "" : "disabled");
                }
            }
            break;

        case HNAP12_ADI_PN_MFIsAllowList:
            {
                int* pfMode;

                if ((pfMode = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WLAN_AccessRestriction, pszBuffer, sizeof(pszBuffer));

                    /* Only want to set this if it's enabled */
                    if (strcmp(pszBuffer, "disabled") != 0)
                    {
                        iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WLAN_AccessRestriction, *pfMode ? "permit" : "deny");
                    }
                    else
                    {
                        iReturn = 1;
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_MFMACList:
            {
                HDK_XML_Struct* psMacList;

                if ((psMacList = HDK_XML_Get_Struct(pStruct, element)) != 0)
                {
                    HDK_XML_Member* pmMacInfo;
                    int iMACListCount = 0;

                    /* Iterate over the Mac list, and build up utopia string */
                    iReturn = 1;
                    for (pmMacInfo = psMacList->pHead; iReturn && pmMacInfo; pmMacInfo = pmMacInfo->pNext)
                    {
                        char pszMacAddr[20] = {'\0'};
                        HDK_XML_MACAddress* pMacAddr;
                        HDK_XML_Struct* psMacInfo;

                        if ((psMacInfo = HDK_XML_GetMember_Struct(pmMacInfo)) != 0 &&
                            (pMacAddr = HDK_XML_Get_MACAddress(psMacInfo, HNAP12_Element_PN_MacAddress)) != 0 &&
                            (pMacAddr->a || pMacAddr->b || pMacAddr->c ||
                             pMacAddr->d || pMacAddr->e || pMacAddr->f) &&
                            HDK_Util_MACToStr(pszMacAddr, pMacAddr) &&
                            ++iMACListCount <= 32 )
                        {
                           safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%s%s", pszBuffer, pszMacAddr);
                           if(safec_rc < EOK)
                           {
                              ERR_CHK(safec_rc);
                           }
                        }
                        if(safec_rc >= 0)
                        {

                            /* If this is not the last one, concatenate the delimeter */
                            if (pmMacInfo->pNext)
                            {
                                safec_rc = strcat_s(pszBuffer, sizeof(pszBuffer)," ");
                                ERR_CHK(safec_rc);
                            }
                        }
                        else
                        {
                            iReturn = 0;
                        }
                    }
                    /* The UT610N can handle a max of 32 port mappings */
                    if (iReturn)
                    {
                        iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WLAN_MACFilter, pszBuffer);
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_LanIPAddress:
            {
                HDK_XML_IPAddress* pIPAddr;

                if ((pIPAddr = HDK_XML_Get_IPAddress(pStruct, element)) != 0 &&
                    HDK_Util_IPToStr(pszBuffer, pIPAddr) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_LAN_IPAddr, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_LanSubnetMask:
            {
                HDK_XML_IPAddress* pIPSubnet;

                if ((pIPSubnet = HDK_XML_Get_IPAddress(pStruct, element)) != 0 &&
                    /* Only 255.255.255.[0|128] subnets are allowed */
                    (pIPSubnet->a == 255) &&
                    (pIPSubnet->b == 255) &&
                    (pIPSubnet->c == 255) &&
                    ((pIPSubnet->d == 0) || (pIPSubnet->d == 128)) &&
                    HDK_Util_IPToStr(pszBuffer, pIPSubnet) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_LAN_Netmask, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_WanRenewTimeout:
            {
                /* Recycle the WAN system */
                iReturn = Utopia_SetEvent(pUTCtx(pMethodCtx), Utopia_Event_WAN_Restart);
            }
            break;

        case HNAP12_ADI_PN_DHCPServerEnabled:
            {
                int* pfEnabled;

                if ((pfEnabled = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_DHCP_ServerEnabled, *pfEnabled ? "1" : "0");
                }
            }
            break;

        case HNAP12_ADI_PN_DHCPLeaseTime:
            {
                int* piLeaseTime;

                if ((piLeaseTime = HDK_XML_Get_Int(pStruct, element)) != 0 &&
                     *piLeaseTime >= 1 &&
                     *piLeaseTime <= 9999 )
                {
                     safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%dm", *piLeaseTime);
                     if(safec_rc < EOK)
                     {
                        ERR_CHK(safec_rc);
                     }
                }
                if(safec_rc >= 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_DHCP_LeaseTime, pszBuffer);
                }
            }

            break;

        case HNAP12_ADI_PN_DHCPIPAddressFirst:
            {
                HDK_XML_IPAddress* pIPFirst;

                if ((pIPFirst = HDK_XML_Get_IPAddress(pStruct, element)) != 0 )
                {
                   safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%u", (unsigned int)pIPFirst->d);
                   if(safec_rc < EOK)
                   {
                      ERR_CHK(safec_rc);
                   }
                }

                if(safec_rc >= 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_DHCP_Start, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_DHCPIPAddressLast:
            {
                HDK_XML_IPAddress *pIPLast;
                int iFirstIPOctet;

                Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_DHCP_Start, pszBuffer, sizeof(pszBuffer));
                iFirstIPOctet = atoi(pszBuffer);

                if ((pIPLast = HDK_XML_Get_IPAddress(pStruct, element)) != 0 )
                {
                   safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%u", (unsigned int)(pIPLast->d - iFirstIPOctet + 1));
                   if(safec_rc < EOK)
                   {
                      ERR_CHK(safec_rc);
                   }
                }
                if(safec_rc >= 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_DHCP_Num, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_DHCPReservations:
            {
                char pszDHCP[22] = {'\0'};
                HDK_XML_IPAddress* pIpAddr;
                HDK_XML_MACAddress* pMacAddr;
                HDK_XML_Member* pmDHCP;
                HDK_XML_Struct* psDHCPs;
                int iCount = 0;

                if ((psDHCPs = HDK_XML_Get_Struct(pStruct, HNAP12_Element_PN_DHCPReservations)) != 0)
                {
                    /* Iterate over the DHCP reservations, and build up NVRAM string */
                    for (pmDHCP = psDHCPs->pHead; pmDHCP; pmDHCP = pmDHCP->pNext)
                    {
                        char pszMacAddr[20] = {'\0'};
                        HDK_XML_Struct* psDHCP;

                        if ((psDHCP = HDK_XML_GetMember_Struct(pmDHCP)) != 0 &&
                            (pIpAddr = HDK_XML_Get_IPAddress(psDHCP, HNAP12_Element_PN_IPAddress)) != 0 &&
                            (pMacAddr = HDK_XML_Get_MACAddress(psDHCP, HNAP12_Element_PN_MacAddress)) != 0 &&
                            HDK_Util_MACToStr(pszMacAddr, pMacAddr) != 0 )
	                    {
                            safec_rc = sprintf_s(pszDHCP, sizeof(pszDHCP),"%s,%u", pszMacAddr, (unsigned int)pIpAddr->d);
                            if(safec_rc < EOK)
                            {
                               ERR_CHK(safec_rc);
                            }
                        }
                        if(safec_rc >= 0)
                        {
                            ++iCount;
                            Utopia_SetIndexed(pUTCtx(pMethodCtx), UtopiaValue_DHCP_StaticHost, iCount, pszDHCP);
                        }
                    }

                    /* Set the dhcp reservation count */
                    safec_rc = sprintf_s(pszDHCP, sizeof(pszDHCP),"%d", iCount);
                    if(safec_rc < EOK)
                    {
                       ERR_CHK(safec_rc);
                    }
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_DHCP_NumStaticHosts, pszDHCP);
                }
            }
            break;

        case HNAP12_ADI_PN_ManageRemote:
            {
                int* pfRemoteManage;

                if ((pfRemoteManage = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WANAccess,
                                         (*pfRemoteManage ? "1" : "0"));
                }
            }
            break;

        case HNAP12_ADI_PN_ManageWireless:
            {
                int* pfManageWireless;

                if ((pfManageWireless = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WIFIAccess,
                                         (*pfManageWireless ? "1" : "0"));
                }
            }
            break;

        case HNAP12_ADI_PN_RemoteSSL:
            {
                int* pfRemoteSSL;

                if ((pfRemoteSSL = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WANHTTPSAccess,
                                         (*pfRemoteSSL ? "1" : "0"));
                }
            }
            break;

        case HNAP12_ADI_PN_RemotePort:
            {
                int* piRemotePort;

                if ((piRemotePort = HDK_XML_Get_Int(pStruct, element)) != 0)
                {
                    safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%d", *piRemotePort);
                    if(safec_rc < EOK)
                    {
                       ERR_CHK(safec_rc);
                    }

                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_Mgmt_WANHTTPPort, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_DomainName:
            {
                char* pszDomainName;

                if ((pszDomainName = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_LAN_Domain, pszDomainName);
                }
            }
            break;

        case HNAP12_ADI_PN_WiredQoS:
            {
                int* pfWiredQoS;

                if ((pfWiredQoS = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_QoS_Enable,
                                         (*pfWiredQoS ? "1" : "0"));
                }
            }
            break;

        case HNAP12_ADI_PN_WanType:
            {
                char* pszType;
                HNAP12_Enum_PN_WANType eWanType = HNAP12_GetEx_PN_WANType(pStruct, element, HNAP12_Enum_PN_WANType__UNKNOWN__);

                if ((pszType = s_EnumToStr(g_WanTypeMap, (int)eWanType)) != 0)
                {
                    if (eWanType == HNAP12_Enum_PN_WANType_StaticPPTP)
                    {
                        Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPTPAddressStatic, "1");
                    }
                    else if (eWanType == HNAP12_Enum_PN_WANType_DynamicPPTP)
                    {
                        Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPTPAddressStatic, "0");
                    }
                    else if (eWanType == HNAP12_Enum_PN_WANType_StaticL2TP)
                    {
                        Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_L2TPAddressStatic, "1");
                    }
                    else if (eWanType == HNAP12_Enum_PN_WANType_DynamicL2TP)
                    {
                        Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_L2TPAddressStatic, "0");
                    }

                    /* Need to unset the IPAddress, Gateway, and Subnet mask for dynamic type */
                    if (eWanType == HNAP12_Enum_PN_WANType_DHCP ||
                        eWanType == HNAP12_Enum_PN_WANType_DHCPPPPoE ||
                        eWanType == HNAP12_Enum_PN_WANType_DynamicPPTP ||
                        eWanType == HNAP12_Enum_PN_WANType_DynamicL2TP)
                    {
                        Utopia_Unset(pUTCtx(pMethodCtx), UtopiaValue_WAN_IPAddr);
                        Utopia_Unset(pUTCtx(pMethodCtx), UtopiaValue_WAN_DefaultGateway);
                        Utopia_Unset(pUTCtx(pMethodCtx), UtopiaValue_WAN_DefaultRouter);
                        Utopia_Unset(pUTCtx(pMethodCtx), UtopiaValue_WAN_Netmask);
                    }

                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_Proto, pszType);
                }
            }
            break;

        case HNAP12_ADI_PN_WanUsername:
            {
                char* pszUsername;

                if ((pszUsername = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_ProtoUsername, pszUsername);
                }
            }
            break;

        case HNAP12_ADI_PN_WanPassword:
            {
                char* pszPassword;

                if ((pszPassword = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_ProtoPassword, pszPassword);
                }
            }
            break;

        case HNAP12_ADI_PN_WanMaxIdleTime:
            {
                int* piMaxIdleTime;

                if ((piMaxIdleTime = HDK_XML_Get_Int(pStruct, element)) != 0)
                {
                    /* Convert to minutes */
                    *piMaxIdleTime /= 60;
                    safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%d", *piMaxIdleTime);
                    if(safec_rc < EOK)
                    {
                       ERR_CHK(safec_rc);
                    }
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPPIdleTime, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_WanAutoReconnect:
            {
                int* pfAutoReconnect;

                if ((pfAutoReconnect = HDK_XML_Get_Bool(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPPConnMethod,
                                         (*pfAutoReconnect ? "redial" : "demand"));
                }
            }
            break;

        case HNAP12_ADI_PN_WanAuthService:
            {
                char* pszAuthService;

                if ((pszAuthService = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    iReturn = 1;
                }
            }
            break;

        case HNAP12_ADI_PN_WanPPPoEService:
            {
                char* pszPPPoEService;

                if ((pszPPPoEService = HDK_XML_Get_String(pStruct, element)) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_PPPoEServiceName, pszPPPoEService);
                }
            }
            break;

        case HNAP12_ADI_PN_WanLoginService:
            {
                char* pszLoginService;
                unsigned int i;

                if ((pszLoginService = HDK_XML_Get_String(pStruct, element)) != 0 &&
                    /* Make sure that the service name is an IPAddress */
                    sscanf(pszLoginService, "%u.%u.%u.%u", &i, &i, &i, &i) == 4)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_ProtoServerAddress, pszLoginService);
                }
            }
            break;

        case HNAP12_ADI_PN_WanIPAddress:
        case HNAP12_ADI_PN_WanGateway:
        case HNAP12_ADI_PN_WanSubnetMask:
            {
                HDK_XML_IPAddress* pIPAddr;

                if ((pIPAddr = HDK_XML_Get_IPAddress(pStruct, element)) != 0 &&
                    HDK_Util_IPToStr(pszBuffer, pIPAddr) != 0)
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx),
                                         (value == HNAP12_ADI_PN_WanIPAddress ? UtopiaValue_WAN_IPAddr :
                                          (value == HNAP12_ADI_PN_WanGateway ? UtopiaValue_WAN_DefaultGateway :
                                           UtopiaValue_WAN_Netmask)), pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_WanDNS:
            {
                HDK_XML_Struct* psDNS;

                if ((psDNS = HDK_XML_Get_Struct(pStruct, element)) != 0)
                {
                    HDK_XML_IPAddress* pPrimary = HDK_XML_Get_IPAddress(psDNS, HNAP12_Element_PN_Primary);
                    HDK_XML_IPAddress* pSecondary = HDK_XML_Get_IPAddress(psDNS, HNAP12_Element_PN_Secondary);
                    HDK_XML_IPAddress* pTertiary = HDK_XML_Get_IPAddress(psDNS, HNAP12_Element_PN_Tertiary);

                    if (pPrimary && (pPrimary->a || pPrimary->b || pPrimary->c || pPrimary->d))
                    {
                        HDK_Util_IPToStr(pszBuffer, pPrimary);
                        iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer1, pszBuffer);

                        if (pSecondary && (pSecondary->a || pSecondary->b || pSecondary->c || pSecondary->d))
                        {
                            HDK_Util_IPToStr(pszBuffer, pSecondary);
                            iReturn &= Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer2, pszBuffer);

                            if (pTertiary && (pTertiary->a || pTertiary->b || pTertiary->c || pTertiary->d))
                            {
                                HDK_Util_IPToStr(pszBuffer, pTertiary);
                                iReturn &= Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer3, pszBuffer);
                            }
                            else
                            {
                                iReturn &= Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer3, "");
                            }
                        }
                        else
                        {
                            iReturn &= Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer2, "") &&
                                Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer3, "");
                        }
                    }
                    else
                    {
                        iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer1, "") &&
                            Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer2, "") &&
                            Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_NameServer3, "");
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_WanMacAddress:
            {
                HDK_XML_MACAddress* pMac;

                if ((pMac = HDK_XML_Get_MACAddress(pStruct, element)) != 0 &&
                    /* Make sure it's not a multicast mac address */
                    ((pMac->a & 0x01) != 0x01) &&
                    HDK_Util_MACToStr(pszBuffer, pMac) != 0 &&
                    (pMac->a || pMac->b || pMac->c || pMac->d || pMac->e || pMac->f))
                {
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_DefHwAddr, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_WanMTU:
            {
                int* piMTU = HDK_XML_Get_Int(pStruct, HNAP12_Element_PN_MTU);

                if ((piMTU = HDK_XML_Get_Int(pStruct, element)) != 0 &&
                    /* Make sure mtu is in range if not 0 */
                    (*piMTU == 0 || *piMTU >= 576))
                {
                    /* Need to set the MTU to the correct value if it's set to auto (0) */
                    if(*piMTU == 0)
                    {
                        HNAP12_Enum_PN_WANType eWanType = HNAP12_GetEx_PN_WANType(pStruct, HNAP12_Element_PN_Type,
                                                                                  HNAP12_Enum_PN_WANType__UNKNOWN__);

                        switch(eWanType)
                        {
                            case HNAP12_Enum_PN_WANType_BigPond:
                            case HNAP12_Enum_PN_WANType_DHCP:
                            case HNAP12_Enum_PN_WANType_Static:
                                {
                                    *piMTU = 1500;
                                }
                                break;

                            case HNAP12_Enum_PN_WANType_DHCPPPPoE:
                                {
                                    *piMTU = 1492;
                                }
                                break;

                            case HNAP12_Enum_PN_WANType_DynamicPPTP:
                            case HNAP12_Enum_PN_WANType_StaticPPTP:
                                {
                                    *piMTU = 1452;
                                }
                                break;

                            case HNAP12_Enum_PN_WANType_DynamicL2TP:
                            case HNAP12_Enum_PN_WANType_StaticL2TP:
                                {
                                    *piMTU = 1460;
                                }
                                break;

                            default:
                                break;
                        }
                    }

                    safec_rc = sprintf_s(pszBuffer, sizeof(pszBuffer),"%d", *piMTU);
                    if(safec_rc < EOK)
                    {
                       ERR_CHK(safec_rc);
                    }
                    iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WAN_MTU, pszBuffer);
                }
            }
            break;

        case HNAP12_ADI_PN_WLanSecurityEnabled:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, SecurityEnabled);
            }
            break;

        case HNAP12_ADI_PN_WLanType:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, Type);
            }
            break;

        case HNAP12_ADI_PN_WLanEncryption:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, Encryption);
            }
            break;

        case HNAP12_ADI_PN_WLanKey:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, Key);
            }
            break;

        case HNAP12_ADI_PN_WLanKeyRenewal:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, KeyRenewal);
            }
            break;

        case HNAP12_ADI_PN_WLanRadiusIP1:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, RadiusIP1);
            }
            break;

        case HNAP12_ADI_PN_WLanRadiusPort1:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, RadiusPort1);
            }
            break;

        case HNAP12_ADI_PN_WLanRadiusSecret1:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, RadiusSecret1);
            }
            break;

        case HNAP12_ADI_PN_WLanEnabled:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, Enabled);
            }
            break;

        case HNAP12_ADI_PN_WLanMode:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, Mode);
            }
            break;

        case HNAP12_ADI_PN_WLanSSID:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, SSID);
            }
            break;

        case HNAP12_ADI_PN_WLanSSIDBroadcast:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, SSIDBroadcast);
            }
            break;

        case HNAP12_ADI_PN_WLanChannelWidth:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, ChannelWidth);
            }
            break;

        case HNAP12_ADI_PN_WLanChannel:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, Channel);
            }
            break;

        case HNAP12_ADI_PN_WLanSecondaryChannel:
            {
                iReturn = WLan_ADISet(pMethodCtx, pStruct, element, SecondaryChannel);
            }
            break;

        case HNAP12_ADI_PN_WLanQoS:
            {
                /* Since this is not an indexed value in syscfg, we need to handle it specially */
                HDK_XML_Struct* psWLanInfos;

                if ((psWLanInfos = HDK_XML_Get_Struct(pStruct, element)) != 0)
                {
                    char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};
                    HDK_XML_Member* pmWLanInfo = 0;

                    /* Get the old value for comparison */
                    Utopia_Get(pUTCtx(pMethodCtx), UtopiaValue_WLAN_WMMSupport, pszBuffer, sizeof(pszBuffer));

                    /* Iterate over the wlan infos, setting each one */
                    for (pmWLanInfo = psWLanInfos->pHead; pmWLanInfo; pmWLanInfo = pmWLanInfo->pNext)
                    {
                        int* pfQoS;

                        if ((pfQoS = HDK_XML_Get_Bool(HDK_XML_GetMember_Struct(pmWLanInfo), HNAP12_Element_PN_QoS)) != 0)
                        {
                            /* If the QoS value is changing, then set it and break */
                            if (*pfQoS != strcmp(pszBuffer, "disabled"))
                            {
                                iReturn = Utopia_Set(pUTCtx(pMethodCtx), UtopiaValue_WLAN_WMMSupport, *pfQoS ? "enabled" : "disabled");
                                break;
                            }
                            else
                            {
                                iReturn = 1;
                            }
                        }
                    }
                }
            }
            break;

        case HNAP12_ADI_PN_ConfigBlob:
            {
                char* psBuf;
                unsigned int ccb;

                if ((psBuf = HDK_XML_Get_Blob(pStruct, element, &ccb)) != 0 &&
                    (iReturn = Utopia_SetAll(pUTCtx(pMethodCtx), psBuf)) != 0)
                {
                    /* Set the reboot event */
                    Utopia_SetEvent(pUTCtx(pMethodCtx), Utopia_Event_Reboot);
                }
            }
            break;

        default:
            break;
    }

    return iReturn;
}

static HDK_XML_Member* s_HOTSPOT_SRV_Device_ADIGet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                                                   HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Member* pMember = 0;

    /* Unused variabled */
    (void) pMethodCtx;
    (void) pStruct;
    (void) element;

    switch(value)
    {
        /* NOTE: This is not a full solution because HasWANAccess and OnGuestNetwork are not implemented */
        case HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_WANAccessStatuses:
            {
                HDK_XML_Struct* psClients;
                HDK_XML_Struct* psWANAccessStatuses;
                HDK_XML_Struct sTemp;

                /* Initialize the temporary struct */
                HDK_XML_Struct_Init(&sTemp);

                /* Pull the WANAccessStatuses from the ConnectedClients */
                if (s_GetConnectedClients(pMethodCtx, &sTemp, element) != 0 &&
                    (psClients = HDK_XML_Get_Struct(&sTemp, element)) != 0 &&
                    (psWANAccessStatuses = HDK_XML_Set_Struct(pStruct, element)) != 0)
                {
                    HDK_XML_Member* pmClient;

                    for (pmClient = psClients->pHead; pmClient; pmClient = pmClient->pNext)
                    {
                        HDK_XML_Struct* psClient = HDK_XML_GetMember_Struct(pmClient);
                        HDK_XML_IPAddress* pIP = HDK_XML_Get_IPAddress(psClient, HNAP12_Element_PN_IPAddress);
                        HDK_XML_MACAddress* pMAC = HDK_XML_Get_MACAddress(psClient, HNAP12_Element_PN_MacAddress);
                        HDK_XML_Struct* psWANAccessStatus;

                        if (pIP && pMAC &&
                            (psWANAccessStatus = HDK_XML_Append_Struct(psWANAccessStatuses, HOTSPOT_Element_Cisco_HotSpot_WANAccessStatus)) != 0)
                        {
                            HDK_XML_Set_MACAddress(psWANAccessStatus, HOTSPOT_Element_Cisco_HotSpot_MACAddress, pMAC);
                            HDK_XML_Set_IPAddress(psWANAccessStatus, HOTSPOT_Element_Cisco_HotSpot_IPAddress, pIP);
                            HDK_XML_Set_Bool(psWANAccessStatus, HOTSPOT_Element_Cisco_HotSpot_HasWANAccess, 0);
                            HDK_XML_Set_Bool(psWANAccessStatus, HOTSPOT_Element_Cisco_HotSpot_OnGuestNetwork, 0);
                        }
                    }

                    pMember = (HDK_XML_Member*)psWANAccessStatuses;
                }

                /* Free the temporary struct */
                HDK_XML_Struct_Free(&sTemp);
            }
            break;

        default:
            break;
    }

    return pMember;
}

/* Device HOTSPOT ADISet implementation, returns -1 for ADI values it does not know about */
static int s_HOTSPOT_SRV_Device_ADISet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                                       HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    /* Return defaults to -1 for ADI values that are not handled */
    int iReturn = -1;

    /* Unused variabled */
    (void) pMethodCtx;
    (void) value;
    (void) pStruct;
    (void) element;

    return iReturn;
}

const char* HDK_SRV_Device_ADIGet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                                  const char* pszName)
{
    const char* pszValue = 0;
    HDK_SRV_ADIValue value;
    const HDK_MOD_Module* pHNAP12Module = HNAP12_Module();
    const HDK_MOD_Module* pHOTSPOTModule = HOTSPOT_Module();
    const HDK_MOD_Module* pModule = 0;

    /* Find the ADI schema node */
    if (HDK_SRV_ADIGetValue(&value, pHNAP12Module, pszNamespace, pszName))
    {
        pModule = pHNAP12Module;
    }
    else if (HDK_SRV_ADIGetValue(&value, pHOTSPOTModule, pszNamespace, pszName))
    {
        pModule = pHOTSPOTModule;
    }

    if (pModule != 0)
    {
        const HDK_XML_SchemaNode* pSchemaNode = HDK_MOD_ADISchemaNode(pModule, value);
        HDK_XML_Member* pMember;

        /* Initialize the temporary struct */
        HDK_XML_Struct sTemp;
        HDK_XML_Struct_Init(&sTemp);

        if (pModule == pHNAP12Module)
        {
            pMember = s_HNAP12_SRV_Device_ADIGet(pMethodCtx, value, &sTemp, pSchemaNode->element);
        }
        else
        {
            pMember = s_HOTSPOT_SRV_Device_ADIGet(pMethodCtx, value, &sTemp, pSchemaNode->element);
        }

        if (pMember)
        {
            pszValue = HDK_SRV_ADISerialize(pModule, value, &sTemp, pSchemaNode->element);
        }

        /* Free the temporary struct */
        HDK_XML_Struct_Free(&sTemp);
    }

    /* A known ADI value get failed so try auto-provisioning */
    if (pszValue == 0)
    {
        char pszBuffer[UTOPIA_BUF_SIZE] = {'\0'};

        if (Utopia_RawGet(pUTCtx(pMethodCtx), (char*)pszNamespace, (char*)pszName, pszBuffer, sizeof(pszBuffer)) != 0)
        {
            pszValue = HDK_SRV_ADISerializeCopy(pszBuffer);
        }
    }

    return pszValue;
}

int HDK_SRV_Device_ADISet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                          const char* pszName, const char* pszValue)
{
    int iResult = 0;
    HDK_SRV_ADIValue value;
    const HDK_MOD_Module* pHNAP12Module = HNAP12_Module();
    const HDK_MOD_Module* pHOTSPOTModule = HOTSPOT_Module();
    const HDK_MOD_Module* pModule = 0;

    /* Find the ADI schema node */
    if (HDK_SRV_ADIGetValue(&value, pHNAP12Module, pszNamespace, pszName))
    {
        pModule = pHNAP12Module;
    }
    else if (HDK_SRV_ADIGetValue(&value, pHOTSPOTModule, pszNamespace, pszName))
    {
        pModule = pHOTSPOTModule;
    }

    if (pModule != 0)
    {
        HDK_XML_Member* pMember;
        const HDK_XML_SchemaNode* pSchemaNode = HDK_MOD_ADISchemaNode(pModule, value);

        /* Initialize the temporary struct */
        HDK_XML_Struct sTemp;
        HDK_XML_Struct_Init(&sTemp);

        /* Deserialize the value */
        pMember = HDK_SRV_ADIDeserialize(pModule, value, &sTemp, pSchemaNode->element, pszValue);

        if (pMember)
        {
            if (pModule == pHNAP12Module)
            {
                /* Set the ADI value member */
                iResult = s_HNAP12_SRV_Device_ADISet(pMethodCtx, value, &sTemp, pSchemaNode->element);
            }
            else
            {
                /* Set the ADI value member */
                iResult = s_HOTSPOT_SRV_Device_ADISet(pMethodCtx, value, &sTemp, pSchemaNode->element);
            }
        }

        /* Free the temporary struct */
        HDK_XML_Struct_Free(&sTemp);
    }

    /* A result less than 0 that we don't know about this ADI value, so let's auto-provision it */
    if (iResult < 0)
    {
        iResult = Utopia_RawSet(pUTCtx(pMethodCtx), (char*)pszNamespace, (char*)pszName, (char*)pszValue);
    }

    return iResult;
}
