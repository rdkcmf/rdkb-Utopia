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
 * FileName    :  log_socket.c
 * Author      :  tao hong(tahong@cisco.com)
 * Date        :  2008-11-11
 * Description :  PAL LOG Module
 *****************************************************************************/
/*
*$Id: pal_log_socket.c,v 1.2 2009/05/14 02:58:20 tahong Exp $
*
*$Log: pal_log_socket.c,v $
*Revision 1.2  2009/05/14 02:58:20  tahong
*add: #include "pal_log_internal.h"
*
*Revision 1.1  2009/05/13 08:59:43  tahong
*create orignal version
*
*Revision 1.3  2008/12/10 05:50:23  tahong
*add cvs header
*
*Revision 1.2  2008/09/24 07:04:47  zhihliu
*merge back to main trunk
*
*/

#include <assert.h>

#include "pal_log.h"
#include "pal_log_internal.h"

INT32 PAL_log_init_local_unix_udp_socket(IN CHAR* socket_addr_path_this_end,
                                         OUT INT32* unix_socket)
{
    struct sockaddr_un socket_addr_this_end;

    *unix_socket = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if(*unix_socket < 0)
    {
        fprintf(stderr,"FAIL to create unix_socket.\n");
        return -1;
    }

    memset(&socket_addr_this_end,0,sizeof(socket_addr_this_end));
    socket_addr_this_end.sun_family = PF_LOCAL;
    if (snprintf(socket_addr_this_end.sun_path,MAX_MODULE_APP_SUN_PATH_LENGTH,socket_addr_path_this_end) < 0)
    {
        fprintf(stderr,"FAIL to assign value to socket_addr_this_end.sun_path.\n");
        close(*unix_socket);
        return -1;      
    }

    unlink(socket_addr_this_end.sun_path);
    if (bind(*unix_socket,(struct sockaddr *)&socket_addr_this_end, sizeof(socket_addr_this_end)) < 0)
    {
        fprintf(stderr,"FAIL to bind unix_socket.\n");
        close(*unix_socket);
        return -1;      
    }
    return 0;
}

INT32 PAL_log_init_local_unix_udp_socket_to_addr(IN CHAR *socket_addr_path_other_end,
                                                   OUT struct sockaddr_un *socket_addr_other_end)
{
    memset(socket_addr_other_end,0,sizeof(*socket_addr_other_end));
    socket_addr_other_end->sun_family = PF_LOCAL;
    if (snprintf(socket_addr_other_end->sun_path,MAX_MODULE_APP_SUN_PATH_LENGTH, socket_addr_path_other_end) < 0)
    {
        fprintf(stderr,"FAIL to assign value to socket_addr_other_end.sun_path.\n");
        return -1;
    }
    return 0;
}

INT32 PAL_log_init_inet_udp_socket(IN UINT16 port,
                                  OUT INT32* udp_socket)
{
    struct sockaddr_in socket_addr_this_end;

    *udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (*udp_socket < 0)
    {
        fprintf(stderr,"FAIL to create udp_socket.\n");
        return -1;
    }

    memset(&socket_addr_this_end,0,sizeof(socket_addr_this_end));
    socket_addr_this_end.sin_family = PF_INET;
    socket_addr_this_end.sin_addr.s_addr = 0;
    socket_addr_this_end.sin_port = htons(port);

    if(bind(*udp_socket,(struct sockaddr *)&socket_addr_this_end,sizeof(socket_addr_this_end))<0)
    {
        fprintf(stderr,"FAIL to bind udp_socket.\n");
        close(*udp_socket);
        return -1;
    }
    return 0;
}

INT32 PAL_log_init_inet_udp_socket_to_addr(IN ULONG socket_ip_addr_other_end,/*network order*/
                                            IN UINT16 port,
                                            OUT struct sockaddr_in *socket_addr_other_end)
{
    memset(socket_addr_other_end,0,sizeof(*socket_addr_other_end));
    socket_addr_other_end->sin_family = PF_INET;
    socket_addr_other_end->sin_addr.s_addr  = socket_ip_addr_other_end;
    socket_addr_other_end->sin_port = port;
    return 0;
}

VOID PAL_log_recv_and_process_packets(IN socket_info_t *socket_info_array, IN INT32 socket_count)
{
    fd_set readSet;    
    struct sockaddr_in in_addr;
    struct sockaddr_un un_addr;
    UINT32 in_addr_len;
    UINT32 un_addr_len;
    INT32 rv,i,max_sockets_num=0;
    CHAR buf[MAX_BUF_SIZE];

    memset(buf,0,sizeof(buf));

    while(1)
    {
        FD_ZERO(&readSet);

        for (i=0;i<socket_count;i++)
        {
            FD_SET(socket_info_array[i].socket_fd,&readSet);
            max_sockets_num = (max_sockets_num>socket_info_array[i].socket_fd) ? max_sockets_num : socket_info_array[i].socket_fd;
        }

        rv=select( max_sockets_num+1, &readSet,NULL,NULL,NULL);
        if(rv==-1)continue; /*break by signal*/

        for (i=0;i<socket_count;i++)
        {        
            if(FD_ISSET(socket_info_array[i].socket_fd,&readSet))
            {
                if(PF_INET == socket_info_array[i].socket_type)
                {
                    in_addr_len = sizeof(in_addr);
                    rv=recvfrom(socket_info_array[i].socket_fd,buf,sizeof(buf),MSG_DONTWAIT,(struct sockaddr *)&in_addr,&in_addr_len);
                    if(rv==-1)break;
                    if(((UINT32)rv)>=sizeof(buf))/*invalid length*/
                        continue;
                    buf[rv]=0; /*to ensure it is null terminated*/
                    socket_info_array[i].packet_handle_func(buf,rv,(struct sockaddr *)&in_addr,in_addr_len);

                }
                else if (PF_LOCAL == socket_info_array[i].socket_type)
                {
                    un_addr_len = sizeof(un_addr);
                    rv=recvfrom(socket_info_array[i].socket_fd,buf,sizeof(buf),MSG_DONTWAIT,(struct sockaddr *)&un_addr,&un_addr_len);
                    if(rv==-1)break;
                    if(((UINT32)rv)>=sizeof(buf))/*invalid length*/
                        continue;
                    buf[rv]=0; /*to ensure it is null terminated*/
                    socket_info_array[i].packet_handle_func(buf,rv,(struct sockaddr *)&un_addr,un_addr_len);
                }
            }
        }
    }
    
}

CHAR LOG_LEVEL_NAME[MAX_LEVEL_NUMBER][MAX_NAME_LENGTH]=
{
    "IGNORE_ALL",
    "FAILURE",
    "WARNING",
    "INFO",
    "DEBUG"
};

