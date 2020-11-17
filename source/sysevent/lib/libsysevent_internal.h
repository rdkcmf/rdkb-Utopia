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

#ifndef __LIB_SYSEVENT_INTERNAL
#define __LIB_SYSEVENT_INTERNAL

#include "sysevent.h"
/*
 * Given a *se_msg_hdr calculate the address of the body 
 * of the message
*/

/* #define SE_MSG_HDR_2_BODY(a) ((char *)(a) + sizeof(se_msg_hdr)) */


/* 
 * Procedure     : SE_print_message
 * Purpose       : Print a message
 * Parameters    :
 *    inmsg          : The message to print
 *    type           : the type of message
 * Return Code   :
 *    0              :
 */
int  SE_print_message(char *inmsg, int type);

/*
 * Procedure     : SE_string2size
 * Purpose       : Given a string, return the number of bytes
 *                 the string will require in a se_msg_string
 * Parameters    :
 *   str             : The string
 * Return Code   : The number of bytes the se_msg_string holding
 *                 this string would required
 */
unsigned int SE_string2size(const char *str);

/*
 * Procedure     : SE_print_message_hdr
 * Purpose       : Print a message header
 * Parameters    :
 *    msg            : The message hdr to print
 * Return Code   :
 *    0              :
 */
int  SE_print_message_hdr(char *hdr);

/*
 * Procedure     : SE_msg_add_string
 * Purpose       : Add a string to a SE_msg buffer. The string
 *                 contains an indication of the string size
 *                 and is 32 bit aligned
 *                 the mbytes field of the se_msg_hdr
 * Parameters    :
 *    msg            : A pointer to the start of the buffer at which to 
 *                     add the string
 *    size           : The maximum number of bytes in buffer
 *    str            : the string to add
 * Return Code   :
 *   0               : problem, string not added
 *   !0              : the number of bytes added to the msg
 */
int SE_msg_add_string(char *msg, unsigned int size, const char *str);

/*
 * Procedure     : SE_msg_hdr_mbytes_fixup
 * Purpose       : Given a se_msg calculate the
 *                 size and enter that into
 *                 the mbytes field of the se_msg_hdr
 * Parameters    :
 *    hdr            : A pointer to a complete se_msg_hdr + message
 * Return Code   :
 *    0              :
 */
int  SE_msg_hdr_mbytes_fixup (se_msg_hdr *hdr);

/*
 * Procedure     : SE_msg_receive
 * Purpose       : Receive a message from a ready fd
 * Parameters    :
 *    fd            : The file descriptor to receive from
 *    replymsg      : A buffer to place the reply message
 *    replymsg_size : The size of the reply message buffer
 *                    On return this contains the number of bytes used
 *    who           : On return the sender as known to the SE Server
 * Return Code   :
 *    SE_MSG_NONE   : error
 *    >0            : The type of message returned
 * Notes         :
 *   This call will block until a message arrives.
 */
int SE_msg_receive(int fd, char *replymsg, unsigned int *replymsg_size, token_t *who);

/*
 * Procedure     : SE_minimal_blocking_msg_receive
 * Purpose       : Receive a message from a ready fd using only a short wait
 * Parameters    :
 *    fd            : The file descriptor to receive from
 *    replymsg      : A buffer to place the reply message
 *    replymsg_size : The size of the reply message buffer
 *                    On return this contains the number of bytes used
 *    who           : On return the sender as known to the SE Server
 * Return Code   :
 *    SE_MSG_NONE   : error
 *    >0            : The type of message returned
 * Notes         :
 *   This call will return SE_MSG_NONE if there is not a message immediately there
 */
int SE_minimal_blocking_msg_receive (int fd, char *replymsg, unsigned int *replymsg_size, token_t *who);

/*
 * Procedure     : SE_msg_send
 * Purpose       : Send a Data Model message
 * Parameters    :
 *    fd            : The file descriptor to send to
 *    sendmsg       : The message to send
 * Return Code   :
 *    SE_MSG_NONE   : error
 *    >0            : The type of message returned
 */
int SE_msg_send (int fd, char *sendmsg);

/* 
 * Procedure     : SE_msg_send_receive
 * Purpose       : Send a message to the Data Model server and wait for a reply
 * Parameters    :
 *    fd            : The file descriptor to send to
 *    sendmsg       : The message to send
 *    replymsg      : A buffer to place the reply message
 *    replymsg_size : The size of the reply message buffer
 *                    On return this contains the number of bytes used
 * Return Code   :
 *    SE_MSG_NONE   : error
 *    >0            : The type of message returned
 */
int SE_msg_send_receive (int fd, char *sendmsg, char *replymsg, unsigned int *replymsg_size);

/*
 * Procedure     : SE_msg_prepare
 * Purpose       : Create a sysevent message
 * Parameters    :
 *    buf           : The message buffer in which to prepare the message
 *    bufsize       : The number of bytes in buf
 *    mtype         : The type of message
 *    sender        : The id of the sender
 * Return Code   :
 *    NULL          : error
 *    non NULL      : start of the body of the message
 */
char *SE_msg_prepare(char *buf, const unsigned int bufsize, const int mtype, const token_t sender);
char *SE_msg_get_data(char *msg, int *size);
int SE_msg_send_data (int fd, char *sendmsg,int msgsize);
int SE_msg_add_data(char *msg, unsigned int size, const char *data, const int data_length);
#endif   /* __LIB_SYSEVENT_INTERNAL */
