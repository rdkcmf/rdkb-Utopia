#!/bin/sh
echo "*******************************************************************"
echo "*                                                                  "
echo "* Copyright (c) 2010 by Cisco Systems, Inc. All Rights Reserved.   "
echo "*                                                                  "
echo "*******************************************************************"
cd /etc/utopia/service.d
UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`
#. /etc/config
LAN_ST=`sysevent get lan-status`
#WAN_ST=`sysevent get wan-status`
BR_MD=`sysevent get bridge_mode`
RG_MD=`syscfg get last_erouter_mode`

/etc/utopia/service.d/cosa_misc.sh&
/etc/utopia/service.d/service_igd.sh snmp_subagent-status
#unit in bridge mode
if [ "$BR_MD" != "0" -o "$RG_MD" = "0" ]; then
    execute_dir /etc/utopia/post.d/
	firewall
	exit 0;
fi

#when no cable plug in, timeout happened, we still need register the sysevent
if [ "$UPTIME" -gt 600 ]; then
    execute_dir /etc/utopia/post.d/
fi

if [ "$LAN_ST" != "started" ]; then
	exit 0;
fi

if [ "$RG_MD" = "1" -o "$RG_MD" = "3" ]; then
	/etc/utopia/service.d/service_igd.sh lan-status&
fi

if [ "$RG_MD" = "1" -o "$RG_MD" = "3" ]; then
	/etc/utopia/service.d/service_mcastproxy.sh lan-status&
fi

if [ "$RG_MD" = "2" -o "$RG_MD" = "3" ]; then
	/etc/utopia/service.d/service_mldproxy.sh lan-status&
fi

#start firewall first
#firewall $*
#force lan client to start DHCP discover now
#gw_lan_refresh
#start others
#if [ "started" = "$LAN_ST" ] ; then
#	/etc/utopia/service.d/service_igd.sh snmp_subagent-status
#	/etc/utopia/service.d/service_igd.sh lan-status&
	
#	if [ "y" = "$CONFIG_CISCO_WECB" ] ; then
#	    /etc/utopia/service.d/service_wecb.sh snmp_subagent-status&
#	    /etc/utopia/service.d/service_wecb.sh wecb-start&
#	fi
#fi
#BR_MODE=`syscfg get bridge_mode`
#EROUTER_MODE=`syscfg get last_erouter_mode`
#if [ 0 = "$BR_MODE" ] && [ 0 != "$EROUTER_MODE" ]  && [ 1 != "$EROUTER_MODE" ]; then
#    /etc/utopia/service.d/service_mldproxy.sh wan-status&
#fi

#start igmpproxy
#if [ 0 = "$BR_MODE" ] && [ 0 != "$EROUTER_MODE" ]  && [ 2 != "$EROUTER_MODE" ]; then
#    /etc/utopia/service.d/service_mcastproxy.sh wan-status&
#fi
