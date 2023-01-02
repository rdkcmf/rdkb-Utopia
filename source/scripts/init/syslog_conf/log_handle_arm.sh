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


V_EVT_LOG_FILE=""
V_SYS_LOG_FILE=""

COMPRESS_CMD_BZ2="tar -cjf "
UNCOMPRESS_CMD_BZ2="tar -xjf "
POSTFIX_BZ2="tar.bz2"

COMPRESS_CMD=$COMPRESS_CMD_BZ2
UNCOMPRESS_CMD=$UNCOMPRESS_CMD_BZ2
POSTFIX=$POSTFIX_BZ2
RD_LOCK="flock -s "
WT_LOCK="flock -x "

LOG_LEVEL_FILE=/nvram/syslog_level
LOG_CONF_FILE=/nvram/syslog.conf
SYSLOG_CONF_FILE=/etc/syslog.conf
SYSLOG_DEFAULT_CONF_FILE=/etc/syslog.conf_default
DPC3939_OLD_FWLOG_FILE_PATH=/nvram/log/firewall
DPC3939_OLD_EVTLOG=/nvram/log/event/eventlog
DPC3939_OLD_SYSTERMLOG=/nvram/log/system/systemlog

# In R1.3, Log path is under /nvram/log
# > R1.4 Log path is under /nvram2/log
# Move txt file into tar package
old_sysevtlog_handle(){
    if [ -e "$1"".0" ] || [  -e "$1" ]
    then
        FILE=""
        DIR=/tmp/`date +%Y%m%d%H%M%s`
        mkdir -p $DIR
        cd $DIR
        NEW_NAME=`syscfg get $2`

        if [ -z $NEW_NAME ]
        then
            syscfg set $2 1
            syscfg commit
            NEW_NAME=1
        else
            let "NEW_NAME++"
            syscfg set $2 $NEW_NAME
            syscfg commit
        fi

        if [ -e "$1"".0" ]
        then

            cp $1.0 $NEW_NAME
            FILE="$1.0"
            let "NEW_NAME++"
            syscfg set $2 $NEW_NAME
            syscfg commit
        fi

        if [ -e "$1" ]
        then
            cp $1 $NEW_NAME
            FILE="$FILE"" $1"
            let "NEW_NAME++"
            syscfg set $2 $NEW_NAME
            syscfg commit
        fi

        ZIP="$1.$POSTFIX"
        NEW_ZIP="${1##*/}.$POSTFIX"
        #un-compress log file
        if [ -e $ZIP ]
        then
             $RD_LOCK $ZIP -c $UNCOMPRESS_CMD $ZIP 
             ZIP_SZ=$(ls -l $ZIP | awk '{print $3}')
        else
             ZIP_SZ=0;
        fi
        $COMPRESS_CMD $NEW_ZIP ./* 
        $WT_LOCK $ZIP -c mv $NEW_ZIP $ZIP
        for oldfile in $FILE ;
        do
            echo "$WT_LOCK $oldfile -c rm -r $oldfile"
            $WT_LOCK $oldfile -c rm -r $oldfile
        done;

        rm -rf $DIR     
    fi
}

old_fwlog_handle(){
    MERGE=0 
    # find old log path
    if [ -e "$1" ]
    then 
        for filename in `ls $1`;
        do
            if [ "$filename" != "fwlog.$POSTFIX" ]
            then
                MERGE=1
            fi
        done

        if [ "$MERGE" == "1" ]
        then
            echo "merge log"
            for filename in `ls $2`;
            do
                if [ "$filename" != "fwlog.$POSTFIX" ]
                then
                    tmp_file="$1/${filename##*/}"
                    cat $2/$filename >> $tmp_file
                fi
            done
            echo "Move all txt log to new location"
            for filename in `ls $1` ;
            do
                if [ "$filename" != "fwlog.$POSTFIX" ]
                then
                    cp -f $1/$filename $2
                    $WT_LOCK $1/$filename -c rm -r $1/$filename
                fi
            done
        fi
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

    SYSTEMLOG=$(grep -e "systemlog" /etc/syslog.conf | awk '{print $2}') 
    EVENTLOG=$(grep -e "eventlog" /etc/syslog.conf | awk '{print $2}')

    SYSTEMLOG_DIR=${SYSTEMLOG%/*}
    if [ ! -d $SYSTEMLOG_DIR ]
    then
        mkdir -p $SYSTEMLOG_DIR
    fi

    EVENTLOG_DIR=${EVENTLOG%/*}
    if [ ! -d $EVENTLOG_DIR ]
    then
	echo "mkdir -p $EVENTLOG_DIR"
        mkdir -p $EVENTLOG_DIR
    fi

    echo "klogd -c $level"
    klogd -c $level    
    echo "syslogd -l $level"
    syslogd -l $level
}

get_log_file()
{
#TEMP=`syscfg get $1`
TEMP=`sysevent get $1`
if [ -z "$TEMP" ]
then
    TEMP=$(grep -e $2 /etc/syslog.conf | awk '{print $2}')
    if [ -n "$TEMP" ]
    then
        #syscfg set $1 $TEMP
        #syscfg commit
        sysevent set $1 $TEMP
    fi
fi
# !
echo $TEMP 
}

remove_log()
{
    FILE="$1"

    if [ ! -z $FILE ]
    then
        DIR=${FILE%/*}

        if [ -d $DIR ]
        then
            echo "rm -rf $1*"
            rm -rf "$1"*
        fi
    fi
}

compress()
{
    if [ "$1" == "syslog" ]
    then
        FILE="$V_SYS_LOG_FILE.0" 
        NEW_NAME=`syscfg get SYS_LOG_F_INSTANCE`
        if [ -z $NEW_NAME ]
        then
            syscfg set SYS_LOG_F_INSTANCE 1
            syscfg commit
            NEW_NAME=1
        else
            let "NEW_NAME++"
            syscfg set SYS_LOG_F_INSTANCE $NEW_NAME
            syscfg commit
        fi
        ZIP="$V_SYS_LOG_FILE.$POSTFIX"
        NEW_ZIP="${V_SYS_LOG_FILE##*/}.$POSTFIX"
        ZIP_SZ_MAX=`syscfg get SYS_LOG_COMMPRESSED_FILE_SIZE`
        # get R1.3 old log path
        OLD_LOG_DIR_13=${DPC3939_OLD_SYSTERMLOG%/*}

        if [ ! -e $FILE ]
        then
            exit 0;
        fi

    elif [ "$1" == "evtlog" ]
    then
        FILE="$V_EVT_LOG_FILE.0"
        NEW_NAME=`syscfg get EVT_LOG_F_INSTANCE`   
        if [ -z $NEW_NAME ]
        then
            syscfg set EVT_LOG_F_INSTANCE 1 
            syscfg commit
            NEW_NAME=1
        else
            let "NEW_NAME++"
            syscfg set EVT_LOG_F_INSTANCE $NEW_NAME
            syscfg commit
        fi

        ZIP="$V_EVT_LOG_FILE.$POSTFIX"
        NEW_ZIP="${V_EVT_LOG_FILE##*/}.tar.bz2"
        ZIP_SZ_MAX=`syscfg get EVT_LOG_COMMPRESSED_FILE_SIZE`
        # get R1.3 old log path
        OLD_LOG_DIR_13=${DPC3939_OLD_EVTLOG%/*}
        if [ ! -e $FILE ]
        then
            exit 0;
        fi
    elif [ "$1" == "fwlog" ]
    then
        FILE=""
        if [ ! -d $V_FW_LOG_FILE_PATH ]
        then
            exit 0;
        fi
        for filename in `ls $V_FW_LOG_FILE_PATH`;
        do
            if [ "$filename" != "fwlog.$POSTFIX" -a "$filename" != `date +%Y%m%d` ];
            then
                FILE="$FILE""$V_FW_LOG_FILE_PATH/$filename "
            fi
        done
        NEW_NAME="./"  
        ZIP="$V_FW_LOG_FILE_PATH/fwlog.$POSTFIX"
        NEW_ZIP="fwlog.$POSTFIX"
        ZIP_SZ_MAX=`syscfg get FW_LOG_COMPRESSED_FILE_SIZE`
        # get old log path
        OLD_LOG_DIR_13=${DPC3939_OLD_FWLOG_FILE_PATH}
        if [ -z "$FILE" ]
        then
            exit 0;
        fi

    fi

        DIR=/tmp/`date +%Y%m%d%H%M%s`_$1
        mkdir -p $DIR
        cd $DIR
        cp $FILE $NEW_NAME
         FILE_SZ=$(ls -l | awk 'BEGIN {SUM=0}{SUM+=$3}END {print SUM}')
        #un-compress log file
        if [ -e $ZIP ]
        then
             $RD_LOCK $ZIP -c $UNCOMPRESS_CMD $ZIP 
             ZIP_SZ=$(ls -l $ZIP | awk '{print $3}')
        else
            ZIP_SZ=0;
        fi

        OLD_FILE=""
        # clean old log file
        if [ $(expr $(($FILE_SZ + 9)) / 10 + $ZIP_SZ) -ge $(expr "$ZIP_SZ_MAX" \* 1024) ]
        then
            OLD_FILE_SIZE=0
            for filename in `ls ./`;
            do
                 let "OLD_FILE_SIZE=$OLD_FILE_SIZE + $(ls -l $filename | awk '{print $3}')"
                OLD_FILE="$OLD_FILE $filename"
                if [ $OLD_FILE_SIZE -ge $FILE_SZ  ]
                then
                    break;
                fi
            done;
        fi

        if [ ! -z "$OLD_FILE" ]
        then
            rm $OLD_FILE
            # Remove old R1.3 log under /nvram
            if [ -e $OLD_LOG_DIR_13 ]
            then
                rm -rf $OLD_LOG_DIR_13
            fi
        fi

        $COMPRESS_CMD $NEW_ZIP ./* 
        $WT_LOCK $ZIP -c mv $NEW_ZIP $ZIP
        for oldfile in $FILE ;
        do
            echo "$WT_LOCK $oldfile -c rm -r $oldfile"
            $WT_LOCK $oldfile -c rm -r $oldfile
        done;

        rm -rf $DIR       
}

uncompress()
{
    if [ -z "$1" ] || [ ! -e "$1" ] || [ -z "$2" ] || [ ! -d "$2" ]
    then
        return 0;
    fi
    DIR=$2
    TAR=$1
    cd "$DIR"
    $RD_LOCK "$TAR" -c "$UNCOMPRESS_CMD" "$TAR" 
}

V_FW_LOG_FILE_PATH=`sysevent get FW_LOG_FILE_PATH_V2`
if [ -z $V_FW_LOG_FILE_PATH ]
then
    V_FW_LOG_FILE_PATH=`syscfg get FW_LOG_FILE_PATH`
fi

V_EVT_LOG_FILE="$(get_log_file EVT_LOG_FILE_V2 eventlog)"
V_SYS_LOG_FILE="$(get_log_file SYS_LOG_FILE_V2 systemlog)"
if [ -z $V_FW_LOG_FILE_PATH ] || [ -z $V_EVT_LOG_FILE ] || [ -z $V_FW_LOG_FILE_PATH ]
then
    exit 0;
fi

V_HANDLE_OLD_LOG_13_FLG=`sysevent get R13_LOG_HANDLE_FLG`
if [ -z $V_HANDLE_OLD_LOG_13_FLG ]
then
    old_sysevtlog_handle $DPC3939_OLD_SYSTERMLOG SYS_LOG_F_INSTANCE 
    old_sysevtlog_handle $DPC3939_OLD_EVTLOG EVT_LOG_F_INSTANCE 
    old_fwlog_handle $DPC3939_OLD_FWLOG_FILE_PATH "$V_FW_LOG_FILE_PATH" 
    sysevent set R13_LOG_HANDLE_FLG 1
fi 


if [ "$1" == "reset" ]
then 
    # kill syslog server, in case they are writting log file when delete
    killall syslogd  >/dev/null 2>/dev/null  
    killall klogd >/dev/null 2>/dev/null
    killall GenFWLog >/dev/null 2>/dev/null
    #delete log file
    remove_log $V_EVT_LOG_FILE
    remove_log $V_SYS_LOG_FILE
    if [ ! -z $V_FW_LOG_FILE_PATH ]
    then
        remove_log $V_FW_LOG_FILE_PATH/*
    fi

    #rm -rf /nvram2/log
    rm -rf /nvram/log

    #delete syslog config file
    if [ -e /nvram/syslog.conf ]
    then
        echo "remove syslog.conf from /nvram"
        rm -rf /nvram/syslog.conf
    fi

    #delete syslog level file
    if [ -e /nvram/syslog_level ]
    then
        echo "remove syslog_level file"
        rm -rf /nvram/syslog_level
    fi
    
    start_syslog 
fi

if [ "$1" == "compress_syslog" ]
then
    compress syslog    
fi

if [ "$1" == "uncompress_syslog" ]
then
    uncompress $DPC3939_OLD_SYSTERMLOG.tar.bz2 $2
    uncompress $V_SYS_LOG_FILE.tar.bz2 $2
fi

if [ "$1" == "compress_evtlog" ]
then
    compress evtlog    
fi

if [ "$1" == "uncompress_evtlog" ]
then
    uncompress $DPC3939_OLD_EVTLOG.tar.bz2 $2 
    uncompress $V_EVT_LOG_FILE.tar.bz2 $2
fi

if [ "$1" == "compress_fwlog" ]
then
    compress fwlog    
fi

if [ "$1" == "uncompress_fwlog" ]
then
    uncompress $DPC3939_OLD_FWLOG_FILE_PATH/fwlog.tar.bz2 $2
    uncompress $V_FW_LOG_FILE_PATH/fwlog.tar.bz2 $2
fi

if [ -z $1 ]
then
    evt="$V_EVT_LOG_FILE.0"
    sys="$V_SYS_LOG_FILE.0"
    if [ -e $evt ]
    then
        log_handle.sh compress_evtlog &
    fi

    if [ -e $sys ]
    then
        log_handle.sh compress_syslog &
    fi

    if [ ! -d $V_FW_LOG_FILE_PATH ]
    then
        exit 0
    fi

    for fw in `ls $V_FW_LOG_FILE_PATH`;
    do
        if [ "$fw" != "fwlog.$POSTFIX" -a "$filename" != `date +%Y%m%d` ];
        then
            log_handle.sh compress_fwlog &
            break
        fi
    done
fi


