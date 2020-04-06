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

AM_CFLAGS = -I$(top_srcdir)/source/include \
            -I$(top_srcdir)/source/util \
            -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/dbus-1.0 \
	    -I${PKG_CONFIG_SYSROOT_DIR}$(libdir)/dbus-1.0/include

AM_LDFLAGS = $(top_builddir)/source/services/lib/libsrvmgr.la \
             $(top_builddir)/source/sysevent/lib/libsysevent.la \
             $(top_builddir)/source/syscfg/lib/libsyscfg.la \
             $(top_builddir)/source/ulog/libulog.la \
             $(top_builddir)/source/utapi/lib/libutapi.la \
             $(top_builddir)/source/utctx/lib/libutctx.la \
             -ltelemetry_msgsender \
             -lcjson

if PLATFORM_HAL_SYNDICATION
AM_LDFLAGS += "-lhal_platform -lsysevent -lutctx -lutapi -lhal_wifi"
endif

bin_PROGRAMS = apply_system_defaults

apply_system_defaults_SOURCES = apply_system_defaults.c
