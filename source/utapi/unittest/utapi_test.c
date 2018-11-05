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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include <utapi/utapi.h>
#include <utapi/lib/utapi_wlan.h>


/*
 * utapi_unittest.c - 
 */

#ifdef ZERO

/* Router-Extension C API method function prototypes */

Needed pages,


* Setup > Basic Setup
* * Setup > Basic Setup > DHCP Reservation
* * Setup > DDNS
* * Administration > Firmware Upgrade
* * Status > Router
*  
*  * Administration > Factory Defaults
*  * Administration > Management > Change Router Password
*  * Applications & Gaming > Port Forwarding
*  * Access Restrictions > Internet Access Policy > Schedule

#endif 

#if 0
static void
set_basic_setup ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}
	lanSetting_t lan;

	strncpy(lan.ipaddr, "192.168.22.1", sizeof(lan.ipaddr));
	strncpy(lan.netmask, "255.255.255.0", sizeof(lan.netmask));
	strncpy(lan.domain, "utopia.net", sizeof(lan.domain));

	int rc = Utopia_SetLanSettings(&ctx, &lan);
	if (SUCCESS != rc) {
		printf("<P> Error: SetLanSettings() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, !rc);
}

static void
print_ppp_settings (wan_ppp_t *ppp)
{
	printf("<P> PPP settings()::\n"
		   "\tusername: %s\n"
		   "\tpassword: %s\n"
		   "\tservice_name: %s\n"
		   "\tserver_ipaddr: %s\n"
		   "\tconn_method: %d\n"
		   "\tmax_idle_time: %d\n"
		   "\tredial_period: %d\n"
		   "\tipAddrStatic: %d\n",
		   ppp->username,
		   ppp->password,
		   ppp->service_name,
		   ppp->server_ipaddr,
		   ppp->conn_method,
		   ppp->max_idle_time,
		   ppp->redial_period,
		   ppp->ipAddrStatic);
}

static void
print_staticwan_settings (wan_static_t *st)
{
	printf("<P> WAN Static settings()::\n"
		   "\tipaddr: %s\n"
		   "\tnetmask: %s\n"
		   "\tdefault gw: %s\n"
		   "\tdns ipaddr1: %s\n"
		   "\tdns ipaddr1: %s\n"
		   "\tdns ipaddr1: %s\n",
		   st->ip_addr,
		   st->subnet_mask,
		   st->default_gw,
		   st->dns_ipaddr1,
		   st->dns_ipaddr2,
		   st->dns_ipaddr3);
}

static void
print_wan_settings ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	wanInfo_t wan;
	bzero(&wan, sizeof(wanInfo_t));
	int rc = Utopia_GetWANSettings(&ctx, &wan);
	if (SUCCESS == rc) {
		printf("<P> GetWANSettings()::\n"
			   "\twan_proto: %d\n"
			   "\tdomainname: %s\n"
			   "\tauto_mtu: %d\n"
			   "\tmtu_size: %d\n",
			   wan.wan_proto,
			   wan.domainname,
			   wan.auto_mtu,
			   wan.mtu_size);
		if (PPPOE == wan.wan_proto ||
			PPTP == wan.wan_proto ||
			L2TP == wan.wan_proto) {
			print_ppp_settings(&wan.ppp);
			if (wan.ppp.ipAddrStatic) {
				print_staticwan_settings(&wan.wstatic);
			}
		}
		if (STATIC == wan.wan_proto) {
			print_staticwan_settings(&wan.wstatic);
		}
	} else {
		printf("<P> Error: GetWANSettings() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, 1);
}

static void
set_wan_settings ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	wanInfo_t wan;

	bzero(&wan, sizeof(wanInfo_t));

	wan.wan_proto = PPPOE;
	wan.auto_mtu = 0;
	wan.mtu_size = 1300;
	strncpy(wan.domainname, "wan_utopia.net", sizeof(wan.domainname));

	wan.ppp.conn_method = 1;
	wan.ppp.max_idle_time = 299;
	wan.ppp.redial_period = 499;
	wan.ppp.ipAddrStatic = 1;
	strncpy(wan.ppp.username, "test_wan_username", sizeof(wan.ppp.username));
	strncpy(wan.ppp.password, "test_wan_password", sizeof(wan.ppp.password));
	strncpy(wan.ppp.service_name, "test_service_name", sizeof(wan.ppp.service_name));
	strncpy(wan.ppp.server_ipaddr, "10.23.11.1", sizeof(wan.ppp.server_ipaddr));

	strncpy(wan.wstatic.ip_addr, "10.1.1.110", sizeof(wan.ppp.username));
	strncpy(wan.wstatic.subnet_mask, "255.255.255.0", sizeof(wan.wstatic.subnet_mask));
	strncpy(wan.wstatic.default_gw, "10.1.1.1", sizeof(wan.wstatic.default_gw));
	strcpy(wan.wstatic.dns_ipaddr1, "10.1.10.11");
	strcpy(wan.wstatic.dns_ipaddr2, "10.1.10.12");
	strcpy(wan.wstatic.dns_ipaddr3, "10.1.10.13");

	int rc = Utopia_SetWANSettings(&ctx, &wan);
	if (SUCCESS != rc) {
		printf("<P> Error: SetWANSettings() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, !rc);
}


static void
print_basic_setup ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	lanSetting_t lan;
	int rc = Utopia_GetLanSettings(&ctx, &lan);
	if (SUCCESS == rc) {
		printf("<P>GetLanSettings()\n"
			   "\t ip=[%s]\n"
			   "\t netmask=[%s]"
			   "\t domain=[%s]\n",
			   lan.ipaddr, lan.netmask, lan.domain);
	} else {
		printf("<P> Error: GetLanSettings() call failed: %d\n", rc);
	}

	deviceSetting_t device;
	rc = Utopia_GetDeviceSettings(&ctx, &device);
	if (SUCCESS == rc) {
		printf("<P> GetDeviceSettings()::\n"
			   "\tHostname: %s\n"
			   "\tLang: %s\n"
			   "\tTZ: %s\n"
			   "\tAutoAdjustDST: %d\n",
			   device.hostname, device.lang, device.tz_gmt_offset, device.auto_dst);
	} else {
		printf("<P> Error: GetDeviceSettings() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, 1);
}

static void
print_port_map (int index, portFwdSingle_t *portmap)
{
	printf("<P> port_map - %d\n"
		   "\t\t enabled - %d\n"
		   "\t\t name - %s\n"
		   "\t\t external port - %d\n"
		   "\t\t internal port - %d\n"
		   "\t\t protocol - %d\n"
		   "\t\t dest_ip - %d\n",
		   index,
		   portmap->enabled,
		   portmap->name,
		   portmap->external_port,
		   portmap->internal_port,
		   portmap->protocol,
		   portmap->dest_ip);
}

static void
print_port_mapping ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	portFwdSingle_t *portmap = NULL;
	int count = 0;

	int rc = Utopia_GetPortForwardingCount(&ctx, &count);
	if (SUCCESS == rc) {
		printf("<P>GetPortForwardingCount()\n"
			   "\t count=[%d]\n",
			   count);
	}

	rc = Utopia_GetPortForwarding(&ctx, &count, &portmap);
	if (SUCCESS == rc) {
		printf("<P>GetPortForwarding()\n"
			   "\t count=[%d]\n",
			   count);
		if (count > 0) {
			int i;
			if (portmap) {
				for (i = 0; i < count; i++) {
					print_port_map(i+1, &portmap[i]);
				}
				free(portmap);
			} else {
				printf("<P> Error: GetPortForwarding() return NULL portmap: %d\n", rc);
			}
		}
	} else {
		printf("<P> Error: GetPortForwarding() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, 1);
}

static void
set_port_mapping ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	portFwdSingle_t portmap[3];
	int count = 3;

	// populate
	portmap[0].enabled = TRUE;
	strcpy(portmap[0].name, "portmap 0");;
	portmap[0].external_port = 65000;
	portmap[0].internal_port = 52000;
	portmap[0].protocol = TCP;
	portmap[0].dest_ip = 100;


	portmap[1].enabled = TRUE;
	strcpy(portmap[1].name, "portmap 1");;
	portmap[1].external_port = 2500;
	portmap[1].internal_port = 5200;
	portmap[1].protocol = UDP;
	portmap[1].dest_ip = 120;

	portmap[2].enabled = FALSE;
	strcpy(portmap[2].name, "portmap 2");;
	portmap[2].external_port = 1102;
	portmap[2].internal_port = 1103;
	portmap[2].protocol = BOTH_TCP_UDP;
	portmap[2].dest_ip = 52;

	int rc = Utopia_SetPortForwarding(&ctx, count, portmap);
	if (SUCCESS != rc) {
		printf("<P> Error: SetPortForwarding() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, !rc);
}

static void
add_port_mapping ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	portFwdSingle_t portmap[3];

	// populate
	portmap[0].enabled = TRUE;
	strcpy(portmap[0].name, "addmap - 1");;
	portmap[0].external_port = 65000;
	portmap[0].internal_port = 52000;
	portmap[0].protocol = TCP;
	portmap[0].dest_ip = 100;


/*
	portmap[1].enabled = TRUE;
	strcpy(portmap[1].name, "portmap 1");;
	portmap[1].external_port = 2500;
	portmap[1].internal_port = 5200;
	portmap[1].protocol = UDP;
	portmap[1].dest_ip = 120;

	portmap[2].enabled = FALSE;
	strcpy(portmap[2].name, "portmap 2");;
	portmap[2].external_port = 1102;
	portmap[2].internal_port = 1103;
	portmap[2].protocol = BOTH_TCP_UDP;
	portmap[2].dest_ip = 52;
*/

	int rc = Utopia_AddPortForwarding(&ctx, portmap);
	if (SUCCESS != rc) {
		printf("<P> Error: AddPortForwarding() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, !rc);
}

static void
print_log_settings ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	char logviewer[IPHOSTNAME_SZ];
	int logenabled;
	int rc = Utopia_GetLogSettings(&ctx, &logenabled, logviewer, sizeof(logviewer));
	if (SUCCESS == rc) {
		printf("<P>GetLogSettings()\n"
			   "\t enabled=[%d]\n"
			   "\t logviewer=[%s]\n",
			   logenabled, logviewer);
	} else {
		printf("<P> Error: GetLogSettings() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, 1);
}

static void
set_log_settings ()
{
	UtopiaContext ctx;

	if (!Utopia_Init(&ctx)) {
		printf("Error initializing context\n");
		return;
	}

	char logviewer[IPHOSTNAME_SZ];
	int log_enabled = 1;
	strncpy(logviewer, "192.168.1.122", sizeof(logviewer));

	int rc = Utopia_SetLogSettings(&ctx, log_enabled, logviewer);
	if (SUCCESS != rc) {
		printf("<P> Error: GetLogSettings() call failed: %d\n", rc);
	}

	Utopia_Free(&ctx, 1);
}


static void
set_dhcp_static_hosts ()
{
	DHCPMap_t map[2];
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		strcpy(map[0].client_name, "tst_static_1");
		strcpy(map[0].macaddr, "11:22:33:44:55:66");
		map[0].host_ip = 22;

		strcpy(map[1].client_name, "tst_static_2");
		strcpy(map[1].macaddr, "66:22:33:44:55:66");
		map[1].host_ip = 33;

		int rc = Utopia_SetDHCPServerStaticHosts(&ctx, 2, map);
		if (SUCCESS == rc) {
			printf("<P> Utopia_SetDHCPServerStaticHosts() rc = %d\n", rc);
			Utopia_Free(&ctx, 1);
		} else {
			printf("<P> Error Utopia_SetDHCPServerStaticHosts() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
print_dhcp_client_list ()
{
	dhcpLANHost_t *client_info = NULL;
	int count = 0;

	int rc = Utopia_GetDHCPServerLANHosts(NULL, &count, &client_info);
	if (SUCCESS == rc) {
		printf("<P> Utopia_GetDHCPServerLANHosts() count = %d\n", count);
		int i;
		for (i = 0; i < count; i++) {
			printf("<P> Utopia_GetDHCPServerLANHosts() %s %s %s %s %ld %d\n",
				   client_info[i].macaddr,
				   client_info[i].ipaddr,
				   client_info[i].hostname,
				   client_info[i].client_id,
				   client_info[i].leasetime,
				   client_info[i].lan_interface);
		}
	} else {
		printf("<P> Calling: Utopia_GetDHCPServerLANHosts() returned: %d\n", rc);
	}
}

static void
print_wlan ()
{
	wifiRadioInfo_t info;
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {

		boolean_t state;

		int rc = Utopia_GetWifiRadioState(&ctx, FREQ_2_4_GHZ, &state);
		if (SUCCESS == rc) {
			printf("<P> Utopia_GetWifiRadioState() \n %d\n", state);
		} else {
			printf("<P> Calling: Utopia_GetWifiRadioState() returned: %d\n", rc);
		}
		rc = Utopia_GetWifiRadioState(&ctx, FREQ_5_GHZ, &state);
		if (SUCCESS == rc) {
			printf("<P> Utopia_GetWifiRadioState() \n %d\n", state);
		} else {
			printf("<P> Calling: Utopia_GetWifiRadioState() returned: %d\n", rc);
		}


		rc = Utopia_GetWifiRadioSettings(&ctx, FREQ_2_4_GHZ, &info);
		if (SUCCESS == rc) {
			printf("<P> Utopia_GetWifiRadioSettings() \n %s\n %d\n", info.ssid, info.ssid_broadcast);
		} else {
			printf("<P> Calling: Utopia_GetWifiRadioSettings() returned: %d\n", rc);
		}
		Utopia_Free(&ctx, 0);
	} else {
		printf("Error initializing context\n");
		return;
	}
}

static void
add_iap (char *name)
{
	iap_entry_t iap[2];
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		bzero(&iap[0], sizeof(iap_entry_t));
		strcpy(iap[0].policyname, name);
		iap[0].enabled = 1;
		iap[0].allow_access = 1;
		iap[0].tod.day = 127;
		strcpy(iap[0].tod.start_time,"00:00");
		strcpy(iap[0].tod.stop_time,"23:59");

		iap[0].lanhosts_set = TRUE;
		iap[0].lanhosts.iplist = malloc(URL_SZ);
		strcpy(iap[0].lanhosts.iplist,"192.168.1.112");
		iap[0].lanhosts.ip_count = 1;

		int rc = Utopia_AddInternetAccessPolicy(&ctx, &iap[0]);
		if (SUCCESS == rc) {
			printf("<P> Utopia_AddInternetAccessPolicy() rc = %d\n", rc);
			Utopia_Free(&ctx, 1);
		} else {
			printf("<P> Error Utopia_AddInternetAccessPolicy() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
delete_iap (char *name)
{
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_DeleteInternetAccessPolicy(&ctx, name);
		if (SUCCESS == rc) {
			printf("<P> Utopia_AddInternetAccessPolicy() rc = %d\n", rc);
			Utopia_Free(&ctx, 1);
		} else {
			printf("<P> Error Utopia_AddInternetAccessPolicy() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
print_iap (char *name)
{
	iap_entry_t iap[2];
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_FindInternetAccessPolicy(&ctx, name, &iap[0], NULL);
		if (UT_SUCCESS == rc) {
			printf("<P> Utopia_FindInternetAccessPolicy() rc = %d\n", rc);
			printf("<P>   name %s, enable %d, allow %d\n"
				   "<P>   tod.day %d, tod start %s, tod stop %s\n"
				   "<P>   lan-ip count %d, lan-ip %s\n",
				   iap[0].policyname, iap[0].enabled, iap[0].allow_access,
				   iap[0].tod.day, iap[0].tod.start_time, iap[0].tod.stop_time, 
				   iap[0].lanhosts.ip_count, iap[0].lanhosts.iplist);
			Utopia_Free(&ctx, 1);
		} else {
			printf("<P> Error Utopia_FindInternetAccessPolicy() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
add_sroute (char *name)
{
	routeStatic_t sroute[2];
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		bzero(&sroute[0], sizeof(routeStatic_t));
		strcpy(sroute[0].name, name);
		strcpy(sroute[0].dest_lan_ip, "192.168.22.0");
		strcpy(sroute[0].netmask, "255.255.255.0");
		strcpy(sroute[0].gateway, "192.168.1.58");
		sroute[0].dest_intf = INTERFACE_LAN;

		int rc = Utopia_AddStaticRoute(&ctx, &sroute[0]);
		if (SUCCESS == rc) {
			printf("<P> Utopia_AddStaticRoute() rc = %d\n", rc);
			Utopia_Free(&ctx, 1);
		} else {
			printf("<P> Error Utopia_AddStaticRoute() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
delete_sroute (char *name)
{
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_DeleteStaticRouteName(&ctx, name);
		if (SUCCESS == rc) {
			printf("<P> Utopia_DeleteStaticRouteName() rc = %d\n", rc);
			Utopia_Free(&ctx, 1);
		} else {
			printf("<P> Error Utopia_DeleteStaticRouteName() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
print_sroutes ()
{
	routeStatic_t *sroute;
	int count;
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_GetStaticRoutes(&ctx, &count, &sroute);
		if (UT_SUCCESS == rc) {
			printf("<P> Utopia_GetStaticRoutes() rc = %d\n", rc);
			int i;
			for (i = 0; i < count; i++) {
				printf("<P>   StaticRoute[%d]\n", i+1);
				printf("<P>   name %s, ip %s, mask %s, gw %s, intf %d\n",
					   sroute[i].name,
					   sroute[i].dest_lan_ip,
					   sroute[i].netmask,
					   sroute[i].gateway,
					   sroute[i].dest_intf);
			}
			Utopia_Free(&ctx, 0);
		} else {
			printf("<P> Error Utopia_GetStaticRoute() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
print_arp_cache ()
{
	arpHost_t *hosts;
	int count;
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_GetARPCacheEntries(&ctx, &count, &hosts);
		if (UT_SUCCESS == rc) {
			printf("<P> Utopia_GetARPCacheEntries() rc = %d\n", rc);
			int i;
			for (i = 0; i < count; i++) {
				printf("<P>   ARP Host [%d]\n", i+1);
				printf("<P>   ip %s, mac %s, intf %d\n",
					   hosts[i].ipaddr,
					   hosts[i].macaddr,
					   hosts[i].interface);
			}
			Utopia_Free(&ctx, 0);
		} else {
			printf("<P> Error Utopia_GetARPCacheEntries() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}

static void
print_wlan_clientlist ()
{
	char *maclist;
	int count;
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_GetWLANClients(&ctx, &count, &maclist);
		if (UT_SUCCESS == rc && maclist) {
			printf("<P> Utopia_GetWLANClients() rc = %d, count=%d\n", rc, count);
			int i;
			for (i = 0; i < count; i++) {
				printf("<P>   WLAN Client [%d] - %s\n", i, maclist + (MACADDR_SZ * i));
			}
			Utopia_Free(&ctx, 0);
		} else {
			printf("<P> Error Utopia_GetWLANClients() rc = %d\n", rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("%s: Error initializing context\n", __FUNCTION__);
	}
}
#endif

//Wifi Additions
static void print_Wifi_RadioState()
{
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {

		boolean_t state;

		int rc = Utopia_GetWifiRadioState(&ctx, FREQ_2_4_GHZ, &state);       
		if (SUCCESS == rc) {
			printf("RadioState for 2_4_GHZ FREQ BAND: %d\n", state);
		} else {
			printf("RadioState for 2.4_GHZ Failed:%d\n",rc);
		}
		rc = Utopia_GetWifiRadioState(&ctx, FREQ_5_GHZ, &state);
		if (SUCCESS == rc) {
			printf("RadioState for 5GHZ FREQ BAND: %d\n", state);
		} else {
			printf("RadioState 5GHZ Failed: %d\n",rc);
		}
		Utopia_Free(&ctx, 0);
	} else {
		printf("Error initializing context\n");    
		return;
	}
}
static void set_Wifi_RadioState()
{
	UtopiaContext ctx;
	wifiInterface_t freq;
	boolean_t state;
	//Input the values

	printf("\nFREQ BAND [0-2.4GHZ 1-5GHZ]:");
	scanf("%d",&freq);
	printf("\nEnable/Disable[0-Disable 1-Enable]:"); 
	scanf("%d",&state); 

	//process using UTAPI
	if (Utopia_Init(&ctx)) {
		int rc = Utopia_SetWifiRadioState(&ctx, freq, state);
		if (SUCCESS == rc) {
			Utopia_Free(&ctx,1);
			printf("\n FREQ BAND %s set to state: %d \n",(freq==0)?"2.4GHZ":"5GHZ\n", state);
		} else {
			printf("SetWifiRadioState Failed: %d\n", rc);
			Utopia_Free(&ctx, 0);
		}

	} else {
		printf("Error initializing context\n");
		return;
	}
	return;
}

static void print_Wifi_RadioSettings()    
{
	wifiRadioInfo_t info;
	UtopiaContext ctx;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_GetWifiRadioSettings(&ctx, FREQ_2_4_GHZ, &info);
		if (SUCCESS == rc) {
			printf("Wifi Radio Settings For 2.4GHZ: \n\t SSID: %s \n\t MACADDR: %s \n\t SSID_BROADCAST: %d \n\t Band[-1-Invalid 0-Auto 20-1 40-2]: %d \n\t Channel : %d \n\t Mode: %d \n\t Sideband: %d\n", info.ssid, info.mac_address,info.ssid_broadcast,info.band, info.channel,info.mode,info.sideband);
		} else {
			printf("Get Wifi Radio Settings 2.4GHZ Failed:%d\n",rc);
		}

		rc = Utopia_GetWifiRadioSettings(&ctx, FREQ_5_GHZ, &info);
		if (SUCCESS == rc) {
			printf("Wifi Radio Settings For 5 GHZ: \n\t SSID: %s \n\t MACADDR: %s \n\t SSID_BROADCAST: %d \n\t Band[-1-Invalid 0-Auto 20-1 40-2]: %d \n\t Channel : %d \n\t Mode: %d \n\t Sideband: %d\n", info.ssid, info.mac_address,info.ssid_broadcast,info.band, info.channel,info.mode,info.sideband);
		} else {
			printf("Get Wifi Radio Settings 5GHZ Failed: %d\n",rc);
		}
		Utopia_Free(&ctx, 0);
	} else {
		printf("Error initializing context\n");
		return;
	}
	return;
}

static void  set_Wifi_RadioSettings()
{ 
	UtopiaContext ctx;
	wifiRadioInfo_t info[0];
	char tempstr[SSID_SIZE];
	int temp;
	int rc=0;
	//Input the values
	//bzero(&info[0], sizeof(wifiRadioInfo_t));
	printf("\nFREQ BAND  0-2.4GHZ-0 1-5GHZ: ");
	scanf("%d",&temp);
	info[0].interface =temp;
	printf("\nEnter SSID(string):"); 
	scanf("%s",tempstr); 
	strcpy(info[0].ssid,tempstr);
	printf("\nEnter SSID_Broadcast[0-disable 1-enable]:"); 
	scanf("%d",&temp);
	info[0].ssid_broadcast = temp; 
	printf("\nEnter Mode[-1-Invalid 1-B 2-G 3-A 4-N 5-BG 6-BGN 7-AN]:"); 
	scanf("%d",&temp);
	info[0].mode = temp; 
	printf("\nEnter Band[-1-Invalid 0-Auto 1-20 2-40]:"); 
	scanf("%d",&temp);
	info[0].band = temp; 
	printf("\nEnter Channel[number]:"); 
	scanf("%d",&temp);
	info[0].channel = temp; 
	printf("\nEnter sideband[0-lower 1-upper]:"); 
	scanf("%d",&temp);
	info[0].sideband = temp; 

	if (Utopia_Init(&ctx)) {
		//process using UTAPI
		rc = Utopia_SetWifiRadioSettings(&ctx,&info[0]);
		if (SUCCESS == rc) {
			Utopia_Free(&ctx, 1);
			printf("\n WifiRadio Settings set to: SSID:%s SSID_BROADCAST:%d Band:%d Channel:%d Mode:%d Sideband:%d\n", info[0].ssid, info[0].ssid_broadcast,
				   info[0].band, info[0].channel,info[0].mode,info[0].sideband);
		} else {
			printf("Set WifiRadioSettings failed:%d \n",rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("Error initializing context\n");
		return;
	}
	return;
}

static void print_Wifi_ConfigMode()
{
	UtopiaContext ctx;
	wifiConfigMode_t config_mode ;

	if (Utopia_Init(&ctx)) {
		int rc = Utopia_GetWifiConfigMode (&ctx, &config_mode);
		if (SUCCESS == rc) {
			printf("Wifi Config Mode[0-Manual 1-WPS]:%d\n",config_mode);
		} else {
			printf("GetWifi Config Mode failed:  %d\n",rc);
		}

		Utopia_Free(&ctx, 0);
	} else {
		printf("Error initializing context\n");
		return;
	}
	return;
}
static void set_Wifi_ConfigMode()
{
	UtopiaContext ctx;
	wifiConfigMode_t config_mode ;

	if (Utopia_Init(&ctx)) {
		printf("\nEnter Configmode[0-Manual 1-WPS]:"); 
		scanf("%d",&config_mode);
		int rc = Utopia_SetWifiConfigMode (&ctx, config_mode);
		if (SUCCESS == rc) {
			Utopia_Free(&ctx,1);
			printf("Wifi Config Mode[0-Manual 1-WPS]:%d\n",config_mode);
		} else {
			printf("Wifi ConfigMode Failed: %d \n",rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("Error initializing context\n");
		return;
	}
}

static void print_WifiSecuritySettings()
{
	UtopiaContext ctx;
	wifiSecurityInfo_t info ;

	if (Utopia_Init(&ctx)) {
		int rc= Utopia_GetWifiSecuritySettings (&ctx,FREQ_2_4_GHZ, &info);
		if (SUCCESS == rc) {

			printf("\nSecurity Settings for 2.4GHZ:");
			printf("\nMode [-1-Invalid 0-Disabled 1-WEP 2-WPA Personal 3-WPA Enterprise 4-WPA2 Personal 5-WPA2 Enterprise 6-RADIUS]:%d",info.mode); 
			printf("\n\tPassphrase:%s",info.passphrase); 
			printf("\n\tWEP TX Key :%d",info.wep_txkey); 
			printf("\n\tWEP Key1: %s",info.wep_key[0]); 
			printf("\n\tWEP Key2: %s",info.wep_key[1]); 
			printf("\n\tWEP Key3: %s",info.wep_key[2]); 
			printf("\n\tWEP Key4: %s",info.wep_key[3]); 
			printf("\n\tWPAEncryption[-1-Invalid 0-AES 1-TKIP 2-TKIP+AES]:%d",info.encrypt); 
			printf("\n\tWPA Shared Key: %s",info.shared_key); 
			printf("\n\tWPA Key Renewal Interval: %d",info.key_renewal_interval); 
			printf("\n\tWPA Radius Server IP: %s",info.radius_server); 
			printf("\n\tWPA Radius Server Port: %d\n",info.radius_port); 


		} else {
			printf("\nGetSecuritySettings2.4GHZ Failed: %d\n",rc);
		}
		rc= Utopia_GetWifiSecuritySettings (&ctx,FREQ_5_GHZ, &info);
		if (SUCCESS == rc) {
			printf("\nSecurity Settings for 5 GHZ:");
			printf("\nMode [-1-Invalid 0-Disabled 1-WEP 2-WPA Personal 3-WPA Enterprise 4-WPA2 Personal 5-WPA2 Enterprise 6-RADIUS]:%d",info.mode); 
			printf("\n\tPassphrase:%s",info.passphrase); 
			printf("\n\tWEP TX Key :%d",info.wep_txkey); 
			printf("\n\tWEP Key1: %s",info.wep_key[0]); 
			printf("\n\tWEP Key2: %s",info.wep_key[1]); 
			printf("\n\tWEP Key3: %s",info.wep_key[2]); 
			printf("\n\tWEP Key4: %s",info.wep_key[3]); 
			printf("\n\tWPAEncryption[-1-Invalid 0-AES 1-TKIP 2-TKIP+AES]:%d",info.encrypt); 
			printf("\n\tWPA Shared Key: %s",info.shared_key); 
			printf("\n\tWPA Key Renewal Interval: %d",info.key_renewal_interval); 
			printf("\n\tWPA Radius Server IP: %s",info.radius_server); 
			printf("\n\tWPA Radius Server Port: %d\n",info.radius_port); 
		} else {
			printf("\nGetSecuritySettings 5GHZ Failed: %d\n",rc);
		}

		Utopia_Free(&ctx, 0);
	}
}
static void set_Wifi_SecuritySettings()
{
	UtopiaContext ctx;
	wifiSecurityInfo_t info[0];
	char tempstr[PASSPHRASE_SZ];
	int temp;
	//Input the values
	printf("\nInterface [0-2.4GHZ 1-5GHZ]:");
	scanf("%d",&temp);
	info[0].interface =temp;
	printf("\nMode [-1-Invalid 0-Disabled 1-WEP 2-WPA Personal 3-WPA Enterprise 4-WPA2 Personal 5-WPA2 Enterprise 6-RADIUS]:"); 
	scanf("%d",&temp); 
	info[0].mode =temp;
	if (info[0].mode != WIFI_SECURITY_DISABLED && info[0].mode != WIFI_SECURITY_INVALID) {

		if (info[0].mode == WIFI_SECURITY_WEP) {
			//missing WEP encryption length 64 or 128 bit
			printf("\nPassPhrase(string):"); 
			scanf("%s",tempstr); 
			strcpy(info[0].passphrase,tempstr);
			printf("\n wep_Tx Key [1-4]:"); 
			scanf("%d",&temp); 
			info[0].wep_txkey =temp;
			printf("\n wep_key-1(string):"); 
			scanf("%s",tempstr); 
			strcpy(info[0].wep_key[0],tempstr);
			printf("\n wep_key-2(string):"); 
			scanf("%s",tempstr); 
			strcpy(info[0].wep_key[1],tempstr);
			printf("\n wep_key-3(string):"); 
			scanf("%s",tempstr); 
			strcpy(info[0].wep_key[2],tempstr);
			printf("\n wep_key-4(string):"); 
			scanf("%s",tempstr); 
			strcpy(info[0].wep_key[3],tempstr);
		}

		printf("\nEnter Encryption[-1-Invalid 0-AES 1-TKIP 2-TKIP+AES]:"); 
		scanf("%d",&temp);
		info[0].encrypt = temp; 
		printf("\nEnter shared_key(string):"); 
		scanf("%s",tempstr); 
		strcpy(info[0].shared_key,tempstr);
		printf("\nEnter Key Renewal inteval[in secs]:"); 
		scanf("%d",&temp);
		info[0].key_renewal_interval = temp;


		if (info[0].mode == WIFI_SECURITY_RADIUS) {
			printf("\nEnter Radius Server IPADDR(XX:YY:ZZ:AA):"); 
			scanf("%s",tempstr); 
			strcpy(info[0].radius_server,tempstr);
			printf("\nEnter Radius Server Port:"); 
			scanf("%d",&temp); 
			info[0].radius_port =temp;
		}

	}

//process using UTAPI
	if (Utopia_Init(&ctx)) {
		int rc = Utopia_SetWifiSecuritySettings(&ctx,&info[0]);
		if (SUCCESS == rc) {
			Utopia_Free(&ctx,1);
			printf("\nSecuritySettings Set to:");
			printf("\nSecurity Settings for interface %s:",(info[0].interface==0)?"2.4GHZ":"5GHZ");
			printf("\nMode [-1-Invalid 0-Disabled 1-WEP 2-WPA Personal 3-WPA Enterprise 4-WPA2 Personal 5-WPA2 Enterprise 6-RADIUS]:%d",info[0].mode); 
			printf("\n\tPassphrase:%s",info[0].passphrase); 
			printf("\n\tWEP TX Key :%d",info[0].wep_txkey); 
			printf("\n\tWEP Key1: %s",info[0].wep_key[0]); 
			printf("\n\tWEP Key2: %s",info[0].wep_key[1]); 
			printf("\n\tWEP Key3: %s",info[0].wep_key[2]); 
			printf("\n\tWEP Key4: %s",info[0].wep_key[3]); 
			printf("\n\tWPAEncryption[-1-Invalid 0-AES 1-TKIP 2-TKIP+AES]:%d",info[0].encrypt); 
			printf("\n\tWPA Shared Key: %s",info[0].shared_key); 
			printf("\n\tWPA Key Renewal Interval: %d",info[0].key_renewal_interval); 
			printf("\n\tWPA Radius Server IP: %s",info[0].radius_server); 
			printf("\n\tWPA Radius Server Port: %d\n",info[0].radius_port); 
		} else {
			printf("\nSetWifiRadioSettings Failed: %d \n",rc);
			Utopia_Free(&ctx, 0);
		}
	} else {
		printf("Error initializing context\n");
		return;
	}


}

/*static void print_Wifi_StatusInfo(){
}
static void set_Wifi_StatusInfo(){
} */

static void usage(){
	printf("Usage: \n"
		   "\tutapitest get radiostate \n"
		   "\tutapitest set radiostate\n "
		   "\tutapitest get radiosettings\n"
		   "\tutapitest set radiosettings\n"
		   "\tutapitest get configmode\n"
		   "\tutapitest set configmode\n"
		   "\tutapitest get securitysettings\n"
		   "\tutapitest set securitysettings\n"
		   //  "\tutapitest get statusinfo\n"
		   //"\tutapitest set statusinfo\n"
		  );
}
int main(int argc, char** argv)
{
	boolean_t iFail;

	if (argc <= 2) {
		usage();
		iFail = 1;
	} else if (strcmp(argv[1],"get") == 0) {
		if (strcmp(argv[2],"radiostate")==0)
			print_Wifi_RadioState();
		else if (strcmp(argv[2],"radiosettings")==0)
			print_Wifi_RadioSettings();
		else if (strcmp(argv[2],"configmode")==0)
			print_Wifi_ConfigMode();
		else if (strcmp(argv[2],"securitysettings")==0)
			print_WifiSecuritySettings();
		//else if (strcmp(argv[2],"statusinfo")==0)
		//print_Wifi_StatusInfo();
		else
			usage();
	} else if (strcmp(argv[1],"set") == 0) {

		if (strcmp(argv[2],"radiostate")==0)
			set_Wifi_RadioState();
		else if (strcmp(argv[2],"radiosettings")==0)
			set_Wifi_RadioSettings();
		else if (strcmp(argv[2],"configmode")==0)
			set_Wifi_ConfigMode();
		else if (strcmp(argv[2],"securitysettings")==0)
			set_Wifi_SecuritySettings();
		//else if (strcmp(argv[2],"statusinfo")==0)
		//	set_Wifi_StatusInfo();
		else
			usage();
	}

	//  print_basic_setup();
	//set_basic_setup();
	//  print_basic_setup();

	// print_wan_settings();
	//  set_wan_settings();
	//  print_wan_settings();

	/*print_port_mapping();
	set_port_mapping();
	print_port_mapping();
	add_port_mapping();
	print_port_/mapping();

	print_log_settings();
	set_log_settings();
	print_log_settings();

	set_dhcp_static_hosts(); */

	//  print_dhcp_client_list();
	/*print_wlan();   */

	/* add_iap("test-iap-policy-0");
	 print_iap("test-iap-policy-0");
	 delete_iap("test-iap-policy-0");
 
	 add_sroute("test-sr-1");
	 print_sroutes();
	 delete_sroute("test-sr-1");
 
	 print_arp_cache();  */

	/* Wifi tests*/


//    print_wlan_clientlist(); */

	return 0;
}
