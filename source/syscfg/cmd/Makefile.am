AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lpthread
AM_LDFLAGS += -lz

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

noinst_LTLIBRARIES = libUtopia_cmd.la
libUtopia_cmd_la_CPPFLAGS = -I$(top_srcdir)/source/include $(CPPFLAGS)
libUtopia_cmd_la_SOURCES = syscfgtest.c.unittest syscfgcmd.c
libUtopia_cmd_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz