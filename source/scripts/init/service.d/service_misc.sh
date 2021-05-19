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

echo "*******************************************************************"
echo "*                                                                  "
echo "* Copyright 2014 Cisco Systems, Inc. 				 "
echo "* Licensed under the Apache License, Version 2.0                   "
echo "*                                                                  "
echo "*******************************************************************"
cd /etc/utopia/service.d
UPTIME=$(cut -d. -f1 /proc/uptime)
#. /etc/config
LAN_ST=`sysevent get lan-status`
#WAN_ST=`sysevent get wan-status`
BR_MD=`sysevent get bridge_mode`
RG_MD=`syscfg get last_erouter_mode`

if [ -f /etc/utopia/service.d/cosa_misc.sh ];then
	echo "running cosa_misc.sh script"
	/etc/utopia/service.d/cosa_misc.sh&
else
	echo "cosa_misc.sh is not found"
fi
/etc/utopia/service.d/service_igd.sh snmp_subagent-status
#unit in bridge mode
if [ "$BR_MD" != "0" -o "$RG_MD" = "0" ]; then
#    execute_dir /etc/utopia/post.d/
	firewall
	exit 0;
fi

#when no cable plug in, timeout happened, we still need register the sysevent
#if [ "$UPTIME" -gt 600 ]; then
#    execute_dir /etc/utopia/post.d/ restart
#fi

if [ "$LAN_ST" != "started" ]; then
	exit 0;
fi

if [ "$RG_MD" = "1" -o "$RG_MD" = "3" ]; then
	/etc/utopia/service.d/service_igd.sh lan-status&
fi

if [ "$RG_MD" = "1" -o "$RG_MD" = "3" ]; then
	/etc/utopia/service.d/service_mcastproxy.sh lan-status&
fi

if [ "$RG_MD" = "2" -o "$RG_MD" = "3" ]; then
	/etc/utopia/service.d/service_mldproxy.sh lan-status&
fi

#start firewall first
#firewall $*
#force lan client to start DHCP discover now
#gw_lan_refresh
#start others
#if [ "started" = "$LAN_ST" ] ; then
#	/etc/utopia/service.d/service_igd.sh snmp_subagent-status
#	/etc/utopia/service.d/service_igd.sh lan-status&
	
#	if [ "y" = "$CONFIG_CISCO_WECB" ] ; then
#	    /etc/utopia/service.d/service_wecb.sh snmp_subagent-status&
#	    /etc/utopia/service.d/service_wecb.sh wecb-start&
#	fi
#fi
#BR_MODE=`syscfg get bridge_mode`
#EROUTER_MODE=`syscfg get last_erouter_mode`
#if [ 0 = "$BR_MODE" ] && [ 0 != "$EROUTER_MODE" ]  && [ 1 != "$EROUTER_MODE" ]; then
#    /etc/utopia/service.d/service_mldproxy.sh wan-status&
#fi

#start igmpproxy
#if [ 0 = "$BR_MODE" ] && [ 0 != "$EROUTER_MODE" ]  && [ 2 != "$EROUTER_MODE" ]; then
#    /etc/utopia/service.d/service_mcastproxy.sh wan-status&
#fi
