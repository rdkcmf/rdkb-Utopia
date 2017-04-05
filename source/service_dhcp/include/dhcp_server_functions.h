int prepare_hostname();
void calculate_dhcp_range (FILE *);
void prepare_dhcp_conf_static_hosts();
void prepare_dhcp_options_wan_dns();
void prepare_whitelist_urls(FILE *);
void do_extra_pools(FILE *local_dhcpconf_file);
int prepare_dhcp_conf();
