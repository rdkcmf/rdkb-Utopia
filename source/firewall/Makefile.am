bin_PROGRAMS = firewall
firewall_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/source/include $(CPPFLAGS)
firewall_SOURCES = raw_socket_send.c nfq_handler.c firewall.c
firewall_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz
