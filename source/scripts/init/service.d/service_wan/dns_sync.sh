#!/bin/sh
# This script will push the DNS from ARM to ATOM.
#------------------------------------------------
TMP_RESOLV_FILE=/tmp/resolv.conf
RESOLV_CONF=/etc/resolv.conf
ATOM_USER_NAME=root
retries=0

ATOM_INTERFACE_IP=`cat /etc/device.properties | grep ATOM_INTERFACE_IP | cut -f 2 -d"="`
LAN_IP=`syscfg get lan_ipaddr`
PEER_COMM_DAT="/etc/dropbear/elxrretyt.swr"
PEER_COMM_ID="/tmp/elxrretyt-$$.swr"
CONFIGPARAMGEN="/usr/bin/configparamgen"

while :
do
        numOfEntries=`cat $RESOLV_CONF | grep nameserver | grep -v 127.0.0.1 | wc -l`
        if [ $numOfEntries -gt 2 -o $retries -eq 120 ]
        then
            break
        fi

        sleep 5
        retries=$(( $retries + 1 ))
done

# Synch the available DNS entries and wait until all entries are available.
DNS_STR=`cat $RESOLV_CONF | grep nameserver | grep -v 127.0.0.1`
echo "$DNS_STR" > $TMP_RESOLV_FILE

DNS_STR_V4=`cat $RESOLV_CONF | grep nameserver | grep -v 127.0.0.1 | grep "\." | cut -d" " -f2`
if [ "$DNS_STR_V4" == "" ]
then
	echo "No IPv4 DNS entries available add default as lan IP"
	echo "$LAN_IP" >> $TMP_RESOLV_FILE
fi

$CONFIGPARAMGEN jx $PEER_COMM_DAT $PEER_COMM_ID
scp -i $PEER_COMM_ID $TMP_RESOLV_FILE $ATOM_USER_NAME@$ATOM_INTERFACE_IP:$RESOLV_CONF > /dev/null 2>&1

if [ $? -eq 0 ]; then
	echo "scp is successful at first instance"
else
	retries=0
	while :
	do
		scp -i $PEER_COMM_ID $TMP_RESOLV_FILE $ATOM_USER_NAME@$ATOM_INTERFACE_IP:$RESOLV_CONF > /dev/null 2>&1
		if [ $? -eq 0 -o  $retries -gt 4 ]
		then
		    if [ $? -eq 0 ]
		    then
			    echo "scp is successful at iteration:$retries"
			else
			    echo "scp of resolv.conf failed."
			fi 
			break	
		else
			sleep 5
			retries=$(( $retries + 1 ))
		fi 
	done	
fi

rm -f $PEER_COMM_ID
