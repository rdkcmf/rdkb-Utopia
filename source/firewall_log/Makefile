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

#Makefile for utopia firewall log code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -fno-exceptions 
CFLAGS += -Os 
CFLAGS += -Wall -Wextra

LDFLAGS += -lsyscfg
LDFLAGS += -lulog

targets_firewall_log := GenFWLog
targets_firewall_log := $(addprefix $(ComponentBuildDir)/,$(targets_firewall_log))

source_files_firewall_log := GenFWLog.c
obj_files_firewall_log := $(addprefix $(ComponentBuildDir)/,$(source_files_firewall_log:%.c=%.o))

$(targets_firewall_log): $(obj_files_firewall_log)

all: $(targets_firewall_log) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_firewall_log) to usr/sbin directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/usr/sbin
	@install -m 0755 $(targets_firewall_log) $(CCSP_INSTALL_ROOT)/usr/sbin/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
