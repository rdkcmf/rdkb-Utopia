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

#ifndef  _DHCP_SERVER_FUNCTIONS_H
#define  _DHCP_SERVER_FUNCTIONS_H
int prepare_hostname();
void calculate_dhcp_range (FILE *local_dhcpconf_file, char *prefix);
void prepare_dhcp_conf_static_hosts();
void prepare_dhcp_options_wan_dns();
void prepare_whitelist_urls(FILE *);
void do_extra_pools (FILE *local_dhcpconf_file, char *prefix, unsigned char bDhcpNs_Enabled, char *pWan_Dhcp_Dns);
int prepare_dhcp_conf();
void check_and_get_wan_dhcp_dns( char *pl_cWan_Dhcp_Dns );
void get_dhcp_option_for_brlan0( char *pDhcpNs_OptionString );
void prepare_static_dns_urls(FILE *fp_local_dhcp_conf);
void UpdateDhcpConfChangeBasedOnEvent();
#endif /* _DHCP_SERVER_FUNCTIONS_H */
