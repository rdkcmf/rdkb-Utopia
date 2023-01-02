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

#source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
source /etc/utopia/service.d/hostname_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
#source /etc/utopia/service.d/service_lan/wlan.sh
source /etc/utopia/service.d/event_handler_functions.sh
#source /etc/utopia/service.d/brcm_ethernet_helper.sh

SERVICE_NAME="bridge"

UDHCPC_PID_FILE=/var/run/udhcpc.pid
UDHCPC_SCRIPT=/etc/utopia/service.d/service_bridge/dhcp_link.sh

#-------------------------------------------------------------
# Registration/Deregistration of dhcp client restart/release/renew handlers
# These are only needed if the dhcp is used
# Note that service_bridge is creating the pseudo service dhcp_client
#-------------------------------------------------------------
HANDLER="/etc/utopia/service.d/service_bridge/dhcp_link.sh"

unregister_dhcp_client_handlers() {
   # ulog bridge status "$PID unregister_dhcp_client_handlers"
   asyncid1=`sysevent get ${SERVICE_NAME}_async_id_1`;
   if [ -n "$asyncid1" ] ; then
      sysevent rm_async $asyncid1
      sysevent set ${SERVICE_NAME}_async_id_1
   fi
   asyncid2=`sysevent get ${SERVICE_NAME}_async_id_2`;
   if [ -n "$asyncid2" ] ; then
      sysevent rm_async $asyncid2
      sysevent set ${SERVICE_NAME}_async_id_2
   fi
   asyncid3=`sysevent get ${SERVICE_NAME}_async_id_3`;
   if [ -n "$asyncid3" ] ; then
      sysevent rm_async $asyncid3
      sysevent set ${SERVICE_NAME}_async_id_3
   fi
}

register_dhcp_client_handlers() {
   # ulog bridge status "$PID register_dhcp_client_handlers"
   # Remove any prior notification requests
   unregister_dhcp_client_handlers

   # instantiate a request to be notified when the dhcp_client-restart changes
   # make it an event (TUPLE_FLAG_EVENT = $TUPLE_FLAG_EVENT)
   asyncid1=`sysevent async dhcp_client-restart "$HANDLER"`;
   sysevent setoptions dhcp_client-restart $TUPLE_FLAG_EVENT
   sysevent set ${SERVICE_NAME}_async_id_1 "$asyncid1"

   # instantiate a request to be notified when the dhcp_client-release / renew changes
   # make it an event (TUPLE_FLAG_EVENT = $TUPLE_FLAG_EVENT)
   asyncid2=`sysevent async dhcp_client-release "$HANDLER"`;
   sysevent setoptions dhcp_client-release $TUPLE_FLAG_EVENT
   sysevent set ${SERVICE_NAME}_async_id_2 "$asyncid2"

   asyncid3=`sysevent async dhcp_client-renew "$HANDLER"`;
   sysevent setoptions dhcp_client-renew $TUPLE_FLAG_EVENT
   sysevent set ${SERVICE_NAME}_async_id_3 "$asyncid3"
}


#--------------------------------------------------------------
# Enslave a physical or virtual interface to a bridge
# 
# Takes parameters : 
#   $1  : the name of the interface to enslave
#   $2  : the name of the interface to enslave it to
#--------------------------------------------------------------
enslave_a_interface() {
   ip link set $1 up
   ip link set $1 allmulticast on
   brctl addif $2 $1
}

#--------------------------------------------------------------
# Bring up the ethernet interfaces
#--------------------------------------------------------------
bringup_ethernet_interfaces() {
   return 0
}

#--------------------------------------------------------------
# Tear down the ethernet interfaces
#--------------------------------------------------------------
teardown_ethernet_interfaces() {
   for loop in $SYSCFG_lan_ethernet_physical_ifnames
   do
      ip link set $loop down
   done
}

#--------------------------------------------------------------
# Bring up the wireless interfaces
#--------------------------------------------------------------
bringup_wireless_interfaces() {

   INCR_AMOUNT=10
   WIFI_IF_INDEX=1

   if [ -n "$SYSCFG_lan_wl_physical_ifnames" ] ; then
       for loop in $SYSCFG_lan_wl_physical_ifnames
       do
           MAC=`syscfg get "macwifi0${WIFI_IF_INDEX}bssid1"`
           
           ifconfig $loop hw ether $MAC
           ip link set $loop allmulticast on
           ulog lan status "setting $loop hw address to $MAC"
           WL_STATE=`syscfg get wl$(($WIFI_IF_INDEX-1))_state`

           ulog lan status "wlancfg $loop $WL_STATE"
           wlancfg $loop $WL_STATE
           wlancfg $loop $WL_STATE
           WIFI_IF_INDEX=`expr $WIFI_IF_INDEX + 1`
      done
   fi
}

#--------------------------------------------------------------
# Teardown the wireless interfaces
#--------------------------------------------------------------
teardown_wireless_interfaces() {
   for loop in $SYSCFG_lan_wl_physical_ifnames
   do
      wlancfg $loop down
      ip link set $loop down
   done

   teardown_wireless_daemons
}

#--------------------------------------------------------------
# stop_firewall
# If the firewall is up, then tear it down
#--------------------------------------------------------------
stop_firewall()
{
   STATUS=`sysevent get firewall-status`
   if [ "stopped" != "$STATUS" ] ; then
      echo "service_bridge : Triggering RDKB_FIREWALL_STOP"
      sysevent set firewall-stop
      sleep 1
      wait_till_end_state firewall
   fi
}

#--------------------------------------------------------------
# add_ebtable_rule
# Add rule in ebtable nat PREROUTING chain
#--------------------------------------------------------------
add_ebtable_rule()
{
    # Add the rule to redirect diagnostic traffic to CM-LAN in bridge mode
    prod_model=`awk -F'[-=]' '/^VERSION/ {print $2}' /etc/versions`
    cmdiag_if=`syscfg get cmdiag_ifname`
    cmdiag_if_mac=`ip link show $cmdiag_if | awk '/link/ {print $2}'`

    dst_ip="10.0.0.1" # RT-10-580 @ XB3 
    ip addr add $dst_ip/24 dev $cmdiag_if
    ip route add default dev $cmdiag_if
    ebtables -t nat -A PREROUTING -p ipv4 --ip-dst $dst_ip -j dnat --to-destination $cmdiag_if_mac
    echo 2 > /proc/sys/net/ipv4/conf/wan0/arp_announce
}

#--------------------------------------------------------------
# del_ebtable_rule
# Delete rule in ebtable nat PREROUTING chain
#--------------------------------------------------------------
del_ebtable_rule()
{
    prod_model=`awk -F'[-=]' '/^VERSION/ {print $2}' /etc/versions`
    cmdiag_if=`syscfg get cmdiag_ifname`
    cmdiag_if_mac=`ip link show $cmdiag_if | awk '/link/ {print $2}'`

    dst_ip="10.0.0.1" # RT-10-580 @ XB3 PRD
    ip addr del $dst_ip/24 dev $cmdiag_if
    ip route del default dev $cmdiag_if
    ebtables -t nat -D PREROUTING -p ipv4 --ip-dst $dst_ip -j dnat --to-destination $cmdiag_if_mac
    echo 0 > /proc/sys/net/ipv4/conf/wan0/arp_announce
}

#--------------------------------------------------------------
# do_start
#--------------------------------------------------------------
do_start()
{
   ulog bridge status "stopping firewall"
   stop_firewall
   ulog bridge status "firewall status is now `sysevent get firewall-status`"

   ulog bridge status "reprogramming ethernet switch to remove vlans"
   #disable_vlan_mode_on_ethernet_switch

   ulog bridge status "bringing up lan interface in bridge mode"
   bringup_ethernet_interfaces
   bringup_wireless_interfaces
   
   brctl addbr $SYSCFG_lan_ifname
   brctl setfd $SYSCFG_lan_ifname 0
   #brctl stp $SYSCFG_lan_ifname on
   brctl stp $SYSCFG_lan_ifname off


   # enslave interfaces to the bridge
   enslave_a_interface $SYSCFG_wan_physical_ifname $SYSCFG_lan_ifname
   for loop in $LAN_IFNAMES
   do
      enslave_a_interface $loop $SYSCFG_lan_ifname
   done

   # bring up the bridge
   ip link set $SYSCFG_lan_ifname up 
   ip link set $SYSCFG_lan_ifname allmulticast on

   ifconfig $SYSCFG_lan_ifname hw ether `get_mac $SYSCFG_wan_physical_ifname` 

   # bridge_mode 1 is dhcp, bridge_mode 2 is static, bridge_mode 3 is full-static
   if ( [ "2" = "$SYSCFG_bridge_mode" ] || [ "3" = "$SYSCFG_bridge_mode" ] ) && [ -n "$SYSCFG_bridge_ipaddr" ] && [ -n "$SYSCFG_bridge_netmask" ] && [ -n "$SYSCFG_bridge_default_gateway" ]; then
      RESOLV_CONF="/etc/resolv.conf"
      echo -n  > $RESOLV_CONF
      if [ -n "$SYSCFG_bridge_domain" ] ; then
         echo "search $SYSCFG_bridge_domain" >> $RESOLV_CONF
      fi
      if [ -n "$SYSCFG_bridge_nameserver1" ]  && [ "0.0.0.0" !=  "$SYSCFG_bridge_nameserver1" ] ; then
         echo "nameserver $SYSCFG_bridge_nameserver1" >> $RESOLV_CONF
      fi
      if [ -n "$SYSCFG_bridge_nameserver2" ]  && [ "0.0.0.0" !=  "$SYSCFG_bridge_nameserver2" ] ; then
         echo "nameserver $SYSCFG_bridge_nameserver2" >> $RESOLV_CONF
      fi
      if [ -n "$SYSCFG_bridge_nameserver3" ]  && [ "0.0.0.0" !=  "$SYSCFG_bridge_nameserver3" ] ; then
         echo "nameserver $SYSCFG_bridge_nameserver3" >> $RESOLV_CONF
      fi
      ip -4 addr add  $SYSCFG_bridge_ipaddr/$SYSCFG_bridge_netmask broadcast + dev $SYSCFG_lan_ifname
      ip -4 route add default dev $SYSCFG_lan_ifname via $SYSCFG_bridge_default_gateway
      # set sysevent tuple showing current state
      sysevent set bridge_ipv4_ipaddr $SYSCFG_bridge_ipaddr
      sysevent set bridge_ipv4_subnet $SYSCFG_bridge_netmask
      sysevent set bridge_default_router $SYSCFG_bridge_default_gateway

   else
      udhcpc -S -b -i $SYSCFG_lan_ifname -h $SYSCFG_hostname -p $UDHCPC_PID_FILE  --arping -s $UDHCPC_SCRIPT 
      register_dhcp_client_handlers
   fi

 #  vendor_block_dos_land_attack

   bringup_wireless_daemons

   prepare_hostname
   
   if [ "1" = "`sysevent get byoi_bridge_mode`" ]; then
      sysevent set dns-start
   fi

   ulog bridge status "lan interface up"
}

#--------------------------------------------------------------
# do_stop
#--------------------------------------------------------------
do_stop()
{
   sysevent set dns-stop

   unregister_dhcp_client_handlers
   ip link set $SYSCFG_lan_ifname down
   ip addr flush dev $SYSCFG_lan_ifname

   teardown_wireless_interfaces
   teardown_ethernet_interfaces

   # remove interfaces from the bridge
   for loop in $LAN_IFNAMES
   do
      ip link set $loop down
      brctl delif $SYSCFG_lan_ifname $loop
   done
   ip link set $SYSCFG_wan_physical_ifname down

   ip link set $SYSCFG_lan_ifname down

   brctl delbr $SYSCFG_lan_ifname
}

do_start_multi()
{
# TODO: add brport to defaults
PRI_L2=`sysevent get primary_lan_l2net`
sysevent set multinet-start $PRI_L2
/etc/utopia/service.d/ebtable_rules.sh
# set brport enabled
# set resync for primary l2net
# set firewall restart
}

do_stop_multi()
{
# set brport disabled
# set resync primary l2net
# set firewall restart
echo
}

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   # Get all provisioning data
   # Figure out the names and addresses of the lan interface
   #
   # SYSCFG_lan_ethernet_physical_ifnames is the physical ethernet interfaces that
   # will be part of the lan
   #
   # SYSCFG_lan_wl_physical_ifnames is the names of each wireless interface as known
   # to the operating system

   SYSCFG_FAILED='false'
   FOO=`utctx_cmd get bridge_mode lan_ifname lan_ethernet_physical_ifnames lan_wl_physical_ifnames wan_physical_ifname bridge_ipaddr bridge_netmask bridge_default_gateway bridge_nameserver1 bridge_nameserver2 bridge_nameserver3 bridge_domain hostname`
   eval $FOO
  if [ $SYSCFG_FAILED = 'true' ] ; then
     ulog bridge status "$PID utctx failed to get some configuration data"
     ulog bridge status "$PID BRIDGE CANNOT BE CONTROLLED"
     exit
  fi

  if [ -z "$SYSCFG_hostname" ] ; then
     SYSCFG_hostname="Utopia"
  fi 

  LAN_IFNAMES="$SYSCFG_lan_ethernet_physical_ifnames"

   # if we are using wireless interfafes then add them
   if [ -n "$SYSCFG_lan_wl_physical_ifnames" ] ; then
      LAN_IFNAMES="$LAN_IFNAMES $SYSCFG_lan_wl_physical_ifnames"
   fi
}

#--------------------------------------------------------------
# service_start
#--------------------------------------------------------------
service_start ()
{
   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "started" != "$STATUS" ] ; then
      do_start_multi
      ERR=$?
      if [ "$ERR" -ne "0" ] ; then
         check_err $? "Unable to bringup bridge"
      else
         sysevent set ${SERVICE_NAME}-errinfo
         sysevent set ${SERVICE_NAME}-status starting
		
         add_ebtable_rule

         # Flush all dynamic mac entries
         echo "LearnFrom=CPE_DYNAMIC" > /proc/net/dbrctl/delalt
         echo "flush_all_sessions" > /proc/net/ti_pp			
         # Force a DHCP renew by issuing a physical link down/up, when WAN port mode switches between bridging and routing
         PSM_MODE=`sysevent get system_psm_mode`
         if [ "$PSM_MODE" != "1" ]; then
            # It is not a good practice to force all physical links to refresh -- should have used arguments to specify which ports/links
            gw_lan_refresh
         fi
         #set hostname            
         prepare_hostname

         sysevent set ${SERVICE_NAME}-status started
      fi
   fi
}

#--------------------------------------------------------------
# service_stop
#--------------------------------------------------------------
service_stop ()
{
   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status` 
   if [ "stopped" != "$STATUS" ] ; then
      do_stop_multi
      ERR=$?
      if [ "$ERR" -ne "0" ] ; then
         check_err $ERR "Unable to teardown bridge"
      else
         del_ebtable_rule

         # Flush all dynamic mac entries
         echo "LearnFrom=CPE_DYNAMIC" > /proc/net/dbrctl/delalt
         echo "flush_all_sessions" > /proc/net/ti_pp		
         sysevent set ${SERVICE_NAME}-errinfo
         sysevent set ${SERVICE_NAME}-status stopped
      fi
   fi
}

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------

service_init 
echo "service_bridge.sh called with $1 $2" > /dev/console
case "$1" in
   ${SERVICE_NAME}-start)
      service_start
      ;;
   ${SERVICE_NAME}-stop)
      service_stop
      ;;
   ${SERVICE_NAME}-restart)
      echo "service_init : setting lan-restarting to 1"
      sysevent set lan-restarting 1
      service_stop
      service_start
      echo "service_init : setting lan-restarting to 0"
      sysevent set lan-restarting 0
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
