SUBDIRS = arch
bin_PROGRAMS = firewall
firewall_CPPFLAGS = -I$(top_srcdir)/source/igd/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/arm_shared/include -I$(top_srcdir)/source/include $(CPPFLAGS)
firewall_SOURCES = raw_socket_send.c nfq_handler.c firewall.c
firewall_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz