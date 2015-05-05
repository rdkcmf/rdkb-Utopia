#!/bin/sh

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

# SAVE_LAN_IPV6_PREFIX

# This function saves the previous LAN prefix if any and if different than the new one
# passed as argument.
# Ultimate goal: be able to send RA on LAN with the old prefix and preferred lifetime = 0
save_lan_ipv6_prefix ()
{
	echo "save_lan_ipv6_prefix($1)" >> $IPV6_LOG
	OLD_PREFIX="`sysevent get ipv6_prefix`"
	if [ ! -z "$OLD_PREFIX" -a "$OLD_PREFIX" != "$1" ]
	then
		echo "Saving old prefix $OLD_PREFIX" >> $IPV6_LOG
		sysevent set previous_ipv6_prefix "$OLD_PREFIX"
	fi
	sysevent set ipv6_prefix $1
}
