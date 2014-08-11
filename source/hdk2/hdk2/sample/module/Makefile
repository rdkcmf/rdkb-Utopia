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

# Location of the HDK
ifndef HDKDIR
HDKDIR = ../..
endif

# HDK build directories
ifndef BUILDDIR_LIBHDKXML
BUILDDIR_LIBHDKXML = $(HDKDIR)/build/libhdkxml
endif
ifndef BUILDDIR_LIBHDKSRV
BUILDDIR_LIBHDKSRV = $(HDKDIR)/build/libhdksrv
endif

# Build directory
ifndef BUILDDIR
BUILDDIR = ../../build/sample/module
endif

# Platform-specific
ifeq ($(shell uname -s), Darwin) # Mac OS X
PLATFORM_LIBEXT = dylib
PLATFORM_CFLAGS =
PLATFORM_LDFLAGS = -dynamiclib
PLATFORM_STRIPFLAGS = -x
else
PLATFORM_LIBEXT = so
PLATFORM_CFLAGS = -pedantic-errors
PLATFORM_LDFLAGS = -shared
PLATFORM_STRIPFLAGS =
endif

# Module name
MODULE = mymodule
MODULE_UC = MYMODULE

# Targets
DYNAMIC_LIB = $(BUILDDIR)/lib$(MODULE).$(PLATFORM_LIBEXT)
STATIC_LIB = $(BUILDDIR)/lib$(MODULE).a

# HSL files
HSL = \
	spec/$(MODULE).hsl \
	state/$(MODULE)_state.hsl

# Excluded HNAP methods
HSL_EXCLUDE =

# HSL flags
HSL_FLAGS = -I $(HDKDIR)/spec $(HSL_EXCLUDE)

# Object files
OBJS_BASE = \
	$(MODULE).o \
	$(MODULE)_methods.o
OBJS = $(OBJS_BASE:%=$(BUILDDIR)/%)

######################################################################

# HSL-generated source files
HSLSRC_BASE = \
	$(MODULE).h \
	$(MODULE).c
HSLSRC = $(HSLSRC_BASE:%=$(BUILDDIR)/%)

# Compiler flags
ifdef MODULE_CXX
MODULE_CC = $(CXX)
MODULE_CFLAGS = $(CXXFLAGS)
else
MODULE_CC = $(CC)
MODULE_CFLAGS = $(CFLAGS) -Wmissing-prototypes
endif
MODULE_CFLAGS += -ansi -Wall -Wextra -Werror -Wno-long-long
MODULE_CFLAGS += -fPIC -fvisibility=hidden $(PLATFORM_CFLAGS)
MODULE_CFLAGS += -D$(MODULE_UC)_BUILD -I$(abspath .) -I$(abspath $(BUILDDIR)) -I$(abspath $(HDKDIR)/include)
ifdef DEBUG
MODULE_CFLAGS += -g
else
MODULE_CFLAGS += -Os
endif

# Linker flags
LDFLAGS += -L$(BUILDDIR_LIBHDKXML) -lhdkxml -L$(BUILDDIR_LIBHDKSRV) -lhdksrv $(PLATFORM_LDFLAGS) -shared-libgcc

# Prerequisite search path
vpath %.c $(BUILDDIR)

# Build target (default)
.PHONY: build
build: $(DYNAMIC_LIB) $(STATIC_LIB)

# Clean the build directory
.PHONY: clean
clean:
	rm -r -f $(BUILDDIR)

# Generate server method stubs
.PHONY: methods
methods:
	python $(HDKDIR)/bin/hdkmod.py --server-methods -b $(MODULE) $(HSL)

# HSL code generation
$(HSLSRC): $(HSL)
	python $(HDKDIR)/bin/hdkmod.py $(HSL_FLAGS) --adi-report -o $(BUILDDIR) -b $(MODULE) $^

# Rule for linking the executable
$(DYNAMIC_LIB): $(OBJS)
	$(MODULE_CC) -o $(DYNAMIC_LIB) $^ $(LDFLAGS)
ifndef DEBUG
	-strip $(PLATFORM_STRIPFLAGS) $(DYNAMIC_LIB)
endif
	@ls -l $(DYNAMIC_LIB)
	@echo

$(STATIC_LIB): $(OBJS)
	ar rcs $(STATIC_LIB) $^
	@ls -l $(STATIC_LIB)
	@echo

# Rule for compiling source files
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(MODULE_CC) -c $(CPPFLAGS) $(MODULE_CFLAGS) -o $@ -c $<

# Rule for building source dependency files
$(BUILDDIR)/.%.depend: %.c $(HSLSRC)
	@mkdir -p $(BUILDDIR)
	@$(MODULE_CC) -MM $(CPPFLAGS) $(MODULE_CFLAGS) $< > $@

# Include dependency files
-include $(OBJS_BASE:%.o=$(BUILDDIR)/.%.depend)
