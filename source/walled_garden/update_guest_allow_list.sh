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

GUEST_EXPIRE_TIME=`sysevent get guest_expire_time`
MAX_GUEST=`syscfg get guest_max_num`
ALLOW_LIST=/var/.guest_allow_list
ALLOW_LIST_TMP=${ALLOW_LIST}_tmp

MAC=`ip neighbor show | awk "/$1/"'{print $5}'`
TIME=`date '+%Y %m %d %H %M 00'`
sed "/$MAC/d" -i $ALLOW_LIST
echo "$MAC,$TIME,$1" >> $ALLOW_LIST
tail -n "$MAX_GUEST" $ALLOW_LIST > $ALLOW_LIST_TMP
rm $ALLOW_LIST
mv $ALLOW_LIST_TMP $ALLOW_LIST

# add one-time cron job for aging
sed "/$MAC TGUEST/d" -i /var/spool/cron/crontabs/root
date -D "%s" -d "$(( `busybox date +%s`+$GUEST_EXPIRE_TIME+10 ))" "+%M %H %d %m * guest_aging.sh T$TIME T$MAC TGUEST" >> /var/spool/cron/crontabs/root
sysevent set crond-restart 1

# reload all iptables rules
firewall

iptables -I FORWARD 1 -s "$1" -p tcp --dport 80 -j REJECT
iptables -I FORWARD 1 -s "$1" -p tcp --dport 443 -j REJECT
awk -F "[ =]+" /tcp.*"$1"/'{system("conntrack_delete "$8" "$10" "$12" "$14)}' /proc/net/nf_conntrack
iptables -D FORWARD -s "$1" -p tcp --dport 80 -j REJECT
iptables -D FORWARD -s "$1" -p tcp --dport 443 -j REJECT
