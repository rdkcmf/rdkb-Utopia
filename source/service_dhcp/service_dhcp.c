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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <net/if.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "lan_handler.h"
#include "dhcp_server_functions.h"
#include "service_dhcp_server.h"
#include "service_ipv4.h"
#include "errno.h"
#include "util.h"

#define isValidSubnetByte(byte) (((byte == 255) || (byte == 254) || (byte == 252) || \
                                  (byte == 248) || (byte == 240) || (byte == 224) || \
                                  (byte == 192) || (byte == 128)) ? 1 : 0)

#define DEVICE_PROPS_FILE   	"/etc/device.properties"
#define BOOL 					int
#define TRUE 					1
#define FALSE 					0
#define MAXLINE 				150
#define THIS					"/usr/bin/service_dhcp"

#define ERROR	-1
#define SUCCESS	0		
#define IOT_SERVICE_PATH        "/etc/utopia/service.d"
#define SERVICE_MULTINET_PATH   "/etc/utopia/service.d/service_multinet"

#if defined (_XB6_PRODUCT_REQ_) || defined(_CBR_PRODUCT_REQ_) || defined (_XB7_PRODUCT_REQ_)
#define CONSOLE_LOG_FILE "/rdklogs/logs/Consolelog.txt.0"
#else
#define CONSOLE_LOG_FILE "/rdklogs/logs/ArmConsolelog.txt.0"
#endif

const char* const g_cComponent_id = "ccsp.servicedhcp";
void* g_vBus_handle = NULL;
FILE* g_fArmConsoleLog = NULL; //Global file pointer

int g_iSyseventfd;
token_t g_tSysevent_token;
char g_cDhcp_Lease_Time[8] = {0}, g_cTime_File[64] = {0};
char g_cBox_Type[8] = {0};
#ifdef XDNS_ENABLE
char g_cXdns_Enabled[8] = {0};
#endif
char g_cMfg_Name[8] = {0}, g_cAtom_Arping_IP[16] = {0};
char g_cMig_Check[8] = {0};

static int dbusInit( void )
{
    int ret = 0;
    char* pCfg = CCSP_MSG_BUS_CFG;

    if (g_vBus_handle == NULL)
    {
#ifdef DBUS_INIT_SYNC_MODE // Dbus connection init
        ret = CCSP_Message_Bus_Init_Synced(g_cComponent_id,
                                           pCfg,
                                           &g_vBus_handle,
                                           Ansc_AllocateMemory_Callback,
                                           Ansc_FreeMemory_Callback);
#else
        ret = CCSP_Message_Bus_Init((char *)g_cComponent_id,
                                    pCfg,
                                    &g_vBus_handle,
                                    (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback,
                                    Ansc_FreeMemory_Callback);
#endif  /* DBUS_INIT_SYNC_MODE */

        if (ret == -1)
        {
            // Dbus connection error
            fprintf(g_fArmConsoleLog, "DBUS connection error\n");
        }
    }
    return ret;
}

void print_with_uptime(const char* input)
{
    struct sysinfo l_sSysInfo;
    struct tm * l_sTimeInfo;
    time_t l_sNowTime;
    int l_iDays, l_iHours, l_iMins, l_iSec;
    char l_cLocalTime[128];

    sysinfo(&l_sSysInfo);
    time(&l_sNowTime);

    l_sTimeInfo = (struct tm *)gmtime(&l_sSysInfo.uptime); 
    l_iSec = l_sTimeInfo->tm_sec; 
    l_iMins = l_sTimeInfo->tm_min; 
    l_iHours = l_sTimeInfo->tm_hour; 
    l_iDays = l_sTimeInfo->tm_yday; 
    l_sTimeInfo = localtime(&l_sNowTime);

    snprintf(l_cLocalTime, sizeof(l_cLocalTime), "%02d:%02d:%02dup%02ddays:%02dhours:%02dmin:%02dsec", 
                           l_sTimeInfo->tm_hour, l_sTimeInfo->tm_min, l_sTimeInfo->tm_sec, 
                           l_iDays, l_iHours, l_iMins, l_iSec);

    fprintf(g_fArmConsoleLog, "%s%s\n", input,l_cLocalTime);
}

void get_device_props()
{
    FILE *l_fFp = NULL;
    l_fFp = fopen(DEVICE_PROPS_FILE, "r");

    if (NULL != l_fFp)
    {   
        char props[255] = {""};
        while(fscanf(l_fFp,"%s", props) != EOF )
        {
            char *property = NULL;
            if(NULL != (property = strstr(props, "BOX_TYPE=")))
            {
                property = property + strlen("BOX_TYPE=");
		/* CID 60267: Out-of-bounds access (OVERRUN)*/
                strncpy(g_cBox_Type, property, sizeof(g_cBox_Type)-1);
		g_cBox_Type[sizeof(g_cBox_Type)-1] = '\0';
            }
#ifdef XDNS_ENABLE
            if(NULL != (property = strstr(props, "XDNS_ENABLE=")))
            {
                property = property + strlen("XDNS_ENABLE=");
		/* CID 53527: Out-of-bounds access (OVERRUN)*/
                strncpy(g_cXdns_Enabled, property, sizeof(g_cXdns_Enabled)-1);
		g_cXdns_Enabled[sizeof(g_cXdns_Enabled)-1] = '\0';
            }
#endif
            if(NULL != (property = strstr(props, "MIG_CHECK=")))
            {
                property = property + strlen("MIG_CHECK=");
		/*CID 71049: Out-of-bounds access (OVERRUN)*/
                strncpy(g_cMig_Check, property, sizeof(g_cMig_Check)-1);
		g_cMig_Check[sizeof(g_cMig_Check)-1] = '\0';
            }
	    if(NULL != (property = strstr(props, "ATOM_ARPING_IP=")))
            {
                property = property + strlen("ATOM_ARPING_IP=");
		/* CID 74595 : Out-of-bounds access (OVERRUN)*/
                strncpy(g_cAtom_Arping_IP, property, sizeof(g_cAtom_Arping_IP)-1);
		g_cAtom_Arping_IP[sizeof(g_cAtom_Arping_IP)-1] = '\0';
            }
        }
        fclose(l_fFp);
    }   
}

int executeCmd(char *cmd)
{
	int l_iSystem_Res;
	l_iSystem_Res = system(cmd);
    if (0 != l_iSystem_Res && ECHILD != errno)
    {
        fprintf(g_fArmConsoleLog, "%s command didnt execute successfully\n", cmd);
        return l_iSystem_Res;
    }
    return 0;
}

void copy_file(char *input_file, char *target_file)
{
    char l_cLine[255] = {0};
    FILE *l_fTargetFile = NULL, *l_fInputFile = NULL;

	l_fInputFile = fopen(input_file, "r");
    l_fTargetFile = fopen(target_file, "w+"); //RDK-B 12160
    if ((NULL != l_fInputFile) && (NULL != l_fTargetFile))
    {
        while(fgets(l_cLine, sizeof(l_cLine), l_fInputFile) != NULL)
        {
            fputs(l_cLine, l_fTargetFile);
        }
    }
	else
	{
		fprintf(g_fArmConsoleLog, "copy of files failed due to error in opening one of the files \n");
	}

    if(l_fInputFile) {
	fclose(l_fInputFile);
    }

    if(l_fTargetFile) {
	fclose(l_fTargetFile);
    }
}

void remove_file(char *tb_removed_file)
{
    int l_iRemove_Res;
    l_iRemove_Res = remove(tb_removed_file);
    if (0 != l_iRemove_Res)
    {
        fprintf(g_fArmConsoleLog, "remove of %s file is not successful error is:%d\n", 
				tb_removed_file, errno);
    }
}

void print_file(char *to_print_file)
{
	char l_cLine[255] = {0};
    FILE *l_fP = NULL;

    l_fP = fopen(to_print_file, "r");
    if (NULL != l_fP)
    {   
        while(fgets(l_cLine, sizeof(l_cLine), l_fP) != NULL)
        {   
            fprintf(g_fArmConsoleLog, "%s", l_cLine);
        }   
        fclose(l_fP);
    }
}

void copy_command_output(FILE *fp, char *out, int len)
{
    char *l_cP = NULL;
    if (fp)
    {   
        fgets(out, len, fp);

        /*we need to remove the \n char in buf*/
        if ((l_cP = strchr(out, '\n'))) 
        {
	    *l_cP = 0;
	}
    }   
}

// If two files are identical it returns TRUE 
// If two files are not identical it returns FALSE
BOOL compare_files(char *input_file1, char *input_file2)
{
	FILE *l_fP1 = NULL, *l_fP2 = NULL; /* File Pointer Read, File Pointer Read */
    char *l_cpFgets_Res = NULL, *l_cpFgets_Res2 = NULL;
    char l_cFilebuff[MAXLINE];
    char l_cFilebuff2[MAXLINE];
    int l_cCmpRes, l_iLineNum = 0;

    l_fP1 = fopen(input_file1, "r");/* opens First file which is read */
    if (l_fP1 == NULL)
    {
        fprintf(g_fArmConsoleLog, "Can't open %s for reading\n", input_file1);
        return FALSE;
    }

    l_fP2 = fopen(input_file2, "r");/* opens Second file which is also read */
    if (l_fP2 == NULL)
    {
	fclose(l_fP1);
        fprintf(g_fArmConsoleLog, "Can't open %s for reading\n", input_file2);
        return FALSE;
    }

    l_cpFgets_Res = fgets(l_cFilebuff, MAXLINE, l_fP1);
    l_cpFgets_Res2 = fgets(l_cFilebuff2, MAXLINE, l_fP2);
    while (l_cpFgets_Res != NULL || l_cpFgets_Res2 != NULL)
    {
        ++l_iLineNum;
        l_cCmpRes = strcmp(l_cFilebuff, l_cFilebuff2);
        if (l_cCmpRes != 0)
        {
            fclose(l_fP1);
            fclose(l_fP2);
            return FALSE;
        }
        l_cpFgets_Res = fgets(l_cFilebuff, MAXLINE, l_fP1);
        l_cpFgets_Res2 = fgets(l_cFilebuff2, MAXLINE, l_fP2);
    }
    fclose(l_fP1);
    fclose(l_fP2);
    return TRUE;
}

void wait_till_end_state (char *process_to_wait)
{
    char l_cSysevent_Cmd[64] = {0}, l_cProcess_Status[16] = {0};
    int l_iTries;
    for (l_iTries = 1; l_iTries <= 9; l_iTries++)
    {
        snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
                 "sysevent get %s-status", process_to_wait);

    	sysevent_get(g_iSyseventfd, g_tSysevent_token, 
                     l_cSysevent_Cmd, l_cProcess_Status, sizeof(l_cProcess_Status));	
        if ((!strncmp(l_cProcess_Status, "starting", 8)) || 
            (!strncmp(l_cProcess_Status, "stopping", 8)))
        {
            sleep(1);
        }
        else
        {
            break;
        }
    }
}

void subnet(char *ipv4Addr, char *ipv4Subnet, char *subnet)
{
    int l_iFirstByte, l_iSecondByte, l_iThirdByte, l_iFourthByte;
    int l_iFirstByteSub, l_iSecondByteSub, l_iThirdByteSub, l_iFourthByteSub;

    sscanf(ipv4Addr, "%d.%d.%d.%d", &l_iFirstByte, &l_iSecondByte, 
           &l_iThirdByte, &l_iFourthByte);

    sscanf(ipv4Subnet, "%d.%d.%d.%d", &l_iFirstByteSub, &l_iSecondByteSub, 
           &l_iThirdByteSub, &l_iFourthByteSub);

    l_iFirstByte = l_iFirstByte & l_iFirstByteSub;
    l_iSecondByte = l_iSecondByte & l_iSecondByteSub;
    l_iThirdByte = l_iThirdByte & l_iThirdByteSub;
    l_iFourthByte = l_iFourthByte & l_iFourthByteSub;

    snprintf(subnet, 16, "%d.%d.%d.%d", l_iFirstByte, 
             l_iSecondByte, l_iThirdByte, l_iFourthByte);
}

unsigned int countSetBits(int byte)
{
    unsigned int l_iCount = 0;
    if (isValidSubnetByte(byte) || 0 == byte)
    {
        while (byte)
        {
            byte &= (byte-1);
            l_iCount++;
        }
        return l_iCount;
    }
    else
    {
        fprintf(g_fArmConsoleLog, "Invalid subnet byte:%d\n", byte);
        return 0;
    }
}

unsigned int mask2cidr(char *subnetMask)
{
    int l_iFirstByte, l_iSecondByte, l_iThirdByte, l_iFourthByte;
    int l_iCIDR = 0;

    sscanf(subnetMask, "%d.%d.%d.%d", &l_iFirstByte, &l_iSecondByte,
            &l_iThirdByte, &l_iFourthByte);

    l_iCIDR += countSetBits(l_iFirstByte);
    l_iCIDR += countSetBits(l_iSecondByte);
    l_iCIDR += countSetBits(l_iThirdByte);
    l_iCIDR += countSetBits(l_iFourthByte);
    return l_iCIDR;
}

int sysevent_syscfg_init()
{
	g_iSyseventfd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION,
                                               "service_dhcp", &g_tSysevent_token);
	g_fArmConsoleLog = fopen(CONSOLE_LOG_FILE, "a+");

	if (NULL == g_fArmConsoleLog) //In error case not returning as it is ok to continue
	{
		g_fArmConsoleLog = stderr; //Redirecting the messages to console terminal
		fprintf(g_fArmConsoleLog, "Error:%d while opening Log file:%s\n", errno, CONSOLE_LOG_FILE);
	}
	else
	{
		fprintf(g_fArmConsoleLog, "Successful in opening while opening Log file:%s\n", CONSOLE_LOG_FILE);
	}	

    if (g_iSyseventfd < 0)       
    {    
        fprintf(g_fArmConsoleLog, "service_dhcp::sysevent_open failed\n");
		return ERROR;
    }        

    /* dbus init based on bus handle value */
    if(g_vBus_handle ==  NULL)
        dbusInit();

    if(g_vBus_handle == NULL)
    {
        fprintf(g_fArmConsoleLog, "service_dhcp_init, DBUS init error\n");
        return ERROR;
    }
	return SUCCESS;
}

int main(int argc, char *argv[])
{
	char l_cL3Inst[8] = {0}, l_cSysevent_Cmd[255] = {0};
	int l_iL3Inst;
	if (argc < 2)	
	{
		fprintf(g_fArmConsoleLog, "Insufficient number of args return\n");
		return 0;
	}

	if (0 == g_iSyseventfd)
		sysevent_syscfg_init();
	
	fprintf(g_fArmConsoleLog, "%s case\n", argv[1]);
	if ((!strncmp(argv[1], "dhcp_server-start", 17)) ||
		(!strncmp(argv[1], "dhcp_server-restart", 19)))
	{
#if !defined (FEATURE_RDKB_DHCP_MANAGER)
		if (3 == argc)
		{	
			dhcp_server_start(argv[2]);
		}
		else
		{
			dhcp_server_start(NULL);
		}
#endif
	}
        else if(!strncmp(argv[1],"dhcp_server-stop",16))
        {
#if !defined (FEATURE_RDKB_DHCP_MANAGER)
             dhcp_server_stop();
#endif
        }
        else if (!strncmp(argv[1], "lan-status", 10))
	{
#if !defined (FEATURE_RDKB_DHCP_MANAGER)
		//If lan-status is called with lan_not_restart then 
		//the same is used in further function calls
		if (4 == argc)
		{	
			lan_status_change(argv[3]);
		}
		else
		{
			lan_status_change(NULL);
		}
#endif
	}
	else if (!strncmp(argv[1], "bring-lan", 9) || !strncmp(argv[1], "pnm-status", 10))
	{
		bring_lan_up();
	}
	#ifdef RDKB_EXTENDER_ENABLED
    	else if(!strncmp(argv[1], "dhcp_conf_change", 16))
    	{
      		UpdateDhcpConfChangeBasedOnEvent();
		dhcp_server_start(NULL);
    	}
      	#endif
	else if (!strncmp(argv[1], "lan-restart", 11))
	{
		lan_restart();
	}
        else if ((!strncmp(argv[1], "ipv4_4-status", 13)) ||
             (!strncmp(argv[1], "ipv4_5-status", 13)))
        {
                if (argc > 2)
                {
                        char buf[32] = {0};
                        char l_cL2Inst[8] = {0};
                        int l_iL2Inst;

                        sscanf(argv[1], "ipv4_%d-status", &l_iL3Inst);
                        if (4 == l_iL3Inst)
                        {
                                sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_handler_async",
                                             l_cL3Inst, sizeof(l_cL3Inst));
                                if (l_cL3Inst[0] != '\0')
                                {
                                        ipv4_status(l_iL3Inst, argv[2]);
                                }
                        }

                        snprintf(buf, sizeof(buf), "ipv4_%d-lower", l_iL3Inst);
                        sysevent_get(g_iSyseventfd, g_tSysevent_token, buf,
                                     l_cL2Inst, sizeof(l_cL2Inst));
                        l_iL2Inst = atoi(l_cL2Inst);
                        snprintf(buf, sizeof(buf), "dhcp_server_%d-ipv4async", l_iL2Inst);
                        sysevent_get(g_iSyseventfd, g_tSysevent_token,
                                     buf, l_cL2Inst, sizeof(l_cL2Inst));
                        if (l_cL2Inst[0] != '\0')
                        {
                                #if !defined (FEATURE_RDKB_DHCP_MANAGER)
                                lan_status_change("lan_not_restart");
                                #else
                                //Setting an event to start dhcp server with brlan1 as code is moved to CcspDHCPMgr component
                                //Further modifications will be taken care later
                                sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-start", "lan_not_restart", 0);
                                #endif
                        }
                }
                else
                {
                        fprintf(g_fArmConsoleLog, "Insufficient number of arguments for %s\n", argv[1]);
                }
	}
    else if (!strncmp(argv[1], "ipv4-resync", 11))
    {
        resync_instance(atoi(argv[2]));
    }
    else if (!strncmp(argv[1], "erouter_mode-updated", 20))
    {
        erouter_mode_updated();
    }
    else if (!strncmp(argv[1], "multinet-resync", 15))
    {
        snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),"dmcli eRT setv Device.WiFi.Radio.1.X_CISCO_COM_ApplySetting bool 'true' 'true'");
        executeCmd(l_cSysevent_Cmd);
    }
    else if (!strncmp(argv[1], "iot_status", 10))
    {
        fprintf(g_fArmConsoleLog, "IOT_LOG : lan_handler received %s status\n", argv[2]);
        if (!strncmp(argv[2],"up",2))
        {
            snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),"%s/handle_sw.sh %s", SERVICE_MULTINET_PATH, "addIotVlan 0 106 -t");
            executeCmd(l_cSysevent_Cmd);

            fprintf(g_fArmConsoleLog, "IOT_LOG : lan_handler done with handle_sw call\n");

            snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),"%s/iot_service.sh up", IOT_SERVICE_PATH);
            executeCmd(l_cSysevent_Cmd);
        }
        else if (!strncmp(argv[2],"down",4))
        {
            snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),"%s/iot_service.sh down", IOT_SERVICE_PATH);
            executeCmd(l_cSysevent_Cmd);
        }
        else if (!strncmp(argv[2],"bootup",6))
        {
            snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),"%s/iot_service.sh bootup", IOT_SERVICE_PATH);
            executeCmd(l_cSysevent_Cmd);
        }
    }
    else if (!strncmp(argv[1], "lan-stop", 8))
    {
        lan_stop();
    }
	else if (!strncmp(argv[1], "lan-start", 9))
	{
		sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 "primary_lan_l3net", l_cL3Inst, 
					 sizeof(l_cL3Inst));	

		l_iL3Inst = atoi(l_cL3Inst);
		fprintf(g_fArmConsoleLog, "Calling ipv4_up with L3 Instance:%d\n", l_iL3Inst);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "ipv4-up", l_cL3Inst, 0);
	}
	//service_ipv4.sh related
	else if(!strncmp(argv[1], "ipv4-up", 7)) 
    {   
        if (argc > 2)
        {
            ipv4_up(argv[2]);
        }
        else
        {
            printf("Insufficient number of arguments for %s\n", argv[1]);
        }
    }
    else if(!strncmp(argv[1], "ipv4-down", 9)) 
    {   
        if (argc > 2)
        {
            teardown_instance(atoi(argv[2]));
        }
        else
        {
            printf("Insufficient number of arguments for %s\n", argv[1]);
        }
    }   
#if !defined (FEATURE_RDKB_DHCP_MANAGER) 
    else if(!strncmp(argv[1], "syslog-status", 13))
    {
        char syslog_status_buf[10]={0};
        sysevent_get(g_iSyseventfd, g_tSysevent_token,
                     "syslog-status", syslog_status_buf,
                     sizeof(syslog_status_buf));
        if(!strncmp(syslog_status_buf, "started", 7))
        {
            syslog_restart_request();
        }
    }
#endif
    else if(!strncmp(argv[1], "ipv4-set_dyn_config", 19))
    {
        if (argc > 2)
        {
            apply_config(atoi(argv[2]),0,0);
        }
        else
        {
            printf("Insufficient number of arguments for %s\n", argv[1]);
        }
    }
    else if(!strncmp(argv[1], "ipv4-sync_tsip_all", 18))
    {
        sync_tsip();
        sync_tsip_asn();
        sysevent_set(g_iSyseventfd, g_tSysevent_token, "wan_staticip-status", "started", 0);
    }
    else if(!strncmp(argv[1], "ipv4-stop_tsip_all", 18))
    {
        remove_tsip_config();
        remove_tsip_asn_config();
    }
    else if(!strncmp(argv[1], "ipv4-resync_tsip", 16))
    {
        resync_tsip();
    }
    else if(!strncmp(argv[1], "ipv4-resync_tsip_asn", 20))
    {
        resync_tsip_asn();
    }
    else if(!strncmp(argv[1], "ipv4-resyncAll", 14))
    {
        resync_all_instance();
    }
#if !defined (FEATURE_RDKB_DHCP_MANAGER)
    else if(!strncmp(argv[1], "dhcp_server-resync", 18))
    {
        resync_to_nonvol(NULL);
    }
#endif
    else if((!strncmp(argv[1], "multinet_1-status", 17)) ||  
            (!strncmp(argv[1], "multinet_2-status", 17)) ||  
            (!strncmp(argv[1], "multinet_3-status", 17)) ||
            (!strncmp(argv[1], "multinet_4-status", 17)))
    {   
        int l_iL2Inst, l_iL3Inst;
        char l_cBridgeMode[2]={0};
        if (argc > 3)
        {   
            sscanf(argv[1], "multinet_%d-status", &l_iL2Inst);
            //handle_l2_status $3 $INST $2
            //$1 - multinet_*-status $2 - status $3 - L3 Instance number
            l_iL3Inst = atoi(argv[3]);
            sysevent_get(g_iSyseventfd, g_tSysevent_token,
                         "bridge_mode", l_cBridgeMode, sizeof(l_cBridgeMode));

            if (!((l_cBridgeMode[0] > '0') && (l_iL2Inst == 1)))
            {
                handle_l2_status(l_iL3Inst, l_iL2Inst, argv[2], 0);
            }
        }
        else
        {
            printf("Insufficient number of arguments for %s\n", argv[1]);
        }
    }
	close(g_iSyseventfd); //can be a memory / fd leak if not done
	fclose(g_fArmConsoleLog);
	return 0;
}
