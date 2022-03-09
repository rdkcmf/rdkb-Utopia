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
# This code brings up the wan for the wan protocol L2TP
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
SELF_NAME=l2tp_wan
WAN_PROTOCOL=`syscfg get wan_proto`

#------------------------------------------------------------------
# l2tp helpers
#------------------------------------------------------------------
L2TP_CONF_DIR=/etc/l2tp
L2TP_CONF_FILE=$L2TP_CONF_DIR"/"l2tp.conf
L2TP_OPTIONS_DIR=/etc/ppp/peers
L2TP_OPTIONS_FILE=$L2TP_OPTIONS_DIR"/utopia-l2tp"
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
}

register_firewall_hooks() {
   # first make sure hooks are clean
   unregister_firewall_hooks

   # firewall pinholes to allow pptp packets from pptp server
   NAME=`sysevent setunique GeneralPurposeFirewallRule " -A INPUT -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p tcp -m tcp --sport 1701 -j xlog_accept_wan2self"`
   sysevent set ${SELF_NAME}_gp_fw_1 "$NAME"

   NAME=`sysevent setunique GeneralPurposeFirewallRule " -A INPUT -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p udp -m udp --sport 1701 -j xlog_accept_wan2self"`
   sysevent set ${SELF_NAME}_gp_fw_2 "$NAME"

   # firewall pinholes to override any dmz forwards or port forwards to other hosts
   NAME=`sysevent setunique NatFirewallRule " -A PREROUTING -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p tcp -m tcp --sport 1701 -j RETURN"`
   sysevent set ${SELF_NAME}_nat_fw_1 "$NAME"

   NAME=`sysevent setunique NatFirewallRule " -A PREROUTING -i $WAN_IFNAME -s $WAN_SERVER_IPADDR -p udp -m udp --sport 1701 -j RETURN"`
   sysevent set ${SELF_NAME}_nat_fw_2 "$NAME"
   echo "l2tp_wan : Triggering RDKB_FIREWALL_RESTART in Register FW hooks"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
}


#------------------------------------------------------------------
# prepare_l2tp
#------------------------------------------------------------------
prepare_l2tp() {

   echo "[utopia][l2tp] Configuring l2tp" > /dev/console

   # create the l2tp peers file

   mkdir -p "$PPTP_PEERS_DIRECTORY"
   prepare_pppd_ip_pre_up_script
   prepare_pppd_ip_up_script
   prepare_pppd_ip_down_script
   prepare_pppd_ipv6_up_script
   prepare_pppd_ipv6_down_script
   prepare_pppd_options
   prepare_pppd_secrets


   # create the l2tp conf file

   mkdir -p $L2TP_CONF_DIR

   echo -n > $L2TP_CONF_FILE

   # Global section (by default, we start in global mode)
   echo "global" >> $L2TP_CONF_FILE

   # Load handlers
   echo "load-handler "sync-pppd.so"" >> $L2TP_CONF_FILE
   echo "load-handler "cmd.so"" >> $L2TP_CONF_FILE

   # Bind address
   echo "listen-port 1701" >> $L2TP_CONF_FILE

   # Configure the sync-pppd handler.  You MUST have a "section sync-pppd" line
   # even if you don't set any options.
   echo "section sync-pppd" >> $L2TP_CONF_FILE
   # echo "lac-pppd-opts \"call utopia-l2tp\"" >> $L2TP_CONF_FILE
   # lac-pppd-opts "file /etc/ppp/peers/utopia-l2tp"" >> $L2TP_CONF_FILE
   echo "lac-pppd-opts \"file /etc/ppp/options\"" >> $L2TP_CONF_FILE


   # Peer section
   echo "section peer" >> $L2TP_CONF_FILE
   L2TP_SERVER_IP=`syscfg get wan_proto_server_address`
   echo "peer $L2TP_SERVER_IP" >> $L2TP_CONF_FILE
   # secret s3cr3t
   echo "port 1701" >> $L2TP_CONF_FILE
   echo "lac-handler sync-pppd" >> $L2TP_CONF_FILE
   echo "lns-handler sync-pppd" >> $L2TP_CONF_FILE
   echo "hide-avps yes" >> $L2TP_CONF_FILE

   # Configure the cmd handler.  You MUST have a "section cmd" line
   # even if you don't set any options.
   echo "section cmd" >> $L2TP_CONF_FILE

   PPP_CONN_METHOD=`syscfg get ppp_conn_method`
   LAN_IFNAME=`syscfg get lan_ifname`

}


# -------------------------------------------------------------

bring_wan_down() {
   ulog l2tp_wan status "$PID bring_wan_down"
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
   echo "l2tp_wan : Triggering RDKB_FIREWALL_RESTART from WAN down"
   t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
   sysevent set firewall-restart
}

bring_wan_up() {
   if [ "l2tp" = "$WAN_PROTOCOL" ] ; then
      ulog l2tp_wan status "$PID bring_wan_up"
      register_firewall_hooks
      # clean some of the sysevent tuples that are shared between the ip_up script and wmon
      sysevent set pppd_current_wan_ipaddr
      sysevent set pppd_current_wan_subnet
      sysevent set pppd_current_wan_ifname
      prepare_l2tp
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

ulog l2tp_wan status "$PID current_ipv4_link_state is $CURRENT_LINK_STATE"
ulog l2tp_wan status "$PID desired_ipv4_wan_state is $DESIRED_WAN_STATE"
ulog l2tp_wan status "$PID current_ipv4_wan_state is $CURRENT_WAN_STATE"
ulog l2tp_wan status "$PID wan_proto is $WAN_PROTOCOL"


case "$1" in
   current_ipv4_link_state)
      ulog l2tp_wan status "$PID ipv4 link state is $CURRENT_LINK_STATE"
      if [ "up" != "$CURRENT_LINK_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog l2tp_wan status "$PID ipv4 link is down. Tearing down wan"
            bring_wan_down
            exit 0
         else
            ulog l2tp_wan status "$PID ipv4 link is down. Wan is already down"
            bring_wan_down
            exit 0
         fi
      else
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog l2tp_wan status "$PID ipv4 link is up. Wan is already up"
            exit 0
         else
            if [ "up" = "$DESIRED_WAN_STATE" ] ; then
                  bring_wan_up
                  exit 0
            else
               ulog l2tp_wan status "$PID ipv4 link is up. Wan is not requested up"
               exit 0
            fi
         fi
      fi
      ;;

   desired_ipv4_wan_state)
      if [ "up" = "$DESIRED_WAN_STATE" ] ; then
         if [ "up" = "$CURRENT_WAN_STATE" ] ; then
            ulog l2tp_wan status "$PID wan is already up."
            exit 0
         else
            if [ "up" != "$CURRENT_LINK_STATE" ] ; then
               ulog l2tp_wan status "$PID wan up request deferred until link is up"
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
      ulog l2tp_wan status "$PID Invalid parameter $1 "
      exit 3
      ;;
esac
