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

bin_PROGRAMS = service_routed

AM_LDFLAGS = -lsecure_wrapper

service_routed_SOURCES = service_routed.c

service_routed_CFLAGS = -I$(top_srcdir)/source \
                        -I$(top_srcdir)/source/include \
                        -I$(top_srcdir)/source/utctx/lib \
                        -I$(top_srcdir)/source/utapi/lib \
                        -I$(top_srcdir)/source/util/utils \
                        -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/dbus-1.0 \
                        -I${PKG_CONFIG_SYSROOT_DIR}$(libdir)/dbus-1.0/include


service_routed_LDADD = $(top_builddir)/source/util/utils/libutopiautil.la \
                       $(top_builddir)/source/services/lib/libsrvmgr.la \
                       $(top_builddir)/source/sysevent/lib/libsysevent.la \
                       $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                       $(top_builddir)/source/utapi/lib/libutapi.la \
                       $(top_builddir)/source/utctx/lib/libutctx.la \
                       $(top_builddir)/source/ulog/libulog.la \
                       -ltelemetry_msgsender
