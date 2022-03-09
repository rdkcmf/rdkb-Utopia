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

HOSTS_FILE=/etc/hosts
HOSTNAME_FILE=/etc/hostname
source /etc/device.properties

#-----------------------------------------------------------------
# set the hostname files
#-----------------------------------------------------------------
prepare_hostname () {
   HOSTNAME=`syscfg get hostname`
   LAN_IPADDR=`sysevent get current_lan_ipaddr`
   SYSEVT_lan_ipaddr_v6=`sysevent get lan_ipaddr_v6`
   LOCDOMAIN_NAME=`syscfg get SecureWebUI_LocalFqdn`
   SECUREWEBUI_ENABLED=`syscfg get SecureWebUI_Enable`

   if [ "" != "$HOSTNAME" ] ; then
      if [ "$MODEL_NUM" == "PX5001B" ] && [ "$SECUREWEBUI_ENABLED" == "true" ]; then
          if [[ $HOSTNAME != *-bci* ]] ; then
              HOSTNAME=$HOSTNAME"-bci"
              syscfg set hostname "$HOSTNAME"
              syscfg commit
          fi
      fi
      echo "$HOSTNAME" > $HOSTNAME_FILE
      hostname "$HOSTNAME"
   fi
       
   if [ "" != "$HOSTNAME" ] ; then
      echo "$LAN_IPADDR     $HOSTNAME" > $HOSTS_FILE
   else
      echo -n > $HOSTS_FILE
   fi
       
   echo "127.0.0.1       localhost" >> $HOSTS_FILE
   echo "::1             localhost" >> $HOSTS_FILE
   if [ "$SECUREWEBUI_ENABLED" = "true" ]; then
       if [ ! -z "$LOCDOMAIN_NAME" ]; then
           if [ ! -z "$LAN_IPADDR" ]; then
               echo "$LAN_IPADDR""         ""$LOCDOMAIN_NAME"  >> $HOSTS_FILE
           fi
           if [ ! -z "$SYSEVT_lan_ipaddr_v6" ]; then
               echo "$SYSEVT_lan_ipaddr_v6""         ""$LOCDOMAIN_NAME"  >> $HOSTS_FILE
           fi
       fi
   fi

   # The following lines are desirable for IPv6 capable hosts
   echo "::1             ip6-localhost ip6-loopback" >> $HOSTS_FILE
   echo "fe00::0         ip6-localnet" >> $HOSTS_FILE
   echo "ff00::0         ip6-mcastprefix" >> $HOSTS_FILE
   echo "ff02::1         ip6-allnodes" >> $HOSTS_FILE
   echo "ff02::2         ip6-allrouters" >> $HOSTS_FILE
   echo "ff02::3         ip6-allhosts" >> $HOSTS_FILE
}

