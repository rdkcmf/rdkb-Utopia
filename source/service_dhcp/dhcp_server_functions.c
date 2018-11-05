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
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "errno.h"
#include "lan_handler.h"
#include "util.h"
#include "dhcp_server_functions.h"

#define HOSTS_FILE              "/etc/hosts"
#define HOSTNAME_FILE           "/etc/hostname"
#define DHCP_STATIC_HOSTS_FILE  "/etc/dhcp_static_hosts"
#define DHCP_OPTIONS_FILE       "/var/dhcp_options"
#define WAN_IF_NAME     		"erouter0"
#define RESOLV_CONF             "/etc/resolv.conf"
#define BOOL                    int
#define TRUE                    1
#define FALSE                   0
#define STATIC_URLS_FILE        "/etc/static_urls"
#define STATIC_DNS_URLS_FILE    "/etc/static_dns_urls"
#define NETWORK_RES_FILE      	"/var/tmp/networkresponse.txt"
#define DHCP_CONF               "/var/dnsmasq.conf"
#define DHCP_LEASE_FILE         "/nvram/dnsmasq.leases"
#define DEFAULT_RESOLV_CONF     "/var/default/resolv.conf"
#define DEFAULT_CONF_DIR      	"/var/default"
#define DEFAULT_FILE            "/etc/utopia/system_defaults"
//#define LAN_IF_NAME     "brlan0"
#define XHS_IF_NAME     "brlan1"
#define ERROR   		-1
#define SUCCESS 		0

#define PSM_NAME_NOTIFY_WIFI_CHANGES    "eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges"
#define PSM_NAME_WIFI_RES_MIG           "eRT.com.cisco.spvtg.ccsp.Device.WiFi.WiFiRestored_AfterMigration"
#define IS_MIG_CHECK_NEEDED(MIG_CHECK)	(!strncmp(MIG_CHECK, "true", 4)) ? (TRUE) : (FALSE)

#define isValidSubnetByte(byte) (((byte == 255) || (byte == 254) || (byte == 252) || \
                                  (byte == 248) || (byte == 240) || (byte == 224) || \
                                  (byte == 192) || (byte == 128)) ? 1 : 0)
                                  
extern int g_iSyseventfd;
extern token_t g_tSysevent_token;
extern char g_cDhcp_Lease_Time[8];
extern char g_cMfg_Name[8];
extern char g_cMig_Check[8];

extern void subnet(char *ipv4Addr, char *ipv4Subnet, char *subnet);
extern void copy_file(char *, char *);
extern void remove_file(char *);

static unsigned int isValidSubnetMask(char *subnetMask);

/*
 * A subnet mask should only have continuous 1s starting from MSB (Most Significant Bit).
 * Like 11111111.11111111.11100000.00000000. 
 * Which means first 19 bits of an IP address belongs to network part and rest is host part. 
 */
static unsigned int isValidSubnetMask(char *subnetMask)
{
    int l_iIpAddrOctets[4] = {-1, -1, -1, -1};
	char *ptr = subnetMask;
	char dotCount = 0;
	char idx = 0;

	if (!ptr)
		return 0;

	// check the subnet mask have 3 dots or not
	while(*ptr != '\0')
	{
		if (*ptr == '.')
			dotCount++;

		ptr++;
	}

	if (3 != dotCount)
		return 0;

	sscanf(subnetMask, "%d.%d.%d.%d", &l_iIpAddrOctets[0], &l_iIpAddrOctets[1],
	    &l_iIpAddrOctets[2], &l_iIpAddrOctets[3]);

	//first octets in subnet mask cannot be 0 and last octets in subnet mask cannot be 255
	if ((0 == l_iIpAddrOctets[0]) || (255 == l_iIpAddrOctets[3])) 
		return 0;

	for (idx = 0; idx < 4; idx++)
	{
		if ((isValidSubnetByte(l_iIpAddrOctets[idx])) || (0 == l_iIpAddrOctets[idx]))
		{
			if (255 != l_iIpAddrOctets[idx])
			{
				idx++; //host part. From this idx all octets should be 0
				break;
			}
		}
		else 
			return 0;
	}

	//host part. From this idx all octets should be 0
	for ( ; idx < 4; idx++) 
	{
		if (0 != l_iIpAddrOctets[idx])
			return 0;
	}	
	
	return 1;
}

static int isValidLANIP(const char* ipStr)
{
        int octet1,octet2,octet3,octet4;
        struct sockaddr_in l_sSocAddr;
        if(!inet_pton(AF_INET, ipStr, &(l_sSocAddr.sin_addr)))
        {
                return 0;
        }

        sscanf(ipStr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4);

        if( ((octet1 != 10) && (octet1 != 172) && (octet1 != 192)) ||
                ((octet1 == 172) && ((octet2<16) || (octet2>31)))  ||
                ((octet1== 192) && ((octet2 != 168) || (octet3== 147)) ) )
        {
                return 0;
        }
        return 1;
}

int prepare_hostname()
{
    char l_cHostName[16] = {0}, l_cCurLanIP[16] = {0};
	FILE *l_fHosts_File = NULL;
	FILE *l_fHosts_Name_File = NULL;
	int l_iRes = 0;
    
    syscfg_get(NULL, "hostname", l_cHostName, sizeof(l_cHostName));
	sysevent_get(g_iSyseventfd, g_tSysevent_token, "current_lan_ipaddr", l_cCurLanIP, sizeof(l_cCurLanIP));

    // Open in Write mode each time for avoiding duplicate entries RDKB- 12295
	l_fHosts_File = fopen(HOSTS_FILE, "w+");
    l_fHosts_Name_File = fopen(HOSTNAME_FILE, "w+");

    if (0 != l_cHostName[0]) 
    {
		l_iRes = sethostname(l_cHostName, sizeof(l_cHostName));
		if (ERROR == l_iRes)
		{
			fprintf(stderr, "Un-Successful in setting hostname error is:%d\n", errno);
		}
		if (NULL == l_fHosts_Name_File)
	    {
    		fprintf(stderr, "Hosts Name file: %s creation failed \n", HOSTNAME_FILE);					
    	}
		else
		{
			fprintf(l_fHosts_Name_File, "%s\n", l_cHostName);
			fclose(l_fHosts_Name_File);
			l_fHosts_Name_File = NULL;
			if (NULL != l_fHosts_File)
			{
				fprintf(l_fHosts_File, "%s		%s\n", l_cCurLanIP, l_cHostName);
			}
			else
            {
				fprintf(stderr, "Hosts file: %s creation failed \n", HOSTS_FILE);
				return 0;
			}
		}	
    }
    else
	{
		fprintf(stderr, "Hostname is empty not writing to %s file\n", HOSTNAME_FILE);
	}	
	
   	if (NULL != l_fHosts_File) 
	{
   		fprintf(l_fHosts_File, "127.0.0.1       localhost\n");
		fprintf(l_fHosts_File, "::1             localhost\n");

		//The following lines are desirable for IPv6 capable hosts
   		fprintf(l_fHosts_File, "::1             ip6-localhost ip6-loopback\n");
   		fprintf(l_fHosts_File, "fe00::0         ip6-localnet\n");
   		fprintf(l_fHosts_File, "ff00::0         ip6-mcastprefix\n");
   		fprintf(l_fHosts_File, "ff02::1         ip6-allnodes\n");
   		fprintf(l_fHosts_File, "ff02::2         ip6-allrouters\n");
   		fprintf(l_fHosts_File, "ff02::3         ip6-allhosts\n");
		fclose(l_fHosts_File);
	}
	else
	{
		fprintf(stderr, "Hosts file: %s creation failed \n", HOSTS_FILE);
	}
	if (NULL != l_fHosts_Name_File) { /*RDKB-12965 & CID:-34535*/
		fclose(l_fHosts_Name_File);
	}
	return 0;
}

void calculate_dhcp_range (FILE *local_dhcpconf_file, char *prefix)
{
    char l_cLanIPAddress[16]  = {0}, l_cLanNetMask[16] = {0}, l_cDhcp_Start[16] = {0};
	char l_cDhcp_End[16] = {0}, l_cDhcp_Num[16]  = {0};
	char l_cLanSubnet[16] = {0}, l_cIpSubnet[16] = {0};

	int l_iNetMask_Last_Oct = 0, l_iStartAddr_Last_Oct = 0, l_iEndAddr_Last_Oct;
	int l_iStartIpValid = 0, l_iEndIpValid = 0;
	int l_iFirstOct, l_iSecOct, l_iThirdOct, l_iLastOct, l_iLast_Ip;
	int l_idhcp_num, l_iCIDR;
	
	struct sockaddr_in l_sSocAddr;

    syscfg_get(NULL, "lan_ipaddr", l_cLanIPAddress, sizeof(l_cLanIPAddress));
    syscfg_get(NULL, "lan_netmask", l_cLanNetMask, sizeof(l_cLanNetMask));
        if (0 == isValidSubnetMask(l_cLanNetMask))
	{
		fprintf(stderr, "[DHCPCORRUPT_TRACE] DHCP Net Mask:%s is corrupted. Setting to default Net Mask\n",
				l_cLanNetMask);
		//copy the default netmask
		memset(l_cLanNetMask, 0, sizeof(l_cLanNetMask));
		sprintf(l_cLanNetMask, "%s", "255.255.255.0");
		syscfg_set(NULL, "lan_netmask", l_cLanNetMask);
		syscfg_commit();
	}

	subnet(l_cLanIPAddress, l_cLanNetMask, l_cLanSubnet);

	sscanf(l_cLanNetMask, "%d.%d.%d.%d", &l_iFirstOct, &l_iSecOct, &l_iThirdOct, &l_iLastOct);
	l_iNetMask_Last_Oct = l_iLastOct;

	syscfg_get(NULL, "dhcp_start", l_cDhcp_Start, sizeof(l_cDhcp_Start));
	fprintf(stderr, "dhcp_start is:%s\n", l_cDhcp_Start);	

	// inet_pton validates whether IP has 4 octets and all octets are in between 0 and 255
	l_iStartIpValid = inet_pton(AF_INET, l_cDhcp_Start, &(l_sSocAddr.sin_addr));
	if (1 == l_iStartIpValid)
	{
		sscanf(l_cDhcp_Start, "%d.%d.%d.%d", &l_iFirstOct, &l_iSecOct, &l_iThirdOct, &l_iLastOct);
		l_iStartAddr_Last_Oct = l_iLastOct;
		
		subnet(l_cDhcp_Start, l_cLanNetMask, l_cIpSubnet);
		if ((l_iStartAddr_Last_Oct < 2 && l_iStartAddr_Last_Oct > 254) &&
            (strncmp(l_cIpSubnet, l_cLanSubnet, sizeof(l_cIpSubnet))))
		{
			fprintf(stderr, "Last Octet of DHCP Start Address:%d is not in range", l_iStartAddr_Last_Oct);
			l_iStartIpValid = 0;
		}
		else if (strncmp(l_cIpSubnet, l_cLanSubnet, sizeof(l_cIpSubnet)))
		{
			fprintf(stderr, "DHCP Start Address:%s is not in the same subnet as LAN IP:%s\n", 
					l_cDhcp_Start, l_cLanIPAddress);

            l_iStartIpValid = 0;	
		}
		else
		{
			fprintf(stderr, "DHCP_SERVER: Start address from syscfg_db :%s\n", l_cDhcp_Start);
		}
	}
	else	
	{
		fprintf(stderr, "Start Address is not in valid format need a re-calculation\n");
		l_iStartIpValid = 0;
	}

	// Start Address is not valid lets calculate it
	if (0 == l_iStartIpValid)
	{
        fprintf(stderr, "DHCP Start Address:%s is not valid re-calculating it\n", l_cDhcp_Start);
        sscanf(l_cLanSubnet, "%d.%d.%d.%d", &l_iFirstOct, &l_iSecOct, &l_iThirdOct, &l_iLastOct);
        l_iLastOct = 2;
        sprintf(l_cDhcp_Start, "%d.%d.%d.%d", l_iFirstOct, l_iSecOct, l_iThirdOct, l_iLastOct);    
        syscfg_set(NULL, "dhcp_start", l_cDhcp_Start);
	}
 	
    syscfg_get(NULL, "dhcp_end", l_cDhcp_End, sizeof(l_cDhcp_End));
    fprintf(stderr, "dhcp_end is:%s\n", l_cDhcp_End);

	l_iEndIpValid = inet_pton(AF_INET, l_cDhcp_End, &(l_sSocAddr.sin_addr));
	if (1 == l_iEndIpValid)
    {
        sscanf(l_cDhcp_End, "%d.%d.%d.%d", &l_iFirstOct, &l_iSecOct, &l_iThirdOct, &l_iLastOct);
        l_iEndAddr_Last_Oct = l_iLastOct;
		
		subnet(l_cDhcp_End, l_cLanNetMask, l_cIpSubnet);
        if (l_iEndAddr_Last_Oct < 2 && l_iEndAddr_Last_Oct > 254)
        {
            fprintf(stderr, "Last Octet of DHCP End Address:%d is not in range", l_iEndAddr_Last_Oct);
            l_iEndIpValid = 0;
			
        }
		else if (strncmp(l_cIpSubnet, l_cLanSubnet, sizeof(l_cIpSubnet)))
        {
            fprintf(stderr, "DHCP End Address:%s is not in the same subnet as LAN IP:%s\n", 
                    l_cDhcp_End, l_cLanIPAddress);

            l_iEndIpValid = 0;    
        }
        else
        {
			fprintf(stderr, "DHCP_SERVER: End address from syscfg_db :%s\n", l_cDhcp_End);
        }
    }
	else
	{
		fprintf(stderr, "End Address is not in valid format need a re-calculation\n");
		l_iEndIpValid = 0;
	}

	// End Address is not valid lets calculate it
	if (0 == l_iEndIpValid)
	{
		//DHCP_NUM is the number of available dhcp address for the lan
		syscfg_get(NULL, "dhcp_num", l_cDhcp_Num, sizeof(l_cDhcp_Num));
		fprintf(stderr, "dhcp_num is:%s\n", l_cDhcp_Num);

		fprintf(stderr, "DHCP END Address:%s is not valid re-calculating it\n", l_cDhcp_End);
        l_iCIDR = mask2cidr(l_cLanNetMask);
        //Extract 1st 3 octets of the lan subnet and 
        //set the last octet to 253 for the start address
        if (24 == l_iCIDR)
        {
            sscanf(l_cLanSubnet, "%d.%d.%d.%d", &l_iFirstOct, &l_iSecOct, &l_iThirdOct, &l_iLastOct);
            l_iLastOct = 253;
            sprintf(l_cDhcp_End, "%d.%d.%d.%d", l_iFirstOct, l_iSecOct, l_iThirdOct, l_iLastOct);
        }
        //Extract 1st 2 octets of the lan subnet and 
        //set the last remaining to 255.253 for the start address
        else if (16 == l_iCIDR)
        {
            sscanf(l_cLanSubnet, "%d.%d.%d.%d", &l_iFirstOct, &l_iSecOct, &l_iThirdOct, &l_iLastOct);
            l_iThirdOct = 255;
            l_iLastOct = 253;

            sprintf(l_cDhcp_End, "%d.%d.%d.%d", l_iFirstOct, l_iSecOct, l_iThirdOct, l_iLastOct);
        }
        //Extract 1st octet of the lan subnet and 
        //set the last remaining octets to 255.255.253 for the start address
        else if (8 == l_iCIDR)
        {
            sscanf(l_cLanSubnet, "%d.%d.%d.%d", &l_iFirstOct, &l_iSecOct, &l_iThirdOct, &l_iLastOct);
            l_iSecOct = 255;
            l_iThirdOct = 255;
            l_iLastOct = 253;

            sprintf(l_cDhcp_End, "%d.%d.%d.%d", l_iFirstOct, l_iSecOct, l_iThirdOct, l_iLastOct);
        }
        else
        {
            fprintf(stderr, "Invalid Subnet mask\n");
        }
        syscfg_set(NULL, "dhcp_end", l_cDhcp_End);
	}

	if (!strncmp(g_cDhcp_Lease_Time, "-1", 2))
    {
    	fprintf(local_dhcpconf_file, "%sdhcp-range=%s,%s,%s,infinite\n", prefix, 
				l_cDhcp_Start, l_cDhcp_End, l_cLanNetMask);
    }
    else
    {
    	fprintf(local_dhcpconf_file, "%sdhcp-range=%s,%s,%s,%s\n", prefix, 
				l_cDhcp_Start, l_cDhcp_End, l_cLanNetMask, g_cDhcp_Lease_Time);
    }
}

//We are not using static hosts in XB3 still we will convert this to C code
/*--------------------------------------------------------------
 Prepare the dhcp server static hosts/ip file
--------------------------------------------------------------*/
void prepare_dhcp_conf_static_hosts()
{
	char l_cLocalStatHosts[32] = {0}, l_cDhcpStatHosts[8]  = {0};
    char l_cHostLine[255] = {0}, l_cSyscfgCmd[32] = {0};
	int l_iStatHostsNum, l_iIter;
    FILE *l_fLocalStatHosts = NULL;

    sprintf(l_cLocalStatHosts, "/tmp/dhcp_static_hosts%d", getpid());
    l_fLocalStatHosts = fopen(l_cLocalStatHosts, "w+"); //It will create a file and open, re-write fresh RDK-B 12160
    if(NULL == l_fLocalStatHosts)
    {
        fprintf(stderr, "File: %s creation failed with error:%d\n", l_cLocalStatHosts, errno);
        return;
    }
    syscfg_get(NULL, "dhcp_num_static_hosts", l_cDhcpStatHosts, sizeof(l_cDhcpStatHosts));
	l_iStatHostsNum = atoi(l_cDhcpStatHosts);
	
	for (l_iIter = 1; l_iIter <= l_iStatHostsNum; l_iIter++)
	{
		sprintf(l_cSyscfgCmd, "dhcp_static_host_%d", l_iIter);
		syscfg_get(NULL, l_cSyscfgCmd, l_cHostLine, sizeof(l_cHostLine));

		fprintf(l_fLocalStatHosts, "%s,%s\n", l_cHostLine, g_cDhcp_Lease_Time);
	}
	remove_file(DHCP_STATIC_HOSTS_FILE);
	fclose(l_fLocalStatHosts);
	copy_file(l_cLocalStatHosts, DHCP_STATIC_HOSTS_FILE);
	remove_file(l_cLocalStatHosts);
}

void prepare_dhcp_options_wan_dns()
{
	// Since we are not using prepare_dhcp_options fn DHCP_OPTION_STR is always empty
	char l_cLocalDhcpOpt[32] = {0}, l_cPropagate_Ns[8] = {0}, l_cWan_Dhcp_Dns[255] = {0}; 
	char *l_cToken = NULL, l_cNs[255] = {""};
	FILE *l_fLocalDhcpOpt = NULL;
	
	sprintf(l_cLocalDhcpOpt, "/tmp/dhcp_options%d", getpid());
    l_fLocalDhcpOpt = fopen(l_cLocalDhcpOpt, "a+"); //It will create a file and open
    if(NULL == l_fLocalDhcpOpt) 
    {    
        fprintf(stderr, "File: %s creation failed with error:%d\n", l_cLocalDhcpOpt, errno);
        return;
    }    
	syscfg_get(NULL, "dhcp_server_propagate_wan_nameserver", l_cPropagate_Ns, sizeof(l_cPropagate_Ns));

	if (strncmp(l_cPropagate_Ns, "1", 1))
	{
    	syscfg_get(NULL, "block_nat_redirection", l_cPropagate_Ns, sizeof(l_cPropagate_Ns));
	    fprintf(stderr, "Propagate NS is set from block_nat_redirection value is:%s\n", l_cPropagate_Ns);
	}		

	if (!strncmp(l_cPropagate_Ns, "1", 1))	
	{
		sysevent_get(g_iSyseventfd, g_tSysevent_token, "wan_dhcp_dns", l_cWan_Dhcp_Dns, sizeof(l_cWan_Dhcp_Dns));	
		if (0 != l_cWan_Dhcp_Dns[0])
		{
			l_cToken = strtok(l_cWan_Dhcp_Dns, " ");
			while (l_cToken != NULL)
			{
				if (0 != l_cNs[0])
				{
					sprintf(l_cNs, "%s,%s", l_cNs, l_cToken);
				}
				else
				{
					sprintf(l_cNs, "%s", l_cToken);
				}
				l_cToken = strtok(NULL, " ");
			}
			l_cNs[strlen(l_cNs)] = '\0';
			fprintf(stderr, "l_cNs is:%s\n", l_cNs);
			fprintf(l_fLocalDhcpOpt, "option:dns-server, %s\n", l_cNs);	
		}
		else
		{
			fprintf(stderr, "wan_dhcp_dns is empty, so file:%s will be empty \n", DHCP_OPTIONS_FILE);
		}	
	}
	remove_file(DHCP_OPTIONS_FILE);
	fclose(l_fLocalDhcpOpt);
	copy_file(l_cLocalDhcpOpt, DHCP_OPTIONS_FILE);
	remove_file(l_cLocalDhcpOpt);
}

void prepare_whitelist_urls(FILE *fp_local_dhcp_conf)
{
	char l_cRedirect_Url[64] = {0}, l_cCloud_Personal_Url[64] = {0}, l_cUrl[64] = {0};
	char l_cErouter0_Ipv4Addr[16] = {0}, l_cNsServer4[16] = {0}, l_cLine[255] = {0};
	FILE *l_fStatic_Urls = NULL, *l_fResolv_Conf = NULL;
    char *l_cRemoveHttp = NULL;
	
    // Redirection URL can be get from DML
    syscfg_get(NULL, "redirection_url", l_cRedirect_Url, sizeof(l_cRedirect_Url));
	if (0 != l_cRedirect_Url[0])
	{
	    if (NULL != (l_cRemoveHttp = strstr(l_cRedirect_Url, "http://")))
    	{   
        	l_cRemoveHttp = l_cRemoveHttp + strlen("http://");
	        strncpy(l_cRedirect_Url, l_cRemoveHttp, sizeof(l_cRedirect_Url));
    	}
        else if (NULL != (l_cRemoveHttp = strstr(l_cRedirect_Url, "https://")))
        {
			l_cRemoveHttp = l_cRemoveHttp + strlen("https://");
            strncpy(l_cRedirect_Url, l_cRemoveHttp, sizeof(l_cRedirect_Url));	
        }
  		else
		{
			fprintf(stderr, "redirection_url doesnt contain http / https tag\n");
		}		   
	}
    fprintf(stderr, "redirection URL after stripping http / https is:%s\n", l_cRedirect_Url);

    // CloudPersonalization URL can be get from DML  
	syscfg_get(NULL, "CloudPersonalizationURL", l_cCloud_Personal_Url, sizeof(l_cCloud_Personal_Url));
    fprintf(stderr, "CloudPersonalizationURL is:%s\n", l_cCloud_Personal_Url);

	if (0 != l_cCloud_Personal_Url[0])
	{
		l_cRemoveHttp = NULL;
		if (NULL != (l_cRemoveHttp = strstr(l_cCloud_Personal_Url, "http://")))
	    {
    	    l_cRemoveHttp = l_cRemoveHttp + strlen("http://");
	        strncpy(l_cCloud_Personal_Url, l_cRemoveHttp, sizeof(l_cCloud_Personal_Url));
    	}
		else if (NULL != (l_cRemoveHttp = strstr(l_cCloud_Personal_Url, "https://")))
		{
			l_cRemoveHttp = l_cRemoveHttp + strlen("https://");
            strncpy(l_cCloud_Personal_Url, l_cRemoveHttp, sizeof(l_cCloud_Personal_Url));
		}
		else
		{
			fprintf(stderr, "CloudPersonalizationURL doesnt contain http / https tag\n");
		}
	}
	iface_get_ipv4addr(WAN_IF_NAME, l_cErouter0_Ipv4Addr, sizeof(l_cErouter0_Ipv4Addr));
    if (0 != l_cErouter0_Ipv4Addr[0])
   	{
    	//TODO see if getting IPv4 name server can be moved to a function
        l_fResolv_Conf = fopen(RESOLV_CONF, "r");
        if (NULL != l_fResolv_Conf)
        {
        	while(fgets(l_cLine, 80, l_fResolv_Conf) != NULL )
            {
            	char *property = NULL;
                if (NULL != (property = strstr(l_cLine, "nameserver ")))
                {
                	property = property + strlen("nameserver ");
                    if (strstr(property, "."))
                    {
                    	//strlen - 1 to strip \n
                        strncpy(l_cNsServer4, property, (strlen(property) - 1));
                        break;
                    }
                }
            }
            fclose(l_fResolv_Conf);
        }
        else
        {
        	fprintf(stderr, "opening of %s file failed with error:%d\n", RESOLV_CONF, errno);
        }
        fprintf(stderr, "Nameserver IPv4 address is:%s", l_cNsServer4);
        //TODO move to function if possible
    }
    // erouter0 doesnt have IPv4 Address not doing anything
    // TODO: ipv6 DNS whitelisting in case of ipv6 only clients         
    else
    {
    	fprintf(stderr, "erouter0 interface doesnt have IPv4 address\n");
    }

	if (0 != l_cNsServer4[0])
	{
		fprintf(fp_local_dhcp_conf, "server=/%s/%s\n", l_cRedirect_Url, l_cNsServer4);	
		fprintf(fp_local_dhcp_conf, "server=/%s/%s\n", l_cCloud_Personal_Url, l_cNsServer4);

		l_fStatic_Urls = fopen(STATIC_URLS_FILE, "r");
		if (NULL != l_fStatic_Urls)
		{
			while(fgets(l_cLine, 80, l_fStatic_Urls) != NULL)
			{
				strncpy(l_cUrl, l_cLine, (strlen(l_cLine) - 1));
				fprintf(fp_local_dhcp_conf, "server=/%s/%s\n", l_cUrl, l_cNsServer4);
			}
			fclose(l_fStatic_Urls);
		}
		else
		{
			fprintf(stderr, "opening of %s file failed with error:%d\n", STATIC_URLS_FILE, errno);
		}
	}
	else
	{
		fprintf(stderr, "IPv4 DNS server is not present in %s file not whitelisting URLs \n", RESOLV_CONF);
	}		
}

void do_extra_pools (FILE *local_dhcpconf_file, char *prefix, unsigned char bDhcpNs_Enabled, char *pWan_Dhcp_Dns)
{
	char l_cDhcpEnabled[8] = {0}, l_cSysevent_Cmd[32] = {0};
	char l_cIpv4Inst[8] = {0}, l_cIpv4InstStatus[8] = {0};
	char l_cDhcp_Start_Addr[16] = {0}, l_cDhcp_End_Addr[16] = {0};
	char l_cLan_Subnet[16] = {0}, l_cDhcp_Lease_Time[8] = {0}, l_cIfName[8] = {0};
	char l_cPools[8] = {0};
	char *l_cToken = NULL;	
	int l_iPool, l_iIpv4Inst;
    
	sysevent_get(g_iSyseventfd, g_tSysevent_token, 
				 "dhcp_server_current_pools", l_cPools, sizeof(l_cPools));

	l_cToken = strtok(l_cPools, "\n");
	while(l_cToken != NULL)	
	{
		if (0 != l_cToken[0])
		{
			l_iPool = atoi(l_cToken);
			sprintf(l_cSysevent_Cmd, "dhcp_server_%d_enabled", l_iPool);			
    		sysevent_get(g_iSyseventfd, g_tSysevent_token, 
						 l_cSysevent_Cmd, l_cDhcpEnabled, sizeof(l_cDhcpEnabled));

			if (!strncmp(l_cDhcpEnabled, "TRUE", 4))
			{
				sprintf(l_cSysevent_Cmd, "dhcp_server_%d_ipv4inst", l_iPool);
    			sysevent_get(g_iSyseventfd, g_tSysevent_token, 
							 l_cSysevent_Cmd, l_cIpv4Inst, sizeof(l_cIpv4Inst));
				l_iIpv4Inst = atoi(l_cIpv4Inst);	
				
				sprintf(l_cSysevent_Cmd, "ipv4_%d-status", l_iIpv4Inst);	
    			sysevent_get(g_iSyseventfd, g_tSysevent_token, 
							 l_cSysevent_Cmd, l_cIpv4InstStatus, sizeof(l_cIpv4InstStatus));
				
				if (!strncmp(l_cIpv4InstStatus, "up", 2))
				{
					sprintf(l_cSysevent_Cmd, "dhcp_server_%d_startaddr", l_iPool);	
    				sysevent_get(g_iSyseventfd, g_tSysevent_token, 
								 l_cSysevent_Cmd, l_cDhcp_Start_Addr, sizeof(l_cDhcp_Start_Addr));

					sprintf(l_cSysevent_Cmd, "dhcp_server_%d_endaddr", l_iPool);	
    				sysevent_get(g_iSyseventfd, g_tSysevent_token, 
								 l_cSysevent_Cmd, l_cDhcp_End_Addr, sizeof(l_cDhcp_End_Addr));

					sprintf(l_cSysevent_Cmd, "dhcp_server_%d_subnet", l_iPool);	
    				sysevent_get(g_iSyseventfd, g_tSysevent_token, 
								 l_cSysevent_Cmd, l_cLan_Subnet, sizeof(l_cLan_Subnet));

					sprintf(l_cSysevent_Cmd, "dhcp_server_%d_leasetime", l_iPool);	
    				sysevent_get(g_iSyseventfd, g_tSysevent_token, 
								 l_cSysevent_Cmd, l_cDhcp_Lease_Time, sizeof(l_cDhcp_Lease_Time));

					sprintf(l_cSysevent_Cmd, "ipv4_%d-ifname", l_iIpv4Inst);	
    				sysevent_get(g_iSyseventfd, g_tSysevent_token, 
								 l_cSysevent_Cmd, l_cIfName, sizeof(l_cIfName));

					//TODO prefix is not considered for first drop
                                        if ((0 != l_cDhcp_Start_Addr[0]) && (0 != l_cDhcp_End_Addr[0]))
					{
						fprintf(local_dhcpconf_file, "%sinterface=%s\n", prefix, l_cIfName);
						fprintf(local_dhcpconf_file, "%sdhcp-range=set:%d,%s,%s,%s,%s\n", prefix, l_iPool,
								l_cDhcp_Start_Addr, l_cDhcp_End_Addr, l_cLan_Subnet, l_cDhcp_Lease_Time);

						fprintf(stderr, "DHCP_SERVER : [BRLAN1] %sdhcp-range=set:%d,%s,%s,%s,%s\n", 
										prefix, l_iPool, l_cDhcp_Start_Addr, l_cDhcp_End_Addr, 
										l_cLan_Subnet, l_cDhcp_Lease_Time);

						// Needs to configure dhcp option for corresponding interface wan dns servers
						if( ( bDhcpNs_Enabled ) && \
							( NULL != pWan_Dhcp_Dns ) && \
							( '\0' != pWan_Dhcp_Dns[ 0 ] ) \
						   )
						{
							fprintf(local_dhcpconf_file, "%sdhcp-option=%s,6,%s\n", prefix, l_cIfName, pWan_Dhcp_Dns);
							fprintf(stderr, "DHCP_SERVER : [%s] %sdhcp-option=%s,6,%s\n", l_cIfName, prefix, l_cIfName, pWan_Dhcp_Dns);
						}
					}
				}
				else
				{
					fprintf(stderr, "%s is not up go to next pool\n", l_cIpv4InstStatus);
				}	
			}
			else
			{
				fprintf(stderr, "%s is not TRUE continue to next pool\n", l_cDhcpEnabled);
			}
		}
		else
		{
			fprintf(stderr, "pool is empty continue to next pool\n");
		}	
		l_cToken = strtok(NULL, " ");
	}
}

//Input to this function
//1st Input Lan IP Address and 2nd Input LAN Subnet Mask
int prepare_dhcp_conf (char *input)
{
    char l_cNetwork_Res[8] = {0}, l_cLocalDhcpConf[32] = {0};
    char l_cLanIPAddress[16] = {0}, l_cLanNetMask[16] = {0}, l_cLan_if_name[16] = {0};
    char l_cCaptivePortalEn[8] = {0}, l_cRedirect_Flag[8] = {0}, l_cMigCase[8] = {0};
    char l_cWifi_Not_Configured[8] = {0}, l_cWifi_Res_Mig[8] = {0};
    char l_cIotEnabled[16] = {0}, l_cIotIfName[16] = {0}, l_cIotStartAddr[16] = {0};
   	char l_cIotEndAddr[16] = {0}, l_cIotNetMask[16] = {0};
	char l_cPropagate_Dom[8] = {0}, l_cLan_Domain[32] = {0}, l_cLog_Level[8] = {0};
	char l_cDhcp_Num[16] = {0}, l_cLan_Status[16] = {0};
    char l_cWan_Service_Stat[16] = {0}, l_cDns_Only_Prefix[8] = {0};
	char *l_cpPsm_Get = NULL;
	char l_cDhcpNs_Enabled[ 32 ] 			 = { 0 },
		 l_cWan_Dhcp_Dns [ 256 ]             = { 0 };

	int l_iMkdir_Res, l_idhcp_num, l_iRet_Val;
	int l_iRetry_Count = 0;

	FILE *l_fLocal_Dhcp_ConfFile = NULL;
    FILE *l_fNetRes = NULL, *l_fDef_Resolv = NULL;

    BOOL l_bCaptivePortal_Mode = FALSE;
	BOOL l_bCaptive_Check = FALSE;
	BOOL l_bMig_Case = TRUE,
		 l_bDhcpNs_Enabled = FALSE,
		 l_bIsValidWanDHCPNs = FALSE;

	if ((NULL != input) && (!strncmp(input, "dns_only", 8)))
	{
		fprintf(stderr, "dns_only case prefix is #\n");
		l_cDns_Only_Prefix[0] = '#';
	}

	sprintf(l_cLocalDhcpConf, "/tmp/dnsmasq.conf%d", getpid());
	l_fLocal_Dhcp_ConfFile = fopen(l_cLocalDhcpConf, "a+"); //It will create a file and open
    if(NULL == l_fLocal_Dhcp_ConfFile) 
    {   
        fprintf(stderr, "File: %s creation failed with error:%d\n", l_cLocalDhcpConf, errno);
		return 0;
    }   
   	
    syscfg_get(NULL, "lan_ipaddr", l_cLanIPAddress, sizeof(l_cLanIPAddress));
    syscfg_get(NULL, "lan_netmask", l_cLanNetMask, sizeof(l_cLanNetMask));
    syscfg_get(NULL, "lan_ifname", l_cLan_if_name, sizeof(l_cLan_if_name));
    syscfg_get(NULL, "CaptivePortal_Enable", l_cCaptivePortalEn, sizeof(l_cCaptivePortalEn));
    syscfg_get(NULL, "redirection_flag", l_cRedirect_Flag, sizeof(l_cRedirect_Flag));

    if((0 == isValidLANIP(l_cLanIPAddress)) || (0 == isValidSubnetMask(l_cLanNetMask)))
    {
        FILE *fp;
        char cmd[512];
        char result[128];
        char lanIP[16]={0}, lanNetMask[16]={0}, dhcpStart[16]={0}, dhcpEnd[16]={0};

        fprintf(stderr, "LAN IP Address OR LAN net mask is not in valid format, setting to default lan_ipaddr:%s lan_netmask:%s\n",l_cLanIPAddress,l_cLanNetMask);
        snprintf(cmd,sizeof(cmd),"grep '$lan_ipaddr\\|$lan_netmask\\|$dhcp_start\\|$dhcp_end' %s"
          " | awk '/\\$lan_ipaddr/ {split($1,ip, \"=\");}"
          " /\\$lan_netmask/ {split($1,mask, \"=\");}"
          " /\\$dhcp_start/ {split($1,start, \"=\");}"
          " /\\$dhcp_end/ {split($1,end, \"=\");}"
          " END {print ip[2], mask[2], start[2], end[2]}'",DEFAULT_FILE);

        fprintf(stderr,"Command = %s\n",cmd);

        if ((fp = popen(cmd, "r")) == NULL)
        {
                fprintf(stderr,"popen ERROR\n");
        }
        else if (fgets(result, sizeof(result), fp) == NULL)
        {
                pclose(fp);
                fprintf(stderr,"popen fgets ERROR\n");
        }
        else
        {       sscanf(result, "%s %s %s %s",lanIP, lanNetMask, dhcpStart, dhcpEnd);

                syscfg_set(NULL, "lan_ipaddr", lanIP);
                syscfg_set(NULL, "lan_netmask", lanNetMask);
                syscfg_set(NULL, "dhcp_start", dhcpStart);
                syscfg_set(NULL, "dhcp_end", dhcpEnd);
                syscfg_commit();

                memset(l_cLanIPAddress, 0, sizeof(l_cLanIPAddress));
                strncpy(l_cLanIPAddress,lanIP,sizeof(l_cLanIPAddress));

                memset(l_cLanNetMask, 0, sizeof(l_cLanNetMask));
                strncpy(l_cLanNetMask,lanNetMask,sizeof(l_cLanNetMask));

                pclose(fp);
        }
    }


    // Static LAN DNS (brlan0)
	syscfg_get(NULL, "dhcp_nameserver_enabled", l_cDhcpNs_Enabled, sizeof(l_cDhcpNs_Enabled));
	if( ( '\0' != l_cDhcpNs_Enabled[ 0 ] ) && \
		( 1 == atoi( l_cDhcpNs_Enabled ) ) 
	  )
	{
		l_bDhcpNs_Enabled = TRUE;
	}

	// Get proper wan dns server list and check whether wan-dns is valid or not
	check_and_get_wan_dhcp_dns( l_cWan_Dhcp_Dns );
	if( '\0' != l_cWan_Dhcp_Dns[ 0 ] )
	{
		l_bIsValidWanDHCPNs = TRUE;
	}

    l_fNetRes = fopen(NETWORK_RES_FILE, "r");
    if (NULL == l_fNetRes)
    {
		fprintf(stderr, "%s file is not present \n", NETWORK_RES_FILE);
    }
    else
	{
        fscanf(l_fNetRes,"%s", l_cNetwork_Res);
		fclose(l_fNetRes);
	}	
   
   
	l_iRet_Val = PSM_VALUE_GET_STRING(PSM_NAME_NOTIFY_WIFI_CHANGES, l_cpPsm_Get);
	while ((CCSP_SUCCESS != l_iRet_Val && l_cpPsm_Get == NULL) && 
		   (l_iRetry_Count++ < 2))
	{
		l_iRet_Val = PSM_VALUE_GET_STRING(PSM_NAME_NOTIFY_WIFI_CHANGES, l_cpPsm_Get);
	}
    if (CCSP_SUCCESS == l_iRet_Val && l_cpPsm_Get != NULL)
	{
    		strncpy(l_cWifi_Not_Configured, l_cpPsm_Get, sizeof(l_cWifi_Not_Configured));
			fprintf(stderr, "DHCP SERVER : NotifyWiFiChanges is %s\n", l_cWifi_Not_Configured);
    	    Ansc_FreeMemory_Callback(l_cpPsm_Get);
        	l_cpPsm_Get = NULL;
	}
	else
	{
		fprintf(stderr, "DHCP SERVER : Error:%d while getting:%s or value is empty\n", 
				l_iRet_Val, PSM_NAME_NOTIFY_WIFI_CHANGES);
	}
	fprintf(stderr, "DHCP SERVER : CaptivePortal_Enabled is %s\n", l_cCaptivePortalEn);
	
	if (IS_MIG_CHECK_NEEDED(g_cMig_Check))
	{
		fprintf(stderr, "Wifi Migration checks are needed\n");
    	
		syscfg_get(NULL, "migration_cp_handler", l_cMigCase, sizeof(l_cMigCase));
                if(!strncmp(l_cMigCase, "true", 4))
                {
                    fprintf(stderr, "DHCP SERVER : Initialize migration case variable to true\n");
                    l_bMig_Case = TRUE;
                }
                else
                {
                   fprintf(stderr, "DHCP SERVER : Initialize migration case variable to false\n");
                   l_bMig_Case = FALSE;
                }
	    sysevent_get(g_iSyseventfd, g_tSysevent_token, "wan_service-status", l_cWan_Service_Stat, sizeof(l_cWan_Service_Stat));

		l_iRetry_Count = 0;
		l_iRet_Val = PSM_VALUE_GET_STRING(PSM_NAME_WIFI_RES_MIG, l_cpPsm_Get);
		while ((CCSP_SUCCESS != l_iRet_Val && l_cpPsm_Get == NULL) && 
			   (l_iRetry_Count++ < 2))
		{
			l_iRet_Val = PSM_VALUE_GET_STRING(PSM_NAME_WIFI_RES_MIG, l_cpPsm_Get);
		}
    	if (CCSP_SUCCESS == l_iRet_Val && l_cpPsm_Get != NULL)
		{
			strncpy(l_cWifi_Res_Mig, l_cpPsm_Get, sizeof(l_cWifi_Res_Mig));
			fprintf(stderr, "DHCP SERVER : WiFiRestored_AfterMigration is %s\n", l_cWifi_Res_Mig);
    	    Ansc_FreeMemory_Callback(l_cpPsm_Get);
	    	l_cpPsm_Get = NULL;
		}	
		else
		{
			fprintf(stderr, "DHCP SERVER : Error:%d while getting:%s or value is empty\n", 
					l_iRet_Val, PSM_NAME_WIFI_RES_MIG);
		}
	
		if ((!strncmp(l_cMigCase, "true", 4)) && (!strncmp(l_cWifi_Res_Mig, "true", 4)))
		{
	    	fprintf(stderr, "DHCP SERVER : WiFi restored case setting MIGRATION_CASE variable to false\n");
		    l_bMig_Case = FALSE;
		}
	}
	else
	{
		fprintf(stderr, "Migration checks are not needed\n");
	}

	if (IS_MIG_CHECK_NEEDED(g_cMig_Check))
	{
		l_bCaptive_Check = ((!strncmp(l_cCaptivePortalEn, "true", 4)) && (FALSE == l_bMig_Case)) ? (TRUE) : (FALSE);
	}
	else
	{
		l_bCaptive_Check = (!strncmp(l_cCaptivePortalEn, "true", 4)) ? (TRUE) : (FALSE);
	}

	if (l_bCaptive_Check)
	{
           if  ((!strncmp(l_cNetwork_Res, "204", 3)) && (!strncmp(l_cRedirect_Flag, "true", 4)) && 
			 (!strncmp(l_cWifi_Not_Configured, "true", 4))) 
           {
              l_bCaptivePortal_Mode = TRUE;
              fprintf(stderr, "DHCP SERVER : WiFi SSID and Passphrase are not modified,set CAPTIVE_PORTAL_MODE\n");
              if (access("/nvram/reverted", F_OK) == 0) //If file is present
              {
                 fprintf(stderr, "DHCP SERVER : Removing reverted flag\n");
                 remove_file("/nvram/reverted");
              }
           }
           else
           {
              l_bCaptivePortal_Mode = FALSE;
              fprintf(stderr, "DHCP SERVER : WiFi SSID and Passphrase are already modified");
              fprintf(stderr, " or no network response ,set CAPTIVE_PORTAL_MODE to false\n");
           }
	}

        fprintf(l_fLocal_Dhcp_ConfFile, "domain-needed\n");
	fprintf(l_fLocal_Dhcp_ConfFile, "bogus-priv\n");

	if (TRUE == l_bCaptivePortal_Mode)
	{
    	// Create a temporary resolv configuration file
	    // Pass that as an option in DNSMASQ
		l_iMkdir_Res = mkdir(DEFAULT_CONF_DIR, S_IRUSR | S_IWUSR);
		//mkdir successful or already exists
		if (0 == l_iMkdir_Res || (0 != l_iMkdir_Res && EEXIST == errno)) 
		{
			l_fDef_Resolv = fopen(DEFAULT_RESOLV_CONF, "a+");	
			if (NULL != l_fDef_Resolv)
			{
    			fprintf(l_fDef_Resolv, "nameserver 127.0.0.1\n");
			    fprintf(l_fLocal_Dhcp_ConfFile, "resolv-file=%s\n",DEFAULT_RESOLV_CONF);
				fclose(l_fDef_Resolv);
			}
			else
			{
				fprintf(stderr, "%s file creation failed\n", DEFAULT_RESOLV_CONF);
			}
		}
	}
   	else
	{
		if ((access(DEFAULT_RESOLV_CONF, F_OK) == 0)) //DEFAULT_RESOLV_CONF file exists
		{
			remove_file(DEFAULT_RESOLV_CONF);
		}

		if( FALSE == l_bDhcpNs_Enabled )
		{
			fprintf(l_fLocal_Dhcp_ConfFile, "resolv-file=%s\n", RESOLV_CONF); 
		}
	}

	fprintf(l_fLocal_Dhcp_ConfFile, "expand-hosts\n");

	//Propagate Domain
	syscfg_get(NULL, "dhcp_server_propagate_wan_domain", l_cPropagate_Dom, sizeof(l_cPropagate_Dom));

	// if we are provisioned to use the wan domain name, the we do so
   	// otherwise we use the lan domain name
	if (!strncmp(l_cPropagate_Dom, "1", 1))
	{
    	sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_domain", l_cLan_Domain, sizeof(l_cLan_Domain));
	}	
	if (0 == l_cLan_Domain[0])	
	{
		syscfg_get(NULL, "lan_domain", l_cLan_Domain, sizeof(l_cLan_Domain));
	}
	if (0 != l_cLan_Domain[0])
	{
		fprintf(l_fLocal_Dhcp_ConfFile, "domain=%s\n",l_cLan_Domain);
	}

	//Log Level is not used but still retaining the code
	syscfg_get(NULL, "log_level", l_cLog_Level, sizeof(l_cLog_Level));
	if (0 == l_cLog_Level[0])
	{
		strncmp(l_cLog_Level, "1", 1);
	}

	if ((NULL != input) && (!strncmp(input, "dns_only", 8)))
	{
		fprintf(l_fLocal_Dhcp_ConfFile, "no-dhcp-interface=%s\n", l_cLan_if_name);
	}
  
	//Not taking into account prefix 
	fprintf(l_fLocal_Dhcp_ConfFile, "%sdhcp-leasefile=%s\n", l_cDns_Only_Prefix, DHCP_LEASE_FILE);
	//DHCP_NUM is the number of available dhcp address for the lan
	syscfg_get(NULL, "dhcp_num", l_cDhcp_Num, sizeof(l_cDhcp_Num));
	if (0 == l_cDhcp_Num[0])
	{
	    fprintf(stderr, "DHCP NUM is empty, set the dhcp_num integer as zero\n");
	    l_idhcp_num = 0;
	}
	else
	{
		l_idhcp_num = atoi(l_cDhcp_Num);	
	    fprintf(stderr, "DHCP NUM is not empty it is :%d\n", l_idhcp_num);
	}
	fprintf(l_fLocal_Dhcp_ConfFile, "%sdhcp-lease-max=%d\n", l_cDns_Only_Prefix, l_idhcp_num);
	fprintf(l_fLocal_Dhcp_ConfFile, "%sdhcp-hostsfile=%s\n", l_cDns_Only_Prefix, DHCP_STATIC_HOSTS_FILE);

	if ( ( FALSE == l_bCaptivePortal_Mode) &&\
		  ( FALSE == l_bDhcpNs_Enabled ) 
		)
	{
		fprintf(l_fLocal_Dhcp_ConfFile, "%sdhcp-optsfile=%s\n", l_cDns_Only_Prefix, DHCP_OPTIONS_FILE);
	}
    
	if ((NULL == input) || 
		((NULL != input) && (strncmp(input, "dns_only", 8)))) //not dns_only case
	{
		fprintf(stderr, "not dns_only case calling other prepare functions\n");
		prepare_dhcp_conf_static_hosts();
		prepare_dhcp_options_wan_dns();
	}
  
   	sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan-status", l_cLan_Status, sizeof(l_cLan_Status));
	if (!strncmp(l_cLan_Status, "started", 7)) 
	{
		// calculate_dhcp_range has code to write dhcp-range 
		fprintf(l_fLocal_Dhcp_ConfFile, "interface=%s\n",l_cLan_if_name);	
		calculate_dhcp_range(l_fLocal_Dhcp_ConfFile, l_cDns_Only_Prefix);

		// Add brlan0 custom dns server configuration
		if( l_bDhcpNs_Enabled )
		{
			char cDhcpNs_OptionString[ 1024 ] = { 0 }; 

			get_dhcp_option_for_brlan0( cDhcpNs_OptionString );

			fprintf(l_fLocal_Dhcp_ConfFile, "%s\n", cDhcpNs_OptionString);
			fprintf(stderr, "DHCP_SERVER : [%s] %s\n", l_cLan_if_name, cDhcpNs_OptionString );
		}
	}
   
    // For boot time optimization, run do_extra_pools only when brlan1 interface is available
	// Ideally interface presence is done by passing SIOCGIFCONF and going thru the complete 
	// Interface list but using SIOCGIFADDR	as it is only call to get IP Address of the interface
	// If the interface is not present then fecthing IPv4 address will fail with error ENODEV
	if (is_iface_present(XHS_IF_NAME))
	{
		fprintf(stderr, "%s interface is present creating dnsmasq\n", XHS_IF_NAME);
        do_extra_pools(l_fLocal_Dhcp_ConfFile, l_cDns_Only_Prefix, l_bDhcpNs_Enabled, l_cWan_Dhcp_Dns);
	}
	else
	{
		fprintf(stderr, "%s interface is not present, not adding dnsmasq entries\n", XHS_IF_NAME);
	}

	//Lost And Found Enable
    syscfg_get(NULL, "lost_and_found_enable", l_cIotEnabled, sizeof(l_cIotEnabled));
	if (!strncmp(l_cIotEnabled, "true", 4))
	{
    	fprintf(stderr, "IOT_LOG : DHCP server configuring for IOT\n");

	    syscfg_get(NULL, "iot_ifname", l_cIotIfName, sizeof(l_cIotIfName));
	    syscfg_get(NULL, "iot_dhcp_start", l_cIotStartAddr, sizeof(l_cIotStartAddr));
	    syscfg_get(NULL, "iot_dhcp_end", l_cIotEndAddr, sizeof(l_cIotEndAddr));
	    syscfg_get(NULL, "iot_netmask", l_cIotNetMask, sizeof(l_cIotNetMask));
		
    	fprintf(l_fLocal_Dhcp_ConfFile, "interface=%s\n", l_cIotIfName);
		if (!strncmp(g_cDhcp_Lease_Time, "-1", 2))
		{
			//TODO add dns_only prefix 
			fprintf(l_fLocal_Dhcp_ConfFile, "%sdhcp-range=%s,%s,%s,infinite\n", 
					l_cDns_Only_Prefix, l_cIotStartAddr, l_cIotEndAddr, l_cIotNetMask);
		}
		else
		{
			//TODO add dns_only prefix 
			fprintf(l_fLocal_Dhcp_ConfFile, "%sdhcp-range=%s,%s,%s,%s\n", 
					l_cDns_Only_Prefix, l_cIotStartAddr, l_cIotEndAddr, l_cIotNetMask, g_cDhcp_Lease_Time);
		}

		// Add iot custom dns server configuration
		if( l_bDhcpNs_Enabled && l_bIsValidWanDHCPNs )
		{
			fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-option=%s,6,%s\n", l_cIotIfName, l_cWan_Dhcp_Dns);
			fprintf(stderr, "DHCP_SERVER : [%s] dhcp-option=%s,6,%s\n",l_cIotIfName, l_cIotIfName, l_cWan_Dhcp_Dns);
		}
	}

	fprintf(stderr, "DHCP server configuring for Mesh network\n");
#if defined (_COSA_INTEL_XB3_ARM_)
   	fprintf(l_fLocal_Dhcp_ConfFile, "interface=l2sd0.112\n");
	fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-range=169.254.0.5,169.254.0.253,255.255.255.0,infinite\n"); 

	// Add l2sd0.112 custom dns server configuration
	if( l_bDhcpNs_Enabled && l_bIsValidWanDHCPNs )
	{
		fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-option=l2sd0.112,6,%s\n", l_cWan_Dhcp_Dns);
		fprintf(stderr, "DHCP_SERVER : [l2sd0.112] dhcp-option=l2sd0.112,6,%s\n", l_cWan_Dhcp_Dns);
	}
	
   	fprintf(l_fLocal_Dhcp_ConfFile, "interface=l2sd0.113\n");
	fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-range=169.254.1.5,169.254.1.253,255.255.255.0,infinite\n"); 

	// Add l2sd0.113 custom dns server configuration
	if( l_bDhcpNs_Enabled && l_bIsValidWanDHCPNs )
	{
		fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-option=l2sd0.113,6,%s\n", l_cWan_Dhcp_Dns);
		fprintf(stderr, "DHCP_SERVER : [l2sd0.113] dhcp-option=l2sd0.113,6,%s\n", l_cWan_Dhcp_Dns);
	}

   	fprintf(l_fLocal_Dhcp_ConfFile, "interface=l2sd0.4090\n");
	fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-range=192.168.251.2,192.168.251.253,255.255.255.0,infinite\n"); 

	// Add l2sd0.4090 custom dns server configuration
	if( l_bDhcpNs_Enabled && l_bIsValidWanDHCPNs )
	{
		fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-option=l2sd0.4090,6,%s\n", l_cWan_Dhcp_Dns);
		fprintf(stderr, "DHCP_SERVER : [l2sd0.4090] dhcp-option=l2sd0.4090,6,%s\n", l_cWan_Dhcp_Dns);
	}
	
#elif defined (INTEL_PUMA7) || (defined (_COSA_BCM_ARM_) && !defined(_CBR_PRODUCT_REQ_)) // ARRIS XB6 ATOM, TCXB6 
        fprintf(l_fLocal_Dhcp_ConfFile, "interface=ath12\n");
        fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-range=169.254.0.5,169.254.0.253,255.255.255.0,infinite\n"); 

		// Add ath12 custom dns server configuration
		if( l_bDhcpNs_Enabled && l_bIsValidWanDHCPNs )
		{
			fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-option=ath12,6,%s\n", l_cWan_Dhcp_Dns);
			fprintf(stderr, "DHCP_SERVER : [ath12] dhcp-option=ath12,6,%s\n", l_cWan_Dhcp_Dns);
		}
		
        fprintf(l_fLocal_Dhcp_ConfFile, "interface=ath13\n");
        fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-range=169.254.1.5,169.254.1.253,255.255.255.0,infinite\n"); 

		// Add ath12 custom dns server configuration
		if( l_bDhcpNs_Enabled && l_bIsValidWanDHCPNs )
		{
			fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-option=ath13,6,%s\n", l_cWan_Dhcp_Dns);
			fprintf(stderr, "DHCP_SERVER : [ath13] dhcp-option=ath13,6,%s\n", l_cWan_Dhcp_Dns);
		}
#endif
	if (TRUE == l_bCaptivePortal_Mode)
	{
		//In factory default condition, prepare whitelisting and redirection IP
		fprintf(l_fLocal_Dhcp_ConfFile, "address=/#/%s\n", l_cLanIPAddress);
		fprintf(l_fLocal_Dhcp_ConfFile, "dhcp-option=252,\"\\n\"\n");
        prepare_whitelist_urls(l_fLocal_Dhcp_ConfFile);
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, "captiveportaldhcp", "completed", 0);
	}

	//Prepare static dns urls
	if( l_bDhcpNs_Enabled )
	{
		prepare_static_dns_urls( l_fLocal_Dhcp_ConfFile );
	}
		
	remove_file(DHCP_CONF);
	fclose(l_fLocal_Dhcp_ConfFile);
	copy_file(l_cLocalDhcpConf, DHCP_CONF);
	remove_file(l_cLocalDhcpConf);
	fprintf(stderr, "DHCP SERVER : Completed preparing DHCP configuration\n");
	return 0;
}

void get_dhcp_option_for_brlan0( char *pDhcpNs_OptionString )
{
	char l_cDhcpNs_1[ 128 ] 	 			 = { 0 },
		 l_cDhcpNs_2[ 128 ] 	 			 = { 0 },
		 l_cDhcpNs_3[ 128 ] 				 = { 0 },
		 l_cDhcpNs_OptionString[ 1024 ] 	 = { 0 };

    // Static LAN DNS
	syscfg_get(NULL, "dhcp_nameserver_1", l_cDhcpNs_1, sizeof(l_cDhcpNs_1));
	syscfg_get(NULL, "dhcp_nameserver_2", l_cDhcpNs_2, sizeof(l_cDhcpNs_2));	
	syscfg_get(NULL, "dhcp_nameserver_3", l_cDhcpNs_3, sizeof(l_cDhcpNs_3));		

	strcpy( l_cDhcpNs_OptionString, "dhcp-option=brlan0,6");

	if( ( '\0' != l_cDhcpNs_1[ 0 ] ) && \
		( 0 != strcmp( l_cDhcpNs_1, "0.0.0.0" ) ) 
	  )
	{
		sprintf( l_cDhcpNs_OptionString, "%s,%s", l_cDhcpNs_OptionString, l_cDhcpNs_1 );
	}

	if( ( '\0' != l_cDhcpNs_2[ 0 ] ) && \
		( 0 != strcmp( l_cDhcpNs_2, "0.0.0.0" ) ) 
	  )
	{
		sprintf( l_cDhcpNs_OptionString, "%s,%s", l_cDhcpNs_OptionString, l_cDhcpNs_2 );
	}

	
	if( ( '\0' != l_cDhcpNs_3[ 0 ] ) && \
		( 0 != strcmp( l_cDhcpNs_3, "0.0.0.0" ) ) 
	  )
	{
		sprintf( l_cDhcpNs_OptionString, "%s,%s", l_cDhcpNs_OptionString, l_cDhcpNs_3 );
	}

	// Copy custom dns servers 
	sprintf( pDhcpNs_OptionString, "%s", l_cDhcpNs_OptionString );
}

void check_and_get_wan_dhcp_dns( char *pl_cWan_Dhcp_Dns )
{
	char l_cWan_Dhcp_Dns[ 256 ] = { 0 };
	int  charCounter 			= 0;

	sysevent_get(g_iSyseventfd, g_tSysevent_token, "wan_dhcp_dns", l_cWan_Dhcp_Dns, sizeof(l_cWan_Dhcp_Dns));	

	fprintf( stderr, "DHCP SERVER : wan_dhcp_dns:%s\n", l_cWan_Dhcp_Dns );

	// Replace " "(space) charecter as ","(comma)
	while( l_cWan_Dhcp_Dns[ charCounter ] != '\0' )
	{
		if( l_cWan_Dhcp_Dns[ charCounter ] == ' ' )
		{
			l_cWan_Dhcp_Dns[ charCounter ] = ',';
		}
		charCounter++;
	}

	sprintf( pl_cWan_Dhcp_Dns, "%s", l_cWan_Dhcp_Dns );
	
	fprintf( stderr, "DHCP SERVER : After conversion wan_dhcp_dns:%s \n", l_cWan_Dhcp_Dns );
}

void prepare_static_dns_urls(FILE *fp_local_dhcp_conf)
{
	char  l_cUrl[ 128 ]   		= { 0 },
		  l_cLine[ 128 ]  		= { 0 };
	FILE *l_fStaticDns_Urls 	= NULL;
	
	fprintf( stderr, "%s\n", __FUNCTION__ );

	l_fStaticDns_Urls = fopen( STATIC_DNS_URLS_FILE, "r" );
	if (NULL != l_fStaticDns_Urls)
	{
		while( fgets( l_cLine, 128, l_fStaticDns_Urls ) != NULL )
		{
			char *pos = NULL;
			
			// Remove line \n charecter from string  
			if ( ( pos = strchr( l_cLine, '\n' ) ) != NULL )
			 *pos = '\0';

			fprintf( stderr, "url=/%s/\n", l_cLine );
			fprintf( fp_local_dhcp_conf, "server=/%s/\n", l_cLine );
			fprintf( stderr, "server=/%s/\n", l_cLine );
			memset( l_cLine, 0, sizeof( l_cLine ) );			
		}
		
		fclose( l_fStaticDns_Urls );
	}
	else
	{
		fprintf(stderr, "opening of %s file failed with error:%d\n", STATIC_DNS_URLS_FILE, errno);
	}
}
