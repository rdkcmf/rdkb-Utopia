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

#ifndef _UTAPI_SECURITY_H_
#define _UTAPI_SECURITY_H_

typedef struct email_notification {
    boolean_t    bEnabled;
    char         send_to[128];
    char         server[128];
    char         username[64];
    char         password[64];
    char         from_addr[128];
    boolean_t    bSendLogs;
    boolean_t    bFirewallBreach;
    boolean_t    bParentalControlBreach;
    boolean_t    bAlertsWarnings;
}__attribute__ ((__packed__)) email_notification_t;

/* 
 * return 0 for success, otherwise failure
 */
int Utopia_GetEmailNotificationSetting(UtopiaContext *ctx, email_notification_t *pSetting);
int Utopia_SetEmailNotificationSetting(UtopiaContext *ctx, const email_notification_t *pSetting);

#endif
