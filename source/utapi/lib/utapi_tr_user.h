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

#ifndef __UTAPI_TR_USER_H__
#define __UTAPI_TR_USER_H__

#define STR_SZ 64
#define PWD_SZ 128
#define MAX_NUM_INSTANCES 255

#define DELIM_CHAR ','

#define TMP_FILE "/tmp/tr_user"

typedef  enum
_USER_ACCESS_PERMISSIONS
{
    USER_ADMIN = 1,
    USER_HOMEUSER,
    USER_RESTRICTED,
    USER_DENIED 
}USER_ACCESS_PERMISSIONS;


/* Config portion of User */

typedef struct
userCfg
{
    unsigned long                   InstanceNumber;

    unsigned char                   bEnabled;
    unsigned char                   RemoteAccessCapable;
    char                            Username[STR_SZ];
    char                            Password[PWD_SZ];
    char                            Language[16];
    char 	                    NumOfFailedAttempts;
    char                            X_RDKCENTRAL_COM_ComparePassword[32];
    char 			    HashedPassword[128];
    int			     	    RemainingAttempts;
    int			       	    LoginCounts;
    int				    LockOutRemainingTime;
#if defined(_COSA_FOR_BCI_)
    int                             NumOfRestoreFailedAttempt;
#endif
    USER_ACCESS_PERMISSIONS         AccessPermissions;
}userCfg_t;

/* Function prototypes */

int Utopia_GetNumOfUsers(UtopiaContext *ctx);
int Utopia_SetNumOfUsers(UtopiaContext *ctx, int count);

int Utopia_GetUserEntry(UtopiaContext *ctx,unsigned long ulIndex, void *pUserEntry);
int Utopia_GetUserCfg(UtopiaContext *ctx,void *pUserCfg);

int Utopia_SetUserCfg(UtopiaContext *ctx, void *pUserCfg);
int Utopia_SetUserValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNumber);

int Utopia_AddUser(UtopiaContext *ctx, void *pUserCfg);
int Utopia_DelUser(UtopiaContext *ctx, unsigned long ulInstanceNumber);

/* Utility functions */
int Utopia_GetUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, userCfg_t *pUserCfg_t);
int Utopia_SetUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, userCfg_t *pUserCfg_t);

#endif // __UTAPI_TR_USER_H__
