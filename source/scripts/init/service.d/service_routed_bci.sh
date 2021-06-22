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

#------------------------------------------------------------------
# This script is used to start the routing daemons (zebra and ripd)
# $1 is the calling event (current_wan_state  current_lan_state  ipv6_prefix)
#------------------------------------------------------------------
source /etc/device.properties

SERVICE_NAME="routed"

case "$1" in
   "${SERVICE_NAME}-start")
      service_routed start
      ;;
   "${SERVICE_NAME}-stop")
      service_routed stop
      ;;
   "${SERVICE_NAME}-restart")
      service_routed restart
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------
   wan-status)
       status=$(sysevent get wan-status)
       if [ "$status" == "started" ]; then
           service_routed start
       elif [ "$status" == "stopped" ]; then
           service_routed stop
       fi
       ;;
   lan-status)
       status=$(sysevent get lan-status)
       if [ "$status" == "started" ]; then
           service_routed start
       elif [ "$status" == "stopped" ]; then
           service_routed stop
       fi
       ;;
   ripd-restart)
       service_routed rip-restart
       ;;
   zebra-restart)
       service_routed radv-restart
       ;;
   staticroute-restart)
       service_routed radv-restart
       ;;
#   ipv6_nameserver|ipv6_dnssl)
#       service_routed radv-restart
#       ;;
   ipv6_prefix|ipv6_nameserver)
if [ "$MODEL_NUM" = "DPC3941B" ] || [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "CGA4131COM" ] || [ "$MODEL_NUM" = "CGA4332COM" ]; then
       service_routed radv-restart
fi
       ;;
   dhcpv6_option_changed)
       service_routed radv-restart
       ;;
   *)
       echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
       exit 3
       ;;
esac

exit 0
