#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2009 by Cisco Systems, Inc. All Rights Reserved.
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
#
# This code brings up the wan for the wan protocol static
# The script assumes that ip connectivity is availiable with the isp
#
# All wan protocols must set the following sysevent tuples
#   current_wan_ifname
#   current_wan_ipaddr
#   current_wan_subnet
#   current_wan_state
#   wan-status
#   current_ipv4_wan_state
#   /proc/sys/net/ipv4/ip_forward
#
# The script is called with one parameter:
#   The value of the parameter is link_change if the ipv4 link state has changed
#   and it is desired_state_change if the desired_ipv4_wan_state has changed
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh

DESIRED_WAN_STATE=`sysevent get desired_ipv4_wan_state`
CURRENT_WAN_STATE=`sysevent get current_ipv4_wan_state`
CURRENT_LINK_STATE=`sysevent get current_ipv4_link_state`
PID="($$)"

bring_wan_down() {
   echo 0 > /proc/sys/net/ipv4/ip_forward
   sysevent set current_wan_ipaddr 0.0.0.0
   sysevent set current_wan_subnet 0.0.0.0
   sysevent set firewall-restart
   ulog static_wan status "$PID setting current_wan_state down"
   sysevent set current_ipv4_wan_state down
   sysevent set current_wan_state down
   sysevent set wan-status stopped
}

bring_wan_up() {
   # sysevent wan_ifname contains the normal wan interface name
   WAN_IFNAME=`sysevent get wan_ifname`
   sysevent set current_wan_ifname $WAN_IFNAME

   SUBNET=`sysevent get ipv4_wan_subnet`
   if [ -n "$SUBNET" ] ; then
      sysevent set current_wan_subnet $SUBNET
   else
      sysevent set current_wan_subnet 255.255.255.0
   fi

   IP=`sysevent get ipv4_wan_ipaddr`
   if [ -n "$IP" ] ; then
      sysevent set current_wan_ipaddr $IP
   else
      sysevent set current_wan_ipaddr 0.0.0.0
   fi

   sysevent set firewall-restart
   echo 1 > /proc/sys/net/ipv4/ip_forward
   ulog static_wan status "$PID setting current_wan_state up"
   sysevent set current_ipv4_wan_state up
   sysevent set current_wan_state up
   sysevent set wan-status started
   sysevent set wan_start_time `cat /proc/uptime | cut -d'.' -f1`
}

# --------------------------------------------------------
# we need to react to two events:
#   desired_ipv4_wan_state - up | down
#   current_ipv4_link_state - up | down
# --------------------------------------------------------

case "$1" in
   current_ipv4_link_state)
      ulog static_wan status "$PID ipv4 link state is $CURRENT_LINK_STATE"
      if [ "up" != "$CURRENT_LINK_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog static_wan status "$PID ipv4 link is down. Tearing down wan"
            bring_wan_down
            exit 0
         else
            ulog static_wan status "$PID ipv4 link is down. Wan is already down"
            exit 0
         fi
      else
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog static_wan status "$PID ipv4 link is up. Wan is already up"
            exit 0
         else
            if [ "up" = "$DESIRED_WAN_STATE" ] ; then
               bring_wan_up
               exit 0
            else
               ulog static_wan status "$PID ipv4 link is up. Wan is not requested up"
               exit 0
            fi
         fi
      fi
      ;;

   desired_ipv4_wan_state)
      if [ "up" = "$DESIRED_WAN_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog static_wan status "$PID wan is already up."
            exit 0
         else
            if [ "up" != "$CURRENT_LINK_STATE" ] ; then
               ulog static_wan status "$PID wan up request deferred until link is up"
               exit 0
            else
               bring_wan_up
               exit 0
            fi
         fi
      else
         if [ "up" != "$CURRENT_WAN_STATE" ] ; then
            ulog static_wan status "$PID wan is already down."
         else
            bring_wan_down
         fi
      fi
      ;;

 *)
      ulog static_wan status "$PID Invalid parameter $1 "
      exit 3
      ;;
esac
