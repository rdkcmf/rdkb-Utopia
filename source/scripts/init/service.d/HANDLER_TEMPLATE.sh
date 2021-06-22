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
#             handler_template.sh 
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

#------------------------------------------------------------------
# name of this service
# This name MUST correspond to the registration of this service.
# Usually the registration occurs in /etc/utopia/registration.d
# The registration code will refer to the path/name of this handler
# to be activated upon default events (and other events)
#------------------------------------------------------------------
SERVICE_NAME="new_service"


#-----------------------------------------------------------------
# create_conf_file
#
# These are examples of how a configuration file can be created
# They are just here as a guideline
#-----------------------------------------------------------------
CONF_FILE="/tmp/foss_service.txt"

create_conf_file () {
(
cat <<'End-of-Text'
From here on everything word will be written 
to the config file.
There is no interpretation.
   If you indent here then the output will include that indent.
If not then the output will also not be indented.
Shell variables such as $$, $1, $CONF_FILE etc are NOT interpreted.
# Shell comments are include as written.
)
End-of-Text
) > $CONF_FILE
   echo "RETURNING 0" > /dev/console
   return 0
}

create_conf_file2 () {
   echo "This is another way to create a config file" > $CONF_FILE
   echo "In this case shell variables such as $$, $1, $CONF_FILE are interpreted and substitued" >> $CONF_FILE
   echo "# comment arent interpreted" >> $CONF_FILE
   echo "RETURNING 1" > /dev/console
   return 1
}


create_conf_file3() {
   # syscfg values can be extracted in batch mode using utctx_cmd.
   # This command returns the requested syscfg tuples in the form
   # SYSCFG_name='value'
   # For simple syscfg tuples such as wan_proto you would get
   #  utctx_cmd get wan_proto
   #  SYSCFG_wan_proto='dhcp'
   # For syscfg tuples belonging to a namespace you would get
   #  utctx_cmd get namespace::name
   #  SYSCFG_namespace_name='foo'
   # 
   # After saving the return from a batched syscfg get call you can 
   # eval the string into shell variables named SYSFG_

utctx_cmd set foo::bar=1 foo_foo=2 bar__foo=3 frappo=4
FOO=`utctx_cmd get foo::bar foo::foo  bar::foo wan_proto frappo`
eval "$FOO"

cat << EOM > $CONF_FILE
From here on this is interpretation if you want.
For example Shell variables such as $$, $1, $CONF_FILE etc are interpreted
   foo_bar=$SYSCFG_foo_bar;
   foo_foo=$SYSCFG_foo_foo;
   bar_foo=$SYSCFG_bar_foo
   wan_proto=$SYSCFG_wan_proto
   frappo=$SYSCFG_frappo
EOM
}


#----------------------------------------------------------------------------------------
#                     Default Event Handlers
#
# Each service has three default events that it should handle
#    ${SERVICE_NAME}-start
#    ${SERVICE_NAME}-stop
#    ${SERVICE_NAME}-restart
#
# For each case:
#   - Clear the service's errinfo
#   - Set the service's status 
#   - Do the work
#   - Check the error code (check_err will set service's status and service's errinfo upon error)
#   - If no error then set the service's status
#----------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
#  function   : service_init
#  - optional procedure to retrieve syscfg configuration data using utctx_cmd
#    this is a protected way of accessing syscfg
#-------------------------------------------------------------------------------------------
service_init ()
{
    utctx_cmd set foo::bar=1 foo_foo=2 bar__foo=3 frappo=4
    FOO=`utctx_cmd get foo::bar foo::foo  bar::foo wan_proto frappo`
    eval "$FOO"
}

#-------------------------------------------------------------------------------------------
#  function   : service_start
#  - Set service-status to starting
#  - Add code to read normalized configuration data from syscfg and/or sysevent 
#  - Create configuration files for the service
#  - Start any service processes 
#  - Set service-status to started
#
#  check_err will check for a non zero return code of the last called function
#  set the service-status to error, and set the service-errinfo, and then exit
#-------------------------------------------------------------------------------------------
service_start ()
{
   # wait_till_end_state will wait a reasonable amount of time waiting for ${SERVICE_NAME}
   # to finish transitional states (stopping | starting)
   wait_till_end_state ${SERVICE_NAME}

   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "started" != "$STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-errinfo 
      sysevent set ${SERVICE_NAME}-status starting
      create_conf_file
      create_conf_file2 "$@"
      create_conf_file3 "$@"
      ...
      check_err $? "Couldnt handle start"
      sysevent set ${SERVICE_NAME}-status started
   fi
}

#-------------------------------------------------------------------------------------------
#  function   : service_stop
#  - Set service-status to stopping
#  - Stop any service processes 
#  - Delete configuration files for the service
#  - Set service-status to stopped
#
#  check_err will check for a non zero return code of the last called function
#  set the service-status to error, and set the service-errinfo, and then exit
#-------------------------------------------------------------------------------------------
service_stop ()
{
   # wait_till_end_state will wait a reasonable amount of time waiting for ${SERVICE_NAME}
   # to finish transitional states (stopping | starting)
   wait_till_end_state ${SERVICE_NAME}

   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "stopped" != "$STATUS" ] ; then
      sysevent set ${SERVICE_NAME}-errinfo 
      sysevent set ${SERVICE_NAME}-status stopping
      ....
      check_err $? "Couldnt handle stop"
      sysevent set ${SERVICE_NAME}-status stopped
   fi
}

#-------------------------------------------------------------------------------------------
# Entry
# The first parameter is the name of the event that caused this handler to be activated
#-------------------------------------------------------------------------------------------

service_init 

case "$1" in
   "${SERVICE_NAME}-start")
      service_start "$@"
      ;;
   "${SERVICE_NAME}-stop")
      service_stop
      ;;
   "${SERVICE_NAME}-restart")
      service_stop
      service_start "$@"
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

   *)
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
