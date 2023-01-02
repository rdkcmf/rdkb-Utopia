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
SELF_NAME=pptp_wan
WAN_PROTOCOL=`syscfg get wan_proto`

#------------------------------------------------------------------
# pptp helpers
#------------------------------------------------------------------
PPTP_PEERS_DIRECTORY=/etc/ppp/peers
PPTP_TUNNEL_NAME=utopia-pptp
PPTP_PEERS_FILE=$PPTP_PEERS_DIRECTORY"/"$PPTP_TUNNEL_NAME
PPTP_OPTIONS_FILE=/etc/ppp/options.pptp
WAN_SERVER_IPADDR=`syscfg get wan_proto_server_address`

#------------------------------------------------------------------
#  Firewall hooks
#------------------------------------------------------------------
unregister_firewall_hooks() {
   NAME=`sysevent get ${SELF_NAME}_gp_fw_1`
   if [ -n "$NAME" ] ; then
      sysevent set "$NAME"
      sysevent set ${SELF_NAME}_gp_fw_1
   fi
   NAME=`sysevent get ${SELF_NAME}_gp_fw_2`
   if [ -n "$NAME" ] ; then
      sysevent set "$NAME"
      sysevent set ${SELF_NAME}_gp_fw_2
   fi
   NAME=`sysevent get ${SELF_NAME}_gp_fw_3`
   if [ -n "$NAME" ] ; then
      sysevent set "$NAME"
      sysevent set ${SELF_NAME}_gp_fw_3
   fi
   NAME=`sysevent get ${SELF_NAME}_nat_fw_1`
   if [ -n "$NAME" ] ; then
      sysevent set "$NAME"
      sysevent set ${SELF_NAME}_nat_fw_1
   fi
   NAME=`sysevent get ${SELF_NAME}_nat_fw_2`
   if [ -n "$NAME" ] ; then
      sysevent set "$NAME"
      sysevent set ${SELF_NAME}_nat_fw_2
   fi
   NAME=`sysevent get ${SELF_NAME}_nat_fw_3`
   if [ -n "$NAME" ] ; then
      sysevent set "$NAME"
      sysevent set ${SELF_NAME}_nat_fw_3
   fi
}

register_firewall_hooks() {
   # first make sure hooks are clean
   unregister_firewall_hooks

   # firewall pinholes to allow pptp packets from pptp server
   NAME=`sysevent setunique GeneralPurposeFirewallRule " -A INPUT -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p tcp -m tcp --sport 1723 -j xlog_accept_wan2self"`
   sysevent set ${SELF_NAME}_gp_fw_1 "$NAME"

   NAME=`sysevent setunique GeneralPurposeFirewallRule " -A INPUT -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p udp -m udp --sport 1723 -j xlog_accept_wan2self"`
   sysevent set ${SELF_NAME}_gp_fw_2 "$NAME"

   NAME=`sysevent setunique GeneralPurposeFirewallRule " -A INPUT -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p 0x2f -j xlog_accept_wan2self"`
   sysevent set ${SELF_NAME}_gp_fw_3 "$NAME"

   # firewall pinholes to override any dmz forwards or port forwards to other hosts  

   NAME=`sysevent setunique NatFirewallRule " -A PREROUTING -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p tcp -m tcp --sport 1723 -j RETURN"`
   sysevent set ${SELF_NAME}_nat_fw_1 "$NAME"

   NAME=`sysevent setunique NatFirewallRule " -A PREROUTING -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p udp -m udp --sport 1723 -j RETURN"`
   sysevent set ${SELF_NAME}_nat_fw_2 "$NAME"

   NAME=`sysevent setunique NatFirewallRule " -A PREROUTING -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p 0x2f -j RETURN"`
   sysevent set ${SELF_NAME}_nat_fw_3 "$NAME"
 
   echo "pptp_wan : Triggering RDKB_FIREWALL_RESTART from Firewall Register hooks"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
}


#------------------------------------------------------------------
# prepare_pptp
#------------------------------------------------------------------
prepare_pptp() {

   echo "[utopia][pptp] Configuring pptp" > /dev/console

   # create the pptp peers file
   # pptp executable
   PPTP_BIN=/usr/sbin/pptp

   mkdir -p $PPTP_PEERS_DIRECTORY
   prepare_pppd_ip_pre_up_script
   prepare_pppd_ip_up_script
   prepare_pppd_ip_down_script
   prepare_pppd_ipv6_up_script
   prepare_pppd_ipv6_down_script
   prepare_pppd_options
   # same secrets file as pppoe
   prepare_pppd_secrets

   echo -n > $PPTP_PEERS_FILE

   USER=`syscfg get wan_proto_username`
   DOMAIN=`syscfg get wan_domain`
   echo "pty \"$PPTP_BIN $WAN_SERVER_IPADDR --nolaunchpppd\"" >> $PPTP_PEERS_FILE
   if [ -z "$DOMAIN" ] ; then
      echo "name $USER"  >> $PPTP_PEERS_FILE
   else
      echo "name $DOMAIN\\\\$USER"  >> $PPTP_PEERS_FILE
   fi
   REMOTE_NAME=`syscfg get wan_proto_remote_name`
   if [ -n "$REMOTE_NAME" ] ; then
      echo "remotename \"$REMOTE_NAME\"" >> $PPTP_PEERS_FILE
   fi

#   echo "file $PPTP_OPTIONS_FILE" >> $PPTP_PEERS_FILE
   echo "ipparam $PPTP_TUNNEL_NAME"  >> $PPTP_PEERS_FILE
}

# -------------------------------------------------------------

bring_wan_down() {
   ulog pptp_wan status "$PID bring_wan_down"
   unregister_firewall_hooks
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
   echo "pptp_wan : Triggering RDKB_FIREWALL_RESTART from WAN down"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
}

bring_wan_up() {
   if [ "pptp" = "$WAN_PROTOCOL" ] ; then
      ulog pptp_wan status "$PID bring_wan_up"
      register_firewall_hooks
      # clean some of the sysevent tuples that are shared between the ip_up script and wmon
      sysevent set pppd_current_wan_ipaddr
      sysevent set pppd_current_wan_subnet
      sysevent set pppd_current_wan_ifname
      prepare_pptp
      do_start_wan_monitor
      sysevent set wan_start_time $(cut -d. -f1 /proc/uptime)
   fi
}

# --------------------------------------------------------
# we need to react to three events:
#   current_ipv4_link_state - up | down
#   desired_ipv4_wan_state - up | down
#   current_ipv4_wan_state - up | down
# --------------------------------------------------------

ulog pptp_wan status "$PID current_ipv4_link_state is $CURRENT_LINK_STATE"
ulog pptp_wan status "$PID desired_ipv4_wan_state is $DESIRED_WAN_STATE"
ulog pptp_wan status "$PID current_ipv4_wan_state is $CURRENT_WAN_STATE"
ulog pptp_wan status "$PID wan_proto is $WAN_PROTOCOL"

case "$1" in
   current_ipv4_link_state)
      ulog pptp_wan status "$PID ipv4 link state is $CURRENT_LINK_STATE"
      if [ "up" != "$CURRENT_LINK_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog pptp_wan status "$PID ipv4 link is down. Tearing down wan"
            bring_wan_down
            exit 0
         else
            ulog pptp_wan status "$PID ipv4 link is down. Wan is already down"
            bring_wan_down
            exit 0
         fi
      else
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog pptp_wan status "$PID ipv4 link is up. Wan is already up"
            exit 0
         else
            if [ "up" = "$DESIRED_WAN_STATE" ] ; then
                  bring_wan_up
                  exit 0
            else
               ulog pptp_wan status "$PID ipv4 link is up. Wan is not requested up"
               exit 0
            fi
         fi
      fi
      ;;

   desired_ipv4_wan_state)
      if [ "up" = "$DESIRED_WAN_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog pptp_wan status "$PID wan is already up."
            exit 0
         else
            if [ "up" != "$CURRENT_LINK_STATE" ] ; then
               ulog pptp_wan status "$PID wan up request deferred until link is up"
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
      ulog pptp_wan status "$PID Invalid parameter $1 "
      exit 3
      ;;
esac
