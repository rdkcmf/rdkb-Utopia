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

#-----------------------------------------------------------------
# set the resolv.conf file
#-----------------------------------------------------------------

overwrite_wandomain () {
   WAN_STATIC_DOMAIN=`syscfg get wan_domain`
    
   #delete old static domain if exists
   `sed -i '/#overwrite domain name start/,/#overwrite domain name end/d' $RESOLV_CONF`

   #append wan_domain to resolv.conf
   #The search keywords are mutually exclusive. If more than one instance of these keywords is present, the last instance wins
   if [ -n "$WAN_STATIC_DOMAIN " ]; then
       echo "#overwrite domain name start" >> $RESOLV_CONF
       echo "search $WAN_STATIC_DOMAIN" >> $RESOLV_CONF
       echo "#overwrite domain name end" >> $RESOLV_CONF
       sysevent set dhcp_domain "$WAN_STATIC_DOMAIN"
       sysevent set dhcp_server-restart
   fi
}

overwrite_wandomain 
