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
# ENTRY
#------------------------------------------------------------------

TYPE=Gre

GRE_IFNAME="gretap0"
GRE_IFNAME_DUMMY="gretap_0"
recover="false"
hotspot_down_notification="false"
SYSEVENT="sysevent"

source /etc/utopia/service.d/ut_plat.sh
source /etc/utopia/service.d/log_capture_path.sh
. /etc/device.properties
source /lib/rdk/t2Shared_api.sh
THIS=/etc/utopia/service.d/service_multinet/handle_gre.sh

if [ "$BOX_TYPE" = "XF3" ] ; then
   export LOG4C_RCPATH=/etc
   BINPATH=/usr/ccsp
else

   if [ "$BOX_TYPE" = "XB3" ] ; then
      BINPATH=/fss/gw/usr/ccsp
   else
      BINPATH=/usr/bin/
   fi
   export LOG4C_RCPATH=/fss/gw/rdklogger
fi

MTU_VAL=1400
MSS_VAL=1360

GRE_PSM_BASE=dmsb.cisco.gre
#HS_PSM_BASE=dmsb.hotspot.gre
HS_PSM_BASE=dmsb.hotspot.tunnel
GRE_PSM_NAME=name
#format for below is comma delimited FQDM
GRE_PSM_BRIDGES=AssociatedBridges 
#GRE_PSM_KAINT=KeepAlive.Interval
GRE_PSM_KAINT=RemoteEndpointHealthCheckPingInterval
#GRE_PSM_KAFINT=KeepAlive.FailInterval
GRE_PSM_KAFINT=RemoteEndpointHealthCheckPingIntervalInFailure
#GRE_PSM_KARECON=ReconnPrimary
GRE_PSM_KARECON=ReconnectToPrimaryRemoteEndpoint
#GRE_PSM_KATHRESH=KeepAlive.Threshold
GRE_PSM_KATHRESH=RemoteEndpointHealthCheckPingFailThreshold
#GRE_PSM_KAPOLICY=KeepAlive.Policy
GRE_PSM_KAPOLICY=KeepAlivePolicy
GRE_PSM_TOS=tos
GRE_PSM_KEY=key
GRE_PSM_CSUM=csumenabled
GRE_PSM_SEQ=seqnumenabled
#GRE_PSM_ENDPOINTS=Endpoints 
GRE_PSM_PRIENDPOINTS=PrimaryRemoteEndpoint
GRE_PSM_SECENDPOINTS=SecondaryRemoteEndpoint
GRE_PSM_ENDPOINT=endpoint
#GRE_PSM_KACOUNT=KeepAlive.Count
GRE_PSM_KACOUNT=RemoteEndpointHealthCheckPingCount
#GRE_PSM_SNOOPCIRC=DHCP.CircuitIDSSID
GRE_PSM_SNOOPCIRC=EnableCircuitID
#GRE_PSM_SNOOPREM=DHCP.RemoteID
GRE_PSM_SNOOPREM=EnableRemoteID
GRE_PSM_ENABLE=enable
HS_PSM_ENABLE=Enable
GRE_PSM_LOCALIFS=LocalInterfaces   
WIFI_PSM_PREFIX=eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID
WIFI_RADIO_INDEX=RadioIndex

GRE_ARP_PROC=hotspot_arpd
HOTSPOT_COMP=CcspHotspot
ARP_NFQUEUE=0

WAN_IF=erouter0

AB_SSID_DELIM=':'
AB_DELIM=","

BASEQUEUE=1


wait_for_erouter0_ready(){
    while [ "`$SYSEVENT get wan-status`" != "started" ] ; do
        echo_t "Waiting for erouter0 to be ready..."
        sleep 1
    done
}

init_snooper_sysevents () {
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
}

read_greInst()
{

       inst=1
       count=0

   eval `psmcli get -e BRIDGE_INST_1 $HS_PSM_BASE.${inst}.interface.1.$GRE_PSM_BRIDGES BRIDGE_INST_2 $HS_PSM_BASE.${inst}.interface.2.$GRE_PSM_BRIDGES BRIDGE_INST_3 $HS_PSM_BASE.${inst}.interface.3.$GRE_PSM_BRIDGES BRIDGE_INST_4 $HS_PSM_BASE.${inst}.interface.4.$GRE_PSM_BRIDGES BRIDGE_INST_5 $HS_PSM_BASE.${inst}.interface.5.$GRE_PSM_BRIDGES WECB_BRIDGES dmsb.wecb.hhs_extra_bridges NAME $GRE_PSM_BASE.${inst}.$GRE_PSM_NAME`

        set '5 6 9 10 16'

        for i in $@; do
            count=`expr $count + 1`
            eval bridgeinfo=\${BRIDGE_INST_${count}}
            succeed_check=`dmcli eRT getv Device.WiFi.SSID.$i.Enable | grep value | cut -f3 -d : | cut -f2 -d " "`
            if [ "true" = "$succeed_check" ]; then
                  BRIDGE_INSTS="$BRIDGE_INSTS $bridgeinfo"
            fi
        done
        
       echo "BRIDGE_INSTS === $BRIDGE_INSTS"
       
      if [ "$BRIDGE_INSTS" == "" ]
      then
          touch "/tmp/tunnel_destroy_flag"
      else
          if [ -f /tmp/tunnel_destroy_flag ]
          then
               rm /tmp/tunnel_destroy_flag
          fi
      fi
     
       wait_for_erouter0_ready
       for i in $BRIDGE_INSTS; do
          brinst=`echo $i |cut -d . -f 4`
          sysevent set multinet-start $brinst
       done
 }


#args: remote endpoint, gre tunnel ifname
create_tunnel () {
    echo "Creating tunnel... remote:$1"
 if [ "$BOX_TYPE" = "XF3" ] ; then
    echo "read_tunnel_param $2"  
    while true 
    do 
       if [ -f /tmp/destroy_tunnel_lock ];then
               sleep 3
        else 
               break
         fi
       
    done 
 fi  
    read_tunnel_params $2
    
    local extra=""
    if [ x1 = x$CSUM ]; then
        extra="csum"
    fi
    
    if [ x != x$KEY ]; then
        extra="$extra key $KEY"
    fi
    
    if [ x != x$TOS ]; then
        extra="$extra dsfield $TOS"
    fi
    
    # TODO: sequence number
    
    # TODO: use assigned lower layer instead
    WAN_IF=`sysevent get current_wan_ifname`
    
    update_bridge_frag_config $inst $1
    
    isgretap0Present=`ip link show | grep gretap0`
    if [ "$isgretap0Present" != "" ]; then
        echo "gretap0 is already present rename it before creating"
        ip link set dev $GRE_IFNAME name $GRE_IFNAME_DUMMY
    fi

    if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ] ; then
    	#Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    	ip link add $2 type gretap remote $1 dev $WAN_IF $extra nopmtudisc
    	ip link set $2 txqueuelen 1000 mtu 1500
    else
    	ip link add $2 type gretap remote $1 dev $WAN_IF $extra
    fi
    ifconfig $2 up
    sysevent set gre_current_endpoint $1
    sysevent set if_${2}-status $IF_READY
}

destroy_tunnel () {
   if [ "$BOX_TYPE" = "XF3" ] ; then
      touch "/tmp/destroy_tunnel_lock"
   fi
    echo "Destroying tunnel... remote"
    #kill `sysevent get dhcpsnoopd_pid`
   if [ "$BOX_TYPE" = "XF3" ] ; then
    echo " destroying $1"
   fi
    ip link del $1
   if [ "$BOX_TYPE" = "XF3" ] ; then
    echo "setting sysevent with null"
   fi
    sysevent set gre_current_endpoint
    #sysevent set dhcpsnoopd_pid
    #sysevent set snooper-log-enable 0
    sysevent set if_${1}-status $IF_DOWN
  if [ "$BOX_TYPE" = "XF3" ] ; then
    sleep 5
    rm "/tmp/destroy_tunnel_lock"
  fi
 
}

gre_preproc () {
    #zqiu: fix for XHH 5G not get IP issue
    #if [ -f $UTOPIAROOT/hhs_validate.sh ]; then
    #    $UTOPIAROOT/hhs_validate.sh
    #fi
    allGreInst="`psmcli getallinst $GRE_PSM_BASE.`"
    query=""
    
    # TODO break 1 to 1 dependence on instance numbers (hotspot and gre interface)
    for i in $allGreInst; do 
        query="$query GRE_$i $GRE_PSM_BASE.$i.$GRE_PSM_NAME"
    done
    
    eval `psmcli get -e $query`
    
    for i in $allGreInst; do
        eval sysevent set gre_\${GRE_${i}}_inst $i
    done
}

init_keepalive_sysevents () {
    keepalive_args="-n `sysevent get wan_ifname`"
    #PRIMARY=`echo $ENDPOINTS | cut -f 1 -d ","`
    #SECONDARY=`echo $ENDPOINTS | cut -f 2 -d "," -s`
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
        sysevent set hotspotfd-max-secondary $KA_RECON_PRIM
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

    ##Disable the keepalive until we see an associated client
    #sysevent set hotspotfd-enable 0
    
    sysevent set hotspotfd-log-enable 1
    
}

bInst_to_bNames () {
    BRIDGES=""
    local binst=""
    local query=""
    local num=0
    local num2=0
    OLD_IFS="$IFS"
    
    IFS="$AB_SSID_DELIM"
    for x in $2; do 
        num=`expr $num + 1`
        IFS="$AB_DELIM"
        for i in $x; do
            num2=`expr $num2 + 1`
#            binst=`echo $i |cut -d . -f 4`
            query="$query WECBB_${num}_${num2} $NET_IDS_DM.$i.$NET_IFNAME"
            eval WECBB_${num}=\"\${WECBB_${num}} \"\'\$WECBB_\'${num}'_'${num2}
        done
        IFS="$AB_SSID_DELIM"
#        eval BRIDGE_$num=\$AB_SSID_DELIM
    done
    
    num=0
    IFS="$AB_DELIM"
    for i in $1; do
        num=`expr $num + 1`
        binst=`echo $i |cut -d . -f 4`
        query="$query BRIDGE_$num $NET_IDS_DM.$binst.$NET_IFNAME"
    done
    IFS="$OLD_IFS"
    
    if [ x != x"$query" ]; then
        eval `eval psmcli get -e $query`
    fi
    
    
    for i in `seq $num`; do
        eval eval BRIDGES=\\\"\\\$BRIDGES \${BRIDGE_${i}} \${WECBB_${i}} \\\$AB_SSID_DELIM \\\"
    done
}

read_init_params () {
	gre_preproc
    #zqiu: short term fix for XHH 5G not get IP issue
    inst=`dmcli eRT setv Device.Bridging.Bridge.4.Port.2.LowerLayers string Device.WiFi.SSID.6`;

    inst=`sysevent get gre_$1_inst`
    #eval `psmcli get -e ENDPOINTS $HS_PSM_BASE.${inst}.$GRE_PSM_ENDPOINTS BRIDGE_INSTS $HS_PSM_BASE.${inst}.$GRE_PSM_BRIDGES  KA_INTERVAL $HS_PSM_BASE.${inst}.$GRE_PSM_KAINT KA_FAIL_INTERVAL $HS_PSM_BASE.${inst}.$GRE_PSM_KAFINT KA_POLICY $HS_PSM_BASE.${inst}.$GRE_PSM_KAPOLICY KA_THRESH $HS_PSM_BASE.${inst}.$GRE_PSM_KATHRESH KA_COUNT $HS_PSM_BASE.${inst}.$GRE_PSM_KACOUNT KA_RECON_PRIM $HS_PSM_BASE.${inst}.$GRE_PSM_KARECON SNOOP_CIRCUIT $HS_PSM_BASE.${inst}.$GRE_PSM_SNOOPCIRC SNOOP_REMOTE $HS_PSM_BASE.${inst}.$GRE_PSM_SNOOPREM WECB_BRIDGES dmsb.wecb.hhs_extra_bridges`
    eval `psmcli get -e PRIMARY $HS_PSM_BASE.${inst}.$GRE_PSM_PRIENDPOINTS SECONDARY $HS_PSM_BASE.${inst}.$GRE_PSM_SECENDPOINTS BRIDGE_INST_1 $HS_PSM_BASE.${inst}.interface.1.$GRE_PSM_BRIDGES BRIDGE_INST_2 $HS_PSM_BASE.${inst}.interface.2.$GRE_PSM_BRIDGES BRIDGE_INST_3 $HS_PSM_BASE.${inst}.interface.3.$GRE_PSM_BRIDGES BRIDGE_INST_4 $HS_PSM_BASE.${inst}.interface.4.$GRE_PSM_BRIDGES BRIDGE_INST_5 $HS_PSM_BASE.${inst}.interface.5.$GRE_PSM_BRIDGES KA_INTERVAL $HS_PSM_BASE.${inst}.$GRE_PSM_KAINT KA_FAIL_INTERVAL $HS_PSM_BASE.${inst}.$GRE_PSM_KAFINT KA_POLICY $HS_PSM_BASE.${inst}.$GRE_PSM_KAPOLICY KA_THRESH $HS_PSM_BASE.${inst}.$GRE_PSM_KATHRESH KA_COUNT $HS_PSM_BASE.${inst}.$GRE_PSM_KACOUNT KA_RECON_PRIM $HS_PSM_BASE.${inst}.$GRE_PSM_KARECON SNOOP_CIRCUIT $HS_PSM_BASE.${inst}.$GRE_PSM_SNOOPCIRC SNOOP_REMOTE $HS_PSM_BASE.${inst}.$GRE_PSM_SNOOPREM WECB_BRIDGES dmsb.wecb.hhs_extra_bridges`

    status=$?
    if [ "$status" != "0" ]
    then
        echo "WARNING: handle_gre.sh read_init_params: psmcli return $status"
    fi
    echo "PRIMARY $PRIMARY SECONDARY $SECONDARY"
    if [ "$PRIMARY" = "" ] || [ "$SECONDARY" = "" ]
    then
        echo "WARNING: handle_gre.sh read_init_params: PRIMARY/SECONDARY NULL"
    fi
    echo "KA_INTERVAL $KA_INTERVAL KA_FAIL_INTERVAL $KA_FAIL_INTERVAL KA_POLICY $KA_POLICY"
    if [ "$KA_INTERVAL" = "" ]
    then
        echo "WARNING: handle_gre.sh read_init_params: KA_INTERVAL NULL"
    fi
    echo "KA_THRESH $KA_THRESH KA_COUNT $KA_COUNT KA_RECON_PRIM $KA_RECON_PRIM"

  BRIDGE_INSTS="$BRIDGE_INST_1,$BRIDGE_INST_2,$BRIDGE_INST_3,$BRIDGE_INST_4,$BRIDGE_INST_5"

    bInst_to_bNames "$BRIDGE_INSTS" "$WECB_BRIDGES"

}

read_tunnel_params () {
    inst=`sysevent get gre_$1_inst`
    eval `psmcli get -e KEY $GRE_PSM_BASE.${inst}.$GRE_PSM_KEY CSUM $GRE_PSM_BASE.${inst}.$GRE_PSM_CSUM TOS $GRE_PSM_BASE.${inst}.$GRE_PSM_TOS`
}

#args: gre ifname
update_bridge_config () {
    inst=`sysevent get gre_$1_inst`
    curBridges="`sysevent get gre_${inst}_current_bridges`"
    
    if [ x != x"$curBridges" ]; then
        remove_bridge_config ${inst} "$curBridges"
    fi
    
    queue=$BASEQUEUE
    
    for br in $BRIDGES; do
        if [ "$AB_SSID_DELIM" = $br ]; then
            queue=`expr $queue + 1`
            continue
        fi
        #changed the queue value for nf queue issue
        if [ $br = "brpublic" ] || [ $br = "brpub" ]; then
           queue=45                               
        fi
        br_snoop_rule="`sysevent setunique GeneralPurposeFirewallRule " -A FORWARD -o $br -p udp --dport=67:68 -j NFQUEUE --queue-bypass --queue-num $queue"`"
        sysevent set gre_${inst}_${br}_snoop_rule "$br_snoop_rule"
  if [ "$BOX_TYPE" = "XF3" ] ; then
       sleep 5
  fi
        
        
        br_mss_rule=`sysevent setunique GeneralPurposeMangleRule " -A POSTROUTING -o $br -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss $MSS_VAL"`
        sysevent set gre_${inst}_${br}_mss_rule "$br_mss_rule"
       if [ "$BOX_TYPE" = "XF3" ] ; then
       sleep 5
       fi
    done
    
    sysevent set gre_${inst}_current_bridges "$BRIDGES"
   if [ "$BOX_TYPE" = "XF3" ] ; then
     sleep 10
   fi
    
}
# TODO: verify indexes and proper teardown
#args gre ifname, "bridge list"
remove_bridge_config () {
        for br in $2; do
            if [ "$AB_SSID_DELIM" = $br ]; then
                continue
            fi
            sysevent set `sysevent get gre_${1}_${br}_snoop_rule`
            sysevent set `sysevent get gre_${1}_${br}_mss_rule`
        done
}

#args: gre inst, gw ip
update_bridge_frag_config () {

    BRIDGES="`sysevent get gre_${1}_current_bridges`"
    for br in $BRIDGES; do
        if [ "$AB_SSID_DELIM" = $br ]; then
            continue
        fi
   if [ "$BOX_TYPE" = "XB6" ] ; then
        echo add br=$br mtu=$MTU_VAL icmp=y segment=y gw=$2 > /proc/net/mtu_mod
   fi
    done
}

#args: gre instance
#out: $ssids - ssid instances
#       $radios - radio instances
#       $ssid_${instance}_radio - radio for the specified ssid
get_ssids() {


       localif_1=`psmcli get $HS_PSM_BASE.${1}.interface.1.$GRE_PSM_LOCALIFS`
       localif_2=`psmcli get $HS_PSM_BASE.${1}.interface.2.$GRE_PSM_LOCALIFS`
       localif_3=`psmcli get $HS_PSM_BASE.${1}.interface.3.$GRE_PSM_LOCALIFS`
       localif_4=`psmcli get $HS_PSM_BASE.${1}.interface.4.$GRE_PSM_LOCALIFS`
       localif_5=`psmcli get $HS_PSM_BASE.${1}.interface.5.$GRE_PSM_LOCALIFS`

       count=0
       set '5 6 9 10 16'
       
       for i in $@; do          
            count=`expr $count + 1`
            eval localinfo=\${localif_${count}}
            succeed_check=`dmcli eRT getv Device.WiFi.SSID.$i.Enable | grep value | cut -f3 -d : | cut -f2 -d " "`
            if [ "true" = "$succeed_check" ]; then
               localifs="$localifs $localinfo"                                                                                             
            fi
            if [ $hotspot_down_notification = "true" ]; then
                localifs="$localifs $localinfo"
            fi
       done
       
        ssids=""                                                                                                                                                              
        for i in $localifs; do                                                                                                                                                 
            winst=`echo $i |cut -d . -f 4`                                                                                                                                     
            ssids="$ssids $winst"                                                                                                                                              
            radio=`dmcli eRT getv ${i}LowerLayers  | grep string,  | awk '{print $5}' | cut -d . -f 4 `                                                                         
            expr match "$radios" '.*\b\('$radio'\)\b.*' > /dev/null                                                                                                             
            if [ 0 != $? ]; then                                                                                                                                                
              #add this radio instance                                                                                                                                         
              radios="$radios $radio"  
              eval mask_$radio=0                                                                                                                                               
            fi                                                                                                                                                                  
            eval ssid_${winst}_radio=$radio                                                                                                                                     
         done                                                                                                                                                                 

}

#args: gre instance, true | false
#bring_down_ssids () {
set_ssids_enabled() {
    #delay SSID manipulation if requested
    sleep `sysevent get hotspot_$1-delay` 2> /dev/null
    sysevent set hotspot_$1-delay 0
    
    get_ssids $1
    for instance in $ssids; do
       dmcli eRT setv Device.WiFi.SSID.${instance}.X_CISCO_COM_RouterEnabled bool $2 &
       dmcli eRT setv Device.WiFi.SSID.${instance}.X_CISCO_COM_EnableOnline bool true &
        eval eval mask=\\\${mask_\${ssid_${instance}_radio}}
        eval eval mask_\${ssid_${instance}_radio}=$(( (2 ** ($instance - 1)) + $mask )) 
    if [ "$BOX_TYPE" = "TCCBR" ] ; then
       dmcli eRT setv Device.WiFi.SSID.${instance}.Enable bool $2 &
    fi
    done
    for rad in $radios; do
        echo "Executing ApplySetting"
        eval dmcli eRT setv Device.WiFi.Radio.$rad.X_CISCO_COM_ApplySettingSSID int \${mask_${rad}}
        dmcli eRT setv Device.WiFi.Radio.$rad.X_CISCO_COM_ApplySetting bool true &
    done
    
    sysevent set hotspot_ssids_up $2
}

# NOTE: Hard coded solution
set_apisolation() {
    get_ssids $1
    for instance in $ssids; do
        dmcli eRT setv Device.WiFi.AccessPoint.$instance.IsolationEnable bool true
    done
}

#args: "wifi instances"
kick_clients () {
    get_ssids $1
    for instance in $ssids; do
        dmcli eRT setv Device.WiFi.AccessPoint.${instance}.X_CISCO_COM_KickAssocDevices bool true &
    done
}
#args: hotspot instance
kill_procs () {
#     kill `sysevent get ${1}_keepalive_pid`
#     sysevent set ${1}_keepalive_pid
#     
#     kill `sysevent get dhcpsnoopd_pid`
#     sysevent set dhcpsnoopd_pid
# TODO: develop scheme for only killing related pids. background task var $1 doesn't work as these processes daemonize
    killall $HOTSPOT_COMP
    sysevent set ${1}_keepalive_pid
	if [ -f /tmp/hotspot_arpd_up ]; then
		rm -rf /tmp/hotspot_arpd_up
	fi
    killall $GRE_ARP_PROC
    
}
#args: hotspot instance
hotspot_down() {

    inst=$1
    
    #Set a delay for SSID manipulation
    sysevent set hotspot_${inst}-delay 10
    
    sysevent rm_async `sysevent get gre_wan_async`
    sysevent rm_async `sysevent get gre_ep_async`
#    sysevent rm_async `sysevent get gre_snooper_clients_async`
    sysevent rm_async `sysevent get gre_primary_async`
    sysevent set gre_ep_async
    sysevent set gre_wan_async
#    sysevent set gre_snooper_clients_async
    sysevent get gre_primary_async
    
    #bridgeFQDM=`psmcli get $HS_PSM_BASE.${inst}.$GRE_PSM_BRIDGES`	
    BRIDGE_INST_1=`psmcli get $HS_PSM_BASE.${inst}.interface.1.$GRE_PSM_BRIDGES`
    BRIDGE_INST_2=`psmcli get $HS_PSM_BASE.${inst}.interface.2.$GRE_PSM_BRIDGES`
    BRIDGE_INST_3=`psmcli get $HS_PSM_BASE.${inst}.interface.3.$GRE_PSM_BRIDGES`
    BRIDGE_INST_4=`psmcli get $HS_PSM_BASE.${inst}.interface.4.$GRE_PSM_BRIDGES`
    BRIDGE_INST_5=`psmcli get $HS_PSM_BASE.${inst}.interface.5.$GRE_PSM_BRIDGES`
    bridgeFQDM="$BRIDGE_INST_1,$BRIDGE_INST_2,$BRIDGE_INST_3,$BRIDGE_INST_4,$BRIDGE_INST_5"
	
    remove_bridge_config ${inst} "`sysevent get gre_${inst}_current_bridges`"

    brinst=""
    OLD_IFS="$IFS"
    IFS=","
    for i in $bridgeFQDM; do
        brinst=`echo $i |cut -d . -f 4`
        sysevent set multinet-down $brinst
    done
    IFS="$OLD_IFS"
    
    kill_procs $inst
    
    #This hotspot_down_notification is to notify that ssid's are checked when tunnel is destroyed (hotspot to be down) 
    hotspot_down_notification="true";
    set_ssids_enabled $inst false
    hotspot_down_notification="false";
    
    sysevent set `sysevent get ${inst}_arp_queue_rule`
    sysevent set ${inst}_arp_queue_rule
    
    sysevent set hotspotfd-tunnelEP
    sysevent set snooper-wifi-clients
    
    sysevent set hotspot_${inst}-status stopped
    
    sysevent set hotspot_ssids_up
    sysevent set hotspot_${inst}-delay

}
#args: hotspot instance
hotspot_up() {
    inst=$1
    #eval `psmcli get -e bridgeFQDM $HS_PSM_BASE.${inst}.$GRE_PSM_BRIDGES ENABLED $HS_PSM_BASE.${inst}.$HS_PSM_ENABLE GRE_ENABLED $GRE_PSM_BASE.${inst}.$GRE_PSM_ENABLE WECB_BRIDGES dmsb.wecb.hhs_extra_bridges`
#TCCBR doesnot support BRIDGE_INST_3 and BRIDGE_INST_4, skip this after completing RDKB-20382
	if [ "$BOX_TYPE" = "TCCBR" ]; then
		eval `psmcli get -e BRIDGE_INST_1 $HS_PSM_BASE.${inst}.interface.1.$GRE_PSM_BRIDGES BRIDGE_INST_2 $HS_PSM_BASE.${inst}.interface.2.$GRE_PSM_BRIDGES BRIDGE_INST_3 $HS_PSM_BASE.${inst}.interface.3.$GRE_PSM_BRIDGES BRIDGE_INST_4 $HS_PSM_BASE.${inst}.interface.4.$GRE_PSM_BRIDGES BRIDGE_INST_5 $HS_PSM_BASE.${inst}.interface.5.$GRE_PSM_BRIDGES ENABLED $HS_PSM_BASE.${inst}.$HS_PSM_ENABLE GRE_ENABLED $GRE_PSM_BASE.${inst}.$GRE_PSM_ENABLE WECB_BRIDGES dmsb.wecb.hhs_extra_bridges`

                bridgeFQDM="$BRIDGE_INST_1,$BRIDGE_INST_2,$BRIDGE_INST_3,$BRIDGE_INST_4,$BRIDGE_INST_5"
	else
		eval `psmcli get -e BRIDGE_INST_1 $HS_PSM_BASE.${inst}.interface.1.$GRE_PSM_BRIDGES BRIDGE_INST_2 $HS_PSM_BASE.${inst}.interface.2.$GRE_PSM_BRIDGES BRIDGE_INST_3 $HS_PSM_BASE.${inst}.interface.3.$GRE_PSM_BRIDGES BRIDGE_INST_4 $HS_PSM_BASE.${inst}.interface.4.$GRE_PSM_BRIDGES BRIDGE_INST_5 $HS_PSM_BASE.${inst}.interface.5.$GRE_PSM_BRIDGES ENABLED $HS_PSM_BASE.${inst}.$HS_PSM_ENABLE GRE_ENABLED $GRE_PSM_BASE.${inst}.$GRE_PSM_ENABLE WECB_BRIDGES dmsb.wecb.hhs_extra_bridges`

		if [ x"1" != x$ENABLED -o x"1" != x$GRE_ENABLED ]; then
			exit 0;
		fi


                    eval `psmcli get -e BRIDGE_INST_1 $HS_PSM_BASE.${inst}.interface.1.$GRE_PSM_BRIDGES BRIDGE_INST_2 $HS_PSM_BASE.${inst}.interface.2.$GRE_PSM_BRIDGES BRIDGE_INST_3 $HS_PSM_BASE.${inst}.interface.3.$GRE_PSM_BRIDGES BRIDGE_INST_4 $HS_PSM_BASE.${inst}.interface.4.$GRE_PSM_BRIDGES WECB_BRIDGES dmsb.wecb.hhs_extra_bridges NAME $GRE_PSM_BASE.${inst}.$GRE_PSM_NAME`
                    count=0
                    bridgeFQDM=""
                    set '5 6 9 10 16'
                    for i in $@; do
                        count=`expr $count + 1`
                        eval bridgeinfo=\${BRIDGE_INST_${count}}
                        succeed_check=`dmcli eRT getv Device.WiFi.SSID.$i.Enable | grep value | cut -f3 -d : | cut -f2 -d " "`               
                         if [ "true" = "$succeed_check" ]; then
                           bridgeFQDM="$bridgeFQDM $bridgeinfo"
                         fi
                    done      
	fi
        
        if [ "$bridgeFQDM" == "" ]
        then
          touch "/tmp/tunnel_destroy_flag"
        else
           if [ -f /tmp/tunnel_destroy_flag ]
           then
               rm /tmp/tunnel_destroy_flag
           fi
        fi
    #Set a delay for first SSID manipulation
    sysevent set hotspot_${inst}-delay 10

    #Hard code for old images
    set_apisolation $inst
    wait_for_erouter0_ready
    brinst=""
    if [ "$BOX_TYPE" = "TCCBR" ] ; then
        OLD_IFS="$IFS"
        IFS=","
    fi
    for i in $bridgeFQDM; do
        brinst=`echo $i |cut -d . -f 4`
        echo "brinst is $brinst"
        sysevent set multinet-start $brinst
    done
    if [ "$BOX_TYPE" = "TCCBR" ] ; then
       IFS="$OLD_IFS"
    fi
    
    for i in $WECB_BRIDGES; do
   if [ "$BOX_TYPE" = "XF3" ] ; then
    echo " setting sysevent hotspot_${inst}-status started"
   fi    
    sysevent set multinet-start $i
    done
    
    sysevent set hotspot_${inst}-status started
    
}

check_ssids () {
    EP="`sysevent get hotspotfd-tunnelEP`"
    WAN="`sysevent get wan-status`"
    curSSIDSTATE="`sysevent get hotspot_ssids_up`"
    
    if [ x = x$1 ]; then
            allGreInst="`psmcli getallinst $HS_PSM_BASE.`"
            inst=`echo $allGreInst | cut -f 1`
            if [ x = x$inst ]; then
                exit 0
            fi
    else
        inst=$1
    fi
    
    if [ x"started" = x$WAN -a x != x$EP -a x"0.0.0.0" != x$EP ]; then
        if [ x"true" = x$curSSIDSTATE ]; then
            kick_clients 1
        else
            set_ssids_enabled $inst true > /dev/null
        fi
    else
        #SSIDs should be down
        if [ x"true" = x$curSSIDSTATE ]; then
            set_ssids_enabled $inst false > /dev/null
        fi
    fi
}

#args: 
set_wecb_bridges() {
    # TODO: parameterize the instance number "1"
    #BRIDGE_INS="`psmcli get $HS_PSM_BASE.1.$GRE_PSM_BRIDGES`"	
    BRIDGE_INST_1="`psmcli get $HS_PSM_BASE.1.interface.1.$GRE_PSM_BRIDGES`"
	BRIDGE_INST_2="`psmcli get $HS_PSM_BASE.1.interface.2.$GRE_PSM_BRIDGES`"
	BRIDGE_INST_3="`psmcli get $HS_PSM_BASE.1.interface.3.$GRE_PSM_BRIDGES`"
	BRIDGE_INST_4="`psmcli get $HS_PSM_BASE.1.interface.4.$GRE_PSM_BRIDGES`"
	BRIDGE_INST_5="`psmcli get $HS_PSM_BASE.1.interface.5.$GRE_PSM_BRIDGES`"
	BRIDGE_INS="$BRIDGE_INST_1,$BRIDGE_INST_2,$BRIDGE_INST_3,$BRIDGE_INST_4,$BRIDGE_INST_5"
	
    local binst=""
    local query=""
    local num=0
    OLD_IFS="$IFS"
    
    IFS="$AB_SSID_DELIM"
    for x in $BRIDGE_INS; do 
        IFS="$AB_DELIM"
        for i in $x; do
            if [ 0 = $num ]; then
                num=1
                continue
            fi
            binst=`echo $i |cut -d . -f 4`
            query="$query $binst"
        done
        num=0
    done
    IFS="$OLD_IFS"
    
    sysevent set wecb_hhs_bridges "$query"
}


#service_init
case "$1" in
#  Synchronous calls from bridge
    #Args: netid, members
    create)
        echo "GRE CREATE: $3" > /dev/console
        
        read_init_params $3
        
        #Initialize
        if [ x = x`sysevent get ${inst}_keepalive_pid` ]; then
            echo "GRE INITIALIZING..." > /dev/console
            async="`sysevent async hotspotfd-tunnelEP $THIS`"
            sysevent set gre_ep_async "$async" > /dev/null
#             async="`sysevent async snooper-wifi-clients $THIS`"
#             sysevent set gre_snooper_clients_async "$async" > /dev/null
            async="`sysevent async wan-status $THIS`"
            sysevent set gre_wan_async "$async" > /dev/null
            async="`sysevent async hotspotfd-primary $THIS`"
            sysevent set gre_primary_async "$async" > /dev/null
            
            init_keepalive_sysevents > /dev/null
            init_snooper_sysevents
            sysevent set snooper-log-enable 1
            echo "Starting hotspot component" > /dev/console
            $HOTSPOT_COMP -subsys eRT. > /dev/null &
            sysevent set ${inst}_keepalive_pid $! > /dev/null
            
            update_bridge_config $3 > /dev/null
            if [ "$BOX_TYPE" = "XF3" ] ; then
            sleep 5
            fi
            arpFWrule=`sysevent setunique GeneralPurposeFirewallRule " -I OUTPUT -o $WAN_IF -p icmp --icmp-type 3 -j NFQUEUE --queue-bypass --queue-num $ARP_NFQUEUE"`
            sysevent set ${inst}_arp_queue_rule "$arpFWrule" > /dev/null
            if [ "$BOX_TYPE" = "XF3" ] ; then
              sleep 5
            fi
            $GRE_ARP_PROC -q $ARP_NFQUEUE  > /dev/null &
            echo "handle_gre : Triggering RDKB_FIREWALL_RESTART"
	    t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
            sysevent set firewall-restart > /dev/null
            if [ "$BOX_TYPE" = "XF3" ] ; then
               sleep 15
               brctl1=`brctl show | grep wl0.2`
               if [ "$brctl1" == "" ]; then           
                       brctl addif brlan2 wl0.2     
                       brctl addif brlan2 wl1.2  
                        ifconfig wl0.2 up
                        ifconfig wl1.2 up   
               fi                                   
                                                    
               brctl2=`brctl show | grep gretap0.102` 
               if [ "$brctl2" == "" ]; then                         
                       vconfig add gretap0 102      
                       ip link set gretap0.102 master brlan2
                       vconfig add gretap0 103      
                       ip link set gretap0.103 master brlan3
               fi                 
               brctl3=`ifconfig gretap0 | grep UP`         
               if [ "$brctl3" == "" ]; then
                       ifconfig gretap0 up
               fi
            fi
            #check_ssids
            
        fi
    
        if [ x"up" = x`sysevent get if_${3}-status` ]; then 
            echo ${TYPE}_READY=\"$3\"
        else
            echo ${TYPE}_READY=\"\"
        fi
        
        ;;
    destroy)
        service_stop
        ;;
    #Args: netid, netvid, members
    addVlan)
 #     if [ x"unknown" != x"$SYSEVENT_current_hsd_mode" ]; then
        
 #     fi
      ;;
    #Args: netid, netvid, members
    delVlan)
    
    ;;
      
#  Sysevent calls
    hotspotfd-tunnelEP)
    
        echo "GRE EP called : $2"

        if [ $2 = "recover" ] ; then                                    
             recover="true"                    
        fi        
 
        curep=`sysevent get gre_current_endpoint`                   
        if [ x != x$curep -a x$curep != x${2} ] || [ $recover = "true" ]; then
            destroy_tunnel $GRE_IFNAME                                        
        fi    


         if [ x"NULL" != x${2} ] || [ $recover = "true" ]; then
              echo "inside create tunnel"                             
              if [ $recover = "true" ] ; then                                    
                  TUNNEL_EP="dmcli eRT getv Device.X_COMCAST-COM_GRE.Tunnel.1.PrimaryRemoteEndpoint"
                  TUNNEL_IP=`$TUNNEL_EP`                                                            
                  curep=`echo "$TUNNEL_IP" | grep value | cut -f3 -d : | cut -f2 -d " "`            
                  echo "dmcli ip : $curep"                                   
                  create_tunnel $curep $GRE_IFNAME
                  if [ "$BOX_TYPE" = "XB3" ] ; then
                      read_greInst
                  fi
              else
                  create_tunnel $2 $GRE_IFNAME
              fi                                                                                     
         fi           

        if [ "$BOX_TYPE" = "XF3" ] ; then 
               sleep 15
               brctl1=`brctl show | grep wl0.2`
               if [ "$brctl1" == "" ]; then           
                       brctl addif brlan2 wl0.2     
                       brctl addif brlan3 wl1.2  
                        ifconfig wl0.2 up
                        ifconfig wl1.2 up   
               fi                                   
                                                    
               brctl2=`brctl show | grep gretap0.102` 
               if [ "$brctl2" == "" ]; then                         
                       vconfig add gretap0 102      
                       ip link set gretap0.102 master brlan2
                       vconfig add gretap0 103      
                       ip link set gretap0.103 master brlan3
               fi                 
               brctl3=`ifconfig gretap0 | grep UP`         
               if [ "$brctl3" == "" ]; then
                       ifconfig gretap0 up
               fi
         fi
    if [ "$BOX_TYPE" = "XB6" ] ; then
        read_greInst 
    fi
    if [ $recover != "true" ] ; then                            
        check_ssids 1                                       
    fi    
    
    ;;
    
    wan-status)
    # TODO make this multi-instance
        if [ x"started" = x${2} ]; then
            #clients=`sysevent get snooper-wifi-clients`
            #if [ x != x$clients -a x0 != x$clients ]; then
                sysevent set hotspotfd-enable 1
            #fi
            if [ x != x`sysevent get gre_current_endpoint` ]; then
                ifconfig $GRE_IFNAME up
                sysevent set if_${GRE_IFNAME}-status $IF_READY
            fi
        else
            sysevent set hotspotfd-enable 0
            ifconfig ${GRE_IFNAME} down
            sysevent set if_${GRE_IFNAME}-status $IF_DOWN
        fi
        if [ "$BOX_TYPE" != "XB6" ] && [ "$BOX_TYPE" != "TCCBR" ]; then
           check_ssids
        fi
    ;;
    
    snmp_subagent-status)
        #This is an entry point guaranteed to indicate the PSM is available.
        gre_preproc
        set_wecb_bridges
    ;;
    
    hotspot-start)
        
        if [ x"NULL" = x$2 ]; then
            allGreInst="`psmcli getallinst $HS_PSM_BASE.`"
            inst=`echo $allGreInst | cut -f 1`
            if [ x = x$inst ]; then
                exit 0
            fi
        else
            inst=$2
        fi
        
        hotspot_up $inst
    ;;
    
    hotspot-stop)
        if [ x"NULL" = x$2 ]; then
            allGreInst="`psmcli getallinst $HS_PSM_BASE.`"
            inst=`echo $allGreInst | cut -f 1`
            if [ x = x$inst ]; then
                exit 0
            fi
        else
            inst=$2
        fi
        
        hotspot_down $inst
    ;;
    
    hotspot-restart)
    
    ;;
    
    #args: hotspot gre instance
    gre-restart|gre-forceRestart)
        curr_tunnel=`sysevent get gre_current_endpoint`
        # NOTE: assuming 1-to-1, identical gre to hotspot instance mapping
        hotspot_started=`sysevent get hotspot_${2}-status`
#         if [ x = x$curr_tunnel ]; then
#             exit 0
#         fi
        set_wecb_bridges
		
		#eval `psmcli get -e bridgeFQDM $HS_PSM_BASE.${2}.$GRE_PSM_BRIDGES ENABLED $HS_PSM_BASE.${2}.$HS_PSM_ENABLE GRE_ENABLED $GRE_PSM_BASE.${2}.$GRE_PSM_ENABLE name $GRE_PSM_BASE.$2.$GRE_PSM_NAME`
        eval `psmcli get -e BRIDGE_INST_1 $HS_PSM_BASE.${2}.interface.1.$GRE_PSM_BRIDGES BRIDGE_INST_2 $HS_PSM_BASE.${2}.interface.2.$GRE_PSM_BRIDGES BRIDGE_INST_3 $HS_PSM_BASE.${2}.interface.3.$GRE_PSM_BRIDGES BRIDGE_INST_4 $HS_PSM_BASE.${2}.interface.4.$GRE_PSM_BRIDGES ENABLED $HS_PSM_BASE.${2}.$HS_PSM_ENABLE GRE_ENABLED $GRE_PSM_BASE.${2}.$GRE_PSM_ENABLE name $GRE_PSM_BASE.$2.$GRE_PSM_NAME`
        bridgeFQDM="$BRIDGE_INST_1,$BRIDGE_INST_2,$BRIDGE_INST_3,$BRIDGE_INST_4"
		
		if [ x != x$curr_tunnel ]; then
            destroy_tunnel $name
        fi
        
        if [ x"1" != x$ENABLED -o x"1" != x$GRE_ENABLED ]; then
            #Disabled
            if [ xstarted = x$hotspot_started ]; then
                hotspot_down $2
            fi
        else
            #Enabled
            if [ xstarted = x$hotspot_started ]; then
                if [ x != x$curr_tunnel ]; then
                    create_tunnel $curr_tunnel $name
                    if [ "$BOX_TYPE" = "XB6" ] || [ "$BOX_TYPE" = "TCCBR" ] ; then                                    
                        read_greInst                                                                    
                        check_ssids 1                                                     
                    fi   
                fi
                #if forceRestart is given, an apply settings is desired. Only set if ssids are already up.
                if [ gre-forceRestart = $1 -a x`sysevent get hotspot_ssids_up` = xtrue ]; then
                    set_ssids_enabled $2 true
                fi
            else
                hotspot_up $2
            fi
        fi
#         name=`psmcli get $GRE_PSM_BASE.$2.$GRE_PSM_NAME`
#         destroy_tunnel $name
#         create_tunnel $curr_tunnel $name
    ;;
    
    #args: hotspot gre instance
    hotspot-update_bridges)
		eval `psmcli get -e BRIDGE_INST_1 $HS_PSM_BASE.${2}.interface.1.$GRE_PSM_BRIDGES BRIDGE_INST_2 $HS_PSM_BASE.${2}.interface.2.$GRE_PSM_BRIDGES BRIDGE_INST_3 $HS_PSM_BASE.${2}.interface.3.$GRE_PSM_BRIDGES BRIDGE_INST_4 $HS_PSM_BASE.${2}.interface.4.$GRE_PSM_BRIDGES BRIDGE_INST_5 $HS_PSM_BASE.${2}.interface.5.$GRE_PSM_BRIDGES WECB_BRIDGES dmsb.wecb.hhs_extra_bridges NAME $GRE_PSM_BASE.$2.$GRE_PSM_NAME`
        BRIDGE_INSTS="$BRIDGE_INST_1,$BRIDGE_INST_2,$BRIDGE_INST_3,$BRIDGE_INST_4,$BRIDGE_INST_5"
        start=""
        brinst=""
        OLD_IFS="$IFS"
        IFS="${AB_DELIM}${AB_SSID_DELIM}"

        wait_for_erouter0_ready
        for i in $BRIDGE_INSTS; do
            brinst=`echo $i |cut -d . -f 4`
            status=`sysevent get multinet_$brinst-status`
            if [ x = x$status -o x$STOPPED_STATUS = x$status ]; then
                sysevent set multinet-start $brinst
                start=1
            fi
        done
        
        for i in $WECB_BRIDGES; do
            if [ x = x"$i" ]; then
                continue
            fi
            status=`sysevent get multinet_$i-status`
            if [ x = x$status -o x$STOPPED_STATUS = x$status ]; then
                sysevent set multinet-start $i
                start=1
            fi
        done
        IFS="$OLD_IFS"
        
        bInst_to_bNames "$BRIDGE_INSTS" "$WECB_BRIDGES"
        update_bridge_config $NAME
        curr_tunnel=`sysevent get gre_current_endpoint`
        if [ x != x$curr_tunnel ]; then
            update_bridge_frag_config $2 $curr_tunnel
        fi
        
        if [ x = x$start ]; then
          echo "handle_gre : Triggering RDKB_FIREWALL_RESTART in update bridges"
	  t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
            sysevent set firewall-restart
        fi
    ;;
    
#     snooper-wifi-clients) 
#         if [ x$2 = x0 ]; then
#             sysevent set hotspotfd-enable 0
#         else
#             sysevent set hotspotfd-enable 1
#         fi
#     ;;
    *)
        exit 3
        ;;
esac


# TODO 
# modify hotspot params
# modify hotspot bridge structure
# generic gre if impl
# remove name hard code
