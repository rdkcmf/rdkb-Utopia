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

#ifndef __UTAPI_TR_DHCP_H__
#define __UTAPI_TR_DHCP_H__

#include  <stdint.h>
#define STR_SZ 64 
#define MAX_NUM_INSTANCES 255
#define DHCPV4_NUM_SERVER_POOLS 1

#define  IPV4_ADDRESS                                                        \
         union                                                               \
         {                                                                   \
            unsigned char           Dot[4];                                  \
            uint32_t                Value;                                   \
         }

#define DELIM_CHAR ','

typedef  enum
_DHCP_SERVER_POOL_STATUS
{
    DHCP_SERVER_POOL_STATUS_Disabled = 1,
    DHCP_SERVER_POOL_STATUS_Enabled,
    DHCP_SERVER_POOL_STATUS_Error_Misconfigured,
    DHCP_SERVER_POOL_STATUS_Error
}DHCP_SERVER_POOL_STATUS;


/* Config portion of DHCPv4 Server */

typedef struct
dhcpV4ServerCfg
{
    unsigned char                   bEnabled;

}dhcpv4ServerCfg_t;

/* Config portion of DHCPv4 Server Pool */

typedef struct
dhcpV4ServerPoolCfg
{
    unsigned long                   InstanceNumber;
    char                            Alias[64];
    unsigned char                   bEnabled;
    unsigned long                   Order;
    char                            Interface[64];         
    char                            VendorClassID[256];
    unsigned char                   VendorClassIDExclude;
    unsigned int                    VendorClassIDMode;
    unsigned char                   ClientID[256];
    unsigned char                   ClientIDExclude;
    unsigned char                   UserClassID[256];
    unsigned char                   UserClassIDExclude;
    unsigned char                   Chaddr[6];
    unsigned char                   ChaddrMask[6];
    unsigned char                   ChaddrExclude;
    unsigned char                   DNSServersEnabled;
    IPV4_ADDRESS                    MinAddress;
    char                            MinAddressUpdateSource[16];
    IPV4_ADDRESS                    MaxAddress;
    char                            MaxAddressUpdateSource[16];
    IPV4_ADDRESS                    ReservedAddresses[8];
    IPV4_ADDRESS                    SubnetMask;
    IPV4_ADDRESS                    DNSServers[4];
    char                            DomainName[64];
    IPV4_ADDRESS                    IPRouters[4];
    int                             LeaseTime;
    int                             X_CISCO_COM_TimeOffset;
    unsigned char                   bAllowDelete;
}dhcpV4ServerPoolCfg_t;

/* Info portion of DHCPv4 Server Pool */

typedef struct
dhcpV4ServerPoolInfo
{
    DHCP_SERVER_POOL_STATUS         Status;
    unsigned long                   activeClientNumber;
}dhcpV4ServerPoolInfo_t;

/* DHCPv4 Server Pool Entry */

typedef struct
dhcpV4ServerPoolEntry
{
    dhcpV4ServerPoolCfg_t      Cfg;
    dhcpV4ServerPoolInfo_t     Info;
}dhcpV4ServerPoolEntry_t;

typedef struct
dhcpV4ServerPoolStaticAddress
{
    unsigned long                   InstanceNumber;
    char                            Alias[64];
    unsigned char                   bEnabled;
    unsigned char                   Chaddr[6];
    IPV4_ADDRESS                    Yiaddr;
    char                            DeviceName[64];
    char                            comments[256];
    unsigned char                   ActiveFlag;
}dhcpV4ServerPoolStaticAddress_t;

/* Function prototypes */

int Utopia_GetDhcpServerEnable(UtopiaContext *ctx, unsigned char *bEnabled);
int Utopia_SetDhcpServerEnable(UtopiaContext *ctx, unsigned char bEnabled);

int Utopia_GetNumberOfDhcpV4ServerPools();

int Utopia_GetDhcpV4ServerPoolEntry(UtopiaContext *ctx,unsigned long ulIndex, void *pEntry);
int Utopia_GetDhcpV4ServerPoolCfg(UtopiaContext *ctx,void *pCfg);
int Utopia_GetDhcpV4ServerPoolInfo(UtopiaContext *ctx, unsigned long ulInstanceNumber, void *pInfo);

int Utopia_SetDhcpV4ServerPoolCfg(UtopiaContext *ctx, void *pCfg);
int Utopia_SetDhcpV4ServerPoolValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNumber, char *pAlias);

int Utopia_GetDhcpV4SPool_NumOfStaticAddress(UtopiaContext *ctx,unsigned long ulPoolInstanceNumber);
int Utopia_GetDhcpV4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber,unsigned long ulIndex, void *pSAddr);
int Utopia_GetDhcpV4SPool_SAddressByInsNum(UtopiaContext *ctx, unsigned long ulClientInstanceNumber, void *pSAddr);

int Utopia_AddDhcpV4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, void *pSAddr);
int Utopia_DelDhcp4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, unsigned long ulInstanceNumber);

int Utopia_SetDhcpV4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, void *pSAddr);
int Utopia_SetDhcpV4SPool_SAddress_Values(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, unsigned long ulIndex, unsigned long ulInstanceNumber, char *pAlias);

/* Utility Functions */
int Utopia_GetDhcpV4SPool_SAddressByIndex(UtopiaContext *ctx, unsigned long ulIndex, dhcpV4ServerPoolStaticAddress_t *pSAddr_t);
#endif // __UTAPI_TR_DHCP_H__
