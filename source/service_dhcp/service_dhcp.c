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

#define DEVICE_PROPS_FILE   	"/etc/device.properties"
#define BOOL 					int
#define TRUE 					1
#define FALSE 					0
#define MAXLINE 				150
#define THIS					"/usr/bin/service_dhcp"

#define ERROR	-1
#define SUCCESS	0		
#define ARM_CONSOLE_LOG_FILE	"/rdklogs/logs/ArmConsolelog.txt.0"

const char* const g_cComponent_id = "ccsp.servicedhcp";
void* g_vBus_handle = NULL;
FILE* g_fArmConsoleLog = NULL;

int g_iSyseventfd;
token_t g_tSysevent_token;
char g_cDhcp_Lease_Time[8] = {0}, g_cTime_File[64] = {0};
char g_cBox_Type[8] = {0}, g_cXdns_Enabled[8] = {0};
char g_cMfg_Name[8] = {0}, g_cAtom_Arping_IP[16] = {0};

int dbusInit( void )
{
    int   ret  = -1;
    char* pCfg = CCSP_MSG_BUS_CFG;

    if(g_vBus_handle == NULL)
    {
        // Dbus connection init
        #ifdef DBUS_INIT_SYNC_MODE
        ret = CCSP_Message_Bus_Init_Synced(g_cComponent_id,
                                           pCfg,
                                           &g_vBus_handle,
                                           Ansc_AllocateMemory_Callback,
                                           Ansc_FreeMemory_Callback);
        #else
        ret = CCSP_Message_Bus_Init(g_cComponent_id,
                                    pCfg,
                                    &g_vBus_handle,
                                    Ansc_AllocateMemory_Callback,
                                    Ansc_FreeMemory_Callback);
        #endif /* DBUS_INIT_SYNC_MODE */
    }

    if (ret == -1)
    {
        // Dbus connection error
        fprintf(stderr, " DBUS connection error\n");
        g_vBus_handle = NULL;
    }

    return ret;
}

void print_with_uptime(const char* input)
{
    struct sysinfo l_sSysInfo;
    struct tm * l_sTimeInfo;
    time_t l_sNowTime;
    int l_iDays, l_iHours, l_iMins, l_iSec;
    char l_cLocalTime[32] = {0};

    sysinfo(&l_sSysInfo);
    time(&l_sNowTime);

    l_sTimeInfo = (struct tm *)gmtime(&l_sSysInfo.uptime); 
    l_iSec = l_sTimeInfo->tm_sec; 
    l_iMins = l_sTimeInfo->tm_min; 
    l_iHours = l_sTimeInfo->tm_hour; 
    l_iDays = l_sTimeInfo->tm_yday; 
    l_sTimeInfo = localtime(&l_sNowTime);

    sprintf(l_cLocalTime, "%02d:%02d:%02dup%02ddays:%02dhours:%02dmin:%02dsec", 
                           l_sTimeInfo->tm_hour, l_sTimeInfo->tm_min, l_sTimeInfo->tm_sec, 
                           l_iDays, l_iHours, l_iMins, l_iSec);

    fprintf(stderr, "%s%s\n", input,l_cLocalTime);
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
                strncpy(g_cBox_Type, property, (strlen(props) - strlen("BOX_TYPE=")));
            }
			if(NULL != (property = strstr(props, "XDNS_ENABLE=")))
            {
                property = property + strlen("XDNS_ENABLE=");
                strncpy(g_cXdns_Enabled, property, (strlen(props) - strlen("XDNS_ENABLE=")));
            }
			if(NULL != (property = strstr(props, "MFG_NAME=")))
            {
                property = property + strlen("MFG_NAME=");
                strncpy(g_cMfg_Name, property, (strlen(props) - strlen("MFG_NAME=")));
            }
			if(NULL != (property = strstr(props, "ATOM_ARPING_IP=")))
            {
                property = property + strlen("ATOM_ARPING_IP=");
                strncpy(g_cAtom_Arping_IP, property, (strlen(props) - strlen("ATOM_ARPING_IP=")));
            }
        }
        fclose(l_fFp);
    }   
}

void executeCmd(char *cmd)
{
	int l_iSystem_Res;
	l_iSystem_Res = system(cmd);
    if (0 != l_iSystem_Res && ECHILD == errno)
    {
        fprintf(stderr, "%s command didnt execute successfully\n", cmd);
    }
}

void copy_file(char *input_file, char *target_file)
{
    char l_cLine[255] = {0};
    FILE *l_fTargetFile = NULL, *l_fInputFile = NULL;

	l_fInputFile = fopen(input_file, "r");
    l_fTargetFile = fopen(target_file, "a+");
    if ((NULL != l_fInputFile) && (NULL != l_fTargetFile))
    {
        while(fgets(l_cLine, sizeof(l_cLine), l_fInputFile) != NULL)
        {
            fputs(l_cLine, l_fTargetFile);
        }
        fclose(l_fInputFile);
        fclose(l_fTargetFile);
    }
	else
	{
		fprintf(stderr, "copy of files failed due to error in opening one of the files \n");
	}
}

void remove_file(char *tb_removed_file)
{
    int l_iRemove_Res;
    l_iRemove_Res = remove(tb_removed_file);
    if (0 != l_iRemove_Res)
    {
        fprintf(stderr, "remove of %s file is not successful error is:%d\n", 
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
            fprintf(stderr, "%s", l_cLine);
        }   
        fclose(l_fP);
    }
}

void copy_command_output(char *cmd, char *out, int len)
{
    FILE *l_fFp = NULL; 
    char l_cBuf[256];
    char *l_cP = NULL;
    l_fFp = popen(cmd, "r");
    if (l_fFp)
    {   
        fgets(l_cBuf, sizeof(l_cBuf), l_fFp);

        /*we need to remove the \n char in buf*/
        if ((l_cP = strchr(l_cBuf, '\n'))) *l_cP = 0;

        strncpy(out, l_cBuf, len-1);
        pclose(l_fFp);
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
        fprintf(stderr, "Can't open %s for reading\n", input_file1);
        return FALSE;
    }

    l_fP2 = fopen(input_file2, "r");/* opens Second file which is also read */
    if (l_fP2 == NULL)
    {
        fprintf(stderr, "Can't open %s for reading\n", input_file2);
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
        snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "sysevent get %s-status", process_to_wait);
    	sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cProcess_Status, sizeof(l_cProcess_Status));	
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

int sysevent_syscfg_init()
{
	g_iSyseventfd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION,
                                               "service_dhcp", &g_tSysevent_token);

	g_fArmConsoleLog = freopen(ARM_CONSOLE_LOG_FILE, "a+", stderr);
	if (NULL == g_fArmConsoleLog) //In error case not returning as it is ok to continue
	{
		fprintf(stderr, "Error:%d while opening Log file:%s\n", errno, ARM_CONSOLE_LOG_FILE);
	}
	else
	{
		fprintf(stderr, "Successful in opening while opening Log file:%s\n", ARM_CONSOLE_LOG_FILE);
	}	

    if (g_iSyseventfd < 0)       
    {    
        fprintf(stderr, "service_dhcp::sysevent_open failed\n");
		return ERROR;
    }        

    if (syscfg_init() != 0) {
        fprintf(stderr, "%s: fail to init syscfg\n", __FUNCTION__);
        return ERROR; 
    }        

    /* dbus init based on bus handle value */
    if(g_vBus_handle ==  NULL)
        dbusInit();

    if(g_vBus_handle == NULL)
    {
        fprintf(stderr, "service_dhcp_init, DBUS init error\n");
        return ERROR;
    }
	return SUCCESS;
}

int main(int argc, char *argv[])
{
	char l_cL3Inst[8] = {0};
	int l_iL3Inst;

	if (argc < 2)	
	{
		fprintf(stderr, "Insufficient number of args return\n");
		return 0;
	}

	if (0 == g_iSyseventfd)
		sysevent_syscfg_init();
	
	fprintf(stderr, "%s case\n", argv[1]);
	if ((!strncmp(argv[1], "dhcp_server-start", 17)) ||
		(!strncmp(argv[1], "dhcp_server-restart", 19)))
	{
		if (3 == argc)
		{	
			dhcp_server_start(argv[2]);
		}
		else
		{
			dhcp_server_start(NULL);
		}
	}
    else if (!strncmp(argv[1], "lan-status", 10))
	{
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
	}
	else if (!strncmp(argv[1], "bring-lan", 9))
	{
		bring_lan_up();
	}
	else if (!strncmp(argv[1], "lan-restart", 11))
	{
		lan_restart();
	}
	else if ((!strncmp(argv[1], "ipv4_4-status", 17)) ||
             (!strncmp(argv[1], "ipv4_5-status", 17)))
	{
		if (argc > 2) 
        {			
            sscanf(argv[1], "ipv4_%d-status", &l_iL3Inst);
        	ipv4_status(l_iL3Inst, argv[2]);			
        }
        else
        {
            fprintf(stderr, "Insufficient number of arguments for %s\n", argv[1]);
        }
	}
	else if (!strncmp(argv[1], "lan-start", 9))
	{
		sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 "primary_lan_l3net", l_cL3Inst, 
					 sizeof(l_cL3Inst));	

		l_iL3Inst = atoi(l_cL3Inst);
		fprintf(stderr, "Calling ipv4_up with L3 Instance:%d\n", l_iL3Inst);
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
    else if((!strncmp(argv[1], "multinet_1-status", 17)) ||  
            (!strncmp(argv[1], "multinet_2-status", 17)) ||  
            (!strncmp(argv[1], "multinet_3-status", 17)) ||
            (!strncmp(argv[1], "multinet_4-status", 17)))
    {   
        int l_iL2Inst, l_iL3Inst;
        if (argc > 3)
        {   
            sscanf(argv[1], "multinet_%d-status", &l_iL2Inst);
            //handle_l2_status $3 $INST $2
            //$1 - multinet_*-status $2 - status $3 - L3 Instance number
            l_iL3Inst = atoi(argv[3]);
            handle_l2_status(l_iL3Inst, l_iL2Inst, argv[2], 0); 
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
