#!/bin/sh 
#

#start log module

LOG_LEVEL_FILE=/nvram/syslog_level
LOG_CONF_FILE=/nvram/syslog.conf
SYSLOG_CONF_FILE=/etc/syslog.conf
SYSLOG_DEFAULT_CONF_FILE=/etc/syslog.conf_default
if [ -e $LOG_CONF_FILE ]
then
    ln -s $LOG_CONF_FILE $SYSLOG_CONF_FILE
else
    ln -s $SYSLOG_DEFAULT_CONF_FILE $SYSLOG_CONF_FILE
fi

if [ -e $LOG_LEVEL_FILE ]
then 
    level=`awk '$1 <= 8 && $1 >= 1' $LOG_LEVEL_FILE`
else
    level=6
fi

SYSTEMLOG=$(grep -e "systemlog" /etc/syslog.conf | awk '{print $2}') 
EVENTLOG=$(grep -e "eventlog" /etc/syslog.conf | awk '{print $2}')

#if [ -e $SYSTEMLOG ]
#then
#    cat $SYSTEMLOG >> $SYSTEMLOG.0
#    rm $SYSTEMLOG
#else
    SYSTEMLOG_DIR=${SYSTEMLOG%/*}
    if [ ! -d $SYSTEMLOG_DIR ]
    then
        mkdir -p $SYSTEMLOG_DIR
    fi
#fi

#if [ -e $EVENTLOG ]
#then
#    cat $EVENTLOG >> $EVENTLOG.0
#    rm $EVENTLOG
#else
    EVENTLOG_DIR=${EVENTLOG%/*}
    if [ ! -d $EVENTLOG_DIR ]
    then
	echo "mkdir -p $EVENTLOG_DIR"
        mkdir -p $EVENTLOG_DIR
    fi
#fi

nice -5 klogd -c $level 
nice -5 syslogd -l $level

