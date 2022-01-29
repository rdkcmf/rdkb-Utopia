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
/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * - Neither name of Intel Corporation nor the names of its contributors 
 * may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/


#include <stdlib.h>
#include <stdarg.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "pal_log.h"
#include "pal_log_internal.h"

/*
   Tags for valid commands issued at the command prompt 
 */
enum cmdloop_logcmds {
    HELP1 = 0,
    HELP2,

    SETROUTERIP,
    SETTXFLAG,
    GETSTATUS,
    SETLEVEL,
    SHOWMODULEINFO,
    GETLASTBUFFER,

    QUIT1,
    QUIT2
};

/*
   Data structure for parsing commands from the command line 
 */
struct cmdloop_commands {
    CHAR *str;                  // the string 
    INT32 cmdnum;                 // the command
    INT32 numargs;                // the number of arguments
} cmdloop_commands;

/*
   Mappings between command text names, command tag,
   and required command arguments for command line
   commands 
 */
LOCAL struct cmdloop_commands cmdloop_cmdlist[] = {
    {"h",       HELP1,          1},
    {"help",    HELP2,          1},

    {"router",  SETROUTERIP,    2},
    {"tx",      SETTXFLAG,      2},
    {"getall",  GETSTATUS,      1},
    {"set",     SETLEVEL,       3},
    {"show",    SHOWMODULEINFO, 3},
    {"getbuf",  GETLASTBUFFER,  1},

    {"q",       QUIT1,          1},
    {"quit",    QUIT2,          1}
};


typedef enum FMODE{
    NEWFILE,
    APPEND,

    MAXFMODE
} E_FMODE;

#define CLOSE(fd) close(fd)
#define STRCMP(str1, str2) (strcasecmp(str1, str2))
#define WAITINTVL (1)
#define SLEEP(s) (sleep(s)) // unit in second


INT32 sock_for_net = -1;
struct sockaddr_in sock_net_addr_send_to;
FILE *fp = NULL;
#define MAX_FILENAME_LENGTH 32
CHAR filename[MAX_FILENAME_LENGTH] = {"debug.log"};
CHAR rcvbuf[MAX_BUF_SIZE] = {0};
#define MAX_IP_ADDR_LENGTH 16
CHAR router_ip[MAX_IP_ADDR_LENGTH] = {0};
BOOL RouterIpConfiged = BOOL_FALSE;


pthread_t LogTid;
BOOL mainQuit = BOOL_FALSE;
BOOL threadQuit = BOOL_FALSE;



/***************************************************************************
*                                    functions for sending cmd to router                                            *
****************************************************************************/
LOCAL VOID _send_buf_to_router(CHAR* buf, INT32 buf_len)
{
    if (BOOL_FALSE==RouterIpConfiged) {
        fprintf(stderr, "packet not sent since router IP not configured yet\n");
    } else {
        sendto(sock_for_net,buf,buf_len, 0, (struct sockaddr *)&sock_net_addr_send_to,sizeof(sock_net_addr_send_to));
    }

    return;
}

LOCAL VOID _send_cmd_tx_flag_to_router(INT32 enable_flag)
{
    CHAR out_buf[MAX_BUF_SIZE];
#undef P_OUT_PKT_HEAD
#undef OUT_PKT_HEAD_LENGTH
#define P_OUT_PKT_HEAD           ((pkt_head_req_tx_t*)out_buf)
#define OUT_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_tx_t))

    P_OUT_PKT_HEAD->log_cmd = LOG_CMD_REQ_TX;
    P_OUT_PKT_HEAD->tx_value = enable_flag;
    _send_buf_to_router(out_buf, OUT_PKT_HEAD_LENGTH);
}

LOCAL VOID _send_cmd_get_status_to_router(VOID)
{
    CHAR out_buf[MAX_BUF_SIZE];
#undef P_OUT_PKT_HEAD
#undef OUT_PKT_HEAD_LENGTH
#define P_OUT_PKT_HEAD           ((pkt_head_req_get_status_t*)out_buf)
#define OUT_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_get_status_t))

    P_OUT_PKT_HEAD->log_cmd = LOG_CMD_REQ_GET_STATUS;

    _send_buf_to_router(out_buf,OUT_PKT_HEAD_LENGTH);
}

LOCAL VOID _send_cmd_set_module_level_to_router(INT32 module_number, INT32 module_level)
{
    CHAR out_buf[MAX_BUF_SIZE];
#undef P_OUT_PKT_HEAD
#undef OUT_PKT_HEAD_LENGTH
#define P_OUT_PKT_HEAD           ((pkt_head_req_set_module_level_t*)out_buf)
#define OUT_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_set_module_level_t))

    P_OUT_PKT_HEAD->log_cmd = LOG_CMD_REQ_SET_MODULE_LEVEL;
    P_OUT_PKT_HEAD->module = module_number;
    P_OUT_PKT_HEAD->level = module_level;

    _send_buf_to_router(out_buf,OUT_PKT_HEAD_LENGTH);
}

LOCAL VOID _send_cmd_get_last_buffer_to_router(VOID)
{
    CHAR out_buf[MAX_BUF_SIZE];
#undef P_OUT_PKT_HEAD
#undef OUT_PKT_HEAD_LENGTH
#define P_OUT_PKT_HEAD           ((pkt_head_req_get_last_buffer_t*)out_buf)
#define OUT_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_get_last_buffer_t))

    P_OUT_PKT_HEAD->log_cmd = LOG_CMD_REQ_GET_LAST_BUFFER;

    _send_buf_to_router(out_buf,OUT_PKT_HEAD_LENGTH);
}

LOCAL VOID _send_cmd_show_module_info_to_router(INT32 module_number, CHAR * input_string)
{
    CHAR out_buf[MAX_BUF_SIZE];
#undef P_OUT_PKT_HEAD
#undef OUT_PKT_HEAD_LENGTH
#define P_OUT_PKT_HEAD           ((pkt_head_req_show_module_debug_info_t*)out_buf)
#define OUT_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_show_module_debug_info_t))

    P_OUT_PKT_HEAD->log_cmd = LOG_CMD_REQ_SHOW_MODULE_DEBUG_INFO;
    P_OUT_PKT_HEAD->module = module_number;

    if (NULL == input_string)
    {
        out_buf[OUT_PKT_HEAD_LENGTH] = '\0';
    }
    else
    {
        snprintf(out_buf+OUT_PKT_HEAD_LENGTH,MAX_BUF_SIZE-OUT_PKT_HEAD_LENGTH,"%s",input_string);
    }
    _send_buf_to_router(out_buf,OUT_PKT_HEAD_LENGTH+strlen(out_buf+OUT_PKT_HEAD_LENGTH)+1);
}

LOCAL INT32 _help()
{
    fprintf(stderr, "\n");
    fprintf(stderr, "****************************************************************************\n");
    fprintf(stderr, "           CMD                |    Meaning\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "router [Router IP]            |set router IP\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "getall                        |get config status(tx status/module debug level)\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "tx  [1/0]                     |set this flag to 1 or 0\n");
    fprintf(stderr, "                              |enable/disable sending log msg to log center\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "set  [ModuleNum] [LogLevel]   |set debug level(0~4) on per-module basis\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "show [ModuleNum] [inputString]|show debuggging info of specific module\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "getbuf                        |get latest content in buffer\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "q/quit                        |quit the app\n");
    fprintf(stderr, "------------------------------|---------------------------------------------\n");
    fprintf(stderr, "h/help                        |get this helper info\n");
    fprintf(stderr, "****************************************************************************\n");
	return 0;
}


/***************************************************************************
*                                    main functions                                                                         *
****************************************************************************/
LOCAL VOID _close_sock(VOID)
{
    if (-1==sock_for_net)
        return;
    else if (sock_for_net>0) {
        CLOSE(sock_for_net);
        sock_for_net = -1;
    }

    return;
}

LOCAL FILE* _open_log_file(CHAR *input_filename, INT32 fmode)
{
    FILE *temp_fp = NULL;

    if (0!=input_filename[0]) {
        switch(fmode) { 
            case NEWFILE: 
                temp_fp = fopen(input_filename, "w+");
                break;
            case APPEND: 
                temp_fp = fopen(input_filename, "a");
                break;
            default:
                temp_fp = NULL;
                break;
        }
    }

    return temp_fp;
}


LOCAL VOID _close_log_file(FILE* input_fp)
{
    if (input_fp) {
        fclose(input_fp);
        input_fp = NULL;
    }

    return;
}

LOCAL VOID _parse_packet(CHAR *buf, INT32 buf_len, struct sockaddr *sock_addr,INT32 sock_len)
{
    if (NULL == buf) return;

    switch(buf[0])
    {
        /*from router transmitter back to center*/
        case LOG_CMD_RSP_TX:
#undef P_IN_PKT_HEAD
#define P_IN_PKT_HEAD           ((pkt_head_rsp_tx_t*)buf)
            if (LOG_CMD_RSQ_OK == P_IN_PKT_HEAD->rv_value)
            {
                fprintf(stderr, "response code of set tx flag: OK\n");
            }
            else
            {
                fprintf(stderr, "response code of set tx to %d: ERROR\n",(P_IN_PKT_HEAD->input_tx_value));
            }
            break;

        case LOG_CMD_RSP_GET_STATUS:
#undef IN_PKT_HEAD_LENGTH
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_rsp_get_status_t))
            fprintf(stderr, "response of get router log status info:\n");
            fprintf(stderr,"******************************************************************************\n");
            fprintf(stderr, "current ROUTER IP is %s\n", inet_ntoa(((struct sockaddr_in*)sock_addr)->sin_addr));
            fprintf(stderr, "%s\n", buf+IN_PKT_HEAD_LENGTH);
            break;

        case LOG_CMD_RSP_SET_MODULE_LEVEL:
#undef P_IN_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#define P_IN_PKT_HEAD           ((pkt_head_rsp_set_module_level_t*)buf)
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_rsp_set_module_level_t))
            if (LOG_CMD_RSQ_OK == P_IN_PKT_HEAD->rv_value)
            {
                fprintf(stderr, "response code of set module level: OK\n");
            }
            else
            {
                fprintf(stderr, "response code of set module(%d) to level(%d): ERROR\n",P_IN_PKT_HEAD->module,P_IN_PKT_HEAD->level);
            }
            break;

        case LOG_CMD_RSP_GET_LAST_BUFFER:
#undef IN_PKT_HEAD_LENGTH
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_rsp_get_last_buffer_t))
            fprintf(stderr, "response of show latest buffer:\n%s\n", buf+IN_PKT_HEAD_LENGTH);
            break;

        /*router app==>router transmitter==> center*/
        case LOG_DATA:
            fprintf(stderr, "%s\n",buf+1);
#ifndef ROUTER_VERSION
            if (NULL == (fp = _open_log_file(filename, APPEND))) {
                fprintf(stderr, "file open error\n");
                break;
            }
            fprintf(fp, "%s", buf+1);
            _close_log_file(fp);
#endif
            break;
        default:
            fprintf(stderr, "unknown packet from router\n" );
            break; 
    }
}


/***************************************************************************************
 * Function: log_task 
 *
 *  Description:
 *      This function is main thread of log client, which is used to receive packet and store into file
 *      if necessary
 *
 *  Parameters:
 *      arg: (in) NA for this function, just to comform with Linux thread function
 * 
 *  Return Values: VOID *
 *      Value returned to the caller who created this thread
 *
 ***************************************************************************************/
LOCAL VOID *_log_task(VOID *arg)
{
    fd_set rSet;
    struct sockaddr_in sock_net_router_addr;
    UINT32 sock_net_router_addr_len = sizeof(sock_net_router_addr);
    INT32 rv = -1;

    /*receive packet*/
    while (BOOL_TRUE != threadQuit) {

        FD_ZERO(&rSet);
        FD_SET(sock_for_net,&rSet);
        rv=select(sock_for_net+1,&rSet,NULL,NULL,NULL);
        if(rv==-1) break; /*break by signal*/
        if(FD_ISSET(sock_for_net,&rSet)) {
            rv=recvfrom(sock_for_net,rcvbuf,sizeof(rcvbuf),0,(struct sockaddr *)&sock_net_router_addr,&sock_net_router_addr_len);
            if(((UINT32)rv)>=sizeof(rcvbuf)) continue;  /*invalid length*/
            rcvbuf[rv]=0; /*to ensure it is null terminated*/
            _parse_packet(rcvbuf,rv,(struct sockaddr *)&sock_net_router_addr,sock_net_router_addr_len);
        }
    }

    pthread_exit(0);
}

LOCAL BOOL _check_ip_validity(CHAR *ip)
{
    INT32 arg1 = -1;
    INT32 arg2 = -1;
    INT32 arg3 = -1;
    INT32 arg4 = -1;
    INT32 num = -1;

    num = sscanf( ip, "%d.%d.%d.%d", &arg1, &arg2, &arg3, &arg4 );
    if (4!=num) return BOOL_FALSE;
    if ((0==arg1==arg2==arg3==arg4) ||
        (255==arg1==arg2==arg3==arg4))
        return BOOL_FALSE;
    if ((arg1<0) || (arg1>255) ||
        (arg2<0) || (arg2>255) ||
        (arg3<0) || (arg3>255) ||
        (arg4<0) || (arg4>255))
        return BOOL_FALSE;

    return BOOL_TRUE;
}

INT32 main()
{

#define MAX_CMD_LINE_LENGTH 48
#define MAX_CMD_LENGTH 16
    CHAR cmdline[MAX_CMD_LINE_LENGTH] = {0};
    CHAR cmd[MAX_CMD_LENGTH] = {0};
    CHAR arg1[MAX_CMD_LENGTH] = {0};
    CHAR arg2[MAX_CMD_LENGTH] = {0};
    INT32 validargs = 0;

    INT32 i = 0;
    INT32 numofcmds = sizeof( cmdloop_cmdlist ) / sizeof( cmdloop_commands );
    INT32 cmdnum = -1;
    INT32 cmdfound = 0;
    INT32 invalidargs = 0;


    /* init sock_for_net */
    sock_for_net = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock_for_net < 0)
    {
        fprintf(stderr,"Fail to create sock_for_net\n");
        return -1;
    }

    /*create thread to receive packet and write into file if necessary*/
    if (pthread_create(&LogTid, NULL, _log_task, NULL)) {
        fprintf(stderr, "ERROR: fail to create thread\n");
        return -1;
    }
    pthread_detach(LogTid);

    _help();

    /* parse user input and send cmd to router if necessary */
    while (BOOL_TRUE != mainQuit) {
        cmdfound = 0;
        invalidargs = 0;

        /* parse user input */
        fgets(cmdline, sizeof(cmdline), stdin);
        validargs = sscanf( cmdline, "%16s %16s %16s", cmd, arg1, arg2 );
        for ( i = 0; i < numofcmds; i++ ) {
        if (0==STRCMP(cmd, cmdloop_cmdlist[i].str)) { // case-insensitive comparison
                cmdnum = cmdloop_cmdlist[i].cmdnum;
                cmdfound++;
                if( validargs != cmdloop_cmdlist[i].numargs )
                    invalidargs++;
                break;
            }
        }
        if ( !cmdfound ) {
            fprintf(stderr, "Command not found; try 'help'\n" );
            continue;
        }
        if ( invalidargs ) {
            fprintf(stderr, "Invalid arguments; try 'help'\n" );
            continue;
        }

        /* send cmd to router */
        switch ( cmdnum ) {
            case SETROUTERIP:
                if (BOOL_TRUE==_check_ip_validity(arg1)) {
                    strncpy(router_ip,arg1,MAX_IP_ADDR_LENGTH);

                    memset(&sock_net_addr_send_to,0,sizeof(sock_net_addr_send_to));
                    sock_net_addr_send_to.sin_family = PF_INET;
                    sock_net_addr_send_to.sin_addr.s_addr = inet_addr(router_ip);
                    sock_net_addr_send_to.sin_port = htons(LOG_ROUTER_TRANSMITTER_UDP_PORT);

                    RouterIpConfiged = BOOL_TRUE; // set flag when effect IP is set
                } else
                    fprintf(stderr, "seems not valid IP, ignore it\n");
                break;

            case SETTXFLAG:
                _send_cmd_tx_flag_to_router(atoi(arg1));
                break;

            case GETSTATUS:
                _send_cmd_get_status_to_router();
                break;

            case SETLEVEL:
                _send_cmd_set_module_level_to_router(atoi(arg1), atoi(arg2));
                break;

            case SHOWMODULEINFO:
                _send_cmd_show_module_info_to_router(atoi(arg1),arg2);
                break;

            case GETLASTBUFFER:
                _send_cmd_get_last_buffer_to_router();
                break;

            case HELP1:
            case HELP2:
                _help();
                break;

            case QUIT1:
            case QUIT2:
                mainQuit = BOOL_TRUE;
                threadQuit = BOOL_TRUE;
                _send_cmd_tx_flag_to_router(LOG_CMD_REQ_TX_DISABLE); // notify log transmitter
                break;

            default:
                fprintf(stderr, "Command not supported, see \"help\"\n" );
                break;
        }

    } 
    _close_sock();

    SLEEP(WAITINTVL); // for graceful quit
    return 0;
}

