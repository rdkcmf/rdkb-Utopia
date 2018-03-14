####################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
#  Copyright 2018 RDK Management
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
##################################################################################


# This program looks for trigger words the stream, and when found invokes 
# trigger process with appropriate parameters
# The input stream is /var/log/messages, and we are looking for utopia triggers 
#
# There is one command line argument : the trigger prefix in log files
# Currently this is UTOPIA.TRIGGER
#
# The success of parsing in this file is totally dependent on the format of the trigger logs 
# If the format of those iptables LOGs changes, it behooves you to change the parsing here.
#
BEGIN { trigname = ARGV[1]; delete ARGV[1]; }
{
   {
      # search for an occurance of the TRIGGER prefix
      if (index($0,trigname) > 0) {
         trigger_start = $7; 
         split(trigger_start, logprefix, ".");
         type = logprefix[3];
         if (match(type, "NEWHOST")) {
            src_str = $11;
            split(src_str,src, "=");
            ip = src[2];
            mac = $10;
            split(mac,src, "=");
            mac = substr(src[2],19,17);
            if ("" != mac && "00:00:00:00:00:00" != mac && "" != ip && "0.0.0.0" != ip) {
               system("newhost " type " " mac " " ip);
            }
         }                                   
      }   
   }   
}
END { }


