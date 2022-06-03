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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "DM_TR181.h"
#include "safec_lib_common.h"
#include "secure_wrapper.h"

static void create_file()
{
    v_secure_system("cat " MOCACFG_FILE_NAME "|grep MOCASTATS|cut -d: -f2- -s|awk '{print $1$2,$3,$4}'|sed 's/^[ ]*//;s/[ ]*$//' > " MOCA_STATS_FILE);

    v_secure_system("cat " MOCACFG_FILE_NAME "|grep SUMMARY|cut -d: -f2- -s|awk '{print $1$2,$3,$4}'|sed 's/^[ ]*//;s/[ ]*$//' > " MOCA_SUM_FILE);

    v_secure_system("cat " MOCACFG_FILE_NAME "|grep PHYCTRL|cut -d: -f2- -s|awk '{print $1$2,$3,$4}'|sed 's/^[ ]*//;s/[ ]*$//' > " MOCA_PHY_FILE);

    v_secure_system("cat " MOCACFG_FILE_NAME "|grep MACCTRL|cut -d: -f2- -s|awk '{print $1$2,$3,$4}'|sed 's/^[ ]*//;s/[ ]*$//' > " MOCA_MAC_FILE);

    v_secure_system("cat " MOCA_MAC_FILE " | sed -n '1,10p' > " MOCA_MAC_FILE_1);

    v_secure_system("cat " MOCA_MAC_FILE " | sed '1,/AssociatedDeviceNumberOfEntries/d' > " MOCA_ASSOC_DEV);
}

int Utopia_Get_TR181_Device_MoCA_Interface_i_Static(Obj_Device_MoCA_Interface_i_static *deviceMocaIntfStatic)
{
    char ulog_msg[256];
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    int nodeId = 0;
    errno_t rc = -1;

    if (NULL == deviceMocaIntfStatic) {
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    create_file();

    retVal = file_parse(MOCA_SUM_FILE, &head);
    if(retVal != SUCCESS){
        free_paramList(head); /*RDKB-7129, CID-32892, free unused resources before exit*/
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return retVal;
    }
    ptr = head;

    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
           ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "Name")){
            /* CID 135359: Destination buffer too small */
	    strncpy(deviceMocaIntfStatic->Name, ptr->param_val, sizeof(deviceMocaIntfStatic->Name)-1);
            deviceMocaIntfStatic->Name[sizeof(deviceMocaIntfStatic->Name)-1] = '\0';
	}else if(!strcasecmp(ptr->param_name, "FirmwareVersion")){
            /* CID 135359: Destination buffer too small */
	    strncpy(deviceMocaIntfStatic->FirmwareVersion, ptr->param_val, sizeof(deviceMocaIntfStatic->FirmwareVersion)-1);
            deviceMocaIntfStatic->FirmwareVersion[sizeof(deviceMocaIntfStatic->FirmwareVersion)-1] = '\0';
	}else if(!strcasecmp(ptr->param_name, "FreqCapabilityMask")){
	    if(getHex(ptr->param_val, deviceMocaIntfStatic->FreqCapabilityMask, HEX_SZ) != SUCCESS){
            rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: FreqCapabilityMask read error !!!\n", __FUNCTION__);
            if(rc < EOK)
            {
               ERR_CHK(rc);
            }
            ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        }
	}else if(!strcasecmp(ptr->param_name, "MACAddress")){
	    if(getHex(ptr->param_val, deviceMocaIntfStatic->MACAddress, MAC_SZ)!= SUCCESS){
            rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Macaddress read error !!!\n", __FUNCTION__);
            if(rc < EOK)
            {
               ERR_CHK(rc);
            }
            ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        }
	}else if(!strcasecmp(ptr->param_name, "Upstream")){
	    deviceMocaIntfStatic->Upstream = (!strncasecmp(ptr->param_val, "false", 5))? FALSE : TRUE ;
	}else if(!strcasecmp(ptr->param_name, "NodeID")){
            nodeId = atoi(ptr->param_val);
        }
    }
    free_paramList(head);
    retVal = ERR_GENERAL;
    ptr = head = NULL;

    
    v_secure_system("cat " MOCA_ASSOC_DEV "|grep -A %d 'NodeID:%d' > "MOCA_MAC_NODE,INST_SIZE,nodeId);
    
    retVal = file_parse(MOCA_MAC_NODE, &head);
    if(retVal != SUCCESS){
        free_paramList(head); /*RDKB-7129, CID-32892, free unused resources before exit*/
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return retVal;
    }
    ptr = head;

    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "HighestVersion")){
		/* CID 135359: Destination buffer too small */
	    strncpy(deviceMocaIntfStatic->HighestVersion, ptr->param_val, sizeof(deviceMocaIntfStatic->HighestVersion)-1);
            deviceMocaIntfStatic->HighestVersion[sizeof(deviceMocaIntfStatic->HighestVersion)-1] = '\0';
	}else if(!strcasecmp(ptr->param_name, "TxBcastPowerReduction[TxPowerControlReduction]")){
	    deviceMocaIntfStatic->TxBcastPowerReduction = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "PacketAggregationCapability")){
	    deviceMocaIntfStatic->PacketAggregationCapability = atoi(ptr->param_val);
        }
    }
    free_paramList(head);
    retVal = ERR_GENERAL;
    ptr = head = NULL;

    retVal = file_parse(MOCA_PHY_FILE, &head);
    if(retVal != SUCCESS){
        free_paramList(head); /*RDKB-7129, CID-32892, free unused resources before exit*/
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return retVal;
    }
    ptr = head;

    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
           ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "NetworkTabooMask")){
            if(getHex(ptr->param_val, deviceMocaIntfStatic->NetworkTabooMask, HEX_SZ) != SUCCESS){
                rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: NetworkTabooMask read error !!!\n", __FUNCTION__);
                if(rc < EOK)
                {
                    ERR_CHK(rc);
                }
                ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            }
        }else if(!strcasecmp(ptr->param_name, "NodeTabooMask")){
            if(getHex(ptr->param_val, deviceMocaIntfStatic->NodeTabooMask, HEX_SZ) != SUCCESS){
                rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: NodeTabooMask read error !!!\n", __FUNCTION__);
                if(rc < EOK)
                {
                   ERR_CHK(rc);
                }
                ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            }
        }else if(!strcasecmp(ptr->param_name, "MaxBitRate")){
            deviceMocaIntfStatic->MaxBitRate = atoi(ptr->param_val);
        }else if(!strcasecmp(ptr->param_name, "QAM256Capable")){
            deviceMocaIntfStatic->QAM256Capable = (!strncasecmp(ptr->param_val, "false", 5))? FALSE : TRUE ;
        }
    }
    free_paramList(head);
    
    return SUCCESS;
}

int Utopia_Get_TR181_Device_MoCA_Interface_i_Dyn(Obj_Device_MoCA_Interface_i_dyn *deviceMocaIntfDyn)
{
    char ulog_msg[256];
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    errno_t   rc = -1;

    if (NULL == deviceMocaIntfDyn) {
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return ERR_INVALID_ARGS;
    }
    create_file();
    
    retVal = file_parse(MOCA_SUM_FILE, &head);
    if(retVal != SUCCESS){
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	free_paramList(head); /*RDKB-7129, CID-33397, free unused resources before exit*/
	return retVal;
    }
    ptr = head;
        
    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
           ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "Status")){
	    if(!strcasecmp(ptr->param_val, "Up"))
	         deviceMocaIntfDyn->Status = 1;
            else if(!strcasecmp(ptr->param_val, "Down"))
		deviceMocaIntfDyn->Status = 2;
	    else
		deviceMocaIntfDyn->Status = 3;
	}else if(!strcasecmp(ptr->param_name, "LastChange")){
	    deviceMocaIntfDyn->LastChange = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "FreqCurrentMask")){
		/* CID 135279: Destination buffer too small */
	    strncpy(deviceMocaIntfDyn->FreqCurrentMask, ptr->param_val, sizeof(deviceMocaIntfDyn->FreqCurrentMask)-1);
            deviceMocaIntfDyn->FreqCurrentMask[sizeof(deviceMocaIntfDyn->FreqCurrentMask)-1] = '\0';
	    if(getHex(ptr->param_val, deviceMocaIntfDyn->FreqCurrentMask, HEX_SZ) != SUCCESS){
	        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: FreqCurrentMask read error !!!\n", __FUNCTION__);
	        if(rc < EOK)
	        {
	           ERR_CHK(rc);
	        }
	        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            }
	}else if(!strcasecmp(ptr->param_name, "NodeID")){
	    deviceMocaIntfDyn->NodeID = atoi(ptr->param_val);
        }
    }
    free_paramList(head);
    retVal = ERR_GENERAL;
    ptr = head = NULL;
    v_secure_system("cat " MOCA_ASSOC_DEV "|grep -A %d 'NodeID:%d' > "MOCA_MAC_NODE,INST_SIZE,(int)deviceMocaIntfDyn->NodeID);
    
    retVal = file_parse(MOCA_MAC_NODE, &head);
    if(retVal != SUCCESS){
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	free_paramList(head); /*RDKB-7129, CID-33397, free unused resources before exit*/
	return retVal;
    }
    ptr = head;
        
    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "TxBcastRate")){
	    deviceMocaIntfDyn->TxBcastRate = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "MaxIngressBW[PHYTxRate]")){
	    deviceMocaIntfDyn->MaxIngressBW = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "MaxEgressBW[PHYRxRate]")){
	    deviceMocaIntfDyn->MaxEgressBW = atoi(ptr->param_val);
        }
    }
    free_paramList(head);
    retVal = ERR_GENERAL;
    ptr = head = NULL;
    
    retVal = file_parse(MOCA_MAC_FILE_1, &head);
    if(retVal != SUCCESS){
	free_paramList(head); /*RDKB-7129, CID-33397, free unused resources before exit*/
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return retVal;
    }
    ptr = head;
        
    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "NetworkCoordinator")){
	    deviceMocaIntfDyn->NetworkCoordinator =  atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "BackupNC")){
	    deviceMocaIntfDyn->BackupNC = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "MaxNodes")){
	    deviceMocaIntfDyn->MaxNodes = (!strncasecmp(ptr->param_val, "false", 5))? FALSE : TRUE ;
	}else if(!strcasecmp(ptr->param_name, "PrivacyEnabled")){
	    deviceMocaIntfDyn->PrivacyEnabled = (!strncasecmp(ptr->param_val, "false", 5))? FALSE : TRUE ;
	}else if(!strcasecmp(ptr->param_name, "CurrentVersion")){
	    /* CID 135279: Destination buffer too small */
	    strncpy(deviceMocaIntfDyn->CurrentVersion, ptr->param_val, sizeof(deviceMocaIntfDyn->CurrentVersion)-1);
            deviceMocaIntfDyn->CurrentVersion[sizeof(deviceMocaIntfDyn->CurrentVersion)-1] = '\0';
        }
    }
    free_paramList(head);
    retVal = ERR_GENERAL;
    ptr = head = NULL;
    
    retVal = file_parse(MOCA_PHY_FILE, &head);
    if(retVal != SUCCESS){
	free_paramList(head); /*RDKB-7129, CID-33397, free unused resources before exit*/
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return retVal;
    }
    ptr = head;
        
    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "LastOperFreq")){
	    deviceMocaIntfDyn->LastOperFreq = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "CurrentOperFreq")){
	    deviceMocaIntfDyn->CurrentOperFreq = atoi(ptr->param_val);
	}
    }
    free_paramList(head);
    
    return SUCCESS;
}

int Utopia_Count_AssociateDeviceEntry(int *devCount)
{
    char ulog_msg[256];
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    errno_t  rc = -1;
    
    create_file();    
    retVal = file_parse(MOCA_MAC_FILE_1, &head);
    if(retVal != SUCCESS){
	free_paramList(head); /*RDKB-7129, CID-33166, free unused resources before exit*/
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}

	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return retVal;
    }
    ptr = head;
    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    while(strcasecmp(ptr->param_name, "AssociatedDeviceNumberOfEntries") != 0)
    {
        ptr=ptr->next;
        if(!ptr)
            break;
    }
    
    if(ptr == NULL){
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Not found- AssociatedDeviceNumberOfEntries !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        free_paramList(head); //CID -125353: Resource leak
        return ERR_NO_NODES;
    }
    *devCount = atoi(ptr->param_val);
    free_paramList(ptr); //CID -125353: Resource leak
    free_paramList(head);
    return SUCCESS;
}

int Utopia_Get_TR181_Device_MoCA_Interface_i_AssociateDevice(Obj_Device_MoCA_Interface_i_AssociatedDevice_i *mocaIntfAssociatedevice, int count)
{
    char ulog_msg[256];
    param_node *ptr = NULL;
    param_node *head = NULL;
    int retVal = ERR_GENERAL;
    int i,j = 0;
    i = (count*19)+ 1;
    j = (count+1)*19;
    errno_t rc = -1;

    if (NULL == mocaIntfAssociatedevice) {
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return ERR_INVALID_ARGS;
    }
    create_file();    

    v_secure_system("cat " MOCA_ASSOC_DEV "|sed -n '%d,%d p'> "MOCA_ASS_INST,i,j);

    retVal = file_parse(MOCA_ASS_INST, &head);
    if(retVal != SUCCESS){
	free_paramList(head); /*RDKB-7129, CID-33335, free unused resources before exit*/
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return retVal;
    }
    ptr = head;
        
    if(!ptr){
        rc = sprintf_s(ulog_msg,  sizeof(ulog_msg),"%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
        if(!strcasecmp(ptr->param_name, "MACAddress")){
            if(getHex(ptr->param_val, mocaIntfAssociatedevice->MACAddress, MAC_SZ) != SUCCESS){
	        rc = sprintf_s(ulog_msg,  sizeof(ulog_msg),"%s: Macaddress read error !!!\n", __FUNCTION__);
	        if(rc < EOK)
	        {
	           ERR_CHK(rc);
	        }
	        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            } 
	}else if(!strcasecmp(ptr->param_name, "NodeID")){
	    mocaIntfAssociatedevice->NodeID =  atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "HighestVersion")){
            /* CID 135330: Destination buffer too small */
	    strncpy(mocaIntfAssociatedevice->HighestVersion, ptr->param_val, sizeof(mocaIntfAssociatedevice->HighestVersion)-1);
            mocaIntfAssociatedevice->HighestVersion[sizeof(mocaIntfAssociatedevice->HighestVersion)-1] = '\0';
	}else if(!strcasecmp(ptr->param_name, "PreferredNC")){
	    mocaIntfAssociatedevice->PreferredNC = (!strncasecmp(ptr->param_val, "false", 5))? FALSE : TRUE ;
	}else if(!strcasecmp(ptr->param_name, "MaxIngressBW[PHYTxRate]")){
	    mocaIntfAssociatedevice->PHYTxRate = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "MaxEgressBW[PHYRxRate]")){
	    mocaIntfAssociatedevice->PHYRxRate = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "TxBcastPowerReduction[TxPowerControlReduction]")){
	    mocaIntfAssociatedevice->TxPowerControlReduction = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "RxPowerLevel")){
	    mocaIntfAssociatedevice->RxPowerLevel = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "TxBcastRate")){
	    mocaIntfAssociatedevice->TxBcastRate = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "RxBcastPowerLevel")){
	    mocaIntfAssociatedevice->RxBcastPowerLevel = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "QAM256Capable")){
	    mocaIntfAssociatedevice->QAM256Capable = (!strncasecmp(ptr->param_val, "false", 5))? FALSE : TRUE ;
	}else if(!strcasecmp(ptr->param_name, "PacketAggregationCapability")){
	    mocaIntfAssociatedevice->PacketAggregationCapability = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "RxSNR")){
	    mocaIntfAssociatedevice->RxSNR = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "Active")){
	    mocaIntfAssociatedevice->Active = (!strncasecmp(ptr->param_val, "false", 5))? FALSE : TRUE ;
	}
    }
    mocaIntfAssociatedevice->Active = TRUE ;
    free_paramList(head);
    retVal = ERR_GENERAL;
    ptr = head = NULL;
 
    retVal = file_parse(MOCA_STATS_FILE, &head);
    if(retVal != SUCCESS){
	free_paramList(head); /*RDKB-7129, CID-33335, free unused resources before exit*/
	rc = sprintf_s(ulog_msg,  sizeof(ulog_msg),"%s: Error in file read !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return retVal;
    }
    ptr = head;
    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }

    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "RxErroredAndMissedPackets"))
	    mocaIntfAssociatedevice->RxErroredAndMissedPackets = atoi(ptr->param_val);
    }
    free_paramList(head);
    retVal = ERR_GENERAL;
    ptr = head = NULL;
    i = 0;

    v_secure_system("cp " MOCA_STATS_FILE " " MOCA_STATS_FILE_TEMP);
    if(count == 0){
        v_secure_system("cat " MOCA_STATS_FILE_TEMP "| sed -n '1,3 p' > " MOCA_STATS_FILE_1);
    }else{
	v_secure_system("cat "MOCA_STATS_FILE"| grep -A 2 'NodeID:%d' > "MOCA_STATS_FILE_1,mocaIntfAssociatedevice->NodeID);
    }

    retVal = file_parse(MOCA_STATS_FILE_1, &head);
    if(retVal != SUCCESS){
        free_paramList(head); /*RDKB-7129, CID-33335, free unused resources before exit*/
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in file read !!!", __FUNCTION__);
        if(rc < EOK)
        {
            ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return retVal;
    }
    ptr = head;
    if(!ptr){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s,%d: No nodes found !!!", __FUNCTION__, __LINE__);
        if(rc < EOK)
        {
           ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    for(; ptr; ptr=ptr->next){
	if(!strcasecmp(ptr->param_name, "TxPackets")){
	    mocaIntfAssociatedevice->TxPackets = atoi(ptr->param_val);
	}else if(!strcasecmp(ptr->param_name, "RxPackets")){
	    mocaIntfAssociatedevice->RxPackets = atoi(ptr->param_val);
        }
    }

    free_paramList(head);
    return SUCCESS;
}
