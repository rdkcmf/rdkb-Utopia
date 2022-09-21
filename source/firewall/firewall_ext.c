#ifdef RDKB_EXTENDER_ENABLED

#include "firewall.h"
#include "firewall_custom.h"

#include<errno.h> 


#define IPV4_TOTAL_HEADER_SIZE 40
#define IPV6_TOTAL_HEADER_SIZE 60

#define MTU_SIZE 1500

extern int  sysevent_fd ;
extern token_t        sysevent_token;

extern char cellular_ifname[32] ;
static char cellular_ipaddr[32] ;

extern char mesh_wan_ifname[32];
static char mesh_wan_ipaddr[32];

extern int mesh_wan_ipv6_num ;
extern char mesh_wan_ipv6addr[IF_IPV6ADDR_MAX][40];

int cellular_wan_ipv6_num = 0;
char cellular_wan_ipv6addr[IF_IPV6ADDR_MAX][40];

#define SYSEVENT_IPV4_MTU_SIZE "ipv4_%s_mtu"
int isExtProfile()
{
      if ( ( EXTENDER_MODE == Get_Device_Mode() ) )
      {
         return 0;
      }
      return -1;
}  

void add_if_mss_clamping(FILE *mangle_fp,int family)
{
   char mtu_event_name[128] = {0}, mtu_val[8] = {0};
   memset(mtu_event_name,0,sizeof(mtu_event_name));
   memset(mtu_val,0,sizeof(mtu_val));
   int iMtuVal=0 ,  mss_clamp_val = 0;
   snprintf(mtu_event_name,sizeof(mtu_event_name),SYSEVENT_IPV4_MTU_SIZE,cellular_ifname);

   sysevent_get(sysevent_fd, sysevent_token, mtu_event_name, mtu_val, sizeof(mtu_val));

   if(mtu_val[0] != '\0' && strlen(mtu_val) != 0 )
   {
      iMtuVal = atoi(mtu_val) ;
      if ( iMtuVal !=0 && iMtuVal != MTU_SIZE )
      {
         if(family == AF_INET)
            mss_clamp_val= iMtuVal - IPV4_TOTAL_HEADER_SIZE ;
         else if (family == AF_INET6)
            mss_clamp_val= iMtuVal - IPV6_TOTAL_HEADER_SIZE ;
         else
            return;

         fprintf(mangle_fp, "-A FORWARD -p tcp --tcp-flags SYN,RST SYN -o %s -j TCPMSS --set-mss %d\n",cellular_ifname,mss_clamp_val); 
         fprintf(mangle_fp, "-A POSTROUTING -o %s -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %d\n",cellular_ifname,mss_clamp_val); 
         if ( 0 == isExtProfile())
         {
            fprintf(mangle_fp, "-A FORWARD -p tcp --tcp-flags SYN,RST SYN -o %s -j TCPMSS --set-mss %d\n",mesh_wan_ifname,mss_clamp_val); 
            fprintf(mangle_fp, "-A POSTROUTING -o %s -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %d\n",mesh_wan_ifname,mss_clamp_val);      
         }

      }

   }

   return ;
}
/*
 *  Procedure     : prepare_subtables
 *  Purpose       : prepare the iptables-restore file that establishes all
 *                  ipv4 firewall rules with the table/subtable structure
 *  Parameters    :
 *    raw_fp         : An open file for raw subtables
 *    mangle_fp      : An open file for mangle subtables
 *    nat_fp         : An open file for nat subtables
 *    filter_fp      : An open file for filter subtables
 * Return Values  :
 *    0              : Success
 */

static int prepare_subtables_ext_mode(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
   FIREWALL_DEBUG("Entering prepare_subtables \n"); 
   
   /*
    * raw
   */
   fprintf(raw_fp, "%s\n", "*raw");


   /*
    * mangle
    */
   fprintf(mangle_fp, "%s\n", "*mangle");


      /*
    * nat
    */
   fprintf(nat_fp, "%s\n", "*nat");

      /*
    * filter
    */
   fprintf(filter_fp, "%s\n", "*filter");
   fprintf(filter_fp, "%s\n", ":wanattack - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_drop_wanattack - [0:0]");
   fprintf(filter_fp, "%s\n", ":xlog_accept_wan2lan - [0:0]");
   fprintf(filter_fp, "%s\n", ":LOG_SSH_DROP - [0:0]");
   fprintf(filter_fp, "%s\n", ":SSH_FILTER - [0:0]");
   return 0;
}  
/*
 *  Procedure     : prepare_ipv4_rule_ex_mode
 *  Purpose       : prepare ipv4 firewall
 *  Parameters    :
 *   raw_fp         : An open file for raw subtables
 *   mangle_fp      : an open file for writing mangle statements
 *   nat_fp         : an open file for writing nat statements
 *   filter_fp      : an open file for writing filter statements
 */
int prepare_ipv4_rule_ex_mode(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
   FIREWALL_DEBUG("Entering prepare_ipv4_rule_ex_mode \n"); 
   prepare_subtables_ext_mode(raw_fp, mangle_fp, nat_fp, filter_fp);

   if (strlen(cellular_ipaddr) != 0 )
      fprintf(nat_fp, "-A  POSTROUTING -o %s -j SNAT --to-source %s\n",cellular_ifname,cellular_ipaddr);

   add_if_mss_clamping(mangle_fp,AF_INET);
   if (strlen(mesh_wan_ipaddr) != 0 )
   {
      fprintf(nat_fp, "-A  PREROUTING -i %s -p udp --dport 53 -j DNAT --to-destination %s\n",mesh_wan_ifname,mesh_wan_ipaddr);
      fprintf(nat_fp, "-A  PREROUTING -i %s -p tcp --dport 53 -j DNAT --to-destination %s\n",mesh_wan_ifname,mesh_wan_ipaddr);      
   }

   fprintf(filter_fp, "-A INPUT -i lo -p udp --dport 53 -j DROP \n");
   fprintf(filter_fp, "-A INPUT -i lo -p tcp --dport 53 -j DROP \n");

   fprintf(filter_fp, "-A INPUT -i %s -j wanattack\n", cellular_ifname);

   do_wan2self_attack(filter_fp,cellular_ipaddr);

   fprintf(filter_fp, "-A INPUT -i %s -p tcp -m tcp --dport 22 -j SSH_FILTER\n",cellular_ifname);
#if defined(_WNXL11BWL_PRODUCT_REQ_)
   fprintf(filter_fp, "-A INPUT -i brlan112 -d 169.254.70.0/24 -j ACCEPT\n");
   fprintf(filter_fp, "-A INPUT -i brlan112 -m pkttype ! --pkt-type unicast -j ACCEPT\n");
   fprintf(filter_fp, "-A INPUT -i brlan113 -d 169.254.71.0/24 -j ACCEPT\n");
   fprintf(filter_fp, "-A INPUT -i brlan113 -m pkttype ! --pkt-type unicast -j ACCEPT\n");
   fprintf(filter_fp, "-A INPUT -i brebhaul -d 169.254.85.0/24 -j ACCEPT\n");
   fprintf(filter_fp, "-A INPUT -i brebhaul -m pkttype ! --pkt-type unicast -j ACCEPT\n");
#endif

   do_ssh_IpAccessTable(filter_fp, "22", AF_INET, cellular_ifname);

   fprintf(filter_fp, "-A xlog_accept_wan2lan -j ACCEPT\n");

   fprintf(filter_fp, "-A  FORWARD -i %s -o %s -j ACCEPT\n",mesh_wan_ifname,cellular_ifname);
   fprintf(filter_fp, "-A  FORWARD -i %s -o %s -j ACCEPT\n",cellular_ifname,mesh_wan_ifname);

 //  do_logs(filter_fp);

   fprintf(raw_fp, "%s\n", "COMMIT");
   fprintf(mangle_fp, "%s\n", "COMMIT");
   fprintf(nat_fp, "%s\n", "COMMIT");
   fprintf(filter_fp, "%s\n", "COMMIT");
   FIREWALL_DEBUG("Exiting prepare_enabled_ipv4_firewall \n"); 

   return 0;
}

int filter_ipv6_icmp_limit_rules(FILE *fp)
{
      FIREWALL_DEBUG("Entering filter_ipv6_icmp_limit_rules \n"); 


      // Should include --limit 10/second for most of ICMP
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 1/0 -m limit --limit 10/sec -j ACCEPT\n"); // No route
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 2 -m limit --limit 10/sec -j ACCEPT\n"); // Packet too big
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 3 -m limit --limit 10/sec -j ACCEPT\n"); // Time exceeded
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 4/1 -m limit --limit 10/sec -j ACCEPT\n"); // Unknown header type
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 4/2 -m limit --limit 10/sec -j ACCEPT\n"); // Unknown option

      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j PING_FLOOD\n", cellular_ifname); // Echo request
      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m limit --limit 10/sec -j ACCEPT\n", cellular_ifname); // Echo reply

      // Should only come from LINK LOCAL addresses, rate limited except 100/second for NA/NS and RS
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 135 -m limit --limit 100/sec -j ACCEPT\n"); // Allow NS from any type source address
      fprintf(fp, "-A INPUT -p icmpv6 -m icmp6 --icmpv6-type 136 -m limit --limit 100/sec -j ACCEPT\n"); // NA

      // But can also come from UNSPECIFIED addresses, rate limited 100/second for NS (for DAD) and MLD
      fprintf(fp, "-A INPUT -s ::/128 -p icmpv6 -m icmp6 --icmpv6-type 135 -m limit --limit 100/sec -j ACCEPT\n"); // NS
      fprintf(fp, "-A INPUT -s ::/128 -p icmpv6 -m icmp6 --icmpv6-type 143 -m limit --limit 100/sec -j ACCEPT\n"); // MLD

      // IPV6 Multicast traffic
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 130 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 131 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 132 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 143 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 151 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 152 -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A INPUT -s fe80::/64 -p icmpv6 -m icmp6 --icmpv6-type 153 -m limit --limit 10/sec -j ACCEPT\n");

      // ICMP varies and are rate limited anyway
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 1/0 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 2 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 3 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 4 -m limit --limit 100/sec -j ACCEPT\n");


      // ICMP messages for MIPv6 (assuming mobile node on the inside)
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 145 -m limit --limit 100/sec -j ACCEPT\n");
      fprintf(fp, "-A FORWARD -p icmpv6 -m icmp6 --icmpv6-type 147 -m limit --limit 100/sec -j ACCEPT\n");

      fprintf(fp, "-A PING_FLOOD -m limit --limit 10/sec -j ACCEPT\n");
      fprintf(fp, "-A PING_FLOOD -j DROP\n");

      FIREWALL_DEBUG("Exiting filter_ipv6_icmp_limit_rules \n"); 

      return 0;
}
/*

 *  Procedure     : prepare_ipv6_rule_ex_mode
 *  Purpose       : prepare ipv4 firewall
 *  Parameters    :
 *   raw_fp         : An open file for raw subtables
 *   mangle_fp      : an open file for writing mangle statements
 *   nat_fp         : an open file for writing nat statements
 *   filter_fp      : an open file for writing filter statements
 * */
int prepare_ipv6_rule_ex_mode(FILE *raw_fp, FILE *mangle_fp, FILE *nat_fp, FILE *filter_fp)
{
   FIREWALL_DEBUG("Entering prepare_ipv4_rule_ex_mode \n"); 
 //  prepare_subtables_ext_mode(raw_fp, mangle_fp, nat_fp, filter_fp);

   /*
    * raw
   */
   fprintf(raw_fp, "%s\n", "*raw");


   /*
    * mangle
    */
   fprintf(mangle_fp, "%s\n", "*mangle");


      /*
    * nat
    */
   fprintf(nat_fp, "%s\n", "*nat");

      /*
    * filter
    */

   add_if_mss_clamping(mangle_fp,AF_INET6);

   int i ;
    for(i = 0; i < mesh_wan_ipv6_num; i++){

      if(mesh_wan_ipv6addr[i][0] != '\0' )
      {
         fprintf(nat_fp, "-A  PREROUTING -i %s -p udp --dport 53 -j DNAT --to-destination %s\n",mesh_wan_ifname,mesh_wan_ipv6addr[i]);
         fprintf(nat_fp, "-A  PREROUTING -i %s -p tcp --dport 53 -j DNAT --to-destination %s\n",mesh_wan_ifname,mesh_wan_ipv6addr[i]);  
      }
    }

   memset(cellular_wan_ipv6addr,0,sizeof(cellular_wan_ipv6addr));
   get_ip6address(cellular_ifname, cellular_wan_ipv6addr, &cellular_wan_ipv6_num,IPV6_ADDR_SCOPE_GLOBAL);

    for(i = 0; i < cellular_wan_ipv6_num; i++)
    {
      if(cellular_wan_ipv6addr[i][0] != '\0' )
      {
         fprintf(nat_fp, "-A  POSTROUTING -o %s -j SNAT --to-source %s\n",cellular_ifname,cellular_wan_ipv6addr[i]);
      }
    }

   fprintf(filter_fp, "%s\n", "*filter");
   fprintf(filter_fp, "%s\n", ":LOG_SSH_DROP - [0:0]");
   fprintf(filter_fp, "%s\n", ":SSH_FILTER - [0:0]");
   fprintf(filter_fp, "%s\n", ":PING_FLOOD - [0:0]");

   fprintf(filter_fp, "-A INPUT -i lo -p udp --dport 53 -j DROP \n");
   fprintf(filter_fp, "-A INPUT -i lo -p tcp --dport 53 -j DROP \n");
   
   fprintf(filter_fp, "-A INPUT -i %s -p tcp -m tcp --dport 22 -j SSH_FILTER\n",cellular_ifname);

   filter_ipv6_icmp_limit_rules(filter_fp);
   do_ssh_IpAccessTable(filter_fp, "22", AF_INET6, cellular_ifname);

   fprintf(filter_fp, "-A  FORWARD -i %s -o %s -j ACCEPT\n",mesh_wan_ifname,cellular_ifname);
   fprintf(filter_fp, "-A  FORWARD -i %s -o %s -j ACCEPT\n",cellular_ifname,mesh_wan_ifname);

   return 0;
}

/*
 * Name           :  service_start_ext_mode
 * Purpose        :  Start firewall service on extender 
 * Parameters     :
 *    None        :
 * Return Values  :
 *    0              : Success
 *    < 0            : Error code
 */
int service_start_ext_mode ()
{
   char *filename1 = "/tmp/.ipt_ext";
   char *filename2 = "/tmp/.ipt_v6_ext";

   memset(cellular_ipaddr,0,sizeof(cellular_ipaddr));
   memset(mesh_wan_ipaddr,0,sizeof(mesh_wan_ipaddr));
   
   errno_t safec_rc = -1;

   safec_rc = strcpy_s(mesh_wan_ipaddr, sizeof(mesh_wan_ipaddr),get_iface_ipaddr(mesh_wan_ifname));
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(cellular_ipaddr, sizeof(cellular_ipaddr),get_iface_ipaddr(cellular_ifname));
   ERR_CHK(safec_rc);


   //pthread_mutex_lock(&firewall_check);
   FIREWALL_DEBUG("Inside firewall service_start()\n");

   /*  ipv4 */
   prepare_ipv4_firewall(filename1);
   v_secure_system("iptables-restore -c  < /tmp/.ipt_ext 2> /tmp/.ipv4table_ext_error");


   prepare_ipv6_firewall(filename2);
   v_secure_system("ip6tables-restore < /tmp/.ipt_v6_ext 2> /tmp/.ipv6table_ext_error");

   FIREWALL_DEBUG("Exiting firewall service_start()\n");
    return 0;
}

#endif
