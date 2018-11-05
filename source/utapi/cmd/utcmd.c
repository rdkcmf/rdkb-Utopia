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
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include <utapi/utapi.h>
#include <utapi/lib/utapi_wlan.h>

typedef enum {
    CMD_FACTORY_RESET,
    CMD_CFG_BACKUP,
    CMD_CFG_RESTORE,
    CMD_HELP,
    CMD_UNKNOWN,
} cmd_t;

static inline void usage()
{
    printf("Usage: utcmd [factory_reset | cfg_backup <file> | cfg_restore <file> | help ]\n");
}

static int get_cmd (const char *cmdstr) 
{
    if (!cmdstr) {
        return CMD_UNKNOWN;
    }

    if (0 == strcasecmp(cmdstr, "factory_reset"))         { return CMD_FACTORY_RESET; }
    if (0 == strcasecmp(cmdstr, "cfg_backup"))            { return CMD_CFG_BACKUP; }
    if (0 == strcasecmp(cmdstr, "cfg_restore"))           { return CMD_CFG_RESTORE; }
    if (0 == strcasecmp(cmdstr, "help"))                  { return CMD_HELP; }

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

    if (argc < 2) {
        usage();
        return 1;
    } 

    argc -= 1;
    char **cmd = argv+1;
    UtopiaContext ctx;

    if (!Utopia_Init(&ctx)) {
        return 2;
    }

    cmd_t cmdtype = get_cmd(cmd[0]);
    switch (cmdtype) {
    case CMD_FACTORY_RESET:
        rc = Utopia_RestoreFactoryDefaults();
        if (UT_SUCCESS == rc) {
            (void) Utopia_Reboot();
        }
        break;
    case CMD_CFG_RESTORE:
        if (argc < 2) {
            usage();
            return 1;
        }
        rc = Utopia_RestoreConfiguration(cmd[1]);
        if (UT_SUCCESS == rc) {
            (void) Utopia_Reboot();
        }
        break;
    case CMD_CFG_BACKUP:
        if (argc < 2) {
            usage();
            return 1;
        }
        rc = Utopia_BackupConfiguration(cmd[1]);
        break;
    default:
        usage(); 
        return 1;
    }

    Utopia_Free(&ctx, 0);

    return rc;
}
