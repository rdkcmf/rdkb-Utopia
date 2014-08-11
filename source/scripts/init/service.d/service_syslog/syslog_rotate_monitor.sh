#!/bin/sh

# This script is called everyminute by cron
# monitor the size of /var/log/messsages and if syslogd has rotated it then signal this via sysevent


LOG_FILE="/var/log/messages"
SYSEVENT_TUPLE="logsize_size"

SIZE=`ls -l /var/log/messages | awk '{print $5}'`
OLDSIZE=`sysevent get $SYSEVENT_TUPLE`

if [ "" = "$OLDSIZE" ] ; then
   OLDSIZE=0
fi

if [ "$OLDSIZE" -gt "$SIZE" ] ; then
   sysevent set syslog_rotated
fi

sysevent set $SYSEVENT_TUPLE $SIZE
