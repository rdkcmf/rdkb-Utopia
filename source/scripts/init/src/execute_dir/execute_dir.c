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
===================================================================
    This programs will execute all files in a given directory
    in lexicographic order. This is useful for startup shell scripts
    that should be called in some order.

    If no argument is given on the command line then the default
    directory will be used. A single argument provides an override
    directory
===================================================================
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

/*
 * routines to go through a well known directory and start up any executables
 * found in that directory
 */
typedef struct file_entry_s {
   char                *name;
   struct file_entry_s *next;
} file_entry_t;

/*
 * Name         : order_dir
 * Purpose      : Put all executables in a given directory in lexicographic order
 * Parameters   :
 *    dir          : The path/name of a directory
 *    head         : head of the list to put executable names on
 */
static file_entry_t *order_dir(char *dir, file_entry_t *head)
{
   // go to the execute directory and start all programs in that dir
   DIR *dp = opendir(dir);
   if (NULL == dp) {
      return(NULL);
   }

   struct dirent *dirp;
   file_entry_t *cur;
   file_entry_t *prev;

   while (NULL != (dirp = readdir(dp))) {
      file_entry_t *newent;

      if (0 == strcmp(dirp->d_name, ".") ||
          0 == strcmp(dirp->d_name, "..")||
	  dirp->d_name[0] == '.') {
         continue;
      }
      if (NULL == (newent = (file_entry_t *)malloc(sizeof(file_entry_t))) ) {
         printf("Unable to allocate memory for dirent. Truncating startup of init.d\n");
         break;
      } else {
         newent->name = strdup(dirp->d_name);
         newent->next = NULL;
      }
      if (NULL == head) {
         head = newent;
      } else if (0 > strcmp(dirp->d_name, head->name)) {
            newent->next = head;
            head = newent;
      } else {
         prev = head;
         cur = head->next;
         while (NULL != cur) {
            if (0 > strcmp(dirp->d_name, cur->name)) {
               cur = NULL;
            } else {
               prev = cur;
               cur = cur->next;
            }
         }
         newent->next = prev->next;
         prev->next = newent;
      }
   }
   closedir(dp);
   return(head);
}

/*
 * Name         : start_execute_dir
 * Purpose      : Execute all executables in a given directory
 * Parameters   :
 *    dir          : The path/name of a directory
 */
static void start_execute_dir(char *dir)
{
   file_entry_t *head = NULL;
   file_entry_t *cur;
   file_entry_t *prev;
   head = order_dir(dir, head);

   // we now have a lexicographically ordered list of filenames
   // in the init directory
   if (NULL == head) {
      return;
   }

   cur = head;
   while (NULL != cur) {
      char command[1026];
      snprintf(command, sizeof(command), "%s/%s", dir, cur->name);
      system(command);
      prev = cur;
      cur = cur->next;
      free(prev->name);
      free(prev);
   }
}

/*
 * Name         : stop_execute_dir
 * Purpose      : Execute all executables in a given directory with the parameter "stop"
 * Parameters   :
 *    dir          : The path/name of a directory
 */
static void stop_execute_dir(char *dir, char *param)
{
   file_entry_t *head = NULL;
   file_entry_t *cur;
   file_entry_t *prev;
   head = order_dir(dir, head);

   // we now have a lexicographically ordered list of filenames
   // in the init directory
   if (NULL == head) {
      return;
   }

   cur = head;
   while (NULL != cur) {
      char command[1026];
      snprintf(command, sizeof(command), "%s/%s %s", dir, cur->name, param);
      system(command);
      prev = cur;
      cur = cur->next;
      free(prev->name);
      free(prev);
   }
}


/*
 * if argv[1] exists then it is the name of the directory containing sysevent
 * registration code
 *
 * if argv[2] exists then call executables with input parameter
 */
int
main(int argc, char **argv)
{
   if (argc < 2) {
      return 0;
   }   
   if (3 == argc) {
      stop_execute_dir(argv[1], argv[2]);
   } else {
      start_execute_dir(argv[1]);
   }
   return(0);
}

