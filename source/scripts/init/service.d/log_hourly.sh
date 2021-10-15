#!/bin/sh
nice -n 19 sh /usr/ccsp/tad/log_hourly.sh &
if [ -f /usr/ccsp/rbus_status_logger.sh ]
then
     /usr/ccsp/rbus_status_logger.sh
fi
