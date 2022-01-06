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
#include <stdlib.h>
#include <string.h>
#include <syscfg/syscfg.h>

typedef enum {
    CMD_GET,
    CMD_SET,
    CMD_UNSET,
    CMD_COMMIT,
    CMD_ISMATCH,
    CMD_SHOW,
    CMD_DESTROY,
    CMD_UNKNOWN,
} cmd_t;

static inline void syscfg_create_usage()
{
    printf("Usage: syscfg_create -f file\n");
}

static inline void syscfg_usage()
{
    printf("Usage: syscfg [show | set [ns] name value | get [ns] name | unset [ns] name | \n       ismatch [ns] name value | commit]\n");
}

static void print_commit_error (int rc)
{
    if (0 == rc) {
        return;
    }
    switch (rc) {
    case ERR_IO_FILE_TOO_BIG:
        fprintf(stderr, "Error: syscfg contents too big to fit flash parition\n");
        break; 
    case ERR_IO_FILE_OPEN:
    case ERR_IO_FILE_STAT:
        fprintf(stderr, "Error: internal error handling tmp file\n");
        break; 
    }
}

static int get_cmd (const char *cmdstr) 
{
    if (!cmdstr) {
        return CMD_UNKNOWN;
    }

    if (0 == strcasecmp(cmdstr, "get"))         { return CMD_GET; }
    if (0 == strcasecmp(cmdstr, "ismatch"))     { return CMD_ISMATCH; }
    if (0 == strcasecmp(cmdstr, "set"))         { return CMD_SET; }
    if (0 == strcasecmp(cmdstr, "unset"))       { return CMD_UNSET; }
    if (0 == strcasecmp(cmdstr, "commit"))      { return CMD_COMMIT; }
    if (0 == strcasecmp(cmdstr, "show"))        { return CMD_SHOW; }
    if (0 == strcasecmp(cmdstr, "destroy"))     { return CMD_DESTROY; }

    return CMD_UNKNOWN;
}

/*
 * Return
 *    0 -- Success
 *    1 -- Failure
 */
int main(int argc, char **argv)
{
    int rc = 0;
    char *program;


    program = strrchr(argv[0], '/');
    program = program ? program+1 : argv[0];

    if (0 == strcmp(program, "syscfg_create")) {
        if (argc < 3) {
            syscfg_create_usage();
            return 1;
        }
        if (0 == strcasecmp(argv[1], "-f")) {
            rc = syscfg_create(argv[2], 0);
        } else {
            syscfg_create_usage();
            return 1;
        } 
        return rc;
    }

    if (0 == strcmp(program, "syscfg_destroy")) {
        // check to see if we are going to force the destroy, if not, prompt the user
        if (argc < 2 || (argc >= 2 && 0 != strcasecmp(argv[1], "-f"))) {
            printf("WARNING!!! Are your sure you want to destroy system configuration?\n This will cause the system to be unstable. Press CTRL-C to abort or ENTER to proceed.\n");
            /*CID 65876: Unchecked return value */
	    if (getchar() != EOF)
		printf("System configuration is going to destroy\n");
        }
        syscfg_destroy();
        return rc;
    }

    if (argc < 2) {
        syscfg_usage();
        return 1;
    } 
   argc -= 1;
   char **cmd = argv+1;
   char *name = NULL, *value = NULL, *ns = NULL;

   cmd_t cmdtype = get_cmd(cmd[0]);
   switch (cmdtype) {
   case CMD_GET:
       if (argc < 2) {
           syscfg_usage();
           return 1;
       }
       char val[512] = {0};
       if (argc > 2) {  // namespace
           ns = cmd[1];
           name = cmd[2];
       } else {
           name = cmd[1];
       }
       // scripts rely on the output of get so don't give a error message
       rc = syscfg_get(ns, name, val, sizeof(val));
       if (0 == rc) {
           puts(val);
       } else {
           puts("");
       }
       break;

   case CMD_SET:
       if (argc < 3) {
           syscfg_usage();
           return 1;
       }
       if (argc > 3) { // namespace specified
           ns = cmd[1];
           name = cmd[2];
           value = cmd[3];
       } else {
           name = cmd[1];
           value = cmd[2];
       }
       rc = syscfg_set(ns, name, value);
       if (0 == rc) {
           // printf("success\n");
           // syscfg_commit();   -- implicit commit
       } else {
           printf("Error. code=%d\n", rc);
       }
       return rc;

   case CMD_ISMATCH:
       if (argc < 3) {
           syscfg_usage();
           return 1;
       }
       if (argc > 3) { // namespace specified
           ns = cmd[1];
           name = cmd[2];
           value = cmd[3];
       } else {
           name = cmd[1];
           value = cmd[2];
       }
       unsigned int out_match = 1;
       rc = syscfg_is_match(ns, name, value, &out_match);
       if (0 == rc) {
           // printf("success\n");
           // syscfg_commit();   -- implicit commit
       } else {
           printf("Error. code=%d\n", rc);
       }
       return out_match;

   case CMD_UNSET:
       if (argc < 2) {
           syscfg_usage();
           return 1;
       }
       if (argc > 2) {  // namespace specified
           ns = cmd[1];
           name = cmd[2];
       } else {
           name = cmd[1];
       }
       rc = syscfg_unset(ns, name);
       if (0 == rc) {
           // printf("success\n");
           // syscfg_commit();   // implicit commit
       } else {
           // printf("Error. code=%d\n", rc);
       }
       return rc;
   case CMD_COMMIT:
       rc = syscfg_commit();
       print_commit_error(rc);
       return rc;
   case CMD_DESTROY:
       printf("WARNING!!! Are you sure you want to do this?\nPress CTRL-C to abort or ENTER to proceed\n");
       syscfg_destroy();
       break;
   case CMD_SHOW:
       {
       int len, sz;
       char *buf;
       buf = malloc(SYSCFG_SZ);
       if (buf) {
           if (0 == syscfg_getall(buf, SYSCFG_SZ, &sz)) {
               char *p = buf;
               while(sz > 0) {
                   len = printf(p);
                   printf("\n");
                   p = p + len + 1;
                   sz -= len + 1;
               }
           } else {
               printf("No entries\n");
           }
           free(buf);
       }
       long int used_sz = 0, max_sz = 0;
       syscfg_getsz(&used_sz, &max_sz);
       printf("Used: %ld of %ld\n", used_sz, max_sz);
       }
       break;
   default:
       syscfg_usage(); 
   }
   return 0;
}
