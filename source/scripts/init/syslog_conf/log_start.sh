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
    if [ ! -d "$SYSTEMLOG_DIR" ]
    then
        mkdir -p "$SYSTEMLOG_DIR"
    fi
#fi

#if [ -e $EVENTLOG ]
#then
#    cat $EVENTLOG >> $EVENTLOG.0
#    rm $EVENTLOG
#else
    EVENTLOG_DIR=${EVENTLOG%/*}
    if [ ! -d "$EVENTLOG_DIR" ]
    then
	echo "mkdir -p $EVENTLOG_DIR"
        mkdir -p "$EVENTLOG_DIR"
    fi
#fi

nice -5 klogd -c $level 

if ([ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ]) ; then
	if [ -f /tmp/utopia_inited ] ; then
		systemctl restart syslog
	fi
else
	nice -5 syslogd -l $level
fi
