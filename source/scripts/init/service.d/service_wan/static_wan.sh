#!/bin/sh
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

#######################################################################
#   Copyright [2014] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the \"License\");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an \"AS IS\" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#######################################################################

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
source /etc/utopia/service.d/log_capture_path.sh
source /lib/rdk/t2Shared_api.sh

DESIRED_WAN_STATE=`sysevent get desired_ipv4_wan_state`
CURRENT_WAN_STATE=`sysevent get current_ipv4_wan_state`
CURRENT_LINK_STATE=`sysevent get current_ipv4_link_state`
PID="($$)"

bring_wan_down() {
   echo 0 > /proc/sys/net/ipv4/ip_forward
   sysevent set current_wan_ipaddr 0.0.0.0
   sysevent set current_wan_subnet 0.0.0.0
   echo "static_wan: Triggering RDKB_FIREWALL_RESTART from WAN down"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
   ulog static_wan status "$PID setting current_wan_state down"
   sysevent set current_ipv4_wan_state down
   sysevent set current_wan_state down
   sysevent set wan-status stopped
}

bring_wan_up() {
   # sysevent wan_ifname contains the normal wan interface name
   WAN_IFNAME=`sysevent get wan_ifname`
   sysevent set current_wan_ifname "$WAN_IFNAME"

   SUBNET=`sysevent get ipv4_wan_subnet`
   if [ -n "$SUBNET" ] ; then
      sysevent set current_wan_subnet "$SUBNET"
   else
      sysevent set current_wan_subnet 255.255.255.0
   fi

   IP=`sysevent get ipv4_wan_ipaddr`
   if [ -n "$IP" ] ; then
      sysevent set current_wan_ipaddr "$IP"
   else
      sysevent set current_wan_ipaddr 0.0.0.0
   fi

   echo "static_wan: Triggering RDKB_FIREWALL_RESTART from WAN up"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
   echo 1 > /proc/sys/net/ipv4/ip_forward
   ulog static_wan status "$PID setting current_wan_state up"
   sysevent set current_ipv4_wan_state up
   sysevent set current_wan_state up
   sysevent set wan-status started
   sysevent set wan_start_time $(cut -d. -f1 /proc/uptime)
   #start ntp time sync
   if [ x"1" = x"`syscfg get ntp_enabled`" ] ; then
       dmcli eRT setv Device.Time.Enable bool 1 &
   fi
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
