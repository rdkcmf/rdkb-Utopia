#!/bin/sh
#Script to put the private LAN into pseudo bridge mode
#This script assumes that LAN bridge is already set up with all LAN ports connected

SERVICE_NAME="bridge"

#Which multinet instance is the private LAN?
INSTANCE=1

#Name of the multinet bridge we are putting into this mode
LAN_BRIDGE="`sysevent get multinet_${INSTANCE}-name`"

#IP address of the LAN interface (which will listen for the web UI connections)
LAN_IP="`sysevent get current_lan_ipaddr`"

#Separate routing table used to ensure that responses from the web UI go directly to the LAN interface, not out erouter0 
BRIDGE_MODE_TABLE=69

#Mode passed in by commandline, can be "enable" or "disable"
SCRIPT_MODE="$1"

flush_connection_info(){
	#Flush packet processor sessions on NPCPU
	ncpu_exec -e "(echo flush_all_sessions > /proc/net/ti_pp)"

	#Flush connection tracking entries
	#NOTE: Disabled for now, there is an Intel bug where this command freezes.  Uncomment once that fix is merged to Comcast
	#conntrack -F
}

#Add or remove rules to block local traffic from reaching DOCSIS bridge
#This is to prevent packets coming from APPCPU network stack from corrupting the CPE mac address table
filter_local_traffic(){
	FILTER_MODE=$1

	if [ "$FILTER_MODE" = "enable" ] ; then
	        #Get lbr0 mac address to block / unblock
	        LBR0_MAC="`cat /sys/class/net/lbr0/address`"
	        LLBR0_MAC="`cat /sys/class/net/llbr0/address`"

		#Block locally-generated traffic from LAN bridge from going to DOCSIS bridge and corrupting CPE table
		ebtables -A OUTPUT -o lbr0 -j DROP
		ebtables -N PSEUDO_BRIDGE_FORWARD_FILTER
		ebtables -I FORWARD -j PSEUDO_BRIDGE_FORWARD_FILTER
		ebtables -A PSEUDO_BRIDGE_FORWARD_FILTER -s $LBR0_MAC -j DROP
		ebtables -A PSEUDO_BRIDGE_FORWARD_FILTER -s $LLBR0_MAC -j DROP
		ebtables -A PSEUDO_BRIDGE_FORWARD_FILTER -j RETURN
	else
		#Unblock locally-generated traffic from LAN bridge from going to DOCSIS bridge and corrupting CPE table
		ebtables -D OUTPUT -o lbr0 -j DROP 2> /dev/null
		ebtables -D FORWARD -j PSEUDO_BRIDGE_FORWARD_FILTER 2> /dev/null
		ebtables -X PSEUDO_BRIDGE_FORWARD_FILTER 2> /dev/null
	fi
}

#Temporarily block traffic through lbr0 while reconfiguring rules 
block_bridge(){
	ebtables -A FORWARD -i llbr0 -j DROP
}

#Unblock bridged traffic through lbr0
unblock_bridge(){
	ebtables -D FORWARD -i llbr0 -j DROP
}

#Delete configuration that might apply to an old, stale IP or mac address, in case LAN bridge gets reconfigured
delete_stale_entries(){
	ebtables -t nat -D PREROUTING -j PSEUDO_BRIDGE_REDIRECT 2> /dev/null
	ebtables -t nat -X PSEUDO_BRIDGE_REDIRECT 2> /dev/null
	iptables -D INPUT -i $LAN_BRIDGE -p udp --dport 67 -j DROP 2> /dev/null
	ip rule del lookup $BRIDGE_MODE_TABLE 2> /dev/null
	ip route flush table $BRIDGE_MODE_TABLE 2> /dev/null
	filter_local_traffic disable 2> /dev/null
}

#Enable pseudo bridge mode.  If already enabled, just refresh parameters (in case bridges were torn down and rebuilt)
enable_bridge_mode(){

	block_bridge

	delete_stale_entries

	#Block DHCP requests on $LAN_BRIDGE
	iptables -I INPUT -i $LAN_BRIDGE -p udp --dport 67 -j DROP

	#Receive packets destined to lan IP
	ebtables -t nat -N PSEUDO_BRIDGE_REDIRECT
	ebtables -t nat -I PREROUTING -j PSEUDO_BRIDGE_REDIRECT
	ebtables -t nat -A PSEUDO_BRIDGE_REDIRECT --logical-in $LAN_BRIDGE -p ipv4 --ip-dst $LAN_IP -j redirect --redirect-target ACCEPT
	ebtables -t nat -A PSEUDO_BRIDGE_REDIRECT -j RETURN

	#Send responses from $LAN_BRIDGE IP to a separate bridge mode route table
	ip rule add from $LAN_IP lookup $BRIDGE_MODE_TABLE
	ip route add table $BRIDGE_MODE_TABLE default dev $LAN_BRIDGE

	#Add lbr0 to $LAN_BRIDGE
	vlan_util add_interface $LAN_BRIDGE lbr0

	#Flush connection tracking and packet processor sessions to avoid stale information
	flush_connection_info

	#Block traffic coming from the lbr0 connector interfaces at the MUX
	filter_local_traffic enable

	unblock_bridge	
}

disable_bridge_mode(){

	block_bridge

	delete_stale_entries

	#Remove local bridge connection from 
	vlan_util del_interface $LAN_BRIDGE lbr0

	#Flush connection tracking and packet processor sessions to avoid stale information
	flush_connection_info

	unblock_bridge
}

echo "service_bridge_puma7.sh called with $1 $2" > /dev/console
case "$1" in
   ${SERVICE_NAME}-start)
      enable_bridge_mode
      ;;
   ${SERVICE_NAME}-stop)
      disable_bridge_mode
      ;;
   ${SERVICE_NAME}-restart)
      sysevent set lan-restarting 1
      disable_bridge_mode
      enable_bridge_mode
      sysevent set lan-restarting 0
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac


