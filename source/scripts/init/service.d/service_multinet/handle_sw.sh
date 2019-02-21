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

source /etc/utopia/service.d/log_capture_path.sh

TYPE=SW

TAGGING_MODE=2
UNTAGGED_MODE=1
NATIVE_MODE=0

SW_ALL_PORTS="sw_1 sw_2 sw_3 sw_4 sw_5 atom arm I2E E2I"

PORTMAP_sw_1="-c 0 -p 0"
PORTMAP_sw_2="-c 0 -p 1"
PORTMAP_sw_3="-c 0 -p 2"
PORTMAP_sw_4="-c 0 -p 3"
PORTMAP_sw_5="-c 16 -p 3"  #moca
PORTMAP_atom="-c 16 -p 0"
PORTMAP_arm="-c 16 -p 7"
PORTMAP_I2E="-c 16 -p 2"
PORTMAP_E2I="-c 0 -p 5"
PORTMAP_IOT_1="-c 16 -p 0"
PORTMAP_IOT_2="-c 16 -p 7"

PORTMAP_DEF_sw_1="-c 34 -p 0"
PORTMAP_DEF_sw_2="-c 34 -p 1"
PORTMAP_DEF_sw_3="-c 34 -p 2"
PORTMAP_DEF_sw_4="-c 34 -p 3"
PORTMAP_DEF_sw_5="-c 16 -p 3 -m 0 -q 1"  #moca
PORTMAP_DEF_atom="-c 16 -p 0 -m 0 -q 1"
PORTMAP_DEF_arm="-c 16 -p 7 -m 0 -q 1"
PORTMAP_DEF_I2E="-c 16 -p 2 -m 0 -q 1"
PORTMAP_DEF_E2I="-c 34 -p 5"

PORTMAP_REM_sw_1="-c 1 -p 0"
PORTMAP_REM_sw_2="-c 1 -p 1"
PORTMAP_REM_sw_3="-c 1 -p 2"
PORTMAP_REM_sw_4="-c 1 -p 3"
PORTMAP_REM_sw_5="-c 17 -p 3"  #moca
PORTMAP_REM_atom="-c 17 -p 0"
PORTMAP_REM_arm="-c 17 -p 7"
PORTMAP_REM_I2E="-c 17 -p 2"
PORTMAP_REM_E2I="-c 1 -p 5"


PORTMAP_VENABLE_sw_1="-c 4 -p 0"
PORTMAP_VENABLE_sw_2="-c 4 -p 1"
PORTMAP_VENABLE_sw_3="-c 4 -p 2"
PORTMAP_VENABLE_sw_4="-c 4 -p 3"
PORTMAP_VENABLE_sw_5="-c 20 -p 3"  #moca
PORTMAP_VDISABLE_sw_5="-c 21 -p 3" #moca
PORTMAP_VENABLE_atom="-c 20 -p 0"
PORTMAP_VENABLE_arm="-c 20 -p 7"
PORTMAP_VENABLE_I2E="-c 20 -p 2"
PORTMAP_VENABLE_E2I="-c 4 -p 5"

EXTPORTS="sw_1|sw_2|sw_3|sw_4"
ATOM_PORTS='ath*|sw_6'

EXT_DEP="I2E-t E2I-a"
ATOM_DEP="atom-t"

#MGMT_PORT_LINUX_IFNAME=l2sm0

add_untagged_moca_vid_special () {
    if [ x$1 = x ]; then
        return
    fi
    mocactl -c 8 -e 1 -v $1
}

del_untagged_moca_vid_special () {
    if [ x$1 = x ]; then
        return
    fi
    mocactl -c 8 -e 0 -v $1

}

#env in: $VID - vid, $TAG - tagging flag, 
#args, add? (1 or 0),
handle_moca () {
    MTPORTS="`sysevent get sw_moca_tports`"
    MUTPORT="`sysevent get sw_moca_utport`"
    
    if [ x1 = x$1 ]; then
        #adding a vlan to moca
        if [ x != x$TAG ]; then
            #adding trunking vlan
            if [ x = x"${MTPORTS}" ]; then
                #need to enable vlans and convert untagged vlan if it exists
                swctl $PORTMAP_VENABLE_sw_5
                add_untagged_moca_vid_special $MUTPORT
                swctl -v $MUTPORT -m $NATIVE_MODE -q 1 $PORTMAP_sw_5
            fi
            MTPORTS="${MTPORTS} $VID"
            sysevent set sw_moca_tports "$MTPORTS"
        else
            #adding untagged vlan
            if [ x != x"${MTPORTS}" ]; then
                #add to moca filter, and add TAG
                add_untagged_moca_vid_special $VID 
                swctl -v $MUTPORT -m $NATIVE_MODE -q 1 $PORTMAP_sw_5
                PORTMAP_sw_5=""
            fi
            sysevent set sw_moca_utport $VID
        fi
    else
        #removing a vlan from moca
        if [ x != x$TAG ]; then
            #removing trunking vlan
            MTPORTS="`echo $MTPORTS| sed 's/ *\<'$VID'\>\( *\)/\1/g'`"
            if [ x = x"${MTPORTS}" ]; then
                #need to disable vlans and re-add untagged vlan if it exists
                swctl $PORTMAP_VDISABLE_sw_5
                del_untagged_moca_vid_special $MUTPORT
                swctl -v $MUTPORT -m $UNTAGGED_MODE -q 1 $PORTMAP_sw_5
            fi
            sysevent set sw_moca_tports "$MTPORTS"
        else
            #removing untagged vlan
            if [ x != x"${MTPORTS}" ]; then
                #del from moca filter, and add TAG
                del_untagged_moca_vid_special $VID 
                TAG=-t
            fi
            if [ x$MUTPORT = x$VID ]; then
                sysevent set sw_moca_utport
            fi
        
        fi 
    fi
    
}

#args portname(unmapped), [t] 
check_for_dependent_ports () {
    if [ x = x$2 ]; then
      name=$1
    else
      name=$1-$2
    fi
    eval case "$1" in \
        $ATOM_PORTS\) \
            PORTS_ATOM_ADD=\"${PORTS_ATOM_ADD} $name\" \
        \;\; \
        $EXTPORTS\) \
            PORTS_EXT_ADD=\"${PORTS_EXT_ADD} $name\" \
        \;\; \
        sw_5\) \
            handle_moca 1\
        \;\; \
    esac
        
}

#args: vid, "ports", [force enable = 1]
sw_add_ports() {
    for i in $2; do
        PORT=`echo $i | cut -d- -f1`
        TAG=`echo $i | cut -d- -s -f2`
        check_for_dependent_ports $PORT $TAG
        eval CMD=\"\${PORTMAP_${PORT}}\"
        if [ x = x"$CMD" ]; then
            continue
        fi
        eval ENABLE=\"\${PORTMAP_VENABLE_${PORT}}\"
        
        
        if [ x = x$TAG ]; then
            TAG=$UNTAGGED_MODE
        else
            TAG=$TAGGING_MODE
        fi
        
        if [ x1 = x$3 -o x = x`sysevent get sw_port_${PORT}_venable` ]; then
            #DEBUG
            echo "--SW handler, swctl $ENABLE"
            swctl $ENABLE
            sysevent set sw_port_${PORT}_venable 1
        fi
        
        #DEBUG
        echo "--SW handler, swctl $CMD -v $1 -m $TAG -q 1"
        swctl $CMD -v $1 -m $TAG -q 1
        
        if [ $TAG = $UNTAGGED_MODE ]; then
            eval DEF_CMD=\"\${PORTMAP_DEF_${PORT}}\"
            if [ x != x"$DEF_CMD" ]; then
                swctl $DEF_CMD -v $1
            fi
        fi
        
        #echo $EXTPORTS |grep $PORT > /dev/null
        #if [ 0 = $? ]; then
        #    PORTS_EXT_ADD="${PORTS_EXT_ADD} $PORT"
        #fi
    done
}

restore_ext_sw() {
    EXTVIDS=`sysevent get sw_ext_vids`
    for i in $EXTVIDS; do
        swctl $PORTMAP_E2I -v $i -m $TAGGING_MODE -q 1
        sw_add_ports $i "`sysevent get sw_vid_${i}_extports`" 1
    done
}

#service_init
case "$1" in
#  Synchronous calls
    #Args: netid, members...
    create)
        echo ${TYPE}_READY=\"$3\"
        ;;
    destroy)
        
        ;;
        
    initialize)
        #vconfig add l2sm0 2
        #ifconfig l2sm0.2 up
        #for i in $SW_ALL_PORTS; do 
        #    eval swctl \${PORTMAP_VENABLE_${i}}
        #    eval swctl \${PORTMAP_${i}} -v 2 -m $NATIVE_MODE -q 1
        #    sysevent set sw_port_${i}_venable 1
        #done
    	echo "SW Init"
    	sysevent set sw_port_sw_5_venable 1
    ;;
    #Args: netid, netvid, members...
    # NOTE: Does not support vlan translation (port specific VID)
    addVlan)
        if [ x = x"$4" ]; then
            return
        fi
		NETID=$2
        VID=$3
	
        for i in ${4}; do
            if [ x"-t" = x"$i" -o x = x"$i" ]; then
                echo "Not adding $i to PORTS_ADD" 
            else
                PORTS_ADD="${PORTS_ADD} $i"
            fi
            shift
        done
        PORTS_EXT_ADD=""
        
        #DEBUG
        echo "--SW handler, adding vlan $VID on net $NETID for $PORTS_ADD"
        VIDPORTS="`sysevent get sw_vid_${VID}_ports`"
        #add to arm port (implicit rule)
        if [ x = x`sysevent get sw_port_arm_venable` ]; then
            #DEBUG
            echo "--SW handler, swctl $PORTMAP_VENABLE_arm"
            swctl $PORTMAP_VENABLE_arm
            sysevent set sw_port_arm_venable 1
        fi
        
        #add the management virtual interface first
#        if [ x = x"$VIDPORTS" ]; then
#            vconfig add $MGMT_PORT_LINUX_IFNAME ${VID}
#            ifconfig $MGMT_PORT_LINUX_IFNAME.${VID} up
#        fi
        
        sw_add_ports $VID "$PORTS_ADD"
        
        if [ x = x"$VIDPORTS" ]; then 
            #DEBUG
            echo "--SW handler, swctl $PORTMAP_arm -v ${VID} -m $TAGGING_MODE -q 1"
            swctl $PORTMAP_arm -v ${VID} -m $TAGGING_MODE -q 1
            #Re-add the default vlan to allow normal handling for untagged traffic
            #swctl $PORTMAP_arm -v 2 -m $NATIVE_MODE -q 1
        fi
        sysevent set sw_vid_${VID}_ports "${VIDPORTS} ${PORTS_ADD}"
        
        #Add to switch connection ports if on external switch
        if [ x != x"$PORTS_EXT_ADD" ]; then
            #Enable if not already
            if [ x = x`sysevent get sw_port_I2E_venable` ]; then
                #DEBUG
                echo "--SW handler, swctl $PORTMAP_VENABLE_I2E"
                swctl $PORTMAP_VENABLE_I2E
                sysevent set sw_port_I2E_venable 1
            fi
            if [ x = x`sysevent get sw_port_E2I_venable` ]; then
                #DEBUG
                echo "--SW handler, swctl $PORTMAP_VENABLE_E2I"
                swctl $PORTMAP_VENABLE_E2I
                sysevent set sw_port_E2I_venable 1
            fi
            
            #Add vlan if not already added
            EXT_VIDPORTS="`sysevent get sw_vid_${VID}_extports`"
            if [ x = x"$EXT_VIDPORTS" ]; then
                #DEBUG
                echo "--SW handler, swctl $PORTMAP_I2E -v $VID -m $TAGGING_MODE -q 1"
                echo "--SW handler, swctl $PORTMAP_E2I -v $VID -m $TAGGING_MODE -q 1"
                swctl $PORTMAP_I2E -v ${VID} -m $TAGGING_MODE -q 1
                #swctl $PORTMAP_I2E -v 2 -m $NATIVE_MODE -q 1
                swctl $PORTMAP_E2I -v ${VID} -m $TAGGING_MODE -q 1
                #swctl $PORTMAP_E2I -v 2 -m $NATIVE_MODE -q 1
                
                #Save list of vids
                sysevent set sw_ext_vids "${VID} `sysevent get sw_ext_vids`"
            fi
            
            #Save list of members
            sysevent set sw_vid_${VID}_extports "${EXT_VIDPORTS}${PORTS_EXT_ADD}"
        fi
        
        if [ x != x"$PORTS_ATOM_ADD" ]; then
            if [ x = x`sysevent get sw_port_atom_venable` ]; then
                #DEBUG
                echo "--SW handler, swctl $PORTMAP_VENABLE_atom"
                swctl $PORTMAP_VENABLE_atom
                sysevent set sw_port_atom_venable 1
            fi
           
            ATOM_VIDPORTS="`sysevent get sw_vid_${VID}_atomports`"
            if [ x = x"$ATOM_VIDPORTS" ]; then
                #DEBUG
                echo "--SW handler, swctl $PORTMAP_atom -v ${VID} -m $TAGGING_MODE -q 1"
                swctl $PORTMAP_atom -v ${VID} -m $TAGGING_MODE -q 1
                #swctl $PORTMAP_atom -v 2 -m $NATIVE_MODE -q 1
            fi
            
            sysevent set sw_vid_${VID}_atomports "${ATOM_VIDPORTS}${PORTS_ATOM_ADD}"
        fi
    ;;
    
    addIotVlan)
          
        #swctl -c 16 -p 0 -v 106 -m 2 -q 1
        #swctl -c 16 -p 7 -v 106 -m 2 -q 1
        #vconfig add l2sd0 106
        #ifconfig l2sd0.106 192.168.106.1 255.255.255.0 up
        echo "IOT_LOG : hande_sw received addIotVlan"
        iotEnabled=`syscfg get lost_and_found_enable`
        if [ "$iotEnabled" = "true" ]
        then
             NETID=$2
             VID=$3
	
             iot_ipaddress=`syscfg get iot_ipaddr`
             iot_interface=`syscfg get iot_ifname`
             if [ $iot_interface == "l2sd0.106" ]; then
              iot_interface=`syscfg get iot_brname`
             fi
             iot_mask=`syscfg get iot_netmask`

             echo "IOT_LOG : Adding VLAN l2sd0.$VID"
             vconfig add l2sd0 $VID
             ifconfig $iot_interface $iot_ipaddress netmask $iot_mask up
             #DEBUG
             echo "IOT_LOG : --SW handler, adding vlan $VID for IOT"
             swctl $PORTMAP_IOT_1 -v $VID -m $TAGGING_MODE -q 1
             swctl $PORTMAP_IOT_2 -v $VID -m $TAGGING_MODE -q 1
        else
             echo "IOT_LOG : No need to add vlan for IOT"
             return
        fi
    ;; 
    #Args: netid, netvid, members
    delVlan)
    
    
        if [ x = x"$4" ]; then
            return
        fi
        
        #DEBUG
        echo "--SW handler, removing vlan $3 on net $2 for $4"
        
        VID=$3
        VIDPORTS="`sysevent get sw_vid_$3_ports`"
        EXT_VIDPORTS="`sysevent get sw_vid_$3_extports`"
        ATOM_VIDPORTS="`sysevent get sw_vid_$3_atomports`"
        
        for i in ${4}; do
            PORT=`echo $i | cut -d- -f1`
            TAG=`echo $i | cut -d- -s -f2`
            VIDPORTS="`echo $VIDPORTS| sed 's/ *\<'$i'\>\( *\)/\1/g'`"
            eval case $PORT in \
                $ATOM_PORTS\) \
                    ATOM_VIDPORTS=\"`echo $ATOM_VIDPORTS| sed 's/ *\<'$i'\>\( *\)/\1/g'`\" \
                \;\; \
                $EXTPORTS\) \
                    EXT_VIDPORTS=\"`echo $EXT_VIDPORTS| sed 's/ *\<'$i'\>\( *\)/\1/g'`\" \
                \;\; \
                sw_5\) \
                    handle_moca 0 \
                \;\; \
            esac
            
            eval CMD=\"\${PORTMAP_REM_${PORT}}\"
            if [ x = x"$CMD" ]; then
                continue
            fi
            
            #DEBUG
            echo "--SW handler, swctl $CMD -v $3"
            swctl $CMD -v $3
            if [ x = x$TAG ]; then
                eval swctl \${PORTMAP_DEF_${PORT}} -v 0
            fi
        done
        
        #check for arm port removal (implicit rule)
        if [ x = x"$VIDPORTS" ]; then 
            #DEBUG
            echo "--SW handler, swctl $PORTMAP_REM_arm -v $3"
            swctl $PORTMAP_REM_arm -v $3
#            vconfig rem $MGMT_PORT_LINUX_IFNAME.$3
        fi
        sysevent set sw_vid_$3_ports "${VIDPORTS}"
        
        #Add to switch connection ports if on external switch
        if [ x = x"$EXT_VIDPORTS" ]; then
            #DEBUG
            echo "--SW handler, swctl $PORTMAP_REM_I2E -v $3"
            echo "--SW handler, swctl $PORTMAP_REM_E2I -v $3"
            swctl $PORTMAP_REM_I2E -v $3
            swctl $PORTMAP_REM_E2I -v $3
            EXTVIDS=`sysevent get sw_ext_vids`
            EXTVIDS="`echo $EXTVIDS| sed 's/ *\<'$VID'\>\( *\)/\1/g'`"
            sysevent set sw_ext_vids $EXTVIDS
        fi
        #Save list of members
        sysevent set sw_vid_$3_extports "${EXT_VIDPORTS}"
        
        #check for atom port removal (implicit rule)
        if [ x = x"$ATOM_VIDPORTS" ]; then 
            #DEBUG
            echo "--SW handler, swctl $PORTMAP_REM_atom -v $3"
            swctl $PORTMAP_REM_atom -v $3
        fi
        sysevent set sw_vid_$3_atomports "${ATOM_VIDPORTS}"
    ;;
      
#  Sysevent calls
    sw_ext_restore)
        restore_ext_sw
    ;;
    
    *)
        exit 3
        ;;
esac
