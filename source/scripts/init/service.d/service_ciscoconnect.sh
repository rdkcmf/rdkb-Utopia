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

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
source /etc/utopia/service.d/event_handler_functions.sh
source /lib/rdk/t2Shared_api.sh

SERVICE_NAME="ciscoconnect"
CC_PSM_BASE=dmsb.CiscoConnect

cc_preproc () 
{
    eval "`psmcli get -e IP $CC_PSM_BASE.l3net BR $CC_PSM_BASE.l2net ENABLE $CC_PSM_BASE.guestEnabled POOL $CC_PSM_BASE.pool`"
    
    sysevent set ${SERVICE_NAME}_guest_l3net "$IP"
    sysevent set ${SERVICE_NAME}_guest_l2net "$BR"
    sysevent set ${SERVICE_NAME}_guest_pool "$POOL"
    sysevent set ${SERVICE_NAME}_guest_enable "$ENABLE"
    
}

start_guestnet () 
{
    sysevent set ipv4-up "`sysevent get ${SERVICE_NAME}_guest_l3net`"
    sysevent set ciscoconnect-guest_status "started"
}

stop_guestnet () 
{
    sysevent set ipv4-down "`sysevent get ${SERVICE_NAME}_guest_l3net`"
    sysevent set multinet-down "`sysevent get ${SERVICE_NAME}_guest_l2net`"
    echo "service_ciscoconnect : Triggering RDKB_FIREWALL_RESTART"
    t2CountNotify "RF_INFO_RDKB_FIREWALL_RESTART"
    sysevent set firewall-restart
    sysevent set ciscoconnect-guest_status "stopped"
}

service_init ()
{
    echo
}


service_start ()
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 
   
   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
   

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

service_restart () 
{
   service_stop
   service_start
}


service_init

case "$1" in
  "${SERVICE_NAME}-start")
     service_start
     ;;
  "${SERVICE_NAME}-stop")
     service_stop
     ;;
  "${SERVICE_NAME}-restart")
     ENABLED=`psmcli get dmsb.CiscoConnect.guestEnabled`
     sysevent set ${SERVICE_NAME}_guest_enable "$ENABLED"
     
     if [ x"started" = x"`sysevent get ciscoconnect-guest_status`" ]; then
        stop_guestnet
     fi
     
     
     if [ x1 = x"$ENABLED" ]; then
        start_guestnet
     fi
     ;;
  snmp_subagent-status)
     cc_preproc
     ;;

  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart]" >&2
      exit 3
      ;;
esac

