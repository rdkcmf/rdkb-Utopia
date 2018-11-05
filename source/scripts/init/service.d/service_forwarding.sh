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
# This script restarts the forwarding system (router or bridge)
#--------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="forwarding"

PID="($$)"

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   SYSCFG_FAILED='false'
   FOO=`utctx_cmd get bridge_mode`
   eval $FOO
   if [ $SYSCFG_FAILED = 'true' ] ; then
      ulog forwarding status "$PID utctx failed to get some configuration data required by service-forwarding"
      ulog forwarding status "$PID THE SYSTEM IS NOT SANE"
      echo "[utopia] utctx failed to get some configuration data required by service-system" > /dev/console
      echo "[utopia] THE SYSTEM IS NOT SANE" > /dev/console
      sysevent set ${SERVICE_NAME}-status error
      sysevent set ${SERVICE_NAME}-errinfo "Unable to get crucial information from syscfg"
      exit
   fi
}

#--------------------------------------------------------------
# service_start
#--------------------------------------------------------------
service_start ()
{
   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`
   bridge_mode=`sysevent get bridge_mode`
   PAUSE=0

   if [ "started" != "$STATUS" ] ; then
      ulog forwarding status "$PID forwarding is starting"
      sysevent set ${SERVICE_NAME}-status starting 
      sysevent set ${SERVICE_NAME}-errinfo 
	  LANRESTART_STATUS=`sysevent get lan_restarted`
      service_init

      # if we are in bridge mode then make sure that the wan, lan are down
      if [ "1" = "$bridge_mode" ] || [ "2" = "$bridge_mode" ] || [ "3" = "$bridge_mode" ]; then
        if [ "2" != "$bridge_mode" ] && [ "3" != "$bridge_mode" ]; then 
            STATUS=`sysevent get wan-status`
            if [ "stopped" != "$STATUS" ] ; then
                ulog forwarding status "stopping wan"
                sysevent set wan-stop
                PAUSE=$(($PAUSE+1))
            fi
        fi
        #STATUS=`sysevent get lan-status`
        #if [ "stopped" != "$STATUS" ] ; then
           ulog forwarding status "stopping lan"
           sysevent set lan-stop
           PAUSE=$(($PAUSE+1))
        #fi
        if [ $PAUSE -gt 0 ] ; then
           sleep $PAUSE
           wait_till_state wan stopped
           wait_till_state lan stopped
        fi
      else 
      # if we are in router mode then make sure that the bridge is down
        STATUS=`sysevent get bridge-status`
        if [ "stopped" != "$STATUS" ] ; then
           ulog forwarding status "stopping bridge"
           sysevent set bridge-stop
           sleep 1
           wait_till_state bridge stopped
        fi
		#router mode handle
		if [ "0" = "$bridge_mode" ]; then 
            STATUS=`sysevent get wan-status`
            if [ "stopped" != "$STATUS" ] ; then
                ulog forwarding status "stopping wan"
                sysevent set wan-stop
                wait_till_state wan stopped
            fi
	    
	    if [ "true" = "$LANRESTART_STATUS" ] ; then
		BREAK_LOOP=0
	        BREAK_COUNT=0
   		while [ $BREAK_LOOP -eq 0 ]
   		do
        	LAN_STATUS_FWD=`sysevent get lan-status`
        	if [ "$LAN_STATUS_FWD" = "stopped" ] || [ "$BREAK_COUNT" -gt 10 ] ; then
                	BREAK_LOOP=1
        	else
                	sleep 2
        	fi
        	BREAK_COUNT=$((BREAK_COUNT+1))
   	    	done

            fi
        fi
      fi

      # Start the network

      # Usually we start up in router mode, but if bridge_mode is set them we start in bridge mode instead
      if [ "1" = "$bridge_mode" ] || [ "2" = "$bridge_mode" ] || [ "3" = "$bridge_mode" ]; then
         ulog forwarding status "starting bridge"
         sysevent set bridge-start
         STATUS=`sysevent get wan-status`
         if [ "2" = "$bridge_mode" ] || [ "3" = "$bridge_mode" ]; then 
            if [ "started" != "$STATUS" ] ; then
                ulog forwarding status "starting wan"
                sysevent set wan-start
            fi
         fi
         # just in case the firewall is still configured for router mode, restart it

         echo "service_forwarding : Triggering RDKB_FIREWALL_RESTART before bridge starting"
         sysevent set firewall-restart
         wait_till_state bridge starting
         MANUFACTURE=`cat /etc/device.properties | grep MANUFACTURE | cut -f2 -d=`

         if [ "$MANUFACTURE" = "Technicolor" ]; then	
	     echo "service_fowarding : Start webgui.sh"
	     sh /etc/webgui.sh
         fi
      else
         ulog forwarding status "starting wan"
         sysevent set wan-start
         STATUS=`sysevent get lan-status`
         if [ "started" != "$STATUS" ] || [ "true" = $LANRESTART_STATUS ] ; then
            ulog forwarding status "starting lan"
            sysevent set lan-start
         fi
         STATUS=`sysevent get firewall-status`
         if [ "stopped" = "$STATUS" ] ; then
            ulog forwarding status "starting firewall"
            echo "service_forwarding : Triggering RDKB_FIREWALL_RESTART before lan_wan starting"
            sysevent set firewall-restart
         fi
         wait_till_state lan starting
         wait_till_state wan starting
      fi

      sysevent set ${SERVICE_NAME}-status started 
      ulog forwarding status "$PID forwarding is started"
   fi
}

#--------------------------------------------------------------
# service_stop
#--------------------------------------------------------------
service_stop ()
{
   # NOTE we only stop wan or bridging NOT lan
   ulog forwarding status "$PID wan/bridge is stopping"
   sysevent set ${SERVICE_NAME}-status stopping 
   sysevent set bridge-stop
   sysevent set wan-stop
   sleep 2
   wait_till_state bridge stopped
   wait_till_state wan stopped

   BREAK_LOOP=0
   BREAK_COUNT=0


   while [ $BREAK_LOOP -eq 0 ]
   do
   	LAN_STATUS_FWD=`sysevent get lan-status`


   	if [ "$LAN_STATUS_FWD" = "stopped" ] || [ "$BREAK_COUNT" -gt 10 ] ; then

		BREAK_LOOP=1
	else
		sleep 2
	fi
	BREAK_COUNT=$((BREAK_COUNT+1))

   done

   sysevent set ${SERVICE_NAME}-status stopped
}

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------

case "$1" in
   ${SERVICE_NAME}-start)
      service_start
      ;;
   ${SERVICE_NAME}-stop)
      service_stop
      ;;
   ${SERVICE_NAME}-restart)
#      service_stop
      sysevent set ${SERVICE_NAME}-status restarting
      service_start
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac

