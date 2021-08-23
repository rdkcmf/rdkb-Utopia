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
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_wlan.h"
#include "safec_lib_common.h"
// extern char ulog_msg[1024];

const wifiPlatformSetup_t wifiPlatform[WIFI_INTERFACE_COUNT] =  
{
    {FREQ_2_4_GHZ, "wl0", "eth0"},
    {FREQ_5_GHZ,   "wl1", "eth1"},
};


static EnumString_Map g_wifiModeMap[] =
{
    { "b",          B_ONLY },
    { "g",          G_ONLY },
    { "a",          A_ONLY },
    { "n",          N_ONLY },
    { "bg",      B_G_MIXED },
    { "bgn",  B_G_N_MIXED },
    { "an",      A_N_MIXED },
    { 0, 0 }
};

static EnumString_Map g_wifiBandMap[] =
{
    { "auto",         BAND_AUTO },
    { "standard",     STD_20MHZ },
    { "wide",         WIDE_40MHZ },
    { 0, 0 }
};


#ifndef UTAPI_NO_SEC_MACFILTER

static EnumString_Map g_wifiEncryptMap[] =
{
    { "tkip",         WPA_ENCRYPT_TKIP },
    { "aes",          WPA_ENCRYPT_AES },
    { "tkip+aes",     WPA_ENCRYPT_TKIP_AES },
    { 0, 0 }
};

static EnumString_Map g_wifiSecMap[] =
{
    { "disabled",        WIFI_SECURITY_DISABLED, },
    { "wep",             WIFI_SECURITY_WEP },
    { "wpa-personal",    WIFI_SECURITY_WPA_PERSONAL, },
    { "wpa-enterprise",  WIFI_SECURITY_WPA_ENTERPRISE, },
    { "wpa2-personal",   WIFI_SECURITY_WPA2_PERSONAL, },
    { "wpa2-enterprise", WIFI_SECURITY_WPA2_ENTERPRISE, },
    { "radius",          WIFI_SECURITY_RADIUS },
    { 0, 0 }
};

static EnumString_Map g_wifiMacFilterMap[] =
{
    { "disabled",   MAC_FILTER_DISABLED },
    { "allow",      MAC_FILTER_ALLOW },
    { "deny",       MAC_FILTER_DENY },
    { 0, 0 }
};

#endif // UTAPI_NO_SEC_MACFILTER

static EnumString_Map g_wifiBasicRateMap[] =
{
    { "1-2mbps",   WIFI_BASICRATE_1_2MBPS },
    { "all",       WIFI_BASICRATE_ALL },
    { "default",   WIFI_BASICRATE_DEFAULT },
    { 0, 0 }
};

static EnumString_Map g_wifiAuthTypeMap[] =
{
    { "open_system",   WIFI_AUTH_OPENSYSTEM },
    { "shared_key",    WIFI_AUTH_SHAREDKEY },
    { "auto",          WIFI_AUTH_AUTO },
    { 0, 0 }
};


static EnumString_Map g_wifiTranmissionRateMap[] =
{
    { "6000000",   WIFI_TX_RATE_6  },
    { "9000000",   WIFI_TX_RATE_9  },
    { "12000000",  WIFI_TX_RATE_12 },
    { "18000000",  WIFI_TX_RATE_18 },
    { "24000000",  WIFI_TX_RATE_24 },
    { "36000000",  WIFI_TX_RATE_36 },
    { "48000000",  WIFI_TX_RATE_48 },
    { "54000000",  WIFI_TX_RATE_54 },
    { "auto",      WIFI_TX_RATE_AUTO },
    { 0, 0 }
};


static EnumString_Map g_wifiPowerMap[] =
{
    { "low",    TX_POWER_LOW },
    { "medium", TX_POWER_MEDIUM },
    { "high",   TX_POWER_HIGH },
    { 0, 0 }
}; 

int Utopia_SetWifiRadioState (UtopiaContext *ctx, wifiInterface_t intf, boolean_t enable)
{
    char *prefix;

    if (NULL == ctx || intf >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    /*
     * Get ifconfig interface name such as eth0, wlan1, etc.
     */
    prefix = wifiPlatform[intf].syscfg_namespace_prefix;

    if (TRUE == enable) {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_State, prefix, "up");
    } else {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_State, prefix, "down");
    }
    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return UT_SUCCESS;
}

int Utopia_GetWifiRadioState (UtopiaContext *ctx, wifiInterface_t intf, boolean_t *enable)
{
    char value[64] = {0};
    char *prefix;
    errno_t safec_rc = -1;
    if (NULL == ctx || NULL == enable || intf >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    /*
     * Get ifconfig interface name such as eth0, wlan1, etc.
     */
    prefix = wifiPlatform[intf].syscfg_namespace_prefix;

    *enable = FALSE;

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_State, prefix, value, sizeof(value))) {
        if (0 == strcasecmp("up", value)) {
            *enable = TRUE;
        }
    } else {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_state: syscfg get %s_state error", prefix);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }

    return UT_SUCCESS;
}

/*
 * Get radio paramters
 *   return 0 -- success
 *          1 -- failure
 */
int Utopia_GetWifiRadioSettings (UtopiaContext *ctx, wifiInterface_t intf, wifiRadioInfo_t *info) 
{
    char value[64];
    char *prefix;
    errno_t safec_rc = -1;
    if (NULL == ctx || NULL == info || intf >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    bzero(info, sizeof(wifiRadioInfo_t));
    info->interface = intf;

    /*
     * Get ifconfig interface name such as eth0, wlan1, etc.
     */
    prefix = wifiPlatform[intf].syscfg_namespace_prefix;

    /* 
     * Get 802.11 Mode 
     */
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_NetworkMode, prefix, value, sizeof(value))) {
        info->mode = s_StrToEnum(g_wifiModeMap, value);
        if (info->mode == -1) {
            safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_settings: invalid %s_network_mode value %s", prefix, value);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            return ERR_WIFI_INVALID_MODE;
        } 
    } else {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_settings: syscfg get %s_network_mode error", prefix);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }
  
    /* 
     * Get Radio Band
     */
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_RadioBand, prefix, value, sizeof(value))) {
        info->band = s_StrToEnum(g_wifiBandMap, value);
        if (info->band == -1) {
            safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_settings: invalid %s_radio_band", prefix);
            if(safec_rc < EOK){
               ERR_CHK(safec_rc);
            }
            ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            return ERR_WIFI_INVALID_MODE;
        }
    } else {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_settings: syscfg get %s_radio_band error", prefix);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }

    /* 
     * Get Radio Channel 
     */
    
    if (!Utopia_GetNamed(ctx, UtopiaValue_WLAN_Channel, prefix, value, sizeof(value))) {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_settings: syscfg get %s_channel error", prefix);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }
    info->channel = atoi(value); 

    /* 
     * Get Channel's SideBand, applicable only for N-mode(s)
     */
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_SideBand, prefix, value, sizeof(value))) {
        info->sideband = (0 == strcasecmp(value, "upper")) ? SIDEBAND_UPPER : SIDEBAND_LOWER;
    }

    /* 
     * Get SSID
     */
    if (!Utopia_GetNamed(ctx, UtopiaValue_WLAN_SSID, prefix, info->ssid, SSID_SIZE)) {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_settings: syscfg get %s_ssid error", prefix);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }

    /* 
     * Get SSID broadcast
     */
    if (!Utopia_GetNamed(ctx, UtopiaValue_WLAN_SSIDBroadcast, prefix, value, sizeof(value))) {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_radio_settings: syscfg get %s_ssid_broadcast error", prefix);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }
    info->ssid_broadcast = atoi(value);

    s_get_interface_mac(wifiPlatform[intf].ifconfig_interface, info->mac_address, MAC_ADDR_SIZE);
    ulogf(ULOG_CONFIG, UL_UTAPI, "%s: interface %d:%s, mac:%s", __FUNCTION__, intf, wifiPlatform[intf].ifconfig_interface, info->mac_address);

    return SUCCESS;
}

int Utopia_SetWifiRadioSettings (UtopiaContext *ctx, wifiRadioInfo_t *info) 
{
    char *prefix, *token;

    if (NULL == ctx || NULL == info || info->interface >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    /*
     * Get ifconfig interface name such as eth0, wlan1, etc.
     */
    prefix = wifiPlatform[info->interface].syscfg_namespace_prefix;

    /* 
     * 802.11 Mode 
     */
    if ((token = s_EnumToStr(g_wifiModeMap, info->mode))) {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_NetworkMode, prefix, token);
    } else {
        return ERR_INVALID_VALUE; 
    }

    /* 
     * Radio Band
     */
    if ((token = s_EnumToStr(g_wifiBandMap, info->band))) {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_RadioBand, prefix, token);
    } else {

        return ERR_INVALID_VALUE; 
    } 

    UTOPIA_SETNAMEDINT(ctx, UtopiaValue_WLAN_Channel, prefix, info->channel);
    if (SIDEBAND_UPPER == info->sideband) {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_SideBand, prefix, "upper");
    } else {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_SideBand, prefix, "lower");
    }

    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_SSID, prefix, info->ssid);

    UTOPIA_SETNAMEDBOOL(ctx, UtopiaValue_WLAN_SSIDBroadcast, prefix, info->ssid_broadcast);

    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return SUCCESS;
}

#ifndef UTAPI_NO_SEC_MACFILTER

/*
 * Get security paramters
 *   return 0 -- success
 *          1 -- failure
 */
int Utopia_GetWifiSecuritySettings (UtopiaContext *ctx, wifiInterface_t intf, wifiSecurityInfo_t *info) 
{
    char value[64];
    char passphrase[PASSPHRASE_SZ] = {0};
    char encrypt[64] = {0};
    errno_t safec_rc = -1;

    if (NULL == ctx || NULL == info || intf >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    char *prefix = wifiPlatform[intf].syscfg_namespace_prefix;

    bzero(info, sizeof(wifiSecurityInfo_t));
    info->interface = intf;

    /*
     *  Get security mode (i.e. WEP, WPA_PERSONAL, etc.)
     */
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_SecurityMode, prefix, value, sizeof(value))) {
        info->mode = s_StrToEnum(g_wifiSecMap, value);
        if (-1 == info->mode) {
            return ERR_INVALID_VALUE;
        }
    } else {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_security_settings: syscfg get %s_security_mode error", prefix);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }

    /*
     * Return if security is disabled.
     */
    if (info->mode == WIFI_SECURITY_DISABLED) {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_security_settings: security disabled");
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_debug(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return SUCCESS;
    }

    Utopia_GetNamed(ctx, UtopiaValue_WLAN_Passphrase, prefix, info->passphrase, sizeof(passphrase));
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_Encryption, prefix, encrypt, sizeof(encrypt))) {
        info->encrypt = s_StrToEnum(g_wifiEncryptMap, encrypt);
        if (-1 == info->encrypt) {
            safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wifi config: invalid encrption %s_encryption [%s]", prefix, encrypt);
            if(safec_rc < EOK){
               ERR_CHK(safec_rc);
            }
            ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        }
    }

    Utopia_GetNamed(ctx, UtopiaValue_WLAN_Key0, prefix, info->wep_key[0], WEP_KEY_SZ);
    Utopia_GetNamed(ctx, UtopiaValue_WLAN_Key1, prefix, info->wep_key[1], WEP_KEY_SZ);
    Utopia_GetNamed(ctx, UtopiaValue_WLAN_Key2, prefix, info->wep_key[2], WEP_KEY_SZ);
    Utopia_GetNamed(ctx, UtopiaValue_WLAN_Key3, prefix, info->wep_key[3], WEP_KEY_SZ);
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_TxKey, prefix, value, sizeof(value))) {
        info->wep_txkey = atoi(value);
    }
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_KeyRenewal, prefix, value, sizeof(value))) {
        info->key_renewal_interval = atoi(value);
    }

    Utopia_GetNamed(ctx, UtopiaValue_WLAN_Shared, prefix, info->shared_key, SHAREDKEY_SZ);
    Utopia_GetNamed(ctx, UtopiaValue_WLAN_RadiusServer, prefix, info->radius_server, IPADDR_SZ);
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_RadiusPort, prefix, value, sizeof(value))) {
        info->radius_port = atoi(value);
    }

    // TODO: a bit of validation ...
    
    return SUCCESS;
}

int Utopia_SetWifiSecuritySettings (UtopiaContext *ctx, wifiSecurityInfo_t *info) 
{
    char *token, value[64];
    errno_t safec_rc = -1;

    if (NULL == ctx || NULL == info || info->interface >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    char *prefix = wifiPlatform[info->interface].syscfg_namespace_prefix;

    /*
     *  Set security mode (i.e. WEP, WPA_PERSONAL, etc.)
     */
    if ((token = s_EnumToStr(g_wifiSecMap, info->mode))) {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_SecurityMode, prefix, token);
    } else {
        return ERR_INVALID_VALUE;
    }
    
    /*
     * Return if security is disabled.
     */
    if (info->mode == WIFI_SECURITY_DISABLED) {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"wlan_security_settings: security disabled");
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_debug(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return SUCCESS;
    }

    Utopia_SetNamed(ctx, UtopiaValue_WLAN_Passphrase, prefix, info->passphrase);
    if ((token = s_EnumToStr(g_wifiEncryptMap, info->encrypt))) {
        Utopia_SetNamed(ctx, UtopiaValue_WLAN_Encryption, prefix, token);
    }

    if (info->mode == WIFI_SECURITY_WEP)
    {
      UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Key0, prefix, info->wep_key[0]);
 
      UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Key1, prefix, info->wep_key[1]);
      UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Key2, prefix, info->wep_key[2]);
      UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Key3, prefix, info->wep_key[3]);

      snprintf(value, sizeof(value), "%d", info->wep_txkey);
      UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_TxKey, prefix, value);
    }
    snprintf(value, sizeof(value), "%d", info->key_renewal_interval);
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_KeyRenewal, prefix, value);

    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Shared, prefix, info->shared_key);
    if (info->mode == WIFI_SECURITY_RADIUS)
    {
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_RadiusServer, prefix, info->radius_server);

    snprintf(value, sizeof(value), "%d", info->radius_port);
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_RadiusPort, prefix, value);
    }
    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return SUCCESS;
}


int Utopia_GetWifiMacFilters (UtopiaContext *ctx, wifiMacFilterInfo_t *info) 
{
    char value[32];

    if (NULL == ctx || NULL == info ) {
        return ERR_INVALID_ARGS;
    }

    bzero(info, sizeof(wifiMacFilterInfo_t));

    info->macFilterMode = MAC_FILTER_DISABLED;
    if (Utopia_Get(ctx, UtopiaValue_WLAN_AccessRestriction, value, sizeof(value))) {
        info->macFilterMode = s_StrToEnum(g_wifiMacFilterMap, value);
        if (-1 == info->macFilterMode) {
            return ERR_INVALID_VALUE;
        }
    }

    char *n, maclist[1024];
    if (Utopia_Get(ctx, UtopiaValue_WLAN_MACFilter, maclist, sizeof(maclist))) {
        int count = 0;
        char *p = maclist;
        while (count < MAX_MACFILTERS && (n = strsep(&p, " "))) {
	    /*CID 135348 : BUFFER_SIZE_WARNING */
            strncpy(info->client_mac_list[count], n, sizeof(info->client_mac_list[count])-1);
	    info->client_mac_list[count][sizeof(info->client_mac_list[count])-1] = '\0';
            count++;
        }
        info->client_count = count;
    }

    return SUCCESS;
}

int Utopia_SetWifiMacFilters (UtopiaContext *ctx, wifiMacFilterInfo_t *info) 
{
    if (NULL == ctx || NULL == info) {
        return ERR_INVALID_ARGS;
    }

    char *token = s_EnumToStr(g_wifiMacFilterMap, info->macFilterMode);
    if (token) {
        UTOPIA_SET(ctx, UtopiaValue_WLAN_AccessRestriction, token);
    } else {
        return ERR_INVALID_VALUE;
    }

    if (MAC_FILTER_DISABLED != info->macFilterMode) {
        int i;
        char maclist[1024] = {0};
        errno_t safec_rc = -1;
        for(i = 0; i < info->client_count; i++) {
            safec_rc = strcat_s(maclist, sizeof(maclist),info->client_mac_list[i]);
            ERR_CHK(safec_rc);
            if (i < info->client_count - 1) {
                safec_rc = strcat_s(maclist, sizeof(maclist)," ");
                ERR_CHK(safec_rc);
            }
        }
        UTOPIA_SET(ctx, UtopiaValue_WLAN_MACFilter, maclist);
    }

    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return SUCCESS;
}

#endif // UTAPI_NO_SEC_MACFILTER

int Utopia_GetWifiAdvancedSettings (UtopiaContext *ctx, wifiInterface_t intf, wifiAdvancedInfo_t *info) 
{
    char value[64];

    if (NULL == ctx || NULL == info || intf >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    char *prefix = wifiPlatform[intf].syscfg_namespace_prefix;

    bzero(info, sizeof(wifiAdvancedInfo_t));
    info->interface = intf;
    
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_ApIsolation, prefix, value, sizeof(value))) {
        info->ap_isolation = (!(strcasecmp(value, "enabled"))) ? TRUE : FALSE;
    }

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_FrameBurst, prefix, value, sizeof(value))) {
        info->frame_burst = (!(strcasecmp(value, "disabled"))) ? FALSE : TRUE;
    }

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_AuthenticationType, prefix, value, sizeof(value))) {
        info->auth_type = s_StrToEnum(g_wifiAuthTypeMap, value);
    }

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_BasicRate, prefix, value, sizeof(value))) {
        info->basic_rate = s_StrToEnum(g_wifiBasicRateMap, value);
    }

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_TransmissionRate, prefix, value, sizeof(value))) {
        info->tx_rate = s_StrToEnum(g_wifiTranmissionRateMap, value);
    }

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_N_TransmissionRate, prefix, value, sizeof(value))) {
        info->n_tx_rate = (!(strcmp(value, "auto"))) ? WIFI_N_TX_RATE_AUTO : atoi(value);
    }

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_TransmissionPower, prefix, value, sizeof(value))) {
        info->tx_power = s_StrToEnum(g_wifiPowerMap, value);
    }

    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_CTSProtectionMode, prefix, value, sizeof(value))) {
        info->auto_cts_protect_mode = (!(strcmp(value, "auto"))) ? TRUE : FALSE;
    }
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_BeaconInterval, prefix, value, sizeof(value))) {
        info->beacon_interval = atoi(value);
    }
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_DTIMInterval, prefix, value, sizeof(value))) {
        info->dtim_interval= atoi(value);
    }
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_FragmentationThreshold, prefix, value, sizeof(value))) {
        info->frag_threshold = atoi(value);
    }
    if (Utopia_GetNamed(ctx, UtopiaValue_WLAN_RTSThreshold, prefix, value, sizeof(value))) {
        info->rts_threshold = atoi(value);
    }

    return SUCCESS;
}

int Utopia_SetWifiAdvancedSettings (UtopiaContext *ctx, wifiAdvancedInfo_t *info) 
{
    char value[64], *token;

    if (NULL == ctx || NULL == info || info->interface >= WIFI_INTERFACE_COUNT) {
        return ERR_INVALID_ARGS;
    }

    char *prefix = wifiPlatform[info->interface].syscfg_namespace_prefix;

    token = (info->ap_isolation) ? "enabled" : "disabled";
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_ApIsolation, prefix, token);

    token = (info->frame_burst) ? "enabled" : "disabled";
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_FrameBurst, prefix, token);

    token = s_EnumToStr(g_wifiAuthTypeMap, info->auth_type);
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_AuthenticationType, prefix, token);
       
    token = s_EnumToStr(g_wifiBasicRateMap, info->basic_rate);
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_BasicRate, prefix, token);

    token = s_EnumToStr(g_wifiTranmissionRateMap, info->tx_rate);
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_TransmissionRate, prefix, token);

    if (WIFI_N_TX_RATE_AUTO == info->n_tx_rate) {
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_N_TransmissionRate, prefix, "auto");
    } else {
        snprintf(value, sizeof(value), "%d", info->n_tx_rate);
        UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_N_TransmissionRate, prefix, value);
    }

    token = s_EnumToStr(g_wifiPowerMap, info->tx_power);
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_TransmissionPower, prefix, token);

    token = info->auto_cts_protect_mode ? "auto" : "disabled";
    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_CTSProtectionMode, prefix, token);

    UTOPIA_SETNAMEDINT(ctx, UtopiaValue_WLAN_BeaconInterval, prefix, info->beacon_interval);
    UTOPIA_SETNAMEDINT(ctx, UtopiaValue_WLAN_DTIMInterval, prefix, info->dtim_interval);
    UTOPIA_SETNAMEDINT(ctx, UtopiaValue_WLAN_FragmentationThreshold, prefix, info->frag_threshold);
    UTOPIA_SETNAMEDINT(ctx, UtopiaValue_WLAN_RTSThreshold, prefix, info->rts_threshold);

    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return SUCCESS;
}

int Utopia_SetWifiQoSSettings (UtopiaContext *ctx, wifiQoS_t *wifiqos)
{
    char *token;
    
    token = (wifiqos->wmm_support) ? "enabled" : "disabled";
    UTOPIA_SET(ctx, UtopiaValue_WLAN_WMMSupport, token);
    token = (wifiqos->no_acknowledgement) ? "enabled" : "disabled";
    UTOPIA_SET(ctx, UtopiaValue_WLAN_NoAcknowledgement, token);

    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return UT_SUCCESS;
}

int Utopia_GetWifiQoSSettings (UtopiaContext *ctx, wifiQoS_t *wifiqos)
{
    char value[64];
    
    if (Utopia_Get(ctx, UtopiaValue_WLAN_WMMSupport, value, sizeof(value))) {
        wifiqos->wmm_support = (!(strcasecmp(value, "enabled"))) ? TRUE : FALSE;
    }
    if (Utopia_Get(ctx, UtopiaValue_WLAN_NoAcknowledgement, value, sizeof(value))) {
        wifiqos->no_acknowledgement = (!(strcasecmp(value, "enabled"))) ? TRUE : FALSE;
    }

    return UT_SUCCESS;
}

int Utopia_GetWifiConfigMode (UtopiaContext *ctx, wifiConfigMode_t *config_mode)
{
    char value[64];
    
    if (NULL == ctx || NULL == config_mode) {
        return ERR_INVALID_ARGS;
    }
    
    if (Utopia_Get(ctx, UtopiaValue_WLAN_ConfigMode, value, sizeof(value))) {
        if (0 == strcasecmp(value, "manual")) {
            *config_mode = WIFI_CONFIG_MANUAL;
        }
        else if (0 == strcasecmp(value, "wps")) {
            *config_mode = WIFI_CONFIG_WPS;
        }
        else {
            return ERR_WIFI_INVALID_CONFIG_MODE;
        }
    }
    else {
        *config_mode = WIFI_CONFIG_MANUAL;
    }

    return UT_SUCCESS;
}

int Utopia_SetWifiConfigMode (UtopiaContext *ctx, wifiConfigMode_t config_mode)
{
    char *token;
    
    if (NULL == ctx) {
        return ERR_INVALID_ARGS;
    }
    
    switch (config_mode) {
    case WIFI_CONFIG_MANUAL:
        token = "manual";
        break;
    case WIFI_CONFIG_WPS:
        token = "wps";
        break;
    default:
        return ERR_WIFI_INVALID_CONFIG_MODE;
    }
    
    UTOPIA_SET(ctx, UtopiaValue_WLAN_ConfigMode, token);

    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return UT_SUCCESS;
}

int Utopia_WPSPushButtonStart (void)
{
    char cmd[512];

    snprintf(cmd, sizeof(cmd), "wlancfg eth0 wps-stop");
    cmd[sizeof(cmd) - 1] = '\0';
    system(cmd);
    snprintf(cmd, sizeof(cmd), "wlancfg eth0 wps-pbc-start");
    cmd[sizeof(cmd) - 1] = '\0';
    system(cmd);

    return SUCCESS;
}

int Utopia_WPSPinStart (int pin)
{
    char cmd[512];

    snprintf(cmd, sizeof(cmd), "wlancfg eth0 wps-pin-start %d", pin);
    cmd[sizeof(cmd) - 1] = '\0';
    system(cmd);

    return SUCCESS;
}

int Utopia_WPSStop (void)
{
    char cmd[512];

    snprintf(cmd, sizeof(cmd), "wlancfg eth0 wps-stop");
    cmd[sizeof(cmd) - 1] = '\0';
    system(cmd);

    return SUCCESS;
}

int Utopia_GetWPSStatus (wpsStatus_t *wps_status)
{
    if (NULL == wps_status) {
        return ERR_INVALID_ARGS;
    }
    
    char cmd[512];
    FILE *fp;
    char wps_status_str[32];
    errno_t safec_rc = -1;

    safec_rc = strcpy_s(cmd, sizeof(cmd),"wlancfg eth0 wps-status");
    ERR_CHK(safec_rc);
    fp = popen(cmd, "r");
    
    if (NULL == fp) {
        return ERR_FILE_NOT_FOUND;
    }
    
    if (NULL == fgets(wps_status_str, sizeof(wps_status_str), fp)) {
        pclose(fp);
        return ERR_INVALID_VALUE;
    }
    
    int len = strlen(wps_status_str);
    
    if (len <= 0) {
        pclose(fp);
        return ERR_INVALID_VALUE;
    }
    
    // Remove a trailing newline
    if (wps_status_str[len - 1] == '\n') {
        wps_status_str[len - 1] = '\0';
    }
    
    if (0 == strcmp(wps_status_str, "wps_stopped")) {
        *wps_status = WPS_STOPPED;
    }
    else if (0 == strcmp(wps_status_str, "wps_in_process")) {
        *wps_status = WPS_IN_PROGRESS;
    }
    else if (0 == strcmp(wps_status_str, "wps_success")) {
        *wps_status = WPS_SUCCESS;
    }
    else {
        *wps_status = WPS_FAILED;
    }
    
    pclose(fp);

    return SUCCESS;
}

int Utopia_GetWifiBridgeSettings (UtopiaContext *ctx, wifiBridgeInfo_t *info)
{
    int bridge_mode;

    if (NULL == ctx || NULL == info) {
        return ERR_INVALID_ARGS;
    }

    if (SUCCESS == Utopia_GetInt(ctx, UtopiaValue_WLAN_BridgeMode, &bridge_mode)) {
        switch (bridge_mode) {
        case 1:
            info->mode = WIFI_BRIDGE_WDS_ENABLED;
            break;
        case 2:
            info->mode = WIFI_BRIDGE_STA_ENABLED;
            break;
        default:
            info->mode = WIFI_BRIDGE_DISABLED;
            break;
        }
    }
    else {
        info->mode = WIFI_BRIDGE_DISABLED;
    }

    if (!Utopia_Get(ctx, UtopiaValue_WLAN_BridgeSSID, info->bssid, MACADDR_SZ)) {
        info->bssid[0] = '\0';
    }

    return UT_SUCCESS;
}

int Utopia_SetWifiBridgeSettings (UtopiaContext *ctx, wifiBridgeInfo_t *info)
{
    if (NULL == ctx || NULL == info) {
        return ERR_INVALID_ARGS;
    }

    switch (info->mode) {
    case WIFI_BRIDGE_WDS_ENABLED:
        UTOPIA_SETINT(ctx, UtopiaValue_WLAN_BridgeMode, 1);
        break;
    case WIFI_BRIDGE_STA_ENABLED:
        UTOPIA_SETINT(ctx, UtopiaValue_WLAN_BridgeMode, 2);
        break;
    default:
        UTOPIA_SETINT(ctx, UtopiaValue_WLAN_BridgeMode, 0);
        break;
    }

    UTOPIA_SET(ctx, UtopiaValue_WLAN_BridgeSSID, info->bssid);

    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return UT_SUCCESS;
}
