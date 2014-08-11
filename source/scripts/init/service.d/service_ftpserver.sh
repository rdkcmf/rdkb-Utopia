#!/bin/sh

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

#------------------------------------------------------------------
# This script is used to start ftp daemon
# $1 is the calling event (ftpserver-restart, lan-status, wan-status, etc)
#------------------------------------------------------------------
source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="ftpserver"
SELF_NAME="`basename $0`"

service_init ()
{
    eval `utctx_cmd get current_lan_ipaddr`
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
   tinyftp -d -s $LAN_IPADDR -p 21 -c $DIR_NAME &

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

