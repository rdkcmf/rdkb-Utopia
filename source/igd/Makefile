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

#Makefile for utopia IGD code
include $(CCSP_ROOT_DIR)/arch/ccsp_common.mk

CFLAGS += $(addprefix -I,$(INCPATH))
CFLAGS += -D_LINUX -pipe 
CFLAGS += -DPAL_ULOG
CFLAGS += -Wall -Wextra -Isrc/inc

LDFLAGS += -lsyscfg
LDFLAGS += -lulog
LDFLAGS += -lpal -lupnp -lixml -lpthread -lthreadutil
LDFLAGS += -lulog -Bdynamic -lutctx -lutapi
LDFLAGS += -lnvramstorage -lticc -lutils_docsis 

targets_igd := IGD
targets_igd := $(addprefix $(ComponentBuildDir)/,$(targets_igd))

source_files_igd := $(call add_files_from_base,src,'*.c')
obj_files_igd := $(addprefix $(ComponentBuildDir)/,$(source_files_igd:%.c=%.o))
-include $(obj_files_igd:%.o=%.d)

xml_files_to_install := $(call add_files_from_base,src/inc,'*.xml')

$(targets_igd): $(obj_files_igd)

all: $(targets_igd) install

clean: 
	rm -f $(ComponentBuildDir)

install:
	@echo "Copying...[$(targets_igd) to usr/sbin directory]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/usr/sbin
	@install -m 0755 $(targets_igd) $(CCSP_INSTALL_ROOT)/usr/sbin/
	@echo "Copying...[$(xml_files_to_install) to etc/IGD/]"
	@install -d -m 0755 $(CCSP_INSTALL_ROOT)/etc/IGD
	@install -m 0755 $(xml_files_to_install) $(CCSP_INSTALL_ROOT)/etc/IGD/

.PHONY : all clean install

#
# include custom post makefile, if exists
#
ifneq ($(findstring $(CCSP_CMPNT_BUILD_CUSTOM_MK_POST), $(wildcard $(ComponentBoardDir)/*.mk)), )
    include $(ComponentBoardDir)/$(CCSP_CMPNT_BUILD_CUSTOM_MK_POST)
endif
