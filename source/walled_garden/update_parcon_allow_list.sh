#!/bin/sh

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

awk -F "[ =]+" /tcp.*$1/'{system("conntrack_delete "$8" "$10" "$12" "$14)}' /proc/net/nf_conntrack
