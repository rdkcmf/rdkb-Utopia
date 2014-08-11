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

NO_CLI := y
NO_TEST := y
NO_DOCS := y

modules := hdk2/libhdkxml
modules += hdk2/libhdksrv
ifndef NO_CLI
modules += hdk2/libhdkcli
endif
modules += hdk2/libhnap12
ifndef NO_TEST
modules += hdk2/unittest
endif
modules += hdk2-modules/hotspot
#modules += hdk2-modules/he-cli
#modules += hdk2-modules/he-auth

HDK2_ROOT_DIR := $(shell pwd)/hdk2

all:
	@for m in $(modules); do echo $$m; HDK2_ROOT_DIR=$(HDK2_ROOT_DIR) make -C $$m $@ || exit 1; done

clean:
	@for m in $(modules); do echo $$m; HDK2_ROOT_DIR=$(HDK2_ROOT_DIR) make -C $$m $@ || exit 1; done

install:
	@for m in $(modules); do echo $$m; HDK2_ROOT_DIR=$(HDK2_ROOT_DIR) make -C $$m $@ || exit 1; done

.PHONY: all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
