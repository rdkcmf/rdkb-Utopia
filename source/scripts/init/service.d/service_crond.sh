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


# ----------------------------------------------------------------------------
# This script prepares cron files and brings up a crond
# The prepared configuration files will have the cron daemon
# execute all files in a well-known directory at predictables times.
# 
# Also this script creates a script to be run by the cron daemon on 
# a daily basis. The purpose of that script will be to trigger the
# ddns-start event
# ----------------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="crond"
SELF_NAME="`basename $0`"

register_docsis_init_handler () 
{
    ID=`sysevent get crond_docsis_async`
    if [ x = x"$ID" ] ; then
       ID=`sysevent async docsis-initialized /etc/utopia/service.d/service_crond.sh`
       sysevent set crond_docsis_async "$ID"
    fi
}

service_start () 
{
   if [ "x1" != "x`sysevent get docsis-initialized`" ]; then
     echo "SERVICE_CROND : register_docsis_init_handler"
     register_docsis_init_handler
      return
   fi
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   killall crond
   
   CRONTAB_DIR="/var/spool/cron/crontabs/"
   CRONTAB_FILE=$CRONTAB_DIR"root"
   if [ ! -e $CRONTAB_FILE ] ; then
      # make a pseudo random seed from our mac address
      # we will get the same values of random over reboots
      # but there will be divergence of values accross hosts
      # which is the property we are looking for
      INT=wan0
      OUR_MAC=`ip link show $INT | grep link | awk '{print $2}'`
      MAC1=`echo $OUR_MAC | awk 'BEGIN { FS = ":" } ; { printf ("%d", "0x"$6) }'`
      MAC2=`echo $OUR_MAC | awk 'BEGIN { FS = ":" } ; { printf ("%d", "0x"$5) }'`
      RANDOM=`expr $MAC1 \* $MAC2`
   
      # prepare busybox crontab directory
      # echo "[utopia][registration] Preparing crond directory"
      mkdir -p /etc/cron/cron.everyminute
      mkdir -p /etc/cron/cron.every5minute
      mkdir -p /etc/cron/cron.every10minute
      mkdir -p /etc/cron/cron.hourly
      mkdir -p /etc/cron/cron.daily
      mkdir -p /etc/cron/cron.weekly
      mkdir -p /etc/cron/cron.monthly
      mkdir -p $CRONTAB_DIR
   
      echo "* * * * *  execute_dir /etc/cron/cron.everyminute" > $CRONTAB_FILE
      echo "1,6,11,16,21,26,31,36,41,46,51,56 * * * *  execute_dir /etc/cron/cron.every5minute" >> $CRONTAB_FILE
      echo "2,12,22,32,42,52 * * * *  execute_dir /etc/cron/cron.every10minute" >> $CRONTAB_FILE
      num1=$RANDOM
      rand1=`expr $num1 % 60`
      echo "$rand1 * * * * execute_dir /etc/cron/cron.hourly" >> $CRONTAB_FILE
      echo "1 */6 * * *  /rdklogger/rxtx100.sh" >> $CRONTAB_FILE
      echo "10 */6 * * *  /usr/ccsp/tad/getSsidNames.sh" >> $CRONTAB_FILE
#rdkb-4297 Runs on the 1st minute of every 12th hour
      echo "1 */12 * * *  /usr/ccsp/pam/moca_status.sh" >> $CRONTAB_FILE

      #zqiu: monitor lan client traffic
      echo "* * * * *   /usr/ccsp/tad/rxtx_lan.sh" >> $CRONTAB_FILE

      echo "1 */6 * * *   /usr/ccsp/tad/monitor_process_info.sh" >> $CRONTAB_FILE
#RDKB-9367, file handle monitor, needs to be run every 12 hours
      echo "1 */12 * * *   /usr/ccsp/tad/FileHandle_Monitor.sh" >> $CRONTAB_FILE

      num1=$RANDOM
      num2=$RANDOM
      rand1=`expr $num1 % 60`
      rand2=`expr $num2 % 24`
      echo "$rand1 $rand2 * * * execute_dir /etc/cron/cron.daily" >> $CRONTAB_FILE
      num1=$RANDOM
      num2=$RANDOM
      num3=$RANDOM
      rand1=`expr $num1 % 60`
      rand2=`expr $num2 % 24`
      rand3=`expr $num3 % 7`
      echo "$rand1 $rand2 * * $rand3 execute_dir /etc/cron/cron.weekly" >> $CRONTAB_FILE
      num1=$RANDOM
      num2=$RANDOM
      num3=$RANDOM
      rand1=`expr $num1 % 60`
      rand2=`expr $num2 % 24`
      rand3=`expr $num3 % 28`
      echo "$rand1 $rand2 $rand3 * * execute_dir /etc/cron/cron.monthly" >> $CRONTAB_FILE
      
      # update mso potd every midnight at 00:05
      echo "5 0 * * * sysevent set potd-start" >> $CRONTAB_FILE 

      # Generate Firewall statistics hourly 
      echo "58 * * * * /usr/bin/GenFWLog" >> $CRONTAB_FILE 

	  # Monitor syscfg DB every 15minutes 
      echo "*/15 * * * * /usr/ccsp/tad/syscfg_recover.sh" >> $CRONTAB_FILE 

      # add a ddns watchdog trigger to be run daily
      echo "#! /bin/sh" > /etc/cron/cron.daily/ddns_daily.sh
      echo "sysevent set ddns-start " >> /etc/cron/cron.daily/ddns_daily.sh
      chmod 700 /etc/cron/cron.daily/ddns_daily.sh
   
      # add starting the ntp client once an hour
      echo "#! /bin/sh" > /etc/cron/cron.hourly/ntp_hourly.sh
      echo "sysevent set ntpclient-restart" >> /etc/cron/cron.hourly/ntp_hourly.sh
      chmod 700 /etc/cron/cron.hourly/ntp_hourly.sh

      # log mem and cpu info once an hour
      echo "#! /bin/sh" > /etc/cron/cron.hourly/log_hourly.sh
      echo "nice -n 19 sh /usr/ccsp/tad/log_mem_cpu_info.sh &" >> /etc/cron/cron.hourly/log_hourly.sh
      echo "sh /usr/ccsp/tad/uptime.sh &" >> /etc/cron/cron.hourly/log_hourly.sh
      chmod 700 /etc/cron/cron.hourly/log_hourly.sh
   
      # add starting the process-monitor every 5 minute
      echo "#! /bin/sh" > /etc/cron/cron.every5minute/pmon_every5minute.sh
      echo "nice -n 19 sh /etc/utopia/service.d/pmon.sh" >> /etc/cron/cron.every5minute/pmon_every5minute.sh
      chmod 700 /etc/cron/cron.every5minute/pmon_every5minute.sh

      # add a sysevent tick every minute
      echo "#! /bin/sh" > /etc/cron/cron.everyminute/sysevent_tick.sh
      echo "sysevent set cron_every_minute" >> /etc/cron/cron.everyminute/sysevent_tick.sh
      chmod 700 /etc/cron/cron.everyminute/sysevent_tick.sh

      # monitor syslog every 5 minute
      echo "#! /bin/sh" > /etc/cron/cron.every5minute/log_every5minute.sh
      echo "/usr/sbin/log_handle.sh" >> /etc/cron/cron.every5minute/log_every5minute.sh
      chmod 700 /etc/cron/cron.every5minute/log_every5minute.sh

	  #monitor start-misc in case wan is not online
      echo "#! /bin/sh" > /etc/cron/cron.everyminute/misc_handler.sh
      echo "/etc/utopia/service.d/misc_handler.sh" >> /etc/cron/cron.everyminute/misc_handler.sh
      chmod 700 /etc/cron/cron.everyminute/misc_handler.sh

	  #monitor start-misc in case wan is not online
      echo "#! /bin/sh" > /etc/cron/cron.everyminute/selfheal_bootup.sh
      echo "/usr/ccsp/tad/selfheal_bootup.sh" >> /etc/cron/cron.everyminute/selfheal_bootup.sh
      chmod 700 /etc/cron/cron.everyminute/selfheal_bootup.sh

	  #monitor cosa_start_rem triggered state in case its not triggered on 
	  #bootup even after 10 minutes then we have to trigger this via cron
	  echo "#! /bin/sh" > /etc/cron/cron.every10minute/selfheal_cosa_start_rem.sh
	  echo "/usr/ccsp/tad/selfheal_cosa_start_rem.sh" >> /etc/cron/cron.every10minute/selfheal_cosa_start_rem.sh
	  chmod 700 /etc/cron/cron.every10minute/selfheal_cosa_start_rem.sh

	  # remove max cpu usage reached indication file once in 24 hours
	  echo "#! /bin/sh" > /etc/cron/cron.daily/remove_max_cpu_usage_file.sh
	  echo "/usr/ccsp/tad/remove_max_cpu_usage_file.sh" >> /etc/cron/cron.daily/remove_max_cpu_usage_file.sh
	  chmod 700 /etc/cron/cron.daily/remove_max_cpu_usage_file.sh
   fi
   
   # start the cron daemon
   # echo "[utopia][registration] Starting cron daemon"
   crond -l 9

   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
   killall crond
   sysevent set ${SERVICE_NAME}-status "stopped"
}

# Entry

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
   ntpclient-status)
      STATUS=`sysevent get ntpclient-status`
      if [ "started" = "$STATUS" ] ; then 
        ulog ${SERVICE_NAME} status "restarting ${SERVICE_NAME} service" 
        killall crond
        crond -l 9
      fi
      ;;
   docsis-initialized)
      service_start
   ;;
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" >&2
      exit 3
      ;;
esac



