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
AM_CFLAGS = -D_GNU_SOURCE
AM_CFLAGS += $(PTHREAD_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/source/include $(DBUS_CFLAGS)
AM_LDFLAGS = -lccsp_common -lsecure_wrapper -lnetfilter_queue -lnfnetlink $(DBUS_LIBS) $(PTHREAD_LIBS) -lrt

bin_PROGRAMS = firewall nfq_handler

firewall_SOURCES = firewall.c firewall_priv.c firewall_interface.c
if CPC_FIREWALL_ENABLE
firewall_SOURCES += firewall_lib.c firewall_dsl.c rabid.c
endif

nfq_handler_SOURCES = raw_socket_send.c nfq_handler.c

firewall_LDADD = $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                 $(top_builddir)/source/sysevent/lib/libsysevent.la \
                 $(top_builddir)/source/ulog/libulog.la

nfq_handler_LDADD = $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                    $(top_builddir)/source/sysevent/lib/libsysevent.la \
                    $(top_builddir)/source/ulog/libulog.la
