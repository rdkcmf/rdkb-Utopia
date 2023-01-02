#!/bin/sh

##################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2018 RDK Management

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

#Script to put the private LAN into pseudo bridge mode

#source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/hostname_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
#source /etc/utopia/service.d/service_lan/wlan.sh
source /etc/utopia/service.d/event_handler_functions.sh
#source /etc/utopia/service.d/brcm_ethernet_helper.sh

source /etc/utopia/service.d/log_capture_path.sh

POSTD_START_FILE="/tmp/.postd_started"

SERVICE_NAME="bridge"

#Separate routing table used to ensure that responses from the web UI go directly to the LAN interface, not out erouter0
BRIDGE_MODE_TABLE=69

#Mode passed in by commandline, can be "enable" or "disable"
SCRIPT_MODE="$1"

# Current gw ip address
LAN_IP=`syscfg get lan_ipaddr`

#Set max CPE bypass to 2 in order to account for mta0 and erouter0
set_max_cpe_bypass() {
    ncpu_exec -e service_bridge.sh set_max_cpe_bypass 
}

wait_till_steady_state ()
{
    LSERVICE=$1
    TRIES=1
    while [ "30" -ge "$TRIES" ] ; do
        LSTATUS=`sysevent get "${LSERVICE}"-status`
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
    ncpu_exec -e service_bridge.sh clear_cpe_table
}

get_wan_if_name(){
    WAN_IF=""
    while [ -z "$WAN_IF" ] ; do
        WAN_IF=`sysevent get wan_ifname`
        if [ -z "$WAN_IF" ] ; then
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
        
        #Don't allow LAN bridge to send traffic to DOCSIS bridge
        ebtables -A BRIDGE_OUTPUT_FILTER --logical-out "$BRIDGE_NAME" -j DROP
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
    ifconfig lbr0 down
}

#Unblock bridged traffic through lbr0
unblock_bridge(){
    ifconfig lbr0 up
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
        ebtables -A BRIDGE_FORWARD_FILTER -s "$CMDIAG_MAC" -o lbr0 -j DROP
        ebtables -A BRIDGE_FORWARD_FILTER -s "$MUX_MAC" -o lbr0 -j DROP
        if [ "$ETHWAN_ENABLED" = "true" ];then
            ebtables -A BRIDGE_FORWARD_FILTER -s "$CMDIAG_MAC" -o nsgmii0 -j DROP
        fi
        ebtables -A BRIDGE_FORWARD_FILTER -j RETURN
        
        #Redirect traffic destined to lan0 IP to lan0 MAC address
        ebtables -t nat -N BRIDGE_REDIRECT
        ebtables -t nat -F BRIDGE_REDIRECT 2> /dev/null
        ebtables -t nat -I PREROUTING -j BRIDGE_REDIRECT
        ebtables -t nat -A BRIDGE_REDIRECT --logical-in "$BRIDGE_NAME" -p ipv4 --ip-dst "$LAN_IP" -j dnat --to-destination "$CMDIAG_MAC"
        ebtables -t nat -A BRIDGE_REDIRECT --logical-in "$BRIDGE_NAME" -p ipv4 --ip-dst "$LAN_IP" -j forward --forward-dev llan0
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
        ip link add "$CMDIAG_IF" type veth peer name l"${CMDIAG_IF}"
        echo 1 > /proc/sys/net/ipv6/conf/lan0/disable_ipv6
        echo 1 > /proc/sys/net/ipv6/conf/llan0/disable_ipv6
        echo 1 > /proc/sys/net/ipv6/conf/adp0/disable_ipv6
        echo 1 > /proc/sys/net/ipv6/conf/a-mux/disable_ipv6
        ifconfig "$CMDIAG_IF" hw ether "$CMDIAG_MAC"
        cmdiag_ebtables_rules enable
        ifconfig l"${CMDIAG_IF}" promisc up
        ifconfig "$CMDIAG_IF" "$LAN_IP" netmask "$LAN_NETMASK" up
        
        #add lan0 interface entry to the TOE netdevList for PP on ATOM configuration
        if [ -d /etc/pp_on_atom ] ; then
             echo "ADD $CMDIAG_IF" > /sys/devices/platform/toe/netif_lut
        fi
    else
        ifconfig "$CMDIAG_IF" down
        ifconfig l"${CMDIAG_IF}" down
        ip link del "$CMDIAG_IF"
        #del lan0 interface entry from the TOE netdevList for PP on ATOM configuration
        if [ -d /etc/pp_on_atom ] ; then
             echo 0 > /sys/devices/platform/toe/enable
             echo "DEL $CMDIAG_IF" > /sys/devices/platform/toe/netif_lut
             echo 1 > /sys/devices/platform/toe/enable
        fi
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
    cmdiag_if=`syscfg get cmdiag_ifname`
    cmdiag_if_mac=`ip link show "$cmdiag_if" | awk '/link/ {print $2}'`

    dst_ip=`syscfg get lan_ipaddr` # RT-10-580 @ XB3 
    if [ "$LAN_IP" != "$dst_ip" ]; then
        ip addr add "$dst_ip"/24 dev "$cmdiag_if"
        ebtables -t nat -A PREROUTING -p ipv4 --ip-dst "$dst_ip" -j dnat --to-destination "$cmdiag_if_mac"
        echo 2 > /proc/sys/net/ipv4/conf/wan0/arp_announce
        ip rule add from "$dst_ip" lookup $BRIDGE_MODE_TABLE
    fi
}

#--------------------------------------------------------------
# del_ebtable_rule
# Delete rule in ebtable nat PREROUTING chain
#--------------------------------------------------------------
del_ebtable_rule()
{
    cmdiag_if=`syscfg get cmdiag_ifname`
    cmdiag_if_mac=`ip link show "$cmdiag_if" | awk '/link/ {print $2}'`

    dst_ip=`syscfg get lan_ipaddr` # RT-10-580 @ XB3 PRD
    if [ "$LAN_IP" != "$dst_ip" ]; then
        ip addr del "$dst_ip"/24 dev "$cmdiag_if"
        ebtables -t nat -D PREROUTING -p ipv4 --ip-dst "$dst_ip" -j dnat --to-destination "$cmdiag_if_mac"
        echo 0 > /proc/sys/net/ipv4/conf/wan0/arp_announce
        ip rule del from "$dst_ip" lookup $BRIDGE_MODE_TABLE
    fi
}

routing_rules(){
    if [ "$1" = "enable" ] ; then
        #Send responses from $BRIDGE_NAME IP to a separate bridge mode route table
        ip rule add from "$LAN_IP" lookup $BRIDGE_MODE_TABLE
        ip route add table $BRIDGE_MODE_TABLE default dev "$CMDIAG_IF"
        add_ebtable_rule
		/etc/utopia/service.d/service_dhcp_server.sh dns-restart
    else
        ip rule del from "$LAN_IP" lookup $BRIDGE_MODE_TABLE
        ip route flush table $BRIDGE_MODE_TABLE
        del_ebtable_rule
    fi
}

forward_wan_lan_traffic()
{

    if [ "$1" = "enable" ] ; then
        # set up veth interface to forward brlan0 and erouter traffic in bridge mode
            echo "BRIDGE MODE case : ethwan enabled, creating veth interface"
            ip link add lbr1 type veth peer name ler0
            ifconfig lbr1 up 
            ifconfig ler0 up
	    echo 1 > /proc/sys/net/ipv6/conf/ler0/disable_ipv6
            echo 1 > /proc/sys/net/ipv6/conf/lbr1/disable_ipv6
	    brctl addif erouter0 ler0
            MAX_WAIT_TIME=120
            TRIES=0
            while [  "$TRIES" -lt "$MAX_WAIT_TIME" ] ; do

                   if [ "`sysevent get multinet_$INSTANCE-status`" = "ready" ];then
                        check_iface_exists_in_bridge=`brctl show brlan0  | grep lbr1`
                        if [ -z "$check_iface_exists_in_bridge" ];then
                            echo_t "multinet_$INSTANCE-status status is ready...,adding lbr1 to brlan0 and breaking the loop"  
                            brctl addif brlan0 lbr1
                        fi
                        break;
                    fi
                 sleep 5
                 TRIES=`expr $TRIES + 5`
            done
    else
            echo "ROUTER MODE case : ethwan enabled, deleting veth interface"
            ifconfig lbr1 down 
            ifconfig ler0 down
            brctl delif brlan0 lbr1
            brctl delif erouter0 ler0
            ip link del lbr1
    fi

}

#--------------------------------------------------------------
# update_bridge_mtu
# Fetch the max MTU size supported from CM agent DML and apply
# this MTU setting to the bridge
#--------------------------------------------------------------
update_bridge_mtu() {
    #Check whether in bridged mode
    MODE=`sysevent get bridge_mode`
    case $MODE in
        ''|*[!0-9]*)
            #Invalid / non-numeric result
            return
        ;;
        *)
            if [ $MODE -lt 1 ] ; then
                #Not in bridged mode
                return
            fi
        ;;
    esac

    #Fetch DOCSIS max supported MTU from DML
    MAXMTU=`dmcli eRT getv Device.X_RDKCENTRAL-COM_CableModem.MaxMTU | grep value | awk '/value/{print $5}'`
    if [ $? -ne 0 ] ; then
        #Error fetching value
        return
    fi

    #Check if value returned is a number and whether it is different than last iteration
    case $MAXMTU in
        ''|*[!0-9]*)
            #Invalid / non-numeric result
            return
        ;;
        *)
            echo "Got MTU value $MAXMTU from CM DML"
            ifconfig $BRIDGE_NAME mtu $MAXMTU
        ;;
    esac
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
       
        if [ "$ETHWAN_ENABLED" = "true" ];then
            forward_wan_lan_traffic enable &
        fi
        #Sync bridge ports
        MULTILAN_FEATURE=$(syscfg get MULTILAN_FEATURE)
        if [ "$MULTILAN_FEATURE" = "1" ]; then            
            sysevent set multinet-up "$INSTANCE"
            #Sync bridge ports
            sysevent set multinet-syncMembers "$INSTANCE"
        else
            sysevent set multinet-syncMembers $INSTANCE
        fi
        
        #Block traffic coming from the lbr0 connector interfaces at the MUX
        filter_local_traffic enable
        
        #Update MTU of bridge
        update_bridge_mtu
        
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
        MULTILAN_FEATURE=$(syscfg get MULTILAN_FEATURE)
        if [ "$MULTILAN_FEATURE" = "1" ]; then
            sysevent set multinet-down "$INSTANCE"
            sysevent set multinet-up "$INSTANCE"
        else
            sysevent set  multinet-syncMembers $INSTANCE
        fi
                
        #Disconnect management interface
        cmdiag_if disable
        filter_local_traffic disable
	routing_rules disable       

        unblock_bridge
        
        #Flush connection tracking and packet processor sessions to avoid stale information
        flush_connection_info
        if [ "$ETHWAN_ENABLED" = "true" ];then
            forward_wan_lan_traffic disable &
        fi
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


echo "service_bridge_puma7.sh called with $1 $2" 
service_init

BRIDGE_NAME="$SYSCFG_lan_ifname"
CMDIAG_IF=`syscfg get cmdiag_ifname`
CMDIAG_MAC=`ncpu_exec -ep service_bridge.sh get_cmdiag_mac`
INSTANCE=`sysevent get primary_lan_l2net`
if [ -z "$INSTANCE" ];then
	INSTANCE=`psmcli get dmsb.MultiLAN.PrimaryLAN_l2net`
fi
LAN_NETMASK=`syscfg get lan_netmask`

ETHWAN_ENABLED=`syscfg get eth_wan_enabled`

case "$1" in
    "${SERVICE_NAME}-start")

        firewall firewall-stop
        /etc/rc3.d/setup_docsis_lan0_path.sh lbr0_on_bridged
        service_start
        if [ ! -f "$POSTD_START_FILE" ];
        then
                touch $POSTD_START_FILE
                execute_dir /etc/utopia/post.d/
        fi        
        gw_lan_refresh
        sysevent set firewall-restart

    ;;
    wan-start)
        update_bridge_mtu
    ;;
    "${SERVICE_NAME}-stop")
    
        /etc/rc3.d/setup_docsis_lan0_path.sh lbr0_on_routed
        service_stop
        if [ ! -f "$POSTD_START_FILE" ];
        then
                touch $POSTD_START_FILE
                execute_dir /etc/utopia/post.d/
        fi        
        gw_lan_refresh
        sysevent set firewall-restart
    ;;
    "${SERVICE_NAME}-restart")
        
        firewall firewall-stop
        sysevent set lan-restarting "$INSTANCE"
        service_stop
        /etc/rc3.d/setup_docsis_lan0_path.sh lbr0_on_bridged
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
