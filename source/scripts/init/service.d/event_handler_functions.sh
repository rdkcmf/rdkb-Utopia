#------------------------------------------------------------------
# Copyright (c) 2010 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

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
      ulog $SERVICE_NAME status "PID ($$) Error ($1) $2"
      sysevent set ${SERVICE_NAME}-status error
      sysevent set ${SERVICE_NAME}-errinfo "Error ($1) $2"
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
      ulog $SERVICE_NAME status "PID ($$) Error ($1) $2"
      sysevent set ${SERVICE_NAME}-status error
      sysevent set ${SERVICE_NAME}-errinfo "Error ($1) $2"
      exit ${1}
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
      LSTATUS=`sysevent get ${LSERVICE}-status`
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
      LSTATUS=`sysevent get ${LSERVICE}-status`
      if [ "$STATE" != "$LSTATUS" ] ; then
         sleep 1
         TRIES=`expr $TRIES + 1`
      else
         return
      fi
   done
}
