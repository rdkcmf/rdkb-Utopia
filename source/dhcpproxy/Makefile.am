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
bin_PROGRAMS = dhcp_proxy
dhcp_proxy_CPPFLAGS = -I$(top_srcdir)/source/igd/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/arm_shared/include -I$(top_srcdir)/source/include $(CPPFLAGS)
dhcp_proxy_SOURCES = packet_util.c dhcp_proxy.c dhcp_util.c dhcp_msg.c
dhcp_proxy_LDFLAGS = -L/var/tmp/pc-rdkb/lib -lpthread -lz -lsrvmgr -lsyscfg -lsysevent -lulog
