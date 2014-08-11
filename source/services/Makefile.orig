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


modules += lib

modules := $(addprefix $(ComponentBaseDir)/,$(modules))

all:
	@for m in $(modules); do echo $$m; make -C $$m $@ || exit 1; done

clean:
	@for m in $(modules); do echo $$m; make -C $$m $@ || exit 1; done

install:
	@for m in $(modules); do echo $$m; make -C $$m $@ || exit 1; done

.PHONY: all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
