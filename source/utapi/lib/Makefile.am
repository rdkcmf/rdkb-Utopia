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
AM_CFLAGS += -fPIC

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libutapi.la

libutapi_la_SOURCES = utapi_tr_wlan.c utapi.c utapi_util.c tr181_util.c utapi_parental_control.c tr181_wrpr.c utapi_wlan.c utapi_security.c utapi_dns.c utapi_tr_dhcp.c utapi_tr_user.c utapi_moca.c utapi_dslite.c

libutapi_la_CPPFLAGS = -I$(top_srcdir)/source/include -I$(top_srcdir)/source/utapi/lib \
                       -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/dbus-1.0 \
                       -I${PKG_CONFIG_SYSROOT_DIR}$(libdir)/dbus-1.0/include

libutapi_la_LDFLAGS = -lsecure_wrapper -ltelemetry_msgsender

libutapi_la_LIBADD = $(top_builddir)/source/ulog/libulog.la
