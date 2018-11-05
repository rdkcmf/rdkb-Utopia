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

# This is called by ez-ipupdate upon successful update of the
# DDNS Server

source /etc/utopia/service.d/date_functions.sh
source /etc/utopia/service.d/ulog_functions.sh


DDNS_HOSTNAME=`syscfg get ddns_hostname`
DDNS_SERVICE=`syscfg get ddns_service`
ulog ddns status "ddns client reports successful update of $DDNS_HOSTNAME with service $DDNS_SERVICE"
WAN_IFADDR=`sysevent get current_wan_ipaddr`

syscfg set wan_last_ipaddr "$WAN_IFADDR"

CURRENT_TIME=`get_current_time`
syscfg set ddns_last_update "$CURRENT_TIME"
syscfg commit
