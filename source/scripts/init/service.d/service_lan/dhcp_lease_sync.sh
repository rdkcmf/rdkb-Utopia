#!/bin/sh
#This script is used to sync the dhcp lease file from ARM to ATOM
#------------------------------------------------
ATOM_RPC_IP=`cat /etc/device.properties | grep ATOM_ARPING_IP | cut -f 2 -d"="`

rpcclient $ATOM_RPC_IP "rm  /nvram/_dnsmasq.leases"
while read line ; do
	rpcclient $ATOM_RPC_IP "echo \"$line\" >> /nvram/_dnsmasq.leases"
done < /nvram/dnsmasq.leases
rpcclient $ATOM_RPC_IP "cp /nvram/_dnsmasq.leases /nvram/dnsmasq.leases"
