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
CFLAGS += -ansi -Wall -Wextra -Werror -Wno-long-long -Wmissing-prototypes
CFLAGS += -fPIC -fvisibility=hidden
CFLAGS += -DHNAP12_BUILD -I$(HDK2_ROOT_DIR)/include
ifdef HDK_XML_LIBXML2
CFLAGS += -DHDK_XML_LIBXML2
endif
ifdef HDK_XML_LOGGING
CFLAGS += -DHDK_LOGGING=1
endif

# Link Flags
LDFLAGS += -lhdkxml -lhdksrv -shared-libgcc

HSL := $(HDK2_ROOT_DIR)/spec/configbackup.hsl
HSL += $(HDK2_ROOT_DIR)/spec/core.hsl
HSL += $(HDK2_ROOT_DIR)/spec/router.hsl
HSL += $(HDK2_ROOT_DIR)/spec/transaction.hsl
HSL += $(HDK2_ROOT_DIR)/spec/wificlient.hsl
HSL += state/configbackup_state.hsl
HSL += state/core_state.hsl
HSL += state/router_state.hsl
HSL += state/wificlient_state.hsl

targets_libhnap12 := libhnap12.a libhnap12.so
targets_libhnap12 := $(addprefix $(ComponentBuildDir)/,$(targets_libhnap12))

source_files_libhnap12 := hnap12.o hnap12_methods.o hnap12_util.o
obj_files_libhnap12 := $(addprefix $(ComponentBuildDir)/,$(source_files_libhnap12:%.c=%.o))

$(targets_libhnap12): $(obj_files_libhnap12)

all: $(targets_libhnap12) install

clean: 
	rm -f $(ComponentBuildDir)
	rm -Rf hnap.[ch] hnap_adi.txt

install: install_fs stage

install_fs:
	@echo "Copying...[$(targets_libhnap12) to $(CCSP_INSTALL_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(targets_libhnap12) $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[$(targets_libhnap12) to $(CCSP_STAGING_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(targets_libhnap12) $(CCSP_STAGING_ROOT)/lib/
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include
	@install -m 0644 hnap12_util.h $(CCSP_STAGING_ROOT)/include/
	@install -m 0644 hnap12.h $(CCSP_STAGING_ROOT)/include/
	@install -m 0644 $(HDK2_ROOT_DIR)/include/hdk_mod.h $(CCSP_STAGING_ROOT)/include/

# HSL code generation
hnap12.c: $(HSL)
	python $(HDK2_ROOT_DIR)/bin/hdkmod.py --adi-report -I $(HDK2_ROOT_DIR)/spec \
			--action-exclude FirmwareUpload -o $(shell pwd) -b hnap12 $^

# Generate server method stubs
methods:
	@python $(HDK2_ROOT_DIR)/bin/hdkmod.py --server-methods -b hnap12 $(HSL)

.PHONY : all clean install install_fs stage methods

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

