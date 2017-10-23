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
 * Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
 *
 * This work is subject to U.S. and international copyright laws and
 * treaties. No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged, condensed,
 * expanded, collected, compiled, linked, recast, transformed or adapted
 * without the prior written consent of Cisco Systems, Inc. Any use or
 * exploitation of this work without authorization could subject the
 * perpetrator to criminal and civil liability.
 */

/*
===================================================================
    This programs will compare syscfg database and sysevent database
    against a default database. If any tuple in syscfg or sysevent is
    not already set, then this program will set it according to the
    default value
===================================================================
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <syscfg/syscfg.h>
#include "sysevent/sysevent.h"

static char default_file[1024];

// The sysevent server is local 
#define SE_WELL_KNOWN_IP    "127.0.0.1"
static short server_port;
static char  server_ip[19];
static int   syscfg_dirty;
#define DEFAULT_FILE "/etc/utopia/system_defaults"
#define SE_NAME "system_default_set"

static int global_fd = 0;

//Flag to indicate a db conversion is necessary
static int convert = 0;

// we can use one global id for sysevent because we are single threaded
token_t global_id;

#if defined (_CBR_PRODUCT_REQ_) || defined (_XB6_PRODUCT_REQ_)
        #define LOG_FILE "/rdklogs/logs/Consolelog.txt.0"
#else
	#define LOG_FILE "/rdklogs/logs/ArmConsolelog.txt.0"
#endif

#define APPLY_PRINT(fmt ...)   {\
   FILE *logfp = fopen ( LOG_FILE , "a+");\
   if (logfp)\
   {\
        fprintf(logfp,fmt);\
        fclose(logfp);\
   }\
}\
/*
 * Procedure     : trim
 * Purpose       : trims a string
 * Parameters    :
 *    in         : A string to trim
 * Return Value  : The trimmed string
 * Note          : This procedure will change the input sting in situ
 */
static char *trim(char *in)
{
   // trim the front of the string
   if (NULL == in) {
      return(NULL);
   }
   char *start = in;
   while(isspace(*start)) {
      start++;
   }
   // trim the end of the string

   char *end = start+strlen(start);
   end--;
   while(isspace(*end)) {
      *end = '\0';
      end--;
   }
   return(start);
}

/*
 * Procedure     : parse_line
 * Purpose       : parses a line into a name and a value
 * Parameters    :
 *    in         : the line to parse
 *    name       : on return the name
 *    value      : on return the value
 * Return Value  : 0 if ok -1 if not
 * Note          : This function will change the contents of in
 */
static int parse_line(char *in, char **name, char **value)
{
   // look for the separator token
   if (NULL == in) {
      return(-1);
   } else {
      *name = *value = NULL;
   }

   char *tok = strchr(in, '=');
   if (NULL == tok) {
      return(-1);
   } else {
      *name=(in + 1);
      *value=(tok + 1); 
      *tok='\0';
   } 
   return(0);
}

/*
 * Procedure     : set_sysevent
 * Purpose       : sets a sysevent tuple if it is not already set
 * Parameters    :
 *    name       : the name of the tuple
 *    value      : the value to set the tuple to
 * Return Value  : 0 if ok, -1 if not
 */
static int set_sysevent(char *name, char *value, int flags) 
{
   int   rc = 0;
   char get_val[512];
   rc = sysevent_get(global_fd, global_id, name, get_val, sizeof(get_val));
   if ('\0' == get_val[0]) {
      if (0x00000000 != flags) {
         rc = sysevent_set_options(global_fd, global_id, name, flags);
      }

      // if the value is prefaced by '$' then we use the
      // current value of syscfg
      char *trimmed_val = trim(value);
      if ('$' == trimmed_val[0]) {
         get_val[0] = '\0';
         rc = syscfg_get(NULL, trimmed_val+1, get_val, sizeof(get_val));
         rc = sysevent_set(global_fd, global_id, name, get_val, 0);
//         printf("[utopia] [init] apply_system_defaults set <@%s, %s, 0x%x>\n", name, get_val, flags);
      } else {
         rc = sysevent_set(global_fd, global_id, name, value, 0);
         APPLY_PRINT("[utopia] [init] apply_system_defaults set <@%s, %s, 0x%x>\n", name, value, flags);
         printf("[utopia] [init] apply_system_defaults set <@%s, %s, 0x%x>\n", name, value, flags);
      }
   } else {
      rc = 0;
   }
   return(rc);
}

/*
 * Procedure     : set_syscfg
 * Purpose       : sets a syscfg tuple if it is not already set
 * Parameters    :
 *    name       : the name of the tuple
 *    value      : the value to set the tuple to
 * Return Value  : 0 if ok, -1 if not
 */
static int set_syscfg(char *name, char *value) 
{
   int rc = 0, force = 0;
   char get_val[512];
   char *ns = NULL;
   char *ns_delimiter;
   

   if (NULL == value || 0 == strlen(value)) {
      return(0);
   }
   
   //Note to force write if the value does not match and is marked, and increment past the mark.
   if (name[0] == '$') {
       if (convert)
           force = 1;
       name ++;
   }

   ns_delimiter = strstr(name, "::");
   if (ns_delimiter)
   {
      *ns_delimiter = '\0';
      ns = name;
      name = ns_delimiter+2;
   }
   else
   {
      ns = NULL;
   }

   get_val[0] = '\0';
   rc = syscfg_get(ns, name, get_val, sizeof(get_val));
   if (0 != rc || 0 == strlen(get_val) || (force && strcmp(get_val, value)) ) {
      rc = syscfg_set(ns, name, value);
      printf("[utopia] [init] apply_system_defaults set <$%s::%s, %s> set(rc=%d) get_val %s force %d  \n", ns, name, value, rc,get_val,force);
      syscfg_dirty++;
   } else {
      rc = 0;
      printf("[utopia] [init] syscfg_get <$%s::%s> fail\n", ns, name);
   }
   return(rc);
}

static int handle_version(char* name, char* value) {
    int ret = 0;
    int rc;
    char get_val[512];
    
    if (!strcmp(name, "$Version")) {
        ret = 1;
        name++;
        rc = syscfg_get(NULL, name, get_val, sizeof(get_val));
        if (rc != 0 || 0 == strlen(get_val) || strcmp(value, get_val))
            convert = 1;
    }

    return ret;
}

static int check_version(void) {
   int handled = 0;
   FILE *fp = fopen(default_file, "r");
   if (NULL == fp) {
      printf("[utopia] no system default file (%s) found\n", default_file);
      return(-1);
   }

   /*
    * The default file must contain one default per line in the format
    * name=value (whitespace is allowed)
    * If the default is for a syscfg tuple, then name must be preceeded with a $
    * If the default is for a sysevent tuple, then name must be preceeded with a @
    * If the first character in the line is # then the line will be ignored
    */
   
   char buf[1024];
   char *line; 
   char *name;
   char *value;
   while (NULL != fgets(buf, sizeof(buf)-1, fp)) {
      line = trim(buf);
      if ('#' == line[0]) {
         // this is a comment
      } else if (0x00 == line[0]) {
         // this is an empty line
      } else if ('$' == line[0]) {
         if (0 !=  parse_line(line, &name, &value)) {
            printf("[utopia] [error] set_defaults failed to set syscfg (%s)\n", line);
         } else { 
            if ( handle_version(trim(name), trim(value)) ) {
                handled = 1;
                break;
            }
         }
      } else if ('@' == line[0]) {
         // this is a sysevent line
      } else {
         // this is a malformed line
         printf("[utopia] set_defaults found a malformed line (%s)\n", line);
      }
   }

   fclose(fp); 
   return(0);
}

/*
 * Procedure     : set_syscfg_defaults
 * Purpose       : Go through a file, parse it into <name, value> tuples,
 *                 and set syscfg namespace (iff not already set),
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int set_syscfg_defaults (void)
{
   FILE *fp = fopen(default_file, "r");
   if (NULL == fp) {
      printf("[utopia] no system default file (%s) found\n", default_file);
      return(-1);
   }

   /*
    * The default file must contain one default per line in the format
    * name=value (whitespace is allowed)
    * If the default is for a syscfg tuple, then name must be preceeded with a $
    * If the default is for a sysevent tuple, then name must be preceeded with a @
    * If the first character in the line is # then the line will be ignored
    */
   
   char buf[1024];
   char *line; 
   char *name;
   char *value;
   while (NULL != fgets(buf, sizeof(buf)-1, fp)) {
      line = trim(buf);
      if ('#' == line[0]) {
         // this is a comment
      } else if (0x00 == line[0]) {
         // this is an empty line
      } else if ('$' == line[0]) {
         if (0 !=  parse_line(line, &name, &value)) {
            printf("[utopia] [error] set_defaults failed to set syscfg (%s)\n", line);
         } else { 
            set_syscfg(trim(name), trim(value));
         }
      } else if ('@' == line[0]) {
         // this is a sysevent line
      } else {
         // this is a malformed line
         printf("[utopia] set_defaults found a malformed line (%s)\n", line);
      }
   }
   fclose(fp); 
   return(0);
}

/*
 * Procedure     : set_sysevent_defaults
 * Purpose       : Go through a file, parse it into <name, value> tuples,
 *                 and set sysevent namespace
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int set_sysevent_defaults (void)
{
   FILE *fp = fopen(default_file, "r");
   if (NULL == fp) {
      printf("[utopia] no system default file (%s) found\n", default_file);
      return(-1);
   }

   /*
    * The default file must contain one default per line in the format
    * name=value (whitespace is allowed)
    * If the default is for a syscfg tuple, then name must be preceeded with a $
    * If the default is for a sysevent tuple, then name must be preceeded with a @
    * If the first character in the line is # then the line will be ignored
    */
   
   char buf[1024];
   char *line; 
   char *name;
   char *value;
   while (NULL != fgets(buf, sizeof(buf)-1, fp)) {
      line = trim(buf);
      if ('#' == line[0]) {
         // this is a comment
      } else if (0x00 == line[0]) {
         // this is an empty line
      } else if ('$' == line[0]) {
         // this is a syscfg line
      } else if ('@' == line[0]) {
         if (0 !=  parse_line(line, &name, &value)) {
            printf("[utopia] set_defaults failed to set sysevent (%s)\n", line);
         } else { 
            char *val = trim(value);
            char *flagstr;
            int flags = 0x00000000;

            int i;
            int len = strlen(val);
            for (i=0; i<len; i++) {
               if (isspace(val[i])) {
                  flagstr = (&(val[i])+1);
                  val[i] = '\0';
                  flags = strtol(flagstr, NULL, 16);
                  break;
               }
            } 
            set_sysevent(trim(name), val, flags);
         }
      } else {
         // this is a malformed line
         printf("[utopia] set_defaults found a malformed line (%s)\n", line);
      }
   }
   fclose(fp); 
   return(0);
}

/*
 * Procedure     : set_defaults
 * Purpose       : Go through a file twice, first for syscfg variables 
 *                 (because sysevent might use syscfg values for initialization),
 *                 and then again for sysevent variables
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int set_defaults(void)
{
   check_version();
   set_syscfg_defaults();
   set_sysevent_defaults();
   return(0);
}

static void printhelp(char *name) {
      printf ("Usage %s --file default_file --port syseventd_port --ip syseventd_ip --help command params\n", name);
}

/*
 * Procedure     : parse_command_line
 * Purpose       : if any command line args then apply them
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int parse_command_line(int argc, char **argv)
{
   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"file", 1, 0, 'f'},
         {"port", 1, 0, 'p'},
         {"ip", 1, 0, 'd'},
         {"help", 0, 0, 'h'},
         {0, 0, 0, 0}
      };

      // optstring has a leading : to stop debug output
      // f takes an argument
      // p takes an argument
      // i takes an argument
      // h takes no argument
      c = getopt_long (argc, argv, ":f:p:i:h", long_options, &option_index);
      if (c == -1) {
         break;
      }

      switch (c) {
         case 'f':
            snprintf(default_file, sizeof(default_file), "%s", optarg);
            break;
        
         case 'p':
            server_port = htons(atoi(optarg));
            break;

         case 'i':
            snprintf(server_ip, sizeof(server_ip), "%s", optarg);
            break;

         case 'h':
         case '?':
            printhelp(argv[0]);
            exit(0);
            break;

         default:
            printhelp(argv[0]);
            break;
      }
   }
   return(0);
}

/*
 *
 */
int main(int argc, char **argv)
{
   server_port = SE_SERVER_WELL_KNOWN_PORT;
   snprintf(server_ip, sizeof(server_ip), "%s", SE_WELL_KNOWN_IP);
   snprintf(default_file, sizeof(default_file), "%s", DEFAULT_FILE);
   syscfg_dirty = 0;

   parse_command_line(argc, argv);

   global_fd = sysevent_open(server_ip, server_port, SE_VERSION, SE_NAME, &global_id);
   APPLY_PRINT("[Utopia] global_fd is %d\n",global_fd);
   if (0 == global_fd) {
      APPLY_PRINT("[Utopia] %s unable to register with sysevent daemon.\n", argv[0]);
      printf("[Utopia] %s unable to register with sysevent daemon.\n", argv[0]);
   }
   int rc;
   rc = syscfg_init();
   if (rc) {
      printf("[Utopia] %s unable to initialize with syscfg context. Reason (%d)\n", 
             argv[0], rc);
      sysevent_close(global_fd, global_id);
      return(-1);
   }



   if ( global_fd <= 0 )
   {		
	APPLY_PRINT("[Utopia] Retrying sysevent open\n");

	global_fd=0;
   	global_fd = sysevent_open(server_ip, server_port, SE_VERSION, SE_NAME, &global_id);
	APPLY_PRINT("[Utopia] Global fd after retry is %d\n",global_fd);	

	if ( global_fd <= 0)
		APPLY_PRINT("[Utopia] Retrying sysevent open also failed %d\n",global_fd);
	
   }

   set_defaults();
   if (syscfg_dirty) {
      printf("[utopia] [init] committing default syscfg values\n");
      syscfg_commit();
   }
   sysevent_close(global_fd, global_id);
   

   return(0);
}

