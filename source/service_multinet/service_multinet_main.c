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

 /* This is the main executable for service_multinet.It is implemented as
  * a foreground command line interface to the multinet library. 
  */
 
 #include "service_multinet_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<time.h>
#include <stdarg.h>
#include "secure_wrapper.h"
#ifdef MULTILAN_FEATURE
#include <unistd.h>
#endif

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

#include "syscfg/syscfg.h"

#ifndef SERVICE_MULTINET_EXE_PATH
#define SERVICE_MULTINET_EXE_PATH "/etc/utopia/service.d/service_multinet_exec"
#endif
#ifdef MULTILAN_FEATURE
#if defined (INTEL_PUMA7)
#define P7_LEGACY_MULTINET_HANDLER "/etc/utopia/service.d/vlan_util_xb6.sh"
#define P7_PP_ON_ATOM "/etc/utopia/use_multinet_exec"
#define MAX_CMD 255
#define LNF_IPV4_CIDR "192.168.106.254/24"
#define MESHBHAUL_IPV4_CIDR "192.168.245.254/24"
#endif
#endif
#define TRUE 1
#define FALSE 0

bool ethWanEnableState=false;
FILE *mnetfp = NULL;
 
 typedef int (*entryCallback)(char* argv[], int argc);
 
 typedef struct entryCall {
     char call[32];
     entryCallback action;
 } EntryCall;
#ifdef MULTILAN_FEATURE
 int handle_lnf_setup(char* argv[], int argc);
 int handle_meshbhaul_setup(char* argv[], int argc);
#endif
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
void addMeshBhaulVlan();
void createMeshVlan();
void addRadiusVlan();
void addIpcVlan();
void setMulticastMac();
#endif
 int handle_up(char* argv[], int argc);
 int handle_down(char* argv[], int argc);
 int handle_start(char* argv[], int argc);
 int handle_stop(char* argv[], int argc);
 int handle_syncMembers(char* argv[], int argc);
 int handle_syncNets(char* argv[], int argc);
 int handle_restart(char* argv[], int argc);
 int handle_ifStatus(char* argv[], int argc);
 int set_multicast_mac(char* argv[], int argc);
 int add_ipc_vlan(char* argv[], int argc);
 int add_radius_vlan(char* argv[], int argc);
 int create_mesh_vlan(char* argv[], int argc);
 // RDKB-15951
 int add_meshbhaul_vlan(char* argv[], int argc);
#if defined(MESH_ETH_BHAUL)
 int handle_meshethbhaul_bridge_setup(char* argv[], int argc);
 int handle_ethbhaul_start(char* argv[], int argc);
 int handle_ethbhaul_stop(char* argv[], int argc);
#endif
 EntryCall calls[] = {
#ifdef MULTILAN_FEATURE
	 {"lnf-setup", handle_lnf_setup},
	 {"meshbhaul-setup", handle_meshbhaul_setup},
#endif
#if defined(MESH_ETH_BHAUL)
	 {"meshethbhaul-bridge-setup", handle_meshethbhaul_bridge_setup},
	 {"meshethbhaul-up", handle_ethbhaul_start},
	 {"meshethbhaul-down", handle_ethbhaul_stop},
#endif
	 {"multinet-up", handle_up},
	 //{"multinet-ifStatus", handle_ifStatus},
	 {"multinet-down", handle_down},
	 {"multinet-start", handle_start},
	 {"multinet-stop", handle_stop},
	 {"multinet-syncMembers", handle_syncMembers},
	 {"multinet-syncNets", handle_syncNets},
	 {"multinet-restart", handle_restart},
	 {"set_multicast_mac", set_multicast_mac},
	 {"add_ipc_vlan", add_ipc_vlan},
	 {"add_radius_vlan", add_radius_vlan},
	 {"create_mesh_vlan", create_mesh_vlan},
	 {"add_meshbhaul_vlan", add_meshbhaul_vlan}
 };

#define LOG_BUFF_SIZE 256
void multinet_log( char* fmt, ...)
{
    time_t now_time;
    struct tm *lc_time;
    char buff[LOG_BUFF_SIZE] = "";
    va_list args;
    int time_size;

    if(mnetfp == NULL)
        return;
    va_start(args, fmt);
    time(&now_time);
    lc_time=localtime(&now_time);
    time_size = strftime(buff, LOG_BUFF_SIZE, "%Y-%m-%d %H:%M:%S ", lc_time);
    strncat(buff,fmt, (LOG_BUFF_SIZE - time_size -1));
    vfprintf(mnetfp, buff, args);
    va_end(args);
    return;
}

 int main(int argc, char* argv[]) {
     int retval;
    int i;

#ifdef MULTILAN_FEATURE
#if defined (INTEL_PUMA7)
//Puma 7 SoC supports using a different multinet handler for PP on ARM use case
//If PP is not on Atom, call the legacy multinet handler instead of this one
	char cmd[MAX_CMD];
	if (access(P7_PP_ON_ATOM, F_OK)) {
		for (i=1;i<argc;i++) {
			strncat(cmd, argv[i], sizeof(cmd) - strnlen(cmd, MAX_CMD-1) - 1);
			strncat(cmd, " ", sizeof(cmd) - strnlen(cmd, MAX_CMD-1) - 1);
		}
		MNET_DEBUG("Forwarding command to legacy handler: %s\n" COMMA P7_LEGACY_MULTINET_HANDLER);
		return v_secure_system(P7_LEGACY_MULTINET_HANDLER " %s",cmd);
 	}

    #if defined(ENABLE_ETH_WAN) || defined(AUTOWAN_ENABLE)

    char ethwan_enable_state[16] ;
    memset(ethwan_enable_state,0,sizeof(ethwan_enable_state));
    /* Determine if Ethernet WAN is enabled */
    if (0 == syscfg_get(NULL, "eth_wan_enabled", ethwan_enable_state, sizeof(ethwan_enable_state)))
    {
        if(0 == strncmp(ethwan_enable_state, "true", sizeof(ethwan_enable_state)))
        {
            ethWanEnableState = true;
        }
    }
    else
    {
        fprintf(stderr, "Error: %s syscfg_get for eth_wan_enabled failed!\n", __FUNCTION__);
    }
    #endif

#endif
#endif

	if (argc < 2) return 1;
        
	if(mnetfp == NULL) {
		mnetfp = fopen ("/rdklogs/logs/MnetDebug.txt", "a+");
	}
#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#endif 
        MNET_DEBUG("ENTERED MULTINET APP, argc = %d \n" COMMA argc)
        if((retval = multinet_lib_init(0, SERVICE_MULTINET_EXE_PATH))) {
            printf("%s failed to init multinet lib. code=%d\n", SERVICE_MULTINET_EXE_PATH, retval);
            exit(retval);
        }
	
	for (i =0; i < sizeof(calls) / sizeof(*calls); ++i) {
		if (!strcmp(calls[i].call, argv[1])) {
			calls[i].action(argv, argc);
			if(mnetfp)
		      fclose(mnetfp);
			return 0;
		}
	}
	
	if (argc == 7) {
            //FIXME more input validation
            
            handle_ifStatus(argv, argc);
        }
	
	//printUsage();
	if(mnetfp)
		fclose(mnetfp);
	return 1;
	
}

//exeName, eventName, netIdString
#ifdef MULTILAN_FEATURE
 int handle_lnf_setup(char* argv[], int argc) {
     MNET_DEBUG("Main: handle_lnf_setup")
     multinet_bridgeDownInst(atoi(argv[2]));
     multinet_bridgeUpInst(atoi(argv[2]), 0);
     // Assign IP to LnF bridge
     multinet_assignBridgeCIDR(atoi(argv[2]), LNF_IPV4_CIDR, 4);
     // Restart firewall
     v_secure_system("sysevent set firewall-restart");
     return 0;
 }

 int handle_meshbhaul_setup(char* argv[], int argc) {
     MNET_DEBUG("Main: handle_meshbhaul_setup")
     multinet_bridgeDownInst(atoi(argv[2]));
     multinet_bridgeUpInst(atoi(argv[2]), 0);
     // Assign IP to mesh backhaul bridge
     multinet_assignBridgeCIDR(atoi(argv[2]), MESHBHAUL_IPV4_CIDR, 4);
     v_secure_system("sysevent set firewall-restart");
     return 0;
 }
#endif
 int handle_up(char* argv[], int argc) {
    MNET_DEBUG("Main: handle_up")
    multinet_bridgeUpInst(atoi(argv[2]), 0);
    return 0;
 }
 int handle_down(char* argv[], int argc) {
     MNET_DEBUG("Main: handle_down")
     multinet_bridgeDownInst(atoi(argv[2]));
     return 0;
 }
 int handle_start(char* argv[], int argc){
     MNET_DEBUG("Main: handle_start")
     multinet_bridgeUpInst(atoi(argv[2]), 1);
     return 0;
 }
 int handle_stop(char* argv[], int argc){
     MNET_DEBUG("Main: handle_stop")
     multinet_bridgeDownInst(atoi(argv[2]));
     return 0;
 }
 int handle_syncMembers(char* argv[], int argc){
     MNET_DEBUG("Main: handle_syncMembers")
     multinet_SyncInst(atoi(argv[2]));
     return 0;
 }
 int handle_syncNets(char* argv[], int argc){
     MNET_DEBUG("Main: handle_syncNets NOT SUPPORTED")
        //deferred
     return 0;
 }
 int handle_restart(char* argv[], int argc){
    MNET_DEBUG("Main: handle_restart")
    multinet_bridgeDownInst(atoi(argv[2]));
    multinet_bridgeUpInst(atoi(argv[2]), 0);
    return 0;
 }
 //exeName, eventName, statusValue, netIdString, ifname, iftype, tag?
 int handle_ifStatus(char* argv[], int argc){
     int l2netInst;
     MNET_DEBUG("Main: handle_ifStatus")
     sscanf(argv[3], "%d", &l2netInst);
     
     multinet_ifStatusUpdate_ids(l2netInst, argv[4], argv[5], argv[2], argv[6]);
     return 0;
 }

 int set_multicast_mac(char* argv[], int argc)
 {
	MNET_DEBUG("Setting Multicast MACs early\n")
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
	setMulticastMac();
#endif
    return 0;
 }

 int add_ipc_vlan(char* argv[], int argc)
 {
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
    addIpcVlan();
#endif
    return 0;
 }

 int add_radius_vlan(char* argv[], int argc)
 {
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
    addRadiusVlan();
#endif
    return 0;
 }

 int create_mesh_vlan(char* argv[], int argc)
 {
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
    createMeshVlan();
#endif
    return 0;
 }

 int add_meshbhaul_vlan(char* argv[], int argc)
 {
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
    addMeshBhaulVlan();
#endif
    return 0;
 }

#if defined(MESH_ETH_BHAUL)
int handle_meshethbhaul_bridge_setup(char* argv[], int argc) {
     int bridgeMode = 0;
     MNET_DEBUG("Main: handle_meshethbhaul_bridge_setup");
     bridgeMode = atoi(argv[2]);

     if( 0 == bridgeMode)
     {
        if(0 != toggle_ethbhaul_ports(FALSE)) {
           MNET_DEBUG("Main: handle_meshethbhaul_bridge_setup: toggle_ethbhaul_ports Failed: FALSE");
        }
     }
     else if( 1 == bridgeMode)
     {
        if(0 != toggle_ethbhaul_ports(TRUE)) {
           MNET_DEBUG("Main: handle_meshethbhaul_bridge_setup: toggle_ethbhaul_ports Failed: TRUE");
        }
     }
     else
     {
        MNET_DEBUG("Main: handle_meshethbhaul_bridge_setup: Wrong Bridge Mode");
     }
     return 0;
 }
 
 int handle_ethbhaul_start(char* argv[], int argc)
 {
    return toggle_ethbhaul_ports(TRUE);
 }

 int handle_ethbhaul_stop(char* argv[], int argc)
 {
    return toggle_ethbhaul_ports(FALSE);
 }
#endif
