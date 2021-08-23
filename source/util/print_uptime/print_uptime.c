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
#include <sys/sysinfo.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <telemetry_busmessage_sender.h>
#include "safec_lib_common.h"

#define DEVICE_PROPS_FILE	"/etc/device.properties"
#define DATA_SIZE	1024

int getValueFromDeviceProperties(char *value, int size,char *name)
{
	FILE *fp;
	char buf[DATA_SIZE] = {0};
	char *temp = NULL;
	int ret = -1;
	errno_t rc = -1;

	fp = fopen(DEVICE_PROPS_FILE, "r");
	if (fp == NULL)
	{
		printf("Error opening %s\n",DEVICE_PROPS_FILE);
		return -1;
        }

	while (fgets(buf, DATA_SIZE, fp) != NULL)
	{
		if (strstr(buf, name) != NULL)
		{
			buf[strcspn(buf, "\r\n")] = 0; // Strip off any carriage returns
			temp = strstr(buf, "=");
			temp++;
			rc = strcpy_s(value, size, temp);
			if( rc != EOK ){
				ERR_CHK(rc);
			}
			ret = 0;
			break;
		}
	}

	fclose(fp);
	return ret;
}

void print_uptime(char *uptimeLog, char *bootfile, char *uptime)
{
#if defined(_COSA_INTEL_USG_ATOM_)
	char cmd[256]={0};
	char armArpingIp[128]="";
	if ( (getValueFromDeviceProperties(armArpingIp, 128,"ARM_ARPING_IP") == 0) && armArpingIp[0] != 0 && strlen(armArpingIp) > 0)
	{
		if(bootfile != NULL)
		{
			if(uptime != NULL)
			{
				snprintf(cmd, 256, "/usr/bin/rpcclient %s \"print_uptime %s %s -u %s\" &", armArpingIp, uptimeLog, bootfile, uptime);
			}
			else
			{
				snprintf(cmd, 256, "/usr/bin/rpcclient %s \"print_uptime %s %s\" &", armArpingIp, uptimeLog, bootfile);
			}
		}
		else
		{
			if(uptime != NULL)
			{
				snprintf(cmd, 256, "/usr/bin/rpcclient %s \"print_uptime %s -u %s\" &", armArpingIp, uptimeLog, uptime);
			}
			else
			{
				snprintf(cmd, 256, "/usr/bin/rpcclient %s \"print_uptime %s\" &", armArpingIp, uptimeLog);
			}
		}
		system(cmd);
	}
#else
    	struct sysinfo l_sSysInfo;
    	struct tm * l_sTimeInfo;
    	time_t l_sNowTime;
    	char l_cLocalTime[32] = {0};
    	FILE *l_fBootLogFile = NULL;
		long lUptime = 0;
	char l_cLine[128] = {0};
	char BOOT_TIME_LOG_FILE[32] = "/rdklogs/logs/BootTime.log";
	errno_t rc = -1;

	sysinfo(&l_sSysInfo);
	time(&l_sNowTime);
	l_sTimeInfo = localtime(&l_sNowTime);

	if(uptime != NULL)
	{
		lUptime = atol(uptime);
	}
	else
	{
		lUptime = l_sSysInfo.uptime;
	}
  
	/* telemetry 2.0 starts */
	if(strstr(uptimeLog, "boot_to_ETH_uptime"))
	{
	    t2_event_d("btime_eth_split", lUptime);
	}
	else if(strstr(uptimeLog, "boot_to_meshagent_uptime"))
	{
	    t2_event_d("btime_mesh_split", lUptime);
	}
	else if(strstr(uptimeLog, "boot_to_MOCA_uptime"))
	{
	    t2_event_d("btime_moca_split", lUptime);
	}
	else if(strstr(uptimeLog, "boot_to_snmp_subagent_v2_uptime"))
	{
	    t2_event_d("bootuptime_SNMPV2Ready_split", lUptime);
	}
	else if(strstr(uptimeLog, "boot_to_wan_uptime"))
	{
	    t2_event_d("btime_wanup_spit", lUptime);
	}
	else if(strstr(uptimeLog, "boot_to_WEBPA_READY_uptime"))
	{
	    t2_event_d("btime_webpa_split", lUptime);
	}
	else if(strstr(uptimeLog, "boot_to_WIFI_uptime"))
	{
	    t2_event_d("bootuptime_wifi_split", lUptime);
	}
	else if(strstr(uptimeLog, "boot_to_XHOME_uptime"))
	{
	    t2_event_d("btime_xhome_split", lUptime);
	}
	/* telemetry 2.0 ends */

	rc = sprintf_s(l_cLocalTime, sizeof(l_cLocalTime), "%02d:%02d:%02d",l_sTimeInfo->tm_hour, l_sTimeInfo->tm_min, l_sTimeInfo->tm_sec);
	if( rc < EOK )
	{
	   ERR_CHK(rc);
	   return;
	}

	if(bootfile != NULL)
	{
		printf("BootUpTime info need to be logged in this file : %s\n", bootfile);
		rc = strcpy_s(BOOT_TIME_LOG_FILE, sizeof(BOOT_TIME_LOG_FILE), bootfile);
		if( rc != EOK )
		{
		   ERR_CHK(rc);
		   return;
		}
	}

	l_fBootLogFile = fopen(BOOT_TIME_LOG_FILE, "a+");
	if (NULL != l_fBootLogFile)
	{
		while(fscanf(l_fBootLogFile,"%s", l_cLine) != EOF)
		{
			if(NULL != strstr(l_cLine, uptimeLog))
			{
				printf("BootUpTime info for %s is already present not adding it again \n", uptimeLog);
				fclose(l_fBootLogFile);
				return;
			}
		}
		fprintf(l_fBootLogFile, "%s [BootUpTime] %s=%ld\n", l_cLocalTime, uptimeLog,lUptime);
		fclose(l_fBootLogFile);
	}
	else //fopen of bootlog file failed atleast write on the console
	{
		printf("%s [BootUpTime] %s=%ld\n", l_cLocalTime, uptimeLog,lUptime);
	}
#endif
}
