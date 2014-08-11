#!/bin/sh
/fss/gw/usr/sbin/GenFWLog -c
/fss/gw/usr/sbin/firewall $*
/fss/gw/usr/sbin/GenFWLog -gc

CONN_F=`sysevent get firewall_flush_conntrack`
WANIP=`sysevent get current_wan_ipaddr`
if [ "$CONN_F" == "1" ] && [ "$WANIP" != "0.0.0.0" ]
then
    conntrack_flush
    sysevent set firewall_flush_conntrack 0
fi



