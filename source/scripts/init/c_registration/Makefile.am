##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
bin_PROGRAMS = 26_potd 15_switchpmon 10_fpm 02_forwarding 10_bootstrap_dns 33_cosa 03_wan_ipv6 20_gwreset 10_mcastproxy 15_dhcpv6_client 02_wlan 15_xhs 15_dhcp_server 20_factorydefault 01_init 10_sysevent_proxy 02_lanHandler 15_qos 04_byoi 15_dnsforwarder 02_managed 25_crond 10_ntpclient 10_ntpd 20_routing 02_multinet 04_byoi_gap 15_http_server 10_mldproxy 15_ddnsclient 02_bridge 15_ssh_server 20_ciscoconnect 02_lan 10_firewall 15_ftp_server 15_igd 15_hotspot 02_ipv4 02_ipv6 15_dhcpv6_server 10_mcastsnooper 02_wan 01_system 15_misc 15_ccsphs 09_xdns

bin_PROGRAMS += 02_parodus

AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_LDFLAGS = -lpthread -lz \
		$(top_builddir)/source/util/utils/libutopiautil.la \
                $(top_builddir)/source/services/lib/libsrvmgr.la \
                $(top_builddir)/source/sysevent/lib/libsysevent.la \
                $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                $(top_builddir)/source/ulog/libulog.la

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir)/source -I$(top_srcdir)/source/include -I$(top_srcdir)/source/util/utils -I$(top_srcdir)/source/services/lib

26_potd_SOURCES = 26_potd.c
15_switchpmon_SOURCES = 15_switchpmon.c
10_fpm_SOURCES = 10_fpm.c
02_forwarding_SOURCES = 02_forwarding.c
10_bootstrap_dns_SOURCES = 10_bootstrap_dns.c
33_cosa_SOURCES = 33_cosa.c
03_wan_ipv6_SOURCES = 03_wan_ipv6.c
20_gwreset_SOURCES = 20_gwreset.c
10_mcastproxy_SOURCES = 10_mcastproxy.c
15_dhcpv6_client_SOURCES = 15_dhcpv6_client.c
02_wlan_SOURCES = 02_wlan.c
15_xhs_SOURCES = 15_xhs.c
15_dhcp_server_SOURCES = 15_dhcp_server.c
20_factorydefault_SOURCES = 20_factorydefault.c
01_init_SOURCES = 01_init.c
10_sysevent_proxy_SOURCES = 10_sysevent_proxy.c
02_lanHandler_SOURCES = 02_lanHandler.c
15_qos_SOURCES = 15_qos.c
04_byoi_SOURCES = 04_byoi.c
15_dnsforwarder_SOURCES = 15_dnsforwarder.c
02_managed_SOURCES = 02_managed.c
25_crond_SOURCES = 25_crond.c
10_ntpclient_SOURCES = 10_ntpclient.c
10_ntpd_SOURCES = 10_ntpd.c
09_xdns_SOURCES = 09_xdns.c
20_routing_SOURCES = 20_routing.c
02_multinet_SOURCES = 02_multinet.c
04_byoi_gap_SOURCES = 04_byoi_gap.c
15_http_server_SOURCES = 15_http_server.c
10_mldproxy_SOURCES = 10_mldproxy.c
15_ddnsclient_SOURCES = 15_ddnsclient.c
02_bridge_SOURCES = 02_bridge.c
15_ssh_server_SOURCES = 15_ssh_server.c
20_ciscoconnect_SOURCES = 20_ciscoconnect.c
02_lan_SOURCES = 02_lan.c
10_firewall_SOURCES = 10_firewall.c
15_ftp_server_SOURCES = 15_ftp_server.c
15_igd_SOURCES = 15_igd.c
15_hotspot_SOURCES = 15_hotspot.c
02_ipv4_SOURCES = 02_ipv4.c
02_ipv6_SOURCES = 02_ipv6.c
15_dhcpv6_server_SOURCES = 15_dhcpv6_server.c
10_mcastsnooper_SOURCES = 10_mcastsnooper.c
02_wan_SOURCES = 02_wan.c
01_system_SOURCES = 01_system.c
15_misc_SOURCES = 15_misc.c
15_leasenotify_SOURCES =15_leasenotify.c
15_ccsphs_SOURCES = 15_ccsphs.c
02_parodus_SOURCES = 02_parodus.c
