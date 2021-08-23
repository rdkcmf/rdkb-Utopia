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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "errno.h"
#include "dhcp_server_functions.h"
#include "print_uptime.h"
#include "util.h"
#include "service_dhcp_server.h"
#include "safec_lib_common.h"

#define THIS        "/usr/bin/service_dhcp"
#define BIN			"dnsmasq"
#define SERVER      "dnsmasq"
#define PMON        "/etc/utopia/service.d/pmon.sh"
#define RESOLV_CONF "/etc/resolv.conf"
#define DHCP_CONF   "/var/dnsmasq.conf"
#define PID_FILE    "/var/run/dnsmasq.pid"
#define RPC_CLIENT	"/usr/bin/rpcclient"
#define XHS_IF_NAME "brlan1"

#define ERROR   	-1
#define SUCCESS 	0
#define BOOL        int
#define TRUE        1
#define FALSE       0

#define DHCP_SLOW_START_1_FILE  "/etc/cron/cron.everyminute/dhcp_slow_start.sh"
#define DHCP_SLOW_START_2_FILE  "/etc/cron/cron.every5minute/dhcp_slow_start.sh"
#define DHCP_SLOW_START_3_FILE  "/etc/cron/cron.every10minute/dhcp_slow_start.sh"

static char dnsOption[8] = "";

extern void copy_command_output(char *, char *, int);
extern void print_with_uptime(const char*);
extern BOOL compare_files(char *, char *);
extern void wait_till_end_state (char *);
extern void copy_file(char *, char *);
extern void remove_file(char *);
extern void print_file(char *);
extern void get_device_props();
extern void executeCmd(char *);
extern int g_iSyseventfd;
extern token_t g_tSysevent_token;

extern char g_cDhcp_Lease_Time[8], g_cTime_File[64];
extern char g_cBox_Type[8];
#ifdef XDNS_ENABLE
extern char g_cXdns_Enabled[8];
#endif
extern char g_cAtom_Arping_IP[16];


void getRFC_Value(const char* dnsOption)
{
        int result = 0;
        char status[16] = "true";
        char dnsSet[8] = " -o ";
        char l_DnsStrictOrderStatus[16] = {0};

        syscfg_get(NULL, "DNSStrictOrder", l_DnsStrictOrderStatus, sizeof(l_DnsStrictOrderStatus));
        result = strcmp (status,l_DnsStrictOrderStatus);
        if (result == 0){
            strncpy((char *)dnsOption,dnsSet, strlen(dnsSet));
            fprintf(stdout, "DNSMASQ getRFC_Value %s %s %d\n",status,l_DnsStrictOrderStatus,sizeof(l_DnsStrictOrderStatus));
            fprintf(stderr, "Starting dnsmasq with additional dns strict order option: %s\n",l_DnsStrictOrderStatus);
        }
        else{
            fprintf(stdout, "FAILURE: DNSMASQ getRFC_Value syscfg_get %s %s %d\n",status,l_DnsStrictOrderStatus,sizeof(l_DnsStrictOrderStatus)); 
            fprintf(stderr, "RFC DNSTRICT ORDER is not defined or Enabled %s\n", l_DnsStrictOrderStatus);
        }
}
int dnsmasq_server_start()
{
    char l_cSystemCmd[255] = {0};
    errno_t safec_rc = -1;

    getRFC_Value (dnsOption);
    fprintf(stdout, "Adding DNSMASQ Option: %s\n", dnsOption);
    strtok(dnsOption,"\n");

#ifdef XDNS_ENABLE
    if (!strncasecmp(g_cXdns_Enabled, "true", 4)) //If XDNS is ENABLED
    {
        char l_cXdnsRefacCodeEnable[8] = {0};

        syscfg_get(NULL, "XDNS_RefacCodeEnable", l_cXdnsRefacCodeEnable, sizeof(l_cXdnsRefacCodeEnable));
        if (!strncmp(l_cXdnsRefacCodeEnable, "1", 1)){
                safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C %s %s --xdns-refac-code",
                                SERVER, DHCP_CONF,dnsOption);
                if(safec_rc < EOK){
                   ERR_CHK(safec_rc);
                }
        }else{
                safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C %s %s",
                                SERVER, DHCP_CONF,dnsOption);
                if(safec_rc < EOK){
                   ERR_CHK(safec_rc);
                }
        }
    }
    else //If XDNS is not enabled 
#endif
    {
        safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -P 4096 -C %s %s", SERVER, DHCP_CONF,dnsOption);
        if(safec_rc < EOK){
           ERR_CHK(safec_rc);
        }
    }

	return system(l_cSystemCmd);
}

void dhcp_server_stop()
{
        char l_cDhcp_Status[16] = {0}, l_cSystemCmd[255] = {0};
        int l_iSystem_Res;
        errno_t safec_rc = -1;
        wait_till_end_state("dhcp_server");
   	sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", l_cDhcp_Status, sizeof(l_cDhcp_Status));	
	if (!strncmp(l_cDhcp_Status, "stopped", 7))
	{
		fprintf(stderr, "DHCP SERVER is already stopped not doing anything\n");
		return;
	}

	//dns is always running
   	prepare_hostname();
   	prepare_dhcp_conf("dns_only");
	
	safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s unsetproc dhcp_server", PMON);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
	l_iSystem_Res = system(l_cSystemCmd); //dnsmasq command
    if (0 != l_iSystem_Res)
	{
		fprintf(stderr, "%s command didnt execute successfully\n", l_cSystemCmd);
	}

	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "stopped", 0);
   	system("killall `basename dnsmasq`");
	remove_file(PID_FILE);
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "stopped", 0);

	memset(l_cSystemCmd, 0x00, sizeof(l_cSystemCmd));

    l_iSystem_Res = dnsmasq_server_start(); //dnsmasq command
    if (0 == l_iSystem_Res)
    {
        fprintf(stderr, "dns-server started successfully\n");
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
    }
	else
	{
        fprintf(stderr, "dns-server didnt start\n");
	}
}

BOOL IsDhcpConfHasInterface(void)
{
    FILE *fp = NULL;
    char buf[512];

    fp = fopen(DHCP_CONF,"r");
    if (NULL == fp)
        return FALSE;
    memset(buf,0,sizeof(buf));
    while (fgets(buf,sizeof(buf),fp) != NULL)
    {
        char *interface = NULL;
        interface = strstr(buf,"interface=");
        if (interface)
        printf ("\ninterface search res : %s\n",interface);
        if (interface)
            return TRUE;
    }

    fprintf(stderr, "dnsmasq.conf does not have any interfaces\n");
    return FALSE;
}


int dhcp_server_start (char *input)
{
	//Declarations
	char l_cDhcpServerEnable[16] = {0}, l_cLanStatusDhcp[16] = {0};
	char l_cSystemCmd[255] = {0}, l_cPsm_Mode[8] = {0}, l_cStart_Misc[8] = {0};
	char l_cPmonCmd[255] = {0}, l_cDhcp_Tmp_Conf[32] = {0};
	char l_cCurrent_PID[8] = {0}, l_cRpc_Cmd[64] = {0};
	char l_cCommand[128] = {0}, l_cBuf[128] = {0};
    char l_cBridge_Mode[8] = {0};
    char l_cDhcp_Server_Prog[16] = {0};
    int dhcp_server_progress_count = 0;

	BOOL l_bRestart = FALSE, l_bFiles_Diff = FALSE, l_bPid_Present = FALSE;
	FILE *l_fFp = NULL;
	int l_iSystem_Res;
        int fd = 0;

	char *l_cToken = NULL;
	errno_t safec_rc = -1;

	service_dhcp_init();

    // DHCP Server Enabled
    syscfg_get(NULL, "dhcp_server_enabled", l_cDhcpServerEnable, sizeof(l_cDhcpServerEnable));

	if (!strncmp(l_cDhcpServerEnable, "0", 1))
	{
      	//when disable dhcp server in gui, we need remove the corresponding process in backend, 
		// or the dhcp server still work.
		fprintf(stderr, "DHCP Server is disabled not proceeding further\n");
		dhcp_server_stop();
		remove_file("/var/tmp/lan_not_restart");
		sysevent_set(g_iSyseventfd, g_tSysevent_token, 
					 "dhcp_server-status", "error", 0);

		sysevent_set(g_iSyseventfd, g_tSysevent_token, 
					 "dhcp_server-errinfo", "dhcp server is disabled by configuration", 0);
      	return 0;
	}
	
	//LAN Status DHCP
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_status-dhcp", l_cLanStatusDhcp, sizeof(l_cLanStatusDhcp));	
	if (strncmp(l_cLanStatusDhcp, "started", 7))
	{
		fprintf(stderr, "lan_status-dhcp is not started return without starting DHCP server\n");
		remove_file("/var/tmp/lan_not_restart");
		return 0;
	}
   
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", l_cDhcp_Server_Prog, sizeof(l_cDhcp_Server_Prog));
    while((!(strncmp(l_cDhcp_Server_Prog, "inprogress", 10))) && (dhcp_server_progress_count < 5))
    {
        fprintf(stderr, "SERVICE DHCP : dhcp_server-progress is inprogress , waiting... \n");
        sleep(2);
        sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", l_cDhcp_Server_Prog, sizeof(l_cDhcp_Server_Prog));
        dhcp_server_progress_count++;
    }

    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "inprogress", 0);
	fprintf(stderr, "SERVICE DHCP : dhcp_server-progress is set to inProgress from dhcp_server_start \n");
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-errinfo", "", 0);
   
	strncpy(l_cDhcp_Tmp_Conf, "/tmp/dnsmasq.conf.orig", sizeof(l_cDhcp_Tmp_Conf));
	copy_file(DHCP_CONF, l_cDhcp_Tmp_Conf);

    prepare_hostname();
    prepare_dhcp_conf();
	//Not calling this function as we are not doing anything here
	//sanitize_leases_file(); 

	//we need to decide whether to completely restart the dns/dhcp_server
	//or whether to just have it reread everything
   	//SIGHUP is reread (except for dnsmasq.conf)

	l_bFiles_Diff = compare_files(DHCP_CONF, l_cDhcp_Tmp_Conf);
	if (FALSE == l_bFiles_Diff) //Files are not identical
	{	
		fprintf(stderr, "files are not identical restart dnsmasq\n");
		l_bRestart = TRUE;
	}
	else
	{
		fprintf(stderr, "files are identical not restarting dnsmasq\n");
	}
	
	l_fFp = fopen(PID_FILE, "r");
	if (NULL == l_fFp) //Mostly the error could be ENOENT(errno 2) 
	{
		fprintf(stderr, "Error:%d while opening file:%s\n", errno, PID_FILE); 
	}
	else	
	{
		fgets(l_cCurrent_PID, sizeof(l_cCurrent_PID), l_fFp);
		fclose(l_fFp); /*RDKB-12965 & CID:-34555*/
	}	
	if (0 == l_cCurrent_PID[0])
	{
		l_bRestart = TRUE;
	}	
	else
	{
	    safec_rc = strcpy_s(l_cCommand, sizeof(l_cCommand),"pidof dnsmasq");
		ERR_CHK(safec_rc);
    	copy_command_output(l_cCommand, l_cBuf, sizeof(l_cBuf));
	    l_cBuf[strlen(l_cBuf)] = '\0';

		if ('\0' == l_cBuf[0] || 0 == l_cBuf[0])
		{
			l_bRestart = TRUE;
		}
		else
		{
			//strstr to check PID didnt work, so had to use strtok
            l_cToken = strtok(l_cBuf, " ");
            while (l_cToken != NULL)
            {
                if (!strncmp(l_cToken, l_cCurrent_PID, strlen(l_cToken)))
                {
                    l_bPid_Present = TRUE;
                    break;
                }
                l_cToken = strtok(NULL, " ");
            }
            if (FALSE == l_bPid_Present)
            {
                fprintf(stderr, "PID:%d is not part of PIDS of dnsmasq\n", atoi(l_cCurrent_PID));
                l_bRestart = TRUE;
            }
            else
            {
                fprintf(stderr, "PID:%d is part of PIDS of dnsmasq\n", atoi(l_cCurrent_PID));
            }
		}
	}
	remove_file(l_cDhcp_Tmp_Conf);
   	system("killall -HUP `basename dnsmasq`");
	if (FALSE == l_bRestart)
	{
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "started", 0);
        sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "completed", 0);
		remove_file("/var/tmp/lan_not_restart");
		return 0;
	}

	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "stopped", 0);
   	system("killall `basename dnsmasq`");
	remove_file(PID_FILE);

        /* Kill dnsmasq if its not stopped properly */
	safec_rc = strcpy_s(l_cCommand, sizeof(l_cCommand),"pidof dnsmasq");
	ERR_CHK(safec_rc);
        memset (l_cBuf, '\0',  sizeof(l_cBuf));
    	copy_command_output(l_cCommand, l_cBuf, sizeof(l_cBuf));
	l_cBuf[strlen(l_cBuf)] = '\0';

	if ('\0' != l_cBuf[0] && 0 != l_cBuf[0])
        {
            fprintf(stderr, "kill dnsmasq with SIGKILL if its still running \n");
            system("kill -KILL `pidof dnsmasq`");
        }
    sysevent_get(g_iSyseventfd, g_tSysevent_token,
                         "bridge_mode", l_cBridge_Mode,
                         sizeof(l_cBridge_Mode));

    // TCCBR:4710- In Bridge mode, Dont run dnsmasq when there is no interface in dnsmasq.conf
    if ((strncmp(l_cBridge_Mode, "0", 1)) && (FALSE == IsDhcpConfHasInterface()))
    {
        fprintf(stderr, "no interface present in dnsmasq.conf %s process not started\n", SERVER);
        safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s unsetproc dhcp_server", PMON);
        if(safec_rc < EOK){
            ERR_CHK(safec_rc);
        }
        l_iSystem_Res = system(l_cSystemCmd); //dnsmasq command
        if (0 != l_iSystem_Res)
        {
            fprintf(stderr, "%s command didnt execute successfully\n", l_cSystemCmd);
        }
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "stopped", 0);
        sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "completed", 0);
		remove_file("/var/tmp/lan_not_restart");
        return 0;
    }
#if defined _BWG_NATIVE_TO_RDKB_REQ_
	/*Run script to reolve the IP address when upgrade from native to rdkb case only */
	system("sh /etc/utopia/service.d/migration_native_rdkb.sh ");
#endif
	//we use dhcp-authoritative flag to indicate that this is
   	//the only dhcp server on the local network. This allows
   	//the dns server to give out a _requested_ lease even if
   	//that lease is not found in the dnsmasq.leases file
	print_with_uptime("RDKB_SYSTEM_BOOT_UP_LOG : starting dhcp-server_from_dhcp_server_start:");
	int l_iDnamasq_Retry;	

	l_iSystem_Res = dnsmasq_server_start(); //dnsmasq command
	if (0 == l_iSystem_Res)
	{
    	fprintf(stderr, "%s process started successfully\n", SERVER);
	}
	else
	{
		if ((!strncmp(g_cBox_Type, "XB6", 3)) || 
			(!strncmp(g_cBox_Type, "TCCBR", 3))) //XB6 / TCCBR case 5 retries are needed
		{
			for (l_iDnamasq_Retry = 0; l_iDnamasq_Retry < 5; l_iDnamasq_Retry++)
			{
            	fprintf(stderr, "%s process failed to start sleep for 5 sec and restart it\n", SERVER);
	            sleep(5);
				l_iSystem_Res = dnsmasq_server_start(); //dnsmasq command
			    if (0 == l_iSystem_Res)
			    {
    				fprintf(stderr, "%s process started successfully\n", SERVER);
					break;
			    }
				else
				{
    				fprintf(stderr, "%s process did not start successfully\n", SERVER);
					continue;
				}
			}
		}
	}

	//DHCP_SLOW_START_NEEDED is always false / set to false so below code is removed
   	/*if [ "1" = "$DHCP_SLOW_START_NEEDED" ] && [ -n "$TIME_FILE" ]; then
    	echo "#!/bin/sh" > $TIME_FILE
        echo "   sysevent set dhcp_server-restart lan_not_restart" >> $TIME_FILE
        chmod 700 $TIME_FILE
   	fi*/

	sysevent_get(g_iSyseventfd, g_tSysevent_token, "system_psm_mode", l_cPsm_Mode, sizeof(l_cPsm_Mode));
	sysevent_get(g_iSyseventfd, g_tSysevent_token, "start-misc", l_cStart_Misc, sizeof(l_cStart_Misc));
	if (strcmp(l_cPsm_Mode, "1")) //PSM Mode is Not 1
	{
		if ((access("/var/tmp/lan_not_restart", F_OK) == -1 && errno == ENOENT) && 
			((NULL == input) || (NULL != input && strncmp(input, "lan_not_restart", 15))))
		{
        	if (!strncmp(l_cStart_Misc, "ready", 5))
			{
                print_with_uptime("RDKB_SYSTEM_BOOT_UP_LOG : Call gw_lan_refresh_from_dhcpscript:");
                system("gw_lan_refresh &");
        	}
		}
     	else
		{
          	fprintf(stderr, "lan_not_restart found! Don't restart lan!\n");
			remove_file("/var/tmp/lan_not_restart");	
		}
	}

   	if (access("/tmp/dhcp_server_start", F_OK) == -1 && errno == ENOENT) //If file not present
	{
    	print_with_uptime("dhcp_server_start is called for the first time private LAN initization is complete");
		if((fd = creat("/tmp/dhcp_server_start", S_IRUSR | S_IWUSR)) == -1)
		{
			fprintf(stderr, "File: /tmp/dhcp_server_start creation failed with error:%d\n", errno);
		} else  {
                        close(fd);
                }
		print_uptime("boot_to_ETH_uptime",NULL, NULL);
       	
		print_with_uptime("LAN initization is complete notify SSID broadcast");
		snprintf(l_cRpc_Cmd, sizeof(l_cRpc_Cmd), "rpcclient %s \"/bin/touch /tmp/.advertise_ssids\"", g_cAtom_Arping_IP);
		executeCmd(l_cRpc_Cmd);
	}
   
    // This function is called for brlan0 and brlan1
    // If brlan1 is available then XHS service is available post all DHCP configuration   
    if (is_iface_present(XHS_IF_NAME))
    {   
        fprintf(stderr, "Xfinityhome service is UP\n");
        if (access("/tmp/xhome_start", F_OK) == -1 && errno == ENOENT)
        {
            if((fd = creat("/tmp/xhome_start", S_IRUSR | S_IWUSR)) == -1)
            {
                fprintf(stderr, "File: /tmp/xhome_start creation failed with error:%d\n", errno);
            }
            else
            {
                close(fd);
            }
            print_uptime("boot_to_XHOME_uptime",NULL, NULL);
        }
    }   
    else
    {   
        fprintf(stderr, "Xfinityhome service is not UP yet\n");
    }

	safec_rc = sprintf_s(l_cPmonCmd, sizeof(l_cPmonCmd),"%s setproc dhcp_server %s %s \"%s dhcp_server-restart\"", 
			PMON, BIN, PID_FILE, THIS);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }

	system(l_cPmonCmd);

    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "started", 0);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "completed", 0);
   	print_with_uptime("DHCP SERVICE :dhcp_server-progress_is_set_to_completed:");
   	fprintf(stderr, "RDKB_DNS_INFO is : -------  resolv_conf_dump  -------\n");
	print_file(RESOLV_CONF);
	return 0;
}

int service_dhcp_init()
{
	char l_cPropagate_Ns[8] = {0}, l_cPropagate_Dom[8] = {0};
	char l_cSlow_Start[8] = {0}, l_cByoi_Enabled[8] = {0};
    char l_cWan_IpAddr[16] = {0}, l_cPrim_Temp_Ip_Prefix[16] = {0}, l_cCurrent_Hsd_Mode[16] = {0};
   	char l_cTemp_Dhcp_Lease[8] = {0}, l_cDhcp_Slow_Start_Quanta[8] = {0};
    char l_cDhcpSlowStartQuanta[8] = {0};

	syscfg_get(NULL, "dhcp_server_propagate_wan_nameserver", l_cPropagate_Ns, sizeof(l_cPropagate_Ns));
	if (strncmp(l_cPropagate_Ns, "1", 1))
	{
	    fprintf(stderr, "Propagate NS is set from block_nat_redirection value is:%s\n", l_cPropagate_Ns);
    	syscfg_get(NULL, "block_nat_redirection", l_cPropagate_Ns, sizeof(l_cPropagate_Ns));
	}

	syscfg_get(NULL, "dhcp_server_propagate_wan_domain", l_cPropagate_Dom, sizeof(l_cPropagate_Dom));

	// Is dhcp slow start feature enabled
	int l_iSlow_Start_Needed;
	syscfg_get(NULL, "dhcp_server_slow_start", l_cSlow_Start, sizeof(l_cSlow_Start));

	syscfg_get(NULL, "byoi_enabled", l_cByoi_Enabled, sizeof(l_cByoi_Enabled));

	if ((!strncmp(l_cPropagate_Ns, "1", 1)) || (!strncmp(l_cPropagate_Dom, "1", 1)) ||
	    (!strncmp(l_cByoi_Enabled, "1", 1)))
	{
	    if (!strncmp(l_cSlow_Start, "1", 1))
	    {
	        sysevent_get(g_iSyseventfd, g_tSysevent_token, "current_wan_ipaddr", 
						 l_cWan_IpAddr, sizeof(l_cWan_IpAddr));

	        sysevent_get(g_iSyseventfd, g_tSysevent_token, "current_hsd_mode", 
						 l_cCurrent_Hsd_Mode, sizeof(l_cCurrent_Hsd_Mode));

	        syscfg_get(NULL, "primary_temp_ip_prefix", l_cPrim_Temp_Ip_Prefix, sizeof(l_cPrim_Temp_Ip_Prefix));

	        if (!strncmp(l_cWan_IpAddr, "0.0.0.0", 7))
    	    {
        	    l_iSlow_Start_Needed = 1;
	        }
    	    if ((!strncmp(l_cByoi_Enabled, "1", 1)) && (!strncmp(l_cCurrent_Hsd_Mode, "primary", 7)) &&
        	    (!strncmp(l_cPrim_Temp_Ip_Prefix, "2", 1))) //TODO complete this if statement
	            //[ "$primary_temp_ip_prefix" = ${wan_ipaddr:0:${#primary_temp_ip_prefix}} ] ; then
    	    {
        	    l_iSlow_Start_Needed = 1;
	        }
    	}
	}

	// Disable this to alway pick lease value from syscfg.db
	l_iSlow_Start_Needed = 0;

	// DHCP_LEASE_TIME is the number of seconds or minutes or hours to give as a lease
	syscfg_get(NULL, "dhcp_lease_time", g_cDhcp_Lease_Time, sizeof(g_cDhcp_Lease_Time));

	if (1 == l_iSlow_Start_Needed)
	{
	    int l_iDhcpSlowQuanta;
    	syscfg_get(NULL, "temp_dhcp_lease_length", l_cTemp_Dhcp_Lease, sizeof(l_cTemp_Dhcp_Lease));

    	if (0 == l_cTemp_Dhcp_Lease[0])
	    {
        	sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_slow_start_quanta", 
						 l_cDhcpSlowStartQuanta, sizeof(l_cDhcpSlowStartQuanta));

	        l_iDhcpSlowQuanta = atoi(l_cDhcpSlowStartQuanta);
    	}
	    else
    	{
	        l_iDhcpSlowQuanta = atoi(l_cTemp_Dhcp_Lease);
    	}

	    if (0 == l_cDhcp_Slow_Start_Quanta[0])
    	{
	        l_iDhcpSlowQuanta = 1;
    	    strncpy(g_cTime_File, DHCP_SLOW_START_1_FILE, sizeof(g_cTime_File));
	    }
    	else if (0 == l_cTemp_Dhcp_Lease[0])
	    {
    	    if (l_iDhcpSlowQuanta < 5)
	        {
    	        l_iDhcpSlowQuanta = l_iDhcpSlowQuanta + 1;
        	    strncpy(g_cTime_File, DHCP_SLOW_START_1_FILE, sizeof(g_cTime_File));
	        }
    	    else if (l_iDhcpSlowQuanta <= 15)
        	{
	            l_iDhcpSlowQuanta = l_iDhcpSlowQuanta + 5;
    	        strncpy(g_cTime_File, DHCP_SLOW_START_2_FILE, sizeof(g_cTime_File));
        	}
	        else if (l_iDhcpSlowQuanta <= 100)
    	    {
        	    l_iDhcpSlowQuanta = l_iDhcpSlowQuanta * 2;
            	strncpy(g_cTime_File, DHCP_SLOW_START_3_FILE, sizeof(g_cTime_File));
	        }
    	    else
        	{
	            l_iDhcpSlowQuanta = atoi(g_cDhcp_Lease_Time);
    	        strncpy(g_cTime_File, DHCP_SLOW_START_3_FILE, sizeof(g_cTime_File));
        	}
    	}

	    if ((0 == l_cTemp_Dhcp_Lease[0]) && (l_iDhcpSlowQuanta > 60))
    	{
	        l_iDhcpSlowQuanta = 60;
    	}
		
		snprintf(l_cDhcp_Slow_Start_Quanta, sizeof(l_cDhcp_Slow_Start_Quanta), "%d", l_iDhcpSlowQuanta);
	    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_slow_start_quanta", l_cDhcp_Slow_Start_Quanta, 0);
		snprintf(g_cDhcp_Lease_Time, sizeof(g_cDhcp_Lease_Time), "%d", l_iDhcpSlowQuanta);
	}
	else
	{
		//Setting the dhcp_slow_start_quanta to empty / NULL
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_slow_start_quanta", "", 0);
	}
	if(0 == g_cDhcp_Lease_Time[0])
	{
		fprintf(stderr, "DHCP Lease time is empty, set to default value 24h\n");
	    strncpy(g_cDhcp_Lease_Time, "24h", sizeof(g_cDhcp_Lease_Time));
	}

	get_device_props();
    return SUCCESS;
}

void lan_status_change(char *input)
{
	char l_cLan_Status[16] = {0}, l_cDhcp_Server_Enabled[8] = {0};
	int l_iSystem_Res;

	sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan-status", l_cLan_Status, sizeof(l_cLan_Status));
	fprintf(stderr, "SERVICE DHCP : Inside lan status change with lan-status:%s\n", l_cLan_Status);
   	fprintf(stderr, "SERVICE DHCP : Current lan status is:%s\n", l_cLan_Status);
    
	syscfg_get(NULL, "dhcp_server_enabled", l_cDhcp_Server_Enabled, sizeof(l_cDhcp_Server_Enabled));
	if (!strncmp(l_cDhcp_Server_Enabled, "0", 1))
	{
    	//set hostname and /etc/hosts cause we are the dns forwarder
        prepare_hostname();

        //also prepare dns part of dhcp conf cause we are the dhcp server too
        prepare_dhcp_conf("dns_only");

        fprintf(stderr, "SERVICE DHCP : Start dhcp-server from lan status change");
           
	    l_iSystem_Res = dnsmasq_server_start(); //dnsmasq command
    	if (0 == l_iSystem_Res)
	    {
    	    fprintf(stderr, "%s process started successfully\n", SERVER);
	    }
		else
		{
			fprintf(stderr, "%s process didn't start successfully\n", SERVER);
		}
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
	}
    else
	{
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan_status-dhcp", "started", 0);
		if (NULL == input)
		{
	        fprintf(stderr, "SERVICE DHCP :  Call start DHCP server from lan status change with NULL\n");
			dhcp_server_start(NULL);
		}
		else
		{
			fprintf(stderr, "SERVICE DHCP :  Call start DHCP server from lan status change with input:%s\n", input);
            dhcp_server_start(input);
		}
	}
}
