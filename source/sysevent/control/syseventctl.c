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
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include "sysevent/sysevent.h"
#include "secure_wrapper.h"

#define SE_NAME             "sectl"


static unsigned short server_port;
static char  server_host[256]; // ip or hostname
static int use_tcp = 0;

static int server_connect(char *host, short port, token_t *token)
{
   int fd = -1;
   if (1 == use_tcp) {
      port &= 0x0000FFFF;
      fd = sysevent_open(host, port, SE_VERSION, SE_NAME, token);
      if (0 > fd) {
         // printf("Unable to register with sysevent daemon at %s %u.\n", host, port);
         return(-1);
      }
   } else {
      fd = sysevent_local_open(UDS_PATH, SE_VERSION, SE_NAME, token);
      if (0 > fd) {
         // printf("Unable to register with sysevent daemon at %s.\n", UDS_PATH);
         return(-1);
      }

   }
   return(fd);
}

static int server_connect_data(char *host, short port, token_t *token)
{
   int fd = -1;
   if (1 == use_tcp) {
      port &= 0x0000FFFF;
      fd = sysevent_open_data(host, port, SE_VERSION, SE_NAME, token);
      if (0 > fd) {
         // printf("Unable to register with sysevent daemon at %s %u.\n", host, port);
         return(-1);
      }
   } else {
      fd = sysevent_local_open_data(UDS_PATH, SE_VERSION, SE_NAME, token);
      if (0 > fd) {
         // printf("Unable to register with sysevent daemon at %s.\n", UDS_PATH);
         return(-1);
      }

   }
   return(fd);
}


static int server_disconnect(int fd, const token_t token)
{
   sysevent_close(fd, token);
   return(0);
}

// if there is no current value then return ""
static int handle_get(char *target) 
{
   int fd; 
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      puts("");
      return(-1);
   }

   se_buffer return_buffer;
   int  rc;
   rc = sysevent_get(fd, token, target, return_buffer, sizeof(return_buffer));
   server_disconnect(fd, token);
   if (rc) {
      printf("Unable to get ->%s<-. Reason (%d) %s\n", target, rc, SE_strerror(rc));   
   } else {
      if ('\0' == return_buffer[0]) {
         puts("");
      } else {
         puts(return_buffer);
      }
   }

   return(rc);
}

static int handle_unset(char *target)
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      puts("");
      return(-1);
   }

   int  rc;
   rc = sysevent_unset(fd, token, target);
   server_disconnect(fd, token);
   if (rc) {
      printf("Unable to get ->%s<-. Reason (%d) %s\n", target, rc, SE_strerror(rc));   
   }
   return(rc);
}

// if there is no current value then return ""
static int handle_get_data(char *target)
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect_data(server_host, server_port, &token))) {
      puts("");
      return(-1);
   }

   unsigned int bin_size = sysevent_get_binmsg_maxsize();
   char *return_buffer = malloc(bin_size);
   int  rc = 0;
   int copiedBufLength = 0;
   if (return_buffer)
   {
       rc = sysevent_get_data(fd, token, target, return_buffer, bin_size ,&copiedBufLength);
       server_disconnect(fd, token);
       if (rc) {
           printf("Unable to get ->%s<-. Reason (%d) %s\n", target, rc, SE_strerror(rc));
       } else {
           FILE *pFile = fopen("/tmp/getdata.bin","wb");
           if (pFile != NULL)
           {
               fwrite(return_buffer,copiedBufLength,1,pFile);
               fclose(pFile);
           }
       }
       free(return_buffer);
       printf("freed %s\n", __FUNCTION__);
   }
   return(rc);
}

static int handle_set(char *target, char *value) 
{
   int fd;
   token_t token;
   if (0 > ( fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }
#if 0
   char *valp;
   if (NULL == value || 0 == strcasecmp ("NULL", value)) {
      valp = NULL;
   } else {
      valp = value;
   }
#endif
   int rc;
   rc = sysevent_set(fd, token, target, value,0);

   server_disconnect(fd, token);
   return(rc);
}

static int handle_set_data(char *target, char *value)
{
   int fd;
   token_t token;
   int length = 0;
   if (0 > ( fd = server_connect_data(server_host, server_port, &token))) {
      return(-1);
   }

   char *valp = NULL;
   if (NULL == value) {
      valp = NULL;
      return -1;
   } else {
      valp = value;
   }

   FILE *pFile = fopen(value,"rb");
   if (pFile != NULL)
   {
       fseek(pFile, 0, SEEK_END);
       length = ftell(pFile);
       rewind(pFile);
       valp = (char *) malloc(length * sizeof(char));
       if (!valp) {
	    fclose(pFile); /* CID 160988: Resource leak */
            return -1;
       }
       fread(valp,length,1,pFile);
       fclose(pFile);
   }

   int rc;
   int fileread = access("/tmp/sysevent_debug", F_OK);
   if (fileread == 0)
   {
          unsigned int bin_size = sysevent_get_binmsg_maxsize();

       v_secure_system("echo fname %s: length %d bin size %u datafd %d >> /tmp/sys_d.txt",__FUNCTION__,length,bin_size,fd);
   }
   rc = sysevent_set_data(fd, token, target, valp, length);

   server_disconnect(fd, token);
   free(valp);
   printf("freed %s\n", __FUNCTION__);
   return(rc);
}


static int handle_setunique(char *target, char *value) 
{
   int fd;
   token_t token;
   if (0 > ( fd = server_connect(server_host, server_port, &token))) {
      puts("");
      return(-1);
   }
#if 0
   char *valp;
   if (NULL == value || 0 == strcasecmp ("NULL", value)) {
      valp = NULL;
   } else {
      valp = value;
   }
#endif
   se_buffer return_buffer;
   int  rc;
   rc = sysevent_set_unique(fd, token, target, value, return_buffer, sizeof(return_buffer));
   if (rc) {
      printf("Unable to setunique ->%s<-. Reason (%d) %s\n", target, rc, SE_strerror(rc));   
   } else {
      if ('\0' == return_buffer[0]) {
         puts("");
      } else {
         puts(return_buffer);
      }
   }

   server_disconnect(fd, token);
   return(rc);
}

// if there is no current value then return ""
static int handle_getunique(char *target, unsigned int *iterator) 
{
   int fd; 
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      puts("");
      return(-1);
   }

   char subject_buffer[100];
   se_buffer value_buffer;
   int  rc;
   rc = sysevent_get_unique(fd, token, target, iterator, subject_buffer, sizeof(subject_buffer), value_buffer, sizeof(value_buffer));
   server_disconnect(fd, token);
   if (rc) {
      printf("Unable to get ->%s<-. Reason (%d) %s\n", target, rc, SE_strerror(rc));   
   } else {
      if ('\0' == value_buffer[0]) {
         puts("");
      } else {
//         char output_buf[512];
//         snprintf(output_buf, sizeof(output_buf), "%s : %s iterator(%u)", subject_buffer, value_buffer, *iterator);
         puts(value_buffer);
      }
   }

   return(rc);
}

static int handle_delunique(char *target, unsigned int *iterator)
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }

   int  rc;
   rc = sysevent_del_unique(fd, token, target, iterator);
   server_disconnect(fd, token);
   if (rc) {
      printf("Unable to get ->%s<-. Reason (%d) %s\n", target, rc, SE_strerror(rc));
   }

   return(rc);
}

static int handle_getiterator(char *target, unsigned int *iterator) 
{
   int fd; 
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      puts("");
      return(-1);
   }

   int  rc;
   rc = sysevent_get_next_iterator(fd, token, target, iterator);
   server_disconnect(fd, token);
   if (rc) {
      printf("Unable to get ->%s<-. Reason (%d) %s\n", target, rc, SE_strerror(rc));   
   } else {
      char output_buf[100];
      snprintf(output_buf, sizeof(output_buf), "%u", *iterator);
      if (SYSEVENT_NULL_ITERATOR == (unsigned int) atoi(output_buf)) {
         puts("");
      } else {
         puts(output_buf);
      }
   }

   return(rc);
}

static int handle_set_options(char *target, int flags)
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }

   int rc;
   rc = sysevent_set_options(fd, token, target, flags);

   server_disconnect(fd, token);
   return(rc);
}
   
// example   async value "/home/enright/src/connectLib/connectClient" test 1
//           async value "/usr/bin/gcalctool" ">&" "/tmp/CRAP"
//           async value "/usr/bin/firefox" "www.google.com" "/tmp/crap"
// numparams contains the number of arguments starting from the executable
// params    contains a pointer to the executable name
// NOTE:
//    set_callback takes a flag which controls the callback
//    You could set the flag to be ACTION_FLAG_MULTIPLE_ACTIVATION
//    which would allow more than one activation at a time.
//    (The default is to serialize the activation target)
//    Currently handle_async does not take a parameter for flag settin
//    so the flag will be hardcoded to ACTION_FLAG_NONE
//  NOTE: we add an implicit first argument which is the name of the event
static int handle_async(int numparams, char **params) 
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }

   int rc;
   // argv[0] should be the target
   // argv[1] should be the function to call
   // argv[2] ... should be parameters

   char *target;
   char *function;
   if (0 > numparams) {
      printf("No Target found.");
      server_disconnect(fd, token);
      return(-1);
   } else {
      target = params[0];
   }

   if (1 > numparams) {
      printf("No call found.");
      server_disconnect(fd, token);
      return(-1);
   } else {
      function = params[1];
   }

   async_id_t async_id;
   int param_count = numparams - 2; // remove 1st 2 args from parameter count
   if (0 >= param_count) {
      rc = sysevent_setcallback(fd, token, ACTION_FLAG_NONE, target, function, 0, NULL, &async_id);
   } else {
      char **args;
      args = (char **) malloc( sizeof(char *) * (param_count));
      if (NULL == args) {
         printf("Memory allocation error.");
         server_disconnect(fd, token);
         return(-1);
      }

      int i;
      for (i=0; i< param_count; i++) {
         args[i] = params[i + 2]; 
      }
      rc = sysevent_setcallback(fd, token, ACTION_FLAG_NONE, target,function, param_count, args, &async_id);
      /* CID 74403: Resource leak */
      free(args);
   }
   printf("0x%x 0x%x\n", async_id.trigger_id, async_id.action_id);
   server_disconnect(fd, token);
   return(rc);
}

// example   async_with_flags 0 value "/home/enright/src/connectLib/connectClient" test 1
//           async_with_flags 1 value "/usr/bin/gcalctool" ">&" "/tmp/CRAP"
//           async_with_flags 1 value "/usr/bin/firefox" "www.google.com" "/tmp/crap"
// numparams contains the number of arguments starting from the executable
// params    contains a pointer to the executable name
//  NOTE: we add an implicit first argument which is the name of the event
static int handle_async_with_flags(action_flag_t flags, int numparams, char **params) 
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }
   int rc;

   char *target;
   char *function;
   if (0 > numparams) {
      printf("No Target found.");
      server_disconnect(fd, token);
      return(-1);
   } else {
      target = params[0];
   }

   if (1 > numparams) {
      printf("No call found.");
      server_disconnect(fd, token);
      return(-1);
   } else {
      function = params[1];
   }

   async_id_t async_id;
   int param_count = numparams - 3; // remove 1st 3 args from parameter count
   if (0 >= param_count) {
      rc = sysevent_setcallback(fd, token, flags, target, function, 0, NULL, &async_id);
   } else {
      char **args;
      args = (char **) malloc( sizeof(char *) * (param_count));
      if (NULL == args) {
         printf("Memory allocation error.");
         server_disconnect(fd, token);
         return(-1);
      }

      int i;
      for (i=0; i< param_count; i++) {
         args[i] = params[i + 2];
      }
      rc = sysevent_setcallback(fd, token, flags, target,function, param_count, args, &async_id);
      /* CID 61153: Resource leak */
      free(args);
   }
   printf("0x%x 0x%x\n", async_id.trigger_id, async_id.action_id);
   server_disconnect(fd, token);
   return(rc);
}



static int handle_remove_async(char *trigger_str, char *action_str) 
{
   if (NULL == trigger_str || NULL == action_str) {
      return(-1);
   }

   async_id_t async_id; 
   async_id.trigger_id = strtoll(trigger_str, NULL, 16);  // strtol trunctates
   async_id.action_id  = strtoll(action_str, NULL, 16);   // strtol trunctates
   int fd;
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }

   int rc;
   rc = sysevent_rmcallback(fd, token, async_id);
   server_disconnect(fd, token);
   return(rc);
}

static int handle_notification(char *subject) 
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }

   async_id_t async_id;
   int rc = sysevent_setnotification(fd, token, subject, &async_id);
   printf("async id:  0x%x 0x%x\n", async_id.trigger_id, async_id.action_id);

   char name_buf[200];
   se_buffer val_buf;
   int  name_size;
   int  val_size;
   int i;
   // in case you cant tell this is just example code
   for (i=0; i<3; i++) {
      name_size = sizeof(name_buf);
      val_size  = sizeof(val_buf);
      rc = sysevent_getnotification(fd, token, name_buf, &name_size, val_buf, &val_size, &async_id);

      printf("rc is %d\n", rc);
      printf("name ->%s<- value ->%s<-\n", name_buf, val_buf);
      printf("asynch_id:  0x%x 0x%x\n", async_id.trigger_id, async_id.action_id);
   }
   rc = sysevent_rmcallback(fd, token, async_id);
   server_disconnect(fd, token);
   return(rc);
}

static int handle_notification_data(char *subject)
{
   int fd;
   token_t token;
   if (0 > (fd = server_connect_data(server_host, server_port, &token))) {
      return(-1);
   }

   async_id_t async_id;
   int rc = sysevent_setnotification(fd, token, subject, &async_id);
   printf("async id:  0x%x 0x%x\n", async_id.trigger_id, async_id.action_id);

   char name_buf[200];
   unsigned int bin_size = sysevent_get_binmsg_maxsize();
   char *val_buf = malloc(bin_size);
   int  name_size;
   int  val_size;
   // in case you cant tell this is just example code
   while (val_buf) 
   {
      name_size = sizeof(name_buf);
      val_size  = bin_size;
      rc = sysevent_getnotification_data(fd, token, name_buf, &name_size, val_buf, &val_size, &async_id);
      if (0 == strcmp(name_buf,subject))
      {
        FILE *pFile = fopen("/tmp/notify_data.bin","wb");
          printf("rc is %d\n", rc);
          printf("name ->%s<- \n", name_buf);
          printf("asynch_id:  0x%x 0x%x\n", async_id.trigger_id, async_id.action_id);

          if (pFile != NULL)
          {
              fwrite(val_buf,val_size,1,pFile);
              fclose(pFile);
          }
          break;
      }
      sleep(1);
   }
   rc = sysevent_rmcallback(fd, token, async_id);
   server_disconnect(fd, token);
   printf("freed %s\n", __FUNCTION__);
   free(val_buf);
   return(rc);
}

static int handle_show(char *filename) 
{
   int fd; 
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      return(-1);
   }

   int  rc;
   rc = sysevent_show(fd, token, filename);
   server_disconnect(fd,token);
   if (rc) {
      printf("Unable to dump to ->%s<-. Reason (%d) %s\n", filename, rc, SE_strerror(rc));   
   }
   return(rc);
}

static int handle_debug(int level)
{
   // NOTE this does NOT call server_connect/disconnect
   return(sysevent_debug(server_host, server_port, level));
}

static int handle_ping(void)
{
   int fd; 
   token_t token;
   if (0 > (fd = server_connect(server_host, server_port, &token))) {
      printf("FAIL");
      return(-1);
   }
   struct timeval tv;
   tv.tv_sec  = 5;
   tv.tv_usec = 0;
   int rc = sysevent_ping_test(fd, token, &tv);   
   server_disconnect(fd,token);
   if (0 != rc) {
      printf("FAIL");
      return(-1);
   }
   printf("SUCCESS");
   return(0);
}

static void printhelp(char *name) {
      printf ("Usage %s --port port --ip server_host --help command params\n", name);
      printf (" A utility to communicate with a sysevent daemon\n");
      printf ("    the default daemon is reached through a unix domain socket\n");
      printf ("    a daemon can also be reach by specifying an ip address/port\n");
      printf (" commands:\n");
      printf ("    get name\n");
      printf ("    set name value\n");
      printf ("    setdata name binary_file_path\n");
      printf ("         successful output stores --> /tmp/setdata.bin\n");
      printf ("    getdata name\n");
      printf ("         successful output stores --> /tmp/getdata.bin\n");
      printf ("    setunique name value\n");
      printf ("       add a value to a pool named name\n");
      printf ("       if the values must maintain order during iteration then the pool name is !name\n");
      printf ("    getunique name [iterator]\n");
      printf ("       iterator as returned by getiterator\n");
      printf ("       if no iterator is provided then get first iterator\n");
      printf ("    delunique name [iterator]\n");
      printf ("       iterator as returned by getiterator\n");
      printf ("       if no iterator is provided then delete first iterator\n");
      printf ("    getiterator name iterator\n");
      printf( "       get next iterator.\n");
      printf ("       if no iterator is provided then get first iterator\n");
      printf ("    setoptions name options\n");
      printf ("       options is a bit field with the values\n");
      printf ("          0x00000000 - default\n");
      printf ("          0x00000001 - send notifications serially\n");
      printf ("          0x00000002 - send notifications upon set even if no value changed\n");
      printf ("          0x00000004 - tuple value is write once read many\n");
      printf ("    async name executable params\n");
      printf ("       executable is the path and name of an executable to run when notified\n");
      printf ("       params is zero or more parameters of the form\n");
      printf ("          name  - to have \"name\" passed in the command-line to the executable\n");
      printf ("          $name  - to have the runtime value of tuple <name> in sysCfg passed in the command-line to the executable\n");
      printf ("          @name  - to have the runtime value of tuple <name> in sysEvent passed in the command-line to the executable\n");
      printf ("        Note that implicit parameters event name and event value (or NULL) will be prepended to param list when executable is invoked\n");
      printf ("    async_with_flags flags name executable params\n");
      printf ("       flags is action_flag_t flags to apply (1 means allow multiple simultaneous invokations of async)\n");
      printf ("       executable is the path and name of an executable to run when notified\n");
      printf ("       params is zero or more parameters of the form\n");
      printf ("          name  - to have \"name\" passed in the command-line to the executable\n");
      printf ("          $name  - to have the runtime value of tuple <name> in sysCfg passed in the command-line to the executable\n");
      printf ("          @name  - to have the runtime value of tuple <name> in sysEvent passed in the command-line to the executable\n");
      printf ("        Note that implicit param aysnc name will be prepended to param list\n");
      printf ("    notification name\n");
      printf ("    notificationdata name\n");
      printf ("          notified output stores into -> /tmp/notify_data.bin\n");
      printf ("    rm_async asyncid_1 asyncid_2\n");
      printf ("       asyncid_1 and 2 are two halves of an asyncid\n");
      printf ("    show  name_of_file_to_print_to\n");
      printf ("    debug debug_level\n");
      printf ("    debug 0xFEEDFEED\n");
      printf ("          print system stats\n");
      printf ("    debug 0xFEECFEEC\n");
      printf ("          print client list\n");
      printf ("    ping\n");
      printf ("          ping syseventd and wait up to 5 secs for response\n");
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
      c = getopt_long (argc, argv, ":p:i:hc", long_options, &option_index);
      if (c == -1) {
         break;
      }

      switch (c) {
         case 'p':
            server_port = (0x0000FFFF & (unsigned short) atoi(optarg));
            break;

         case 'i':
            snprintf(server_host, sizeof(server_host), "%s", optarg);
            use_tcp=1;
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
 * Utility to send SE commands to a sysevent daemon
 *
 * first parameter must be the ip address or hostname of the sysevent daemon
 * second parameter must be command  (set get async etc)
 */
int main(int argc, char **argv)
{

   int retval = -1;
   // set defaults
   snprintf(server_host, sizeof(server_host), "127.0.0.1");
   server_port = SE_SERVER_WELL_KNOWN_PORT;
   use_tcp = 0;

   //nice value of -20 is removed as syseventd should run with normal priority

   // parse commandline for options and readjust defaults if requested
   int next_arg = get_options(argc, argv);

   // there is at least 1 parameters left
   if (1 > argc-next_arg) {
      printhelp(argv[0]);
      return(retval);
   }

   // parse the next command-line argument as the command  
   if (!strcmp(argv[next_arg], "get")) {
      if ((argc-1) != next_arg+1) {
         printhelp(argv[0]);
      } else {
         retval = handle_get(argv[next_arg+1]);
      }
      return(retval);
   }

      // parse the next command-line argument as the command
   if (!strcmp(argv[next_arg], "unset")) {
      if ((argc-1) != next_arg+1) {
         printhelp(argv[0]);
      } else {
         retval = handle_unset(argv[next_arg+1]);
      }
      return(retval);
   }

   if (!strcmp(argv[next_arg], "getdata")) {
      if ((argc-1) != next_arg+1) {
         printhelp(argv[0]);
      } else {
         retval = handle_get_data(argv[next_arg+1]);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "set")) {

      char *val;
      if (argc <= next_arg+2                || 
          NULL == (val = argv[next_arg+2])  ||
          0 == strcasecmp("NULL", val)) {
         val = NULL;
      } 
      if ((argc-1) != next_arg+1 && (argc-1) != next_arg+2) {
         printhelp(argv[0]);
      } else {
         retval = handle_set(argv[next_arg+1], val);
      }
      return(retval);
   }
 if (!strcmp(argv[next_arg], "setdata")) {

      char *val;
      int fileread = access("/tmp/sysevent_debug", F_OK);
      if (argc <= next_arg+2                ||
          NULL == (val = argv[next_arg+2])  ||
          0 == strcasecmp("NULL", val)) {
         val = NULL;
      }
      if (fileread == 0)
      {
          v_secure_system("echo fname %s: arg.count %d index %d >>  /tmp/sys_d.txt",__FUNCTION__,argc,next_arg);
      }
      if ((argc-1) != next_arg+1 && (argc-1) != next_arg+2) {
         printhelp(argv[0]);
      } else {
         retval = handle_set_data(argv[next_arg+1], val);
      }
      return(retval);
   }

   if (!strcmp(argv[next_arg], "setunique")) {

      char *val;
      if (argc <= next_arg+2                || 
          NULL == (val = argv[next_arg+2])  ||
          0 == strcasecmp("NULL", val)) {
         val = NULL;
      } 
      if ((argc-1) != next_arg+1 && (argc-1) != next_arg+2) {
         printhelp(argv[0]);
      } else {
         retval = handle_setunique(argv[next_arg+1], val);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "getunique")) {
      if ((argc-1) < next_arg+1) {
         printhelp(argv[0]);
      } else {
         unsigned int iterator;
         if (NULL == argv[next_arg+2]) {
            iterator = SYSEVENT_NULL_ITERATOR;
         } else {
            iterator= atoi(argv[next_arg+2]);
         }
         retval = handle_getunique(argv[next_arg+1], &iterator);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "delunique")) {
      if ((argc-1) < next_arg+1) {
         printhelp(argv[0]);
      } else {
         unsigned int iterator;
         if (NULL == argv[next_arg+2]) {
            iterator = SYSEVENT_NULL_ITERATOR;
         } else {
            iterator = atoi(argv[next_arg+2]);
         }
         retval = handle_delunique(argv[next_arg+1], &iterator);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "getiterator")) {
      if ((argc-1) < next_arg+1) {
         printhelp(argv[0]);
      } else {
         unsigned int iterator;
         if (NULL == argv[next_arg+2]) {
            iterator = SYSEVENT_NULL_ITERATOR;
         } else {
            iterator = atoi(argv[next_arg+2]);
         }
         retval = handle_getiterator(argv[next_arg+1], &iterator);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "setoptions")) {

      if ((argc-1) != next_arg+2) {
         printhelp(argv[0]);
      } else {
         long int flags;
         flags = strtoll(argv[next_arg+2], NULL, 16); // strtol truncates on some platforms
         retval = handle_set_options(argv[next_arg+1], flags);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "async")) {
      if ((argc-1) < next_arg+2) {
         printhelp(argv[0]);
      } else {
         retval = handle_async(argc-2, &(argv[next_arg+1]));
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "async_with_flags")) {
      if ((argc-1) < next_arg+3) {
         printhelp(argv[0]);
      } else {
         long int flags;
         flags = strtoll(argv[next_arg+1], NULL, 16); // strtol truncates on some platforms

         retval = handle_async_with_flags(flags, argc-2, &(argv[next_arg+2]));
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "rm_async")) {
      if ((argc-1) != next_arg+2) {
         printhelp(argv[0]);
      } else {
         retval = handle_remove_async(argv[next_arg+1], argv[next_arg+2]);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "notification")) {
      if ((argc-1) != next_arg+1) {
         printhelp(argv[0]);
      } else {
         retval = handle_notification(argv[next_arg+1]);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "notificationdata")) {
       if ((argc-1) != next_arg+1) {
           printhelp(argv[0]);
       } else {
           retval = handle_notification_data(argv[next_arg+1]);
       }
       return(retval);
   }

   if (!strcmp(argv[next_arg], "show")) {
      if ((argc-1) != next_arg+1) {
         printhelp(argv[0]);
      } else {
         retval = handle_show(argv[next_arg+1]);
      }
      return(retval);
   }
   if (!strcmp(argv[next_arg], "debug")) {
      if ((argc-1) != next_arg+1) {
         printhelp(argv[0]);
      } else {
         long int level;
         level = strtoll(argv[next_arg+1], NULL, 16); // strtol truncates on some platforms

         handle_debug(level);
      }
      return(0);
   }
   if (!strcmp(argv[next_arg], "ping")) {
      handle_ping();
      return(0);
   }
   printf("%s: Unknown argument %s\n", argv[0], argv[next_arg]);
   printhelp(argv[0]);
   return(retval);
}
