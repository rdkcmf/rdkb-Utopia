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

# This script will push the DNS from ARM to ATOM.
#------------------------------------------------

. /etc/device.properties

TMP_RESOLV_FILE=/tmp/tmp_resolv.conf
RESOLV_CONF=/etc/resolv.conf
ATOM_USER_NAME=root
retries=0

#ATOM_INTERFACE_IP=`cat /etc/device.properties | grep ATOM_INTERFACE_IP | cut -f 2 -d"="`
#ATOM_RPC_IP=`cat /etc/device.properties | grep ATOM_ARPING_IP | cut -f 2 -d"="`
LAN_IP=`syscfg get lan_ipaddr`
PEER_COMM_ID="/tmp/elxrretyt.swr"
if [ ! -f /usr/bin/GetConfigFile ];then
    echo "Error: GetConfigFile Not Found"
    exit 127
fi

#If we don't have an IP to copy the DNS settings to, there is no need to proceed
if [ -z "$ATOM_INTERFACE_IP" ] ; then
    echo "DNS sync not needed"
    exit  0
fi

while :
do
        numOfEntries=`cat $RESOLV_CONF | grep nameserver | grep -v 127.0.0.1 | wc -l`
        if [ "$numOfEntries" -gt 2 -o $retries -eq 120 ]
        then
            break
        fi

        sleep 5
        retries=$(( $retries + 1 ))
done

# Synch the available DNS entries and wait until all entries are available.
#DNS_STR=`cat $RESOLV_CONF | grep nameserver | grep -v 127.0.0.1`
#echo "$DNS_STR" > $TMP_RESOLV_FILE

DNS_STR_IPV4=`cat $RESOLV_CONF | grep nameserver | grep -v 127.0.0.1 | grep "\."`
DNS_STR_IPV6=`cat $RESOLV_CONF  | grep nameserver | grep -v 127.0.0.1 | grep "\:"`

if [ -n "$DNS_STR_IPV4" ];then
    echo "$DNS_STR_IPV4" > $TMP_RESOLV_FILE
else
    > $TMP_RESOLV_FILE
fi

if [ -n "$DNS_STR_IPV6" ];then
    echo "$DNS_STR_IPV6" >> $TMP_RESOLV_FILE
fi


DNS_STR_V4=`echo "$DNS_STR_IPV4" | cut -d" " -f2`
if [ -z "$DNS_STR_V4" ]
then
	echo "No IPv4 DNS entries available add default as lan IP"
	echo "$LAN_IP" >> $TMP_RESOLV_FILE
fi

echo "TMP_RESOLV_FILE = $TMP_RESOLV_FILE"
cat $TMP_RESOLV_FILE

GetConfigFile $PEER_COMM_ID
scp -i $PEER_COMM_ID $TMP_RESOLV_FILE $ATOM_USER_NAME@"$ATOM_INTERFACE_IP":$RESOLV_CONF > /dev/null 2>&1

if [ $? -eq 0 ]; then
	echo "scp is successful at first instance"
else
	retries=0
	while :
	do
		scp -i $PEER_COMM_ID $TMP_RESOLV_FILE $ATOM_USER_NAME@"$ATOM_INTERFACE_IP":$RESOLV_CONF > /dev/null 2>&1
		if [ $? -eq 0 -o  $retries -gt 4 ]
		then
		    if [ $retries -le 4 ]
		    then
			    echo "scp is successful at iteration:$retries"
			else
			    echo "scp of resolv.conf failed. trying rpcclient"
			    FILE_STR=`cat $TMP_RESOLV_FILE`
			    rpcclient "$ATOM_ARPING_IP" "echo \"$FILE_STR\" > $RESOLV_CONF"
			fi 
			break	
		else
			sleep 5
			retries=$(( $retries + 1 ))
		fi 
	done	
fi

rm -f $PEER_COMM_ID
rm -f $TMP_RESOLV_FILE
