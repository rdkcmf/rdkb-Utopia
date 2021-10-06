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


#include "service_multinet_base.h"
#include "service_multinet_swfab.h"

#include "service_multinet_swfab_plat.h"
#include "service_multinet_swfab_deps.h"
#include "service_multinet_swfab_LinIF.h"
#if defined (MULTILAN_FEATURE)
#if defined (INTEL_PUMA7)
#include "puma7_plat_sw.h"
#endif
#endif
#include <string.h>

int portHelper(char *bridge, char *port, int tagging, int vid, BOOL up);


static int map(PL2Net net, PMemberControl members, BOOL mark) {
    int i;
    PNetInterface iface;
    PPlatformPort matchPort;
    
    for (i = 0; i < members->numMembers; ++i) {
        iface = members->member[i].interface;
        MNET_DEBUG("map, if: %s\n" COMMA iface->name)
        if (!members->handled[i] && !iface->map) {
            mapToPlat(iface);
            
            if (NULL == ((PPlatformPort)iface->map)) {
                MNET_DEBUG("Interface MAP pointer is NULL continue\n")
                continue;
            }

            //Populate status event name
            //TODO, make this part of mapToPlat
            if ((matchPort = (PPlatformPort)iface->map)) {
                MNET_DEBUG("map, %s mapped to hal %d\n" COMMA iface->name COMMA matchPort->hal->id)
                iface->dynamic = matchPort->isDynamic;
                if (matchPort->isDynamic) {
                    matchPort->hal->eventString(matchPort->portID, iface->eventName, sizeof(((PNetInterface)0)->eventName)); 
                }
                if(mark) members->handled[i] = mark;
            }
            
            
        }
    }
    
    return 0;
}

#ifdef MULTILAN_FEATURE
static int swfab_configVlan_inner(PPortConfigControl portConfig, PL2Net net, BOOL add) {
    SWFabHALArg args;
    PSWFabHAL hal = NULL;
    int result = 0;

    if (! portConfig->handled)
    {       
        hal = portConfig->config.platPort->hal;

		if (NULL != hal)
        {	
            MNET_DBG_CMD(printPlatport(portConfig->config.platPort))
            args.portID = portConfig->config.platPort->portID;
            args.hints.network = net;
            args.vidParams = portConfig->config.vidParams; 
            portConfig->handled =1;

            MNET_DEBUG("swfab_configVlan, Calling configVlan on hal %d.\n" COMMA hal->id);

            if ( hal->configVlan(&args, 1, add) != 0 )
            {
                MNET_DEBUG("swfab_configVlan failed for hal %d!\n" COMMA hal->id);
                result = -1;
            }
            else
            {
                MNET_DEBUG("swfab_configVlan, Hal %d succeeded.\n" COMMA hal->id);
            }
		}
	    else
		{
			MNET_DEBUG("hal is NULL!\n")
            result = -1;
		}
    }
    return result;
}
#endif

static int swfab_configVlan(PL2Net net, PMemberControl members, BOOL add) {
    
    int i;
#ifndef MULTILAN_FEATURE
    int j;
#endif
#ifdef MULTILAN_FEATURE
    PortConfigControl portConfig;
#else
    SWFabHALArg args[HAL_MAX_PORTS];
    PortConfigControl portConfigs[MAX_ADD_PORTS] = {{{NULL},0}};
#endif
    //int numArgs[NUM_HALS] = {0};
#ifndef MULTILAN_FEATURE
    int numArgs =0, numConfigs=0;
    PSWFabHAL hal = NULL;
#endif
    PPlatformPort platPort, trunkPort;
    PVlanTrunkState vlanState;
    List trunkPorts = {0};
    ListIterator portIter;
    PListItem item;
    
    MNET_DEBUG("swfab_configVlan, net %d, numMembers %d, add: %hhu\n" COMMA net->inst COMMA members->numMembers COMMA add)
    
    //Fill map to handlers
    map(net, members,0);
    
    MNET_DEBUG("swfab_configVlan, map() returned.\n" );
    
    //NOTE: This is a single vlan per bridge implementation. Change logic here to 
    //take into account any additional vlans / pvids.
    getVlanState(&vlanState, net->vid); 
    
    MNET_DEBUG("swfab_configVlan, getVlanState() returned.\n")
    
    for(i = 0; i < members->numMembers; ++i) {
        
        platPort = (PPlatformPort) members->member[i].interface->map;
        
        MNET_DEBUG("swfab_configVlan, considering %s, ready: %d, dynamic: %d, handled:%d platport: " COMMA members->member[i].interface->name COMMA members->member[i].bReady COMMA members->member[i].interface->dynamic COMMA members->handled[i]) 
        MNET_DBG_CMD(printPlatport(platPort); printf("\n"))
        if (!platPort || (add && !members->member[i].bReady && members->member[i].interface->dynamic)|| members->handled[i]) continue;
        
        //Map member info to port config
#ifdef MULTILAN_FEATURE
        portConfig.config.platPort = platPort;
        portConfig.config.vidParams.vid = net->vid;
        portConfig.config.vidParams.pvid = net->vid;
        portConfig.handled = 0;
        portConfig.config.vidParams.tagging = members->member[i].bTagging;

#if defined (MULTILAN_FEATURE)
        //Change in logic for WiFi interfaces: Instead of using handler returned by map, using generic portHelper()
        //to create User VLAN (if necessary) and connect interface to the bridge.
        if (!strcmp("WiFi", members->member[i].interface->type->name ))
        {
#if defined (INTEL_PUMA7)
            if (ep_check_if_really_bridged(net, members->member[i].interface->name))
#endif
            {
                if ( portHelper(net->name, members->member[i].interface->name, members->member[i].bTagging, net->vid, add) != -1 )
                {
                    if (add)
                    {
                        members->member[i].bReady = STATUS_STARTED;
                    }
                }
                else
                {
                    if (add)
                    {
                        members->member[i].bReady = STATUS_STOPPED;
                    }
                }
            }
            continue;
        }
#endif

        if ( swfab_configVlan_inner(&portConfig, net, add) == 0 )
        {
            if (add) 
            {
                members->member[i].bReady = STATUS_STARTED;
            }
        }
        else
        {
            if (add)
            {
                members->member[i].bReady = STATUS_STOPPED;
            }
        }

#else
        portConfigs[numConfigs].config.platPort = platPort;
        portConfigs[numConfigs].config.vidParams.vid = net->vid;
        portConfigs[numConfigs].config.vidParams.pvid = net->vid;
        portConfigs[numConfigs].handled = 0;
        portConfigs[numConfigs++].config.vidParams.tagging = members->member[i].bTagging;
#endif
        
        MNET_DEBUG("swfab_configVlan, calling dep model for %s.\n" COMMA members->member[i].interface->name)
        
        if (add)
            addAndGetTrunkPorts(vlanState, platPort, &trunkPorts);
        else
            removeAndGetTrunkPorts(vlanState, platPort, &trunkPorts);
        
        MNET_DEBUG("swfab_configVlan, dep model returned for %s. number of trunk ports: %d\n" COMMA members->member[i].interface->name COMMA trunkPorts.count)
        
        initIterator(&trunkPorts, &portIter);
        //Loop through trunking ports and add their config as well
        while ((item = getNext(&portIter))) {
            trunkPort = (PPlatformPort) item->data;
            MNET_DBG_CMD(printPlatport(trunkPort)) 
#ifdef MULTILAN_FEATURE
            portConfig.config.platPort = trunkPort;
            portConfig.config.vidParams.vid = net->vid;
            portConfig.config.vidParams.pvid = net->vid;
            portConfig.handled = 0;
            portConfig.config.vidParams.tagging = 1; //trunkPort->isShared ? 1 : members->member[i].bTagging;
            swfab_configVlan_inner(&portConfig, net, add);
#else
            portConfigs[numConfigs].config.platPort = trunkPort;
            portConfigs[numConfigs].config.vidParams.vid = net->vid;
            portConfigs[numConfigs].config.vidParams.pvid = net->vid;
            portConfigs[numConfigs].handled = 0;
            portConfigs[numConfigs++].config.vidParams.tagging = 1; //trunkPort->isShared ? 1 : members->member[i].bTagging;
#endif
        }
        MNET_DEBUG("swfab_configVlan, trunk ports added.\n" )
        
        clearList(&trunkPorts);
    }
    
#ifndef MULTILAN_FEATURE
    
    //Now iterate through the ports, and aggregate commands to each hal into a single call
    for (i = 0; i < numConfigs; ++i) 
	{
        if (portConfigs[i].handled) continue;
        
        numArgs = 0;
        hal = portConfigs[i].config.platPort->hal;
		if (NULL != hal)
        {	
            for (j = i; j < numConfigs; ++j) 
			{
            	if (portConfigs[j].config.platPort->hal->id != hal->id) continue;
	            MNET_DEBUG("swfab_configVlan, aggregating\n")
    	        MNET_DBG_CMD(printPlatport(portConfigs[j].config.platPort))
        	    args[numArgs].portID = portConfigs[j].config.platPort->portID;
            	args[numArgs].hints.network = net;
	            args[numArgs++].vidParams = portConfigs[j].config.vidParams; 
    	        portConfigs[j].handled =1;
        	}
        	MNET_DEBUG("swfab_configVlan, Calling configVlan on hal %d. numArgs %d\n" COMMA hal->id COMMA numArgs)
	        hal->configVlan(args, numArgs, add);
    	    MNET_DEBUG("swfab_configVlan, Hal %d returned.\n" COMMA hal->id)
		}
	    else
		{
			MNET_DEBUG("hal is NULL continue to next port\n")
			continue;
		}
    }    
#endif
    saveVlanState(vlanState);
    return 0;
}

void printPlatport(PPlatformPort port) {
    char mystring[60];
    if (!port) { printf("NULL PORT"); return;}
    if (!port->hal) {printf("NO HAL ON PORT"); return; }
    port->hal->stringID(port->portID, mystring, sizeof(mystring));
    MNET_DEBUG("Platport %s" COMMA mystring)
}




static int fillArgs(PHALArgMemberMap args, PMember members, int* handledArray, int numMembers) {
    PPlatformPort matchPort, curPort;
    int numArgs = 0;
    int j;
    
    matchPort = (PPlatformPort) members[0].interface->map;
    
    if (!matchPort) return 0;
    MNET_DEBUG("fillArgs, matching hal %d against %s\n" COMMA matchPort->hal->id COMMA members[0].interface->name)
    for (j = 0; j < numMembers; ++j) {
        curPort = (PPlatformPort) members[j].interface->map;
        if (curPort && curPort->hal->id == matchPort->hal->id) {
            args->args[numArgs].portID = curPort->portID;
            args->args[numArgs].hints.iface = members[j].interface;
            args->args[numArgs].hints.network = args->network;
            args->members[numArgs++] = members + j;
            //args[numArgs].numPorts = 1;
            //args[numArgs].member = members->member + j;
            //args[numArgs++].net = net;
            MNET_DEBUG("fillArgs, matched with %s\n" COMMA members[j].interface->name)
            handledArray[j] = 1;
        }
    }
    
    return numArgs;
}


//Public
int swfab_addVlan(PL2Net net, PMemberControl members) {
    return swfab_configVlan(net,members,1);
}

int swfab_removeVlan(PL2Net net, PMemberControl members) {
    return swfab_configVlan(net,members,0);
}

int swfab_create(PL2Net net, PMemberControl members) {
    
    int i, j;
    SWFabHALArg args[members->numMembers];
    PMember memberAry[members->numMembers];
    HALArgMemberMap argMap;
    int numArgs;
    PPlatformPort matchPort;
    
    MNET_DEBUG("swfab_create, net %d, numMembers %d\n" COMMA net->inst COMMA members->numMembers)
    memset(args, 0, sizeof(SWFabHALArg)*members->numMembers);
    argMap.args = args;
    argMap.members = memberAry;
    argMap.network = net;
    
    
    
    //Fill map to handlers
    map(net, members, 0);
    
    MNET_DEBUG("swfab_create, map() returned.\n")
    
    //Call add for each HAL only once
    for (i = 0; i < members->numMembers; ++i) {
        MNET_DEBUG("swfab_create, looking at i=%d\n" COMMA i)
        if (!members->handled[i]) {
            numArgs = fillArgs(&argMap, members->member + i, members->handled + i, members->numMembers-i);
            
            //List of args collected, call the hal
            if (numArgs) {
                matchPort = (PPlatformPort) members->member[i].interface->map;
                
                MNET_DEBUG("Calling hal initIF, halID: %d, numArgs: %d\n" COMMA matchPort->hal->id COMMA numArgs)
                matchPort->hal->initIF(argMap.args, numArgs, 1);
                MNET_DEBUG("Hal initIF returned, halID: %d\n" COMMA matchPort->hal->id)
            }
            //copy the ready flag retruned by the HAL into the member structure
            for (j = 0; j< numArgs; ++j) {
                argMap.members[j]->bReady =  argMap.args[j].ready;
            }
        }
    }
    
    return 0;
}

int swfab_domap(PL2Net net, PMemberControl members) {
    return map(net, members, 1);
        
}
