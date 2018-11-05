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

#ifndef _UTAPI_PARENTAL_CONTROL_H_
#define _UTAPI_PARENTAL_CONTROL_H_

#include "autoconf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mng_devs
{
    boolean_t enable;
    boolean_t allow_all; //If true, all the devices is allowed to connect to the network except for those devices with md_dev.is_block set to true. Vice versa.
} mng_devs_t;

int Utopia_GetMngDevsCfg(UtopiaContext *ctx, mng_devs_t *mng_devs);
int Utopia_SetMngDevsCfg(UtopiaContext *ctx, const mng_devs_t *mng_devs);

//----------------------------------------------------------------
typedef struct blkurl
{
    boolean_t always_block; //If set to false, always block. Other only block by start_time ~ end_time in time of day and block_days in day of week.
    unsigned long ins_num;
    char alias[256];
    char block_method[8]; //must be "URL" or "KEYWD". "URL" prevents access to specific website URLS. "KEYWD" restricts access to websites names that contain specific words.
    char site[1024]; //The url or the keyword, e.g. "www.google.com" or "google" or "www.google.com,www.aol.com,www.twitter.com"
    char start_time[64]; //e.g. "20:00"
    char end_time[64];
    char block_days[64]; //e.g. "Mon,Wed,Fri"
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
    char mac[32]; //e.g. "00:11:22:33:44:55"
    char device_name[128]; //e.g. "Jack's PC"
#endif
}blkurl_t;

int Utopia_GetBlkURLCfg(UtopiaContext *ctx, int *enable);
int Utopia_SetBlkURLCfg(UtopiaContext *ctx, const int enable);
int Utopia_GetBlkURLInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins);
int Utopia_GetNumberOfBlkURL(UtopiaContext *ctx, int *num);
int Utopia_GetBlkURLByIndex(UtopiaContext *ctx, unsigned long ulIndex, blkurl_t *blkurl);
int Utopia_SetBlkURLByIndex(UtopiaContext *ctx, unsigned long ulIndex, const blkurl_t *blkurl);
int Utopia_SetBlkURLInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ins, const char *alias);
int Utopia_AddBlkURL(UtopiaContext *ctx, const blkurl_t *blkurl);
int Utopia_DelBlkURL(UtopiaContext *ctx, unsigned long ins);

//--------------------------------------------------------------------
typedef struct trusted_user
{
    boolean_t trusted;
    unsigned long ins_num;
    char alias[256];

    char host_descp[64]; //e.g. "Bob's computer"
    unsigned char ipaddrtype; // 4 or 6
    char ipaddr[64];
}trusted_user_t;

int Utopia_GetTrustedUserInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins);
int Utopia_GetNumberOfTrustedUser(UtopiaContext *ctx, int *num);
int Utopia_GetTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, trusted_user_t *trusted_user);
int Utopia_SetTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, const trusted_user_t *trusted_user);
int Utopia_SetTrustedUserInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias);
int Utopia_AddTrustedUser(UtopiaContext *ctx, const trusted_user_t *trusted_user);
int Utopia_DelTrustedUser(UtopiaContext *ctx, unsigned long ins);

//----------------------------------------------------------------------
typedef struct ms_serv
{
    boolean_t always_block;
    unsigned long ins_num;
    char alias[256];

    char descp[64]; //e.g. "FTP download"
    char protocol[8]; // must be one of "TCP" / "UDP" / "BOTH"
    unsigned long start_port;
    unsigned long end_port;
    char start_time[64]; //e.g. 20:00
    char end_time[64];
    char block_days[64];
}ms_serv_t;


int Utopia_GetMngServsCfg(UtopiaContext *ctx, int *enable);
int Utopia_SetMngServsCfg(UtopiaContext *ctx, const int enable);
int Utopia_GetMSServInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins);
int Utopia_GetNumberOfMSServ(UtopiaContext *ctx, int *num);
int Utopia_GetMSServByIndex(UtopiaContext *ctx, unsigned long ulIndex, ms_serv_t *ms_serv);
int Utopia_SetMSServByIndex(UtopiaContext *ctx, unsigned long ulIndex, const ms_serv_t *ms_serv);
int Utopia_SetMSServInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias);
int Utopia_AddMSServ(UtopiaContext *ctx, const ms_serv_t *ms_serv);
int Utopia_DelMSServ(UtopiaContext *ctx, unsigned long ins);

//----------------------------------------------------------------------
typedef struct ms_trusteduser
{
    boolean_t trusted;
    unsigned long ins_num;
    char alias[256];

    char host_descp[64];
    unsigned char ipaddrtype; // 4 or 6
    char ipaddr[64];
}ms_trusteduser_t;
        
int Utopia_GetMSTrustedUserInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins);
int Utopia_GetNumberOfMSTrustedUser(UtopiaContext *ctx, int *num);
int Utopia_GetMSTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, ms_trusteduser_t *ms_trusteduser);
int Utopia_SetMSTrustedUserByIndex(UtopiaContext *ctx, unsigned long ulIndex, const ms_trusteduser_t *ms_trusteduser);
int Utopia_SetMSTrustedUserInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias);
int Utopia_AddMSTrustedUser(UtopiaContext *ctx, const ms_trusteduser_t *ms_trusteduser);
int Utopia_DelMSTrustedUser(UtopiaContext *ctx, unsigned long ins);

//-------------------------------------------------------------------
typedef struct md_dev
{
    unsigned long ins_num;
    char alias[256];

    boolean_t is_block; //If set to true, this device is prevented from connecting this network. Other, it is allowed.
    boolean_t always; //Always blocked or allowed based on the value of is_block.
    char descp[64]; //e.g. "Bob's computer"
    char macaddr[64];
    char start_time[64]; //e.g. 20:00
    char end_time[64];
    char block_days[64];
}md_dev_t;
    
int Utopia_GetMDDevInsNumByIndex(UtopiaContext *ctx, unsigned long uIndex, int *ins);
int Utopia_GetNumberOfMDDev(UtopiaContext *ctx, int *num);
int Utopia_GetMDDevByIndex(UtopiaContext *ctx, unsigned long ulIndex, md_dev_t *md_dev);
int Utopia_SetMDDevByIndex(UtopiaContext *ctx, unsigned long ulIndex, const md_dev_t *md_dev);
int Utopia_SetMDDevInsAndAliasByIndex(UtopiaContext *ctx, unsigned long ulIndex, int ins, const char *alias);
int Utopia_AddMDDev(UtopiaContext *ctx, const md_dev_t *md_dev);
int Utopia_DelMDDev(UtopiaContext *ctx, unsigned long ins);

#ifdef __cplusplus
}
#endif

#endif
