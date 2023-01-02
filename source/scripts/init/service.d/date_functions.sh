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
# Common timestamp parsing routines
# Note that our timestamp has format MM:DD:YY_HH:MM:SS
#------------------------------------------------------------------

# get_current_time makes a current timestamp
get_current_time() {
   CURRENT_TIME=`date '+%m:%d:%y_%H:%M:%S'`
   echo "$CURRENT_TIME"
}

# get_month is called with a timestamp and returns the month
get_month() {
   TIMESTAMP=$1
   MONTH=0
   SAVEIFS=$IFS
   IFS=:
   for p in $TIMESTAMP
   do
      if [ "0" = "$MONTH" ] ; then
         MONTH=$p
      fi
   done
   IFS=$SAVEIFS
   echo "$MONTH"
}

# get_day is called with a timestamp and returns the day
get_day() {
   TIMESTAMP=$1
   DAY=0
   MONTH=0
   SAVEIFS=$IFS
   IFS=:
   for p in $TIMESTAMP
   do
      if [ "0" = "$MONTH" ] ; then
         MONTH=$p
      elif [ "0" = "$DAY" ] ; then
         DAY=$p
      fi
   done
   IFS=$SAVEIFS
   echo "$DAY"
}

# get_year is called with a timestamp and returns the year
get_year() {
   TIMESTAMP=$1
   DAY=0
   MONTH=0
   YEAR=0
   YEAR2=0
   SAVEIFS=$IFS
   IFS=:
   for p in $TIMESTAMP
   do
      if [ "0" = "$MONTH" ] ; then
         MONTH=$p
      elif [ "0" = "$DAY" ] ; then
         DAY=$p
      elif [ "0" = "$YEAR" ] ; then
         YEAR=$p
      fi
   done
   IFS=_
   for p in $YEAR
   do
      if [ "0" = "$YEAR2" ] ; then
         YEAR2=$p
      fi
   done
   IFS=$SAVEIFS
   echo "$YEAR2"
}

# get_hour is called with a timestamp and returns the hour
get_hour() {
   TIMESTAMP=$1
   STRING1=""
   STRING2=""
   SAVEIFS=$IFS
   IFS=_
   for p in $TIMESTAMP
   do
      if [ -z "$STRING1" ] ; then
         STRING1=$p
      elif [ -z "$STRING2" ] ; then
         STRING2=$p
      fi
   done
   HOUR=0
   MIN=0
   SEC=0
   IFS=:
   for p in $STRING2
   do
      if [ "0" = "$HOUR" ] ; then
         HOUR=$p
      elif [ "0" = "$MIN" ] ; then
         MIN=$p
      elif [ "0" = "$SEC" ] ; then
         SEC=$p
      fi
   done
   IFS=$SAVEIFS
   echo "$HOUR"
}

# get_min is called with a timestamp and returns the min
get_min() {
   TIMESTAMP=$1
   STRING1=""
   STRING2=""
   SAVEIFS=$IFS
   IFS=_
   for p in $TIMESTAMP
   do
      if [ -z "$STRING1" ] ; then
         STRING1=$p
      elif [ -z "$STRING2" ] ; then
         STRING2=$p
      fi
   done
   HOUR=0
   MIN=0
   SEC=0
   IFS=:
   for p in $STRING2
   do
      if [ "0" = "$HOUR" ] ; then
         HOUR=$p
      elif [ "0" = "$MIN" ] ; then
         MIN=$p
      elif [ "0" = "$SEC" ] ; then
         SEC=$p
      fi
   done
   IFS=$SAVEIFS
   echo "$MIN"
}

# get_sec is called with a timestamp and returns the sec
get_sec() {
   TIMESTAMP=$1
   STRING1=""
   STRING2=""
   SAVEIFS=$IFS
   IFS=_
   for p in $TIMESTAMP
   do
      if [ -z "$STRING1" ] ; then
         STRING1=$p
      elif [ -z "$STRING2" ] ; then
         STRING2=$p
      fi
   done
   HOUR=0
   MIN=0
   SEC=0
   IFS=:
   for p in $STRING2
   do
      if [ "0" = "$HOUR" ] ; then
         HOUR=$p
      elif [ "0" = "$MIN" ] ; then
         MIN=$p
      elif [ "0" = "$SEC" ] ; then
         SEC=$p
      fi
   done
   IFS=$SAVEIFS
   echo "$SEC"
}

# days_in_month returns the number of days in a given month
days_in_month() {
   MONTH=$1
   if [ "2" = "$MONTH" ] ; then
      echo "28"
   elif [ "02" = "$MONTH" ] ; then
      echo "28"
   elif [ "4" = "$MONTH" ] ; then
      echo "30"
   elif [ "4" = "$MONTH" ] ; then
      echo "30"
   elif [ "04" = "$MONTH" ] ; then
      echo "30"
   elif [ "6" = "$MONTH" ] ; then
      echo "30"
   elif [ "06" = "$MONTH" ] ; then
      echo "30"
   elif [ "9" = "$MONTH" ] ; then
      echo "30"
   elif [ "09" = "$MONTH" ] ; then
      echo "30"
   elif [ "11" = "$MONTH" ] ; then
      echo "30"
   else
      echo "31"
   fi
}

# days_in_months returns the number of days in the months given.
# The count of days includes the starting and ending months
days_in_months() {
   MONTH1=$1
   MONTH2=$2
   ACCUMULATOR=0

   if [ "$MONTH2" -ge "$MONTH1" ] ; then
      COUNTER=`expr "$MONTH2" - "$MONTH1"`
      COUNTER=`expr "$COUNTER" + 1`
   else
      COUNTER=`expr "$MONTH1" - "$MONTH2"`
      COUNTER=`expr 13 - "$COUNTER"`
   fi

   while [ "$COUNTER" -gt 0 ]
   do
      MONTH=`days_in_month "$MONTH1"`
      ACCUMULATOR=`expr $ACCUMULATOR + "$MONTH"`
      COUNTER=`expr "$COUNTER" - 1`
      MONTH1=`expr "$MONTH1" + 1`
      if [ "12" -lt "$MONTH1" ] ; then
         MONTH1=1
      fi
   done
   echo "$ACCUMULATOR"
}


days_from_basedate() {
  YEAR1=00
  MONTH1=01
  DAY1=01
  YEAR2=`get_year "$1"`
  MONTH2=`get_month "$1"`
  DAY2=`get_day "$1"`

  ACCUMULATOR=0

  # figure out the number of days in the whole years between dates
  MULTIPLIER=`expr "$YEAR2" - $YEAR1`
  PRODUCT=`expr "$MULTIPLIER" \* 365`
  ACCUMULATOR=`expr $ACCUMULATOR + "$PRODUCT"`

  # figure out the days in the whole months between the dates
  PRODUCT=`days_in_months $MONTH1 "$MONTH2"`
  ACCUMULATOR=`expr "$ACCUMULATOR" + "$PRODUCT"`

  PRODUCT=`expr "$DAY2" - $DAY1`
  ACCUMULATOR=`expr "$ACCUMULATOR" + "$PRODUCT"`

  echo "$ACCUMULATOR"
}

# delta_days is called with two timestamps and returns the diff in days
# or part thereof
delta_days() {

   DAYS1=`days_from_basedate "$1"`
   DAYS2=`days_from_basedate "$2"`

   CUMMULATIVE_DAYS=`expr "$DAYS2" - "$DAYS1"`

   echo "$CUMMULATIVE_DAYS"
}

mins_from_basedate() {
  HOUR1=00
  HOUR2=`get_hour "$1"`
  MIN2=`get_min "$1"`

  ACCUMULATOR=0

  # figure out the number of days in the whole years between dates
  MULTIPLIER=`expr "$HOUR2" - $HOUR1`
  PRODUCT=`expr "$MULTIPLIER" \* 60`
  ACCUMULATOR=`expr $ACCUMULATOR + "$PRODUCT"`

  # figure out the days in the whole months between the dates
  ACCUMULATOR=`expr "$ACCUMULATOR" + "$MIN2"`

  echo "$ACCUMULATOR"
}

# delta_mins is called with two timestamps and returns the diff in minutes
# NOTE: It only looks at hours and mins NOT days
delta_mins() {

   MINS1=`mins_from_basedate "$1"`
   MINS2=`mins_from_basedate "$2"`

   CUMMULATIVE_MINS=`expr "$MINS2" - "$MINS1"`

   echo "$CUMMULATIVE_MINS"
}
