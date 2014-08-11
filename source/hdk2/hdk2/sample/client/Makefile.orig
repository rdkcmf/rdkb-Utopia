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
BUILDDIR_LIBHDKCLI = $(HDKDIR)/build/libhdkcli
endif

# Build directory
ifndef BUILDDIR
BUILDDIR = ../../build/sample/client
endif

# Platform-specific
ifeq ($(shell uname -s), Darwin) # Mac OS X
PLATFORM_CFLAGS =
PLATFORM_LDFLAGS =
PLATFORM_LIBEXT = dylib
else
PLATFORM_CFLAGS = -pedantic-errors
PLATFORM_LDFLAGS =
PLATFORM_LIBEXT = so
endif

# HSL files
HSL = \
	$(HDKDIR)/spec/core.hsl \
	$(HDKDIR)/spec/router.hsl

# HSL flags
HSL_FLAGS = -I $(HDKDIR)/spec

HSL_SCHEMA = hdk_client_schema
HSL_CPP = hdk_client

# Targets
EXE = $(BUILDDIR)/client

# Object files
OBJS_BASE = \
	hdk_client_schema.o \
	hdk_client.o \
    client.o
OBJS = $(OBJS_BASE:%=$(BUILDDIR)/%)

# HSL-generated source files
HSL_SCHEMA_SRC_BASE = \
	$(HSL_SCHEMA).h \
	$(HSL_SCHEMA).c
HSL_SCHEMA_SRC = $(HSL_SCHEMA_SRC_BASE:%=$(BUILDDIR)/%)

HSL_CPP_SRC_BASE = \
	$(HSL_CPP).h \
	$(HSL_CPP).cpp
HSL_CPP_SRC = $(HSL_CPP_SRC_BASE:%=$(BUILDDIR)/%)

# Compiler flags. Note that exceptions are disabled so we can safely link with the static libgcc
CLIENT_CC = $(CXX)
CLIENT_CFLAGS = $(CXXFLAGS)
CLIENT_CFLAGS += -Wall -Wextra -Werror -Wno-long-long -fno-exceptions $(PLATFORM_CFLAGS)
CLIENT_CFLAGS += -I$(abspath $(HDKDIR)/include) -I$(abspath $(BUILDDIR))
ifdef DEBUG
CLIENT_CFLAGS += -g
else
CLIENT_CFLAGS += -Os
endif

ifndef MODULE_NOID
MODULE_NOID = aba86406-9b84-4eee-9250-82f911a1b891
endif
MODULE_FRIENDLY_NAME = "Sample Router"

# Linker flags. Note the static linking with libgcc
LDFLAGS += -static-libgcc -L$(BUILDDIR) $(PLATFORM_LDFLAGS) -L$(BUILDDIR_LIBHDKCLI) -lhdkcli -L$(BUILDDIR_LIBHDKXML) -lhdkxml -lcurl

# Prerequisite search path
vpath %.c $(BUILDDIR)
vpath %.cpp $(BUILDDIR)

# Build target (default)
.PHONY: build
build: $(EXE)

# Clean the build directory
.PHONY: clean
clean:
	rm -r -f $(BUILDDIR)

# Rule for linking the executable.  A symbolic link is created in the build dir for the static stdc++ library so the linker will link to that version.
$(EXE): $(OBJS)
	@if [ ! -L $(BUILDDIR)/libstdc++.a ]; then ln -s `$(CXX) -print-file-name=libstdc++.a` $(BUILDDIR); fi
	$(CLIENT_CC) -o $(EXE) $^ $(LDFLAGS)
ifndef DEBUG
	-strip $(EXE)
endif
	@ls -l $(EXE)
	@echo
	@if [ ! -L $(BUILDDIR)/libhdkxml.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath $(BUILDDIR_LIBHDKXML)/libhdkxml.$(PLATFORM_LIBEXT)) $(BUILDDIR); fi
	@if [ ! -L $(BUILDDIR)/libhdkcli.$(PLATFORM_LIBEXT) ]; then ln -s $(abspath $(BUILDDIR_LIBHDKCLI)/libhdkcli.$(PLATFORM_LIBEXT)) $(BUILDDIR); fi

# HSL code generation
$(HSL_SCHEMA_SRC): $(HSL)
	python $(HDKDIR)/bin/hdkmod.py $(HSL_FLAGS) --client -o $(BUILDDIR) -b $(HSL_SCHEMA) --noid $(MODULE_NOID) --friendly-name $(MODULE_FRIENDLY_NAME) $^

$(HSL_CPP_SRC): $(HSL)
	python $(HDKDIR)/bin/hdkcli_cpp.py $(HSL_FLAGS) --module-name $(HSL_SCHEMA) -o $(BUILDDIR) -b $(HSL_CPP) $^

# Rule for compiling source files
$(BUILDDIR)/%.o: %.cpp
	$(CLIENT_CC) $(CPPFLAGS) $(CLIENT_CFLAGS) -o $@ -c $<

$(BUILDDIR)/%.o: $(BUILDDIR)/%.c
	$(CLIENT_CC) $(CPPFLAGS) $(CLIENT_CFLAGS) -o $@ -c $<

$(BUILDDIR)/%.o: $(BUILDDIR)/%.cpp
	$(CLIENT_CC) $(CPPFLAGS) $(CLIENT_CFLAGS) -o $@ -c $<

# Rule for building source dependency files
$(BUILDDIR)/.%.depend: %.cpp %.c
	@mkdir -p $(BUILDDIR)
	@$(CLIENT_CC) -MM $(CPPFLAGS) $(CLIENT_CFLAGS) $< > $@

# Include dependency files
-include $(OBJS_BASE:%.o=$(BUILDDIR)/.%.depend)
