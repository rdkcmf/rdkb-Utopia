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

#include "service_multinet_lib.h" 
#include "service_multinet_nv.h"
#include "service_multinet_ev.h"
#include "service_multinet_ep.h"
#include "service_multinet_handler.h"
#include "service_multinet_plat.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "nethelper.h"

/*
 * The service_multinet library provides service fuctions for manipulating the lifecycle 
 * and live configuration of system bridges and their device specific interface members. 
 * Authoritative configuration is considered to be held in nonvol storage, so most functions
 * will take this external configuration as input. 
 */

unsigned char isDaemon;
char* executableName;

static int add_members(PL2Net network, PMember interfaceBuf, int numMembers);
static int remove_members(PL2Net network, PMember live_members, int numLiveMembers);
static SERVICE_STATUS check_status(PMember live_members, int numLiveMembers);
static int resolve_member_diff(PL2Net network, PMember members, int* numMembers, PMember live_members, int* numLiveMembers, PMember keep_members, int* numKeepMembers); 
static int isMemberEqual(PMember a, PMember b);

//TODO Move these to a common lib
static int nethelper_bridgeCreate(char* brname) {
    
    char cmdBuff[80];
    snprintf(cmdBuff, sizeof(cmdBuff), "brctl addbr %s; ifconfig %s up", brname, brname);
    MNET_DEBUG("SYSTEM CALL: \"%s\"\n" COMMA cmdBuff)
    system(cmdBuff);
}

static int nethelper_bridgeDestroy(char* brname) {
    
    char cmdBuff[80];
    snprintf(cmdBuff, sizeof(cmdBuff), "ifconfig %s down; brctl delbr %s", brname, brname);
    system(cmdBuff);
}

/* Public interface section */

/* multinet_bridgeUp
 * 
 * Uses the network instance ID to load the stored configuration and initialize 
 * the bridge network. 
 * 
 * Intended to only be called once for a given bridge.
 */
int multinet_bridgeUp(PL2Net network, int bFirewallRestart){
    Member memberBuf[MAX_MEMBERS];
    NetInterface interfaceBuf[MAX_MEMBERS];
    IFType ifTypeBuf[MAX_MEMBERS];
    int numMembers = 0, i;
    
    memset(memberBuf,0, sizeof(memberBuf));
    memset(interfaceBuf,0, sizeof(interfaceBuf));
    memset(ifTypeBuf,0, sizeof(ifTypeBuf));
    for (i = 0; i < MAX_MEMBERS; ++i) {
        memberBuf[i].interface = interfaceBuf + i;
        interfaceBuf[i].type = ifTypeBuf +i;
    }
    
    //Load members from nv
    numMembers = nv_get_members(network, memberBuf, sizeof(memberBuf)/sizeof(*memberBuf));
    MNET_DEBUG("Get members for %d complete. \n" COMMA network->inst)
    numMembers += plat_addImplicitMembers(network, memberBuf+numMembers);
    MNET_DEBUG("plat_addImplicitMembers for %d complete. \n" COMMA network->inst)
    
    //create bridge
    nethelper_bridgeCreate(network->name);
    
    ep_set_bridge(network);
    
    ev_set_netStatus(network, STATUS_PARTIAL); //NOTE bring back if routing issue can be solved
    
    MNET_DEBUG("Bridge create for %d complete. \n" COMMA network->inst)
    
    add_members(network, memberBuf, numMembers);
    
    MNET_DEBUG("add_members for %d complete. \n" COMMA network->inst)
    
    ep_set_allMembers(network, memberBuf, numMembers);
    
    MNET_DEBUG("ep_set_allMembers for %d complete. \n" COMMA network->inst)
    
    
    ev_set_netStatus(network, check_status(memberBuf, numMembers)); 
    
    MNET_DEBUG("Status send for %d complete. \n" COMMA network->inst)
    
    //ep_set_info(network);
    ep_add_active_net(network);
    
    
    MNET_DEBUG("ep_set_bridge for %d complete. \n" COMMA network->inst)
    
    if (bFirewallRestart)
        ev_firewall_restart(); 
    
    return 0;
}
int multinet_bridgeUpInst(int l2netInst, int bFirewallRestart){
    L2Net l2net;
    if(!ep_netIsStarted(l2netInst)) {
        MNET_DEBUG("Found %d is not started. Starting.\n" COMMA l2netInst)
        nv_get_bridge(l2netInst, &l2net);
        MNET_DEBUG("nv fetch complete for %d. Name: %s, Vid: %d\n" COMMA l2netInst COMMA l2net.name COMMA l2net.vid)
        multinet_bridgeUp(&l2net, bFirewallRestart);
        MNET_DEBUG("multinet_bridgeUp for %d complete. \n" COMMA l2netInst)
    }
}



int multinet_bridgeDown(PL2Net network){
    Member memberBuf[MAX_MEMBERS];
    NetInterface interfaceBuf[MAX_MEMBERS];
    IFType ifTypeBuf[MAX_MEMBERS];
    int numMembers = 0, i;
    
    memset(memberBuf,0, sizeof(memberBuf));
    memset(interfaceBuf,0, sizeof(interfaceBuf));
    memset(ifTypeBuf,0, sizeof(ifTypeBuf));
    
    for (i = 0; i < MAX_MEMBERS; ++i) {
        memberBuf[i].interface = interfaceBuf + i;
        interfaceBuf[i].type = ifTypeBuf +i;
    }
    
    //read ep ready memebers
    numMembers = ep_get_allMembers(network, memberBuf, sizeof(memberBuf)/sizeof(*memberBuf));
    MNET_DEBUG("multinet_bridgeDown, ep_get_allMembers returned\n")
    //delete vlans
    remove_members(network, memberBuf, numMembers);
    MNET_DEBUG("remove_members, ep_get_allMembers returned\n")
    
    //delete bridge
    nethelper_bridgeDestroy(network->name);
    
    //clear info and status
    ev_set_netStatus(network, STATUS_STOPPED); 
    
    ep_clear(network);
    ep_rem_active_net(network);
}
int multinet_bridgeDownInst(int l2netInst){
    L2Net l2net;
    if(ep_netIsStarted(l2netInst)) {
        nv_get_bridge(l2netInst, &l2net);
        multinet_bridgeDown(&l2net);
    }
}

int multinet_Sync(PL2Net network, PMember members, int numMembers){
    int i;
    
    Member live_members[MAX_MEMBERS];
    NetInterface interfaceBuf[MAX_MEMBERS];
    IFType ifTypeBuf[MAX_MEMBERS];
    
    memset(live_members,0, sizeof(live_members));
    memset(interfaceBuf,0, sizeof(interfaceBuf));
    memset(ifTypeBuf,0, sizeof(ifTypeBuf));
    
    for (i = 0; i < MAX_MEMBERS; ++i) {
        live_members[i].interface = interfaceBuf + i;
        interfaceBuf[i].type = ifTypeBuf +i;
    }
    
    Member keep_members[MAX_MEMBERS];
    NetInterface interfaceBuf2[MAX_MEMBERS];
    IFType ifTypeBuf2[MAX_MEMBERS];
    
    memset(live_members,0, sizeof(live_members));
    memset(interfaceBuf2,0, sizeof(interfaceBuf2));
    memset(ifTypeBuf2,0, sizeof(ifTypeBuf2));
    
    for (i = 0; i < MAX_MEMBERS; ++i) {
        live_members[i].interface = interfaceBuf2 + i;
        interfaceBuf2[i].type = ifTypeBuf2 +i;
    }
    
    int numKeepMembers = 0;
    int numLiveMembers;
    //L2Net nv_net;
    L2Net live_net;
    
    
    //Don't re-sync members of networks that are not up
    if(!ep_netIsStarted(network->inst)) {
        return -1;
    }
    
    //nv_get_bridge(network->inst, &nv_net);
    ep_get_bridge(network->inst, &live_net);
    
    //Check disabled
    if (!network->bEnabled) {
        multinet_bridgeDown(network);
        return 0;
    }
    
    numLiveMembers = ep_get_allMembers(&live_net, live_members, sizeof(live_members)/sizeof(*live_members));
    //numNvMembers = nv_get_members(&nv_net, nv_members, sizeof(nv_members));
    
    //Sync vlan
    //Only pare down add/remove lists if there is no vlan change. If the vlan is different, 
    //  all interfaces must have the old vlan deleted, and the new vlan must be added. 
    if (live_net.vid == network->vid) {
        resolve_member_diff(network, members, &numMembers, live_members, &numLiveMembers, keep_members, &numKeepMembers); //Modifies buffers into new and remove lists
    }
    
    MNET_DEBUG("Resolved member diff. %d add, %d keep, %d remove\n" COMMA numMembers COMMA numKeepMembers COMMA numLiveMembers)
    
    //Sync members
    remove_members(network, live_members, numLiveMembers);
    
    add_members(network, members, numMembers);
    
    //numLiveMembers = ep_get_allMembers(&live_net, live_members, sizeof(live_members)/sizeof(*live_members));
    
    for ( i =0; i < numMembers; ++i) {
        keep_members[numKeepMembers++] = members[i];
    }
    
    ep_set_allMembers(network, keep_members, numKeepMembers);
    
    ep_set_bridge(network);
    
    ev_set_netStatus(network, check_status(keep_members, numKeepMembers)); 
    
    //Sync name TODO Deferred!
    
    
}
int multinet_SyncInst(int l2netInst){
    L2Net nv_net;
    Member nv_members[MAX_MEMBERS];
    NetInterface interfaceBuf[MAX_MEMBERS];
    IFType ifTypeBuf[MAX_MEMBERS];
    int i;
    
    for (i = 0; i < MAX_MEMBERS; ++i) {
        nv_members[i].interface = interfaceBuf + i;
        interfaceBuf[i].type = ifTypeBuf +i;
    }
    
    int numNvMembers;
    
    nv_net.inst = l2netInst;
    if (ep_netIsStarted(l2netInst)) {
        nv_get_bridge(l2netInst, &nv_net);
        numNvMembers = nv_get_members(&nv_net, nv_members, sizeof(nv_members)/sizeof(*nv_members));
        numNvMembers += plat_addImplicitMembers(&nv_net, nv_members+numNvMembers);
        return multinet_Sync(&nv_net, nv_members, numNvMembers);
    }
    return 0;
}

int multinet_bridgesSync(){
    //TODO: compare nonvol instances to running instances, and stop instances that no longer exist.
    return 0;
}

//TODO: This should only take in an "NetInterface" and search the member list for
//relevant members. 
int multinet_ifStatusUpdate(PL2Net network, PMember interface, IF_STATUS status){
    int i;
    Member live_members[MAX_MEMBERS];
    NetInterface interfaceBuf[MAX_MEMBERS];
    IFType ifTypeBuf[MAX_MEMBERS];
    
    memset(live_members,0, sizeof(live_members));
    memset(interfaceBuf,0, sizeof(interfaceBuf));
    memset(ifTypeBuf,0, sizeof(ifTypeBuf));
    
    for (i = 0; i < MAX_MEMBERS; ++i) {
        live_members[i].interface = interfaceBuf + i;
        interfaceBuf[i].type = ifTypeBuf +i;
    }
    int numLiveMembers;
    
    interface->bReady = status;
    
    if (status == IF_STATUS_UP) {
        add_vlan_for_members(network, interface, 1);
        MNET_DEBUG("multinet_ifStatusUpdate: finished with add_vlan_for_members\n")
//         if (interface->bLocal) {
//             nethelper_bridgeAddIf(network->name, interface->name);
//         }
    } //FIXME should remove vlans for down ports, even if it's a no-op
    
    //ep_set_memberStatus(network, interface);
    
    numLiveMembers = ep_get_allMembers(network, live_members, sizeof(live_members)/sizeof(*live_members));
    MNET_DEBUG("multinet_ifStatusUpdate: finished with ep_get_allMembers\n")
    for ( i = 0; i < numLiveMembers; ++i) {
        if (isMemberEqual(interface, live_members + i)) {
            live_members[i].bReady = status;
        }
    }
    MNET_DEBUG("multinet_ifStatusUpdate: finished setting ready bits\n")
    ep_set_allMembers(network, live_members,numLiveMembers);
    MNET_DEBUG("multinet_ifStatusUpdate: finished set all members\n")
    
    ev_set_netStatus(network, check_status(live_members, numLiveMembers));
    MNET_DEBUG("multinet_ifStatusUpdate: exit\n")
}
int multinet_ifStatusUpdate_ids(int l2netInst, char* ifname, char* ifType, char* status, char* tagging){ 
    IFType type = {0};
    NetInterface iface = {0};
    Member member = {0};
    L2Net net = {0};
    IF_STATUS ifStatus;
    strcpy(type.name, ifType);
    strcpy(iface.name, ifname);
    iface.type = &type;
    member.interface = &iface;
    
    nv_get_bridge(l2netInst, &net);
    ev_string_to_status(status, &ifStatus);
    
    
    member.bTagging = strcmp("tag", tagging) ? 0 : 1;
    
    return multinet_ifStatusUpdate(&net, &member, ifStatus);
    
}

int multinet_lib_init(BOOL daemon, char* exeName) { 
    int retval;
    isDaemon = daemon;
    
    retval = ev_init();
    
    if (retval) return retval; // Failed to initialize the library
    
    MNET_DEBUG("Setting executableName: %s\n" COMMA exeName)
    
    executableName = strdup(exeName);
    
    handlerInit();
    
    return 0;
}

/* Internal interface section */

static int add_members(PL2Net network, PMember interfaceBuf, int numMembers) 
{                                          
   	char cmdBuff[128] = {0}; 
	int i;
	int offset = 0;

	//For brlan2 and brlan3 case continue to use the old method of creating l2sd0 and gretap0 interfaces
    if (1 != network->inst && 2 != network->inst)
    {
        //register for member status
        create_and_register_if(network, interfaceBuf, numMembers);
    
    	//add vlans for ready members
    	add_vlan_for_members(network, interfaceBuf, numMembers);
	}
	else
	{
		for (i = 0; i < numMembers; ++i) 
		{
        	MNET_DEBUG(" -- interface:%s interface type:%s \n" COMMA interfaceBuf[i].interface->name COMMA interfaceBuf[i].interface->type->name)
			if(!strncmp(interfaceBuf[i].interface->type->name, "SW", 2))
			{
				MNET_DEBUG(" -- Interface type is SW")
				sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", "addVlan", network->inst, 
														 network->vid, interfaceBuf[i].interface->name, interfaceBuf[i].bTagging ? "-t" : "");
    
				MNET_DEBUG(" -- Command for SW interface is:%s\n" COMMA cmdBuff)			
		        system(cmdBuff);
			}
			else if(!strncmp(interfaceBuf[i].interface->type->name, "WiFi", 4))
			{
				MNET_DEBUG(" -- Interface type is WIFI")
				sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_wifi.sh", "addVlan", network->inst, 
														 network->vid, interfaceBuf[i].interface->name, interfaceBuf[i].bTagging ? "-t" : "");	

				MNET_DEBUG(" -- Command for wifi interface is:%s\n" COMMA cmdBuff)			
		        system(cmdBuff);
			}
			else if(!strncmp(interfaceBuf[i].interface->type->name, "Link", 4))
			{
				MNET_DEBUG(" --Interface type is Link")
				if (interfaceBuf[i].bTagging) 
				{
                	offset += snprintf(cmdBuff+offset, 
                    		           sizeof(cmdBuff) - offset,
                            		   "vconfig add %s %d ; ", 
		                               interfaceBuf[i].interface->name, 
        		                       network->vid);
            	}
    
            	offset += snprintf(cmdBuff+offset, 
                	               sizeof(cmdBuff) - offset,
                    	           "brctl addif %s %s", 
                        	       network->name,
                            	   interfaceBuf[i].interface->name); 
    
	            if (interfaceBuf[i].bTagging) 
				{
                	offset += snprintf(cmdBuff+offset, 
                    			       sizeof(cmdBuff) - offset,
			                           ".%d", 
            			               network->vid); 
	            }
    
        		offset += snprintf(cmdBuff+offset, 
    	                           sizeof(cmdBuff) - offset,
                	               " ; ifconfig %s up", 
                    	           interfaceBuf[i].interface->name);

	            if (interfaceBuf[i].bTagging) 
				{
                	offset += snprintf(cmdBuff+offset, 
                    		           sizeof(cmdBuff) - offset,
                            		   " ; ifconfig %s.%d up", 
		                               interfaceBuf[i].interface->name, 
        		                       network->vid);
            	}	
				MNET_DEBUG(" -- Command for Link type interface is:%s\n" COMMA cmdBuff)
				system(cmdBuff);
			}
			else
			{
				MNET_DEBUG("Other interface types are not processed for brlan0 and brlan1\n")
			}
        	memset(cmdBuff, 0, sizeof(cmdBuff));
	        offset = 0;		
		}
	}
}

static int remove_members(PL2Net network, PMember live_members, int numLiveMembers) {
    
	char cmdBuff[128] = {0}; 
    int i;
    int offset = 0;

	//For brlan2 and brlan3 case continue to use the old method of removing l2sd0 and gretap0 interfaces
    if (1 != network->inst && 2 != network->inst)
    {  
    	unregister_if(network, live_members, numLiveMembers);    
	    remove_vlan_for_members(network, live_members, numLiveMembers);
	}	
	else
	{
		for (i = 0; i < numLiveMembers; ++i) 
    	{   
        	MNET_DEBUG(" -- interface:%s interface type:%s \n" COMMA live_members[i].interface->name COMMA live_members[i].interface->type->name)
	        if(!strncmp(live_members[i].interface->type->name, "SW", 2)) 
    	    {
        	    MNET_DEBUG(" -- Interface type is SW")
            	sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", "delVlan", network->inst, 
														 network->vid, live_members[i].interface->name, live_members[i].bTagging ? "-t" : "");
    
	            MNET_DEBUG(" -- Command for SW interface is:%s\n" COMMA cmdBuff)    
    	        system(cmdBuff);
        	}
	        else if(!strncmp(live_members[i].interface->type->name, "WiFi", 4)) 
    	    {
        	    MNET_DEBUG(" -- Interface type is WIFI")
            	sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_wifi.sh", "delVlan", network->inst, 
														 network->vid, live_members[i].interface->name, live_members[i].bTagging ? "-t" : "");

	            MNET_DEBUG(" -- Command for wifi interface is:%s\n" COMMA cmdBuff)
    	        system(cmdBuff);
        	}
	        else if((!strncmp(live_members[i].interface->type->name, "Link", 4))) 
        	{
	            MNET_DEBUG(" --Interface type is Link / Gre\n")
				if (live_members[i].bTagging) 
				{
            	    offset += snprintf(cmdBuff+offset, 
                	    	           sizeof(cmdBuff) - offset,
                    	    	       "vconfig rem %s.%d",
									   live_members[i].interface->name,
        		                       network->vid); 
	            } 
				else 
				{
            	    offset += snprintf(cmdBuff+offset, 
	    		                       sizeof(cmdBuff) - offset,
    	        	                   "brctl delif %s %s", 
									   network->name,
		                               live_members[i].interface->name);
			
            	}
            	MNET_DEBUG(" -- Command for Link type interface is:%s\n" COMMA cmdBuff)
            	system(cmdBuff);
        	}
			else
			{
				MNET_DEBUG("Nothing to do for other interface types\n")
			}		
        	memset(cmdBuff, 0, sizeof(cmdBuff));
	        offset = 0;		
		}
	}
}

static SERVICE_STATUS check_status(PMember live_members, int numLiveMembers) {
    int i;
    
    int all = 1, none=1;
    
    for (i = 0; i < numLiveMembers; ++i ) {
        if (!live_members[i].interface->dynamic || live_members[i].bReady) {
            none = 0;
        } else {
            all = 0;
        }
    }
    
    if (all)
        return STATUS_STARTED;
    else
        return STATUS_PARTIAL;
}

static int isMemberEqual(PMember a, PMember b) {
 
    MNET_DEBUG("Comparing interfaces, %s:%s-%s, %s:%s-%s\n" COMMA a->interface->type->name COMMA a->interface->name COMMA a->bTagging ? "t" : "ut" COMMA b->interface->type->name COMMA b->interface->name COMMA b->bTagging ? "t" : "ut")
    if (strncmp(a->interface->name, b->interface->name, sizeof(a->interface->name)))
        return 0;
    if (strncmp(a->interface->type->name, b->interface->type->name, sizeof(a->interface->type->name)))
        return 0;
    if (a->bTagging != b->bTagging)
        return 0;
    
    MNET_DEBUG("Returning match\n")
    
    return 1;
}

static inline void deleteFromMemberArray(PMember memberArray, int index, int* numMembers) {
    MNET_DEBUG("Deleting %s-%s\n" COMMA memberArray[index].interface->type->name COMMA memberArray[index].bTagging ? "t" : "ut")
    if (*numMembers > 1)
        memberArray[index] = memberArray[*numMembers - 1];
    
    (*numMembers)--;
}

static int resolve_member_diff(PL2Net network, 
                               PMember members, int* numMembers, 
                               PMember live_members, int* numLiveMembers,
                               PMember keep_members, int* numKeepMembers) {
    int i, j; 
    
    for (i = 0; i < *numMembers; ++i ) {
        for (j = 0; j < *numLiveMembers; ++j) {
            if (isMemberEqual(members + i, live_members + j)) {
                keep_members[*numKeepMembers] = members[i];
                deleteFromMemberArray(members, i, numMembers);
                deleteFromMemberArray(live_members, j, numLiveMembers);
                
                (*numKeepMembers)++;
                --i;
                break;
            }
        }
    }
    
    return 0;
}


//TODO:Deferred BRIDGES SYNC!!!
//TODO: inspect differences from script
//TODO: use utplat defines

