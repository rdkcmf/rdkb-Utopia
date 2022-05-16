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
# This script is used to start/stop wecb_master
#------------------------------------------------------------------
SERVICE_NAME="wecb"
source /etc/device.properties
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/ut_plat.sh

if [ "$BOX_TYPE" = "XB6" ] && [ "$MANUFACTURE" = "Arris" ]; then
	export LOG4C_RCPATH=/etc
else
	export LOG4C_RCPATH=/rdklogger
fi

SELF_NAME="`basename "$0"`"

service_start() {

	INST=`sysevent get primary_lan_l3net`
    if [ x"`sysevent get ipv4_"${INST}"-status`" = x"$L3_UP_STATUS"  -a x"`sysevent get ${SERVICE_NAME}-status`" != x"started"  -a x"completed" = x"`sysevent get moca_init`" ] ; then
        ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service"
	    ulimit -s 1024 && wecb_master&
		echo '#!/bin/sh' > /var/volatile/wecb_master.sh
		echo 'ulimit -s 1024 && wecb_master&' >> /var/volatile/wecb_master.sh 
		chmod +x /var/volatile/wecb_master.sh
		/etc/utopia/service.d/pmon.sh register wecb_master
		/etc/utopia/service.d/pmon.sh setproc wecb_master wecb_master /var/run/wecb_master.pid "/var/volatile/wecb_master.sh" 
        sysevent set ${SERVICE_NAME}-errinfo
        sysevent set ${SERVICE_NAME}-status "started"
    fi
}

service_stop () {
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
   #unregister wecb_master from pmon to let this script to bring it up when lan restart.
   /etc/utopia/service.d/pmon.sh unregister wecb_master 
   #rongwei added
   kill "`pidof wecb_master`"
   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

service_init() {
	#sysevent set ${SERVICE_NAME}-status "init"
    echo "swctl commands to set Multicast MAC is done as part of service_multinet"
}

handle_ipv4_status() {

	if [ x"$1" = x"`sysevent get primary_lan_l3net`" ] && [ x"completed" = x"`sysevent get moca_init`" ]; then
		if [ x"$L3_UP_STATUS" = x"$2" ]; then
			service_start;
		else
			service_stop;
		fi
	fi
}

#---------------------------------------------------------------

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
  lan-status)
      ;;
  multinet_1-status)
   ;;
  snmp_subagent-status)
        service_init
   ;;
   ipv4_*-status)
        INST=${1%-*}
        INST=${INST#*_}
        handle_ipv4_status "$INST" "$2"
        ;;
   moca_init)
	service_start
        ;;
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart|lan-status]" >&2
      exit 3
      ;;
esac



