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

#Makefile for utopia syscfg command line code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
LDFLAGS += -lsyscfg
LDFLAGS += -lulog

targets_syscfg := syscfg
targets_syscfg := $(addprefix $(ComponentBuildDir)/,$(targets_syscfg))

source_files_syscfg := syscfgcmd.c
obj_files_syscfg := $(addprefix $(ComponentBuildDir)/,$(source_files_syscfg:%.c=%.o))

$(targets_syscfg): $(obj_files_syscfg)


all: $(targets_syscfg) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_syscfg) to SBIN directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/sbin
	@install -m 0755 $(targets_syscfg) $(CCSP_INSTALL_ROOT)/sbin/
	@ln -sf syscfg $(CCSP_INSTALL_ROOT)/sbin/syscfg_create
	@ln -sf syscfg $(CCSP_INSTALL_ROOT)/sbin/syscfg_destroy
	@ln -sf syscfg $(CCSP_INSTALL_ROOT)/sbin/syscfg_format
	@ln -sf syscfg $(CCSP_INSTALL_ROOT)/sbin/syscfg_check

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
