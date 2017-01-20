#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7)
#include "sysevent/sysevent.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "handle_sw.h"
#include "swctl_hal.h"
#include "service_multinet_base.h"

int hdl_sw_sysevent_fd;
token_t hdl_sw_sysevent_token;

void check_for_dependent_ports(char *port, int *tag, int *atom_port, int *ext_port)
{
	printf("port received is:%s\n", port);
	if(!strncmp(port, "ath", 3) || (!strncmp(port, "sw_6", 4)))
	{
		printf("Atom ports should be added\n");
		*atom_port = 1;
		*ext_port = 0;
		*tag = TAGGING_MODE;
	}
	else if ((!strncmp(port, "sw_1", 4)) || (!strncmp(port, "sw_2", 4)) || 
			 (!strncmp(port, "sw_3", 4)) || (!strncmp(port, "sw_4", 4)))
	{
		printf("external ports should be added\n");
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

//handle_moca is a function for configuring MOCA port
void handle_moca(int vlan_id, int tagged)
{
	char l_cMoca_Tports[16] = {0}, l_cMoca_Utport[8] = {0};
	int l_iMoca_UtPort = 100;
	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_tports", l_cMoca_Tports, sizeof(l_cMoca_Tports));
	sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_utport", l_cMoca_Utport, sizeof(l_cMoca_Utport));
	
    printf("sw_moca_tports:%s sw_moca_utport:%s\n", l_cMoca_Tports, l_cMoca_Utport);
    // adding a vlan to moca
	if (TAGGING_MODE == tagged)
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
            MNET_DEBUG("--SW handler swctl %s -v %d -m %d -q 1\n" COMMA PORTMAP_sw_5 COMMA l_iMoca_UtPort COMMA NATIVE_MODE);
			swctl(16, 3, l_iMoca_UtPort, NATIVE_MODE, 1, -1, NULL, NULL);
		
			strncat(l_cMoca_Tports, l_cMoca_Utport, sizeof(l_cMoca_Utport));	
        	sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_tports", l_cMoca_Tports, 0);
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
            MNET_DEBUG("--SW handler swctl %s -v %d -m %d -q 1\n" COMMA PORTMAP_sw_5 COMMA l_iMoca_UtPort COMMA NATIVE_MODE);
			swctl(16, 3, l_iMoca_UtPort, NATIVE_MODE, 1, -1, NULL, NULL);
		}
		else
		{
			printf("sw_moca_tports already has a value not doing anything \n");
		}
		sprintf(l_cMoca_Utport, "%d", vlan_id);
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_moca_utport", l_cMoca_Utport, 0);
	}
}

void execSwCtl(char *port, int vlan_id, int tagged)
{
	printf("Inside execSwCtl function\n");
	int port_venable = 0, command = 0, command_ven, command_def, l_iPort;
	
	char val[20];
	char sysevent_cmd[50];
    memset(val, 0x00, sizeof(val));
    memset(sysevent_cmd, 0x00, sizeof(sysevent_cmd));

	snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_port_%s_venable",port);
    sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, val, sizeof(val));
    port_venable = atoi(val);

    printf("execSwCtl::port_venable for port:%s is:%d\n", port, port_venable);

	if (!strncmp("sw_1", port, 4))
    {
        printf("sw_1 case use PORTMAP_sw_1\n");
		command_ven = 4;
		command_def = 34;
		l_iPort = 0;
    }
    else if (!strncmp("sw_2", port, 4))
    {
        printf("sw_2 case use PORTMAP_sw_2\n");
		command_ven = 4;
		command_def = 34;
		l_iPort = 1;
    }
    else if (!strncmp("sw_3", port, 4))
    {
        printf("sw_3 case use PORTMAP_sw_3\n");
		command_ven = 4;
		command_def = 34;
		l_iPort = 2;
    }
    else if (!strncmp("sw_4", port, 4))
    {
        printf("sw_4 case use PORTMAP_sw_4\n");
		command_ven = 4;
		command_def = 34;
		l_iPort = 3;
    }
    else if (!strncmp("sw_5", port, 4))
    {
        printf("sw_5 case use PORTMAP_sw_5\n");
		handle_moca(vlan_id, tagged); //sw_5 is a MOCA port
		command = 16;
		command_def = 16;
		command_ven = 20;
		l_iPort = 3;
    }
    else
    {
        printf("It should not come here\n");
    }

	if (1 != port_venable)
	{
		MNET_DEBUG("--SW handler swctl -c %d -p %d\n" COMMA command_ven COMMA l_iPort)
		swctl(command_ven, l_iPort, -1, -1, -1, -1, NULL, NULL);

	    printf("sysevent set %s 1\n", sysevent_cmd);
        sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, "1", 0);
	}

    MNET_DEBUG("--SW handler swctl -c %d -p %d -v %d -m %d -q 1 \n" COMMA command COMMA l_iPort COMMA vlan_id COMMA tagged)
	swctl(command, l_iPort, vlan_id, tagged, 1, -1, NULL, NULL); //e.g: swctl -c 0 -p 0 -v 100 -m 1 -q 1

    if (UNTAGGED_MODE == tagged && (strncmp("sw_5", port, 4)))
    {   
    	printf("Untagged case execute swctl for port:%s\n", port);
        MNET_DEBUG("--SW handler swctl -c %d -p %d -v %d\n" COMMA command_def COMMA l_iPort COMMA vlan_id)
		swctl(command_def, l_iPort, vlan_id, -1, -1, -1, NULL, NULL); //e.g swctl -c 34 -p 0 -v 100 
    }	
}

void sw_add_ports(int vlan_id, char* ports_add, int *atom_port, int *ext_port)
{
	printf("Inside sw_add_ports function ports_add is:%s\n", ports_add);
	char port[10];
	char cmdBuff[80];	
	char sysevent_cmd[50];
	char val[20];

	memset(port, 0x00, sizeof(port));	
	memset(cmdBuff, 0x00, sizeof(cmdBuff));	
	memset(sysevent_cmd, 0x00, sizeof(sysevent_cmd));	
	memset(val, 0x00, sizeof(val));	

	int i = 0, port_venable = 0;
	int tagged = UNTAGGED_MODE;
	
	while(*ports_add != '\0')
	{
		if(*ports_add == ' ' || *(ports_add+1) == '\0')
		{
			if (*(ports_add+1) == '\0')
				port[i] = *(ports_add);

			//check for dependent ports
			check_for_dependent_ports(port, &tagged, atom_port, ext_port);
			if (1 == *atom_port)
			{
				printf("Dont do anything continue\n");
				goto reset;
			}

			execSwCtl(port, vlan_id, tagged);
			
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
	hdl_sw_sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "handle_sw", &hdl_sw_sysevent_token);	

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
	char cmdBuff[80] = {0}, l_cVlan_Id[8] = {0};
	char vidPorts[50] = {0}, ext_vidPorts[50] = {0}, atom_vidPorts[50] = {0}, l_cExt_Vids[16] = {0};
	int arm_venable = 0, i2e_venable = 0, e2i_venable = 0;
	int atom_venable = 0, atom_port = 0, ext_port = 0;
	int l_iLen;

	printf("Inside addVlan function\n");
	if (0 == hdl_sw_sysevent_fd)
	{
		printf("hdl_sw_sysevent_fd is zero call handle_sw_init\n");
		handle_sw_init();
	}

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

	printf("arm_venable:%d atom_venable:%d i2e_venable:%d e2i_venable:%d\n", arm_venable, atom_venable, i2e_venable, e2i_venable);
	if (1 != arm_venable)
	{
		MNET_DEBUG("--SW handler, swctl %s\n" COMMA PORTMAP_VENABLE_arm)
		swctl(20, 7, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 7
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_arm_venable", "1", 0);
	}

	if (!vidPorts[0])
	{
	    snprintf(cmdBuff, sizeof(cmdBuff), "vconfig add %s %d; ifconfig %s.%d up", MGMT_PORT_LINUX_IFNAME, vlan_id, MGMT_PORT_LINUX_IFNAME, vlan_id);
	    MNET_DEBUG("Creating vlan:%d for l2sm0 interface command is:%s\n" COMMA vlan_id COMMA cmdBuff)
	    system(cmdBuff);    

	    MNET_DEBUG("--SW handler, swctl %s -v %d -m %d -q 1\n" COMMA PORTMAP_arm COMMA vlan_id COMMA TAGGING_MODE)
	    swctl(16, 7, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 16 -p 7 -v <vlan_id> -m 2 -q 1
	}
    sw_add_ports(vlan_id, ports_add, &atom_port, &ext_port);
	
	l_iLen = strlen(vidPorts);		
	if (0 != vidPorts[0] && vidPorts[l_iLen] != ' ')
		strncat(vidPorts, " ", 1);

	strncat(vidPorts, ports_add, (size_t)(strlen(ports_add)));
	printf("sysevent set sw_vid_%d_ports %s\n", vlan_id, vidPorts);
	snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_ports",vlan_id);
	sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, vidPorts, 0);

	if (1 == ext_port)
	{
		if (1 != i2e_venable)
		{
			MNET_DEBUG("--SW handler, swctl %s\n" COMMA PORTMAP_VENABLE_I2E)
			swctl(20, 2, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 2

		    printf("sysevent set PORTMAP_VENABLE_I2E 1\n");
		    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_I2E_venable", "1", 0); 
		}
		if (1 != e2i_venable)	
		{
			MNET_DEBUG("--SW handler, swctl %s\n" COMMA PORTMAP_VENABLE_E2I)
			swctl(4, 5, -1, -1, -1, -1, NULL, NULL); //swctl -c 4 -p 5
					
		    printf("sysevent set PORTMAP_VENABLE_E2I 1\n");
		    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_E2I_venable", "1", 0); 
		}
		if (!ext_vidPorts[0])
		{
			MNET_DEBUG("--SW handler, swctl %s -v %d -m %d -q 1\n" COMMA PORTMAP_I2E COMMA vlan_id COMMA TAGGING_MODE)
			swctl(16, 2, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 16 -p 2 -v <vlan_id> -m 2 -q 1
			MNET_DEBUG("--SW handler, swctl %s -v %d -m %d -q 1\n" COMMA PORTMAP_E2I COMMA vlan_id COMMA TAGGING_MODE)
			swctl(0, 5, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 0 -p 5 -v <vlan_id> -m 2 -q 1

			sysevent_get(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_ext_vids", l_cExt_Vids, sizeof(l_cExt_Vids));
			l_iLen = strlen(l_cExt_Vids);    
		    if (0 != l_cExt_Vids[0] && l_cExt_Vids[l_iLen] != ' ')
        		strncat(l_cExt_Vids, " ", 1);
			
			sprintf(l_cVlan_Id, "%d", vlan_id);
			strncat(l_cExt_Vids, l_cVlan_Id, (size_t)(strlen(l_cVlan_Id)));
			printf("sysevent set sw_ext_vids %s\n", l_cExt_Vids);
			sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_ext_vids", l_cExt_Vids, 0);
		}
		snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_extports",vlan_id);
		strcat(ext_vidPorts, ports_add);
		printf("sysevent set sw_vid_%d_extports %s\n", vlan_id, ext_vidPorts);
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, ext_vidPorts, 0);
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
		snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_atomports",vlan_id);
		strcat(atom_vidPorts, ports_add);
		printf("sysevent set sw_vid_%d_atomports %s\n", vlan_id, atom_vidPorts);
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, atom_vidPorts, 0);
	}
}

int delVlan(char* argv[], int argc)
{
	printf("Inside delVlan function\n");
}

int restore_ext_sw(char* argv[], int argc)
{
	printf("Inside restore_ext_sw function\n");
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
#endif
