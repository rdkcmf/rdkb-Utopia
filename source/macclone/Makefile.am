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
bin_PROGRAMS = macclone
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

macclone_CPPFLAGS = -I$(top_srcdir)/source/pal/upnp/include \
		    -I$(top_srcdir)/source/pal/xml/include \
		    -I$(top_srcdir)/source/pal/kernel/include \
		    -I$(top_srcdir)/source/pal/include \
		    -I$(top_srcdir)/source/pal/log/include \
		    $(DBUS_CFLAGS) 

macclone_SOURCES = macclone.c

macclone_LDFLAGS =  -lpthread -lz $(DBUS_LIBS)

macclone_LDADD = $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                $(top_builddir)/source/sysevent/lib/libsysevent.la \
                $(top_builddir)/source/ulog/libulog.la \
                $(top_builddir)/source/services/lib/libsrvmgr.la
