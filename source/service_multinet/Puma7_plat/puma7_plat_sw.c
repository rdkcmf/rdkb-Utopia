/****************************************************************************
  Copyright 2017 Intel Corporation

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
******************************************************************************/

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
*****************************bLibInited*****************************************/
#include "service_multinet_base.h"
#include "puma7_plat_sw.h"
#include "puma7_plat_map.h"
#include "service_multinet_ep.h"
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define PP_DRIVER_MODULE_ID                         (0xDF)
#define PP_DRIVER_DELETE_VPID                       _IOWR (PP_DRIVER_MODULE_ID, 3, pp_dev_ioctl_param_t)
#define PP_DRIVER_ADD_VPID                          _IOWR (PP_DRIVER_MODULE_ID, 4, pp_dev_ioctl_param_t)

#define MAX_CMD_SIZE 256
#define GRE_CLAMP_MTU 1400

/* Wrap the system_wrapper() call, setting the default SIGCHLD handler before calling,
 * and restoring the old handler after the call.  Needed so that system_wrapper() will 
 * return success or failure. 
 */
int system_wrapper(const char * command)
{
    int result = 0;
#ifdef _GNU_SOURCE
    sighandler_t old_signal;
#else
    sig_t old_signal;
#endif

    /* Set the default SIGCHLD handler */
    if ((old_signal = signal(SIGCHLD, SIG_DFL)) == SIG_ERR)
    {
        MNET_DEBUG("ERROR: Couldn't set default SIGCHLD handler!\n");
        return -1;
    }

    result = system(command);

    /* Restore previous SIGCHLD handler */
    if (signal(SIGCHLD, old_signal) == SIG_ERR)
    {
        MNET_DEBUG("ERROR: Couldn't restore previous SIGCHLD handler!\n");
        return -1;
    }

    return result;
}

/* Creates or destroys a VPID for an interface */
static int configure_vpid(char * ifname, bool create)
{
    int rc = 0;
    int ppDevfd = NULL;
    pp_dev_ioctl_param_t interface_param = {{0}};
    void * data = (void *)&interface_param;
    unsigned int cmd;

    /* Do not create or delete VPID for MoCA interfaces */
    if ( strstr( ifname, "moca" ) )
    {
        MNET_DEBUG("Skipping VPID create/delete for MoCA interface.\n");
        return 0;
    }

    /* Populate the structures needed for the ioctl */
    if (create)
    {
        cmd = PP_DRIVER_ADD_VPID;
    }
    else
    {
        cmd = PP_DRIVER_DELETE_VPID;
    }   
    interface_param.qos_virtual_scheme_idx = 0;
    snprintf(interface_param.device_name, sizeof(interface_param.device_name), "%s", ifname);

    /* Open the device handle */
    if ( ( ppDevfd = open ( "/dev/pp" , O_RDWR ) ) < 0 )
    {
        fprintf(stderr, "Error in open PP driver %d\n", ppDevfd);
        return -1;
    }

    /* Send Command to PP driver via ioctl */
    if ((rc = ioctl(ppDevfd, cmd, data)) != 0)
    {
        close(ppDevfd);
        return -1;
    }

    close(ppDevfd);
    return 0;
}

static int psm_get_record(const char *name, char *val, int size)
{
    FILE *fp;
    char cmd[MAX_CMD_SIZE];

    snprintf(cmd, sizeof(cmd), "psmcli get %s", name);

    if ((fp = popen(cmd, "rb")) == NULL) {
        return -1;
    }

    fgets(val, size, fp);

    pclose(fp);
    return 0;
}

/* Helper function to connect/disconnect ports to/from bridge */
int portHelper(char *bridge, char *port, int tagging, int vid, BOOL up)
{
    int result = 0;
    char temp_ifname[MAX_CMD_SIZE];
    char cmdBuff[MAX_CMD_SIZE];
    char suffix[8];

    strncpy(temp_ifname, port, sizeof(temp_ifname));

    if (up)
    {
        //Create VLAN interface if necessary
        if (tagging) {
            snprintf(cmdBuff, sizeof(cmdBuff), "vconfig add %s %d", temp_ifname, vid);
            MNET_DEBUG("portHelper, command is %s\n" COMMA cmdBuff);
            system_wrapper(cmdBuff);
            snprintf(suffix, sizeof(suffix), ".%d", vid);
            strncat(temp_ifname, suffix, sizeof(temp_ifname));
        }

        //Create VPID used to enable PP acceleration
        if (0 != (result = configure_vpid(temp_ifname, true)))
        {
            MNET_DEBUG("Failed to create VPID!\n");
            //It's OK if this fails
        }

        //Make sure interface is UP
        snprintf(cmdBuff, sizeof(cmdBuff), "ifconfig %s up", temp_ifname);
        MNET_DEBUG("portHelper, command is %s\n" COMMA cmdBuff);
        result = system_wrapper(cmdBuff);

        //Connect vlan interface to bridge.  This must succeed or entire function fails.
        snprintf(cmdBuff, sizeof(cmdBuff), "ip link set nomaster %s", temp_ifname);
        MNET_DEBUG("portHelper, command is %s\n" COMMA cmdBuff);
        system_wrapper(cmdBuff);
        snprintf(cmdBuff, sizeof(cmdBuff), "brctl addif %s %s", bridge, temp_ifname);
        MNET_DEBUG("portHelper, command is %s\n" COMMA cmdBuff);
        result = system_wrapper(cmdBuff);
    }
    else
    {
        //Use VLAN interface if necessary
        if (tagging) {
            snprintf(suffix, sizeof(suffix), ".%d", vid);
            strncat(temp_ifname, suffix, sizeof(temp_ifname));
        }

        //Delete VPID used to enable PP acceleration
        if (0 != (result = configure_vpid(temp_ifname, false)))
        {
            MNET_DEBUG("Failed to destroy VPID!\n");
            //It's OK if this fails
        }

        //Disconnect vlan interface from bridge
        snprintf(cmdBuff, sizeof(cmdBuff), "brctl delif %s %s", bridge, temp_ifname);
        MNET_DEBUG("portHelper, command is %s\n" COMMA cmdBuff);
        result = system_wrapper(cmdBuff);

        //Delete VLAN interface if necessary
        if (tagging) {
            snprintf(cmdBuff, sizeof(cmdBuff), "vconfig rem %s", temp_ifname);
            MNET_DEBUG("portHelper, command is %s\n" COMMA cmdBuff);
            system_wrapper(cmdBuff);
        }
    }

    return result;
}

/* Function with GRE specific handling for adding a port to the bridge */
int configVlan_GRE(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    char cmdBuff[MAX_CMD_SIZE];
    char ruleBuff[MAX_CMD_SIZE];
    char temp_ifname[MAX_CMD_SIZE];
    int result = 0;

    for (i = 0; i < numArgs; ++i ) {
        strncpy(temp_ifname, (char*)args[i].portID, sizeof(temp_ifname));

        /* Handle hotspot bridges differently than generic GRE */
        if (!strcmp("gretap0", temp_ifname))
        {
            return linuxIfConfigVlan(args, numArgs, up);
        }

        /* Connect the gre interface to the bridge */
        if(0 != (result = portHelper(args[0].hints.network->name, temp_ifname, args[i].vidParams.tagging, args[i].vidParams.vid, up)))
        {
            MNET_DEBUG("%s: portHelper failed!\n" COMMA __FUNCTION__);
        }

        /* Handle the iptables rule for TCP MSS clamping */
        if (0 == result) {
            /* Generate the MTU clamping rule */
            snprintf(ruleBuff, MAX_CMD_SIZE, " -A POSTROUTING -o %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %d", 
                     args[0].hints.network->name, GRE_CLAMP_MTU);

            /* Add or delete the rule */
            snprintf(cmdBuff, MAX_CMD_SIZE, "sysevent %s GeneralPurposeMangleRule \"%s\"", 
                     (up ? "setunique" : "delunique"), ruleBuff);

            MNET_DEBUG("configVlan_GRE command is %s\n" COMMA cmdBuff);
            system(cmdBuff);

            /* Restart firewall so rule takes effect */
            MNET_DEBUG("configVlan_GRE restarting firewall\n");
            system("sysevent set firewall-restart");
        }
        
    }

    return result;
}

/* Function with external switch specific handling for adding a port to the bridge */
int configVlan_ESW(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    PSwPortState portState;
    char temp_ifname[MAX_CMD_SIZE];
    int result = 0;

    memset(temp_ifname, 0, MAX_CMD_SIZE);
   
    for (i = 0; i < numArgs; ++i ) {
        portState = (PSwPortState) args[i].portID;
        stringIDExtSw(portState, temp_ifname, sizeof(temp_ifname));

        if(0 != (result = portHelper(args[0].hints.network->name, temp_ifname, args[i].vidParams.tagging, args[i].vidParams.vid, up)))
        {
            MNET_DEBUG("%s: portHelper failed!\n" COMMA __FUNCTION__);
        }
    }

    return result;
}

/* Function generic interface handling for adding a port to the bridge */
int configVlan_puma7(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    char cmdBuff[MAX_CMD_SIZE];
    char temp_ifname[MAX_CMD_SIZE];
    int result = 0;
   
    for (i = 0; i < numArgs; ++i ) {
        strncpy(temp_ifname, (char*)args[i].portID, sizeof(temp_ifname));

        if(0 != (result = portHelper(args[0].hints.network->name, temp_ifname, args[i].vidParams.tagging, args[i].vidParams.vid, up)))
        {
            MNET_DEBUG("%s: portHelper failed!\n" COMMA __FUNCTION__);
        }
    }

    return result;
}

/* Function with wifi-specific handling for adding a port to the bridge */
int configVlan_WiFi(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    char cmdBuff[MAX_CMD_SIZE];
    char temp_ifname[MAX_CMD_SIZE];
    int result;

    for (i = 0; i < numArgs; ++i ) 
	{ 
        strncpy(temp_ifname, (char*)args[i].portID, sizeof(temp_ifname));

        if (up)
        {
            //Form command to add wifi
            if (args[i].vidParams.tagging) {
                sprintf(cmdBuff, "%s/%s create_vap %s %s %d", 
                        SERVICE_D_BASE_DIR, 
                        PUMA7_WIFI_UTIL, 
                        temp_ifname, 
                        args[0].hints.network->name,
                        args[i].vidParams.vid
                        );
            }
            else
            {
                sprintf(cmdBuff, "%s/%s create_vap %s %s", 
                        SERVICE_D_BASE_DIR, 
                        PUMA7_WIFI_UTIL, 
                        temp_ifname, 
                        args[0].hints.network->name
                        );
            }
            //Run command here
            MNET_DEBUG("%s: command is %s\n" COMMA __FUNCTION__ COMMA cmdBuff);
            result = system_wrapper(cmdBuff);
        }
        else
        {
            //Form command to delete wifi
                sprintf(cmdBuff, "%s/%s delete_vap %s", 
                        SERVICE_D_BASE_DIR, 
                        PUMA7_WIFI_UTIL, 
                        temp_ifname
                        );            
            //Run command here
            MNET_DEBUG("%s: command is %s\n" COMMA __FUNCTION__ COMMA cmdBuff);
            result = system_wrapper(cmdBuff);
        }
        
    }

    return result;
}

/* Get event ID for external switch port */
int eventIDSw (void* portID, char* stringbuf, int bufSize) {
    PSwPortState portState = (PSwPortState) portID;
    return (eventIDFromStringPortID((void*)portState->stringID, stringbuf, bufSize));
}

/* Get port name for an external switch port */
int stringIDExtSw (void* portID, char* stringbuf, int bufSize) {
    PSwPortState portState = (PSwPortState) portID;
    int retval = snprintf(stringbuf, bufSize, "%s", portState->stringID);
    
    return retval ? retval + 1 : 0;
}
