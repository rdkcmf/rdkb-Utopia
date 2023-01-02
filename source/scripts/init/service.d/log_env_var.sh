#!/bin/sh

##################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2018 RDK Management

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
NVRAM2_SUPPORTED="no"
ATOM_IP=""
UPLOAD_THRESHOLD=""


TMP_UPLOAD="/tmp/logs/"
LOG_SYNC_PATH="/nvram2/logs/"
LOG_SYNC_BACK_UP_PATH="/nvram2/logs/"
LOG_SYNC_BACK_UP_REBOOT_PATH="/nvram2/logs/"

. /etc/device.properties

if [ -f /etc/os-release ] || [ -f /etc/device.properties ]; then
      LOG_FOLDER="/rdklogs"
else
      LOG_FOLDER="/var/tmp"
fi

LOG_UPLOAD_FOLDER="/nvram"
RDK_LOGGER_PATH="/rdklogger"
LOG_PATH="$LOG_FOLDER/logs/"

ATOM_LOG_PATH="/rdklogs/logs/"

backupenable=`syscfg get logbackup_enable`
#dmesg sync
DMESG_FILE="/rdklogs/logs/messages.txt"
lastdmesgsync="/tmp/lastdmesgsynctime"
journal_log="/rdklogs/logs/journal_logs.txt.0"

#if [ -f /etc/device.properties ]
#then
#   isNvram2Supported=`cat /etc/device.properties | grep NVRAM2_SUPPORTED | cut -f2 -d=`
#   atom_sync=`cat /etc/device.properties | grep ATOM_SYNC | cut -f2 -d=`
#   UPLOAD_THRESHOLD=`cat /etc/device.properties | grep LOG_UPLOAD_THRESHOLD  | cut -f2 -d=`
#   model=`cat /etc/device.properties | grep MODEL_NUM  | cut -f2 -d=`
#  BOX_TYPE=`cat /etc/device.properties | grep BOX_TYPE  | cut -f2 -d=`
#fi

#if [ "$atom_sync" = "yes" ]
#then
   #ATOM_IP=`cat /etc/device.properties | grep ATOM_IP | cut -f2 -d=` 
#fi	

#if [ "$NVRAM2_SUPPORTED" = "yes" ] && [ "$backupenable" = "true" ]
#then

#else
 LOG_BACK_UP_PATH="$LOG_UPLOAD_FOLDER/logbackup/"
 LOGTEMPPATH="$LOG_FOLDER/backuplogs/"
 LOG_BACK_UP_REBOOT="$LOG_UPLOAD_FOLDER/logbackupreboot/"
#fi

#If device is having SYNC PATH overrides, it will get applied here.
#if [ -f /etc/device.properties ]
#then
#	LOG_SYNC_PATH_override=`cat /etc/device.properties | grep LOG_SYNC_PATH  | cut -f2 -d=`
#	LOG_SYNC_BACK_UP_PATH_override=`cat /etc/device.properties | grep LOG_SYNC_BACK_UP_PATH  | cut -f2 -d=`
#	LOG_SYNC_BACK_UP_REBOOT_PATH_override=`cat /etc/device.properties | grep LOG_SYNC_BACK_UP_REBOOT_PATH  | cut -f2 -d=`
#	if [ -n "$LOG_SYNC_PATH_override" ] && [ -n "$LOG_SYNC_BACK_UP_PATH_override" ] && [ -n "$LOG_SYNC_BACK_UP_REBOOT_PATH_override" ]
#	then
#		LOG_SYNC_PATH=$LOG_SYNC_PATH_override
#		LOG_SYNC_BACK_UP_PATH=$LOG_SYNC_BACK_UP_PATH_override
#		LOG_SYNC_BACK_UP_REBOOT_PATH=$LOG_SYNC_BACK_UP_REBOOT_PATH_override
#	fi	
#fi

#This change is needed for ArrisXB6 to choose sync location dynamically.
if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ];then
	isNvram2Mounted=`grep nvram2 /proc/mounts`
	if [ -z "$isNvram2Mounted" ];then
		LOG_SYNC_PATH="/nvram/logs/"
		LOG_SYNC_BACK_UP_PATH="/nvram/logs/"
		LOG_SYNC_BACK_UP_REBOOT_PATH="/nvram/logs/"
	fi
fi
#TCCBBR product is a noMoca product. This nvram file shall be used by cosa_start_rem.sh .
if [ "$BOX_TYPE" == "TCCBR" ];then
	touch /nvram/disableCcspMoCA
fi

#BCI devices do not support TR069. This nvram file shall be used by cosa_start_rem.sh .
if [ "$BOX_TYPE" == "TCCBR" ];then
       touch /nvram/disableCcspTr069PaSsp
fi

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

#Devices that have more nvram size can override default upload threshold (1.5MB) through device.properties
if [ -n "$LOG_UPLOAD_THRESHOLD" ]
then
	MAXSIZE=$LOG_UPLOAD_THRESHOLD
fi    

if [ -z $LOG_PATH ]; then
    LOG_PATH="$LOG_FOLDER/logs/"
fi
if [ -z "$PERSISTENT_PATH" ]; then
    PERSISTENT_PATH="/tmp"
fi

LOG_FILE_FLAG="$LOG_FOLDER/filescreated"

if [ "$BOX_TYPE" = "XB3" ];then
CONSOLEFILE="$LOG_FOLDER/logs/ArmConsolelog.txt.0"
else
CONSOLEFILE="$LOG_FOLDER/logs/Consolelog.txt.0"
fi

SELFHEALFILE="$LOG_FOLDER/logs/SelfHeal.txt.0"
SELFHEALFILE_BOOTUP="$LOG_SYNC_PATH/SelfHealBootUp.txt.0"
lockdir=$LOG_FOLDER/rxtx
DCMRESPONSE="/nvram/DCMresponse.txt"
DCMRESPONSE_TMP="/tmp/DCMresponse.txt"
DCM_SETTINGS_PARSED="/tmp/DCMSettingsParsedForLogUpload"

TMP_LOG_UPLOAD_PATH="/tmp/log_upload"
RAM_OOPS_FILE_LOCATION="/sys/fs/pstore/"
RAM_OOPS_FILE="*-ramoops*"
RAM_OOPS_FILE0="dmesg-ramoops-0"
RAM_OOPS_FILE0_HOST="dmesg-ramoops-0_host"
RAM_OOPS_FILE1="dmesg-ramoops-1"
RAM_OOPS_FILE1_HOST="dmesg-ramoops-1_host"
