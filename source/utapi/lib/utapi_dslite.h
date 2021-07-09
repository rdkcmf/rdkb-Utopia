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
######################################################################*/

#ifndef __UTAPI_TR_DSLITE_H__
#define __UTAPI_TR_DSLITE_H__

#define STR_SZ 257
#define MAX_NUM_INSTANCES 255

typedef struct
DsLiteCfg
{
    unsigned long  InstanceNumber;
    int            active;
    int            status;
    char           alias[64+1];
    int            mode;
    int            addr_type;
    char           addr_inuse[256+1];
    char           addr_fqdn[256+1];
    char           addr_ipv6[256+1];
    int            origin;
    char           tunnel_interface[256+1];
    char           tunneled_interface[256+1];
    int            mss_clamping_enable;
    unsigned long  tcpmss;
    int            ipv6_frag_enable;
    char           tunnel_v4addr[64+1];
}DsLiteCfg_t;

int Utopia_GetDsliteEnable(UtopiaContext *ctx, boolean_t *bEnabled);
int Utopia_SetDsliteEnable(UtopiaContext *ctx, boolean_t bEnabled);
int Utopia_GetNumOfDsliteEntries(UtopiaContext *ctx,unsigned long *cnt);
int Utopia_SetNumOfDsliteEntries(UtopiaContext *ctx,unsigned long cnt);
int Utopia_GetDsliteCfg(UtopiaContext *ctx,DsLiteCfg_t *pDsliteCfg);
int Utopia_SetDsliteCfg(UtopiaContext *ctx,DsLiteCfg_t *pDsliteCfg);
int Utopia_AddDsliteEntry(UtopiaContext *ctx, DsLiteCfg_t *pDsliteCfg);
int Utopia_DelDsliteEntry(UtopiaContext *ctx, unsigned long ulInstanceNumber);

int Utopia_GetDsliteEntry(UtopiaContext *ctx,unsigned long ulIndex, void *pDsliteEntry);
int Utopia_SetDsliteInsNum(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNumber);

/* Utility functions */
int Utopia_GetDsliteByIndex(UtopiaContext *ctx, unsigned long ulIndex, DsLiteCfg_t *pDsLiteCfg_t);
int Utopia_SetDsliteByIndex(UtopiaContext *ctx, unsigned long ulIndex, DsLiteCfg_t *pDsLiteCfg_t);

#endif // __UTAPI_TR_DSLITE_H__
