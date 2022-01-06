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
   Copyright [2015] [Cisco Systems, Inc.]
 
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
#include "service_multinet_nv.h"
#include <stdio.h>
#include <string.h>
#include "safec_lib_common.h"
#if defined(_COSA_INTEL_XB3_ARM_) || defined(INTEL_PUMA7)
#include "ccsp_custom.h"
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"
#include <stdbool.h>
#if defined(MESH_ETH_BHAUL)
#include "syscfg/syscfg.h"
#endif
#endif /* _COSA_INTEL_XB3_ARM_ */
#if defined(ENABLE_ETH_WAN) || defined(AUTOWAN_ENABLE)
#include "ccsp_hal_ethsw.h"
#endif
#ifdef MULTILAN_FEATURE
char* typeStrings[] = {
    "SW", "Gre", "Link", "Eth", "WiFi", "Moca"
};
#else
char* typeStrings[] = {
    "SW", "Gre", "Link", "Eth", "WiFi"
};
#endif
#if defined(MOCA_HOME_ISOLATION) && defined(MULTILAN_FEATURE)
#if defined(INTEL_PUMA7)
char* miInterfaceStrings[] = {"sw_5","nmoca0"};
#elif defined(_COSA_INTEL_XB3_ARM_)
char* miInterfaceStrings[] = {"sw_5"};
#endif
#endif

#if defined(_COSA_INTEL_XB3_ARM_) || defined(INTEL_PUMA7)

static const char* const multinet_component_id = "ccsp.multinet";
static void* bus_handle = NULL;

#define CCSP_SUBSYS 	"eRT."
#define CCSP_CR_COMPONENT_ID "eRT.com.cisco.spvtg.ccsp.CR"
#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(bus_handle, CCSP_SUBSYS, name, NULL, &(str))
#define CCSP_BRIDGE_PORT_ETHBHAUL_ALIAS "MeshEthBH"
#if defined(INTEL_PUMA7)
#define CCSP_BRIDGE_PORT_XHS_INDEX      2
#elif defined(_COSA_INTEL_XB3_ARM_)
#define CCSP_BRIDGE_PORT_XHS_INDEX      4
#endif
#if defined(ENABLE_ETH_WAN) || defined(AUTOWAN_ENABLE)
#define CCSP_BRIDGE_PORT_ETHWAN_INDEX   ETHWAN_DEF_INTF_NUM + 1
#endif

static int dbusInit( void )
{
    int ret = 0;
    char* pCfg = CCSP_MSG_BUS_CFG;

    if (bus_handle == NULL)
    {
#ifdef DBUS_INIT_SYNC_MODE
        ret = CCSP_Message_Bus_Init_Synced(multinet_component_id,
                                           pCfg,
                                           &bus_handle,
                                           Ansc_AllocateMemory_Callback,
                                           Ansc_FreeMemory_Callback);
#else
        ret = CCSP_Message_Bus_Init((char *)multinet_component_id,
                                    pCfg,
                                    &bus_handle,
                                    (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback,
                                    Ansc_FreeMemory_Callback);
#endif

        if (ret == -1)
        {
            fprintf(stderr, "DBUS connection error\n");
        }
    }

    return ret;
}

#if defined(MESH_ETH_BHAUL)
static int mbus_get(char *path, char *val, int size)
{
    int                      compNum = 0;
    int                      valNum = 0;
    componentStruct_t        **ppComponents = NULL;
    parameterValStruct_t     **parameterVal = NULL;
    char                     *ppDestComponentName = NULL;
    char                     *ppDestPath = NULL;
    char                     *paramNames[1];

    if (!path || !val || size < 0)
        return -1;

    if (!bus_handle) {
         fprintf(stderr, "DBUS not connected\n");
         return -1;
    }

    if (CcspBaseIf_discComponentSupportingNamespace(bus_handle, CCSP_CR_COMPONENT_ID, path, CCSP_SUBSYS, &ppComponents, &compNum) != CCSP_SUCCESS) {
        fprintf(stderr, "failed to find component for %s \n", path);
        return -1;
    }
    ppDestComponentName = ppComponents[0]->componentName;
    ppDestPath = ppComponents[0]->dbusPath;
    paramNames[0] = path;

    if(CcspBaseIf_getParameterValues(bus_handle, ppDestComponentName, ppDestPath, paramNames, 1, &valNum, &parameterVal) != CCSP_SUCCESS) {
        fprintf(stderr, "failed to get value for %s \n", path);
        free_componentStruct_t(bus_handle, compNum, ppComponents);
        return -1;
    }

    if(valNum >= 1) {
        strncpy(val, parameterVal[0]->parameterValue, size);
        free_parameterValStruct_t(bus_handle, valNum, parameterVal);
        free_componentStruct_t(bus_handle, compNum, ppComponents);
    }
    return 0;
}

/* Enable or disable a boolean value in the DML. */
static int mbus_set_bool(char *path, BOOL val)
{
    CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    int                      compNum = 0;
    componentStruct_t        **ppComponents = NULL;
    char                     *ppDestComponentName = NULL;
    char                     *ppDestPath = NULL;
    parameterValStruct_t     param_val[1];
    char*                    faultParam = NULL;
    int                      ret = 0;

    param_val[0].parameterName = path;
    param_val[0].parameterValue = (val ? "true" : "false");
    param_val[0].type = ccsp_boolean;
    
    if (!path)
        return -1;

    if (!bus_handle) {
         fprintf(stderr, "DBUS not connected\n");
         return -1;
    }

    if (CcspBaseIf_discComponentSupportingNamespace(bus_handle, CCSP_CR_COMPONENT_ID, path, CCSP_SUBSYS, &ppComponents, &compNum) != CCSP_SUCCESS) {
        fprintf(stderr, "failed to find component for %s \n", path);
        return -1;
    }

    ppDestComponentName = ppComponents[0]->componentName;
    ppDestPath = ppComponents[0]->dbusPath;

    ret = CcspBaseIf_setParameterValues(
        bus_handle, 
        ppDestComponentName, 
        ppDestPath, 
        0,
        0,
        (void*)&param_val,
        1,
        TRUE,
        &faultParam 
        );
    
    if( ( ret != CCSP_SUCCESS ) && ( faultParam!=NULL )) {
        fprintf(stderr, " %s:%d Failed to set %s\n",__FUNCTION__,__LINE__, path);
        bus_info->freefunc( faultParam );
        return -1;
    }

    return 0;
}

int nv_toggle_ethbhaul_ports(BOOL onOff)
{
    int bridge_count = 0;
    int port_count = 0;
    int bridge_index = 0;
    int port_index = 0;
    char cmdBuff[256] = {0};
    char valBuff[256] = {0};
    bool skipXhsPort = false;
    char xhsAlias[20] = {0};
    int retval = 0;
#if defined(ENABLE_ETH_WAN) || defined(AUTOWAN_ENABLE)
    bool skipEthWanPort = false;
    char ethwan_enable[20] = {0};
    char ethWanAlias[20] = {0};
#endif

    /* dbus init based on bus handle value */
    if(bus_handle == NULL)
        dbusInit( );

    if(bus_handle == NULL)
    {
        MNET_DEBUG("nv_get_bridge, Dbus init error\n")
        return 0;
    }

    /* Determine if any ports need to be skipped */
    snprintf(cmdBuff, sizeof(cmdBuff), "Device.Bridging.Bridge.2.Port.%d.Enable", CCSP_BRIDGE_PORT_XHS_INDEX);
    if ( (mbus_get(cmdBuff, valBuff, sizeof(valBuff))) != 0) {
        fprintf(stderr, "Error: %s couldn't get xhs enable!\n", __FUNCTION__);
    }

    if(0 == strncmp(valBuff, "true", sizeof(valBuff)))
    {
        skipXhsPort = true;
        snprintf(xhsAlias, sizeof(xhsAlias), "%s_%i", CCSP_BRIDGE_PORT_ETHBHAUL_ALIAS, CCSP_BRIDGE_PORT_XHS_INDEX);
    }

#if defined(ENABLE_ETH_WAN) || defined(AUTOWAN_ENABLE)
    /* Determine if Ethernet WAN is enabled */
    if (0 == syscfg_get(NULL, "eth_wan_enabled", ethwan_enable, sizeof(ethwan_enable)))
    {
        if(0 == strncmp(ethwan_enable, "yes", sizeof(ethwan_enable)))
        {
            skipEthWanPort = true;
            snprintf(ethWanAlias, sizeof(ethWanAlias), "%s_%i", CCSP_BRIDGE_PORT_ETHBHAUL_ALIAS, CCSP_BRIDGE_PORT_ETHWAN_INDEX);
        }
    }
    else
    {
        fprintf(stderr, "Error: %s syscfg_get for eth_wan_enabled failed!\n", __FUNCTION__);
    }
#endif

    /* Get count of bridges */
    if ( (mbus_get("Device.Bridging.BridgeNumberOfEntries", valBuff, sizeof(valBuff))) != 0) {
        fprintf(stderr, "Error: %s couldn't get count of bridges!\n", __FUNCTION__);
        return -1;
    }

    bridge_count = atoi(valBuff);
    MNET_DEBUG("For ethbhaul found %d bridges\n" COMMA bridge_count);

    for (bridge_index = 1; bridge_index <= bridge_count; bridge_index++)
    {
        /* Get count of ports */
        snprintf(cmdBuff, sizeof(cmdBuff), "Device.Bridging.Bridge.%d.PortNumberOfEntries", bridge_index);
        if ( (mbus_get(cmdBuff, valBuff, sizeof(valBuff))) != 0) {
            fprintf(stderr, "Error: %s couldn't get count of ports for bridge %d!\n", __FUNCTION__, bridge_index);
            continue;
        }
        port_count = atoi(valBuff);
        MNET_DEBUG("For ethbhaul bridge %d found %d ports\n" COMMA bridge_index COMMA port_count);

        for (port_index = 1; port_index <= port_count; port_index++)
        {
            /* Get alias of port */
            snprintf(cmdBuff, sizeof(cmdBuff), "Device.Bridging.Bridge.%d.Port.%d.Alias", bridge_index, port_index);
            if ( (mbus_get(cmdBuff, valBuff, sizeof(valBuff))) != 0) {
                fprintf(stderr, "Error: %s couldn't get alias of bridge %d port %d!\n", __FUNCTION__, bridge_index, port_index);
                continue;
            }
            /* Check whether port alias contains the string signifying it is an ethbhaul port */
            if (strstr(valBuff, CCSP_BRIDGE_PORT_ETHBHAUL_ALIAS))
            {
                if(    (0 == strncmp(xhsAlias, valBuff, sizeof(xhsAlias)) && (true == skipXhsPort) && (bridge_index == 2))
#if defined(ENABLE_ETH_WAN) || defined(AUTOWAN_ENABLE)
                    || (0 == strncmp(ethWanAlias, valBuff, sizeof(ethWanAlias)) && (true == skipEthWanPort) )
#endif
                )
                {
                    MNET_DEBUG("Bridge %d port %d alias [%s] is already being utilized, skip\n" COMMA bridge_index COMMA port_index COMMA valBuff);
                }
                else
                {
                    MNET_DEBUG("Bridge %d port %d alias [%s] is ethbhaul, %s\n" COMMA bridge_index COMMA port_index COMMA valBuff COMMA (onOff ? "enable" : "disable"));
                    snprintf(cmdBuff, sizeof(cmdBuff), "Device.Bridging.Bridge.%d.Port.%d.Enable", bridge_index, port_index);
                    if ( (mbus_set_bool(cmdBuff, onOff)) != 0)
                    {
                        fprintf(stderr, "Error: Failed to enable bridge %d port %d for ethbhaul!\n", bridge_index, port_index);
                        retval = -1;
                    }
                }
            }
            else
            {
                MNET_DEBUG("Bridge %d port %d alias [%s] is NOT ethbhaul, skip\n" COMMA bridge_index COMMA port_index COMMA valBuff);
            }
        }
    } 

    return retval;
}
#endif /* MESH_ETH_BHAUL */
#endif

int nv_get_members(PL2Net net, PMember memberList, int numMembers) 
{

#if !defined(_COSA_INTEL_XB3_ARM_) && !defined(INTEL_PUMA7)

    int i;
    char cmdBuff[512];
    char valBuff[256];
    int offset = 0;
    FILE* psmcliOut;
    char* ifToken = NULL, *dash = NULL;
    errno_t  rc = -1;
    
    int actualNumMembers = 0;
    
    offset += snprintf(cmdBuff, sizeof(cmdBuff),"psmcli get -e");
    
    for (i = 0; i < sizeof(typeStrings)/sizeof(*typeStrings); ++i) {
        MNET_DEBUG("nv_get_members, adding lookup string index %d. offset=%d\n" COMMA i COMMA offset)
        offset += snprintf(cmdBuff+offset, sizeof(cmdBuff)-offset, " X dmsb.l2net.%d.Members.%s", net->inst, typeStrings[i]);
        MNET_DEBUG("nv_get_members, current lookup offset: %d, string: %s\n" COMMA offset COMMA cmdBuff)
    }
     
    psmcliOut = popen(cmdBuff, "r");

    if(psmcliOut) { /*RDKB-7137, CID-33511, null before use*/

        for(i = 0; fgets(valBuff, sizeof(valBuff), psmcliOut); ++i) {
            MNET_DEBUG("nv_get_members, current lookup line %s, i=%d\n" COMMA valBuff COMMA i)
            ifToken = strtok(valBuff+2, "\" \n");
            while(ifToken) { //FIXME: check for memberList overflow
                MNET_DEBUG("nv_get_members, current lookup token %s\n" COMMA ifToken)
                if ((dash = strchr(ifToken, '-'))){
                    *dash = '\0';
                    memberList[actualNumMembers].bTagging = 1;
                } else {
                    memberList[actualNumMembers].bTagging = 0;
                }
                memberList[actualNumMembers].interface->map = NULL; // No mapping has been performed yet
                rc = strcpy_s(memberList[actualNumMembers].interface->name, sizeof(memberList[actualNumMembers].interface->name), ifToken);
                ERR_CHK(rc);
                rc = strcpy_s(memberList[actualNumMembers].interface->type->name, sizeof(memberList[actualNumMembers].interface->type->name), typeStrings[i]);
                ERR_CHK(rc);
                actualNumMembers++;
                if (dash) *dash = '-'; // replace character just in case it would confuse strtok
                
                ifToken = strtok(NULL, "\" \n");
            }
        }

        pclose(psmcliOut); /*RDKB-7137, CID-33325, free unused resources before exit */
    }

#else

    /* Get psm value via dbus instead of psmcli util */

    char  cmdBuff[512];
    char  *ifToken, 
		  *dash,
		  *pStr = NULL;
    int   actualNumMembers = 0,
		  i,
		  rc;
    int   HomeIsolation_en = 0;
#if defined(MULTILAN_FEATURE)
    int   iterator = 0;
    bool  skipMocaIsoInterface = false;
#endif

	/* dbus init based on bus handle value */
	if(bus_handle == NULL)
		dbusInit( );

	if(bus_handle == NULL)
	{
		MNET_DEBUG("nv_get_members, Dbus init error\n")
		return 0;
	}
#if defined(MOCA_HOME_ISOLATION)
	if(net->inst == 1)
	{
		snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.HomeNetworkIsolation"); 
		rc = PSM_VALUE_GET_STRING(cmdBuff, pStr);
		if(rc == CCSP_SUCCESS && pStr != NULL)
                HomeIsolation_en = atoi(pStr);
		MNET_DEBUG("nv_get_members, HomeIsolation_en %d\n" COMMA HomeIsolation_en )
	}
#endif
    for (i = 0; i < sizeof(typeStrings)/sizeof(*typeStrings); ++i) 
	{
		/* dmsb.l2net.%d.Members.%s*/
		snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.%d.Members.%s",net->inst, typeStrings[i]); 

		MNET_DEBUG("nv_get_members, lookup string %s index %d\n" COMMA cmdBuff COMMA i)
		
		rc = PSM_VALUE_GET_STRING(cmdBuff, pStr);

		if(rc == CCSP_SUCCESS && pStr != NULL)
		{
			ifToken = strtok(pStr, "\" \n");
	
			while(ifToken) 
			{ 
				if ((dash = strchr(ifToken, '-')))
				{
					*dash = '\0';
					memberList[actualNumMembers].bTagging = 1;
				} 
				else 
				{
					memberList[actualNumMembers].bTagging = 0;
				}
				
				memberList[actualNumMembers].interface->map = NULL; // No mapping has been performed yet
#if defined(MOCA_HOME_ISOLATION)
				if(net->inst == 1)
				{
					if(HomeIsolation_en == 0)
					{
						
						rc = strcpy_s(memberList[actualNumMembers].interface->name, sizeof(memberList[actualNumMembers].interface->name), ifToken);
						ERR_CHK(rc);
						MNET_DEBUG("%s, interface %s\n" COMMA __FUNCTION__ COMMA memberList[actualNumMembers].interface->name )
						rc = strcpy_s(memberList[actualNumMembers].interface->type->name, sizeof(memberList[actualNumMembers].interface->type->name), typeStrings[i]);
						ERR_CHK(rc);
						actualNumMembers++;
					}
					else
					{
						
						MNET_DEBUG("%s, interface token %s\n" COMMA __FUNCTION__ COMMA ifToken )
#if defined(MULTILAN_FEATURE)
						skipMocaIsoInterface = false;
						for (iterator = 0; iterator < sizeof(miInterfaceStrings)/sizeof(*miInterfaceStrings); ++iterator) {
							if(0 == strncmp(miInterfaceStrings[iterator], ifToken, strlen(miInterfaceStrings[iterator])))
							{
								MNET_DEBUG("%s, interface token %s matches MoCA Isolation %s skipping\n" COMMA __FUNCTION__ COMMA ifToken COMMA miInterfaceStrings[iterator])
								skipMocaIsoInterface = true;
								break;
							}
						}

						if(true != skipMocaIsoInterface)
#else
						if(0 != strcmp(ifToken,"sw_5"))
#endif
						{
							rc = strcpy_s(memberList[actualNumMembers].interface->name, sizeof(memberList[actualNumMembers].interface->name), ifToken);
							ERR_CHK(rc);
							MNET_DEBUG("%s, interface %s\n" COMMA __FUNCTION__ COMMA memberList[actualNumMembers].interface->name )
							rc = strcpy_s(memberList[actualNumMembers].interface->type->name, sizeof(memberList[actualNumMembers].interface->type->name), typeStrings[i]);
							ERR_CHK(rc);
							actualNumMembers++;
						}
					}
				}
				else
				{
#endif
					rc = strcpy_s(memberList[actualNumMembers].interface->name, sizeof(memberList[actualNumMembers].interface->name), ifToken);
					ERR_CHK(rc);
					MNET_DEBUG("%s, interface %s\n" COMMA __FUNCTION__ COMMA memberList[actualNumMembers].interface->name )
					rc = strcpy_s(memberList[actualNumMembers].interface->type->name, sizeof(memberList[actualNumMembers].interface->type->name), typeStrings[i]);
					ERR_CHK(rc);
					actualNumMembers++;
#if defined(MOCA_HOME_ISOLATION)
				}
#endif

				if (dash) *dash = '-'; // replace character just in case it would confuse strtok
				
				ifToken = strtok(NULL, "\" \n");
			}

			Ansc_FreeMemory_Callback(pStr);
			pStr = NULL;
		} 
   	}
#endif

    return actualNumMembers;
    
}

int nv_get_bridge(int l2netInst, PL2Net net) 
{
    char cmdBuff[128];

#if !defined(_COSA_INTEL_XB3_ARM_) && !defined(INTEL_PUMA7)

    char tmpBuf[8];
    FILE* psmcliOut;
    int matches;

    snprintf(cmdBuff, sizeof(cmdBuff), "psmcli get -e X dmsb.l2net.%d.Name X dmsb.l2net.%d.Vid X dmsb.l2net.%d.Enable", l2netInst, l2netInst, l2netInst);

    psmcliOut = popen(cmdBuff, "r");
    if (!psmcliOut)
        return 0;

    /*
       The output from the psmcli command will be on 3 separate lines so rely
       on the space characters in the fscanf format string to match any
       whitespace, including the newline characters. Limit the Name string to
       15 chars to match the size of net->name (which is 16) and limit the
       Enable string to 7 chars to match the size of the local tmpBuf buffer
       (which is 8).
    */
    matches = fscanf (psmcliOut, "X=\"%15[^\"]\" X=\"%d\" X=\"%7[^\"]\"", net->name, &net->vid, tmpBuf);
    pclose(psmcliOut);
    if (matches != 3)
        return 0;
    net->bEnabled = (strcasecmp(tmpBuf, "TRUE") == 0) ? 1 : 0;
    net->inst = l2netInst;

#else

	/* Get psm value via dbus instead of psmcli util */

	int  rc;
	char *pStr = NULL;
	errno_t  safec_rc = -1;

	/* dbus init based on bus handle value */
	if(bus_handle == NULL)
		dbusInit( );
	if(bus_handle == NULL)
	{
		MNET_DEBUG("nv_get_bridge, Dbus init error\n")
		return 0;
	}

	/* dmsb.l2net.%d.Name */
	snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.%d.Name",l2netInst); 

	rc = PSM_VALUE_GET_STRING(cmdBuff, pStr);
	if(rc == CCSP_SUCCESS && pStr != NULL)
	{
		 safec_rc = strcpy_s(net->name, sizeof(net->name), pStr);
		 ERR_CHK(safec_rc);
		 Ansc_FreeMemory_Callback(pStr);
		 pStr = NULL;
	} 

	/* dmsb.l2net.%d.Vid */ 
#if defined (_BWG_PRODUCT_REQ_)
        /* dmsb.l2net.%d.XfinityNewVid  */
        if((l2netInst == 3)||(l2netInst == 4)||(l2netInst == 7)||(l2netInst == 8)||(l2netInst == 11)) {
           snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.%d.XfinityNewVid",l2netInst);
        }
        else
            snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.%d.Vid",l2netInst);
#else
        snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.%d.Vid",l2netInst);
#endif

	rc = PSM_VALUE_GET_STRING(cmdBuff, pStr);
	if(rc == CCSP_SUCCESS && pStr != NULL)
	{
		 net->vid = atoi(pStr);
		 Ansc_FreeMemory_Callback(pStr);
		 pStr = NULL;
	}  

	/* dmsb.l2net.%d.Enable */
	snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.%d.Enable",l2netInst); 

	rc = PSM_VALUE_GET_STRING(cmdBuff, pStr);
	if(rc == CCSP_SUCCESS && pStr != NULL)
	{
		 if(!strcmp("FALSE", pStr)) 
		 {
			net->bEnabled = 0;
		 }
		 else 
		 {
			net->bEnabled = 1;
		 }

		 Ansc_FreeMemory_Callback(pStr);
		 pStr = NULL;
	}  

	net->inst = l2netInst;
#endif /* !_COSA_INTEL_XB3_ARM_ */
    return 0;
}

int nv_get_primary_l2_inst(void) 
{
    int primary_l2_inst = 0;

/* Use to get psm value via dbus instead of psmcli util */
#if !defined(_COSA_INTEL_XB3_ARM_) && !defined(INTEL_PUMA7)
    char cmdBuff[512] = {0};
    char valBuff[80] = {0};
    FILE* psmcliOut = NULL;

    snprintf(cmdBuff, sizeof(cmdBuff), "psmcli get %s", MNET_NV_PRIMARY_L2_INST_KEY);
    psmcliOut = popen(cmdBuff, "r");

    if(psmcliOut) /*RDKB-7137, CID-33276, null check before use */
    {
        fgets(valBuff, sizeof(valBuff), psmcliOut);
        if(strnlen(valBuff, sizeof(valBuff)) > 0)
            primary_l2_inst = atoi(valBuff);

        pclose(psmcliOut);
    }
#else
    int rc;
    char *pStr = NULL;
    /* dbus init based on bus handle value */
    if(bus_handle == NULL)
    dbusInit( );

    if(bus_handle == NULL)
    {
        MNET_DEBUG("nv_get_primary_l2_inst, Dbus init error\n")
        return 0;
    }

    rc = PSM_VALUE_GET_STRING(MNET_NV_PRIMARY_L2_INST_KEY, pStr);
    if(rc == CCSP_SUCCESS && pStr != NULL)
    {
         sscanf(pStr, MNET_NV_PRIMARY_L2_INST_FORMAT, &primary_l2_inst);
         Ansc_FreeMemory_Callback(pStr);
         pStr = NULL;
    } 
#endif /* !_COSA_INTEL_XB3_ARM_ */

    return primary_l2_inst;
}
