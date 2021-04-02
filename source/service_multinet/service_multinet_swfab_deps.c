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
#include <stddef.h>

#include "service_multinet_base.h"
#include "service_multinet_swfab_deps.h"
#include "service_multinet_util.h"
#include "service_multinet_swfab_plat.h"
#include "service_multinet_swfab_ep.h"

#define MULTINET_DEBUG 1

static List vlanState;

//TODO optimize so that exec calls don't have to load the whole tree every time.
int getVlanState(PVlanTrunkState* vidState, int vid) {
    ListIterator iter;
    PListItem item;
    PVlanTrunkState curState;
    initIterator(&vlanState,&iter);
    MNET_DEBUG("getVlanState, searching for existing vlan for vid %d\n" COMMA vid)
    while ((item = getNext(&iter))) {
        curState = (PVlanTrunkState) item->data;
        if (curState->vid == vid) {
            *vidState = curState;
            return 1;
        }
    }
    MNET_DEBUG("getVlanState, allocate new vlan state\n")
    //WARNING -- error checking needed
    curState = addAndAlloc(&vlanState, sizeof(VlanTrunkState));
    curState->vid = vid;
    loadVlanState(curState);
    *vidState = curState;
    
    return 0;
}

int saveVlanState(PVlanTrunkState vidState) {
    
    int entities[MAX_ENTITIES];
    int numEntities = 0;
    ListIterator iter;
    PListItem item;
    PEntityPortList curEntity;
    
    ListIterator portIter;
    PListItem portItem;
    PPlatformPort curPort;
    PTrunkPort trunkPort;
    PEntityPath path;
    
    EntityPath paths[MAX_PATHS];
    
    char portMemberBuff[512];
    char* portMemberNameList[MAX_ADD_PORTS];
    int numPorts;
    int buffOffset = 0;
    
    //Loop through member entities and add them to entity list
    initIterator(&vidState->memberEntities,&iter);
    while ((item = getNext(&iter))) {
        curEntity = (PEntityPortList) item->data;
        if (vidState->entitiesDirty)
            entities[numEntities++] = curEntity->entity;
        
        if (!curEntity->dirty) continue;
        
        //While we have the entity, add its member ports
        numPorts = 0;
        buffOffset = 0;
        initIterator(&curEntity->memberPorts,&portIter);
        while ((portItem = getNext(&portIter)) 
                && (buffOffset < sizeof(portMemberBuff))
                && (numPorts != MAX_ADD_PORTS))
        {
            curPort = (PPlatformPort) portItem->data;
            portMemberNameList[numPorts] = portMemberBuff + buffOffset;
            buffOffset += curPort->hal->stringID(curPort->portID, portMemberNameList[numPorts++], sizeof(portMemberBuff) - buffOffset);
        }
        ep_set_entity_vid_portMembers(vidState->vid, curEntity->entity, portMemberNameList, numPorts);
    }
    if (vidState->entitiesDirty)
        ep_set_entity_vidMembers(vidState->vid, entities, numEntities);
    
    
    //Loop through trunk ports and add them to the port list
    numPorts = 0;
    buffOffset = 0;
    
    initIterator(&vidState->trunkPorts, &portIter);
    while ((portItem = getNext(&portIter))) {
        trunkPort = (PTrunkPort) portItem->data;
        
        
        portMemberNameList[numPorts] = portMemberBuff + buffOffset;
        buffOffset += trunkPort->port->hal->stringID(trunkPort->port->portID, portMemberNameList[numPorts++], sizeof(portMemberBuff) - buffOffset);
        
        
        if (!trunkPort->dirty) continue;
        
        //Also add each trunk port's dependent paths
        numEntities = 0;
        initIterator(&trunkPort->pathList, &iter);
        while ((item = getNext(&iter))) {
            path = (PEntityPath) item->data;
            paths[numEntities++] = *path;
        }
        ep_set_trunkPort_vid_paths(vidState->vid, portMemberNameList[numPorts-1], paths, numEntities);
    }
    if (vidState->trunksDirty)
        ep_set_trunkPort_vidMembers(vidState->vid, portMemberNameList, numPorts);
    return 0;
}

int loadVlanState(PVlanTrunkState vidState){
    int entities[MAX_ENTITIES];
    int numEntities = sizeof(entities)/sizeof(*entities);
    int i, j = 0;
    MNET_DEBUG("loadVlanState, entry.\n")
    ep_get_entity_vidMembers(vidState->vid, entities, &numEntities);
    MNET_DEBUG("loadVlanState, ep_get_entity_vidMembers returned %d entities\n" COMMA numEntities)
    //char portName[80];
    char portMemberBuff[512];
    char* portMemberNameList[MAX_ADD_PORTS];
    int numPorts;
    PEntityPortList entity;
    
    for (i = 0; i < numEntities; ++i) {
        entity = addEntity(vidState, entities[i]);
	if(entity == NULL)
	{
		MNET_DEBUG("loadVlanState entity is NULL at [%d]\n"COMMA __LINE__)
		return 0;
	}
        numPorts = sizeof(portMemberNameList)/sizeof(*portMemberNameList);
        ep_get_entity_vid_portMembers(vidState->vid, entities[i], portMemberNameList, &numPorts, portMemberBuff, sizeof(portMemberBuff));
		MNET_DEBUG("loadVlanState, ep_get_entity_vid_portMembers returned %d ports for %d vlanID\n" COMMA numPorts COMMA vidState->vid)
	
        for (j = 0; j < numPorts; ++j) {
	    if(j >= MAX_ADD_PORTS)
	    {
		MNET_DEBUG("loadVlanState numPorts is exceeded [%d]\n"COMMA j)
		break;
	    }
            addMemberPort(entity,plat_mapFromString(portMemberNameList[j]));
        }
        entity->dirty = 0;
    }
    vidState->entitiesDirty = 0;
    
    MNET_DEBUG("loadVlanState, got all entity info, moving to trunk ports\n")
    
    EntityPath paths[MAX_PATHS];
    PTrunkPort newPort;
    numPorts = sizeof(portMemberNameList)/sizeof(*portMemberNameList);
    ep_get_trunkPort_vidMembers(vidState->vid, portMemberNameList, &numPorts, portMemberBuff, sizeof(portMemberBuff));
    MNET_DEBUG("loadVlanState, ep_get_trunkPort_vidMembers returned %d ports.\n" COMMA numPorts)
    for (i = 0; i < numPorts; ++i) {
	if(j >= MAX_ADD_PORTS)
	{
		MNET_DEBUG("loadVlanState numPorts is exceeded [%d]\n"COMMA j)
		break;
	}
        newPort = addTrunkPort(vidState, plat_mapFromString(portMemberNameList[i]));
        numEntities = sizeof(paths)/sizeof(*paths);
        ep_get_trunkPort_vid_paths(vidState->vid, portMemberNameList[i], paths, &numEntities);
		MNET_DEBUG("loadVlanState, ep_get_trunkPort_vid_paths returned %d Entities.\n" COMMA numEntities)
		
        for (j = 0; j < numEntities; ++j) {
            addPathToTrunkPort(newPort, paths + j);
        }
        newPort->dirty = 0;
    }
    vidState->trunksDirty = 0;
    return 0;
}

//TODO optimize so that exec calls don't have to load the whole tree every time.
int addAndGetTrunkPorts(PVlanTrunkState vidState, PPlatformPort newPort, PList listToAppend) {
    int i;
    PEntityPortList entity;
    int changedPorts = 0;
    ListIterator entityIter;
    PListItem item;
    PEntityPathDeps pathDeps;
    
    entity = getEntity(vidState, newPort->entity);
    
    MNET_DEBUG("addAndGetTrunkPorts, retrieved entity %d, %s\n" COMMA newPort->entity COMMA entity?"yes":"no")
    
    if (!entity) {
        initIterator(&vidState->memberEntities, &entityIter);
        while ((item = getNext(&entityIter))) {
            
            entity = (PEntityPortList) item->data;
            MNET_DEBUG("addAndGetTrunkPorts, getting path to entity %d\n" COMMA entity->entity)
            pathDeps = newPort->entity < entity->entity ? getPathDeps(newPort->entity, entity->entity) : getPathDeps(entity->entity, newPort->entity); 
            MNET_DEBUG("addAndGetTrunkPorts, got %d ports\n" COMMA pathDeps->numPorts)
            for (i = 0; i < pathDeps->numPorts; ++i) {
                MNET_DEBUG("Refing ") MNET_DBG_CMD(printPlatport(pathDeps->trunkPorts[i])) MNET_DBG_CMD(printf("\n"))
                if (refTrunkPort(vidState, pathDeps->trunkPorts[i], &pathDeps->path)) {
                    addToList(listToAppend, pathDeps->trunkPorts[i]);
                    changedPorts++;
                }
            }
        }
        
        entity = addEntity(vidState, newPort->entity);
        MNET_DEBUG("addAndGetTrunkPorts, added new entity %d\n" COMMA entity->entity)
        
    }
    
    addMemberPort(entity, newPort);
    MNET_DEBUG("addAndGetTrunkPorts, added member port for hal %d\n" COMMA newPort->hal->id)

    return changedPorts;
}

//TODO optimize so that exec calls don't have to load the whole tree every time.
int removeAndGetTrunkPorts(PVlanTrunkState vidState, PPlatformPort oldPort, PList listToAppend){
    PTrunkPort trunkPort;
    int changedPorts = 0;
    ListIterator portIter;
    PListItem item;
    char portname[60];
    
    if (deRefEntity(vidState, oldPort->entity, oldPort)) {
        initIterator(&vidState->trunkPorts, &portIter);
        while ((item = getNext(&portIter))) {
            trunkPort = (PTrunkPort) item->data;
	    if(trunkPort) {
		if (deRefTrunkPort(trunkPort, oldPort->entity)) {
			addToList(listToAppend, trunkPort->port);
			changedPorts++;
			if(trunkPort->port && trunkPort->port->hal) {
				trunkPort->port->hal->stringID(trunkPort->port->portID, portname, sizeof(portname));
				ep_set_trunkPort_vid_paths(vidState->vid, portname, NULL, 0);
				removeCurrent(&portIter);
				vidState->trunksDirty = 1;
				//disposeTrunkPort(trunkPort);
			}
		}
	    }
        }
    }
    
    return changedPorts;
}

int addMemberPort(PEntityPortList entity, PPlatformPort port) {
	ListIterator iter;
	PListItem item;
	PPlatformPort curPort;

	if (!port)
		return -1;
	
	entity->dirty = 1;

	//Check if this port is already on this list, and if so, don't add it again
	initIterator(&entity->memberPorts,&iter);
	while ((item = getNext(&iter))) {
		curPort = (PPlatformPort) item->data;
		if(curPort->hal->id == port->hal->id && curPort->hal->isEqual(curPort, port)) {
			MNET_DEBUG("addMemberPort, skip dupe port ") MNET_DBG_CMD(printPlatport(port)) MNET_DEBUG(" in entity %d\n" COMMA entity->entity)
			return 0;
		}
	}

	MNET_DEBUG("addMemberPort, adding ") MNET_DBG_CMD(printPlatport(port)) MNET_DEBUG(" to entity %d\n" COMMA entity->entity)
	
	return addToList(&entity->memberPorts, port);
}

int remMemberPort(PEntityPortList entity, PPlatformPort port) {
    ListIterator iter;
    PListItem item;
    PPlatformPort curPort;
    MNET_DEBUG("remMemberPort, entry\n")
    initIterator(&entity->memberPorts,&iter);
    MNET_DEBUG("remMemberPort, iterator initted\n")
    MNET_DEBUG("Match port: ") MNET_DBG_CMD(printPlatport(port); printf("\n"))
    while ((item = getNext(&iter))) {
        curPort = (PPlatformPort) item->data;
        MNET_DEBUG("Comparing to ") MNET_DBG_CMD(printPlatport(curPort); printf("\n"))
        if(curPort->hal->id == port->hal->id && curPort->hal->isEqual(curPort, port)) {
            MNET_DEBUG("remMemberPort, found match, removing member port from entity %d\n" COMMA entity->entity)
            removeCurrent(&iter);
            entity->dirty = 1;
            MNET_DEBUG("remMemberPort, removed last port, return\n")
            return 1;
        }
    }
    MNET_DEBUG("remMemberPort, return\n")
    
    return 0;
}

int deRefEntity(PVlanTrunkState vidState, int entity, PPlatformPort port) {
    ListIterator iter;
    PListItem item;
    PEntityPortList curEntity;
    MNET_DEBUG("deRefEntity, entry\n")
    initIterator(&vidState->memberEntities,&iter);
    while ((item = getNext(&iter))) {
        curEntity = (PEntityPortList) item->data;
        if (curEntity->entity == entity) {
            remMemberPort(curEntity, port);
            if (!listSize(&curEntity->memberPorts)) {
                ep_set_entity_vid_portMembers(vidState->vid, entity, NULL, 0);
                removeCurrent(&iter);
                vidState->entitiesDirty = 1;
                //removeFromList(&vidState->memberEntities, item);
                MNET_DEBUG("deRefEntity, removed all paths\n")
                return 1;
            }
            MNET_DEBUG("deRefEntity, return\n")
            return 0;
        }
    }
    
    //Should not get here
    return 0;
}

PEntityPortList getEntity(PVlanTrunkState vidState, int entity) {
    ListIterator iter;
    PListItem item;
    PEntityPortList curEntity;
    MNET_DEBUG("getEntity, entry\n")
    initIterator(&vidState->memberEntities,&iter);
    while ((item = getNext(&iter))) {
        curEntity = (PEntityPortList) item->data;
        if(curEntity->entity == entity)
            return curEntity;
    }
    MNET_DEBUG("getEntity, return\n")
    return NULL;
}

int refTrunkPort(PVlanTrunkState vidState, PPlatformPort port, PEntityPath path) {
    ListIterator iter;
    PListItem item;
    PTrunkPort curPort;
    MNET_DEBUG("refTrunkPort, entry\n")
    initIterator(&vidState->trunkPorts,&iter);
    while ((item = getNext(&iter))) {
        curPort = (PTrunkPort) item->data;
        if(port->hal->id == curPort->port->hal->id && curPort->port->hal->isEqual(curPort->port, port)) {
            addPathToTrunkPort(curPort, path);
            return 0;
        }
    }
    
    //Getting here means the port is not yet a member. Allocate a dependency list wrapper and add it
    //to the vid.
    
    //WARNING - Error handling required.
//     curPort = (PTrunkPort) addAndAlloc(vidState->trunkPorts, sizeof(TrunkPort));
    curPort = addTrunkPort(vidState, port);
    if (curPort) {
//         curPort->port = port;
        addPathToTrunkPort(curPort, path);
    }
    MNET_DEBUG("refTrunkPort, return\n")
    return 1;
}

int addPathToTrunkPort(PTrunkPort port, PEntityPath path) {
    PEntityPath newPath;
    MNET_DEBUG("addPathToTrunkPort, entry\n")
    if (port) {
        newPath = (PEntityPath)addAndAlloc(&port->pathList, sizeof(*path));
        *newPath = *path;
        port->dirty = 1;
    } 
    MNET_DEBUG("addPathToTrunkPort, return\n")
    return 0;
}

PTrunkPort addTrunkPort(PVlanTrunkState vidState, PPlatformPort platport) {
    MNET_DEBUG("addTrunkPort, entry\n")
    PTrunkPort newPort  = (PTrunkPort) addAndAlloc(&vidState->trunkPorts, sizeof(TrunkPort));
    if (newPort)
        newPort->port = platport;
    vidState->trunksDirty=1;
    MNET_DEBUG("addTrunkPort, return\n")
    return newPort;
}

int deRefTrunkPort(PTrunkPort port, int entity){
    ListIterator iter;
    PListItem item;
    PEntityPath path;
    initIterator(&port->pathList,&iter);
    while ((item = getNext(&iter))) {
        path = (PEntityPath) item->data;
        if (path->A == entity || path->B == entity) {
            removeCurrent(&iter);
            port->dirty = 1;
        }
    }
    
    return (listSize(&port->pathList) == 0 ? 1 : 0);
}

PEntityPortList addEntity(PVlanTrunkState vidState, int entity) {
    //WARNING - Error handling required.
    PEntityPortList newEntity;
    newEntity = (PEntityPortList) addAndAlloc(&vidState->memberEntities, sizeof(EntityPortList));
    if (newEntity)
        newEntity->entity = entity;
    vidState->entitiesDirty = 1;
    return newEntity;
}

