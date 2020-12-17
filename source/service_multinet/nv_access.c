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
#if defined(_COSA_INTEL_XB3_ARM_) || defined(INTEL_PUMA7)
#include "ccsp_custom.h"
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"
#include <stdbool.h>
#endif /* _COSA_INTEL_XB3_ARM_ */
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

const char* const multinet_component_id = "ccsp.multinet";
static void* 	  bus_handle = NULL;

#if defined(_COSA_INTEL_XB3_ARM_) || defined(INTEL_PUMA7)
#define CCSP_SUBSYS 	"eRT."
#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(bus_handle, CCSP_SUBSYS, name, NULL, &(str))

int dbusInit( void )
{
	int   ret  = -1;
	char* pCfg = CCSP_MSG_BUS_CFG;

	if( bus_handle == NULL ) 
	{
		MNET_DEBUG("dbusInit, called\n")

		// Dbus connection init
		#ifdef DBUS_INIT_SYNC_MODE
			ret = CCSP_Message_Bus_Init_Synced(multinet_component_id, 
								               pCfg, 
								               &bus_handle, 
								               Ansc_AllocateMemory_Callback, 
								               Ansc_FreeMemory_Callback);
		#else
			ret = CCSP_Message_Bus_Init(multinet_component_id, 
								        pCfg, 
								        &bus_handle, 
								        Ansc_AllocateMemory_Callback, 
								        Ansc_FreeMemory_Callback);
		#endif /* DBUS_INIT_SYNC_MODE */
	}

	if ( ret == -1 )
	{
		// Dbus connection error
		fprintf(stderr, " DBUS connection error\n");
		bus_handle = NULL;
	}

	return ret;
}
#endif /* _COSA_INTEL_XB3_ARM_  */

int nv_get_members(PL2Net net, PMember memberList, int numMembers) 
{
/* Use to get psm value via dbus instead of psmcli util */
#if !defined(_COSA_INTEL_XB3_ARM_) && !defined(INTEL_PUMA7)
    int i;
    char cmdBuff[512] = {0};
    char valBuff[256] = {0};
    int offset = 0;
    FILE* psmcliOut = NULL;
    char* ifToken = NULL, *dash = NULL;
    
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
                strcpy(memberList[actualNumMembers].interface->name, ifToken);
                strcpy(memberList[actualNumMembers++].interface->type->name, typeStrings[i]);
                if (dash) *dash = '-'; // replace character just in case it would confuse strtok
                
                ifToken = strtok(NULL, "\" \n");
            }
        }

        pclose(psmcliOut); /*RDKB-7137, CID-33325, free unused resources before exit */
    }
#else
    char  cmdBuff[512];
    char  *ifToken, 
		  *dash,
		  *pStr = NULL;
    int   actualNumMembers = 0,
		  i,
		  rc;
    int   HomeIsolation_en = 0;
    int   iterator = 0;
    bool  skipMocaIsoInterface = false;

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
		memset(cmdBuff,0,sizeof(cmdBuff));
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
		memset(cmdBuff,0,sizeof(cmdBuff));
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
						
						strcpy(memberList[actualNumMembers].interface->name, ifToken);
						MNET_DEBUG("%s, interface %s\n" COMMA __FUNCTION__ COMMA memberList[actualNumMembers].interface->name )
						strcpy(memberList[actualNumMembers++].interface->type->name, typeStrings[i]);
					}
					else
					{
						
						MNET_DEBUG("%s, interface token %s\n" COMMA __FUNCTION__ COMMA ifToken )
#if defined(MULTILAN_FEATURE)
						for (iterator = 0; iterator < sizeof(miInterfaceStrings)/sizeof(*miInterfaceStrings); ++iterator) {
							if(0 == strncmp(miInterfaceStrings[iterator], ifToken, sizeof(miInterfaceStrings[iterator])))
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
						strcpy(memberList[actualNumMembers].interface->name, ifToken);
						MNET_DEBUG("%s, interface %s\n" COMMA __FUNCTION__ COMMA memberList[actualNumMembers].interface->name )
						strcpy(memberList[actualNumMembers++].interface->type->name, typeStrings[i]);
	
						}
					}
				}
				else
				{
#endif
					strcpy(memberList[actualNumMembers].interface->name, ifToken);
					MNET_DEBUG("%s, interface %s\n" COMMA __FUNCTION__ COMMA memberList[actualNumMembers].interface->name )
					strcpy(memberList[actualNumMembers++].interface->type->name, typeStrings[i]);
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
#endif /* !_COSA_INTEL_XB3_ARM_ */

    return actualNumMembers;
    
}

int nv_get_bridge(int l2netInst, PL2Net net) 
{
    char cmdBuff[512] = {0};
	int  rc;
	char *pStr = NULL;

/* Use to get psm value via dbus instead of psmcli util */
#if !defined(_COSA_INTEL_XB3_ARM_) && !defined(INTEL_PUMA7)
    char valBuff[80] = {0};
    char tmpBuf[15] = {0};
    FILE* psmcliOut = NULL;

    snprintf(cmdBuff, sizeof(cmdBuff), 
           "psmcli get -e X dmsb.l2net.%d.Name X dmsb.l2net.%d.Vid X dmsb.l2net.%d.Enable", 
             l2netInst, l2netInst, l2netInst);

    psmcliOut = popen(cmdBuff, "r");

    if(psmcliOut) /*RDKB-7137, CID-33276, null check before use */
    {

        fgets(valBuff, sizeof(valBuff), psmcliOut);
        *strrchr(valBuff, '"') = '\0';
        sscanf(valBuff, "X=\"%s\n", net->name); 

        fgets(valBuff, sizeof(valBuff), psmcliOut);
        sscanf(valBuff, "X=\"%d\"\n", &net->vid);

        fgets(valBuff, sizeof(valBuff), psmcliOut);
        sscanf(valBuff, "X=\"%s\"\n", tmpBuf);
        if(!strcmp("FALSE", tmpBuf)) {
            net->bEnabled = 0;
        } else {
            net->bEnabled = 1;
        }

        net->inst = l2netInst;

        pclose(psmcliOut);
    }
#else

	/* dbus init based on bus handle value */
 	if(bus_handle == NULL)
	dbusInit( );

	if(bus_handle == NULL)
	{
		MNET_DEBUG("nv_get_bridge, Dbus init error\n")
		return 0;
	}

	/* dmsb.l2net.%d.Name */
	memset(cmdBuff,0,sizeof(cmdBuff));
    snprintf(cmdBuff, sizeof(cmdBuff),"dmsb.l2net.%d.Name",l2netInst); 

	rc = PSM_VALUE_GET_STRING(cmdBuff, pStr);
	if(rc == CCSP_SUCCESS && pStr != NULL)
	{
		 strcpy(net->name, pStr);
		 Ansc_FreeMemory_Callback(pStr);
		 pStr = NULL;
	} 

	/* dmsb.l2net.%d.Vid */ 
	memset(cmdBuff,0,sizeof(cmdBuff));
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
	memset(cmdBuff,0,sizeof(cmdBuff));
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
    int rc;
    char *pStr = NULL;
    int primary_l2_inst = 0;

/* Use to get psm value via dbus instead of psmcli util */
#if !defined(_COSA_INTEL_XB3_ARM_) && !defined(INTEL_PUMA7)
    char cmdBuff[512] = {0};
    char valBuff[80] = {0};
    char tmpBuf[15] = {0};
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
