#######################################################################
#   Copyright [2014] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the \"License\");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an \"AS IS\" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#######################################################################

#######################################################################
#   Copyright [2014] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the \"License\");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an \"AS IS\" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#######################################################################

#
# Copyright (c) 2008-2010 Cisco Systems, Inc. All rights reserved.
#
# Cisco Systems, Inc. retains all right, title and interest (including all
# intellectual property rights) in and to this computer program, which is
# protected by applicable intellectual property laws.  Unless you have obtained
# a separate written license from Cisco Systems, Inc., you are not authorized
# to utilize all or a part of this computer program for any purpose (including
# reproduction, distribution, modification, and compilation into object code),
# and you must immediately destroy or return to Cisco Systems, Inc. all copies
# of this computer program.  If you are licensed by Cisco Systems, Inc., your
# rights to utilize this computer program are limited by the terms of that
# license.  To obtain a license, please contact Cisco Systems, Inc.
#
# This computer program contains trade secrets owned by Cisco Systems, Inc.
# and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
# maintain the confidentiality of this computer program and related information
# and to not disclose this computer program and related information to any
# other person or entity.
#
# THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
# SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
# INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
# PURPOSE, TITLE, AND NONINFRINGEMENT.
#

# Platform-specific
ifeq ($(shell uname -s), Darwin) # Mac OS X
PLATFORM_LIBEXT = dylib
else
PLATFORM_LIBEXT = so
endif

# Build target (default)
.PHONY: build
build: malloc_interposer \
       libhdkxml \
       libhdkcli \
       libhdkcli-c++ \
       libhdkcli-logging \
       libhdkcli-c++-logging \
       unittest \
       unittest-static \
       unittest-c++ \
       unittest-static-c++ \
       unittest-logging \
       unittest-static-logging

# Clean the build directory
.PHONY: clean
clean:
	rm -r -f actual/
	rm -r -f build/
	for dir in tests/*; \
	do \
	    rm -r -f $$dir/actual; \
	done

# malloc_interposer
.PHONY: malloc_interposer
malloc_interposer:
	make -C ../../unittest BUILDDIR=$(abspath build/malloc_interposer) build_malloc_interposer

# libhdkxml
.PHONY: libhdkxml
libhdkxml:
	make -C ../../libhdkxml BUILDDIR=$(abspath build/libhdkxml)

# libhdkcli
.PHONY: libhdkcli
libhdkcli:
	make -C .. BUILDDIR=$(abspath build/libhdkcli) BUILDDIR_LIBHDKXML=$(abspath build/libhdkxml)

# libhdkcli-c++
.PHONY: libhdkcli-c++
libhdkcli-c++:
	make -C .. BUILDDIR=$(abspath build/libhdkcli-c++) BUILDDIR_LIBHDKXML=$(abspath build/libhdkxml) HDK_CLI_CXX=1

# libhdkcli-logging
.PHONY: libhdkcli-logging
libhdkcli-logging:
	make -C .. BUILDDIR=$(abspath build/libhdkcli-logging) BUILDDIR_LIBHDKXML=$(abspath build/libhdkxml) HDK_CLI_LOGGING=1

# libhdkcli-c++-logging
.PHONY: libhdkcli-c++-logging
libhdkcli-c++-logging:
	make -C .. BUILDDIR=$(abspath build/libhdkcli-c++-logging) BUILDDIR_LIBHDKXML=$(abspath build/libhdkxml) HDK_CLI_CXX=1 HDK_CLI_LOGGING=1

# unittest, libhdkxml, libhdkcli
.PHONY: unittest
unittest:
	@if [ ! -d build/unittest ]; then mkdir -p build/unittest; fi
	@if [ ! -L build/unittest/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest; fi
	@if [ ! -L build/unittest/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest; fi
	@if [ ! -L build/unittest/libhdkcli.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkcli/libhdkcli.$(PLATFORM_LIBEXT)) build/unittest; fi
	make -f Makefile.build BUILDDIR=build/unittest

# unittest, libhdkxml-static, libhdkcli-static
.PHONY: unittest-static
unittest-static:
	@if [ ! -d build/unittest-static ]; then mkdir -p build/unittest-static; fi
	@if [ ! -L build/unittest-static/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static; fi
	@if [ ! -L build/unittest-static/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml/libhdkxml.a) build/unittest-static; fi
	@if [ ! -L build/unittest-static/libhdkcli.a ]; then ln -s $(abspath build/libhdkcli/libhdkcli.a) build/unittest-static; fi
	make -f Makefile.build BUILDDIR=build/unittest-static UNITTEST_LDFLAGS="-lhdkxml -lexpat -lcurl"

# unittest-c++, libhdkxml, libhdkcli
.PHONY: unittest-c++
unittest-c++:
	@if [ ! -d build/unittest-c++ ]; then mkdir -p build/unittest-c++; fi
	@if [ ! -L build/unittest-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-c++; fi
	@if [ ! -L build/unittest-c++/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest-c++; fi
	@if [ ! -L build/unittest-c++/libhdkcli.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkcli/libhdkcli.$(PLATFORM_LIBEXT)) build/unittest-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-c++ UNITTEST_CXX=1

# unittest-c++, libhdkxml-static, libhdkcli-static
.PHONY: unittest-static-c++
unittest-static-c++:
	@if [ ! -d build/unittest-static-c++ ]; then mkdir -p build/unittest-static-c++; fi
	@if [ ! -L build/unittest-static-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static-c++; fi
	@if [ ! -L build/unittest-static-c++/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml/libhdkxml.a) build/unittest-static-c++; fi
	@if [ ! -L build/unittest-static-c++/libhdkcli.a ]; then ln -s $(abspath build/libhdkcli/libhdkcli.a) build/unittest-static-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-static-c++ UNITTEST_CXX=1 UNITTEST_LDFLAGS="-lhdkxml -lexpat -lcurl"

# unittest-logging, libhdkxml, libhdkcli-logging
.PHONY: unittest-logging
unittest-logging:
	@if [ ! -d build/unittest-logging ]; then mkdir -p build/unittest-logging; fi
	@if [ ! -L build/unittest-logging/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-logging; fi
	@if [ ! -L build/unittest-logging/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest-logging; fi
	@if [ ! -L build/unittest-logging/libhdkcli.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkcli-logging/libhdkcli.$(PLATFORM_LIBEXT)) build/unittest-logging; fi
	make -f Makefile.build BUILDDIR=build/unittest-logging

# unittest-static-logging, libhdkxml-static, libhdkcli-static-logging
.PHONY: unittest-static-logging
unittest-static-logging:
	@if [ ! -d build/unittest-static-logging ]; then mkdir -p build/unittest-static-logging; fi
	@if [ ! -L build/unittest-static-logging/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static-logging; fi
	@if [ ! -L build/unittest-static-logging/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml/libhdkxml.a) build/unittest-static-logging; fi
	@if [ ! -L build/unittest-static-logging/libhdkcli.a ]; then ln -s $(abspath build/libhdkcli-logging/libhdkcli.a) build/unittest-static-logging; fi
	make -f Makefile.build BUILDDIR=build/unittest-static-logging UNITTEST_LDFLAGS="-lhdkxml -lexpat -lcurl"