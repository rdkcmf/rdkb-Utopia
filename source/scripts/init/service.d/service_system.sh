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
# This script restarts the system
#--------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="system"

PID="($$)"

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   # Get all provisioning data

   SYSCFG_FAILED='false'
   FOO=`utctx_cmd get wan_physical_ifname wan_virtual_ifnum lan_ifname`
   eval "$FOO"
   if [ $SYSCFG_FAILED = 'true' ] ; then
      ulog system status "$PID utctx failed to get some configuration data required by service-system"
      ulog system status "$PID THE SYSTEM IS NOT SANE"
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
   if [ "started" != "$STATUS" ] ; then
      ulog system status "$PID system is starting"
      sysevent set ${SERVICE_NAME}-status starting 
      sysevent set ${SERVICE_NAME}-errinfo 
      service_init

      # This script is a reasonable place to figure out the name of our wan interface
      # It should be done earlier than starting lan or wan, and this is plenty early
      # It depends on whether we are using a physical interface, or a vlan interface
      if [ -z "$SYSCFG_wan_virtual_ifnum" ] ; then
         WAN_IFNAME=$SYSCFG_wan_physical_ifname
      else
         WAN_IFNAME=vlan${SYSCFG_wan_virtual_ifnum}
      fi
      sysevent set wan_ifname "$WAN_IFNAME"

      # Start managed service
      sysevent set managed-start

      # Start the network
      sysevent set forwarding-start

      # Start the http daemon
      sysevent set httpd-start

      sysevent set ${SERVICE_NAME}-status started 
      ulog system status "$PID system is started"
   fi
}

#--------------------------------------------------------------
# service_stop
#--------------------------------------------------------------
service_stop ()
{
   ulog system status "$PID system is stopping"
   sysevent set ${SERVICE_NAME}-status stopping 
   echo "system is going down in 5 sec" > /dev/console
   sysevent set lan-stop
   sysevent set bridge-stop
   echo "system is going down in 4 sec" > /dev/console
   sleep 3
   sysevent set wan-stop
   sysevent set managed-stop
   echo "system is going down in 1 sec" > /dev/console
   sleep 1
   echo "system is going down now" > /dev/console
   sysevent set ${SERVICE_NAME}-status stopped 
   ulog system status "$PID system is stopped"
   reboot
}

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------

case "$1" in
   "${SERVICE_NAME}-start")
      service_start
      ;;
   "${SERVICE_NAME}-stop")
      service_stop
      ;;
   "${SERVICE_NAME}-restart")
      service_stop
#      service_start
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac

