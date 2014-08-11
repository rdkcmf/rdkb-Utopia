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

# Build target (default)
.PHONY: build
build: malloc_interposer \
       libhdkxml-expat \
       libhdkxml-libxml2 \
       libhdkxml-expat-c++ \
       libhdkxml-libxml2-c++ \
       libhdkxml-expat-nologging \
       unittest-expat \
       unittest-static-expat \
       unittest-libxml2 \
       unittest-static-libxml2 \
       unittest-expat-c++ \
       unittest-static-expat-c++ \
       unittest-libxml2-c++ \
       unittest-static-libxml2-c++

# Clean the build directory
.PHONY: clean
clean:
	rm -r -f actual/
	rm -r -f build/

# malloc_interposer
.PHONY: malloc_interposer
malloc_interposer:
	make -C ../../unittest BUILDDIR=$(abspath build/malloc_interposer) build_malloc_interposer

# libhdkxml (C, expat, logging)
.PHONY: libhdkxml-expat
libhdkxml-expat:
	make -C .. BUILDDIR=$(abspath build/libhdkxml-expat) HDK_XML_LOGGING=1

# libhdkxml (C, libxml2, logging)
.PHONY: libhdkxml-libxml2
libhdkxml-libxml2:
	make -C .. BUILDDIR=$(abspath build/libhdkxml-libxml2) HDK_XML_LIBXML2=1 HDK_XML_LOGGING=1

# libhdkxml (C++, expat, logging)
.PHONY: libhdkxml-expat-c++
libhdkxml-expat-c++:
	make -C .. BUILDDIR=$(abspath build/libhdkxml-expat-c++) HDK_XML_CXX=1 HDK_XML_LOGGING=1

# libhdkxml (C++, libxml2, logging)
.PHONY: libhdkxml-libxml2-c++
libhdkxml-libxml2-c++:
	make -C .. BUILDDIR=$(abspath build/libhdkxml-libxml2-c++) HDK_XML_CXX=1 HDK_XML_LIBXML2=1 HDK_XML_LOGGING=1

# libhdkxml (C, expat)
.PHONY: libhdkxml-expat-nologging
libhdkxml-expat-nologging:
	make -C .. BUILDDIR=$(abspath build/libhdkxml-expat-nologging)

# unittest (C), libhdkxml (C, expat)
.PHONY: unittest-expat
unittest-expat:
	@if [ ! -d build/unittest-expat ]; then mkdir -p build/unittest-expat; fi
	@if [ ! -L build/unittest-expat/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-expat; fi
	@if [ ! -L build/unittest-expat/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml-expat/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest-expat; fi
	make -f Makefile.build BUILDDIR=build/unittest-expat

# unittest (C), libhdkxml-static (C, expat)
.PHONY: unittest-static-expat
unittest-static-expat:
	@if [ ! -d build/unittest-static-expat ]; then mkdir -p build/unittest-static-expat; fi
	@if [ ! -L build/unittest-static-expat/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static-expat; fi
	@if [ ! -L build/unittest-static-expat/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml-expat/libhdkxml.a) build/unittest-static-expat; fi
	make -f Makefile.build BUILDDIR=build/unittest-static-expat UNITTEST_LDFLAGS="-lexpat"

# unittest (C), libhdkxml (C, libxml2)
.PHONY: unittest-libxml2
unittest-libxml2:
	@if [ ! -d build/unittest-libxml2 ]; then mkdir -p build/unittest-libxml2; fi
	@if [ ! -L build/unittest-libxml2/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-libxml2; fi
	@if [ ! -L build/unittest-libxml2/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml-libxml2/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest-libxml2; fi
	make -f Makefile.build BUILDDIR=build/unittest-libxml2

# unittest (C), libhdkxml static (C, libxml2)
.PHONY: unittest-static-libxml2
unittest-static-libxml2:
	@if [ ! -d build/unittest-static-libxml2 ]; then mkdir -p build/unittest-static-libxml2; fi
	@if [ ! -L build/unittest-static-libxml2/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static-libxml2; fi
	@if [ ! -L build/unittest-static-libxml2/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml-libxml2/libhdkxml.a) build/unittest-static-libxml2; fi
	make -f Makefile.build BUILDDIR=build/unittest-static-libxml2 UNITTEST_LDFLAGS="-lxml2"

# unittest (C++), libhdkxml (C, expat)
.PHONY: unittest-expat-c++
unittest-expat-c++:
	@if [ ! -d build/unittest-expat-c++ ]; then mkdir -p build/unittest-expat-c++; fi
	@if [ ! -L build/unittest-expat-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-expat-c++; fi
	@if [ ! -L build/unittest-expat-c++/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml-expat/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest-expat-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-expat-c++ UNITTEST_CXX=1

# unittest (C++), libhdkxml static (C, expat)
.PHONY: unittest-static-expat-c++
unittest-static-expat-c++:
	@if [ ! -d build/unittest-static-expat-c++ ]; then mkdir -p build/unittest-static-expat-c++; fi
	@if [ ! -L build/unittest-static-expat-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static-expat-c++; fi
	@if [ ! -L build/unittest-static-expat-c++/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml-expat/libhdkxml.a) build/unittest-static-expat-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-static-expat-c++ UNITTEST_CXX=1 UNITTEST_LDFLAGS="-lexpat"

# unittest (C++), libhdkxml (C, libxml2)
.PHONY: unittest-libxml2-c++
unittest-libxml2-c++:
	@if [ ! -d build/unittest-libxml2-c++ ]; then mkdir -p build/unittest-libxml2-c++; fi
	@if [ ! -L build/unittest-libxml2-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-libxml2-c++; fi
	@if [ ! -L build/unittest-libxml2-c++/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/libhdkxml-libxml2/libhdkxml.$(PLATFORM_LIBEXT)) build/unittest-libxml2-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-libxml2-c++ UNITTEST_CXX=1

# unittest (C++), libhdkxml static (C, libxml2)
.PHONY: unittest-static-libxml2-c++
unittest-static-libxml2-c++:
	@if [ ! -d build/unittest-static-libxml2-c++ ]; then mkdir -p build/unittest-static-libxml2-c++; fi
	@if [ ! -L build/unittest-static-libxml2-c++/malloc_interposer.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath build/malloc_interposer/malloc_interposer.$(PLATFORM_LIBEXT)) build/unittest-static-libxml2-c++; fi
	@if [ ! -L build/unittest-static-libxml2-c++/libhdkxml.a ]; then ln -s $(abspath build/libhdkxml-libxml2/libhdkxml.a) build/unittest-static-libxml2-c++; fi
	make -f Makefile.build BUILDDIR=build/unittest-static-libxml2-c++ UNITTEST_CXX=1 UNITTEST_LDFLAGS="-lxml2"
