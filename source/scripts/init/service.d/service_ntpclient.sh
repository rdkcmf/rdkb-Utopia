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

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
source /lib/rdk/t2Shared_api.sh
if [ -f /lib/rdk/utils.sh ];then
     . /lib/rdk/utils.sh
fi
CRON_DIR="/var/spool/cron/crontabs/"
CRONTAB_FILE=$CRON_DIR"root"
SERVICE_NAME="ntpclient"
SELF_NAME="`basename "$0"`"


TZ_FILE=/etc/TZ

RETRY_SOON_FILENAME=/etc/cron/cron.everyminute/ntp_retry_soon.sh

prepare_retry_soon_file()
{
   addCron "* * * * *  sysevent set ntpclient-start"
}

service_start ()
{
    # this needs to be hooked up to syscfg for specific timezone
    if [ "HST10" = "$SYSCFG_TZ" ] ; then
        ln -sf /usr/share/zoneinfo/HST /etc/localtime
    elif [ "AKST9" = "$SYSCFG_TZ" ] || [ "AKST9AKDT,M3.2.0/02:00,M11.1.0/02:00" = "$SYSCFG_TZ" ] || [ "AKST9AKDT,M3.2.0,M11.1.0" = "$SYSCFG_TZ" ] ; then
        ln -sf /usr/share/zoneinfo/America/Anchorage /etc/localtime
    elif [ "PST8" = "$SYSCFG_TZ" ] || [ "PST8PDT,M3.2.0/02:00,M11.1.0/02:00" = "$SYSCFG_TZ" ] ; then
        ln -sf /usr/share/zoneinfo/America/Los_Angeles /etc/localtime
    elif [ "MST7" = "$SYSCFG_TZ" ] || [ "MST7MDT,M3.2.0/02:00,M11.1.0/02:00" = "$SYSCFG_TZ" ] ; then
        ln -sf /usr/share/zoneinfo/America/Denver /etc/localtime
    elif [ "CST6" = "$SYSCFG_TZ" ] || [ "CST6CDT,M3.2.0/02:00,M11.1.0/02:00" = "$SYSCFG_TZ" ] ; then
        ln -sf /usr/share/zoneinfo/America/Chicago /etc/localtime
    elif [ "EST5" = "$SYSCFG_TZ" ] || [ "EST5EDT,M3.2.0/02:00,M11.1.0/02:00" = "$SYSCFG_TZ" ] ; then
        ln -sf /usr/share/zoneinfo/America/New_York /etc/localtime
    elif [ "VET4" = "$SYSCFG_TZ" ] || [ "CLT4" = "$SYSCFG_TZ" ] || [ "CLT4CLST,M10.2.6/04:00,M3.2.6/03:00" = "$SYSCFG_TZ" ] ; then
        ln -sf /usr/share/zoneinfo/America/Caracas /etc/localtime
    else
        ln -sf /usr/share/zoneinfo/UTC /etc/localtime
    fi

   if [ -n "$SYSCFG_ntp_enabled" ] && [ "0" = "$SYSCFG_ntp_enabled" ] ; then
      syscfg set ntp_status 1
      return 0
   fi

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "starting"
   ulog ${SERVICE_NAME} status "starting" 
   syscfg set ntp_status 2

   if [ "started" != "$CURRENT_WAN_STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-status "error"
      sysevent set ${SERVICE_NAME}-errinfo "wan connection down"
      ulog ${SERVICE_NAME} status "error starting ${SERVICE_NAME} service - wan down" 
      syscfg set ntp_status 5
      return 0
   fi

   # every time we start we want to cycle through our pool of ntp servers
   INDEX=`sysevent get ntp_pool_index`
   if [ -z "$INDEX" ] || [ "3" -lt "$INDEX" ] ; then
      INDEX=0
   fi

   # also, if we got ntp servers through dhcp, then we use those
   NTP_SERVER=`sysevent get dhcpc_ntp_server1`
   if [ -n "$NTP_SERVER" ] ; then
      USE_DHCP=1
   else
      USE_DHCP=0
   fi

   INDEX=`expr $INDEX + 1` 

   if [ "1" = "$USE_DHCP" ] ; then
      NTP_SERVER=`sysevent get dhcpc_ntp_server"$INDEX"`
   else
      eval "`echo NTP_SERVER='$'SYSCFG_ntp_server"${INDEX}"`"
   fi


   if [ -z "$NTP_SERVER" ] || [ "0.0.0.0" = "$NTP_SERVER" ] ; then
      INDEX=1
      if [ "1" = "$USE_DHCP" ] ; then
         NTP_SERVER=`sysevent get dhcpc_ntp_server$INDEX`
      else
         eval "`echo NTP_SERVER='$'SYSCFG_ntp_server${INDEX}`"
      fi
   fi

   # failsafe in case we still havent got an ntp server
   if [ -z "$NTP_SERVER" ] || [ "0.0.0.0" = "$NTP_SERVER" ] ; then
      # we did not get any ntp server lets check sanity
      # we KNOW that there are a maximum of 3 ntp servers provisioned in 
      # syscfg. So check all 3
      NTP_SERVER=$SYSCFG_ntp_server1
      if [ -z "$NTP_SERVER" ] || [ "0.0.0.0" = "$NTP_SERVER" ] ; then
         NTP_SERVER=$SYSCFG_ntp_server2 
         if [ -z "$NTP_SERVER" ] || [ "0.0.0.0" = "$NTP_SERVER" ] ; then
            NTP_SERVER=$SYSCFG_ntp_server3 
            if [ -z "$NTP_SERVER" ] || [ "0.0.0.0" = "$NTP_SERVER" ] ; then
               exit 0
            else
               INDEX=3
            fi
         else
            INDEX=2
         fi
      else
         INDEX=1
      fi
   fi

   `sysevent set ntp_pool_index $INDEX`

   if [ -n "$SYSCFG_TZ" ] ; then
      echo "$SYSCFG_TZ" > $TZ_FILE
   fi

   # if we had been unable to get a response from the ntp server, then we had put
   # a request into cron to retry every minute. Cancel that request
   removeCron "sysevent set ntpclient-start"
   
   # now try to connect to the ntp server
   RESULT=`ntpclient -h "$NTP_SERVER" -i 60 -s`

   # if there is a transient failure we try again
   if [ -z "$RESULT" ] ; then
      sleep 10
      RESULT=`ntpclient -h "$NTP_SERVER" -i 60 -s`
   fi

   # if we dont get a result we should force system to try again soon because
   # otherwise we only try 1/hr.
   if [ -z "$RESULT" ] && [ ! grep 'sysevent set ntpclient-start' $CRONTAB_FILE ] ; then
      prepare_retry_soon_file
      sysevent set ${SERVICE_NAME}-status "error"
      sysevent set ${SERVICE_NAME}-errinfo "No result from NTP Server"
      syscfg set ntp_status 4
      return 0
   fi

   ulog ${SERVICE_NAME} status "updated time" 
   if [ -n "$SYSCFG_InternetAccessPolicyCount" ] && [ "0" != "$SYSCFG_InternetAccessPolicyCount" ] ; then
      # if there is an Internet Access Policy then we need to give the firewall a chance to react to the 
      # new known time
       echo "service_ntpclient : Triggering RDKB_FIREWALL_RESTART"
       t2CountNotify "RF_INFO_RDKB_FIREWALL_RESTART"
      sysevent set firewall-restart
   fi
   sysevent set ${SERVICE_NAME}-status "started"
   syscfg set ntp_status 3

   # Set FirstUseDate in Syscfg if this is the first time we are doing a successful NTP Sych
   DEVICEFIRSTUSEDATE=`syscfg get device_first_use_date`
   if [ "0" = "$DEVICEFIRSTUSEDATE" ] ; then
      FIRSTUSEDATE=`date +%Y-%m-%dT%H:%M:%S`
      syscfg set device_first_use_date "$FIRSTUSEDATE"
   fi
}

service_stop ()
{
   sysevent set ${SERVICE_NAME}-status
   sysevent set ${SERVICE_NAME}-status "stopped"
}


service_init ()
{
    FOO=`utctx_cmd get ntp_server1 ntp_server2 ntp_server3 ntp_enabled TZ InternetAccessPolicyCount`
    eval "$FOO"
}


# Entry

service_init

CURRENT_WAN_STATUS=`sysevent get wan-status`
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
  wan-status)
      if [ "started" = "$CURRENT_WAN_STATUS" ] ; then
         service_start
      fi
      ;;
  bridge-status)
      # if there is no wan but we are bridging then do ntp
      if [ "stopped" = "$CURRENT_WAN_STATUS" ] ; then
         CURRENT_WAN_STATUS=`sysevent get bridge-status`
         if [ "started" = "$CURRENT_WAN_STATUS" ] ; then
            service_start
         fi
      fi
      ;;
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | wan-status ]" >&2
      exit 3
      ;;
esac

