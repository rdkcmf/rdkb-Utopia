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
AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lpthread
AM_LDFLAGS += -lz
AM_LDFLAGS += -lsrvmgr
AM_LDFLAGS += -lsyscfg
AM_LDFLAGS += -lsysevent
AM_LDFLAGS += -lulog

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

noinst_LTLIBRARIES = libUtopia_c_registration.la
libUtopia_c_registration_la_CPPFLAGS = -I$(top_srcdir)/source/cosa/include/linux -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/include -I$(top_srcdir)/source/hdk2/hdk2/include -I$(top_srcdir)/source/pal/upnp/include -I$(top_srcdir)/source/pal/xml/include -I$(top_srcdir)/source/pal/kernel/include -I$(top_srcdir)/source/pal/include -I$(top_srcdir)/source/pal/log/include $(CPPFLAGS)
libUtopia_c_registration_la_SOURCES = 26_potd.c 15_switchpmon.c 10_fpm.c 02_forwarding.c 10_bootstrap_dns.c 33_cosa.c 03_wan_ipv6.c 20_gwreset.c 10_mcastproxy.c 15_dhcpv6_client.c 02_wlan.c 15_xhs.c 15_dhcp_server.c 20_factorydefault.c 01_init.c 10_sysevent_proxy.c 02_lanHandler.c 15_qos.c 04_byoi.c 15_dnsforwarder.c 02_managed.c 25_crond.c 10_ntpclient.c 20_routing.c REGISTRATION_TEMPLATE.c.txt 02_multinet.c 04_byoi_gap.c 15_http_server.c 10_mldproxy.c 15_ddnsclient.c 02_bridge.c 15_ssh_server.c 20_ciscoconnect.c 02_lan.c 10_firewall.c 02_syslog.c 15_ftp_server.c 15_wecb.c 15_igd.c 15_hotspot.c 02_ipv4.c 15_dhcpv6_server.c 10_mcastsnooper.c 02_wan.c 01_system.c
libUtopia_c_registration_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz -lsrvmgr -lsyscfg -lsysevent -lulog
