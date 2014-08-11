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

#Makefile for utopia sysevent daemon code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -Os -fno-exceptions -I../lib 
CFLAGS += -Wall -Wextra 
CFLAGS += -DIPV6=1

# SET_REPLY_REQUIRED forces sysevent_set to return a message to confirm success
# if this is not defined then the system behavior changes in that a client
# could set a sysevent tuple and then read it, and it may not have been set yet
# ie. the return does not block on the set
# NOTE: keep in sync with lib/Makefile
CFLAGS += -DSET_REPLY_REQUIRED

# USE_SYSCFG enables the code that can lookup info in syscfg
CFLAGS += -DUSE_SYSCFG

# define SE_SERVER_CODE_DEBUG if you want to compile in debug statement
# Compiling in debug code adds about 20% to image size
CFLAGS += -DSE_SERVER_CODE_DEBUG

# define REDIRECT_CODE_DEBUG if you want to get debug messages redirected to /tmp
CFLAGS += -DREDIRECT_CODE_DEBUG

ifdef REDIRECT_CHILD_OUTPUT_TO_CONSOLE
CFLAGS += -DREDIRECT_CHILD_OUTPUT_TO_CONSOLE
endif

LDFLAGS += -lpthread -lsysevent 
LDFLAGS +=  -lsyscfg -lulog

targets_sysevent_server := syseventd
targets_sysevent_server := $(addprefix $(ComponentBuildDir)/,$(targets_sysevent_server))

source_files_sysevent_server :=	syseventd_main.c worker_threads.c triggerMgr.c
source_files_sysevent_server +=	clientsMgr.c dataMgr.c se_debug.c utils.c 
obj_files_sysevent_server := $(addprefix $(ComponentBuildDir)/,$(source_files_sysevent_server:%.c=%.o))

$(targets_sysevent_server): $(obj_files_sysevent_server)

all: $(targets_sysevent_server)

install:
	@echo "Copying...[$(targets_sysevent_server) to SBIN directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/sbin
	@install -m 0755 $(targets_sysevent_server) $(CCSP_INSTALL_ROOT)/sbin/

clean: 
	rm -f $(ComponentBuildDir)

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
