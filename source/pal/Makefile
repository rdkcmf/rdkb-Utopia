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

#Makefile for Utopia libpal code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -Iinclude -Ilog/include -Iupnp/include -Ikernel/include -Ixml/include

targets_libpal := libpal.a libpal.so
targets_libpal := $(addprefix $(ComponentBuildDir)/,$(targets_libpal))

source_files_libpal := $(call add_files_from_base,xml,'*.c')
source_files_libpal += $(call add_files_from_base,upnp,'*.c')
source_files_libpal += $(call add_files_from_base,kernel,'*.c')
#source_files_libpal += $(call add_files_from_base,log,'*.c')
obj_files_libpal := $(addprefix $(ComponentBuildDir)/,$(source_files_libpal:%.c=%.o))

$(targets_libpal): $(obj_files_libpal)

all: $(targets_libpal) install

clean: 
	rm -f $(ComponentBuildDir)

install: install_fs stage

install_fs:
	@echo "Copying...[Libraries to Install Directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[Header files to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include/pal
	@install -m 0755 include/pal_def.h $(CCSP_STAGING_ROOT)/include/pal/
	@install -m 0755 upnp/include/pal_upnp.h $(CCSP_STAGING_ROOT)/include/pal/
	@install -m 0755 upnp/include/pal_upnp_device.h $(CCSP_STAGING_ROOT)/include/pal/
	@install -m 0755 log/include/pal_log.h $(CCSP_STAGING_ROOT)/include/pal/
	@install -m 0755 xml/include/pal_xml.h $(CCSP_STAGING_ROOT)/include/pal/
	@install -m 0755 xml/include/pal_xml2s.h $(CCSP_STAGING_ROOT)/include/pal/
	@install -m 0755 kernel/include/pal_kernel.h $(CCSP_STAGING_ROOT)/include/pal/
	@echo "Copying...[Libraries to Staging Directory]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(ComponentBuildDir)/lib*.so $(CCSP_STAGING_ROOT)/lib/
	@install -m 0755 $(ComponentBuildDir)/lib*.a $(CCSP_STAGING_ROOT)/lib/

.PHONY : all clean install install_fs stage

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
