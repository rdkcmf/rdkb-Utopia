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
# This script initializes all layer 2 networks, and prepares them for layer 3 provisioning.
# Upon completion of bringing up the appropriate wan handler bringing up the wan,
# it is expected that the wan handler will set the wan-status up.
#
# All wan protocols must set the following sysevent tuples
#   current_wan_ifname
#   current_wan_ipaddr
#   current_wan_subnet
#   current_wan_state
#   wan-status
#
# Wan protocols are responsible for setting up:
#   /etc/resolv.conf
# Wan protocols are also responsible for restarting the dhcp server if
# the wan domain or dns servers change
#------------------------------------------------------------------
#--------------------------------------------------------------
#
# This script set the handlers for 2 events:
#    desired_ipv4_link_state
#    desired_ipv4_wan_state
# and then as appropriate sets these events.
# The desired_ipv4_link_state handler establishes ipv4 connectivity to
# the isp (currently dhcp or static).
# The desired_ipv4_wan_state handler uses the connectivity to the isp to
# set up a connection that is used as the ipv4 wan. This connection
# may be dhcp, static, pppoe, l2tp, pptp or in the future other protocols
#--------------------------------------------------------------



#CONSTRAINTS!!!!
# interface types and all names must be unique. i.e. you cannot have an interface named "gre"
# if there is a "gre" interface type

source /etc/utopia/service.d/log_capture_path.sh

source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh
#source /etc/utopia/service.d/service_wan/ppp_helpers.sh
source /lib/rdk/t2Shared_api.sh


#source /etc/utopia/service.d/psm_functions.sh

SERVICE_NAME="multinet"
source /etc/utopia/service.d/ut_plat.sh

THIS=/etc/utopia/service.d/service_multinet.sh



# TODO: move to plat script




#LOCAL VARS
PID="($$)"
export ALL_READY
export ALL_MEMBERS
export ALL_NEW_MEMBERS=""
export ALL_NEW_READY=""
export BRIDGE=""
export NAME=""

#####################################################################################

#args: netid, fw restart (bool)
start_net () {
    NETID=$1
    
    ALL_MEMBERS=""
    ALL_READY=""
    STATUS=`sysevent get ${SERVICE_NAME}_${NETID}-status`

    if [ x"$STATUS" != x"$STOPPED_STATUS" -a x"$STATUS" != x ]; then
        return
    fi

    load_net "$NETID"
    
    # TODO: CHECK FOR ENABLED
    
    if [ x"${DISABLED_STATUS}" = x"$CURNET_ENABLED" ]; then
        echo "Net $NETID started, but disabled. Exiting."
        return
    fi
    
    sysevent set ${SERVICE_NAME}_${NETID}-status starting
    sysevent set ${SERVICE_NAME}_${NETID}-vid ${CURNET_VID}
    
    for i in $INTERFACE_TYPES; do
        eval TOADD_${i}=CURNET_MEMBERS_${i}
    done

    config_new_members "$NETID"
    
    ALL_READY="${ALL_NEW_READY}"
    ALL_MEMBERS="${ALL_NEW_MEMBERS}"
    
    sysevent set ${SERVICE_NAME}_$NETID-allMembers "${ALL_MEMBERS}"
    sysevent set ${SERVICE_NAME}_$NETID-readyMembers "${ALL_READY}"
    
    ACTIVE_INST="`sysevent get ${SERVICE_NAME}-instances`"
    ACTIVE_INST="`echo $ACTIVE_INST | sed 's/ *\<'$1'\> */ /'`"
    sysevent set ${SERVICE_NAME}-instances "$ACTIVE_INST $1"
    
    #if [ x != x$LOCAL_READY -o x != x$BRIDGE ]; then
    #    ip link set $NAME up
    #fi
    
    update_net_status "$NETID"
    
    if [ x1 = x"$2" ]; then
        echo "MultinetService Triggering RDKB_FIREWALL_RESTART"
	t2CountNotify "RF_INFO_RDKB_FIREWALL_RESTART"
        sysevent set firewall-restart
    fi
    
}

# TODO verify
stop_net () {

    sysevent set ${SERVICE_NAME}_${1}-status $STOPPING_STATUS 
    TO_REMOVE=`sysevent get ${SERVICE_NAME}_${1}-allMembers`
    BRIDGING=`sysevent get ${SERVICE_NAME}_${1}-bridging`
    if [ x1 = x"$BRIDGING" ]; then
        BRIDGE=`sysevent get ${SERVICE_NAME}_${1}-name`
    fi
    CURNET_VID=`sysevent get ${SERVICE_NAME}_${1}-vid`
    NEW_ALL_MEMBERS=""

    del_old_members "$1"
    
    if [ x != x"$BRIDGE" ]; then
        ifconfig "$BRIDGE" down
        brctl delbr "$BRIDGE"
    fi
    
    sysevent set ${SERVICE_NAME}_${1}-status $STOPPED_STATUS
    sysevent set ${SERVICE_NAME}_${1}-allMembers
    sysevent set ${SERVICE_NAME}_${1}-bridging
    sysevent set ${SERVICE_NAME}_${1}-vid
    sysevent set ${SERVICE_NAME}_${1}-readyMembers
    sysevent set ${SERVICE_NAME}_${1}-$L2_LOCAL_READY_PARAM
    sysevent set ${SERVICE_NAME}_$1-name
    
    ACTIVE_INST="`sysevent get ${SERVICE_NAME}-instances`"
    ACTIVE_INST="`echo $ACTIVE_INST | sed 's/ *\<'$1'\> */ /'`"
    sysevent set ${SERVICE_NAME}-instances "$ACTIVE_INST"

}

if_basename () {
    echo "$1" | cut -d. -f1 | cut -d- -f1 | cut -d: -f2
}

#args: ifname, iftype
fix_status () {
    IFSTAT="`sysevent get if_$1-status`"
    READYSTAT=`eval echo \\${${2}_READY} | awk '/\<'"$1"'\>/ {print "1"}'`
    
    if [ x"$IF_READY" = x"$IFSTAT" ]; then
        if [ x = x"$READYSTAT" ]; then
            eval ${2}_READY=\"\${${2}_READY} $1\"
        fi
    else 
        if [ x1 = x"$READYSTAT" -a x"$IF_DOWN" = x"$IFSTAT" ]; then
            eval ${2}_READY=\""`eval echo \\$"{${2}_READY}" | sed 's/ *\<'"$1"'\> */ /g'`"\"
        fi
    fi
        
}

#Env in: TOADD_<types>=(varname of memberlist)
#       <types>_HANDLER
#       NETID
#       ALL_NEW_MEMBERS
#       TOADD_<types>  This variable contains the name of the variable containing the list of interfaces to be added
#               to this l2net from the specified type
config_new_members () {
    ALL_NEW_MEMBERS=""
    ALL_NEW_READY=""
    NETID=$1
    
    #DEBUG:
    echo "RDKB_SYSTEM_BOOT_UP_LOG : Entering config_new_members. Netid: ${NETID}"
    
#Bring up / create all interfaces
    for i in $INTERFACE_TYPES; do
        eval eval CURTYPE_MEMBERS=\"\\\${\${TOADD_${i}}}\"
        
        if [ x = x"$CURTYPE_MEMBERS" ]; then
            #DEBUG:
            echo "RDKB_SYSTEM_BOOT_UP_LOG : No Members of type: ${i}"
            continue
        fi
        
        #DEBUG:
        echo "RDKB_SYSTEM_BOOT_UP_LOG : TOADD_${i}: ${CURTYPE_MEMBERS}"
        
        eval CURTYPE_HANDLER=\${${i}_HANDLER}
        REGISTERED=""
        
        #Register for member interface status 
        for j in $CURTYPE_MEMBERS; do 
            IF_BASE=`if_basename "$j"`
            RET=`echo "$REGISTERED" | awk '/\<'"$IF_BASE"'\>/ {print "1"}'` 
            if [ x"$RET" = x ]; then
                REGISTERED="${REGISTERED} $IF_BASE"
            fi
            eval IF_$IF_BASE=\"\${IF_${IF_BASE}} $j\"
        done
        
        RETURNED_STATUS=`$CURTYPE_HANDLER create "$NETID" "${REGISTERED# }"`
        
        #DEBUG:
        echo "RDKB_SYSTEM_BOOT_UP_LOG : Handler returned: ${RETURNED_STATUS}"
        
        #Script prints status info in the format {type}_READY=[ifname ...]
        eval $RETURNED_STATUS
        
        #DEBUG:
        echo "RDKB_SYSTEM_BOOT_UP_LOG : Registering for these interfaces: ${REGISTERED}"
        
        for j in $REGISTERED; do
            if [ x = x"`sysevent get net_${NETID}_${j}_async`" ]; then
                async="`sysevent async if_${j}-status $THIS $NETID $i`"
                sysevent set net_${NETID}_${j}_async "$async"
                fix_status $j $i
            fi
        done
        
        #Collect all members, in the space delimeted format "type:ifname ..."
        ALL_NEW_MEMBERS="${ALL_NEW_MEMBERS} `echo ${CURTYPE_MEMBERS} | sed 's/\([^ ]*\)/'"${i}"':\1/g'`"
        
        
    done
    
    #DEBUG:
    echo "RDKB_SYSTEM_BOOT_UP_LOG : ALL_NEW_MEMBERS: ${ALL_NEW_MEMBERS}"

    #Set up remote interfaces
    for i in $REMOTE_INTERFACE_TYPES; do
        eval CURTYPE_IFS=\"\${${i}_READY}\"
        if [ x != x"$CURTYPE_IFS" ]; then
            eval CURTYPE_HANDLER=\${${i}_HANDLER}
            CURTYPE_MEMBERS=""
            for j in $CURTYPE_IFS; do
                eval CURTYPE_MEMBERS=\"${CURTYPE_MEMBERS} \${IF_${j}}\"
            done
            #DEBUG:
            echo "RDKB_SYSTEM_BOOT_UP_LOG : Adding vlans via $CURTYPE_HANDLER with netid: ${NETID} vid: $CURNET_VID, on: ${CURTYPE_MEMBERS}"
            $CURTYPE_HANDLER addVlan $NETID $CURNET_VID "$CURTYPE_MEMBERS"
            
            ALL_NEW_READY="${ALL_NEW_READY} `echo ${CURTYPE_MEMBERS} | sed 's/\([^ ]*\)/'"${i}"':\1/g'`"
        fi
    done

    #Collect local interfaces
    for i in $LOCAL_INTERFACE_TYPES; do
        eval CURTYPE_MEMBERS=\"\${CURNET_MEMBERS_${i}}\"
        if [ x != x"$CURTYPE_MEMBERS" ]; then
            ALL_LOCAL="${ALL_LOCAL} $CURTYPE_MEMBERS"
        fi
        
        eval CURTYPE_IFS=\"\${${i}_READY}\"
        if [ x != x"$CURTYPE_IFS" ]; then
            CURTYPE_MEMBERS=""
            for j in $CURTYPE_IFS; do
                eval CURTYPE_MEMBERS=\"${CURTYPE_MEMBERS} \${IF_${j}}\"
            done
            LOCAL_READY="${LOCAL_READY} ${CURTYPE_MEMBERS}"
            ALL_NEW_READY="${ALL_NEW_READY} `echo ${CURTYPE_MEMBERS} | sed 's/\([^ ]*\)/'"${i}"':\1/g'`"
        fi
        
    done
    
    #DEBUG:
    echo "RDKB_SYSTEM_BOOT_UP_LOG : ALL_LOCAL: ${ALL_LOCAL}"

    #Add bridge if necessary
    if [ x1 != x"`sysevent get ${SERVICE_NAME}_$NETID-bridging`" ]; then
        if [ `echo "$ALL_LOCAL" | wc -w` -gt 0 ]; then
            #DEBUG:
            echo "RDKB_SYSTEM_BOOT_UP_LOG : Adding bridge: $CURNET_NAME for netid:${NETID}"
            BRIDGE="$CURNET_NAME"
            NAME="$BRIDGE"
            brctl addbr "$CURNET_NAME"
            sysevent set ${SERVICE_NAME}_$NETID-name $CURNET_NAME
            sysevent set ${SERVICE_NAME}_$NETID-bridging 1
            BRIDGE_CREATED=1
        else
            #DEBUG:
            echo "RDKB_SYSTEM_BOOT_UP_LOG : Not bridging for netid:${NETID}. Name=$ALL_LOCAL"
            NAME=`sysevent get ${SERVICE_NAME}_$NETID-name`
            if [ x = x"$NAME" -o 0 = `expr match "$ALL_LOCAL" ".*$NAME.*"` ]; then
                NAME="$ALL_LOCAL"
                NAME="${NAME/-t/.$CURNET_VID}"
                sysevent set ${SERVICE_NAME}_$NETID-name $NAME
                sysevent set ${SERVICE_NAME}_$NETID-bridging 0
            fi
        fi
    else
        #DEBUG:
        echo "Using current bridge for netid:${NETID}. ${CURNET_NAME}"
        BRIDGE="$CURNET_NAME"
    fi

    # TODO: BRIDGE FILTERS

    #Assign vlans, and populate bridge if more than one member
    if [ x"$LOCAL_READY" != x ]; then
        #Add vlans
        for i in $LOCAL_READY; do
            #DEBUG:
            echo "RDKB_SYSTEM_BOOT_UP_LOG : Configuring local interface:$i, with netvid:$CURNET_VID, bridge:\"${BRIDGE}\" for netid:${NETID}"
            configure_if "add" "$i" "$CURNET_VID" "$BRIDGE"
        done
    fi
    
    if [ x1 = x$BRIDGE_CREATED ]; then
        ifconfig "$BRIDGE" up
    fi
}


#Deconfigure and unregister members
#args: netid
#envIn: TO_REMOVE, CURNET_VID, BRIDGE, NEW_ALL_MEMBERS
#envIn/Out: ALL_READY
del_old_members () {
    #DEBUG:
    echo "RDKB_SYSTEM_BOOT_UP_LOG : del_old_members:${1}, ${TO_REMOVE}"
    for i in $TO_REMOVE; do
        TYPE=`echo "$i" | cut -d: -f1`
        IFNAME=`echo "$i" | cut -d: -f2`
        IFBASE=`if_basename "$IFNAME"`
        # TODO more precise regex for type search
        echo "$LOCAL_INTERFACE_TYPES" | grep "$TYPE" > /dev/null
        if [ $? = 0 ]; then
            # LOCAL
            #DEBUG:
            echo "remove local:${IFNAME}"
            configure_if "del" "$IFNAME" "$CURNET_VID" "$BRIDGE"
        else
            # Remote
            #DEBUG:
            echo "remove remote:${IFNAME}"
            eval CURTYPE_HANDLER=\${${TYPE}_HANDLER}
            $CURTYPE_HANDLER delVlan "$1" "$CURNET_VID" "$IFNAME"
        fi
        
        echo x "$NEW_ALL_MEMBERS" | grep "${TYPE}":"${IFBASE}" > /dev/null
        if [ $? != 0 ]; then
            async="`sysevent get net_${1}_${IFBASE}_async`"
            if [ x != x"$async" ]; then
                #DEBUG:
                echo "remove registration:net_${1}_${IFBASE}_async."
                sysevent rm_async $async
                sysevent set net_${1}_${IFBASE}_async
            fi
        fi
        ALL_READY="${ALL_READY/$i/}"
    done
    #DEBUG:
    echo "Exiting del_old_members:${1}"
}

#Env in: ALL_MEMBERS, ALL_READY
#Args: netid
update_net_status () {

    LOCAL_READY=0
    
    for i in $ALL_MEMBERS; do
        echo x "$ALL_READY" | grep "$i" > /dev/null
        if [ $? = 0 ]; then
            ANYREADY=1
        else
            PARTIAL=1
        fi
    done
    
    for i in $LOCAL_INTERFACE_TYPES; do 
        echo x "$ALL_READY" |grep "${i}": > /dev/null
        if [ $? = 0 ]; then
            LOCAL_READY=1
        fi
    done

    #Set status and bring up any ready interfaces
    if [ x$PARTIAL = x1 ]; then
        if [ x$ANYREADY = x1 ]; then
            sysevent set ${SERVICE_NAME}_$1-status $PARTIAL_STATUS
        else
            sysevent set ${SERVICE_NAME}_$1-status $NONE_READY_STATUS
        fi
    else
        sysevent set ${SERVICE_NAME}_$1-status $READY_STATUS
    fi
    
    sysevent set ${SERVICE_NAME}_${1}-$L2_LOCAL_READY_PARAM $LOCAL_READY
    
    #DEBUG:
    echo "RDKB_SYSTEM_BOOT_UP_LOG : Net status update:${1}, `sysevent get ${SERVICE_NAME}_$1-status`, localready:$LOCAL_READY"

}

# TODO: FIX READY FUNCTIONS... NOT USING RIGHT SYSEVENT AND NOT ALL USES USE TYPE:IFNAME MODEL
#Args: net, ifname
#Env out: ALL_MEMBERS, ALL_READY
add_ready () {
    ALL_MEMBERS="`sysevent get ${SERVICE_NAME}_$1-allMembers`"
    ALL_READY="`sysevent get ${SERVICE_NAME}_$1-readyMembers`"
    
    echo x "$ALL_READY" | grep "$2" > /dev/null
    if [ $? != 0 ]; then
        ALL_READY="${ALL_READY} $2"
        sysevent set ${SERVICE_NAME}_$1-readyMembers "${ALL_READY}"
    fi
}

#Args: net, ifname
#Env out: ALL_MEMBERS, ALL_READY
del_ready () {
    ALL_MEMBERS="`sysevent get ${SERVICE_NAME}_$1-allMembers`"
    ALL_READY="`sysevent get ${SERVICE_NAME}_$1-readyMembers`"
    
    ALL_READY="`echo "$ALL_READY" | sed 's/ *\<'$2'\> */ /g'`"
    #ALL_READY=${ALL_READY/$2/} 
    
    sysevent set ${SERVICE_NAME}_$1-readyMembers "${ALL_READY}"
}

# args: ifname, status, netid, iftype
handle_member_status_update () {

    ALL_MEMBERS="`sysevent get ${SERVICE_NAME}_"$3"-allMembers`"
    MEMBERS="`echo "$ALL_MEMBERS" | awk '{for(i=1;i<=NF;++i)if($i~/'${4}':'${1}'.*\>/)printf "%s ",$i }'`"
    MEMBERS="${MEMBERS//${4}:/}"
    #for i in $ALL_MEMBERS; do 
    #    if [ 0 != `expr match $i .*\b${1}\b.*` ]; then 
    #        MEMBERS=${MEMBERS} ${i}
    #    fi
    #done
    
    if [ x"$IF_READY" = x"$2" ]; then
        ADDREM=add
    else
        ADDREM=del
    fi
    
    CURNET_VID=`psmcli get "${NET_IDS_DM}"."${3}"."${NET_VID}"`
    
    BRIDGING=`sysevent get ${SERVICE_NAME}_$3-bridging`
    if [ x1 = x"$BRIDGING" ]; then
        BRIDGE=`sysevent get ${SERVICE_NAME}_$3-name`
    fi
    configure_members "$MEMBERS" $ADDREM "$3" "$4"  
    
    #Check status of all member interfaces to update net status
    update_net_status "$3"
    
    
}

#args: (members...), "add"|"del", netid, iftype
#env in: CURNET_VID, [BRIDGE]
#Configure or deconfigure a type's members, without affecting registration status
configure_members() {
    LOCAL=1
    echo "$REMOTE_INTERFACE_TYPES" | grep "$4" > /dev/null
    if [ $? = 0 ]; then
        LOCAL=0
    fi
    
    READY_ACT=${2}_ready
        
    if [ $LOCAL = 1 ]; then
        
        ACTION=$2

        #Add the evented interface
        for i in $1; do
            configure_if "$ACTION" "$i" "$CURNET_VID" "$BRIDGE"
            $READY_ACT "$3" "$4":"$i"
        done
    else
        ACTION=${2}Vlan
        
        eval CURTYPE_HANDLER=\${${4}_HANDLER}
        $CURTYPE_HANDLER "$ACTION" "$3" "$CURNET_VID" "$1"
        for i in $1; do
            $READY_ACT "$3" "$4":"$i"
        done
        
    fi
}


#Args: add/del, ifname, network vid, [bridge name]
configure_if () {
    IF_BASE=${2%.*}
    IF_BASE=${IF_BASE%-t}
    IF_VLANNUM=`echo "$2" | cut -d. -f2 -s`
    
    #Use interface specific vlanID if specified (for translation)
    if [ x = x"$IF_VLANNUM" ]; then
        IF_TAGGED_FLAG=`echo "$2" | cut -d- -f2 -s`
        if [ x != x"${IF_TAGGED_FLAG}" ]; then
            IF_VLANNUM=$3
        fi   
    fi
    
    if [ x = x"${IF_VLANNUM}" ]; then
        if [ x"add" = x"$1" ]; then
            if [ x != x"$4" ]; then
                brctl addif "$4" "${IF_BASE}"
            fi
            ifconfig "${IF_BASE}" up
        else
            if [ x"del" = x"$1" ]; then
            #These calls may fail of interface no longer exists, but this allows graceful teardown 
            #in the case they still do.
                if [ x != x"$4" ]; then
                    brctl delif "$4" "${IF_BASE}"
                fi
            fi
        fi
    else
        IF_FULL=${IF_BASE}.${IF_VLANNUM}
        if [ x"add" = x"$1" ]; then
            vconfig add "$IF_BASE" "$IF_VLANNUM"
            if [ x != x"$4" ]; then
                echo 1 > /proc/sys/net/ipv6/conf/"${IF_FULL}"/disable_ipv6
                brctl addif "$4" "${IF_FULL}"
            fi
            ifconfig "${IF_BASE}" up
            ifconfig "${IF_FULL}" up
        else
            if [ x"del" = x"$1" ]; then
            #These calls may fail of interface no longer exists, but this allows graceful teardown 
            #in the case they still do.
                if [ x != x"$4" ]; then
                    brctl delif "$4" "${IF_FULL}"
                fi
                vconfig rem "${IF_FULL}"
            fi
        fi
    fi

    
}

#Args: netid
#Situations: Add member, remove member, TODO: change vid (Deferred)
sync_members ()
{
	echo "RDKB_SYSTEM_BOOT_UP_LOG : sync_members called $1"

    STAT=`sysevent get multinet_$1-status`
    if [ x = x"$STAT" -o x"$STOPPED_STATUS" = x"$STAT" ]; then
        # There is nothing to do if the current network is not up
		echo "RDKB_SYSTEM_BOOT_UP_LOG : sync_members before exit"        
        return
    fi
    load_net "$1"
    
    if [ x"${DISABLED_STATUS}" = x"$CURNET_ENABLED" ]; then 
        # If the network was disabled, bring it down. NOTE: Enabled networks must be started after being enabled
        stop_net "$1"
		echo "RDKB_SYSTEM_BOOT_UP_LOG : sync_members before exit"
        return
    fi

    ALL_MEMBERS="`sysevent get ${SERVICE_NAME}_$1-allMembers`"
    ALL_READY="`sysevent get ${SERVICE_NAME}_$1-readyMembers`"
    if [ x1 = x`sysevent get ${SERVICE_NAME}_$1-bridging` ]; then
        BRIDGE=`sysevent get ${SERVICE_NAME}_$1-name`
    fi
    
    TO_REMOVE="${ALL_MEMBERS}"

#Consider: add bridge if now more than one local, leave bridge if now less than 2 local
#          update external normally. update status registrations

    for i in $INTERFACE_TYPES; do
        eval CURTYPE_MEMBERS=\"\${CURNET_MEMBERS_${i}}\"
        for j in $CURTYPE_MEMBERS; do 
        # TODO, fix this grep to more exact regex
            #echo $TO_REMOVE | grep ${i}:${j} > /dev/null
            #expr match "$TO_REMOVE" '.*\b\('${i}:${j}'\)\b.*' > /dev/null
            RET=`echo "$TO_REMOVE" | awk '/(\s|^)'"${i}:${j}"'(\s|$)/ {print "1"}'`
            if [ x"$RET" = x1 ]; then
                TO_REMOVE="${TO_REMOVE/${i}:${j}/}"
                UNCHANGED_MEMBERS="${UNCHANGED_MEMBERS} ${i}:${j}"
            else
                eval NEW_MEMBERS_$i=\"\${NEW_MEMBERS_${i}} "$j"\"
            fi
            NEW_ALL_MEMBERS="${NEW_ALL_MEMBERS} ${i}:${j}"
        done
        eval TOADD_${i}=NEW_MEMBERS_${i}
    done
    
    # Remove old members, remote and local
    
    del_old_members "$1"
    
    if [ x = x"$BRIDGE" ]; then
        echo "$TO_REMOVE" | grep `sysevent get ${SERVICE_NAME}_$1-name` > /dev/null
        if [ 0 = $? ]; then
            sysevent set ${SERVICE_NAME}_$1-name
            #l2net name has been deleted. update status to notify other entities.
            update_net_status "$1"
        fi
    fi
    
    config_new_members "$1"
    
    ALL_READY="${ALL_READY} ${ALL_NEW_READY}"
    ALL_MEMBERS="${NEW_ALL_MEMBERS}"
    
    #ip link set $NAME up
    
    sysevent set ${SERVICE_NAME}_$NETID-allMembers "${ALL_MEMBERS}"
    sysevent set ${SERVICE_NAME}_$NETID-readyMembers "${ALL_READY}"
    
    update_net_status "$1"
    
}

# TODO: fix up copy from l3 script
sync_networks ()
{
    ACTIVE_INST="`sysevent get ${SERVICE_NAME}-instances`"
    NV_INST="`psmcli getallinst "${NET_IDS_DM}".`"
    
    TO_REM=""
    UNCHANGED=""
    TO_ADD="$NV_INST"
    
    for cur_nv_inst in $ACTIVE_INST; do
        expr match "$TO_ADD" '.*\b\('"$cur_nv_inst"'\)\b.*' > /dev/null
        if [ 0 = $? ]; then
            #Keeping this active instance
            TO_ADD="`echo $TO_ADD | sed 's/ *\<'$cur_nv_inst'\> */ /'`"
            UNCHANGED="${UNCHANGED} $cur_nv_inst"
        else
            #Remove this instance
            TO_REM="${TO_REM} $cur_nv_inst"
        fi
    done
    
    for inst in $TO_REM; do
        stop_net "$inst"
        sysevent set ${SERVICE_NAME}_${inst}-status
    done
    
#     for inst in $TO_ADD; do
#         start_net $inst
#     done
    
    #sysevent set ${SERVICE_NAME}-instances "${UNCHANGED}${TO_ADD}"
}

service_start ()
{
#
    NET_IDS="`psmcli getallinst "$NET_IDS_DM".`"
    for i in $NET_IDS; do
        start_net "$i" &
    done
    
    sysevent set ${SERVICE_NAME}-instances $NET_IDS

    wait
}

service_stop ()
{
echo
}

service_init ()
{
echo
}

#Retrieve: CURNET_NAME, CURNET_VID, CURNET_MEMBERS_<types>
#
load_net() {
    NET_GET_STRING="CURNET_NAME ${NET_IDS_DM}.${1}.${NET_IFNAME} CURNET_VID ${NET_IDS_DM}.${1}.${NET_VID} CURNET_ENABLED ${NET_IDS_DM}.${1}.${NET_ENABLED}"
    
    for i in $INTERFACE_TYPES; do 
        NET_GET_STRING="${NET_GET_STRING} CURNET_MEMBERS_$i ${NET_IDS_DM}.${1}.${NET_MEMBERS}.${i}"
    done

    eval "`psmcli get -e ${NET_GET_STRING}`"
    
    export CURNET_NAME
    export CURNET_VID
}

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------

echo "RDKB_SYSTEM_BOOT_UP_LOG : service_multinet called with $1 $2 $3 $4"

#service_init
case "$1" in
    "${SERVICE_NAME}-start")
        if [ x"NULL" = x"$2" ]; then
            service_start
        else
            start_net "$2" 1
        fi
        ;;
    "${SERVICE_NAME}-stop")
        service_stop
        ;;
    "${SERVICE_NAME}-restart")
 #     if [ x"unknown" != x"$SYSEVENT_current_hsd_mode" ]; then
         sysevent set ${SERVICE_NAME}-restarting 1
         service_stop
         service_start
         sysevent set ${SERVICE_NAME}-restarting 0
 #     fi
        ;;
    if_*-status)
        #Params: event name, if status value, netid, iftype
        ENDCHAR=`expr ${#1} - 7`
        IFNAME=`echo "$1" | cut -c 4-"$ENDCHAR"`
        
        handle_member_status_update "$IFNAME" "$2" "$3" "$4"
        ;;
    "${SERVICE_NAME}-up")
        #NETID=`echo $1 | cut -d_ -f2 | cut -d- -f1`
        start_net "$2"
    ;;
    "${SERVICE_NAME}-down")
        #NETID=`echo $1 | cut -d_ -f2 | cut -d- -f1`
        stop_net "$2"
    ;;

    "${SERVICE_NAME}-syncMembers")
        #STARTCHAR=${#SERVICE_NAME}
        #ENDCHAR=`expr ${#1} - 12`
        #NETID=`echo $1 | cut -c $STARTCHAR-$ENDCHAR`
        sync_members "$2"
        #Params: event name, event value?, 
    ;;
    "${SERVICE_NAME}-syncNets")
        #Params: none used
        sync_networks
    ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac

# TODO CHECK READY MEMBERS ALL MEMBERS COMPATABILITY
