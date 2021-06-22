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

#------------------------------------------------------------------
# Service to provide automatic configuration restore methods using
# usb drive. This is intended only for internal development purpose.
# SHOULD NOT be included in a production system
#
# 1) Restore to factory defaults
#    Checks for presense of .reset_factory_default
#    Does a factory reset is present
#
# 2) Restore configuration
#    Checks for presense of .restore_config
#    Uses the contents of .restore_config file to populate syscfg and 
#    does a reboot
#
# Assumptions
# a)   (1) overrides (2), i.e of both .reset_factory_default and .restore_config
#      is present, factory default operation is performed
# b)   .restore_config should be a valid syscfg text file. It should be created 
#      using 'syscfg show > .restore_config'
#------------------------------------------------------------------
source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="configrestore"
SELF_NAME="`basename "$0"`"

FACTORY_DEFAULT_FILE=.factory_default
RESTORE_CONFIG_FILE=.restore_config

service_start()
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   service_usb_device_state

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
   # noop

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

service_usb_insert () 
{
  MOUNT_PT=`sysevent get usb_device_mount_pt`
  if [ -z "$MOUNT_PT" ] ; then
      ulog ${SERVICE_NAME} status "unspecified usb device mount path"
      return
  fi

  CHECK_FILE=${MOUNT_PT}/${FACTORY_DEFAULT_FILE}
  ulog ${SERVICE_NAME} status "checking for factory default file $CHECK_FILE"
  if [ -f "$CHECK_FILE" ] ; then
      ulog ${SERVICE_NAME} status "restore factory defaults"
      # note, this cmd will do a system reboot
      utcmd factory_reset
      return
  fi

  CHECK_FILE=${MOUNT_PT}/${RESTORE_CONFIG_FILE}
  ulog ${SERVICE_NAME} status "checking for config restore file $CHECK_FILE"
  # Restore configuration
  if [ -f "$CHECK_FILE" ] ; then
      ulog ${SERVICE_NAME} status "restore configuration to $CHECK_FILE "
      # note, this cmd will do a system reboot
      utcmd cfg_restore "$CHECK_FILE"
      return
  fi
}

service_usb_remove () 
{
   ulog ${SERVICE_NAME} status "handle usb removal" 
   # noop
}

service_usb_device_state () 
{
   ulog ${SERVICE_NAME} status "handle usb device state in ${SERVICE_NAME} service" 

   USB_STATE=`sysevent get usb_device_state`
   case "$USB_STATE" in
      inserted)
          service_usb_insert
          ;;
      removed)
          service_usb_remove
          ;;
      *)
          ulog ${SERVICE_NAME} status "unknown usb device state ${USB_STATE}" 
          ;;
   esac
}

# Entry

case "$1" in
  "${SERVICE_NAME}-start")
      service_start
      ;;
  "${SERVICE_NAME}-restart")
      service_start
      ;;
  "${SERVICE_NAME}-stop")
      # no-op
      ;;
  usb_device_state)
      service_usb_device_state
      ;;
  *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-restart | ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | usb_device_state ]" > /dev/console
      exit 3
      ;;
esac

