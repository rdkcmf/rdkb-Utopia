#!/bin/sh

#This script is used to sync the dhcp lease file from ARM to ATOM
#------------------------------------------------

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

DHCP_LEASE_FILE_ARM="/nvram/dnsmasq.leases"
DHCP_LEASE_FILE_ATOM="/nvram/dnsmasq.leases"

nice -n 19 scp $DHCP_LEASE_FILE_ARM root@$ATOM_IP:$DHCP_LEASE_FILE_ATOM  > /dev/null 2>&1
