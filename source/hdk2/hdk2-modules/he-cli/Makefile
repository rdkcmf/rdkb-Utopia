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
CFLAGS += -ansi -Wall -Wextra -Wno-long-long -Wmissing-prototypes
CFLAGS += -fPIC -fvisibility=hidden
CFLAGS += -DHE_CLI_BUILD -I$(HDK2_ROOT_DIR)/include
ifdef HE_CLI_UNITTEST
MODULE_CFLAGS += -DHE_CLI_UNITTEST
endif

# Link Flags
LDFLAGS += -lhdkxml -lhdksrv -shared-libgcc

HSL := spec/event-notify.hsl

targets_libhe_cli := libhe_cli.a libhe_cli.so
targets_libhe_cli := $(addprefix $(ComponentBuildDir)/,$(targets_libhe_cli))

source_files_libhe_cli := he_cli.o he_cli_methods.o
obj_files_libhe_cli := $(addprefix $(ComponentBuildDir)/,$(source_files_libhe_cli:%.c=%.o))

$(targets_libhe_cli): $(obj_files_libhe_cli)

all: $(targets_libhe_cli) install

clean: 
	rm -f $(ComponentBuildDir)
	rm -Rf he_cli.[ch] he_cli_adi.txt

install: install_fs stage

install_fs:
	@echo "Copying...[$(targets_libhe_cli) to $(CCSP_INSTALL_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(targets_libhe_cli) $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[$(targets_libhe_cli) to $(CCSP_STAGING_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(targets_libhe_cli) $(CCSP_STAGING_ROOT)/lib/
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include
	@install -m 0644 he_cli.h $(CCSP_STAGING_ROOT)/include/

# HSL code generation
he_cli.c: $(HSL)
	@python $(HDK2_ROOT_DIR)/bin/hdkmod.py --adi-report -I $(HDK2_ROOT_DIR)/spec \
			-o $(shell pwd) -b he_cli $^

# Generate server method stubs
methods:
	@python $(HDK2_ROOT_DIR)/bin/hdkmod.py --server-methods -I $(HDK2_ROOT_DIR)/spec \
			-b he_cli $(HSL)

.PHONY : all clean install install_fs stage methods

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
