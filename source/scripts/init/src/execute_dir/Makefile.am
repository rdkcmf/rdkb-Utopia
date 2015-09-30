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
AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lpthread
AM_LDFLAGS += -lz
AM_LDFLAGS += -lsrvmgr
AM_LDFLAGS += -lsyscfg
AM_LDFLAGS += -lsysevent
AM_LDFLAGS += -lulog

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

noinst_LTLIBRARIES = libUtopia_execute_dir.la
libUtopia_execute_dir_la_CPPFLAGS = -I$(top_srcdir)/source/cosa/include/linux -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/include -I$(top_srcdir)/source/hdk2/hdk2/include -I$(top_srcdir)/source/pal/upnp/include -I$(top_srcdir)/source/pal/xml/include -I$(top_srcdir)/source/pal/kernel/include -I$(top_srcdir)/source/pal/include -I$(top_srcdir)/source/pal/log/include $(CPPFLAGS)
libUtopia_execute_dir_la_SOURCES = execute_dir.c
libUtopia_execute_dir_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz -lsrvmgr -lsyscfg -lsysevent -lulog
