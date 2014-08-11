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

#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_security.h"


int Utopia_GetEmailNotificationSetting(UtopiaContext *ctx, email_notification_t *pSetting)
{
    if (NULL == ctx || NULL == pSetting){
        return ERR_INVALID_ARGS;
    }

    bzero(pSetting, sizeof(email_notification_t));

    Utopia_GetBool(ctx, UtopiaValue_Security_EmailEnabled, &pSetting->bEnabled);
    Utopia_Get(ctx, UtopiaValue_Security_EmailSendTo, pSetting->send_to, sizeof(pSetting->send_to));
    Utopia_Get(ctx, UtopiaValue_Security_EmailServer, pSetting->server, sizeof(pSetting->server));
    Utopia_Get(ctx, UtopiaValue_Security_EmailUsername, pSetting->username, sizeof(pSetting->username));
    Utopia_Get(ctx, UtopiaValue_Security_EmailPassword, pSetting->password, sizeof(pSetting->password));
    Utopia_Get(ctx, UtopiaValue_Security_EmailFromAddr, pSetting->from_addr, sizeof(pSetting->from_addr));
    Utopia_GetBool(ctx, UtopiaValue_Security_SendLogs, &pSetting->bSendLogs);
    Utopia_GetBool(ctx, UtopiaValue_Security_FirewallBreach, &pSetting->bFirewallBreach);
    Utopia_GetBool(ctx, UtopiaValue_Security_ParentalControlBreach, &pSetting->bParentalControlBreach);
    Utopia_GetBool(ctx, UtopiaValue_Security_AlertsWarnings, &pSetting->bAlertsWarnings);

    return SUCCESS;
}


int Utopia_SetEmailNotificationSetting(UtopiaContext *ctx, const email_notification_t *pSetting)
{
    if (NULL == ctx || NULL == pSetting){
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_EmailEnabled, pSetting->bEnabled);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailSendTo, pSetting->send_to);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailServer, pSetting->server);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailUsername, pSetting->username);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailPassword, pSetting->password);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailFromAddr, pSetting->from_addr);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_SendLogs, pSetting->bSendLogs);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_FirewallBreach, pSetting->bFirewallBreach);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_ParentalControlBreach, pSetting->bParentalControlBreach);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_AlertsWarnings, pSetting->bAlertsWarnings);

    return SUCCESS;
}
