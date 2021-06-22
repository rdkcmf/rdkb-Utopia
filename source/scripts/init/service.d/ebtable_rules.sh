#!/bin/sh -
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

# setting all ebtables rules here.

do_block_lan_to_wanip() {
    lan_if=$1
    wan_if=$2
    wan_addrs=`ip -4 addr show "$wan_if" | awk -F'[ /]+' '/inet/ {print $3}'`
    wan_addrs6=`ip -6 addr show "$wan_if" | awk -F'[ /]+' '/inet/ {print $3}'`
    echo  "[blocking] $lan_if <-> $wan_if ..."

    # for each IPV4 and IPV6 addresses
    # NOTE: the traffic could be:
    #   LAN_PC (172.16.80.44) -> l2sd0 -> lbr0 -> cni0 -> WAN_GW (172.16.80.1) -> wan0 (172.16.64.11)
    # instead of 
    #   LAN_PC (172.16.80.44) -> l2sd0 -> lbr0 -> wan0 (172.16.64.11)
    # we couldn't use "-d <wan0_mac>"

    for addr in $wan_addrs; do
        if [ -n "$addr" ]; then
            echo  "  filter traffic from $lan_if to $addr"
            ebtables -I FORWARD -i "$lan_if" -p ip --ip-destination "$addr" -j DROP
        fi
    done

    for addr6 in $wan_addrs6; do
        if [ -n "$addr6" ]; then
            echo  "  filter traffic from $lan_if to $addr6"
            ebtables -I FORWARD -i "$lan_if" -p ip6 --ip6-destination "$addr6" -j DROP
        fi
    done
}

#
# block traffic for lan phy interfaces (bridge ports) to wan interfaces' ip.
#
block_lan_to_wanip_all() {
    lan_ifs=`syscfg get lan_ethernet_physical_ifnames`
    wan_ifs=`syscfg get emta_wan_ifname`
    wan_ifs="$wan_ifs `syscfg get ecm_wan_ifname`"
    wan_ifs="$wan_ifs `syscfg get wan_physical_ifname`"

    for lanif in $lan_ifs; do
        for wanif in $wan_ifs; do
            do_block_lan_to_wanip "$lanif" "$wanif"
        done
    done
}

set_ebtable_rules() {

    #down this interface firstly avoid wan packages slip into lan side
    ip link set lbr0 down

    # flush all rules first
    echo "[EBTABLE] flush all rules"
    ebtables -F
    
    echo 0 > /proc/sys/net/ipv4/conf/all/arp_ignore
    echo 2 > /proc/sys/net/ipv4/conf/wan0/arp_ignore
    echo 2 > /proc/sys/net/ipv4/conf/lan0/arp_ignore
    echo 2 > /proc/sys/net/ipv4/conf/erouter0/arp_ignore
    echo 2 > /proc/sys/net/ipv4/conf/mta0/arp_ignore
    echo 2 > /proc/sys/net/ipv4/conf/esafe0/arp_ignore
    echo 2 > /proc/sys/net/ipv4/conf/esafe1/arp_ignore
    echo 0 > /proc/sys/net/ipv4/conf/brlan0/arp_ignore
    echo 0 > /proc/sys/net/ipv4/conf/l2sd0/arp_ignore

    # rules are different in bridge/router mode
    bridge_mode=`sysevent get bridge_mode`
    if [ "0" != "$bridge_mode" ] ; then
        #
        # bridge mode
        #
        echo "[EBTABLE] Block LAN->WAN_IP in bridge mode"
        block_lan_to_wanip_all
        
        ATOM_MAC=`arp 192.168.101.3 | awk '{print $4}'`
        echo "$ATOM_MAC" | grep ':'
        if [ $? != 0 ]; then
            arping -c 1 -I l2sd0.500 192.168.101.3
            ATOM_MAC=`arp 192.168.101.3 | awk '{print $4}'`
        fi
        
        ebtables -I FORWARD -o lbr0 -s "$ATOM_MAC" -j DROP
        ebtables -I FORWARD -o lbr0 -p IPv4 --ip-destination 192.168.100.1 -j ACCEPT
        #ebtables -I FORWARD -o lbr0 -p ARP --arp-ip-dst 192.168.100.1 -j DROP
        
        
        
        ebtables -I OUTPUT -o lbr0 -j DROP
    else
        #
        # router mode
        #
        echo "[EBTABLE] Do not block LAN->WAN_IP in bridge mode"

        echo "[EBTABLE] Isolate LAN/WAN in router mode"
        ebtables -A OUTPUT -o lbr0 -j DROP
        ebtables -A INPUT -i lbr0 -j DROP
        ebtables -I INPUT -p ARP -i l2sd0 --arp-ip-src ! 192.168.100.3 --arp-ip-dst 192.168.100.1 -j DROP
        ebtables -I FORWARD -i lbr0 -j DROP
        ebtables -I FORWARD -o lbr0 -j DROP
        ebtables -I FORWARD -p ARP -d ! Broadcast -o lbr0 -j ACCEPT
        ebtables -I FORWARD -i lbr0 -p ARP --arp-ip-src 192.168.100.1 -j ACCEPT
        #ebtables -I FORWARD -o lbr0 -p ARP --arp-ip-dst 192.168.100.1 -j ACCEPT
        ebtables -I FORWARD -i lbr0 -p IPv4 --ip-source 192.168.100.1 -j ACCEPT
        ebtables -I FORWARD -o lbr0 -p IPv4 --ip-destination 192.168.100.1 -j ACCEPT
    fi

    ip link set lbr0 up
}

set_ebtable_rules
