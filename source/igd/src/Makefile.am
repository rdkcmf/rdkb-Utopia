AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_CFLAGS += -D_LINUX -pipe
AM_CFLAGS += -DPAL_ULOG
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib
AM_LDFLAGS += -lpthread
AM_LDFLAGS += -lz
AM_LDFLAGS += -lsrvmgr
AM_LDFLAGS += -lsyscfg
AM_LDFLAGS += -lsysevent
AM_LDFLAGS += -lulog
AM_LDFLAGS += -lpal -lupnp -lixml -lthreadutil
AM_LDFLAGS += -Bdynamic -lutctx -lutapi
AM_LDFLAGS += -lnvramstorage -lticc -lutils_docsis

AM_CPPFLAGS = -Wall -Wextra -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

bin_PROGRAMS = IGD 
IGD_CPPFLAGS = -I$(top_srcdir)/source/cosa/include/linux -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/source/igd/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/arm_shared/include -I$(top_srcdir)/source/hdk2/hdk2/include -I$(top_srcdir)/source/pal/upnp/include -I$(top_srcdir)/source/pal/xml/include -I$(top_srcdir)/source/pal/kernel/include -I$(top_srcdir)/source/pal/include -I$(top_srcdir)/source/pal/log/include $(CPPFLAGS) -I$(top_srcdir)/source/igd/src/inc -I$(top_srcdir)/source/include
IGD_SOURCES = igd_service_wan_ethlink_cfg.c igd_device_root.c igd_utility.c igd_device_lan.c igd_service_wan_commif_cfg.c igd_action_port_mapping.c igd_device_wan_connect.c igd_device_wan.c igd_platform_independent_inf.c igd_service_lanhostconfigmgmt.c igd_service_wan_connect.c igd_service_layer3_fwd.c
IGD_LDFLAGS =  -L/var/tmp/pc-rdkb/lib -lpthread -lz -lsrvmgr -lsyscfg -lsysevent -lulog -lpal -lupnp -lixml -lthreadutil -Bdynamic -lutctx -lutapi -lnvramstorage -lticc -lutils_docsis
