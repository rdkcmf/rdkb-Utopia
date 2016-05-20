#!/bin/sh
#Script to put the private LAN into pseudo bridge mode

#source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/hostname_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
#source /etc/utopia/service.d/service_lan/wlan.sh
source /etc/utopia/service.d/event_handler_functions.sh
#source /etc/utopia/service.d/brcm_ethernet_helper.sh

SERVICE_NAME="bridge"

#Which multinet instance is the private LAN?
INSTANCE=1

#TODO: Should we get these from CCSP instead of hardcoding them?
LAN_IP="10.0.0.1"
LAN_NETMASK="255.255.255.0"

#Separate routing table used to ensure that responses from the web UI go directly to the LAN interface, not out erouter0 
BRIDGE_MODE_TABLE=69

#Mode passed in by commandline, can be "enable" or "disable"
SCRIPT_MODE="$1"

wait_till_steady_state ()
{
	LSERVICE=$1
	TRIES=1
	while [ "30" -ge "$TRIES" ] ; do
		LSTATUS=`sysevent get ${LSERVICE}-status`
		if [ "starting" = "$LSTATUS" ] || [ "stopping" = "$LSTATUS" ] || [ "partial" = "$LSTATUS" ] ; then
			sleep 1
			TRIES=`expr $TRIES + 1`
		else
			return
		fi
	done
	echo "$0: Timed out waiting for $LSERVICE to be in a steady state"
}

wait_till_started ()
{
	LSERVICE=$1
	TRIES=1
	while [ "30" -ge "$TRIES" ] ; do
		LSTATUS=`sysevent get ${LSERVICE}-status`
		if [ "started" = "$LSTATUS" -o "ready" = "$LSTATUS" ] ; then
			return
		else
			sleep 1
			TRIES=`expr $TRIES + 1`
		fi
	done
	echo "$0: Timed out waiting for $LSERVICE to be started or ready"
}


wait_till_stopped()
{
	LSERVICE=$1
	TRIES=1
	while [ "30" -ge "$TRIES" ] ; do
		LSTATUS=`sysevent get ${LSERVICE}-status`
		if [ "stopped" = "$LSTATUS" ] ; then
			return
		else
			sleep 1
			TRIES=`expr $TRIES + 1`
		fi
	done
	echo "$0: Timed out waiting for $LSERVICE to be stopped"
}

flush_connection_info(){
	#Flush packet processor sessions on NPCPU
	ncpu_exec -e "(echo flush_all_sessions > /proc/net/ti_pp)"

	#Flush CPE table
	ncpu_exec -e "(echo \"LearnFrom=CPE_DYNAMIC\" > /proc/net/dbrctl/delalt)"

	#Flush connection tracking entries
	#NOTE: Disabled for now, there is an Intel bug where this command freezes.  Uncomment once that fix is merged to Comcast
	#conntrack -F
}

#Add or remove rules to block local traffic from reaching DOCSIS bridge
filter_local_traffic(){
	FILTER_MODE=$1

	if [ "$FILTER_MODE" = "enable" ] ; then
		#Block locally-generated traffic from LAN bridge from going to DOCSIS bridge and corrupting CPE table
		ebtables -A OUTPUT -o lbr0 -j DROP
	else
		#Unblock locally-generated traffic from LAN bridge from going to DOCSIS bridge and corrupting CPE table
		ebtables -D OUTPUT -o lbr0 -j DROP 2> /dev/null
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

restart_webgui()
{
	killall lighttpd
	/etc/webgui.sh
}

do_start_multi()
{
	PRI_L2=`sysevent get primary_lan_l2net`
	#This is to address a BUG in vlan_util_xb6.sh that it doesn't set state to starting
	sysevent set multinet-start $PRI_L2
	wait_till_started multinet_${PRI_L2}
}

do_stop_multi()
{
	PRI_L2=`sysevent get primary_lan_l2net`
	sysevent set multinet-stop $PRI_L2
	wait_till_stopped multinet_${PRI_L2}
}


cmdiag_ebtables_rules()
{
	if [ "$1" = "enable" ] ; then
		CMDIAG_MAC="`cat /sys/class/net/lan0/address`"

		#Don't allow lan0 to send traffic to DOCSIS bridge
		ebtables -N BRIDGE_FORWARD_FILTER
		ebtables -F BRIDGE_FORWARD_FILTER 2> /dev/null
		ebtables -I FORWARD -j BRIDGE_FORWARD_FILTER
		ebtables -A BRIDGE_FORWARD_FILTER -s $CMDIAG_MAC -o lbr0 -j DROP
		ebtables -A BRIDGE_FORWARD_FILTER -j RETURN

		#Redirect traffic destined to lan0 IP to lan0 MAC address
		ebtables -t nat -N BRIDGE_REDIRECT
		ebtables -t nat -F BRIDGE_REDIRECT 2> /dev/null
		ebtables -t nat -I PREROUTING -j BRIDGE_REDIRECT
		ebtables -t nat -A BRIDGE_REDIRECT -p ipv4 --ip-dst $LAN_IP -j dnat --to-destination $CMDIAG_MAC
		ebtables -t nat -A BRIDGE_REDIRECT -j RETURN
	else
		ebtables -D FORWARD -j BRIDGE_FORWARD_FILTER
		ebtables -X BRIDGE_FORWARD_FILTER
		ebtables -t nat -D PREROUTING -j BRIDGE_REDIRECT
		ebtables -t nat -X BRIDGE_REDIRECT
	fi
}

#Create a virtual lan0 management interface and connect it to the bride
#Also prevent this interface from sending any packets to the DOCSIS bridge
cmdiag_if()
{
	if [ "$1" = "enable" ] ; then
		ip link add $CMDIAG_IF type veth peer name l${CMDIAG_IF} 
		echo 1 > /proc/sys/net/ipv6/conf/llan0/disable_ipv6
		ifconfig $CMDIAG_IF hw ether $CMDIAG_MAC
		cmdiag_ebtables_rules enable
		ifconfig l${CMDIAG_IF} promisc up
		ifconfig $CMDIAG_IF $LAN_IP netmask $LAN_NETMASK up
		vlan_util add_interface $LAN_BRIDGE l${CMDIAG_IF}
	else
		vlan_util del_interface $LAN_BRIDGE l${CMDIAG_IF}
		ifconfig $CMDIAG_IF down
		ifconfig l${CMDIAG_IF} down
		ip link del $CMDIAG_IF
		cmdiag_ebtables_rules disable
	fi
}

#Enable pseudo bridge mode.  If already enabled, just refresh parameters (in case bridges were torn down and rebuilt)
service_start(){
	wait_till_steady_state ${SERVICE_NAME}
	STATUS=`sysevent get ${SERVICE_NAME}-status`
	if [ "started" != "$STATUS" ] ; then
		do_start_multi

		sysevent set ${SERVICE_NAME}-errinfo
		sysevent set ${SERVICE_NAME}-status starting

		block_bridge

		#Block DHCP requests on $LAN_BRIDGE
		iptables -I INPUT -i $CMDIAG_IF -p udp --dport 67 -j DROP

		#Connect management interface
		cmdiag_if enable

		#Send responses from $LAN_BRIDGE IP to a separate bridge mode route table
		ip rule add from $LAN_IP lookup $BRIDGE_MODE_TABLE
		ip route add table $BRIDGE_MODE_TABLE default dev $CMDIAG_IF

		#Add lbr0 to $LAN_BRIDGE
		vlan_util add_interface $LAN_BRIDGE lbr0

		#Flush connection tracking and packet processor sessions to avoid stale information
		flush_connection_info
	
		#Block traffic coming from the lbr0 connector interfaces at the MUX
		filter_local_traffic enable

		unblock_bridge

		prepare_hostname

		restart_webgui

		gw_lan_refresh

		sysevent set ${SERVICE_NAME}-errinfo
		sysevent set ${SERVICE_NAME}-status started
		
	fi
}

service_stop(){
	wait_till_steady_state ${SERVICE_NAME}
	STATUS=`sysevent get ${SERVICE_NAME}-status`
	if [ "stopped" != "$STATUS" ] ; then

		sysevent set ${SERVICE_NAME}-errinfo
		sysevent set ${SERVICE_NAME}-status stopping

		block_bridge

		#Remove local bridge connection from 
		vlan_util del_interface $LAN_BRIDGE lbr0

		#Disconnect management interface
		cmdiag_if disable
		filter_local_traffic disable
	
		#Flush connection tracking and packet processor sessions to avoid stale information
		flush_connection_info

		unblock_bridge
		do_stop_multi

		iptables -D INPUT -i $CMDIAG_IF -p udp --dport 67 -j DROP

		restart_webgui

		gw_lan_refresh

		sysevent set ${SERVICE_NAME}-errinfo
		sysevent set ${SERVICE_NAME}-status stopped

	fi
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
   if [ "" != "$SYSCFG_lan_wl_physical_ifnames" ] ; then
      LAN_IFNAMES="$LAN_IFNAMES $SYSCFG_lan_wl_physical_ifnames"
   fi
}


echo "service_bridge_puma7.sh called with $1 $2" > /dev/console
service_init

LAN_BRIDGE="$SYSCFG_lan_ifname"
CMDIAG_IF=`syscfg get cmdiag_ifname`
CMDIAG_MAC=`ncpu_exec -ep "(cat /sys/class/net/lan0/address)"`

case "$1" in
   ${SERVICE_NAME}-start)
      service_start
      ;;
   ${SERVICE_NAME}-stop)
      service_stop
      ;;
   ${SERVICE_NAME}-restart)
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


