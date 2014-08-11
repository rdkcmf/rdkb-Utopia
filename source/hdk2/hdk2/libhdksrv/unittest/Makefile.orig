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
       libhdksrv \
       libhdksrv-c++ \
       libhdksrv-nologging \
       unittest \
       unittest-static \
       unittest-c++ \
       unittest-static-c++

# Clean the build directory
.PHONY: clean
clean:
	rm -r -f actual/
	rm -r -f build/

# malloc_interposer
.PHONY: malloc_interposer
malloc_interposer:
	make -C ../../unittest BUILDDIR=$(abspath build/malloc_interposer) build_malloc_interposer

# libhdkxml
.PHONY: libhdkxml
libhdkxml:
	make -C ../../libhdkxml BUILDDIR=$(abspath build/libhdkxml)

# libhdksrv (C, logging)
.PHONY: libhdksrv
libhdksrv:
	make -C .. BUILDDIR=$(abspath build/libhdksrv) BUILDDIR_LIBHDKXML=$(abspath build/libhdkxml) HDK_SRV_LOGGING=1

# libhdksrv-c++ (C++ logging)
.PHONY: libhdksrv-c++
libhdksrv-c++:
	make -C .. BUILDDIR=$(abspath build/libhdksrv-c++) BUILDDIR_LIBHDKXML=$(abspath build/libhdkxml) HDK_SRV_CXX=1 HDK_SRV_LOGGING=1

# libhdksrv-nologging (C)
.PHONY: libhdksrv-nologging
libhdksrv-nologging:
	make -C .. BUILDDIR=$(abspath build/libhdksrv-nologging) BUILDDIR_LIBHDKXML=$(abspath build/libhdkxml)

# unittest, libhdkxml, libhdksrv
.PHONY: unittest
unittest:
	@if [ ! -d build/unittest ]; then mkdir -p build/unittest; fi
	@if [ ! -L build/unittest/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest; fi
	@if [ ! -L build/unittest/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest; fi
	@if [ ! -L build/unittest/libhdksrv.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdksrv/libhdksrv.$(PLATFORM_LIBEXT)) build/unittest; fi
	make -f Makefile.build BUILDDIR=build/unittest

# unittest-static, libhdkxml-static, libhdksrv-static
.PHONY: unittest-static
unittest-static:
	@if [ ! -d build/unittest-static ]; then mkdir -p build/unittest-static; fi
	@if [ ! -L build/unittest-static/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static; fi
	@if [ ! -L build/unittest-static/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml/libhdkxml.a) build/unittest-static; fi
	@if [ ! -L build/unittest-static/libhdksrv.a ]; then ln -s $(abspath build/libhdksrv/libhdksrv.a) build/unittest-static; fi
	make -f Makefile.build BUILDDIR=build/unittest-static UNITTEST_LDFLAGS="-lhdkxml -lexpat -ldl"

# unittest-c++, libhdkxml, libhdksrv
.PHONY: unittest-c++
unittest-c++:
	@if [ ! -d build/unittest-c++ ]; then mkdir -p build/unittest-c++; fi
	@if [ ! -L build/unittest-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-c++; fi
	@if [ ! -L build/unittest-c++/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest-c++; fi
	@if [ ! -L build/unittest-c++/libhdksrv.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdksrv/libhdksrv.$(PLATFORM_LIBEXT)) build/unittest-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-c++ UNITTEST_CXX=1

# unittest-static-c++, libhdkxml-static, libhdksrv-static
.PHONY: unittest-static-c++
unittest-static-c++:
	@if [ ! -d build/unittest-static-c++ ]; then mkdir -p build/unittest-static-c++; fi
	@if [ ! -L build/unittest-static-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static-c++; fi
	@if [ ! -L build/unittest-static-c++/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml/libhdkxml.a) build/unittest-static-c++; fi
	@if [ ! -L build/unittest-static-c++/libhdksrv.a ]; then ln -s $(abspath build/libhdksrv/libhdksrv.a) build/unittest-static-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-static-c++ UNITTEST_CXX=1 UNITTEST_LDFLAGS="-lhdkxml -lexpat -ldl"