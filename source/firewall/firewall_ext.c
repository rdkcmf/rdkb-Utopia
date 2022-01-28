#ifdef RDKB_EXTENDER_ENABLED

#include "firewall.h"
#include "firewall_custom.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include<errno.h> 

extern int  sysevent_fd ;
extern token_t        sysevent_token;

static char wan_ifname[32] ;


int create_socket() 
{
   int sockfd = 0;
         sockfd = socket(AF_INET, SOCK_STREAM, 0);
         if(sockfd == -1){
         fprintf(stderr, "Could not get socket.\n");
         return -1;
         }
         return sockfd;
}

char* get_iface_ipaddr(const char* iface_name)
{
   if(!iface_name )
         return NULL;
      struct ifreq ifr;
      memset(&ifr,0,sizeof(struct ifreq));
      int skfd = 0;
      if ((skfd = create_socket() ) < 0) {
         printf("socket error %s\n", strerror(errno));
         return NULL;
      }
         
      ifr.ifr_addr.sa_family = AF_INET;
      strncpy(ifr.ifr_name, iface_name, IFNAMSIZ-1);
      if ( ioctl(skfd, SIOCGIFADDR, &ifr)  < 0 )
      {
         printf("Failed to get %s IP Address\n",iface_name);
         close(skfd);
         return NULL;   
      }
      close(skfd);
      return (inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}

int isExtProfile()
{
      if ( ( EXTENDER_MODE == Get_Device_Mode() ) )
      {
         return 0;
      }
      return -1;
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


   fprintf(filter_fp, "-A INPUT -i %s -j wanattack\n", wan_ifname);

   do_wan2self_attack(filter_fp,get_iface_ipaddr(wan_ifname));

   fprintf(filter_fp, "-A INPUT -i %s -p tcp -m tcp --dport 22 -j SSH_FILTER\n",wan_ifname);

   do_ssh_IpAccessTable(filter_fp, "22", AF_INET, wan_ifname);

   fprintf(filter_fp, "-A xlog_accept_wan2lan -j ACCEPT\n");

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

      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 128 -j PING_FLOOD\n", wan_ifname); // Echo request
      fprintf(fp, "-A INPUT -i %s -p icmpv6 -m icmp6 --icmpv6-type 129 -m limit --limit 10/sec -j ACCEPT\n", wan_ifname); // Echo reply

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
   fprintf(filter_fp, "%s\n", "*filter");
   fprintf(filter_fp, "%s\n", ":LOG_SSH_DROP - [0:0]");
   fprintf(filter_fp, "%s\n", ":SSH_FILTER - [0:0]");
   fprintf(filter_fp, "%s\n", ":PING_FLOOD - [0:0]");

   fprintf(filter_fp, "-A INPUT -i %s -p tcp -m tcp --dport 22 -j SSH_FILTER\n",wan_ifname);

   filter_ipv6_icmp_limit_rules(filter_fp);
   do_ssh_IpAccessTable(filter_fp, "22", AF_INET6, wan_ifname);

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

   memset(wan_ifname,0,sizeof(wan_ifname));
   sysevent_get(sysevent_fd, sysevent_token, "cellular_ifname", wan_ifname, sizeof(wan_ifname));


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
