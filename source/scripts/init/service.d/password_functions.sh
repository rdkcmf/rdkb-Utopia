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


PASSWD_FILE=/etc/shadow
TMP_PASSWORD_FILE=/tmp/.tmp_passwd

# change the encrypted password for a user in /etc/shadow
# parameter 1 is the name of the user
# parameter 2 is a file containing the encrypted password
update_shadow() {
   TMP_FILE=/tmp/.sed_pw_temp
   newpw=`cat "$2"`
   if [ -z "$newpw" ] ; then
      return 0
   fi

   oldpw=`grep "$1" $PASSWD_FILE | cut -f 2 -d ':'`

   # since the password strings could contain '/' we need to try a couple of
   # sed delimiters
   eval "sed 's/${oldpw}/${newpw}/g' $PASSWD_FILE" > $TMP_FILE

   if [ -z "`cat $TMP_FILE`" ] ; then
      eval "sed 's%${oldpw}%${newpw}%g' $PASSWD_FILE" > $TMP_FILE
   fi
   if [ -z "`cat $TMP_FILE`" ] ; then
      eval "sed 's]${oldpw}]${newpw}]g' $PASSWD_FILE" > $TMP_FILE
   fi
   if [ -z "`cat $TMP_FILE`" ] ; then
      eval "sed 's!${oldpw}!${newpw}!g' $PASSWD_FILE" > $TMP_FILE
   fi

    # one of those should have worked
   if [ -n "`cat $TMP_FILE`" ] ; then
      cat $TMP_FILE > $PASSWD_FILE
   fi
   rm -f $TMP_FILE
}

# given a password root, create a password for admin and store it in syscfg
gen_admin_passwd() {
   SEED_PW=$1
   if [  -z "$SEED_PW" ] ; then
      SEED_PW="admin"
   fi

   SEED_PW=${SEED_PW}_admin

   cryptpw -m md5 $SEED_PW 1r2FkUle > $TMP_PASSWORD_FILE

   # update the shadow file 
   update_shadow admin $TMP_PASSWORD_FILE

   # return the encrypted password to the caller
   cat $TMP_PASSWORD_FILE
}

# given a password root, create a password for root and store it in syscfg
gen_root_passwd() {
   SEED_PW=$1
   if [ -z "$SEED_PW" ] ; then
      SEED_PW="admin"
   fi

   SEED_PW=${SEED_PW}_root

   cryptpw -m md5 $SEED_PW 1f4Menrx > $TMP_PASSWORD_FILE

   # update the shadow file 
   update_shadow root $TMP_PASSWORD_FILE

   # return the encrypted password to the caller
   cat $TMP_PASSWORD_FILE
}

case "$1" in
  admin_pw)
      gen_admin_passwd "$2"
      ;;
  root_pw)
      gen_root_passwd "$2"
      ;;
  *)
     exit 3
     ;;
esac

