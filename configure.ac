#                                              -*- Autoconf -*-
# Process this file with autoconf to produce configure script.
#

AC_PREREQ([2.65])
AC_INIT([ccsp_common], [1.0], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE
LT_INIT

AC_PREFIX_DEFAULT(`pwd`)
AC_ENABLE_SHARED
AC_DISABLE_STATIC

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AM_PROG_LIBTOOL(libtool)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES(
	source/igd/src/Makefile
	source/igd/Makefile
	source/firewall_log/Makefile
	source/utctx/bin/Makefile
	source/utctx/lib/Makefile
	source/utctx/sample/Makefile
	source/utctx/Makefile
	source/macclone/Makefile
	source/trigger/Makefile
	source/firewall/arch/intel_usg/boards/rdkb_arm/source/Makefile
	source/firewall/arch/intel_usg/boards/rdkb_arm/Makefile
	source/firewall/arch/intel_usg/boards/Makefile
	source/firewall/arch/intel_usg/Makefile
	source/firewall/arch/Makefile
	source/firewall/Makefile
	source/syscfg/cmd/Makefile
	source/syscfg/lib/Makefile
	source/syscfg/Makefile
	source/wmon/Makefile
	source/ulog/Makefile
	source/dhcpproxy/Makefile
	source/pmon/Makefile
	source/utapi/cmd/Makefile
	source/utapi/lib/Makefile
	source/utapi/Makefile
	source/sysevent/fork_helper/Makefile
	source/sysevent/lib/Makefile
	source/sysevent/server/Makefile
	source/sysevent/proxy/Makefile
	source/sysevent/control/Makefile
	source/sysevent/Makefile
	source/scripts/init/c_registration/Makefile
	source/scripts/init/src/execute_dir/Makefile
	source/scripts/init/src/apply_system_defaults/Makefile
	source/scripts/init/src/Makefile
	source/scripts/init/service.d/Makefile
	source/scripts/init/syslog_conf/Makefile
	source/scripts/init/Makefile
	source/scripts/Makefile
	source/hdk2/hdk2-modules/he-auth/Makefile
	source/hdk2/hdk2-modules/he-cli/Makefile
	source/hdk2/hdk2-modules/hotspot/Makefile
	source/hdk2/hdk2-modules/Makefile
	source/hdk2/hdk2/libhdkxml/Makefile
	source/hdk2/hdk2/libhdksrv/Makefile
	source/hdk2/hdk2/libhdkcli/Makefile
	source/hdk2/hdk2/libhnap12/Makefile
	source/hdk2/hdk2/Makefile
	source/hdk2/Makefile
	source/hnap/cgi/Makefile
	source/hnap/hdkadi/Makefile
	source/hnap/Makefile
	source/pal/upnp/src/Makefile
	source/pal/upnp/Makefile
	source/pal/xml/src/Makefile
	source/pal/xml/Makefile
	source/pal/kernel/src/Makefile
	source/pal/kernel/Makefile
	source/pal/log/src/Makefile
	source/pal/log/Makefile
	source/pal/Makefile
	source/newhost/Makefile
	source/services/lib/Makefile
	source/services/Makefile
	source/Makefile
	Makefile
)


AC_OUTPUT

