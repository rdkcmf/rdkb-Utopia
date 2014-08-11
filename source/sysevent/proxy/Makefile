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

default : all

XTARGETS := syseventd_proxy
OUTPUT_DIR := $(OBJDIR)/sbin

EXTRA_CFLAGS += -I../lib -I$(INCLUDEDIR)/include  -Os -fno-exceptions
#if you want to disable ipv6 then set NO_IPV6
#EXTRA_CFLAGS += -DNO_IPV6
EXTRA_CFLAGS += -Wall -Wextra #-Werror


EXTRA_LDFLAGS += -lsysevent
EXTRA_LDFLAGS += -lulog

$(OUTPUT_DIR)/syseventd_proxy: proxy_main.c 

####common targets-------------------

STATLIB_TARGETS = $(STATIC_TARGETS:%=$(OUTPUT_DIR)/%.a)
SO_TARGETS = $(DYNAMIC_TARGETS:%=$(OUTPUT_DIR)/%.so)
EXETARGETS = $(XTARGETS:%=$(OUTPUT_DIR)/%)


all: $(SO_TARGETS) $(STATLIB_TARGETS) $(EXETARGETS)
	 

$(EXETARGETS): 
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS) -o $@  $^

$(SO_TARGETS):
	$(CC) -shared $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ $^

$(STATLIB_TARGETS):
	$(LD) -r -o $@ $^

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS) -c -o $@ $^

.PHONY : clean
clean: 
	rm -f $(OBJECTS) $(SO_TARGETS) $(STATLIB_TARGETS) $(EXETARGETS)


#PUBLIC_INCLUDE := utapi.h utapi_wlan.h