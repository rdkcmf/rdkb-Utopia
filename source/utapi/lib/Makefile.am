AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_CFLAGS += -fPIC
AM_LDFLAGS = -L/var/tmp/pc-rdkb/lib

AM_CPPFLAGS = -Wall -Werror -fPIC
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

lib_LTLIBRARIES = libutapi.la
libutapi_la_CPPFLAGS = -I$(top_srcdir)/source/include $(CPPFLAGS)
libutapi_la_SOURCES = utapi_tr_wlan.c utapi.c utapi_util.c tr181_util.c utapi_parental_control.c tr181_wrpr.c utapi_wlan.c utapi_security.c utapi_dns.c utapi_tr_dhcp.c utapi_tr_user.c utapi_moca.c
libutapi_la_LDFLAGS = 

libutapi_ladir = $(top_builddir)/source/include/utapi
libutapi_la_HEADERS = utapi_wlan.h
