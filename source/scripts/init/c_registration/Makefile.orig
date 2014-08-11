#######################################################################
#   Copyright [2014] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the \"License\");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an \"AS IS\" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#######################################################################

#Makefile for utopia compiled registration code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))

LDFLAGS += -lsysevent -lulog -lsrvmgr -lsyscfg

$(ComponentBuildDir)/01_init: 				01_init.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/01_system: 			01_system.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_lan: 				02_lan.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_bridge: 			02_bridge.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_forwarding: 		02_forwarding.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_ipv4: 				02_ipv4.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_lanHandler: 		02_lanHandler.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_managed: 			02_managed.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_multinet: 			02_multinet.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_syslog: 			02_syslog.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_wan: 				02_wan.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/02_wlan: 				02_wlan.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/03_wan_ipv6: 			03_wan_ipv6.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/04_byoi: 				04_byoi.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/04_byoi_gap: 			04_byoi_gap.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_bootstrap_dns: 		10_bootstrap_dns.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_firewall: 			10_firewall.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_fpm: 				10_fpm.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_mldproxy: 			10_mldproxy.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_mcastproxy: 		10_mcastproxy.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_mcastsnooper: 		10_mcastsnooper.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_ntpclient: 			10_ntpclient.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/10_sysevent_proxy:		10_sysevent_proxy.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_dhcpv6_client: 		15_dhcpv6_client.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_dhcpv6_server: 		15_dhcpv6_server.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_dnsforwarder: 		15_dnsforwarder.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_ddnsclient: 		15_ddnsclient.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_dhcp_server: 		15_dhcp_server.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_http_server: 		15_http_server.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_igd: 				15_igd.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_qos: 				15_qos.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_ftp_server:			15_ftp_server.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_hotspot:			15_hotspot.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_wecb: 				15_wecb.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_xhs:				15_xhs.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_ssh_server:			15_ssh_server.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/15_switchpmon:			15_switchpmon.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/20_ciscoconnect:		20_ciscoconnect.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/20_factorydefault:		20_factorydefault.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/20_gwreset:			20_gwreset.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/20_routing: 			20_routing.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/25_crond:				25_crond.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/26_potd:				26_potd.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

$(ComponentBuildDir)/33_cosa:				33_cosa.c
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ 2> $(@:%=%.warn) || (cat $(@:%=%.warn); exit 1)

source_files_c_registration := $(call add_files_from_base,,'*.c')
targets_c_registration := $(addprefix $(ComponentBuildDir)/,$(source_files_c_registration:%.c=%))

install_targets := 02_forwarding 25_crond 02_lanHandler 02_wan 10_firewall 15_dhcp_server 15_ddnsclient
install_targets += 15_ssh_server 20_routing 15_igd 10_mcastproxy 10_mldproxy 33_cosa 26_potd 02_bridge
install_targets += 02_ipv4 02_multinet 15_wecb
ifeq ($(CONFIG_CISCO_FEATURE_CISCOCONNECT),y)
install_targets += 20_ciscoconnect
endif
ifeq ($(CONFIG_CISCO_XHS),y)
install_targets += 15_xhs
endif
ifeq ($(CONFIG_CISCO_HOTSPOT),y)
install_targets += 15_hotspot
endif

install_targets := $(addprefix $(ComponentBuildDir)/,$(install_targets))

target_install_dir := $(CCSP_INSTALL_ROOT)/etc/utopia/registration.d


#all: $(targets_c_registration) install
all: dirs $(install_targets) install

#$(ComponentBuildDir)/%: $(ComponentBuildDir)/%.o
#$(ComponentBuildDir)/./%: $(ComponentBuildDir)/./%.o

dirs:
	@mkdir -p $(ComponentBuildDir)

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(install_targets) to $(target_install_dir) directory]"
	@install -d -m 0755 $(target_install_dir)
	@install -m 0755 $(install_targets) $(target_install_dir)

.PHONY : all clean dirs install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

