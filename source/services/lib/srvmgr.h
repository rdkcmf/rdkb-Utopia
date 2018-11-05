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

#ifndef _SRVMGR_H_
#define _SRVMGR_H_

#if defined (_CBR_PRODUCT_REQ_) || defined (_XB6_PRODUCT_REQ_)
#define CONSOLE_LOG_FILE "/rdklogs/logs/Consolelog.txt.0"
#else
#define CONSOLE_LOG_FILE "/rdklogs/logs/ArmConsolelog.txt.0"
#endif

#define DBG_PRINT(fmt ...)     {\
										FILE     *fp        = NULL;\
                                        fp = fopen ( CONSOLE_LOG_FILE, "a+");\
                                        if (fp)\
                                        {\
                                            fprintf(fp,fmt);\
                                            fclose(fp);\
                                        }\
                               }\

/*
 * These are flags which correspond to sysevent.h flags.
 * They MUST be in sync
 */
#define TUPLE_FLAG_NORMAL                   "0x00000000"
#define TUPLE_FLAG_SERIAL                   "0x00000001"
#define TUPLE_FLAG_EVENT                    "0x00000002"
#define ACTION_FLAG_NORMAL                  "0x00000000"
#define ACTION_FLAG_NOT_THREADSAFE          "0x00000001"
#define ACTION_FLAG_COLLAPSE_PENDING_QUEUE  "0x00000002"

/*
 * Typedef      : cmd_type_t
 * Purpose      : The command line parameter
 */
typedef enum {
   nochoice, 
   start, 
   stop, 
   restart
} cmd_type_t;

/*
 * Procedure     : parse_cmd_line
 * Purpose       : Figure out what the command line parameter is
 * Parameters    :
 *   argc            : The number of command line parameters
 *   argv            : The command line string array
 * Return Value  : the cmd_type_t
 */
cmd_type_t parse_cmd_line (int argc, char **argv);


/*
 * Procedure     : sm_register
 * Purpose       : Register a service's handler for activation upon default events
 *                 and upon custom events
 * Parameters    :
 *   srv_name        : The name of the service registering
 *   default_handler : The path/file to the handler for default events
 *   custom          : Information about the event and handlers using the format:
 *                        event_name | path/file to handler to be activated |
 *                        sysevent activation flags or NULL | sysevent tuple flags or NULL |
 *                        any extra parameters to be given to the handler upon activation
 *                     Note that extra parameters can be a syscfg runtime value (eg $wan_proto),
 *                     a sysevent runtime value (eg @current_wan_ipaddr) or a constant.
 * Return Value  :
 *   0                : success
 *   <0               : some failure
 */
int sm_register (const char* srv_name, const char* default_handler, const char** custom);

/*
 * Procedure     : sm_unregister
 * Purpose       : Unregister all notifications for a service
 * Parameters    :
 *   srv_name        : The name of the service unregistering
 * Return Value  :
 *   0                : success
 *   <0               : some failure
 */
int sm_unregister (const char* srv_name);

#endif  // _SRVMGR_H_
