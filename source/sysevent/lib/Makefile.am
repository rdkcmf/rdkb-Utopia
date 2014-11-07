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

lib_LTLIBRARIES = libsysevent.la
libsysevent_la_CPPFLAGS = -I$(top_srcdir)/source/include $(CPPFLAGS)
libsysevent_la_SOURCES = libsysevent.c
libsysevent_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz
