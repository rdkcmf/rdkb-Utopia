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
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**
 * C version of "service_wan" scripts:
 * service_wan.sh/dhcp_link.sh/dhcp_wan.sh/static_link.sh/static_wan.sh
 *
 * The reason to re-implement service_wan with C is for boot time,
 * shell scripts is too slow.
 */

/* 
 * since this utility is event triggered (instead of daemon),
 * we have to use some global var to (sysevents) mark the states. 
 * I prefer daemon, so that we can write state machine clearly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "util.h"
#include "errno.h"
#include <sys/sysinfo.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <regex.h>
#ifdef FEATURE_SUPPORT_ONBOARD_LOGGING
#include "cimplog.h"
#define LOGGING_MODULE "Utopia"
#define OnboardLog(...)                 onboarding_log(LOGGING_MODULE, __VA_ARGS__)
#else
#define OnboardLog(...)
#endif

#define RESOLVE_CONF_BIN_FULL_PATH  "/sbin/resolvconf"
#define IP_UTIL_BIN_FULL_PATH "/sbin/ip.iproute2"
#define ARM_CONSOLE_LOG_FILE   "/rdklogs/logs/ArmConsolelog.txt.0"
#define RESOLV_CONF "/etc/resolv.conf"
#define RESOLV_CONF_TMP "/tmp/resolv_temp.conf"
#define  BUFSIZE 4196

static int            sysevent_fd = -1;
static char          *sysevent_name = "udhcpc";
static token_t        sysevent_token;
static unsigned short sysevent_port;
static char           sysevent_ip[19];
bool dns_changed = false;

typedef struct udhcpc_script_t
{
    char *wan_type;
    char *box_type;
    char *input_option; 
    char *dns;
    char *router;
    bool resconf_exist; // resolvconf bin
    bool ip_util_exist;
    bool broot_is_nfs;
}udhcpc_script_t;

void compare_and_delete_old_dns(udhcpc_script_t *pinfo);

struct dns_server{
 char data[BUFSIZE];

};

int sysevent_init()
{
    snprintf(sysevent_ip, sizeof(sysevent_ip),"%s","127.0.0.1");
    sysevent_port = SE_SERVER_WELL_KNOWN_PORT;
    sysevent_fd =  sysevent_open(sysevent_ip, sysevent_port, SE_VERSION, sysevent_name, &sysevent_token);
    if (sysevent_fd < 0)
        return -1;
    return 0;
}

void udhcpc_sysevent_close()
{
    if (0 <= sysevent_fd)
    {
        sysevent_close(sysevent_fd, sysevent_token);
    }
}

char* GetDeviceProperties(char *param)
{
    FILE *fp1=NULL;
    char *valPtr = NULL;
    char out_val[100]={0};
    if (!param)
        return NULL;
    fp1 = fopen("/etc/device.properties", "r");
    if (fp1 == NULL)
    {
        printf("Error opening properties file! \n");
        return NULL;
    }

    while (fgets(out_val,100, fp1) != NULL)
    {
        if (strstr(out_val, param) != NULL)
        {
            out_val[strcspn(out_val, "\r\n")] = 0; // Strip off any carriage returns

            valPtr = strstr(out_val, "=");
            valPtr++;
            break;
        }
    }
    fclose(fp1);
    if (valPtr)
    {
       return strdup(valPtr);
    }
    return valPtr;
}

void dump_dhcp_offer()
{
    printf("\n interface: %s \n",getenv("interface"));
    printf("\n ip: %s \n",getenv("ip"));
    printf("\n subnet: %s \n",getenv("subnet"));
    printf("\n broadcast: %s \n",getenv("broadcast"));
    printf("\n lease: %s \n",getenv("lease"));
    printf("\n router: %s \n",getenv("router"));
    printf("\n hostname: %s \n",getenv("hostname"));
    printf("\n domain: %s \n",getenv("domain"));
    printf("\n siaddr: %s \n",getenv("siaddr"));
    printf("\n sname: %s \n",getenv("sname"));
    printf("\n serverid: %s \n",getenv("serverid"));
    printf("\n tftp: %s \n",getenv("tftp"));
    printf("\n timezone: %s \n",getenv("timezone"));
    printf("\n timesvr: %s \n",getenv("timesvr"));
    printf("\n namesvr: %s \n",getenv("namesvr"));
    printf("\n ntpsvr: %s \n",getenv("ntpsvr"));
    printf("\n dns: %s \n",getenv("dns"));
    printf("\n wins: %s \n",getenv("wins"));
    printf("\n logsvr: %s \n",getenv("logsvr"));
    printf("\n cookiesvr: %s \n",getenv("cookiesvr"));
    printf("\n lprsvr: %s \n",getenv("lprsvr"));
    printf("\n swapsvr: %s \n",getenv("swapsvr"));
    printf("\n boot_file: %s \n",getenv("boot_file"));
    printf("\n bootfile: %s \n",getenv("bootfile"));
    printf("\n bootsize: %s \n",getenv("bootsize"));
    printf("\n rootpath: %s \n",getenv("rootpath"));
    printf("\n ipttl: %s \n",getenv("ipttl"));
    printf("\n mtuipttl: %s \n",getenv("mtuipttl"));
    printf("\n vendorspecific: %s \n",getenv("vendorspecific"));
}

int handle_defconfig(udhcpc_script_t *pinfo)
{
    char buf[128];
    if (!pinfo)
        return -1;
    if (pinfo->resconf_exist)
    {
        snprintf(buf,sizeof(buf),"/sbin/resolvconf -d %s.udhcpc",getenv("interface"));
        system(buf);
    }

    if (!pinfo->broot_is_nfs)
    {
        if (pinfo->ip_util_exist)
        {
            snprintf(buf,sizeof(buf),"ip -4 addr flush dev %s",getenv("interface"));
            system(buf);
            snprintf(buf,sizeof(buf),"ip link set dev %s up",getenv("interface"));
            system(buf);
        }
        else
        {
            snprintf(buf,sizeof(buf),"/sbin/ifconfig %s 0.0.0.0",getenv("interface"));
            system(buf);
        }    
    }
    return 0;
}

int save_dhcp_offer(udhcpc_script_t *pinfo)
{
    char eventname[256];
    char buf[128];
    int result = -1;
    if (!pinfo)
        return -1;
// Enable for Debugging
#if 0
    dump_dhcp_offer();
#endif
    compare_and_delete_old_dns(pinfo); //compare and remove old dns configuration from resolv.conf
    snprintf(eventname,sizeof(eventname),"ipv4_%s_ipaddr",getenv("interface"));
    sysevent_set(sysevent_fd, sysevent_token, eventname, getenv("ip"), 0);

    snprintf(eventname,sizeof(eventname),"ipv4_%s_subnet",getenv("interface"));
    sysevent_set(sysevent_fd, sysevent_token, eventname, getenv("mask"), 0);

    snprintf(eventname,sizeof(eventname),"ipv4_%s_lease_time",getenv("interface"));
    sysevent_set(sysevent_fd, sysevent_token, eventname, getenv("lease"), 0);

    snprintf(eventname,sizeof(eventname),"ipv4_%s_dhcp_server",getenv("interface"));
    sysevent_set(sysevent_fd, sysevent_token, eventname, getenv("serverid"), 0);

    snprintf(eventname,sizeof(eventname),"ipv4_%s_dhcp_state",getenv("interface"));
    if (pinfo->input_option)
        sysevent_set(sysevent_fd, sysevent_token, eventname, pinfo->input_option, 0);

    snprintf(eventname,sizeof(eventname),"ipv4_%s_start_time",getenv("interface"));
    memset(buf,0,sizeof(buf));
    result = read_cmd_output("cut -d. -f1 /proc/uptime",buf,sizeof(buf));
    if (result == 0)
    {
        printf("\n %s uptime: %s\n",__FUNCTION__,buf);
        sysevent_set(sysevent_fd, sysevent_token, eventname, buf, 0);
    }
    set_dns_sysevents(pinfo);
    set_router_sysevents(pinfo);
    return 0;
}

int set_dns_sysevents(udhcpc_script_t *pinfo)
{
    char dns[256] ;
    char *tok = NULL;
    int dns_n = 0;    
    char eventname[256];
    char val[32];

    if (!pinfo)
        return -1;
    if (!pinfo->dns)
        return -1;
    snprintf(dns,sizeof(dns),"%s",pinfo->dns);
    tok = strtok(dns, " ");
    if (tok)
    {        
        snprintf(eventname,sizeof(eventname),"ipv4_%s_dns_%d",getenv("interface"),dns_n);
        sysevent_set(sysevent_fd, sysevent_token, eventname, tok, 0);
        ++dns_n;
    }
    while (NULL != tok)
    {
        tok = strtok(NULL, " ");
        if (tok)
        {
            snprintf(eventname,sizeof(eventname),"ipv4_%s_dns_%d",getenv("interface"),dns_n);
            sysevent_set(sysevent_fd, sysevent_token, eventname, tok, 0);       
            ++dns_n;
        }
    }
    snprintf(eventname,sizeof(eventname),"ipv4_%s_dns_number",getenv("interface"));
    snprintf(val,sizeof(val),"%d",dns_n);
    sysevent_set(sysevent_fd, sysevent_token, eventname,val, 0);
    return 0;
}

int update_ipv4dns(udhcpc_script_t *pinfo)
{
    FILE *fp = NULL;
    char *tok = NULL;
    char *dns = getenv("dns");

    if (!pinfo)
        return -1;

    if (!dns)
        return -1;

    fp = fopen("/tmp/.ipv4dnsserver","w");
    if (NULL == fp)
        return -1;

    printf ("\n update resolv confg dns :%s \n", dns);
    tok = strtok(dns, " ");
    while (NULL != tok)
    {
        printf ("\n tok :%s \n",tok);
        fprintf(fp,"%s\n",tok);
        tok = strtok(NULL, " ");
    }
    fclose(fp);
    return 0;
}
int update_dns_tofile(udhcpc_script_t *pinfo)
{
    char dns[256];
    char *tok = NULL;
    char buf[128];
    char val[64];
    int result = -1;

    if (!pinfo)
        return -1;

    if (!pinfo->dns)
        return -1;
    snprintf(dns,sizeof(dns),"%s",pinfo->dns);
    if ((access("/tmp/.ipv4dnsserver", F_OK) == 0))
    {
        tok = strtok(dns, " ");
        printf ("\n %s dns:%s \n",__FUNCTION__, pinfo->dns);
        while (NULL != tok)
        {
            snprintf(buf,sizeof(buf),"grep %s /tmp/.ipv4dnsserver",tok);
            memset(val,0,sizeof(val));
            result = read_cmd_output(buf,val,sizeof(val));
            printf ("\n result %d grep:%s \n",result,val);
            if (0 == result)
            {
                if (strlen(val) <= 0)
                {
                    char utc_time[64];
                    char uptime[64];                

                    result = read_cmd_output("date -u",utc_time,sizeof(utc_time));
                    if (result < 0)
                    {
                        printf ("\n [date -u] cmd failed\n");            
                    }

                    result = read_cmd_output("cat /proc/uptime | awk '{ print $1 }' | cut -d\"\.\" -f1",uptime,sizeof(uptime));
                    if (0 == result)
                    {
                        printf ("\nuptime  %s tok : %s\n",uptime,tok);
                        snprintf(buf,sizeof(buf),"echo %s DNS_server_IP_changed:%s >> %s",utc_time,uptime,ARM_CONSOLE_LOG_FILE);
			OnboardLog("DNS_server_IP_changed:%s\n",uptime);
                        system(buf);
                        snprintf(buf,sizeof(buf),"echo %s >> /tmp/.ipv4dnsserver",tok);
                        system(buf);
                    }

                }
            }
            tok = strtok(NULL, " ");
        }
    }
    else
    {
        update_ipv4dns(pinfo);
    }
    return 0;
}


int add_route(udhcpc_script_t *pinfo)
{
    char router[256];
    char *tok = NULL;
    char buf[128];
    char val[32];
    int metric = 0;

    if (!pinfo)
        return -1;
    if (!pinfo->router)
        return -1;
    snprintf(router,sizeof(router),"%s",pinfo->router);
    tok = strtok(router, " ");
    if (tok)
    {      
        if (pinfo->ip_util_exist)
        {
            snprintf(buf,sizeof(buf),"ip route add default via %s metric %d",tok,metric);
        }
        else
        {
            snprintf(buf,sizeof(buf),"route add default gw %s dev %s metric %d 2>/dev/null",tok,metric);
        }
        printf("\n %s router:%s buf: %s\n",__FUNCTION__,router,buf);
        system(buf);
        ++metric;
    }
    while (NULL != tok)
    {
        tok = strtok(NULL, " ");
        if (tok)
        {
            if (pinfo->ip_util_exist)
            {
                snprintf(buf,sizeof(buf),"ip route add default via %s metric %d",tok,metric);
            }
            else
            {
                snprintf(buf,sizeof(buf),"route add default gw %s dev %s metric %d 2>/dev/null",tok,getenv("interface"),metric);
            }
            printf("\n %s router:%s buf: %s\n",__FUNCTION__,router,buf);
            system(buf);
            ++metric;
        }
    }
    return 0;
}

int set_wan_sysevents()
{
    char *serverid = getenv("serverid");
    char *lease = getenv("lease");
    char *opt58 = getenv("opt58");
    char *opt59 = getenv("opt59");
    char *subnet = getenv("subnet");
    int result = -1;

    if (serverid && strlen(serverid) > 0)
    {
        sysevent_set(sysevent_fd, sysevent_token, "wan_dhcp_svr", serverid, 0);
    }

    if (lease && strlen(lease) > 0)
    {
        char lease_date[64];
        char lease_exp[128];
        char buf[128];
        sysevent_set(sysevent_fd, sysevent_token, "wan_lease_time", lease, 0);        
        result = read_cmd_output("date \+\"\%Y\.\%m\.\%d-\%T\"",lease_date,sizeof(lease_date));
        if (0 == result)
        {
            snprintf(buf,sizeof(buf),"date -d\"%s:%s\" \+\"%%Y\.%%m\.%%d-%%T %%Z\"",lease_date,lease);
            result = read_cmd_output(buf,lease_exp,sizeof(lease_exp));
            if (0 == result)
            {                
                sysevent_set(sysevent_fd, sysevent_token, "wan_lease_expiry", lease_exp, 0);        
            }            
        }
    }

    if (opt58 && strlen(opt58) > 0)
    {
        char lease_date[64];
        char lease_renew[128];
        char buf[128];
        sysevent_set(sysevent_fd, sysevent_token, "wan_renew_time", opt58, 0);        
        result = read_cmd_output("date \+\"\%Y\.\%m\.\%d-\%T\"",lease_date,sizeof(lease_date));
        if (0 == result)
        {
            snprintf(buf,sizeof(buf),"date -d\"%s:0x%s\" \+\"%%Y\.%%m\.%%d-%%T %%Z\"",lease_date,opt58);
            result = read_cmd_output(buf,lease_renew,sizeof(lease_renew));
            if (0 == result)
            {                
                sysevent_set(sysevent_fd, sysevent_token, "wan_lease_renew", lease_renew, 0);
            }            
        }
    }

    if (opt59 && strlen(opt59) > 0)
    {
        char lease_date[64];
        char lease_bind[128];
        char buf[128];
        sysevent_set(sysevent_fd, sysevent_token, "wan_rebind_time", opt59, 0);        
        result = read_cmd_output("date \+\"\%Y\.\%m\.\%d-\%T\"",lease_date,sizeof(lease_date));
        if (0 == result)
        {
            snprintf(buf,sizeof(buf),"date -d\"%s:0x%s\" \+\"%%Y\.%%m\.%%d-%%T %%Z\"",lease_date,opt59);
            result = read_cmd_output(buf,lease_bind,sizeof(lease_bind));
            if (0 == result)
            {                
                sysevent_set(sysevent_fd, sysevent_token, "wan_lease_rebind", lease_bind, 0);
            }            
        }
    }

    if (subnet && strlen(subnet) > 0)
    {
        sysevent_set(sysevent_fd, sysevent_token, "wan_mask", subnet, 0);
    }

    return 0;
}

int set_router_sysevents(udhcpc_script_t *pinfo)
{
    char router[256];
    char *tok = NULL;
    int gw_n = 0;    
    char eventname[256];
    char val[32];

    if (!pinfo)
        return -1;

    if (!pinfo->router)
        return -1;
    snprintf(router,sizeof(router),"%s",pinfo->router);
    tok = strtok(router, " ");
    if (tok)
    {        
        snprintf(eventname,sizeof(eventname),"ipv4_%s_gw_%d",getenv("interface"),gw_n);
        sysevent_set(sysevent_fd, sysevent_token, "default_router", tok, 0);
        sysevent_set(sysevent_fd, sysevent_token, eventname, tok, 0);
        ++gw_n;
    }
    while (NULL != tok)
    {
        tok = strtok(NULL, " ");
        if (tok)
        {
            snprintf(eventname,sizeof(eventname),"ipv4_%s_gw_%d",getenv("interface"),gw_n);
            sysevent_set(sysevent_fd, sysevent_token, "default_router", tok, 0);
            sysevent_set(sysevent_fd, sysevent_token, eventname, tok, 0);       
            ++gw_n;
        }
    }
    snprintf(eventname,sizeof(eventname),"ipv4_%s_gw_number",getenv("interface"));
    snprintf(val,sizeof(val),"%d",gw_n);
    sysevent_set(sysevent_fd, sysevent_token, eventname,val, 0);
    return 0;
}

void compare_and_delete_old_dns(udhcpc_script_t *pinfo)
{
  FILE* fptr = NULL;
  FILE* ftmp = NULL;
  char*  buffer = NULL;
  char *tok = NULL;
  char dns[256]={0};
  size_t read = 0;
  size_t size = BUFSIZE;
  char INTERFACE[BUFSIZE]={0};
  char dns_server_no_query[BUFSIZE]={0};
  char dns_servers_number[BUFSIZE]={0};
  int dns_server_no;
  int i;

  sysevent_get(sysevent_fd, sysevent_token, "wan_ifname", INTERFACE, sizeof(INTERFACE));
  snprintf(dns_server_no_query, sizeof(dns_server_no_query), "ipv4_%s_dns_number", INTERFACE);
  sysevent_get(sysevent_fd, sysevent_token, dns_server_no_query , dns_servers_number, sizeof(dns_servers_number));
  dns_server_no=atoi(dns_servers_number);

  if(!dns_server_no)
  {
        dns_changed=true;
  }

  struct dns_server* dns_server_list = malloc(sizeof(struct dns_server) * dns_server_no);
  for(i=0;i<dns_server_no;i++)
  {
     char nameserver_ip_query[BUFSIZE]={0};
     char nameserver_ip[BUFSIZE]={0};
     snprintf(nameserver_ip_query, sizeof(nameserver_ip_query), "ipv4_%s_dns_%d", INTERFACE,i);
     sysevent_get(sysevent_fd, sysevent_token, nameserver_ip_query , nameserver_ip, sizeof(nameserver_ip));
     snprintf(dns_server_list[i].data ,BUFSIZE,"nameserver %s",nameserver_ip);
  }

  snprintf(dns,sizeof(dns),"%s",pinfo->dns);
  printf("\n %s Comparing old and new ipv4 dns config dns=%s\n",__FUNCTION__,dns);
  tok = strtok(dns, " ");
  while (NULL != tok && dns_server_no != 0)
  {
        char new_nameserver[BUFSIZE]={0};
        snprintf(new_nameserver,sizeof(new_nameserver),"nameserver %s",tok);
        for(i=0;i<dns_server_no;i++)
        {
                if(strncmp(dns_server_list[i].data,new_nameserver,sizeof(new_nameserver)) !=0)
                {
                        dns_changed=true;
                        printf("\n %s %s is not present in old dns config so resolv_conf file overide form service_udhcp\n",__FUNCTION__,new_nameserver);
                        break;
                }
        }
        tok = strtok(NULL, " ");
  }

  if(dns_changed)
  {

  fptr  =  fopen(RESOLV_CONF,"r");
  if (fptr  ==  NULL)
  {
    perror("Error in opening resolv.conf file in read mode ");
    exit(1);
  }

  ftmp =  fopen(RESOLV_CONF_TMP,"w");
  if (ftmp  ==  NULL)
  {
    perror("Error in opening resolv_temp.conf file in write mode");
    exit(1);
  }

  while((read = getline(&buffer, &size, fptr)) != -1)
  {
      char* search_domain = NULL;
      char* search_domain_altrnte = NULL;
      int search_ipv4_dns = 0;
      char  ipv4_dns_query[BUFSIZE] = {0};

      for(i=0;i<dns_server_no;i++)
      {
              char* ipv4_dns_match = NULL;
              ipv4_dns_match = strstr(buffer,dns_server_list[i].data) || strstr(buffer,"nameserver 127.0.0.1");
              if(ipv4_dns_match !=NULL)
              {
                      search_ipv4_dns=1;
              }

      }
      if(!dns_server_no && strstr(buffer,"nameserver 127.0.0.1") != NULL)
      {
                search_ipv4_dns=1;
      }



      search_domain = strstr(buffer,"domain");
      search_domain_altrnte = strstr(buffer,"search");
      if(search_domain == NULL && search_domain_altrnte == NULL && !search_ipv4_dns)
      {
          fprintf(ftmp, "%s",buffer);
      }
   }

   if(dns_server_list != NULL)
   {
      free(dns_server_list);
   }
      fclose(fptr);
      fclose(ftmp);
      buffer = NULL;
      read = 0;
      FILE* fIN = NULL;
      FILE* fout = NULL;

      fout = fopen(RESOLV_CONF,"w");
      if (fout  ==  NULL)
      {
        perror("Error in opening resolv.conf file in write mode");
        exit(1);
      }

    fIN =  fopen(RESOLV_CONF_TMP,"r");
    if (fIN  ==  NULL)
    {
      perror("Error in opening resolv_temp.conf file in read mode");
      exit(1);
    }

      while((read = getline(&buffer, &size, fIN)) != -1)
      {

            fprintf(fout, "%s",buffer);
      }

      fclose(fout);
      fclose(fIN);
      remove(RESOLV_CONF_TMP);
   }
}


int update_resolveconf(udhcpc_script_t *pinfo)
{
    FILE *fp = NULL;
    char *tok = NULL;
    char dns[256];

    if (!pinfo)
        return -1;

    if (!pinfo->dns)
        return -1;
    snprintf(dns,sizeof(dns),"%s",pinfo->dns);
    fp = fopen(RESOLV_CONF,"a");
    if (NULL == fp)
        {
        perror("Error in opening resolv.conf file in append mode");
        return -1;
        }

    fprintf(fp,"domain %s\n",getenv("domain"));
    printf ("\n update resolv confg dns :%s \n", pinfo->dns);
    tok = strtok(dns, " ");
    while (NULL != tok)
    {
        printf ("\n tok :%s \n",tok);
        fprintf(fp,"nameserver %s\n",tok);
        tok = strtok(NULL, " ");
    }
    fclose(fp);
    return 0;
}

int handle_wan(udhcpc_script_t *pinfo)
{
    char buf[128];
    char *mask = getenv("mask");
    char *ip = getenv("ip");
    char router[256];
    int result = -1;

    if (!pinfo)
        return -1;

    if (pinfo->router)
        snprintf(router,sizeof(router),"%s",pinfo->router);

    save_dhcp_offer(pinfo);
    if (pinfo->ip_util_exist)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"ip addr add dev %s %s/%s broadcast %s",getenv("interface"),getenv("ip"),getenv("mask"),getenv("broadcast"));
        system(buf);        
        if (mask && ip)
        {
            printf ("\n IP is %s and mask is %s \n",ip, mask);
            sysevent_set(sysevent_fd, sysevent_token, "ipv4_wan_subnet", mask, 0);
            sysevent_set(sysevent_fd, sysevent_token, "ipv4_wan_ipaddr", ip, 0);
        }
        sysevent_set(sysevent_fd, sysevent_token, "current_ipv4_link_state", "up", 0);
        sysevent_set(sysevent_fd, sysevent_token, "wan_service-status", "started", 0);
        sysevent_set(sysevent_fd, sysevent_token, "wan-status", "started", 0);
    }    

    if (pinfo->wan_type && !strcmp(pinfo->wan_type,"EPON"))
    {
        memset(buf,0,sizeof(buf));
        result = read_cmd_output("cat /proc/uptime | awk '{ print $1 }' | cut -d\"\.\" -f1",buf,sizeof(buf));
        if (result == 0)
        {
            printf("\nWan_init_complete:%s\n",buf);
        } 
        system("touch /tmp/wan_ready");
        system("print_uptime \"boot_to_wan_uptime\"");
    }
    else
    {
        if (ip)
        {
            snprintf(buf,sizeof(buf),"/sbin/ifconfig %s %s broadcast %s netmask %s",
                    getenv("interface"),ip,getenv("broadcast"),getenv("subnet"));
            system(buf);
            printf("\n %s router:%s buf: %s\n",__FUNCTION__,router,buf);
        }
    }

    if (pinfo->box_type && strcmp("XB3",pinfo->box_type))
    {
        if (router && strlen(router) > 0)
        {
            int metric = 0;
            if (!pinfo->broot_is_nfs)
            {
                if (pinfo->ip_util_exist)
                {
                    system("while ip route del default 2>/dev/null ; do :; done");
                    printf("\nExit ip while\n");
                }
                else
                {
                    system("while route del default gw 0.0.0.0 dev $interface 2>/dev/null ; do :; done");
                }
            }        
        }
        add_route(pinfo);
    }

    // Set default route
    if (pinfo->ip_util_exist)
    {
	snprintf(buf,sizeof(buf),"ip route add default via %s dev %s table erouter",router, getenv("interface"));
    }
    else
    {
	snprintf(buf,sizeof(buf),"route add default via %s dev %s table erouter",router, getenv("interface"));
    }
    printf("\nSet default route command %s\n",buf);
    system(buf);

    set_wan_sysevents();
    //update .ipv4dnsserver file
    update_dns_tofile(pinfo);

    if (pinfo->resconf_exist)
    {
        snprintf(buf,sizeof(buf),"/sbin/resolvconf -a %s.udhcpc",getenv("interface"));
        system(buf);
    }
    else   
    {
        //update resolve.conf
        if(dns_changed)
        {
                update_resolveconf(pinfo);

                FILE *fIn=NULL;
                if(fIn = fopen("/tmp/ipv4_renew_dnsserver_restart","r"))
                {
                        fclose(fIn);
                        /*As there is a change in resolv.conf restarting dhcp-server (dnsmasq)*/
                        printf("\nAs there is a change in resolv.conf restarting dhcp-server (dnsmasq)\n",__FUNCTION__);
                        sysevent_set(sysevent_fd, sysevent_token, "dhcp_server-stop","", 0);
                        sysevent_set(sysevent_fd, sysevent_token, "dhcp_server-start","", 0);
                }



                system("touch /tmp/ipv4_renew_dnsserver_restart");
        }
        else
        {
                printf("\nNot Adding new IPV4 DNS Config to resolv.conf\n",__FUNCTION__);
        }
        dns_changed=false; 
        sysevent_set(sysevent_fd, sysevent_token, "dhcp_domain",getenv("domain"), 0);
    }
    return 0;
}

int read_cmd_output(char *cmd, char *output_buf, int size_buf)
{
    FILE *f = NULL;
    char *pos = NULL;

    if (!cmd || (!output_buf) || (size_buf <= 0))
        return -1;

    f = popen(cmd,"r");
    if(f==NULL){
        return -1;
    }
    fgets(output_buf,size_buf,f);
    /* remove trailing newline */
    if((pos = strrchr(output_buf, '\n')) != NULL)
        *pos = '\0';
    pclose(f);
    return 0;
}

bool root_is_nfs()
{
    int result = -1;
    char out[128];
    memset(out,0,sizeof(out));
    result = read_cmd_output("sed -n 's/^[^ ]* \([^ ]*\) \([^ ]*\) .*$/\1 \2/p' /proc/mounts | grep \"^/ \\(nfs\\|smbfs\\|ncp\\|coda\\)$\"",out,128);
    if ((0 == result) && (strlen(out) > 0))
        return true;
    return false;
}

int init_udhcpc_script_info(udhcpc_script_t *pinfo, char *option)
{
    char *dns = NULL;
    char *router = NULL;
    if (!pinfo)
        return -1;
    memset(pinfo,0,sizeof(struct udhcpc_script_t));
    if ((access(RESOLVE_CONF_BIN_FULL_PATH, F_OK) == 0))
    {
        pinfo->resconf_exist = true;
        printf("\nRES conf bin exist\n");
    }    
    if ((access(IP_UTIL_BIN_FULL_PATH, F_OK) == 0))
    {
        pinfo->ip_util_exist = true;
        printf("\nip bin exist\n");
    }
    pinfo->broot_is_nfs = root_is_nfs();
    printf("\nrootfs %d\n",pinfo->broot_is_nfs);
    pinfo->input_option = option;
    pinfo->wan_type = GetDeviceProperties("WAN_TYPE");
    pinfo->box_type = GetDeviceProperties("BOX_TYPE");
    dns = getenv("dns");
    router = getenv("router");
    if (dns)
    { 
        pinfo->dns = strdup(dns);
    }
    if (router)
    {
        pinfo->router = strdup(router);
    }
    if (pinfo->wan_type)
    {
        printf("\nwan_type %s \n",pinfo->wan_type);
    }
 
    if (pinfo->box_type)
    {
	    printf("\nbox_type %s \n",pinfo->box_type);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    udhcpc_script_t info;

    if ((argc < 2) || !argv) {
        return -1;
    }
    if (!argv[1])
    {
        return -1;
    }

    printf ("\n service_udhcpc arg %s \n",argv[1]);
    if (sysevent_init() < 0)
    {
        return -1;
    }    
    init_udhcpc_script_info(&info,argv[1]);
    if (!strcmp (argv[1],"deconfig"))
    {
        handle_defconfig(&info);
    }
    else if ((!strcmp (argv[1],"bound")) || (!strcmp (argv[1],"renew")))
    {    
        handle_wan(&info);
    }

    udhcpc_sysevent_close(); 
    if (info.wan_type)
        free(info.wan_type);
    if (info.box_type)
        free(info.box_type);
    if (info.dns)
        free(info.dns);
    if (info.router)
        free(info.router);

    return 0;
}
