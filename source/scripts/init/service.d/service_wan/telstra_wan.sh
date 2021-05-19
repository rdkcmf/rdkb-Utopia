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
# This code brings up the wan for the wan protocol telstra
# The script assumes that ip connectivity is availiable with the isp
#
# All wan protocols must set the following sysevent tuples
#   current_wan_ifname
#   current_wan_ipaddr
#   current_wan_subnet
#   current_wan_state
#   /proc/sys/net/ipv4/ip_forward
#
# The script is called with one parameter:
#   The value of the parameter is link_change if the ipv4 link state has changed
#   and it is desired_state_change if the desired_ipv4_wan_state has changed
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
source /etc/utopia/service.d/service_wan/ppp_helpers.sh
source /lib/rdk/t2Shared_api.sh

DESIRED_WAN_STATE=`sysevent get desired_ipv4_wan_state`
CURRENT_WAN_STATE=`sysevent get current_ipv4_wan_state`
CURRENT_LINK_STATE=`sysevent get current_ipv4_link_state`
WAN_IFNAME=`sysevent get wan_ifname`
LAN_IFNAME=`syscfg get lan_ifname`
PID="($$)"


#------------------------------------------------------------------
# prepare_telstra
#------------------------------------------------------------------
prepare_telstra() {

   echo "[utopia][telstra] Configuring telstra" > /dev/console
   TELSTRA_CONF_FILE=/tmp/bpalogin.conf
   TELSTRA_BIN=/usr/sbin/bpalogin
   echo -n > $TELSTRA_CONF_FILE
  

   # Logging can be sent to syslog or sysout.
   #logging sysout
   #/usr/bin/newaliases > /dev/null 2>&1
   $TELSTRA_BIN -c $TELSTRA_CONF_FILE
   mkdir -p /var/lock/subsys
   touch /var/lock/subsys/bpalogin
}

# -------------------------------------------------------------

bring_wan_down() {
   echo 0 > /proc/sys/net/ipv4/ip_forward
   $TELSTRA_BIN=bpalogin
   killall -9 $TELSTRA_BIN

   sysevent set pppd_current_wan_ipaddr
   sysevent set pppd_current_wan_subnet
   sysevent set pppd_current_wan_ifname
   sysevent set current_wan_ipaddr 0.0.0.0
   sysevent set current_wan_subnet 0.0.0.0
   echo "telstra_wan : Triggering RDKB_FIREWALL_RESTART"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
}

bring_wan_up() {
   # clean some of the sysevent tuples that are shared between the ip_up script and wmon
   sysevent set pppd_current_wan_ipaddr
   sysevent set pppd_current_wan_subnet
   sysevent set pppd_current_wan_ifname
   prepare_pptp
   do_start_wan_monitor
#   bringing up the wan now happens in wmon.c
#   echo 1 > /proc/sys/net/ipv4/ip_forward
   sysevent set wan_start_time $(cut -d. -f1 /proc/uptime)
}

# --------------------------------------------------------
# we need to react to three events:
#   current_ipv4_link_state - up | down
#   desired_ipv4_wan_state - up | down
#   current_ipv4_wan_state - up | down
# --------------------------------------------------------

ulog telstra_wan status "$PID current_ipv4_link_state is $CURRENT_LINK_STATE"
ulog telstra_wan status "$PID desired_ipv4_wan_state is $DESIRED_WAN_STATE"
ulog telstra_wan status "$PID current_ipv4_wan_state is $CURRENT_WAN_STATE"

case "$1" in
   current_ipv4_link_state)
      ulog telstra_wan status "$PID ipv4 link state is $CURRENT_LINK_STATE"
      if [ "up" != "$CURRENT_LINK_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog telstra_wan status "$PID ipv4 link is down. Tearing down wan"
            bring_wan_down
            exit 0
         else
            ulog telstra_wan status "$PID ipv4 link is down. Wan is already down"
            bring_wan_down
            exit 0
         fi
      else
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog telstra_wan status "$PID ipv4 link is up. Wan is already up"
            exit 0
         else
            if [ "up" = "$DESIRED_WAN_STATE" ] ; then
               bring_wan_up
               exit 0
            else
               ulog telstra_wan status "$PID ipv4 link is up. Wan is not requested up"
               exit 0
            fi
         fi
      fi
      ;;

   desired_ipv4_wan_state)
      if [ "up" = "$DESIRED_WAN_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog telstra_wan status "$PID wan is already up."
            exit 0
         else
            if [ "up" != "$CURRENT_LINK_STATE" ] ; then
               ulog telstra_wan status "$PID wan up request deferred until link is up"
               exit 0
            else
               bring_wan_up
               exit 0
            fi
         fi
      else
         bring_wan_down
      fi
      ;;

 *)
      ulog telstra_wan status "$PID Invalid parameter $1 "
      exit 3
      ;;
esac
