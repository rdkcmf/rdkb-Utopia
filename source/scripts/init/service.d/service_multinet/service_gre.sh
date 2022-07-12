#!/bin/sh

##########################################################################
#  Copyright 2017-2018 Intel Corporation
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
##########################################################################

DMSB_GRE_TUNNEL="dmsb.gre.tunnel"
GRE_INTERFACE="interface"

GRE_TUNNEL_ALIAS="Alias"
GRE_TUNNEL_REMOTEENDPOINTS="RemoteEndpoints"
GRE_TUNNEL_KEEPALIVEPOLICY="KeepAlivePolicy"
GRE_TUNNEL_KEEPALIVETIMEOUT="KeepAliveTimeout"
GRE_TUNNEL_KEEPALIVETHRESHOLD="KeepAliveThreshold"
GRE_TUNNEL_DELIVERYHEADERPROTOCOL="DeliveryHeaderProtocol"
GRE_TUNNEL_DEFAULTDSCPMARK="DefaultDSCPMark"
GRE_TUNNEL_KEEPALIVECOUNT="KeepAliveCount"
GRE_TUNNEL_KEEPALIVEINTERVAL="KeepAliveInterval"
GRE_TUNNEL_KEEPALIVEFAILUREINTERVAL="KeepAliveFailureInterval"
GRE_TUNNEL_KEEPALIVERECOVERINTERVAL="KeepAliveRecoverInterval"
GRE_TUNNEL_WIFI_INTFS="WiFiInterfaces"
GRE_TUNNEL_SNOOP_CIRCUIT_ENABLE="SnoopCircuitEnable"
GRE_TUNNEL_SNOOP_REMOTE_ENABLE="SnoopRemoteEnable"
GRE_TUNNEL_MAX_CLIENTS="MaxClients"

GRE_TUNNEL_IF_LOWERLAYERS="LowerLayers"
GRE_TUNNEL_IF_PROTOCOLIDOVERRIDE="ProtocolIdOverride"
GRE_TUNNEL_IF_USECHECKSUM="UseCheckSum"
GRE_TUNNEL_IF_KEYIDENTIFIERGENERATIONPOLICY="KeyIdentifierGenerationPolicy"
GRE_TUNNEL_IF_KEYIDENTIFIER="KeyIdentifier"
GRE_TUNNEL_IF_USESEQUENCENUMBER="UseSequenceNumber"

GRE_ARP_PROC=hotspot_arpd
HOTSPOT_COMP=CcspHotspot
ARP_NFQUEUE=0
BINPATH=/fss/gw/usr/ccsp
DEL_FOR_HS=0

MTU_VAL=1500
TX_QUEUE_LEN=1000

THIS=/etc/utopia/service.d/service_multinet/service_gre.sh

#Function to read parameters needed for hotspot initilaization
#Params
# $1 - Gre Tunnel Instance
read_init_params ()
{
    #Hotpsot implenetation is supporting two remote end points as of now
    tunnelInst=$1
    REMOTEENDPOINTS=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_REMOTEENDPOINTS`
    IFS=","
    set -- $REMOTEENDPOINTS
    PRIMARY=$1
    SECONDARY=$2

    KA_THRESH=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_KEEPALIVETHRESHOLD`
    KA_INTERVAL=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_KEEPALIVEINTERVAL`
    KA_RECOVER_INTERTVAL=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_KEEPALIVERECOVERINTERVAL`
    KA_POLICY=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_KEEPALIVEPOLICY`
    KA_COUNT=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_KEEPALIVECOUNT`
    KA_FAIL_INTERVAL=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_KEEPALIVEFAILUREINTERVAL`
    SNOOP_CIRCUIT=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_SNOOP_CIRCUIT_ENABLE`
    SNOOP_REMOTE=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_SNOOP_REMOTE_ENABLE`
    MAX_CLIENTS=`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_MAX_CLIENTS`
}

#Function to set sysevents for Hotspot DHCP Snooping
init_snooper_sysevents ()
{
    if [ x1 = x$SNOOP_CIRCUIT ]; then
        sysevent set snooper-circuit-enable 1
    else
        sysevent set snooper-circuit-enable 0
    fi

    if [ x1 = x$SNOOP_REMOTE ]; then
        sysevent set snooper-remote-enable 1
    else
        sysevent set snooper-remote-enable 0
    fi

    if [ x = x`sysevent get snooper-max-clients` ]; then
        sysevent set snooper-max-clients $MAX_CLIENTS
    fi
}

#Function to set sysevents for Hotspot Keep Alive Events
init_keepalive_sysevents () {

    keepalive_args="-n `sysevent get wan_ifname`"

    if [ x = x`sysevent get hotspotfd-primary` ]; then
        sysevent set hotspotfd-primary $PRIMARY
    fi

    if [ x = x`sysevent get hotspotfd-secondary` ]; then
        sysevent set hotspotfd-secondary $SECONDARY
    fi

    if [ x = x`sysevent get hotspotfd-threshold` ]; then
            sysevent set hotspotfd-threshold $KA_THRESH
    fi

    if [ x = x`sysevent get hotspotfd-keep-alive` ]; then
        sysevent set hotspotfd-keep-alive $KA_INTERVAL
    fi

    if [ x = x`sysevent get hotspotfd-max-secondary` ]; then
        sysevent set hotspotfd-max-secondary $KA_RECOVER_INTERTVAL
    fi

    if [ x = x`sysevent get hotspotfd-policy` ]; then
        sysevent set hotspotfd-policy $KA_POLICY
    fi

    if [ x = x`sysevent get hotspotfd-count` ]; then
        sysevent set hotspotfd-count $KA_COUNT
    fi

    if [ x = x`sysevent get hotspotfd-dead-interval` ]; then
        sysevent set hotspotfd-dead-interval $KA_FAIL_INTERVAL
    fi

    if [ x"started" = x`sysevent get wan-status` ]; then
        sysevent set hotspotfd-enable 1
        keepalive_args="$keepalive_args -e 1"
    else
        sysevent set hotspotfd-enable 0
    fi

    sysevent set hotspotfd-log-enable 1

}

#Function to kill Hotpsot and its related processes
#Params
# $1 - Gre Tunnel Instance
kill_procs ()
{
    killall $HOTSPOT_COMP
    sysevent set ${1}_keepalive_pid
    killall $GRE_ARP_PROC
}

#Function to get the SSIDs which are associated with the Hotspot bridge
get_ssids ()
{
    hotspot_br=`sysevent get hotspot_bridge`
    WIFI_INTFS=`psmcli get dmsb.l2net.$hotspot_br.Members.WiFi`
    for wifiintf in $WIFI_INTFS
    do
        BRG_PORT_INST=`psmcli getallinst dmsb.l2net.$hotspot_br.Port.`
        for port in $BRG_PORT_INST
        do
            IFWIFIPORT=`psmcli get dmsb.l2net.$hotspot_br.Port.$port.Name`
            if [ "$IFWIFIPORT" = "$wifiintf" ]; then
                isPortEnabled=`psmcli get dmsb.l2net.$hotspot_br.Port.$port.Enable`
                if [ "$isPortEnabled" = "TRUE" ]; then
                    wifiSSID=`$BINPATH/ccsp_bus_client_tool eRT getv Device.Bridging.Bridge.$hotspot_br.Port.$port.LowerLayers | grep string, | awk '{print $5}' `
                    allSSIDs="$allSSIDs $wifiSSID"
                    instSSID=${wifiSSID#${wifiSSID%?}}
                    allSSIDinsts="$allSSIDinsts $instSSID"
                fi
            fi
        done
    done

    hs_gre_inst=`sysevent get hotspot_gre_tunnel_inst`

    allSSIDs=` echo $allSSIDs | tr ' ' '\n' | sort -u`
    status=`psmcli set $DMSB_GRE_TUNNEL.$hs_gre_inst.$GRE_TUNNEL_WIFI_INTFS $allSSIDs`

    allSSIDinsts=` echo $allSSIDinsts | tr ' ' '\n' | sort -u `
}

#Function to enable/disable the SSIDs and Radios for Hotspot
#Params
# $1 - True / False to enable / disable SSIDs
set_ssids_enabled() {

    hs_gre_inst=`sysevent get hotspot_gre_tunnel_inst`
     #delay SSID manipulation if requested
    sleep `sysevent get hotspot_$hs_gre_inst-delay` 2> /dev/null
    sysevent set hotspot_$hs_gre_inst-delay 0

    get_ssids

    for device_wifi_ssid in $allSSIDs; do
       instSSID=${device_wifi_ssid#${device_wifi_ssid%?}}
       $BINPATH/ccsp_bus_client_tool eRT setv ${device_wifi_ssid}.X_CISCO_COM_RouterEnabled bool $1 &
       $BINPATH/ccsp_bus_client_tool eRT setv ${device_wifi_ssid}.X_CISCO_COM_EnableOnline bool true &
       rad=`dmcli eRT getv ${device_wifi_ssid}.LowerLayers  | grep string,  | awk '{print $5}' | cut -d . -f 4 `
       $BINPATH/ccsp_bus_client_tool eRT setv Device.WiFi.Radio.$rad.X_CISCO_COM_ApplySettingSSID string $instSSID
       $BINPATH/ccsp_bus_client_tool eRT setv Device.WiFi.Radio.$rad.X_CISCO_COM_ApplySetting bool true &
    done

    sysevent set hotspot_ssids_up $1
}

#Function to decide the state of SSIDs
check_ssids () {
    hs_gre_inst=`sysevent get hotspot_gre_tunnel_inst`
    EP="`sysevent get ${hs_gre_inst}_current_remoteEP`"
    WAN="`sysevent get wan-status`"
    curSSIDSTATE="`sysevent get hotspot_ssids_up`"

    if [ x"started" = x$WAN -a x != x$EP -a x"0.0.0.0" != x$EP ]; then
        if [ x"true" = x$curSSIDSTATE ]; then
            kick_clients 1
        else
            set_ssids_enabled true > /dev/null
        fi
    else
        #SSIDs should be down
        if [ x"true" = x$curSSIDSTATE ]; then
            set_ssids_enabled false > /dev/null
        fi
    fi
}

#Function to set the API isolation for AccesPoints for Hotspot
set_apisolation() {
    get_ssids
    for instance in $allSSIDinsts; do
        $BINPATH/ccsp_bus_client_tool eRT setv Device.WiFi.AccessPoint.${instance}.IsolationEnable bool true
    done
}

#Function to remove other associated devices from the Accesspoint
kick_clients () {
    get_ssids
    for instance in $allSSIDinsts; do
        $BINPATH/ccsp_bus_client_tool eRT setv Device.WiFi.AccessPoint.${instance}.X_CISCO_COM_KickAssocDevices bool true &
    done
}

#Function to invoke processes for Hotspot
#Params
# $1 - GRE Tunnel Instance
start_hotspot()
{
    read_init_params $1
    async="`sysevent async hotspotfd-tunnelEP $THIS`"
    sysevent set gre_ep_async "$async" > /dev/null
    async="`sysevent async wan-status $THIS`"
    sysevent set gre_wan_async "$async" > /dev/null
    async="`sysevent async hotspotfd-primary $THIS`"
    sysevent set gre_primary_async "$async" > /dev/null

    init_keepalive_sysevents > /dev/null
    init_snooper_sysevents
    sysevent set snooper-log-enable 1
    set_apisolation $1
    set_ssids_enabled true
    $HOTSPOT_COMP -subsys eRT. > /dev/null &
    sysevent set ${1}_keepalive_pid $! > /dev/null
    $GRE_ARP_PROC -q $ARP_NFQUEUE  > /dev/null &
}

#Function to create a new GRE tunnel
#Params :
# $1 - Tunnel Instance Number
# $2 - Remote EP
create_tunnel()
{
    GRE_TUN_IF_NAME="`psmcli get $DMSB_GRE_TUNNEL.$1.$GRE_INTERFACE.1.Name`"
    WAN_IF=`sysevent get wan_ifname`
    DELIVERY_PROTOCOL="`psmcli get $DMSB_GRE_TUNNEL.$1.$GRE_TUNNEL_DELIVERYHEADERPROTOCOL`"

    # if (1)=IPV4 creates a IPV4 tunnel else (2)=IPV6 creates an IPV6 Tunnel
    if [ "$DELIVERY_PROTOCOL" = 1 ];then
        ip link add $GRE_TUN_IF_NAME type gretap remote $2 dev $WAN_IF nopmtudisc
        ip link set $GRE_TUN_IF_NAME txqueuelen $TX_QUEUE_LEN mtu $MTU_VAL up
    elif [ "$DELIVERY_PROTOCOL" = 2 ];then
        IPv6_ADDR=`sysevent get ipv6_dhcp6_addr`
        ip link add $GRE_TUN_IF_NAME type ip6gretap remote $2 local $IPv6_ADDR dev $WAN_IF
        ip link set $GRE_TUN_IF_NAME txqueuelen $TX_QUEUE_LEN mtu $MTU_VAL up
    fi

    sysevent set ${1}_current_remoteEP $2
}

# Set Firewall rules for the bridges
#Params :
# $1 - GRE Tunnel Instance Number
# $2 - Bridge Instance
set_br_fw_rules()
{
    greInst=$1
    BridgeInst=$2
    bridgeName=`psmcli get dmsb.l2net.$BridgeInst.Name`
    DELIVERY_PROTOCOL="`psmcli get $DMSB_GRE_TUNNEL.$greInst.$GRE_TUNNEL_DELIVERYHEADERPROTOCOL`"
    REMOTEIP="`sysevent get ${greInst}_current_remoteEP`"

    if [ "$DELIVERY_PROTOCOL" = 1 ];then
        ipv4_gre_rule=`sysevent setunique GeneralPurposeFirewallRule " -A INPUT -s $REMOTEIP -p gre -j ACCEPT"`
        sysevent set ${greInst}_ipv4_gre_rule $ipv4_gre_rule
        ipv4_gre_fw_rule=`sysevent setunique GeneralPurposeFirewallRule " -A FORWARD -i $bridgeName -j ACCEPT"`
        sysevent set ${greInst}_ipv4_gre_fw_rule $ipv4_gre_fw_rule
        ipv4_v6_gre_fw_rule=`sysevent setunique v6GeneralPurposeFirewallRule " -A FORWARD -i $bridgeName -j ACCEPT"`
        sysevent set ${greInst}_ipv4_v6_gre_fw_rule $ipv4_v6_gre_fw_rule
    elif [ "$DELIVERY_PROTOCOL" = 2 ];then
        ipv6_gre_rule=`sysevent setunique v6GeneralPurposeFirewallRule " -I INPUT -s $REMOTEIP -p gre -j ACCEPT"`
        sysevent set ${greInst}_ipv6_gre_rule $ipv6_gre_rule
        ipv6_gre_fw_rule=`sysevent setunique v6GeneralPurposeFirewallRule " -A FORWARD -i $bridgeName -j ACCEPT"`
        sysevent set ${greInst}_ipv6_gre_fw_rule $ipv6_gre_fw_rule
        ipv6_v4_gre_fw_rule=`sysevent setunique GeneralPurposeFirewallRule " -A FORWARD -i $bridgeName -j ACCEPT"`
        sysevent set ${greInst}_ipv6_v4_gre_fw_rule $ipv6_v4_gre_fw_rule
    fi
}

#Function to configure hotspot parameters and set firewall rules
#Params
# $1 - GRE Tunnel Instance
# $2 - Bridge Instance
configure_hotspot()
{
    greInst=$1
    BridgeInst=$2
    bridgeName=`psmcli get dmsb.l2net.$BridgeInst.Name`
    WAN_IF=`sysevent get wan_ifname`

    if [ x = x`sysevent get ${greInst}_keepalive_pid` ]; then
        sysevent set hotspot_gre_tunnel_inst $greInst
        sysevent set hotspot_bridge $BridgeInst
        HsBrPort=`psmcli get ${greInst}_PortInstance`
        sysevent set hotspot_br_port $HsBrPort
        arpFWRule=`sysevent setunique GeneralPurposeFirewallRule " -I OUTPUT -o $WAN_IF -p icmp --icmp-type 3 -j NFQUEUE --queue-bypass --queue-num $ARP_NFQUEUE"`
        br_snoop_rule="`sysevent setunique GeneralPurposeFirewallRule " -A FORWARD -o $bridgeName -p udp --dport=67:68 -j NFQUEUE --queue-bypass --queue-num 1"`"
        sysevent set cur_arp_rule $arpFWRule
        sysevent set cur_br_snoop_rule $br_snoop_rule
        start_hotspot $greInst
    fi
}

#Function set check if its a Hotspot bridge or not and set firewall rules accordignly
#Params :
# $1 - Bridge Instance Number
set_firewall_rules()
{
    BridgeInst=$1
    Bridge_Alias=`psmcli get dmsb.l2net.$BridgeInst.Alias`

    GRE_INTFS=`psmcli get dmsb.l2net.$BridgeInst.Members.Gre | xargs`
    GRE_ALL_INST="`psmcli getallinst $DMSB_GRE_TUNNEL.`"

    for greInst in $GRE_ALL_INST
    do
        isTunnelEnabled="`psmcli get $DMSB_GRE_TUNNEL.$greInst.Enable`"
        if [ "$isTunnelEnabled" = "TRUE" -o "$isTunnelEnabled" = 1 ]; then
            isIntfEnabled="`psmcli get $DMSB_GRE_TUNNEL.$greInst.$GRE_INTERFACE.1.Enable`"
            if [ "$isIntfEnabled" = "TRUE" -o "$isIntfEnabled" = 1 ]; then
                GRE_TUN_IF_NAME="`psmcli get $DMSB_GRE_TUNNEL.$greInst.$GRE_INTERFACE.1.Name`"
                if [ "$GRE_INTFS" = "$GRE_TUN_IF_NAME" ];then
                    BRG_PORT_INST=`psmcli getallinst dmsb.l2net.$BridgeInst.Port.`
                    for port in $BRG_PORT_INST
                    do
                       IFGREPORT=`psmcli get dmsb.l2net.$BridgeInst.Port.$port.Name`
                        if [ "$IFGREPORT" = "$GRE_TUN_IF_NAME" ]; then
                            isPortEnabled=`psmcli get dmsb.l2net.$BridgeInst.Port.$port.Enable`
                            if [ "$isPortEnabled" = "TRUE" ]; then
                                psmcli set ${greInst}_BridgeInstance $BridgeInst
                                psmcli set ${greInst}_PortInstance $port
                            fi
                        fi
                    done
                    if [ "$Bridge_Alias" = "hotspot_gre_br" ]
                    then
                        configure_hotspot $greInst $BridgeInst
                    fi
                    set_br_fw_rules $greInst $BridgeInst
                fi
            fi
        fi
    done
    sysevent set firewall-restart
}

#Function to bring up the bridge if it is not up and add interface to bridge
bring_up_bridge_on_boot()
{
    BridgeInst=`psmcli get ${1}_BridgeInstance`

    status=`sysevent get multinet_$BridgeInst-status`
    if [ x = x$status -o x$STOPPED_STATUS = x$status ]; then
        sysevent set multinet-start $BridgeInst
    else
        sysevent set multinet-syncMembers $BridgeInst
    fi
}

#Function to bring down the hotspot process
#Params :
# $1 - GRE Instance
stop_hotspot()
{
    GRE_TUN_IF_NAME="`psmcli get $DMSB_GRE_TUNNEL.$1.$GRE_INTERFACE.1.Name`"

    ip link del $GRE_TUN_IF_NAME
    kill_procs $1

    #Used to disconnect the clients connected to Hotspot SSID
    set_ssids_enabled false

    psmcli del ${1}_BridgeInstance
    psmcli del ${1}_PortInstance

    psmcli set $DMSB_GRE_TUNNEL.${1}.PrimaryEndPoint
    psmcli set $DMSB_GRE_TUNNEL.${1}.SecondaryEndPoint

    dmcli eRT setv Device.GRE.Tunnel.${1}.Enable bool false

    if [ x != x`sysevent get cur_br_snoop_rule` ]; then
         sysevent set `sysevent get cur_br_snoop_rule`
    fi

    if [ x != x`sysevent get cur_arp_rule` ]; then
         sysevent set `sysevent get cur_arp_rule`
    fi

    if [ x != x`sysevent get ${1}_ipv4_gre_rule` ]; then
         sysevent set `sysevent get ${1}_ipv4_gre_rule`
    fi

    if [ x != x`sysevent get ${1}_ipv4_gre_fw_rule` ]; then
         sysevent set `sysevent get ${1}_ipv4_gre_fw_rule`
    fi

    if [ x != x`sysevent get ${1}_ipv4_v6_gre_fw_rule` ]; then
         sysevent set `sysevent get ${1}_ipv4_v6_gre_fw_rule`
    fi
    
    if [ x != x`sysevent get ${1}_ipv6_gre_rule` ]; then
         sysevent set `sysevent get ${1}_ipv6_gre_rule`
    fi

    if [ x != x`sysevent get ${1}_ipv6_gre_fw_rule` ]; then
         sysevent set `sysevent get ${1}_ipv6_gre_fw_rule`
    fi

    if [ x != x`sysevent get ${1}_ipv6_v4_gre_fw_rule` ]; then
         sysevent set `sysevent get ${1}_ipv6_v4_gre_fw_rule`
    fi

    sysevent set firewall-restart
}

#Function to delete the tunnel instance
#Params :
# $1 - Check if Hotpsot-GRE Br / GRE Br is getting deleted
# $2 - GRE Instance
# $3 - Bridge Instance
# $4 - Bridge Port Instance
delete_tunnel()
{
    GRE_TUN_IF_NAME="`psmcli get $DMSB_GRE_TUNNEL.$2.$GRE_INTERFACE.1.Name`"

    #In case of deletion due to switching of end points we need to delete this rule
    if [ x1 = x"$1" ]; then
        sysevent set `sysevent get ${2}_ipv4_gre_rule`
    fi

    dmcli eRT setv Device.Bridging.Bridge.$3.Port.$4.Enable bool false
    ip link del $GRE_TUN_IF_NAME
}

#Function to find the gre tunnels that are active
find_active_gre_tunnels()
{
    GRE_ALL_INST="`psmcli getallinst $DMSB_GRE_TUNNEL.`"
    for greInst in $GRE_ALL_INST
    do
        isTunnelEnabled="`psmcli get $DMSB_GRE_TUNNEL.$greInst.Enable`"
        if [ "$isTunnelEnabled" = "TRUE" -o "$isTunnelEnabled" = 1 ]
        then
            isIntfEnabled="`psmcli get $DMSB_GRE_TUNNEL.$greInst.$GRE_INTERFACE.1.Enable`"
            if [ "$isIntfEnabled" = "TRUE" -o "$isIntfEnabled" = 1 ]
            then
                GRE_TUN_IF_NAME="`psmcli get $DMSB_GRE_TUNNEL.$greInst.$GRE_INTERFACE.1.Name`"
                BridgeInst=`psmcli get ${greInst}_BridgeInstance`
                PortInst=`psmcli get ${greInst}_PortInstance`

                DEL_FOR_HS=0
                echo "Deleting the tunnel and recreating it " > /dev/console
                delete_tunnel  $DEL_FOR_HS $greInst $BridgeInst $PortInst

                currentBrMembers=`sysevent get multinet_${BridgeInst}-allMembers`
                output=`echo $currentBrMembers | grep ${GRE_TUN_IF_NAME}`
                while [ x != x"$output" ]
                do
                    currentBrMembers=`sysevent get multinet_${BridgeInst}-allMembers`
                    output=`echo $currentBrMembers | grep ${GRE_TUN_IF_NAME}`
                done
                REM_EP=`sysevent get ${greInst}_current_remoteEP`
                create_tunnel $greInst $REM_EP
                dmcli eRT setv Device.Bridging.Bridge.$BridgeInst.Port.$PortInst.Enable bool true
            fi
        fi
    done
}

case "$1" in

    igre-start)
        # $2 is the GRE Tunnel Instance
        tunnelInst=$2
        REMOTEIP="`psmcli get $DMSB_GRE_TUNNEL.$tunnelInst.$GRE_TUNNEL_REMOTEENDPOINTS`"
        IFS=","
        set -- $REMOTEIP
        PRIMARY=$1
        SECONDARY=$2
        psmcli set $DMSB_GRE_TUNNEL.$tunnelInst.PrimaryEndPoint $PRIMARY
        psmcli set $DMSB_GRE_TUNNEL.$tunnelInst.SecondaryEndPoint $SECONDARY

        create_tunnel $tunnelInst $PRIMARY

        bring_up_bridge_on_boot $tunnelInst
    ;;

    igre-stop)

        DEL_FOR_HS=0
        Br_Inst=`psmcli get ${2}_BridgeInstance`
        Port_Inst=`psmcli get ${2}_PortInstance`
        delete_tunnel $DEL_FOR_HS $2 $Br_Inst $Port_Inst
    ;;

    igre-hotspot-stop)
        #This event is called when hotspot bridge is brought down
        hs_br_inst=`sysevent get hotspot_bridge`
        if [ x$hs_br_inst = x${2} ]; then
            hs_gre_inst=`sysevent get hotspot_gre_tunnel_inst`
            stop_hotspot $hs_gre_inst
        fi
    ;;

    ipv6_dhcp6_addr)
        find_active_gre_tunnels
    ;;

    igre-bringup-gre-hs)
        # This event will be set the bridge instance when multinet-start happens
        # $2 is the bridge instance
        set_firewall_rules $2
    ;;
    
        wan-status)
        # $2 - Refers to the wan-status
        if [ x"started" = x${2} ]; then
          sysevent set hotspotfd-enable 1
       else
          sysevent set hotspotfd-enable 0
        fi
        check_ssids
    ;;

    hotspotfd-tunnelEP)
        # $2 - Refers to the remote EP the hotspot process wants us to use
        hs_br_inst=`sysevent get hotspot_bridge`
        hs_gre_inst=`sysevent get hotspot_gre_tunnel_inst`
        hs_br_port_inst=`sysevent get hotspot_br_port`
        DEL_FOR_HS=1
        curep=`sysevent get ${hs_gre_inst}_current_remoteEP`
        if [ x"" != x"$curep" -a x"$curep" != x"${2}" ]; then
            delete_tunnel $DEL_FOR_HS $hs_gre_inst $hs_br_inst $hs_br_port_inst
        fi

        if [ x"NULL" != x"${2}" ]; then
            create_tunnel $hs_gre_inst $2
            dmcli eRT setv Device.Bridging.Bridge.$hs_br_inst.Port.$hs_br_port_inst.Enable bool true
            ipv4_gre_rule=`sysevent setunique GeneralPurposeFirewallRule " -A INPUT -s $2 -p gre -j ACCEPT"`
            sysevent set ${hs_gre_inst}_ipv4_gre_rule $ipv4_gre_rule
            sysevent set firewall-restart
        fi
        check_ssids
    ;;
esac
