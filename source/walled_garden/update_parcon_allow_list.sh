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

PARCON_EXPIRE_TIME=`sysevent get parcon_expire_time`

MAC=`ip neighbor show | awk "/$1/"'{print $5}'`
TIME=`date '+%Y %m %d %H %M 00'`
sed "/$MAC/d" -i /var/.parcon_allow_list
echo "$MAC,$TIME" >> /var/.parcon_allow_list

# add one-time cron job for aging
sed "/$MAC TPARCON/d" -i /var/spool/cron/crontabs/root
date -D "%s" -d "$(( `busybox date +%s`+$PARCON_EXPIRE_TIME+10 ))" "+%M %H %d %m * parcon_aging.sh T$TIME T$MAC TPARCON" >> /var/spool/cron/crontabs/root
sysevent set crond-restart 1

# reload all iptables rules
firewall

awk -F "[ =]+" /tcp.*"$1"/'{system("conntrack_delete "$8" "$10" "$12" "$14)}' /proc/net/nf_conntrack
