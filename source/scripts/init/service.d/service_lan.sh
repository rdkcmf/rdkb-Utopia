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

source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
#source /etc/utopia/service.d/service_lan/wlan.sh
source /etc/utopia/service.d/event_handler_functions.sh
#source /etc/utopia/service.d/service_lan/lan_hooks.sh
#source /etc/utopia/service.d/brcm_ethernet_helper.sh
source /lib/rdk/t2Shared_api.sh

SERVICE_NAME="lan"

subnet() {
    if [ "$2" ]; then
        NM="$2"
    else
        NM="248.0.0.0"
    fi
    if [ "$1" ]; then
        IP="$1"
    else
        IP="255.253.252.100"
    fi
    #
    n="${NM%.*}";m="${NM##*.}"
    l="${IP%.*}";r="${IP##*.}";c=""
    if [ "$m" = "0" ]; then
        c=".0"
        m="${n##*.}";n="${n%.*}"
        r="${l##*.}";l="${l%.*}"
        if [ "$m" = "0" ]; then
            c=".0$c"
            m="${n##*.}";n="${n%.*}"
            r="${l##*.}";l="${l%.*}"
            if [ "$m" = "0" ]; then
                c=".0$c"
                m=$n
                r=$l;l=""
            fi
        fi
    fi
    let s=256-$m
    let r=$r/$s*$s
    if [ "$l" ]; then
        SNW="$l.$r$c"
    else
        SNW="$r$c"
    fi

    echo $SNW
}

isvalid() {
	if [ $1 -eq 255 ] || [ $1 -eq 254 ] || [ $1 -eq 252 ] || [ $1 -eq 248 ] || [ $1 -eq 240 ] || [ $1 -eq 224 ] || [ $1 -eq 192 ] || [ $1 -eq 128 ] || [ $1 -eq 0 ]; then
		echo 1
	else
		echo 0
	fi
}

calbits()
{
	count=0
	bitfield=$1
	while [ $bitfield -ne 0 ]; do
		bitfield_1=`expr "$bitfield" - 1`
		bitfield=$((bitfield & bitfield_1))
		let count+=1
	done
	echo "$count"
}

mask2cidr() {
	numberofbits=0
	fields=`echo "$1" | sed 's/\./ /g'`
	for field in $fields ; do
		if [ "`isvalid "$field"`" -eq 1 ]; then
			numberofbits=$((numberofbits + `calbits "$field"`))
		else
			echo "Error: $field is not recognised"; exit 1
		fi
	done
	echo $numberofbits
}

#--------------------------------------------------------------
# Enslave a physical or virtual interface to a bridge
# 
# Takes parameters : 
#   $1  : the name of the interface to enslave
#   $2  : the name of the interface to enslave it to
#--------------------------------------------------------------
enslave_a_interface() {
   # ulog lan status "Enslaving interface $1 to $2"
#   echo "[lan] enslave_a_interface called" > /dev/console
   
   brctl addif "$2" "$1"
}

#--------------------------------------------------------------
# Bring up the ethernet interfaces
#--------------------------------------------------------------
bringup_ethernet_interfaces() {
   # if we are using virtual ethernet interfaces then
   # create them now
#   echo "[lan] bringup_ethernet_interfaces called" > /dev/console
   if [ -n "$SYSCFG_lan_ethernet_virtual_ifnums" ] ; then
       for loop in $SYSCFG_lan_ethernet_physical_ifnames
       do
         config_vlan "$loop" "$SYSCFG_lan_ethernet_virtual_ifnums"
       done
   fi

   for loop in $SYSCFG_lan_ethernet_physical_ifnames
   do
      ifconfig "$loop" 0.0.0.0
      ip link set "$loop" allmulticast on
   done
}

#--------------------------------------------------------------
# Tear down the ethernet interfaces
#--------------------------------------------------------------
teardown_ethernet_interfaces() {
#   echo "[lan] teardown_ethernet_interfaces called" > /dev/console
   if [ -z "$SYSCFG_lan_ethernet_virtual_ifnums" ] ; then
      for loop in $SYSCFG_lan_ethernet_physical_ifnames
      do
         ip link set "$loop" down
      done
   else
      # if we are using virtual ethernet interfaces then
      # tear them down
      for loop in $SYSCFG_lan_ethernet_virtual_ifnums
      do
         unconfig_vlan "$loop"
      done
   fi
}

#--------------------------------------------------------------
# Bring up the wireless interfaces
#--------------------------------------------------------------
bringup_wireless_interfaces() {

    WIFI_IF_INDEX=1

    if [ -n "$SYSCFG_lan_wl_physical_ifnames" ] ; then
        for loop in $SYSCFG_lan_wl_physical_ifnames
        do
            ### Set the main interface (eth*) ###

            MAC=`syscfg get macwifi0${WIFI_IF_INDEX}bssid1`
            OUI=`cat /sys/class/net/"$loop"/address|awk -F: '{print $1 $2 $3}'`
            NIC=`cat /sys/class/net/"$loop"/address|awk -F: '{print $4 $5 $6}'`
            if [ -n "$MAC" ]; then
                ulog lan status "setting $loop hw address to $MAC"
            else
                MAC=$OUI$NIC
                ulog lan status "setting $loop hw address to default ($MAC)"
            fi
            ifconfig "$loop" hw ether "$MAC"
            ip link set "$loop" allmulticast on

            ### Set the virtual interfaces (wl*.*) ###

            WIFI_VIF_INDEX=1
            VIFS=`syscfg get wl$(($WIFI_IF_INDEX-1))_virtual_ifnames`

            if [ -n "$VIFS" ]; then
                for vif in $VIFS
                do
                    # Create a virtual interface in prompt first 
                    wl -i "$loop" ssid -C $WIFI_VIF_INDEX "" 

                    MAC=`syscfg get macwifi0${WIFI_IF_INDEX}bssid$(($WIFI_VIF_INDEX+1))`
                    if [ -n "$MAC" ]; then
                        ulog lan status "setting $vif hw address to $MAC"
                    else
                        MAC=$OUI`printf "%06x" $((0x$NIC+$WIFI_VIF_INDEX))`
                        ulog lan status "setting $vif hw address to default ($MAC)"
                    fi
                    ifconfig "$vif" hw ether "$MAC"
                    ip link set "$vif" allmulticast on

                    WIFI_VIF_INDEX=`expr $WIFI_VIF_INDEX + 1`
                done
            fi

            ### Bring up the interfaces ###

            ulog lan status "wlancfg $loop up"
            wlancfg "$loop" up

            ip link set "$loop" up
            if [ -n "$VIFS" ] ; then
                for vif in $VIFS
                do
                    ip link set "$vif" up
                done
            fi

            WIFI_IF_INDEX=`expr $WIFI_IF_INDEX + 1`
        done
    fi
}

#--------------------------------------------------------------
# Teardown the wireless interfaces
#--------------------------------------------------------------
teardown_wireless_interfaces() {

    WIFI_IF_INDEX=1

    if [ -n "$SYSCFG_lan_wl_physical_ifnames" ] ; then
        for loop in $SYSCFG_lan_wl_physical_ifnames
        do
            VIFS=`syscfg get wl$(($WIFI_IF_INDEX-1))_virtual_ifnames`
            if [ -n "$VIFS" ] ; then
                for vif in $VIFS
                do
                    ip link set "$vif" down
                done
            fi

            ulog lan status "wlancfg $loop down"
            wlancfg "$loop" down
            ip link set "$loop" down

            WIFI_IF_INDEX=`expr $WIFI_IF_INDEX + 1`
        done
    fi

    #teardown_wireless_daemons
}

#When we do do_start, it will delete all IPs on brlan0 and add back again
#This may make some services on brlan0 doesn't work
#this function is used to restart such services.
restart_necessary_functions()
{
   echo "NOT IMPLEMENTED"
}

#--------------------------------------------------------------
# do_start
#--------------------------------------------------------------
do_start()
{
#   echo "[lan] do start called" > /dev/console
   ulog lan status "bringing up lan interface"

   # in case it takes a long time to get a lease, we prepare a very
   # minimal resolv.conf file that assumes the dns is on the local interface
   DNS_CONF=`cat /etc/resolv.conf`
#song:   if [ -z "$DNS_CONF" -a  -n "$SYSCFG_lan_ipaddr" ]; then
#song:      echo "nameserver $SYSCFG_lan_ipaddr" >> /etc/resolv.conf
#song:   fi
   bringup_ethernet_interfaces
   bringup_wireless_interfaces
   
#song:   brctl addbr $SYSCFG_lan_ifname
#song:   brctl setfd $SYSCFG_lan_ifname 0
#song:   brctl stp $SYSCFG_lan_ifname off

   # enslave interfaces to the bridge
#song:   for loop in $LAN_IFNAMES
#song:   do
      # disable IPv6 for bridge port so it won't interfere bridge interface's DAD procedure
#song:      echo 1 > /proc/sys/net/ipv6/conf/$loop/disable_ipv6
#song:      enslave_a_interface $loop $SYSCFG_lan_ifname
#song:   done

   # For USGv2 Only: setup policy routing for packet from LAN interface
#song:   ip rule add iif $SYSCFG_lan_ifname lookup erouter
   ip rule add from "$SYSCFG_lan_ipaddr" lookup erouter

   SYSEVT_lan_ipaddr_v6=`sysevent get lan_ipaddr_v6`
   SYSEVT_lan_prefix_v6=`sysevent get lan_prefix_v6`

   # bring up the bridge
   ip addr add "$SYSCFG_lan_ipaddr"/"$SYSCFG_lan_netmask" broadcast + dev "$SYSCFG_lan_ifname"
   ip -6 addr add "$SYSEVT_lan_ipaddr_v6"/"$SYSEVT_lan_prefix_v6" dev "$SYSCFG_lan_ifname" valid_lft forever preferred_lft forever
   ip link set "$SYSCFG_lan_ifname" up 
   ip link set "$SYSCFG_lan_ifname" allmulticast on 
   echo 1 > /proc/sys/net/ipv6/conf/"$SYSCFG_lan_ifname"/autoconf
   echo 1 > /proc/sys/net/ipv6/conf/"$SYSCFG_lan_ifname"/disable_ipv6
   echo 0 > /proc/sys/net/ipv6/conf/"$SYSCFG_lan_ifname"/disable_ipv6

   # For USGv2 Only: setup policy routing for packet to LAN interface (from eRouter WAN Interface IP address)
   SUBNET=`subnet "$SYSCFG_lan_ipaddr" "$SYSCFG_lan_netmask"`
   MASKBITS=`mask2cidr "$SYSCFG_lan_netmask"`
   ip route add table erouter "$SUBNET"/"$MASKBITS" dev "$SYSCFG_lan_ifname"

   ulog lan status "switch off bridge pkts to iptables (bridge-nf-call-arptables)"
   #echo 0 > /proc/sys/net/bridge/bridge-nf-call-arptables
   #echo 0 > /proc/sys/net/bridge/bridge-nf-call-iptables
   #echo 0 > /proc/sys/net/bridge/bridge-nf-call-ip6tables
   #echo 0 > /proc/sys/net/bridge/bridge-nf-filter-vlan-tagged
   #echo 0 > /proc/sys/net/bridge/bridge-nf-filter-pppoe-tagged

 #  vendor_block_dos_land_attack

   # bringup_wireless_daemons
   
   # enable ipv6 forwarding
   echo 1 > /proc/sys/net/ipv6/conf/all/forwarding

   #lan_if_up_hook

   sysevent set desired_moca_link_state up
   sysevent set current_lan_ipaddr "$SYSCFG_lan_ipaddr"
   
   #to add ipv6 prefix for this interface
   LAN_IPV6_PREFIX=`sysevent get ipv6_prefix`
   if [ -n "$LAN_IPV6_PREFIX" ] ; then
        ip -6 route add "$LAN_IPV6_PREFIX" dev "$SYSCFG_lan_ifname"
   fi
#song:add	
	ip link set lbr0 up

   restart_necessary_functions

   ulog lan status "lan interface up"
}

#--------------------------------------------------------------
# do_stop
#--------------------------------------------------------------
do_stop()
{
#   echo "[lan] do stop called" > /dev/console
   sysevent set desired_moca_link_state down
   
   OLDIP=`ip addr show dev "$SYSCFG_lan_ifname" label "$SYSCFG_lan_ifname" | grep "inet " | awk '{split($2,foo, "/"); print(foo[1]);}'`
   ip link set "$SYSCFG_lan_ifname" down
   ip addr flush dev "$SYSCFG_lan_ifname"

   teardown_wireless_interfaces
   teardown_ethernet_interfaces

   # remove interfaces from the bridge
   for loop in $LAN_IFNAMES
   do
      ip link set "$loop" down
#song:      brctl delif $SYSCFG_lan_ifname $loop
   done

   # For USGv2 Only: remove bridge to lbr0 and associated ebtable rules
#song:   brctl delif $SYSCFG_lan_ifname lbr0
   ip link set lbr0 down
#song:   ebtables -D OUTPUT -o lbr0 -j DROP
#song:   ebtables -D INPUT -i lbr0 -j DROP
#song:   ebtables -D FORWARD -i lbr0 -j DROP
#song:   ebtables -D FORWARD -o lbr0 -j DROP
#song:   ebtables -D FORWARD -i lbr0 -p ARP --arp-ip-src 192.168.100.1 -j ACCEPT
#song:   ebtables -D FORWARD -o lbr0 -p ARP --arp-ip-dst 192.168.100.1 -j ACCEPT
#song:   ebtables -D FORWARD -i lbr0 -p IPv4 --ip-src 192.168.100.1 -j ACCEPT
#song:   ebtables -D FORWARD -o lbr0 -p IPv4 --ip-dst 192.168.100.1 -j ACCEPT

   # For USGv2 Only: remove policy routing for packet from/to LAN interface
#song:   ip rule del iif $SYSCFG_lan_ifname lookup erouter
   ip rule del from "$SYSCFG_lan_ipaddr" lookup erouter
   # For USGv2 Only: remove old policy route for packet from GW interface
   if [ "$OLDIP" != "$SYSCFG_lan_ipaddr" -a "" != "$OLDIP" ]; then
      ip rule del from "$OLDIP" lookup erouter
   fi
   
   SUBNET=`subnet "$SYSCFG_lan_ipaddr" "$SYSCFG_lan_netmask"` 
   MASKBITS=`mask2cidr "$SYSCFG_lan_netmask"` 
   ip route del table erouter "$SUBNET"/"$MASKBITS"

#song   ip link set $SYSCFG_lan_ifname down
#song:   brctl delbr $SYSCFG_lan_ifname
}

#--------------------------------------------------------------
# do_start_no_bridge
#--------------------------------------------------------------
do_start_no_bridge()
{
#   echo "[lan] do_start_no_bridge called" > /dev/console
   echo_t "SYSCFG_lan_dhcp_client = $SYSCFG_lan_dhcp_client"
   if [ "1" = "$SYSCFG_lan_dhcp_client" ] ; then
#      echo "starting up lan dhcp" > /dev/console
      ulog lan status "bringing up lan interface via dhcp"
      do_start_lan_dhcp
   else
      ulog lan status "bringing up lan interface"

      # bring up without the bridge
      ip addr add "$SYSCFG_lan_ipaddr"/"$SYSCFG_lan_netmask" broadcast + dev "$SYSCFG_lan_ifname"
      ip link set "$SYSCFG_lan_ifname" up 
      #ip link set $SYSCFG_lan_ifname allmulticast on 

      # Maybe this forwarding enable should be in wanControl???
      echo 1 > /proc/sys/net/ipv6/conf/all/forwarding

      sysevent set current_lan_ipaddr "$SYSCFG_lan_ipaddr"
      
      ulog lan status "lan interface up"
   fi

   sysevent set desired_moca_link_state up
}

#--------------------------------------------------------------
# do_stop_no_bridge
#--------------------------------------------------------------
do_stop_no_bridge()
{
#   echo "[lan] do stop no bridge called" > /dev/console
   if [ "1" = "$SYSCFG_lan_dhcp_client" ] ; then
      do_stop_lan_dhcp
   else
      sysevent set desired_moca_link_state down
   
      ip link set "$SYSCFG_lan_ifname" down
      ip addr flush dev "$SYSCFG_lan_ifname"
   fi
}

#--------------------------------------------------------------
# do_start_lan_dhcp
#--------------------------------------------------------------
do_start_lan_dhcp()
{
   ip addr flush dev "$SYSCFG_lan_ifname"
   
   unregister_lan_dhcp_handler
   register_lan_dhcp_handler

   sysevent set current_lan_ipaddr 0.0.0.0
   ip link set "$SYSCFG_lan_ifname" up
   ip link set "$SYSCFG_lan_ifname" multicast on

   echo 1 > /proc/sys/net/ipv6/conf/all/forwarding

   #lan_if_up_hook

   ulog lan status "$PID setting desired_lan_dhcp_link up"
#   echo "setting desired lan dhcp link up" > /dev/console
   sysevent set desired_lan_dhcp_link up
}

#--------------------------------------------------------------
# do_stop_lan_dhcp
#--------------------------------------------------------------
do_stop_lan_dhcp()
{
   ulog lan status "$PID tearing down lan (dhcp)"
 #  echo "$PID tearing down lan (dhcp)" > /dev/console
   sysevent set desired_lan_dhcp_link down

   # the lan dhcp handler will set lan-status to stopped when it is done
   DONE=`sysevent get lan-status`
   while [ "stopped" != "$DONE" ]
   do
      sleep 1
      DONE=`sysevent get lan-status`
   done

   unregister_lan_dhcp_handler

   ip link set "$SYSCFG_lan_ifname" down
   ip addr flush dev "$SYSCFG_lan_ifname"
}

HANDLER="/etc/utopia/service.d/service_lan/dhcp_lan.sh"
#--------------------------------------------------------------
# register_lan_dhcp_handler
#--------------------------------------------------------------
register_lan_dhcp_handler()
{
   ulog lan status "$PID installing handlers for dhcp_lan"
#   echo "$PID installing handlers for dhcp_lan" > /dev/console

   asyncid=`sysevent async desired_lan_dhcp_link "$HANDLER"`
   sysevent set ${SERVICE_NAME}_desired_lan_dhcp_link_asyncid "$asyncid"
   
   asyncid=`sysevent async dhcp_lan-restart "$HANDLER"`
   sysevent setoptions dhcp_lan-restart "$TUPLE_FLAG_EVENT"
   sysevent set ${SERVICE_NAME}_dhcp_async_1 "$asyncid"

   asyncid=`sysevent async dhcp_lan-renew "$HANDLER"`
   sysevent setoptions dhcp_lan-renew "$TUPLE_FLAG_EVENT"
   sysevent set ${SERVICE_NAME}_dhcp_async_2 "$asyncid"

   asyncid=`sysevent async dhcp_lan-release "$HANDLER"`
   sysevent setoptions dhcp_lan-release "$TUPLE_FLAG_EVENT"
   sysevent set ${SERVICE_NAME}_dhcp_async_3 "$asyncid"

   asyncid=`sysevent async lan-status "$HANDLER"`
   sysevent set ${SERVICE_NAME}_dhcp_async_4 "$asyncid"
}

#--------------------------------------------------------------
# unregister_lan_dhcp_handler
#--------------------------------------------------------------
unregister_lan_dhcp_handler()
{
   ulog lan status "$PID uninstalling handlers for dhcp_lan"
#   echo "$PID uninstalling handlers for dhcp_lan" > /dev/console

   asyncid=`sysevent get ${SERVICE_NAME}_desired_lan_dhcp_link_asyncid`;
   if [ -n "$asyncid" ] ; then
      sysevent rm_async "$asyncid"
      sysevent set ${SERVICE_NAME}_desired_lan_dhcp_link_asyncid
   fi
   
   asyncid=`sysevent get ${SERVICE_NAME}_dhcp_async_1`;
   if [ -n "$asyncid" ] ; then
      sysevent rm_async "$asyncid"
      sysevent set ${SERVICE_NAME}_dhcp_async_1
   fi

   asyncid=`sysevent get ${SERVICE_NAME}_dhcp_async_2`;
   if [ -n "$asyncid" ] ; then
      sysevent rm_async "$asyncid"
      sysevent set ${SERVICE_NAME}_dhcp_async_2
   fi

   asyncid=`sysevent get ${SERVICE_NAME}_dhcp_async_3`;
   if [ -n "$asyncid" ] ; then
      sysevent rm_async "$asyncid"
      sysevent set ${SERVICE_NAME}_dhcp_async_3
   fi

   asyncid=`sysevent get ${SERVICE_NAME}_dhcp_async_4`;
   if [ -n "$asyncid" ] ; then
      sysevent rm_async "$asyncid"
      sysevent set ${SERVICE_NAME}_dhcp_async_4
   fi
}

#----------------------------------------------------------------------------
# standard functions
#----------------------------------------------------------------------------

lan_create () {
    brctl addbr "$SYSCFG_lan_ifname"
    brctl setfd "$SYSCFG_lan_ifname" 0
    brctl stp "$SYSCFG_lan_ifname" off

    # enslave interfaces to the bridge
    for loop in $LAN_IFNAMES
    do
      # disable IPv6 for bridge port so it won't interfere bridge interface's DAD procedure
      echo 1 > /proc/sys/net/ipv6/conf/"$loop"/disable_ipv6
      enslave_a_interface "$loop" "$SYSCFG_lan_ifname"
    done

    # For USGv2 Only: setup policy routing for packet from LAN interface
    ip rule add iif "$SYSCFG_lan_ifname" lookup erouter
    
    register_docsis_handler
    add_docsis_bridge
}

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   # Get all provisioning data
   # Figure out the names and addresses of the lan interface
   #
   # SYSCFG_lan_ethernet_virtual_ifnums is the vlan nums of all ethernet switchs that 
   # will be part of the lan
   # If vlans are not used to separate the physical ethernet switch into 
   # a wan port and lan ports, then LAN_ETHERNET_VIRTUAL_IFNUMS will be blank
   #
   # SYSCFG_lan_ethernet_physical_ifnames is the physical ethernet interfaces that
   # will be part of the lan
   #
   # SYSCFG_lan_wl_physical_ifnames is the names of each wireless interface as known
   # to the operating system

   SYSCFG_FAILED='false'
   FOO=`utctx_cmd get lan_ifname lan_ethernet_virtual_ifnums lan_ethernet_physical_ifnames lan_wl_physical_ifnames lan_ipaddr lan_netmask lan_dhcp_client`
   eval "$FOO"
  if [ $SYSCFG_FAILED = 'true' ] ; then
     ulog lan status "$PID utctx failed to get some configuration data"
     ulog lan status "$PID LAN CANNOT BE CONTROLLED"
     exit
  fi

   #figure out the interfaces that are part of the lan
   # if we are not using virtual ethernet interfaces then
   # use the physical ethernet names
   if [ -z "$SYSCFG_lan_ethernet_virtual_ifnums" ] ; then
      LAN_IFNAMES="$SYSCFG_lan_ethernet_physical_ifnames"
    else
       # in this architecture, vlans are used to separate the physical
       # ethernet switch into a wan port and lan ports.
       for loop in $SYSCFG_lan_ethernet_physical_ifnames
       do
         LAN_IFNAMES="$LAN_IFNAMES vlan$SYSCFG_lan_ethernet_virtual_ifnums"
       done
   fi

   # if we are using wireless interfafes then add them
   if [ -n "$SYSCFG_lan_wl_physical_ifnames" ] ; then
      LAN_IFNAMES="$LAN_IFNAMES $SYSCFG_lan_wl_physical_ifnames"
   fi

   # TODO: have to create additional bridges and move vifs to them
   VIFS0=`syscfg get "wl0_virtual_ifnames"`
   if [ -n "$VIFS0" ] ; then
      LAN_IFNAMES="$LAN_IFNAMES $VIFS0"
   fi
   VIFS1=`syscfg get "wl1_virtual_ifnames"`
   if [ -n "$VIFS1" ] ; then
      LAN_IFNAMES="$LAN_IFNAMES $VIFS1"
   fi
   
  LAN_CREATED=`sysevent get lan_created`
	 if [ -z "$LAN_CREATED" ] ; then
	 		lan_create
	 		sysevent set lan_created 1
	 fi

}

#--------------------------------------------------------------
# service_start
#--------------------------------------------------------------
service_start ()
{
#   echo "lan service start called" > /dev/console
#song:   register_docsis_handler
   wait_till_end_state ${SERVICE_NAME}

   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "started" != "$STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status starting
      if [ -n "$LAN_IFNAMES" ]; then
         do_start
      else
         do_start_no_bridge
      fi
      ERR=$?
      if [ "$ERR" -ne "0" ] ; then
         check_err $? "Unable to bringup lan"
      elif [ "" = "$SYSCFG_lan_dhcp_client" -o "0" = "$SYSCFG_lan_dhcp_client" ] ; then
         sysevent set ${SERVICE_NAME}-errinfo
         sysevent set ${SERVICE_NAME}-status started
#song:         add_docsis_bridge
         echo_t "service_lan : Triggering RDKB_FIREWALL_RESTART"
	 t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
         sysevent set firewall-restart
      fi
	  #rongwei added
#	  killall wecb_master 2>/dev/null
	  killall CcspHomeSecurity 2>/dev/null
#	  ulimit -s 200 && wecb_master& 
	  sleep 1
	  ###echo `ps | grep wecb_master | grep -v grep | awk '{print $1}'` > /var/run/wecb_master.pid
#	  echo '#!/bin/sh' > /var/wecb_master.sh
#	  echo 'ulimit -s 200 && wecb_master&' >> /var/wecb_master.sh 
#	  chmod +x /var/wecb_master.sh
#	  /etc/utopia/service.d/pmon.sh register wecb_master
#	  /etc/utopia/service.d/pmon.sh setproc wecb_master wecb_master /var/run/wecb_master.pid "/var/wecb_master.sh" 
	  CcspHomeSecurity 8081&
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
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status stopping
      if [ -n "$LAN_IFNAMES" ]; then
         do_stop
      else
         do_stop_no_bridge
      fi
      ERR=$?
      if [ "$ERR" -ne "0" ] ; then
         check_err $ERR "Unable to teardown lan"
      elif [ "" = "$SYSCFG_lan_dhcp_client" -o "0" = "$SYSCFG_lan_dhcp_client" ] ; then
         sysevent set ${SERVICE_NAME}-errinfo
         sysevent set ${SERVICE_NAME}-status stopped
      fi
	  #unregister wecb_master from pmon to let this script to bring it up when lan restart.
	  /etc/utopia/service.d/pmon.sh unregister wecb_master 
	  #rongwei added
#	  killall wecb_master 2>/dev/null
	  killall CcspHomeSecurity 2>/dev/null
   fi
}


add_docsis_bridge () 
{
	 VAR=`sysevent get lan_created`
	 if [ -n "$VAR" ] ; then
	 		exit
	 fi

         brctl addif "$SYSCFG_lan_ifname" lbr0

         # leichen2: move all ebtables rule to ebtable_rules.sh
         #ebtables -A OUTPUT -o lbr0 -j DROP
         #ebtables -A INPUT -i lbr0 -j DROP
         #ebtables -I FORWARD -i lbr0 -j DROP
         #ebtables -I FORWARD -o lbr0 -j DROP
         #ebtables -I FORWARD -i lbr0 -p ARP --arp-ip-src 192.168.100.1 -j ACCEPT
         #ebtables -I FORWARD -o lbr0 -p ARP --arp-ip-dst 192.168.100.1 -j ACCEPT
         #ebtables -I FORWARD -i lbr0 -p IPv4 --ip-source 192.168.100.1 -j ACCEPT
         #ebtables -I FORWARD -o lbr0 -p IPv4 --ip-destination 192.168.100.1 -j ACCEPT
         /etc/utopia/service.d/ebtable_rules.sh

}

register_docsis_handler () {
    ID=`sysevent get lan_docsis_async`
    if [ x = x"$ID" ] ; then
       ID=`sysevent async docsis-initialized /etc/utopia/service.d/service_lan.sh`
       sysevent set lan_docsis_async "$ID"
    fi
}


#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------

service_init 

case "$1" in
   "${SERVICE_NAME}-start")
      service_start
      ;;
   "${SERVICE_NAME}-stop")
      service_stop
      ;;
   "${SERVICE_NAME}-restart")
      echo "service_init : setting lan-restarting to 1"
      sysevent set lan-restarting 1
      service_stop
      service_start
      echo "service_init : setting lan-restarting to 0"
      sysevent set lan-restarting 0
      ;;
   docsis-initialized)
         # For USGv2 Only: allow LAN site access to CM Diagnostic IP 192.168.100.1
      add_docsis_bridge
      ;;
   *)   
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
