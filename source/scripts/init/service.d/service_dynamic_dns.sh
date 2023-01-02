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

SERVICE_NAME="dynamic_dns"

CONF_FILE=/etc/ez-ipupdate.conf
OUTPUT_FILE=/var/tmp/ez-ipupdate.out
RETRY_SOON_FILENAME="/etc/cron/cron.everyminute/ddns_retry.sh"
CHECK_INTERVAL_FILENAME="/etc/cron/cron.everyminute/ddns_check_interval.sh"
PID="($$)"


CACHE_FILE_PREFIX=/tmp/ez-ipupdate.cache.
WAN_IFNAME=`sysevent get current_wan_ifname`
CACHE_FILE=${CACHE_FILE_PREFIX}${WAN_IFNAME}
GENERAL_FILE=/tmp/ddns-general.trace

DnsIdx=1
DDNS_UNAME=`syscfg get arddnsclient_${DnsIdx}::Username`
DDNS_PWD=`syscfg get arddnsclient_${DnsIdx}::Password`
DDNS_CLIENT_SET_SERVER=`syscfg get arddnsclient_${DnsIdx}::Server`
DDNS_HOST=`syscfg get ddns_host_name_${DnsIdx}`
DDNS_HOST_ENABLE=`syscfg get ddns_host_enable_${DnsIdx}`
DDNS_ENABLE=`syscfg get arddnsclient_${DnsIdx}::enable`

#CLIENT Status
CLIENT_CONNECTING=1
CLIENT_AUTHENTICATING=2
CLIENT_UPDATED=3
CLIENT_ERROR_MISCONFIGURED=4
CLIENT_ERROR=5
CLIENT_DISABLED=6

#LAST Error Status
NO_ERROR=1
MISCONFIGURATION_ERROR=2
DNS_ERROR=3
CONNECTION_ERROR=4
AUTHENTICATION_ERROR=5
TIMEOUT_ERROR=6
PROTOCOL_ERROR=7

#Host Status
HOST_REGISTERED=1
HOST_UPDATE_NEEDED=2
HOST_UPDATING=3
HOST_ERROR=4
HOST_DISABLED=5

update_ddns_server() {
   #DDNS_CLIENT_SET_SERVER=Device.DynamicDNS.Server.$DnsServerIdx
   DnsIdx=`echo "${DDNS_CLIENT_SET_SERVER}" | cut -d"." -f4`
   EXTRA_PARAMS=""

   resolve_error="Couldn't resolve host"
   connecting1_error="Failed to connect to"
   connecting2_error="connect fail"

   #No-ip.com
   register_success_no_ip="good"
   update_success_no_ip="nochg"
   hostname_error_no_ip="nohost"
   username_error_no_ip="badauth"
   password_error_no_ip="badauth"
   general_error_no_ip=""
   token_error_no_ip=""

   #Dyndns.org:Empty reply from server
   register_success_dyndns="good"
   update_success_dyndns="nochg"
   hostname_error_dyndns="nohost"
   username_error_dyndns="badauth"
   password_error_dyndns="badauth"
   token_error_dyndns=""

   #duckdns.org
   register_success_duckdns="OK"
   general_error_duckdns="KO"

   #afraid.org
   register_success_afraid="Updated"
   update_success_afraid="has not changed"
   hostname_error_afraid=""
   username_error_afraid=""
   password_error_afraid=""
   general_error_afraid="Unable to locate this record"
   token_error_afraid=""

   #easydns.com
   register_success_easydns="OK"          
   update_success_easydns="OK"            
   hostname_error_easydns=""
   username_error_easydns="NO_AUTH"       
   password_error_easydns="NO_AUTH"       
   token_error_easydns=""

   #changeip.com
   register_success_changeip="Successful Update"
   update_success_changeip="Successful Update"
   hostname_error_changeip="Hostname pattern does not exist"
   username_error_changeip="badauth"
   password_error_changeip="badauth"
   token_error_changeip=""
   service_changeip_com="changeip"

   #Set Return status
   ps | grep ez-ipupdate | grep -v grep
   if [ $? -eq 0 ];then
       killall -9 ez-ipupdate
   fi

   UPDATE_UTIL=""
   WanIpAddress=$1


   DDNS_SERVER=`syscfg get ddns_server_servicename_"${DnsIdx}"`
   DDNS_SERVER_ENABLE=`syscfg get ddns_server_enable_"${DnsIdx}"`
   if [ "$DDNS_ENABLE" == 1 ] && [ "$DDNS_SERVER_ENABLE" == 1 ]; then
      ddns_enable_x=1
   else
      ddns_enable_x=0
   fi

   EXTRA_PARAMS=""
    if [ "1" = "$ddns_enable_x" ]; then
         ddns_service_x="$DDNS_SERVER"
         ddns_username_x="$DDNS_UNAME"
         ddns_password_x="$DDNS_PWD"
         ddns_hostname_x="$DDNS_HOST"
         ddns_host_enable_x="$DDNS_HOST_ENABLE"
         if [ "0" = "ddns_host_enable_x" ];then
         ddns_hostname_x=""
         fi
         ddns_stoken_x=""
         ddns_service_name_mod=`echo "${ddns_service_x}" | tr '-' '_'`
         sysevent set ddns_return_status"${DnsIdx}" error-connect
         pwdlen=${#ddns_password_x}
         tmp=""
         ddns_password_x_mod=""
         for (( pos=0 ; pos<pwdlen ; pos++ )); do
             tmp=${ddns_password_x:$pos:1}
             case "$tmp" in
                 [-_.~a-zA-Z0-9])
                 ;;
                 *)
                     printf -v tmp '%%%02x' "'$tmp"
                 ;;
             esac
             ddns_password_x_mod+="${tmp}"
         done

         if [ "$ddns_service_x" == "dyndns" ]; then
              EXTRA_PARAMS="--interface=erouter0"
              #EXTRA_PARAMS="${EXTRA_PARAMS} --daemon"
              EXTRA_PARAMS="${EXTRA_PARAMS} --max-interval=2073600"
              EXTRA_PARAMS="${EXTRA_PARAMS} --service-type=${ddns_service_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --user=${ddns_username_x}:${ddns_password_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --host=${ddns_hostname_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --address=${WanIpAddress}"
              UPDATE_UTIL="/usr/bin/ez-ipupdate"
         elif [ "$ddns_service_x" == "easydns" ]; then
              EXTRA_PARAMS="--interface erouter0"
              EXTRA_PARAMS="${EXTRA_PARAMS} -o /var/tmp/ipupdate.${ddns_service_name_mod}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --url https://${ddns_username_x}:${ddns_password_x_mod}@api.cp.easydns.com/dyn/generic.php"
              EXTRA_PARAMS="${EXTRA_PARAMS}?hostname=${ddns_hostname_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&myip=${WanIpAddress} --trace-ascii $GENERAL_FILE"
         elif [ "$ddns_service_x" == "changeip" ]; then
              EXTRA_PARAMS="--interface erouter0"
              EXTRA_PARAMS="${EXTRA_PARAMS} -o /var/tmp/ipupdate.${ddns_service_name_mod}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --url http://nic.changeip.com/nic/update"
              EXTRA_PARAMS="${EXTRA_PARAMS}?u=${ddns_username_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&p=${ddns_password_x_mod}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&hostname=${ddns_hostname_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&ip=${WanIpAddress} --trace-ascii $GENERAL_FILE"
              UPDATE_UTIL="/usr/bin/curl"
         elif [ "$ddns_service_x" == "afraid" ]; then
              EXTRA_PARAMS="--interface erouter0"
              EXTRA_PARAMS="${EXTRA_PARAMS} -o /var/tmp/ipupdate.${ddns_service_name_mod}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --user ${ddns_username_x}:${ddns_password_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --insecure --url https://freedns.afraid.org/nic/update"
              EXTRA_PARAMS="${EXTRA_PARAMS}?hostname=${ddns_hostname_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&myip=${WanIpAddress} --trace-ascii $GENERAL_FILE"
              UPDATE_UTIL="/usr/bin/curl"
          elif [ "$ddns_service_x" == "no-ip" ]; then
              EXTRA_PARAMS="--interface erouter0"
              EXTRA_PARAMS="${EXTRA_PARAMS} -o /var/tmp/ipupdate.${ddns_service_name_mod}"
              EXTRA_PARAMS="${EXTRA_PARAMS} --url http://${ddns_username_x}:${ddns_password_x_mod}@dynupdate.no-ip.com/nic/update"
              EXTRA_PARAMS="${EXTRA_PARAMS}?hostname=${ddns_hostname_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&myip=${WanIpAddress} --trace-ascii $GENERAL_FILE"
              UPDATE_UTIL="/usr/bin/curl"
          elif [ "$ddns_service_x" == "duckdns" ]; then
              EXTRA_PARAMS="--interface erouter0"
              EXTRA_PARAMS="${EXTRA_PARAMS} -o /var/tmp/ipupdate.${ddns_service_name_mod} -g"
              EXTRA_PARAMS="${EXTRA_PARAMS} --insecure --url https://www.duckdns.org/update"
              EXTRA_PARAMS="${EXTRA_PARAMS}?domains=${ddns_hostname_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&token=${ddns_username_x}"
              EXTRA_PARAMS="${EXTRA_PARAMS}&ip=${WanIpAddress}&verbose=true --trace-ascii $GENERAL_FILE"
              UPDATE_UTIL="/usr/bin/curl"
        fi

         if [ -n "${EXTRA_PARAMS}" ]; then

              if [ -z "$ddns_hostname_x" ]; then
                   echo "###### sysevent set ddns_return_status error-domain"
                   syscfg set ddns_client_Status $CLIENT_ERROR
                   syscfg set ddns_host_status_1 $HOST_ERROR
                   syscfg set ddns_client_Lasterror $DNS_ERROR
                   syscfg commit
                   continue
              fi

              if [ -z "$ddns_stoken_x" ] && ([ -z "$ddns_username_x" ] || [ -z "$ddns_password_x" ]); then
                  echo "$PID ddns_return_status error-auth"
                  syscfg set ddns_client_Status $CLIENT_ERROR
                  syscfg set ddns_host_status_1 $HOST_ERROR
                  syscfg set ddns_client_Lasterror $AUTHENTICATION_ERROR
                  syscfg commit
                  continue
              fi

              cat /dev/null  > $GENERAL_FILE
              if [ -f "/var/tmp/ipupdate.${ddns_service_name_mod}" ]; then
                  rm /var/tmp/ipupdate."${ddns_service_name_mod}"
              fi

              if [ "$ddns_service_x" == "dyndns" ]; then
                  ${UPDATE_UTIL} "${EXTRA_PARAMS}" > "$OUTPUT_FILE" 2>&1
                  RET_CODE=$?

                  if [ "0" != "$RET_CODE" ]; then
                      # we clear the wan_last_ipaddr to force us to retry once the error is cleared
                      if [ -n "$SYSCFG_wan_last_ipaddr" ] ; then
                          echo "$PID unset syscfg wan_last_ipaddr"
                          syscfg unset wan_last_ipaddr
                          syscfg commit
                      fi
                      grep -q "error connecting" $OUTPUT_FILE
                      if [ "0" = "$?" ]; then
                         syscfg set ddns_client_Status $CLIENT_ERROR
                         syscfg set ddns_host_status_1 $HOST_ERROR
                         syscfg set ddns_client_Lasterror $CONNECTION_ERROR
                         syscfg commit
                      else
                         grep -q "authentication failure" $OUTPUT_FILE
                         if [ "0" = "$?" ]; then
                            syscfg set ddns_client_Status $CLIENT_ERROR
                            syscfg set ddns_host_status_1 $HOST_ERROR
                            syscfg set ddns_client_Lasterror $AUTHENTICATION_ERROR
                            syscfg commit
                         else
                            grep -q "invalid hostname" $OUTPUT_FILE
                            if [ "0" = "$?" ]; then
                               syscfg set ddns_client_Status $CLIENT_ERROR
                               syscfg set ddns_host_status_1 $HOST_ERROR
                               syscfg set ddns_client_Lasterror $MISCONFIGURATION_ERROR
                               syscfg commit
                            else
                               grep -q "malformed hostname" $OUTPUT_FILE
                               if [ "0" = "$?" ]; then
                                  syscfg set ddns_client_Status $CLIENT_ERROR
                                  syscfg set ddns_host_status_1 $HOST_ERROR
                                  syscfg set ddns_client_Lasterror $MISCONFIGURATION_ERROR
                                  syscfg commit
                               else
                                  syscfg set ddns_client_Status $CLIENT_ERROR
                                  syscfg set ddns_host_status_1 $HOST_ERROR
                                  syscfg set ddns_client_Lasterror $DNS_ERROR
                                  syscfg commit
                               fi
                            fi
                         fi
                      fi
                  else #else of ""0" != "$RET_CODE""
                      syscfg set ddns_client_Status $CLIENT_UPDATED
                      syscfg set ddns_host_status_1 $HOST_REGISTERED
                      syscfg set ddns_client_Lasterror $NO_ERROR
                      sysevent set ddns_return_status"${DnsIdx}" success
                      syscfg commit
                  fi
              else

                   ${UPDATE_UTIL} "${EXTRA_PARAMS}" > $OUTPUT_FILE 2>&1
                   RET_CODE=$?

                   if [ "0" != "$RET_CODE" ]; then
                       grep -q "$connecting1_error" $GENERAL_FILE
                       if [ "0" = "$?" ]; then
                           sysevent set ddns_return_status error-connect
                           sysevent set ddns_return_status"${DnsIdx}" error-connect
                           sysevent set ${SERVICE_NAME}-status started

                           # we clear the wan_last_ipaddr to force us to retry once the error is cleared
                           if [ -n "$SYSCFG_wan_last_ipaddr" ] ; then
                               ulog ddns status "$PID unset syscfg wan_last_ipaddr"
                               syscfg unset wan_last_ipaddr
                               syscfg commit
                           fi

                           sysevent set ddns_failure_time "`get_current_time`"
                           sysevent set ddns_return_status error-connect
                           syscfg set ddns_client_Status $CLIENT_ERROR
                           syscfg set ddns_host_status_1 $HOST_ERROR
                           syscfg set ddns_client_Lasterror $CONNECTION_ERROR
                           syscfg commit

                           continue
                       fi
                       grep -q "$connecting2_error" $GENERAL_FILE
                       if [ "0" = "$?" ]; then
                           syscfg set ddns_client_Status $CLIENT_ERROR
                           syscfg set ddns_host_status_1 $HOST_ERROR
                           syscfg set ddns_client_Lasterror $CONNECTION_ERROR
                           syscfg commit

                           # we clear the wan_last_ipaddr to force us to retry once the error is cleared
                           if [ -n "$SYSCFG_wan_last_ipaddr" ] ; then
                               ulog ddns status "$PID unset syscfg wan_last_ipaddr"
                               syscfg unset wan_last_ipaddr
                               syscfg commit
                           fi


                           continue
                       fi
                       grep -q "$resolve_error" $GENERAL_FILE
                       if [ "0" = "$?" ]; then
                           syscfg set ddns_client_Status $CLIENT_ERROR
                           syscfg set ddns_host_status_1 $HOST_ERROR
                           syscfg set ddns_client_Lasterror $CONNECTION_ERROR
                           syscfg commit
                           # we clear the wan_last_ipaddr to force us to retry once the error is cleared
                           if [ -n "$SYSCFG_wan_last_ipaddr" ] ; then
                               ulog ddns status "$PID unset syscfg wan_last_ipaddr"
                               syscfg unset wan_last_ipaddr
                               syscfg commit
                           fi

                           continue
                       fi
                   fi

                   return_str_name="register_success_${ddns_service_name_mod}"
                   return_str=`eval echo '$'"$return_str_name"`
                   if [ -n "$return_str" ]; then
                       grep -q "$return_str" /var/tmp/ipupdate."${ddns_service_name_mod}"
                       if [ "0" = "$?" ]; then
                           sysevent set ddns_return_status"${DnsIdx}" success
                           syscfg set ddns_client_Status $CLIENT_UPDATED
                           syscfg set ddns_host_status_1 $HOST_REGISTERED
                           syscfg set ddns_client_Lasterror $NO_ERROR
                           syscfg commit
                           RET_CODE=0
                           continue
                       fi
                   fi

                   return_str_name="update_success_${ddns_service_name_mod}"
                   return_str=`eval echo '$'"$return_str_name"`
                   if [ -n "$return_str" ]; then
                       grep -q "$return_str" /var/tmp/ipupdate."${ddns_service_name_mod}"
                       if [ "0" = "$?" ]; then
                           sysevent set ddns_return_status"${DnsIdx}" success
                           syscfg set ddns_client_Status $CLIENT_UPDATED
                           syscfg set ddns_host_status_1 $HOST_REGISTERED
                           syscfg set ddns_client_Lasterror $NO_ERROR
                           syscfg commit
                           RET_CODE=0
                           continue
                       fi
                   fi
                   return_str_name="token_error_${ddns_service_name_mod}"
                   return_str=`eval echo '$'"$return_str_name"`
                   if [ -n "$return_str" ]; then
                       grep -q "$return_str" /var/tmp/ipupdate."${ddns_service_name_mod}"
                       if [ "0" = "$?" ]; then
                           syscfg set ddns_client_Status $CLIENT_ERROR
                           syscfg set ddns_host_status_1 $HOST_ERROR
                           syscfg set ddns_client_Lasterror $AUTHENTICATION_ERROR
                           syscfg commit

                           continue
                       fi
                   fi

                   return_str_name="hostname_error_${ddns_service_name_mod}"
                   return_str=`eval echo '$'"$return_str_name"`
                   if [ -n "$return_str" ]; then
                       grep -q "$return_str" /var/tmp/ipupdate."${ddns_service_name_mod}"
                       if [ "0" = "$?" ]; then
                           syscfg set ddns_client_Status $CLIENT_ERROR
                           syscfg set ddns_host_status_1 $HOST_ERROR
                           syscfg set ddns_client_Lasterror $MISCONFIGURATION_ERROR
                           syscfg commit
                           continue
                       fi
                   fi

                   return_str_name="username_error_${ddns_service_name_mod}"
                   return_str=`eval echo '$'"$return_str_name"`
                   if [ -n "$return_str" ]; then
                       grep -q "$return_str" /var/tmp/ipupdate."${ddns_service_name_mod}"
                       if [ "0" = "$?" ]; then
                           syscfg set ddns_client_Status $CLIENT_ERROR
                           syscfg set ddns_host_status_1 $HOST_ERROR
                           syscfg set ddns_client_Lasterror $AUTHENTICATION_ERROR
                           syscfg commit
                           continue
                       fi
                   fi
                   return_str_name="password_error_${ddns_service_name_mod}"
                   return_str=`eval echo '$'"$return_str_name"`
                   if [ -n "$return_str" ]; then
                       grep -q "$return_str" /var/tmp/ipupdate."${ddns_service_name_mod}"
                       if [ "0" = "$?" ]; then
                            syscfg set ddns_client_Status $CLIENT_ERROR
                            syscfg set ddns_host_status_1 $HOST_ERROR
                            syscfg set ddns_client_Lasterror $AUTHENTICATION_ERROR
                            syscfg commit
                            continue
                       fi
                   fi

                   return_str_name="KO"
                   grep -q "$return_str" /var/tmp/ipupdate.${ddns_service_name_mod}
                   if [ "0" = "$?" ]; then
                       syscfg set ddns_client_Status $CLIENT_ERROR
                       syscfg set ddns_host_status_1 $HOST_ERROR
                       syscfg set ddns_client_Lasterror $AUTHENTICATION_ERROR
                       syscfg commit
                   fi

                   return_str_name="general_error_${ddns_service_name_mod}"
                   return_str=`eval echo '$'"$return_str_name"`
                   if [ -n "$return_str" ]; then
                       grep -q "$return_str" /var/tmp/ipupdate."${ddns_service_name_mod}"
                       if [ "0" = "$?" ]; then
                           #sysevent set ddns_return_status error-auth
                           #echo "###### sysevent set ddns_return_status error-auth"
                           syscfg set ddns_client_Status $CLIENT_ERROR
                           syscfg set ddns_host_status_1 $HOST_ERROR
                           syscfg set ddns_client_Lasterror $AUTHENTICATION_ERROR
                           syscfg commit
                           continue
                       fi
                   fi

              fi #end of the provider classify
         fi #end of the un-supported provider

         # Empty the trace file after populating required data
         if [ -f "$GENERAL_FILE" ]; then
            cat /dev/null  > $GENERAL_FILE
         fi
   fi #end of "[ "1" = "$ddns_enable_x" ]"
          


   #If there is a success in one instance, then consider the ddns_return_status
   #as success, to trigger the update in update_ddns_if_needed if need.
   ddns_enable_x="$DDNS_ENABLE"
   if [ "1" = "$ddns_enable_x" ]; then
       tmp_status=`sysevent get ddns_return_status"${DnsIdx}"`
       if [ "success" = "$tmp_status" ] ; then
           /etc/utopia/service.d/service_ddns/ddns_success.sh
           sysevent set ddns_failure_time 0
           sysevent set ddns_updated_time "`date "+%s"`"
           sysevent set ddns_return_status"${DnsIdx}" success
           syscfg set ddns_client_Status $CLIENT_UPDATED
           syscfg set ddns_host_status_1 $HOST_REGISTERED
           syscfg set ddns_client_Lasterror $NO_ERROR
           syscfg set ddns_host_lastupdate_1 "`get_current_time`"
           syscfg commit
           addCron "* * * * *  /etc/utopia/service.d/service_dynamic_dns.sh dynamic_dns-check"
           break
       fi
   fi

   #If there is no error-connect for any provider, then delete the $RETRY_SOON_FILENAME.
   RETRY_SOON_NEEDED=0
   ddns_enable_x=`syscfg get ddns_server_enable_"${DnsIdx}"`
   if [ "1" = "$ddns_enable_x" ]; then
       tmp_status=`sysevent get ddns_return_status"${DnsIdx}"`
       if [ "error-connect" = "$tmp_status" ] ; then
           sysevent set ddns_return_status
           sysevent set ddns_failure_time "`date "+%s"`"
           sysevent set ddns_updated_time
           RETRY_SOON_NEEDED=1
           break
       fi
   fi
   if [ "0" = "$RETRY_SONN_NEEDED" ]; then
        removeCron "/etc/utopia/service.d/service_dynamic_dns.sh dynamic_dns-retry"
   fi
}

#---------------------------------------------------------------------------------------
# do_start
#---------------------------------------------------------------------------------------
do_start() {

   UPDATE_NEEDED=0
   # 1. Our wan ip address has changed
   if [ -z "$SYSCFG_wan_last_ipaddr" ] ; then
      WAN_LAST_IPADDR=0.0.0.0
   else
      WAN_LAST_IPADDR=$SYSCFG_wan_last_ipaddr
   fi
   CURRENT_WAN_IPADDR=`sysevent get current_wan_ipaddr`
   DnsIdx=`echo ${DDNS_CLIENT_SET_SERVER} | cut -d"." -f4`
   CURRENT_RETURN_STATUS=`sysevent get ddns_return_status${DnsIdx}`

   if [ -z "$SYSCFG_wan_last_ipaddr" ] ; then
      WAN_LAST_IPADDR=0.0.0.0
   fi
   if ([ -z "$CURRENT_WAN_IPADDR" ] || [ "0.0.0.0" = "$CURRENT_WAN_IPADDR" ]); then
      ulog ddns status "$PID current wan ip addr is 0.0.0.0"
      return 0
   fi
   if [ "0.0.0.0" = "$WAN_LAST_IPADDR" ] ; then
      UPDATE_NEEDED=1
      ulog ddns status "$PID ddns update required due to no previous update"
   elif [ "$WAN_LAST_IPADDR" !=  "$CURRENT_WAN_IPADDR" ] ; then
        UPDATE_NEEDED=1
        ulog ddns status "$PID ddns update required due change in wan ip address from $WAN_LAST_IPADDR to $CURRENT_WAN_IPADDR"

   elif [ "$WAN_LAST_IPADDR" =  "$CURRENT_WAN_IPADDR" ] && [ "$CURRENT_RETURN_STATUS" != "success" ] ; then
        UPDATE_NEEDED=1
        ulog ddns status "$PID ddns update required due to the startup of the router even if no change in wan ip address from $WAN_LAST_IPADDR to $CURRENT_WAN_IPADDR"
   fi

   ulog ddns status "$PID ddns update required status is $UPDATE_NEEDED"
   if [ "1" = "$UPDATE_NEEDED" ] ; then
      sysevent set ddns_return_status success
      syscfg set ddns_client_Status $CLIENT_CONNECTING
      syscfg set ddns_host_status_1 $HOST_UPDATING
      syscfg commit
      # ----------------------------------------------------------------------
      # If updating_ddns_server.txt exists then a query to the DDNS server is
      # already in progress. Wait for it to complete.
      # ----------------------------------------------------------------------
      while [ -f "/var/tmp/updating_ddns_server.txt" ]; do sleep 2; done
      touch "/var/tmp/updating_ddns_server.txt"
      update_ddns_server "$CURRENT_WAN_IPADDR"
      rm "/var/tmp/updating_ddns_server.txt"

   else
      # if no update needed, consider it as ddns "success"
      ulog ddns status "$PID No ddns update is required at this time"
      sysevent set ddns_return_status success
#      syscfg set ddns_client_Status 1
#      syscfg set ddns_host_status_1 3
      syscfg commit
   fi

#Retry Mechanism
   if [ "$RETRY_SOON_NEEDED" == "1" ] ; then
       addCron "* * * * *  /etc/utopia/service.d/service_dynamic_dns.sh dynamic_dns-retry"
   fi
}



update_ddns_if_needed () {
   WAN_LAST_IPADDR=$SYSCFG_wan_last_ipaddr
   CURRENT_WAN_IPADDR=`sysevent get current_wan_ipaddr`
   CURRENT_STATE=`sysevent get wan-status`
   if [ "started" != "$CURRENT_STATE" ] ; then
      CURRENT_WAN_IPADDR="0.0.0.0"
   fi

   case "$CURRENT_WAN_IPADDR" in
      0.0.0.0)
        ulog ddns status "$PID wan state is down. No ddns update possible"
           sysevent set ddns_return_status

        ;;
   *)
      PRIORERROR=`sysevent get ddns_return_status`
      service_ddns_enable="$DDNS_ENABLE"
      if [ "0" != "$service_ddns_enable" ] ; then
         if [ -z "$PRIORERROR" ] || [ "success" = "$PRIORERROR" ] ; then
            # if the wan ip address changed, then the system requires a few secs to stabilize
            # eg. firewall needs to be reset. Give it a few secs to do so
            if [ "$WAN_LAST_IPADDR" != "$CURRENT_WAN_IPADDR" ] ; then
                sleep 5
                ulog ddns status "$PID Current wan ip address is $CURRENT_WAN_IPADDR. Continuing"
            fi
            do_start
         else
            syscfg set ddns_client_Status $CLIENT_ERROR
            syscfg set ddns_host_status_1 $HOST_ERROR
            syscfg set ddns_client_Lasterror $TIMEOUT_ERROR
            syscfg commit
         fi
      else
         ulog ddns status "$PID ddns is not enabled"
         syscfg set ddns_client_Status $CLIENT_DISABLED
         syscfg set ddns_host_status_1 $HOST_DISABLED
         syscfg set ddns_client_Lasterror $MISCONFIGURATION_ERROR
         syscfg commit
      fi
      ;;
   esac
}


#service_check_interval
service_check_interval ()
{
    LAST_UPDATE_TIME=`sysevent get ddns_updated_time`
    CURRENT_TIME=`date "+%s"`
    DELTA=`expr "$CURRENT_TIME" - "$LAST_UPDATE_TIME"`
    DnsIdx=`echo "${DDNS_CLIENT_SET_SERVER}" | cut -d"." -f4`
    ddns_enable_x=`syscfg get ddns_server_enable_"$DnsIdx"`
    if [ "$ddns_enable_x" == "1" ]; then
        check_interval=`syscfg get ddns_server_checkinterval_"${DnsIdx}"`
        if [ "$check_interval" != "0" ] && [ "$check_interval" -le "$DELTA" ]; then
            do_start
        fi
    fi
}


#service_retry
service_retry ()
{

    FAILURE_TIME=`sysevent get ddns_failure_time`
    CURRENT_TIME=`date "+%s"`
    DELTA=`expr "$CURRENT_TIME" - "$FAILURE_TIME"`
    DnsIdx=`echo "${DDNS_CLIENT_SET_SERVER}" | cut -d"." -f4`
    ddns_enable_x=`syscfg get ddns_server_enable_"$DnsIdx"`
    if [ "1" = "$ddns_enable_x" ]; then
        retry_interval=`syscfg get ddns_server_retryinterval_"${DnsIdx}"`
        if [ "$retry_interval" -le 60 ]; then
            retry_interval=60
        else
            modulus=`expr $retry_interval % 60`
            quotient=`expr $retry_interval / 60`
            if [ "$modulus" -ge 30 ]; then
                retry_interval=`expr $retry_interval + "$modulus"`
            else
                retry_interval=`expr "$retry_interval" - "$modulus"`
            fi
        fi
        max_retries=`syscfg get ddns_server_maxretries_"${DnsIdx}"`
        SYSCFG_max_retries=`sysevent get ddns_check_maxretries`
        if [ "$FAILURE_TIME" != "0" ] && [ "$retry_interval" -le "$DELTA" ] && [ "$SYSCFG_max_retries" -lt "$max_retries" ]; then
             SYSCFG_max_retries=`expr "$SYSCFG_max_retries" + 1`
             sysevent set ddns_check_maxretries "$SYSCFG_max_retries"
             sysevent set ddns_return_status
             do_start
        fi
    fi

}


#----------------------------------------------------------------------
# service_init
#----------------------------------------------------------------------
service_init ()
{
    FOO=`utctx_cmd get wan_last_ipaddr`
    eval "$FOO"
}


#----------------------------------------------------------------------
#  service_start
#  ddns periodically checks to see if it should update the ddns server
#  ddns-start is called by cron periodically
#----------------------------------------------------------------------
service_start ()
{
   sysevent set ddns_return_status
   sysevent set ddns_check_maxretries 0
   sysevent set ddns_updated_time 0
   sysevent set ddns_failure_time 0

   DSLITE_ENABLE=`syscfg get dslite_enable`
   if [ "$DSLITE_ENABLE" == "1" ] || [ "$DDNS_ENABLE" == "0" ]; then
      exit 0
   fi

   syscfg set ddns_client_Status $CLIENT_CONNECTING
   syscfg set ddns_host_status_1 $HOST_UPDATE_NEEDED
   syscfg commit
   update_ddns_if_needed
}

service_stop ()
{
    ulog ddns status "$PID ddns status update is not required"
    #syscfg set ddns_client_Status $CLIENT_UPDATED
    #syscfg set ddns_host_status_1 $HOST_REGISTERED
    #syscfg commit
}


service_init
echo "service_dynamic_dns: $1 $2"

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
   "${SERVICE_NAME}-retry")
      service_retry
      ;;
   "${SERVICE_NAME}-check")
      service_check_interval
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
