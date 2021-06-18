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
 *  For trigger NEWHOST it registers a newly discovered lan host
 */
 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "sysevent/sysevent.h"

static int            sysevent_fd = 0;
static char          *sysevent_name = "trigger";
static token_t        sysevent_token;
static unsigned short sysevent_port;
static char           sysevent_ip[19];

#define lan_hosts_dir "/tmp/lanhosts"
#define hosts_filename "lanhosts"

#define TRIGGER_PREFIX "trigger_"

#define TIMER_DIR "/etc/cron/cron.everyminute"

#define MAX_QUERY 256

#define MAX_SYSCFG_ENTRIES 256  // a failsafe to make sure we dont count ridiculously high in the case of data corruption

/*
 *
 */
static void printhelp(char *name) {
   printf ("Usage %s --port sysevent_port --ip sysevent_ip --help  trigger_type  [parameters based on trigger_type]\n", name);
   printf ("             trigger_type is:\n");
   printf ("                NEWHOST:\n");
   printf ("                    mac   : the mac address of the new host\n");
   printf ("                    ip    : the ip address of the new host\n");
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
 ================================================================================
 =                        newhost trigger
 =
 = A new host has been discovered on our lan
 ================================================================================
 */

/*
 * handle_newhost_trigger
 *   Handle the discovery of a new lan host
 *
 * Parameters :
 *    ip     : ip address of the new host
 *    mac    : mac address of the new host
 * Return
 *    0        : ok
 *  -ve        : some error
 */
static int handle_newhost_trigger(char *ip, char *mac)
{
   char buf[1024];
   FILE *kh_fp = fopen(lan_hosts_dir"/"hosts_filename, "r+");
   if (NULL == kh_fp) {
      kh_fp = fopen(lan_hosts_dir"/"hosts_filename, "w+");
   }
   if (NULL != kh_fp) {
      while (NULL != fgets(buf, sizeof(buf), kh_fp)) {
         if (NULL != strstr(buf, ip)) {
            fclose(kh_fp);
            return(0);
         }
      }
      fprintf(kh_fp, "%s   %s\n", ip, mac);
      fclose(kh_fp);
      printf("%s Calling RDKB_FIREWALL_RESTART \n",__FUNCTION__);
      sysevent_set(sysevent_fd, sysevent_token, "firewall-restart", NULL, 0);
   }
   return(0);
}

/*
 ================================================================================
 = 
 ================================================================================
 */


/*
 * Return Values  :
 *    0              : Success
 *   -2              : Problem with sysevent
 *
 *  Parameters   :
 *     type            : The trigger type. This may be:
 *        "NEWHOST" 
 * Other Parameters :
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

   // There are a minimum of 1 arguments (type)
   if (1 > argc - next_arg) {
      printf("Too few arguments\n");
      printhelp(argv[0]);
      exit(0);
   }

   char *type = argv[next_arg];
   char *ip = NULL;
   char *mac = NULL;

   int t_type = (0 == strcmp("NEWHOST", type)) ? 1 : 0;

   if (0 == t_type) {
      printf("Unknown trigger type %s\n", type);
      printhelp(argv[0]);
      exit(0);
   } 

   sysevent_fd =  sysevent_open(sysevent_ip, sysevent_port, SE_VERSION, sysevent_name, &sysevent_token);
   if (0 > sysevent_fd) {
      printf("Unable to register with sysevent daemon at %s %u.\n", sysevent_ip, sysevent_port);
      rc = -2;
      /* CID 72342: Improper use of negative value */
      return(rc);
   }
   switch(t_type) {
      case(1):    // NEWHOST
         if (3 != argc - next_arg) {
            printf("Too few arguments for a NEWHOST trigger\n");
            printhelp(argv[0]);
            exit(0);
         }
         mac = argv[next_arg+1];
         ip = argv[next_arg+2];
         rc = handle_newhost_trigger(ip, mac);
         break;
      default:
         printf("Unknown trigger type %s (%d)\n", type, t_type);
         printhelp(argv[0]);
         exit(0);
         break;
   }
            
   sysevent_close(sysevent_fd, sysevent_token);
   return(rc);
}
