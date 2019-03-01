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

# Specify ccsp cpu arch
AC_ARG_WITH([ccsp-arch],
[AC_HELP_STRING([--with-ccsp-arch={arm,atom,pc,mips}],
                [specify the ccsp board CPU platform])],
[case x"$withval" in
   xarm)
     CCSP_ARCH=arm
     ;;
   xatom)
     CCSP_ARCH=atom
     ;;
   xpc)
     CCSP_ARCH=pc
     ;;
   xmips)
     CCSP_ARCH=mips
     ;;
   *)
     AC_MSG_ERROR([$withval is an invalid option to --with-ccsp-arch])
     ;;  
 esac],
[CCSP_ARCH=''])
if test x"${CCSP_ARCH}" != x; then
  AC_DEFINE_UNQUOTED(CCSP_ARCH, "$CCSP_ARCH",
                     [The board CPU architecture])
fi

AM_CONDITIONAL(CCSP_ARCH_ARM, test "x$CCSP_ARCH" = xarm)
AM_CONDITIONAL(CCSP_ARCH_ATOM, test "x$CCSP_ARCH" = xatom)
AM_CONDITIONAL(CCSP_ARCH_PC, test "x$CCSP_ARCH" = xpc)
AM_CONDITIONAL(CCSP_ARCH_MIPS, test "x$CCSP_ARCH" = xmips)
AM_CONDITIONAL([PLATFORM_HAL_SYNDICATION], [test $PLATFORM_HAL_SYNDICATION = yes])

# Specify ccsp platform (device)
AC_ARG_WITH([ccsp-platform],
[AC_HELP_STRING([--with-ccsp-platform={intel_usg,pc,bcm}],
                [specify the ccsp platform])],
[case x"$withval" in
   xintel_usg)
     CCSP_PLATFORM=intel_usg
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

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC

PKG_CHECK_MODULES([DBUS],[dbus-1 >= 1.6.18])

AC_CONFIG_FILES(
	source/igd/src/Makefile
	source/igd/Makefile
	source/firewall_log/Makefile
	source/utctx/bin/Makefile
	source/utctx/lib/Makefile
	source/utctx/Makefile
	source/firewall/Makefile
	source/syscfg/cmd/Makefile
	source/syscfg/lib/Makefile
	source/syscfg/Makefile
	source/ulog/Makefile
	source/newhost/Makefile
	source/macclone/Makefile
	source/dhcpproxy/Makefile
	source/utapi/cmd/Makefile
	source/utapi/lib/Makefile
	source/utapi/Makefile
	source/pmon/Makefile
	source/pal/Makefile
	source/sysevent/lib/Makefile
	source/sysevent/control/Makefile
	source/sysevent/fork_helper/Makefile
	source/sysevent/proxy/Makefile
	source/sysevent/server/Makefile
	source/sysevent/Makefile
	source/services/lib/Makefile
	source/services/Makefile
	source/service_wan/Makefile
    source/service_udhcpc/Makefile
	source/service_dhcp/Makefile
	source/service_multinet/Makefile
	source/service_routed/Makefile
        source/service_ipv6/Makefile
	source/util/Makefile
	source/util/rpc/Makefile
    source/util/rpc/client/Makefile
    source/util/rpc/server/Makefile
    source/util/utils/Makefile
	source/trigger/Makefile
        source/scripts/Makefile
        source/scripts/init/Makefile
        source/scripts/init/c_registration/Makefile
	source/scripts/init/src/Makefile
	source/scripts/init/src/apply_system_defaults/Makefile
	source/scripts/init/src/execute_dir/Makefile
	source/Makefile
	Makefile
)


AC_OUTPUT

