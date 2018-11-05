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
bin_PROGRAMS = service_multinet_exec

#AM_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/Utopia/source/include -I$(top_srcdir)/source/util -I$(top_srcdir)/source/service_multinet/Puma6_plat $(CPPFLAGS)
AM_CFLAGS = -fno-exceptions
AM_CFLAGS = -Os
AM_CFLAGS = -Wall -Wextra

AM_LDFLAGS =  -lpthread -lz -lccsp_common $(DBUS_LIBS) -lsecure_wrapper

service_multinet_exec_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/source/include -I$(top_srcdir)/source/util/utils -I$(top_srcdir)/source/service_multinet/Puma6_plat -I$(top_srcdir)/source/include/service_multinet $(DBUS_CFLAGS)
service_multinet_exec_SOURCES = ep_access.c  ev_access.c  nv_access.c  service_multinet_handler.c  service_multinet_lib.c  service_multinet_main.c  service_multinet_swfab.c  service_multinet_swfab_deps.c  service_multinet_swfab_ep.c service_multinet_swfab_LinIF.c  util/service_multinet_util.c Puma6_plat/puma6_plat_map.c  Puma6_plat/puma6_plat_sw.c

if !CCSP_PLATFORM_BCM
service_multinet_exec_SOURCES += Puma6_plat/handle_sw.c Puma6_plat/configure_l2switch.c
endif

service_multinet_exec_LDADD = $(top_builddir)/source/util/utils/libutopiautil.la \
                    $(top_builddir)/source/services/lib/libsrvmgr.la \
                    $(top_builddir)/source/sysevent/lib/libsysevent.la \
                    $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                    $(top_builddir)/source/ulog/libulog.la
