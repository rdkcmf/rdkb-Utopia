#!/bin/sh

if [ -f /etc/os-release ] || [ -f /etc/device.properties ]; then
      LOG_FOLDER="/rdklogs"
else
      LOG_FOLDER="/var/tmp"
fi

LOG_UPLOAD_FOLDER="/nvram"
RDK_LOGGER_PATH="/fss/gw/rdklogger"
LOG_PATH="$LOG_FOLDER/logs/"

LOG_BACK_UP_PATH="$LOG_UPLOAD_FOLDER/logbackup/"
LOGTEMPPATH="$LOG_FOLDER/backuplogs/"
LOG_BACK_UP_REBOOT="$LOG_UPLOAD_FOLDER/logbackupreboot/"

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

lockdir=$LOG_FOLDER/rxtx

