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


# This script is called everyminute by cron
# monitor the size of /var/log/messsages and if syslogd has rotated it then signal this via sysevent


LOG_FILE="/var/log/messages"
SYSEVENT_TUPLE="logsize_size"

SIZE=`ls -l /var/log/messages | awk '{print $5}'`
OLDSIZE=`sysevent get $SYSEVENT_TUPLE`

if [ -z "$OLDSIZE" ] ; then
   OLDSIZE=0
fi

if [ "$OLDSIZE" -gt "$SIZE" ] ; then
   sysevent set syslog_rotated
fi

sysevent set $SYSEVENT_TUPLE "$SIZE"
