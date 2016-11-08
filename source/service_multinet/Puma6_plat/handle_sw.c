#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7)
#include "sysevent/sysevent.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "handle_sw.h"
#include "swctl_hal.h"

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

void execSwCtl(char *port, int vlan_id, int tagged)
{
	printf("Inside execSwCtl function\n");
	int port_venable = 0, command = 0, command_ven, command_def, port_cmd;
	
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
		port_cmd = 0;
    }
    else if (!strncmp("sw_2", port, 4))
    {
        printf("sw_2 case use PORTMAP_sw_2\n");
		command_ven = 4;
		command_def = 34;
		port_cmd = 1;
    }
    else if (!strncmp("sw_3", port, 4))
    {
        printf("sw_3 case use PORTMAP_sw_3\n");
		command_ven = 4;
		command_def = 34;
		port_cmd = 2;
    }
    else if (!strncmp("sw_4", port, 4))
    {
        printf("sw_4 case use PORTMAP_sw_4\n");
		command_ven = 4;
		command_def = 34;
		port_cmd = 3;
    }
    else if (!strncmp("sw_5", port, 4))
    {
        printf("sw_5 case use PORTMAP_sw_5\n");
		command = 16;
		command_ven = 16;
		command_def = 20;
		port_cmd = 3;
    }
    else
    {
        printf("It should not come here\n");
    }

	if (1 != port_venable)
	{
		printf("--SW handler swctl -c %d -p %d\n", command_ven, port_cmd);		
		swctl(command_ven, port_cmd, -1, -1, -1, -1, NULL, NULL);

	    printf("sysevent set %s 1\n", sysevent_cmd);
        sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, "1", 0);
	}

    printf("--SW handler swctl -c %d -p %d -v %d -m %d -q 1 \n", command, port_cmd, vlan_id, tagged);
	swctl(command, port_cmd, vlan_id, tagged, 1, -1, NULL, NULL); //e.g: swctl -c 0 -p 0 -v 100 -m 1 -q 1

    if (UNTAGGED_MODE == tagged)
    {   
    	printf("Untagged case execute swctl for port:%s\n", port);
        printf("--SW handler swctl -c %d -p %d -v %d\n", command_def, port_cmd, vlan_id);
		swctl(command_def, port_cmd, vlan_id, -1, -1, -1, NULL, NULL); //e.g swctl -c 34 -p 0 -v 100 
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
				ports_add++;
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
	printf("Inside addVlan function\n");
	if (0 == hdl_sw_sysevent_fd)
	{
		printf("hdl_sw_sysevent_fd is zero call handle_sw_init\n");
		handle_sw_init();
	}
	char sysevent_cmd[50];
	char cmdBuff[80];
	char vidPorts[50] = {0};
	char ext_vidPorts[50] = {0};
	char atom_vidPorts[50] = {0};
	char val[20];
	int arm_venable = 0, i2e_venable = 0, e2i_venable = 0, atom_venable = 0, atom_port = 0, ext_port = 0;

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
		printf("--SW handler, swctl %s\n",PORTMAP_VENABLE_arm);
		swctl(20, 7, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 7
		sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_arm_venable", "1", 0);
	}

	if (!vidPorts[0])
	{
	    snprintf(cmdBuff, sizeof(cmdBuff), "vconfig add %s %d; ifconfig %s.%d up", MGMT_PORT_LINUX_IFNAME, vlan_id, MGMT_PORT_LINUX_IFNAME, vlan_id);
	    printf("command is:%s\n", cmdBuff);
	    system(cmdBuff);    

	    printf("--SW handler, swctl %s -v %d -m %d -q 1\n", PORTMAP_arm, vlan_id, TAGGING_MODE);
	    swctl(16, 7, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 16 -p 7 -v <vlan_id> -m 2 -q 1
	}
        sw_add_ports(vlan_id, ports_add, &atom_port, &ext_port);
	snprintf(sysevent_cmd, sizeof(sysevent_cmd), "sw_vid_%d_ports",vlan_id);
	strcat(vidPorts, ports_add);
	printf("sysevent set sw_vid_%d_ports %s\n", vlan_id, vidPorts);
	sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, sysevent_cmd, vidPorts, 0);

	if (1 == ext_port)
	{
		if (1 != i2e_venable)
		{
			printf("--SW handler, swctl %s\n", PORTMAP_VENABLE_I2E);
			swctl(20, 2, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 2

		    printf("sysevent set PORTMAP_VENABLE_I2E 1\n");
		    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_I2E_venable", "1", 0); 
		}
		if (1 != e2i_venable)	
		{
			printf("--SW handler, swctl %s\n", PORTMAP_VENABLE_E2I);
			swctl(4, 5, -1, -1, -1, -1, NULL, NULL); //swctl -c 4 -p 5
					
		    printf("sysevent set PORTMAP_VENABLE_E2I 1\n");
		    sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_E2I_venable", "1", 0); 
		}
		if (!ext_vidPorts[0])
		{
			printf("--SW handler, swctl %s -v %d -m %d -q 1\n", PORTMAP_I2E, vlan_id, TAGGING_MODE);
			swctl(16, 2, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 16 -p 2 -v <vlan_id> -m 2 -q 1
			printf("--SW handler, swctl %s -v %d -m %d -q 1\n", PORTMAP_E2I, vlan_id, TAGGING_MODE);
			swctl(0, 5, vlan_id, TAGGING_MODE, 1, -1, NULL, NULL); //swctl -c 0 -p 5 -v <vlan_id> -m 2 -q 1
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
			printf("--SW handler, swctl %s\n",PORTMAP_VENABLE_atom);
			swctl(20, 0, -1, -1, -1, -1, NULL, NULL); //swctl -c 20 -p 0
			sysevent_set(hdl_sw_sysevent_fd, hdl_sw_sysevent_token, "sw_port_atom_venable", "1", 0);
		}
		if (!atom_vidPorts[0])
		{
			printf("--SW handler, swctl %s -v %d -m %d -q 1\n", PORTMAP_atom, vlan_id, TAGGING_MODE);
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
#endif
