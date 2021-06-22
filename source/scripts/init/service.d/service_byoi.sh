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
#             handler_template.sh 
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh
source /etc/utopia/service.d/service_managed/hsd_mode_functions.sh
source /etc/utopia/service.d/ip_functions.sh

#------------------------------------------------------------------
# name of this service
# This name MUST correspond to the registration of this service.
# Usually the registration occurs in /etc/utopia/registration.d
# The registration code will refer to the path/name of this handler
# to be activated upon default events (and other events)
#------------------------------------------------------------------
SERVICE_NAME="byoi"
BYOI_BRIDGE_SERVICE="bridge"
byoi_bridge_mode=`sysevent get byoi_bridge_mode`

#Main handler functions--------------------------------------------
handle_bridge_ipv4_ipaddr_change () {

   if [ x"0" = x"$SYSCFG_byoi_enabled" ] || [ x"1" != x"$byoi_bridge_mode" ] || [ x"0" != x"$SYSCFG_bridge_mode" ]; then
      exit 0
   fi

   stop_dhcp_proxy

   bridge_ipv4_ipaddr=`sysevent get bridge_ipv4_ipaddr`
   if [ x"0.0.0.0" = x"$bridge_ipv4_ipaddr" ]; then
      exit 0
   fi

   is_private "$bridge_ipv4_ipaddr"

   if [ "0" = $? ]; then
      ulog byoi info "Public ip detected on wan interface. Switching to router mode" 
      #KILL DHCP PROXY AND REMOVE EBTABLES RULES
      exiting_byoi_bridge_mode
      sysevent set byoi_bridge_mode 0
      sysevent set forwarding-restart
   else
      ulog byoi info "Private ip detected on bridge interface. Starting dhcp proxy" 
      start_dhcp_proxy "$bridge_ipv4_ipaddr"
   fi
}

handle_current_wan_ipaddr_change () {
   current_hsd_mode=`sysevent get current_hsd_mode`
   current_wan_ipaddr=`sysevent get current_wan_ipaddr`
   echo "current wan ipaddr change. $current_hsd_mode : $current_wan_ipaddr---------------"
   if [ "1" != "$SYSCFG_byoi_enabled" ] || [ "1" != "$SYSCFG_autobridge_enabled" ] || [ "dhcp" != "$SYSCFG_wan_proto" ]; then
      return 0
   fi

   if [ x"byoi" != x"$current_hsd_mode" ] ; then
      sysevent set dhcp_server-restart
      return 0
   fi

   is_private "$current_wan_ipaddr"

   if [ 1 = $? ] ; then
      echo "Private lan ip detected on wan interface. switching to byoi bridge mode" 
      ulog byoi info "Private lan ip detected on wan interface. switching to byoi bridge mode" 
      sysevent set byoi_bridge_mode 1
      sysevent set forwarding-restart
      entering_byoi_bridge_mode
      sleep 1
      wait_till_end_state forwarding
      wait_till_end_state bridge
      #LAUNCH DHCP PROXY AND SETUP EBTABLES RULES
   else
      sysevent set dhcp_server-restart
   fi

   echo "exiting current_wan_ipaddr handler-----------------"
}

handle_current_hsd_mode () {
   current_hsd_mode=`sysevent get current_hsd_mode`
   echo "handle current hsd called------------------"
   
   if [ x"unknown" = x"$current_hsd_mode" ]; then
#      sysevent set wan-restart
      echo "returning out of current_hsd_mode, it is unknown" > /dev/console
      return
   fi

   wan_status=`sysevent get wan-status`
   bridge_status=`sysevent get bridge-status`
   if [ x"`sysevent get operating_byoi_mode`" = x"$current_hsd_mode" ]; then
      if [ x"started" = x"$wan_status" ] || [ x"started" = x"$bridge_status" ]; then
         echo "returning out of current_hsd_mode due to match with operating_byoi_mode" > /dev/console
         return
      fi
   fi

#   if [ "byoi" != $current_hsd_mode ] && [ "1" = $byoi_bridge_mode ]; then
#      exiting_byoi_bridge_mode
#      sysevent set byoi_bridge_mode 0
#      sysevent set forwarding-restart
#      return
#   fi

   wan_restarting=`sysevent get wan-restarting`

   echo "current_hsd_mode changed, ws:$wan_status; wr: $wan_restarting -----------------" > /dev/console
   if [ x"stopping" != x"$wan_status" -o x"1" != x"$wan_restarting" ] ; then
      echo "Restarting wan" > /dev/console
      sysevent set wan-restart
   fi

   sysevent set operating_byoi_mode "$current_hsd_mode"
   echo "exiting current_hsd_mode handler ------------------"
}

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

#-------------------------------------------------------------------------------------------
#  function   : service_init
#  - optional procedure to retrieve syscfg configuration data using utctx_cmd
#    this is a protected way of accessing syscfg
#-------------------------------------------------------------------------------------------
service_init ()
{
    STATUS=`sysevent get ${SERVICE_NAME}-status`
    
    FOO=`utctx_cmd get byoi_enabled bridge_mode primary_wan_proto last_configured_hsd_mode last_provisioned_hsd_mode autobridge_enabled wan_proto`

    eval "$FOO"
}

#-------------------------------------------------------------------------------------------
#  function   : service_start
#  - Set service-status to starting
#  - Add code to read normalized configuration data from syscfg and/or sysevent 
#  - Create configuration files for the service
#  - Start any service processes 
#  - Set service-status to started
#
#  check_err will check for a non zero return code of the last called function
#  set the service-status to error, and set the service-errinfo, and then exit
#-------------------------------------------------------------------------------------------
service_start ()
{
   # wait_till_end_state will wait a reasonable amount of time waiting for ${SERVICE_NAME}
   # to finish transitional states (stopping | starting)
   wait_till_end_state ${SERVICE_NAME}

   if [ "started" != "$STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-status starting
      sysevent set ${SERVICE_NAME}-errinfo 

      validate_hsd_mode
      sysevent set desired_hsd_mode "$desired_hsd_mode"

#-----------old code----------
#      primary_HSD_allowed=`sysevent get primary_HSD_allowed`
#      
#      if [ -z "$current_hsd_mode" ] || [ "unknown" = "$current_hsd_mode" ]; then
#          if [ x"$HSD_ENABLED" = x"$primary_HSD_allowed" ] ; then
#             sysevent set desired_hsd_mode primary
#          else
#             if [ -z "$primary_HSD_allowed" ]; then
#                sysevent set primary_HSD_allowed $HSD_USR_SELECT
#             else
#                if [ -z "$last_configured_hsd_mode" ] || [ x"$HSD_DISABLED" = x"$primary_HSD_allowed" -a x"primary" = x"$last_configured_hsd_mode" ] ; then
#                   last_configured_hsd_mode="byoi"
##                   syscfg set last_configured_hsd_mode "byoi"
##                   syscfg commit
#                fi
#                sysevent set desired_hsd_mode $last_configured_hsd_mode
#             fi
#          fi
#      fi

      sysevent set ${SERVICE_NAME}-status started
   fi

}

#-------------------------------------------------------------------------------------------
#  function   : service_stop
#  - Set service-status to stopping
#  - Stop any service processes 
#  - Delete configuration files for the service
#  - Set service-status to stopped
#
#  check_err will check for a non zero return code of the last called function
#  set the service-status to error, and set the service-errinfo, and then exit
#-------------------------------------------------------------------------------------------
service_stop ()
{
   # wait_till_end_state will wait a reasonable amount of time waiting for ${SERVICE_NAME}
   # to finish transitional states (stopping | starting)
   wait_till_end_state ${SERVICE_NAME}

   
   if [ "stopped" != "$STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-errinfo 
      sysevent set ${SERVICE_NAME}-status stopping
      #stop timer daemon********************************

      sysevent set ${SERVICE_NAME}-status stopped
   fi
}

#-------------------------------------------------------------------------------------------
# Entry
# The first parameter is the name of the event that caused this handler to be activated
#-------------------------------------------------------------------------------------------

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
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

   desired_hsd_mode)
#      if [ "started" = "${STATUS}" ]; then
         handle_desired_hsd_mode
#      fi
   ;;

   retry_desired_hsd_mode)
         handle_desired_hsd_mode
   ;;

   current_hsd_mode)
      if [ "started" = "${STATUS}" ]; then
         handle_current_hsd_mode
      fi
   ;;

   current_wan_ipaddr)
      if [ "started" = "${STATUS}" ]; then
         handle_current_wan_ipaddr_change
      fi
   ;;

   bridge_ipv4_ipaddr)
      if [ "started" = "${STATUS}" ]; then
         handle_bridge_ipv4_ipaddr_change
      fi
   ;;

   primary_HSD_allowed)
#      if [ "started" = "${STATUS}" ]; then
         handle_primary_HSD_allowed_change
#      fi
   ;;

   system-start)
      if [ "1" = "$SYSCFG_byoi_enabled" ]; then
         service_start
      fi
   ;;

   *)
      echo "$SERVICE_NAME called with \"$1\"" > /dev/console
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
