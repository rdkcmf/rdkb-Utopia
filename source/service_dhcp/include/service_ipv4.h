/************************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**************************************************************************/

void ipv4_up(char *);
void handle_l2_status (int, int, char *, int);
void remove_tsip_config();
void remove_tsip_asn_config();
void sync_tsip ();
void resync_tsip();
void sync_tsip_asn ();
void resync_tsip_asn();
BOOL apply_config(int l3_inst, char *staticIpv4Addr, char *staticIpv4Subnet);
void resync_all_instance();

  
