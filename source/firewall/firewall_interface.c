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

#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "firewall_custom.h"

__attribute__((weak))
int firewall_lib_init(void *bus_handle, int sysevent_fd, token_t sysevent_token)
{
    FIREWALL_DEBUG("Entering firewall_lib_init\n");
    FIREWALL_DEBUG("Exiting firewall_lib_init\n");
    return 0;
}

__attribute__((weak))
void firewall_lib_deinit()
{
    FIREWALL_DEBUG("Entering firewall_lib_deinit\n");       
    FIREWALL_DEBUG("Exiting firewall_lib_deinit\n");       
}

/*
 *  Enable portmap traffic only on loopback and PEER IP
 */
__attribute__((weak))
void filterPortMap(FILE *filt_fp)
{
    FIREWALL_DEBUG("Entering filterPortMap\n");
    FIREWALL_DEBUG("Exiting filterPortMap\n");
}

/*
 * Open special ports from wan to self/lan
 */
__attribute__((weak))
void do_openPorts(FILE *filter_fp)
{
    FIREWALL_DEBUG("Entering do_openPorts\n");
    FIREWALL_DEBUG("Exiting do_openPorts\n"); 	  
}

/*
 * Forward special ports from wan to self/lan
 */
__attribute__((weak))
void do_forwardPorts(FILE *filter_fp)
{
    FIREWALL_DEBUG("Entering do_forwardPorts\n");
    FIREWALL_DEBUG("Exiting do_forwardPorts\n");
}

/*
 * Add Video Analytics to allow only from lan interface
 */
__attribute__((weak))
void do_OpenVideoAnalyticsPort (FILE *filter_fp)
{
    FIREWALL_DEBUG("Entering do_OpenVideoAnalyticsPort\n");
    FIREWALL_DEBUG("Exiting do_OpenVideoAnalyticsPort\n");
}

/*              
 ==========================================================================
                            WhilteListing IPs
 ==========================================================================
 */             
__attribute__((weak))
void do_ssh_IpAccessTable(FILE *filt_fp, const char *port, int family, const char *interface)
{
    FIREWALL_DEBUG("Entering do_ssh_IpAccessTable\n");
    FIREWALL_DEBUG("Exiting do_ssh_IpAccessTable\n");
}

__attribute__((weak))
void do_snmp_IpAccessTable(FILE *filt_fp, int family)
{
    FIREWALL_DEBUG("Entering do_snmp_IpAccessTable\n");
    FIREWALL_DEBUG("Exiting do_snmp_IpAccessTable\n");
}

__attribute__((weak))
void do_tr69_whitelistTable(FILE *filt_fp, int family)
{
    FIREWALL_DEBUG("Entering do_tr69_whitelistTable\n");
    FIREWALL_DEBUG("Exiting do_tr69_whitelistTable\n");
}


/*              
 ==========================================================================
                Xconf Markings
 ==========================================================================
 */             
__attribute__((weak))
int prepare_xconf_rules(FILE *mangle_fp)
{
    FIREWALL_DEBUG("Entering prepare_xconf_rules\n");
    FIREWALL_DEBUG("Exiting prepare_xconf_rules\n");
    return 0;
}

/*              
 ==========================================================================
                Rabid Rules
 ==========================================================================
 */
#if !(defined(_COSA_INTEL_XB3_ARM_) || defined(_COSA_BCM_MIPS_))
__attribute__((weak))
int prepare_rabid_rules(FILE *filter_fp, FILE *mangle_fp, ip_ver_t ver)
{
    FIREWALL_DEBUG("Entering prepare_rabid_rules \n");
    FIREWALL_DEBUG("Exiting prepare_rabid_rules \n");
    return 0;
}
#else
__attribute__((weak))
int prepare_rabid_rules_v2020Q3B(FILE *filter_fp, FILE *mangle_fp, ip_ver_t ver)
{
    FIREWALL_DEBUG("Entering prepare_rabid_rules \n");
    FIREWALL_DEBUG("Exiting prepare_rabid_rules \n");
    return 0;
}
#endif

__attribute__((weak))
void update_rabid_features_status()
{
    FIREWALL_DEBUG("Entering update_rabid_features_status \n");
    FIREWALL_DEBUG("Exiting update_rabid_features_status \n");
}

__attribute__((weak))
int prepare_rabid_rules_for_mapt(FILE *filter_fp, ip_ver_t ver)
{
    FIREWALL_DEBUG("Entering prepare_rabid_rules_for_mapt \n");
    FIREWALL_DEBUG("Exiting prepare_rabid_rules_for_mapt \n");
    return 0;
}


/*              
 ==========================================================================
                Ethwan MSO GUI acess Markings
 ==========================================================================
 */             
__attribute__((weak))
void ethwan_mso_gui_acess_rules(FILE *filter_fp,FILE *mangle_fp)
{
    FIREWALL_DEBUG("Entering ethwan_mso_gui_acess_rules\n");
    FIREWALL_DEBUG("Exiting ethwan_mso_gui_acess_rules\n");
}
