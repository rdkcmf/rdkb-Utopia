AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lpthread
AM_LDFLAGS += -lz
AM_LDFLAGS += -lsrvmgr
AM_LDFLAGS += -lsyscfg
AM_LDFLAGS += -lsysevent
AM_LDFLAGS += -lulog

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

noinst_LTLIBRARIES = libUtopia_hotspot.la
libUtopia_hotspot_la_CPPFLAGS = -I$(top_srcdir)/source/cosa/include/linux -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/source/igd/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/arm_shared/include -I$(top_srcdir)/source/hdk2/hdk2/include -I$(top_srcdir)/source/pal/upnp/include -I$(top_srcdir)/source/pal/xml/include -I$(top_srcdir)/source/pal/kernel/include -I$(top_srcdir)/source/pal/include -I$(top_srcdir)/source/pal/log/include $(CPPFLAGS)
libUtopia_hotspot_la_SOURCES = hotspot_methods.c
libUtopia_hotspot_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz -lsrvmgr -lsyscfg -lsysevent -lulog