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
source /etc/utopia/service.d/log_capture_path.sh

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

SERVICE_NAME="crond"
SELF_NAME="`basename $0`"
#BOX_TYPE=`cat /etc/device.properties | grep BOX_TYPE  | cut -f2 -d=`

service_start () 
{
   echo_t "SERVICE_CROND : starting ${SERVICE_NAME} service"
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   killall crond
   
   CRONTAB_DIR="/var/spool/cron/crontabs/"
   CRONTAB_FILE=$CRONTAB_DIR"root"
   if [ ! -e $CRONTAB_FILE ] || [ ! -e "/etc/cron/cron.monthly" ]  ; then
      echo_t "SERVICE_CROND : creating cron files"
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
#RDKB-17984: Runs every 12 hours and prints mesh status
      if [ "$BOX_TYPE" != "XB3" ]; then
       echo "1 */12 * * *  /usr/ccsp/wifi/mesh_status.sh" >> $CRONTAB_FILE
      fi

      if [ "$BOX_TYPE" == "XB3" ]; then
       echo "*/10 * * * * /rdklogger/log_ps_cpu_mem_host.sh" >> $CRONTAB_FILE
      fi
      #zqiu: monitor lan client traffic
      echo "* * * * *   /usr/ccsp/tad/rxtx_lan.sh" >> $CRONTAB_FILE

      echo "1 */6 * * *   /usr/ccsp/tad/monitor_zombies.sh" >> $CRONTAB_FILE
#RDKB-9367, file handle monitor, needs to be run every 12 hours
      echo "1 */12 * * *   /usr/ccsp/tad/FileHandle_Monitor.sh" >> $CRONTAB_FILE

      if [ ! -f "/nvram/syscfg_clean" ]; then
        echo "0 2 * * *   /usr/ccsp/tad/syscfg_cleanup.sh" >> $CRONTAB_FILE
      fi
      
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
      if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ] ; then
      	#Intel Proposed RDKB Generic Bug Fix from XB6 SDK
      	#Don't Zero iptable Counter
      	echo "58 * * * * /usr/bin/GenFWLog -nz" >> $CRONTAB_FILE
      else
      	echo "58 * * * * /usr/bin/GenFWLog" >> $CRONTAB_FILE
      fi

	  # Monitor syscfg DB every 15minutes 
      echo "*/15 * * * * /usr/ccsp/tad/syscfg_recover.sh" >> $CRONTAB_FILE

      # Monitor resource_monitor.sh every 5 minutes TCCBR-3288
      if [ "$BOX_TYPE" = "TCCBR" ]; then 
         echo "*/5 * * * * /usr/ccsp/tad/resource_monitor_recover.sh" >> $CRONTAB_FILE
      fi

      # RDKB-23651
      if [ "x$THERMALCTRL_ENABLE" == "xtrue" ]; then
         echo "*/15 * * * * /usr/ccsp/tad/check_fan.sh" >> $CRONTAB_FILE
      fi

      if [ "$BOX_TYPE" == "HUB4" ]; then
          # add syncing the timeoffset everyday at 01:00 AM
          echo "0 1 * * * /etc/sky/sync_timeoffset.sh" >> $CRONTAB_FILE

          #To monitor all wifi interface packets in every 15minutes
          echo "*/15 * * * * /etc/sky/monitor_wifi_packets.sh" >> $CRONTAB_FILE

          #To monitor all wifi interface dhd dump in every 15minutes
          echo "*/15 * * * * /etc/sky/monitor_dhd_dump.sh" >> $CRONTAB_FILE
      fi

      # Logging current chain mask value of 2G - runs on 1st minute of every 12th hour - only for 3941 box
      MODEL="`grep MODEL_NUM /etc/device.properties | cut -d "=" -f2`"
      if [ -n "$(echo $MODEL | grep 3941)" ]; then
         echo "1 */12 * * *  rpcclient 169.254.101.2 \"/etc/ath/CurrentChainMask_Logging.sh\"" >> $CRONTAB_FILE
      fi

      # Add Unique Telemetry ID if enabled
      telemtery_enable=`syscfg get unique_telemetry_enable`
      telemtery_time_interval=`syscfg get unique_telemetry_interval`
      telemtery_tag=`syscfg get unique_telemetry_tag`

      if [ "$telemtery_enable" = "true" ] && [ 0$telemtery_time_interval -gt 0 ] && [ ! -z "$telemtery_tag" -a "$telemtery_tag" != " " ] ; then
        #Convert time interval(in minutes) to days, hours and minutes
        d=$(($telemtery_time_interval / (60*24)))
        h=$((($telemtery_time_interval % (60*24)) / 60))
        m=$((($telemtery_time_interval % (60*24)) % 60))

        if [ $d -gt 0 ] ; then
          day="*/$d"
          hour="$h"
          mins="$m"
        elif [ $h -gt 0 ] ; then
          day="*"
          hour="*/$h"
          mins="$m"
        else
          day="*"
          hour="*"
          mins="*/$m"
        fi

        echo "$mins $hour $day * * /usr/ccsp/pam/unique_telemetry_id.sh" >> $CRONTAB_FILE
      fi

      # add a ddns watchdog trigger to be run daily
      echo "#! /bin/sh" > /etc/cron/cron.daily/ddns_daily.sh
      echo "sysevent set ddns-start " >> /etc/cron/cron.daily/ddns_daily.sh
      chmod 700 /etc/cron/cron.daily/ddns_daily.sh

	  # add a mta over power status script to be run daily
      echo "#! /bin/sh" > /etc/cron/cron.daily/mta_overcurrent_status_daily.sh
      echo "sh /usr/ccsp/mta/mta_overcurrentfault_status.sh &" >> /etc/cron/cron.daily/mta_overcurrent_status_daily.sh
      chmod 700 /etc/cron/cron.daily/mta_overcurrent_status_daily.sh

      # add starting the ntp client once an hour
      echo "#! /bin/sh" > /etc/cron/cron.hourly/ntp_hourly.sh
      echo "sysevent set ntpclient-restart" >> /etc/cron/cron.hourly/ntp_hourly.sh
      chmod 700 /etc/cron/cron.hourly/ntp_hourly.sh

      # log mem and cpu info once an hour
      echo "#! /bin/sh" > /etc/cron/cron.hourly/log_hourly.sh
      echo "nice -n 19 sh /usr/ccsp/tad/log_hourly.sh &" >> /etc/cron/cron.hourly/log_hourly.sh
      if [ -f /usr/ccsp/rbus_status_logger.sh ]
      then
          echo "/usr/ccsp/rbus_status_logger.sh" >> /etc/cron/cron.hourly/log_hourly.sh
      fi
      chmod 700 /etc/cron/cron.hourly/log_hourly.sh
   
      # add starting the process-monitor every 5 minute
      echo "#! /bin/sh" > /etc/cron/cron.every5minute/pmon_every5minute.sh
      echo "nice -n 19 sh /etc/utopia/service.d/pmon.sh" >> /etc/cron/cron.every5minute/pmon_every5minute.sh
      chmod 700 /etc/cron/cron.every5minute/pmon_every5minute.sh

      # add starting the var and tmp log-monitor every 5 minute
      if [ "$BOX_TYPE" == "HUB4" ]; then
          echo "#! /bin/sh" > /etc/cron/cron.every5minute/monitor_log_every5minute.sh
          echo "nice -n 19 sh /etc/sky/monitor_log.sh" >> /etc/cron/cron.every5minute/monitor_log_every5minute.sh
          chmod 700 /etc/cron/cron.every5minute/monitor_log_every5minute.sh
      fi

      # add a sysevent tick every minute
      echo "#! /bin/sh" > /etc/cron/cron.everyminute/sysevent_tick.sh
      echo "sysevent set cron_every_minute" >> /etc/cron/cron.everyminute/sysevent_tick.sh
      chmod 700 /etc/cron/cron.everyminute/sysevent_tick.sh

      # monitor syslog every 5 minute
#      echo "#! /bin/sh" > /etc/cron/cron.every5minute/log_every5minute.sh
#     echo "/usr/sbin/log_handle.sh" >> /etc/cron/cron.every5minute/log_every5minute.sh
#      chmod 700 /etc/cron/cron.every5minute/log_every5minute.sh

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

      #monitor syslog every 10 minutes
      echo "#! /bin/sh" > /etc/cron/cron.every10minute/log_every10minute.sh
      echo "/usr/sbin/log_handle.sh" >> /etc/cron/cron.every10minute/log_every10minute.sh
      chmod 700 /etc/cron/cron.every10minute/log_every10minute.sh

	#This variable is to check RFC ETHWAN Mode Enabled/Disabled from syscfg DB
	rfc_ethwan_status=""
	rfc_ethwan_status=`syscfg get eth_wan_enabled`

	#This variable is to check RFC WANLinkHeal Enabled/Disabled from syscfg DB
	rfc_wanlinkheal_status=""
	rfc_wanlinkheal_status=`syscfg get wanlinkheal`

	#To invoke start_gw_health script on bootup-check, Follwing condition need to statisfy
	#RFC ETHWAN should be false/null and WAN_TYPE should be DOCSIS
	#RF WANLinkHeal should be true and BOX_TYPE should be plaftorm specfic
	#In CISCOXB3 platform, does not have WAN_TYPE paramenter in /etc/device.properties file, So added MODEL_NUM Check along with WAN_TYPE.
	if [ "x$rfc_ethwan_status" == "xfalse" ] || [ "$rfc_ethwan_status" == "" ]; then
		if [ "x$WAN_TYPE" == "xDOCSIS" ] || [ "x$MODEL_NUM" == "xDPC3941" ] || [ "x$MODEL_NUM" == "xDPC3941B" ] || [ "x$MODEL_NUM" == "xDPC3939B" ]; then
			if [ "x$rfc_wanlinkheal_status" == "xtrue" ]; then
				if [ "x$BOX_TYPE" == "xXB3" ] || [ "x$BOX_TYPE" == "xXB6" ] || [ "x$BOX_TYPE" == "xTCCBR" ]; then
					echo_t "RFC WANLinkHeal Feature is Enabled"
					echo "#! /bin/sh" > /etc/cron/cron.every10minute/start_gw_heath.sh
					echo "/usr/ccsp/tad/start_gw_heath.sh" >> /etc/cron/cron.every10minute/start_gw_heath.sh
					chmod 700 /etc/cron/cron.every10minute/start_gw_heath.sh
				else
					echo_t "RFC WANLinkHeal Feature is not Enabled"
				fi
			else
				echo_t "Set RFC WANLinkHeal flag to Enable for WANLinkHeal Feature support"
			fi
		else
			echo_t "This Device WAN TYPE is not DOCSIS, Needed DOCSIS type Device for WANLinkHeal"
		fi
	else
		echo_t "This Device should not enabled ETHWAN mode"
	fi

	  # remove max cpu usage reached indication file once in 24 hours
	  echo "#! /bin/sh" > /etc/cron/cron.daily/remove_max_cpu_usage_file.sh
	  echo "/usr/ccsp/tad/remove_max_cpu_usage_file.sh" >> /etc/cron/cron.daily/remove_max_cpu_usage_file.sh
	  chmod 700 /etc/cron/cron.daily/remove_max_cpu_usage_file.sh
   fi
 


 
   # start the cron daemon
   # echo "[utopia][registration] Starting cron daemon"
   echo_t "SERVICE_CROND : Starting cron daemon"
   crond -l 9

   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () 
{
   echo_t "SERVICE_CROND : stopping ${SERVICE_NAME} service"
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
   killall crond
   sysevent set ${SERVICE_NAME}-status "stopped"
}

# Entry
echo_t "SERVICE_CROND : event $1"
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
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" >&2
      exit 3
      ;;
esac



