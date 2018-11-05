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
# This script is called when the value of <desired_ipv4_link_state, *> changes.
# desired_ipv4_link_state is one of:
#    up             - The system wants to bring the wan ipv4 link up
#    down           - The system wants to bring the wan ipv4 link down
#
# The script is called with one parameter:
#   The value of the parameter is phylink_wan_state if the physical link state has changed
#   and it is desired_state_change if the desired_ipv4_link_state has changed
#
# This script is responsible for bringing up connectivity with
# the ISP using static provisioning.
#
# It is responsible for provisioning the interface IP Address, and the
# routing table. And also /etc/resolv.conf
#
# Upon success it must set:
#    sysevent ipv4_wan_ipaddr
#    sysevent ipv4_wan_subnet
#    sysevent current_ipv4_link_state
#
#------------------------------------------------------------------

source /etc/utopia/service.d/hostname_functions.sh

prepare_hostname
