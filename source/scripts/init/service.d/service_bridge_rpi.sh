#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
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

##########################################################################
#   Copyright [2018] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#########################################################################


source /etc/utopia/service.d/hostname_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="bridge"

UDHCPC_PID_FILE=/var/run/udhcpc.pid
UDHCPC_SCRIPT=/etc/utopia/service.d/service_bridge/dhcp_link.sh

POSTD_START_FILE="/tmp/.postd_started"

#Separate routing table used to ensure that responses from the web UI go directly to the LAN interface, not out erouter0
BRIDGE_MODE_TABLE=69



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
      sysevent rm_async "$asyncid1"
      sysevent set ${SERVICE_NAME}_async_id_1
   fi
   asyncid2=`sysevent get ${SERVICE_NAME}_async_id_2`;
   if [ -n "$asyncid2" ] ; then
      sysevent rm_async "$asyncid2"
      sysevent set ${SERVICE_NAME}_async_id_2
   fi
   asyncid3=`sysevent get ${SERVICE_NAME}_async_id_3`;
   if [ -n "$asyncid3" ] ; then
      sysevent rm_async "$asyncid3"
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
   sysevent setoptions dhcp_client-restart "$TUPLE_FLAG_EVENT"
   sysevent set ${SERVICE_NAME}_async_id_1 "$asyncid1"

   # instantiate a request to be notified when the dhcp_client-release / renew changes
   # make it an event (TUPLE_FLAG_EVENT = $TUPLE_FLAG_EVENT)
   asyncid2=`sysevent async dhcp_client-release "$HANDLER"`;
   sysevent setoptions dhcp_client-release "$TUPLE_FLAG_EVENT"
   sysevent set ${SERVICE_NAME}_async_id_2 "$asyncid2"

   asyncid3=`sysevent async dhcp_client-renew "$HANDLER"`;
   sysevent setoptions dhcp_client-renew "$TUPLE_FLAG_EVENT"
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
   ip link set "$1" up
   ip link set "$1" allmulticast on
   brctl addif "$2" "$1"
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
      ip link set "$loop" down
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
           
           ifconfig "$loop" hw ether "$MAC"
           ip link set "$loop" allmulticast on
           ulog lan status "setting $loop hw address to $MAC"
           WL_STATE=`syscfg get wl$(($WIFI_IF_INDEX-1))_state`

           ulog lan status "wlancfg $loop $WL_STATE"
           wlancfg "$loop" "$WL_STATE"
           wlancfg "$loop" "$WL_STATE"
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
      wlancfg "$loop" down
      ip link set "$loop" down
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
   echo "Inside add_ebtable_rule"
    # Add the rule to redirect diagnostic traffic to CM-LAN in bridge mode
    #prod_model=`awk -F'[-=]' '/^VERSION/ {print $2}' /etc/versions`

    cmdiag_if=`syscfg get cmdiag_ifname`
    cmdiag_if_mac=`ip link show "$cmdiag_if" | awk '/link/ {print $2}'`
    cmdiag_ip="192.168.100.1"
    wan_if=`syscfg get wan_physical_ifname`


    wan_if=`syscfg get wan_physical_ifname` #erouter0
    subnet_wan=`ip route show | awk '/'"$wan_if"'/ {print $1}' | tail -1` 

     echo "###############################################"
     echo "cmdiag_if=$cmdiag_if"
     echo "cmdiag_ip=$cmdiag_ip"
     echo "wan_if=$wan_if"
     echo "subnet_wan=$subnet_wan"
     echo "###############################################"

    echo "ip route del $subnet_wan dev $wan_if"
    ip route del "$subnet_wan" dev "$wan_if"

    echo "ip route add $subnet_wan dev $cmdiag_if" #proto kernel scope link src $cmdiag_ip"
    ip route add "$subnet_wan" dev "$cmdiag_if" #proto kernel scope link src $cmdiag_ip

    dst_ip="10.0.0.1" # RT-10-580 @ XB3 
    echo "ip addr add $dst_ip/24 dev $cmdiag_if"
    ip addr add $dst_ip/24 dev "$cmdiag_if"

    echo "ebtables -t nat -A PREROUTING -p ipv4 --ip-dst $dst_ip -j dnat --to-destination $cmdiag_if_mac"
    ebtables -t nat -A PREROUTING -p ipv4 --ip-dst $dst_ip -j dnat --to-destination "$cmdiag_if_mac"

    echo 2 > /proc/sys/net/ipv4/conf/wlan0/arp_announce
    echo "echo 2 > /proc/sys/net/ipv4/conf/wlan0/arp_announce"
}

#--------------------------------------------------------------
# del_ebtable_rule
# Delete rule in ebtable nat PREROUTING chain
#--------------------------------------------------------------
del_ebtable_rule()
{
    prod_model=`awk -F'[-=]' '/^VERSION/ {print $2}' /etc/versions`
    cmdiag_if=`syscfg get cmdiag_ifname`
    cmdiag_if_mac=`ip link show "$cmdiag_if" | awk '/link/ {print $2}'`
    
    wan_if=`syscfg get wan_physical_ifname`
    wan_ip=`sysevent get ipv4_wan_ipaddr`
    subnet_wan=`ip route show | grep "$cmdiag_if" | grep -v 192.168.100. | grep -v 10.0.0 | awk '/'"$cmdiag_if"'/ {print $1}'`

    ip route del "$subnet_wan" dev "$cmdiag_if"
    ip route add "$subnet_wan" dev "$wan_if" proto kernel scope link src "$wan_ip"



    dst_ip="10.0.0.1" # RT-10-580 @ XB3 PRD
    ip addr del $dst_ip/24 dev "$cmdiag_if"
    ebtables -t nat -D PREROUTING -p ipv4 --ip-dst $dst_ip -j dnat --to-destination "$cmdiag_if_mac"
    #echo 0 > /proc/sys/net/ipv4/conf/wan0/arp_announce
    echo 0 > /proc/sys/net/ipv4/conf/wlan0/arp_announce
    echo "echo 0 > /proc/sys/net/ipv4/conf/wlan0/arp_announce"
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
   
   brctl addbr "$SYSCFG_lan_ifname"
   brctl setfd "$SYSCFG_lan_ifname" 0
   #brctl stp $SYSCFG_lan_ifname on
   brctl stp "$SYSCFG_lan_ifname" off


   # enslave interfaces to the bridge
   enslave_a_interface "$SYSCFG_wan_physical_ifname" "$SYSCFG_lan_ifname"
   for loop in $LAN_IFNAMES
   do
      enslave_a_interface "$loop" "$SYSCFG_lan_ifname"
   done

   # bring up the bridge
   ip link set "$SYSCFG_lan_ifname" up 
   ip link set "$SYSCFG_lan_ifname" allmulticast on

   ifconfig "$SYSCFG_lan_ifname" hw ether "`get_mac "$SYSCFG_wan_physical_ifname"`" 

   # bridge_mode 1 is dhcp, bridge_mode 2 is static
   if [ "2" = "$SYSCFG_bridge_mode" ] && [ -n "$SYSCFG_bridge_ipaddr" ] && [ -n "$SYSCFG_bridge_netmask" ] && [ -n "$SYSCFG_bridge_default_gateway" ]; then
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
      ip -4 addr add  "$SYSCFG_bridge_ipaddr"/"$SYSCFG_bridge_netmask" broadcast + dev "$SYSCFG_lan_ifname"
      ip -4 route add default dev "$SYSCFG_lan_ifname" via "$SYSCFG_bridge_default_gateway"
      # set sysevent tuple showing current state
      sysevent set bridge_ipv4_ipaddr "$SYSCFG_bridge_ipaddr"
      sysevent set bridge_ipv4_subnet "$SYSCFG_bridge_netmask"
      sysevent set bridge_default_router "$SYSCFG_bridge_default_gateway"

   else
      udhcpc -S -b -i "$SYSCFG_lan_ifname" -h "$SYSCFG_hostname" -p $UDHCPC_PID_FILE  --arping -s $UDHCPC_SCRIPT 
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
   ip link set "$SYSCFG_lan_ifname" down
   ip addr flush dev "$SYSCFG_lan_ifname"

   teardown_wireless_interfaces
   teardown_ethernet_interfaces

   # remove interfaces from the bridge
   for loop in $LAN_IFNAMES
   do
      ip link set "$loop" down
      brctl delif "$SYSCFG_lan_ifname" "$loop"
   done
   ip link set "$SYSCFG_wan_physical_ifname" down

   ip link set "$SYSCFG_lan_ifname" down

   brctl delbr "$SYSCFG_lan_ifname"
}

do_start_multi()
{
# TODO: add brport to defaults
PRI_L2=`sysevent get primary_lan_l2net`
sysevent set multinet-start "$PRI_L2"
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
   eval "$FOO"
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

#Create a virtual lan0 management interface and connect it to the bride
#Also prevent this interface from sending any packets to the DOCSIS bridge
virtual_interface()
{
   echo "Inside virtual_interface"
    CMDIAG_IF=`syscfg get cmdiag_ifname`
    LAN_IP=`syscfg get lan_ipaddr`
    LAN_NETMASK=`syscfg get lan_netmask`

    if [ "$1" = "enable" ] ; then
        echo "ip link add $CMDIAG_IF type veth peer name l${CMDIAG_IF}"
        ip link add "$CMDIAG_IF" type veth peer name l"${CMDIAG_IF}"

        echo 1 > /proc/sys/net/ipv6/conf/llan0/disable_ipv6

        echo "ifconfig $CMDIAG_IF hw ether `cat /sys/class/net/lan0/address`"
        ifconfig "$CMDIAG_IF" hw ether "`cat /sys/class/net/${CMDIAG_IF}/address`"
      
        virtual_interface_ebtables_rules enable

        echo "ifconfig l${CMDIAG_IF} promisc up"
        ifconfig l"${CMDIAG_IF}" promisc up

        echo "ifconfig $CMDIAG_IF $LAN_IP netmask $LAN_NETMASK up"
        ifconfig "$CMDIAG_IF" "$LAN_IP" netmask "$LAN_NETMASK" up

        if [ "$LAN_IP" != "$dst_ip" ]; then
                ifconfig "$CMDIAG_IF" $dst_ip netmask "$LAN_NETMASK" up
        fi
    else
        ifconfig "$CMDIAG_IF" down
        ifconfig l"${CMDIAG_IF}" down
        ip link del "$CMDIAG_IF"
        virtual_interface_ebtables_rules disable
    fi
}

virtual_interface_ebtables_rules ()
{
    CMDIAG_IF=`syscfg get cmdiag_ifname`
    CMDIAG_MAC=`cat /sys/class/net/"${CMDIAG_IF}"/address`   
    EROUTER_MAC=`cat /sys/class/net/erouter0/address`
    BRIDGE_NAME=`syscfg get lan_ifname`
    LAN_IP=`syscfg get lan_ipaddr`
     if [ "$1" = "enable" ] ; then
##Filter table
           #--------------------------------------------------------------------------------------
           #####Forward rules for virtual interface(Dont allow lan0 to send traffic to erouter0)
           #--------------------------------------------------------------------------------------

        ebtables -N BRIDGE_FORWARD_FILTER
        ebtables -F BRIDGE_FORWARD_FILTER 2> /dev/null
        ebtables -I FORWARD -j BRIDGE_FORWARD_FILTER

        echo "ebtables -A BRIDGE_FORWARD_FILTER -s $CMDIAG_MAC -o erouter0 -j DROP"
        ebtables -A BRIDGE_FORWARD_FILTER -s "$CMDIAG_MAC" -o erouter0 -j DROP

        echo "ebtables -A BRIDGE_FORWARD_FILTER -j RETURN"
        ebtables -A BRIDGE_FORWARD_FILTER -j RETURN

##NAT TABLE
         #--------------------------------------------------------------------------------------
         ####Redirect traffic destined to lan0 IP to lan0 MAC address from brlan0(Prerouting rules)
         #--------------------------------------------------------------------------------------
        ebtables -t nat -N BRIDGE_REDIRECT
        ebtables -t nat -F BRIDGE_REDIRECT 2> /dev/null
        ebtables -t nat -I PREROUTING -j BRIDGE_REDIRECT

        echo "ebtables -t nat -A BRIDGE_REDIRECT --logical-in $BRIDGE_NAME -p ipv4 --ip-dst $LAN_IP 
              -j dnat --to-destination $CMDIAG_MAC"
        ebtables -t nat -A BRIDGE_REDIRECT --logical-in "$BRIDGE_NAME" -p ipv4 --ip-dst "$LAN_IP" -j dnat --to-destination "$CMDIAG_MAC"

        #echo "ebtables -t nat -A BRIDGE_REDIRECT --logical-in $BRIDGE_NAME -p ipv4 --ip-dst $LAN_IP 
         #    -j forward --forward-dev l$CMDIAG_IF"
        #ebtables -t nat -A BRIDGE_REDIRECT --logical-in $BRIDGE_NAME -p ipv4 --ip-dst $LAN_IP -j forward --forward-dev l$CMDIAG_IF

        echo "ebtables -t nat -A BRIDGE_REDIRECT -j RETURN"
        ebtables -t nat -A BRIDGE_REDIRECT -j RETURN

###BROUTE TABLE
         #--------------------------------------------------------------------------------------
         #DROP target in this BROUTING chain actually broutes the frame(frame has to be routed)
         #--------------------------------------------------------------------------------------
        echo "ebtables -t broute -A BROUTING -i erouter0 -d $EROUTER_MAC -j redirect --redirect-target DROP"
        ebtables -t broute -A BROUTING -i erouter0 -d "$EROUTER_MAC" -j redirect --redirect-target DROP

   else
        echo "ebtables -D FORWARD -j BRIDGE_FORWARD_FILTER"
        ebtables -D FORWARD -j BRIDGE_FORWARD_FILTER

        echo "ebtables -X BRIDGE_FORWARD_FILTER"
        ebtables -X BRIDGE_FORWARD_FILTER

        echo "ebtables -t nat -D PREROUTING -j BRIDGE_REDIRECT"
        ebtables -t nat -D PREROUTING -j BRIDGE_REDIRECT

        echo "ebtables -t nat -X BRIDGE_REDIRECT"
        ebtables -t nat -X BRIDGE_REDIRECT
    fi
}

add_to_group()
{
  bridge_name=`syscfg get lan_ifname`
  
  bridge_dir="/sys/class/net/$bridge_name"
  lan_ethernet_ifname=`syscfg get lan_ethernet_physical_ifnames`

  if [  -d "$bridge_dir" ] ;then
     bridge_status=`cat /sys/class/net/"$bridge_name"/operstate`
     if [ "$bridge_status" = "down" ] ; then
        echo "brctl addbr $bridge_name"
        brctl addbr "$bridge_name"
        ip link set "$bridge_name" up
     fi
  else
        echo "brctl addbr $bridge_name"
        brctl addbr "$bridge_name"
        ip link set "$bridge_name" up
  fi

  ifconfig "$lan_ethernet_ifname" up
  brctl addif "$bridge_name" "$lan_ethernet_ifname"

  cmdiag_if=`syscfg get cmdiag_ifname`
  wan_if=`syscfg get wan_physical_ifname`

  echo "brctl addif brlan0 l$cmdiag_if"
  brctl addif brlan0 l"$cmdiag_if"

  echo "brctl addif brlan0 $wan_if"
  brctl addif brlan0 "$wan_if"

  echo "brctl delif $bridge_name wlan0"
  brctl delif "$bridge_name" wlan0

}
del_from_group()
{
  bridge_name=`syscfg get lan_ifname`
  brctl addif "$bridge_name" wlan0

  cmdiag_if=`syscfg get cmdiag_ifname`
  wan_if=`syscfg get wan_physical_ifname`

  echo "brctl delif brlan0 l$cmdiag_if $wan_if"
  brctl delif brlan0 l"$cmdiag_if" "$wan_if"
}

filter_local_traffic()
{
     if [ "$1" = "enable" ] ; then
        echo "ebtables -N BRIDGE_OUTPUT_FILTER"
        ebtables -N BRIDGE_OUTPUT_FILTER
        ebtables -F BRIDGE_OUTPUT_FILTER 2> /dev/null
        ebtables -I OUTPUT -j BRIDGE_OUTPUT_FILTER

        echo "ebtables -A BRIDGE_OUTPUT_FILTER --logical-out $BRIDGE_NAME -j DROP"
        ebtables -A BRIDGE_OUTPUT_FILTER --logical-out "$BRIDGE_NAME" -j DROP
        echo "ebtables -A BRIDGE_OUTPUT_FILTER -o erouter0 -j DROP"
        ebtables -A BRIDGE_OUTPUT_FILTER -o erouter0 -j DROP

        #Return from filter chain
        echo "ebtables -A BRIDGE_OUTPUT_FILTER -j RETURN"
        ebtables -A BRIDGE_OUTPUT_FILTER -j RETURN
     else
        ebtables -D OUTPUT -j BRIDGE_OUTPUT_FILTER
        ebtables -X BRIDGE_OUTPUT_FILTER
     fi
}


routing_rules(){
    CMDIAG_IF=`syscfg get cmdiag_ifname`
    LAN_IP=`syscfg get lan_ipaddr`
    if [ "$1" = "enable" ] ; then

        #Send responses from $BRIDGE_NAME IP to a separate bridge mode route table
        echo "ip rule add from $LAN_IP lookup $BRIDGE_MODE_TABLE"
        ip rule add from "$LAN_IP" lookup $BRIDGE_MODE_TABLE

        #if [ "$LAN_IP" != "$dst_ip" ]; then
        #        echo "ip rule add from $dst_ip lookup $BRIDGE_MODE_TABLE"
        #        ip rule add from $dst_ip lookup $BRIDGE_MODE_TABLE
        #fi

        echo "ip route add table $BRIDGE_MODE_TABLE default dev $CMDIAG_IF"
        ip route add table $BRIDGE_MODE_TABLE default dev "$CMDIAG_IF"

    else
        echo "ip rule del from $LAN_IP lookup $BRIDGE_MODE_TABLE"
        ip rule del from "$LAN_IP" lookup $BRIDGE_MODE_TABLE

        #if [ $LAN_IP != $dst_ip ]; then
        #        ip rule del from $dst_ip lookup $BRIDGE_MODE_TABLE
        #fi

        echo "ip route flush table $BRIDGE_MODE_TABLE"
        ip route flush table $BRIDGE_MODE_TABLE
    fi
}

block_bridge(){
    ebtables -A FORWARD -i erouter0 -j DROP
}

#Unblock bridged traffic through erouter0
unblock_bridge(){
    ebtables -D FORWARD -i erouter0 -j DROP
}


#--------------------------------------------------------------
# service_start
#--------------------------------------------------------------
service_start ()
{
   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`
   echo "sysevent get ${SERVICE_NAME}-status $STATUS"
   if [ "started" != "$STATUS" ] ; then

         sysevent set ${SERVICE_NAME}-errinfo
         sysevent set ${SERVICE_NAME}-status starting

         block_bridge
		
         virtual_interface enable #create lan0 interface and write ebtable rules

         routing_rules enable

         add_to_group

         filter_local_traffic enable 

         unblock_bridge

         # Force a DHCP renew by issuing a physical link down/up, when WAN port mode switches between bridging and routing
         PSM_MODE=`sysevent get system_psm_mode`
         #if [ "$PSM_MODE" != "1" ]; then
            # It is not a good practice to force all physical links to refresh -- should have used arguments to specify which ports/links
            #gw_lan_refresh
         #fi

       prepare_hostname
       if [  -f /tmp/wifi_initialized ];then
          sysevent set ${SERVICE_NAME}-errinfo
          sysevent set ${SERVICE_NAME}-status started
       else
            sleep 60
            sysevent set ${SERVICE_NAME}-errinfo
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
   #STATUS=`sysevent get ${SERVICE_NAME}-status` 
   #if [ "stopped" != "$STATUS" ] ; then

        sysevent set ${SERVICE_NAME}-errinfo
        sysevent set ${SERVICE_NAME}-status stopping

        block_bridge

        del_from_group

        #Disconnect management interface
        virtual_interface disable
        filter_local_traffic disable
        routing_rules disable

        unblock_bridge

        #Flush connection tracking and packet processor sessions to avoid stale information
        flush_connection_info

        sysevent set ${SERVICE_NAME}-errinfo
        sysevent set ${SERVICE_NAME}-status stopped

#    fi

}

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------
BRIDGE_NAME="$SYSCFG_lan_ifname"
CMDIAG_IF=`syscfg get cmdiag_ifname`

INSTANCE=`sysevent get primary_lan_l2net`
LAN_NETMASK=`syscfg get lan_netmask`


service_init 
echo "service_bridge.sh called with $1 $2" > /dev/console
case "$1" in
   "${SERVICE_NAME}-start")
      firewall firewall-stop
      service_start
      if [ ! -f "$POSTD_START_FILE" ];
      then
          touch $POSTD_START_FILE
          execute_dir /etc/utopia/post.d/
      fi         
      #gw_lan_refresh
      sysevent set firewall-restart
      ;;
   "${SERVICE_NAME}-stop")
        service_stop
        if [ ! -f "$POSTD_START_FILE" ];
        then
              touch $POSTD_START_FILE
              execute_dir /etc/utopia/post.d/
        fi           
        #gw_lan_refresh
        sysevent set firewall-restart

      ;;
   "${SERVICE_NAME}-restart")
      sysevent set lan-restarting 1
      service_stop
      service_start
      sysevent set lan-restarting 0
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
