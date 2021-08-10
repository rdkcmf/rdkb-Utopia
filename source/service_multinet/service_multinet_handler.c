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
#include <string.h>

#include "service_multinet_handler.h"
#include "service_multinet_swfab.h"
#include "service_multinet_ev.h"

struct allIfHandlers handlers;

PMemberHandler handlerList = (PMemberHandler)&handlers;


int numHandlers = sizeof(handlers)/sizeof(handlers.defaultHandlers[0]);

int handlerInit() {
    handlers.defaultHandlers[IFTYPE_SWFAB].add_vlan_for_members = swfab_addVlan;
    handlers.defaultHandlers[IFTYPE_SWFAB].remove_vlan_for_members = swfab_removeVlan;
    handlers.defaultHandlers[IFTYPE_SWFAB].create = swfab_create;
    handlers.defaultHandlers[IFTYPE_SWFAB].ensure_mapping = swfab_domap;
    
#ifdef MULTINET_IFHANDLER_PLUGIN    
    mnet_plugin_init(&handlers);
#endif
    
    return 0;    
}

int create_and_register_if(PL2Net net, PMember members, int numMembers){
    int i;
    MemberControl memberControl;
    int handleArray[numMembers];
    
    memberControl.member = members;
    memberControl.handled = handleArray;
    memberControl.numMembers = numMembers;
    MNET_DEBUG("Enter create_and_register_if for %d, numMembers: %d\n" COMMA net->inst COMMA numMembers)
    memset(memberControl.handled, 0,  sizeof(*memberControl.handled)*memberControl.numMembers);
    for (i = 0; i < numHandlers /*&& members->remaining*/; ++i) {
        MNET_DEBUG("Calling create on handler %d\n" COMMA i)
        handlerList[i].create(net, &memberControl); 
        MNET_DEBUG("Create on handler %d returned\n" COMMA i)
    }
    
    //register
    for (i = 0; i < memberControl.numMembers; ++i) {
        MNET_DEBUG("Check for registering ifstatus for %s, net %d\n" COMMA memberControl.member[i].interface->name COMMA net->inst)
        if (memberControl.member[i].interface->dynamic) {
            MNET_DEBUG("Registering for ifstatus on %s, net %d\n" COMMA memberControl.member[i].interface->name COMMA net->inst)
            ev_register_ifstatus(net, memberControl.member + i, memberControl.member[i].interface->eventName, memberControl.member[i].interface->eventName, (BOOL *)&memberControl.member[i].bReady);
        }
    }
    
    MNET_DEBUG("Returning create_and_register_if for %d\n" COMMA net->inst)
    return 0;
}

int unregister_if(PL2Net net, PMember members, int numMembers) {
    int i;
    MemberControl memberControl;
    int handleArray[numMembers];
    /* CID 57687: Sizeof not portable */
    memset(handleArray, 0, sizeof(handleArray));
    
    memberControl.member = members;
    memberControl.handled = handleArray;
    memberControl.numMembers = numMembers;

    for ( i = 0; i < numHandlers /*&& members->remaining*/; ++i) {
        handlerList[i].ensure_mapping(net, &memberControl); 
    }
    
    for (i = 0; i < memberControl.numMembers; ++i) {
        if (memberControl.member[i].interface->dynamic) {
            ev_unregister_ifstatus(net, memberControl.member[i].interface->eventName);
        }
    }
    return 0;
}
    

int add_vlan_for_members(PL2Net net, PMember members, int numMembers) {
    int i;
    MemberControl memberControl;
    int handleArray[numMembers];
    /* CID 63700: Sizeof not portable */
    memset(handleArray, 0, sizeof(handleArray));

    memberControl.member = members;
    memberControl.handled = handleArray;
    memberControl.numMembers = numMembers;

    for (i = 0; i < numHandlers /*&& members->remaining*/; ++i) {
        handlerList[i].add_vlan_for_members(net, &memberControl); 
    }
    return 0;
}

int remove_vlan_for_members(PL2Net net, PMember members, int numMembers) {
    int i;
    MemberControl memberControl;
    int handleArray[numMembers];
    /* CID 58043: Sizeof not portable */
    memset(handleArray, 0, sizeof(handleArray));

    memberControl.member = members;
    memberControl.handled = handleArray;
    memberControl.numMembers = numMembers;
    for (i = 0; i < numHandlers /*&& members->remaining*/; ++i) {
        handlerList[i].remove_vlan_for_members(net, &memberControl); 
    }
    return 0;
}
