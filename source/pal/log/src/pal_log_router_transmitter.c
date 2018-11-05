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
 * FileName    :  log_router_transmitter.c
 * Author      :  tao hong(tahong@cisco.com)
 * Date        :  2008-11-11
 * Description :  PAL LOG Module
 *****************************************************************************/
/*
*$Id: pal_log_router_transmitter.c,v 1.3 2009/05/27 03:06:45 tahong Exp $
*
*$Log: pal_log_router_transmitter.c,v $
*Revision 1.3  2009/05/27 03:06:45  tahong
*change "log_router_transmitter" to "pal_log_router_transmitter"
*
*Revision 1.2  2009/05/14 02:58:14  tahong
*add: #include "pal_log_internal.h"
*
*Revision 1.1  2009/05/13 08:59:43  tahong
*create orignal version
*
*Revision 1.4  2008/12/10 05:50:18  tahong
*add cvs header
*
*Revision 1.3  2008/12/08 05:50:33  tahong
*because the place where running log_router_transmitter is changed from rc/serivice.c to hnap/hnap_eghn_ext.c
*so it's possible to kill httpd process and run httpd again
*so we need to make sure not to run log_router_transmitter again
*
*Revision 1.2  2008/09/24 07:04:47  zhihliu
*merge back to main trunk
*
*/

#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

#include "pal_log.h"
#include "pal_log_internal.h"


LOCAL INT32 sock_for_net;
LOCAL INT32 sock_for_app;

typedef struct module_info_in_router_transmitter_s
{
    CHAR level;
    CHAR module_app_addr[MAX_MODULE_APP_SUN_PATH_LENGTH];
    CHAR module_id;
    CHAR module_name[MAX_NAME_LENGTH];
}module_info_in_router_transmitter_t;
LOCAL module_info_in_router_transmitter_t module_info_in_router_transmitter_map[MAX_MODULE_NUMBER];
LOCAL INT32 active_module_count = 0;

LOCAL INT32 send_packet_to_center_flag = 0;
LOCAL struct sockaddr_in sock_net_addr_center;

LOCAL CHAR stored_buffer[MAX_BUF_SIZE] = {0};/*'\0' is not stored in buffer*/
LOCAL INT32 flag_stored_buffer_length_over_max = 0;
LOCAL INT32 index_tail = 0;/*(end of buffer)+1; (index_tail-index_head) is buffer length*/

LOCAL VOID _log_router_transmitter_packet_handle(IN CHAR *buf, IN INT32 buf_len, IN struct sockaddr *sock_addr,IN INT32 sock_len)
{
    CHAR out_buf[MAX_BUF_SIZE];
    CHAR *temp_index;
    INT32 i = 0;
    struct sockaddr_un un_addr;
    LOCAL INT32 global_log_msg_num = 0;
    LOCAL struct timeval start_time, current_time;
    INT32 remain_space = 0;

    if ( (NULL == buf) 
         || (buf_len < 1)        || (buf_len > MAX_BUF_SIZE)
         || (NULL == sock_addr ) || (sock_len <0) )
    {
        fprintf(stderr, "ERROR: parameter error in %s:%d\n",__FILE__,__LINE__);
        return;
    }

    switch(buf[0])
    {
        /*from center to router transmitter*/
        case LOG_CMD_REQ_TX:
#undef P_IN_PKT_HEAD
#undef P_OUT_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#undef OUT_PKT_HEAD_LENGTH
#define P_IN_PKT_HEAD           ((pkt_head_req_tx_t*)buf)
#define P_OUT_PKT_HEAD          ((pkt_head_rsp_tx_t*)out_buf)
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_tx_t))
#define OUT_PKT_HEAD_LENGTH     (sizeof(pkt_head_rsp_tx_t))

            if (buf_len != IN_PKT_HEAD_LENGTH)
            {
                fprintf(stderr, "ERROR: parameter error in %s:%d\n",__FILE__,__LINE__);
                return;
            }

            P_OUT_PKT_HEAD->log_cmd = LOG_CMD_RSP_TX;
            if (LOG_CMD_REQ_TX_ENABLE == P_IN_PKT_HEAD->tx_value )
            {
                PAL_log_init_inet_udp_socket_to_addr(((struct sockaddr_in*)sock_addr)->sin_addr.s_addr, ((struct sockaddr_in*)sock_addr)->sin_port, &sock_net_addr_center);
                send_packet_to_center_flag = 1;
                P_OUT_PKT_HEAD->rv_value = LOG_CMD_RSQ_OK;
                sendto(sock_for_net,out_buf,2,0,(struct sockaddr *)sock_addr,sock_len);
            }
            else if (LOG_CMD_REQ_TX_DISABLE == P_IN_PKT_HEAD->tx_value)
            {
                send_packet_to_center_flag = 0;
                P_OUT_PKT_HEAD->rv_value = LOG_CMD_RSQ_OK;
                sendto(sock_for_net,out_buf,2,0,(struct sockaddr *)sock_addr,sock_len);
            }
            else
            {
                P_OUT_PKT_HEAD->rv_value = LOG_CMD_RSQ_ERROR;
                P_OUT_PKT_HEAD->input_tx_value = P_IN_PKT_HEAD->tx_value;
                sendto(sock_for_net,out_buf,OUT_PKT_HEAD_LENGTH,0,(struct sockaddr *)sock_addr,sock_len);
            }

            break;

        case LOG_CMD_REQ_GET_STATUS:
#undef P_IN_PKT_HEAD
#undef P_OUT_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#undef OUT_PKT_HEAD_LENGTH
//#define P_IN_PKT_HEAD                 ((pkt_head_req_get_status_t*)buf)
#define P_OUT_PKT_HEAD          ((pkt_head_rsp_get_status_t*)out_buf)
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_get_status_t))
#define OUT_PKT_HEAD_LENGTH     (sizeof(pkt_head_rsp_get_status_t))

            if (buf_len != IN_PKT_HEAD_LENGTH)
            {
                fprintf(stderr, "ERROR: parameter error in %s:%d\n",__FILE__,__LINE__);
                return;
            }

            P_OUT_PKT_HEAD->log_cmd = LOG_CMD_RSP_GET_STATUS;
            temp_index = out_buf+OUT_PKT_HEAD_LENGTH;
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            /*tx status*/
            snprintf(temp_index,remain_space,"******************************************************************************\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            snprintf(temp_index,remain_space,"Current NETWORK PACKET TX status: %s\n",(send_packet_to_center_flag==1)?"ENABLE":"DISABLE");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;
            
            /*module name/Module num/current level*/
            snprintf(temp_index,remain_space,"******************************************************************************\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            snprintf(temp_index,remain_space,"ModuleName\tModuleNumber\tCurrentLogLevel\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            snprintf(temp_index,remain_space,"---------------------------------------------------------\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            for(i=0;i<active_module_count;i++)
            {
                snprintf(temp_index,remain_space,"%s\t\t%d\t\t%s(%d)\n",module_info_in_router_transmitter_map[i].module_name,
                                                          i,
                                                          LOG_LEVEL_NAME[(INT32)(module_info_in_router_transmitter_map[i].level)],
                                                          module_info_in_router_transmitter_map[i].level);
                temp_index+=strlen(temp_index);
                remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
                remain_space = (remain_space>0)?remain_space:0;                
            }

            snprintf(temp_index,remain_space,"%s\t%d\t\t%s\n","ALL MODULES", MAX_MODULE_NUMBER, "***");
            temp_index+=strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            /*level num/level name*/
            snprintf(temp_index,remain_space,"******************************************************************************\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            snprintf(temp_index,remain_space,"Level Number\tLevel Name\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            snprintf(temp_index,remain_space,"--------------------------------\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            for(i=0;i<MAX_LEVEL_NUMBER;i++)
            {
                snprintf(temp_index,remain_space,"\t%d\t%s\n",i,LOG_LEVEL_NAME[i]);
                temp_index+=strlen(temp_index);
                remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
                remain_space = (remain_space>0)?remain_space:0;
            }
            snprintf(temp_index,remain_space,"******************************************************************************\n\n\n");
            temp_index += strlen(temp_index);
            remain_space = MAX_BUF_SIZE - (temp_index-out_buf);
            remain_space = (remain_space>0)?remain_space:0;

            sendto(sock_for_net,out_buf,OUT_PKT_HEAD_LENGTH+strlen(out_buf+OUT_PKT_HEAD_LENGTH),0,(struct sockaddr *)sock_addr,sock_len);
            break;

        case LOG_CMD_REQ_SET_MODULE_LEVEL:
#undef P_IN_PKT_HEAD
#undef P_OUT_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#undef OUT_PKT_HEAD_LENGTH
#define P_IN_PKT_HEAD           ((pkt_head_req_set_module_level_t*)buf)
#define P_OUT_PKT_HEAD          ((pkt_head_rsp_set_module_level_t*)out_buf)
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_set_module_level_t))
#define OUT_PKT_HEAD_LENGTH     (sizeof(pkt_head_rsp_set_module_level_t))
            
            if (buf_len != IN_PKT_HEAD_LENGTH)
            {
                fprintf(stderr, "ERROR: parameter error in %s:%d\n",__FILE__,__LINE__);
                return;
            }

            P_OUT_PKT_HEAD->log_cmd = LOG_CMD_RSP_SET_MODULE_LEVEL;
            
            if ( (P_IN_PKT_HEAD->level < PAL_LOG_LEVEL_IGNORE_ALL)
                ||(P_IN_PKT_HEAD->level > PAL_LOG_LEVEL_DEBUG) )/*level num range check*/
            {
                P_OUT_PKT_HEAD->rv_value = LOG_CMD_RSQ_ERROR;
                P_OUT_PKT_HEAD->module = P_IN_PKT_HEAD->module;
                P_OUT_PKT_HEAD->level = P_IN_PKT_HEAD->level;
                sendto(sock_for_net,out_buf,OUT_PKT_HEAD_LENGTH,0,(struct sockaddr *)sock_addr,sock_len);                
            }
            else if (MAX_MODULE_NUMBER  == P_IN_PKT_HEAD->module)/*module num == ALL MODULE*/
            {
                for (i=0;i<active_module_count;i++)
                {
                    module_info_in_router_transmitter_map[i].level = P_IN_PKT_HEAD->level;
                }
                P_OUT_PKT_HEAD->rv_value = LOG_CMD_RSQ_OK;
                sendto(sock_for_net,out_buf,2,0,(struct sockaddr *)sock_addr,sock_len);
            }
            else if ( (P_IN_PKT_HEAD->module < 0)
                       ||(P_IN_PKT_HEAD->module > active_module_count-1) )/*module num range check*/
            {                
                P_OUT_PKT_HEAD->rv_value = LOG_CMD_RSQ_ERROR;
                P_OUT_PKT_HEAD->module = P_IN_PKT_HEAD->module;
                P_OUT_PKT_HEAD->level = P_IN_PKT_HEAD->level;
                sendto(sock_for_net,out_buf,OUT_PKT_HEAD_LENGTH,0,(struct sockaddr *)sock_addr,sock_len);
            }
            else
            {
                module_info_in_router_transmitter_map[(INT32)(P_IN_PKT_HEAD->module)].level = P_IN_PKT_HEAD->level;
                P_OUT_PKT_HEAD->rv_value = LOG_CMD_RSQ_OK;
                sendto(sock_for_net,out_buf,2,0,(struct sockaddr *)sock_addr,sock_len);
            }
            break;

        case LOG_CMD_REQ_GET_LAST_BUFFER:
#undef P_IN_PKT_HEAD
#undef P_OUT_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#undef OUT_PKT_HEAD_LENGTH
#define P_IN_PKT_HEAD           ((pkt_head_req_get_last_buffer_t*)buf)
#define P_OUT_PKT_HEAD          ((pkt_head_rsp_get_last_buffer_t*)out_buf)
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_get_last_buffer_t))
#define OUT_PKT_HEAD_LENGTH     (sizeof(pkt_head_rsp_get_last_buffer_t))

            if (buf_len != IN_PKT_HEAD_LENGTH)
            {
                fprintf(stderr, "ERROR: parameter error in %s:%d\n",__FILE__,__LINE__);
                return;
            }

            P_OUT_PKT_HEAD->log_cmd = LOG_CMD_RSP_GET_LAST_BUFFER;

            if ( 0 == send_packet_to_center_flag)/*tx status check*/
            {
                snprintf(out_buf+OUT_PKT_HEAD_LENGTH, MAX_BUF_SIZE-OUT_PKT_HEAD_LENGTH, "TX is still DISABLE, pls ENABLE it first.\n");
                return;
            }

            if (0 == flag_stored_buffer_length_over_max)/*buffer is never over max*/
            {
                if (index_tail == 0)/*buffer is empty*/
                {
                    snprintf(out_buf+OUT_PKT_HEAD_LENGTH, MAX_BUF_SIZE-OUT_PKT_HEAD_LENGTH, "no buffer");
                }
                else if (index_tail<=MAX_BUF_SIZE-OUT_PKT_HEAD_LENGTH-1)/*1 is the last '\0'*//*buffer length is less than MAX length - 2*/
                {
                    snprintf(out_buf+OUT_PKT_HEAD_LENGTH, index_tail, "%s", stored_buffer);
                }
                else/*buffer length is more than MAX length - 2, only copy the last MAX length - 2 chars*/
                {
                    snprintf(out_buf+OUT_PKT_HEAD_LENGTH, index_tail-OUT_PKT_HEAD_LENGTH, "%s", stored_buffer+(OUT_PKT_HEAD_LENGTH+1));
                }
            }
            else/*buffer starts from begining again */
            {
                snprintf(out_buf+OUT_PKT_HEAD_LENGTH, MAX_BUF_SIZE-index_tail-OUT_PKT_HEAD_LENGTH, "%s", stored_buffer+index_tail+(OUT_PKT_HEAD_LENGTH+1));
                snprintf(out_buf+OUT_PKT_HEAD_LENGTH+MAX_BUF_SIZE-index_tail-(OUT_PKT_HEAD_LENGTH+1), index_tail+1, "%s", stored_buffer);
            }

            sendto(sock_for_net,out_buf,OUT_PKT_HEAD_LENGTH+strlen(out_buf+OUT_PKT_HEAD_LENGTH),0,(struct sockaddr *)&sock_net_addr_center,sizeof(sock_net_addr_center));/*last '\0' is not transferred*/
            break;

        /*center==>router transmitter==>router app*/
        case LOG_CMD_REQ_SHOW_MODULE_DEBUG_INFO:
#undef P_IN_PKT_HEAD
#undef P_OUT_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#undef OUT_PKT_HEAD_LENGTH
#define P_IN_PKT_HEAD           ((pkt_head_req_show_module_debug_info_t*)buf)
#define P_OUT_PKT_HEAD          ((pkt_head_req_show_module_debug_info_t*)out_buf)
#define IN_PKT_HEAD_LENGTH     (sizeof(pkt_head_req_show_module_debug_info_t))
#define OUT_PKT_HEAD_LENGTH     (sizeof(pkt_head_req_show_module_debug_info_t))
            
            if ( (buf_len < IN_PKT_HEAD_LENGTH)
                  ||(P_IN_PKT_HEAD->module < 0)
                  ||(P_IN_PKT_HEAD->module > active_module_count-1) )
            {
                fprintf(stderr, "ERROR: parameter error in %s:%d\n",__FILE__,__LINE__);
                return;
            }

            P_OUT_PKT_HEAD->log_cmd = LOG_CMD_REQ_SHOW_MODULE_DEBUG_INFO;
            P_OUT_PKT_HEAD->module = module_info_in_router_transmitter_map[(INT32)(P_IN_PKT_HEAD->module)].module_id;
            snprintf(out_buf+OUT_PKT_HEAD_LENGTH,MAX_BUF_SIZE-OUT_PKT_HEAD_LENGTH,"%s",buf+IN_PKT_HEAD_LENGTH);
            PAL_log_init_local_unix_udp_socket_to_addr(module_info_in_router_transmitter_map[(INT32)(P_IN_PKT_HEAD->module)].module_app_addr, &un_addr);

            sendto(sock_for_app,out_buf,OUT_PKT_HEAD_LENGTH+strlen(out_buf+OUT_PKT_HEAD_LENGTH),0,(struct sockaddr *)&un_addr,sizeof(un_addr));
            break;

        case LOG_CMD_REQ_REGISTER_NEW_MODULE:
#undef P_IN_PKT_HEAD
#undef P_OUT_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#undef OUT_PKT_HEAD_LENGTH
#define P_IN_PKT_HEAD           ((pkt_head_req_register_new_module_t*)buf)
#define IN_PKT_HEAD_LENGTH      (sizeof(pkt_head_req_register_new_module_t))

            if ( (buf_len < IN_PKT_HEAD_LENGTH)
                  ||(P_IN_PKT_HEAD->module < 0)
                  ||(P_IN_PKT_HEAD->module > MAX_MODULE_NUMBER-1) )
            {
                fprintf(stderr, "ERROR: parameter error in %s:%d\n",__FILE__,__LINE__);
                return;
            }

            if (0 == active_module_count)
            {
                i = 0;
                active_module_count++;
            }
            else
            {
                for(i=0;i<active_module_count;i++)/*search if already exist; if exist, overwrite it*/
                {
                    if(strcmp(module_info_in_router_transmitter_map[i].module_name,buf+IN_PKT_HEAD_LENGTH) == 0)
                    {
                        break;
                    }
                }
                if(active_module_count == i)/*no match*/
                {
                    if (MAX_MODULE_NUMBER-1 == active_module_count)/*module count check*/
                    {
                        fprintf(stderr,"\n\n\nMODULE number over MAX!!!new module registering failed!!!\n\n\n");
                        return;
                    }
                    active_module_count++;
                }
            }

            module_info_in_router_transmitter_map[i].level = PAL_LOG_LEVEL_WARNING;
            snprintf(module_info_in_router_transmitter_map[i].module_app_addr,MAX_MODULE_APP_SUN_PATH_LENGTH,((struct sockaddr_un*)sock_addr)->sun_path);;
            module_info_in_router_transmitter_map[i].module_id = P_IN_PKT_HEAD->module;
            snprintf(module_info_in_router_transmitter_map[i].module_name,MAX_BUF_SIZE-IN_PKT_HEAD_LENGTH,buf+IN_PKT_HEAD_LENGTH);
            break;

        case LOG_DATA:
#undef P_IN_PKT_HEAD
#undef P_OUT_PKT_HEAD
#undef IN_PKT_HEAD_LENGTH
#undef OUT_PKT_HEAD_LENGTH
#define P_IN_PKT_HEAD           ((log_data_app_2_transmitter_t*)buf)
#define P_OUT_PKT_HEAD          ((log_data_transmitter_2_center_t*)out_buf)
#define IN_PKT_HEAD_LENGTH     (sizeof(log_data_app_2_transmitter_t))
#define OUT_PKT_HEAD_LENGTH     (sizeof(log_data_transmitter_2_center_t))
            /*app addr+ id==>level*/
            for(i=0;i<active_module_count;i++)/*search if module exist*/
            {
                if( (strcmp(module_info_in_router_transmitter_map[i].module_app_addr,((struct sockaddr_un*)sock_addr)->sun_path) == 0)
                    &&(module_info_in_router_transmitter_map[i].module_id == P_IN_PKT_HEAD->module) )
                {
                    break;
                }
            }
            if(active_module_count == i)/*no match*/
            {
                break;
            }

            if(P_IN_PKT_HEAD->level > module_info_in_router_transmitter_map[i].level)/*PACKET level > set level, then ignore this PACKET*/
                break;

            global_log_msg_num++;/*PACKET index num*/

            if (1 == global_log_msg_num)
            {
                gettimeofday(&start_time,NULL);
            }
            gettimeofday(&current_time,NULL);/*PACKET time mark*/
            
            P_OUT_PKT_HEAD->log_cmd = LOG_DATA;
            snprintf(out_buf+OUT_PKT_HEAD_LENGTH,
                     LOG_DATA_TRANSMITTER_2_CENTER_FIX_CONTENT_LENGTH+1,/*1 is the last '\0'*/
                     "%08d %07ld.%06lds ",
                     global_log_msg_num,
                     (current_time.tv_usec>start_time.tv_usec)?(current_time.tv_sec-start_time.tv_sec):(current_time.tv_sec-start_time.tv_sec-1),
                     (current_time.tv_usec>start_time.tv_usec)?(current_time.tv_usec-start_time.tv_usec):(1000*1000+current_time.tv_usec-start_time.tv_usec) );
            snprintf(out_buf+OUT_PKT_HEAD_LENGTH+LOG_DATA_TRANSMITTER_2_CENTER_FIX_CONTENT_LENGTH,
                     MAX_BUF_SIZE-OUT_PKT_HEAD_LENGTH-LOG_DATA_TRANSMITTER_2_CENTER_FIX_CONTENT_LENGTH,
                     "%s",
                     buf+IN_PKT_HEAD_LENGTH);

            //fprintf(stderr,"out\n%s\n",out_buf+OUT_PKT_HEAD_LENGTH);/*printf into console*/
            //fprintf(stderr,"the length is %d\n",strlen(out_buf+OUT_PKT_HEAD_LENGTH));/*printf into console*/

            /*insert into buffer, '\0' is not stored in buffer, use memcpy instead of sprintf*/
            if (strlen(out_buf+OUT_PKT_HEAD_LENGTH) > (MAX_BUF_SIZE - (UINT32)index_tail))
            {
                if(0 == flag_stored_buffer_length_over_max)
                {
                    flag_stored_buffer_length_over_max = 1;
                }
                memcpy(stored_buffer+index_tail,(out_buf+OUT_PKT_HEAD_LENGTH),(MAX_BUF_SIZE-index_tail));
                memcpy(stored_buffer+0,(out_buf+OUT_PKT_HEAD_LENGTH)+(MAX_BUF_SIZE-index_tail),strlen(out_buf+OUT_PKT_HEAD_LENGTH)-(MAX_BUF_SIZE-index_tail));
                index_tail = strlen(out_buf+OUT_PKT_HEAD_LENGTH)-(MAX_BUF_SIZE-index_tail);
            }
            else
            {
                memcpy(stored_buffer+index_tail,(out_buf+OUT_PKT_HEAD_LENGTH),strlen(out_buf+OUT_PKT_HEAD_LENGTH));
                index_tail += strlen(out_buf+OUT_PKT_HEAD_LENGTH);
            }

            if (!send_packet_to_center_flag)
                return;
            sendto(sock_for_net,out_buf,OUT_PKT_HEAD_LENGTH+strlen(out_buf+OUT_PKT_HEAD_LENGTH),0,(struct sockaddr *)&sock_net_addr_center,sizeof(sock_net_addr_center));/*last '\0' is not transferred*/
            break;

        default:
            break;
    }

}

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
    
#define BUFF_SIZE 50    
LOCAL INT32 _find_process_by_name(CHAR* process_name)
{
    DIR *procEnt;
    struct dirent *next;
    char *ret;
    
    int i=0;

    procEnt = opendir("/proc");
    
    if (!procEnt)
    {
        fprintf(stderr, "FAILURE openning /proc\n");
        return -1;
    }
    
    while ((next = readdir(procEnt)) != NULL)
    {
        FILE *status;

        char buffer[BUFF_SIZE];
        char name[BUFF_SIZE];

        //Validate pid entry
        if (strcmp(next->d_name, "..") == 0 ||
            !isdigit(*next->d_name)
            
        )
            continue;

        //Read status entry
        sprintf(buffer, "/proc/%s/status", next->d_name);
        if (! (status = fopen(buffer, "r")) ) 
        {
            continue;
        }
        
        ret = fgets(buffer, BUFF_SIZE-1, status);
        fclose(status);
        if (!ret) 
        {
            continue;
        }

        sscanf(buffer, "%*s %s", name);
        if (strncmp(name, process_name, 15) == 0)
        {
            return strtol(next->d_name, NULL, 0) == getpid() ? 0 : 1;
        }
    }
    return 0;
}

INT32 main(VOID)
{
#define LOG_TRANSMITTER_SOCKET_NUM 2
    socket_info_t router_transmitter_2_socket[LOG_TRANSMITTER_SOCKET_NUM];

    if (_find_process_by_name("pal_log_router_transmitter") != 0)
    {
        fprintf(stderr,"\nlog_router_transmitter is running, no need to start again\n");
        return 0;
    }

    if(fork())
        return(0);	

    memset(module_info_in_router_transmitter_map,0,sizeof(module_info_in_router_transmitter_map));
    
    if (PAL_log_init_inet_udp_socket(LOG_ROUTER_TRANSMITTER_UDP_PORT,&sock_for_net) != 0)
    {
        return -1;
    }

    if (PAL_log_init_local_unix_udp_socket(LOG_ROUTER_TRANSMITTER_FILE_NAME, &sock_for_app) != 0)
    {
        close(sock_for_net);
        return -1;
    }

    router_transmitter_2_socket[0].socket_fd = sock_for_net;
    router_transmitter_2_socket[0].socket_type = PF_INET;
    router_transmitter_2_socket[0].packet_handle_func = _log_router_transmitter_packet_handle;

    router_transmitter_2_socket[1].socket_fd = sock_for_app;
    router_transmitter_2_socket[1].socket_type = PF_LOCAL;
    router_transmitter_2_socket[1].packet_handle_func = _log_router_transmitter_packet_handle;

    PAL_log_recv_and_process_packets(router_transmitter_2_socket, LOG_TRANSMITTER_SOCKET_NUM);

    return 0;
}

