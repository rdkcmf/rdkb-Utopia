bin_PROGRAMS = NetworkMonitor
AM_CPPFLAGS = -I$(top_srcdir)/source -I$(top_srcdir)/source/include $(CPPFLAGS) $(DBUS_CFLAGS)

NetworkMonitor_SOURCES = NetworkMonitor.c
NetworkMonitor_LDFLAGS = -lccsp_common -lsecure_wrapper

NetworkMonitor_LDADD = $(top_builddir)/source/sysevent/lib/libsysevent.la 