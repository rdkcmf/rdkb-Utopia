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
#include "utapi_util.h"
#include "utapi_parental_control.h"

int Utopia_GetBlkURLCfg(UtopiaContext *ctx, int *enable)
{
    UTOPIA_GETBOOL(ctx, UtopiaValue_ParentalControl_ManagedSites_Enabled, enable);
    return SUCCESS;
}

int Utopia_SetBlkURLCfg(UtopiaContext *ctx, const int enable)
{
    UTOPIA_SETBOOL(ctx, UtopiaValue_ParentalControl_ManagedSites_Enabled, enable);
    return SUCCESS;
}

int Utopia_GetMngServsCfg(UtopiaContext *ctx, int *enable)
{
    UTOPIA_GETBOOL(ctx, UtopiaValue_ParentalControl_ManagedServices_Enabled, enable);
    return SUCCESS;
}

int Utopia_SetMngServsCfg(UtopiaContext *ctx, const int enable)
{
    UTOPIA_SETBOOL(ctx, UtopiaValue_ParentalControl_ManagedServices_Enabled, enable);
    return SUCCESS;
}

int Utopia_GetMngDevsCfg(UtopiaContext *ctx, mng_devs_t *mng_devs)
{
    UTOPIA_GETBOOL(ctx, UtopiaValue_ParentalControl_ManagedDevices_Enabled, &mng_devs->enable);
    UTOPIA_GETBOOL(ctx, UtopiaValue_ParentalControl_ManagedDevices_AllowAll, &mng_devs->allow_all);
    return SUCCESS;
}

int Utopia_SetMngDevsCfg(UtopiaContext *ctx, const mng_devs_t *mng_devs)
{
    UTOPIA_SETBOOL(ctx, UtopiaValue_ParentalControl_ManagedDevices_Enabled, mng_devs->enable);
    UTOPIA_SETBOOL(ctx, UtopiaValue_ParentalControl_ManagedDevices_AllowAll, mng_devs->allow_all);
    return SUCCESS;
}

int Utopia_GetBlkURLInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins)
{
    return Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_InsNum, uIndex+1, ins);
}

static int g_ParentalControl_ManagedSiteBlockCount = 0;

int Utopia_GetNumberOfBlkURL(UtopiaContext *ctx, int *num)
{
    int rc = SUCCESS;
    
    if(g_ParentalControl_ManagedSiteBlockCount == 0)
        Utopia_GetInt(ctx, UtopiaValue_ParentalControl_ManagedSiteBlockedCount, &g_ParentalControl_ManagedSiteBlockCount);
    
    *num = g_ParentalControl_ManagedSiteBlockCount;
    return rc;
}

int Utopia_GetBlkURLByIndex(UtopiaContext *ctx, unsigned long ulIndex, blkurl_t *blkurl)
{
    int ins_num;
    int index = ulIndex+1;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_InsNum, index, &ins_num); blkurl->ins_num = ins_num;
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Alias, index, blkurl->alias, sizeof(blkurl->alias));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Method, index, blkurl->block_method, sizeof(blkurl->block_method));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Site, index, blkurl->site, sizeof(blkurl->site));
    Utopia_GetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Always, index, &blkurl->always_block);
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_StartTime, index, blkurl->start_time, sizeof(blkurl->start_time));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_EndTime, index, blkurl->end_time, sizeof(blkurl->end_time));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Days, index, blkurl->block_days, sizeof(blkurl->block_days));
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_MAC, index, blkurl->mac, sizeof(blkurl->mac));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_DeviceName, index, blkurl->device_name, sizeof(blkurl->device_name));
#endif
    return 0;
}

int Utopia_SetBlkURLByIndex(UtopiaContext *ctx, unsigned long ulIndex, const blkurl_t *blkurl)
{
    int index = ulIndex+1;
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "pcms_%d", index);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked, index, s_tokenbuf);

    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_InsNum, index, blkurl->ins_num);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Alias, index, (char*)blkurl->alias);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Method, index, (char*)blkurl->block_method);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Site, index, (char*)blkurl->site);
    Utopia_SetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Always, index, blkurl->always_block);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_StartTime, index, (char*)blkurl->start_time);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_EndTime, index, (char*)blkurl->end_time);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Days, index, (char*)blkurl->block_days);
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_MAC, index, (char*)blkurl->mac);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_DeviceName, index, (char*)blkurl->device_name);
#endif
    return 0;
}

/*
 * Set instance number and alias to specific entry by index 
 */
int Utopia_SetBlkURLInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ins, const char *alias)
{
    int index = ulIndex+1;
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_InsNum, index, ins);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Alias, index, (char*)alias);
    return 0;
}

int Utopia_AddBlkURL(UtopiaContext *ctx, const blkurl_t *blkurl)
{
    int index;

    Utopia_GetNumberOfBlkURL(ctx, &index);

    g_ParentalControl_ManagedSiteBlockCount++;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedSiteBlockedCount, g_ParentalControl_ManagedSiteBlockCount);

    Utopia_SetBlkURLByIndex(ctx, index, blkurl);

    printf("%s-%d RDKB_PCONTROL[URL]:%lu,%s\n",__FUNCTION__,__LINE__,blkurl->ins_num,blkurl->site);
    return 0;
}

int Utopia_DelBlkURL(UtopiaContext *ctx, unsigned long ins)
{
    int count, index;

    Utopia_GetNumberOfBlkURL(ctx, &count);
    for (index = 0; index < count; index++)
    {
        int ins_num;
        Utopia_GetBlkURLInsNumByIndex(ctx, index, &ins_num);
        if (ins_num == (int)ins) break;
    }

    if (index >= count)
    {
        return -1;
    }

    if (index < count-1)
    {
        for (;index < count-1; index++)
        {
            blkurl_t blkurl;
            Utopia_GetBlkURLByIndex(ctx, index+1, &blkurl);
            Utopia_SetBlkURLByIndex(ctx, index, &blkurl);
        }
    }

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_InsNum, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Alias, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Method, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Site, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Always, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_StartTime, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_EndTime, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_Days, count);
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_MAC, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked_DeviceName, count);
#endif
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteBlocked, count);

    g_ParentalControl_ManagedSiteBlockCount--;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedSiteBlockedCount, g_ParentalControl_ManagedSiteBlockCount);

    Utopia_GetNumberOfBlkURL(ctx, &count);
    for (index = 0; index < count; index++)
    {
        blkurl_t blkurl;
        Utopia_GetBlkURLByIndex(ctx, index, &blkurl);
        printf("%s-%d RDKB_PCONTROL[URL]:%lu,%s\n",__FUNCTION__,__LINE__,blkurl.ins_num,blkurl.site);
    }
    return 0;
}

int Utopia_GetTrustedUserInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins)
{
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_InsNum, uIndex+1, ins);
    return 0;
}

static int g_ParentalControl_ManagedSiteTrustCount = 0;
 
int Utopia_GetNumberOfTrustedUser(UtopiaContext *ctx, int *num)
{
    if(g_ParentalControl_ManagedSiteTrustCount == 0)
        Utopia_GetInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrustedCount, &g_ParentalControl_ManagedSiteTrustCount);

    *num = g_ParentalControl_ManagedSiteTrustCount;
    return 0;
}

int Utopia_GetTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, trusted_user_t *trusted_user)
{
    int ins_num;
    int index = ulIndex+1;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_InsNum, index, &ins_num); trusted_user->ins_num = ins_num;
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Alias, index, trusted_user->alias, sizeof(trusted_user->alias));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Desc, index, trusted_user->host_descp, sizeof(trusted_user->host_descp));
    int iptype;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_IpType, index, &iptype); trusted_user->ipaddrtype = iptype;
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_IpAddr, index, trusted_user->ipaddr, sizeof(trusted_user->ipaddr));
    Utopia_GetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Trusted, index, &trusted_user->trusted);
    return 0;
}

int Utopia_SetTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, const trusted_user_t *trusted_user)
{
    int index = ulIndex+1;
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "pcmst_%d", index);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted, index, s_tokenbuf);

    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_InsNum, index, trusted_user->ins_num);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Alias, index, (char*)trusted_user->alias);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Desc, index, (char*)trusted_user->host_descp);
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_IpType, index, trusted_user->ipaddrtype);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_IpAddr, index, (char*)trusted_user->ipaddr);
    Utopia_SetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Trusted, index, trusted_user->trusted);
    return 0;
}

int Utopia_SetTrustedUserInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias)
{
    int index = ulIndex+1;
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_InsNum, index, ins);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Alias, index, (char*)alias);
    return 0;
}

int Utopia_AddTrustedUser(UtopiaContext *ctx, const trusted_user_t *trusted_user)
{
    int index;

    Utopia_GetNumberOfTrustedUser(ctx, &index);

    g_ParentalControl_ManagedSiteTrustCount++;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrustedCount, g_ParentalControl_ManagedSiteTrustCount);

    Utopia_SetTrustedUserByIndex(ctx, index, trusted_user);
    printf("%s-%d RDKB_PCONTROL[TUSER]:%lu,%s\n",__FUNCTION__,__LINE__,trusted_user->ins_num,trusted_user->ipaddr);
    return 0;
}

int Utopia_DelTrustedUser(UtopiaContext *ctx, unsigned long ins)
{
    int count, index;

    Utopia_GetNumberOfTrustedUser(ctx, &count);
    for (index = 0; index < count; index++)
    {
        int ins_num;
        Utopia_GetTrustedUserInsNumByIndex(ctx, index, &ins_num);
        if (ins_num == (int)ins) break;
    }

    if (index >= count)
    {
        return -1;
    }

    if (index < count-1)
    {
        for (;index < count-1; index++)
        {
            trusted_user_t trusted_user;
            Utopia_GetTrustedUserByIndex(ctx, index+1, &trusted_user);
            Utopia_SetTrustedUserByIndex(ctx, index, &trusted_user);
        }
    }

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_InsNum, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Alias, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Desc, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_IpType, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_IpAddr, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted_Trusted, count);

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedSiteTrusted, count);

    g_ParentalControl_ManagedSiteTrustCount--;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedSiteTrustedCount, g_ParentalControl_ManagedSiteTrustCount);

    Utopia_GetNumberOfTrustedUser(ctx, &count);
    for (index = 0; index < count; index++)
    {
        trusted_user_t trusted_user;
        Utopia_GetTrustedUserByIndex(ctx, index, &trusted_user);
        printf("%s-%d RDKB_PCONTROL[TUSER]:%lu,%s\n",__FUNCTION__,__LINE__,trusted_user.ins_num,trusted_user.ipaddr);
    }
    return 0;
}

int Utopia_GetMSServInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins)
{
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_InsNum, uIndex+1, ins);
    return 0;
}

static int g_ParentalControl_ManagedServiceBlockCount = 0;
 
int Utopia_GetNumberOfMSServ(UtopiaContext *ctx, int *num)
{
    if(g_ParentalControl_ManagedServiceBlockCount == 0)
        Utopia_GetInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlockedCount, &g_ParentalControl_ManagedServiceBlockCount);

    *num = g_ParentalControl_ManagedServiceBlockCount;
    return 0;
}

int Utopia_GetMSServByIndex(UtopiaContext *ctx, unsigned long ulIndex, ms_serv_t *ms_serv)
{
    int ins_num;
    int index = ulIndex+1;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_InsNum, index, &ins_num); ms_serv->ins_num = ins_num;
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Alias, index, ms_serv->alias, sizeof(ms_serv->alias));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Desc, index, ms_serv->descp, sizeof(ms_serv->descp));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Proto, index, ms_serv->protocol, sizeof(ms_serv->protocol));
    int start_port, end_port;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_StartPort, index, &start_port); ms_serv->start_port = start_port;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_EndPort, index, &end_port); ms_serv->end_port = end_port;
    Utopia_GetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Always, index, &ms_serv->always_block);
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_StartTime, index, ms_serv->start_time, sizeof(ms_serv->start_time));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_EndTime, index, ms_serv->end_time, sizeof(ms_serv->end_time));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Days, index, ms_serv->block_days, sizeof(ms_serv->block_days));

    return 0;
}

int Utopia_SetMSServByIndex(UtopiaContext *ctx, unsigned long ulIndex, const ms_serv_t *ms_serv)
{
    int index = ulIndex+1;
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "pcmse_%d", index);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked, index, s_tokenbuf);

    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_InsNum, index, ms_serv->ins_num);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Alias, index, (char*)ms_serv->alias);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Desc, index, (char*)ms_serv->descp);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Proto, index, (char*)ms_serv->protocol);
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_StartPort, index, ms_serv->start_port);
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_EndPort, index, ms_serv->end_port);
    Utopia_SetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Always, index, ms_serv->always_block);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_StartTime, index, (char*)ms_serv->start_time);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_EndTime, index, (char*)ms_serv->end_time);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Days, index, (char*)ms_serv->block_days);

    return 0;
}

int Utopia_SetMSServInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias)
{
    int index = ulIndex+1;
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_InsNum, index, ins);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Alias, index, (char*)alias);
    return 0;
}

int Utopia_AddMSServ(UtopiaContext *ctx, const ms_serv_t *ms_serv)
{
    int index;

    Utopia_GetNumberOfMSServ(ctx, &index);

    g_ParentalControl_ManagedServiceBlockCount++;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlockedCount, g_ParentalControl_ManagedServiceBlockCount);

    Utopia_SetMSServByIndex(ctx, index, ms_serv);
    printf("%s-%d RDKB_PCONTROL[MSSERV]:%lu,%s\n",__FUNCTION__,__LINE__,ms_serv->ins_num,ms_serv->descp);
    return 0;
}

int Utopia_DelMSServ(UtopiaContext *ctx, unsigned long ins)
{
    int count, index;

    Utopia_GetNumberOfMSServ(ctx, &count);
    for (index = 0; index < count; index++)
    {
        int ins_num;
        Utopia_GetMSServInsNumByIndex(ctx, index, &ins_num);
        if (ins_num == (int)ins) break;
    }

    if (index >= count)
    {
        return -1;
    }

    if (index < count-1)
    {
        for (;index < count-1; index++)
        {
            ms_serv_t ms_serv;
            Utopia_GetMSServByIndex(ctx, index+1, &ms_serv);
            Utopia_SetMSServByIndex(ctx, index, &ms_serv);
        }
    }

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_InsNum, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Alias, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Desc, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Proto, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_StartPort, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_EndPort, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Always, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_StartTime, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_EndTime, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked_Days, count);

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceBlocked, count);

    g_ParentalControl_ManagedServiceBlockCount--;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedServiceBlockedCount, g_ParentalControl_ManagedServiceBlockCount);
    Utopia_GetNumberOfMSServ(ctx, &count);
    for (index = 0; index < count; index++)
    {
        ms_serv_t ms_serv;
        Utopia_GetMSServByIndex(ctx, index, &ms_serv);
        printf("%s-%d RDKB_PCONTROL[MSSERV]:%lu,%s\n",__FUNCTION__,__LINE__,ms_serv.ins_num,ms_serv.descp);
    }

    return 0;
}

int Utopia_GetMSTrustedUserInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins)
{
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_InsNum, uIndex+1, ins);
    return 0;
}

static int g_ParentalControl_ManagedServiceTrustCount = 0;
 
int Utopia_GetNumberOfMSTrustedUser(UtopiaContext *ctx, int *num)
{
    if(g_ParentalControl_ManagedServiceTrustCount == 0)
        Utopia_GetInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrustedCount, &g_ParentalControl_ManagedServiceTrustCount);
    
    *num = g_ParentalControl_ManagedServiceTrustCount;
    return 0;
}

int Utopia_GetMSTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, ms_trusteduser_t *ms_trusteduser)
{
    int ins_num, iptype;
    int index = ulIndex+1;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_InsNum, index, &ins_num); ms_trusteduser->ins_num = ins_num;
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Alias, index, ms_trusteduser->alias, sizeof(ms_trusteduser->alias));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Desc, index, ms_trusteduser->host_descp, sizeof(ms_trusteduser->host_descp));
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_IpType, index, &iptype); ms_trusteduser->ipaddrtype = iptype;
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_IpAddr, index, ms_trusteduser->ipaddr, sizeof(ms_trusteduser->ipaddr));
    Utopia_GetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Trusted, index, &ms_trusteduser->trusted);
    return 0;
}

int Utopia_SetMSTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, const ms_trusteduser_t *ms_trusteduser)
{
    int index = ulIndex+1;
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "pcmset_%d", index);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted, index, s_tokenbuf);

    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_InsNum, index, ms_trusteduser->ins_num);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Alias, index, (char*)ms_trusteduser->alias);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Desc, index, (char*)ms_trusteduser->host_descp);
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_IpType, index, ms_trusteduser->ipaddrtype);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_IpAddr, index, (char*)ms_trusteduser->ipaddr);
    Utopia_SetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Trusted, index, ms_trusteduser->trusted);
    return 0;
}

int Utopia_SetMSTrustedUserInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias)
{
    int index = ulIndex+1;
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_InsNum, index, ins);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Alias, index, (char*)alias);
    return 0;
}

int Utopia_AddMSTrustedUser(UtopiaContext *ctx, const ms_trusteduser_t *ms_trusteduser)
{
    int index;

    Utopia_GetNumberOfMSTrustedUser(ctx, &index);

    g_ParentalControl_ManagedServiceTrustCount++;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrustedCount, g_ParentalControl_ManagedServiceTrustCount);

    Utopia_SetMSTrustedUserByIndex(ctx, index, ms_trusteduser);

    printf("%s-%d RDKB_PCONTROL[MSTUSER]:%lu,%s\n",__FUNCTION__,__LINE__,ms_trusteduser->ins_num,ms_trusteduser->ipaddr);
    return 0;
}

int Utopia_DelMSTrustedUser(UtopiaContext *ctx, unsigned long ins)
{
    int count, index;

    Utopia_GetNumberOfMSTrustedUser(ctx, &count);
    for (index = 0; index < count; index++)
    {
        int ins_num;
        Utopia_GetMSTrustedUserInsNumByIndex(ctx, index, &ins_num);
        if (ins_num == (int)ins) break;
    }

    if (index >= count)
    {
        return -1;
    }

    if (index < count-1)
    {
        for (;index < count-1; index++)
        {
            ms_trusteduser_t trusted_user;
            Utopia_GetMSTrustedUserByIndex(ctx, index+1, &trusted_user);
            Utopia_SetMSTrustedUserByIndex(ctx, index, &trusted_user);
        }
    }

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_InsNum, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Alias, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Desc, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_IpType, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_IpAddr, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted_Trusted, count);

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedServiceTrusted, count);

    g_ParentalControl_ManagedServiceTrustCount--;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedServiceTrustedCount, g_ParentalControl_ManagedServiceTrustCount);

    Utopia_GetNumberOfMSTrustedUser(ctx, &count);
    for (index=0; index < count; index++)
    {
        ms_trusteduser_t trusted_user;
        Utopia_GetMSTrustedUserByIndex(ctx, index, &trusted_user);
        printf("%s-%d RDKB_PCONTROL[MSTUSER]:%lu,%s\n",__FUNCTION__,__LINE__,trusted_user.ins_num,trusted_user.ipaddr);
    }
    return 0;
}

int Utopia_GetMDDevInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins)
{
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedDevice_InsNum, uIndex+1, ins);
    return 0;
}

static int g_ParentalControl_ManagedDeviceCount = 0;
 
int Utopia_GetNumberOfMDDev(UtopiaContext *ctx, int *num)
{
    if(g_ParentalControl_ManagedDeviceCount == 0)
        Utopia_GetInt(ctx, UtopiaValue_ParentalControl_ManagedDeviceCount, &g_ParentalControl_ManagedDeviceCount);
    
    *num = g_ParentalControl_ManagedDeviceCount;
    return 0;
}

int Utopia_GetMDDevByIndex(UtopiaContext *ctx, unsigned long ulIndex, md_dev_t *md_dev)
{
    int ins_num;
    int index = ulIndex+1;
    Utopia_GetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedDevice_InsNum, index, &ins_num); md_dev->ins_num = ins_num;
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Alias, index, md_dev->alias, sizeof(md_dev->alias));
    Utopia_GetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedDevice_Block, index, &md_dev->is_block);
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Desc, index, md_dev->descp, sizeof(md_dev->descp));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_MacAddr, index, md_dev->macaddr, sizeof(md_dev->macaddr));
    Utopia_GetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedDevice_Always, index, &md_dev->always);
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_StartTime, index, md_dev->start_time, sizeof(md_dev->start_time));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_EndTime, index, md_dev->end_time, sizeof(md_dev->end_time));
    Utopia_GetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Days, index, md_dev->block_days, sizeof(md_dev->block_days));
    return 0;
}

int Utopia_SetMDDevByIndex(UtopiaContext *ctx, unsigned long ulIndex, const md_dev_t *md_dev)
{
    int index = ulIndex+1;
    snprintf(s_tokenbuf, sizeof(s_tokenbuf), "pcmd_%d", index);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice, index, s_tokenbuf);

    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedDevice_InsNum, index, md_dev->ins_num);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Alias, index, (char*)md_dev->alias);
    Utopia_SetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedDevice_Block, index, md_dev->is_block);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Desc, index, (char*)md_dev->descp);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_MacAddr, index, (char*)md_dev->macaddr);
    Utopia_SetIndexedBool(ctx, UtopiaValue_ParentalControl_ManagedDevice_Always, index, md_dev->always);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_StartTime, index, (char*)md_dev->start_time);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_EndTime, index, (char*)md_dev->end_time);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Days, index, (char*)md_dev->block_days);
    return 0;
}

int Utopia_SetMDDevInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias)
{
    int index = ulIndex+1;
    Utopia_SetIndexedInt(ctx, UtopiaValue_ParentalControl_ManagedDevice_InsNum, index, ins);
    Utopia_SetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Alias, index, (char*)alias);
    return 0;
}

int Utopia_AddMDDev(UtopiaContext *ctx, const md_dev_t *md_dev)
{
    int index,i,j,size;
    char mac_addr[12];

    Utopia_GetNumberOfMDDev(ctx, &index);

    g_ParentalControl_ManagedDeviceCount++;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedDeviceCount, g_ParentalControl_ManagedDeviceCount);

    Utopia_SetMDDevByIndex(ctx, index, md_dev);
    j = 0;
    size = strlen(md_dev->macaddr);
    for (i = 0; i < size; i++) {
    	if ((md_dev->macaddr)[i] != ':'){
            char ch1 = (md_dev->macaddr)[i];
            mac_addr[j] = ch1;
            j++;
        }
    }
    mac_addr[j] = '\0';
    printf("%s-%d RDKB_PCONTROL[MDDEV]:%lu,%s\n",__FUNCTION__,__LINE__,md_dev->ins_num,mac_addr);
    return 0;
}

int Utopia_DelMDDev(UtopiaContext *ctx, unsigned long ins)
{
    int count, index, i, j, size;
    char mac_addr[12];

    Utopia_GetNumberOfMDDev(ctx, &count);
    for (index = 0; index < count; index++)
    {
        int ins_num;
        Utopia_GetMDDevInsNumByIndex(ctx, index, &ins_num);
        if (ins_num == (int)ins) break;
    }

    if (index >= count)
    {
        return -1;
    }

    if (index < count-1)
    {
        for (;index < count-1; index++)
        {
            md_dev_t md_dev;
            Utopia_GetMDDevByIndex(ctx, index+1, &md_dev);
            Utopia_SetMDDevByIndex(ctx, index, &md_dev);
        }
    }

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_InsNum, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Alias, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Block, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Desc, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_MacAddr, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Always, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_StartTime, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_EndTime, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice_Days, count);

    Utopia_UnsetIndexed(ctx, UtopiaValue_ParentalControl_ManagedDevice, count);

    g_ParentalControl_ManagedDeviceCount--;
    Utopia_SetInt(ctx, UtopiaValue_ParentalControl_ManagedDeviceCount, g_ParentalControl_ManagedDeviceCount);

    Utopia_GetNumberOfMDDev(ctx, &count);
    for (index=0; index < count; index++)
    {
	md_dev_t md_dev;
	Utopia_GetMDDevByIndex(ctx, index, &md_dev);
	j = 0;
        size = strlen(md_dev.macaddr);
        for (i = 0; i < size; i++) {
            if (md_dev.macaddr[i] != ':'){
                char ch1 = md_dev.macaddr[i];
                mac_addr[j] = ch1;
                j++;
            }
    	}
        mac_addr[j] = '\0';
        printf("%s-%d RDKB_PCONTROL[MDDEV]:%lu,%s\n",__FUNCTION__,__LINE__,md_dev.ins_num,mac_addr);
    }
    return 0;
}
