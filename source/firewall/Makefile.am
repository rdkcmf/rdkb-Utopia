SUBDIRS = arch

noinst_LTLIBRARIES = libUtopia_firewall.la
libUtopia_firewall_la_CPPFLAGS = -I$(top_srcdir)/source/cosa/include/linux -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/source/igd/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/arm_shared/include -I$(top_srcdir)/source/hdk2/hdk2/include -I$(top_srcdir)/source/pal/upnp/include -I$(top_srcdir)/source/pal/xml/include -I$(top_srcdir)/source/pal/kernel/include -I$(top_srcdir)/source/pal/include -I$(top_srcdir)/source/pal/log/include $(CPPFLAGS)
libUtopia_firewall_la_SOURCES = raw_socket_send.c nfq_handler.c firewall.c
libUtopia_firewall_la_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz -lsrvmgr -lsyscfg -lsysevent -lulog