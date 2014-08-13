SUBDIRS = source

lib_LTLIBRARIES=libccsp_common.la
libccsp_common_la_CPPFLAGS= -D_ANSC_LINUX -D_ANSC_USER -D_ANSC_LITTLE_ENDIAN_ -D_COSA_SIM_ -I$(top_srcdir)/source/cosa/include/linux -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/source/igd/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/rdkb_arm/include -I$(top_srcdir)/source/firewall/arch/intel_usg/boards/arm_shared/include -I$(top_srcdir)/source/hdk2/hdk2/include -I$(top_srcdir)/source/pal/upnp/include -I$(top_srcdir)/source/pal/xml/include -I$(top_srcdir)/source/pal/kernel/include -I$(top_srcdir)/source/pal/include -I$(top_srcdir)/source/pal/log/include
libccsp_common_la_LDFLAGS=
libccsp_common_la_SOURCES=
libccsp_common_la_DEPENDENCIES= \
	source/igd/src/libUtopia_src.la \
	source/firewall_log/libUtopia_firewall_log.la \
	source/utctx/bin/libUtopia_bin.la \
	source/utctx/lib/libUtopia_lib.la \
	source/utctx/sample/libUtopia_sample.la \
	source/macclone/libUtopia_macclone.la \
	source/trigger/libUtopia_trigger.la \
	source/firewall/arch/intel_usg/boards/rdkb_arm/source/libUtopia_source.la \
	source/firewall/libUtopia_firewall.la \
	source/syscfg/cmd/libUtopia_cmd.la \
	source/syscfg/lib/libUtopia_lib.la \
	source/wmon/libUtopia_wmon.la \
	source/ulog/libUtopia_ulog.la \
	source/dhcpproxy/libUtopia_dhcpproxy.la \
	source/pmon/libUtopia_pmon.la \
	source/utapi/cmd/libUtopia_cmd.la \
	source/utapi/lib/libUtopia_lib.la \
	source/sysevent/fork_helper/libUtopia_fork_helper.la \
	source/sysevent/lib/libUtopia_lib.la \
	source/sysevent/server/libUtopia_server.la \
	source/sysevent/proxy/libUtopia_proxy.la \
	source/sysevent/control/libUtopia_control.la \
	source/scripts/init/c_registration/libUtopia_c_registration.la \
	source/scripts/init/src/execute_dir/libUtopia_execute_dir.la \
	source/scripts/init/src/apply_system_defaults/libUtopia_apply_system_defaults.la \
	source/scripts/init/service.d/libUtopia_service_d.la \
	source/scripts/init/syslog_conf/libUtopia_syslog_conf.la \
	source/hdk2/hdk2-modules/he-auth/libUtopia_he-auth.la \
	source/hdk2/hdk2-modules/he-cli/libUtopia_he-cli.la \
	source/hdk2/hdk2-modules/hotspot/libUtopia_hotspot.la \
	source/hdk2/hdk2/libhdkxml/libUtopia_libhdkxml.la \
	source/hdk2/hdk2/libhdksrv/libUtopia_libhdksrv.la \
	source/hdk2/hdk2/sample/hnapd/libUtopia_hnapd.la \
	source/hdk2/hdk2/sample/dom/libUtopia_dom.la \
	source/hdk2/hdk2/sample/client/libUtopia_client.la \
	source/hdk2/hdk2/sample/module/libUtopia_module.la \
	source/hdk2/hdk2/sample/cgi/libUtopia_cgi.la \
	source/hdk2/hdk2/libhdkcli/libUtopia_libhdkcli.la \
	source/hdk2/hdk2/libhnap12/libUtopia_libhnap12.la \
	source/hnap/cgi/libUtopia_cgi.la \
	source/hnap/hdkadi/libUtopia_hdkadi.la \
	source/pal/upnp/src/libUtopia_src.la \
	source/pal/xml/src/libUtopia_src.la \
	source/pal/kernel/src/libUtopia_src.la \
	source/pal/log/src/libUtopia_src.la \
	source/newhost/libUtopia_newhost.la \
	source/services/lib/libUtopia_lib.la

libccsp_common_la_LIBADD=$(libccsp_common_la_DEPENDENCIES)
