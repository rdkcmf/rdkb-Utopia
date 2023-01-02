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
# This script is called to restart syslog based on system's 
# logging configuration
# 
# $log_level=[0|1|2]
#      0 - disabled 
#      1 - normal logging - default
#      2 - extra logging - log enabled level
#      3 - debug logging
#
# syslog level=0 to 7
#      0 - Emergency
#      1 - Alert
#      2 - Critical
#      3 - Error
#      4 - Warning (maps to 0 - disabled)
#      5 - Notice  (maps to 1 - normal logging - default)
#      6 - Info    (maps to 2 - extra logging  - log enabled level)
#      7 - Debug   (maps to 3 - debug logging)
#
# $log_remote=0|<ip-addr>:[port]
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="syslog"
SELF_NAME="`basename "$0"`"

SOURCE="/etc/utopia/service.d/service_syslog/syslog_rotate_monitor.sh"
TARGET_DIR="/etc/cron/cron.everyminute"
TARGET="${TARGET_DIR}/syslog_rotate_monitor.sh"

service_start()
{
#   killall syslogd


   if [ -z "$SYSCFG_log_level" ] ; then
       SYSCFG_log_level=1
   fi

   if [ -z "$SYSCFG_log_remote" ] ; then
       SYSCFG_log_remote=0
   fi

   case "$SYSCFG_log_level" in
       0) SYSLOG_LEVEL=4 ;;
       1) SYSLOG_LEVEL=5 ;;
       2) SYSLOG_LEVEL=6 ;;
       3) SYSLOG_LEVEL=7 ;;
       *) SYSLOG_LEVEL=5 ;;
   esac

   # busybox syslogd's log level runs from 1 to 8, instead of syslog way of 0 to 7
   # so increment SYSLOG_LEVEL by 1 before using it
   BB_SYSLOG_LEVEL=`expr $SYSLOG_LEVEL + 1`
#   if [ "0" != "$SYSCFG_log_remote" ] ; then
#       /sbin/syslogd -l $BB_SYSLOG_LEVEL -L -R $SYSCFG_log_remote
#   else
#      /sbin/syslogd -l $BB_SYSLOG_LEVEL
#   fi

   if [ "1" = "$USE_SYSEVENT" ] ; then
      sysevent set ${SERVICE_NAME}-status "started"
      sysevent set syslog_rotated
   fi
   if [ ! -f "$TARGET" ] ; then
      mkdir -p $TARGET_DIR
      ln -sf $SOURCE $TARGET
   fi
   #rongwei added
   echo '*.* /var/log/remote.log' > /etc/ti_syslog.conf
   ti_syslogd -r -m 30 -f /etc/ti_syslog.conf -p /none/none
}

service_stop ()
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

   #rongwei added 
   killall ti_syslogd
#   killall syslogd
   rm -f $TARGET

   if [ "1" = "$USE_SYSEVENT" ] ; then
      sysevent set ${SERVICE_NAME}-status "stopped"
   fi
}

# syslog might be started before sysevent, 
# so check if alive before using it
pidof syseventd > /dev/null
if [ $? -eq 0 ] ; then
    USE_SYSEVENT=1
else
    USE_SYSEVENT=0
fi

service_init ()
{
    FOO=`utctx_cmd get log_level log_remote`
    eval "$FOO"
}

#--------------------------------------------------------------------------------
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
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" >&2
      exit 3
      ;;
esac

