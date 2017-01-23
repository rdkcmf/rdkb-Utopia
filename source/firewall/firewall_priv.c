/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/


/*
 * Custom Functions
 */
#include <stdio.h>
#include "firewall_custom.h"

void do_device_based_pp_disabled_appendrule(FILE *fp, const char *ins_num, const char *lan_ifname, const char *query)
{
#if !defined(_PLATFORM_RASPBERRYPI_)
     fprintf(fp, ":pp_disabled_%s - [0:0]\n", ins_num);
     fprintf(fp, "-A pp_disabled -j pp_disabled_%s\n", ins_num);
     fprintf(fp, "-A pp_disabled -i %s -m mac --mac-source %s -p tcp -m multiport --dports 80,443 -m state --state ESTABLISHED -m connbytes --connbytes 0:5 --connbytes-dir original --connbytes-mode packets -j GWMETA --dis-pp\n", lan_ifname, query);
#endif
}

void do_device_based_pp_disabled_ip_appendrule(FILE *fp, const char *ins_num, const char *ipAddr)
{
#if !defined(_PLATFORM_RASPBERRYPI_)
	fprintf(fp, "-A pp_disabled_%s -d %s -p tcp -m multiport --sports 80,443 -m state --state ESTABLISHED -m connbytes --connbytes 0:5 --connbytes-dir reply --connbytes-mode packets -j GWMETA --dis-pp\n", ins_num, ipAddr);
#endif
}

int do_parcon_mgmt_lan2wan_pc_site_appendrule(FILE *fp)
{
#if !defined(_PLATFORM_RASPBERRYPI_)
	fprintf(fp, "-A lan2wan_pc_site -p tcp -m multiport --dports 80,443,8080 -m state --state ESTABLISHED -m "
				"connbytes --connbytes 0:5 --connbytes-dir original --connbytes-mode packets -j GWMETA --dis-pp\n");
#endif
	return 1;
}

void do_parcon_mgmt_lan2wan_pc_site_insertrule(FILE *fp, int index, char *nstdPort)
{
#if !defined(_PLATFORM_RASPBERRYPI_)
	fprintf(fp, "-I lan2wan_pc_site %d -p tcp -m tcp --dport %s -m state --state ESTABLISHED -m "
			"connbytes --connbytes 0:5 --connbytes-dir original --connbytes-mode packets -j GWMETA "
			"--dis-pp\n", index, nstdPort);
#endif
}

