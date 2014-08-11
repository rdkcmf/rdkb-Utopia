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

#Makefile for Utopia sysevent library code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -Os -fno-exceptions -fno-strict-aliasing
CFLAGS += -Wall -Wextra
CFLAGS += -g

# SET_REPLY_REQUIRED forces sysevent_set to return a message to confirm success
# if this is not defined then the system behavior changes in that a client
# could set a sysevent tuple and then read it, and it may not have been set yet
# ie. the return does not block on the set
# NOTE. keep in sync with server/Makefile
CFLAGS += -DSET_REPLY_REQUIRED

#if you want to disable ipv6 then set NO_IPV6
#CFLAGS += -DNO_IPV6
CFLAGS += -DIPV6=1

# enforce ISO C standard
#CFLAGS += -x c -pedantic-errors

# Since there are shell scripts which rely on sysevent_get to return
# the value of a variable via puts, it is important that
# libsysevent is silent UNLESS doing some debugging.
# Also the library will be a smaller
#CFLAGS += -DENABLE_LIBSYSEVENT_DEBUG

LDFLAGS += -lulog

targets_libsysevent := libsysevent.a libsysevent.so
targets_libsysevent := $(addprefix $(ComponentBuildDir)/,$(targets_libsysevent))

source_files_libsysevent := $(call add_files_from_base,,'*.c')
obj_files_libsysevent := $(addprefix $(ComponentBuildDir)/,$(source_files_libsysevent:%.c=%.o))

$(targets_libsysevent): $(obj_files_libsysevent)

all: $(targets_libsysevent) install
	 
clean: 
	rm -f $(ComponentBuildDir)

install: install_fs stage

install_fs:
	@echo "Copying...[Libraries to Install Directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_INSTALL_ROOT)/lib

stage:
	@echo "Copying...[Header files to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include/sysevent
	@install -m 0755 sysevent.h $(CCSP_STAGING_ROOT)/include/sysevent
	@install -m 0755 libsysevent_internal.h $(CCSP_STAGING_ROOT)/include/sysevent
	@echo "Copying...[Libraries to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_STAGING_ROOT)/lib

.PHONY : all clean install install_fs stage

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
