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

#Makefile for utopia sample context code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -x c -pedantic-errors -Wall -Wextra -Werror -Wmissing-prototypes -Wno-long-long
CFLAGS += -fpack-struct -Os -ffunction-sections -fdata-sections

LDFLAGS += -Wl,--gc-sections -shared-libgcc 
LDFLAGS += -lsyscfg -lsysevent -lutctx -lulog -lpthread

targets_sample_utctx := sample_utctx
targets_sample_utctx := $(addprefix $(ComponentBuildDir)/,$(targets_sample_utctx))

source_files_sample_utctx := sample_utctx.c
obj_files_sample_utctx := $(addprefix $(ComponentBuildDir)/,$(source_files_sample_utctx:%.c=%.o))

$(targets_sample_utctx): $(obj_files_sample_utctx)


all: $(targets_sample_utctx) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_sample_utctx) to SBIN directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/sbin
	@install -m 0755 $(targets_sample_utctx) $(CCSP_INSTALL_ROOT)/sbin/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
