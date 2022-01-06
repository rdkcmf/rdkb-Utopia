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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/ip.h>  /* for iphdr */
#include <linux/netfilter.h>            /* for NF_ACCEPT */
#include <libnetfilter_queue/libnetfilter_queue.h>
#include "ulog.h"
#include "syscfg/syscfg.h"
#include "sysevent/sysevent.h"
#include <telemetry_busmessage_sender.h>

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

#define TRIGGER_QUEUE 22 // the netfilter queue THIS MUST BE UNIQUE IN THE SYSTEM
static struct nfq_handle*   nfq_h;
static struct nfq_q_handle* trigger_q;
static int                  nfqueue_fd = 0;
static int                  restart_firewall = 0;

typedef struct {
   int            active;
   int            protocol; // 1 = tcp, 2 = udp, 3 = both
   unsigned short low_port;
   unsigned short high_port;
   int            lifetime; // number of minutes this trigger lives
   int            quanta; // number of minutes left in this triggers lifetime 
   struct in_addr to_host; 
   char*          tcp_rule;
   char*          udp_rule;
   char*          tcp_forward_rule;
   char*          udp_forward_rule;
} trigger_info_t; 

/*
 * trigger_info is a dynamically sized array where a trigger with trigger id x
 * is at index x-1
 */
static trigger_info_t *trigger_info = NULL;
static int             high_trigger = 0;  // highest trigger in the trigger_info


static int            sysevent_fd = 0;
static char          *sysevent_name = "trigger";
static token_t        sysevent_token;
static unsigned short sysevent_port;
static char           sysevent_ip[19];

#define MAX_QUERY 256
#define MAX_SYSCFG_ENTRIES 256  // a failsafe to make sure we dont count ridiculously high in the case of data corruption
#define DEFAULT_MINS 3

/*
 ***********************************************************************************
 * Procedure : printhelp
 ***********************************************************************************
 */
static void printhelp(char *name) {
   printf ("Usage %s --port sysevent_port --ip sysevent_ip --help\n", name);
}

/*
 * Procedure     : get_options
 * Purpose       : read commandline parameters and set configurable
 *                 parameters
 * Parameters    :
 *    argc       : The number of input parameters
 *    argv       : The input parameter strings
 * Return Value  :
 *   the index of the first not optional argument
 */
static int get_options(int argc, char **argv)
{
   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"port", 1, 0, 'p'},
         {"ip", 1, 0, 'i'},
         {"help", 0, 0, 'h'},
         {0, 0, 0, 0}
      };

      // optstring has a leading : to stop debug output
      // p takes an argument
      // i takes an argument
      // h takes no argument
      c = getopt_long (argc, argv, ":p:i:h", long_options, &option_index);
      if (c == -1) {
         break;
      }

      switch (c) {
         case 'p':
            sysevent_port = (0x0000FFFF & (unsigned short) atoi(optarg));
            break;

         case 'i':
            snprintf(sysevent_ip, sizeof(sysevent_ip), "%s", optarg);
            break;

         case 'h':
         case '?':
            printhelp(argv[0]);
            exit(0);

         default:
            printhelp(argv[0]);
            break;
      }
   }
   return(optind);
}

/*
 ===============================================================================
                            trigger helpers
 ===============================================================================
 */

/*
 ********************************************************************************
 *  Procedure    : stop_forwarding
 *  Purpose      : remove an active trigger forwarding
 *  Parameters   :
 *      id  : the id of the trigger to remove
 *  Return Value :
 *      0   : No firewall restart is required
 *      1   : Firewall restart is required
 ********************************************************************************
 */
static int stop_forwarding(const int id)
{
   int idx = id-1;

   if (0 > idx || 1 != (trigger_info[idx]).active) {
      return(0);
   }

   int   restart_firewall = 0;
   char *rule;
   if (NULL != (rule = (trigger_info[idx]).tcp_rule)) {
      sysevent_set(sysevent_fd, sysevent_token, rule, NULL, 0);
      free((trigger_info[idx]).tcp_rule);
      restart_firewall = 1;
   }
   if (NULL != (rule = (trigger_info[idx]).tcp_forward_rule)) {
      sysevent_set(sysevent_fd, sysevent_token, rule, NULL, 0);
      free((trigger_info[idx]).tcp_forward_rule);
      restart_firewall = 1;
   }
   if (NULL != (rule = (trigger_info[idx]).udp_rule)) {
      sysevent_set(sysevent_fd, sysevent_token, rule, NULL, 0);
      free((trigger_info[idx]).udp_rule);
      restart_firewall = 1;
   }
   if (NULL != (rule = (trigger_info[idx]).udp_forward_rule)) {
      sysevent_set(sysevent_fd, sysevent_token, rule, NULL, 0);
      free((trigger_info[idx]).udp_forward_rule);
      restart_firewall = 1;
   }

   memset (&(trigger_info[idx]), 0, sizeof(trigger_info_t));
   return(restart_firewall);
}

/*
 ********************************************************************************
 *  Procedure    : start_forwarding
 *  Purpose      : start an active trigger forwarding
 *  Parameters   :
 *      id  : the id of the trigger to add
 *  Return Value :
 *      0   : No firewall restart is required
 *      1   : Firewall restart is required
 ********************************************************************************
 */
static int start_forwarding(const int id)
{
   int idx = id-1;
   char rule[MAX_QUERY+150];
   char ref_tuple[MAX_QUERY];
   char dnat[256];
   int  rc;

   if ( (0 > idx)                              || 
        (0 == (trigger_info[idx]).to_host.s_addr) ) {
      return(0);
   } else {
      snprintf(dnat, sizeof(dnat), "%s", inet_ntoa((trigger_info[idx]).to_host));
   }

   /*
    * TCP 
    */
   if (0x00000001 & (trigger_info[idx]).protocol) {
      /*
       * dnat rule
       */
      snprintf(rule, sizeof(rule),
               " -A PREROUTING -p tcp -m tcp -d $WAN_IPADDR --dport %d:%d -j DNAT --to-destination %s\n",
               (trigger_info[idx]).low_port, (trigger_info[idx]).high_port, dnat);

      if (NULL == (trigger_info[idx]).tcp_rule) {
         /*
          * the pool rule does not exist.
          * Create the rule in the sysevent pool used by the firewall (<NatFirewallRule, >),
          * and store the reference to the rule in tcp_rule
          */
         ref_tuple[0] = '\0';
         rc = sysevent_set_unique(sysevent_fd, sysevent_token, "NatFirewallRule", rule, ref_tuple, sizeof(ref_tuple));
         if (0 == rc && '\0' != ref_tuple[0]) {
            (trigger_info[idx]).tcp_rule = strdup(ref_tuple);
         }
      } else {
         /*
          * there is already a pool rule, so just overwrite it
          */
         sysevent_set(sysevent_fd, sysevent_token, (trigger_info[idx]).tcp_rule, rule, 0);
      }

      /*
       * forward rule
       */
      snprintf(rule, sizeof(rule), 
               "-A FORWARD -p tcp -m tcp -d %s --dport %d:%d -j ACCEPT\n", 
               dnat, (trigger_info[idx]).low_port, (trigger_info[idx]).high_port);
      if (NULL == (trigger_info[idx]).tcp_forward_rule) {
         /*
          * the pool rule does not exist.
          * Create the rule in the sysevent pool used by the firewall (<GeneralPurposeFirewallRule, >),
          * and store the reference to the rule in tcp_forward_rule
          */
         ref_tuple[0] = '\0';
         rc = sysevent_set_unique(sysevent_fd, sysevent_token, "GeneralPurposeFirewallRule", rule, ref_tuple, sizeof(ref_tuple));
         if (0 == rc && '\0' != ref_tuple[0]) {
            (trigger_info[idx]).tcp_forward_rule = strdup(ref_tuple);
         }
      } else {
         /*
          * there is already a pool rule, so just overwrite it
          */
         sysevent_set(sysevent_fd, sysevent_token, (trigger_info[idx]).tcp_forward_rule, rule, 0);
      }
   } else {
      char *lrule;
      /* 
       * This trigger does not use tcp, so make sure that no tcp forward rules are set
       */
      if (NULL != (lrule = (trigger_info[idx]).tcp_rule)) {
         sysevent_set(sysevent_fd, sysevent_token, lrule, NULL, 0);
         free((trigger_info[idx]).tcp_rule);
         (trigger_info[idx]).tcp_rule = NULL;
      }
      if (NULL != (lrule = (trigger_info[idx]).tcp_forward_rule)) {
         sysevent_set(sysevent_fd, sysevent_token, lrule, NULL, 0);
         free((trigger_info[idx]).tcp_forward_rule);
         (trigger_info[idx]).tcp_forward_rule = NULL;
      }
   }

   /*
    * UDP 
    */
   if (0x00000002 & (trigger_info[idx]).protocol) {
      /*
       * dnat rule
       */
      snprintf(rule, sizeof(rule),
               " -A PREROUTING -p udp -m udp -d $WAN_IPADDR --dport %d:%d -j DNAT --to-destination %s\n",
               (trigger_info[idx]).low_port, (trigger_info[idx]).high_port, dnat);

      if (NULL == (trigger_info[idx]).udp_rule) {
         /*
          * the pool rule does not exist.
          * Create the rule in the sysevent pool used by the firewall (<NatFirewallRule, >),
          * and store the reference to the rule in udp_rule
          */
         ref_tuple[0] = '\0';
         rc = sysevent_set_unique(sysevent_fd, sysevent_token, "NatFirewallRule", rule, ref_tuple, sizeof(ref_tuple));
         if (0 == rc && '\0' != ref_tuple[0]) {
            (trigger_info[idx]).udp_rule = strdup(ref_tuple);
         }
      } else {
         /*
          * there is already a pool rule, so just overwrite it
          */
         sysevent_set(sysevent_fd, sysevent_token, (trigger_info[idx]).udp_rule, rule, 0);
      }

      /*
       * forward rule
       */
      snprintf(rule, sizeof(rule), 
               "-A FORWARD -p udp -m udp -d %s --dport %d:%d -j ACCEPT\n", 
               dnat, (trigger_info[idx]).low_port, (trigger_info[idx]).high_port);
      if (NULL == (trigger_info[idx]).udp_forward_rule) {
         /*
          * the pool rule does not exist.
          * Create the rule in the sysevent pool used by the firewall (<GeneralPurposeFirewallRule, >),
          * and store the reference to the rule in udp_forward_rule
          */
         ref_tuple[0] = '\0';
         rc = sysevent_set_unique(sysevent_fd, sysevent_token, "GeneralPurposeFirewallRule", rule, ref_tuple, sizeof(ref_tuple));
         if (0 == rc && '\0' != ref_tuple[0]) {
            (trigger_info[idx]).udp_forward_rule = strdup(ref_tuple);
         }
      } else {
         /*
          * there is already a pool rule, so just overwrite it
          */
         sysevent_set(sysevent_fd, sysevent_token, (trigger_info[idx]).udp_forward_rule, rule, 0);
      }
   } else {
      char *lrule;
      /* 
       * This trigger does not use udp, so make sure that no udp forward rules are set
       */
      if (NULL != (lrule = (trigger_info[idx]).udp_rule)) {
         sysevent_set(sysevent_fd, sysevent_token, lrule, NULL, 0);
         free((trigger_info[idx]).udp_rule);
         (trigger_info[idx]).udp_rule = NULL;
      }
      if (NULL != (lrule = (trigger_info[idx]).udp_forward_rule)) {
         sysevent_set(sysevent_fd, sysevent_token, lrule, NULL, 0);
         free((trigger_info[idx]).udp_forward_rule);
         (trigger_info[idx]).udp_forward_rule = NULL;
      }
   }

   (trigger_info[idx]).active = 1;
   return(1);
}

/*
 ================================================================================
 =  Procedure    : update_quanta
 =  Purpose      : decrement all quanta by a given amount and if necessary remove triggers
 =  Parameters   :
 =      secs  : the number of since the last update
 =  Return Value :
 =      0           : firewall doesnt need to be restarted
 =      1           : firewall needs to be restarted
 ================================================================================
 */
static int update_quanta(const long secs)
{
   int  i;
   int  restart_firewall = 0;
   long left;

   for (i=0 ; i<high_trigger ; i++) {
      if (0 < (left = (trigger_info[i]).quanta)) {
         if (0 >= (left-secs)) {
            (trigger_info[i]).quanta = 0;
            stop_forwarding(i+1); // trigger id is idx+1
            restart_firewall = 1;
         } else {
            (trigger_info[i]).quanta -= secs;
         } 
      }
   }
   return(restart_firewall);
}

/*
 ================================================================================
 =  Procedure   : update_trigger_entry
 =  Purpose     : If the trigger entry doesnt exist then add it
 =                If the trigger entry exists with a different dnat then change it
 =                Otherwise update the quanta
 =  Parameters  :
 =     id          : The id of the trigger to prepare info
 =     host        : The host to dnat to
 ================================================================================
 */
static int update_trigger_entry(int id, struct in_addr host)
{
   int  rc;
   int  idx = id-1;  // we know the element postion in the array

   /*
    * If the array of triggers is not large enough for the new trigger, then reallocate the array
    * But do some sanity checking (dont let triggers grow beyond MAX_SYSCFG_ENTRIES)
    */
   if (MAX_SYSCFG_ENTRIES < id) {
      return(-1);
   }

   if (0 >= id) {
      return(-1);
   }
   
   if (high_trigger < id) {
      trigger_info = (trigger_info_t *) realloc (trigger_info, (id * sizeof (trigger_info_t)));
      if (NULL == trigger_info) {
         high_trigger = 0;
         return(-3);
      } else {
         int i;
         for (i=high_trigger ; i< id ; i++) {
            memset(&(trigger_info[i]), 0, sizeof(trigger_info_t));
         }
         high_trigger = id;
      }
   }

   if (1 == (trigger_info[idx]).active) {
      if (host.s_addr == (trigger_info[idx]).to_host.s_addr) {
         trigger_info[idx].quanta = trigger_info[idx].lifetime;
         return(0);
      } else {
         (trigger_info[idx]).to_host = host;
         (trigger_info[idx]).quanta = (trigger_info[idx]).lifetime;
         start_forwarding(idx+1); // trigger id is idx+1
         printf("%s Triggering RDKB_FIREWALL_RESTART active\n",__FUNCTION__);
         t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
         sysevent_set(sysevent_fd, sysevent_token, "firewall-restart", NULL, 0);
         restart_firewall = 0;
         return(0);
      }
   }

   char query[256];
   char namespace[256];
   query[0] = '\0';
   rc = syscfg_get(NULL, "PortRangeTriggerCount", query, sizeof(query)); 
   if (0 != rc || 0 == strlen(query)) {
      return(-4);
   }

   int count = atoi(query);
   if (0 == count) {
      return(-5);
   }

   int y;
   for (y=1 ; y<=count ; y++) {
      namespace[0] = '\0';
      snprintf(query, sizeof(query), "PortRangeTrigger_%d", y);
      rc = syscfg_get(NULL, query, namespace, sizeof(namespace));
      if (0 != rc || '\0' == namespace[0]) {
         continue;
      }

      // what is the trigger id
      char cid[10];
      cid[0] = '\0';
      rc = syscfg_get(namespace, "trigger_id", cid, sizeof(cid));
      if (0 != rc || '\0' == cid[0]) {
         continue;
      } else {
         if (id != atoi(cid)) {
            continue;
         }
      }
 
      // we found the trigger

      // is the rule enabled
      query[0] = '\0';
      rc = syscfg_get(namespace, "enabled", query, sizeof(query));
      if (0 != rc || '\0' == query[0] || 0 == strcmp("0", query)) {
        return(-6);
      } 

      // what is the forward protocol
      char prot[10];
      prot[0] = '\0';
      rc = syscfg_get(namespace, "forward_protocol", prot, sizeof(prot));
      if (0 != rc || '\0' == prot[0]) {
         (trigger_info[idx]).protocol = 3;
      } else {
         if (0 == strcmp("udp", prot)) {
            (trigger_info[idx]).protocol = 2;
         } else if (0 == strcmp("tcp", prot)) {
            (trigger_info[idx]).protocol = 1;
         } else {
            (trigger_info[idx]).protocol = 3;
         }
      }
      // what is the forwarding port range
      char portrange[30];
      char sdport[10];
      char edport[10];
      portrange[0]= '\0';
      sdport[0]   = '\0';
      edport[0]   = '\0';
      rc = syscfg_get(namespace, "forward_range", portrange, sizeof(portrange));
      if (0 != rc || '\0' == portrange[0]) {
         return(-7);
      } else {
         int r = 0;
         if (2 != (r = sscanf(portrange, "%10s %10s", sdport, edport))) {
            if (1 == r) {
               snprintf(edport, sizeof(edport), "%s", sdport);
            } else {
               return(-8);
            }
         }
         (trigger_info[idx]).low_port = (unsigned short) atoi(sdport);
         (trigger_info[idx]).high_port = (unsigned short) atoi(edport);
      }

      // how many minutes does a trigger last without seeing an interleaving trigger packet
      char lifetime[10];
      lifetime[0] = '\0';
      rc = syscfg_get(namespace, "lifetime", lifetime, sizeof(lifetime));
      if (0 != rc || '\0' == lifetime[0]) {
         (trigger_info[idx]).lifetime = DEFAULT_MINS * 60;
      } else {
         (trigger_info[idx]).lifetime = atoi(lifetime) * 60;
      }
      (trigger_info[idx]).quanta = trigger_info[idx].lifetime;

      (trigger_info[idx]).to_host    = host;
      (trigger_info[idx]).active     = 1;
      start_forwarding(idx+1);  // trigger id is idx+1
      printf("%s Triggering RDKB_FIREWALL_RESTART end\n",__FUNCTION__);
      t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
      sysevent_set(sysevent_fd, sysevent_token, "firewall-restart", NULL, 0);
      restart_firewall = 0;
      return(0);
   }
   return(-10);
}

/*
 ********************************************************************************
 *  Procedure : trigger_callback
 ********************************************************************************
 */
static int trigger_callback(struct nfq_q_handle* qh, 
                            struct nfgenmsg*     nfmsg,
                            struct nfq_data*     nfa, 
                            void*                data)
{
   (void)                       nfmsg;
   (void)                        data;
   int                         data_len;
   unsigned char               *payload;
   int                         id = 0;
   struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);

   if (ph) {
      id = ntohl(ph->packet_id);
   }
 
   // get the trigger id == the mark
   u_int32_t mark = nfq_get_nfmark(nfa);

   data_len = nfq_get_payload(nfa, &payload);
   if (0 > data_len) {
      fprintf(stderr, "Error getting trigger payload\n");
      return(-1);
   }

   struct iphdr *iph = ((struct iphdr *)payload);
   struct in_addr saddr = {iph->saddr};

   if (0 != update_trigger_entry(mark, saddr)) {
      ulogf(ULOG_FIREWALL, UL_TRIGGER, "Unable to update trigger %d to host %s", mark, inet_ntoa(saddr));
   }
   return nfq_set_verdict_mark(qh, id, NF_ACCEPT, htonl(0), 0, NULL);
}

/*
 ********************************************************************************
 *  Procedure  : handle_nfqueue_packet
 *  Purpose    : logic for handling a packet from the netfilter queue
 *  Parameters :
 *      fd  : the netfilter queue file descriptor
 ********************************************************************************
 */
static int handle_nfqueue_packet(int fd)
{
   char buf[4096] __attribute__ ((aligned));
   int  num_bytes;

   num_bytes = recv(fd, buf, sizeof(buf), 0);
   if (0 < num_bytes) {
      nfq_handle_packet(nfq_h, buf, num_bytes);  // this will call the callback
   }
   return(0);
}

/*
 ********************************************************************************
 *  Procedure    : get_next_timeout
 *  Purpose      : find the number of seconds until the next timeout
 *  Return Value :
 *    -1            : There is no need for a timout
 *     else         : number of secs (including 0) till at least one trigger is expiring
 ********************************************************************************
 */
long get_next_timeout()
{
   int i;
   int next = -1;
   int quanta;

   for (i=0 ; i< high_trigger; i++) {
      if (1 == (trigger_info[i]).active) {
         if ( (0 <= (quanta = (trigger_info[i]).quanta)) && (-1 == next || quanta < next) ) {
            next = quanta;
         }
      }
   }
   return(next);
}

/*
 ********************************************************************************
 *  Procedure  : main_loop
 *  Purpose    : control main logic for triggers 
 *  Parameters :
 *      queue_fd  : the netfilter queue file descriptor
 ********************************************************************************
 */
static int main_loop(int queue_fd)
{
   int            rc;
   fd_set         rd_set;
   int            max_fd;
   struct timeval timeout  = {0, 0};
   long           waitsecs;

   for ( ; ; ) {
      max_fd = 1;
      FD_ZERO(&rd_set);
      FD_SET(queue_fd, &rd_set);
      if (queue_fd >= max_fd) {
         max_fd = queue_fd+1;
      }

      long secs = get_next_timeout();
      if (-1 == secs) {
	//zqiu: we still need to yeild to other process to avoid cpu occupation.
	sleep(1);
         waitsecs = timeout.tv_sec = timeout.tv_usec = 0;
         rc = select(max_fd, &rd_set, NULL, NULL, NULL);
      } else {
         waitsecs = timeout.tv_sec = secs;
         timeout.tv_usec = 0;
         rc = select(max_fd, &rd_set, NULL, NULL, &timeout);
      }

      restart_firewall = 0;

      /*
       * This is not portable to all OSes, but linux stores the unused portion of
       * the timeout in timeout.
       */
      if (0 != waitsecs) {
         restart_firewall = update_quanta(waitsecs - timeout.tv_sec);
      }

      if (-1 >= rc) {
         if (restart_firewall) {
            printf("%s Triggering RDKB_FIREWALL_RESTART intial\n",__FUNCTION__);
            t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
            sysevent_set(sysevent_fd, sysevent_token, "firewall-restart", NULL, 0);
            restart_firewall = 0;
         }
         continue;
      }

      if (0 < rc) {
         if (FD_ISSET(queue_fd, &rd_set)) {
            handle_nfqueue_packet(queue_fd);
         }
      }

      if (restart_firewall) { 
         printf("%s Triggering RDKB_FIREWALL_RESTART end\n",__FUNCTION__);
         t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
         sysevent_set(sysevent_fd, sysevent_token, "firewall-restart", NULL, 0);
         restart_firewall = 0;
      }
   } 
   return(0);
}

/*
 ********************************************************************************
 *  Procedure : prepare nf_queue
 *  Purpose   : Prepare the netfilter queue used to receive packets from netfilters
 ********************************************************************************
 */
static int prepare_nf_queue()
{
   trigger_info = NULL;
   high_trigger = 0;


   if (!(nfq_h = nfq_open())) {
      ulog(ULOG_FIREWALL, UL_TRIGGER, "Unable to open the netfilter queue");
//      return(-1);
   }

   if (0 > nfq_unbind_pf(nfq_h, AF_INET)) {
      ulog(ULOG_FIREWALL, UL_TRIGGER, "Unable to unbind the netfilter queue");
      return(1);
   }

   if (0 > nfq_bind_pf(nfq_h, AF_INET)) {
      ulog(ULOG_FIREWALL, UL_TRIGGER, "Unable to bind the netfilter queue");
      return(1);
   }

    if (!(trigger_q = nfq_create_queue(nfq_h,  TRIGGER_QUEUE, &trigger_callback, NULL))) {
      ulogf(ULOG_FIREWALL, UL_TRIGGER, "Unable to create the netfilter queue (%d)", TRIGGER_QUEUE);
       return(1);
   }

//   if (0 > nfq_set_mode(trigger_q, NFQNL_COPY_META, 0xffff)) {
   if (0 > nfq_set_mode(trigger_q, NFQNL_COPY_PACKET, 0xffff)) {
      ulogf(ULOG_FIREWALL, UL_TRIGGER, "Unable to set mode on queue (%d)", TRIGGER_QUEUE);
      return(1);
   }

   if (-1 == (nfqueue_fd = nfq_fd(nfq_h)) ) {
      ulogf(ULOG_FIREWALL, UL_TRIGGER, "Unable to get file descriptor for queue (%d)", TRIGGER_QUEUE);
      return(1);
   }

   return(0);
}

/*
 * Procedure     : initialize_system
 * Purpose       : Initialize the system
 * Parameters    : None
 * Return Code   :
 *    0             : All is initialized
 */
static int initialize_system(void)
{
   ulog_init();
   ulog(ULOG_FIREWALL, UL_TRIGGER, "Firewall Trigger process started");
   sysevent_fd =  sysevent_open(sysevent_ip, sysevent_port, SE_VERSION, sysevent_name, &sysevent_token);
   if (0 > sysevent_fd) {
      ulog(ULOG_FIREWALL, UL_TRIGGER, "Error sysevent not inited");
      return(-2);
   }

   prepare_nf_queue();
   return(0);
}
/*
 * Procedure     : deinitialize_system
 * Purpose       : UniInitialize the system
 * Parameters    : None
 * Return Code   :
 *    0             : All is uninitialized
 */
static int deinitialize_system(void)
{
   nfq_destroy_queue(trigger_q);
   nfq_close(nfq_h);
   sysevent_close(sysevent_fd, sysevent_token);
   ulog(ULOG_FIREWALL, UL_TRIGGER, "Firewall Trigger process stopped");
   return(0);
}

/*
 * Procedure     : terminate_signal_handler
 * Purpose       : Handle a signal
 * Parameters    :
 *   signum         : The signal received
 * Return Code   : None
 */
static void terminate_signal_handler (int signum)
{
   int i;
   for (i=0; i<high_trigger; i++) {
      if (1 == trigger_info[i].active) {
        stop_forwarding(i+1);  // trigger id is idx+1 
      }
   }
   printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
   t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
   sysevent_set(sysevent_fd, sysevent_token, "firewall-restart", NULL, 0);

   ulogf(ULOG_FIREWALL, UL_TRIGGER, "Received signal %d. Terminating", signum);
   deinitialize_system();
   exit(0);
}

/*
 * daemon initialization code
 */
static int daemon_init(void)
{
   pid_t pid;

   // are we already a daemon
   if (1 == getppid()) {
      return(0);
   }

   if ( 0 > (pid = fork()) ) {
      return(-1);
   } else if (0 != pid) {
      exit(0);  // parent exits
   }

   // child
   setsid();   // become session leader
   chdir("/"); // change working directory


   umask(0);   // clear our file mode creation mask

   return(0);
}

/*
 * Return Values  :
 *    0              : Success
 *   -1              : Problem with syscfg
 *   -2              : Problem with sysevent
 *
 *  Parameters   :
 *     type            : The trigger type. This may be:
 *        "DNAT", "DNAT_EXPIRED", "NEWHOST" 
 * Other Parameters :
 *     type = "DNAT"
 *        id              : The unique id of the trigger
 *        ip           : The IP address of the host to DNAT to
 *     type = "DNAT_EXPIRED"
 *        id              : The unique id of the trigger
 *     type = "NEWHOST"
 *        mac          : The mac of the newly discovered las host
 *        ip           : The ip address of the newly discovered las host
 * Return Values   :
 * 
 */
int main(int argc, char **argv)
{
   int   rc = 0;

   // set defaults
   snprintf(sysevent_ip, sizeof(sysevent_ip), "127.0.0.1");
   sysevent_port = SE_SERVER_WELL_KNOWN_PORT;

   // parse commandline for options and readjust defaults if requested
   int next_arg = get_options(argc, argv);

   if (0 != argc - next_arg) {
      printhelp(argv[0]);
      exit(0);
   }
#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#endif
   daemon_init();

   /* set up signal handling
    * we want:
    *
    * SIGINT     Interrupt from keyboard - Terminate
    * SIGQUIT    Quit from keyboard - Terminate
    * SIGTERM   Termination signal  - Terminate
    * SIGKILL    Kill signal Cant be blocked or Caught
    * All others are default
    */
   struct sigaction new_action, old_action;

   new_action.sa_handler = terminate_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);

   if (-1 == sigaction (SIGINT, NULL, &old_action)) {
      fprintf(stderr, "Cannot get signal handler for SIGINT\n");
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGINT, &new_action, NULL)) {
            fprintf(stderr, "Cannot set signal handler for SIGINT\n");
            return(-1);
        }
      }
   }

   if (-1 == sigaction (SIGQUIT, NULL, &old_action)) {
      fprintf(stderr, "Cannot get signal handler for SIGQUIT\n");
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGQUIT, &new_action, NULL)) {
            fprintf(stderr, "Cannot set signal handler for SIGQUIT\n");
            return(-1);
        }
      }
   }

   if (-1 == sigaction (SIGTERM, NULL, &old_action)) {
      fprintf(stderr, "Cannot get signal handler for SIGTERM\n");
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
        if (-1 == sigaction (SIGTERM, &new_action, NULL)) {
            fprintf(stderr, "Cannot set signal handler for SIGTERM\n");
            return(-1);
        }
      }
   }

   if (0 != initialize_system()) {
      exit(0);
   }

   main_loop(nfqueue_fd);

   deinitialize_system();
            
   return(rc);
}
