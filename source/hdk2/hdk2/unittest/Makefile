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
CFLAGS += -Wall -Wextra -Werror -Wno-long-long $(PLATFORM_CFLAGS)
CFLAGS += -I$(HDK2_ROOT_DIR)/include

LDFLAGS += -lhdkxml -lhdksrv -ldl -shared-libgcc

targets_unittest := unittest
targets_unittest := $(addprefix $(ComponentBuildDir)/,$(targets_unittest))

source_files_unittest := dhcp_msg.c dhcp_proxy.c dhcp_util.c packet_util.c
obj_files_unittest := $(addprefix $(ComponentBuildDir)/,$(source_files_unittest:%.c=%.o))

$(targets_unittest): $(obj_files_unittest)

targets_libmalloc_interposer := libmalloc_interposer.so
targets_libmalloc_interposer := $(addprefix $(ComponentBuildDir)/,$(targets_libmalloc_interposer))

source_files_libmalloc_interposer := malloc_interposer.c
obj_files_libmalloc_interposer := $(addprefix $(ComponentBuildDir)/,$(source_files_libmalloc_interposer:%.c=%.o))

$(targets_libmalloc_interposer): $(obj_files_libmalloc_interposer)

all: $(targets_libmalloc_interposer) $(targets_unittest)

clean: 
	rm -f $(ComponentBuildDir)

.PHONY : all clean

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

