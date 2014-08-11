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

#
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -pedantic-errors
CFLAGS += -Wall -Wextra -Werror -Wno-long-long -Wmissing-prototypes
CFLAGS += -fPIC -fvisibility=hidden
CFLAGS += -DHDK_CLI_BUILD -I$(HDK2_ROOT_DIR)/include
ifdef HDK_XML_LOGGING
CFLAGS += -DHDK_LOGGING=1
endif

# Link Flags  
LDFLAGS += -lhdkxml -lcurl -shared-libgcc

targets_libhdkcli := libhdkcli.a libhdkcli.so
targets_libhdkcli := $(addprefix $(ComponentBuildDir)/,$(targets_libhdkcli))

source_files_libhdkcli := hdk_cli.c hdk_cli_log.c hdk_cli_http_curl.c
obj_files_libhdkcli := $(addprefix $(ComponentBuildDir)/,$(source_files_libhdkcli:%.c=%.o))

$(targets_libhdkcli): $(obj_files_libhdkcli)

all: $(targets_libhdkcli) install

clean: 
	rm -f $(ComponentBuildDir)

install: install_fs stage

install_fs:
	@echo "Copying...[$(targets_libhdkcli) to $(CCSP_INSTALL_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(targets_libhdkcli) $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[$(targets_libhdkcli) to $(CCSP_STAGING_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(targets_libhdkcli) $(CCSP_STAGING_ROOT)/lib/

.PHONY : all clean install install_fs stage

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
