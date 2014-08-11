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

#Makefile for dhcp_proxy code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))

targets_dhcpproxy := dhcp_proxy
targets_dhcpproxy := $(addprefix $(ComponentBuildDir)/,$(targets_dhcpproxy))

source_files_dhcpproxy := dhcp_msg.c dhcp_proxy.c dhcp_util.c packet_util.c
obj_files_dhcpproxy := $(addprefix $(ComponentBuildDir)/,$(source_files_dhcpproxy:%.c=%.o))

$(targets_dhcpproxy): $(obj_files_dhcpproxy)


all: $(targets_dhcpproxy) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_dhcpproxy) to usr/sbin directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/usr/sbin
	@install -m 0755  $(targets_dhcpproxy) $(CCSP_INSTALL_ROOT)/usr/sbin/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
