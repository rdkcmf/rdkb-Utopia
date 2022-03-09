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

source /etc/utopia/service.d/event_flags

#-----------------------------------------------------------------
# check_err
#
# Parameter 1 is the return code to check
# If the return code is not 0, then this function will:
#    set the service status to error, and 
#    set the service errinfo to Parameter 2, and
#    output a statement to /var/log/messages 
# The function returns when done
#-----------------------------------------------------------------
check_err ()
{
   if [ "${1}" -ne "0" ] ; then
      ulog "$SERVICE_NAME" status "PID ($$) Error ($1) $2"
      sysevent set "${SERVICE_NAME}"-status error
      sysevent set "${SERVICE_NAME}"-errinfo "Error ($1) $2"
   fi
}

#-----------------------------------------------------------------
# check_err_exit
#
# Parameter 1 is the return code to check
# If the return code is not 0, then this function will:
#    set the service status to error, and 
#    set the service errinfo to Parameter 2, and
#    output a statement to /var/log/messages 
# If there is a non zero return code the process will be exited
#-----------------------------------------------------------------
check_err_exit ()
{
   if [ "${1}" -ne "0" ] ; then
      ulog "$SERVICE_NAME" status "PID ($$) Error ($1) $2"
      sysevent set "${SERVICE_NAME}"-status error
      sysevent set "${SERVICE_NAME}"-errinfo "Error ($1) $2"
      exit "${1}"
   fi
}

#-----------------------------------------------------------------
# wait_till_end_state
#
# Waits a reasonable amount of time for a service to be out of 
# starting or stopping state
#-----------------------------------------------------------------
wait_till_end_state ()
{
  LSERVICE=$1
  TRIES=1
   while [ "9" -ge "$TRIES" ] ; do
      LSTATUS=`sysevent get "${LSERVICE}"-status`
      if [ "starting" = "$LSTATUS" ] || [ "stopping" = "$LSTATUS" ] ; then
         sleep 1
         TRIES=`expr $TRIES + 1`
      else
         return
      fi
   done
}

#-----------------------------------------------------------------
# wait_till_state
#
# Waits a reasonable amount of time for a service to reach a certain
# state
#-----------------------------------------------------------------
wait_till_state ()
{
   LSERVICE=$1
   STATE=$2
   TRIES=1
   while [ "9" -ge "$TRIES" ] ; do
      LSTATUS=`sysevent get "${LSERVICE}"-status`
      if [ "$STATE" != "$LSTATUS" ] ; then
         sleep 1
         TRIES=`expr $TRIES + 1`
      else
         return
      fi
   done
}
