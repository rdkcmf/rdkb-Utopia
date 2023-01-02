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
# This script is used to manage the ipv4 interface instances
#------------------------------------------------------------------

source /lib/rdk/t2Shared_api.sh

#dynamic data structures
#
#

# The intended external apis include:
# l3net-up/down {l3 id}
# l3net-resync_all_instances    for instance addition and deletion
# l3net-resync {l3_id}
# l2net_{l2 id}-status
# 
SERVICE_NAME="ipv4"
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/ut_plat.sh
source /etc/utopia/service.d/log_capture_path.sh

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi


STATIC_IPV4SUBNET=""
STATIC_IPV4ADDR=""
EMPTY_IPADDR="0.0.0.0"

service_start() {
echo

}

service_stop () {
 echo
}

service_init() {
   SYSCFG_FAILED='false'
   FOO=`utctx_cmd get last_erouter_mode`
   eval $FOO
   if [ $SYSCFG_FAILED = 'true' ] ; then
      ulog wan status "$PID utctx failed to get some configuration data"
      ulog wan status "$PID WAN CANNOT BE CONTROLLED"
      exit
   fi
}

#args: l3 instance, l2 instance, l2 status, [flag:bringup lower?]
handle_l2_status () {

	echo_t "service_ipv4 PARTIAL_STATUS:${PARTIAL_STATUS} READY_STATUS:${READY_STATUS}"

    BOX_TYPE=`cat /etc/device.properties | grep BOX_TYPE | cut -f2 -d=`
    echo_t "Box Type is $BOX_TYPE"

    if [ x${PARTIAL_STATUS} = x${3} -o x${READY_STATUS} = x${3} ] && [ x1 = x`sysevent get ${L2SERVICE_NAME}_$2-${L2_LOCAL_READY_PARAM}` ]; then
    #l2 up
        OUR_STATUS=`sysevent get ${SERVICE_NAME}_$1-status`
        if [ x$OUR_STATUS = x"$L3_UP_STATUS" -o x$OUR_STATUS = x"$L3_AWAITING_STATUS" ]; then
            #we're already prepared so nothing needs to be done
            return
        fi
    
        IFNAME=`sysevent get ${L2SERVICE_NAME}_$2-${L2SERVICE_IFNAME}`
        sysevent set ${SERVICE_NAME}_$1-ifname $IFNAME

		echo_t "service_ipv4 IFNAME:$IFNAME STATIC_IPV4ADDR:$STATIC_IPV4ADDR STATIC_IPV4SUBNET:$STATIC_IPV4SUBNET"
		
        load_static_l3 $1
        if [ x != x${STATIC_IPV4ADDR} -a x != x${STATIC_IPV4SUBNET} ]; then
            #apply static config if exists
            CUR_IPV4_ADDR=$STATIC_IPV4ADDR
            CUR_IPV4_SUBNET=$STATIC_IPV4SUBNET
            sysevent set ${SERVICE_NAME}_${1}-ipv4_static 1
			echo_t "service_ipv4 : Triggering apply_config" 
            apply_config $1
            if [ 0 = $? ]; then
                sysevent set ${SERVICE_NAME}_${1}-status $L3_UP_STATUS
		echo_t "service_ipv4 : Triggering RDKB_FIREWALL_RESTART"
		t2CountNotify "RF_INFO_RDKB_FIREWALL_RESTART"
				sysevent set firewall-restart
                if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ]; then
				uptime=$(cut -d. -f1 /proc/uptime)
				if [ -e "/usr/bin/onboarding_log" ]; then
				    /usr/bin/onboarding_log "RDKB_FIREWALL_RESTART:$uptime"
				fi

				if [ x4 = x$1 ]; then
                    echo "IPv4 address is set for brlan0 MOCA interface is UP"
			
                    if [ ! -f "/tmp/moca_start" ]; then
                        print_uptime "boot_to_MOCA_uptime"
                        touch /tmp/moca_start
                    fi
                fi
               fi
            fi
            if [ "$BOX_TYPE" = "rpi" ]; then
                 LAN_STATUS=`sysevent get lan-status`
                 if [ "$LAN_STATUS" = "stopped" ]; then
                      echo_t "service_ipv4 : Starting lan-status"
                      sysevent set lan-status started
                 fi
            fi
        else
            #Otherwise, announce readiness for provisioning
            # TODO: consider applying dynamic config currently in sysevent
            sysevent set ${SERVICE_NAME}_${1}-ipv4_static 0
            sysevent set ${SERVICE_NAME}_${1}-status $L3_AWAITING_STATUS
        fi
        
    else
    #l2 down
        sysevent set ${SERVICE_NAME}_${1}-status pending
        if [ x = x$3 -o x$STOPPED_STATUS = x$3 ] && [ x != x$4 ] ; then
        #    BOX_TYPE=`cat /etc/device.properties | grep BOX_TYPE | cut -f2 -d=`
            BRIDGE_MODE=`sysevent get bridge_mode`
            if [ "$BOX_TYPE" = "XB3" ] && [ "$BRIDGE_MODE" = "0" ]; then
                echo "In Router mode:brlan0 initialization is done in PSM & brlan1 initialization is done in cosa_start_rem.sh"
            else
                sysevent set ${L2SERVICE_NAME}-up $2
            fi  
        fi        
    fi 
}

#args: l3 instance
apply_config () {
    if [ x = x${IFNAME} ]; then
        IFNAME=`sysevent get ${SERVICE_NAME}_$1-ifname`
    fi
    
    if [ x = x${CUR_IPV4_ADDR} ]; then
        CUR_IPV4_ADDR=`sysevent get ${SERVICE_NAME}_${1}-ipv4addr`
    fi
    
    if [ x = x${CUR_IPV4_SUBNET} ]; then
        CUR_IPV4_SUBNET=`sysevent get ${SERVICE_NAME}_${1}-ipv4subnet`
    fi
    
    if [ x = x${CUR_IPV4_ADDR} -o x = x${CUR_IPV4_SUBNET} ]; then 
        sysevent set ${SERVICE_NAME}_${1}-status error
        sysevent set ${SERVICE_NAME}_${1}-error "Missing IP or subnet"
        return 1
    fi
    
    echo 2 > /proc/sys/net/ipv4/conf/$IFNAME/arp_ignore
    
    #WAN_IFNAME=`sysevent get current_wan_ifname`
    
    RT_TABLE=`expr $1 + 10`
    
    MASKBITS=`mask2cidr $CUR_IPV4_SUBNET`
    # dslite_enabled will be set when DSLITE_FEATURE_SUPPORT compilation flag is set
    DSLITE_ENABLED=`sysevent get dslite_enabled`

    ISIPV4NEEDED="TRUE"
    if [ "$rdkb_extender" = "true" ];then

      	DEVICE_MODE=`syscfg get Device_Mode`
        XHS_L3INST=5
        PRIVATE_LAN_L3INST=4

        #dont assign ipv4 for private lan and xhs interface if device is in extender mode.
        if [ "1" = "$DEVICE_MODE" ]; then
            if [ "$1" = "$PRIVATE_LAN_L3INST" ] || [ "$1" = "$XHS_L3INST" ]; then
                ISIPV4NEEDED="FALSE"
            fi
        fi
    fi
    if [ "TRUE" = "$ISIPV4NEEDED" ]; then
    #If dslite is disabled and the lan bridge is brlan0, then do not assign IPv4 on brlan0 during ipv6 only mode.
    #For other lan bridges assign IPv4 always.
    if [ xbrlan0 = x${IFNAME} -a x1 != x$DSLITE_ENABLED ]; then
	if [ "1" == "$SYSCFG_last_erouter_mode" ] || [ "3" == "$SYSCFG_last_erouter_mode" ]; then
	    ip addr add $CUR_IPV4_ADDR/$MASKBITS broadcast + dev $IFNAME
        fi
    else
        ip addr add $CUR_IPV4_ADDR/$MASKBITS broadcast + dev $IFNAME
    fi
    fi

    
    # TODO: Fix this static workaround. Should have configurable routing policy.
    SUBNET=`subnet $CUR_IPV4_ADDR $CUR_IPV4_SUBNET`
    #ip route del $SUBNET/$MASKBITS dev $IFNAME

    if [ "$BOX_TYPE" = "XB6" ] && [ "$MANUFACTURE" = "Arris" ]; then
    #Update interface MTU if there was a valid MTU value in PSM
        case $NV_MTU in
            ''|*[!0-9]*)
                #Invalid / non-numeric MTU
            ;;
            *)
                if [ $NV_MTU -gt 0 ] ; then
                    #If you try to set an MTU that is the same as current Linux MTU
                    #Linux ignores it, so set the MTU twice to make sure it applies
                    TMP_MTU=`expr $NV_MTU - 1`
                    ip link set dev $IFNAME mtu $TMP_MTU
                    ip link set dev $IFNAME mtu $NV_MTU
                fi
            ;;
        esac
    fi
    
    ip rule add from $CUR_IPV4_ADDR lookup $RT_TABLE
    ip rule add iif $IFNAME lookup erouter
    ip rule add iif $IFNAME lookup $RT_TABLE
    #ip rule add iif $WAN_IFNAME lookup $RT_TABLE
    ip route add table $RT_TABLE $SUBNET/$MASKBITS dev $IFNAME
    ip route add table all_lans $SUBNET/$MASKBITS dev $IFNAME

    # bind 161/162 port to brlan0 interface
    if [ xbrlan0 = x${IFNAME} ]; then
        #sysevent set ipv4_address $CUR_IPV4_ADDR
        #sysevent set snmppa_socket_entry add
	
	#XB6 Kernel chooses erouter0 as default. Need brlan0 route for XHS (ARRISXB6-5086)
        if [ "$BOX_TYPE" = "XB6" ]; then
		ip route add 239.255.255.250/32 dev brlan0
	fi
    fi
    
    #assign lan interface a global ipv6 address
    #I can't find other way to do this. Just put here temporarily.
    if [ xbrlan0 = x${IFNAME} ]; then
        SYSEVT_lan_ipaddr_v6_prev=`sysevent get lan_ipaddr_v6_prev`
        SYSEVT_lan_ipaddr_v6=`sysevent get lan_ipaddr_v6`
        SYSEVT_lan_prefix_v6=`sysevent get lan_prefix_v6`
        LAN_IFNAME=$IFNAME


        if [ "$SYSEVT_lan_ipaddr_v6_prev" != "$SYSEVT_lan_ipaddr_v6" ]; then
            if [ -n "$SYSEVT_lan_ipaddr_v6_prev" ]; then
                ip -6 addr del $SYSEVT_lan_ipaddr_v6_prev/64 dev $LAN_IFNAME valid_lft forever preferred_lft forever
            fi

            ip -6 addr add $SYSEVT_lan_ipaddr_v6/64 dev $LAN_IFNAME valid_lft forever preferred_lft forever
        fi
    fi

    if [ xbrlan0 = x${IFNAME} ]; then
        #echo " !!!!!!!!!! Syncing True Static IP !!!!!!!!!!"
        sync_tsip
        sync_tsip_asn
        sysevent set wan_staticip-status started
    fi

    # END ROUTING TODO
    
    sysevent set ${SERVICE_NAME}_${1}-ipv4addr $CUR_IPV4_ADDR
    sysevent set ${SERVICE_NAME}_${1}-ipv4subnet $CUR_IPV4_SUBNET       
    if [ xbrlan0 == x${IFNAME} ]; then
	UPNP_STATUS=`syscfg get start_upnp_service`     
        if [ "true" = "$UPNP_STATUS" ] ;
        then
            if [ -f /lib/rdk/start-upnp-service ] ;
            then
                /lib/rdk/start-upnp-service start
            else
                systemctl restart xcal-device &
                systemctl restart xupnp &
            fi
        fi
    fi
    return 0
}

remove_config () {
    if [ x = x${IFNAME} ]; then
        IFNAME=`sysevent get ${SERVICE_NAME}_$1-ifname`
    fi
    
    if [ x = x${CUR_IPV4_ADDR} ]; then
        CUR_IPV4_ADDR=`sysevent get ${SERVICE_NAME}_${1}-ipv4addr`
    fi
    
    if [ x = x${CUR_IPV4_SUBNET} ]; then
        CUR_IPV4_SUBNET=`sysevent get ${SERVICE_NAME}_${1}-ipv4subnet`
    fi
    
    #WAN_IFNAME=`sysevent get current_wan_ifname`
    
    RT_TABLE=`expr $1 + 10`
    
    MASKBITS=`mask2cidr $CUR_IPV4_SUBNET`
    ip addr del $CUR_IPV4_ADDR/$MASKBITS dev $IFNAME
    # TODO: Fix this static workaround. Should have configurable routing policy.
    SUBNET=`subnet $CUR_IPV4_ADDR $CUR_IPV4_SUBNET`
    ip rule del from $CUR_IPV4_ADDR lookup $RT_TABLE
    ip rule del iif $IFNAME lookup erouter
    ip rule del iif $IFNAME lookup $RT_TABLE
    #ip rule del iif $WAN_IFNAME lookup $RT_TABLE
    ip route del table $RT_TABLE $SUBNET/$MASKBITS dev $IFNAME
    ip route del table all_lans $SUBNET/$MASKBITS dev $IFNAME

    # del 161/162 port from brlan0 interface when it is teardown
    if [ xbrlan0 = x${IFNAME} ]; then
		#sysevent set snmppa_socket_entry delete
	#XB6 Kernel chooses erouter0 as default. Need brlan0 route for XHS (ARRISXB6-5086)
        if [ "$BOX_TYPE" = "XB6" ]; then
		ip route delete 239.255.255.250/32 dev brlan0
	fi
   
	UPNP_STATUS=`syscfg get start_upnp_service`     
        if [ "true" = "$UPNP_STATUS" ] ;
        then
            if [ -f /lib/rdk/start-upnp-service ] ;
            then
                /lib/rdk/start-upnp-service stop
                killall xcal-device
                killall xdiscovery
            else
                systemctl stop xcal-device &
                systemctl stop xupnp &
            fi
            ifconfig brlan0:0 down
        fi
    fi

    # END ROUTING TODO
    sysevent set ${SERVICE_NAME}_${1}-ipv4addr 
    sysevent set ${SERVICE_NAME}_${1}-ipv4subnet 
    sysevent set ${SERVICE_NAME}_${1}-ipv4_static
    sysevent set ${SERVICE_NAME}_${1}-status down
    
}

#args: 
load_static_l3 () {
    if [ "$BOX_TYPE" = "XB6" ] && [ "$MANUFACTURE" = "Arris" ]; then
        eval `psmcli get -e STATIC_IPV4ADDR ${IPV4_NV_PREFIX}.$1.${IPV4_NV_IP} STATIC_IPV4SUBNET ${IPV4_NV_PREFIX}.$1.${IPV4_NV_SUBNET} NV_MTU ${IPV4_NV_PREFIX}.$1.${IPV4_NV_MTU}`
    else
        eval `psmcli get -e STATIC_IPV4ADDR ${IPV4_NV_PREFIX}.$1.${IPV4_NV_IP} STATIC_IPV4SUBNET ${IPV4_NV_PREFIX}.$1.${IPV4_NV_SUBNET}`
    fi
}

#args: l3 instance
#envin: LOWER
teardown_instance () {
    if [ x != x$LOWER ] || [ x != x`sysevent get ${SERVICE_NAME}_${1}-lower` ]; then
        async="`sysevent get ${SERVICE_NAME}_${1}-l2async`"
        sysevent rm_async $async
        remove_config $1
        sysevent set ${SERVICE_NAME}_${1}-l2async
        sysevent set ${SERVICE_NAME}_${1}-lower
        
        ACTIVE_INST="`sysevent get ${SERVICE_NAME}-instances`"
        ACTIVE_INST="`echo $ACTIVE_INST | sed 's/ *\<'$1'\> */ /'`"
        sysevent set ${SERVICE_NAME}-instances "$ACTIVE_INST"
    fi
}



resync_all_instances () {
    ACTIVE_INST="`sysevent get ${SERVICE_NAME}-instances`"
    NV_INST="`psmcli getallinst ${IPV4_NV_PREFIX}.`"
    
    TO_REM=""
    UNCHANGED=""
    TO_ADD="$NV_INST"
    
    for cur_nv_inst in $ACTIVE_INST; do
        expr match "$TO_ADD" '.*\b\('$cur_nv_inst'\)\b.*' > /dev/null
        if [ 0 = $? ]; then
            #Keeping this active instance
            TO_ADD="`echo $TO_ADD | sed 's/ *\<'$cur_nv_inst'\> */ /'`"
            UNCHANGED="${UNCHANGED} $cur_nv_inst"
        else
            #Remove this instance
            TO_REM="${TO_REM} $cur_nv_inst"
        fi
    done
    
    #DEBUG
    echo_t "L3 Resync all instances. TO_REM:$TO_REM , TO_ADD:$TO_ADD"
    
    for inst in $TO_REM; do
        teardown_instance $inst
        sysevent set ${SERVICE_NAME}_${inst}-status
    done
    
    for inst in $TO_ADD; do
        resync_instance $inst
    done
    
    #sysevent set ${SERVICE_NAME}-instances "${UNCHANGED}${TO_ADD}"
    
}

#This function should only be called when an instance needs to be brought 
#args: l3 instance
resync_instance () {

    echo_t "RDKB_SYSTEM_BOOT_UP_LOG : In resync_instance to bring up an instance."
    if [ "$BOX_TYPE" = "XB6" ] && [ "$MANUFACTURE" = "Arris" ]
    then
        eval `psmcli get -e NV_ETHLOWER ${IPV4_NV_PREFIX}.${1}.EthLink NV_IP ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_IP} NV_SUBNET ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_SUBNET} NV_ENABLED ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_ENABLED} NV_MTU ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_MTU}`
    else
        eval `psmcli get -e NV_ETHLOWER ${IPV4_NV_PREFIX}.${1}.EthLink NV_IP ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_IP} NV_SUBNET ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_SUBNET} NV_ENABLED ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_ENABLED}`
    fi

    if [ -z "$NV_ETHLOWER" ]
    then
	echo_t "RDKB_SYSTEM_BOOT_UP_LOG : NV_ETHLOWER returned null, retrying"
	NV_ETHLOWER=`psmcli get ${IPV4_NV_PREFIX}.${1}.EthLink`
    fi
    if [ -z "$NV_IP" ]
    then
	echo_t "RDKB_SYSTEM_BOOT_UP_LOG : NV_IP returned null, retrying"
	NV_IP=`psmcli get ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_IP}`
    fi
     if [ -z "$NV_SUBNET" ]
    then
        echo_t "RDKB_SYSTEM_BOOT_UP_LOG : NV_SUBNET returned null, retrying"
        NV_SUBNET=`psmcli get ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_SUBNET}`
    fi
    if [ -z "$NV_ENABLED" ]
    then
        echo_t "RDKB_SYSTEM_BOOT_UP_LOG : NV_ENABLED returned null, retrying"
        NV_ENABLED=`psmcli get ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_ENABLED}`
    fi
    
    if [ "$BOX_TYPE" = "XB6" ] && [ "$MANUFACTURE" = "Arris" ]
    then
         if [ -z "$NV_MTU" ]
         then
             echo_t "RDKB_SYSTEM_BOOT_UP_LOG : NV_MTU returned null, retrying"
             NV_ENABLED=`psmcli get ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_MTU}`
        fi
    fi

    if [ x = x$NV_ENABLED -o x$DM_FALSE = x$NV_ENABLED ]; then
        teardown_instance $1
        return
    fi
    
    #Find l2net instance from EthLink instance.
    NV_LOWER=`psmcli get ${ETH_DM_PREFIX}.${NV_ETHLOWER}.l2net`

    if [ -z "$NV_LOWER" ]
    then
	echo_t "RDKB_SYSTEM_BOOT_UP_LOG : NV_LOWER returned null, retrying"
	NV_LOWER=`psmcli get ${ETH_DM_PREFIX}.${NV_ETHLOWER}.l2net`
    fi
    LOWER=`sysevent get ${SERVICE_NAME}_${1}-lower`
    #IP=`sysevent get ${SERVICE_NAME}_${1}-ipv4addr`
    #SUBNET=`sysevent get ${SERVICE_NAME}_${1}-ipv4subnet`
    CUR_IPV4_ADDR=`sysevent get ${SERVICE_NAME}_${1}-ipv4addr`
    CUR_IPV4_SUBNET=`sysevent get ${SERVICE_NAME}_${1}-ipv4subnet`
    
    #DEBUG
    echo_t "RDKB_SYSTEM_BOOT_UP_LOG : Syncing l3 instance ($1), NV_ETHLOWER:$NV_ETHLOWER, NV_LOWER:$NV_LOWER , NV_ENABLED:$NV_ENABLED , NV_IP:$NV_IP , NV_SUBNET:$NV_SUBNET , LOWER:$LOWER , CUR_IPV4_ADDR:$CUR_IPV4_ADDR , CUR_IPV4_SUBNET:$CUR_IPV4_SUBNET"
    
    if [ x$NV_LOWER != x$LOWER ]; then
        #different lower layer, teardown and switch
        if [ x != x$LOWER ]; then
            teardown_instance $1
        fi
        sysevent set ${SERVICE_NAME}_${1}-lower "$NV_LOWER"
        if [ x != x${NV_LOWER} ]; then 
            ACTIVE_INST="`sysevent get ${SERVICE_NAME}-instances`"
            ACTIVE_INST="`echo $ACTIVE_INST | sed 's/ *\<'$1'\> */ /'`"
            sysevent set ${SERVICE_NAME}-instances "$ACTIVE_INST $1"
            
            async="`sysevent async ${L2SERVICE_NAME}_$NV_LOWER-status $THIS $1`"
            sysevent set ${SERVICE_NAME}_${1}-l2async "$async"
            handle_l2_status $1 $NV_LOWER "`sysevent get ${L2SERVICE_NAME}_$NV_LOWER-status`" 1
        fi
    else 
        if [ x != x$CUR_IPV4_ADDR ] && [ x1 = x`sysevent get ${SERVICE_NAME}_${1}-ipv4_static` -o x != x${NV_IP} ]; then
            if [ x$CUR_IPV4_ADDR != x$NV_IP -o x$CUR_IPV4_SUBNET != x$NV_SUBNET ]; then
                #Same lower layer, but static IP info changed.
                remove_config $1
                handle_l2_status $1 $NV_LOWER "`sysevent get ${L2SERVICE_NAME}_$NV_LOWER-status`"
            fi
        fi
    fi
}

sync_tsip () {

    eval `psmcli get -e NV_TSIP_ENABLE ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_ENABLE} NV_TSIP_IP ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_IP} NV_TSIP_SUBNET ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_SUBNET} NV_TSIP_GATEWAY ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_GATEWAY}`
    echo_t "Syncing from PSM True Static IP Enable:$NV_TSIP_ENABLE, IP:$NV_TSIP_IP, SUBNET:$NV_TSIP_SUBNET, GATEWAY:$NV_TSIP_GATEWAY"

    #apply the new original true static ip
    if [ x != x$NV_TSIP_ENABLE -a x0 != x$NV_TSIP_ENABLE ]; then
        MASKBITS=`mask2cidr $NV_TSIP_SUBNET`
        SUBNET=`subnet $NV_TSIP_IP $NV_TSIP_SUBNET`
        ip addr add $NV_TSIP_IP/$MASKBITS broadcast + dev brlan0
        ip route add table 14 $SUBNET/$MASKBITS dev brlan0
        ip route add table all_lans $SUBNET/$MASKBITS dev brlan0

        #if [ x != x$NV_TSIP_GATEWAY -a x$EMPTY_IPADDR != x$NV_TSIP_GATEWAY ]; then
            #Override the original gateway
            #WAN_IFNAME=`sysevent get wan_ifname`
            #echo "!!!!!!!!!! ip -4 route add table erouter default dev $WAN_IFNAME via $NV_TSIP_GATEWAY"
            #ip -4 route del table erouter default
            #ip -4 route add table erouter default dev $WAN_IFNAME via $NV_TSIP_GATEWAY
        #fi
    fi
}

sync_tsip_asn () {

    NV_INST="`psmcli getallinst ${IPV4_TSIP_ASNPREFIX}.`"

    for cur_nv_inst in $NV_INST; do
        eval `psmcli get -e NV_TSIP_ASN_ENABLE ${IPV4_TSIP_ASNPREFIX}.${cur_nv_inst}.${IPV4_TSIP_ENABLE} NV_TSIP_ASN_IP ${IPV4_TSIP_ASNPREFIX}.${cur_nv_inst}.${IPV4_TSIP_IP} NV_TSIP_ASN_SUBNET ${IPV4_TSIP_ASNPREFIX}.${cur_nv_inst}.${IPV4_TSIP_SUBNET}`
        echo_t "Syncing from PSM asn Enable:${NV_TSIP_ASN_ENABLE}, IP:${NV_TSIP_ASN_IP}, SUBNET:${NV_TSIP_ASN_SUBNET}"

        if [ x != x$NV_TSIP_ASN_ENABLE -a x0 != x$NV_TSIP_ASN_ENABLE ]; then
            MASKBITS=`mask2cidr $NV_TSIP_ASN_SUBNET`
            SUBNET=`subnet $NV_TSIP_ASN_IP $NV_TSIP_ASN_SUBNET`
            ip addr add $NV_TSIP_ASN_IP/$MASKBITS broadcast + dev brlan0
            ip route add table 14 $SUBNET/$MASKBITS dev brlan0
            ip route add table all_lans $SUBNET/$MASKBITS dev brlan0
        fi
    done
}

# resync true static ip
resync_tsip () {

    eval `psmcli get -e NV_TSIP_ENABLE ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_ENABLE} NV_TSIP_IP ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_IP} NV_TSIP_SUBNET ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_SUBNET} NV_TSIP_GATEWAY ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_GATEWAY}`
    echo_t "From PSM True Static IP Enable:${NV_TSIP_ENABLE}, IP:${NV_TSIP_IP}, SUBNET:${NV_TSIP_SUBNET}, GATEWAY:${NV_TSIP_GATEWAY}"

    IPV4_ADDR=`sysevent get ipv4-tsip_IPAddress`
    IPV4_SUBNET=`sysevent get ipv4-tsip_Subnet`
    IPV4_GATEWAY=`sysevent get ipv4-tsip_Gateway`
    echo_t "From Command line True Static IP Enable:$1, IP:${IPV4_ADDR}, SUBNET:${IPV4_SUBNET}, GATEWAY:${IPV4_GATEWAY}"
    if [ "$1" = "1" ] ; then
	t2CountNotify "SYS_INFO_StaticIP_setMso" 
    fi
       
    #delete the original true static ip first
    if [ x != x$NV_TSIP_IP -a x != x$NV_TSIP_SUBNET ]; then
        MASKBITS=`mask2cidr $NV_TSIP_SUBNET`
        SUBNET=`subnet $NV_TSIP_IP $NV_TSIP_SUBNET`
        ip addr del $NV_TSIP_IP/$MASKBITS broadcast + dev brlan0
        ip route del table 14 $SUBNET/$MASKBITS dev brlan0
        ip route del table all_lans $SUBNET/$MASKBITS dev brlan0

        #if [ x != x$NV_TSIP_GATEWAY -a x$EMPTY_IPADDR != x$NV_TSIP_GATEWAY ]; then
            #ip rule del...
            #WAN_IFNAME=`sysevent get wan_ifname`
            #ip -4 route del table erouter default
            #ip -4 route del table erouter default dev $WAN_IFNAME via $NV_TSIP_GATEWAY
        #fi
    fi

    #apply the new original true static ip
    if [ x != x$1 -a x0 != x$1 ]; then
        MASKBITS=`mask2cidr ${IPV4_SUBNET}`
        SUBNET=`subnet $IPV4_ADDR $IPV4_SUBNET`
        ip addr add $IPV4_ADDR/$MASKBITS broadcast + dev brlan0
        ip route add table 14 $SUBNET/$MASKBITS dev brlan0
        ip route add table all_lans $SUBNET/$MASKBITS dev brlan0

        #if [ x != x${IPV4_GATEWAY} -a x$EMPTY_IPADDR != x${IPV4_GATEWAY} ]; then
            #ip rule add...
            #WAN_IFNAME=`sysevent get wan_ifname`
            #echo "!!!!!!!!!! ip -4 route add table erouter default dev $WAN_IFNAME via $NV_TSIP_GATEWAY"
            #ip -4 route add table erouter default dev $WAN_IFNAME via $NV_TSIP_GATEWAY
        #fi
    #else
        # Restore defaul gateway of $WAN_IFNAME
        #OLD_DEFAULT_ROUTER=`sysevent get default_router`
        #echo "!!!!!!!!!! Restore default gateway $OLD_DEFAULT_ROUTER"
        #ip -4 route add table erouter default dev $WAN_IFNAME via $OLD_DEFAULT_ROUTER
    fi
}

resync_tsip_asn () {

    eval `psmcli get -e NV_TSIP_ASN_ENABLE ${IPV4_TSIP_ASNPREFIX}.${1}.${IPV4_TSIP_ENABLE} NV_TSIP_ASN_IP ${IPV4_TSIP_ASNPREFIX}.${1}.${IPV4_TSIP_IP} NV_TSIP_ASN_SUBNET ${IPV4_TSIP_ASNPREFIX}.${1}.${IPV4_TSIP_SUBNET}`
    echo_t "From PSM asn Enable:${NV_TSIP_ASN_ENABLE}, IP:${NV_TSIP_ASN_IP}, SUBNET:${NV_TSIP_ASN_SUBNET}"

    IPV4_ENABLE=`sysevent get ipv4-tsip_asn_enable`
    IPV4_ADDR=`sysevent get ipv4-tsip_asn_ipaddress`
    IPV4_SUBNET=`sysevent get ipv4-tsip_asn_subnet`
    echo_t "From CL  asn Enable:${IPV4_ENABLE}, IP:${IPV4_ADDR}, SUBNET:${IPV4_SUBNET}"

    #delete the original additional subnet first
    if [ x != x$NV_TSIP_ASN_ENABLE -a x0 != x$NV_TSIP_ASN_ENABLE ]; then
        MASKBITS=`mask2cidr $NV_TSIP_ASN_SUBNET`
        SUBNET=`subnet $NV_TSIP_ASN_IP $NV_TSIP_ASN_SUBNET`
        ip addr del $NV_TSIP_ASN_IP/$MASKBITS broadcast + dev brlan0
        ip route del table 14 $SUBNET/$MASKBITS dev brlan0
        ip route del table all_lans $SUBNET/$MASKBITS dev brlan0
    fi

    #apply the new additional subnet
    if [ x != x${IPV4_ENABLE} -a x0 != x${IPV4_ENABLE} ]; then
        MASKBITS=`mask2cidr ${IPV4_SUBNET}`
        SUBNET=`subnet $IPV4_ADDR $IPV4_SUBNET`
        ip addr add $IPV4_ADDR/$MASKBITS broadcast + dev brlan0
        ip route add table 14 $SUBNET/$MASKBITS dev brlan0
        ip route add table all_lans $SUBNET/$MASKBITS dev brlan0
    fi
}

remove_tsip_config () {

    eval `psmcli get -e NV_TSIP_ENABLE ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_ENABLE} NV_TSIP_IP ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_IP} NV_TSIP_SUBNET ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_SUBNET} NV_TSIP_GATEWAY ${IPV4_TSIP_PREFIX}.${IPV4_TSIP_GATEWAY}`
    echo_t "Delete True Static IP Enable:$NV_TSIP_ENABLE, IP:$NV_TSIP_IP, SUBNET:$NV_TSIP_SUBNET, GATEWAY:$NV_TSIP_GATEWAY"

    if [ x != x$NV_TSIP_IP -a x != x$NV_TSIP_SUBNET ]; then
        MASKBITS=`mask2cidr $NV_TSIP_SUBNET`
        SUBNET=`subnet $NV_TSIP_IP $NV_TSIP_SUBNET`
        ip addr del $NV_TSIP_IP/$MASKBITS broadcast + dev brlan0
        ip route del table 14 $SUBNET/$MASKBITS dev brlan0
        ip route del table all_lans $SUBNET/$MASKBITS dev brlan0

        #if [ x != x$NV_TSIP_GATEWAY -a x$EMPTY_IPADDR != x$NV_TSIP_GATEWAY ]; then
            # Restore defaul gateway of $WAN_IFNAME
            #ip -4 route del table erouter default
            #WAN_IFNAME=`sysevent get wan_ifname`          
            #OLD_DEFAULT_ROUTER=`sysevent get default_router`
            #echo "!!!!!!!!!! Restore default gateway $OLD_DEFAULT_ROUTER"
            #ip -4 route add table erouter default dev $WAN_IFNAME via $OLD_DEFAULT_ROUTER
        #fi
    fi

    NV_INST="`psmcli getallinst ${IPV4_TSIP_ASNPREFIX}.`"

    for cur_nv_inst in $NV_INST; do
        eval `psmcli get -e NV_TSIP_ASN_ENABLE ${IPV4_TSIP_ASNPREFIX}.${cur_nv_inst}.${IPV4_TSIP_ENABLE} NV_TSIP_ASN_IP ${IPV4_TSIP_ASNPREFIX}.${cur_nv_inst}.${IPV4_TSIP_IP} NV_TSIP_ASN_SUBNET ${IPV4_TSIP_ASNPREFIX}.${cur_nv_inst}.${IPV4_TSIP_SUBNET}`
        echo_t "Delete asn Enable:${NV_TSIP_ASN_ENABLE}, IP:${NV_TSIP_ASN_IP}, SUBNET:${NV_TSIP_ASN_SUBNET}"

        if [ x0 != x${NV_TSIP_ASN_ENABLE} ]; then
            MASKBITS=`mask2cidr $NV_TSIP_ASN_SUBNET`
            SUBNET=`subnet $NV_TSIP_ASN_IP $NV_TSIP_ASN_SUBNET`
            ip addr del $NV_TSIP_ASN_IP/$MASKBITS broadcast + dev brlan0
            ip route del table 14 $SUBNET/$MASKBITS dev brlan0
            ip route del table all_lans $SUBNET/$MASKBITS dev brlan0
        fi
    done
}

#---------------------------------------------------------------

echo_t "RDKB_SYSTEM_BOOT_UP_LOG : service_ipv4 called with $1 $2 $3"

service_init

case "$1" in
  ${SERVICE_NAME}-start)
      service_start
      ;;
  ${SERVICE_NAME}-stop)
      service_stop
      ;;
  ${SERVICE_NAME}-restart)
      service_stop
      service_start
      ;;
      
   ${L2SERVICE_NAME}_*-status)
        #Event from l2 members
        #args: l2 status value, ipv4 instance
        INST=${1%-*}
        INST=${INST#*_}
        handle_l2_status $3 $INST $2 
   ;;
   
   #args: none
   ${SERVICE_NAME}-set_dyn_config)
        #INST=${1%-*}
        #INST=${INST#*_}
        apply_config $2
   ;;
   
   ${SERVICE_NAME}-resync)
        #INST=${1%-*}
        #INST=${INST#*_}
        resync_instance $2
   ;;
   
   ${SERVICE_NAME}-resyncAll)
        resync_all_instances
   ;;

   ${SERVICE_NAME}-sync_tsip_all)
        sync_tsip
        sync_tsip_asn
        sysevent set wan_staticip-status started
   ;;

   ${SERVICE_NAME}-stop_tsip_all)
        remove_tsip_config
   ;;

   ${SERVICE_NAME}-resync_tsip)
        resync_tsip $2
   ;;

   ${SERVICE_NAME}-resync_tsip_asn)
        resync_tsip_asn $2
   ;;
   
   ${SERVICE_NAME}-up)
        #INST=${1%-*}
        #INST=${INST#*_}
        LOWER=`sysevent get ${SERVICE_NAME}_${2}-lower`
        if [ x = x$LOWER ]; then
            resync_instance $2
        else
            handle_l2_status $2 $LOWER "`sysevent get ${L2SERVICE_NAME}_${LOWER}-status`" 1
        fi
   ;;
   
   ${SERVICE_NAME}-down)
        #INST=${1%-*}
        #INST=${INST#*_}
        teardown_instance $2
   ;;
   
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart]" >&2
      exit 3
      ;;
esac

