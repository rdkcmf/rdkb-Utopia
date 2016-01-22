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

#ifndef MNET_HANDLER_H
#define MNET_HANDLER_H

#include "service_multinet_base.h"
#ifdef MULTINET_IFHANDLER_PLUGIN
#include "service_multinet_ifplugin_defs.h"
#endif
enum defaultHandlers {
	IFTYPE_SWFAB = 0,
	NUM_DEFAULT_IFTYPES
};

typedef struct memberControl {
    PMember member;
    int* handled;
    int numMembers;
    //int remaining;
}MemberControl, *PMemberControl;

typedef int (*memberHandlerFunc)(PL2Net net, PMemberControl members);

typedef struct memberHandler {
    memberHandlerFunc create;
    memberHandlerFunc add_vlan_for_members;
    memberHandlerFunc remove_vlan_for_members;
    memberHandlerFunc ensure_mapping;
}MemberHandler, *PMemberHandler;

struct allIfHandlers {
#ifdef MULTINET_IFHANDLER_PLUGIN
    MemberHandler pluginHandlers[NUM_PLUGIN_IFTYPES];
#endif
    
    MemberHandler defaultHandlers[NUM_DEFAULT_IFTYPES];
    
};

extern PMemberHandler handlerList;
extern int numHandlers;
int handlerInit();

int create_and_register_if(PL2Net net, PMember members, int numMembers);
int unregister_if(PL2Net net, PMember members, int numMembers);

int add_vlan_for_members(PL2Net net, PMember members, int numMembers);
int remove_vlan_for_members(PL2Net net, PMember members, int numMembers);


#endif
