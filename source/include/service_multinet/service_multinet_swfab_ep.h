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
#ifndef MNET_SWFAB_EP_H
#define MNET_SWFAB_EP_H

#include "service_multinet_swfab_deps.h"

#define SWFAB_ENTITY_PORTMEMBER_KEY_FORMAT(vlan,entID) "vid_%d_entity_%d_members", vlan, entID
#define SWFAB_PORT_PATHREF_KEY_FORMAT(vlan,portID) "vid_%d_port_%s_paths", vlan, portID
#define SWFAB_VID_ENTITYMEMBER_KEY_FORMAT(vlan) "vid_%d_entities", vlan
#define SWFAB_VID_TRUNKMEMBER_KEY_FORMAT(vlan) "vid_%d_trunkports", vlan

int ep_set_entity_vid_portMembers(int vid, int entity, char* memberPortNames[], int numPorts);
int ep_set_entity_vidMembers(int vid, int entities[], int numEntities);

int ep_set_trunkPort_vid_paths(int vid, char* portName, PEntityPath paths, int numPaths);
int ep_set_trunkPort_vidMembers(int vid, char* portNames[], int numPorts);



int ep_get_entity_vid_portMembers(int vid, int entity, char* memberPortNames[], int* numPorts, char buf[], int bufSize );
int ep_get_entity_vidMembers(int vid, int entities[], int* numEntities);

int ep_get_trunkPort_vid_paths(int vid, char* portName, PEntityPath paths, int* numPaths);
int ep_get_trunkPort_vidMembers(int vid, char* portNames[], int* numPorts, char buf[], int bufSize);

#endif
