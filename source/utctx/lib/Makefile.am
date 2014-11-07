AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_CFLAGS += -DUTCTX_LOG
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lsyscfg -lsysevent -lulog 

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

lib_LTLIBRARIES = libutctx.la
libutctx_la_CPPFLAGS = -I$(top_srcdir)/source/include -I$(top_srcdir)/source/include/sysevent $(CPPFLAGS)
libutctx_la_SOURCES = utctx.c utctx_rwlock.c
libutctx_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lsyscfg -lsysevent -lulog
