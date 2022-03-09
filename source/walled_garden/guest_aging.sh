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

# prepare allow list
EXPIRED_CLIENTS="/var/.guest_expired_clients"
GUEST_EXPIRE_TIME=`sysevent get guest_expire_time`
awk -F, -v expire="$GUEST_EXPIRE_TIME" 'mktime($2)+expire<=systime(){print $1" "$3}' /var/.guest_allow_list > $EXPIRED_CLIENTS
awk -F, -v expire="$GUEST_EXPIRE_TIME" 'mktime($2)+expire>systime()' /var/.guest_allow_list > /var/.guest_allow_list_tmp

rm /var/.guest_allow_list
mv /var/.guest_allow_list_tmp /var/.guest_allow_list

# reload iptables rules before deleting conntrack entries
firewall

# delete conntrack entries
#for line in $(cat $EXPIRED_CLIENTS); do
cat $EXPIRED_CLIENTS | while read line; do
    IP=`echo -n "$line" | awk '{print $2}'`
    iptables -I FORWARD 1 -s "$IP" -p tcp --dport 80 -j REJECT
    iptables -I FORWARD 1 -s "$IP" -p tcp --dport 443 -j REJECT
    awk -F "[ =]+" /tcp.*"$IP"/'{system("conntrack_delete "$8" "$10" "$12" "$14)}' /proc/net/nf_conntrack
    iptables -D FORWARD -s "$IP" -p tcp --dport 80 -j REJECT
    iptables -D FORWARD -s "$IP" -p tcp --dport 443 -j REJECT
done

# delete cron entries
CRON_DIR="/var/spool/cron/crontabs"
ROOT_CRON="root"
TMP_FILE="tmp$$.root"

awk '!/guest_aging/' /var/spool/cron/crontabs/root > $CRON_DIR/$TMP_FILE
awk -FT -v expire="$GUEST_EXPIRE_TIME" '/guest_aging/ && mktime($2)+expire>systime()' /var/spool/cron/crontabs/root >> $CRON_DIR/$TMP_FILE

rm $CRON_DIR/$ROOT_CRON
mv $CRON_DIR/$TMP_FILE $CRON_DIR/$ROOT_CRON

sysevent set crond-restart 1
