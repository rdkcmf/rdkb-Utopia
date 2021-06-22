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


LOG_LEVEL_FILE=/nvram/syslog_level
LOG_CONF_FILE=/nvram/syslog.conf
SYSLOG_CONF_FILE=/etc/syslog.conf
SYSLOG_DEFAULT_CONF_FILE=/etc/syslog.conf_default

DOCSIS_FACILITY=local6
KERN_FACILITY=kern
get_log_level(){
    echo "Please input the log level"
    echo " 1. LOG_EMERG"
    echo " 2. LOG_ALERT"
    echo " 3. LOG_CRIT"
    echo " 4. LOG_ERR"
    echo " 5. LOG_WARNING"
    echo " 6. LOG_NOTICE"
    echo " 7. LOG_INFO"
    echo " 8. LOG_DEBUG"
    read log_level
    
    if [ "$log_level" -eq 1 ];then
        level="emerg"
    elif [ "$log_level" -eq 2 ];then
        level="alert"
    elif [ "$log_level" -eq 3 ];then
    	level="crit"
    elif [ "$log_level" -eq 4 ];then
        level="err"
    elif [ "$log_level" -eq 5 ];then
    	level="warn"
    elif [ "$log_level" -eq 6 ];then
    	level="notice"
    elif [ "$log_level" -eq 7 ];then
        level="info"
    elif [ "$log_level" -eq 8 ];then
    	level="debug"
    else
    	level="*"
    fi
}

start_syslog(){
    if [ -e $SYSLOG_CONF_FILE ]
    then
        rm $SYSLOG_CONF_FILE
    fi

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
    echo "klogd -c $level"
    klogd -c $level    
    echo "syslogd -l $level"
    syslogd -l $level
}

set_docsis_target(){
    echo "Please input the Docsis log target,press Enter to set default value "
    echo "1. /var/log/docsis(default)"
    echo "2. console"
    echo "3. other path"
    read target
    
    get_log_level 
    
    if [ "$target" == "2" ];then
        path="/dev/console"
    elif [ "$target" == "3" ];then 
        echo "Input path"
        read path
    else
        path="/var/log/docsis"
    fi
    echo "$DOCSIS_FACILITY.$level $path" 
    echo "$DOCSIS_FACILITY.$level $path" >> $LOG_CONF_FILE
    echo "Done"
}

set_kern_target(){
    echo "Please input Kernel log target,press Enter to set default value"
    echo "1. /var/log/kernel(default)"
    echo "2. others"
    read target

    if [ "$target" == "2" ];then
        echo "Input path"
        read path
    else
        path="/var/log/kernel"
    fi
    get_log_level
    echo "$KERN_FACILITY.$level  $path" >> $LOG_CONF_FILE
    echo "Done"
}

ask_reboot(){
    echo "Re-start syslog ? (y/N)"
    read is_restart
    if [ "$is_restart" == "y" ]
    then
        echo "kill syslogd"     
        killall syslogd
        echo "kill klogd"     
        killall klogd
        start_syslog
    fi
}

echo "Please input the number:"
echo "1. Setting the syslog level"
echo "2. Setting the syslog target"
echo "3. reboot the syslog"

read op

if [ "$op" -eq 1 ]
then
    echo "Please input the syslog Level"
    echo " 1. LOG_EMERG"
    echo " 2. LOG_ALERT"
    echo " 3. LOG_CRIT"
    echo " 4. LOG_ERR"
    echo " 5. LOG_WARNING"
    echo " 6. LOG_NOTICE"
    echo " 7. LOG_INFO"
    echo " 8. LOG_DEBUG"
    read log_level
    if [ "$log_level" -ge 1 ] && [ "$log_level" -lt 9 ]
    then
        echo "$log_level" > $LOG_LEVEL_FILE
        echo "Done"
    else
        echo "Wrong Number"
    fi    
    ask_reboot
fi

if [ "$op" -eq 2 ]
then
    echo ""
    echo "" > $LOG_CONF_FILE
    set_docsis_target
    echo ""
    set_kern_target
    ask_reboot
fi

if [ "$op" -eq 3 ]
then
    killall syslogd
    killall klogd
    start_syslog
fi
