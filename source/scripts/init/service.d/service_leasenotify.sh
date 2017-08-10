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
source /etc/utopia/service.d/log_capture_path.sh

BIN_NAME="notify_lease"

service_start() {
	NOTIFY_PID=`pidof $BIN_NAME`
	if [ "x$NOTIFY_PID" = "x" ] && [ -f /usr/bin/$BIN_NAME ]; then
      		echo "RDKB_NOTIFYLEASE : Mesh is enabled, initializing notify_lease"
       		/usr/bin/$BIN_NAME
	fi

}

service_stop() {
	
	NOTIFY_PID=`pidof $BIN_NAME`
	if [ "x$NOTIFY_PID" != "x" ] && [ -f /usr/bin/$BIN_NAME ]; then
      		echo "RDKB_NOTIFYLEASE : Mesh is disabled , stopping notify_lease"
       		kill -9 `pidof $BIN_NAME`
	fi
}

case "$1" in
  mesh_enable)
      	Mesh_Enable=`echo $2 | cut -d "|" -f 2`
	if [ "$Mesh_Enable" = "true" ]; then
		service_start
	else
		service_stop
	fi
	
      ;;
 *)
	echo "Doesn't support $1 event"
        exit 1
	;;
esac
