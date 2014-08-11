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

include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS +=  -Os -fno-exceptions
CFLAGS += -DIPV6=1
CFLAGS += -Wall -Wextra 
CFLAGS += -Werror 

LDFLAGS += -lulog

targets_syseventd_fork_helper := syseventd_fork_helper
targets_syseventd_fork_helper := $(addprefix $(ComponentBuildDir)/,$(targets_syseventd_fork_helper))

source_files_syseventd_fork_helper := fork_helper.c
obj_files_syseventd_fork_helper := $(addprefix $(ComponentBuildDir)/,$(source_files_syseventd_fork_helper:%.c=%.o))

$(targets_syseventd_fork_helper): $(obj_files_syseventd_fork_helper)

all: $(targets_syseventd_fork_helper) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_syseventd_fork_helper) to SBIN directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/sbin
	@install -m 0755 $(targets_syseventd_fork_helper) $(CCSP_INSTALL_ROOT)/sbin/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

