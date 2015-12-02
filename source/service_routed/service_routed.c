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
 * C version of "service_routed.sh" script.
 *
 * The reason to re-implement service_routed with C is for boot time,
 * shell scripts is too slow.
 */

/* 
 * since this utility is event triggered (instead of daemon),
 * we have to use some global var to (sysevents) mark the states. 
 * I prefer daemon, so that we can write state machine clearly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <net/if.h>
#include <signal.h>
#include "util.h"

#define PROG_NAME       "SERVICE-ROUTED"


#define ZEBRA_PID_FILE  "/var/zebra.pid"
#define RIPD_PID_FILE   "/var/ripd.pid"
#define ZEBRA_CONF_FILE "/etc/zebra.conf"
#define RIPD_CONF_FILE  "/etc/ripd.conf"

struct serv_routed {
    int         sefd;
    int         setok;

    bool        lan_ready;
    bool        wan_ready;
};

static int fw_restart(struct serv_routed *sr)
{
    char val[16];
    char wan_if[IFNAMSIZ];

    sysevent_get(sr->sefd, sr->setok, "parcon_nfq_status", val, sizeof(val));
    if (strcmp(val, "started") != 0) {
        syscfg_get(NULL, "wan_physical_ifname", wan_if, sizeof(wan_if));

        iface_get_hwaddr(wan_if, val, sizeof(val));
        vsystem("((nfq_handler 4 %s &)&)", val);
        sysevent_set(sr->sefd, sr->setok, "parcon_nfq_status", "started", 0);
    }

    sysevent_set(sr->sefd, sr->setok, "firewall-restart", NULL, 0);
    return 0;
}

static int daemon_stop(const char *pid_file, const char *prog)
{
    FILE *fp;
    char pid_str[10];
    int pid = -1;

    if (!pid_file && !prog)
        return -1;

    if (pid_file) {
        if ((fp = fopen(pid_file, "rb")) != NULL) {
            if (fgets(pid_str, sizeof(pid_str), fp) != NULL && atoi(pid_str) > 0)
                pid = atoi(pid_str);

            fclose(fp);
        }
    }

    if (pid <= 0 && prog)
        pid = pid_of(prog, NULL);

    if (pid > 0) {
        kill(pid, SIGTERM);
    }
    
    if (pid_file)
        unlink(pid_file);
    return 0;
}

static int route_set(struct serv_routed *sr)
{
    if (vsystem("ip -6 rule add iif brlan0 table erouter;"
            "gw=$(ip -6 route show default dev erouter0 | awk '/via/ {print $3}');"
            "if [ \"$gw\" != \"\" ]; then"
            "  ip -6 route add default via $gw dev erouter0 table erouter;"
            "fi") != 0)
        return -1;
    return 0;
}

static int route_unset(struct serv_routed *sr)
{
    
    if (vsystem("ip -6 route del default dev erouter0 table erouter") != 0)
        printf("rule doen't exist. Continue... \n");
    if (vsystem("ip -6 rule del iif brlan0 table erouter") != 0)
        return -1;

    return 0;
}

static int gen_zebra_conf(int sefd, token_t setok)
{
    FILE *fp;
    char rtmod[16], static_rt_cnt[16], ra_en[16], dh6s_en[16];
    char name_servs[1024] = {0};
    char prefix[64], orig_prefix[64], lan_addr[64];
    char preferred_lft[16], valid_lft[16], wan_status[32];
    char m_flag[16], o_flag[16];
    char rec[256], val[512];
    char buf[6];
    FILE *responsefd;
    char *networkResponse = "/var/tmp/networkresponse.txt";
    int iresCode = 0;
    char responseCode[10];
    int inCaptivePortal = 0;
    int nopt, i;
    char lan_if[IFNAMSIZ];
    char *start, *tok, *sp;
    static const char *zebra_conf_base = \
        "hostname zebra\n"
        "!password zebra\n"
        "!enable password admin\n"
        "!log stdout\n"
        "log syslog\n"
        "log file /var/log/zebra.log\n"
        "table 255\n";

    if ((fp = fopen(ZEBRA_CONF_FILE, "wb")) < 0) {
        fprintf(stderr, "%s: fail to open file %s\n", __FUNCTION__, ZEBRA_CONF_FILE);
        return -1;
    }

    if (fwrite(zebra_conf_base, strlen(zebra_conf_base), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

    /* TODO: static route */

    syscfg_get(NULL, "router_adv_enable", ra_en, sizeof(ra_en));
    if (strcmp(ra_en, "1") != 0) {
        fclose(fp);
        return 0;
    }

    sysevent_get(sefd, setok, "ipv6_prefix", prefix, sizeof(prefix));
    sysevent_get(sefd, setok, "previous_ipv6_prefix", orig_prefix, sizeof(orig_prefix));
    sysevent_get(sefd, setok, "current_lan_ipv6address", lan_addr, sizeof(lan_addr));
    sysevent_get(sefd, setok, "ipv6_prefix_prdtime", preferred_lft, sizeof(preferred_lft));
    sysevent_get(sefd, setok, "ipv6_prefix_vldtime", valid_lft, sizeof(valid_lft));
    sysevent_get(sefd, setok, "current_wan_state", wan_status, sizeof(wan_status));

    if (atoi(preferred_lft) <= 0)
        snprintf(preferred_lft, sizeof(preferred_lft), "300");
    if (atoi(valid_lft) <= 0)
        snprintf(valid_lft, sizeof(valid_lft), "300");
    syscfg_get(NULL, "lan_ifname", lan_if, sizeof(lan_if));

    fprintf(fp, "# Based on prefix=%s, old_previous=%s, LAN IPv6 address=%s\n", 
            prefix, orig_prefix, lan_addr);

    if (strlen(prefix) || strlen(orig_prefix)) {
        fprintf(fp, "interface %s\n", lan_if);
        fprintf(fp, "   no ipv6 nd suppress-ra\n");
        if (strlen(prefix))
            fprintf(fp, "   ipv6 nd prefix %s %s %s\n", prefix, valid_lft, preferred_lft);
/*
        if (strlen(orig_prefix))
            fprintf(fp, "   ipv6 nd prefix %s 300 0\n", orig_prefix);
*/
	fprintf(fp, "   ipv6 nd ra-interval 3\n");

	if(strcmp(wan_status,"up" ) != 0 )
	{
	        if (strlen(orig_prefix))
        	    fprintf(fp, "   ipv6 nd prefix %s 300 0\n", orig_prefix);

                fprintf(fp, "   ipv6 nd ra-lifetime 0\n");
	}
	else
        	fprintf(fp, "   ipv6 nd ra-lifetime 180\n");

        syscfg_get(NULL, "router_managed_flag", m_flag, sizeof(m_flag));
        if (strcmp(m_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd managed-config-flag\n");

        syscfg_get(NULL, "router_other_flag", o_flag, sizeof(o_flag));
        if (strcmp(o_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd other-config-flag\n");

        syscfg_get(NULL, "dhcpv6s_enable", dh6s_en, sizeof(dh6s_en));
        if (strcmp(dh6s_en, "1") == 0)
            fprintf(fp, "   ipv6 nd other-config-flag\n");

        fprintf(fp, "   ipv6 nd router-preference medium\n");

	// During captive portal no need to pass DNS
	// Check the reponse code received from Web Service
   	if((responsefd = fopen(networkResponse, "r")) != NULL) 
   	{
       		if(fgets(responseCode, sizeof(responseCode), responsefd) != NULL)
       		{
		  	iresCode = atoi(responseCode);
          	}
   	}
        syscfg_get( NULL, "redirection_flag", buf, sizeof(buf));
    	if( buf != NULL )
    	{
		if ((strncmp(buf,"true",4) == 0) && iresCode == 204)
		{
			inCaptivePortal = 1;        		
		}
	}
	
	if(inCaptivePortal != 1)
	{
		if (strlen(lan_addr))
            			fprintf(fp, "   ipv6 nd rdnss %s 300\n", lan_addr);
	}

        /* static IPv6 DNS */
        syscfg_get(NULL, "dhcpv6spool00::optionnumber", val, sizeof(val));
        nopt = atoi(val);
        for (i = 0; i < nopt; i++) {
            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::bEnabled", i);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (atoi(val) != 1)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::Tag", i);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (atoi(val) != 23)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::PassthroughClient", i);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (strlen(val) > 0)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::Value", i);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (strlen(val) == 0)
                continue;

            for (start = val; (tok = strtok_r(start, ", \r\t\n", &sp)); start = NULL) {
                snprintf(name_servs + strlen(name_servs), 
                        sizeof(name_servs) - strlen(name_servs), "%s ", tok);
            }
        }

	if(inCaptivePortal != 1)
	{
       		/* DNS from WAN (if no static DNS) */
       		if (strlen(name_servs) == 0) {
       			sysevent_get(sefd, setok, "ipv6_nameserver", name_servs + strlen(name_servs), 
               		sizeof(name_servs) - strlen(name_servs));
       		}

        		for (start = name_servs; (tok = strtok_r(start, " ", &sp)); start = NULL)
            		fprintf(fp, "   ipv6 nd rdnss %s 300\n", tok);
		}
	}
    

    fprintf(fp, "interface %s\n", lan_if);
    fprintf(fp, "   ip irdp multicast\n");

    fclose(fp);
    return 0;
}

static int gen_ripd_conf(int sefd, token_t setok)
{
    /* should be similar to CosaDmlGenerateRipdConfigFile(), 
     * but there're too many dependencies for that function.
     * It's not good, but DM will generate that file */
    return 0;
}

static int radv_start(struct serv_routed *sr)
{
    /* XXX: 
     * 1) even IPv4 only zebra should start (ripd need it) !
     * 2) IPv6-only do not use wan-status  */
#if 0
    char rtmod[16];

    syscfg_get(NULL, "last_erouter_mode", rtmod, sizeof(rtmod));
    if (atoi(rtmod) != 2 && atoi(rtmod) != 3) { /* IPv6 or Dual-Stack */
        fprintf(stderr, "%s: last_erouter_mode %s\n", __FUNCTION__, rtmod);
        return 0;
    }

    if (!sr->lan_ready || !sr->wan_ready) {
        fprintf(stderr, "%s: LAN or WAN is not ready !\n", __FUNCTION__);
        return -1;
    }
#else
    if (!sr->lan_ready) {
        fprintf(stderr, "%s: LAN is not ready !\n", __FUNCTION__);
        return -1;
    }
#endif

    if (gen_zebra_conf(sr->sefd, sr->setok) != 0) {
        fprintf(stderr, "%s: fail to save zebra config\n", __FUNCTION__);
        return -1;
    }

    daemon_stop(ZEBRA_PID_FILE, "zebra");
    vsystem("zebra -d -f %s -u root", ZEBRA_CONF_FILE);
    return 0;
}

static int radv_stop(struct serv_routed *sr)
{
    return daemon_stop(ZEBRA_PID_FILE, "zebra");
}

static int radv_restart(struct serv_routed *sr)
{
    if (radv_stop(sr) != 0)
        fprintf(stderr, "%s: radv_stop error\n", __FUNCTION__);

    return radv_start(sr);
}

static int rip_start(struct serv_routed *sr)
{
    char enable[16];

    if (!serv_can_start(sr->sefd, sr->setok, "rip"))
        return -1;

    if (!sr->lan_ready || !sr->wan_ready) {
        fprintf(stderr, "%s: LAN or WAN is not ready !\n", __FUNCTION__);
        return -1;
    }

    sysevent_get(sr->sefd, sr->setok, "rip_enabled", enable, sizeof(enable));
    if (strcmp(enable, "1") != 0) {
        fprintf(stderr, "%s: RIP not enabled\n", __FUNCTION__);
        return 0;
    }

    sysevent_set(sr->sefd, sr->setok, "rip-status", "starting", 0);

    if (gen_ripd_conf(sr->sefd, sr->setok) != 0) {
        fprintf(stderr, "%s: fail to generate ripd config\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
        return -1;
    }

    if (vsystem("ripd -d -f %s -u root", RIPD_CONF_FILE) != 0) {
        sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
        return -1;
    }

    sysevent_set(sr->sefd, sr->setok, "rip-status", "started", 0);
    return 0;
}

static int rip_stop(struct serv_routed *sr)
{
    if (!serv_can_stop(sr->sefd, sr->setok, "rip"))
        return -1;

    sysevent_set(sr->sefd, sr->setok, "rip-status", "stopping", 0);

    if (daemon_stop(RIPD_PID_FILE, "ripd") != 0) {
        sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
        return -1;
    }

    sysevent_set(sr->sefd, sr->setok, "rip-status", "stopped", 0);
    return 0;
}

static int rip_restart(struct serv_routed *sr)
{
    if (rip_stop(sr) != 0)
        fprintf(stderr, "%s: rip_stop error\n", __FUNCTION__);

    return rip_start(sr);
}

static int serv_routed_start(struct serv_routed *sr)
{
    char status[16], enable[16], rtmod[16];
    char prefix[64];

    /* state check */
    if (!serv_can_start(sr->sefd, sr->setok, "routed"))
        return -1;

    if (!sr->lan_ready) {
        fprintf(stderr, "%s: LAN is not ready !\n", __FUNCTION__);
        return -1;
    }

    syscfg_get(NULL, "last_erouter_mode", rtmod, sizeof(rtmod));
    if (atoi(rtmod) != 2) { /* IPv4-only or Dual-Stack */
        if (!sr->wan_ready) {
            fprintf(stderr, "%s: IPv4-WAN is not ready !\n", __FUNCTION__);
            return -1;
        }
    } else { /* IPv6-only */
        sysevent_get(sr->sefd, sr->setok, "ipv6_prefix", prefix, sizeof(prefix));
        if (strlen(prefix) == 0) {
            fprintf(stderr, "%s: IPv6-WAN is not ready !\n", __FUNCTION__);
            return -1;
        }
    }

    sysevent_set(sr->sefd, sr->setok, "routed-status", "starting", 0);

    /* RA daemon */
    if (radv_start(sr) != 0) {
        fprintf(stderr, "%s: radv_start error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    /* RIP daemon */
    if (rip_start(sr) != 0) {
        fprintf(stderr, "%s: rip_start error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    /* route and policy routes */
    if (route_set(sr) != 0) {
        fprintf(stderr, "%s: route_set error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    /* nfq & firewall */
    if (fw_restart(sr) != 0) {
        fprintf(stderr, "%s: fw_restart error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    sysevent_set(sr->sefd, sr->setok, "routed-status", "started", 0);
    return 0;
}

static int serv_routed_stop(struct serv_routed *sr)
{
    if (!serv_can_stop(sr->sefd, sr->setok, "routed"))
        return -1;

    sysevent_set(sr->sefd, sr->setok, "routed-status", "stopping", 0);

    if (route_unset(sr) != 0)
        fprintf(stderr, "%s: route_unset error\n", __FUNCTION__);

    if (rip_stop(sr) != 0)
        fprintf(stderr, "%s: rip_stop error\n", __FUNCTION__);

    if (radv_stop(sr) != 0)
        fprintf(stderr, "%s: radv_stop error\n", __FUNCTION__);

    if (fw_restart(sr) != 0)
        fprintf(stderr, "%s: fw_restart error\n", __FUNCTION__);

    sysevent_set(sr->sefd, sr->setok, "routed-status", "stopped", 0);
    return 0;
}

static int serv_routed_restart(struct serv_routed *sr)
{
    if (serv_routed_stop(sr) != 0)
        fprintf(stderr, "%s: serv_routed_stop error\n", __FUNCTION__);

    return serv_routed_start(sr);
}

static int serv_routed_init(struct serv_routed *sr)
{
    char wan_st[16], lan_st[16];
    char buf[16];

    memset(sr, 0, sizeof(struct serv_routed));

    if ((sr->sefd = sysevent_open(SE_SERV, SE_SERVER_WELL_KNOWN_PORT, 
                    SE_VERSION, PROG_NAME, &sr->setok)) < 0) {
        fprintf(stderr, "%s: fail to open sysevent\n", __FUNCTION__);
        return -1;
    }

    if (syscfg_init() != 0) {
        fprintf(stderr, "%s: fail to init syscfg\n", __FUNCTION__);
        return -1;
    }

    sysevent_get(sr->sefd, sr->setok, "wan-status", wan_st, sizeof(wan_st));
    if (strcmp(wan_st, "started") == 0)
        sr->wan_ready = true;

    sysevent_get(sr->sefd, sr->setok, "lan-status", lan_st, sizeof(lan_st));
    if (strcmp(lan_st, "started") == 0)
        sr->lan_ready = true;

    return 0;
}

static int serv_routed_term(struct serv_routed *sr)
{
    sysevent_close(sr->sefd, sr->setok);
    return 0;
}

struct cmd_op {
    const char  *cmd;
    int         (*exec)(struct serv_routed *sr);
    const char  *desc;
};

static struct cmd_op cmd_ops[] = {
    {"start",       serv_routed_start,  "start service route daemons"},
    {"stop",        serv_routed_stop,   "stop service route daemons"},
    {"restart",     serv_routed_restart,"restart service route daemons"},
    {"route-set",   route_set,      "set route entries"},
    {"route-unset", route_unset,    "unset route entries"},
    {"rip-start",   rip_start,      "start RIP daemon"},
    {"rip-stop",    rip_stop,       "stop RIP daemon"},
    {"rip-restart", rip_restart,    "restart RIP daemon"},
    {"radv-start",  radv_start,     "start RA daemon"},
    {"radv-stop",   radv_stop,      "stop RA daemon"},
    {"radv-restart",radv_restart,   "restart RA daemon"},
};

static void usage(void)
{
    int i;

    fprintf(stderr, "USAGE\n");
    fprintf(stderr, "    %s COMMAND\n", PROG_NAME);
    fprintf(stderr, "COMMANDS\n");
    for (i = 0; i < NELEMS(cmd_ops); i++)
        fprintf(stderr, "    %-20s%s\n", cmd_ops[i].cmd, cmd_ops[i].desc);
}

int main(int argc, char *argv[])
{
    int i;
    struct serv_routed sr;

    fprintf(stderr, "[%s] -- IN\n", PROG_NAME);

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (serv_routed_init(&sr) != 0)
        exit(1);

    for (i = 0; i < NELEMS(cmd_ops); i++) {
        if (strcmp(argv[1], cmd_ops[i].cmd) != 0 || !cmd_ops[i].exec)
            continue;

        fprintf(stderr, "[%s] EXEC: %s\n", PROG_NAME, cmd_ops[i].cmd);

        if (cmd_ops[i].exec(&sr) != 0)
            fprintf(stderr, "[%s]: fail to exec `%s'\n", PROG_NAME, cmd_ops[i].cmd);

        break;
    }
    if (i == NELEMS(cmd_ops))
        fprintf(stderr, "[%s] unknown command: %s\n", PROG_NAME, argv[1]);

    if (serv_routed_term(&sr) != 0)
        exit(1);

    fprintf(stderr, "[%s] -- OUT\n", PROG_NAME);
    exit(0);
}
