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
 * Copyright (c) 2008-2009 Cisco Systems, Inc. All rights reserved.
 *
 * Cisco Systems, Inc. retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have obtained
 * a separate written license from Cisco Systems, Inc., you are not authorized
 * to utilize all or a part of this computer program for any purpose (including
 * reproduction, distribution, modification, and compilation into object code),
 * and you must immediately destroy or return to Cisco Systems, Inc. all copies
 * of this computer program.  If you are licensed by Cisco Systems, Inc., your
 * rights to utilize this computer program are limited by the terms of that
 * license.  To obtain a license, please contact Cisco Systems, Inc.
 *
 * This computer program contains trade secrets owned by Cisco Systems, Inc.
 * and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
 * maintain the confidentiality of this computer program and related information
 * and to not disclose this computer program and related information to any
 * other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
 * SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_wlan.h"
#include "DM_TR181.h"


int Utopia_Get_DeviceDnsRelayForwarding(UtopiaContext *pCtx, int index, void *str_handle)
{
    int iVal = -1;
    char tokenBuf[64] = {'\0'};
    char tokenVal[64] = {'\0'};
    if(!str_handle){
	sprintf(ulog_msg, "%s: Invalid Input Parameter", __FUNCTION__);
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    Obj_Device_DNS_Relay *deviceDnsRelay = (Obj_Device_DNS_Relay*)str_handle;

    sprintf(tokenBuf, "tr_dns_relay_forwarding_enable_%d", index);
    tokenBuf[strlen(tokenBuf)] = '\0';
    Utopia_RawGet(pCtx, NULL, tokenBuf, tokenVal, sizeof(tokenVal));
    deviceDnsRelay->Enable = (!strncasecmp(tokenVal, "false", 5))? FALSE : TRUE ;  
    sprintf(ulog_msg, "%s: Get Enable key & val = %s, %u", __FUNCTION__, tokenBuf, deviceDnsRelay->Enable);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    
    memset(tokenBuf, 0, sizeof(tokenBuf));
    memset(tokenVal, 0, sizeof(tokenVal));
    sprintf(tokenBuf, "tr_dns_relay_forwarding_server_%d", index);
    tokenBuf[strlen(tokenBuf)] = '\0';
    Utopia_RawGet(pCtx, NULL, tokenBuf, tokenVal, sizeof(tokenVal));
    deviceDnsRelay->DNSServer.Value = inet_addr(tokenVal);
    
    memset(tokenBuf, 0, sizeof(tokenBuf));
    memset(tokenVal, 0, sizeof(tokenVal));
    sprintf(tokenBuf, "tr_dns_relay_forwarding_interface_%d", index);
    tokenBuf[strlen(tokenBuf)] = '\0';
    Utopia_RawGet(pCtx, NULL, tokenBuf, tokenVal, sizeof(tokenVal));
    tokenVal[strlen(tokenVal)] = '\0';
    strcpy(deviceDnsRelay->Interface, tokenVal);

    return UT_SUCCESS;
}

int Utopia_Set_DeviceDnsRelayForwarding(UtopiaContext *pCtx, int index, void *str_handle)
{
    int iVal = -1;
    char tokenBuf[64] = {'\0'};
    char tokenVal[64] = {'\0'};
    char cmd[128] = {'\0'};
    if (!pCtx || !str_handle) {
	sprintf(ulog_msg, "%s: Invalid Input Parameter", __FUNCTION__);
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    Obj_Device_DNS_Relay *deviceDnsRelay = (Obj_Device_DNS_Relay*)str_handle;

    sprintf(tokenBuf, "tr_dns_relay_forwarding_enable_%d", index);
    tokenBuf[strlen(tokenBuf)] = '\0';
    sprintf(ulog_msg, "%s: Set Enable key & val = %s, %u", __FUNCTION__, tokenBuf, deviceDnsRelay->Enable);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    if(deviceDnsRelay->Enable == FALSE){
        sprintf(ulog_msg, "%s: Enable is FALSE \n", __FUNCTION__, tokenBuf);
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        Utopia_RawSet(pCtx, NULL, tokenBuf, "false");
    }else{
        sprintf(ulog_msg, "%s: Enable is TRUE \n", __FUNCTION__, tokenBuf);
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        Utopia_RawSet(pCtx, NULL, tokenBuf, "true");
    }
    memset(tokenBuf, 0, sizeof(tokenBuf));
    memset(tokenVal, 0, sizeof(tokenVal));
    sprintf(tokenBuf, "tr_dns_relay_forwarding_server_%d", index);
    tokenBuf[strlen(tokenBuf)] = '\0';
    sprintf(tokenVal, "%d.%d.%d.%d", 
                      (deviceDnsRelay->DNSServer.Value) & 0xFF,
                      (deviceDnsRelay->DNSServer.Value >> 8)  & 0xFF,
                      (deviceDnsRelay->DNSServer.Value >> 16) & 0xFF,
                      (deviceDnsRelay->DNSServer.Value >> 24) & 0xFF );
    tokenVal[strlen(tokenVal)] = '\0';
    Utopia_RawSet(pCtx, NULL, tokenBuf, tokenVal);

    memset(tokenBuf, 0, sizeof(tokenBuf));
    memset(tokenVal, 0, sizeof(tokenVal));
    sprintf(tokenBuf, "tr_dns_relay_forwarding_interface_%d", index);
    tokenBuf[strlen(tokenBuf)] = '\0';
    strncpy(tokenVal, deviceDnsRelay->Interface, strlen(deviceDnsRelay->Interface));
    tokenVal[strlen(deviceDnsRelay->Interface)] = '\0';
    Utopia_RawSet(pCtx, NULL, tokenBuf, tokenVal);
    
    return UT_SUCCESS;
}
