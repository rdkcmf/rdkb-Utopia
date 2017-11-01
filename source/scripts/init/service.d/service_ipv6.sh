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
# name of this service
# This name MUST correspond to the registration of this service.
# Usually the registration occurs in /etc/utopia/registration.d
# The registration code will refer to the path/name of this handler
# to be activated upon default events (and other events)
#------------------------------------------------------------------
SERVICE_NAME="ipv6"



#----------------------------------------------------------------------------------------
#                     Default Event Handlers
#
# Each service has three default events that it should handle
#    ${SERVICE_NAME}-start
#    ${SERVICE_NAME}-stop
#    ${SERVICE_NAME}-restart
#
# For each case:
#   - Clear the service's errinfo
#   - Set the service's status 
#   - Do the work
#   - Check the error code (check_err will set service's status and service's errinfo upon error)
#   - If no error then set the service's status
#----------------------------------------------------------------------------------------
ipv6_notification_start ()
{
    count=0;
    while [ $count -lt 24 ] #2 mins loop 5*24 = 120 secs
    do
        ipv6check=`ifconfig erouter0 | grep "inet6 addr" | grep Global | awk -F ":" '{print $NF}'`
	if [ "$ipv6check" == "Global" ];then
            echo "ipv6 available"
            sysevent set ipv6-notification up
            exit
        else
	    #ipv6 not available, sleep for 5 secs
            sleep 5
        fi
	count=$((count+1))
        if [ $count -eq 24 ];then
            echo "Maximum loop reached, exiting script"
	fi
    done

}
ipv6_notification_stop ()
{
    CURRENT_ipv6_STATUS=`sysevent get ipv6-notification`
    if [ "up" = "$CURRENT_WAN_STATUS" ];then
        sysevent set ipv6-notification down
    fi
}

case "$1" in
   ${SERVICE_NAME}-start)
      service_ipv6 start
      ;;
   ${SERVICE_NAME}-stop)
      service_ipv6 stop
      ;;
   ${SERVICE_NAME}-restart)
      service_ipv6 restart
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

   #currently, leverage this event to represent the enabled lan interfaces
   multinet-instances)
      if [ x`sysevent get ipv6_prefix` != x"" ]; then
          service_ipv6 restart
          service_routed route-unset
          service_routed route-set
      fi
      ;;
   multinet_1-status)
      multinet_status=`sysevent get multinet_1-status`
      if [ "ready" = "$multinet_status" ] ; then
          if [ x`sysevent get ipv6_prefix` != x"" ]; then
              service_ipv6 restart multinet
              if [ "$?" = "0" ] ; then
                  service_routed route-unset
                  service_routed route-set
              fi
          fi
      fi
      ;;
   erouter_topology-mode)
      if [ x`sysevent get ipv6_prefix` != x"" ]; then
          service_ipv6 restart
      fi
      ;;

   ipv6_prefix)
      #service_ipv6 restart
     if [ x`sysevent get ipv6_prefix` != x"" ]; then
         multinet_status=`sysevent get multinet_1-status`
         if [ "ready" = "$multinet_status" ] ; then
           service_ipv6 restart
         fi
     fi
     ;;

  wan-status)
      CURRENT_WAN_STATUS=`sysevent get wan-status`
      CURRENT_LAN_STATUS=`sysevent get lan-status`
      if [ "started" = "$CURRENT_WAN_STATUS" ] ; then
         ipv6_notification_start
      elif [ "stopped" = "$CURRENT_WAN_STATUS" ] ; then
         ipv6_notification_stop
      fi
      ;;
  dhcpv6s-start)
      service_ipv6 dhcpv6s-start
      ;;
  dhcpv6s-stop)
      service_ipv6 dhcpv6s-stop
      ;;
  dhcpv6s-restart)
      service_ipv6 dhcpv6s-restart
      ;;
   *)
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
