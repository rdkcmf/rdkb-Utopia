#!/usr/bin/env sh

#
# ShutdownReady script for DRG39xx
#
# Copyright (C) 2010 Cisco Systems, Inc.
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
    fuser -km $mount
    echo "unmount $mount..." > /dev/console
    umount $mount
done
