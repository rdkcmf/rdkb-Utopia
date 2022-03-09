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
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="mcastsnooper"
SELF_NAME="`basename "$0"`"

do_stop_igmp_snooper() {
   if [ -f /var/run/igmp_snooper.pid ]; then
      LAST_SESSION_PID=`cat /var/run/igmp_snooper.pid`
      if [ -n "$LAST_SESSION_PID" ]; then
         kill "$LAST_SESSION_PID"
      fi
      rm /var/run/igmp_snooper.pid
   fi
}

do_start_igmp_snooper () {

   if [ -n "$SYSCFG_lan_sw_unit" ]; then
      sw_opt="-u $SYSCFG_lan_sw_unit"
   fi

   if [ -n "$SYSCFG_lan_ifname" ]; then
      if_opt="-i $SYSCFG_lan_ifname"
   fi

   if [ -n "$SYSCFG_lan_sw_np_port" ]; then
      querier_opt="-q $SYSCFG_lan_sw_np_port"
   fi

   do_stop_igmp_snooper

   igmp_snooper "$sw_opt" "$if_opt" "$querier_opt" -v &
   echo $! > /var/run/igmp_snooper.pid
}

service_init ()
{
   eval `utctx_cmd get mcastsnooper_enabled lan_sw_unit lan_ifname lan_sw_np_port`
#   WAN_IFNAME=`sysevent get current_wan_ifname`
}

service_start () 
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`

   if [ "started" != "$STATUS" ] ; then

      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status starting

      do_start_igmp_snooper

      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status started

   fi
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`

   if [ "stopped" != "$STATUS" ] ; then

      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status stopping

      do_stop_igmp_snooper

      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status stopped

   fi
}

# Entry

service_init

case "$1" in
  "${SERVICE_NAME}-start")
      service_start
      ;;
  "${SERVICE_NAME}-stop")
      service_stop
      ;;
  "${SERVICE_NAME}-restart")
      service_stop
      service_start
      ;;
  wan-status)
      # do nothing for now
      ;;
  lan-status)
      CURRENT_LAN_STATUS=`sysevent get lan-status`
      if [ "started" = "$CURRENT_LAN_STATUS" ] && [ "1" == "$SYSCFG_mcastsnooper_enabled" ] ; then
         service_start
      elif [ "stopped" = "$CURRENT_LAN_STATUS" ] ; then
         service_stop 
      fi
      ;;
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | lan-status ]" >&2
      exit 3
      ;;
esac
