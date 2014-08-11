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

install_scripts_service_d := ulog_functions.sh
install_scripts_service_d += event_handler_functions.sh
install_scripts_service_d += service_registration_functions.sh
install_scripts_service_d += sysevent_functions.sh
install_scripts_service_d += event_flags
install_scripts_service_d += interface_functions.sh
install_scripts_service_d += hostname_functions.sh
install_scripts_service_d += set_resolv_conf.sh
install_scripts_service_d += set_hostname.sh
install_scripts_service_d += set_wandomain.sh
install_scripts_service_d += date_functions.sh
install_scripts_service_d += ipv6_functions.sh
install_scripts_service_d += pmon.sh
install_scripts_service_d += service_forwarding.sh
install_scripts_service_d += service_crond.sh
install_scripts_service_d += service_syslog.sh
install_scripts_service_d += service_lan.sh 
install_scripts_service_d += service_wan.sh
install_scripts_service_d += brcm_ethernet_helper.sh
install_scripts_service_d += service_firewall/firewall_log_handle.sh
install_scripts_service_d += service_dhcp_server.sh
install_scripts_service_d += service_ddns.sh
install_scripts_service_d += service_routed.sh
install_scripts_service_d += service_igd.sh
install_scripts_service_d += igd_cmd.sh
install_scripts_service_d += service_mcastproxy.sh
install_scripts_service_d += service_mldproxy.sh
install_scripts_service_d += $(call add_files_from_board,,hhs_validate.sh)
install_scripts_service_d += $(call add_files_from_board,,service_cosa.sh)
install_scripts_service_d += $(call add_files_from_board,,service_dhcpv6_client.sh)
install_scripts_service_d += $(call add_files_from_board,,service_bridge.sh)
install_scripts_service_d += lan_handler.sh
install_scripts_service_d += rt_tables
install_scripts_service_d += ebtable_rules.sh
ifeq ($(CONFIG_CISCO_XHS),y)
install_scripts_service_d += service_xhs.sh
endif
install_scripts_service_d += service_potd.sh
install_scripts_service_d += service_wecb.sh
ifeq ($(CONFIG_CISCO_FEATURE_CISCOCONNECT),y)
install_scripts_service_d += service_ciscoconnect.sh
endif
install_scripts_service_d += service_multinet.sh
install_scripts_service_d += service_ipv4.sh
install_scripts_service_d += ut_plat.sh

install_scripts_service_d_syslog := service_syslog/syslog_rotate_monitor.sh

install_scripts_service_d_lan := service_lan/lan_hooks.sh

install_scripts_service_d_wan := service_wan/dhcp_link.sh
install_scripts_service_d_wan += service_wan/dhcp_wan.sh
install_scripts_service_d_wan += service_wan/static_link.sh
install_scripts_service_d_wan += service_wan/static_wan.sh
install_scripts_service_d_wan += service_wan/ppp_helpers.sh

install_scripts_service_d_dhcp_server := service_dhcp_server/dhcp_server_functions.sh
install_scripts_service_d_dhcp_server += service_dhcp_server/dnsmasq_dhcp.script

install_scripts_service_d_ddns := service_ddns/ddns_success.sh

install_scripts_service_d_bridge := service_bridge/dhcp_link.sh

install_scripts_service_d_multinet := $(wildcard service_multinet/handle_*.sh)

target_install_dir := $(CCSP_INSTALL_ROOT)/etc/utopia/service.d

all: dirs install

clean: 
	rm -f $(ComponentBuildDir)

dirs:
	@install -d -m 0755 $(target_install_dir)
	@install -d -m 0755 $(target_install_dir)/service_syslog
	@install -d -m 0755 $(target_install_dir)/service_lan
	@install -d -m 0755 $(target_install_dir)/service_wan
	@install -d -m 0755 $(target_install_dir)/service_dhcp_server
	@install -d -m 0755 $(target_install_dir)/service_ddns
	@install -d -m 0755 $(target_install_dir)/service_bridge
	@install -d -m 0755 $(target_install_dir)/service_multinet

install: dirs
	@echo "Copying...[$(install_scripts_service_d) to $(target_install_dir) directory]"
	@install -m 0755 $(install_scripts_service_d) $(target_install_dir)
	@echo "Copying...[$(install_scripts_service_d_syslog) to $(target_install_dir)/service_syslog directory]"
	@install -m 0755 $(install_scripts_service_d_syslog) $(target_install_dir)/service_syslog
	@echo "Copying...[$(install_scripts_service_d_lan) to $(target_install_dir)/service_lan directory]"
	@install -m 0755 $(install_scripts_service_d_lan) $(target_install_dir)/service_lan
	@echo "Copying...[$(install_scripts_service_d_wan) to $(target_install_dir)/service_wan directory]"
	@install -m 0755 $(install_scripts_service_d_wan) $(target_install_dir)/service_wan
	@echo "Copying...[$(install_scripts_service_d_dhcp_server) to $(target_install_dir)/service_dhcp_server directory]"
	@install -m 0755 $(install_scripts_service_d_dhcp_server) $(target_install_dir)/service_dhcp_server
	@echo "Copying...[$(install_scripts_service_d_ddns) to $(target_install_dir)/service_ddns directory]"
	@install -m 0755 $(install_scripts_service_d_ddns) $(target_install_dir)/service_ddns
	@install -m 0755 service_sshd/service_dropbearssh.sh $(target_install_dir)/service_sshd.sh
	@echo "Copying...[$(install_scripts_service_d_bridge) to $(target_install_dir)/service_bridge directory]"
	@install -m 0755 $(install_scripts_service_d_bridge) $(target_install_dir)/service_bridge
	@echo "Copying...[$(install_scripts_service_d_multinet) to $(target_install_dir)/service_multinet directory]"
	@install -m 0755 $(install_scripts_service_d_multinet) $(target_install_dir)/service_multinet

.PHONY: all clean dirs install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

