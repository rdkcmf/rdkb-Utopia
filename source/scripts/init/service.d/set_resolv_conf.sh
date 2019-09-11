#!/bin/sh
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

RESOLV_CONF=/etc/resolv.conf
RESOLV_CONF_TMP="/tmp/resolv_tmp.conf"

#-----------------------------------------------------------------
# set the resolv.conf file
#-----------------------------------------------------------------

prepare_resolv_conf () {
   WAN_DOMAIN=`syscfg get  wan_domain`
   NAMESERVER1=`syscfg get nameserver1`
   NAMESERVER2=`syscfg get nameserver2`
  
   cp $RESOLV_CONF $RESOLV_CONF_TMP
 
   if [ "" != "$WAN_DOMAIN" ] ; then
       sed -i '/domain/d' "$RESOLV_CONF_TMP"
   fi

       
       if [[ ( "0.0.0.0" != "$NAMESERVER1"  &&  "" != "$NAMESERVER1" ) || ( "0.0.0.0" != "$NAMESERVER2"  &&  "" != "$NAMESERVER2" ) ]] ; then
           #Removing IPV4 old DNS Config.
           interface=`sysevent get wan_ifname`
           get_dns_number=`sysevent get ipv4_${interface}_dns_number`
           sed -i '/domain/d' "$RESOLV_CONF_TMP"
           sed -i '/nameserver 127.0.0.1/d' "$RESOLV_CONF_TMP"
                if [ "$get_dns_number" != "" ]; then
                        counter=0;
                        while [ $counter -lt $get_dns_number ]; do
                        get_old_dns_server=`sysevent get ipv4_${interface}_dns_$counter`
                        ipv4_dns_server="nameserver $get_old_dns_server"
                        sed -i "/$ipv4_dns_server/d" "$RESOLV_CONF_TMP"
                        let counter=counter+1
                        done
                fi
       fi

   N=""
   while read line; do
   N="${N}$line
"
   done < $RESOLV_CONF_TMP
   echo -n "$N" > "$RESOLV_CONF"
   rm -rf $RESOLV_CONF_TMP

   
     

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

