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
#
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -pedantic-errors
CFLAGS += -ansi -Wall -Wextra -Wno-long-long -Wmissing-prototypes
CFLAGS += -fPIC -fvisibility=hidden
CFLAGS += -DHOTSPOT_BUILD -I$(HDK2_ROOT_DIR)/include

# Link Flags
LDFLAGS += -lhdkxml -lhdksrv -shared-libgcc

HSL := spec/device_list.hsl
HSL += spec/hnd2.hsl
HSL += spec/guest_access.hsl
HSL += spec/parental_controls.hsl
HSL += spec/setup.hsl
HSL += state/device_list_state.hsl
HSL += state/hnd2_state.hsl
HSL += state/guest_access_state.hsl
HSL += state/parental_controls_state.hsl
HSL += state/setup_state.hsl

targets_libhotspot := libhotspot.a libhotspot.so
targets_libhotspot := $(addprefix $(ComponentBuildDir)/,$(targets_libhotspot))

source_files_libhotspot := hotspot.o hotspot_methods.o hotspot_util.o
obj_files_libhotspot := $(addprefix $(ComponentBuildDir)/,$(source_files_libhotspot:%.c=%.o))

$(targets_libhotspot): $(obj_files_libhotspot)

all: $(targets_libhotspot) install

clean: 
	rm -f $(ComponentBuildDir)
	rm -Rf hotspot.[ch] hotspot_adi.txt hotspot_util.c

install: install_fs stage

install_fs:
	@echo "Copying...[$(targets_libhotspot) to $(CCSP_INSTALL_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(targets_libhotspot) $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[$(targets_libhotspot) to $(CCSP_STAGING_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(targets_libhotspot) $(CCSP_STAGING_ROOT)/lib/
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include
	@install -m 0644 hotspot.h $(CCSP_STAGING_ROOT)/include/

# HSL code generation
hotspot.c: $(HSL)
	@python $(HDK2_ROOT_DIR)/bin/hdkmod.py --adi-report -I $(HDK2_ROOT_DIR)/spec \
			-I spec -I $(HDK2_ROOT_DIR)/libhnap12/state -o $(shell pwd) -b hotspot $^

hotspot_util.c: $(HDK2_ROOT_DIR)/libhnap12/hnap12_util.c
	@ln -sf $< $@

# Generate server method stubs
methods:
	@python $(HDK2_ROOT_DIR)/bin/hdkmod.py --server-methods -b hotspot $(HSL)

.PHONY : all clean install install_fs stage methods

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
