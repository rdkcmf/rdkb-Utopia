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

#Makefile for utopia firewall trigger code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -fno-exceptions 
CFLAGS += -Os 
CFLAGS += -Wall -Wextra

LDFLAGS += -lsyscfg -lulog -lsysevent
LDFLAGS += -lnetfilter_queue -lnfnetlink

targets_trigger := trigger
targets_trigger := $(addprefix $(ComponentBuildDir)/,$(targets_trigger))

source_files_trigger := trigger.c
obj_files_trigger := $(addprefix $(ComponentBuildDir)/,$(source_files_trigger:%.c=%.o))

$(targets_trigger): $(obj_files_trigger)

all: $(targets_trigger) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_trigger) to usr/sbin directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/usr/sbin
	@install -m 0755 $(targets_trigger) $(CCSP_INSTALL_ROOT)/usr/sbin/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

