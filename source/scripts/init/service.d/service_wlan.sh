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


source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="wlan";
SYSCFG_lan_wl_physical_ifnames=`syscfg get lan_wl_physical_ifnames`;

service_stop ()
{
   wait_till_end_state ${SERVICE_NAME}

   STATUS=`sysevent get ${SERVICE_NAME}-status` 
   if [ "stopped" != "$STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status stopping

      if [ -n "$SYSCFG_lan_wl_physical_ifnames" ] ; then
         for loop in $SYSCFG_lan_wl_physical_ifnames
         do
            ulog lan status "wlancfg $loop down"
            wlancfg "$loop" down
            ip link set "$loop" "$down"
         done
      fi

      ulog lan status "killing wireless daemons"
      killall -q wps_monitor
      killall -q eapd
      killall -q nas

      # turn off the WiFi LED
      echo set value 17 1 > /proc/cns3xxx/gpio

      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status stopped
   fi
}

service_restart ()
{
   wait_till_end_state ${SERVICE_NAME}

   # WLAN service can restart at any state.
#   STATUS=`sysevent get ${SERVICE_NAME}-status` 
#   if [ "stopped" != "$STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status starting

      if [ -n "$SYSCFG_lan_wl_physical_ifnames" ] ; then
         WIFI_IF_INDEX=1
         for loop in $SYSCFG_lan_wl_physical_ifnames
         do
            WL_STATE=`syscfg get wl$(($WIFI_IF_INDEX-1))_state`
            ulog lan status "wlancfg $loop $WL_STATE"
            wlancfg "$loop" "$WL_STATE"
            ip link set "$loop" "$WL_STATE"
            WIFI_IF_INDEX=`expr $WIFI_IF_INDEX + 1`
         done
      fi

      ulog lan status "restarting wireless daemons"
      killall -q wps_monitor
      killall -q eapd
      killall -q nas
      eapd
      nas
      wps_monitor &
 
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status started
#   fi
}

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------

case "$1" in
   "${SERVICE_NAME}-stop")
      service_stop
      ;;
   "${SERVICE_NAME}-restart")
      sysevent set wlan-restarting 1
      service_restart
      sysevent set wlan-restarting 0
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME}.sh [ ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart ]" > /dev/console
      exit 3
      ;;
esac
