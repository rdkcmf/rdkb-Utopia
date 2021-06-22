#!/usr/bin/env sh
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


#
# ShutdownReady script for DRG39xx
#
# This script identifies persistent storage filesystems that are mounted and
# writeable and attempts to unmount them.  This is done to prevent corruption
# of the persistent media when power is later removed.
#
# WARNING: This script assumes a NFS/Flash based root filesystem, and assumes
# all HDD/USB mounted filesystems are not system critical.  It also does not
# currently support logical volumes.
#
# TODO: Consider sending a "wall" warning message when invoked
#

# We won't be able to unmount the filesystem this script is executing from, or
# the filesystem the caller is currently in, but we can at least facilitate the
# error message that will be printed by changing our current working directory.
cd /

# Find all the writeable Flash/HDD/USB persistent mounts
pmounts=`egrep "^mtd|^/dev/[sh]d[:alpha:]" /proc/mounts | grep "rw," | cut -d' ' -f2`

# Attempt to unmount the filesystems
for mount in $pmounts
do
    fuser -km "$mount"
    echo "unmount $mount..." > /dev/console
    umount "$mount"
done
