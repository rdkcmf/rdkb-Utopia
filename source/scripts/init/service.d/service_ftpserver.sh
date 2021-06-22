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
# This script is used to start ftp daemon
# $1 is the calling event (ftpserver-restart, lan-status, wan-status, etc)
#------------------------------------------------------------------
source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="ftpserver"
SELF_NAME="`basename "$0"`"

service_init ()
{
    eval "`utctx_cmd get current_lan_ipaddr`"
    LAN_IPADDR=$SYSCFG_current_lan_ipaddr
}

service_start()
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   DIR_NAME=/tmp/ftp
   if [ ! -d $DIR_NAME ] ; then
      # provide a default directory
      mkdir -p $DIR_NAME
      chown admin $DIR_NAME
      chgrp admin $DIR_NAME
      chmod 755 $DIR_NAME
   fi

   # start a ftp daemon
   # echo "[utopia] Starting FTP daemon" > /dev/console
   tinyftp -d -s "$LAN_IPADDR" -p 21 -c $DIR_NAME &

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () {
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

   killall tinyftp

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "started"
}

service_lanwan_status ()
{
      CURRENT_LAN_STATE=`sysevent get lan-status`
      CURRENT_WAN_STATE=`sysevent get wan-status`
      if [ "stopped" = "$CURRENT_LAN_STATE" ] && [ "stopped" == "$CURRENT_WAN_STATE" ] ; then
         service_stop
      else
         service_start
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
  lan-status)
      service_lanwan_status
      ;;
  wan-status)
      service_lanwan_status
      ;;
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart|lan-status|wan-status]" >&2
      exit 3
      ;;
esac

