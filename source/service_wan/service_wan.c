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
#if PUMA6_OR_NEWER_SOC_TYPE
#include "asm-arm/arch-avalanche/generic/avalanche_pp_api.h"
#include "netutils.h"
#endif

#define PROG_NAME       "SERVICE-WAN"
#define ER_NETDEVNAME "erouter0"

char DHCPC_PID_FILE[100]="";

#define DHCPV6_PID_FILE 		"/var/run/erouter_dhcp6c.pid"
#define DHCP6C_PROGRESS_FILE 	"/tmp/dhcpv6c_inprogress"

//this value is from erouter0 dhcp client(5*127+10*4)
#define SW_PROT_TIMO   675 
#define RESOLV_CONF_FILE  "/etc/resolv.conf"

#define WAN_STARTED "/var/wan_started"
enum wan_prot {
    WAN_PROT_DHCP,
    WAN_PROT_STATIC,
};

/*
 * XXX:
 * no idea why COSA_DML_DEVICE_MODE_DeviceMode is 1, and 2, 3, 4 for IPv4/IPv6/DS
 * and sysevent last_erouter_mode use 0, 1, 2, 3 instead.
 * let's just follow the last_erouter_mode. :-(
 */
enum wan_rt_mod {
    WAN_RTMOD_UNKNOW,
    WAN_RTMOD_IPV4, // COSA_DML_DEVICE_MODE_Ipv4 - 1
    WAN_RTMOD_IPV6, // COSA_DML_DEVICE_MODE_Ipv6 - 1
    WAN_RTMOD_DS,   // COSA_DML_DEVICE_MODE_Dualstack - 1
};

struct serv_wan {
    int             sefd;
    int             setok;
    char            ifname[IFNAMSIZ];
    enum wan_rt_mod rtmod;
    enum wan_prot   prot;
    int             timo;
};

struct cmd_op {
    const char      *cmd;
    int             (*exec)(struct serv_wan *sw);
    const char      *desc;
};

static int wan_start(struct serv_wan *sw);
static int wan_stop(struct serv_wan *sw);
static int wan_restart(struct serv_wan *sw);
static int wan_iface_up(struct serv_wan *sw);
static int wan_iface_down(struct serv_wan *sw);
static int wan_addr_set(struct serv_wan *sw);
static int wan_addr_unset(struct serv_wan *sw);

static int wan_dhcp_start(struct serv_wan *sw);
static int wan_dhcp_stop(struct serv_wan *sw);
static int wan_dhcp_restart(struct serv_wan *sw);
static int wan_dhcp_release(struct serv_wan *sw);
static int wan_dhcp_renew(struct serv_wan *sw);

static int wan_static_start(struct serv_wan *sw);
static int wan_static_stop(struct serv_wan *sw);

#if defined(_PLATFORM_IPQ_)
static int wan_static_start_v6(struct serv_wan *sw);
static int wan_static_stop_v6(struct serv_wan *sw);
#endif

static struct cmd_op cmd_ops[] = {
    {"start",       wan_start,      "start service wan"},
    {"stop",        wan_stop,       "stop service wan"},
    {"restart",     wan_restart,    "restart service wan"},
    {"iface-up",    wan_iface_up,   "bring interface up"},
    {"iface-down",  wan_iface_down, "tear interface down"},
    {"addr-set",    wan_addr_set,   "set IP address with specific protocol"},
    {"addr-unset",  wan_addr_unset, "unset IP address with specific protocol"},

    /* protocol specific */
    {"dhcp-start",  wan_dhcp_start, "trigger DHCP procedure"},
    {"dhcp-stop",   wan_dhcp_stop,  "stop DHCP procedure"},
    {"dhcp-restart",wan_dhcp_restart, "restart DHCP procedure"},
    {"dhcp-release",wan_dhcp_release,"trigger DHCP release"},
    {"dhcp-renew",  wan_dhcp_renew, "trigger DHCP renew"},
};

static int Getdhcpcpidfile(char *pidfile,int size )
{
#if defined(_PLATFORM_IPQ_)
        strncpy(pidfile,"/tmp/udhcpc.erouter0.pid",size);

#elif (defined _COSA_INTEL_XB3_ARM_) || (defined INTEL_PUMA7)
      {

        char udhcpflag[10]="";
        syscfg_get( NULL, "UDHCPEnable", udhcpflag, sizeof(udhcpflag));
        if( 0 == strcmp(udhcpflag,"true")){
                strncpy(pidfile,"/tmp/udhcpc.erouter0.pid",size);
        }
        else
        {
                strncpy(pidfile,"/var/run/eRT_ti_udhcpc.pid",size);
        }
     }
#else
        strncpy(pidfile,"/tmp/udhcpc.erouter0.pid",size);
#endif
return 0;
}

static int dhcp_stop(const char *ifname)
{
    FILE *fp;
    char pid_str[10];
    int pid = -1;

    Getdhcpcpidfile(DHCPC_PID_FILE,sizeof(DHCPC_PID_FILE));
    if ((fp = fopen(DHCPC_PID_FILE, "rb")) != NULL) {
        if (fgets(pid_str, sizeof(pid_str), fp) != NULL && atoi(pid_str) > 0)
            pid = atoi(pid_str);

        fclose(fp);
    }

    if (pid <= 0)
#if defined(_PLATFORM_IPQ_)
        pid = pid_of("udhcpc", ifname);
#elif (defined _COSA_INTEL_XB3_ARM_) || (defined INTEL_PUMA7)
        {
        char udhcpflag[10]="";
        syscfg_get( NULL, "UDHCPEnable", udhcpflag, sizeof(udhcpflag));
        if( 0 == strcmp(udhcpflag,"true")){
                pid = pid_of("udhcpc", ifname);
        }
        else
        {
                pid = pid_of("ti_udhcpc", ifname);
        }
        }
#else
        pid = pid_of("udhcpc", ifname);
#endif

    if (pid > 0) {
        kill(pid, SIGUSR2); // triger DHCP release
        sleep(1);
        kill(pid, SIGTERM); // terminate DHCP client

        /*
        sleep(1);
        if (pid_of("ti_udhcpc", ifname) == pid) {
            fprintf(stderr, "%s: ti_udhcpc is still exist ! kill -9 it\n", __FUNCTION__);
            kill(pid, SIGKILL);
        }
        */
    }
    unlink(DHCPC_PID_FILE);

    unlink("/tmp/udhcp.log");
    return 0;
}


#define VENDOR_SPEC_FILE "/etc/udhcpc.vendor_specific"
#define VENDOR_OPTIONS_LENGTH 100

/***
 * Parses a file containing vendor specific options
 *
 * options:  buffer containing the returned parsed options
 * length:   length of options
 *
 * returns:  0 on successful parsing, else -1
 ***/
static int dhcp_parse_vendor_info( char *options, const int length )
{
    FILE *fp;
    char subopt_num[12], subopt_value[64];
    int num_read;
    
    if ((fp = fopen(VENDOR_SPEC_FILE, "ra")) != NULL) {
        int opt_len = 0;   //Total characters read
        
        //Start the string off with "43:"
        opt_len = sprintf(options, "43:");

        while ((num_read = fscanf(fp, "%11s %63s", subopt_num, subopt_value)) == 2) {
            char *ptr;
     
            if (length - opt_len < 6) {
                fprintf( stderr, "%s: Too many options\n", __FUNCTION__ );
                return -1;
            }
            
            //Print the option number
            if (strcmp(subopt_num, "SUBOPTION2") == 0) {
                opt_len += sprintf(options + opt_len, "02");
            }
            else if (strcmp(subopt_num, "SUBOPTION3") == 0) {
                opt_len += sprintf(options + opt_len, "03");
            }
            else {
                fprintf( stderr, "%s: Invalid suboption\n", __FUNCTION__ );
                return -1;
            }
            
            //Print the length of the sub-option value
            opt_len += sprintf(options + opt_len, "%02x", strlen(subopt_value));

            //Print the sub-option value in hex
            for (ptr=subopt_value; (char)*ptr != (char)0; ptr++) {
                if (length - opt_len <= 2) {
                    fprintf( stderr, "%s: Too many options\n", __FUNCTION__ );
                    return -1;
                }
                opt_len += sprintf(options + opt_len, "%02x", *ptr);
            }
        } //while
        
        if ((num_read != EOF) && (num_read != 2)) {
            fprintf(stderr, "%s: Error parsing file\n", __FUNCTION__);
            return -1;
        }
    }
    else {
        fprintf(stderr, "%s: Cannot read %s\n", __FUNCTION__, VENDOR_SPEC_FILE);
        return -1;
    }
    
    return 0;
}


static int dhcp_start(struct serv_wan *sw)
{
    char l_cErouter_Mode[16] = {0}, l_cWan_if_name[16] = {0}, l_cDhcpv6c_Enabled[8] = {0};
    int l_iErouter_Mode, err;

    syscfg_get(NULL, "last_erouter_mode", l_cErouter_Mode, sizeof(l_cErouter_Mode));
    l_iErouter_Mode = atoi(l_cErouter_Mode);

    syscfg_get(NULL, "wan_physical_ifname", l_cWan_if_name, sizeof(l_cWan_if_name));
    //if the syscfg is not giving any value hardcode it to erouter0
    Getdhcpcpidfile(DHCPC_PID_FILE,sizeof(DHCPC_PID_FILE));
    if (0 == l_cWan_if_name[0])
    {   
       strncpy(l_cWan_if_name, "erouter0", 8); 
       l_cWan_if_name[8] = '\0';
    }
   if (sw->rtmod == WAN_RTMOD_IPV4 || sw->rtmod == WAN_RTMOD_DS)  
   {
     
  /*TCHXB6 is configured to use udhcpc */
#if defined(_PLATFORM_IPQ_)
    err = vsystem("/sbin/udhcpc -t 5 -n -i %s -p %s -s /etc/udhcpc.script",sw->ifname, DHCPC_PID_FILE);

    /* DHCP client didn't able to get Ipv4 configurations */
    if ( -1 == access(DHCPC_PID_FILE, F_OK) )
    {
      printf("%s: WAN service not able to get IPv4 configuration"
           " in 5 lease try\n", __func__);
    }
#elif (defined _COSA_INTEL_XB3_ARM_) || (defined INTEL_PUMA7)
      {

    char udhcpflag[10]="";
    syscfg_get( NULL, "UDHCPEnable", udhcpflag, sizeof(udhcpflag));
    if( 0 == strcmp(udhcpflag,"true")){
    char options[VENDOR_OPTIONS_LENGTH];

    if ((err = dhcp_parse_vendor_info(options, VENDOR_OPTIONS_LENGTH)) == 0) {
        err = vsystem("/sbin/udhcpc -i %s -p %s -V eRouter1.0 -O ntpsrv -O timezone -O 125 -x %s -s /etc/udhcpc.script", sw->ifname, DHCPC_PID_FILE, options);
    }
    }
   else
   {

    err = vsystem("ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i %s "
                 "-H DocsisGateway -p %s -B -b 1",
                 sw->ifname, DHCPC_PID_FILE);
   }
   }
#else

    char options[VENDOR_OPTIONS_LENGTH];

    if ((err = dhcp_parse_vendor_info(options, VENDOR_OPTIONS_LENGTH)) == 0) {
        err = vsystem("/sbin/udhcpc -i %s -p %s -V eRouter1.0 -O ntpsrv -O timezone -O 125 -x %s -s /etc/udhcpc.script", sw->ifname, DHCPC_PID_FILE, options);
    }
#endif

/*
	err = vsystem("strace -o /tmp/stracelog -f ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i %s "
              "-H DocsisGateway -p %s -B -b 1",
              ifname, DHCPC_PID_FILE);
*/
	if (err != 0)
                   fprintf(stderr, "%s: fail to launch erouter plugin\n", __FUNCTION__);
   }
	err = 0; //temporary hack for ARRISXB3-3748

    return err == 0 ? 0 : -1;
}

static int route_config(const char *ifname)
{
#if defined(_PLATFORM_IPQ_)
    if (vsystem("ip rule add iif %s lookup all_lans && "
                "ip rule add oif %s lookup erouter ",
                ifname, ifname) != 0) {
    }
#else
    if (vsystem("ip rule add iif %s lookup all_lans && "
                "ip rule add oif %s lookup erouter && "
                "ip -6 rule add oif %s lookup erouter ",
                ifname, ifname, ifname) != 0)
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
#endif

    return 0;
}

static int route_deconfig(const char *ifname)
{
#if defined(_PLATFORM_IPQ_)
    if (vsystem("ip rule del iif %s lookup all_lans && "
                "ip rule del oif %s lookup erouter ",
                ifname, ifname) != 0) {
    }
#else
    if (vsystem("ip rule del iif %s lookup all_lans && "
                "ip rule del oif %s lookup erouter && "
                " ip -6 rule del oif %s lookup erouter ",
                ifname, ifname, ifname) != 0)
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
#endif

    return 0;
}

#if defined(_PLATFORM_IPQ_)
static int route_config_v6(const char *ifname)
{
    if (vsystem("ip -6 rule add iif %s lookup all_lans && "
                "ip -6 rule add oif %s lookup erouter ",
                ifname, ifname) != 0) {
    /*
     * NOTE : Not returning error, as vsystem() always returns -1
     */
    }

    return 0;
}

static int route_deconfig_v6(const char *ifname)
{
    if (vsystem("ip -6 rule del iif %s lookup all_lans && "
                "ip -6 rule del oif %s lookup erouter ",
                ifname, ifname) != 0) {
    /*
     * NOTE : Not returning error, as vsystem() always returns -1
     */
    }

    return 0;
}
#endif

int checkFileExists(const char *fname)
{
    FILE *file;
    if (file = fopen(fname, "r"))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

void get_dateanduptime(char *buffer, int *uptime)
{
    struct 	timeval  tv;
    struct 	tm       *tm;
    struct 	sysinfo info;
    char 	fmt[ 64 ], buf [64];

    sysinfo( &info );
    gettimeofday( &tv, NULL );
    
    if( (tm = localtime( &tv.tv_sec ) ) != NULL)
    {
	strftime( fmt, sizeof( fmt ), "%y%m%d-%T.%%06u", tm );
	snprintf( buf, sizeof( buf ), fmt, tv.tv_usec );
    }
    
    sprintf( buffer, "%s", buf);
    *uptime= info.uptime;
}

static int wan_start(struct serv_wan *sw)
{
    char status[16];
#if defined(_PLATFORM_IPQ_)
    char buf[16] = {0};
#endif
    int ret;
	int uptime = 0;
	char buffer[64] = {0};
    get_dateanduptime(buffer,&uptime);
	printf("%s Wan_init_start:%d\n",buffer,uptime);
    /* state check */
    sysevent_get(sw->sefd, sw->setok, "wan_service-status", status, sizeof(status));
    if (strcmp(status, "starting") == 0 || strcmp(status, "started") == 0) {
        fprintf(stderr, "%s: service wan has already %s !\n", __FUNCTION__, status);
        return 0;
    } else if (strcmp(status, "stopping") == 0) {
        fprintf(stderr, "%s: cannot start in status %s !\n", __FUNCTION__, status);
        return -1;
    }

    /* do start */
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "starting", 0);

#if defined(_PLATFORM_IPQ_)
    /*
     * If we are in routing mode and executing a wan-restart
     * sysevent last_erouter_mode will allow us to stop the
     * correct services before starting them
     */
    syscfg_get(NULL, "last_erouter_mode", buf, sizeof(buf));
    sysevent_set(sw->sefd, sw->setok, "last_erouter_mode", buf, 0);
#endif

    if (wan_iface_up(sw) != 0) {
        fprintf(stderr, "%s: wan_iface_up error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }

#if defined(_PLATFORM_IPQ_)
    /*
     * IPV6 static and dhcp configurations
     */
    if (sw->rtmod == WAN_RTMOD_IPV6 || sw->rtmod == WAN_RTMOD_DS) {
       switch (sw->prot) {
       case WAN_PROT_DHCP:
               fprintf(stderr, "Starting DHCPv6 Client now\n");
               system("/etc/utopia/service.d/service_dhcpv6_client.sh enable");
               if (sw->rtmod == WAN_RTMOD_IPV6)
                       sysevent_set(sw->sefd, sw->setok, "wan-status", "starting", 0);
               break;
       case WAN_PROT_STATIC:
               if (wan_static_start_v6(sw) != 0) {
                       fprintf(stderr, "%s: wan_static_start error\n", __FUNCTION__);
                       return -1;
               }
               break;
       default:
               fprintf(stderr, "%s: unknow wan protocol\n", __FUNCTION__);
       }
       if (route_config_v6(sw->ifname) != 0) {
               fprintf(stderr, "%s: route_config_v6 error\n", __FUNCTION__);
               sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
               return -1;
       }
    }
#endif

#if defined (INTEL_PUMA7)
     //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    /* set current_wan_ifname at wan-start, for all erouter modes */
    if (sw->rtmod != WAN_RTMOD_UNKNOW) {
    	/* set sysevents and trigger for other modules */
    	sysevent_set(sw->sefd, sw->setok, "current_wan_ifname", sw->ifname, 0);
    }
#endif
	
    if (sw->rtmod != WAN_RTMOD_IPV4 && sw->rtmod != WAN_RTMOD_DS)
        goto done; /* no need to config addr/route if IPv4 not enabled */

    if (wan_addr_set(sw) != 0) {
        fprintf(stderr, "%s: wan_addr_set error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }

    if (route_config(sw->ifname) != 0) {
        fprintf(stderr, "%s: route_config error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }

#if defined(_PLATFORM_IPQ_)
    /*
     * Saving the WAN protocol configuration to the sysevent variable.
     * It's value will specify the protocol configuration of the previously
     * running WAN service, which will be used in case of WAN restart.
     */
    if (sw->prot == WAN_PROT_DHCP) {
           sysevent_set(sw->sefd, sw->setok, "last_wan_proto", "dhcp", 0);
    }else if (sw->prot == WAN_PROT_STATIC) {
           sysevent_set(sw->sefd, sw->setok, "last_wan_proto", "static", 0);
    }
#endif
done:
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "started", 0);

#if defined(_PLATFORM_IPQ_)
    /*
     * Firewall should be run, once dhcp/v6 client are started and wan_service-status
     * is set to started. */
    vsystem("firewall && execute_dir /etc/utopia/post.d/ restart");
#endif

    printf("Network Response script called to capture network response\n ");
    /*Network Response captured ans stored in /var/tmp/network_response.txt*/
	
    system("sh /etc/network_response.sh &");

    ret = checkFileExists(WAN_STARTED);
    printf("Check wan started ret is %d\n",ret);
    if ( 0 == ret )
    {
	system("touch /var/wan_started");
	system("print_uptime \"boot_to_wan_uptime\"");
    }
    else
    {
	char  str[100] = {0};
        printf("%s wan_service-status is started again, upload logs\n",__FUNCTION__);
	sprintf(str,"/rdklogger/uploadRDKBLogs.sh \"\" HTTP \"\" false ");
	system(str);
    }
    get_dateanduptime(buffer,&uptime);
	printf("%s Wan_init_complete:%d\n",buffer,uptime);
    return 0;
}

static int wan_stop(struct serv_wan *sw)
{
    char val[64];
    char status[16];
#if defined(_PLATFORM_IPQ_)
    char buf[16] = {0};
#endif

    /* state check */
    sysevent_get(sw->sefd, sw->setok, "wan_service-status", status, sizeof(status));
    if (strcmp(status, "stopping") == 0 || strcmp(status, "stopped") == 0) {
        fprintf(stderr, "%s: service wan has already %s !\n", __FUNCTION__, status);
        return 0;
    } else if (strcmp(status, "starting") == 0) {
        fprintf(stderr, "%s: cannot start in status %s !\n", __FUNCTION__, status);
        return -1;
    }
 
    /* do stop */
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "stopping", 0);

#if defined(_PLATFORM_IPQ_)
    /*
     * To facilitate mode switch between IPV4, IPv6 and Mix mode we set last_erouter_mode
     * to 1, 2, 3 respectively and do wan-restart, to stop the right set of services we
     * store the mode value in last_erouter_mode sysevent variable during wan start phase and
     * use it to reset the sw->rtmod here.
     * sysevent last_erouter_mode = 1 Last running ip mode was IPV4.
     *                                   = 2 Last running ip mode was IPv6.
     *                                   = 3 Last running ip mode was Dual stack.
     */
    sysevent_get(sw->sefd, sw->setok, "last_erouter_mode", buf, sizeof(buf));
    switch (atoi(buf)) {
    case 1:
        sw->rtmod = WAN_RTMOD_IPV4;
        break;
    case 2:
        sw->rtmod = WAN_RTMOD_IPV6;
        break;
    case 3:
        sw->rtmod = WAN_RTMOD_DS;
        break;
    default:
        fprintf(stderr, "%s: unknow RT mode (last_erouter_mode)\n", __FUNCTION__);
        sw->rtmod = WAN_RTMOD_UNKNOW;
        break;
    }

    /*
     * Fetching the configuration of previously running WAN service.
     * last_wan_proto values:
     *                 dhcp   : Last running WAN service's protocol was dhcp.
     *                 static : Last running WAN service's protocol was static.
     */
     sysevent_get(sw->sefd, sw->setok, "last_wan_proto", status, sizeof(status));
     if (strcmp(status, "dhcp") == 0) {
       sw->prot = WAN_PROT_DHCP;
     } else if (strcmp(status, "static") == 0) {
       sw->prot = WAN_PROT_STATIC;
     }

    if (sw->rtmod == WAN_RTMOD_IPV6 || sw->rtmod == WAN_RTMOD_DS) {
       if (sw->prot == WAN_PROT_DHCP) {
               fprintf(stderr, "Disabling DHCPv6 Client\n");
               system("/etc/utopia/service.d/service_dhcpv6_client.sh disable");
       } else if (sw->prot == WAN_PROT_STATIC) {
               if (wan_static_stop_v6(sw) != 0) {
                       fprintf(stderr, "%s: wan_static_stop_v6 error\n", __FUNCTION__);
                       return -1;
               }
       }
        if (route_deconfig_v6(sw->ifname) != 0) {
               fprintf(stderr, "%s: route_deconfig_v6 error\n", __FUNCTION__);
               sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
               return -1;
        }
    }
#endif

    if (sw->rtmod == WAN_RTMOD_IPV4 || sw->rtmod == WAN_RTMOD_DS) {
        if (route_deconfig(sw->ifname) != 0) {
            fprintf(stderr, "%s: route_deconfig error\n", __FUNCTION__);
            sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
            return -1;
        }

        if (wan_addr_unset(sw) != 0) {
            fprintf(stderr, "%s: wan_addr_unset error\n", __FUNCTION__);
            sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
            return -1;
        }
    }

#if !defined(_PLATFORM_IPQ_)
    if (wan_iface_down(sw) != 0) {
        fprintf(stderr, "%s: wan_iface_down error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }
#endif

    printf("%s wan_service-status is stopped, take log back up\n",__FUNCTION__);
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "stopped", 0);
	char  str[100] = {0};
	sprintf(str,"/rdklogger/backupLogs.sh false \"\" wan-stopped");
    system(str);
    return 0;
}

static int wan_restart(struct serv_wan *sw)
{
    int err;

    sysevent_set(sw->sefd, sw->setok, "wan-restarting", "1", 0);

    if (wan_stop(sw) != 0)
        fprintf(stderr, "%s: wan_stop error\n", __FUNCTION__);

    if ((err = wan_start(sw)) != 0)
        fprintf(stderr, "%s: wan_start error\n", __FUNCTION__);

    sysevent_set(sw->sefd, sw->setok, "wan-restarting", "0", 0);
    return err;
}
#if PUMA6_OR_NEWER_SOC_TYPE
int SendIoctlToPpDev( unsigned int cmd, void* data)
{
   int rc;
   int pp_fd;

   printf(" Entry %s \n", __FUNCTION__);

    if ( ( pp_fd = open ( "/dev/pp" , O_RDWR ) ) < 0 )
    {
        printf(" Error in open PP driver %d\n", pp_fd);
        close(pp_fd);
        return -1;
    }

    /* Send Command to PP driver */
    if ((rc = ioctl(pp_fd, cmd, data)) != 0)
    {
        printf(" Error ioctl %d return with %d\n", cmd, rc);
        close(pp_fd);
        return -1;
    }

    close(pp_fd);
    return 0;

}
#endif
static int wan_iface_up(struct serv_wan *sw)
{
#if 1 // XXX: MOVE these code to IPv6 scripts, why put them in IPv4 service wan ??
    char proven[64];
    char mtu[16];

    switch (sw->rtmod) {
    case WAN_RTMOD_IPV6:
    case WAN_RTMOD_DS:
        syscfg_get(NULL, "router_adv_provisioning_enable", proven, sizeof(proven));
        if (atoi(proven) == 1) {
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", sw->ifname, "1");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra", sw->ifname, "2");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", sw->ifname, "1");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra_pinfo", sw->ifname, "0");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/autoconf", sw->ifname, "1");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", sw->ifname, "0");
        } else {
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra", sw->ifname, "0");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/autoconf", sw->ifname, "0");
        }

        sysctl_iface_set("/proc/sys/net/ipv6/conf/all/forwarding", NULL, "1");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/forwarding", sw->ifname, "1");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/forwarding", "wan0", "0");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/forwarding", "mta0", "0");
        break;
    default:
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", sw->ifname, "1");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/autoconf", sw->ifname, "0");
        break;
    }
#endif

    syscfg_get(NULL, "wan_mtu", mtu, sizeof(mtu));
    if (atoi(mtu) < 1500 && atoi(mtu) > 0)
        vsystem("ip -4 link set %s mtu %s", sw->ifname, mtu);

    sysctl_iface_set("/proc/sys/net/ipv4/conf/%s/arp_announce", sw->ifname, "1");
    vsystem("ip -4 link set %s up", sw->ifname);
#if PUMA6_OR_NEWER_SOC_TYPE

    if(0 == strncmp(sw->ifname,ER_NETDEVNAME,strlen(ER_NETDEVNAME)))
    {
        avalanche_pp_local_dev_addr_ioctl_params_t pp_gwErtMacAddr;
        NETUTILS_GET_MACADDR(ER_NETDEVNAME, &pp_gwErtMacAddr.u.mac_addr);
        pp_gwErtMacAddr.op_type = ADD_ADDR;
        pp_gwErtMacAddr.addr_type = GW_MAC_ADDR;
        SendIoctlToPpDev(PP_DRIVER_SET_LOCAL_DEV_ADDR,&pp_gwErtMacAddr);
    }
    {// send IOCTL for l2sd0
        avalanche_pp_local_dev_addr_ioctl_params_t pp_gwL2Sd0MacAddr;
        NETUTILS_GET_MACADDR("l2sd0", &pp_gwL2Sd0MacAddr.u.mac_addr);
        pp_gwL2Sd0MacAddr.op_type = ADD_ADDR;
        pp_gwL2Sd0MacAddr.addr_type = RND_MAC_ADDR;
        SendIoctlToPpDev(PP_DRIVER_SET_LOCAL_DEV_ADDR,&pp_gwL2Sd0MacAddr);
    }
#endif
    return 0;
}

static int wan_iface_down(struct serv_wan *sw)
{
    int err;

    err = vsystem("ip -4 link set %s down", sw->ifname);

#if PUMA6_OR_NEWER_SOC_TYPE

    if(0 == strncmp(sw->ifname,ER_NETDEVNAME,strlen(ER_NETDEVNAME)))
    {
        avalanche_pp_local_dev_addr_ioctl_params_t pp_gwErtMacAddr;
        NETUTILS_GET_MACADDR(ER_NETDEVNAME, &pp_gwErtMacAddr.u.mac_addr);
        pp_gwErtMacAddr.op_type = FLUSH_LIST;
        pp_gwErtMacAddr.addr_type = GW_MAC_ADDR;
        SendIoctlToPpDev(PP_DRIVER_SET_LOCAL_DEV_ADDR,&pp_gwErtMacAddr);
    }
#endif
	err = 0; //temporary hack for ARRISXB3-3748
    return err == 0 ? 0 : -1;
}

static int wan_addr_set(struct serv_wan *sw)
{
    char val[64];
    char state[16];
    char mischandler_ready[10] ={0};
    int timo,count=0;
    FILE *fp;
    char ipaddr[16];
    char lanstatus[10] = {0};
    char brmode[4] = {0};

    sysevent_set(sw->sefd, sw->setok, "wan-status", "starting", 0);
    sysevent_set(sw->sefd, sw->setok, "wan-errinfo", NULL, 0);

    switch (sw->prot) {
    case WAN_PROT_DHCP:
        if (wan_dhcp_start(sw) != 0) {
            fprintf(stderr, "%s: wan_dhcp_start error\n", __FUNCTION__);
            return -1;
        }

        break;
    case WAN_PROT_STATIC:
        if (wan_static_start(sw) != 0) {
            fprintf(stderr, "%s: wan_static_start error\n", __FUNCTION__);
            return -1;
        }

        break;
    default:
        fprintf(stderr, "%s: unknow wan protocol\n", __FUNCTION__);
        return -1;
    }

#if !defined(_PLATFORM_IPQ_)
    /*
     * The trigger of 'current_ipv4_link_state' to 'up' is moved to WAN service
     * from Gateway provisioning App. This is done to save the delay in getting
     * the configuration done, and to support the WAN restart functionality.
     */
    fprintf(stderr, "[%s] start waiting for protocol ...\n", PROG_NAME);
    for (timo = sw->timo; timo > 0; timo--) {
        sysevent_get(sw->sefd, sw->setok, "current_ipv4_link_state", state, sizeof(state));
        if (strcmp(state, "up") == 0)
            break;
        sleep(1);
    }
    if (timo == 0)
        fprintf(stderr, "[%s] wait for protocol TIMEOUT !\n", PROG_NAME);
    else
        fprintf(stderr, "[%s] wait for protocol SUCCESS !\n", PROG_NAME);

#endif
#if !defined (INTEL_PUMA7)
    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    /* set sysevents and trigger for other modules */
    sysevent_set(sw->sefd, sw->setok, "current_wan_ifname", sw->ifname, 0);
#endif

    memset(val, 0 ,sizeof(val));
    sysevent_get(sw->sefd, sw->setok, "ipv4_wan_subnet", val, sizeof(val));
    if (strlen(val))
        sysevent_set(sw->sefd, sw->setok, "current_wan_subnet", val, 0);
    else
    {
        do
       {
	   count++;
	   sleep(2);
   	   sysevent_get(sw->sefd, sw->setok, "ipv4_wan_subnet", val, sizeof(val));
	   if ( '\0' == val[0] )
		   printf("ipv4_wan_subnet is NULL, retry count is %d\n",count);
	   else
		   printf("ipv4_wan_subnet value is %s, count is %d\n",val,count);

       } while ( 0 == strlen(val) && count < 3 );
	count=0; 
    	if (strlen(val))
		sysevent_set(sw->sefd, sw->setok, "current_wan_subnet", val, 0);
	else
	        sysevent_set(sw->sefd, sw->setok, "current_wan_subnet", "255.255.255.0", 0);
    }
    
    memset(val, 0 ,sizeof(val));
    sysevent_get(sw->sefd, sw->setok, "ipv4_wan_ipaddr", val, sizeof(val));
    if (strlen(val)){
        printf("Setting current_wan_ipaddr  %s\n",val);     
        sysevent_set(sw->sefd, sw->setok, "current_wan_ipaddr", val, 0);
    }
    else
    {
       printf("Wait for ipv4_wan_ipaddr to get valid ip \n");
       do
       {
           count++;
	    sleep(2);
	    sysevent_get(sw->sefd, sw->setok, "ipv4_wan_ipaddr", val, sizeof(val));
	    if ( '\0' == val[0] )
		   printf("ipv4_wan_ipaddr is NULL, retry count is %d\n",count);
	    else
		   printf("ipv4_wan_ipaddr value is %s, count is %d\n",val,count);

       } while ( 0 == strlen(val) && count < 3 );
	count=0; 
        printf("Setting current_wan_ipaddr  %s\n",val);     
   	if (strlen(val))
       		 sysevent_set(sw->sefd, sw->setok, "current_wan_ipaddr", val, 0);
	else
	        sysevent_set(sw->sefd, sw->setok, "current_wan_ipaddr", "0.0.0.0", 0);
     }

    memset(val, 0 ,sizeof(val));
    syscfg_get(NULL, "dhcp_server_propagate_wan_domain", val, sizeof(val));
    if (atoi(val) != 1)
        syscfg_get(NULL, "dhcp_server_propagate_wan_nameserver", val, sizeof(val));

    if (atoi(val) == 1) {
        //if ((fp = fopen("/var/tmp/lan_not_restart", "wb")) != NULL)
        //    fclose(fp);
        sysevent_set(sw->sefd, sw->setok, "dhcp_server-restart", "lan_not_restart", 0);
    }
#if 1 // wan-status triggers service_routed, which will restart firewall
    /* this logic are really strange, it means whan lan is ok but "start-misc" is not, 
     * do not start firewall fully. but "start-misc" means ? 
     * why not use "lan-status" ? 
     *
     * It not good idea to trigger other module here, firewall itself should register 
     * "lan-status" and "wan-status" and determine which part should be launched. */
    memset(val, 0 ,sizeof(val));
    sysevent_get(sw->sefd, sw->setok, "start-misc", val, sizeof(val));
    sysevent_get(sw->sefd, sw->setok, "current_lan_ipaddr", ipaddr, sizeof(ipaddr));

    sysevent_get(sw->sefd, sw->setok,"bridge_mode", brmode, sizeof(brmode));
    sysevent_get(sw->sefd, sw->setok,"lan-status", lanstatus, sizeof(lanstatus));
    int bridgeMode = atoi(brmode);

    if (strcmp(val, "ready") != 0 && strlen(ipaddr) && strcmp(ipaddr, "0.0.0.0") != 0) {
        fprintf(stderr, "%s: start-misc: %s current_lan_ipaddr %s\n", __FUNCTION__, val, ipaddr);
        fprintf(stderr, "[%s] start firewall partially\n", PROG_NAME);

        sysevent_get(sw->sefd, sw->setok, "parcon_nfq_status", val, sizeof(val));
        if (strcmp(val, "started") != 0) {
            iface_get_hwaddr(sw->ifname, val, sizeof(val));
            vsystem("((nfq_handler 4 %s &)&)", val);
            sysevent_set(sw->sefd, sw->setok, "parcon_nfq_status", "started", 0);
        }

    /* Should not be executed before wan_service-status is set to started for _PLATFORM_IPQ_ */
#if !defined(_PLATFORM_IPQ_) && !defined(_PLATFORM_RASPBERRYPI_)
        vsystem("firewall && gw_lan_refresh && execute_dir /etc/utopia/post.d/ restart");
#endif
#if defined(_PLATFORM_RASPBERRYPI_)
        vsystem("firewall && execute_dir /etc/utopia/post.d/ restart");
#endif
    } else if(bridgeMode != 0 && strcmp(lanstatus, "stopped") == 0 ) {
    	vsystem("firewall && execute_dir /etc/utopia/post.d/ restart");
    } else {

	sysevent_get(sw->sefd, sw->setok, "misc-ready-from-mischandler",mischandler_ready, sizeof(mischandler_ready));
	if(strcmp(mischandler_ready,"true") == 0)
	{
		//only for first time
#if !defined(_PLATFORM_RASPBERRYPI_)
		fprintf(stderr, "[%s] ready is set from misc handler. Doing gw_lan_refresh\n", PROG_NAME);
		system("gw_lan_refresh ");
#endif
		sysevent_set(sw->sefd, sw->setok, "misc-ready-from-mischandler", "false", 0);
	}
#if defined(_PLATFORM_RASPBERRYPI_)
	vsystem("execute_dir /etc/utopia/post.d/ restart");
#endif
        fprintf(stderr, "[%s] start firewall fully\n", PROG_NAME);
        printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "firewall-restart", NULL, 0);
    }
#endif
    if (sw->rtmod == WAN_RTMOD_IPV6 || sw->rtmod == WAN_RTMOD_DS)
    {
    	fprintf(stderr, "Starting DHCPv6 Client now\n");
    	system("/etc/utopia/service.d/service_dhcpv6_client.sh enable");	
    }

    sysctl_iface_set("/proc/sys/net/ipv4/ip_forward", NULL, "1");
    sysevent_set(sw->sefd, sw->setok, "current_wan_state", "up", 0);
    sysevent_set(sw->sefd, sw->setok, "firewall_flush_conntrack", "1", 0);

    sysevent_set(sw->sefd, sw->setok, "wan-status", "started", 0);
/*XB6 brlan0 comes up earlier so ned to find the way to restart the firewall
 IPv6 not yet supported so we can't restart in service routed  because of missing zebra.conf*/
#if  defined(INTEL_PUMA7) || defined(_COSA_BCM_ARM_) || defined(_PLATFORM_IPQ_)
        printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "firewall-restart", NULL, 0);
#endif

    fprintf(stderr, "[%s] Synching DNS to ATOM...\n", PROG_NAME);
    vsystem("/etc/utopia/service.d/service_wan/dns_sync.sh &");

    return 0;
}

static int wan_addr_unset(struct serv_wan *sw)
{

    sysevent_set(sw->sefd, sw->setok, "wan-status", "stopping", 0);
    sysevent_set(sw->sefd, sw->setok, "wan-errinfo", NULL, 0);

    sysevent_set(sw->sefd, sw->setok, "current_wan_ipaddr", "0.0.0.0", 0);
    sysevent_set(sw->sefd, sw->setok, "current_wan_subnet", "0.0.0.0", 0);
    sysevent_set(sw->sefd, sw->setok, "current_wan_state", "down", 0);

    switch (sw->prot) {
    case WAN_PROT_DHCP:
        if (wan_dhcp_stop(sw) != 0) {
            fprintf(stderr, "%s: wan_dhcp_stop error\n", __FUNCTION__);
            return -1;
        }

        break;
    case WAN_PROT_STATIC:
        if (wan_static_stop(sw) != 0) {
            fprintf(stderr, "%s: wan_static_stop error\n", __FUNCTION__);
            return -1;
        }

        break;
    default:
        fprintf(stderr, "%s: unknow wan protocol\n", __FUNCTION__);
        return -1;
    }

    vsystem("ip -4 addr flush dev %s", sw->ifname);

	fprintf(stderr, "Disabling DHCPv6 Client now\n");
    system("/etc/utopia/service.d/service_dhcpv6_client.sh disable");	

    printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
    sysevent_set(sw->sefd, sw->setok, "firewall-restart", NULL, 0);

#if defined(_PLATFORM_IPQ_)
    vsystem("killall -q dns_sync.sh");
#endif
    sysevent_set(sw->sefd, sw->setok, "wan-status", "stopped", 0);
    return 0;
}

static int wan_dhcp_start(struct serv_wan *sw)
{
    int pid; 
    int has_pid_file = 0;
#if defined(_PLATFORM_IPQ_)
    int ret = -1;
#endif

# if defined(_PLATFORM_IPQ_)
        pid = pid_of("udhcpc", sw->ifname);
#elif (defined _COSA_INTEL_XB3_ARM_) || (defined INTEL_PUMA7)
       {
        char udhcpflag[10]="";
        syscfg_get( NULL, "UDHCPEnable", udhcpflag, sizeof(udhcpflag));
        if( 0 == strcmp(udhcpflag,"true")){
                pid = pid_of("udhcpc", sw->ifname);
        }
        else
        {
                pid = pid_of("ti_udhcpc", sw->ifname);
        }
      }
#else
        pid = pid_of("udhcpc", sw->ifname);
#endif

    Getdhcpcpidfile(DHCPC_PID_FILE,sizeof(DHCPC_PID_FILE));
    if (access(DHCPC_PID_FILE, F_OK) == 0)
        has_pid_file = 1;

    if (pid > 0 && has_pid_file) {
        fprintf(stderr, "%s: DHCP client has already running as PID %d\n", __FUNCTION__, pid);
        return 0;
    }
    
    if (pid > 0 && !has_pid_file)
        kill(pid, SIGKILL);
    else if (pid <= 0 && has_pid_file)
        dhcp_stop(sw->ifname);

#if defined(_PLATFORM_IPQ_)
    /*
     * Setting few sysevent parameters which were previously getting set
     * in Gateway provisioning App. This is done to save the delay
     * in configuration and to support WAN restart functionality.
     */
    if ( 0 != (ret = dhcp_start(sw)) )
    {
       return ret;
    }

    system("sysevent set current_ipv4_link_state up");
    system("sysevent set ipv4_wan_ipaddr `ifconfig erouter0 \
                   | grep \"inet addr\" | cut -d':' -f2 | awk '{print$1}'`");
    system("sysevent set ipv4_wan_subnet `ifconfig erouter0 \
                   | grep \"inet addr\" | cut -d':' -f4 | awk '{print$1}'`");
    return 0;
#else
    return dhcp_start(sw);
#endif
}

static int wan_dhcp_stop(struct serv_wan *sw)
{
    return dhcp_stop(sw->ifname);
}

static int wan_dhcp_restart(struct serv_wan *sw)
{
    if (dhcp_stop(sw->ifname) != 0)
        fprintf(stderr, "%s: dhcp_stop error\n", __FUNCTION__);

    return dhcp_start(sw);
}

static int wan_dhcp_release(struct serv_wan *sw)
{
    FILE *fp;
    char pid[10];
    
    Getdhcpcpidfile(DHCPC_PID_FILE,sizeof(DHCPC_PID_FILE));
    if ((fp = fopen(DHCPC_PID_FILE, "rb")) == NULL)
        return -1;

    if (fgets(pid, sizeof(pid), fp) != NULL && atoi(pid) > 0)
        kill(atoi(pid), SIGUSR2); // triger DHCP release

    fclose(fp);

    vsystem("ip -4 addr flush dev %s", sw->ifname);
    return 0;
}

static int wan_dhcp_renew(struct serv_wan *sw)
{
    FILE *fp;
    char pid[10];
    char line[64], *cp;

    Getdhcpcpidfile(DHCPC_PID_FILE,sizeof(DHCPC_PID_FILE));
    if ((fp = fopen(DHCPC_PID_FILE, "rb")) == NULL)
        return dhcp_start(sw);

    if (fgets(pid, sizeof(pid), fp) != NULL && atoi(pid) > 0)
        kill(atoi(pid), SIGUSR1); // triger DHCP release

    fclose(fp);
    sysevent_set(sw->sefd, sw->setok, "current_wan_state", "up", 0);

    if ((fp = fopen("/proc/uptime", "rb")) == NULL)
        return -1;
    if (fgets(line, sizeof(line), fp) != NULL) {
        if ((cp = strchr(line, ',')) != NULL)
            *cp = '\0';
        sysevent_set(sw->sefd, sw->setok, "wan_start_time", line, 0);
    }
    fclose(fp);

    return 0;
}

static int resolv_static_config(struct serv_wan *sw)
{
    FILE *fp = NULL;
    char wan_domain[64] = {0};
    char name_server[3][32] = {0};
    char v6_name_server[32] = {0};
    int i = 0;
    char name_str[16] = {0};

    if((fp = fopen(RESOLV_CONF_FILE, "w+")) == NULL)
    {
        fprintf(stderr, "%s: Open %s error!\n", __FUNCTION__, RESOLV_CONF_FILE);
        return -1;
    }

    syscfg_get(NULL, "wan_domain", wan_domain, sizeof(wan_domain));
    if(wan_domain[0] != '\0') {
        fprintf(fp, "search %s\n", wan_domain);
        sysevent_set(sw->sefd, sw->setok, "dhcp_domain", wan_domain, 0);
    }

    memset(name_server, 0, sizeof(name_server));
    for(; i < 3; i++) {
        snprintf(name_str, sizeof(name_str), "nameserver%d", i+1);
        syscfg_get(NULL, name_str, name_server[i], sizeof(name_server[i]));
        if(name_server[i][0] != '\0' && strcmp(name_server[i], "0.0.0.0")) {
            printf("nameserver%d:%s\n", i+1, name_server[i]);
            fprintf(fp, "nameserver %s\n", name_server[i]);
        }
    }

    fclose(fp);
    return 0;
}

static int resolv_static_deconfig(struct serv_wan *sw)
{
    FILE *fp = NULL;

    if((fp = fopen(RESOLV_CONF_FILE, "w+")) == NULL) {
        fprintf(stderr, "%s: Open %s error!\n", __FUNCTION__, RESOLV_CONF_FILE);
        return -1;
    }

    fclose(fp);
    return 0;
}

static int wan_static_start(struct serv_wan *sw)
{
    char wan_ipaddr[16] = {0};
    char wan_netmask[16] = {0};
    char wan_default_gw[16] = {0};

    if(resolv_static_config(sw) != 0) {
        fprintf(stderr, "%s: Config resolv file failed!\n");
    }

    /*get static config*/
    syscfg_get(NULL, "wan_ipaddr", wan_ipaddr, sizeof(wan_ipaddr));
    syscfg_get(NULL, "wan_netmask", wan_netmask, sizeof(wan_netmask));
    syscfg_get(NULL, "wan_default_gateway", wan_default_gw, sizeof(wan_default_gw));

    if(vsystem("ip -4 addr add %s/%s broadcast + dev %s", wan_ipaddr, wan_netmask, sw->ifname) != 0) {
        fprintf(stderr, "%s: Add address to interface %s failed!\n", __FUNCTION__, sw->ifname);
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
    }

    if(vsystem("ip -4 link set %s up", sw->ifname) != 0) {
        fprintf(stderr, "%s: Set interface %s up failed!\n", __FUNCTION__, sw->ifname);
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
    }

    if(vsystem("ip -4 route add table erouter default dev %s via %s && "
                "ip rule add from %s lookup erouter", sw->ifname, wan_default_gw, wan_ipaddr) != 0)
    {
        fprintf(stderr, "%s: router related config failed!\n", __FUNCTION__);
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
    }

    /*set related sysevent*/
    sysevent_set(sw->sefd, sw->setok, "default_router", wan_default_gw, 0);
    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_ipaddr", wan_ipaddr, 0);
    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_subnet", wan_netmask, 0);
    sysevent_set(sw->sefd, sw->setok, "current_ipv4_link_state", "up", 0);
    sysevent_set(sw->sefd, sw->setok, "dhcp_server-restart", NULL, 0);

    return 0;
}

static int wan_static_stop(struct serv_wan *sw)
{
    char wan_ipaddr[16] = {0};

    if(resolv_static_deconfig(sw) != 0) {
        fprintf(stderr, "%s: deconfig resolv file failed!\n", __FUNCTION__);
    }

    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_ipaddr", "0.0.0.0", 0);
    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_subnet", "0.0.0.0", 0);

    sysevent_set(sw->sefd, sw->setok, "default_router", NULL, 0);
    syscfg_get(NULL, "wan_ipaddr", wan_ipaddr, sizeof(wan_ipaddr));
    vsystem("ip rule del from %s lookup erouter", wan_ipaddr);
    vsystem("ip -4 route del table erouter default dev %s", sw->ifname);

    sysevent_set(sw->sefd, sw->setok, "current_ipv4_link_state", "down", 0);

    return 0;
}

#if defined(_PLATFORM_IPQ_)
static int wan_static_start_v6(struct serv_wan *sw)
{
    unsigned char wan_ipaddr_v6[16] = {0};
    unsigned char wan_prefix_v6[16] = {0};
    unsigned char wan_default_gw_v6[16] = {0};

    /* get static ipv6 config */
    syscfg_get(NULL, "wan_ipv6addr", wan_ipaddr_v6, sizeof(wan_ipaddr_v6));
    syscfg_get(NULL, "wan_ipv6_prefix", wan_prefix_v6, sizeof(wan_prefix_v6));
    syscfg_get(NULL, "wan_ipv6_default_gateway", wan_default_gw_v6, sizeof(wan_default_gw_v6));

    /*
     * NOTE : Not checking for return, as it always returns -1
     */
    vsystem("ip -6 addr add %s/%s dev %s", wan_ipaddr_v6, wan_prefix_v6, sw->ifname);

    vsystem("ip -6 route add table erouter default dev %s via %s && "
                "ip -6 rule add from %s lookup erouter", sw->ifname, wan_default_gw_v6, wan_ipaddr_v6);

    if (sw->rtmod == WAN_RTMOD_IPV6)
       sysevent_set(sw->sefd, sw->setok, "wan-status", "started", 0);

    return 0;
}

static int wan_static_stop_v6(struct serv_wan *sw)
{
    unsigned char wan_ipaddr_v6[16] = {0};
    unsigned char wan_prefix_v6[16] = {0};
    unsigned char wan_default_gw_v6[16] = {0};

    /* get static ipv6 config */
    syscfg_get(NULL, "wan_ipv6addr", wan_ipaddr_v6, sizeof(wan_ipaddr_v6));
    syscfg_get(NULL, "wan_ipv6_prefix", wan_prefix_v6, sizeof(wan_prefix_v6));
    syscfg_get(NULL, "wan_ipv6_default_gateway", wan_default_gw_v6, sizeof(wan_default_gw_v6));

    /*
     * NOTE : Not checking for return, as it always returns -1
     */
    vsystem("ip -6 addr del %s/%s dev %s", wan_ipaddr_v6, wan_prefix_v6, sw->ifname);

    vsystem("ip -6 route del table erouter default dev %s via %s && "
                "ip -6 rule del from %s lookup erouter", sw->ifname, wan_default_gw_v6, wan_ipaddr_v6);

    if (sw->rtmod == WAN_RTMOD_IPV6)
       sysevent_set(sw->sefd, sw->setok, "wan-status", "stopped", 0);

    return 0;
}
#endif

static int serv_wan_init(struct serv_wan *sw, const char *ifname, const char *prot)
{
    char buf[32];

    if ((sw->sefd = sysevent_open(SE_SERV, SE_SERVER_WELL_KNOWN_PORT, 
                    SE_VERSION, PROG_NAME, &sw->setok)) < 0) {
        fprintf(stderr, "%s: fail to open sysevent\n", __FUNCTION__);
        return -1;
    }

    if (syscfg_init() != 0) {
        fprintf(stderr, "%s: fail to init syscfg\n", __FUNCTION__);
        return -1;
    }

    if (ifname)
        snprintf(sw->ifname, sizeof(sw->ifname), "%s", ifname);
    else
        syscfg_get(NULL, "wan_physical_ifname", sw->ifname, sizeof(sw->ifname));

    if (!strlen(sw->ifname)) {
        fprintf(stderr, "%s: fail to get ifname\n", __FUNCTION__);
        return -1;
    }

    if (prot)
        snprintf(buf, sizeof(buf), "%s", prot);
    else
        syscfg_get(NULL, "wan_proto", buf, sizeof(buf));

    /* IPQ Platform : For WAN stop, protocol field will be modified in
     * the WAN stop functionality */
    if (strcasecmp(buf, "dhcp") == 0)
        sw->prot = WAN_PROT_DHCP;
    else if (strcasecmp(buf, "static") == 0)
        sw->prot = WAN_PROT_STATIC;
    else {
        fprintf(stderr, "%s: fail to get wan protocol\n", __FUNCTION__);
        return -1;
    }

    syscfg_get(NULL, "last_erouter_mode", buf, sizeof(buf));
    switch (atoi(buf)) {
    case 1:
        sw->rtmod = WAN_RTMOD_IPV4;
        break;
    case 2:
        sw->rtmod = WAN_RTMOD_IPV6;
        break;
    case 3:
        sw->rtmod = WAN_RTMOD_DS;
        break;
    default:
        fprintf(stderr, "%s: unknow RT mode (last_erouter_mode)\n", __FUNCTION__);
        sw->rtmod = WAN_RTMOD_UNKNOW;
        break;
    }

    sw->timo = SW_PROT_TIMO;

    return 0;
}

static int serv_wan_term(struct serv_wan *sw)
{
    sysevent_close(sw->sefd, sw->setok);
    return 0;
}

static void usage(void)
{
    int i;

    fprintf(stderr, "USAGE\n");
    fprintf(stderr, "    %s COMMAND [ INTERFACE [ PROTOCOL ] ]\n", PROG_NAME);
    fprintf(stderr, "COMMANDS\n");
    for (i = 0; i < NELEMS(cmd_ops); i++)
        fprintf(stderr, "    %-20s%s\n", cmd_ops[i].cmd, cmd_ops[i].desc);
    fprintf(stderr, "PROTOCOLS\n");
        fprintf(stderr, "    dhcp, static\n");
}

int main(int argc, char *argv[])
{
    int i;
    struct serv_wan sw;

    fprintf(stderr, "[%s] -- IN\n", PROG_NAME);

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (serv_wan_init(&sw, (argc > 2 ? argv[2] : NULL), (argc > 3 ? argv[3] : NULL)) != 0)
        exit(1);

    /* execute commands */
    for (i = 0; i < NELEMS(cmd_ops); i++) {
        if (strcmp(argv[1], cmd_ops[i].cmd) != 0 || !cmd_ops[i].exec)
            continue;

        fprintf(stderr, "[%s] exec: %s\n", PROG_NAME, cmd_ops[i].cmd);

        if (cmd_ops[i].exec(&sw) != 0)
            fprintf(stderr, "[%s]: fail to exec `%s'\n", PROG_NAME, cmd_ops[i].cmd);

        break;
    }
    if (i == NELEMS(cmd_ops))
        fprintf(stderr, "[%s] unknown command: %s\n", PROG_NAME, argv[1]);

    if (serv_wan_term(&sw) != 0)
        exit(1);

    fprintf(stderr, "[%s] -- OUT\n", PROG_NAME);
    exit(0);
}
