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
#endif /* _DHCP_SERVER_FUNCTIONS_H */
