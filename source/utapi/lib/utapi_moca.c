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
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_wlan.h"
#include "DM_TR181.h"
#include "safec_lib_common.h"
#include "secure_wrapper.h"

int Utopia_GetMocaIntf_Static(void *str_handle)
{
    char ulog_msg[256];
    Obj_Device_MoCA_Interface_i_static *deviceMocaIntfStatic = NULL;
    errno_t  rc = -1;

    if (str_handle == NULL) {
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    deviceMocaIntfStatic = (Obj_Device_MoCA_Interface_i_static *)str_handle;

    v_secure_system("mocacfg moca0 > " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 moca >> " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 mac >> " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 phy >> " MOCACFG_FILE_NAME);
    if(Utopia_Get_TR181_Device_MoCA_Interface_i_Static(deviceMocaIntfStatic) != SUCCESS){
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in MoCA_Intf_GET !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }
        
    return UT_SUCCESS;
}

int Utopia_GetMocaIntf_Dyn(void *str_handle)
{
    char ulog_msg[256];
    Obj_Device_MoCA_Interface_i_dyn *deviceMocaIntfDyn = NULL;
    errno_t  rc = -1;
    
    if (str_handle == NULL) {
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    deviceMocaIntfDyn = (Obj_Device_MoCA_Interface_i_dyn*)str_handle;

    v_secure_system("mocacfg moca0 > " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 moca >> " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 mac >> " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 phy >> " MOCACFG_FILE_NAME);

    if(Utopia_Get_TR181_Device_MoCA_Interface_i_Dyn(deviceMocaIntfDyn) != SUCCESS){
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in MoCA_Intf_GET !!!", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }
        
    return UT_SUCCESS;
}

int Utopia_GetMocaIntf_Cfg(UtopiaContext *pCtx, void *str_handle)
{
    char ulog_msg[256];
    Obj_Device_MoCA_Interface_i_cfg *deviceMocaIntfCfg = NULL;
    int iVal = -1;
    char buf[64] = {'\0'};
    errno_t  rc = -1;
    
    if (!pCtx || !str_handle) {
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
	if(rc < EOK)
	{
	    ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    deviceMocaIntfCfg = (Obj_Device_MoCA_Interface_i_cfg*)str_handle;

    Utopia_GetInt(pCtx, UtopiaValue_Moca_Enable, &iVal);
    deviceMocaIntfCfg->Enable = (iVal == 0)? FALSE : TRUE ;

    Utopia_GetInt(pCtx, UtopiaValue_Moca_PreferredNC, &iVal);
    deviceMocaIntfCfg->PreferredNC = (iVal == 0)? FALSE : TRUE ;

    if(Utopia_GetInt(pCtx, UtopiaValue_Moca_PrivEnabledSet, &iVal) != 0)
        deviceMocaIntfCfg->PrivacyEnabledSetting = FALSE;   /*default value */
    else
        deviceMocaIntfCfg->PrivacyEnabledSetting = (iVal == 0)? FALSE : TRUE ;

    Utopia_Get(pCtx, UtopiaValue_Moca_Alias, (char *)&deviceMocaIntfCfg->Alias, UTOPIA_TR181_PARAM_SIZE);
    
    Utopia_Get(pCtx, UtopiaValue_Moca_FreqCurMaskSet, buf, sizeof(buf));
    buf[strlen(buf)] = '\0';
    /*CID 62105: Unchecked return value */
    if(getHex(buf, deviceMocaIntfCfg->FreqCurrentMaskSetting, HEX_SZ) != SUCCESS){
       sprintf(ulog_msg, "%s: FreqCurrentMaskSetting read error !!!\n", __FUNCTION__);
       ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }

    if(Utopia_Get(pCtx, UtopiaValue_Moca_KeyPassPhrase, (char *)&deviceMocaIntfCfg->KeyPassphrase, KEYPASS_SZ) == 0)
        memset(deviceMocaIntfCfg->KeyPassphrase, 0, KEYPASS_SZ);    /*default value */
    if(Utopia_GetInt(pCtx, UtopiaValue_Moca_TxPowerLimit, (int*)&deviceMocaIntfCfg->TxPowerLimit) != 0)
        deviceMocaIntfCfg->TxPowerLimit = 36;       /*default value */
    if(Utopia_GetInt(pCtx, UtopiaValue_Moca_PwrCntlPhyTarget, (int*)&deviceMocaIntfCfg->PowerCntlPhyTarget) != 0)
        deviceMocaIntfCfg->PowerCntlPhyTarget = 236; /*default value */
    if(Utopia_GetInt(pCtx, UtopiaValue_Moca_BeaconPwrLimit, (int*)&deviceMocaIntfCfg->BeaconPowerLimit) != 0)
        deviceMocaIntfCfg->BeaconPowerLimit = 0;  /*default value */

    return UT_SUCCESS;
}

int Utopia_SetMocaIntf_Cfg(UtopiaContext *pCtx, void *str_handle)
{
    char ulog_msg[256];
    Obj_Device_MoCA_Interface_i_cfg *deviceMocaIntfCfg = NULL;
    int iVal = -1;
    char buf[64] = {'\0'};
    char key_val[64] = {'\0'};
    errno_t  rc = -1;
    
    if (!pCtx || !str_handle) {
	rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
	if(rc < EOK)
	{
	   ERR_CHK(rc);
	}
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    deviceMocaIntfCfg = (Obj_Device_MoCA_Interface_i_cfg*)str_handle;

    iVal = (deviceMocaIntfCfg->Enable == FALSE)? 0:1;
    if(iVal == 1){
        v_secure_system("mocacfg moca up");
	Utopia_Set(pCtx, UtopiaValue_Moca_Enable, "up");
    }else{
	v_secure_system("mocacfg moca down");
	Utopia_Set(pCtx, UtopiaValue_Moca_Enable, "down");
    }

    if(deviceMocaIntfCfg->PreferredNC == FALSE)
    {
        rc = strcpy_s(buf, sizeof(buf), "slave");
        ERR_CHK(rc);
    }
    else
    {
        rc = strcpy_s(buf, sizeof(buf), "master");
        ERR_CHK(rc);
    }
    Utopia_Set(pCtx, UtopiaValue_Moca_PreferredNC, buf);
    v_secure_system("mocacfg -s moca nc %s", buf);

    if(deviceMocaIntfCfg->PrivacyEnabledSetting == FALSE)
    {
        rc = strcpy_s(buf, sizeof(buf),"disable");
        ERR_CHK(rc);
    }
    else
    {
        rc = strcpy_s(buf, sizeof(buf), "enable");
        ERR_CHK(rc);
    }
    Utopia_Set(pCtx, UtopiaValue_Moca_PrivEnabledSet, buf);
    v_secure_system("mocacfg -s moca privacy %s", buf);

    Utopia_Set(pCtx, UtopiaValue_Moca_Alias, deviceMocaIntfCfg->Alias);

    rc = sprintf_s(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", 
                 deviceMocaIntfCfg->FreqCurrentMaskSetting[0],
		 deviceMocaIntfCfg->FreqCurrentMaskSetting[1],
		 deviceMocaIntfCfg->FreqCurrentMaskSetting[2],
		 deviceMocaIntfCfg->FreqCurrentMaskSetting[3],
		 deviceMocaIntfCfg->FreqCurrentMaskSetting[4],
		 deviceMocaIntfCfg->FreqCurrentMaskSetting[5],
		 deviceMocaIntfCfg->FreqCurrentMaskSetting[6],
		 deviceMocaIntfCfg->FreqCurrentMaskSetting[7]
	   );
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    buf[16] = '\0';
    Utopia_Set(pCtx, UtopiaValue_Moca_FreqCurMaskSet, buf);
    Utopia_RawSet(pCtx, NULL, "FreqMode", "manual");
    v_secure_system("mocacfg -s moca fmode manual");
    v_secure_system("mocacfg -s moca fplan 0x%s", buf);

    Utopia_Set(pCtx, UtopiaValue_Moca_KeyPassPhrase, deviceMocaIntfCfg->KeyPassphrase);
    v_secure_system("mocacfg -s moca ppassword %s", deviceMocaIntfCfg->KeyPassphrase);
 
    Utopia_SetInt(pCtx, UtopiaValue_Moca_TxPowerLimit, deviceMocaIntfCfg->TxPowerLimit); 
    rc = sprintf_s(key_val, sizeof(key_val), "%lu", deviceMocaIntfCfg->TxPowerLimit);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    
    v_secure_system("mocacfg -s moca maxtxpower %s", key_val);

    Utopia_SetInt(pCtx, UtopiaValue_Moca_PwrCntlPhyTarget, deviceMocaIntfCfg->PowerCntlPhyTarget);
    rc = sprintf_s(key_val, sizeof(key_val), "%lu", deviceMocaIntfCfg->PowerCntlPhyTarget);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    v_secure_system("mocacfg -s moca phyrate %s", key_val);

    Utopia_SetInt(pCtx, UtopiaValue_Moca_BeaconPwrLimit, deviceMocaIntfCfg->BeaconPowerLimit);
    rc = sprintf_s(key_val, sizeof(key_val), "%lu", deviceMocaIntfCfg->BeaconPowerLimit);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    
    v_secure_system("mocacfg -s moca bbackoff %s", key_val);

    return UT_SUCCESS;
}

int Utopia_GetMocaIntf_AssociateDevice(void *str_handle, int count)
{
    char ulog_msg[256];
    Obj_Device_MoCA_Interface_i_AssociatedDevice_i *mocaIntfAssociatedevice = NULL;
    errno_t  rc = -1;

    if (str_handle == NULL) {
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Invalid Input Parameter", __FUNCTION__);
        if(rc < EOK)
        {
           ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_ARGS;
    }
    mocaIntfAssociatedevice = (Obj_Device_MoCA_Interface_i_AssociatedDevice_i *)str_handle;

    v_secure_system("mocacfg moca0 > " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 moca >> " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 mac >> " MOCACFG_FILE_NAME);
    v_secure_system("mocacfg -g moca0 phy >> " MOCACFG_FILE_NAME);

    if(Utopia_Get_TR181_Device_MoCA_Interface_i_AssociateDevice(mocaIntfAssociatedevice, count) != SUCCESS){
        rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error in MoCA_Intf_AssociateDevice_GET !!!", __FUNCTION__);
        if(rc < EOK)
        {
           ERR_CHK(rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_ITEM_NOT_FOUND;
    }

    return UT_SUCCESS;
}



