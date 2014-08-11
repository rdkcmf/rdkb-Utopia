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
HDKDIR = ../..

# HDK library directories
ifndef BUILDDIR_LIBHDKXML
BUILDDIR_LIBHDKXML = $(HDKDIR)/build/libhdkxml
endif
ifndef BUILDDIR_LIBHDKSRV
BUILDDIR_LIBHDKSRV = $(HDKDIR)/build/libhdksrv
endif

# Build directory
ifndef BUILDDIR
BUILDDIR = ../../build/sample/hnapd
endif

# Platform-specific
ifeq ($(shell uname -s), Darwin) # Mac OS X
PLATFORM_LIBEXT = dylib
else
PLATFORM_LIBEXT = so
endif

# Targets
EXE = $(BUILDDIR)/hnapd

# Object files
OBJS_BASE = \
	hnapd.o \
	hnapd_context.o
OBJS = $(OBJS_BASE:%=$(BUILDDIR)/%)

# Build platform
OS := $(shell uname -s)

# Compiler flags
ifdef HNAPD_CXX
HNAPD_CC = $(CXX)
HNAPD_CFLAGS = $(CXXFLAGS)
else
HNAPD_CC = $(CC)
HNAPD_CFLAGS = $(CFLAGS) -Wmissing-prototypes
endif
HNAPD_CFLAGS += -Wall -Wextra -Werror -Wno-long-long
ifneq ($(OS), Darwin)
HNAPD_CFLAGS += -pedantic-errors
endif
HNAPD_CFLAGS += -I$(abspath .) -I$(abspath $(HDKDIR)/include)
ifdef DEBUG
HNAPD_CFLAGS += -g
else
HNAPD_CFLAGS += -Os
endif

# Linker flags
LDFLAGS += -L$(BUILDDIR_LIBHDKXML) -lhdkxml -L$(BUILDDIR_LIBHDKSRV) -lhdksrv -shared-libgcc

# Build target (default)
.PHONY: build
build: $(EXE)

# Clean the build directory
.PHONY: clean
clean:
	rm -r -f $(BUILDDIR)

# Rule for linking the executable and linking in required library dependancies
$(EXE): $(OBJS)
	$(HNAPD_CC) -o $(EXE) $^ $(LDFLAGS)
ifndef DEBUG
	-strip $(EXE)
endif
	@ls -l $(EXE)
	@echo
	@if [ ! -L $(BUILDDIR)/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath $(BUILDDIR_LIBHDKXML)/libhdkxml.$(PLATFORM_LIBEXT)) $(BUILDDIR); fi
	@if [ ! -L $(BUILDDIR)/libhdksrv.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath $(BUILDDIR_LIBHDKSRV)/libhdksrv.$(PLATFORM_LIBEXT)) $(BUILDDIR); fi

# Rule for compiling source files
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(HNAPD_CC) -c $(CPPFLAGS) $(HNAPD_CFLAGS) -o $@ -c $<

# Rule for building source dependency files
$(BUILDDIR)/.%.depend: %.c
	@mkdir -p $(BUILDDIR)
	@$(HNAPD_CC) -MM $(CPPFLAGS) $(HNAPD_CFLAGS) $< > $@

# Include dependency files
-include $(OBJS_BASE:%.o=$(BUILDDIR)/.%.depend)
