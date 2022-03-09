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

#This script is used to sync the dhcp lease file from ARM to ATOM
#------------------------------------------------

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

DHCP_LEASE_FILE_ARM="/nvram/dnsmasq.leases"
DHCP_LEASE_FILE_ATOM="/nvram/dnsmasq.leases"
DHCP_LEASE_FILE_ATOM_TMP="/tmp/dnsmasq.leases"
PEER_COMM_ID="/tmp/elxrretyt.swr"
if [ ! -f /usr/bin/GetConfigFile ];then
    echo "Error: GetConfigFile Not Found"
    exit 127
fi

GetConfigFile $PEER_COMM_ID
scp -i $PEER_COMM_ID $DHCP_LEASE_FILE_ARM root@"$ATOM_IP":$DHCP_LEASE_FILE_ATOM_TMP  > /dev/null 2>&1
rpcclient "$ATOM_ARPING_IP" "flock $DHCP_LEASE_FILE_ATOM -c \"cp $DHCP_LEASE_FILE_ATOM_TMP $DHCP_LEASE_FILE_ATOM\""
rpcclient "$ATOM_ARPING_IP" "rm -f $DHCP_LEASE_FILE_ATOM_TMP"
rm -f $PEER_COMM_ID
