AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lutapi -lutctx -lulog -lsyscfg -lsysevent

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

bin_PROGRAMS = utcmd
utcmd_CPPFLAGS = -I$(top_srcdir)/source/include $(CPPFLAGS)
utcmd_SOURCES = utcmd.c
utcmd_LDFLAGS =  -L$(top_srcdir)/lib -lutapi -lutctx -lulog -lsyscfg -lsysevent
