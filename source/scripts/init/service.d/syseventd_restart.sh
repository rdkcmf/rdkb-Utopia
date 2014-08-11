#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh

#------------------------------------------------------------------
# This script is used to restart syseventd daemon and to
# bring its internal state to as before by re-registring all apps
# This script is typically called by process monitor if & when 
# it detects syseventd daemon has died
# This script doesn't take any parameter
#------------------------------------------------------------------


do_restart() {
   ulog system "Restarting sysevent subsystem'
   /sbin/syseventd

   sleep 2

   apply_system_defaults

   INIT_DIR=/etc/utopia/registration.d
   # run all executables in the sysevent registration directory
   execute_dir $INIT_DIR

   ulog system "Restarting lan and wan'
   sysevent set lan-start
   sysevent set wan-start
}

#------------------------------------------------------------------

do_restart

