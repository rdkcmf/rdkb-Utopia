#!/bin/sh

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

RESOLV_CONF=/etc/resolv.conf


#-----------------------------------------------------------------
# set the resolv.conf file
#-----------------------------------------------------------------

prepare_resolv_conf () {
   WAN_DOMAIN=`syscfg get  wan_domain`
   NAMESERVER1=`syscfg get nameserver1`
   NAMESERVER2=`syscfg get nameserver2`
   IPv6_NAMESERVERS=`cat /etc/resolv.conf  | grep nameserver | grep :`
   
   echo -n  > $RESOLV_CONF

   WAN_DNS=
   if [ "" != "$WAN_DOMAIN" ] ; then
      echo "search $WAN_DOMAIN" >> $RESOLV_CONF
   fi
   if [ "0.0.0.0" != "$NAMESERVER1" ] && [ "" != "$NAMESERVER1" ] ; then
      echo "nameserver $NAMESERVER1" >> $RESOLV_CONF
      WAN_DNS=`echo $WAN_DNS $NAMESERVER1`
   fi
   if [ "0.0.0.0" != "$NAMESERVER2" ]  && [ "" != "$NAMESERVER2" ]; then
      echo "nameserver $NAMESERVER2" >> $RESOLV_CONF
      WAN_DNS=`echo $WAN_DNS $NAMESERVER2`
   fi
   if [ "0.0.0.0" != "$NAMESERVER3" ]  && [ "" != "$NAMESERVER3" ]; then
      echo "nameserver $NAMESERVER3" >> $RESOLV_CONF
      WAN_DNS=`echo $WAN_DNS $NAMESERVER3`
   fi

   sysevent set wan_dhcp_dns "${WAN_DNS}"
   sysevent set dhcp_server-restart

   echo $IPv6_NAMESERVERS >> $RESOLV_CONF
}

prepare_resolv_conf

