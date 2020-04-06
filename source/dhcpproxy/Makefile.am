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
AM_CFLAGS =

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = dhcp_proxy

dhcp_proxy_SOURCES = packet_util.c dhcp_proxy.c dhcp_util.c dhcp_msg.c

dhcp_proxy_CPPFLAGS = -I$(top_srcdir)/source/include

dhcp_proxy_LDADD = $(top_builddir)/source/services/lib/libsrvmgr.la \
                   $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                   $(top_builddir)/source/sysevent/lib/libsysevent.la \
                   $(top_builddir)/source/ulog/libulog.la
