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

source /lib/rdk/t2Shared_api.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh

if [ -f /etc/device.properties ]; then
	. /etc/device.properties
fi

SERVICE_NAME="CcspXdnsSsp"

service_restart ()
{
   echo "restarting CcspXdnsSsp, on WAN re/start"
   t2CountNotify "SYS_ERROR_Xdns_restart"
   if [ "$BOX_TYPE" = "XB3" ]; then
          kill -9 `pidof $SERVICE_NAME` > /dev/null 2>&1
	  cd /usr/ccsp/xdns
          /usr/bin/$SERVICE_NAME -subsys eRT.
	  cd - 
    else 
         systemctl stop $SERVICE_NAME.service
	 systemctl start $SERVICE_NAME.service
    fi   
}

# Entry

case "$1" in
  "${SERVICE_NAME}-start")
      service_restart
      ;;
  "${SERVICE_NAME}-stop")
      ;;
  "${SERVICE_NAME}-restart")
      service_restart
      ;;
  resolvconf_updated)
	 if [ "$2" -eq 1 ];then
	         service_restart
	 fi
      ;;
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | wan-status ]" >&2
      exit 3
      ;;
esac

