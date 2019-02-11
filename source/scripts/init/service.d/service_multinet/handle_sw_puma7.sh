#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
# Copyright 2017 Intel Corporation
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

#############################################################################
#  Copyright 2017 Intel Corporation
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
###############################################################################


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

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------


# This is a empty switch handler in puma7 we don't need to call swctl directly rather we should be calling vlan utils to 
# create bridge and adding interfaces. Need to make changes in utopia scripts and code to handle it properly which is WIP
# Once utopia is modified we can remove this handler.

SWITCH_MAP_FILE="/var/tmp/map/mapping_table.txt"
PORT_VLAN_START=200
PORT_VLAN_INC=100
PORT_IF_PREFIX="sw_"
PORT_IF_OFFSET=0

#Adds syscfg entry for network interface we create for this switch port
#Argument: interface name
find_add_syscfg_entry(){
    EXISTING=`syscfg get CosaEthIntIDs::${1}`
    #Add only if it doesn't already exist
    if [ "$EXISTING" = "" ] ; then
        #Find the next available interface ID to use
        MAX_INDEX=1
        EXISTING_LIST=`syscfg show|egrep -e "^CosaEthIntIDs::"|cut -d "," -f 2`
        while read -r LINE; do
            if [ $LINE -gt $MAX_INDEX ] ; then
                MAX_INDEX=$LINE
            fi
        done <<< "$EXISTING_LIST"
        MAX_INDEX=`expr $MAX_INDEX + 1`
        KEY="CosaEthIntIDs::${1}"
        VALUE="Interface${MAX_INDEX},${MAX_INDEX}"
        echo "Going to add ${1} command: syscfg set $KEY $VALUE"
        syscfg set $KEY $VALUE
    else
        echo "Not adding ${1}, already exists, value: $EXISTING"
    fi
}

#Argument: Port ID
get_port_if_name() {
	PORT_IF_NUM=`expr $1 + $PORT_IF_OFFSET`
	PORT_IF_NAME="${PORT_IF_PREFIX}${PORT_IF_NUM}"
}
#Argument: Port ID
get_auto_vlan() {
	AUTO_VLAN=`expr $1 \* $PORT_VLAN_INC + $PORT_VLAN_START`
}

#Arguments: Port ID, VLAN ID
add_vlan_to_port() {
	swctl -l 3 -c 4 -p $1
	swctl -l 3 -c 0 -p $1 -v $2 -m 2 -q 1
}

#Arguments: Port ID, VLAN ID
del_vlan_from_port() {
	get_auto_vlan $1
	swctl -l 3 -c 1 -p $1 -v $2
}

#Get the port ID and base interface for each switch port in mapping file
extract_from_map_line() {
	LOGICAL_PORT="${4}"
	BASE_IF="${14}"	
}

#Argument: "up" or "down"
configure_all_switch_ports() {
	case "$1" in
	up)
		#Configure first port which will ensure mapping file is created
		swctl -l 3 -c 4 -p 0

		#Iterate through switch mapping file and configure every port
		cat $SWITCH_MAP_FILE | while read LINE; do
			extract_from_map_line $LINE
			get_auto_vlan $LOGICAL_PORT
			get_port_if_name $LOGICAL_PORT
			add_vlan_to_port $LOGICAL_PORT $AUTO_VLAN
			ip link add link $BASE_IF name $PORT_IF_NAME type vlan id $AUTO_VLAN
			sysctl -w net.ipv6.conf.$PORT_IF_NAME.disable_ipv6=1
			ifconfig $PORT_IF_NAME up
			find_add_syscfg_entry $PORT_IF_NAME
		done
	;;
	down)
		#Configure first port which will ensure mapping file is created
		swctl -l 3 -c 4 -p 0

		#Iterate through switch mapping file and configure every port
		cat $SWITCH_MAP_FILE | while read LINE; do
			extract_from_map_line $LINE
			get_auto_vlan $LOGICAL_PORT
			get_port_if_name $LOGICAL_PORT
			del_vlan_from_port $LOGICAL_PORT $AUTO_VLAN
			ip link del $PORT_IF_NAME
		done
	;;
	esac
}
MULTILAN_FEATURE=$(syscfg get MULTILAN_FEATURE)
if [ $MULTILAN_FEATURE = 1 ]; then
	case "$1" in
	initialize)
		configure_all_switch_ports up
	;;
	deinitialize)
		configure_all_switch_ports down
	;;
	*)
		exit 3
	;;
	esac
else
	exit 0
fi
