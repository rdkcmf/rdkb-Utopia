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
#
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -pedantic-errors
CFLAGS += -ansi -Wall -Wextra -Wno-long-long -Wmissing-prototypes
CFLAGS += -fPIC -fvisibility=hidden
CFLAGS += -DHE_AUTH_BUILD -I$(HDK2_ROOT_DIR)/include
ifdef HE_AUTH_UNITTEST
CFLAGS += -DHE_AUTH_UNITTEST
endif

# Link Flags
LDFLAGS += -lhdkxml -lhdksrv -shared-libgcc

HSL := spec/registrar.hsl
HSL += spec/event-dispatch.hsl
HSL += state/registrar_state.hsl
HSL += state/event-dispatch_state.hsl

targets_libhe_auth := libhe_auth.a libhe_auth.so
targets_libhe_auth := $(addprefix $(ComponentBuildDir)/,$(targets_libhe_auth))

source_files_libhe_auth := he_auth.o he_auth_methods.o
obj_files_libhe_auth := $(addprefix $(ComponentBuildDir)/,$(source_files_libhe_auth:%.c=%.o))

$(targets_libhe_auth): $(obj_files_libhe_auth)

all: $(targets_libhe_auth) install

clean: 
	rm -f $(ComponentBuildDir)
	rm -Rf he_auth.[ch] he_auth_adi.txt

install: install_fs stage

install_fs:
	@echo "Copying...[$(targets_libhe_auth) to $(CCSP_INSTALL_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(targets_libhe_auth) $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[$(targets_libhe_auth) to $(CCSP_STAGING_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(targets_libhe_auth) $(CCSP_STAGING_ROOT)/lib/

# HSL code generation
he_auth.c: $(HSL)
	@python $(HDK2_ROOT_DIR)/bin/hdkmod.py --adi-report -I $(HDK2_ROOT_DIR)/spec \
			-I spec -o $(shell pwd) -b he_auth $^

# Generate server method stubs
methods:
	@python $(HDK2_ROOT_DIR)/bin/hdkmod.py --server-methods -I $(HDK2_ROOT_DIR)/spec \
			-I spec -b he_auth $(HSL)

.PHONY : all clean install install_fs stage methods

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
