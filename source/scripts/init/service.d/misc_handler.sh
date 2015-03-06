#! /bin/sh
UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`
if [ "$UPTIME" -lt 600 ]
then
    exit 0
fi
LAN_WAN_READY=`sysevent get start-misc`
if [ "$LAN_WAN_READY" != "ready" ]
then
    sysevent set start-misc ready

    STARTED_FLG=`sysevent get parcon_nfq_status`

    if [ x"$STARTED_FLG" != x"started" ]; then
        BRLAN0_MAC=`ifconfig l2sd0 | grep HWaddr | awk '{print $5}'`
        ((nfq_handler 4 $BRLAN0_MAC &)&)
        ((nfq_handler 6 $BRLAN0_MAC &)&)
        sysevent set parcon_nfq_status started
    fi

    rm /etc/cron/cron.everyminute/misc_handler.sh
else
    rm /etc/cron/cron.everyminute/misc_handler.sh
fi
