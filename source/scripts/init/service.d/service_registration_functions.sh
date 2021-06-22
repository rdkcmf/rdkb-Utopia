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
#                       service_registration_functions
#
# This file implements the service registration helper code
#
#------------------------------------------------------------------


#------------------------------------------------------------------
#                    A Note On Sysevent - the event subsystem
#
# Although it is not necessary to understand sysevent if you are using
# the utilities within this file, some developers may want to utilize 
# the underlying sysevent subsystem directly in order to take advantage 
# of the simplicity of avoiding wrappers. For those developers, here is
# an outline of sysevent activation handler registration.
#
# Sysevent is the component that will activate handler upon events. Sysevent provides
# quite a bit of flexibility to how events are triggered, and how handlers are run.
# This flexibility is controlled by activation flags (describing how to run the handler),
# and tuple flags (describing how to interpret events). The default is to trigger an
# event only when the tuple value changes, and to serialize the activation of each unique handler.
#
# When an event is triggered, the handler will be called with a parameter specifying
# the name of the event. It is also possible to specify additional parameters to be passed
# to a handler. The parameters may be constants, and/or runtime values of syscfg, and/or
# runtime values of sysevent.
#
# The following examples demonstrate the range of behaviors:
#
# examples:
#    name of a handler to be activated upon some event
#    HANDLER="/etc/utopia/service.d/new_service_handler.sh"
#
#    a) register for $HANDLER to be activated whenever <event_name, > CHANGES value
#       and ensure that
#       if multiple value changes occur, then only one instance of $HANDLER will be run at a time.
#       (Note that sysevent will not allow an activation handler to run indefinately, so there is
#       no risk of being blocked)
#          sysevent async event_name $HANDLER
#    b) register for $HANDLER to be activated whenever any value is SET for <event_name, >
#          sysevent async event_name $HANDLER
#          sysevent setoptions event_name $TUPLE_FLAG_EVENT
#    c) register for $HANDLER to be activated whenever <event_name, > changes value, and
#       ensure that all activation handlers for <event_name, > are run serially, one after the
#       other. 
#          sysevent async event_name $HANDLER
#          sysevent setoptions event_name $TUPLE_FLAG_SERIAL
#    d) register for $HANDLER to be activated whenever any value is set for <event_name, > and
#       ensure that all activation handlers for <event_name, > are run serially, one after the
#       other.
#          sysevent async event_name $HANDLER
#          sysevent setoptions event_name 0x00000003    (note that flag is $TUPLE_FLAG_SERIAL | $TUPLE_FLAG_EVENT)
#    e) register for $HANDLER to be activated whenever <event_name, > changes value. 
#       If multiple value changes occur, do NOT enforce that only one instance of $HANDLER will be run at a time.
#          sysevent async_with_flags $ACTION_FLAG_NOT_THREADSAFE event_name $HANDLER
#    f) register for $HANDLER to be activated whenever <event_name, > changes value, and ensure that
#       if multiple value changes occur, then only one instance of $HANDLER will be run at a time.
#       But also ensure that only one activation (besides the currently running activation) will be queued.
#       This means that a handler will not have an infinitly long queue of pending activations
#          sysevent async_with_flags $ACTION_FLAG_COLLAPSE_PENDING_QUEUE event_name $HANDLER
#    g) register for $HANDLER to be activated whenever any value is set for <event_name, >, and
#       ensure that if multiple value changes occur, then only one instance of $HANDLER will be 
#       run at a time.
#          sysevent async_with_flags $ACTION_FLAG_NOT_THREADSAFE event_name $HANDLER
#          sysevent setoptions event_name $TUPLE_FLAG_EVENT
#    h) register for $HANDLER to be activated whenever <event_name, > changes, and
#       pass the parameter "new_param" as the second parameter in the activation of the handler
#          sysevent async event_name $HANDLER new_param
#    i) register for $HANDLER to be activated whenever <event_name, > changes, and
#       pass the runtime value of syscfg tuple <wan_proto> as the second parameter, and
#       pass the runtime value of sysevent tuple <current_wan_ipaddr, > as the third parameter
#       to the handler.
#          sysevent async event_name $HANDLER \$wan_proto @current_wan_ipaddr
#       Note that '$' signifies that a syscfg runtime value is required. The leading '\'
#       is needed to escape shell processing.
#       Also note that '@' signifies that a sysevent runtime value is required.
#
#    Note that in general you want to be careful about using the sysevent setoptions command because this
#    affects ALL handlers registered for the tuple. So if you want to use setoptions you should think
#    about the expected behaviour of other handler. Also, you rarely want to use sysevent async_with_flags,
#    because running multiple instances of the handler process can cause problems if you dont consider
#    the interactions possible when two simultaneous processes are running.
#
# Note that calls to sysevent async or sysevent async_with_flags will return an async id.
# The async id can be used to cancel notifications. It is considered good behavior to save
# the async id, and use it when unregistering.
# for example:
#    asyncid=`sysevent async event_name $HANDLER`;
#    sysevent set event_name_asyncid_1 $asyncid
# and later
#    sysevent rm_async `sysevent get event_name_asyncid_1`
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_flags

##################################################################################
#                           Helper Functions
#  These functions are generally just used by the Main Functions below
##################################################################################

SM_PREFIX="xsm_"
SM_POSTFIX="_async_id"

#------------------------------------------------------------------
# Name       : sm_save_async
# Parameters :
#    $1 is the name of the service for which the async string is desired
#    $2 is the name of the event
#    $3 is the async id
#------------------------------------------------------------------
sm_save_async() {
   # if the event is a standard event then use a standard event naming convention
   if [ "${1}-start" = "$2" ] || [ "${1}-stop" = "$2" ] || [ "${1}-restart" = "$2" ] ; then
      sysevent set ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${2}" "${2} ${3}"
   else
      # if the event is not a standard event then use an indexable naming convention
      SMSA_IDX=1
      SMSA_TUPLE=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_${SMSA_IDX}`
      until [ -z "$SMSA_TUPLE" ] ; do
         SMSA_IDX=`expr $SMSA_IDX + 1`
         SMSA_TUPLE=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMSA_IDX}"`
      done
      sysevent set ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMSA_IDX}" "${2} ${3}"
   fi
}

#------------------------------------------------------------------
# Name       : sm_rm_event
# Parameters :
#    $1 is the name of the service for which the event should be removed
#    $2 is the name of the event
#------------------------------------------------------------------
sm_rm_event() {
   # if the event is a standard event then use a standard event naming convention
   if [ "${1}-start" = "$2" ] || [ "${1}-stop" = "$2" ] || [ "${1}-restart" = "$2" ] ; then
      SMRM_STR=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${2}"`
      if [ -n "$SMRM_STR" ] ; then
         SMRM_ASYNC=`echo "$SMRM_STR" | cut -f 2,3 -d ' '`
         if [ -n "$SMRM_ASYNC" ] ; then
            sysevent rm_async "$SMRM_ASYNC"
            ulog srvmgr status "Unregistered $1 from $2"
         fi
         sysevent set ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${2}"
      fi
   else
      SMRM_IDX=1
      SMRM_STR=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_${SMRM_IDX}`
      SMRM_EVENT=`echo "$SMRM_STR" | cut -f 1 -d ' ' | sed 's/ //'`
      while [ -n "$SMRM_STR" ] && [ "$2" != "$SMRM_EVENT" ] ; do
         SMRM_IDX=`expr $SMRM_IDX + 1`
         SMRM_STR=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMRM_IDX}"`
         SMRM_EVENT=`echo "$SMRM_STR" | cut -f 1 -d ' ' | sed 's/ //'`
      done

      # we found the async, now remove it and remove the hole in the array
      if [ "$2" = "$SMRM_EVENT" ] ; then
         SMRM_ASYNC=`echo "$SMRM_STR" | cut -f 2,3 -d ' '`
         if [ -n "$SMRM_ASYNC" ] ; then
            sysevent rm_async "$SMRM_ASYNC"
            ulog srvmgr status "Unregistered $1 from $2"
         fi

         SMRM_NEXT_IDX=`expr "$SMRM_IDX" + 1`
         SMRM_NEXT_STR=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMRM_NEXT_IDX}"`
         until [ -z "$SMRM_NEXT_STR" ] ; do
            sysevent set ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMRM_IDX}" "$SMRM_NEXT_STR"
            SMRM_IDX=$SMRM_NEXT_IDX
            SMRM_NEXT_IDX=`expr "$SMRM_NEXT_IDX" + 1`
            SMRM_NEXT_STR=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMRM_NEXT_IDX}"`
         done
      fi
   fi
}

#------------------------------------------------------------------
# Name       : sm_register_one_event
# Parameters :
#   $1  is the name of the service which is registering for the event
#   $2  is a string representing the event.
#      "event name | event handler | activation flags or NULL | tuple flags or NULL | additional parameters"
#   Note that $2 must be enclosed in quotation marks
#
# The event string format
# -----------------------
#
#  event name | event handler | activation flags or NULL | tuple flags or NULL | additional parameters
#
# The first field must be the name of the event
# The second field must be the path and name of the handler that is to be activated upon the event
#
# There are optional fields:
# field 3:
#   activation_flags:
#      ACTION_FLAG_NOT_THREADSAFE (0x00000001)  : 
#                    If set, it is legal to have multiple simultaneous activations of the handler
#                    (the default is only one activation of the handler at a time, and 
#                     wait for prior activation to complete before activating a new handler.
#                     Note: sysevent will wait for a reasonable time for a prior handler to
#                           exit before killing it and allowing the next handler to be activated)
#      ACTION_FLAG_COLLAPSE_PENDING_QUEUE (0x00000002)  :
#                    If set, then if there are pending activations, ensure that the queue depth does not
#                    exceed currently executing activation, and 1 (the latest) pending execution 
#      NULL        : Use default activation policy (one activation at a time)
# field 4:
#   tuple_flags     :
#      TUPLE_FLAG_SERIAL (0x00000001)  :
#                   If there are multiple handler which are registered for the event, then the handlers
#                   will be activated serially. The default is to activate all (unique) handlers in parallel.
#                   Note that this flag affects the activate of different handlers using the same event,
#                        as opposed to the above activation flag which pertains to multiple activations
#                        of the same handler, whether or not using the same event.
#     TUPLE_FLAG_EVENT (0x00000002)  :
#                  Sysevent will not emit an event (activate handlers) if a tuple value is set to the 
#                  existing value. This flag causes sysevent to always emit an event (activate handlers)
#                  whether or not the tuple value changes. 
#                  Note that for standard events the default is to make the tuple an event
#    NULL        : Use default tuple policy which is TUPLE_FLAG_EVENT
#
# An example to illustrate the difference between activation_flag ACTION_FLAG_NOT_THREADSAFE and tuple_flag TUPLE_FLAG_EVENT
# Given a tuple <wan_state, > there can be several handlers that are tied to that tuple,
# and that are activated when the tuple's value changes (or in the case of TUPLE_FLAG_EVENT tuples, when
# the value is set. In this example assume that /etc/foo.sh and /etc/bar.sh are registered for activation
# on the tuple <wan_state, >
#
# If /etc/foo.sh set its activation_flag to ACTION_FLAG_NOT_THREADSAFE, and /etc/bar.sh is using the default,
# and if wan_state is set to a, and immediately set to b, and then immediately c, then it is 
# possible for wan_state to emit 3 events <wan_state,a> <wan_state,b> <wan_state,c>, and thus
# each handler will be called 3 times.
# For /etc/foo.sh, it will be allowed to run 3 simultaneous processes' (each one is /etc/foo.sh).
# But /etc/bar.sh will only be allowed to run 1 at a time. So each subsequent process will be held
# back until the previous one has finished. You can think of foo.sh as being a multi-threaded 
# application, and bar.sh is a single-threaded application. It is way easier to write a single-threaded
# application since you dont need to be concerned with multiple instances colliding over shared memory.
#
# Now, if <wan_state, > has been declared TUPLE_FLAG_SERIAL, and assuming that /etc/foo.sh was registered
# before /etc/bar.sh, then when wan_state is set to a, /etc/foo.sh will be run to completion, followed by
# /etc/bar. If <wan_state, > had not been declared TUPLE_FLAG_SERIAL, then /etc/foo.sh and /etc/bar.sh
# could run in parallel.
#
# other fields:
#   Each remaining field is a parameter that is to be passed to the handler
#   upon activation. Note that the 1st parameter passed to the handler is
#   the name of the event. (This parameter is NOT declared in the event string.)
#
#   The parameters may be constants, sysevent values, or syscfg values
#   - a constant is a string
#   - a sysevent value is the name of the sysevent tuple prepended by @ eg. @current_wan_ipaddr
#   - a syscfg value is the name of the syscfg tuple prepended by \$ eg, \$wan_proto 
#     note that you must escape the $ because otherwise the shell slurps the $ character
# 
#------------------------------------------------------------------
sm_register_one_event() {
   SMR_SERVICE=$1
   SMR_EVENT_STRING=$2

   # parse the event string removing extraneous whitespace
   SMR_EVENT_NAME=`echo "$SMR_EVENT_STRING" | cut -f 1 -d '|' | sed 's/ //'` 
   SMR_EVENT_HANDLER=`echo "$SMR_EVENT_STRING" | cut -f 2 -d '|' | sed 's/ //'`
   SMR_ACTIVATION_FLAGS=`echo "$SMR_EVENT_STRING" | cut -f 3 -d '|' | sed 's/ //'`
   SMR_TUPLE_FLAGS=`echo "$SMR_EVENT_STRING" | cut -f 4 -d '|' | sed 's/ //'`
   # dont use sed to remove blanks - there are spaces between parameters
   SMR_PARAMETERS=`echo "$SMR_EVENT_STRING" | cut -f 5 -d '|' `
   
   if [ -z "$SMR_EVENT_NAME" ] || [ -z "$SMR_EVENT_HANDLER" ] ; then
      return 1
   fi

   # if the event handler is NULL, then we ignore this event. The intention of NULL is to unregister
   # any previous registration. We have already done that above.
   if [ "NULL" = "$SMR_EVENT" ] ; then
      return 0
   fi

   # if this service has already registered for this event, then cancel the previous registration
   # this is probably overkill since we likely already called sm_unregister, but better safe than sorry
   sm_rm_event "${SMR_SERVICE}" "${SMR_EVENT_NAME}"


   # if there are activation flags on the event then respect them   
   if [ -n "$SMR_ACTIVATION_FLAGS" ] && [ "NULL" != "$SMR_ACTIVATION_FLAGS" ] ; then
      asyncid=`sysevent async_with_flags "$SMR_ACTIVATION_FLAGS" "$SMR_EVENT_NAME" "$SMR_EVENT_HANDLER" "$SMR_PARAMETERS"`;
   else
      asyncid=`sysevent async "$SMR_EVENT_NAME" "$SMR_EVENT_HANDLER" "$SMR_PARAMETERS"`;
   fi
   if [ -n "$asyncid" ] ; then
      if [ -n "$SMR_TUPLE_FLAGS" ] && [ "NULL" != "$SMR_TUPLE_FLAGS" ] ; then
         sysevent setoptions "$SMR_EVENT_NAME" "$SMR_TUPLE_FLAGS"
      fi
      sm_save_async "$SMR_SERVICE" "$SMR_EVENT_NAME" "$asyncid"
   fi
   ulog srvmgr status "($$) Registered $SMR_SERVICE for $SMR_EVENT_NAME"
} 

#------------------------------------------------------------------
# Name       : sm_register_for_default_events
# Parameters : 
#   $1   is the name of the service to register for default events
#   $2   is the path and name of the handler to activate upon default event
#
# This registers the service for:
#   SERVICE_NAME-start, SERVICE_NAME-stop, SERVICE_NAME-restart
# 
# Note that the default services are registered as sysevent events
#------------------------------------------------------------------
sm_register_for_default_events() {
   RDE_SERVICE=$1
   RDE_HANDLER=$2

   if [ -z "$RDE_SERVICE" ] || [ -z "$RDE_HANDLER" ] ; then
      return 1
   fi

   # default events are all sysevent events (tuple flag = $TUPLE_FLAG_EVENT)
   RDE_START_STRING="${RDE_SERVICE}-start|${RDE_HANDLER}|NULL|${TUPLE_FLAG_EVENT}"
   RDE_STOP_STRING="${RDE_SERVICE}-stop|${RDE_HANDLER}|NULL|${TUPLE_FLAG_EVENT}"
   RDE_RESTART_STRING="${RDE_SERVICE}-restart|${RDE_HANDLER}|NULL|${TUPLE_FLAG_EVENT}"

   sm_register_one_event "$RDE_SERVICE" "$RDE_START_STRING"
   sm_register_one_event "$RDE_SERVICE" "$RDE_STOP_STRING"
   sm_register_one_event "$RDE_SERVICE" "$RDE_RESTART_STRING"
}

##################################################################################
#                          MAIN FUNCTIONS
#
# These are the functions that will be called by most services
# registering/unregistering for events
##################################################################################

#------------------------------------------------------------------
# Name       : sm_unregister
# Parameters :
#   $1   is the name of the service to unregister all events
#
# This will remove all event registrations owned by this service,
# as long as the async id returned from each registration was
# stored in a sysevent tuple name with the correct format 
#------------------------------------------------------------------
sm_unregister() {
   SMU_SERVICE=$1

   # remove the default events
   for SMU_EVENT in ${1}-start ${1}-stop ${1}-restart
   do
      SMU_STR=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMU_EVENT}"`
      if [ -n "$SMU_STR" ] ; then
         SMU_ASYNC=`echo "$SMU_STR" | cut -f 2,3 -d ' '`
         if [ -n "$SMU_ASYNC" ] ; then
            sysevent rm_async "$SMU_ASYNC"
            ulog srvmgr status "Unregistered $1 from ${SMU_EVENT}"
         fi
         sysevent set ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMU_EVENT}"
      fi
   done

   # remove the custom events
   SMU_IDX=1
   SMU_STR=`sysevent get ${SM_PREFIX}${1}${SM_POSTFIX}_${SMU_IDX}`
   while [ -n "$SMU_STR" ] ; do
      SMU_EVENT=`echo "$SMU_STR" | cut -f 1 -d ' '`
      SMU_ASYNC=`echo "$SMU_STR" | cut -f 2,3 -d ' '`
      if [ -n "$SMU_ASYNC" ] ; then
         sysevent rm_async "$SMU_ASYNC"
         ulog srvmgr status "Unregistered $SMU_SERVICE from $SMU_EVENT"
      fi
      sysevent set ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMU_IDX}"
      SMU_IDX=`expr $SMU_IDX + 1`
      SMU_STR=`sysevent get ${SM_PREFIX}"${1}"${SM_POSTFIX}_"${SMU_IDX}"`
   done

   # register the service status as stopped
   sysevent set "${SMU_SERVICE}"-status stopped
}

#------------------------------------------------------------------
# Name       : sm_register
# Parameters :
#   $1  is the name of the service which is registering for the event
#   $2  is the path/name of the handler to be invoked for the default events
#   $3 is a string of custom events string. Each custom event string 
#      is delimited by a ';'
#
#  Note   : The service status will be set to stopped
#------------------------------------------------------------------
sm_register() {
   SM_SERVICE=$1
   SM_EVENT_HANDLER=$2
   SM_CUSTOM_EVENTS=$3

   if [ -z "$SM_SERVICE" ] ; then
      return 1
   fi
   if [ -z "$SM_EVENT_HANDLER" ] ; then
      return 1
   fi

   # unregister for any events currently registered to this service
   sm_unregister "$SM_SERVICE"


   # register for the default events SERVICE-start, SERVICE-stop, SERVICE-restart
   if [ "NULL" != "$SM_EVENT_HANDLER" ] ; then
      sm_register_for_default_events "$SM_SERVICE" "$SM_EVENT_HANDLER"
   fi

   # register for custom events
   if [ -n "$SM_CUSTOM_EVENTS" ] && [ "NULL" != "$SM_CUSTOM_EVENTS" ] ; then 
      SAVEIFS=$IFS
      IFS=';'
      for custom in $SM_CUSTOM_EVENTS ; do
         if [ -n "$custom" ] && [ " " != "$custom" ] ; then
            IFS=$SAVEIFS
            sm_register_one_event "$SM_SERVICE" "$custom"
            IFS=';'
         fi
      done
      IFS=$SAVEIFS
   fi

   # no need to register the service status as stopped
   # it was already done in the sm_unregister
   #sysevent set ${SM_SERVICE}-status stopped
}
