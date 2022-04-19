##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#                                              -*- Autoconf -*-
# Process this file with autoconf to produce configure script.
#

AC_PREREQ([2.65])
AC_INIT([Utopia], [1.0], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE
LT_INIT
GTEST_ENABLE_FLAG = ""

AC_ARG_ENABLE([gtestapp],
             AS_HELP_STRING([--enable-gtestapp],[enable Gtest support (default is no)]),
             [
               case "${enableval}" in
                yes) GTEST_SUPPORT_ENABLED=true
                     GTEST_ENABLE_FLAG="-DGTEST_ENABLE"
                     m4_if(m4_sysval,[0],[AC_CONFIG_FILES([source/test/Makefile])]);;
                no) GTEST_SUPPORT_ENABLED=false AC_MSG_ERROR([Gtest support is disabled]);;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-gtestapp ]);;
               esac
             ],
             [echo "Gtestapp is disabled"])
AM_CONDITIONAL([WITH_GTEST_SUPPORT], [test x$GTEST_SUPPORT_ENABLED = xtrue])
AC_PREFIX_DEFAULT(`pwd`)
AC_ENABLE_SHARED
AC_DISABLE_STATIC

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AM_PROG_CC_C_O
AM_PROG_LIBTOOL(libtool)

AM_CONDITIONAL([PLATFORM_HAL_SYNDICATION], [test "$PLATFORM_HAL_SYNDICATION" = "yes"])

# Specify ccsp platform (device)
AC_ARG_WITH([ccsp-platform],
[AC_HELP_STRING([--with-ccsp-platform={intel_usg,pc,bcm,intel_puma7}],
                [specify the ccsp platform])],
[case x"$withval" in
   xintel_usg)
     CCSP_PLATFORM=intel_usg
     ;;
   xintel_puma7)
     CCSP_PLATFORM=intel_puma7
     ;;
   xpc)
     CCSP_PLATFORM=pc
     ;;
   xbcm)
     CCSP_PLATFORM=bcm
     ;;
   *)
     AC_MSG_ERROR([$withval is an invalid option to --with-ccsp-platform])
     ;;
 esac],
[CCSP_PLATFORM=''])
if test x"${CCSP_PLATFORM}" != x; then
  AC_DEFINE_UNQUOTED(CCSP_PLATFORM, "$CCSP_PLATFORM",
                     [The CCSP platform device])
fi

AM_CONDITIONAL(CCSP_PLATFORM_BCM, test "x$CCSP_PLATFORM" = xbcm)
AM_CONDITIONAL(CCSP_PLATFORM_INTEL_PUMA7, test "x$CCSP_PLATFORM" = xintel_puma7)
AM_CONDITIONAL([MULTILAN_FEATURE], [test "$MULTILAN_FEATURE" = "yes"])
AM_CONDITIONAL([CPC_FIREWALL_ENABLE], [test "x$CPC_FIREWALL_ENABLE" = "xtrue"])
# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC

AX_PTHREAD

PKG_CHECK_MODULES([DBUS],[dbus-1 >= 1.6.18])

# Checks for DSLite Feature Support
AC_ARG_ENABLE([dslite_feature_support],
[  --enable-dslite_feature_support=val Turn on DSLite Feature, val=true or false],
[case "${enableval}" in
  yes) dslite_feature_support=true ;;
  no)  dslite_feature_support=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-dslite_feature_support]) ;;
esac],[dslite_feature_support=true])
AM_CONDITIONAL(DSLITE_FEATURE_SUPPORT, test x"$dslite_feature_support" = x"true")

AM_COND_IF( [DSLITE_FEATURE_SUPPORT], [
AC_CONFIG_FILES([
   source/service_dslite/Makefile
])] )

AC_CONFIG_FILES(
	Makefile
	source/Makefile
	source/dhcpproxy/Makefile
	source/firewall/Makefile
	source/firewall_log/Makefile
	source/igd/Makefile
	source/igd/src/Makefile
	source/macclone/Makefile
	source/newhost/Makefile
	source/pal/Makefile
	source/pmon/Makefile
	source/scripts/Makefile
	source/scripts/init/Makefile
	source/scripts/init/c_registration/Makefile
	source/scripts/init/src/Makefile
	source/scripts/init/src/apply_system_defaults/Makefile
	source/scripts/init/src/execute_dir/Makefile
	source/services/Makefile
	source/services/lib/Makefile
	source/service_dhcp/Makefile
	source/service_ipv6/Makefile
	source/service_multinet/Makefile
	source/service_routed/Makefile
	source/service_udhcpc/Makefile
	source/service_wan/Makefile
	source/syscfg/Makefile
	source/syscfg/cmd/Makefile
	source/syscfg/lib/Makefile
	source/sysevent/Makefile
	source/sysevent/control/Makefile
	source/sysevent/fork_helper/Makefile
	source/sysevent/lib/Makefile
	source/sysevent/proxy/Makefile
	source/sysevent/server/Makefile
	source/trigger/Makefile
	source/ulog/Makefile
	source/utapi/Makefile
	source/utapi/cmd/Makefile
	source/utapi/lib/Makefile
	source/utctx/Makefile
	source/utctx/bin/Makefile
	source/utctx/lib/Makefile
	source/util/Makefile
	source/util/print_uptime/Makefile
	source/util/rpc/Makefile
	source/util/rpc/client/Makefile
	source/util/rpc/server/Makefile
	source/util/utils/Makefile
)

AC_SUBST(GTEST_ENABLE_FLAG)
AC_OUTPUT
