#/bin/sh

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
# This script monitors /var/log/messages for utopia triggers.
# When it finds them it will inform the trigger handler
#
# ----------------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="firewall_trigger_monitor"
TRIGGER_HANDLER=trigger

service_start()
{
   PID=$$
   ulog triggers status "$PID starting trigger monitoring process"
   if [ -z `pidof trigger` ] ; then
      $TRIGGER_HANDLER
   fi
   sysevent set ${SERVICE_NAME}-status started
}

service_stop() {
   killall $TRIGGER_HANDLER
   sysevent set ${SERVICE_NAME}-status stopped
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
   *)
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" >&2
      exit 3
      ;;
esac
