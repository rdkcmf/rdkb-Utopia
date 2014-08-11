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

#Makefile for Utopia api library code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -fPIC
CFLAGS += -Wall -Wextra -Wmissing-prototypes
CFLAGS += -Os -fpack-struct -ffunction-sections -fdata-sections

targets_libutapi := libutapi.a libutapi.so
targets_libutapi := $(addprefix $(ComponentBuildDir)/,$(targets_libutapi))

source_files_libutapi := utapi.c tr181_wrpr.c utapi_moca.c utapi_dns.c 
source_files_libutapi += tr181_util.c utapi_util.c utapi_wlan.c 
source_files_libutapi += utapi_tr_wlan.c utapi_tr_dhcp.c utapi_tr_user.c 
source_files_libutapi += utapi_security.c utapi_parental_control.c
obj_files_libutapi := $(addprefix $(ComponentBuildDir)/,$(source_files_libutapi:%.c=%.o))

$(targets_libutapi): $(obj_files_libutapi)

all: $(targets_libutapi) install

clean: 
	rm -f $(ComponentBuildDir)

install: install_fs stage

install_fs:
	@echo "Copying...[Libraries to Install Directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[Header files to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include/utapi
	@install -m 0755 utapi.h $(CCSP_STAGING_ROOT)/include/utapi
	@install -m 0755 utapi_util.h $(CCSP_STAGING_ROOT)/include/utapi
	@install -m 0755 utapi_wlan.h $(CCSP_STAGING_ROOT)/include/utapi
	@install -m 0755 utapi_tr_user.h $(CCSP_STAGING_ROOT)/include/utapi
	@install -m 0755 utapi_tr_dhcp.h $(CCSP_STAGING_ROOT)/include/utapi
	@install -m 0755 utapi_security.h $(CCSP_STAGING_ROOT)/include/utapi
	@install -m 0755 utapi_parental_control.h $(CCSP_STAGING_ROOT)/include/utapi
	@echo "Copying...[Libraries to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_STAGING_ROOT)/lib/

.PHONY : all clean install install_fs stage

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

