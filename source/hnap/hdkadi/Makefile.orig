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

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -Wmissing-prototypes -Wall -Wextra -Wno-long-long
CFLAGS += -pedantic-errors -fPIC -fvisibility=hidden
CFLAGS += -Os -DHDK_CGI_FCGI -DHDK_ADI_BUILD
ifdef HDK_LOGGING
CFLAGS += -DHDK_LOGGING=1
endif

# Linker flags
LDFLAGS += -shared -shared-libgcc
LDFLAGS += -lulog
LDFLAGS += -lsyscfg -lsysevent -lutctx -lhnap12 -lhotspot -lfcgi

targets_libhdkadi := libhdkadi.a libhdkadi.so
targets_libhdkadi := $(addprefix $(ComponentBuildDir)/,$(targets_libhdkadi))

source_files_libhdkadi := $(call add_files_from_base,,'*.c')
obj_files_libhdkadi := $(addprefix $(ComponentBuildDir)/,$(source_files_libhdkadi:%.c=%.o))

$(targets_libhdkadi): $(obj_files_libhdkadi)

all: $(targets_libhdkadi) install

clean: 
	rm -f $(ComponentBuildDir)

install: install_fs stage

install_fs:
	@echo "Copying...[Libraries to Install Directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[Libraries to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib/
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_STAGING_ROOT)/lib/
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include
	@install -m 0644 hdk_device.h $(CCSP_STAGING_ROOT)/include/

.PHONY : all clean install install_fs stage

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
