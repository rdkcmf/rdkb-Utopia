#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2008,2010 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------


#------------------------------------------------------------------
# This script sets up the wan handlers.
# Upon completion of bringing up the appropriate wan handler bringing up the wan,
# it is expected that the wan handler will set the wan-status up.
#
# All wan protocols must set the following sysevent tuples
#   current_wan_ifname
#   current_wan_ipaddr
#   current_wan_subnet
#   current_wan_state
#   wan-status
#
# Wan protocols are responsible for setting up:
#   /etc/resolv.conf
# Wan protocols are also responsible for restarting the dhcp server if
# the wan domain or dns servers change
#------------------------------------------------------------------
#--------------------------------------------------------------
#
# This script set the handlers for 2 events:
#    desired_ipv4_link_state
#    desired_ipv4_wan_state
# and then as appropriate sets these events.
# The desired_ipv4_link_state handler establishes ipv4 connectivity to
# the isp (currently dhcp or static).
# The desired_ipv4_wan_state handler uses the connectivity to the isp to
# set up a connection that is used as the ipv4 wan. This connection
# may be dhcp, static, pppoe, l2tp, pptp or in the future other protocols
#--------------------------------------------------------------

echo "---------------------------------------------------------"
echo "-------------------- service_wan.sh ---------------------"
echo "---------------------------------------------------------"
echo ">>>>> $1"
set -x

SERVICE_NAME=wan

case "$1" in
    ${SERVICE_NAME}-start)
        service_wan start
        ;;
    ${SERVICE_NAME}-stop)
        service_wan stop
        ;;
    ${SERVICE_NAME}-restart)
        service_wan restart
        ;;
    phylink_wan_state)
        status=`sysevent get phylink_wan_state`
        if [ "up" == "$status" ]; then
            service_wan start
        elif [ "down" == "$status" ]; then
            service_wan stop
        fi
        ;;
    erouter_mode-updated)
        service_wan restart
        ;;

    dhcp_client-restart)
        service_wan dhcp-restart
        ;;

    dhcp_client-release)
        service_wan dhcp-release
        ;;

    dhcp_client-renew)
        service_wan dhcp-renew
        ;;
    *)
        echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
        exit 3
        ;;
esac
