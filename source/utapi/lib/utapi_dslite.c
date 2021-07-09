/*#####################################################################
# Copyright 2017-2019 ARRIS Enterprises, LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#####################################################################*/
#ifdef DSLITE_FEATURE_SUPPORT
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_dslite.h"
#include "DM_TR181.h"
#include "safec_lib_common.h"

static int g_IndexMapDslite[MAX_NUM_INSTANCES+1] = {-1};

int Utopia_GetDsliteEnable(UtopiaContext *ctx, boolean_t *bEnabled)
{
    boolean_t flag;
#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_GetBool(ctx, UtopiaValue_Dslite_Enable, &flag);
    if (TRUE == flag)
    {
        *bEnabled = TRUE ;
    }
    else
    {
        *bEnabled = FALSE ;
    }
    return SUCCESS;
}

int Utopia_SetDsliteEnable(UtopiaContext *ctx, boolean_t bEnabled)
{
#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** with !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    UTOPIA_SETBOOL(ctx, UtopiaValue_Dslite_Enable, bEnabled);

    return SUCCESS;
}

int Utopia_GetNumOfDsliteEntries(UtopiaContext *ctx,unsigned long *cnt)
{
    int ivalue;
#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_GetInt(ctx, UtopiaValue_Dslite_Count, &ivalue);
    *cnt = ivalue;

    return SUCCESS;
}

int Utopia_SetNumOfDsliteEntries(UtopiaContext *ctx,unsigned long cnt)
{
#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** with !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    UTOPIA_SETINT(ctx, UtopiaValue_Dslite_Count, cnt);

    return SUCCESS;
}

int Utopia_GetDsliteCfg(UtopiaContext *ctx,DsLiteCfg_t *pDsliteCfg)
{
    unsigned long ulIndex = 0;
    DsLiteCfg_t DsLiteCfg_tmp;

    if((NULL == ctx) || (NULL == pDsliteCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if(0 == pDsliteCfg->InstanceNumber)
        return ERR_INVALID_ARGS;

    memset(&DsLiteCfg_tmp, 0, sizeof(DsLiteCfg_t));
    ulIndex = g_IndexMapDslite[pDsliteCfg->InstanceNumber];
    Utopia_GetDsliteByIndex(ctx,ulIndex,&DsLiteCfg_tmp);
    memcpy(pDsliteCfg, &DsLiteCfg_tmp, sizeof(DsLiteCfg_t));
    return SUCCESS;
}

int Utopia_SetDsliteCfg(UtopiaContext *ctx,DsLiteCfg_t *pDsliteCfg)
{
    unsigned long ulIndex = 0;
    DsLiteCfg_t DsLiteCfg_tmp;

    if((NULL == ctx) || (NULL == pDsliteCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    memset(&DsLiteCfg_tmp, 0, sizeof(DsLiteCfg_t));
    memcpy(&DsLiteCfg_tmp, pDsliteCfg, sizeof(DsLiteCfg_t));

    if(0 == DsLiteCfg_tmp.InstanceNumber)
        return ERR_INVALID_ARGS;

    ulIndex = g_IndexMapDslite[DsLiteCfg_tmp.InstanceNumber];
    Utopia_SetDsliteByIndex(ctx,ulIndex,&DsLiteCfg_tmp);
    return SUCCESS;
}

int Utopia_AddDsliteEntry(UtopiaContext *ctx, DsLiteCfg_t *pDsliteCfg)
{
    unsigned long ulIndex = 0;
    unsigned long count = 0;
    DsLiteCfg_t DsLiteCfg_tmp;

    if((NULL == ctx) || (NULL == pDsliteCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    memset(&DsLiteCfg_tmp, 0, sizeof(DsLiteCfg_t));
    memcpy(&DsLiteCfg_tmp, pDsliteCfg, sizeof(DsLiteCfg_t));

    if(0 == DsLiteCfg_tmp.InstanceNumber)
        return ERR_INVALID_ARGS;

    Utopia_GetNumOfDsliteEntries(ctx,&count);
    ulIndex = count;
    g_IndexMapDslite[DsLiteCfg_tmp.InstanceNumber] = ulIndex;
    Utopia_SetIndexedInt(ctx, UtopiaValue_Dslite_InsNum, (ulIndex + 1) ,DsLiteCfg_tmp.InstanceNumber);
    Utopia_SetDsliteByIndex(ctx,ulIndex,&DsLiteCfg_tmp);
    Utopia_SetNumOfDsliteEntries(ctx,(count + 1));

    return SUCCESS;
}

int Utopia_DelDsliteEntry(UtopiaContext *ctx, unsigned long ulInstanceNumber)
{
    unsigned long count = 0;
    unsigned long ulIndex = 0;
    DsLiteCfg_t dsliteCfg;

    if(NULL == ctx){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if(0 == ulInstanceNumber)
        return ERR_INVALID_ARGS;
    ulIndex = g_IndexMapDslite[ulInstanceNumber];

    Utopia_GetNumOfDsliteEntries(ctx,&count);
    count = count - 1;
    Utopia_SetNumOfDsliteEntries(ctx,count);

    if(count != 0)
    {
       ulIndex++;
       for(;ulIndex <= count; ulIndex++)
       {
          Utopia_GetDsliteByIndex(ctx,ulIndex,&dsliteCfg);
          Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_InsNum,(ulIndex + 1),(int *)&dsliteCfg.InstanceNumber);
          Utopia_SetIndexedInt(ctx,UtopiaValue_Dslite_InsNum,ulIndex,dsliteCfg.InstanceNumber);

          g_IndexMapDslite[dsliteCfg.InstanceNumber] = (ulIndex - 1);

          Utopia_SetDsliteByIndex(ctx,(ulIndex - 1),&dsliteCfg);
        }
        /* Now unset the last index */
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_InsNum,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Active,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Alias,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Mode,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Addr_Type,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Addr_Fqdn,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Addr_IPv6,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Mss_Clamping_Enable,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Tcpmss,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_IPv6_Frag_Enable,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Dslite_Tunnel_V4Addr,ulIndex);
    }

    return SUCCESS;
}

int Utopia_GetDsliteEntry(UtopiaContext *ctx,unsigned long ulIndex, void *pDsliteEntry)
{
    if((NULL == ctx) || (NULL == pDsliteEntry)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

   DsLiteCfg_t DsLiteCfg_tmp;

   memset(&DsLiteCfg_tmp, 0, sizeof(DsLiteCfg_t));
   /* Do we have an InstanceNumber already ? */
   if(0 != Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_InsNum,(ulIndex + 1), (int *)&(DsLiteCfg_tmp.InstanceNumber))) {
       DsLiteCfg_tmp.InstanceNumber = 0;
   } else {
       g_IndexMapDslite[DsLiteCfg_tmp.InstanceNumber] = ulIndex;
   }

   Utopia_GetDsliteByIndex(ctx,ulIndex,&DsLiteCfg_tmp);
   memcpy(pDsliteEntry, &DsLiteCfg_tmp, sizeof(DsLiteCfg_t));

   return SUCCESS;
}

int Utopia_SetDsliteInsNum(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNumber)
{
    if(NULL == ctx){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_SetIndexedInt(ctx, UtopiaValue_Dslite_InsNum, (ulIndex + 1) ,ulInstanceNumber);
    g_IndexMapDslite[ulInstanceNumber] = ulIndex;
    return SUCCESS;
}

int Utopia_GetDsliteByIndex(UtopiaContext *ctx, unsigned long ulIndex, DsLiteCfg_t *pDsLiteCfg_t)
{
#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_InsNum,(ulIndex + 1),(int *)&(pDsLiteCfg_t->InstanceNumber));
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_Active,(ulIndex + 1),&(pDsLiteCfg_t->active));
    Utopia_GetIndexed(ctx,UtopiaValue_Dslite_Alias,(ulIndex + 1),pDsLiteCfg_t->alias,65);
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_Mode,(ulIndex + 1),&(pDsLiteCfg_t->mode));
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_Addr_Type,(ulIndex + 1),&(pDsLiteCfg_t->addr_type));
    Utopia_GetIndexed(ctx,UtopiaValue_Dslite_Addr_Fqdn,(ulIndex + 1),pDsLiteCfg_t->addr_fqdn,STR_SZ);
    Utopia_GetIndexed(ctx,UtopiaValue_Dslite_Addr_IPv6,(ulIndex + 1),pDsLiteCfg_t->addr_ipv6,STR_SZ);
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_Mss_Clamping_Enable,(ulIndex + 1),&(pDsLiteCfg_t->mss_clamping_enable));
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_Tcpmss,(ulIndex + 1),(int *)&(pDsLiteCfg_t->tcpmss));
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_IPv6_Frag_Enable,(ulIndex + 1),&(pDsLiteCfg_t->ipv6_frag_enable));

    /*Read-only parameters*/
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_Status,(ulIndex + 1),&(pDsLiteCfg_t->status));
    Utopia_GetIndexed(ctx,UtopiaValue_Dslite_Addr_Inuse,(ulIndex + 1),pDsLiteCfg_t->addr_inuse,STR_SZ);
    Utopia_GetIndexedInt(ctx,UtopiaValue_Dslite_Origin,(ulIndex + 1),&(pDsLiteCfg_t->origin));
    Utopia_GetIndexed(ctx,UtopiaValue_Dslite_Tunnel_Interface,(ulIndex + 1),pDsLiteCfg_t->tunnel_interface,STR_SZ);
    Utopia_GetIndexed(ctx,UtopiaValue_Dslite_Tunneled_Interface,(ulIndex + 1),pDsLiteCfg_t->tunneled_interface,STR_SZ);
    Utopia_GetIndexed(ctx,UtopiaValue_Dslite_Tunnel_V4Addr,(ulIndex + 1),pDsLiteCfg_t->tunnel_v4addr,65);

    return SUCCESS;
}

int Utopia_SetDsliteByIndex(UtopiaContext *ctx, unsigned long ulIndex, DsLiteCfg_t *pDsLiteCfg_t)
{

#ifdef _DEBUG_
    char ulog_msg[256];
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_SetIndexedInt(ctx,UtopiaValue_Dslite_Active,(ulIndex + 1),pDsLiteCfg_t->active);
    Utopia_SetIndexed(ctx,UtopiaValue_Dslite_Alias,(ulIndex + 1), pDsLiteCfg_t->alias);
    Utopia_SetIndexedInt(ctx,UtopiaValue_Dslite_Mode,(ulIndex + 1),pDsLiteCfg_t->mode);
    Utopia_SetIndexedInt(ctx,UtopiaValue_Dslite_Addr_Type,(ulIndex + 1),pDsLiteCfg_t->addr_type);
    Utopia_SetIndexed(ctx,UtopiaValue_Dslite_Addr_Fqdn,(ulIndex + 1), pDsLiteCfg_t->addr_fqdn);
    Utopia_SetIndexed(ctx,UtopiaValue_Dslite_Addr_IPv6,(ulIndex + 1), pDsLiteCfg_t->addr_ipv6);
    Utopia_SetIndexedInt(ctx,UtopiaValue_Dslite_Mss_Clamping_Enable,(ulIndex + 1),pDsLiteCfg_t->mss_clamping_enable);
    Utopia_SetIndexedInt(ctx,UtopiaValue_Dslite_Tcpmss,(ulIndex + 1),pDsLiteCfg_t->tcpmss);
    Utopia_SetIndexedInt(ctx,UtopiaValue_Dslite_IPv6_Frag_Enable,(ulIndex + 1),pDsLiteCfg_t->ipv6_frag_enable);
    Utopia_SetIndexed(ctx,UtopiaValue_Dslite_Tunnel_V4Addr,(ulIndex + 1), pDsLiteCfg_t->tunnel_v4addr);

    return SUCCESS;
}
#endif
