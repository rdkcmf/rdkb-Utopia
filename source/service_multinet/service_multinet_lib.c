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
#include "syscfg/syscfg.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "errno.h"
//#include "nethelper.h"
#define LOCAL_BRLAN1UP_FILE "/tmp/brlan1_up"
#if defined(MOCA_HOME_ISOLATION)
#define LOCAL_MOCABR_UP_FILE "/tmp/MoCABridge_up"
#define MOCA_BRIDGE_IP "169.254.30.1"
#endif

/* Syscfg keys used for calculating mac addresses of local interfaces and bridges */
#define BASE_MAC_SYSCFG_KEY                  "base_mac_address"
/* Offset at which LAN bridge mac addresses will start */
#define BASE_MAC_BRIDGE_OFFSET_SYSCFG_KEY    "base_mac_bridge_offset"
#define CMD_STRING_LEN 80
#define MAC_ADDRESS_OCTET_MAX 0x100
#define MAC_ADDRESS_LOCAL_MASK 0x02

 /* The service_multinet library provides service fuctions for manipulating the lifecycle 
 * and live configuration of system bridges and their device specific interface members. 
 * Authoritative configuration is considered to be held in nonvol storage, so most functions
 * will take this external configuration as input. 
 */

unsigned char isDaemon;
char* executableName;
static int syscfg_init_done = 0;

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
    return 0;
}

/* nethelper_bridgeCreateUniqueMac
 *
 * Creates a bridge with a unique and consistent mac address.
 * The mac address is calculated by starting with the base mac address taken from
 * the syscfg key defined in BASE_MAC_SYSCFG_KEY.  Then the bridge instance number is added
 * to the least-significant octet.  If the resulting octet is larger than MAC_ADDRESS_OCTET_MAX,
 * the value becomes the remainder of the intermediate value divided by MAC_ADDRESS_OCTET_MAX.
 * This is to allow roll-over in the resulting mac address octet.
 * Finally, the Universal/Local bit (found in MAC_ADDRESS_LOCAL_MASK) is set on the
 * resulting mac address to specify that this is NOT a globally-unique mac address.
*/
static int nethelper_bridgeCreateUniqueMac(char* brname, int id) {
    char cmdBuff[CMD_STRING_LEN];
    int result = 0;
    int mac_offset = 0;
    int mac[6];
    int got_base_mac = 0;

    /* Create bridge */
    snprintf(cmdBuff, sizeof(cmdBuff), "brctl addbr %s", brname);
    MNET_DEBUG("SYSTEM CALL: \"%s\"\n" COMMA cmdBuff);
    system(cmdBuff);

    if (!syscfg_init_done)
    {
        syscfg_init();
        syscfg_init_done = 1;
    }

    if (!syscfg_get(NULL, BASE_MAC_BRIDGE_OFFSET_SYSCFG_KEY, cmdBuff, sizeof(cmdBuff)))
    {
        MNET_DEBUG("Got %s = %s from syscfg" COMMA BASE_MAC_BRIDGE_OFFSET_SYSCFG_KEY COMMA cmdBuff);
        mac_offset = atoi(cmdBuff);

        if (!syscfg_get(NULL, BASE_MAC_SYSCFG_KEY, cmdBuff, sizeof(cmdBuff)))
        {
            MNET_DEBUG("Got %s = %s from syscfg" COMMA BASE_MAC_SYSCFG_KEY COMMA cmdBuff);

            got_base_mac = 1;

            if (sscanf(cmdBuff, "%x:%x:%x:%x:%x:%x",
                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]
                ) == 6)
            {
                /* Add offset and instance to least significant octet */
                mac[5] += (mac_offset + id);
                /* Handle roll-over */
                mac[5] %= MAC_ADDRESS_OCTET_MAX;
                /* Set as a local mac address */
                mac[0] |= MAC_ADDRESS_LOCAL_MASK;
                /* Set the newly-generated mac address to the bridge */
                snprintf(cmdBuff, sizeof(cmdBuff), "ifconfig %s hw ether %02x:%02x:%02x:%02x:%02x:%02x",
                    brname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                MNET_DEBUG("SYSTEM CALL: \"%s\"\n" COMMA cmdBuff);
                system(cmdBuff);
            }
        }
        else
        {
            MNET_DEBUG("Couldn't get %s from syscfg" COMMA BASE_MAC_SYSCFG_KEY);
        }
    }
    else
    {
        MNET_DEBUG("Couldn't get %s from syscfg" BASE_MAC_BRIDGE_OFFSET_SYSCFG_KEY);
    }

    /* Workaround, Linux will not generate a link-local address if no switch ports connected */
    if (got_base_mac)
    {
        snprintf(cmdBuff, sizeof(cmdBuff), "ip link add tmp%d type dummy", id);
        system(cmdBuff);
        snprintf(cmdBuff, sizeof(cmdBuff), "echo 1 > /proc/sys/net/ipv6/conf/tmp%d/disable_ipv6", id);
        system(cmdBuff);
        snprintf(cmdBuff, sizeof(cmdBuff), "ifconfig tmp%d up", id);
        system(cmdBuff);
        snprintf(cmdBuff, sizeof(cmdBuff), "brctl addif %s tmp%d", brname, id);
        system(cmdBuff);
    }

    /* Bring bridge up */
    snprintf(cmdBuff, sizeof(cmdBuff), "ifconfig %s up", brname);
    MNET_DEBUG("SYSTEM CALL: \"%s\"\n" COMMA cmdBuff);
    system(cmdBuff);

    if (got_base_mac)
    {
        snprintf(cmdBuff, sizeof(cmdBuff), "ip link del tmp%d", id);
        system(cmdBuff);
    }

    return result;
}

static int nethelper_bridgeDestroy(char* brname) {
    
    char cmdBuff[80];
    snprintf(cmdBuff, sizeof(cmdBuff), "ifconfig %s down; brctl delbr %s", brname, brname);
    system(cmdBuff);
    return 0;
}
#if defined(MOCA_HOME_ISOLATION)
void ConfigureMoCABridge(L2Net l2net)
{
    char cmdBuff[512];
    snprintf(cmdBuff, sizeof(cmdBuff), "ip link set %s allmulticast on; ifconfig %s %s;ip link set %d up",l2net.name, l2net.name, MOCA_BRIDGE_IP,l2net.name);
    system(cmdBuff);
    system("echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts");
    system("sysctl -w net.ipv4.conf.all.arp_announce=3");
    system("ip rule add from all iif brlan10 lookup all_lans");
    system("ip rule add from all iif brlan0 lookup moca");

}
#endif
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
    nethelper_bridgeCreateUniqueMac(network->name, network->inst);
    
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
    int fd = 0;/*RDKB-12965 & CID:- 34240*/
	memset(&l2net,0,sizeof(l2net)); /*RDKB-12965 & CID:-33815*/
    if(!ep_netIsStarted(l2netInst)) {
        MNET_DEBUG("Found %d is not started. Starting.\n" COMMA l2netInst)
        nv_get_bridge(l2netInst, &l2net);
        MNET_DEBUG("nv fetch complete for %d. Name: %s, Vid: %d\n" COMMA l2netInst COMMA l2net.name COMMA l2net.vid)
        multinet_bridgeUp(&l2net, bFirewallRestart);
        MNET_DEBUG("multinet_bridgeUp for %d complete. \n" COMMA l2netInst)
		// For brlan1 case create a temp file so that cosa_start_rem.sh execution can continue.
        if (2 == l2netInst)
        {
            MNET_DEBUG("brlan1 case creating %s file \n" COMMA LOCAL_BRLAN1UP_FILE)
            if((fd = creat(LOCAL_BRLAN1UP_FILE, S_IRUSR | S_IWUSR)) == -1) /*RDKB-12965 & CID:- 34240*/
            {
                MNET_DEBUG("%s file creation failed with error:%d\n" COMMA LOCAL_BRLAN1UP_FILE COMMA errno)
            }
            else
            {
                MNET_DEBUG("%s file creation is successful \n" COMMA LOCAL_BRLAN1UP_FILE)
		close(fd); /*RDKB-12965 & CID:- 34240*/
            }
        }
#if defined(MOCA_HOME_ISOLATION)
	if(9 == l2netInst)
	{
	    ConfigureMoCABridge(l2net);
            MNET_DEBUG("MoCA Bridge case creating %s file \n" COMMA LOCAL_MOCABR_UP_FILE)
            if(creat(LOCAL_MOCABR_UP_FILE, S_IRUSR | S_IWUSR) == -1)
            {
                MNET_DEBUG("%s file creation failed with error:%d\n" COMMA LOCAL_MOCABR_UP_FILE COMMA errno)
            }
            else
            {
                MNET_DEBUG("%s file creation is successful \n" COMMA LOCAL_MOCABR_UP_FILE)
            }

	}
#endif
    }
    return 0;
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
    return 0;
}
int multinet_bridgeDownInst(int l2netInst){
    L2Net l2net;
    if(ep_netIsStarted(l2netInst)) {
        nv_get_bridge(l2netInst, &l2net);
        multinet_bridgeDown(&l2net);
    }
    return 0;
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
    
    //Interfaces to keep will have status already started
    for (i = 0; i < numKeepMembers; ++i) {
        keep_members[i].bReady = STATUS_STARTED;
    }

    for (i = 0; i < numMembers; ++i) {
        keep_members[numKeepMembers++] = members[i];
    }
    
    ep_set_allMembers(network, keep_members, numKeepMembers);
    
    ep_set_bridge(network);
    
    ev_set_netStatus(network, check_status(keep_members, numKeepMembers)); 
    
    //Sync name TODO Deferred!
    
    return 0;
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
    return 0;
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
    //register for member status
    create_and_register_if(network, interfaceBuf, numMembers);
    
    //add vlans for ready members
    add_vlan_for_members(network, interfaceBuf, numMembers);                                

    return 0;
}

static int remove_members(PL2Net network, PMember live_members, int numLiveMembers) 
{
    unregister_if(network, live_members, numLiveMembers);
    remove_vlan_for_members(network, live_members, numLiveMembers);    
    return 0;
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

