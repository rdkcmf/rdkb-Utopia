AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -lsysevent
AM_LDFLAGS += -lulog

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

lib_LTLIBRARIES = libsrvmgr.la
libsrvmgr_la_CPPFLAGS = -I$(top_srcdir)/source/include $(CPPFLAGS)
libsrvmgr_la_SOURCES = srvmgr.c
libsrvmgr_la_LDFLAGS = -lsysevent -lulog
