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
# This script is used to monitor process that are suppose to be
# running and, if not they are restarted
#
# There are many ways to use this script
#
# 1) without any arg, it checks the processes to be monitored
#
# 2) register/unregister a feature to be monitored,
#    Usage: pmon.sh [register | unregister] <feature-name>
#    Eg: pmon.sh register httpd
#    Eg: pmon.sh unregister ssh
#
# 2) Set (or unset) process details of a feature to be monitored,
#    Usage: pmon.sh setproc <feature-name> <process-name> <pidfile|none> <restart-cmd>
#    Usage: pmon.sh unsetproc <feature-name>
#    Eg: pmon.sh setproc httpd /var/run/mini_httpd.pid "/etc/utopia/service.d/service_httpd.sh httpd-restart"
#    Eg: pmon.sh unsetproc httpd
#
# When the feature is enabled AND started, appropriate <feature>.sh script need to
# add this pmon sysevent entry with more details about itself
#    pmon_proc_<feature>
#
# WARNING: sysevent isn't locked down, so we are not thread-safe
#          to aleviate this, register & enable are seperated into
#          two different operations.
#
#------------------------------------------------------------------
# Sysevent namespace
#   pmon_feature_count - current maximum 'number' of pmon_proc_X entries
#       always monetonically increases. register can fill holes caused
#       by unregister
#   pmon_feature_<x> - list of features to be monitered by process-monitor
#   pmon_proc_<feature> - process details (feature_name process_name pid/none restart-cmd)
#       Could be empty or "" string due to an unregister
#       such entries are silently ignored
#------------------------------------------------------------------
#
# Config file format
#	<process-name> <pid-file | none> <restart-cmd>
# For eg:
#	"mini_httpd /var/run/mini_httpd.pid /etc/init.d/httpd.sh restart"
#	"samba		none					/etc/init.d/samba restart
#
#------------------------------------------------------------------

do_check_process() {

	UPTIME=$(cut -d. -f1 /proc/uptime)
	if [ "$UPTIME" -lt 600 ]
	then
		echo "Uptime is less than 10 mins, exiting from pmon."
		exit 0
	fi

	# echo "[utopia] Running process monitor" > /dev/console

	LOCAL_CONF_FILE=/tmp/pmon.conf$$

	# Add static pmon entries
	echo "syseventd	/var/run/syseventd.pid /etc/utopia/service.d/syseventd_restart.sh" > $LOCAL_CONF_FILE

	# Add dynamic pmon entries stashed in sysevent
	# by various modules
	COUNT=$(sysevent get pmon_feature_count)
	if [ -z "$COUNT" ]
	then
		COUNT=0
	fi

	for ct in $(seq 1 $COUNT)
	do
		feature=$(sysevent get pmon_feature_"$ct")
		if [ -n "$feature" ]
		then
			PROC_ENTRY=$(sysevent get pmon_proc_"$feature")
			if [ -n "$PROC_ENTRY" ]
			then
				process_name=$(echo "$PROC_ENTRY" | cut -d' ' -f1)
				pid=$(echo "$PROC_ENTRY" | cut -d' ' -f2)
				restartcmd=$(echo "$PROC_ENTRY" | cut -d' ' -f3-)
				if [ -n "$process_name" ] && [ -n "$pid" ] && [ -n "$restartcmd" ]
				then
					echo "$process_name $pid $restartcmd" >> $LOCAL_CONF_FILE
				fi
			fi
		fi
	done

	cat $LOCAL_CONF_FILE > /tmp/pmon.conf
	rm -f $LOCAL_CONF_FILE
	pmon /tmp/pmon.conf
}

do_register()
{
	if [ -z "$1" ]
	then
		echo "pmon-register: invalid parameter [$1]" > /dev/console
		return 1
	fi

	# echo "[utopia] process monitor register feature [$1]" > /dev/console

	COUNT=$(sysevent get pmon_feature_count)
	if [ -z "$COUNT" ]
	then
		COUNT=0
	fi

	FREE_SLOT=0

	for ct in $(seq 1 $COUNT)
	do
		FEATURE=$(sysevent get pmon_feature_"$ct")
		if [ -z "$FEATURE" ]
		then
			FREE_SLOT=$ct
		else
			if [ "$FEATURE" = "$1" ]
			then
				# echo "pmon-register: already monitoring $FEATURE, nothing to do" > /dev/console
				return
			fi
		fi
	done

	if [ "$FREE_SLOT" != "0" ]
	then
		SLOT=$FREE_SLOT
	else
		COUNT=$((COUNT+1))
		SLOT=$COUNT
		sysevent set pmon_feature_count $COUNT
	fi

	sysevent set pmon_feature_$SLOT "$1"
}

do_unregister()
{
	if [ -z "$1" ]
	then
		# echo "pmon-unregister: invalid parameter [$1]" > /dev/console
		return 1
	fi

	# echo "[utopia] process monitor unregister feature [$1]" > /dev/console

	COUNT=$(sysevent get pmon_feature_count)
	if [ -z "$COUNT" ]
	then
		COUNT=0
	fi

	for ct in $(seq 1 $COUNT)
	do
		feature=$(sysevent get pmon_feature_"$ct")
		if [ "$feature" = "$1" ]
		then
			sysevent set pmon_feature_"$ct"
			sysevent set pmon_proc_"$feature"
			return
		fi
	done

	# echo "pmon-unregister: entry for $1 not found" > /dev/console
        
}

do_setproc ()
{
	if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ]
	then
		echo "pmon-setproc: invalid parameter(s) " > /dev/console
		return 1
	fi

	sysevent set pmon_proc_"$1" "$2 $3 $4"
}

do_unsetproc ()
{
	if [ -z "$1" ]
	then
		echo "pmon-unsetproc: invalid parameter " > /dev/console
		return 1
	fi

	sysevent set pmon_proc_"$1"
}

case "$1" in
	register)	do_register "$2" "$3" "$4" ;;
	unregister)	do_unregister "$2" ;;
	setproc)	do_setproc "$2" "$3" "$4" "$5" ;;
	unsetproc)	do_unsetproc "$2" ;;
	*)		do_check_process ;;
esac
