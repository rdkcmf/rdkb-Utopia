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
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="sysevent_proxy"
PMON=/etc/utopia/service.d/pmon.sh
PID_FILE="/var/run/syseventd_proxy"
BIN=syseventd_proxy

# This service starts up sysevent daemon proxy.
# The proxy will establish a connection with a remote sysevent daemon
# and exit if the remote daemon becomes unresponsive. 
# procmon will make sure the daemon restarts


#-----------------------------------------------------------------

service_init ()
{
   FOO=`utctx_cmd get local_syseventd bootstrap_syseventd secondary_syseventd`
   eval "$FOO"
   SCRIPT_FILE="/etc/syseventp.conf"
}

#-----------------------------------------------------------------

service_start ()
{

   if [ -n "$SYSCFG_bootstrap_syseventd" -a -n "$SYSCFG_local_syseventd" ]  ; then
      if [ "$SYSCFG_local_syseventd" = "$SYSCFG_bootstrap_syseventd" ] ; then
         # this is the syseventd on the platform containing dns forwarder
         if [ -n "$SYSCFG_secondary_syseventd" ] ; then
            /sbin/syseventd_proxy "$SYSCFG_secondary_syseventd" $SCRIPT_FILE
            $PMON setproc syseventd_proxy $BIN $PID_FILE "/sbin/syseventd_proxy $SYSCFG_secondary_syseventd $SCRIPT_FILE"
         fi
      else
         # every other platform
         # syseventd_proxy monitors connection to remote sysevent daemon and sets sysevent remote_syseventd
         # to progate the connection state
         /sbin/syseventd_proxy "$SYSCFG_bootstrap_syseventd" $SCRIPT_FILE remote_syseventd
         $PMON setproc syseventd_proxy $BIN $PID_FILE "/sbin/syseventd_proxy $SYSCFG_bootstrap_syseventd $SCRIPT_FILE remote_syseventd"
      fi
   fi
   sysevent set ${SERVICE_NAME}-status started
}

service_stop ()
{
   if [ -n "$SYSCFG_bootstrap_syseventd" -a -n "$SYSCFG_local_syseventd" ]  ; then
      if [ "$SYSCFG_local_syseventd" = "$SYSCFG_bootstrap_syseventd" ] ; then
         # this is the syseventd on the platform containing dns forwarder
         if [ -n "$SYSCFG_secondary_syseventd" ] ; then
            $PMON unsetproc syseventd_proxy
            killall -TERM "`cat $PID_FILE`"
            rm $PID_FILE
         fi
      else
         # every other platform
         $PMON unsetproc syseventd_proxy
         killall -TERM "`cat $PID_FILE`"
         rm $PID_FILE
      fi
   fi
   sysevent set ${SERVICE_NAME}-status stopped
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
      service_stop
      service__start
      ;;
   bootstrap_dns-status)
      B_STATUS=`sysevent get bootstrap_dns-status`
      STATUS=`sysevent get "{SERVICE_NAME}"-status`
      if [ "started" = "$B_STATUS" ] ; then
         if [ "started" != "$STATUS" ] ; then
            service_start
         fi
      else
         if [ "started" = "$STATUS" ] ; then
            service_stop
         fi
      fi
      ;;
   *)
      echo "Usage: $SERVICE_NAME [start|stop|restart|lan-status]" >&2
      exit 3
      ;;
esac
