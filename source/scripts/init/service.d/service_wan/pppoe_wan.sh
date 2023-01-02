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
# This code brings up the wan for the wan protocol PPTP
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
WAN_PROTOCOL=`syscfg get wan_proto`

#------------------------------------------------------------------
# prepare_pppoe
#------------------------------------------------------------------
prepare_pppoe() {

   echo "[utopia][pppoe] Configuring pppoe" > /dev/console

   # create the pppoe peers file
   mkdir -p "$PPP_PEERS_DIRECTORY"
   prepare_pppd_ip_pre_up_script
   prepare_pppd_ip_up_script
   prepare_pppd_ip_down_script
   prepare_pppd_ipv6_up_script
   prepare_pppd_ipv6_down_script
   prepare_pppd_options
   prepare_pppd_secrets

   echo -n > "$PPPOE_PEERS_FILE"

   echo "plugin  rp-pppoe.so" >> "$PPPOE_PEERS_FILE"
   # Ethernet interface name
   INTERFACE_NAME=`sysevent get wan_ifname`
   echo "# Ethernet interface name" >> "$PPPOE_PEERS_FILE"
   echo "$INTERFACE_NAME" >> "$PPPOE_PEERS_FILE"
   USER=`syscfg get wan_proto_username`
   DOMAIN=`syscfg get wan_domain`
   if [ -z "$DOMAIN" ] ; then
      echo "user $USER" >> "$PPPOE_PEERS_FILE"
   else
      echo "user $DOMAIN\\\\$USER"  >> "$PPPOE_PEERS_FILE"
   fi
   # What should be in the second column in /etc/ppp/*-secrets
   REMOTE_NAME=`syscfg get wan_proto_remote_name`
   if [ -n "$REMOTE_NAME" ] ; then
      echo "remotename \"$REMOTE_NAME\"" >> "$PPPOE_PEERS_FILE"
   fi
   # If needed, specify the service and the access concentrator name
   SERVICE=`syscfg get pppoe_service_name`
   if [ -n "$SERVICE" ] ; then
      echo "rp_pppoe_service $SERVICE" >> "$PPPOE_PEERS_FILE"
   fi
   AC_NAME=`syscfg get pppoe_access_concentrator_name`
   if [ -n "$AC_NAME" ] ; then
      echo "rp_pppoe_ac $AC_NAME" >> "$PPPOE_PEERS_FILE"
   fi
   # The settings below usually don't need to be changed
   echo "noauth" >> "$PPPOE_PEERS_FILE"
   echo "hide-password" >> "$PPPOE_PEERS_FILE"
   echo "updetach" >> "$PPPOE_PEERS_FILE"
   echo "debug" >> "$PPPOE_PEERS_FILE"
   echo "defaultroute" >> "$PPPOE_PEERS_FILE"
   echo "noipdefault" >> "$PPPOE_PEERS_FILE"
   echo "usepeerdns" >> "$PPPOE_PEERS_FILE"
}

# -------------------------------------------------------------

bring_wan_down() {
   ulog pppoe_wan status "$PID bring_wan_down"
   echo 0 > /proc/sys/net/ipv4/ip_forward
   do_stop_wan_monitor
   STATUS=`sysevent get wan-status`
   if [ "stopped" != "$STATUS" ] ; then
      sysevent set wan-status stopped
   fi

   sysevent set pppd_current_wan_ipaddr
   sysevent set pppd_current_wan_subnet
   sysevent set pppd_current_wan_ifname
   sysevent set current_wan_ipaddr 0.0.0.0
   sysevent set current_wan_subnet 0.0.0.0
   echo "pppoe_wan : Triggering RDKB_FIREWALL_RESTART"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
}

bring_wan_up() {
   if [ "pppoe" = "$WAN_PROTOCOL" ] ; then
      ulog pppoe_wan status "$PID bring_wan_up"
      # clean some of the sysevent tuples that are shared between the ip_up script and wmon
      sysevent set pppd_current_wan_ipaddr
      sysevent set pppd_current_wan_subnet
      sysevent set pppd_current_wan_ifname
      prepare_pppoe
      do_start_wan_monitor
      sysevent set wan_start_time $(cut -d. -f1 /proc/uptime)
   fi
}


# --------------------------------------------------------
# we need to react to three events:
#   current_ipv4_link_state - up | down
#   desired_ipv4_wan_state - up | down
# --------------------------------------------------------

ulog pppoe_wan status "$PID current_ipv4_link_state is $CURRENT_LINK_STATE"
ulog pppoe_wan status "$PID desired_ipv4_wan_state is $DESIRED_WAN_STATE"
ulog pppoe_wan status "$PID current_ipv4_wan_state is $CURRENT_WAN_STATE"
ulog pppoe_wan status "$PID wan_proto is $WAN_PROTOCOL"

case "$1" in
   current_ipv4_link_state)
      ulog pppoe_wan status "$PID ipv4 link state is $CURRENT_LINK_STATE"
      if [ "up" != "$CURRENT_LINK_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog pppoe_wan status "$PID ipv4 link is down. Tearing down wan"
            bring_wan_down
            exit 0
         else
            ulog pppoe_wan status "$PID ipv4 link is down. Wan is already down"
            bring_wan_down
            exit 0
         fi
      else
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog pppoe_wan status "$PID ipv4 link is up. Wan is already up"
            exit 0
         else
            if [ "up" = "$DESIRED_WAN_STATE" ] ; then
                  bring_wan_up
                  exit 0
            else
               ulog pppoe_wan status "$PID ipv4 link is up. Wan is not requested up"
               exit 0
            fi
         fi
      fi
      ;;

   desired_ipv4_wan_state)
      if [ "up" = "$DESIRED_WAN_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog pppoe_wan status "$PID wan is already up."
            exit 0
         else
            if [ "up" != "$CURRENT_LINK_STATE" ] ; then
               ulog pppoe_wan status "$PID wan up request deferred until link is up"
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
      ulog pppoe_wan status "$PID Invalid parameter $1 "
      exit 3
      ;;
esac

