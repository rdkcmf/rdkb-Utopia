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
 * DM_TR181_h - TR-181 data model structures
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"

#define UTOPIA_TR181_PARAM_SIZE		64
#define NAME_LENGTH	1024
#define MAC_SZ		6
#define MIN_MAC_LEN	12
#define MAX_MAC_LEN	17
#define MAX_HEX_LEN	16
#define VERSION_SZ	16
#define SSID_SZ		32
#define BUF_SZ		256
#define LINE_SZ		1024
#define HEX_SZ		8
#define KEYPASS_SZ	18
#define INST_SIZE	18

#define ERR_INSUFFICIENT_MEM 1
#undef ERR_INVALID_PARAM
#define ERR_INVALID_PARAM 2
#define ERR_FILE_OPEN_FAIL 3
#define ERR_NO_NODES 4
#define ERR_FILE_CLOSE_FAIL 5
#define ERR_GENERAL 100

#define MOCACFG_FILE_NAME_1  	"/mnt/appdata0/moca_cfg.txt"
#define MOCACFG_FILE_NAME 	"/tmp/moca_cfg.txt"
#define MOCA_SUM_FILE		"/tmp/moca0.txt"
#define MOCA_MAC_FILE		"/tmp/mocaMac.txt"
#define MOCA_PHY_FILE		"/tmp/mocaPhy.txt"
#define MOCA_STATS_FILE		"/tmp/mocaStats.txt"
#define MOCA_STATS_FILE_1       "/tmp/mocaStats1.txt"
#define MOCA_STATS_FILE_TEMP    "/tmp/mocaStatsTmp.txt"
#define MOCA_MAC_FILE_1		"/tmp/mocaMac1.txt"
#define MOCA_ASSOC_DEV		"/tmp/mocaAssDev.txt"
#define MOCA_ASS_INST		"/tmp/mocaAssDevInst.txt"
#define MOCA_TEMP_ASSOC_DEV	"/tmp/mocaAssDevTemp.txt"
#define MOCA_MAC_NODE		"/tmp/mocaNode.txt"
#define ETHERNET_ASSOC_DEVICE_FILE "/tmp/ethernet_AssocDevice.txt"

typedef unsigned char bool_t;

typedef struct param_node_{
        char param_name[NAME_LENGTH];
        char param_val[NAME_LENGTH];
        struct param_node_ *next;
}param_node;


typedef struct _Obj_Device_MoCA_{
	int InterfaceNumberOfEntries;	
}Obj_Device_MoCA;

typedef struct _Obj_Device_MoCA_Interface_i_static{
	char Name[UTOPIA_TR181_PARAM_SIZE];
        bool_t Upstream;
        unsigned char MACAddress[MAC_SZ];
        char FirmwareVersion[UTOPIA_TR181_PARAM_SIZE];
        unsigned long MaxBitRate;
        char HighestVersion[UTOPIA_TR181_PARAM_SIZE];
        unsigned char FreqCapabilityMask[HEX_SZ];
        unsigned char NetworkTabooMask[HEX_SZ];
        unsigned char NodeTabooMask[HEX_SZ];
        unsigned long TxBcastPowerReduction;
        bool_t QAM256Capable;
        unsigned long PacketAggregationCapability;
}Obj_Device_MoCA_Interface_i_static;

typedef struct _Obj_Device_MoCA_Interface_i_dyn{
	int Status;
        unsigned long LastChange;
        unsigned long MaxIngressBW;
        unsigned long MaxEgressBW;
        char CurrentVersion[UTOPIA_TR181_PARAM_SIZE];
        unsigned long NetworkCoordinator;
        unsigned long NodeID;
        bool_t MaxNodes;
        unsigned long BackupNC;
        bool_t PrivacyEnabled;
        unsigned char FreqCurrentMask[HEX_SZ];
        unsigned long CurrentOperFreq;
        unsigned long LastOperFreq;
        unsigned long TxBcastRate;
        /*
	 * Extensions
	 */
        bool_t MaxIngressBWThresholdReached;
        bool_t MaxEgressBWThresholdReached;
}Obj_Device_MoCA_Interface_i_dyn;

typedef struct _Obj_Device_MoCA_Interface_i_cfg{
	unsigned long InstanceNumber;
	char Alias[UTOPIA_TR181_PARAM_SIZE];
	
	bool_t Enable;	
	bool_t PreferredNC;
	bool_t PrivacyEnabledSetting;
	unsigned char FreqCurrentMaskSetting[HEX_SZ];
	char KeyPassphrase[KEYPASS_SZ];
	unsigned long TxPowerLimit;
	unsigned long PowerCntlPhyTarget;
	unsigned long BeaconPowerLimit;
	/*
	 * Extensions
	 */
        unsigned long MaxIngressBWThreshold;
        unsigned long MaxEgressBWThreshold;
}Obj_Device_MoCA_Interface_i_cfg;

typedef struct _Obj_Device_MoCA_Interface_i_Stats_{
        unsigned long BytesSent;
        unsigned long BytesReceived;
        unsigned long PacketsSent;
        unsigned long PacketsReceived;
        unsigned int  ErrorsSent;
        unsigned int  ErrorsReceived;
        unsigned int  UnicastPacketsSent;
        unsigned int  UnicastPacketsReceived;
        unsigned int  DiscardPacketsSent;
        unsigned int  DiscardPacketsReceived;
        unsigned long MulticastPacketsSent;
        unsigned long MulticastPacketsReceived;
        unsigned long BroadcastPacketsSent;
        unsigned long BroadcastPacketsReceived;
        unsigned int  UnknownProtoPacketsReceived;
}Obj_Device_MoCA_Interface_i_Stats;

typedef struct _Obj_Device_MoCA_Interface_i_QoS_{
        unsigned int  EgressNumFlows;
        unsigned int  IngressNumFlows;
        unsigned int  FlowStatsNumberOfEntries;
}Obj_Device_MoCA_Interface_i_QoS;

typedef struct _Obj_Device_MoCA_Interface_i_QoS_FlowStats_i_{
        unsigned int  FlowID;
        unsigned int  MaxRate;
        unsigned int  MaxBurstSize;
        unsigned int  LeaseTime;
        unsigned int  LeaseTimeLeft;
        unsigned int  FlowPackets;
	char PacketDA[UTOPIA_BUF_SIZE];
}Obj_Device_MoCA_Interface_i_QoS_FlowStats_i;

typedef struct _Obj_Device_MoCA_Interface_i_AssociatedDevice_i_{
	unsigned char MACAddress[MAC_SZ];
	unsigned int  NodeID;
	bool_t PreferredNC;
	char HighestVersion[UTOPIA_TR181_PARAM_SIZE];
	unsigned long PHYTxRate;
	unsigned long PHYRxRate;
	unsigned long TxPowerControlReduction;
	unsigned long RxPowerLevel;
	unsigned long TxBcastRate;
	unsigned long RxBcastPowerLevel;
	unsigned long TxPackets;
	unsigned long RxPackets;
	unsigned long RxErroredAndMissedPackets;
	bool_t QAM256Capable;
	unsigned long PacketAggregationCapability;
	unsigned long RxSNR;
	bool_t Active;	
}Obj_Device_MoCA_Interface_i_AssociatedDevice_i;


#define  TR181_IPV4_ADDRESS             \
         union                          \
         {                              \
                unsigned char Dot[4];   \
                unsigned long Value;    \
         }
typedef struct _Obj_Device_DNS_Relay_{
        unsigned long InstanceNumber;
        char Alias[UTOPIA_TR181_PARAM_SIZE];

        bool_t Enable;
        int    Status;
        TR181_IPV4_ADDRESS DNSServer;
        char   Interface[UTOPIA_TR181_PARAM_SIZE]; /* IP interface name */
        int Type;
}Obj_Device_DNS_Relay;

int Utopia_Get_TR181_Device_MoCA_Interface_i_Static(Obj_Device_MoCA_Interface_i_static *deviceMocaIntfStatic);
int Utopia_Get_TR181_Device_MoCA_Interface_i_Dyn(Obj_Device_MoCA_Interface_i_dyn *deviceMocaIntfDyn);

int Utopia_GetMocaIntf_Static(void *str_handle);
int Utopia_GetMocaIntf_Dyn(void *str_handle);
int Utopia_GetMocaIntf_Cfg(UtopiaContext *pCtx, void *str_handle);
int Utopia_SetMocaIntf_Cfg(UtopiaContext *pCtx, void *str_handle);
int Utopia_Count_AssociateDeviceEntry(int *devCount);
int Utopia_Get_TR181_Device_MoCA_Interface_i_AssociateDevice(Obj_Device_MoCA_Interface_i_AssociatedDevice_i *mocaIntfAssociatedevice, int count);

int Utopia_Get_DeviceDnsRelayForwarding(UtopiaContext *pCtx, int index, void *str_handle);
int Utopia_Set_DeviceDnsRelayForwarding(UtopiaContext *pCtx, int index, void *str_handle);

int file_parse(char* file_name, param_node **head);
void free_paramList(param_node *head);
int getMac(char * macAddress, int len, unsigned char * mac);
int getHex(char *hex_val, unsigned char *hexVal, int hexLen);
int getHexGeneric(char *hex_val, unsigned char *hexVal, int hexLen);

