##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
bin_PROGRAMS = service_dhcp

AM_CFLAGS = -fno-exceptions -Os -Wall -Wextra
AM_LDFLAGS =  -lpthread -lz -lccsp_common $(DBUS_LIBS)
service_dhcp_CPPFLAGS = -I$(top_srcdir)/include \
						-I$(top_srcdir)/source/include \
						-I$(top_srcdir)/source/util/utils \
						-I$(top_srcdir)/source/service_dhcp/include \
						$(DBUS_CFLAGS)

service_dhcp_SOURCES = service_dhcp.c lan_handler.c dhcp_server_functions.c service_dhcp_server.c
service_dhcp_LDADD = $(top_builddir)/source/util/utils/libutopiautil.la \
        		     $(top_builddir)/source/sysevent/lib/libsysevent.la \
					 $(top_builddir)/source/syscfg/lib/libsyscfg.la
