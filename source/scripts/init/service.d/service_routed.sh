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
# Copyright (c) 2009,2010 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

#------------------------------------------------------------------
# This script is used to start the routing daemons (zebra and ripd)
# $1 is the calling event (current_wan_state  current_lan_state  ipv6_prefix)
#------------------------------------------------------------------

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
   ipv6_prefix|ipv6_nameserver)
       service_routed radv-restart
       ;;
   *)
       echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
       exit 3
       ;;
esac

exit 0
