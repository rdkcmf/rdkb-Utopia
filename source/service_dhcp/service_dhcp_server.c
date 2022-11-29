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
#include "secure_wrapper.h"
#include "lan_handler.h"

#define THIS        "/usr/bin/service_dhcp"
#define BIN			"dnsmasq"
#define SERVER      "dnsmasq"
#define PMON        "/etc/utopia/service.d/pmon.sh"
#define RESOLV_CONF "/etc/resolv.conf"
#define DHCP_CONF   "/var/dnsmasq.conf"
#define PID_FILE    "/var/run/dnsmasq.pid"
#define RPC_CLIENT	"/usr/bin/rpcclient"
#define XHS_IF_NAME "brlan1"

#define DEVICE_PROPERTIES "/etc/device.properties"
#define DHCP_TMP_CONF     "/tmp/dnsmasq.conf.orig"

#define ERROR   	-1
#define SUCCESS 	0
#define BOOL        int
#define TRUE        1
#define FALSE       0

#define DHCP_SLOW_START_1_FILE  "/etc/cron/cron.everyminute/dhcp_slow_start.sh"
#define DHCP_SLOW_START_2_FILE  "/etc/cron/cron.every5minute/dhcp_slow_start.sh"
#define DHCP_SLOW_START_3_FILE  "/etc/cron/cron.every10minute/dhcp_slow_start.sh"

static char dnsOption[8] = "";

extern void copy_command_output(FILE *, char *, int);
extern void print_with_uptime(const char*);
extern BOOL compare_files(char *, char *);
extern void wait_till_end_state (char *);
extern void copy_file(char *, char *);
extern void remove_file(char *);
extern void print_file(char *);
extern void get_device_props();
extern int executeCmd(char *);
extern int g_iSyseventfd;
extern token_t g_tSysevent_token;

extern char g_cDhcp_Lease_Time[8], g_cTime_File[64];
extern char g_cBox_Type[8];
#ifdef XDNS_ENABLE
extern char g_cXdns_Enabled[8];
#endif
extern char g_cAtom_Arping_IP[16];
extern int executeCmd(char *);
extern FILE* g_fArmConsoleLog; //Global file pointer declaration

#ifdef RDKB_EXTENDER_ENABLED
unsigned int Get_Device_Mode()
{
  	char dev_type[16] = {0};
    
        syscfg_get(NULL, "Device_Mode", dev_type, sizeof(dev_type));
        unsigned int dev_mode = atoi(dev_type);
        Dev_Mode mode;
        if(dev_mode==1)
        {
          mode =EXTENDER_MODE;
        }
        else
          mode = ROUTER;

        return mode;

}
#endif

void _get_shell_output(FILE *fp, char *buf, int len)
{
    char * p;

    if (fp)
    {
        if(fgets (buf, len-1, fp) != NULL)
        {
            buf[len-1] = '\0';
            if ((p = strchr(buf, '\n'))) {
                *p = '\0';
            }
        }
    v_secure_pclose(fp);
    }
}

static int getValueFromDevicePropsFile(char *str, char **value)
{
    FILE *fp = fopen(DEVICE_PROPERTIES, "r");
    char buf[ 1024 ] = { 0 };
    char *tempStr = NULL;
    int ret = 0;
    if( NULL != fp )
    {
        while ( fgets( buf, sizeof( buf ), fp ) != NULL )
        {
            if ( strstr( buf, str ) != NULL )
            {
                buf[strcspn( buf, "\r\n" )] = 0; // Strip off any carriage returns
                tempStr = strstr( buf, "=" );
                tempStr++;
                *value = tempStr;
                ret = 0;
                break;
            }
        }
        if( NULL == *value)
        {
            fprintf(g_fArmConsoleLog,"\n%s is not present in device.properties file\n",str);
            ret = -1;
        }
    }
    else
    {
        fprintf(g_fArmConsoleLog,"\nFailed to open file:%s\n", DEVICE_PROPERTIES);
        return -1;
    }
    if( fp )
    {
        fclose(fp);
    }
    return ret;
}

int get_Pool_cnt(char arr[15][2],FILE *pipe)
{
    fprintf(g_fArmConsoleLog,"\nInside %s - \n",__FUNCTION__);
    int iter=0;
    char sg_buff[2]={0};
    if (NULL == pipe)
    {
        fprintf(g_fArmConsoleLog,"\n Unable to open pipe for get_Pool_cnt pipe\n");
        return -1;
    }
    while(fgets(sg_buff, sizeof(sg_buff), pipe) != NULL )
    {
        if (atoi(sg_buff)!=0 && strncmp(sg_buff,"",1) != 0)
        {
            fprintf(g_fArmConsoleLog,"\n%s - Value=%s\n",__FUNCTION__,sg_buff);
            strncpy(arr[iter],sg_buff,2);
            iter++;
        }
    }
    fprintf(g_fArmConsoleLog,"\n%s ENDS ..... with Pool_Count=%d\n",__FUNCTION__,iter);
    return iter;
}

int get_PSM_VALUES_FOR_POOL(char *cmd,char *arr)
{
    fprintf(g_fArmConsoleLog,"\n%s - cmd=%s - ",__FUNCTION__,cmd); //NTR
    char* l_cpPsm_Get = NULL;
    int l_iRet_Val;
    l_iRet_Val = PSM_VALUE_GET_STRING(cmd, l_cpPsm_Get);
    fprintf(g_fArmConsoleLog,"\n%s - l_iRet_Val=%d - ",__FUNCTION__,l_iRet_Val); //NTR
    if (CCSP_SUCCESS == l_iRet_Val)
    {
        if (l_cpPsm_Get != NULL)
        {
            strncpy(arr, l_cpPsm_Get, 16);
            Ansc_FreeMemory_Callback(l_cpPsm_Get);
            l_cpPsm_Get = NULL;
        }
        else
        {
            fprintf(g_fArmConsoleLog, "\npsmcli get of :%s is empty\n", cmd);
            return -1;
        }
    }
    else
    {
        fprintf(g_fArmConsoleLog, "\nError:%d while getting parameter:%s\n",l_iRet_Val, cmd);
        return -1;
    }
    return 0;
}

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
            fprintf(g_fArmConsoleLog, "Starting dnsmasq with additional dns strict order option: %s\n",l_DnsStrictOrderStatus);
        }
        else{
            fprintf(stdout, "FAILURE: DNSMASQ getRFC_Value syscfg_get %s %s %d\n",status,l_DnsStrictOrderStatus,sizeof(l_DnsStrictOrderStatus)); 
            fprintf(g_fArmConsoleLog, "RFC DNSTRICT ORDER is not defined or Enabled %s\n", l_DnsStrictOrderStatus);
        }
}
int dnsmasq_server_start()
{
    char l_cSystemCmd[255] = {0};
    errno_t safec_rc = -1;

    getRFC_Value (dnsOption);
    fprintf(g_fArmConsoleLog, "\n%s Adding DNSMASQ Option: %s\n",__FUNCTION__, dnsOption);
    strtok(dnsOption,"\n");
    char l_cXdnsRefacCodeEnable[8] = {0};
    char l_cXdnsEnable[8] = {0};
    syscfg_get(NULL, "XDNS_RefacCodeEnable", l_cXdnsRefacCodeEnable, sizeof(l_cXdnsRefacCodeEnable));
    syscfg_get(NULL, "X_RDKCENTRAL-COM_XDNS", l_cXdnsEnable, sizeof(l_cXdnsEnable));
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_) && !defined(_PLATFORM_IPQ_)
#ifdef XDNS_ENABLE
    if (!strncasecmp(g_cXdns_Enabled, "true", 4)) //If XDNS is ENABLED
    {
        char l_cXdnsRefacCodeEnable[8] = {0};
	    char l_cXdnsEnable[8] = {0};

        syscfg_get(NULL, "XDNS_RefacCodeEnable", l_cXdnsRefacCodeEnable, sizeof(l_cXdnsRefacCodeEnable));
        syscfg_get(NULL, "X_RDKCENTRAL-COM_XDNS", l_cXdnsEnable, sizeof(l_cXdnsEnable));
        if (!strncmp(l_cXdnsRefacCodeEnable, "1", 1) && !strncmp(l_cXdnsEnable, "1", 1)){
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
#else
    char *XDNS_Enable=NULL;
    char *Box_Type=NULL;
    getValueFromDevicePropsFile("XDNS_ENABLE", &XDNS_Enable);
    getValueFromDevicePropsFile("MODEL_NUM", &Box_Type);
    fprintf(g_fArmConsoleLog, "\n%s Inside non XB3 block  g_cXdns_Enabled=%s XDNS_Enable=%s Box_Type=%s.......\n",__FUNCTION__,g_cXdns_Enabled,XDNS_Enable,Box_Type);
    if (!strncasecmp(g_cXdns_Enabled, "true", 4) || !strncasecmp(XDNS_Enable, "true", 4)) //If XDNS is ENABLED
    {
         char DNSSEC_FLAG[8]={0};
         syscfg_get(NULL, "XDNS_DNSSecEnable", DNSSEC_FLAG, sizeof(DNSSEC_FLAG));
         if ((!strncmp(Box_Type, "CGA4332COM", 10) || !strncmp(Box_Type, "CGA4131COM", 10)) && !strncasecmp(l_cXdnsEnable, "1", 1) && !strncasecmp(DNSSEC_FLAG, "1", 1))
         {
             if(!strncmp(l_cXdnsRefacCodeEnable, "1", 1))
             {
                 safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C %s %s --dhcp-authoritative --proxy-dnssec --cache-size=0 --xdns-refac-code",SERVER, DHCP_CONF,dnsOption);
                 if(safec_rc < EOK)
                 {
                     ERR_CHK(safec_rc);
                 }
             }
             else
             {
                 safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C %s %s --dhcp-authoritative --proxy-dnssec --cache-size=0 --stop-dns-rebind --log-facility=/rdklogs/logs/dnsmasq.log",SERVER, DHCP_CONF,dnsOption);
                 if(safec_rc < EOK)
                 {
                     ERR_CHK(safec_rc);
                 }
             }
         }
         else
         {
             if(!strncmp(l_cXdnsRefacCodeEnable, "1", 1) && !strncasecmp(l_cXdnsEnable, "1", 1))
             {
               safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C %s %s --dhcp-authoritative --xdns-refac-code  --stop-dns-rebind --log-facility=/rdklogs/logs/dnsmasq.log",SERVER, DHCP_CONF,dnsOption);
               if(safec_rc < EOK)
               {
                  ERR_CHK(safec_rc);
               }
             }
             else
             {
               safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C %s %s --dhcp-authoritative --stop-dns-rebind --log-facility=/rdklogs/logs/dnsmasq.log ",SERVER, DHCP_CONF,dnsOption);
               if(safec_rc < EOK)
               {
                  ERR_CHK(safec_rc);
               }
             }
         }
    }
    else // XDNS not enabled
    {
        safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s -P 4096 -C %s",SERVER, DHCP_CONF);
        if(safec_rc < EOK)
        {
          ERR_CHK(safec_rc);
        }
    }
#endif
    return executeCmd(l_cSystemCmd);
}

#if !defined (FEATURE_RDKB_DHCP_MANAGER)
void dhcp_server_stop()
{
        char l_cDhcp_Status[16] = {0}, l_cSystemCmd[255] = {0};
        int l_iSystem_Res;
        errno_t safec_rc = -1;
        fprintf(g_fArmConsoleLog,"\n%s Waiting for dhcp server end state\n",__FUNCTION__);
        wait_till_end_state("dhcp_server");
        fprintf(g_fArmConsoleLog,"\n%s dhcp server ended\n",__FUNCTION__);
   	sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", l_cDhcp_Status, sizeof(l_cDhcp_Status));	
	if (!strncmp(l_cDhcp_Status, "stopped", 7))
	{
		fprintf(g_fArmConsoleLog, "DHCP SERVER is already stopped not doing anything\n");
		return;
	}

#ifdef RDKB_EXTENDER_ENABLED
    if (Get_Device_Mode() == EXTENDER_MODE)
    {
        // Device is extender, check if ipv4 and mesh link are ready
        char l_cMeshWanLinkStatus[16] = {0};

        sysevent_get(g_iSyseventfd, g_tSysevent_token, "mesh_wan_linkstatus", l_cMeshWanLinkStatus, sizeof(l_cMeshWanLinkStatus));

        if ( strncmp(l_cMeshWanLinkStatus, "up", 2) != 0 )
        {
            fprintf(g_fArmConsoleLog, "mesh_wan_linkstatus and ipv4_connection_state is not up\n");
            return;
        }
    }
#endif

	//dns is always running
   	prepare_hostname();
   	prepare_dhcp_conf("dns_only");
	
	safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s unsetproc dhcp_server", PMON);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }
	l_iSystem_Res = v_secure_system("%s",l_cSystemCmd); //dnsmasq command
    if (0 != l_iSystem_Res)
	{
		fprintf(g_fArmConsoleLog, "%s command didnt execute successfully\n", l_cSystemCmd);
	}

	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "stopped", 0);
   	v_secure_system("killall `basename dnsmasq`");
    if (access(PID_FILE, F_OK) == 0) {
        remove_file(PID_FILE);
    }
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "stopped", 0);

	memset(l_cSystemCmd, 0x00, sizeof(l_cSystemCmd));

    l_iSystem_Res = dnsmasq_server_start(); //dnsmasq command
    if (0 == l_iSystem_Res)
    {
        fprintf(g_fArmConsoleLog, "dns-server started successfully\n");
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
    }
	else
	{
        fprintf(g_fArmConsoleLog, "dns-server didnt start\n");
	}
}
#endif

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
	{
            printf ("\ninterface search res : %s\n",interface);
	    fclose(fp); /* CID -43141 : Resource leak */
            return TRUE;
	}    
    }

    fprintf(g_fArmConsoleLog, "dnsmasq.conf does not have any interfaces\n");
    fclose(fp); /* CID -43141 : Resource leak */
    return FALSE;
}

int syslog_restart_request()
{
    fprintf(g_fArmConsoleLog,"\n Inside %s function\n",__FUNCTION__);
    char l_cSyscfg_get[16] = {0};
    int l_cRetVal=0;
    char Dhcp_server_status[10]={0};
    int l_crestart=0;
    char l_cCurrent_PID[8] = {0};

    sysevent_get(g_iSyseventfd, g_tSysevent_token,"dhcp_server-status", Dhcp_server_status, sizeof(Dhcp_server_status));
    if(strncmp(Dhcp_server_status,"started",7))
    {
        fprintf(g_fArmConsoleLog, "SERVICE DHCP : Return from syslog_restart_request as the event status is not started \n");
        return 0;
    }
    
    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-errinfo", "", 0);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server_errinfo", "", 0);
    wait_till_end_state("dns");
    wait_till_end_state("dhcp_server");

    copy_file(DHCP_CONF, "/tmp/dnsmasq.conf.orig");
    syscfg_get(NULL, "dhcp_server_enabled", l_cSyscfg_get, sizeof(l_cSyscfg_get));
    if (!strncmp(l_cSyscfg_get, "0", 1))
    {
        prepare_hostname();
        prepare_dhcp_conf("dns_only");
    }
    else
    {
        prepare_hostname();
        prepare_dhcp_conf(NULL);
        //no use of Sanitize lease file 
    }
    memset(l_cSyscfg_get,0,16);
    if(access(DHCP_CONF, F_OK) != -1 && access(DHCP_TMP_CONF, F_OK) != -1)
    {
        FILE *fp = NULL;
        if (FALSE == compare_files(DHCP_CONF, DHCP_TMP_CONF)) //Files are not identical
        {
                fprintf(g_fArmConsoleLog, "files are not identical restart dnsmasq\n");
                l_crestart=1;
        }
        else
        {
            fprintf(g_fArmConsoleLog, "files are identical not restarting dnsmasq\n");
        }
        fp = fopen(PID_FILE, "r");

        if (NULL == fp) //Mostly the error could be ENOENT(errno 2) 
        {
            fprintf(g_fArmConsoleLog, "Error:%d while opening file:%s\n", errno, PID_FILE);
        }
        else
        {
            fgets(l_cCurrent_PID, sizeof(l_cCurrent_PID), fp);
            //fclose(l_fFp); /*RDKB-12965 & CID:-34555*/
        }
        if (0 == l_cCurrent_PID[0])
        {
            l_crestart = 1;
        }
        else
        {
            char l_cBuf[128] = {0};
            char *l_cToken = NULL;
            FILE *fp1 = NULL;
	    fp1 = v_secure_popen("r","pidof dnsmasq");
	    if(!fp1)
            {
		    fprintf(g_fArmConsoleLog, "%s Failed in opening pipe \n", __FUNCTION__);
	    }
	    else
	    {
                copy_command_output(fp1, l_cBuf, sizeof(l_cBuf));
		v_secure_pclose(fp1);
	    }

            l_cBuf[strlen(l_cBuf)] = '\0';

            if ('\0' == l_cBuf[0] || 0 == l_cBuf[0])
            {
                l_crestart = 1;
            }
            else
            {
                //strstr to check PID didnt work, so had to use strtok
                int l_bPid_Present = 0;

                l_cToken = strtok(l_cBuf, " ");
                while (l_cToken != NULL)
                {
                    if (!strncmp(l_cToken, l_cCurrent_PID, strlen(l_cToken)))
                    {
                        l_bPid_Present = 1;
                        break;
                    }
                    l_cToken = strtok(NULL, " ");
                }
                if (0 == l_bPid_Present)
                {
                    fprintf(g_fArmConsoleLog, "PID:%d is not part of PIDS of dnsmasq\n", atoi(l_cCurrent_PID));
                    l_crestart = 1;
                }
                else
                {
                    fprintf(g_fArmConsoleLog, "PID:%d is part of PIDS of dnsmasq\n", atoi(l_cCurrent_PID));
                }
            }
        }
        if (access(DHCP_TMP_CONF, F_OK) == 0)
        {
            remove_file(DHCP_TMP_CONF);
        }
        v_secure_system("killall -HUP `basename dnsmasq`");
        if(l_crestart == 0)
        {
            return -1; // or return need to confirm
        }
        v_secure_system("killall `basename dnsmasq`");
        if (access(PID_FILE, F_OK) == 0) {
            remove_file(PID_FILE);
        }

        memset(l_cSyscfg_get,0,16);
        syscfg_get(NULL, "dhcp_server_enabled", l_cSyscfg_get, sizeof(l_cSyscfg_get));
        if(!strncmp(l_cSyscfg_get,"0",1))
        {
            l_cRetVal=dnsmasq_server_start();
            fprintf(g_fArmConsoleLog, "\n%s dnsmasq_server_start returns %d\n", __FUNCTION__,l_cRetVal);
            sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
        }
        else
        {
            //we use dhcp-authoritative flag to indicate that this is
            //the only dhcp server on the local network. This allows
            //the dns server to give out a _requested_ lease even if
            //that lease is not found in the dnsmasq.leases file
            //Get the DNS strict order option
            l_cRetVal=dnsmasq_server_start();
            fprintf(g_fArmConsoleLog, "\n%s dnsmasq_server_start returns %d\n", __FUNCTION__,l_cRetVal);
            //DHCP_SLOW_START_NEEDED is always false / set to false so below code is removed
            /*if [ "1" = "$DHCP_SLOW_START_NEEDED" ] && [ -n "$TIME_FILE" ]; then
            echo "#!/bin/sh" > $TIME_FILE
            echo "   sysevent set dhcp_server-restart lan_not_restart" >> $TIME_FILE
            chmod 700 $TIME_FILE
            fi*/
            sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
            sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "started", 0);
        }
    }
    return 0;
}

#if !defined (FEATURE_RDKB_DHCP_MANAGER)
int dhcp_server_start (char *input)
{
        fprintf(g_fArmConsoleLog, "\nInside  %s function with arg %s\n", __FUNCTION__,input);
	//Declarations
	char l_cDhcpServerEnable[16] = {0}, l_cLanStatusDhcp[16] = {0};
	char l_cSystemCmd[255] = {0}, l_cPsm_Mode[8] = {0}, l_cStart_Misc[8] = {0};
	char l_cPmonCmd[255] = {0}, l_cDhcp_Tmp_Conf[32] = {0};
	char l_cCurrent_PID[8] = {0}, l_cRpc_Cmd[64] = {0};
	char l_cBuf[128] = {0};
    char l_cBridge_Mode[8] = {0};
    char l_cDhcp_Server_Prog[16] = {0};
    int dhcp_server_progress_count = 0;

	BOOL l_bRestart = FALSE, l_bFiles_Diff = FALSE, l_bPid_Present = FALSE;
	FILE *l_fFp = NULL;
	int l_iSystem_Res;
        FILE *fptr = NULL;
	char *l_cToken = NULL;
	errno_t safec_rc = -1;

	service_dhcp_init();

    // DHCP Server Enabled
    syscfg_get(NULL, "dhcp_server_enabled", l_cDhcpServerEnable, sizeof(l_cDhcpServerEnable));

	if (!strncmp(l_cDhcpServerEnable, "0", 1))
	{
      	//when disable dhcp server in gui, we need remove the corresponding process in backend, 
		// or the dhcp server still work.
		fprintf(g_fArmConsoleLog, "DHCP Server is disabled not proceeding further\n");
		dhcp_server_stop();
		remove_file("/var/tmp/lan_not_restart");
		sysevent_set(g_iSyseventfd, g_tSysevent_token, 
					 "dhcp_server-status", "error", 0);

		sysevent_set(g_iSyseventfd, g_tSysevent_token, 
					 "dhcp_server-errinfo", "dhcp server is disabled by configuration", 0);
      	return 0;
	}
	
#ifdef RDKB_EXTENDER_ENABLED
    if (Get_Device_Mode() == EXTENDER_MODE)
    {
        // Device is extender, check if ipv4 and mesh link are ready
        char l_cMeshWanLinkStatus[16] = {0};

        sysevent_get(g_iSyseventfd, g_tSysevent_token, "mesh_wan_linkstatus", l_cMeshWanLinkStatus, sizeof(l_cMeshWanLinkStatus));	
    
        if ( strncmp(l_cMeshWanLinkStatus, "up", 2) != 0 )
        {
            fprintf(g_fArmConsoleLog, "mesh_wan_linkstatus and ipv4_connection_state is not up\n");
            return 1;
        }
    }
#endif
	
        sysevent_get(g_iSyseventfd, g_tSysevent_token,
                         "bridge_mode", l_cBridge_Mode,
                         sizeof(l_cBridge_Mode));

	//LAN Status DHCP
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_status-dhcp", l_cLanStatusDhcp, sizeof(l_cLanStatusDhcp));	
	if (strncmp(l_cLanStatusDhcp, "started", 7) && ( 0 == atoi(l_cBridge_Mode) ) )
	{
		fprintf(g_fArmConsoleLog, "lan_status-dhcp is not started return without starting DHCP server\n");
		remove_file("/var/tmp/lan_not_restart");
		return 0;
	}
   
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", l_cDhcp_Server_Prog, sizeof(l_cDhcp_Server_Prog));
    while((!(strncmp(l_cDhcp_Server_Prog, "inprogress", 10))) && (dhcp_server_progress_count < 5))
    {
        fprintf(g_fArmConsoleLog, "SERVICE DHCP : dhcp_server-progress is inprogress , waiting... \n");
        sleep(2);
        sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", l_cDhcp_Server_Prog, sizeof(l_cDhcp_Server_Prog));
        dhcp_server_progress_count++;
    }

    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "inprogress", 0);
	fprintf(g_fArmConsoleLog, "SERVICE DHCP : dhcp_server-progress is set to inProgress from dhcp_server_start \n");
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-errinfo", "", 0);
   
	strncpy(l_cDhcp_Tmp_Conf, "/tmp/dnsmasq.conf.orig", sizeof(l_cDhcp_Tmp_Conf));
    if (access(DHCP_CONF, F_OK) == 0) {
        copy_file(DHCP_CONF, l_cDhcp_Tmp_Conf);
    }

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
		fprintf(g_fArmConsoleLog, "files are not identical restart dnsmasq\n");
		l_bRestart = TRUE;
	}
	else
	{
		fprintf(g_fArmConsoleLog, "files are identical not restarting dnsmasq\n");
	}
	
	l_fFp = fopen(PID_FILE, "r");
	if (NULL == l_fFp) //Mostly the error could be ENOENT(errno 2) 
	{
		fprintf(g_fArmConsoleLog, "Error:%d while opening file:%s\n", errno, PID_FILE); 
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
		fptr = v_secure_popen("r","pidof dnsmasq");
		if(!fptr)
	        {
			fprintf(g_fArmConsoleLog, "%s Error in opening pipe\n",__FUNCTION__);
		}
		else
		{
                     copy_command_output(fptr, l_cBuf, sizeof(l_cBuf));
		     v_secure_pclose(fptr);
		}
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
                if (strcmp(l_cToken, l_cCurrent_PID) == 0)
                {
                    l_bPid_Present = TRUE;
                    break;
                }
                l_cToken = strtok(NULL, " ");
            }
            if (FALSE == l_bPid_Present)
            {
                fprintf(g_fArmConsoleLog, "PID:%d is not part of PIDS of dnsmasq\n", atoi(l_cCurrent_PID));
                l_bRestart = TRUE;
            }
            else
            {
                fprintf(g_fArmConsoleLog, "PID:%d is part of PIDS of dnsmasq\n", atoi(l_cCurrent_PID));
            }
		}
	}
    if (access(l_cDhcp_Tmp_Conf, F_OK) == 0)
    {
        remove_file(l_cDhcp_Tmp_Conf);
    }
   	v_secure_system("killall -HUP `basename dnsmasq`");
	if (FALSE == l_bRestart)
	{
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "started", 0);
        sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "completed", 0);
		remove_file("/var/tmp/lan_not_restart");
		return 0;
	}

	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "stopped", 0);
   	v_secure_system("killall `basename dnsmasq`");
    if (access(PID_FILE, F_OK) == 0) {
        remove_file(PID_FILE);
    }

        /* Kill dnsmasq if its not stopped properly */
	fptr = v_secure_popen("r","pidof dnsmasq");
        memset (l_cBuf, '\0',  sizeof(l_cBuf));
	if(!fptr)
	{
		fprintf(g_fArmConsoleLog, "%s Error in opening pipe\n",__FUNCTION__);
	}
	else
	{
            copy_command_output(fptr, l_cBuf, sizeof(l_cBuf));
	    v_secure_pclose(fptr);
	}
	l_cBuf[strlen(l_cBuf)] = '\0';

	if ('\0' != l_cBuf[0] && 0 != l_cBuf[0])
        {
            fprintf(g_fArmConsoleLog, "kill dnsmasq with SIGKILL if its still running \n");
            v_secure_system("kill -KILL `pidof dnsmasq`");
        }

    // TCCBR:4710- In Bridge mode, Dont run dnsmasq when there is no interface in dnsmasq.conf
    if ((strncmp(l_cBridge_Mode, "0", 1)) && (FALSE == IsDhcpConfHasInterface()))
    {
        fprintf(g_fArmConsoleLog, "no interface present in dnsmasq.conf %s process not started\n", SERVER);
        safec_rc = sprintf_s(l_cSystemCmd, sizeof(l_cSystemCmd),"%s unsetproc dhcp_server", PMON);
        if(safec_rc < EOK){
            ERR_CHK(safec_rc);
        }
        l_iSystem_Res = v_secure_system("%s",l_cSystemCmd); //dnsmasq command
        if (0 != l_iSystem_Res)
        {
            fprintf(g_fArmConsoleLog, "%s command didnt execute successfully\n", l_cSystemCmd);
        }
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "stopped", 0);
        sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "completed", 0);
		remove_file("/var/tmp/lan_not_restart");
        return 0;
    }
#if defined _BWG_NATIVE_TO_RDKB_REQ_
	/*Run script to reolve the IP address when upgrade from native to rdkb case only */
	v_secure_system("sh /etc/utopia/service.d/migration_native_rdkb.sh ");
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
    	fprintf(g_fArmConsoleLog, "%s process started successfully\n", SERVER);
	}
	else
	{
		if ((!strncmp(g_cBox_Type, "XB6", 3)) || 
			(!strncmp(g_cBox_Type, "TCCBR", 3))) //XB6 / TCCBR case 5 retries are needed
		{
			for (l_iDnamasq_Retry = 0; l_iDnamasq_Retry < 5; l_iDnamasq_Retry++)
			{
            	fprintf(g_fArmConsoleLog, "%s process failed to start sleep for 5 sec and restart it\n", SERVER);
	            sleep(5);
				l_iSystem_Res = dnsmasq_server_start(); //dnsmasq command
                            fprintf(g_fArmConsoleLog, "\n%s dnsmasq_server_start returns %d .......\n", __FUNCTION__,l_iSystem_Res);
			    if (0 == l_iSystem_Res)
			    {
    				fprintf(g_fArmConsoleLog, "%s process started successfully\n", SERVER);
					break;
			    }
				else
				{
    				fprintf(g_fArmConsoleLog, "%s process did not start successfully\n", SERVER);
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
                v_secure_system("gw_lan_refresh &");
        	}
		}
     	else
		{
          	fprintf(g_fArmConsoleLog, "lan_not_restart found! Don't restart lan!\n");
			remove_file("/var/tmp/lan_not_restart");	
		}
	}

        FILE *fp = fopen( "/tmp/dhcp_server_start", "r");
        if( NULL == fp )
        {
            print_with_uptime("dhcp_server_start is called for the first time private LAN initization is complete");
            fp = fopen( "/tmp/dhcp_server_start", "w+");
            if ( NULL == fp) // If file not present
            {
                fprintf(g_fArmConsoleLog, "File: /tmp/dhcp_server_start creation failed with error:%d\n", errno);

            }
            else
            {
                fclose(fp);
            }
            print_uptime("boot_to_ETH_uptime",NULL, NULL);
            print_with_uptime("LAN initization is complete notify SSID broadcast");
            snprintf(l_cRpc_Cmd, sizeof(l_cRpc_Cmd), "rpcclient %s \"/bin/touch /tmp/.advertise_ssids\"", g_cAtom_Arping_IP);
            executeCmd(l_cRpc_Cmd);
        }
        else
        {
            fclose(fp);
        }
    // This function is called for brlan0 and brlan1
    // If brlan1 is available then XHS service is available post all DHCP configuration   
    if (is_iface_present(XHS_IF_NAME))
    {   
        fprintf(g_fArmConsoleLog, "Xfinityhome service is UP\n");
        FILE *fp = fopen( "/tmp/xhome_start", "r");
        if( NULL == fp )
        {
            fp = fopen( "/tmp/xhome_start", "w+");
            if ( NULL == fp)
            {
                fprintf(g_fArmConsoleLog, "File: /tmp/xhome_start creation failed with error:%d\n", errno);
            }
            else
            {
                fclose(fp);
            }				
            print_uptime("boot_to_XHOME_uptime",NULL, NULL);
        }
        else
        {
            fclose(fp);
        }
    }
    else
    {   
        fprintf(g_fArmConsoleLog, "Xfinityhome service is not UP yet\n");
    }

	safec_rc = sprintf_s(l_cPmonCmd, sizeof(l_cPmonCmd),"%s setproc dhcp_server %s %s \"%s dhcp_server-restart\"", 
			PMON, BIN, PID_FILE, THIS);
    if(safec_rc < EOK){
       ERR_CHK(safec_rc);
    }

    executeCmd(l_cPmonCmd);

    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-status", "started", 0);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress", "completed", 0);
   	print_with_uptime("DHCP SERVICE :dhcp_server-progress_is_set_to_completed:");
   	fprintf(g_fArmConsoleLog, "RDKB_DNS_INFO is : -------  resolv_conf_dump  -------\n");
	print_file(RESOLV_CONF);
        fprintf(g_fArmConsoleLog,"\n %s function ENDS\n",__FUNCTION__);
	return 0;
}
#endif

void resync_to_nonvol(char *RemPools)
{
    fprintf(g_fArmConsoleLog,"\nInside %s function with arg %s\n",__FUNCTION__,RemPools);
    char Pool_List[6][40]={"dmsb.dhcpv4.server.pool.%s.Enable",
                           "dmsb.dhcpv4.server.pool.%s.IPInterface",
                           "dmsb.dhcpv4.server.pool.%s.MinAddress",
                           "dmsb.dhcpv4.server.pool.%s.MaxAddress",
                           "dmsb.dhcpv4.server.pool.%s.SubnetMask",
                           "dmsb.dhcpv4.server.pool.%s.LeaseTime"};
        //0-S_Enable,1-Ipv4Inst,2-StartAddr,3-EndAddr,4-SubNet,5-LeaseTime
    char Pool_Values[6][16]={0};
    char l_cSystemCmd[255]={0};
    async_id_t l_sAsyncID,l_sAsyncID_setcallback;
    //15 pools max
    char REM_POOLS[15][2]={0},CURRENT_POOLS[15][2]={0},LOAD_POOLS[15][2]={0},NV_INST[15][2]={0},tmp_buff[15][2]={0};
    int iter,iter1,match_found,tmp_cnt=0,ret_val,CURRENT_POOLS_cnt=0,NV_INST_cnt=0,REM_POOLS_cnt=0;
    char CUR_IPV4[16]={0},sg_buff[100]={0};
    char asyn[100]={0};
    char l_sAsyncString[120];
    FILE *pipe =NULL;
    if (RemPools == NULL)
    {
	pipe = v_secure_popen("r","sysevent get dhcp_server_current_pools");
        if(!pipe)
        {
            fprintf(g_fArmConsoleLog, "%s Failed in opening pipe \n", __FUNCTION__);
	}
        else
        {  
            CURRENT_POOLS_cnt=get_Pool_cnt(CURRENT_POOLS,pipe);
	    v_secure_pclose(pipe);
        }
	pipe = v_secure_popen("r","psmcli getallinst dmsb.dhcpv4.server.pool.");
        if(!pipe)
        {
            fprintf(g_fArmConsoleLog, "%s Failed in opening pipe \n", __FUNCTION__);
	}
        else
        {
            NV_INST_cnt=get_Pool_cnt(NV_INST,pipe);
	    v_secure_pclose(pipe);
        }
        if(CURRENT_POOLS_cnt != -1 || NV_INST_cnt != -1)
        {
            memcpy(REM_POOLS,CURRENT_POOLS,sizeof(CURRENT_POOLS[0][0])*15*2);
            memcpy(LOAD_POOLS,NV_INST,sizeof(NV_INST[0][0])*15*2);
        }
        else
        {
            CURRENT_POOLS_cnt=0;
            NV_INST_cnt=0;
        }
    }
    else
    {
        //As of now none of the functions are using REM_POOLS as argument. Implementation will be done later if needed.
        fprintf(g_fArmConsoleLog,"\nSince this function with rempools parameter is not used by anyone, depricated the implemenation");
    }
    if(NV_INST_cnt ==0 && CURRENT_POOLS_cnt ==0 )
    {
        fprintf(g_fArmConsoleLog,"\nNumber of pools available is 0");
        return;
    }
    for(iter=0;iter<CURRENT_POOLS_cnt;iter++)
    {
        match_found=0;
        for(iter1=0;iter1<NV_INST_cnt;iter1++)
        {
            if(strncmp(LOAD_POOLS[iter1],REM_POOLS[iter],2) ==0)
            {
                match_found++;
            }
        }
        if (match_found == 0)
        {
            strncpy(tmp_buff[tmp_cnt++],REM_POOLS[iter],2);
        }
    }
    memset(REM_POOLS,0,sizeof(REM_POOLS[0][0])*15*2);
    memcpy(REM_POOLS,tmp_buff,sizeof(REM_POOLS[0][0])*15*2);
    memset(tmp_buff,0,sizeof(tmp_buff[0][0])*15*2);

    REM_POOLS_cnt=tmp_cnt;
    tmp_cnt=0;
    match_found=0;


	for(iter=0;iter<NV_INST_cnt;iter++)
	{
	    memset(Pool_Values,0,sizeof(Pool_Values[0][0])*6*16);
		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%d_ipv4inst",atoi(LOAD_POOLS[iter]));
		sysevent_get(g_iSyseventfd, g_tSysevent_token, sg_buff, CUR_IPV4, sizeof(CUR_IPV4));


		//psmcli to get all the details
		for(iter1=0;iter1<6;iter1++)
		{
			memset(sg_buff,0,sizeof(sg_buff));
			snprintf(sg_buff,sizeof(sg_buff),Pool_List[iter1],LOAD_POOLS[iter]);
			ret_val=get_PSM_VALUES_FOR_POOL(sg_buff,Pool_Values[iter1]);
			if(ret_val != 0)
			{
				fprintf(g_fArmConsoleLog,"\nFailed to copy values if %s",sg_buff);
			}
		}


		if(strncmp(CUR_IPV4,Pool_Values[1],sizeof(CUR_IPV4)) != 0 && strncmp(CUR_IPV4,"",1)) // Pool_Values[1]=NewInst
		{
            snprintf(l_cSystemCmd,sizeof(l_cSystemCmd),"sysevent rm_async \"`sysevent get dhcp_server_%s-ipv4async`\"",LOAD_POOLS[iter]);
			v_secure_system("%s", l_cSystemCmd);
		}

        //enabled
		memset(sg_buff,0,sizeof(sg_buff));
		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_enabled",LOAD_POOLS[iter]);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[0], 0);
		//IPInterface
		memset(sg_buff,0,sizeof(sg_buff));
		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_ipv4inst",LOAD_POOLS[iter]);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[1], 0);
		//MinAddress
		memset(sg_buff,0,sizeof(sg_buff));
		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_startaddr",LOAD_POOLS[iter]);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[2], 0);
		//MaxAddress
		memset(sg_buff,0,sizeof(sg_buff));
		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_endaddr",LOAD_POOLS[iter]);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[3], 0);
		//SubnetMask
		memset(sg_buff,0,sizeof(sg_buff));
		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_subnet",LOAD_POOLS[iter]);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[4], 0);
		//LeaseTime
		memset(sg_buff,0,sizeof(sg_buff));
		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_leasetime",LOAD_POOLS[iter]);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[5], 0);
	}

	if(REM_POOLS_cnt > 0)
	{
		for(iter=0;iter<REM_POOLS_cnt;iter++)
	    {
			memset(Pool_Values,0,sizeof(Pool_Values[0][0])*6*16);
 		    snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%d_ipv4inst",atoi(REM_POOLS[iter]));
		    sysevent_get(g_iSyseventfd, g_tSysevent_token, sg_buff, CUR_IPV4, sizeof(CUR_IPV4));


		//psmcli to get all the details
		    for(iter1=0;iter1<6;iter1++)
		    {
			    memset(sg_buff,0,sizeof(sg_buff));
			    snprintf(sg_buff,sizeof(sg_buff),Pool_List[iter1],REM_POOLS[iter]);
			    ret_val=get_PSM_VALUES_FOR_POOL(sg_buff,Pool_Values[iter1]);
			    if(!ret_val)
			    {
				    fprintf(g_fArmConsoleLog,"Failed to copy values if %s",sg_buff);
			    }
		    }

		    if(strncmp(CUR_IPV4,Pool_Values[1],sizeof(CUR_IPV4)) != 0 && strncmp(CUR_IPV4,"",1)) // Pool_Values[1]=NewInst
		    {
                        memset(sg_buff,0,sizeof(sg_buff));
                        snprintf(sg_buff, sizeof(sg_buff), "dhcp_server_%s-ipv4async", REM_POOLS[iter]);
                        sysevent_get(g_iSyseventfd, g_tSysevent_token, sg_buff,l_sAsyncString, sizeof(l_sAsyncString));
                        sscanf(l_sAsyncString, "%d %d", &l_sAsyncID.trigger_id, &l_sAsyncID.action_id);
                        sysevent_rmcallback(g_iSyseventfd, g_tSysevent_token, l_sAsyncID);
                    }

                    //enabled
		    memset(sg_buff,0,sizeof(sg_buff));
		    snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_enabled",REM_POOLS[iter]);
		    sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[0], 0);

		//IPInterface
		    memset(sg_buff,0,sizeof(sg_buff));
		    snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_ipv4inst",REM_POOLS[iter]);
		    sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[1], 0);

		//MinAddress
		    memset(sg_buff,0,sizeof(sg_buff));
		    snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_startaddr",REM_POOLS[iter]);
		    sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[2], 0);

		//MaxAddress
		    memset(sg_buff,0,sizeof(sg_buff));
		    snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_endaddr",REM_POOLS[iter]);
		    sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[3], 0);

		//SubnetMask
		    memset(sg_buff,0,sizeof(sg_buff));
		    snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_subnet",REM_POOLS[iter]);
		    sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[4], 0);

		//LeaseTime
		    memset(sg_buff,0,sizeof(sg_buff));
		    snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s_leasetime",REM_POOLS[iter]);
		    sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff,Pool_Values[5], 0);
        }
	}

	// Remove LOAD_POOLS and REM_POOLS from CURRENT_POOLS
	for(iter=0;iter<CURRENT_POOLS_cnt;iter++)
    {
        match_found=0;
	    for(iter1=0;iter1<NV_INST_cnt;iter1++)
	    {
	            if(strncmp(LOAD_POOLS[iter1],CURRENT_POOLS[iter],2) ==0)
		    {
		        match_found++;
		    }
	    }
	    if (match_found == 0)
	    {
	        strncpy(tmp_buff[tmp_cnt++],CURRENT_POOLS[iter],2);
	    }
    }
    memset(CURRENT_POOLS,0,sizeof(CURRENT_POOLS[0][0])*15*2);
    memcpy(CURRENT_POOLS,tmp_buff,sizeof(CURRENT_POOLS[0][0])*15*2);
    memset(tmp_buff,0,sizeof(tmp_buff[0][0])*15*2);
    CURRENT_POOLS_cnt=tmp_cnt;

    for(iter=0;iter<CURRENT_POOLS_cnt;iter++)
    {
        match_found=0;
	    for(iter1=0;iter1<REM_POOLS_cnt;iter1++)
	    {
	        if(strncmp(REM_POOLS[iter1],CURRENT_POOLS[iter],2) ==0)
		    {
		        match_found++;
		    }
	    }
	    if (match_found == 0)
	    {
	        strncpy(tmp_buff[tmp_cnt++],CURRENT_POOLS[iter],2);
	    }
    }

    memset(CURRENT_POOLS,0,sizeof(CURRENT_POOLS[0][0])*15*2);
    memcpy(CURRENT_POOLS,tmp_buff,sizeof(CURRENT_POOLS[0][0])*15*2);
    memset(tmp_buff,0,sizeof(tmp_buff[0][0])*15*2);
    CURRENT_POOLS_cnt=tmp_cnt;        //Remove LOAD_POOLS and REM_POOLS from CURRENT_POOLS ENDS

    char psm_tmp_buff[2];
    char *l_cParam[1] = {0};
	for(iter=0;iter<NV_INST_cnt;iter++)
	{
		memset(psm_tmp_buff,0,sizeof(psm_tmp_buff));
		memset(sg_buff,0,sizeof(sg_buff));
		memset(asyn,0,sizeof(asyn));

		snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s-ipv4async",LOAD_POOLS[iter]);
		sysevent_get(g_iSyseventfd, g_tSysevent_token, sg_buff, asyn, sizeof(asyn));

	        memset(sg_buff,0,sizeof(sg_buff));
		snprintf(sg_buff,sizeof(sg_buff),Pool_List[1],LOAD_POOLS[iter]);
		ret_val=get_PSM_VALUES_FOR_POOL(sg_buff,psm_tmp_buff); // get the value for dmsb.dhcpv4.server.pool.%s.IPInterface
		if(!ret_val)
		{
		    fprintf(g_fArmConsoleLog,"\n Failed to copy values for %s",sg_buff);
		}
		if(strncmp(asyn,"",1) == 0)
		{
			#if (defined _COSA_INTEL_XB3_ARM_)
			    fprintf(g_fArmConsoleLog,"\nSERVICE DHCP : skip ipv4async event for xhome in xb3");
		    #else
				memset(l_cSystemCmd,0,sizeof(l_cSystemCmd));
                                snprintf(l_cSystemCmd, sizeof(l_cSystemCmd), "ipv4_%s-status", psm_tmp_buff);

                                sysevent_setcallback(g_iSyseventfd, g_tSysevent_token, ACTION_FLAG_NONE, l_cSystemCmd, THIS, 1, l_cParam, &l_sAsyncID_setcallback);
                                memset(l_cSystemCmd,0,sizeof(l_cSystemCmd));
                                snprintf(l_cSystemCmd, sizeof(l_cSystemCmd), "%d %d", l_sAsyncID_setcallback.action_id, l_sAsyncID_setcallback.trigger_id); //l_cAsyncIdstring is l_cSystemCmd here
                  
                                memset(sg_buff,0,sizeof(sg_buff));
                                snprintf(sg_buff,sizeof(sg_buff),"dhcp_server_%s-ipv4async",LOAD_POOLS[iter]);
                                sysevent_set(g_iSyseventfd, g_tSysevent_token, sg_buff, l_cSystemCmd, 0);
			#endif
		}
	}
	memset(sg_buff,0,sizeof(sg_buff));
	iter=0;
	while(strncmp(LOAD_POOLS[iter],"",1) != 0 || strncmp(CURRENT_POOLS[iter],"",1) != 0)
	{
		if( strncmp(CURRENT_POOLS[iter],"",1) != 0)
		{
			strcat(sg_buff,CURRENT_POOLS[iter]);
		        strcat(sg_buff," ");
		}
		if(strncmp(LOAD_POOLS[iter],"",1) != 0)
		{
			strcat(sg_buff,LOAD_POOLS[iter]);
			strcat(sg_buff," ");
		}
		iter++;
	}
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server_current_pools", sg_buff, 0);
        fprintf(g_fArmConsoleLog,"\n%s function ENDS \n",__FUNCTION__);
}

int service_dhcp_init()
{
        fprintf(g_fArmConsoleLog,"\nInside %s function\n",__FUNCTION__);
	char l_cPropagate_Ns[8] = {0}, l_cPropagate_Dom[8] = {0};
	char l_cSlow_Start[8] = {0}, l_cByoi_Enabled[8] = {0};
    char l_cWan_IpAddr[16] = {0}, l_cPrim_Temp_Ip_Prefix[16] = {0}, l_cCurrent_Hsd_Mode[16] = {0};
   	char l_cTemp_Dhcp_Lease[8] = {0}, l_cDhcp_Slow_Start_Quanta[8] = {0};
    char l_cDhcpSlowStartQuanta[8] = {0};

	syscfg_get(NULL, "dhcp_server_propagate_wan_nameserver", l_cPropagate_Ns, sizeof(l_cPropagate_Ns));
	if (strncmp(l_cPropagate_Ns, "1", 1))
	{
	    fprintf(g_fArmConsoleLog, "Propagate NS is set from block_nat_redirection value is:%s\n", l_cPropagate_Ns);
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
		fprintf(g_fArmConsoleLog, "DHCP Lease time is empty, set to default value 24h\n");
	    strncpy(g_cDhcp_Lease_Time, "24h", sizeof(g_cDhcp_Lease_Time));
	}

	get_device_props();
    return SUCCESS;
}

#if !defined (FEATURE_RDKB_DHCP_MANAGER)
void lan_status_change(char *input)
{

#ifdef RDKB_EXTENDER_ENABLED
    if (Get_Device_Mode() == EXTENDER_MODE)
    {
        // Device is extender, check if ipv4 and mesh link are ready
        char l_cMeshWanLinkStatus[16] = {0};

        sysevent_get(g_iSyseventfd, g_tSysevent_token, "mesh_wan_linkstatus", l_cMeshWanLinkStatus, sizeof(l_cMeshWanLinkStatus));

        if ( strncmp(l_cMeshWanLinkStatus, "up", 2) != 0 ) 
        {
            fprintf(g_fArmConsoleLog, "mesh_wan_linkstatus and ipv4_connection_state is not up\n");
            return;
        }
    }
#endif 
        fprintf(g_fArmConsoleLog,"\nInside %s function with arg=%s\n",__FUNCTION__,input);
	char l_cLan_Status[16] = {0}, l_cDhcp_Server_Enabled[8] = {0};
	int l_iSystem_Res;

	sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan-status", l_cLan_Status, sizeof(l_cLan_Status));
	fprintf(g_fArmConsoleLog, "SERVICE DHCP : Inside lan status change with lan-status:%s\n", l_cLan_Status);
   	fprintf(g_fArmConsoleLog, "SERVICE DHCP : Current lan status is:%s\n", l_cLan_Status);
    
	syscfg_get(NULL, "dhcp_server_enabled", l_cDhcp_Server_Enabled, sizeof(l_cDhcp_Server_Enabled));
	if (!strncmp(l_cDhcp_Server_Enabled, "0", 1))
	{
    	//set hostname and /etc/hosts cause we are the dns forwarder
        prepare_hostname();

        //also prepare dns part of dhcp conf cause we are the dhcp server too
        prepare_dhcp_conf("dns_only");

        fprintf(g_fArmConsoleLog, "SERVICE DHCP : Start dhcp-server from lan status change");
           
	    l_iSystem_Res = dnsmasq_server_start(); //dnsmasq command
    	if (0 == l_iSystem_Res)
	    {
    	    fprintf(g_fArmConsoleLog, "%s process started successfully\n", SERVER);
	    }
		else
		{
			fprintf(g_fArmConsoleLog, "%s process didn't start successfully\n", SERVER);
		}
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, "dns-status", "started", 0);
	}
    else
	{
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan_status-dhcp", "started", 0);
		if (NULL == input)
		{
	        fprintf(g_fArmConsoleLog, "SERVICE DHCP :  Call start DHCP server from lan status change with NULL\n");
			dhcp_server_start(NULL);
		}
		else
		{
			fprintf(g_fArmConsoleLog, "SERVICE DHCP :  Call start DHCP server from lan status change with input:%s\n", input);
            dhcp_server_start(input);
		}
	}
}
#endif
