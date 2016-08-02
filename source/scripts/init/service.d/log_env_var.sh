#!/bin/sh

if [ -f /etc/os-release ] || [ -f /etc/device.properties ]; then
      LOG_FOLDER="/rdklogs"
else
      LOG_FOLDER="/var/tmp"
fi

LOG_UPLOAD_FOLDER="/nvram"
RDK_LOGGER_PATH="/rdklogger"
LOG_PATH="$LOG_FOLDER/logs/"

ATOM_LOG_PATH="/rdklogs/logs/"
ATOM_IP=""
backupenable=`syscfg get logbackup_enable`
isNvram2Supported="no"
if [ -f /etc/device.properties ]
then
   isNvram2Supported=`cat /etc/device.properties | grep NVRAM2_SUPPORTED | cut -f2 -d=`
   atom_sync=`cat /etc/device.properties | grep ATOM_SYNC | cut -f2 -d=` 
fi

if [ "$atom_sync" = "yes" ]
then
   ATOM_IP=`cat /etc/device.properties | grep ATOM_IP | cut -f2 -d=` 
fi	

#if [ "$isNvram2Supported" = "yes" ] && [ "$backupenable" = "true" ]
#then
 LOG_SYNC_PATH="/nvram2/logs/"
 LOG_SYNC_BACK_UP_PATH="/nvram2/logs/"
 LOG_SYNC_BACK_UP_REBOOT_PATH="/nvram2/logs/"
#else
 LOG_BACK_UP_PATH="$LOG_UPLOAD_FOLDER/logbackup/"
 LOGTEMPPATH="$LOG_FOLDER/backuplogs/"
 LOG_BACK_UP_REBOOT="$LOG_UPLOAD_FOLDER/logbackupreboot/"
#fi


HAVECRASH="$LOG_FOLDER/processcrashed"
FLAG_REBOOT="$LOG_FOLDER/waitingreboot"
UPLOAD_ON_REBOOT="$LOG_UPLOAD_FOLDER/uploadonreboot"
LOG_UPLOAD_ON_REQUEST="$LOG_UPLOAD_FOLDER/loguploadonrequest/"

UPLOAD_ON_REQUEST="$LOG_FOLDER/uploadingonrequest"
UPLOAD_ON_REQUEST_SUCCESS="$LOG_UPLOAD_FOLDER/uploadsuccess"
REGULAR_UPLOAD="$LOG_FOLDER/uploading"

UPLOADRESULT="$LOG_UPLOAD_FOLDER/resultOfupload"
WAN_INTERFACE="erouter0"
OutputFile="/tmp/httpresult.txt"
HTTP_CODE="/tmp/curl_httpcode"
S3_URL="https://ssr.ccp.xcal.tv/cgi-bin/rdkb_snmp.cgi"
WAITINGFORUPLOAD="$LOG_UPLOAD_FOLDER/waitingforupload"

if [ -f /etc/os-release ] || [ -f /etc/device.properties ]; then
      MAXSIZE=1536
else
      MAXSIZE=524288
fi
MAXLINESIZE=2

URL="https://ssr.ccp.xcal.tv/cgi-bin/rdkb.cgi"

if [ -z $LOG_PATH ]; then
    LOG_PATH="$LOG_FOLDER/logs/"
fi
if [ -z $PERSISTENT_PATH ]; then
    PERSISTENT_PATH="/tmp"
fi

LOG_FILE_FLAG="$LOG_FOLDER/filescreated"
CONSOLEFILE="$LOG_FOLDER/logs/ArmConsolelog.txt.0"

SELFHEALFILE="$LOG_FOLDER/logs/SelfHeal.txt.0"
SELFHEALFILE_BOOTUP="$LOG_SYNC_PATH/SelfHealBootUp.txt.0"
lockdir=$LOG_FOLDER/rxtx

