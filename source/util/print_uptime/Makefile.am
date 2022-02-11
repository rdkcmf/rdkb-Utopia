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

AM_CFLAGS = -fno-exceptions -Wall -Wextra

lib_LTLIBRARIES = libprint_uptime.la

libprint_uptime_la_CPPFLAGS = -I$(top_srcdir)/source/include
libprint_uptime_la_SOURCES = print_uptime.c
libprint_uptime_la_LDFLAGS = -ltelemetry_msgsender

bin_PROGRAMS = print_uptime

print_uptime_CPPFLAGS = -I$(top_srcdir)/source/include -I$(top_srcdir)/source/util/print_uptime -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/dbus-1.0 ${PKG_CONFIG_SYSROOT_DIR}$(libdir)/dbus-1.0/include
print_uptime_SOURCES = main.c
print_uptime_LDADD = libprint_uptime.la
