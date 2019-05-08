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

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

SERVICE_NAME="routed"

case "$1" in
   ${SERVICE_NAME}-start)
      service_routed start
      ;;
   ${SERVICE_NAME}-stop)
      service_routed stop
      ;;
   ${SERVICE_NAME}-restart)
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
           #if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ] ; then
           	#Intel Proposed RDKB Generic Bug Fix from XB6 SDK
            	service_routed radv-restart
           #else
           #	service_routed stop
           #fi
       fi
       ;;
   lan-status)
       status=$(sysevent get lan-status)
       if [ "$status" == "started" ]; then
           service_routed start
       elif [ "$status" == "stopped" ]; then
           # As per Sky requirement, radvd should run with ULA prefix though the wan-status is down
           if [ "x$BOX_TYPE" != "xHUB4" ]; then
               service_routed stop
           fi
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
   ipv6_prefix|ipv6_nameserver)
       service_routed radv-restart
       ;;
   *)
       echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
       exit 3
       ;;
esac

exit 0
