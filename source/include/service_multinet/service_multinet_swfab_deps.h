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
#ifndef MNET_DEPS_H
#define MNET_DEPS_H

#include "service_multinet_base.h"
#include "service_multinet_swfab.h"
#include "service_multinet_util.h"

typedef struct vlanDepState {
    int vid;
    List trunkPorts;
    List memberEntities;
    BOOL trunksDirty;
    BOOL entitiesDirty;
} VlanTrunkState, *PVlanTrunkState;

typedef struct trunkPort {
    PPlatformPort port;
    List pathList;
    BOOL dirty;
} TrunkPort, *PTrunkPort;

typedef struct entityPortlist {
    int entity;
    List memberPorts;
    BOOL dirty;
} EntityPortList, *PEntityPortList;

typedef struct entityPath {
    int A;
    int B;
} EntityPath, *PEntityPath;

typedef struct entityPathDeps {
    EntityPath path;
    int numPorts;
    PPlatformPort* trunkPorts;
    
} EntityPathDeps, *PEntityPathDeps;


//---- Public

/** Fills listToAppend with new trunk ports that should be configured (PPlatformPort)
 */
int addAndGetTrunkPorts(PVlanTrunkState vidState, PPlatformPort newPort, PList listToAppend);
int removeAndGetTrunkPorts(PVlanTrunkState vidState, PPlatformPort oldPort, PList listToAppend);

int getVlanState(PVlanTrunkState* vidState, int vid);

//---- Private


/** Search for entity within the specified vid. 
 * Returns: NULL if entity is not a vid member. EntityPortList otherwise.
 */
PEntityPortList getEntity(PVlanTrunkState vidState, int entity);

PEntityPortList addEntity(PVlanTrunkState vidState, int entity);

PTrunkPort addTrunkPort(PVlanTrunkState vidState, PPlatformPort platport);

int addPathToTrunkPort(PTrunkPort port, PEntityPath path);

/** Adds a path reference to this port, or adds the port
 * if it was not previously referenced.
 * Returns: 1 if new port is added to the vid. 0 if only a ref is added
 */
int refTrunkPort(PVlanTrunkState vidState, PPlatformPort port, PEntityPath path);

/** Searches for and removes path references matching the specified entity.
 * Returns: 1 if no paths remain for this trunk port. 0 otherwise.
 */
int deRefTrunkPort(PTrunkPort port, int entity);

/**Remove member port, and if the port list is empty, remove this entity from the vid.
 * Returns: 1 if entity was emptied. 0 otherwise.
 */
int deRefEntity(PVlanTrunkState vidState, int entity, PPlatformPort port);

int addMemberPort(PEntityPortList entity, PPlatformPort port);
int remMemberPort(PEntityPortList entity, PPlatformPort port);

int saveVlanState(PVlanTrunkState vidState);
int loadVlanState(PVlanTrunkState vidState);

#endif
