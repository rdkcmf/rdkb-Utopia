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
CFLAGS += -Wall -Wextra -Werror -Wno-long-long -Wmissing-prototypes
CFLAGS += -fPIC -fvisibility=hidden
CFLAGS += -DHDK_XML_BUILD -I$(HDK2_ROOT_DIR)/include
ifdef HDK_XML_LIBXML2
CFLAGS += -DHDK_XML_LIBXML2
endif
ifdef HDK_XML_LOGGING
CFLAGS += -DHDK_LOGGING=1
endif

# Link Flags
LDFLAGS += -shared-libgcc
ifdef HDK_XML_LIBXML2
LDFLAGS += -lxml2
else
LDFLAGS += -lexpat
endif

targets_libhdkxml := libhdkxml.a libhdkxml.so
targets_libhdkxml := $(addprefix $(ComponentBuildDir)/,$(targets_libhdkxml))

source_files_libhdkxml := hdk_xml_encode.c  hdk_xml_parse.c  hdk_xml_serialize.c 
source_files_libhdkxml += hdk_xml_type.c hdk_xml_log.c hdk_xml_schema.c 
source_files_libhdkxml += hdk_xml_stream.c hdk_xml_validate.c
obj_files_libhdkxml := $(addprefix $(ComponentBuildDir)/,$(source_files_libhdkxml:%.c=%.o))

$(targets_libhdkxml): $(obj_files_libhdkxml)

all: $(targets_libhdkxml) install

clean: 
	rm -f $(ComponentBuildDir)

install: install_fs stage

install_fs:
	@echo "Copying...[$(targets_libhdkxml) to $(CCSP_INSTALL_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/lib
	@install -m 0755 $(targets_libhdkxml) $(CCSP_INSTALL_ROOT)/lib/

stage:
	@echo "Copying...[$(targets_libhdkxml) to $(CCSP_STAGING_ROOT)/lib/]"
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/lib
	@install -m 0755 $(targets_libhdkxml) $(CCSP_STAGING_ROOT)/lib/
	@install -d -m 0755 $(CCSP_STAGING_ROOT)/include
	@install -m 0644 $(HDK2_ROOT_DIR)/include/hdk_xml.h $(CCSP_STAGING_ROOT)/include/
	@install -m 0644 $(HDK2_ROOT_DIR)/include/hdk_log.h $(CCSP_STAGING_ROOT)/include/

.PHONY : all clean install install_fs stage

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif

