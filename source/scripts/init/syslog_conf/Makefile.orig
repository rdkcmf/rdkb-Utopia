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

UT_TARGET_NAME ?= ga

install_at_usr_sbin := syslog_conf_tool.sh log_start.sh 
install_at_usr_sbin += $(call add_files_from_board,,log_handle.sh)

install_at_etc := syslog.conf_default

all: install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/etc
	@echo "Copying...[$(install_at_etc) to $(CCSP_INSTALL_ROOT)/etc directory]"
	@install -m 0644 $(install_at_etc) $(CCSP_INSTALL_ROOT)/etc
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/usr/sbin
	@echo "Copying...[$(install_at_usr_sbin) to $(CCSP_INSTALL_ROOT)/usr/sbin directory]"
	@install -m 0755 $(install_at_usr_sbin) $(CCSP_INSTALL_ROOT)/usr/sbin

.PHONY: all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

