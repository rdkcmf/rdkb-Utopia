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
# This script is used to manage the ipv4 interface instances
#------------------------------------------------------------------


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



STATIC_IPV4SUBNET=""
STATIC_IPV4ADDR=""
EMPTY_IPADDR="0.0.0.0"


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

mask2cidr() {
	numberofbits=0
	echo "Mask2cidr called on :${1}:" > /dev/console
	fields=`echo $1 | sed 's/\./ /g'`
	for field in $fields ; do
		echo "dec:${field}:" > /dev/console
		if [ $field -eq 255 ]; then
			numberofbits=$((numberofbits + 8))
		elif [ $field -eq 254 ]; then
			numberofbits=$((numberofbits + 7))
		elif [ $field -eq 252 ]; then
			numberofbits=$((numberofbits + 6))
		elif [ $field -eq 248 ]; then
			numberofbits=$((numberofbits + 5))
		elif [ $field -eq 240 ]; then
			numberofbits=$((numberofbits + 4))
		elif [ $field -eq 224 ]; then
			numberofbits=$((numberofbits + 3))
		elif [ $field -eq 192 ]; then
			numberofbits=$((numberofbits + 2))
		elif [ $field -eq 128 ]; then
			numberofbits=$((numberofbits + 1))
		elif [ $field -eq 0 ]; then
			:
		else
			echo "Error: $field is not recognised"; exit 1
		fi
	done
	echo $numberofbits
}

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
    if [ x${PARTIAL_STATUS} = x${3} -o x${READY_STATUS} = x${3} ] && [ x1 = x`sysevent get ${L2SERVICE_NAME}_$2-${L2_LOCAL_READY_PARAM}` ]; then
    #l2 up
        OUR_STATUS=`sysevent get ${SERVICE_NAME}_$1-status`
        if [ x$OUR_STATUS = x"$L3_UP_STATUS" -o x$OUR_STATUS = x"$L3_AWAITING_STATUS" ]; then
            #we're already prepared so nothing needs to be done
            return
        fi
    
        IFNAME=`sysevent get ${L2SERVICE_NAME}_$2-${L2SERVICE_IFNAME}`
        sysevent set ${SERVICE_NAME}_$1-ifname $IFNAME
        
        load_static_l3 $1
        if [ x != x${STATIC_IPV4ADDR} -a x != x${STATIC_IPV4SUBNET} ]; then
            #apply static config if exists
            CUR_IPV4_ADDR=$STATIC_IPV4ADDR
            CUR_IPV4_SUBNET=$STATIC_IPV4SUBNET
            sysevent set ${SERVICE_NAME}_${1}-ipv4_static 1
            apply_config $1
            if [ 0 = $? ]; then
                sysevent set ${SERVICE_NAME}_${1}-status $L3_UP_STATUS
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
            sysevent set ${L2SERVICE_NAME}-up $2
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
    #If it's ipv6 only mode, doesn't config ipv4 address. For ipv6 other things, we don't take care.
    if [ xbrlan0 = x${IFNAME} ]; then
        if [ "1" == "$SYSCFG_last_erouter_mode" ] || [ "3" == "$SYSCFG_last_erouter_mode" ]; then
	    ip addr add $CUR_IPV4_ADDR/$MASKBITS broadcast + dev $IFNAME
        fi
    else
        ip addr add $CUR_IPV4_ADDR/$MASKBITS broadcast + dev $IFNAME
    fi

    
    # TODO: Fix this static workaround. Should have configurable routing policy.
    SUBNET=`subnet $CUR_IPV4_ADDR $CUR_IPV4_SUBNET`
    #ip route del $SUBNET/$MASKBITS dev $IFNAME
    
    ip rule add from $CUR_IPV4_ADDR lookup $RT_TABLE
    ip rule add iif $IFNAME lookup erouter
    ip rule add iif $IFNAME lookup $RT_TABLE
    #ip rule add iif $WAN_IFNAME lookup $RT_TABLE
    ip route add table $RT_TABLE $SUBNET/$MASKBITS dev $IFNAME
    ip route add table all_lans $SUBNET/$MASKBITS dev $IFNAME

    # bind 161/162 port to brlan0 interface
    if [ xbrlan0 = x${IFNAME} ]; then
        snmpcmd -s /var/tmp/cm_snmp_ctrl -t 1 -a $CUR_IPV4_ADDR -c SNMPA_ADD_SOCKET_ENTRY -i $IFNAME -p 161
        snmpcmd -s /var/tmp/cm_snmp_ctrl -t 1 -a $CUR_IPV4_ADDR -c SNMPA_ADD_SOCKET_ENTRY -i $IFNAME -p 162
    fi
    
    #I can't find other way to do this. Just put here temporarily.
    if [ xbrlan0 = x${IFNAME} ]; then


        #echo " !!!!!!!!!! Syncing True Static IP !!!!!!!!!!"
        sync_tsip
        sync_tsip_asn
        sysevent set wan_staticip-status started
    fi

    # END ROUTING TODO
    
    sysevent set ${SERVICE_NAME}_${1}-ipv4addr $CUR_IPV4_ADDR
    sysevent set ${SERVICE_NAME}_${1}-ipv4subnet $CUR_IPV4_SUBNET
   
    sysevent set firewall-restart
    
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
        snmpcmd -s /var/tmp/cm_snmp_ctrl -t 1 -a $CUR_IPV4_ADDR -c SNMPA_DELETE_SOCKET_ENTRY -i $IFNAME -p 161
        snmpcmd -s /var/tmp/cm_snmp_ctrl -t 1 -a $CUR_IPV4_ADDR -c SNMPA_DELETE_SOCKET_ENTRY -i $IFNAME -p 162
    fi

    # END ROUTING TODO
    sysevent set ${SERVICE_NAME}_${1}-ipv4addr 
    sysevent set ${SERVICE_NAME}_${1}-ipv4subnet 
    sysevent set ${SERVICE_NAME}_${1}-ipv4_static
    sysevent set ${SERVICE_NAME}_${1}-status down
    
}

#args: 
load_static_l3 () {
    eval `psmcli get -e STATIC_IPV4ADDR ${IPV4_NV_PREFIX}.$1.${IPV4_NV_IP} STATIC_IPV4SUBNET ${IPV4_NV_PREFIX}.$1.${IPV4_NV_SUBNET}`
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
    echo "L3 Resync all instances. TO_REM:$TO_REM , TO_ADD:$TO_ADD"
    
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

    echo "RDKB_SYSTEM_BOOT_UP_LOG : In resync_instance to bring up an instance."
    eval `psmcli get -e NV_ETHLOWER ${IPV4_NV_PREFIX}.${1}.EthLink NV_IP ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_IP} NV_SUBNET ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_SUBNET} NV_ENABLED ${IPV4_NV_PREFIX}.${1}.${IPV4_NV_ENABLED}`
    
    if [ x = x$NV_ENABLED -o x$DM_FALSE = x$NV_ENABLED ]; then
        teardown_instance $1
        return
    fi
    
    #Find l2net instance from EthLink instance.
    NV_LOWER=`psmcli get ${ETH_DM_PREFIX}.${NV_ETHLOWER}.l2net`

    LOWER=`sysevent get ${SERVICE_NAME}_${1}-lower`
    #IP=`sysevent get ${SERVICE_NAME}_${1}-ipv4addr`
    #SUBNET=`sysevent get ${SERVICE_NAME}_${1}-ipv4subnet`
    CUR_IPV4_ADDR=`sysevent get ${SERVICE_NAME}_${1}-ipv4addr`
    CUR_IPV4_SUBNET=`sysevent get ${SERVICE_NAME}_${1}-ipv4subnet`
    
    #DEBUG
    echo "RDKB_SYSTEM_BOOT_UP_LOG : Syncing l3 instance ($1), NV_ETHLOWER:$NV_ETHLOWER, NV_LOWER:$NV_LOWER , NV_ENABLED:$NV_ENABLED , NV_IP:$NV_IP , NV_SUBNET:$NV_SUBNET , LOWER:$LOWER , CUR_IPV4_ADDR:$CUR_IPV4_ADDR , CUR_IPV4_SUBNET:$CUR_IPV4_SUBNET"
    
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
    echo "Syncing from PSM True Static IP Enable:$NV_TSIP_ENABLE, IP:$NV_TSIP_IP, SUBNET:$NV_TSIP_SUBNET, GATEWAY:$NV_TSIP_GATEWAY"

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
        echo "Syncing from PSM asn Enable:${NV_TSIP_ASN_ENABLE}, IP:${NV_TSIP_ASN_IP}, SUBNET:${NV_TSIP_ASN_SUBNET}"

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
    echo "From PSM True Static IP Enable:${NV_TSIP_ENABLE}, IP:${NV_TSIP_IP}, SUBNET:${NV_TSIP_SUBNET}, GATEWAY:${NV_TSIP_GATEWAY}"

    IPV4_ADDR=`sysevent get ipv4-tsip_IPAddress`
    IPV4_SUBNET=`sysevent get ipv4-tsip_Subnet`
    IPV4_GATEWAY=`sysevent get ipv4-tsip_Gateway`
    echo "From Command line True Static IP Enable:$1, IP:${IPV4_ADDR}, SUBNET:${IPV4_SUBNET}, GATEWAY:${IPV4_GATEWAY}"

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
    echo "From PSM asn Enable:${NV_TSIP_ASN_ENABLE}, IP:${NV_TSIP_ASN_IP}, SUBNET:${NV_TSIP_ASN_SUBNET}"

    IPV4_ENABLE=`sysevent get ipv4-tsip_asn_enable`
    IPV4_ADDR=`sysevent get ipv4-tsip_asn_ipaddress`
    IPV4_SUBNET=`sysevent get ipv4-tsip_asn_subnet`
    echo "From CL  asn Enable:${IPV4_ENABLE}, IP:${IPV4_ADDR}, SUBNET:${IPV4_SUBNET}"

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
    echo "Delete True Static IP Enable:$NV_TSIP_ENABLE, IP:$NV_TSIP_IP, SUBNET:$NV_TSIP_SUBNET, GATEWAY:$NV_TSIP_GATEWAY"

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
        echo "Delete asn Enable:${NV_TSIP_ASN_ENABLE}, IP:${NV_TSIP_ASN_IP}, SUBNET:${NV_TSIP_ASN_SUBNET}"

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

