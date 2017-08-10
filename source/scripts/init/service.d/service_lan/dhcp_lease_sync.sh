#!/bin/sh

#This script is used to sync the dhcp lease file from ARM to ATOM
#------------------------------------------------

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

DHCP_LEASE_FILE_ARM="/nvram/dnsmasq.leases"
DHCP_LEASE_FILE_ATOM="/nvram/dnsmasq.leases"
DHCP_LEASE_FILE_ATOM_TMP="/tmp/dnsmasq.leases"
PEER_COMM_DAT="/etc/dropbear/elxrretyt.swr"
PEER_COMM_ID="/tmp/elxrretyt-$$.swr"
CONFIGPARAMGEN="/usr/bin/configparamgen"


$CONFIGPARAMGEN jx $PEER_COMM_DAT $PEER_COMM_ID
scp -i $PEER_COMM_ID $DHCP_LEASE_FILE_ARM root@$ATOM_IP:$DHCP_LEASE_FILE_ATOM_TMP  > /dev/null 2>&1
rpcclient $ATOM_ARPING_IP "flock $DHCP_LEASE_FILE_ATOM -c \"cp $DHCP_LEASE_FILE_ATOM_TMP $DHCP_LEASE_FILE_ATOM\""
rpcclient $ATOM_ARPING_IP "rm -f $DHCP_LEASE_FILE_ATOM_TMP"
rm -f $PEER_COMM_ID
