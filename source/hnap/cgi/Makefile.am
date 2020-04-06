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

ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = libUtopia_cgi.la

libUtopia_cgi_la_SOURCES = hdk_cgi_context.c hdk_cgi.c

libUtopia_cgi_la_CPPFLAGS = -I$(top_srcdir)/source/cosa/include/linux \
                            -I$(top_srcdir)/source/igd/arch/intel_usg/boards/rdkb_arm/include \
                            -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include \
                            -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/arm_shared/include \
                            -I$(top_srcdir)/source/hdk2/hdk2/include \
                            -I$(top_srcdir)/source/pal/upnp/include \
                            -I$(top_srcdir)/source/pal/xml/include \
                            -I$(top_srcdir)/source/pal/kernel/include \
                            -I$(top_srcdir)/source/pal/include \
                            -I$(top_srcdir)/source/pal/log/include

libUtopia_cgi_la_LDFLAGS = -lsrvmgr -lsyscfg -lsysevent -lulog
