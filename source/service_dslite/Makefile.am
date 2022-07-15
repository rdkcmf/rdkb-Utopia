#######################################################################
# Copyright 2017-2019 ARRIS Enterprises, LLC.
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
#######################################################################

bin_PROGRAMS = service_dslite

service_dslite_SOURCES = service_dslite.c
service_dslite_CFLAGS = -I$(top_srcdir)/source -I$(top_srcdir)/source/include -I$(top_srcdir)/source/util/utils
service_dslite_LDFLAGS = -levent -levent_core -levent_extra -lpthread -lsecure_wrapper
service_dslite_LDADD = $(top_builddir)/source/util/utils/libutopiautil.la \
                       $(top_builddir)/source/services/lib/libsrvmgr.la \
                       $(top_builddir)/source/sysevent/lib/libsysevent.la \
                       $(top_builddir)/source/syscfg/lib/libsyscfg.la \
                       $(top_builddir)/source/ulog/libulog.la
