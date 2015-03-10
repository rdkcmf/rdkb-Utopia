AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

bin_PROGRAMS = utcmd
utcmd_CPPFLAGS = -I$(top_srcdir)/source/include -I$(top_srcdir)/source $(CPPFLAGS)
utcmd_SOURCES = utcmd.c
utcmd_LDADD = $(top_builddir)/source/utapi/lib/libutapi.la \
              $(top_builddir)/source/utctx/lib/libutctx.la
