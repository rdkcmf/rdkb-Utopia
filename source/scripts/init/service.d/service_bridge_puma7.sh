#!/bin/sh
#Script to put the private LAN into pseudo bridge mode

#source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/hostname_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
#source /etc/utopia/service.d/service_lan/wlan.sh
source /etc/utopia/service.d/event_handler_functions.sh
#source /etc/utopia/service.d/brcm_ethernet_helper.sh

SERVICE_NAME="bridge"
MULTINET_HANDLER="/etc/utopia/service.d/vlan_util_xb6.sh"

#Separate routing table used to ensure that responses from the web UI go directly to the LAN interface, not out erouter0
BRIDGE_MODE_TABLE=69

#Mode passed in by commandline, can be "enable" or "disable"
SCRIPT_MODE="$1"

#Set max CPE bypass to 2 in order to account for mta0 and erouter0
set_max_cpe_bypass() {
    ncpu_exec -e "(echo 2 > /proc/arris/max_cpe_bypass)" 
}

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

flush_connection_info(){
    #Flush connection tracking - This will also flush packet processor sessions
    conntrack_flush
    
    #Flush CPE table
    ncpu_exec -e "(echo \"LearnFrom=CPE_DYNAMIC\" > /proc/net/dbrctl/delalt)"
}

get_wan_if_name(){
    WAN_IF=""
    while [ "$WAN_IF" = "" ] ; do
        WAN_IF=`sysevent get wan_ifname`
        if [ "$WAN_IF" = "" ] ; then
            echo "Waiting for wan_ifname value..."
            sleep 1
        fi
    done
}

#Add or remove rules to block local traffic from reaching DOCSIS bridge
filter_local_traffic(){
    if [ "$1" = "enable" ] ; then
        #Create a new chain to local traffic filtering
        ebtables -N BRIDGE_OUTPUT_FILTER
        ebtables -F BRIDGE_OUTPUT_FILTER 2> /dev/null
        ebtables -I OUTPUT -j BRIDGE_OUTPUT_FILTER
        
        #Don't allow a-mux or LAN bridge to send traffic to DOCSIS bridge
        ebtables -A BRIDGE_OUTPUT_FILTER --logical-out a-mux -j DROP
        ebtables -A BRIDGE_OUTPUT_FILTER --logical-out $BRIDGE_NAME -j DROP
        ebtables -A BRIDGE_OUTPUT_FILTER -o lbr0 -j DROP
        
        #Return from filter chain
        ebtables -A BRIDGE_OUTPUT_FILTER -j RETURN
    else
        #Delete the local traffic filter chain
        ebtables -D OUTPUT -j BRIDGE_OUTPUT_FILTER
        ebtables -X BRIDGE_OUTPUT_FILTER
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

cmdiag_ebtables_rules()
{
    if [ "$1" = "enable" ] ; then
        CMDIAG_MAC="`cat /sys/class/net/lan0/address`"
        MUX_MAC="`cat /sys/class/net/adp0/address`"
        
        #Don't allow lan0 or MUX to send traffic to DOCSIS bridge
        ebtables -N BRIDGE_FORWARD_FILTER
        ebtables -F BRIDGE_FORWARD_FILTER 2> /dev/null
        ebtables -I FORWARD -j BRIDGE_FORWARD_FILTER
        ebtables -A BRIDGE_FORWARD_FILTER -s $CMDIAG_MAC -o lbr0 -j DROP
        ebtables -A BRIDGE_FORWARD_FILTER -s $MUX_MAC -o lbr0 -j DROP
        ebtables -A BRIDGE_FORWARD_FILTER -s $CMDIAG_MAC -o llbr0 -j DROP
        ebtables -A BRIDGE_FORWARD_FILTER -s $MUX_MAC -o llbr0 -j DROP
        ebtables -A BRIDGE_FORWARD_FILTER -j RETURN
        
        #Redirect traffic destined to lan0 IP to lan0 MAC address
        ebtables -t nat -N BRIDGE_REDIRECT
        ebtables -t nat -F BRIDGE_REDIRECT 2> /dev/null
        ebtables -t nat -I PREROUTING -j BRIDGE_REDIRECT
        ebtables -t nat -A BRIDGE_REDIRECT --logical-in $BRIDGE_NAME -p ipv4 --ip-dst $LAN_IP -j dnat --to-destination $CMDIAG_MAC
        ebtables -t nat -A BRIDGE_REDIRECT --logical-in $BRIDGE_NAME -p ipv4 --ip-dst $LAN_IP -j forward --forward-dev llan0
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
        echo 1 > /proc/sys/net/ipv6/conf/adp0/disable_ipv6
        echo 1 > /proc/sys/net/ipv6/conf/a-mux/disable_ipv6
        ifconfig $CMDIAG_IF hw ether $CMDIAG_MAC
        cmdiag_ebtables_rules enable
        ifconfig l${CMDIAG_IF} promisc up
        ifconfig $CMDIAG_IF $LAN_IP netmask $LAN_NETMASK up
    else
        ifconfig $CMDIAG_IF down
        ifconfig l${CMDIAG_IF} down
        ip link del $CMDIAG_IF
        cmdiag_ebtables_rules disable
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

    wan_if=`syscfg get wan_physical_ifname`
    cmdiag_ip="192.168.100.1"
    subnet_wan=`ip route show | awk '/'$wan_if'/ {print $1}'`

    ip route del $subnet_wan dev $wan_if
    ip route add $subnet_wan dev $cmdiag_if #proto kernel scope link src $cmdiag_ip

    dst_ip="10.0.0.1" # RT-10-580 @ XB3 
    ip addr add $dst_ip/24 dev $cmdiag_if
    ebtables -t nat -A PREROUTING -p ipv4 --ip-dst $dst_ip -j dnat --to-destination $cmdiag_if_mac
    echo 2 > /proc/sys/net/ipv4/conf/wan0/arp_announce
    ip rule add from $dst_ip lookup $BRIDGE_MODE_TABLE
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

    wan_if=`syscfg get wan_physical_ifname`
    wan_ip=`sysevent get ipv4_wan_ipaddr`
    subnet_wan=`ip route show | grep $cmdiag_if | grep -v 192.168.100. | grep -v 10.0.0 | awk '/'$cmdiag_if'/ {print $1}'`

    ip route del $subnet_wan dev $cmdiag_if
    ip route add $subnet_wan dev $wan_if proto kernel scope link src $wan_ip

    dst_ip="10.0.0.1" # RT-10-580 @ XB3 PRD
    ip addr del $dst_ip/24 dev $cmdiag_if
    ebtables -t nat -D PREROUTING -p ipv4 --ip-dst $dst_ip -j dnat --to-destination $cmdiag_if_mac
    echo 0 > /proc/sys/net/ipv4/conf/wan0/arp_announce
    ip rule del from $dst_ip lookup $BRIDGE_MODE_TABLE
}

routing_rules(){
    if [ "$1" = "enable" ] ; then
        #Send responses from $BRIDGE_NAME IP to a separate bridge mode route table
        ip rule add from $LAN_IP lookup $BRIDGE_MODE_TABLE
        ip route add table $BRIDGE_MODE_TABLE default dev $CMDIAG_IF
        # add_ebtable_rule
    else
        ip rule del from $LAN_IP lookup $BRIDGE_MODE_TABLE
        ip route flush table $BRIDGE_MODE_TABLE
        # del_ebtable_rule
    fi
}

#Enable pseudo bridge mode.  If already enabled, just refresh parameters (in case bridges were torn down and rebuilt)
service_start(){
    wait_till_steady_state ${SERVICE_NAME}
    STATUS=`sysevent get ${SERVICE_NAME}-status`
    if [ "started" != "$STATUS" ] ; then
        sysevent set ${SERVICE_NAME}-errinfo
        sysevent set ${SERVICE_NAME}-status starting
        
        block_bridge
        
        #Connect management interface
        cmdiag_if enable
        
        routing_rules enable
        
        #Sync bridge ports
        $MULTINET_HANDLER multinet-syncMembers $INSTANCE
        
        #Block traffic coming from the lbr0 connector interfaces at the MUX
        filter_local_traffic enable
        
        unblock_bridge
        
        prepare_hostname
                
        #Flush connection tracking and packet processor sessions to avoid stale information
        flush_connection_info

        #Use Arris max_cpe_bypass parameter to allow erouter0 and mta0 not to count against max_cpe total
        set_max_cpe_bypass
 
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
        
        #Sync bridge members
        $MULTINET_HANDLER multinet-syncMembers $INSTANCE
                
        #Disconnect management interface
        cmdiag_if disable
        filter_local_traffic disable
	routing_rules disable       

        unblock_bridge
        
        #Flush connection tracking and packet processor sessions to avoid stale information
        flush_connection_info
        
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

BRIDGE_NAME="$SYSCFG_lan_ifname"
CMDIAG_IF=`syscfg get cmdiag_ifname`
CMDIAG_MAC=`ncpu_exec -ep "(cat /sys/class/net/lan0/address)"`
INSTANCE=`sysevent get primary_lan_l2net`
#LAN_IP=`syscfg get lan_ipaddr`
LAN_IP="10.0.0.1"
LAN_NETMASK=`syscfg get lan_netmask`

case "$1" in
    ${SERVICE_NAME}-start)
        firewall firewall-stop
        service_start
        execute_dir /etc/utopia/post.d/
        gw_lan_refresh
        sysevent set firewall-restart
    ;;
    ${SERVICE_NAME}-stop)
        service_stop
        execute_dir /etc/utopia/post.d/
        gw_lan_refresh
        sysevent set firewall-restart
    ;;
    ${SERVICE_NAME}-restart)
        firewall firewall-stop
        sysevent set lan-restarting $INSTANCE
        service_stop
        service_start
        sysevent set lan-restarting 0
        gw_lan_refresh
        sysevent set firewall-restart
    ;;
    *)
        echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
        exit 3
    ;;
esac
