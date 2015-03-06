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
# This script is used to start/stop Home Security HNAP process
#------------------------------------------------------------------
SERVICE_NAME="ccsphs"
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/ut_plat.sh


SELF_NAME="`basename $0`"

service_start() {

	INST=`sysevent get homesecurity_lan_l3net`	
    if [ x`sysevent get ipv4_${INST}-status` = x$L3_UP_STATUS  -a x`sysevent get ${SERVICE_NAME}-status` != x"started" ] ; then
        ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 
		CcspHomeSecurity 8081&
        sysevent set ${SERVICE_NAME}-errinfo
        sysevent set ${SERVICE_NAME}-status "started"
    fi
}

service_stop () {
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
   kill `pidof CcspHomeSecurity`
   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}


handle_ipv4_status() {
	if [ x$1 = x`sysevent get homesecurity_lan_l3net` ]; then
		if [ x$L3_UP_STATUS = x$2 ]; then
			service_start;
		else
			service_stop;
		fi
	fi
}

service_init() {
	echo
}

#---------------------------------------------------------------

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
      ;;
  multinet_2-status)
      ;;
  ipv4_*-status)
      INST=${1%-*}
      INST=${INST#*_}
      handle_ipv4_status $INST $2
      ;;
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart|lan-status]" >&2
      exit 3
      ;;
esac




