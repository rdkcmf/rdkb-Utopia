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
AM_CFLAGS += -D_LINUX
AM_CFLAGS += -DPAL_ULOG
AM_CFLAGS += -DCONFIG_VENDOR_MODEL=\"\"
AM_CFLAGS += $(PTHREAD_CFLAGS)

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = IGD

IGD_SOURCES = igd_service_wan_ethlink_cfg.c igd_device_root.c igd_utility.c igd_device_lan.c igd_service_wan_commif_cfg.c igd_action_port_mapping.c igd_device_wan_connect.c igd_device_wan.c igd_platform_independent_inf.c igd_service_lanhostconfigmgmt.c igd_service_wan_connect.c igd_service_layer3_fwd.c

IGD_CPPFLAGS = -I$(top_srcdir)/source/pal/upnp/include \
               -I$(top_srcdir)/source/pal/xml/include \
               -I$(top_srcdir)/source/pal/kernel/include \
               -I$(top_srcdir)/source/pal/include \
               -I$(top_srcdir)/source/pal/log/include \
               -I$(top_srcdir)/source/igd/src/inc \
               -I$(top_srcdir)/source/include \
               -I$(top_srcdir)/source/include/utapi

IGD_LDFLAGS = -lthreadutil -Bdynamic -lupnp $(PTHREAD_LIBS)

IGD_LDADD = $(top_builddir)/source/services/lib/libsrvmgr.la \
            $(top_builddir)/source/sysevent/lib/libsysevent.la \
            $(top_builddir)/source/ulog/libulog.la \
            $(top_builddir)/source/utctx/lib/libutctx.la \
            $(top_builddir)/source/pal/libpal.la \
            $(top_builddir)/source/utapi/lib/libutapi.la \
            $(top_builddir)/source/syscfg/lib/libsyscfg.la
