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

#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_security.h"
#include "strings.h"

int Utopia_GetEmailNotificationSetting(UtopiaContext *ctx, email_notification_t *pSetting)
{
    if (NULL == ctx || NULL == pSetting){
        return ERR_INVALID_ARGS;
    }

    bzero(pSetting, sizeof(email_notification_t));

    void* pEnabled               = &pSetting->bEnabled;
    void* pSendLogs              = &pSetting->bSendLogs;
    void* pFirewallBreach        = &pSetting->bFirewallBreach;
    void* pParentalControlBreach = &pSetting->bParentalControlBreach;
    void* pAlertsWarnings        = &pSetting->bAlertsWarnings;

    Utopia_GetBool(ctx, UtopiaValue_Security_EmailEnabled, pEnabled);
    Utopia_Get(ctx, UtopiaValue_Security_EmailSendTo, pSetting->send_to, sizeof(pSetting->send_to));
    Utopia_Get(ctx, UtopiaValue_Security_EmailServer, pSetting->server, sizeof(pSetting->server));
    Utopia_Get(ctx, UtopiaValue_Security_EmailUsername, pSetting->username, sizeof(pSetting->username));
    Utopia_Get(ctx, UtopiaValue_Security_EmailPassword, pSetting->password, sizeof(pSetting->password));
    Utopia_Get(ctx, UtopiaValue_Security_EmailFromAddr, pSetting->from_addr, sizeof(pSetting->from_addr));
    Utopia_GetBool(ctx, UtopiaValue_Security_SendLogs, pSendLogs);
    Utopia_GetBool(ctx, UtopiaValue_Security_FirewallBreach, pFirewallBreach);
    Utopia_GetBool(ctx, UtopiaValue_Security_ParentalControlBreach, pParentalControlBreach);
    Utopia_GetBool(ctx, UtopiaValue_Security_AlertsWarnings, pAlertsWarnings);

    return SUCCESS;
}


int Utopia_SetEmailNotificationSetting(UtopiaContext *ctx, const email_notification_t *pSetting)
{
    if (NULL == ctx || NULL == pSetting){
        return ERR_INVALID_ARGS;
    }

    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_EmailEnabled, pSetting->bEnabled);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailSendTo, (char *)pSetting->send_to);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailServer, (char *)pSetting->server);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailUsername, (char *)pSetting->username);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailPassword, (char *)pSetting->password);
    UTOPIA_SET(ctx, UtopiaValue_Security_EmailFromAddr, (char *)pSetting->from_addr);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_SendLogs, pSetting->bSendLogs);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_FirewallBreach, pSetting->bFirewallBreach);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_ParentalControlBreach, pSetting->bParentalControlBreach);
    UTOPIA_SETBOOL(ctx, UtopiaValue_Security_AlertsWarnings, pSetting->bAlertsWarnings);

    return SUCCESS;
}
