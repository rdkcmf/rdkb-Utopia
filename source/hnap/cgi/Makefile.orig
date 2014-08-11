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
#Makefile for utopia syscfg command line code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -Wmissing-prototypes -Wall -Wextra -Werror -Wno-long-long
CFLAGS += -pedantic-errors
ifdef HDK_LOGGING
CFLAGS += -DHDK_LOGGING=1
endif

LDFLAGS += -lcrypt -lexpat -lpthread -ldl -shared-libgcc -lfcgi -lm
LDFLAGS += -lsyscfg -lsysevent -lutctx -lulog
LDFLAGS += -lhdkxml -lhdksrv -lhnap12 -lhotspot -lhdkadi

targets_hnap_cgi := index.fcgi
targets_hnap_cgi := $(addprefix $(ComponentBuildDir)/,$(targets_hnap_cgi))

source_files_hnap_cgi := $(call add_files_from_base,,'*.c')
obj_files_hnap_cgi := $(addprefix $(ComponentBuildDir)/,$(source_files_hnap_cgi:%.c=%.o))

$(targets_hnap_cgi): $(obj_files_hnap_cgi)

all: $(targets_hnap_cgi) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_hnap_cgi) to SBIN directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/www/HNAPI
	@install -m 0755 $(targets_hnap_cgi) $(CCSP_INSTALL_ROOT)/www/HNAPI/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
