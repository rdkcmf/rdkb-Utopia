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

source /etc/utopia/service.d/date_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh
if [ -f /lib/rdk/utils.sh ];then
     . /lib/rdk/utils.sh
fi

SERVICE_NAME="ddns"

CONF_FILE=/etc/ez-ipupdate.conf
OUTPUT_FILE=/etc/ez-ipupdate.out
PID="($$)"


CACHE_FILE_PREFIX=/tmp/ez-ipupdate.cache.
WAN_IFNAME=`sysevent get current_wan_ifname`
CACHE_FILE=${CACHE_FILE_PREFIX}${WAN_IFNAME}

#--------------------------------------------------------------
# prepare_ddns_config_file
#--------------------------------------------------------------
prepare_ddns_config_file() {
   LOCAL_CONF_FILE=/tmp/ez-ipupdate.conf$$
   if [ -z "$SYSCFG_ddns_service" ] ; then
      SYSCFG_ddns_service=dyndns
   fi

SYSCFG_ddns_service=`syscfg get ddns_service`
SYSCFG_ddns_username=`syscfg get ddns_username`
SYSCFG_ddns_password=`syscfg get ddns_password`
SYSCFG_ddns_hostname=`syscfg get ddns_hostname`
SYSCFG_ddns_mx=`syscfg get ddns_mx`
SYSCFG_ddns_mx_backup=`syscfg get ddns_mx_backup`
SYSCFG_ddns_wildcard=`syscfg get ddns_wildcard`

cat << EOM > $LOCAL_CONF_FILE
service-type=$SYSCFG_ddns_service
user=${SYSCFG_ddns_username}:${SYSCFG_ddns_password}
host=${SYSCFG_ddns_hostname}
interface=$WAN_IFNAME
max-interval=2073600
cache-file=$CACHE_FILE
retrys=1
#daemon
EOM

   if [ -n "$SYSCFG_ddns_mx" ]; then
      echo "mx=$SYSCFG_ddns_mx" >> $LOCAL_CONF_FILE
      if [ "1" = "$SYSCFG_ddns_mx_backup" ] ; then
         echo "backmx=YES" >> $LOCAL_CONF_FILE
      fi
   fi
   if [ "1" = "$SYSCFG_ddns_wildcard" ] ; then
      echo "wildcard" >> $LOCAL_CONF_FILE
   fi

   cat $LOCAL_CONF_FILE > $CONF_FILE
   rm -f $LOCAL_CONF_FILE
}

#--------------------------------------------------------------
# prepare_extra_commandline_params
#--------------------------------------------------------------
prepare_extra_commandline_params () {
   EXTRA_PARAMS=""
   if [ -n "$SYSCFG_ddns_server" ] ; then
      EXTRA_PARAMS="$EXTRA_PARAMS --server $SYSCFG_ddns_server"
   fi

   echo "$EXTRA_PARAMS"
}

#---------------------------------------------------------------------------------------
# update_ddns_server
# return values:
#    0            : success
#    1            : error requiring explicit user intervention (eg. authentication)
#    2            : error that requires quiet time before retry (eg. connection failure)
#---------------------------------------------------------------------------------------
update_ddns_server() {
#   prepare_ddns_config_file

#   EXTRA_PARAMS=`prepare_extra_commandline_params`
#   ez-ipupdate $EXTRA_PARAMS -c $CONF_FILE -e /etc/utopia/service.d/service_ddns/ddns_success.sh > $OUTPUT_FILE 2>&1
   DnsIdx=1
   EXTRA_PARAMS=""
   while [ $DnsIdx -le 2 ]; do
       ddns_enable_x=`syscfg get ddns_enable${DnsIdx}`
       if [ "1" = "$ddns_enable_x" ]; then
           ddns_service_x=`syscfg get ddns_service${DnsIdx}`
           ddns_username_x=`syscfg get ddns_username${DnsIdx}`
           ddns_password_x=`syscfg get ddns_password${DnsIdx}`
           ddns_hostname_x=`syscfg get ddns_hostname${DnsIdx}`
           EXTRA_PARAMS="--interface=erouter0"
           EXTRA_PARAMS="${EXTRA_PARAMS} --cache-file=/var/ez-ipupdate.cache.${ddns_service_x}"
           EXTRA_PARAMS="${EXTRA_PARAMS} --daemon"
           EXTRA_PARAMS="${EXTRA_PARAMS} --max-interval=2073600"
           EXTRA_PARAMS="${EXTRA_PARAMS} --service-type=${ddns_service_x}"
           EXTRA_PARAMS="${EXTRA_PARAMS} --user=${ddns_username_x}:${ddns_password_x}"
           EXTRA_PARAMS="${EXTRA_PARAMS} --host=${ddns_hostname_x}"
       fi
       DnsIdx=`expr $DnsIdx + 1`
   done
           
   if [ -n "${EXTRA_PARAMS}" ]; then
       echo "/usr/bin/ez-ipupdate ${EXTRA_PARAMS} " 
       /usr/bin/ez-ipupdate ${EXTRA_PARAMS} 
       RET_CODE=$?
   else
       RET_CODE=1
   fi
       
   if [ "0" != "$RET_CODE" ]; then
      # we clear the wan_last_ipaddr to force us to retry once the error is cleared
      if [ -n "$SYSCFG_wan_last_ipaddr" ] ; then
         ulog ddns status "$PID unset syscfg wan_last_ipaddr"
         syscfg unset wan_last_ipaddr
         syscfg commit
      fi

      # set a sysevent tuple to indicate a 10 minute backoff time
      sysevent set ddns_failure_time "`get_current_time`"

      grep -q "error connecting" $OUTPUT_FILE
      if [ "0" = "$?" ]; then
         sysevent set ddns_return_status error-connect
         ulog ddns status "$PID ddns_return_status error-connect"
         sysevent set ${SERVICE_NAME}-errinfo "connection error"
         sysevent set ${SERVICE_NAME}-status error
         return 2
      fi
      grep -q "authentication failure" $OUTPUT_FILE
      if [ "0" = "$?" ]; then
         sysevent set ddns_return_status error-auth
         ulog ddns status "$PID ddns_return_status error-auth"
         sysevent set ${SERVICE_NAME}-errinfo "authentication error"
         sysevent set ${SERVICE_NAME}-status error
         return 2
      fi

      # what error is this???
      sysevent set ${SERVICE_NAME}-errinfo "ddns client error"
      sysevent set ${SERVICE_NAME}-status error
      return 2
      
   else
      removeCron "sysevent set ddns-start"
      sysevent set ddns_return_status success
      ulog ddns status "$PID ddns_return_status success"
      sysevent set ${SERVICE_NAME}-status started
   fi
}

#---------------------------------------------------------------------------------------
# do_start
#---------------------------------------------------------------------------------------
do_start() {
   # There are 2 reasons to do a ddns update:
   # 1. The wan ip address has changed
   # 2. It has been a long time since we last updated, and we dont want the 
   #    ddns service to cancel our subscription when it thinks we are no longer
   #    utilizing the service

   UPDATE_NEEDED=0

   # 1. Our wan ip address has changed
   if [ -z "$SYSCFG_wan_last_ipaddr" ] ; then
      WAN_LAST_IPADDR=0.0.0.0
   else
      WAN_LAST_IPADDR=$SYSCFG_wan_last_ipaddr
   fi
   CURRENT_WAN_IPADDR=`sysevent get current_wan_ipaddr`
   if [ -z "$CURRENT_WAN_IPADDR" ] || [ "0.0.0.0" = "$CURRENT_WAN_IPADDR" ] ; then
      ulog ddns status "$PID current wan ip addr is 0.0.0.0"
      return 0
   fi
   ulog ddns status "$PID ddns do_start: wan_last_ipaddr=$WAN_LAST_IPADDR"
   ulog ddns status "$PID ddns do_start: current_wan_ipaddr=$CURRENT_WAN_IPADDR"
   if [ "0.0.0.0" = "$WAN_LAST_IPADDR" ] ; then
      UPDATE_NEEDED=1
      ulog ddns status "$PID ddns update required due to no previous update"
   elif [ "$WAN_LAST_IPADDR" !=  "$CURRENT_WAN_IPADDR" ] ; then  
        UPDATE_NEEDED=1
        ulog ddns status "$PID ddns update required due change in wan ip address from $WAN_LAST_IPADDR to $CURRENT_WAN_IPADDR"
   fi

   # 2. Its been a long time since our last update
   if [ "0" = "$UPDATE_NEEDED" ] ; then
      if [ -z "$SYSCFG_ddns_last_update" ] || [ "0" = "$SYSCFG_ddns_last_update" ] ; then
         UPDATE_NEEDED=1
         ulog ddns status "$PID ddns update required due to no previous update on record"
      else
         # just in case this script is called at system boot
         # we need to give ntp client a chance to update the time
         sleep 5
         CURRENT_TIME=`get_current_time`
         DELTA_DAYS=`delta_days "$SYSCFG_ddns_last_update"  "$CURRENT_TIME"`
         DIFF=`expr "$SYSCFG_ddns_update_days" - "$DELTA_DAYS"`
         if [ "$DIFF" -le 0 ] ; then
            UPDATE_NEEDED=1
            ulog ddns status "$PID ddns update required due to no update in $SYSCFG_ddns_update_days days"
         fi
      fi
   fi 
   
   ulog ddns status "$PID ddns update required status is $UPDATE_NEEDED"
   LAST_FAIL_TIME=`sysevent get ddns_failure_time`
   if [ -n "$LAST_FAIL_TIME" ] ; then
      NOW=`get_current_time`
      DELTA=`delta_mins "$LAST_FAIL_TIME" "$NOW"`
      if [ -n "$DELTA" ] ; then
         if [ "3" -gt "$DELTA" ] ; then
            # we need to back off for 10 minutes between updates
            # so setup a cron job to retry in future
            ulog ddns status "$PID ddns update required but we are in a quiet period. Will retry later"
            sysevent set ${SERVICE_NAME}-errinfo "mandated quiet period"
            sysevent set ${SERVICE_NAME}-status error
	    removeCron "sysevent set ddns-start"
            addCron "1,6,11,16,21,26,31,36,41,46,51,56 * * * * sysevent set ddns-start"
         else 
            sysevent set ddns_failure_time
         fi
      fi 
   fi

   if [ "1" = "$UPDATE_NEEDED" ] ; then
      sysevent set ddns_return_status success
      sysevent set ${SERVICE_NAME}-errinfo
      update_ddns_server
      # update_ddns_server will set ddns-status
      # and ddns-errinfo if necessary
   else
      # if no update needed, consider it as ddns "success"
      ulog ddns status "$PID No ddns update is required at this time"
      sysevent set ddns_return_status success
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status started
   fi
}

#----------------------------------------------------------------------
#  update_ddns_if_needed
#  ddns periodically checks to see if it should update the ddns server
#  This is the function that will be called whenever that periodic event 
#  occurs
#----------------------------------------------------------------------
update_ddns_if_needed () {
   WAN_IPADDR=`sysevent get current_wan_ipaddr`
   CURRENT_STATE=`sysevent get wan-status`
   if [ "started" != "$CURRENT_STATE" ] ; then
      WAN_IPADDR="0.0.0.0"
   fi

   case "$WAN_IPADDR" in
      0.0.0.0)
        ulog ddns status "$PID wan state is down. No ddns update possible"
        sysevent set ${SERVICE_NAME}-errinfo "wan is down. No update possible"
        sysevent set ${SERVICE_NAME}-status error
        ;;
   *)
      PRIORERROR=`sysevent get ddns_return_status`
      if [ "0" != "$SYSCFG_ddns_enable" ] ; then
         if [ -z "$PRIORERROR" ] || [ "success" = "$PRIORERROR" ] ; then
            # if the wan ip address changed, then the system requires a few secs to stabilize
            # eg. firewall needs to be reset. Give it a few secs to do so
            sleep 5 
            ulog ddns status "$PID Current wan ip address is $WAN_IPADDR. Continuing"
            do_start
         else
            ulog ddns status "$PID No ddns update due to prior ddns error (${PRIORERROR})"
            sysevent set ${SERVICE_NAME}-errinfo "No update possible due to prior ddns error (${PRIORERROR})"
            sysevent set ${SERVICE_NAME}-status error
         fi
      else
         ulog ddns status "$PID ddns is not enabled"
         sysevent set ${SERVICE_NAME}-status stopped
      fi
      ;;
   esac
}

#----------------------------------------------------------------------
# service_init
#----------------------------------------------------------------------
service_init ()
{
    #FOO=`utctx_cmd get ddns_enable wan_last_ipaddr ddns_last_update ddns_update_days ddns_hostname ddns_username ddns_password ddns_service ddns_mx ddns_mx_backup ddns_wildcard ddns_server`
    FOO=`utctx_cmd get ddns_enable wan_last_ipaddr ddns_last_update ddns_update_days ddns_mx ddns_mx_backup ddns_wildcard `
    eval "$FOO"
}

#----------------------------------------------------------------------
#  service_start
#  ddns periodically checks to see if it should update the ddns server
#  ddns-start is called by cron periodically
#----------------------------------------------------------------------
service_start ()
{
   sysevent set ${SERVICE_NAME}-errinfo
   update_ddns_if_needed
}

#----------------------------------------------------------------------
#  service_start
#  ddns periodically checks to see if it should update the ddns server
#  There is no real meaning to a stop event
#----------------------------------------------------------------------
service_stop ()
{
   sysevent set ${SERVICE_NAME}-errinfo
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
      service_start
      ;;
   current_wan_ipaddr)
      PRIORERROR=`sysevent get ddns_return_status`
      if [ "error-connect" = "$PRIORERROR" ] ; then
         sysevent set ddns_return_status
      fi
      update_ddns_if_needed
      ;;
   wan-status)
      update_ddns_if_needed
      ;;
   *)
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac

