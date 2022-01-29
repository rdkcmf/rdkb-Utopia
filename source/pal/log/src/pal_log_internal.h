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

/*****************************************************************************
 * FileName    :  pal_log.h
 * Author      :  tao hong(tahong@cisco.com)
 * Date        :  2008-11-11
 * Description :  PAL LOG Module
 *****************************************************************************/
/*
*$Id: pal_log_internal.h,v 1.1 2009/05/13 08:59:43 tahong Exp $
*
*$Log: pal_log_internal.h,v $
*Revision 1.1  2009/05/13 08:59:43  tahong
*create orignal version
*
*Revision 1.3  2008/12/10 05:50:08  tahong
*add cvs header
*
*Revision 1.2  2008/09/24 07:04:47  zhihliu
*merge back to main trunk
*
*/

#ifndef LOG_INTERNAL_H
#define LOG_INTERNAL_H

#include "pal_def.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/un.h>
#include <unistd.h>

#include "pal_log.h"

#define IN 
#define OUT

/***************************************************************************
*                                                   for buffer                                                                 *
****************************************************************************/
/*ETHERNET_MTU_SIZE             1518
ETHERNET_PACKET_HEAD_SIZE 18
IP_PACKET_HEAD_SIZE             20
UDP_PACKET_HEAD_SIZE          8*/
#define MAX_BUF_SIZE (1518-18-20-8)
#define LOG_DATA_TRANSMITTER_2_CENTER_FIX_CONTENT_LENGTH (8+1+7+1+6+2)

/***************************************************************************
*                                                   for module/level                                                       *
****************************************************************************/
#define MAX_MODULE_NUMBER 127
#define MAX_LEVEL_NUMBER 5
#define MAX_NAME_LENGTH 20

#define PAL_LOG_LEVEL_IGNORE_ALL 0
extern CHAR LOG_LEVEL_NAME[MAX_LEVEL_NUMBER][MAX_NAME_LENGTH];

/***************************************************************************
*                                                   for cmd                                                                    *
****************************************************************************/
typedef enum log_cmd_s
{
/*between center and router transmitter*/
    LOG_CMD_REQ_TX =0x01,
    LOG_CMD_REQ_GET_STATUS,
    LOG_CMD_REQ_SET_MODULE_LEVEL,
    LOG_CMD_REQ_GET_LAST_BUFFER,

    LOG_CMD_RSP_TX =0x11,
    LOG_CMD_RSP_GET_STATUS,
    LOG_CMD_RSP_SET_MODULE_LEVEL,
    LOG_CMD_RSP_GET_LAST_BUFFER,
/*center==>router transmitter==>router app*/
    LOG_CMD_REQ_SHOW_MODULE_DEBUG_INFO = 0x21,

//    LOG_CMD_RSQ_SHOW_MODULE_DEBUG_INFO = 0x31,
/*between router transmitter and router app*/
    LOG_CMD_REQ_REGISTER_NEW_MODULE = 0x41,

//    LOG_CMD_RSQ_REGISTER_NEW_MODULE = 0x51,
/*router app==>router transmitter==>center*/
    LOG_DATA = 0x61,
}log_cmd_t;

typedef struct pkt_head_req_tx_s
{
    char log_cmd;
    char tx_value;
}pkt_head_req_tx_t;

typedef struct pkt_head_req_get_status_s
{
    char log_cmd;
}pkt_head_req_get_status_t;

typedef struct pkt_head_req_set_module_level_s
{
    char log_cmd;
    char module;
    char level;
}pkt_head_req_set_module_level_t;

typedef struct pkt_head_req_get_last_buffer_s
{
    char log_cmd;
}pkt_head_req_get_last_buffer_t;

typedef struct pkt_head_rsp_tx_s
{
    char log_cmd;
    char rv_value;
    char input_tx_value;
}pkt_head_rsp_tx_t;

typedef struct pkt_head_rsp_get_status_s
{
    char log_cmd;
}pkt_head_rsp_get_status_t;

typedef struct pkt_head_rsp_set_module_level_s
{
    char log_cmd;
    char rv_value;
    char module;
    char level;
}pkt_head_rsp_set_module_level_t;

typedef struct pkt_head_rsp_get_last_buffer_s
{
    char log_cmd;
}pkt_head_rsp_get_last_buffer_t;

typedef struct pkt_head_req_show_module_debug_info_s
{
    char log_cmd;
    char module;
}pkt_head_req_show_module_debug_info_t;

typedef struct pkt_head_req_register_new_module_s
{
    char log_cmd;
    char module;
}pkt_head_req_register_new_module_t;

typedef struct log_data_app_2_transmitter_s
{
    char log_cmd;
    char module;
    char level;
}log_data_app_2_transmitter_t;

typedef struct log_data_transmitter_2_center_s
{
    char log_cmd;
}log_data_transmitter_2_center_t;

#define LOG_CMD_REQ_TX_ENABLE 1
#define LOG_CMD_REQ_TX_DISABLE 0

#define LOG_CMD_RSQ_OK 1
#define LOG_CMD_RSQ_ERROR 0

/***************************************************************************
*                                                   for socket                                                                 *
****************************************************************************/
#define LOG_ROUTER_TRANSMITTER_UDP_PORT 10000

#define LOG_ROUTER_TRANSMITTER_FILE_NAME "/tmp/log_transmitter"
#define LOG_ROUTER_APP_FILE_NAME "/tmp/log_app"
#define MAX_MODULE_APP_SUN_PATH_LENGTH 108

typedef VOID (*packet_handle_func_t)(IN CHAR *buf, IN INT32 buf_len, IN struct sockaddr *sock_addr,IN INT32 sock_len);
typedef struct socket_info_s{
    INT32 socket_fd;
    INT32 socket_type;/*PF_INET for IPV4 Internet protocols, PF_LOCAL for Local communication*/
    packet_handle_func_t packet_handle_func;
}socket_info_t;

extern INT32 PAL_log_init_local_unix_udp_socket(IN CHAR* socket_addr_path_this_end,
                                                OUT INT32* unix_socket);


extern INT32 PAL_log_init_local_unix_udp_socket_to_addr(IN CHAR *socket_addr_path_other_end,
                                                          OUT struct sockaddr_un *socket_addr_other_end);


extern INT32 PAL_log_init_inet_udp_socket(IN UINT16 port,
                                         OUT INT32* udp_socket);


extern INT32 PAL_log_init_inet_udp_socket_to_addr(IN ULONG socket_ip_addr_other_end,/*network order*/
                                                   IN UINT16 port,
                                                   OUT struct sockaddr_in *socket_addr_other_end);


extern VOID PAL_log_recv_and_process_packets(IN socket_info_t *socket_info_array, IN INT32 socket_count);


#endif/*#ifndef LOG_INTERNAL_H*/

