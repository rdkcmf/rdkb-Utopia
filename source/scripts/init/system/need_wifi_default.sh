#!/bin/sh

##################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2018 RDK Management

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

#zhicheng_qiu@cbale.comcast.com

source  /etc/device.properties
BUTTON_THRESHOLD=15
FACTORY_RESET_KEY=factory_reset
FACTORY_RESET_RGWIFI=y
FACTORY_RESET_WIFI=w
SYSCFG_MOUNT=/nvram
PUNIT_RESET_DURATION=0;
if [ "$BOX_TYPE" = "XB3" ]; then
SYSCFG_FILE="/nvram/syscfg.db"
else
SYSCFG_FILE="/opt/secure/data/syscfg.db"
fi

#first time boot
if [ ! -f $SYSCFG_FILE ]; then
   echo 1;
   exit 0;
fi

# Hardware reset
if cat /proc/P-UNIT/status | grep -q "Reset duration from shadow register"; then
   # Note: Only new P-UNIT firmwares and Linux drivers (>= 1.1.x) support this.
   PUNIT_RESET_DURATION=`cat /proc/P-UNIT/status|grep "Reset duration from shadow register"|awk -F '[ |\.]' '{ print $9 }'`
elif cat /proc/P-UNIT/status | grep -q "Last reset duration"; then
   PUNIT_RESET_DURATION=`cat /proc/P-UNIT/status|grep "Last reset duration"|awk -F '[ |\.]' '{ print $7 }'`
else
   PUNIT_RESET_DURATION=0;
fi

if test "$BUTTON_THRESHOLD" -le "$PUNIT_RESET_DURATION"; then
   echo 1;
   exit 0;
fi

#software reset
SYSCFG_FR_VAL="`syscfg get $FACTORY_RESET_KEY`"
if [ "$FACTORY_RESET_RGWIFI" = "$SYSCFG_FR_VAL" ]; then
   echo 1;
   exit 0;
elif [ "$FACTORY_RESET_WIFI" = "$SYSCFG_FR_VAL" ]; then
   echo 1;
   exit 0;
fi

echo 0;

