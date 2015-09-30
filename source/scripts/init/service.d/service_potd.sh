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
# Copyright (c) 2009 by Cisco Systems, Inc. All Rights Reserved.
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
# This script is used to update mso potd (password of the day) 
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="potd"
SELF_NAME="`basename $0`"

POTD=/usr/sbin/sa_potd

service_start() {
    ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service"
    #
    # Chances are when we run potd service, system time has not yet got from ToD
    #
#for i in $(seq 300)
#do
	YEAR=`date +%Y`
	if [ "$YEAR" = "1970" ]; then
		sleep 60
	fi
#done
    killall sa_potd
    $POTD &
    #MSO_PASSWD=`$POTD`
    #if [ "$MSO_PASSWD" != "" ]; then
    #  syscfg set user_password_1 $MSO_PASSWD
    #fi
    
    sysevent set ${SERVICE_NAME}-status "started"
}

service_stop() {
    ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
    sysevent set ${SERVICE_NAME}-status "stopped"
}

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
  wan-status)
      WAN_STATUS=`sysevent get wan-status`
      if [ "started" = "$WAN_STATUS" ] ; then
         service_start
      fi
      ;;
  phylink_wan_state)
      WAN_LINK_STATUS=`sysevent get phylink_wan_state`
      if [ "up" == "$WAN_LINK_STATUS" ] ; then
         service_start
      fi
      ;;
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart|wan-status|phylink_wan_state]" >&2
      exit 3
      ;;
esac
