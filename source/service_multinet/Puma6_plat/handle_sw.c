/************************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**************************************************************************/

#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7)
#include "sysevent/sysevent.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "handle_sw.h"
#include "swctl_hal.h"
#include "service_multinet_base.h"
#include "errno.h"
#include "safec_lib_common.h"
#include "secure_wrapper.h"

#define MOCACTL 	"/usr/sbin/mocactl"
#define IPC_VLAN	500	
#define RADIUS_VLAN	4090	
#define MESHBHAUL_VLAN	1060	

int hdl_sw_sysevent_fd;
token_t hdl_sw_sysevent_token;

void check_for_dependent_ports(char *port, int *tag, int *atom_port, int *ext_port)
{
	if(!strncmp(port, "ath", 3) || (!strncmp(port, "sw_6", 4)))
	{
		*atom_port = 1;
		*ext_port = 0;
		*tag = TAGGING_MODE;
	}
	else if ((!strncmp(port, "sw_1", 4)) || (!strncmp(port, "sw_2", 4)) || 
			 (!strncmp(port, "sw_3", 4)) || (!strncmp(port, "sw_4", 4)))
	{
		*atom_port = 0;
		*ext_port = 1;
	}
	else if (!strncmp(port, "sw_5", 4))
	{
		*atom_port = 0;
		*ext_port = 0;
	}
	else
	{
		printf("It shouldnt come here\n");
	}
}

// This function removes all the occurances of "to_remove_mem" from "from_member"
void sw_remove_member(char *from_member, char *to_remove_mem)
{
    int l_iIter = 0, l_iIndex = 0;
    char l_cTemp[64] = {0};

	if (0 == from_member[0] || 0 == to_remove_mem[0])
    {   
        printf("One of the inputs is not correct not removing anything\n");    
        return;
    }   
    while(from_member[l_iIter] !='\0') 
    {   
        if (!strncmp(&from_member[l_iIter], to_remove_mem, strlen(to_remove_mem)))
        {   
            l_iIter += strlen(to_remove_mem);
        }   
        l_cTemp[l_iIndex] = from_member[l_iIter]; 
        l_iIter++;
        l_iIndex++;
   	}   
   	l_cTemp[l_iIndex]='\0'; 
   	strncpy(from_member, l_cTemp, (strlen(l_cTemp)+1));  
}

//handle_moca is a function for configuring MOCA port
void handle_moca(int vlan_id, int *tagged, int add)
{
	char l_cMoca_Tports[16] = {0}, l_cMoca_Utport[8] = {0}, l_cMoca_Tport[8] = {0};
	int l_iMoca_UtPort = 100;
	errno_t  rc  = -1;

	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_tports", l_cMoca_Tports, sizeof(l_cMoca_Tports));
	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_utport", l_cMoca_Utport, sizeof(l_cMoca_Utport));
	
	if (1 == add) //Add MOCA ports case
	{
	    // adding a vlan to moca
		if (TAGGING_MODE == *tagged)
		{
    		// Adding trunking vlan
			if (0 == l_cMoca_Tports[0])
			{
    	    	// Need to enable vlans and convert untagged vlan if it exists
        	    MNET_DEBUG("--SW handler swctl %s\n" COMMA PORTMAP_VENABLE_sw_5);
				swctl(20, 3, -1, -1, -1, -1, NULL, NULL);
    	
		        // Not calling add_untagged_moca_vid_special $MUTPORT as mocactl is not there
				if (0 == l_cMoca_Utport[0]) 
				{
					printf("sw_moca_utport is empty using the default value 100\n");
				}
				else
				{
					l_iMoca_UtPort = atoi(l_cMoca_Utport);
				}
				if (access(MOCACTL, F_OK) == 0)
	            {   
    	            MNET_DEBUG("--SW handler mocactl -c 8 -e 1 -v %d \n" 
						       COMMA l_iMoca_UtPort);

            	    v_secure_system("mocactl -c 8 -e 1 -v %d", l_iMoca_UtPort);
	            }   
    	        else
        	    {   
            	    printf("mocactl is not present error:%d, not adding moca vlanid\n", errno);
	            }   
    	        MNET_DEBUG("--SW handler swctl %s -v %d -m %d -q 1\n" 
						   COMMA PORTMAP_sw_5 COMMA l_iMoca_UtPort COMMA NATIVE_MODE);

        	    swctl(16, 3, l_iMoca_UtPort, NATIVE_MODE, 1, -1, NULL, NULL);
    
            	strncat(l_cMoca_Tports, l_cMoca_Utport, (sizeof(l_cMoca_Tports) - sizeof(l_cMoca_Utport)));
	            sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, 
							 "sw_moca_tports", l_cMoca_Tports, 0);
			}
			else
			{
				printf("sw_moca_tports already has a value not doing anything \n");
			}
		}
		else
		{
    		// Adding untagged vlan
			if (0 == l_cMoca_Tports[0])
			{
        		// Add to moca filter, and add TAG
	            // Not calling add_untagged_moca_vid_special $VID as mocactl is not there
				if (0 == l_cMoca_Utport[0]) 
        	    {
            	    printf("sw_moca_utport is empty using the default value 100\n");
	            }
    	        else
        	    {
            	    l_iMoca_UtPort = atoi(l_cMoca_Utport);
	            }

				if (access(MOCACTL, F_OK) == 0)
				{
					MNET_DEBUG("--SW handler mocactl -c 8 -e 1 -v %d \n" COMMA vlan_id);
					v_secure_system("mocactl -c 8 -e 1 -v %d", vlan_id);
				}
				else
				{
					printf("mocactl is not present error:%d, not adding moca vlanid\n", errno);
				}
    	        MNET_DEBUG("--SW handler swctl %s -v %d -m %d -q 1\n" 
							COMMA PORTMAP_sw_5 COMMA l_iMoca_UtPort COMMA NATIVE_MODE);

				swctl(16, 3, l_iMoca_UtPort, NATIVE_MODE, 1, -1, NULL, NULL);
			}
			else
			{
				printf("sw_moca_tports already has a value not doing anything \n");
			}
			rc = sprintf_s(l_cMoca_Utport, sizeof(l_cMoca_Utport), "%d", vlan_id);
			if(rc < EOK)
			{
				ERR_CHK(rc);
			}
			sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_utport", l_cMoca_Utport, 0);
		}
	}
	else //Delete MOCA port case
	{
		// Removing a vlan from moca
		if (TAGGING_MODE == *tagged)
		{
            // Removing trunking vlan
            rc = sprintf_s(l_cMoca_Tport, sizeof(l_cMoca_Tport), "%d", vlan_id);
            if(rc < EOK)
            {
				ERR_CHK(rc);
            }
			sw_remove_member(l_cMoca_Tports, l_cMoca_Tport);
			if (0 == l_cMoca_Tports[0])
			{
                // Need to disable vlans and re-add untagged vlan if it exists
                MNET_DEBUG("--SW handler swctl %s\n" COMMA PORTMAP_VDISABLE_sw_5)
				swctl(21, 3, -1, -1, -1, -1, NULL, NULL);

				l_iMoca_UtPort = atoi(l_cMoca_Utport);
				if (access(MOCACTL, F_OK) == 0)
                {   
                    MNET_DEBUG("--SW handler mocactl -c 8 -e 0 -v %d \n" 
							   COMMA l_iMoca_UtPort);
                    v_secure_system("mocactl -c 8 -e 1 -v %d", l_iMoca_UtPort);
                }   
                else
                {   
                    printf("mocactl is not present error:%d, not deleting moca vlanid\n", errno);
                }
				MNET_DEBUG("--SW handler swctl %s -v %d -m %d -q 1\n" 
							COMMA PORTMAP_sw_5 COMMA l_iMoca_UtPort COMMA UNTAGGED_MODE)
				swctl(16, 3, l_iMoca_UtPort, UNTAGGED_MODE, 1, -1, NULL, NULL);
            }
			sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_tports", l_cMoca_Tports, 0);
		}
        else
		{
        	// Removing untagged vlan
			if (0 != l_cMoca_Tports[0])
			{
                // Del from moca filter, and add TAG
				*tagged = TAGGING_MODE;
				if (access(MOCACTL, F_OK) == 0)
                {
                    MNET_DEBUG("--SW handler mocactl -c 8 -e 0 -v %d \n" COMMA vlan_id);
                    v_secure_system("mocactl -c 8 -e 0 -v %d", vlan_id);
                }
                else
                {
                    printf("mocactl is not present error:%d, not deleting moca vlanid\n", errno);
                }
            }

			l_iMoca_UtPort = atoi(l_cMoca_Utport);
			if (l_iMoca_UtPort == vlan_id)
			{
				sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_utport", "", 0);
            }
        }	
	}
}

void execSwCtl(char *port, int vlan_id, int tagged, int add)
{
	int command = 0, command_ven = 4, command_def = 34, l_iCmd_Rem = 1; 
	int port_venable = 0, l_iPort;
	char val[20] = {0}, sysevent_cmd[50] = {0};

	if (!strncmp("sw_1", port, 4))
		l_iPort = 0;
    else if (!strncmp("sw_2", port, 4))
		l_iPort = 1;
    else if (!strncmp("sw_3", port, 4))
		l_iPort = 2;
    else if (!strncmp("sw_4", port, 4))
		l_iPort = 3;
    else if (!strncmp("sw_5", port, 4))
    {
		handle_moca(vlan_id, &tagged, add ? ADD: DELETE); //sw_5 is a MOCA port
		command = 16;
		command_def = 16;
		command_ven = 20;
		l_iPort = 3;
		l_iCmd_Rem = 17;
    }
    else
    {
        printf("It should not come here for port:%s\n", port);
		return;
    }
	if (ADD == add) //Addition case
	{
		snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_port_%s_venable",port);
    	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, val, sizeof(val));
	    port_venable = atoi(val);

		if (1 != port_venable)
		{
			MNET_DEBUG("--SW handler swctl -c %d -p %d\n" COMMA command_ven COMMA l_iPort)
			swctl(command_ven, l_iPort, -1, -1, -1, -1, NULL, NULL);

	        sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, "1", 0);
		}

	    MNET_DEBUG("--SW handler swctl -c %d -p %d -v %d -m %d -q 1 \n" 
				   COMMA command COMMA l_iPort COMMA vlan_id COMMA tagged)

		//e.g: swctl -c 0 -p 0 -v 100 -m 1 -q 1
		swctl(command, l_iPort, vlan_id, tagged, 1, -1, NULL, NULL);

	    if (UNTAGGED_MODE == tagged && (strncmp("sw_5", port, 4)))
    	{   
	        MNET_DEBUG("--SW handler swctl -c %d -p %d -v %d\n" 
					   COMMA command_def COMMA l_iPort COMMA vlan_id)

			swctl(command_def, l_iPort, vlan_id, -1, -1, -1, NULL, NULL); //e.g swctl -c 34 -p 0 -v 100 
	    }	
	}
	else //Delete case
	{
    	MNET_DEBUG("--SW handler, swctl -c %d -p %d -v %d\n" 
					COMMA l_iCmd_Rem COMMA l_iPort COMMA vlan_id)

		swctl(l_iCmd_Rem, l_iPort, vlan_id, -1, -1, -1, NULL, NULL);
           
		if (UNTAGGED_MODE == tagged) 
		{
			if (!strncmp("sw_5", port, 4))
			{
				MNET_DEBUG("--SW handler, swctl -c %d -p %d -v %d -m %d -q 1\n" COMMA command_def 
							COMMA l_iPort COMMA 0 COMMA NATIVE_MODE)

				swctl(command_def, l_iPort, 0, 0, 1, -1, NULL, NULL);
			}
			else
			{		
				MNET_DEBUG("--SW handler, swctl -c %d -p %d -v %d\n" 
						   COMMA command_def COMMA l_iPort COMMA 0)

				swctl(command_def, l_iPort, 0, -1, -1, -1, NULL, NULL);
			}
		}
	}
}

void sw_add_ports(int vlan_id, char* ports_add, int *atom_port, int *ext_port)
{
	char port[10] = {0};
	int i = 0, tagged = UNTAGGED_MODE;

	while(*ports_add != '\0')
	{
		if(*ports_add == ' ' || *(ports_add+1) == '\0')
		{
			if (*(ports_add+1) == '\0')
            {
				port[i] = *(ports_add);
				port[i + 1] = '\0';
            }

			//check for dependent ports
			check_for_dependent_ports(port, &tagged, atom_port, ext_port);
			if (1 == *atom_port)
			{
				goto reset;
			}

			execSwCtl(port, vlan_id, tagged, ADD);
			
			reset:
			memset(port, 0x00, sizeof(port));	
			tagged = UNTAGGED_MODE;
			i = 0;
		}
		else
		{
			if (*ports_add == '-' && *(ports_add+1) == 't')
			{
				tagged = TAGGING_MODE;
			}
			else
				port[i] = *(ports_add);
			i++;
		}
		ports_add++;
	}
}

void handle_sw_init()
{
	hdl_sw_sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, 
									   SE_VERSION, "handle_sw", &hdl_sw_sysevent_token);	

	if (hdl_sw_sysevent_fd < 0) 
    {    
        printf("sysevent_open failed\n");
    } 
	else 
	{
        printf("sysevent_open success\n");
    }
}

void addVlan(int net_id, int vlan_id, char *ports_add)
{
	char val[20] = {0}, sysevent_cmd[50] = {0};
	/*char cmdBuff[80] = {0};*/
	char l_cVlan_Id[8] = {0};
	char vidPorts[50] = {0}, ext_vidPorts[50] = {0}, atom_vidPorts[50] = {0}, l_cExt_Vids[16] = {0};
	int arm_venable = 0, i2e_venable = 0, e2i_venable = 0;
	int atom_venable = 0, atom_port = 0, ext_port = 0;
	int l_iLen;
	errno_t   rc = -1;

	if (0 == hdl_sw_sysevent_fd)
		handle_sw_init();

    printf("--SW handler, adding vlan %d on net %d for ports:%s\n", vlan_id, net_id, ports_add);
	
	snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_ports",vlan_id);
	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, vidPorts, sizeof(vidPorts));

	snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_extports",vlan_id);
	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, ext_vidPorts, sizeof(ext_vidPorts));

	snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_atomports",vlan_id);
	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, atom_vidPorts, sizeof(atom_vidPorts));

	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_arm_venable", val, sizeof(val));
	arm_venable = atoi(val);

	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_atom_venable", val, sizeof(val));
	atom_venable = atoi(val);

	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_I2E_venable", val, sizeof(val));
	i2e_venable = atoi(val);

	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_E2I_venable", val, sizeof(val));
	e2i_venable = atoi(val);

	if (1 != arm_venable)
	{
		MNET_DEBUG("--SW handler, swctl %s\n" COMMA PORTMAP_VENABLE_arm)
		swctl(20, 7, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 7
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_arm_venable", "1", 0);
	}

	if (!vidPorts[0])
	{
/*	    snprintf(cmdBuff, sizeof(cmdBuff), "vconfig add %s %d; ifconfig %s.%d up",
				 MGMT_PORT_LINUX_IFNAME, vlan_id, MGMT_PORT_LINUX_IFNAME, vlan_id);
	    MNET_DEBUG("Creating vlan:%d for l2sm0 interface command is:%s\n" 
					COMMA vlan_id COMMA cmdBuff)
	    system(cmdBuff);    */

	    MNET_DEBUG("--SW handler, swctl %s -v %d -m %d -q 1\n" 
					COMMA PORTMAP_arm COMMA vlan_id COMMA TAGGING_MODE)

		//swctl -c 16 -p 7 -v <vlan_id> -m 2 -q 1
	    swctl(16, 7, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); 
	}
    sw_add_ports(vlan_id, ports_add, &atom_port, &ext_port);
	
	l_iLen = strlen(vidPorts);		
	if (0 != vidPorts[0] && vidPorts[l_iLen] != ' ')
		strncat(vidPorts, " ", strlen(vidPorts) - 1);

	strncat(vidPorts, ports_add, (size_t)(strlen(ports_add)));
	snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_ports",vlan_id);
	sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, vidPorts, 0);

	if (1 == ext_port)
	{
		if (1 != i2e_venable)
		{
			MNET_DEBUG("--SW handler, swctl %s\n" COMMA PORTMAP_VENABLE_I2E)
			swctl(20, 2, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 2

		    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, 
						 "sw_port_I2E_venable", "1", 0); 
		}
		if (1 != e2i_venable)	
		{
			MNET_DEBUG("--SW handler, swctl %s\n" COMMA PORTMAP_VENABLE_E2I)
			swctl(4, 5, -1, -1, -1, -1, NULL, NULL); //swctl -c 4 -p 5
					
		    printf("sysevent set PORTMAP_VENABLE_E2I 1\n");
		    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, 
						 "sw_port_E2I_venable", "1", 0); 
		}
		if (!ext_vidPorts[0])
		{
			MNET_DEBUG("--SW handler, swctl %s -v %d -m %d -q 1\n" 
						COMMA PORTMAP_I2E COMMA vlan_id COMMA TAGGING_MODE)

			swctl(16, 2, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 16 -p 2 -v <vlan_id> -m 2 -q 1
			MNET_DEBUG("--SW handler, swctl %s -v %d -m %d -q 1\n" 
						COMMA PORTMAP_E2I COMMA vlan_id COMMA TAGGING_MODE)

			swctl(0, 5, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 0 -p 5 -v <vlan_id> -m 2 -q 1

			sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, 
						 "sw_ext_vids", l_cExt_Vids, sizeof(l_cExt_Vids));
			l_iLen = strlen(l_cExt_Vids);    
		    if (0 != l_cExt_Vids[0] && l_cExt_Vids[l_iLen] != ' ')
		    {
        		strncat(l_cExt_Vids, " ", strlen(l_cExt_Vids) - 1);
		    }
			
			rc = sprintf_s(l_cVlan_Id, sizeof(l_cVlan_Id), "%d", vlan_id);
			if(rc < EOK)
			{
				ERR_CHK(rc);
			}
			strncat(l_cExt_Vids, l_cVlan_Id, (size_t)(strlen(l_cVlan_Id)));
			printf("sysevent set sw_ext_vids %s\n", l_cExt_Vids);
			sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, 
						 "sw_ext_vids", l_cExt_Vids, 0);
		}
        l_iLen = strlen(ext_vidPorts);
		if (0 != ext_vidPorts[0] && ext_vidPorts[l_iLen] != ' ')
        	strncat(ext_vidPorts, " ", strlen(ext_vidPorts) - 1);

		snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_extports",vlan_id);
		strncat(ext_vidPorts, ports_add, (size_t)(strlen(ports_add)));
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, 
				  	 sysevent_cmd, ext_vidPorts, 0);
	}

	if (1 == atom_port)
	{
		if (1 != atom_venable)
		{
			MNET_DEBUG("--SW handler, swctl %s\n" COMMA PORTMAP_VENABLE_atom)
			swctl(20, 0, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 0
			sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_atom_venable", "1", 0);
		}
		if (!atom_vidPorts[0])
		{
			MNET_DEBUG("--SW handler, swctl %s -v %d -m %d -q 1\n" COMMA PORTMAP_atom COMMA vlan_id COMMA TAGGING_MODE)
			swctl(16, 0, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 16 -p 0 -v <vlan_id> -m 2 -q 1
		}
		l_iLen = strlen(atom_vidPorts);
        if (0 != atom_vidPorts[0] && atom_vidPorts[l_iLen] != ' ')
            strncat(atom_vidPorts, " ", strlen(atom_vidPorts) - 1);

		snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_atomports",vlan_id);
		strncat(atom_vidPorts, ports_add, (size_t)(strlen(ports_add)));
		printf("sysevent set sw_vid_%d_atomports %s\n", vlan_id, atom_vidPorts);
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, atom_vidPorts, 0);
	}
}

void delVlan(int net_id, int vlan_id, char *ports_add)
{
	char l_cVid_Ports[64] = {0}, l_cExt_Vid_Ports[64] = {0};
	char l_cAtom_Vid_Ports[32] = {0}, l_cExt_Vids[8] = {0};
	char l_cCmd_Buff[128] = {0}, l_cVlan_Id[8] = {0}, l_cPort[8] = {0};
	errno_t   rc  = -1;
	
	int l_iAtom_Port, l_iExt_Port, l_iIter = 0, l_iTagged = UNTAGGED_MODE;

    printf("--SW handler, removing vlan %d on net %d for %s\n", vlan_id, net_id, ports_add);
	if (0 == hdl_sw_sysevent_fd)
		handle_sw_init();

	snprintf(l_cCmd_Buff, sizeof(l_cCmd_Buff), "sw_vid_%d_ports",vlan_id);
    sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, l_cCmd_Buff, l_cVid_Ports, sizeof(l_cVid_Ports));

    snprintf(l_cCmd_Buff, sizeof(l_cCmd_Buff), "sw_vid_%d_extports",vlan_id);
    sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, l_cCmd_Buff, l_cExt_Vid_Ports, sizeof(l_cExt_Vid_Ports));

    snprintf(l_cCmd_Buff, sizeof(l_cCmd_Buff), "sw_vid_%d_atomports",vlan_id);
    sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, l_cCmd_Buff, l_cAtom_Vid_Ports, sizeof(l_cAtom_Vid_Ports));
        
	while(*ports_add != '\0')
    {
        if(*ports_add == ' ' || *(ports_add+1) == '\0')
        {
			if (*(ports_add+1) == '\0')
            {
                l_cPort[l_iIter] = *(ports_add);
                l_cPort[l_iIter + 1] = '\0';
            }

            //check for dependent ports
            check_for_dependent_ports(l_cPort, &l_iTagged, &l_iAtom_Port, &l_iExt_Port);
			sw_remove_member(l_cVid_Ports, l_cPort);
            if (1 == l_iAtom_Port)
				sw_remove_member(l_cAtom_Vid_Ports, l_cPort);	
			else if (1 == l_iExt_Port)
				sw_remove_member(l_cExt_Vid_Ports, l_cPort);	
			else if (!strncmp(l_cPort, "sw_5", 4))
			{
                printf("Removing MOCA port:%s\n", l_cPort);
			}
			else
			{
				printf("It shouldnt come here for port:%s\n", l_cPort);
				return;
			}

            execSwCtl(l_cPort, vlan_id, l_iTagged, DELETE);
            memset(l_cPort, 0x00, sizeof(l_cPort));
            l_iTagged = UNTAGGED_MODE;
            l_iIter = 0;
        }
        else
        {
            if (*ports_add == '-' && *(ports_add+1) == 't')
            {
                l_iTagged = TAGGING_MODE;
            }
            else
                l_cPort[l_iIter] = *(ports_add);
            l_iIter++;
        }
        ports_add++;
    }
	// check for arm port removal (implicit rule)
	if (0 == l_cVid_Ports[0])
	{
    	MNET_DEBUG("--SW handler, swctl %s -v %d\n" COMMA PORTMAP_REM_arm COMMA vlan_id)
		swctl(17, 7, vlan_id, -1, -1, -1, NULL, NULL);
/*
		snprintf(l_cCmd_Buff, sizeof(l_cCmd_Buff), "vconfig rem %s.%d", MGMT_PORT_LINUX_IFNAME, vlan_id);
	    MNET_DEBUG("Removing vlan %s.%d\n" COMMA MGMT_PORT_LINUX_IFNAME COMMA vlan_id)
		system(l_cCmd_Buff);*/
    }
	snprintf(l_cCmd_Buff, sizeof(l_cCmd_Buff), "sw_vid_%d_ports",vlan_id);
    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, l_cCmd_Buff, l_cVid_Ports, 0);
        
    // Add to switch connection ports if on external switch
    if (0 == l_cExt_Vid_Ports[0])
	{
    	MNET_DEBUG("--SW handler, swctl %s -v %d" COMMA PORTMAP_REM_I2E COMMA vlan_id)
		swctl(17, 2, vlan_id, -1, -1, -1, NULL, NULL);

		MNET_DEBUG("--SW handler, swctl %s -v %d" COMMA PORTMAP_REM_E2I COMMA vlan_id)
		swctl(1, 5, vlan_id, -1, -1, -1, NULL, NULL);
            
    	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_ext_vids", l_cExt_Vids, sizeof(l_cExt_Vids));
		rc = sprintf_s(l_cVlan_Id, sizeof(l_cVlan_Id), "%d", vlan_id);
		if(rc < EOK)
		{
			ERR_CHK(rc);
		}
		sw_remove_member(l_cExt_Vids, l_cVlan_Id);
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_ext_vids", l_cExt_Vids, 0);
    }
    // Save list of members
	snprintf(l_cCmd_Buff, sizeof(l_cCmd_Buff), "sw_vid_%d_extports",vlan_id);
    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, l_cCmd_Buff, l_cExt_Vid_Ports, 0);
        
    // check for atom port removal (implicit rule)
	if (0 == l_cAtom_Vid_Ports[0])
	{
		MNET_DEBUG("--SW handler, swctl %s -v %d\n" COMMA PORTMAP_REM_atom COMMA vlan_id)
		swctl(17, 0, vlan_id, -1, -1, -1, NULL, NULL);
    }
	snprintf(l_cCmd_Buff, sizeof(l_cCmd_Buff), "sw_vid_%d_atomports",vlan_id);
    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, l_cCmd_Buff, l_cAtom_Vid_Ports, 0);
}

int restore_ext_sw(char* argv[], int argc)
{
	printf("Inside restore_ext_sw function\n");
	return 0;
}

void setMulticastMac()
{
    //Following commands are used to set Multicast MAC Address
    //swctl -c 23 -p 2 -s 01:00:5E:7F:FF:FA
    //swctl -c 23 -p 3 -s 01:00:5E:7F:FF:FA
    //swctl -c 23 -p 7 -s 01:00:5E:7F:FF:FA
    MNET_DEBUG("--SW handler, swctl -c 23 -p 2 -s 01:00:5E:7F:FF:FA\n")
    swctl(23, 2, -1, -1, -1, -1, "01:00:5E:7F:FF:FA", NULL);

    MNET_DEBUG("--SW handler, swctl -c 23 -p 3 -s 01:00:5E:7F:FF:FA\n")
    swctl(23, 3, -1, -1, -1, -1, "01:00:5E:7F:FF:FA", NULL);

    MNET_DEBUG("--SW handler, swctl -c 23 -p 7 -s 01:00:5E:7F:FF:FA\n")
    swctl(23, 7, -1, -1, -1, -1, "01:00:5E:7F:FF:FA", NULL);
}

void addIpcVlan()
{
	addVlan(0, IPC_VLAN, "sw_6");
}

void addRadiusVlan()
{
	addVlan(0, RADIUS_VLAN, "sw_6");
}

// RDKB-15951 
void addMeshBhaulVlan()
{
	addVlan(0, MESHBHAUL_VLAN, "sw_6");
}

void createMeshVlan()
{
	
	swctl(16, 0, 112, TAGGING_MODE, 1, -1, NULL, NULL);
	swctl(16, 7, 112, TAGGING_MODE, 1, -1, NULL, NULL);
        v_secure_system("vconfig add l2sd0 112; ifconfig l2sd0.112 169.254.0.254 netmask 255.255.255.0 up");
	
	swctl(16, 0, 113, TAGGING_MODE, 1, -1, NULL, NULL);
	swctl(16, 7, 113, TAGGING_MODE, 1, -1, NULL, NULL);
    v_secure_system("vconfig add l2sd0 113; ifconfig l2sd0.113 169.254.1.254 netmask 255.255.255.0 up");
}
#endif
