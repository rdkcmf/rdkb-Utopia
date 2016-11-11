#!/bin/sh
# This script will push the DNS from ARM to ATOM.
#------------------------------------------------
ATOM_RPC_IP=`cat /etc/device.properties | grep ATOM_ARPING_IP | cut -f 2 -d"="`
retries=0

# Synch the available DNS entries and wait until all entries are available.
DNS_STR=`cat /etc/resolv.conf | grep nameserver | grep -v 127.0.0.1`
rpcclient $ATOM_RPC_IP "echo \"$DNS_STR\" > /etc/resolv.conf"

while :
do
        numOfEntries=`cat /etc/resolv.conf | grep nameserver | grep -v 127.0.0.1 | wc -l`
        if [ $numOfEntries -gt 2 -o $retries -eq 120 ]
        then
            break
        fi

        sleep 1
        retries=$(( $retries + 1 ))
done

DNS_STR=`cat /etc/resolv.conf | grep nameserver | grep -v 127.0.0.1`
rpcclient $ATOM_RPC_IP "echo \"$DNS_STR\" > /etc/resolv.conf"
