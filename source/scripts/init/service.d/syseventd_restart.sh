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

source /etc/utopia/service.d/ulog_functions.sh

#------------------------------------------------------------------
# This script is used to restart syseventd daemon and to
# bring its internal state to as before by re-registring all apps
# This script is typically called by process monitor if & when 
# it detects syseventd daemon has died
# This script doesn't take any parameter
#------------------------------------------------------------------


do_restart() {
   ulog system "Restarting sysevent subsystem"
   /sbin/syseventd

   sleep 2

   apply_system_defaults

   INIT_DIR=/etc/utopia/registration.d
   # run all executables in the sysevent registration directory
   execute_dir $INIT_DIR

   ulog system "Restarting lan and wan"
   sysevent set lan-start
   sysevent set wan-start
}

#------------------------------------------------------------------

do_restart

