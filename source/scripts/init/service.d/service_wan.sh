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
# This script sets up the wan handlers.
# Upon completion of bringing up the appropriate wan handler bringing up the wan,
# it is expected that the wan handler will set the wan-status up.
#
# All wan protocols must set the following sysevent tuples
#   current_wan_ifname
#   current_wan_ipaddr
#   current_wan_subnet
#   current_wan_state
#   wan-status
#
# Wan protocols are responsible for setting up:
#   /etc/resolv.conf
# Wan protocols are also responsible for restarting the dhcp server if
# the wan domain or dns servers change
#------------------------------------------------------------------
#--------------------------------------------------------------
#
# This script set the handlers for 2 events:
#    desired_ipv4_link_state
#    desired_ipv4_wan_state
# and then as appropriate sets these events.
# The desired_ipv4_link_state handler establishes ipv4 connectivity to
# the isp (currently dhcp or static).
# The desired_ipv4_wan_state handler uses the connectivity to the isp to
# set up a connection that is used as the ipv4 wan. This connection
# may be dhcp, static, pppoe, l2tp, pptp or in the future other protocols
#--------------------------------------------------------------

source /etc/utopia/service.d/log_capture_path.sh

echo_t "---------------------------------------------------------"
echo_t "-------------------- service_wan.sh ---------------------"
echo_t "---------------------------------------------------------"
echo_t "RDKB_SYSTEM_BOOT_UP_LOG : service_wan.sh script called to bring up WAN INTERFACE"
echo_t ">>>>> $1"
set -x

touch /var/resolv.conf
touch /var/tmp/resolv.conf

SERVICE_NAME=wan

case "$1" in
    "${SERVICE_NAME}-start")
        nice -n -5 service_wan start
        ;;
    "${SERVICE_NAME}-stop")
        service_wan stop
        ;;
    "${SERVICE_NAME}-restart")
        service_wan restart
        ;;
    phylink_wan_state)
        status=`sysevent get phylink_wan_state`
        if [ "up" == "$status" ]; then
            service_wan start
        elif [ "down" == "$status" ]; then
            service_wan stop
        fi
        ;;
    erouter_mode-updated)
        service_wan restart
        ;;

    dhcp_client-restart)
        service_wan dhcp-restart
        ;;

    dhcp_client-release)
        service_wan dhcp-release
        ;;

    dhcp_client-renew)
        service_wan dhcp-renew
        ;;
    *)
        echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
        exit 3
        ;;
esac
