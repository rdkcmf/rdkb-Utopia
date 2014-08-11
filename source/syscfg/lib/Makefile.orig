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

#Makefile for Utopia syscfg library code

include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += -Wall -D_GNU_SOURCE -DSC_SYSV_SEM $(addprefix -I,$(INCPATH))
LDFLAGS += -lulog

targets_libsyscfg := libsyscfg.a libsyscfg.so
targets_libsyscfg := $(addprefix $(ComponentBuildDir)/,$(targets_libsyscfg))

source_files_libsyscfg := $(call add_files_from_base,,'*.c')
obj_files_libsyscfg := $(addprefix $(ComponentBuildDir)/,$(source_files_libsyscfg:%.c=%.o))

$(targets_libsyscfg): $(obj_files_libsyscfg)

all: $(targets_libsyscfg) install

clean: 
	rm -f $(ComponentBuildDir)

install: install_fs stage

install_fs:
	@echo "Copying...[Libraries to Install Directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[Header files to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include/syscfg
	@install -m 0755 syscfg.h $(CCSP_STAGING_ROOT)/include/syscfg/
	@echo "Copying...[Libraries to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_STAGING_ROOT)/lib/

.PHONY : all clean install install_fs stage

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
