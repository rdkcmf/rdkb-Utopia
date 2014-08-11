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
#include <ctype.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_tr_user.h"
#include "DM_TR181.h"

static int g_IndexMapUser[MAX_NUM_INSTANCES+1] = {-1};

int Utopia_GetNumOfUsers(UtopiaContext *ctx)
{
    int cnt = 0;

    if(NULL == ctx) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if( 0 != Utopia_GetInt(ctx,UtopiaValue_User_Count,&cnt))
        return 0;
    else
        return cnt;
}

int Utopia_SetNumOfUsers(UtopiaContext *ctx, int count)
{
    if(NULL == ctx) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_SetInt(ctx,UtopiaValue_User_Count,count); 
    return SUCCESS;
}

int Utopia_GetUserEntry(UtopiaContext *ctx, unsigned long ulIndex, void *pUserEntry)
{
    if((NULL == ctx) || (NULL == pUserEntry)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

   userCfg_t *pUserEntry_t = (userCfg_t *)pUserEntry;

   /* Do we have an InstanceNumber already ? */
   if(0 != Utopia_GetIndexedInt(ctx,UtopiaValue_UserIndx_InsNum,(ulIndex + 1), &(pUserEntry_t->InstanceNumber))) {
       pUserEntry_t->InstanceNumber = 0;
   } else {
       g_IndexMapUser[pUserEntry_t->InstanceNumber] = ulIndex;
   }
   
   Utopia_GetUserByIndex(ctx,ulIndex,pUserEntry_t); 
   return SUCCESS;
}

int Utopia_GetUserCfg(UtopiaContext *ctx, void *pUserCfg)
{
    unsigned long ulIndex = 0;

    if((NULL == ctx) || (NULL == pUserCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    
    userCfg_t *pUserCfg_t = (userCfg_t *)pUserCfg;
    if(0 == pUserCfg_t->InstanceNumber)
        return ERR_INVALID_ARGS;

    ulIndex = g_IndexMapUser[pUserCfg_t->InstanceNumber];
    Utopia_GetUserByIndex(ctx,ulIndex,pUserCfg_t); 
    return SUCCESS;
}

int Utopia_AddUser(UtopiaContext *ctx, void *pUserCfg)
{
    unsigned long ulIndex = 0;
    int count = 0;
    
    if((NULL == ctx) || (NULL == pUserCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    userCfg_t *pUserCfg_t = (userCfg_t *)pUserCfg;

    if(0 == pUserCfg_t->InstanceNumber)
        return ERR_INVALID_ARGS;

    count = Utopia_GetNumOfUsers(ctx);
    ulIndex = count;
    g_IndexMapUser[pUserCfg_t->InstanceNumber] = ulIndex;
    Utopia_SetIndexedInt(ctx, UtopiaValue_UserIndx_InsNum, (ulIndex + 1) ,pUserCfg_t->InstanceNumber);
    Utopia_SetUserByIndex(ctx,ulIndex,pUserCfg_t);
    Utopia_SetNumOfUsers(ctx,(count + 1));

    return SUCCESS;
}

int Utopia_DelUser(UtopiaContext *ctx, unsigned long ulInstanceNumber)
{
    int count = 0;
    unsigned long ulIndex = 0;
    userCfg_t userCfg;
    char buf[STR_SZ] = {'\0'};

    if(NULL == ctx){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if(0 == ulInstanceNumber)
        return ERR_INVALID_ARGS;
    ulIndex = g_IndexMapUser[ulInstanceNumber];

    count = Utopia_GetNumOfUsers(ctx);
    count = count - 1;
    Utopia_SetNumOfUsers(ctx,count);
    Utopia_GetUserByIndex(ctx,ulIndex,&userCfg);

    /* Delete user from Linux DB if user is added there */
    if((TRUE == userCfg.bEnabled) && (TRUE == userCfg.RemoteAccessCapable)) {
        sprintf(buf,"deluser %s",userCfg.Username);
        system(buf);
    }

    if(count != 0)
    {
       ulIndex++;
       for(;ulIndex <= count; ulIndex++)
       {
          Utopia_GetUserByIndex(ctx,ulIndex,&userCfg);
          Utopia_GetIndexedInt(ctx,UtopiaValue_UserIndx_InsNum,(ulIndex + 1), &userCfg.InstanceNumber);
          Utopia_SetIndexedInt(ctx,UtopiaValue_UserIndx_InsNum,ulIndex, userCfg.InstanceNumber);

          g_IndexMapUser[userCfg.InstanceNumber] = (ulIndex - 1);

          Utopia_SetUserByIndex(ctx,(ulIndex - 1),&userCfg);
        }
        /* Now unset the last index */
        Utopia_UnsetIndexed(ctx,UtopiaValue_UserIndx_InsNum,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_UserName,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_Password,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_User_Language,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_User_Enabled,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_User_RemoteAccess,ulIndex);
        Utopia_UnsetIndexed(ctx,UtopiaValue_User_Access_Permissions,ulIndex);
    }

    return SUCCESS;
}

int Utopia_SetUserCfg(UtopiaContext *ctx, void *pUserCfg)
{
    unsigned long ulIndex = 0;

    if((NULL == ctx) || (NULL == pUserCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    userCfg_t *pUserCfg_t = (userCfg_t *)pUserCfg;

    if(0 == pUserCfg_t->InstanceNumber)
        return ERR_INVALID_ARGS;

    ulIndex = g_IndexMapUser[pUserCfg_t->InstanceNumber];
    Utopia_SetUserByIndex(ctx,ulIndex,pUserCfg_t);
    return SUCCESS;
}

int Utopia_SetUserValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNumber)
{
    if(NULL == ctx){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_SetIndexedInt(ctx, UtopiaValue_UserIndx_InsNum, (ulIndex + 1) ,ulInstanceNumber);
    g_IndexMapUser[ulInstanceNumber] = ulIndex;
    return SUCCESS;
}

int Utopia_GetUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, userCfg_t *pUserCfg_t)
{
    int iVal = 0;

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_GetIndexed(ctx,UtopiaValue_UserName,(ulIndex + 1),pUserCfg_t->Username,STR_SZ);
    Utopia_GetIndexed(ctx,UtopiaValue_Password,(ulIndex + 1),pUserCfg_t->Password,STR_SZ);
    Utopia_GetIndexed(ctx,UtopiaValue_User_Language,(ulIndex + 1),pUserCfg_t->Language,sizeof(pUserCfg_t->Language));

    Utopia_GetIndexedInt(ctx,UtopiaValue_User_Enabled,(ulIndex + 1),&iVal);
    pUserCfg_t->bEnabled = (0 == iVal) ? FALSE : TRUE;
    iVal = 0; 

    Utopia_GetIndexedInt(ctx,UtopiaValue_User_RemoteAccess,(ulIndex + 1),&iVal);
    pUserCfg_t->RemoteAccessCapable = (0 == iVal) ? FALSE : TRUE;

    Utopia_GetIndexedInt(ctx,UtopiaValue_User_Access_Permissions,(ulIndex + 1),&(pUserCfg_t->AccessPermissions));

    return SUCCESS;

}

int Utopia_SetUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, userCfg_t *pUserCfg_t)
{
    int iVal = 0;
    char buf[BUF_SZ] = {'\0'};
    char tmpBuf[STR_SZ] = {'\0'};

#ifdef _DEBUG_
    sprintf(ulog_msg, "%s: ********Entered ****** !!!", __FUNCTION__);
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    /* First delete the old username from Linux if its already there */
    /* This is required to take care of the change in username itself */
    if(0 != Utopia_GetIndexed(ctx,UtopiaValue_UserName,(ulIndex + 1),tmpBuf,STR_SZ)) {
        sprintf(buf,"deluser %s",tmpBuf);
        system(buf);
        memset(buf,0,BUF_SZ);  
    }
    Utopia_SetIndexed(ctx,UtopiaValue_UserName,(ulIndex + 1), pUserCfg_t->Username);
    Utopia_SetIndexed(ctx,UtopiaValue_Password,(ulIndex + 1),pUserCfg_t->Password);
    Utopia_SetIndexed(ctx,UtopiaValue_User_Language,(ulIndex + 1),pUserCfg_t->Language);

    iVal = (FALSE == pUserCfg_t->bEnabled) ? 0 : 1;
    Utopia_SetIndexedInt(ctx,UtopiaValue_User_Enabled,(ulIndex + 1),iVal);
    iVal = 0; 

    iVal = (FALSE == pUserCfg_t->RemoteAccessCapable) ? 0 : 1;
    Utopia_SetIndexedInt(ctx,UtopiaValue_User_RemoteAccess,(ulIndex + 1),iVal);

    Utopia_SetIndexedInt(ctx,UtopiaValue_User_Access_Permissions,(ulIndex + 1),pUserCfg_t->AccessPermissions);

    if((TRUE == pUserCfg_t->bEnabled) && (TRUE == pUserCfg_t->RemoteAccessCapable)) {
        /* Add the user with a home directory */
        sprintf(buf,"adduser -h /tmp/home/%s %s",pUserCfg_t->Username,pUserCfg_t->Username);
        system(buf);
        memset(buf,0,BUF_SZ);
        sprintf(buf,"echo %s:%s > %s",pUserCfg_t->Username,pUserCfg_t->Password,TMP_FILE);
        system(buf);
        memset(buf,0,BUF_SZ);
        sprintf(buf,"chpasswd < %s ",TMP_FILE);
        system(buf);
        memset(buf,0,BUF_SZ);
        sprintf(buf,"rm %s ",TMP_FILE);
        system(buf);
    }

    return SUCCESS;
}
