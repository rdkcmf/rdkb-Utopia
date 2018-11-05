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
 * FileName    :  log_router_for_app.c
 * Author      :  tao hong(tahong@cisco.com)
 * Date        :  2008-11-11
 * Description :  PAL LOG Module
 *****************************************************************************/
/*
*$Id: pal_log_router_for_app.c,v 1.2 2009/05/14 02:58:08 tahong Exp $
*
*$Log: pal_log_router_for_app.c,v $
*Revision 1.2  2009/05/14 02:58:08  tahong
*add: #include "pal_log_internal.h"
*
*Revision 1.1  2009/05/13 08:59:43  tahong
*create orignal version
*
*Revision 1.3  2008/12/10 05:50:13  tahong
*add cvs header
*
*Revision 1.2  2008/09/24 07:04:47  zhihliu
*merge back to main trunk
*
*/

#include <stdarg.h>
#include "pal_log.h"
#include "pal_log_internal.h"

LOCAL pthread_mutex_t register_mutex = PTHREAD_MUTEX_INITIALIZER;
LOCAL INT32 log_router_for_app_inited = 0;
LOCAL INT32 sock_for_app;
LOCAL struct sockaddr_un sock_app_transmitter_addr;

typedef struct module_info_in_router_app_s
{
    CHAR module_name[MAX_NAME_LENGTH];
    pal_log_show_module_debug_info_callback_t log_show_module_debug_info_callback_func;
}module_info_in_router_app_t;
LOCAL module_info_in_router_app_t module_info_in_router_app_map[MAX_MODULE_NUMBER];
LOCAL INT32 max_active_local_module_number = 0;

LOCAL VOID _send_buf_to_router_transmitter(IN CHAR* buf, IN INT32 buf_len)
{
    if ( (NULL == buf) 
         || (buf_len < 0)
         || (buf_len > MAX_BUF_SIZE) )
         return;

    sendto(sock_for_app,buf,buf_len,0,(struct sockaddr *)&sock_app_transmitter_addr,sizeof(sock_app_transmitter_addr));
}

LOCAL VOID _log_router_app_packet_handle(IN CHAR *buf, IN INT32 buf_len, IN struct sockaddr *sock_addr, IN INT32 sock_len)
{
    if ( (NULL == buf) 
         || (buf_len < 2)
         || (buf_len > MAX_BUF_SIZE))
         return;

    switch(buf[0])
    {
        case LOG_CMD_REQ_SHOW_MODULE_DEBUG_INFO:
            /*call show_module_debug_info callback function*/
            if (NULL != module_info_in_router_app_map[((INT32)buf[1])].log_show_module_debug_info_callback_func)
                module_info_in_router_app_map[((INT32)buf[1])].log_show_module_debug_info_callback_func(buf+2);
            break;
        default:
            break;
    }
}

LOCAL VOID *_log_app_recv_task(IN VOID* args)
{
#define LOG_APP_SOCKET_NUM 1
    socket_info_t router_app_socket[LOG_APP_SOCKET_NUM];

    router_app_socket[0].socket_fd = sock_for_app;
    router_app_socket[0].socket_type = PF_LOCAL;
    router_app_socket[0].packet_handle_func = _log_router_app_packet_handle;

    PAL_log_recv_and_process_packets(router_app_socket,LOG_APP_SOCKET_NUM);
    return NULL;
}

INT32 PAL_log_register(IN CHAR * module_name, IN pal_log_show_module_debug_info_callback_t log_show_module_debug_info_func)
{
    pthread_t log_app_recv_tid;

    CHAR buf[MAX_BUF_SIZE];
    INT32 i;

    if ( (NULL == module_name)
          ||(strlen(module_name)+1 > MAX_NAME_LENGTH) )/*1 is the last '\0'*/
        return -1;
    
    pthread_mutex_lock(&register_mutex);

    if (0 == log_router_for_app_inited)
    {
        log_router_for_app_inited = 1;
        snprintf(buf,MAX_MODULE_APP_SUN_PATH_LENGTH,"%s%d",LOG_ROUTER_APP_FILE_NAME,getpid());
        if (PAL_log_init_local_unix_udp_socket(buf, &sock_for_app) != 0)
        {
            pthread_mutex_unlock(&register_mutex);
            return -1;
        }

        if (PAL_log_init_local_unix_udp_socket_to_addr(LOG_ROUTER_TRANSMITTER_FILE_NAME, &sock_app_transmitter_addr) != 0)
        {
            close(sock_for_app);
            pthread_mutex_unlock(&register_mutex);
            return -1;
        }
        memset(module_info_in_router_app_map,0,sizeof(module_info_in_router_app_map));


        /*create thread to receive packet and write into file if necessary*/
        if (pthread_create(&log_app_recv_tid, NULL, _log_app_recv_task, NULL))
        {
            fprintf(stderr, "ERROR: Failed to create thread _log_app_recv_task\n");
            close(sock_for_app);
            pthread_mutex_unlock(&register_mutex);
            return -1;
        }
        pthread_detach(log_app_recv_tid);
    }

    for(i=0;i<max_active_local_module_number;i++)/*search if already exist*/
    {
        if(strcmp(module_info_in_router_app_map[i].module_name,module_name) == 0)
        {
            break;
        }
    }
    if(max_active_local_module_number == i)/*no match*/
    {
        if (MAX_MODULE_NUMBER-1 == max_active_local_module_number)/*module count check*/
        {
            fprintf(stderr,"\n\n\nMODULE number over MAX!!!new module registering failed!!!\n\n\n");
            return -1;        
        }
        max_active_local_module_number++;
    }

    snprintf(module_info_in_router_app_map[i].module_name,MAX_NAME_LENGTH,module_name);
    module_info_in_router_app_map[i].log_show_module_debug_info_callback_func = log_show_module_debug_info_func;    
    buf[0] = LOG_CMD_REQ_REGISTER_NEW_MODULE;
    buf[1] = i;/*local module id*/
    snprintf(buf+2,MAX_BUF_SIZE-2,module_name);
    _send_buf_to_router_transmitter(buf,2+strlen(module_name)+1);

    pthread_mutex_unlock(&register_mutex);

    return 0;
}


VOID PAL_log (IN CHAR* module_name, IN UINT32 level_number, IN CHAR *file, IN UINT32 line, IN const CHAR *fmt, ... )
{
    va_list args = NULL;
    CHAR buf[MAX_BUF_SIZE];
    CHAR *buf_index = buf;
    INT32 remain_space = 0;
    INT32 i;

    if ( (NULL == module_name)
          ||(strlen(module_name)+1 > MAX_NAME_LENGTH)
          ||(level_number <= PAL_LOG_LEVEL_IGNORE_ALL)
          ||(level_number > PAL_LOG_LEVEL_DEBUG) )
    return;

    /*module name==>local module number*/
    for(i=0;i<max_active_local_module_number;i++)
    {
        if(strncmp(module_info_in_router_app_map[i].module_name,module_name,strlen(module_name)) == 0)
        {
            break;
        }
    }
    if (max_active_local_module_number == i)
    {
        return;
    }

    buf[0] = LOG_DATA;
    buf[1] = i;/*local module number*/
    buf[2] = level_number;
    buf_index += 3;
    remain_space = MAX_BUF_SIZE - (buf_index-buf);
    remain_space = (remain_space>0)?remain_space:0;

    snprintf(buf_index, remain_space,"[%s:%s]", module_name, LOG_LEVEL_NAME[level_number]);/*send module name and level name*/
    buf_index += strlen(buf_index);/*buf_index point to the last '\0' */
    remain_space = MAX_BUF_SIZE - (buf_index-buf);
    remain_space = (remain_space>0)?remain_space:0;

    snprintf(buf_index, remain_space,"{%s:%d} ", file, line);/*send __FILE__ and __LINE__ out*/
    buf_index += strlen(buf_index);
    remain_space = MAX_BUF_SIZE - (buf_index-buf);
    remain_space = (remain_space>0)?remain_space:0;

    va_start (args, fmt);
    vsnprintf(buf_index, remain_space, fmt, args);/*send user content out*/
    va_end (args);
    buf_index += strlen(buf_index);
    remain_space = MAX_BUF_SIZE - (buf_index-buf);
    remain_space = (remain_space>0)?remain_space:0;

    snprintf(buf_index, remain_space,"\n");
    buf_index += strlen(buf_index);/*buf_index point to the last '\0' */

#ifdef LOG_DEBUG
    fprintf(stderr, "%s\n", buf+3);
#endif/*#ifdef LOG_DEBUG*/

    _send_buf_to_router_transmitter(buf,(buf_index-buf));/*last '\0' is not transferred*/

    return;
}

