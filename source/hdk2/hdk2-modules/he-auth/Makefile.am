AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_CFLAGS += -fPIC -fvisibility=hidden
AM_CFLAGS += -DHE_AUTH_BUILD
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lhdkxml -lhdksrv -shared-libgcc

AM_CPPFLAGS = -ansi -Wall -Wextra -Wno-long-long -Wmissing-prototypes
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

lib_LTLIBRARIES = libhe_auth.la
libhe_auth_la_CPPFLAGS = -I$(top_builddir)/source/cosa/include/linux -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_builddir)/source/igd/arch/intel_usg/boards/rdkb_arm/include -I$(top_builddir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include -I$(top_builddir)/source/firewall/arch/intel_usg/boards/arm_shared/include -I$(top_builddir)/source/hdk2/hdk2/include -I$(top_builddir)/source/pal/upnp/include -I$(top_builddir)/source/pal/xml/include -I$(top_builddir)/source/pal/kernel/include -I$(top_builddir)/source/pal/include -I$(top_builddir)/source/pal/log/include $(CPPFLAGS)
libhe_auth_la_SOURCES = he_auth_methods.c
libhe_auth_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lhdkxml -lhdksrv -shared-libgcc
