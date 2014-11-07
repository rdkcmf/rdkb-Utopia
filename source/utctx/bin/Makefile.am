AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lsyscfg -lsysevent -lutctx -lulog -lpthread

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

bin_PROGRAMS = utctx_cmd
utctx_cmd_CPPFLAGS = -I$(top_srcdir)/source/include -I$(top_srcdir)/source/include/utctx $(CPPFLAGS)
utctx_cmd_SOURCES = utctx_cmd.c
utctx_cmd_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lsyscfg -lsysevent -L$(top_srcdir)/lib -lutctx -lulog -lpthread
