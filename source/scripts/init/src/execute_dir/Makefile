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

#Makefile for utopia folder execute code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -fno-exceptions
CFLAGS += -Os
CFLAGS += -Wall -Wextra

targets_execute_dir := execute_dir
targets_execute_dir := $(addprefix $(ComponentBuildDir)/,$(targets_execute_dir))

source_files_execute_dir := $(call add_files_from_base,,'*.c')
obj_files_execute_dir := $(addprefix $(ComponentBuildDir)/,$(source_files_execute_dir:%.c=%.o))

$(targets_execute_dir): $(obj_files_execute_dir)

all: $(targets_execute_dir) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_execute_dir) to SBIN directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/sbin
	@install -m 0755 $(targets_execute_dir) $(CCSP_INSTALL_ROOT)/sbin/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
