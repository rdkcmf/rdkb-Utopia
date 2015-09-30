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
# Copyright (c) 2010 by Cisco Systems, Inc. All Rights Reserved.
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
# This script restarts the forwarding system (router or bridge)
#--------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
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

      service_init

      # if we are in bridge mode then make sure that the wan, lan are down
      if [ "1" = "$bridge_mode" ] || [ "2" = "$bridge_mode" ]; then
        STATUS=`sysevent get wan-status`
        if [ "stopped" != "$STATUS" ] ; then
           ulog forwarding status "stopping wan"
           sysevent set wan-stop
           PAUSE=$(($PAUSE+1))
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
      fi

      # Start the network

      # Usually we start up in router mode, but if bridge_mode is set them we start in bridge mode instead
      if [ "1" = "$bridge_mode" ] || [ "2" = "$bridge_mode" ] ; then
         ulog forwarding status "starting bridge"
         sysevent set bridge-start
         # just in case the firewall is still configured for router mode, restart it
         sysevent set firewall-restart
         wait_till_state bridge starting
      else
         ulog forwarding status "starting wan"
         sysevent set wan-start
         STATUS=`sysevent get lan-status`
         if [ "started" != "$STATUS" ] ; then
            ulog forwarding status "starting lan"
            sysevent set lan-start
         fi
         STATUS=`sysevent get firewall-status`
         if [ "stopped" = "$STATUS" ] ; then
            ulog forwarding status "starting firewall"
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
      service_stop
      service_start
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac

