#!/bin/sh
#######################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2019 RDK Management

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#######################################################################################

source /etc/log_timestamp.sh    # define 'echo_t' ASAP!

if [ -f /etc/device.properties ]; then
	source /etc/device.properties
fi

if [ -f /etc/utopia/service.d/device_time_sync_peer_wrapper.sh ]; then
	source /etc/utopia/service.d/device_time_sync_peer_wrapper.sh
else
	echo_t "RDKB_SYSTEM_BOOT_UP_LOG : service_time_sync_peer.sh script called after ntp time sync but EXITING due to no Device Wrapper Function" >> "$NTPD_LOG_NAME"
	return 0
fi

#Local variables
SERVICE_NAME="time_sync_peer"
SELF_NAME="`basename "$0"`"
SLEEP_SEC=1
START_YEAR=1970
LOCKFILE="/var/run/time_sync_peer.pid"
CRONFILE=$CRON_SPOOL"/root"
CRONFILE_BK="/tmp/cron_tab$$.txt"

do_time_sync_with_peer() {
    (
    echo $! > ${LOCKFILE}
        while true; do
            if [ ! -f $LOCKFILE ]; then break; fi;

                #Get date from RDKB Stack Processor in the form of seconds since the epoch
                timestamp=`date +"%s"`
                #Convert seconds since epoch to year
                year=`date --date="@${timestamp}" +"%Y"`
                #Check if the extracted year is greater than 1970 (assuming the system time will be updated by ntpd)
                if [ "$year" -gt $START_YEAR ]; then
                    timeprint=$timestamp
                    #notify time sync event to Peer for system time update
                    echo_t "Notify system time update of $timeprint to Peer" >> "$NTPD_LOG_NAME"
                    device_time_sync_peer
                    sysevent set ${SERVICE_NAME}-status "started"
                    break;
                else
                    sleep $SLEEP_SEC
                fi
        done
        rm -rf ${LOCKFILE} > /dev/null 2>&1
    ) &
}

service_start ()
{
   do_time_sync_with_peer
}


service_stop ()
{
       sysevent set ${SERVICE_NAME}-status "stopped"
}

service_init ()
{
    if [ -f "$CRONFILE" ]
    then
    	# Dump existing cron jobs to a file & add new job
    	crontab -l -c "$CRON_SPOOL" > $CRONFILE_BK

    	# Check whether specific cron jobs are existing or not
    	existingcron=$(grep "time_sync_peer-restart" $CRONFILE_BK)

    	if [ -z "$existingcron" ]; then
    		echo "*/30 * * * * sysevent set time_sync_peer-restart" >> $CRONFILE_BK

    		echo_t "RDKB_SYSTEM_BOOT_UP_LOG : service_time_sync_peer.sh setting periodic sync" >> "$NTPD_LOG_NAME"
    		crontab $CRONFILE_BK -c "$CRON_SPOOL"
    	else
    		echo_t "service_time_sync_peer.sh periodic sync already setup" >> "$NTPD_LOG_NAME"
    	fi

    	rm -rf $CRONFILE_BK
    fi
}

# Entry

case "$1" in
  "${SERVICE_NAME}-start")
      service_init
      service_start
      ;;
  "${SERVICE_NAME}-stop")
      service_stop
      ;;
  "${SERVICE_NAME}-restart")
      service_stop
      service_start
      ;;
  ntp_time_sync)
      CURRENT_NTPD_STATUS=`sysevent get ntp_time_sync`

      if [ "1" = "$CURRENT_NTPD_STATUS" ] ; then
         echo_t "RDKB_SYSTEM_BOOT_UP_LOG : service_time_sync_peer.sh called after ntp time sync" >> "$NTPD_LOG_NAME"
         service_init
         service_start
      fi
      ;;
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | ntp_time_sync ]" >&2
      exit 3
      ;;
esac
