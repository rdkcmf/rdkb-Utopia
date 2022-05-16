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
if [ -f /etc/device.properties ]
then
     source /etc/device.properties
fi
UTOPIAROOT=/etc/utopia/service.d
SERVICEROOT=$UTOPIAROOT/service_${SERVICE_NAME}
THIS=$UTOPIAROOT/service_${SERVICE_NAME}.sh

#MULTINET

#Local interfaces will be consulted for bringup only
LOCAL_INTERFACE_TYPES="Gre Link Eth"
REMOTE_INTERFACE_TYPES="SW WiFi"
INTERFACE_TYPES="$LOCAL_INTERFACE_TYPES $REMOTE_INTERFACE_TYPES"

L2SERVICE_NAME=multinet
L2_LOCAL_READY_PARAM=localready
L2SERVICE_IFNAME=name

Eth_HANDLER=${UTOPIAROOT}/service_${L2SERVICE_NAME}/handle_eth.sh
Link_HANDLER=${UTOPIAROOT}/service_${L2SERVICE_NAME}/handle_link.sh
SW_HANDLER=${UTOPIAROOT}/service_${L2SERVICE_NAME}/handle_sw.sh
WiFi_HANDLER=${UTOPIAROOT}/service_${L2SERVICE_NAME}/handle_wifi.sh
Gre_HANDLER=${UTOPIAROOT}/service_${L2SERVICE_NAME}/handle_gre.sh

#PSM mapping
PSM_PREFIX=dmsb
NET_IDS_DM=$PSM_PREFIX.l2net
NET_IFNAME=Name
NET_MEMBERS=Members
NET_VID=Vid
NET_ENABLED=Enable

#Multinet Statuses
PARTIAL_STATUS=partial
NONE_READY_STATUS=pending
READY_STATUS=ready
STOPPED_STATUS=stopped
STOPPING_STATUS=stopping
ENABLED_STATUS=TRUE
DISABLED_STATUS=FALSE



#Interface Statuses 
IF_READY=ready
IF_DOWN=down

#L3 net statuses
L3_UP_STATUS=up
L3_AWAITING_STATUS=unconfigured

ETH_DM_PREFIX=dmsb.EthLink

#l3 net nonvols
IPV4_NV_PREFIX=dmsb.l3net
IPV4_NV_IP=V4Addr
IPV4_NV_SUBNET=V4SubnetMask
IPV4_NV_ENABLED=Enable
if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ]; then
IPV4_NV_MTU=MaxMTU
fi

#true static ip
IPV4_TSIP_PREFIX=dmsb.truestaticip
IPV4_TSIP_ASNPREFIX=dmsb.truestaticip.Asn
IPV4_TSIP_ENABLE=Enable
IPV4_TSIP_IP=Ipaddress
IPV4_TSIP_SUBNET=Subnetmask
IPV4_TSIP_GATEWAY=Gateway


#dhcp server
DHCPS_POOL_NVPREFIX=dmsb.dhcpv4.server.pool
IPV4_DM=IPInterface
START_ADDR_DM=MinAddress
END_ADDR_DM=MaxAddress
SUBNET_DM=SubnetMask
LEASE_DM=LeaseTime
ENABLE_DM=Enable

#UPNP
UPNP_DM=UPnPIGDEnabled

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

isvalid() {
	if [ "$1" -eq 255 ] || [ "$1" -eq 254 ] || [ "$1" -eq 252 ] || [ "$1" -eq 248 ] || [ "$1" -eq 240 ] || [ "$1" -eq 224 ] || [ "$1" -eq 192 ] || [ "$1" -eq 128 ] || [ "$1" -eq 0 ]; then
		echo 1
	else
		echo 0
	fi
}

calbits()
{
	count=0
	bitfield=$1
	while [ "$bitfield" -ne 0 ]; do
		bitfield_1=`expr "$bitfield" - 1`
		bitfield=$((bitfield & bitfield_1))
		let count+=1
	done
	echo "$count"
}

mask2cidr() {
	numberofbits=0
	echo "Mask2cidr called on :${1}:" > /dev/console
	fields=`echo "$1" | sed 's/\./ /g'`
	for field in $fields ; do
		echo "dec:${field}:" > /dev/console
		if [ "`isvalid "$field"`" -eq 1 ]; then
			numberofbits=$((numberofbits + `calbits "$field"`))
		else
			echo "Error: $field is not recognised"; exit 1
		fi
	done
	echo $numberofbits
}
