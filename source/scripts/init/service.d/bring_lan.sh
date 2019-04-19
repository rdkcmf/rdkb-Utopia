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
. /etc/device.properties
echo "bring_lan.sh script is called setting bring-lan to up" > /dev/console
sysevent set bring-lan up

#BOX_TYPE=`cat /etc/device.properties | grep BOX_TYPE | cut -f2 -d=`
BRIDGE_MODE=`syscfg get bridge_mode`
if [ "$BOX_TYPE" = "XB3" ] && [ "$BRIDGE_MODE" = "0" ]; then
    echo "XB3 case:Router mode: Start brlan0 initialization" > /dev/console
    sysevent set multinet-up 1
else
    echo "brlan0 initialization for non-XB3 platforms and in bridge-mode is done in service_ipv4.sh"
fi
