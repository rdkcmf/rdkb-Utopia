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

 /*Copyright (c) 2000 Torsten Landschoff <torsten@debian.org>
 		           Sebastian Krahmer  <krahmer@cs.uni-potsdam.de>
License: BSD

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_tr_wlan.h"
#include "DM_TR181.h"
#include <arpa/inet.h>
#include "utapi_tr_wlan.h"
#include "safec_lib_common.h"

const wifiTRPlatformSetup_t wifiTRPlatform[] =
{
    {FREQ_2_4_GHZ, "wl0", "eth0", "SSID0", "ap0"},
    {FREQ_5_GHZ,   "wl1", "eth1", "SSID1", "ap1"}
};

static wifiTRPlatformSetup_t wifiTRPlatform_multiSSID[(MAX_SSID_PER_RADIO * 2)] =
{
    {FREQ_2_4_GHZ, "wl0",   "eth0",  "SSID0", "ap0"},
    {FREQ_5_GHZ,   "wl1",   "eth1",  "SSID1", "ap1"},
    {FREQ_2_4_GHZ, "wl0.1",   "eth0",  "SSID2", "ap2"},
    {FREQ_2_4_GHZ, "wl0.2",   "eth0",  "SSID3", "ap3"},
    {FREQ_2_4_GHZ, "wl0.3",   "eth0",  "SSID4", "ap4"},
    {FREQ_5_GHZ,   "wl1.1",   "eth1",  "SSID5", "ap5"},
    {FREQ_5_GHZ,   "wl1.2",   "eth1",  "SSID6", "ap6"},
    {FREQ_5_GHZ,   "wl1.3",   "eth1",  "SSID7", "ap7"}
};

static int g_IndexMapRadio[MAX_NUM_INSTANCES+1] = {-1};
static int g_IndexMapSSID[MAX_NUM_INSTANCES+1] = {-1};
static int g_IndexMapAP[MAX_NUM_INSTANCES+1] = {-1};

const int FREQ_5_GHZ_CHANNELS[] = {0, 36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165};

int Utopia_GetWifiRadioInstances()
{

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    return WIFI_RADIO_NUM_INSTANCES;
}

int Utopia_GetWifiRadioEntry(UtopiaContext *ctx, unsigned long ulIndex, void *pEntry)
{
    if((NULL == ctx) || (NULL == pEntry)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

   wifiRadioEntry_t *pEntry_t = ( wifiRadioEntry_t *)pEntry;

   Utopia_GetIndexedWifiRadioCfg(ctx, ulIndex, &(pEntry_t->Cfg));
   Utopia_GetWifiRadioSinfo(ulIndex, &(pEntry_t->StaticInfo));
   Utopia_GetIndexedWifiRadioDinfo(ctx, ulIndex, &(pEntry_t->DynamicInfo));
   return SUCCESS;
}

int Utopia_GetIndexedWifiRadioCfg(UtopiaContext *ctx, unsigned long ulIndex, void *cfg)
{
    char *prefix;
    int iVal = -1;
    if((NULL == ctx) || (NULL == cfg)){
        return ERR_INVALID_ARGS;
    }

    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiRadioCfg_t *cfg_t = (wifiRadioCfg_t *)cfg;

    prefix = wifiTRPlatform[ulIndex].syscfg_namespace_prefix;

    /* Check if we have an InstanceNumber stored for this Radio */
    if(Utopia_GetNamedInt(ctx,UtopiaValue_WLAN_Radio_Instance_Num,prefix,&iVal))
    {
        /* Failed to find an InstanceNumber in syscfg. This is during initialization*/
        cfg_t->InstanceNumber = (ulIndex + 1);
        g_IndexMapRadio[ulIndex+1] = ulIndex;
        Utopia_GetWifiRadioCfg(ctx,1,cfg_t);
    }
    else
    {
        /* Found the Instance number in syscfg */
        if(iVal > MAX_NUM_INSTANCES)
           g_IndexMapRadio[(iVal % MAX_NUM_INSTANCES)] = ulIndex;
        else
           g_IndexMapRadio[iVal] = ulIndex;
        cfg_t->InstanceNumber = iVal;
        Utopia_GetWifiRadioCfg(ctx,0,cfg_t);
    }
    return SUCCESS;
}

int Utopia_GetWifiRadioCfg(UtopiaContext *ctx, int dummyInstanceNum, void *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }
    char ulog_msg[256];
    wifiRadioCfg_t *cfg_t = (wifiRadioCfg_t *)cfg;
    char buf[BUF_SZ] = {'\0'};
    char strbuf[STR_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    unsigned long ulVal = 0;
    char *prefix;
    unsigned long ulIndex;
    errno_t safec_rc = -1;

#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if(cfg_t->InstanceNumber > MAX_NUM_INSTANCES)
        ulIndex = g_IndexMapRadio[(cfg_t->InstanceNumber % MAX_NUM_INSTANCES)];
    else
        ulIndex = g_IndexMapRadio[cfg_t->InstanceNumber];

    if(dummyInstanceNum)
        cfg_t->InstanceNumber = 0;

    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }
 
    prefix =  wifiTRPlatform[ulIndex].syscfg_namespace_prefix ;

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s | grep Radio |cut -d'}' -f2- -s |cut -d. -f2- -s > %s ", wifiTRPlatform[ulIndex].ifconfig_interface, WLANCFG_RADIO_FULL_FILE);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    system(buf);
    system("cat " WLANCFG_RADIO_FULL_FILE " | grep Extensions | cut -d. -f2- -s > " WLANCFG_RADIO_EXTN_FILE);
    system("cat " WLANCFG_RADIO_FULL_FILE " | sed -e '/Extensions/{N;d;}'|sed -e '/Stats/{N;d;}' > " WLANCFG_RADIO_FILE);

    retVal = file_parse(WLANCFG_RADIO_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127,CID-32897, free unused resources before exit */
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "OperatingFrequencyBand")){
            cfg_t->OperatingFrequencyBand = atoi(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "OperatingStandards")){
            cfg_t->OperatingStandards = 0;
            if(strchr(ptr->param_val,'a')!=NULL)
                cfg_t->OperatingStandards |= WIFI_STD_a;
            if(strchr(ptr->param_val,'b')!=NULL)
                cfg_t->OperatingStandards |= WIFI_STD_b;
            if(strchr(ptr->param_val,'g')!=NULL)
                cfg_t->OperatingStandards |= WIFI_STD_g;
            if(strchr(ptr->param_val,'n')!=NULL)
                cfg_t->OperatingStandards |= WIFI_STD_n;
        }else if(!strcasecmp(ptr->param_name, "Channel")){
            cfg_t->Channel = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "AutoChannelEnable")){
            cfg_t->AutoChannelEnable = (0 == atoi(ptr->param_val))? FALSE : TRUE ;
        }else if(!strcasecmp(ptr->param_name, "AutoChannelRefreshPeriod")){
            if(atoi(ptr->param_val) < 1)
                cfg_t->AutoChannelRefreshPeriod = 0;
            else
                cfg_t->AutoChannelRefreshPeriod = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "OperatingChannelBandwidth")){
            ulVal = atoi(ptr->param_val);
            if((ulVal < 1) || (ulVal > 2))
                cfg_t->OperatingChannelBandwidth = STD_20MHZ;
            else
                cfg_t->OperatingChannelBandwidth = ulVal;
            ulVal = 0;
        }else if(!strcasecmp(ptr->param_name, "ExtensionChannel")){
            ulVal = atoi(ptr->param_val);
            if((ulVal < 1) || (ulVal > 2))
                cfg_t->ExtensionChannel = SIDEBAND_LOWER; /* Default value */
            else
                cfg_t->ExtensionChannel = ulVal;
            ulVal = 0;
        }else if(!strcasecmp(ptr->param_name, "GuardInterval")){
            ulVal = atoi(ptr->param_val);
            if(ulVal == 0)
                cfg_t->GuardInterval = 2;
            else if(ulVal == 1)
                cfg_t->GuardInterval = 1;
            else
                cfg_t->GuardInterval = 3;
            ulVal = 0;
        }else if(!strcasecmp(ptr->param_name, "MCS")){
            cfg_t->MCS = atoi(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "TransmitPower")){
            cfg_t->TransmitPower = atoi(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "IEEE80211hEnabled")){
            cfg_t->IEEE80211hEnabled = (0 == atoi(ptr->param_val))? FALSE : TRUE ;
        }else if(!strcasecmp(ptr->param_name, "RegulatoryDomain")){
            strncpy(cfg_t->RegulatoryDomain,ptr->param_val,2);
            cfg_t->RegulatoryDomain[2] = 'I';
            cfg_t->RegulatoryDomain[3] = '\0';
        }
    }
    free_paramList(head);

    ptr = head = NULL;

    retVal = file_parse(WLANCFG_RADIO_EXTN_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127,CID-32897, free unused resources before exit */
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "APIsolation")){
            cfg_t->APIsolation = (0 == atoi(ptr->param_val))? FALSE : TRUE ;
        }else if(!strcasecmp(ptr->param_name, "FrameBurst")){
            cfg_t->FrameBurst = (0 == atoi(ptr->param_val))? FALSE : TRUE ;
        }else if(!strcasecmp(ptr->param_name, "CTSProtectionMode")){
            cfg_t->CTSProtectionMode = (0 == atoi(ptr->param_val))? FALSE : TRUE ;
        }else if(!strcasecmp(ptr->param_name, "BeaconInterval")){
            cfg_t->BeaconInterval = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "DTIMInterval")){
            cfg_t->DTIMInterval = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "FragmentationThreshold")){
            cfg_t->FragmentationThreshold = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "RTSThreshold")){
            cfg_t->RTSThreshold = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "TransmissionRate")){
            /* Currently TxRates are retrieved from syscfg */
        }else if(!strcasecmp(ptr->param_name, "BasicRate")){
            /* Currently BasicRates are retrieved from syscfg */
        }
    }
    free_paramList(head);

    
    /* Currently we dont support the Radio to be disabled */
    cfg_t->bEnabled = TRUE ;

    /* If AutoChannel is disabled, refresh period should be taken from syscfg */
    if(FALSE == cfg_t->AutoChannelEnable) {
        Utopia_GetNamedLong(ctx,UtopiaValue_WLAN_AutoChannelCycle ,prefix, &ulVal);
        if(ulVal == 71582787)
            cfg_t->AutoChannelRefreshPeriod = 0;
        else
            cfg_t->AutoChannelRefreshPeriod = ( ulVal * 60 ); /* Value is stored in minute in syscfg */
    }

    memset(strbuf,0,STR_SZ);
    Utopia_GetNamed(ctx, UtopiaValue_WLAN_TransmissionRate, prefix, strbuf, STR_SZ);
    if(!strcmp(strbuf,"auto")){
        cfg_t->TxRate = (WIFI_TX_RATE_AUTO + 1); /* There is an enum mis-match */
    }else {
        ulVal = atol(strbuf); 
        switch(ulVal){
            case 6: 
                cfg_t->TxRate = (WIFI_TX_RATE_6 + 1); /* There is an enum mis-match */
                break;
            case 9:
                cfg_t->TxRate = (WIFI_TX_RATE_9 + 1); /* There is an enum mis-match */
                break;
            case 12:
                cfg_t->TxRate = (WIFI_TX_RATE_12 + 1); /* There is an enum mis-match */
                break;
            case 18:
                cfg_t->TxRate = (WIFI_TX_RATE_18 + 1); /* There is an enum mis-match */
                break;
            case 24:
                cfg_t->TxRate = (WIFI_TX_RATE_24 + 1); /* There is an enum mis-match */
                break;
            case 36:
                cfg_t->TxRate = (WIFI_TX_RATE_36 + 1); /* There is an enum mis-match */
                break;
            case 48:
                cfg_t->TxRate = (WIFI_TX_RATE_48 + 1); /* There is an enum mis-match */
                break;
            case 54:
                cfg_t->TxRate = (WIFI_TX_RATE_54 + 1); /* There is an enum mis-match */
                break;
            default:
                cfg_t->TxRate = (WIFI_TX_RATE_AUTO + 1); /* There is an enum mis-match */
       } 
    }
  
    memset(strbuf,0,STR_SZ);
    Utopia_GetNamed(ctx, UtopiaValue_WLAN_BasicRate, prefix,strbuf,STR_SZ);
    if(!strcmp(strbuf,"default")){
        cfg_t->BasicRate = (WIFI_BASICRATE_DEFAULT + 1); /* There is an enum mis-match */
    }else if(!strcmp(strbuf,"1-2mbps")){
        cfg_t->BasicRate = (WIFI_BASICRATE_1_2MBPS + 1); /* There is an enum mis-match */
    }else {
        cfg_t->BasicRate = (WIFI_BASICRATE_ALL + 1); /* There is an enum mis-match */
    }
   
    Utopia_GetNamed(ctx, UtopiaValue_WLAN_Alias, prefix, cfg_t->Alias, sizeof(cfg_t->Alias));
    return SUCCESS;
}

int Utopia_SetWifiRadioCfg(UtopiaContext *ctx, void *cfg)
{
    char buf[STR_SZ] = {0};
    char* prefix;
    unsigned long ulVal = 0;
    errno_t safec_rc = -1;
    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiRadioCfg_t *cfg_t = (wifiRadioCfg_t *)cfg;
    unsigned long ulIndex;
    if(cfg_t->InstanceNumber > MAX_NUM_INSTANCES)
        ulIndex = g_IndexMapRadio[(cfg_t->InstanceNumber % MAX_NUM_INSTANCES)];
    else
        ulIndex = g_IndexMapRadio[cfg_t->InstanceNumber];

    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    prefix =  wifiTRPlatform[ulIndex].syscfg_namespace_prefix ;
    
    /* NOTE: Currently we dont support the Radio to be disabled */

    UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Alias, prefix,cfg_t->Alias);

    if (cfg_t->OperatingStandards & WIFI_STD_a )
    {
        safec_rc = strcat_s(buf, sizeof(buf),"a");
        ERR_CHK(safec_rc);
    }
    if (cfg_t->OperatingStandards & WIFI_STD_b )
    {
       safec_rc = strcat_s(buf, sizeof(buf),"b");
       ERR_CHK(safec_rc);
    }
    if (cfg_t->OperatingStandards & WIFI_STD_g )
    {
       safec_rc = strcat_s(buf, sizeof(buf),"g");
       ERR_CHK(safec_rc);
    }
    if (cfg_t->OperatingStandards & WIFI_STD_n )
    {
       safec_rc = strcat_s(buf, sizeof(buf),"n");
       ERR_CHK(safec_rc);
    }
    if(0 == strlen(buf)) {/* We didnt get any valid Operating Standards */
	if(0 == ulIndex) {
	   safec_rc = strcpy_s(buf, sizeof(buf),"bgn");
	   ERR_CHK(safec_rc);
        }
	else {
	   safec_rc = strcpy_s(buf, sizeof(buf),"an");
	   ERR_CHK(safec_rc);
        }
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Network_Mode,prefix,buf);

    /* Validation Required */
    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_Channel,prefix,cfg_t->Channel);

    /* Set it in [minute]s in syscfg; The Broadcom driver supports it only in minute */
    if(0 == cfg_t->AutoChannelRefreshPeriod)
        ulVal = 71582787; /* Zero means Auto Channel selection only at boot time */
    else if( cfg_t->AutoChannelRefreshPeriod < 60)
        ulVal = 1; /* This is the shortest period when we could do Channel Selection */
    else
        ulVal = ((cfg_t->AutoChannelRefreshPeriod)/60);
    Utopia_SetNamedLong(ctx,UtopiaValue_WLAN_AutoChannelCycle,prefix,ulVal);

    /* Validation Required */
    memset(buf,0,STR_SZ);
    if(cfg_t->MCS == -1){
       safec_rc = strcpy_s(buf, sizeof(buf),"auto");
       ERR_CHK(safec_rc);
    } else {
       safec_rc = sprintf_s(buf, sizeof(buf),"%d",cfg_t->MCS);
       if(safec_rc < EOK){
          ERR_CHK(safec_rc);
       }
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_N_TransmissionRate,prefix,buf);

    memset(buf,0,STR_SZ);
    if(cfg_t->TransmitPower == TX_POWER_LOW){
       safec_rc = strcpy_s(buf, sizeof(buf),"low");
       ERR_CHK(safec_rc);
    }
    else if(cfg_t->TransmitPower == TX_POWER_MEDIUM){
      safec_rc = strcpy_s(buf, sizeof(buf),"medium");
      ERR_CHK(safec_rc);
    }
    else if(cfg_t->TransmitPower == TX_POWER_HIGH){
       safec_rc = strcpy_s(buf, sizeof(buf),"high");
       ERR_CHK(safec_rc);
    } else {
        safec_rc = strcpy_s(buf, sizeof(buf),"high"); /* Set the default value */
        ERR_CHK(safec_rc);
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_TransmissionPower,prefix,buf);

    memset(buf,0,STR_SZ);
    if(cfg_t->OperatingChannelBandwidth == STD_20MHZ){
       safec_rc = strcpy_s(buf, sizeof(buf),"standard");
       ERR_CHK(safec_rc);
    }
    else if(cfg_t->OperatingChannelBandwidth == WIDE_40MHZ){
        safec_rc = strcpy_s(buf, sizeof(buf),"wide");
        ERR_CHK(safec_rc);
    }
    else if(cfg_t->OperatingChannelBandwidth == BAND_AUTO){
        safec_rc = strcpy_s(buf, sizeof(buf),"auto");
        ERR_CHK(safec_rc);
    }
    else {
        safec_rc = strcpy_s(buf, sizeof(buf),"auto"); /* Set the default */
        ERR_CHK(safec_rc);
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_RadioBand,prefix,buf);

    memset(buf,0,STR_SZ);
    if(cfg_t->ExtensionChannel == SIDEBAND_UPPER){
       safec_rc = strcpy_s(buf, sizeof(buf),"upper");
       ERR_CHK(safec_rc);
    }
    else {
        safec_rc = strcpy_s(buf, sizeof(buf),"lower");
        ERR_CHK(safec_rc);
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_SideBand,prefix,buf);

    memset(buf,0,STR_SZ);
    if((cfg_t->GuardInterval) == GI_SHORT){
        safec_rc = strcpy_s(buf, sizeof(buf),"short");
        ERR_CHK(safec_rc);
    }
    else if((cfg_t->GuardInterval - 2) == GI_LONG) { /* There is a mismatch between the enums */
        safec_rc = strcpy_s(buf, sizeof(buf),"long");
        ERR_CHK(safec_rc);
    }
    else if((cfg_t->GuardInterval -1) == GI_AUTO) {
	    safec_rc = strcpy_s(buf, sizeof(buf),"auto");
        ERR_CHK(safec_rc);
    }
    else {
        safec_rc = strcpy_s(buf, sizeof(buf),"auto"); /* There was an error - Set it to default value */
        ERR_CHK(safec_rc);
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_GuardInterval,prefix,buf);
 
    memset(buf,0,STR_SZ);
    safec_rc = strcpy_s(buf, sizeof(buf),((cfg_t->IEEE80211hEnabled == TRUE)? "h" : "off"));
    ERR_CHK(safec_rc);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Regulatory_Mode,prefix,buf);

    if(cfg_t->AutoChannelEnable == TRUE)
         Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_Channel,prefix,0);

    /* Cisco Extensions */
    memset(buf,0,STR_SZ);
    safec_rc = strcpy_s(buf, sizeof(buf),((cfg_t->APIsolation == FALSE)? "disabled" : "enabled"));
    ERR_CHK(safec_rc);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_ApIsolation,prefix,buf);

    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_BeaconInterval,prefix,cfg_t->BeaconInterval);

    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_DTIMInterval,prefix,cfg_t->DTIMInterval);

    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_FragmentationThreshold,prefix,cfg_t->FragmentationThreshold);

    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_RTSThreshold,prefix,cfg_t->RTSThreshold);

    memset(buf,0,STR_SZ);
    if(cfg_t->FrameBurst == FALSE){
	   safec_rc = strcpy_s(buf, sizeof(buf),"disabled");
       ERR_CHK(safec_rc);
    }
    else{
        safec_rc = strcpy_s(buf, sizeof(buf),"enabled");
        ERR_CHK(safec_rc);
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_FrameBurst,prefix,buf);

    memset(buf,0,STR_SZ);
    safec_rc = strcpy_s(buf, sizeof(buf),((cfg_t->CTSProtectionMode == FALSE)? "disabled" : "auto"));
    ERR_CHK(safec_rc);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_CTSProtectionMode,prefix,buf);

    /* Validation required */
    memset(buf,0,STR_SZ);
    switch((int)cfg_t->TxRate) {
        case 1:
            safec_rc = strcpy_s(buf, sizeof(buf),"auto");
            ERR_CHK(safec_rc);
            break;
        case 2:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",6);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break;
        case 3:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",9);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break;
        case 4:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",12);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break;
        case 5:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",18);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break; 
        case 6:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",24);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break;
        case 7:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",36);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break;
        case 8:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",48);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break;
        case 9:
            safec_rc = sprintf_s(buf, sizeof(buf),"%d",54);
            if(safec_rc < EOK){
              ERR_CHK(safec_rc);
            }
            break;
        default:
            safec_rc = strcpy_s(buf, sizeof(buf),"auto");
            ERR_CHK(safec_rc);
            
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_TransmissionRate,prefix,buf);

    memset(buf,0,STR_SZ);
    strncpy(buf,cfg_t->RegulatoryDomain,2);
    buf[2] = '\0';
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Regulatory_Domain,prefix,buf);

    memset(buf,0,STR_SZ);
    if(cfg_t->BasicRate == 1){
        safec_rc = strcpy_s(buf, sizeof(buf),"default");
        ERR_CHK(safec_rc);
    }else if(cfg_t->BasicRate == 2){
        safec_rc = strcpy_s(buf, sizeof(buf),"1-2mbps");
        ERR_CHK(safec_rc);
    }else{
        safec_rc = strcpy_s(buf, sizeof(buf),"all");
        ERR_CHK(safec_rc);
    }
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_BasicRate,prefix, buf);

    return SUCCESS;
}

int Utopia_GetWifiRadioSinfo(unsigned long ulIndex, void *sInfo)
{
    if (NULL == sInfo) {
        return ERR_INVALID_ARGS;
    }

    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    errno_t safec_rc = -1;

#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiRadioSinfo_t *sInfo_t = (wifiRadioSinfo_t *)sInfo;
    /* Common values for both 2.4GHz and 5GHz Radios */
    sInfo_t->bUpstream = FALSE;
    sInfo_t->AutoChannelSupported = TRUE;
    safec_rc = strcpy_s(sInfo_t->TransmitPowerSupported, sizeof(sInfo_t->TransmitPowerSupported),"25,50,100");
    ERR_CHK(safec_rc);
    sInfo_t->IEEE80211hSupported = TRUE;
    sInfo_t->MaxBitRate = 300000000; /*Get this from WlanCfg */

    if(0 == ulIndex) /* This is a 2.4GHz channel */
    {
        safec_rc = strcpy_s(sInfo_t->Name, sizeof(sInfo_t->Name),"wl0");
        ERR_CHK(safec_rc);
        sInfo_t->SupportedFrequencyBands = (FREQ_2_4_GHZ + 1);
        sInfo_t->SupportedStandards = (WIFI_STD_b | WIFI_STD_g | WIFI_STD_n);
        safec_rc = strcpy_s(sInfo_t->PossibleChannels, sizeof(sInfo_t->PossibleChannels),"1-11");
        ERR_CHK(safec_rc);

    }else /* This is a 5GHz channel */
    {
        safec_rc = strcpy_s(sInfo_t->Name, sizeof(sInfo_t->Name),"wl1");
        ERR_CHK(safec_rc);
        sInfo_t->SupportedFrequencyBands = (FREQ_5_GHZ + 1);
        sInfo_t->SupportedStandards = (WIFI_STD_a | WIFI_STD_n);
        safec_rc = strcpy_s(sInfo_t->PossibleChannels, sizeof(sInfo_t->PossibleChannels),"36,40,44,48,52,56,60,64,149,153,157,161,165");
        ERR_CHK(safec_rc);
    }
    return SUCCESS;
}

int Utopia_GetIndexedWifiRadioDinfo(UtopiaContext *ctx, unsigned long ulIndex, void *dInfo)
{
    char *prefix;
    int iVal = -1;
    if((NULL == ctx) || (NULL == dInfo)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    prefix = wifiTRPlatform[ulIndex].syscfg_namespace_prefix;

    /* Check if we have an InstanceNumber stored for this Radio */
    if(Utopia_GetNamedInt(ctx,UtopiaValue_WLAN_Radio_Instance_Num,prefix,&iVal))
    {
        /* Failed to find an InstanceNumber in syscfg. This is during initialization*/
        g_IndexMapRadio[ulIndex+1] = ulIndex;
        Utopia_GetWifiRadioDinfo((ulIndex + 1),dInfo);
    }
    else
    {
        /* Found the Instance number in syscfg */
        if(iVal > MAX_NUM_INSTANCES)
           g_IndexMapRadio[(iVal % MAX_NUM_INSTANCES)] = ulIndex;
        else
           g_IndexMapRadio[iVal] = ulIndex;
        Utopia_GetWifiRadioDinfo(iVal,dInfo);
    }
    return SUCCESS;
}
int Utopia_GetWifiRadioDinfo(unsigned long ulInstanceNum, void *dInfo)
{
    if (NULL == dInfo) {
        return ERR_INVALID_ARGS;
    }
    char ulog_msg[256];
    errno_t safec_rc = -1;
#ifdef _DEBUG_

    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiRadioDinfo_t *dInfo_t = (wifiRadioDinfo_t *)dInfo;
    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    unsigned long ulIndex;
    if(ulInstanceNum > MAX_NUM_INSTANCES)
        ulIndex = g_IndexMapRadio[(ulInstanceNum % MAX_NUM_INSTANCES)];
    else
        ulIndex = g_IndexMapRadio[ulInstanceNum];

    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s | grep Radio |cut -d'}' -f2- -s |cut -d. -f2- -s > %s ", wifiTRPlatform[ulIndex].ifconfig_interface, WLANCFG_RADIO_FULL_FILE);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));
    safec_rc = sprintf_s(buf, sizeof(buf),"cat %s |  sed -e '/Extensions/{N;d;}'|sed -e '/Stats/{N;d;}' > %s", WLANCFG_RADIO_FULL_FILE, WLANCFG_RADIO_FILE );
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    system(buf);
    
    retVal = file_parse(WLANCFG_RADIO_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127,CID-33277, free unused resource before exit*/
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "Status")){   
            dInfo_t->Status = atoi(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "LastChange")){
            dInfo_t->LastChange = atol(ptr->param_val);
        }
    }
    free_paramList(head);

    /* Index number 0 means 2.4GHz channel */
    /* Index number 1 which means 5GHz channel */
    safec_rc = strcpy_s(dInfo_t->ChannelsInUse, sizeof(dInfo_t->ChannelsInUse),((0 == ulIndex) ? "1,6,11" : "36,40,44,48,52,56,60,64,149,153,157,161,165"));
    ERR_CHK(safec_rc);
    return SUCCESS;
}

int Utopia_GetWifiRadioStats(unsigned long instanceNum, void *stats)
{
    if (NULL == stats) {
        return ERR_INVALID_ARGS;
    }

    char ulog_msg[256];
    errno_t safec_rc = -1;

#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiRadioStats_t *stats_t = (wifiRadioStats_t *)stats;
    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    unsigned long ulIndex;
    if(instanceNum > MAX_NUM_INSTANCES)
        ulIndex = g_IndexMapRadio[(instanceNum % MAX_NUM_INSTANCES)];
    else
        ulIndex = g_IndexMapRadio[instanceNum];

    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s | grep Radio |cut -d'}' -f2- -s |cut -d. -f2- -s > %s ", wifiTRPlatform[ulIndex].ifconfig_interface, WLANCFG_RADIO_FULL_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));
    safec_rc = sprintf_s(buf, sizeof(buf),"cat %s | grep Stats | cut -d. -f2- -s > %s", WLANCFG_RADIO_FULL_FILE, WLANCFG_RADIO_STATS_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);

    retVal = file_parse(WLANCFG_RADIO_STATS_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127, CID-33224, Free unused resource before exit*/
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "BytesSent")){
            stats_t->BytesSent = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "BytesReceived")){
            stats_t->BytesReceived = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "BytesReceived")){
            stats_t->BytesReceived = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "PacketsSent")){
            stats_t->PacketsSent = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "PacketsReceived")){
            stats_t->PacketsReceived = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "ErrorsSent")){
            stats_t->ErrorsSent = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "ErrorsReceived")){
            stats_t->ErrorsReceived = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "DiscardPacketSent")){
            stats_t->DiscardPacketsSent = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "DiscardPacketReceived")){
            stats_t->DiscardPacketsReceived = atol(ptr->param_val);
        }
    }
    free_paramList(head);

    return SUCCESS;
}

int Utopia_WifiRadioSetValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNum, char *pAlias)
{
    char *prefix;
    if (NULL == ctx || NULL == pAlias) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    /* First set the ulIndex in the global Index Map */
    if(ulInstanceNum > MAX_NUM_INSTANCES)
        g_IndexMapRadio[(ulInstanceNum % MAX_NUM_INSTANCES)] = ulIndex;
    else
        g_IndexMapRadio[ulInstanceNum] = ulIndex;

    if(ulIndex >= WIFI_RADIO_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    /* Now set these values in syscfg */
    prefix = wifiTRPlatform[ulIndex].syscfg_namespace_prefix;

    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_Radio_Instance_Num,prefix,ulInstanceNum);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Alias,prefix, pAlias);

    return SUCCESS;
}

/* Functions for Wifi SSID */

int Utopia_GetWifiSSIDInstances(UtopiaContext *ctx)
{
    if(NULL == ctx) {
        return ERR_INVALID_ARGS;
    }
    int count = 0;
    int i = 0;
    char ifCfg[STR_SZ] = {'\0'};
    errno_t safec_rc = -1;

#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_GetInt(ctx,UtopiaValue_WLAN_SSID_Num,&count);  
    /* 8 SSIDs are statically allocated, if its more than that, allocate them here */
    if(count > STATIC_SSID_COUNT) {
       /* Initialize the wifiTRPlatform_multiSSID table */
       for(i = STATIC_SSID_COUNT; i < count; i++) {
           Utopia_GetIndexed(ctx,UtopiaValue_WLAN_SSID_Radio,i,ifCfg,sizeof(ifCfg));
           allocateMultiSSID_Struct(i);
           if( 0 == strncmp(ifCfg,"wl0",3)) {
               wifiTRPlatform_multiSSID[i].interface = FREQ_2_4_GHZ;
               safec_rc = strcpy_s(wifiTRPlatform_multiSSID[i].ifconfig_interface, STR_SZ,"eth0");
               ERR_CHK(safec_rc);
           }else if (0 == strncmp(ifCfg,"wl1",3)) {
               wifiTRPlatform_multiSSID[i].interface = FREQ_5_GHZ;
               safec_rc = strcpy_s(wifiTRPlatform_multiSSID[i].ifconfig_interface, STR_SZ,"eth1");
               ERR_CHK(safec_rc);
           }
           safec_rc = strcpy_s(wifiTRPlatform_multiSSID[i].syscfg_namespace_prefix, STR_SZ,ifCfg);
           ERR_CHK(safec_rc);
           safec_rc = sprintf_s(wifiTRPlatform_multiSSID[i].ssid_name, STR_SZ,"SSID%d",i);
           if(safec_rc < EOK){
              ERR_CHK(safec_rc);
           }
           safec_rc = sprintf_s(wifiTRPlatform_multiSSID[i].ap_name, STR_SZ,"ap%d",i);
           if(safec_rc < EOK){
              ERR_CHK(safec_rc);
           }
       }
    } else if(count < STATIC_SSID_COUNT) {
        count = STATIC_SSID_COUNT; /* These are statically configured */
    }
    return count;
}

int Utopia_GetWifiSSIDEntry(UtopiaContext *ctx, unsigned long ulIndex, void *pEntry)
{

    if((NULL == ctx) || (NULL == pEntry)){
        return ERR_INVALID_ARGS;
    }

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDEntry_t *pEntry_t = ( wifiSSIDEntry_t *)pEntry;

    Utopia_GetIndexedWifiSSIDCfg(ctx, ulIndex, &(pEntry_t->Cfg));
    Utopia_GetWifiSSIDSInfo(ulIndex, &(pEntry_t->StaticInfo)); 
    Utopia_GetIndexedWifiSSIDDInfo(ctx, ulIndex, &(pEntry_t->DynamicInfo));
    return SUCCESS;
}

int Utopia_GetIndexedWifiSSIDCfg(UtopiaContext *ctx, unsigned long ulIndex, void *cfg)
{
    char *prefix;
    int iVal = -1;
    if((NULL == ctx) || (NULL == cfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDCfg_t *cfg_t = (wifiSSIDCfg_t *)cfg;

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    prefix = wifiTRPlatform_multiSSID[ulIndex].ssid_name;

    /* Check if we have an InstanceNumber stored for this SSID */
    if(Utopia_GetNamedInt(ctx,UtopiaValue_WLAN_SSID_Instance_Num,prefix,&iVal))
    {
	/* Failed to find an InstanceNumber in syscfg. This is during initialization*/
        cfg_t->InstanceNumber = (ulIndex + 1);
        g_IndexMapSSID[ulIndex+1] = ulIndex;	
        Utopia_GetWifiSSIDCfg(ctx,1,cfg_t);
    }
    else
    {
        /* Found the Instance number in syscfg */
        if(iVal > MAX_NUM_INSTANCES)
	   g_IndexMapSSID[(iVal % MAX_NUM_INSTANCES)] = ulIndex;
	else
           g_IndexMapSSID[iVal] = ulIndex;
        cfg_t->InstanceNumber = iVal;
        Utopia_GetWifiSSIDCfg(ctx,0,cfg_t);
    }
    return SUCCESS;
} 

int Utopia_GetWifiSSIDCfg(UtopiaContext *ctx, int dummyInstanceNum, void *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }

    char ulog_msg[256];
    errno_t safec_rc = -1;

#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDCfg_t *cfg_t = (wifiSSIDCfg_t *)cfg;
    char buf[BUF_SZ] = {'\0'};
    char state[STR_SZ] = {'\0'};
    char tmpBuf[STR_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    char *prefix;
    unsigned long ulIndex;
    int vif_num = 0;

    if(cfg_t->InstanceNumber > MAX_NUM_INSTANCES)
	ulIndex = g_IndexMapSSID[(cfg_t->InstanceNumber % MAX_NUM_INSTANCES)];
    else
        ulIndex = g_IndexMapSSID[cfg_t->InstanceNumber];
    /* This means that we dont have an instance number for this entry yet */
    if(dummyInstanceNum)
	cfg_t->InstanceNumber = 0;

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }
    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4) {
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        }
        else
            vif_num = 0;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep SSID |cut -d'}' -f2- -s |cut -d. -f2- -s > %s ", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WLANCFG_SSID_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));

    retVal = file_parse(WLANCFG_SSID_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127,CID-33259, free unused resource before exit*/
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "SSID")){
            if(ptr->param_val[0]){
                safec_rc = strcpy_s(cfg_t->SSID, sizeof(cfg_t->SSID),ptr->param_val);
                ERR_CHK(safec_rc);
            }
            else
            {
                /* Set a temporary SSID */
                prefix = wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix;
                safec_rc = sprintf_s(tmpBuf, sizeof(tmpBuf),"Cisco-SSID-%lu",ulIndex);
                if(safec_rc < EOK){
                   ERR_CHK(safec_rc);
                }
                UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_SSID,prefix,tmpBuf);
                safec_rc = strcpy_s(cfg_t->SSID, sizeof(cfg_t->SSID),tmpBuf);
                ERR_CHK(safec_rc);
		free(ptr->param_name); /* CID 124859 */
            }
        }
    }
    free_paramList(head);

    /* Get Alias from syscfg */    
    prefix =  wifiTRPlatform_multiSSID[ulIndex].ssid_name; 
    Utopia_GetNamed(ctx,UtopiaValue_WLAN_SSID_Alias,prefix,cfg_t->Alias,sizeof(cfg_t->Alias));
    if(0 == (strlen(cfg_t->Alias))){
	   safec_rc = strcpy_s(cfg_t->Alias, sizeof(cfg_t->Alias),prefix);
       ERR_CHK(safec_rc);
    }

    /* It is always enabled for primary SSIDs */
    if(ulIndex < PRIMARY_SSID_COUNT)
        cfg_t->bEnabled = TRUE;
    else {
        Utopia_GetNamed(ctx,UtopiaValue_WLAN_AP_State,wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix,state,STR_SZ);
        cfg_t->bEnabled = (!strcmp(state,"up")) ? TRUE : FALSE;
    }

    /* RadioName can be retrieved from this table */
    strncpy(cfg_t->WiFiRadioName,wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix,3);

    return SUCCESS;
}

/* Static Info is only retrieved at the init function and hence needs only ulIndex */
int Utopia_GetWifiSSIDSInfo(unsigned long ulIndex, void *sInfo)
{
    if (NULL == sInfo) {
        return ERR_INVALID_ARGS;
    }

    char ulog_msg[256];
    errno_t safec_rc = -1;

#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDSInfo_t *sInfo_t = (wifiSSIDSInfo_t *)sInfo;
    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    int vif_num = 0;

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }
    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep SSID |cut -d'}' -f2- -s |cut -d. -f2- -s | awk '{print $1$2}' > %s ", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WLANCFG_SSID_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));

    retVal = file_parse(WLANCFG_SSID_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127, CID-33247, free unused resources before exit*/
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "BSSID")){
	   if(getHex(ptr->param_val, sInfo_t->BSSID, MAC_SZ) != SUCCESS){
                safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: BSSID read error !!!\n", __FUNCTION__);
                if(safec_rc < EOK){
                   ERR_CHK(safec_rc);
                }
                ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            }

        }else if(!strcasecmp(ptr->param_name,"MACAddress")){
	    if(getHex(ptr->param_val , sInfo_t->MacAddress, MAC_SZ) != SUCCESS){
                safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Macaddress read error !!!\n", __FUNCTION__);
                if(safec_rc < EOK){
                   ERR_CHK(safec_rc);
                }
                ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            }

        }
    }
    free_paramList(head);

    /* SSID Names are currently stored in this table */
    safec_rc = strcpy_s(sInfo_t->Name, sizeof(sInfo_t->Name),wifiTRPlatform_multiSSID[ulIndex].ssid_name);
    ERR_CHK(safec_rc);

    return SUCCESS;
}

int Utopia_GetIndexedWifiSSIDDInfo(UtopiaContext *ctx, unsigned long ulIndex, void *dInfo)
{
    char *prefix;
    int iVal = -1;
    if (NULL == ctx || NULL == dInfo) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    prefix = wifiTRPlatform_multiSSID[ulIndex].ssid_name;

    /* Check if we have an InstanceNumber stored for this SSID */
    if(Utopia_GetNamedInt(ctx,UtopiaValue_WLAN_SSID_Instance_Num,prefix,&iVal))
    {
        /* Failed to find an InstanceNumber in syscfg. This is during initialization*/
        g_IndexMapSSID[ulIndex+1] = ulIndex;
        Utopia_GetWifiSSIDDInfo((ulIndex+1),dInfo);
    }
    else
    {
        /* Found the Instance number in syscfg */
	if(iVal > MAX_NUM_INSTANCES)
	   g_IndexMapSSID[(iVal % MAX_NUM_INSTANCES)] = ulIndex;
        else
           g_IndexMapSSID[iVal] = ulIndex;
        Utopia_GetWifiSSIDDInfo(iVal,dInfo);
    }
    return SUCCESS;
}

int Utopia_GetWifiSSIDDInfo(unsigned long ulInstanceNum, void *dInfo)
{
    if (NULL == dInfo) {
        return ERR_INVALID_ARGS;
    }

    char ulog_msg[256];
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDDInfo_t *dInfo_t = (wifiSSIDDInfo_t *)dInfo;
    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    unsigned long ulIndex;
    int vif_num = 0;

    if(ulInstanceNum > MAX_NUM_INSTANCES)
	ulIndex = g_IndexMapSSID[(ulInstanceNum % MAX_NUM_INSTANCES)];
    else
	ulIndex = g_IndexMapSSID[ulInstanceNum];

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }
	
    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep SSID |cut -d'}' -f2- -s |cut -d. -f2- -s | awk '{print $1$2}' > %s ", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WLANCFG_SSID_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));

    retVal = file_parse(WLANCFG_SSID_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127, CID-33106, free unused resources before exit*/
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "Status")){
	   dInfo_t->Status = atoi(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name,"LastChange")){
	   dInfo_t->LastChange = atol(ptr->param_val);
        }
    }
    free_paramList(head);
 
    return SUCCESS;
}

int Utopia_SetWifiSSIDCfg(UtopiaContext *ctx, void *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDCfg_t *cfg_t = (wifiSSIDCfg_t *)cfg;
    char *prefix;
    unsigned long ulIndex;

    if(cfg_t->InstanceNumber > MAX_NUM_INSTANCES)
	ulIndex = g_IndexMapSSID[(cfg_t->InstanceNumber % MAX_NUM_INSTANCES)];
    else
    	ulIndex = g_IndexMapSSID[cfg_t->InstanceNumber];

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    /* Set Alias */
    prefix =  wifiTRPlatform_multiSSID[ulIndex].ssid_name;
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_SSID_Alias,prefix,cfg_t->Alias);

    prefix =  wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix; 

    /* Set enabled/disabled */
    if(FALSE == cfg_t->bEnabled)
        Utopia_SetNamed(ctx,UtopiaValue_WLAN_AP_State,prefix,"down");
    else
        Utopia_SetNamed(ctx,UtopiaValue_WLAN_AP_State,prefix,"up");

    /* Set SSID */
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_SSID,prefix,cfg_t->SSID);

    return SUCCESS;
}

int Utopia_AddWifiSSID(UtopiaContext *ctx, void *entry)
{
    int count,i,j = 0;
    char append[STR_SZ] = {'\0'};
    char intfName[STR_SZ] = {'\0'};

    if((NULL == ctx) || (NULL == entry)){
        return ERR_INVALID_ARGS;
    }
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDEntry_t *entry_t = (wifiSSIDEntry_t *)entry;
    wifiSSIDCfg_t cfg_t = entry_t->Cfg;

    if(0 == cfg_t.InstanceNumber)
        return ERR_INVALID_ARGS;

    /* Get SSID Count for cfg_t->Radio */
    Utopia_GetInt(ctx,UtopiaValue_WLAN_SSID_Num, &count);

    /* Find out the free secondary SSID */
    for(j = START_DYNAMIC_SSID; j < MAX_SSID_PER_RADIO; j++) {
        safec_rc = sprintf_s(append, sizeof(append),".%d",j);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        safec_rc = strcpy_s(intfName, sizeof(intfName),cfg_t.WiFiRadioName);
        ERR_CHK(safec_rc);
        safec_rc = strcat_s(cfg_t.WiFiRadioName, sizeof(cfg_t.WiFiRadioName),append);
        ERR_CHK(safec_rc);
        for(i = STATIC_SSID_COUNT; i < count; i++) {
            if(0 == strcmp(wifiTRPlatform_multiSSID[i].syscfg_namespace_prefix,cfg_t.WiFiRadioName))
                break;
        }
        if(i == count){ /* We found an empty SSID */
            g_IndexMapSSID[cfg_t.InstanceNumber] = count;
            count += 1; /* Increment the count */
            Utopia_SetInt(ctx,UtopiaValue_WLAN_SSID_Num,count);
            allocateMultiSSID_Struct(i);
            /* Fill in the wifiTRPlatform_multiSSID */
            if( 0 == strncmp(cfg_t.WiFiRadioName,"wl0",3)) {
               wifiTRPlatform_multiSSID[i].interface = FREQ_2_4_GHZ;
               safec_rc = strcpy_s(wifiTRPlatform_multiSSID[i].ifconfig_interface,  STR_SZ,"eth0");
               ERR_CHK(safec_rc);
           }else if (0 == strncmp(cfg_t.WiFiRadioName,"wl1",3)) {
               wifiTRPlatform_multiSSID[i].interface = FREQ_5_GHZ;
               safec_rc = strcpy_s(wifiTRPlatform_multiSSID[i].ifconfig_interface, STR_SZ,"eth1");
               ERR_CHK(safec_rc);
           }
           safec_rc = strcpy_s(wifiTRPlatform_multiSSID[i].syscfg_namespace_prefix, STR_SZ,cfg_t.WiFiRadioName);
           ERR_CHK(safec_rc);
           safec_rc = sprintf_s(wifiTRPlatform_multiSSID[i].ssid_name, STR_SZ,"SSID%d",i);
           if(safec_rc < EOK){
              ERR_CHK(safec_rc);
           }
           safec_rc = sprintf_s(wifiTRPlatform_multiSSID[i].ap_name, STR_SZ,"ap%d",i);
           if(safec_rc < EOK){
              ERR_CHK(safec_rc);
           }
           /* Set the Instance Number in syscfg */
           Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_SSID_Instance_Num,wifiTRPlatform_multiSSID[i].ssid_name,cfg_t.InstanceNumber);
           /* Set Radio for this index */
           Utopia_SetIndexed(ctx,UtopiaValue_WLAN_SSID_Radio,i,cfg_t.WiFiRadioName);
           /* Call SetCfg to set other parameters */
           Utopia_SetWifiSSIDCfg(ctx,&cfg_t);
           /* Get the static and dynamic info */
           Utopia_GetWifiSSIDSInfo(i, &(entry_t->StaticInfo));
           Utopia_GetIndexedWifiSSIDDInfo(ctx, i, &(entry_t->DynamicInfo));

           return SUCCESS;
        }
        append[0] = '\0';
        /* Reset the RadioName to original value */
        safec_rc = strcpy_s(cfg_t.WiFiRadioName, sizeof(cfg_t.WiFiRadioName),intfName);
        ERR_CHK(safec_rc);
    }

    /* The only reason why it would come here is if all the SSIDs are occupied */
    cfg_t.WiFiRadioName[0] = '\0';
    return ERR_WIFI_NO_FREE_SSID;
}

int Utopia_DelWifiSSID(UtopiaContext *ctx, unsigned long ulInstanceNumber)
{
    int count = 0;
    unsigned long ulIndex = 0;
    unsigned long ulInsNum = 0;
    char ifCfg[STR_SZ] = {'\0'};
    errno_t safec_rc = -1;

    if(NULL == ctx){
        return ERR_INVALID_ARGS;
    }
#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
                                                                                             

    if(0 == ulInstanceNumber)
        return ERR_INVALID_ARGS;

    ulIndex = g_IndexMapSSID[ulInstanceNumber];
    g_IndexMapSSID[ulInstanceNumber] = 0;

    if(ulIndex < STATIC_SSID_COUNT)
        return ERR_WIFI_CAN_NOT_DELETE;

    Utopia_GetInt(ctx,UtopiaValue_WLAN_SSID_Num, &count);
    count = count - 1; /* InstanceNumber deletion */
    UTOPIA_SETINT(ctx, UtopiaValue_WLAN_SSID_Num, count);

    if(count > STATIC_SSID_COUNT) /* Required only if we have dynamic SSIDs */
    {
       for(;ulIndex < count; ulIndex++)
       {
          /* Move the array forward */
          wifiTRPlatform_multiSSID[ulIndex].interface = wifiTRPlatform_multiSSID[ulIndex+1].interface;
          safec_rc = strcpy_s(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix, STR_SZ,wifiTRPlatform_multiSSID[ulIndex+1].syscfg_namespace_prefix);
          ERR_CHK(safec_rc);
          safec_rc = strcpy_s(wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, STR_SZ,wifiTRPlatform_multiSSID[ulIndex+1].ifconfig_interface);
          ERR_CHK(safec_rc);
          safec_rc = strcpy_s(wifiTRPlatform_multiSSID[ulIndex].ssid_name, STR_SZ,wifiTRPlatform_multiSSID[ulIndex+1].ssid_name);
          ERR_CHK(safec_rc);
          safec_rc = strcpy_s(wifiTRPlatform_multiSSID[ulIndex].ap_name, STR_SZ,wifiTRPlatform_multiSSID[ulIndex+1].ap_name );
          ERR_CHK(safec_rc);
          Utopia_GetIndexed(ctx,UtopiaValue_WLAN_SSID_Radio,(ulIndex + 1),ifCfg,sizeof(ifCfg));
          Utopia_SetIndexed(ctx,UtopiaValue_WLAN_SSID_Radio,ulIndex,ifCfg);
          Utopia_GetNamedInt(ctx,UtopiaValue_WLAN_SSID_Instance_Num,wifiTRPlatform_multiSSID[ulIndex+1].ssid_name,(int *)&ulInsNum);
          g_IndexMapSSID[ulInsNum] = ulIndex; /* Point the instance number to correct index */
       }
       freeMultiSSID_Struct(ulIndex);
    }          
    return SUCCESS;
}

int Utopia_WifiSSIDSetValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNum, char *pAlias)
{
    char *prefix;
    if (NULL == ctx || NULL == pAlias) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    /* First set the ulIndex in the global Index Map */
    if(ulInstanceNum > MAX_NUM_INSTANCES)
	g_IndexMapSSID[(ulInstanceNum % MAX_NUM_INSTANCES)] = ulIndex;
    else
        g_IndexMapSSID[ulInstanceNum] = ulIndex;

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    /* Now set these values in syscfg */

    prefix = wifiTRPlatform_multiSSID[ulIndex].ssid_name;

    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_SSID_Instance_Num,prefix,ulInstanceNum);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_SSID_Alias,prefix, pAlias);

    return SUCCESS;
}

int Utopia_GetWifiSSIDStats(unsigned long ulInstanceNum, void *stats)
{
    if (NULL == stats) {
        return ERR_INVALID_ARGS;
    }
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiSSIDStats_t *stats_t = (wifiSSIDStats_t *)stats;
    char if_name[STR_SZ] = {'\0'};
    unsigned long ulIndex;

    if(ulInstanceNum > MAX_NUM_INSTANCES)
        ulIndex = g_IndexMapRadio[(ulInstanceNum % MAX_NUM_INSTANCES)];
    else
        ulIndex = g_IndexMapRadio[ulInstanceNum];

    if(ulIndex >= WIFI_SSID_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    safec_rc = strcpy_s(if_name, sizeof(if_name),wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface); 
    ERR_CHK(safec_rc);
    stats_t->BytesReceived = parse_proc_net_dev(if_name,1);
    stats_t->PacketsReceived = parse_proc_net_dev(if_name,2);
    stats_t->ErrorsReceived = parse_proc_net_dev(if_name,3);
    stats_t->DiscardPacketsReceived = parse_proc_net_dev(if_name,4);
    stats_t->BytesSent = parse_proc_net_dev(if_name,9);
    stats_t->PacketsSent = parse_proc_net_dev(if_name,10);
    stats_t->ErrorsSent = parse_proc_net_dev(if_name,11);
    stats_t->DiscardPacketsSent = parse_proc_net_dev(if_name,12);

    return SUCCESS;
}
int Utopia_GetWifiAPInstances(UtopiaContext *ctx)
{
    int count = 0;
#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    Utopia_GetInt(ctx,UtopiaValue_WLAN_SSID_Num,&count);
    if(count < STATIC_SSID_COUNT)
        count = STATIC_SSID_COUNT; /* We will have a minimum of eight APs always */
    return count;
}

int Utopia_GetWifiAPEntry(UtopiaContext *ctx, char *pSSID, void *pEntry)
{
    if (NULL == ctx || NULL == pEntry || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPEntry_t *pEntry_t = ( wifiAPEntry_t *)pEntry;
    int index = Utopia_GetWifiAPIndex(ctx, pSSID);

    /* Did we get the SSID */
    if(index < 0)
        return ERR_INVALID_ARGS;

    unsigned long ulIndex = index;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }
    Utopia_GetIndexedWifiAPCfg(ctx, ulIndex, &(pEntry_t->Cfg));
    Utopia_GetWifiAPInfo(ctx, pSSID, &(pEntry_t->Info));
    return SUCCESS;
}

int Utopia_GetIndexedWifiAPCfg(UtopiaContext *ctx, unsigned long ulIndex, void *pCfg)
{
    char *prefix;
    int iVal = -1;
    if (NULL == ctx || NULL == pCfg) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg,  sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPCfg_t *cfg_t = (wifiAPCfg_t *)pCfg;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    prefix = wifiTRPlatform_multiSSID[ulIndex].ap_name;

    /* Check if we have an InstanceNumber stored for this SSID */
    if(Utopia_GetNamedInt(ctx,UtopiaValue_WLAN_AP_Instance_Num,prefix,&iVal))
    {
        /* Failed to find an InstanceNumber in syscfg. This is during initialization*/
        cfg_t->InstanceNumber = (ulIndex + 1);
        g_IndexMapAP[ulIndex+1] = ulIndex;
        /* This is a dummy instance number which needs to be deleetd */
        Utopia_GetWifiAPCfg(ctx,1,cfg_t);
    }
    else
    {
        /* Found the Instance number in syscfg */
        if(iVal > MAX_NUM_INSTANCES)
	   g_IndexMapAP[(iVal % MAX_NUM_INSTANCES)] = ulIndex;
	else
           g_IndexMapAP[iVal] = ulIndex;
        cfg_t->InstanceNumber = iVal;
        Utopia_GetWifiAPCfg(ctx,0,cfg_t);
    }
    return SUCCESS;
}

int Utopia_GetWifiAPCfg(UtopiaContext *ctx,int dummyInstanceNum, void *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }
    char ulog_msg[256];
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPCfg_t *cfg_t = (wifiAPCfg_t *)cfg;
    char buf[BUF_SZ] = {'\0'};
    char state[STR_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    char *prefix;
    unsigned long ssidInstanceNum = 0;
    unsigned long ulIndex;
    int vif_num = 0;
   
    if(cfg_t->InstanceNumber > MAX_NUM_INSTANCES)
	ulIndex = g_IndexMapAP[(cfg_t->InstanceNumber % MAX_NUM_INSTANCES)];
    else
 	ulIndex = g_IndexMapAP[cfg_t->InstanceNumber];
    if(dummyInstanceNum)
	cfg_t->InstanceNumber = 0;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }
   
    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }
 
    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep AccessPoint |cut -d'}' -f2- -s |cut -d. -f2- -s | awk '{print $1$2}' > %s ", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WLANCFG_AP_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));
    
    retVal = file_parse(WLANCFG_AP_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127,CID-32951, free unused resources before exit */
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "SSIDAdvertisementEnabled")){
           cfg_t->SSIDAdvertisementEnabled = (0 == atoi(ptr->param_val))? FALSE : TRUE;
        }else if(!strcasecmp(ptr->param_name,"RetryLimit")){
           cfg_t->RetryLimit = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name,"WMMEnable")){
           cfg_t->WMMEnable = (0 == atoi(ptr->param_val))? FALSE : TRUE;
        }else if(!strcasecmp(ptr->param_name,"UAPSDEnable")){
           if(FALSE == cfg_t->WMMEnable)
               cfg_t->UAPSDEnable = FALSE ; /* If WMM is disabled, UAPSD is disabled as well */
           else
               cfg_t->UAPSDEnable = (0 == atoi(ptr->param_val))? FALSE : TRUE;
        }
    }
    free_paramList(head);

    /* Get AP Alias */
    prefix = wifiTRPlatform_multiSSID[ulIndex].ap_name;
    Utopia_GetNamed(ctx,UtopiaValue_WLAN_AP_Alias,prefix,cfg_t->Alias,sizeof(cfg_t->Alias));
    if(0 == (strlen(cfg_t->Alias))){
      safec_rc = strcpy_s(cfg_t->Alias, sizeof(cfg_t->Alias),prefix);
      ERR_CHK(safec_rc);
    }
 
     /* It is always enabled for primary SSIDs */
    if(ulIndex < PRIMARY_SSID_COUNT)
        cfg_t->bEnabled = TRUE;
    else {
        Utopia_GetNamed(ctx,UtopiaValue_WLAN_AP_State,wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix,state,STR_SZ);
        cfg_t->bEnabled = (!strcmp(state,"up")) ? TRUE : FALSE;
    }
 
    /* SSID Reference is a fixed path name referring to the corresponding instance in SSID table */
    prefix = wifiTRPlatform_multiSSID[ulIndex].ssid_name;

    /* Check if we have an InstanceNumber stored for this SSID */
    if(Utopia_GetNamedInt(ctx,UtopiaValue_WLAN_SSID_Instance_Num,prefix,(int *)&ssidInstanceNum))
    {
	/* We didnt find any instance num for this SSID */
      safec_rc = sprintf_s(cfg_t->SSID, sizeof(cfg_t->SSID),"Device.WiFi.SSID.%lu.",(ulIndex+1));
      if(safec_rc < EOK){
         ERR_CHK(safec_rc);
      }
    }
    else
    {
        safec_rc = sprintf_s(cfg_t->SSID, sizeof(cfg_t->SSID),"Device.WiFi.SSID.%lu.",ssidInstanceNum);
        if(safec_rc < EOK){
         ERR_CHK(safec_rc);
        }
    }

    return SUCCESS;
}

int Utopia_GetWifiAPInfo(UtopiaContext *ctx, char *pSSID, void *info)
{
    if ((NULL == ctx) || (NULL == info) || (NULL == pSSID)) {
        return ERR_INVALID_ARGS;
    }
    char ulog_msg[256];
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPInfo_t *info_t = (wifiAPInfo_t *)info;
    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    int Val = -1;
    int vif_num = 0;
    int index = Utopia_GetWifiAPIndex(ctx,pSSID);
    if(index < 0)
       return ERR_INVALID_ARGS;

    unsigned long ulIndex = index;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep AccessPoint |cut -d'}' -f2- -s |cut -d. -f2- -s | awk '{print $1$2}' > %s ", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WLANCFG_AP_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));

    retVal = file_parse(WLANCFG_AP_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127,CID-33342, free unused resources before exit */
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "Status")){
           Val = atoi(ptr->param_val);
           /* Status is mapped differently in driver and COSA */
           if(Val == 1) /* AP Is up */
	     info_t->Status = 2;
           else if(Val == 2) /* AP is down */
  	     info_t->Status = 1;
	   else if(Val == 8) /* Error */
	     info_t->Status = 4;
	   else /* Unknown */
	     info_t->Status = 3;
        }
    }
    free_paramList(head);
    info_t->WMMCapability = TRUE;
    info_t->UAPSDCapability = TRUE;

    return SUCCESS;
}

int Utopia_SetWifiAPCfg(UtopiaContext *ctx, void *cfg)
{
    char buf[STR_SZ];
    int Val = -1;

    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPCfg_t *cfg_t = (wifiAPCfg_t *)cfg;
    char *prefix;
    unsigned long ulIndex;

    if(cfg_t->InstanceNumber > MAX_NUM_INSTANCES)
	ulIndex = g_IndexMapAP[(cfg_t->InstanceNumber % MAX_NUM_INSTANCES)];
    else
 	ulIndex = g_IndexMapAP[cfg_t->InstanceNumber];

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    /* Set Alias */
    prefix = wifiTRPlatform_multiSSID[ulIndex].ap_name;
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_AP_Alias,prefix,cfg_t->Alias);

    /* These are prefixed on radio - Would need to change when we support multi-SSID */

    prefix =  wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix ;
    
    memset(buf,0,STR_SZ);
    safec_rc = strcpy_s(buf, sizeof(buf), ((cfg_t->WMMEnable == FALSE) ? "disabled" : "enabled"));
    ERR_CHK(safec_rc);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_AP_WMM,prefix,buf);

    memset(buf,0,STR_SZ);
    safec_rc = strcpy_s(buf, sizeof(buf), ((cfg_t->UAPSDEnable == FALSE) ? "disabled" : "enabled"));
    ERR_CHK(safec_rc);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_AP_WMM_UAPSD,prefix,buf);
    
    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_AP_Retry_Limit,prefix,cfg_t->RetryLimit); 

    if(cfg_t->SSIDAdvertisementEnabled == FALSE)
	Val = 0;
    else
	Val = 1;
    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_SSIDBroadcast,prefix,Val);

    if(FALSE == cfg_t->bEnabled)
        Utopia_SetNamed(ctx,UtopiaValue_WLAN_AP_State,prefix,"down");
    else
        Utopia_SetNamed(ctx,UtopiaValue_WLAN_AP_State,prefix,"up");

    return SUCCESS;
}

int Utopia_WifiAPSetValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNum, char *pAlias)
{
    char *prefix;
    if ((NULL == ctx) || (NULL == pAlias)) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    
    /* First set the ulIndex in the global Index Map */
    if(ulInstanceNum > MAX_NUM_INSTANCES)
	g_IndexMapAP[(ulInstanceNum % MAX_NUM_INSTANCES)] = ulIndex;
    else
    	g_IndexMapAP[ulInstanceNum] = ulIndex;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    /* Now set these values in syscfg */

    prefix = wifiTRPlatform_multiSSID[ulIndex].ap_name;
    Utopia_SetNamedInt(ctx,UtopiaValue_WLAN_AP_Instance_Num,prefix,ulInstanceNum);
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_AP_Alias,prefix, pAlias);

    return SUCCESS;
}

int Utopia_GetWifiAPSecEntry(UtopiaContext *ctx,char *pSSID, void *pEntry)
{
    if (NULL == ctx || NULL == pEntry || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPSecEntry_t *pEntry_t = (wifiAPSecEntry_t *)pEntry;
  
    Utopia_GetWifiAPSecCfg(ctx, pSSID, &(pEntry_t->Cfg));
    Utopia_GetWifiAPSecInfo(ctx, pSSID, &(pEntry_t->Info));
    return SUCCESS;
}

int Utopia_GetWifiAPSecCfg(UtopiaContext *ctx,char *pSSID, void *cfg)
{
    if (NULL == ctx || NULL == cfg || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }
    char ulog_msg[256];
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPSecCfg_t *cfg_t = (wifiAPSecCfg_t *)cfg;
    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    int modeEnabled;
    int vif_num = 0;
    int index = Utopia_GetWifiAPIndex(ctx, pSSID);
    if(index < 0)
       return ERR_INVALID_ARGS;

    unsigned long ulIndex = index;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep Security |cut -d'}' -f2- -s |cut -d. -f3- -s | awk '{print $1$2}' > %s ", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WLANCFG_AP_SEC_FILE);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));

    retVal = file_parse(WLANCFG_AP_SEC_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
          ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); /*RDKB-7127,CID-32998, free unused resources before exit */
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "ModeEnabled")){
           modeEnabled = atoi(ptr->param_val);
           cfg_t->ModeEnabled = 0;
           switch(modeEnabled)
           {
              case 1:
                 cfg_t->ModeEnabled = WIFI_SECURITY_None;
                 break;
              case 2:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WEP_64;
                 break;
              case 3:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WEP_128;
                 break;
              case 4:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WPA_Personal;
                 break;
              case 5:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WPA2_Personal;
                 break;
              case 6:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WPA_WPA2_Personal;
                 break;
              case 7:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WPA_Enterprise;
                 break;
              case 8:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WPA2_Enterprise;
                 break;
              case 9:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WPA_WPA2_Enterprise;
                 break;
              default:
                 cfg_t->ModeEnabled = WIFI_SECURITY_WPA2_Personal;
                 break;
           }
        }else if(!strcasecmp(ptr->param_name,"RekeyingInterval")){
           if(0 == strlen(ptr->param_val))
              cfg_t->RekeyingInterval = 3600; /* Default Values */
           else
              cfg_t->RekeyingInterval = atol(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name,"WEPKey")) {
            cfg_t->WEPKeyp[0] = '\0' ; /*Default Empty */
            if(ptr->param_val)
            {
                if (WIFI_SECURITY_WEP_64 == cfg_t->ModeEnabled)
                    getHexGeneric(ptr->param_val,cfg_t->WEPKeyp,5);
                else if (WIFI_SECURITY_WEP_128 == cfg_t->ModeEnabled)
                    getHexGeneric(ptr->param_val,cfg_t->WEPKeyp,13);
             }
         }else if(!strcasecmp(ptr->param_name,"KeyPassphrase")) {
            safec_rc = strcpy_s(cfg_t->KeyPassphrase, sizeof(cfg_t->KeyPassphrase),"wpa2psk"); /*Default Value */
            ERR_CHK(safec_rc);
            if(ptr->param_val){
                safec_rc = strcpy_s(cfg_t->KeyPassphrase, sizeof(cfg_t->KeyPassphrase),ptr->param_val);
                ERR_CHK(safec_rc);
            }
         }else if(!strcasecmp(ptr->param_name,"EncryptionMethod")) {
            cfg_t->EncryptionMethod = WIFI_SECURITY_AES_TKIP; /*Default Value */
            if(ptr->param_val)
                cfg_t->EncryptionMethod = atoi(ptr->param_val);
         }else if(!strcasecmp(ptr->param_name,"RadiusServerIP")) {
            if(ptr->param_val)
                cfg_t->RadiusServerIPAddr.Value = inet_addr(ptr->param_val);
         }else if(!strcasecmp(ptr->param_name,"RadiusServerPort")) {
            cfg_t->RadiusServerPort = 1812; /* Default Value */
            if(ptr->param_val)
                cfg_t->RadiusServerPort = atoi(ptr->param_val);  
         }else if (!strcasecmp(ptr->param_name,"RadiusSharedSecret")) {
            if(ptr->param_val){
                safec_rc = strcpy_s(cfg_t->RadiusSecret, sizeof(cfg_t->RadiusSecret),ptr->param_val);
                ERR_CHK(safec_rc);
            }
         }
     }
     cfg_t->PreSharedKey[0] = '\0'; /* PSK is to be always returned as EMPTY */     
     free_paramList(head); /*RDKB-7127,CID-32998, free unused resources before exit */
     return SUCCESS;
}

int Utopia_GetWifiAPSecInfo(UtopiaContext *ctx,char *pSSID, void *info)
{
   if (NULL == ctx || NULL == info || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPSecInfo_t *info_t = (wifiAPSecInfo_t *)info;
   
    info_t->ModesSupported = (WIFI_SECURITY_None | WIFI_SECURITY_WEP_64 | WIFI_SECURITY_WEP_128 | WIFI_SECURITY_WPA_Personal | WIFI_SECURITY_WPA2_Personal | WIFI_SECURITY_WPA_WPA2_Personal| WIFI_SECURITY_WPA_Enterprise | WIFI_SECURITY_WPA2_Enterprise | WIFI_SECURITY_WPA_WPA2_Enterprise) ;
    return SUCCESS;
}

int Utopia_SetWifiAPSecCfg(UtopiaContext *ctx, char *pSSID, void *cfg)
{
    char buf[STR_SZ];

    if (NULL == ctx || NULL == cfg || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPSecCfg_t *cfg_t = (wifiAPSecCfg_t *)cfg;
    char *prefix;
    int index = Utopia_GetWifiAPIndex(ctx, pSSID);
    if(index < 0)
        return ERR_INVALID_ARGS;

    unsigned long ulIndex = index; 

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    /* These are prefixed on radio - Would need to change when we support multi-SSID */
    prefix =  wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix ;

    memset(buf,0,STR_SZ);
  
    if(cfg_t->ModeEnabled & WIFI_SECURITY_None) {
        safec_rc = strcpy_s(buf, sizeof(buf),"disabled");
        ERR_CHK(safec_rc);
    }
    else if((cfg_t->ModeEnabled & WIFI_SECURITY_WEP_64) || (cfg_t->ModeEnabled & WIFI_SECURITY_WEP_128)) {
        safec_rc = strcpy_s(buf, sizeof(buf),"wep");
        ERR_CHK(safec_rc);
    }
    else if(cfg_t->ModeEnabled & WIFI_SECURITY_WPA_Personal) {
        safec_rc = strcpy_s(buf, sizeof(buf),"wpa-personal");
        ERR_CHK(safec_rc);
        UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip"); /* Set the default Encryption */
    }
    else if(cfg_t->ModeEnabled & WIFI_SECURITY_WPA2_Personal) {
        safec_rc = strcpy_s(buf, sizeof(buf),"wpa2-personal");
        ERR_CHK(safec_rc);
        UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip+aes"); /* Set the default Encryption */
    }
    else if(cfg_t->ModeEnabled & WIFI_SECURITY_WPA_Enterprise) {
        safec_rc = strcpy_s(buf, sizeof(buf),"wpa-enterprise");
        ERR_CHK(safec_rc);
        UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip"); /* Set the default Encryption */
    } 
    else if (cfg_t->ModeEnabled & WIFI_SECURITY_WPA2_Enterprise) {
        safec_rc = strcpy_s(buf, sizeof(buf),"wpa2-enterprise");
        ERR_CHK(safec_rc);
        UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip+aes"); /* Set the default Encryption */
    }
    else if (cfg_t->ModeEnabled & WIFI_SECURITY_WPA_WPA2_Personal) {
        safec_rc = strcpy_s(buf, sizeof(buf),"wpa-wpa2-personal");
        ERR_CHK(safec_rc);
        UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip+aes"); 
    }
    else if (cfg_t->ModeEnabled & WIFI_SECURITY_WPA_WPA2_Enterprise) {
        safec_rc = strcpy_s(buf, sizeof(buf),"wpa-wpa2-enterprise");
        ERR_CHK(safec_rc);
        UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip+aes");
    }
    else { 
        safec_rc = strcpy_s(buf, sizeof(buf),"wpa2-personal"); /* Default value and no other sec.modes supported */
        ERR_CHK(safec_rc);
        UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip+aes"); /* Set the default Encryption */
    }
 
    UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_SecurityMode,prefix,buf);

    memset(buf,0,STR_SZ);
    if(cfg_t->ModeEnabled & WIFI_SECURITY_WEP_64)
    {
       /* It is a WEP-64 Key, so expect it to be of 5 byte */
       safec_rc = sprintf_s(buf, sizeof(buf),"%02X%02X%02X%02X%02X",cfg_t->WEPKeyp[0],cfg_t->WEPKeyp[1],cfg_t->WEPKeyp[2],cfg_t->WEPKeyp[3],cfg_t->WEPKeyp[4]);
       if(safec_rc < EOK){
          ERR_CHK(safec_rc);
       }
          buf[10] = '\0';

       UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Key0, prefix, buf); 
       Utopia_SetNamedInt(ctx, UtopiaValue_WLAN_TxKey, prefix, 0); 
    }
    else if(cfg_t->ModeEnabled & WIFI_SECURITY_WEP_128) 
    {
       /* It is a WEP-128 Key, so expect it to be 13 byte */
       safec_rc = sprintf_s(buf, sizeof(buf),"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",cfg_t->WEPKeyp[0],cfg_t->WEPKeyp[1],cfg_t->WEPKeyp[2],cfg_t->WEPKeyp[3],cfg_t->WEPKeyp[4],cfg_t->WEPKeyp[5],cfg_t->WEPKeyp[6],cfg_t->WEPKeyp[7],cfg_t->WEPKeyp[8],cfg_t->WEPKeyp[9],cfg_t->WEPKeyp[10],cfg_t->WEPKeyp[11],cfg_t->WEPKeyp[12]);
       if(safec_rc < EOK){
          ERR_CHK(safec_rc);
       }
       buf[26] = '\0';

       UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Key0, prefix, buf); 
       Utopia_SetNamedInt(ctx, UtopiaValue_WLAN_TxKey, prefix, 0); 
     }
    
    if((cfg_t->ModeEnabled & WIFI_SECURITY_WPA_Personal) || (cfg_t->ModeEnabled & WIFI_SECURITY_WPA2_Personal))
    { 
       if((0 != strlen(cfg_t->KeyPassphrase))) 
       {
          UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Passphrase, prefix, cfg_t->KeyPassphrase);
       }
       if(0 != cfg_t->RekeyingInterval)
       {
          Utopia_SetNamedInt(ctx, UtopiaValue_WLAN_KeyRenewal, prefix, cfg_t->RekeyingInterval);
       }
       else
       {
          Utopia_SetNamedInt(ctx, UtopiaValue_WLAN_KeyRenewal, prefix, 3600); /* Default Value */
       }
    }else if((cfg_t->ModeEnabled & WIFI_SECURITY_WPA_Enterprise ) || (cfg_t->ModeEnabled & WIFI_SECURITY_WPA2_Enterprise))
    {
       /* Set the values in Syscfg */
       UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_Shared, prefix, cfg_t->RadiusSecret);
       Utopia_SetNamedInt(ctx, UtopiaValue_WLAN_RadiusPort, prefix, cfg_t->RadiusServerPort);
       struct in_addr addrVal;
       addrVal.s_addr = cfg_t->RadiusServerIPAddr.Value;
       UTOPIA_SETNAMED(ctx, UtopiaValue_WLAN_RadiusServer, prefix, inet_ntoa(addrVal));
    }
    if(cfg_t->EncryptionMethod == WIFI_SECURITY_TKIP)
    {
       UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip");
    }
    else if (cfg_t->EncryptionMethod == WIFI_SECURITY_AES)
    {
       UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"aes");
    }
    else
    {
       UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_Encryption,prefix,"tkip+aes"); /* For everything else */
    }

    return SUCCESS;
}

int Utopia_GetWifiAPWPSEntry(UtopiaContext *ctx, char*pSSID, void *pEntry)
{
    if (NULL == ctx || NULL == pEntry || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPWPSEntry_t *pEntry_t = (wifiAPWPSEntry_t *)pEntry;

    Utopia_GetWifiAPWPSCfg(ctx, pSSID, &(pEntry_t->Cfg));

    pEntry_t->Info.ConfigMethodsSupported = (WIFI_WPS_METHOD_PushButton | WIFI_WPS_METHOD_Pin);

    return SUCCESS;
}

int Utopia_GetWifiAPWPSCfg(UtopiaContext *ctx, char*pSSID, void *cfg)
{
    if (NULL == ctx || NULL == cfg || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    wifiAPWPSCfg_t *cfg_t = (wifiAPWPSCfg_t *)cfg;
    char buf[STR_SZ] = {'\0'};
   
    char *prefix;
    int index = Utopia_GetWifiAPIndex(ctx, pSSID);
    if(index < 0)
        return ERR_INVALID_ARGS;
   
    unsigned long ulIndex = index ;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    /* These are prefixed on radio - Would need to change when we support multi-SSID */
    prefix =  wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix ;
    cfg_t->ConfigMethodsEnabled = 0; 

    Utopia_GetNamed(ctx,UtopiaValue_WLAN_WPS_PIN_Method,prefix,buf,STR_SZ);
    if(0 == strlen(buf))
        cfg_t->ConfigMethodsEnabled = (cfg_t->ConfigMethodsEnabled | WIFI_WPS_METHOD_Pin); /* If its not set, mark it as enabled */
    if(!strcmp(buf,"enabled"))
        cfg_t->ConfigMethodsEnabled = (cfg_t->ConfigMethodsEnabled | WIFI_WPS_METHOD_Pin);

    memset(buf,0,STR_SZ);
    Utopia_GetNamed(ctx,UtopiaValue_WLAN_WPS_PBC_Method,prefix,buf,STR_SZ);
    if(0 == strlen(buf))
        cfg_t->ConfigMethodsEnabled = (cfg_t->ConfigMethodsEnabled | WIFI_WPS_METHOD_PushButton); /* If its not set, mark it as enabled */
    if(!strcmp(buf,"enabled"))
        cfg_t->ConfigMethodsEnabled = (cfg_t->ConfigMethodsEnabled | WIFI_WPS_METHOD_PushButton);

    memset(buf,0,STR_SZ);

    Utopia_GetNamed(ctx,UtopiaValue_WLAN_WPS_Enabled,prefix,buf,STR_SZ);
    if(!strcmp(buf,"disabled"))
         cfg_t->bEnabled = FALSE;
    else
         cfg_t->bEnabled = TRUE;

    return SUCCESS;                                                                        
}

int Utopia_SetWifiAPWPSCfg(UtopiaContext *ctx,char *pSSID, void *cfg)
{
    if (NULL == ctx || NULL == cfg || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    char ulog_msg[256];
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

   wifiAPWPSCfg_t *cfg_t = (wifiAPWPSCfg_t *)cfg;
   char buf[STR_SZ] = {'\0'};
   char *prefix;
   int index = Utopia_GetWifiAPIndex(ctx, pSSID);
   if(index < 0)
       return ERR_INVALID_ARGS;

   unsigned long ulIndex = index ;
  
   if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    } 

   /* These are prefixed on radio - Would need to change when we support multi-SSID */
    prefix =  wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix ;

   memset(buf,0,STR_SZ);
   /* First set these values in syscfg */
   safec_rc = strcpy_s(buf, sizeof(buf), ((cfg_t->ConfigMethodsEnabled & WIFI_WPS_METHOD_Pin) ? "enabled" : "disabled" ));
   ERR_CHK(safec_rc);
   UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_WPS_PIN_Method,prefix,buf);

   memset(buf,0,STR_SZ);
   safec_rc = strcpy_s(buf, sizeof(buf), ((cfg_t->ConfigMethodsEnabled & WIFI_WPS_METHOD_PushButton) ? "enabled" : "disabled" ));
   ERR_CHK(safec_rc);
   UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_WPS_PBC_Method,prefix,buf);

   memset(buf,0,STR_SZ);
   safec_rc = strcpy_s(buf, sizeof(buf), ((FALSE == cfg_t->bEnabled) ? "disabled" : "enabled" ));
   ERR_CHK(safec_rc);
   UTOPIA_SETNAMED(ctx,UtopiaValue_WLAN_WPS_Enabled,prefix,buf);

   return SUCCESS;
}

unsigned long Utopia_GetAssociatedDevicesCount(UtopiaContext *ctx, char *pSSID)
{
    if (NULL == ctx || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }
    char ulog_msg[256];
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    unsigned long ulAssocDevCount = 0;
    int vif_num = 0;
    int index = Utopia_GetWifiAPIndex(ctx, pSSID);
    if(index < 0)
        return ERR_INVALID_ARGS;

    unsigned long ulIndex = index ;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep AssociatedDevice |cut -d'}' -f2- -s |cut -d. -f2- -s | awk '{print $1$2}' > %s ", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WLANCFG_AP_ASSOC_DEV_FILE);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));

    retVal = file_parse(WLANCFG_AP_ASSOC_DEV_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head);/*RDKB-7127, CID-33021, free unused resource before exit*/
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
          ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "AssociatedDeviceNumberOfEntries")){
           ulAssocDevCount = atoi(ptr->param_val);
        }
    }

    free_paramList(head);/*RDKB-7127, CID-33021, free unused resource before exit*/
    return ulAssocDevCount;

}

int Utopia_GetAssocDevice(UtopiaContext *ctx, char *pSSID, unsigned long ulIndex, void *assocDev)
{
    if (NULL == ctx || NULL == assocDev || NULL == pSSID) {
        return ERR_INVALID_ARGS;
    }
    char ulog_msg[256];
    errno_t safec_rc = -1;
#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    wifiAPAssocDevice_t *assocDev_t = (wifiAPAssocDevice_t *)assocDev;
    char buf[BUF_SZ] = {'\0'};
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    int vif_num = 0;
    int index = Utopia_GetWifiAPIndex(ctx, pSSID);
    if(index < 0)
        return ERR_INVALID_ARGS;

    unsigned long ulAPIndex = index;
   
    if(ulAPIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    if(ulAPIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulAPIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulAPIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }
 
    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep AssociatedDevice_%lu |cut -d'}' -f2- -s |cut -d. -f3- -s | awk '{print $1$2}' > %s ", wifiTRPlatform_multiSSID[ulAPIndex].ifconfig_interface, vif_num, ulIndex, WLANCFG_AP_ASSOC_DEV_FILE);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    system(buf);
    memset(buf, 0, sizeof(buf));

    retVal = file_parse(WLANCFG_AP_ASSOC_DEV_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
          ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head);/*RDKB-7127, CID-33011, free unused resource before exit*/
        return retVal;
    }
    ptr = head;

    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "MACAddress")){
            if(getHex(ptr->param_val , assocDev_t->MacAddress, MAC_SZ) != SUCCESS){
                safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Macaddress read error !!!\n", __FUNCTION__);
                if(safec_rc < EOK){
                  ERR_CHK(safec_rc);
                }
                ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            }
        } else if(!strcasecmp(ptr->param_name,"AuthenticationState")) {
            assocDev_t->AuthenticationState = (0 == atoi(ptr->param_val)) ? FALSE : TRUE;
        } else if(!strcasecmp(ptr->param_name,"LastDataDownlinkRate")) {
            assocDev_t->LastDataDownlinkRate = atol(ptr->param_val);
            if((assocDev_t->LastDataDownlinkRate < 1000) || (assocDev_t->LastDataDownlinkRate > 600000))
                assocDev_t->LastDataDownlinkRate = 10000; /* Default Value */
        } else if(!strcasecmp(ptr->param_name,"LastDataUplinkRate")) {
            assocDev_t->LastDataUplinkRate = atol(ptr->param_val);
            if((assocDev_t->LastDataUplinkRate < 1000) || (assocDev_t->LastDataUplinkRate > 600000))
                assocDev_t->LastDataUplinkRate = 10000; /* Default Value */
        } else if(!strcasecmp(ptr->param_name,"SignalStrength")) {
            assocDev_t->SignalStrength = atoi(ptr->param_val);
            if((assocDev_t->SignalStrength >0) || (assocDev_t->SignalStrength < -200))
                assocDev_t->SignalStrength = -1; /* Default Value */
        } else if(!strcasecmp(ptr->param_name,"Active")) {
            assocDev_t->Active = (0 == atoi(ptr->param_val)) ? FALSE : TRUE;
        }
    }

    free_paramList(head);/*RDKB-7127, CID-33011, free unused resource before exit*/
    return SUCCESS;
}

unsigned long Utopia_GetWifiAPIndex(UtopiaContext *ctx, char *pSSID)
{
    int i = 0;
    char strVal[STR_SZ] = {'\0'};
    char *prefix;

    if ((NULL == ctx) || (NULL == pSSID)) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t safec_rc = -1;
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** with SSID %s!!!", __FUNCTION__,pSSID);
    if(safec_rc < EOK){
      ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    for(i=0;i<WIFI_SSID_NUM_INSTANCES;i++)
    {
	prefix =  wifiTRPlatform_multiSSID[i].syscfg_namespace_prefix ;
   	if(0 == Utopia_GetNamed(ctx,UtopiaValue_WLAN_SSID,prefix,strVal,sizeof(strVal)))
	   return ERR_SSID_NOT_FOUND;
        if(!(strcmp(pSSID,strVal)))
        {
           /* We have found the SSID. */
           return i;
        }
     }
     /* If it comes here, it means that we havent found the SSID - Some Problem */
     return ERR_SSID_NOT_FOUND;
}

int getMacList(char *macList, unsigned char *macAddr, char *tok, unsigned long *numlist)
{
    int i = 0;
    char *buf = NULL;
    unsigned char mac[50][6];

    if(!macList)
        return ERR_INVALID_ARGS;

    buf = strtok(macList, tok);
    while(buf != NULL)
    {
        getHexGeneric(buf, mac[i], 6);
        i++;
        buf = strtok(NULL, tok);
    }
    memcpy(macAddr, mac, 6*i);
    *numlist = i;

    return SUCCESS;
}

int setMacList(unsigned char *macAddr, char *macList, unsigned long numMac)
{
    int i;
    int j = 0;
    char mac[50][18];
    errno_t safec_rc = -1;
    if(!macList)
        return ERR_INVALID_ARGS;

    for(i =0; i<numMac; i++){
       if(i > 0){
           /* Here, macList is pointer, It's pointing to 1024 bytes data */
           safec_rc = strcat_s(macList, 1024," ");
           ERR_CHK(safec_rc);
       }
       safec_rc = sprintf_s(mac[i], 18,"%02x:%02x:%02x:%02x:%02x:%02x", macAddr[j], macAddr[j+1], macAddr[j+2], macAddr[j+3], macAddr[j+4], macAddr[j+5]);
       if(safec_rc < EOK){
          ERR_CHK(safec_rc);
       }
       safec_rc = strcat_s(macList, 1024,mac[i]);
       ERR_CHK(safec_rc);
       j +=6;
    }
    return SUCCESS;
}

int Utopia_GetWifiAPMFCfg(UtopiaContext *ctx, char *pSSID, void *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }

    char ulog_msg[256];
    wifiMacFilterCfg_t *cfg_t = (wifiMacFilterCfg_t *)cfg;
    char buf[1024] = {'\0'};
    char maclist[1024] = {'\0'};
    unsigned long numlist = 0;
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    char token[64] = {'\0'};
    int vif_num = 0;
    errno_t safec_rc = -1;

    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: ********Entered ****** with SSID %s!!!", __FUNCTION__,pSSID);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    ulogf(ULOG_CONFIG, UL_UTAPI, ulog_msg);

    int index = Utopia_GetWifiAPIndex(ctx, pSSID);

    /* Did we get the SSID */
    if(index < 0)
        return ERR_INVALID_ARGS;

    unsigned long ulIndex = index;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }

    if(ulIndex < 2)
        vif_num = 0;
    else {
        if(strlen(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix) > 4)
            vif_num = atoi(&(wifiTRPlatform_multiSSID[ulIndex].syscfg_namespace_prefix[4]));
        else
            vif_num = 0;
    }

    safec_rc = sprintf_s(buf, sizeof(buf),"wlancfg_tr %s %d | grep Filter | cut -d. -f5- -s | sed 's/ //' | sed 's/ /,/g' > %s", wifiTRPlatform_multiSSID[ulIndex].ifconfig_interface, vif_num, WIFI_MACFILTER_FILE);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    system(buf);

    retVal = file_parse(WIFI_MACFILTER_FILE, &head);

    if(retVal != SUCCESS){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head);/*RDKB-7127, CID-33359, free unused resource before exit*/
        return retVal;
    }
    ptr = head;
    if(!ptr){
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg),"%s: No nodes found!!!", __FUNCTION__);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_NO_NODES;
    }
    cfg_t->macFilterEnabled = FALSE;
    cfg_t->macFilterMode    = FALSE;

    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "MacFilterAccess")){
            if(!strcasecmp(ptr->param_val, "disabled"))
                cfg_t->macFilterMode = TRUE;
            else if(!strcasecmp(ptr->param_val, "allow")) 
                cfg_t->macFilterMode = FALSE;
            else if(!strcasecmp(ptr->param_val, "deny"))
                cfg_t->macFilterMode = TRUE;
        }else if(!strcasecmp(ptr->param_name, "MacFilterEnable")){ 
            cfg_t->macFilterEnabled = (!strncasecmp(ptr->param_val, "false", 5 ))? FALSE : TRUE;
        }else if(!strcasecmp(ptr->param_name, "MACAddressFilterList")){
            if(cfg_t->macFilterEnabled == FALSE)
            {
                Utopia_Get(ctx, UtopiaValue_WLAN_MACFilter, maclist, sizeof(maclist));
                if(strlen(maclist) <= 1){
                    numlist = 0;
                }else{
                    safec_rc = strcpy_s(token, sizeof(token)," ");
                    ERR_CHK(safec_rc);
                    getMacList(maclist, cfg_t->macAddress, token, &numlist);
                }
            }
            else
            {
                if(strlen(ptr->param_val) <= 1){
                    numlist = 0;
                }else{
                    safec_rc = strcpy_s(token, sizeof(token),",");
                    ERR_CHK(safec_rc);
                    getMacList(ptr->param_val, cfg_t->macAddress, token, &numlist);
                }
            }
        }else{
            continue;
        }
    }
    cfg_t->NumberMacAddrList = numlist;

    free_paramList(head);
    ptr = head = NULL;

    return SUCCESS;
}

int Utopia_SetWifiAPMFCfg(UtopiaContext *ctx, char *pSSID, void *cfg)
{
    char buf[BUF_SZ]   = {'\0'};
    char maclist[1024] = {'\0'};    
    char numlist[64]   = {'\0'};
    errno_t safec_rc = -1;

    if (NULL == ctx || NULL == cfg) {
        return ERR_INVALID_ARGS;
    }

    wifiMacFilterCfg_t *cfg_t = (wifiMacFilterCfg_t *)cfg;

    int index = Utopia_GetWifiAPIndex(ctx, pSSID);

    /* Did we get the SSID */
    if(index < 0)
        return ERR_INVALID_ARGS;

    unsigned long ulIndex = index;

    if(ulIndex >= WIFI_AP_NUM_INSTANCES){
        return ERR_INVALID_ARGS;
    }
    safec_rc = strcpy_s(buf, sizeof(buf),((cfg_t->macFilterEnabled == FALSE)? "false" : "true"));
    ERR_CHK(safec_rc);
    UTOPIA_SET(ctx, UtopiaValue_WLAN_Enable_MACFilter, buf);

    memset(buf, 0, BUF_SZ);
    if (cfg_t->macFilterEnabled == FALSE){
        safec_rc = strcpy_s(buf, sizeof(buf),"disabled");
        ERR_CHK(safec_rc);
        cfg_t->macFilterMode = TRUE;
    }else{
        safec_rc = strcpy_s(buf, sizeof(buf),((cfg_t->macFilterMode == TRUE)? "deny" : "allow"));
        ERR_CHK(safec_rc);
    }
    UTOPIA_SET(ctx, UtopiaValue_WLAN_AccessRestriction, buf);

    safec_rc = sprintf_s(numlist, sizeof(numlist),"%ld", cfg_t->NumberMacAddrList);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
    UTOPIA_SET(ctx, UtopiaValue_WLAN_NUM_MACFilter, numlist);

    /*Convert unsigned char to space separated string */
    setMacList(cfg_t->macAddress, maclist, cfg_t->NumberMacAddrList);
    UTOPIA_SET(ctx, UtopiaValue_WLAN_MACFilter, maclist);

    /* Set WLAN Restart event */
    Utopia_SetEvent(ctx,Utopia_Event_WLAN_Restart);

    return SUCCESS;
}

/* Utility Functions */
unsigned long instanceNum_find(unsigned long ulIndex, int *numArray, int numArrayLen)
{
    int i = 0;

    for(i=0;i<numArrayLen;i++)
    {
	if(ulIndex == numArray[i])
	   return i;
    }

    return 0;
}

unsigned long parse_proc_net_dev(char *if_name, int field_to_parse)
{
        char buf[BUF_SZ];
        char *bufptr;
        FILE *file;
        int if_name_len = strlen(if_name);
        unsigned long return_stats = 0;

        file = fopen("/proc/net/dev", "r");
        if (!file)
                return 0;

        while (!return_stats && fgets( buf, sizeof(buf), file ))
        {
                /* find the interface and make sure it matches -- space before the name and : after it */
                if ((bufptr = strstr(buf, if_name)) &&
                        (bufptr == buf || *(bufptr-1) == ' ') &&
                        *(bufptr + if_name_len) == ':')
                {
                        bufptr = bufptr + if_name_len + 1;

                        /* grab the nth field from it */
                        while( --field_to_parse && *bufptr != '\0')
                        {
                                while (*bufptr != '\0' && *(bufptr++) == ' ');
                                while (*bufptr != '\0' && *(bufptr++) != ' ');
                        }

                        /* get rid of any final spaces */
                        while (*bufptr != '\0' && *bufptr == ' ') bufptr++;

                        if (*bufptr != '\0')
                                return_stats = strtol(bufptr, NULL, 10);

                        break;
                }
        }

        fclose(file);
        return return_stats;
}

void allocateMultiSSID_Struct(int i)
{
    wifiTRPlatform_multiSSID[i].syscfg_namespace_prefix = (char *)malloc(STR_SZ);
    wifiTRPlatform_multiSSID[i].ifconfig_interface = (char *)malloc(STR_SZ);
    wifiTRPlatform_multiSSID[i].ssid_name = (char *)malloc(STR_SZ);
    wifiTRPlatform_multiSSID[i].ap_name = (char *)malloc(STR_SZ);
}

void freeMultiSSID_Struct(int i)
{
    free(wifiTRPlatform_multiSSID[i].syscfg_namespace_prefix);
    free(wifiTRPlatform_multiSSID[i].ifconfig_interface);     
    free(wifiTRPlatform_multiSSID[i].ssid_name);
    free(wifiTRPlatform_multiSSID[i].ap_name);
}
