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
#include "service_multinet_handler.h"
#include "service_multinet_plat.h"
#include "service_multinet_swfab_plat.h"
#include "service_multinet_util.h"

#include <stdio.h>
#include <string.h>

#include "puma6_plat_map.h"
#include "puma6_plat_sw.h"
#include <stdlib.h>

#include "service_multinet_swfab_LinIF.h"
#include "service_multinet_swfab_gre.h"
#include "secure_wrapper.h"

//TODO, move what we can to a common handler library

int greIfInit(PSWFabHALArg args, int numArgs, BOOL up) {
    
    MNET_DEBUG("greIfInit running: %s create %d \"%s\"" COMMA SERVICE_MULTINET_DIR "/handle_gre.sh" COMMA args[0].hints.network->inst COMMA (char*)args[0].portID);
    v_secure_system(SERVICE_MULTINET_DIR "/handle_gre.sh create %d '%s'",args[0].hints.network->inst, (char*)args[0].portID);
    return 0;
}

int ifhandlerNoop(PSWFabHALArg args, int numArgs, BOOL up) {
    return 0;
}

int ifInitAllReady(PSWFabHALArg args, int numArgs, BOOL up) {
    int i;
    for (i = 0; i < numArgs; ++i) {
        args[i].ready = 1;
    }
    return 0;
}



int isEqualStringCompare(void* portIDa, void* portIDb) {
    return (!strcmp((char*)portIDa, (char*) portIDb));
}

int isEqualIntCompare(void* portIDa, void* portIDb) {
    return *((int*)portIDa) == *((int*)portIDb) ? 1 : 0;
}

int stringIDPortIDDirect (void* portID, char* stringbuf, int bufSize) {
    
    int retval = snprintf(stringbuf, bufSize, "%s", (char*) portID);
    
    return retval ? retval + 1 : 0; 
}

int eventIDFromStringPortID (void* portID, char* stringbuf, int bufsize) {
    int retval = snprintf(stringbuf, bufsize, "if_%s-status", (char*)portID);
    
    return retval ? retval + 1 : 0;
}



static SWFabHAL halList[] = {
    {HAL_NOOP, ifhandlerNoop, ifhandlerNoop, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID},
    {HAL_WIFI, ifhandlerNoop, configVlan_WiFi, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID}, // WIFI ports will depend on switch ports only
    {HAL_ESW, ifhandlerNoop, configVlan_ESW, isEqualIntCompare, stringIDExtSw, eventIDSw},
    {HAL_ISW, ifhandlerNoop, configVlan_ISW, isEqualIntCompare, stringIDIntSw, eventIDSw},
    {HAL_GRE, greIfInit, linuxIfConfigVlan, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID},
    {HAL_LINUX, ifhandlerNoop/*linuxIfInit*/, linuxIfConfigVlan, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID},
};
static PlatformPort wifiPortList[] = {
    {(void*)"ath0", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath1", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath2", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath3", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath4", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath5", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath6", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath7", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath8", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath9", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath10", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath11", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath12", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath13", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath14", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)"ath15", ENTITY_AP, halList + HAL_WIFI, 0}
};

SwPortState intSwIDs[4] = { 
    {0,"dep1_atom", 0, {0},0,0},
    {2, "dep3_itoe", 0, {0},0,0},
    {3, "sw_5", 0, {0},0,0},
    {7, "dep2_arm", 0, {0},0,0}
};
SwPortState extSwIDs[5] = {
    {0, "sw_1", 0, {0},0,0},
    {1, "sw_2", 0, {0},0,0},
    {2, "sw_3", 0, {0},0,0},
    {3, "sw_4", 0, {0},0,0},
    {5, "dep0_etoi", 0, {0},0,0}
};

SwPortState extraPorts[5] = {
    {0, "sw_6", 0, {0},0,0},
    {1, "sw_7", 0, {0},0,0},
    {2, "sw_8", 0, {0},0,0},
    {3, "sw_9", 0, {0},0,0},
    {5, "sw_10", 0, {0},0,0}
}; 

static PlatformPort accessSwPortList[] = {
    {(void*)(extSwIDs + 0), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 1
    {(void*)(extSwIDs + 1), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 2
    {(void*)(extSwIDs + 2), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 3
    {(void*)(extSwIDs + 3), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 4
    {(void*)(intSwIDs + 2), ENTITY_ISW, halList + HAL_ISW, 0},  // MOCA
    {(void*)(extraPorts + 0), ENTITY_ESW, halList + HAL_ESW, 0}, //extra sw port
    {(void*)(extraPorts + 1), ENTITY_ESW, halList + HAL_ESW, 0}, //extra sw port
    {(void*)(extraPorts + 2), ENTITY_ESW, halList + HAL_ESW, 0}, //extra sw port
    {(void*)(extraPorts + 3), ENTITY_ESW, halList + HAL_ESW, 0}, //extra sw port
    {(void*)(extraPorts + 4), ENTITY_ESW, halList + HAL_ESW, 0} //extra sw port
};

static PlatformPort trunkSwPortList[] = {
    {(void*)(extSwIDs + 4), ENTITY_ESW, halList + HAL_ESW, 0}, //E2I
    {(void*)(intSwIDs + 0), ENTITY_ISW, halList + HAL_ISW, 0}, //Atom
    {(void*)(intSwIDs + 3), ENTITY_ISW, halList + HAL_ISW, 0}, //ARM
    {(void*)(intSwIDs + 1), ENTITY_ISW, halList + HAL_ISW, 0}  //I2E
};

static PlatformPort grePortList[] = {
    {(void*)"gretap0", ENTITY_NP, halList + HAL_GRE, 1},
    {(void*)"gretap1", ENTITY_NP, halList + HAL_GRE, 1},
    {(void*)"gretap2", ENTITY_NP, halList + HAL_GRE, 1},
    {(void*)"gretap3", ENTITY_NP, halList + HAL_GRE, 1}
};

// typedef struct typeMap {
//     char typeName[80];
// }TypeMap;

/** These structures statically define a phantom port on the NP forcing path lookups to 
 * connect the router to every other entity in the system
 */
static PlatformPort npPlaceholderPort = {(void*)"vNPPort", ENTITY_NP, halList + HAL_NOOP, 0};
static IFType nulltype = {"virt", NULL};
static NetInterface armEntityIF = {&nulltype, "np", {0}, 0, (void*)&npPlaceholderPort};
static Member armEntityMember = { &armEntityIF, 0, 1, 1};    

/**This is the linux interface providing connectivity to all lan ports, listed as a 
 * dependency everywhere.
 */
static PlatformPort npLinkPort = {(void*)"l2sd0", ENTITY_NP, halList + HAL_LINUX, 0};
static PlatformPort brWanPort = {(void*)"lbr0", ENTITY_NP, halList + HAL_LINUX, 0};

//Public platform specific implementation


int plat_addImplicitMembers(PL2Net nv_net, PMember memberBuf) {
    *memberBuf = armEntityMember;
    return 1;
}

/** This is a platform specific mapping between DM ports and physical ports. 
 * Integrators may utilize any approach to expedite mapping, such as port naming conventions.
 */
int mapToPlat(PNetInterface iface) {
    int portIndex=0;
    if (!strcmp("WiFi", iface->type->name)) {
        sscanf(iface->name, "ath%d", &portIndex);
        iface->map = wifiPortList + portIndex;
    } /*else if (!strcmp("Link", iface->type->name)) {
        if (!strcmp("l2sd0", iface->name))
            iface->map = &npLinkPort;
    }*/ else if (!strcmp("SW", iface->type->name)) {
	if ( 1 == sscanf(iface->name, "sw_%d", &portIndex) ||
	     1 == sscanf(iface->name, "eth%d", &portIndex) )
	    iface->map = accessSwPortList + (portIndex - 1);
	else
	    iface->map = NULL;
    } else if (!strcmp("Moca", iface->type->name)) {
        
    } else if (!strcmp("Gre", iface->type->name)) {
        sscanf(iface->name, "gretap%d", &portIndex);
        iface->map = grePortList + portIndex;
    } else if (!strcmp("virt", iface->type->name)) {
        iface->map = &npPlaceholderPort;
    } else if (!strcmp("Eth", iface->type->name)) {
        iface->map = &brWanPort;
    }
    
    return 0;
}

PPlatformPort plat_mapFromString(char* portIdString) {
    int portIndex;
    if (strstr(portIdString, "dep")) {
        sscanf(portIdString, "dep%d", &portIndex);
        return trunkSwPortList + portIndex;
    } else if (strstr(portIdString, "ath")) {
        sscanf(portIdString, "ath%d", &portIndex);
        return wifiPortList + portIndex;
    } else if (strstr(portIdString, "l2sd0")) {
        return &npLinkPort;
    } else if (strstr(portIdString, "sw_")) {
        sscanf(portIdString, "sw_%d", &portIndex);
        return (accessSwPortList + (portIndex - 1));
    } else if (strstr(portIdString, "gretap")) {
        sscanf(portIdString, "gretap%d", &portIndex);
        return  (grePortList + portIndex);
    } else if (strstr(portIdString, "vNPPort")) {
    
        return &npPlaceholderPort;
    } else if (strstr(portIdString, "lbr0")) {
        return &brWanPort;
    }
    
    
    return NULL;
}


PEntityPathDeps* depArray  = 
(PEntityPathDeps[3]) {
        (EntityPathDeps[3]){ 
            {{1,2},1,(PPlatformPort[2]){&npLinkPort}}, 
            {{1,3},1,(PPlatformPort[4]){&npLinkPort}}, 
            {{1,4},1,(PPlatformPort[3]){&npLinkPort}} 
        },
        (EntityPathDeps[2]){ {{0}, 0, NULL} },
        (EntityPathDeps[1]){ {{0}, 0 ,NULL} }
    };


PEntityPathDeps getPathDeps(int entityLow, int entityHigh) {
    return &depArray[entityLow-1][entityHigh-1-entityLow];
}





// static int linuxEthHandler_create(NetInterface* netif, L2Net* net);
// static int linuxEthHandler_destroy(NetInterface* netif, L2Net* net);
// static int linuxEthHandler_addVlan(NetInterface* netif, L2Net* net);
// static int linuxEthHandler_delVlan(NetInterface* netif, L2Net* net);
// static int linuxEthHandler_getLocalName(NetInterface* netif, L2Net* net);
// static int greHandler_create(NetInterface* netif, L2Net* net);
// static int greHandler_destroy(NetInterface* netif, L2Net* net);
// 
// 
// 
// //---------------Default handlers
// static int linuxEthHandler_create(NetInterface* netif, L2Net* net){
// 	//IFUP(netif->name)
// 	return 0;
// }
// static int linuxEthHandler_destroy(NetInterface* netif, L2Net* net) {
// 	//no-op
// 	return 0;
// }
// static int linuxEthHandler_addVlan(NetInterface* netif, L2Net* net) {
// 	
// }
// static int linuxEthHandler_delVlan(NetInterface* netif, L2Net* net){
// 	
// }
// static int linuxEthHandler_getLocalName(NetInterface* netif, L2Net* net){
// 	
// }
// static int greHandler_create(NetInterface* netif, L2Net* net){
// 	
// }
// static int greHandler_destroy(NetInterface* netif, L2Net* net){
// 	
// }


