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
#ifndef MNET_EP_H 
#define MNET_EP_H

#include "service_multinet_base.h"

#define MNET_EP_ALLMEMBERS_KEY_FORMAT(x) "multinet_%d-allMembers", x
#define MNET_EP_MEMBER_FORMAT(ifname, iftype, ready) "%[^:]:%[^,],%hhu", iftype, ifname, ready
#define MNET_EP_MEMBER_SET_FORMAT(ifname, iftype, ready) "%s:%s,%hhu" , iftype, ifname, ready
#define MNET_EP_BRIDGE_VID_FORMAT(instance) "multinet_%d-vid", instance
#define MNET_EP_BRIDGE_NAME_FORMAT(instance) "multinet_%d-name", instance
#define MNET_EP_BRIDGE_MODE_KEY "bridge_mode"

//int ep_set_memberStatus(PL2Net net, PMember member); 

int ep_get_allMembers(PL2Net net, PMember live_members, int numMembers);
int ep_set_allMembers(PL2Net net, PMember members, int numMembers);

int ep_clear(PL2Net net); // TODO


int ep_add_active_net(PL2Net net); // TODO deferred
int ep_rem_active_net(PL2Net net); // TODO deferred

int ep_netIsStarted(int netInst); // TODO

int ep_get_bridge(int l2netinst, PL2Net net); 
int ep_set_bridge(PL2Net net);

int ep_get_bridge_mode(void);

//-- Raw

int ep_set_rawString(char* key, char* value);
int ep_get_rawString(char* key, char* value, int valueSize);


#endif

