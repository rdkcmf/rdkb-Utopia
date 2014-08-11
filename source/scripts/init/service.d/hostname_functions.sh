#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

HOSTS_FILE=/etc/hosts
HOSTNAME_FILE=/etc/hostname

#-----------------------------------------------------------------
# set the hostname files
#-----------------------------------------------------------------
prepare_hostname () {
   HOSTNAME=`syscfg get hostname`
   LAN_IPADDR=`sysevent get current_lan_ipaddr`

   if [ "" != "$HOSTNAME" ] ; then
      echo "$HOSTNAME" > $HOSTNAME_FILE
      hostname $HOSTNAME
   fi

   if [ "" != "$HOSTNAME" ] ; then
      echo "$LAN_IPADDR     $HOSTNAME" > $HOSTS_FILE
   else
      echo -n > $HOSTS_FILE
   fi
   echo "127.0.0.1       localhost" >> $HOSTS_FILE
   echo "::1             localhost" >> $HOSTS_FILE

   # The following lines are desirable for IPv6 capable hosts
   echo "::1             ip6-localhost ip6-loopback" >> $HOSTS_FILE
   echo "fe00::0         ip6-localnet" >> $HOSTS_FILE
   echo "ff00::0         ip6-mcastprefix" >> $HOSTS_FILE
   echo "ff02::1         ip6-allnodes" >> $HOSTS_FILE
   echo "ff02::2         ip6-allrouters" >> $HOSTS_FILE
   echo "ff02::3         ip6-allhosts" >> $HOSTS_FILE
}

