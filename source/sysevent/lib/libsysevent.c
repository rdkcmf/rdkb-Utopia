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
  ================================================================
                       libsysevent.c

  This is a library of APIs to send/recv/parse messages between
  a client and a sysevent daemon.

  The sysevent daemon can run locally and will be accessible by 
  TCP/IP or UDS.
  The sysevent daemon can also run remotely and will be accessible
  by TCP/IP

  Messages are carried by a stream protocol, and the library chops the
  stream into sysevent messages.

  Author : mark enright
  ================================================================
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <netdb.h>     // for gethostbyname
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>

#include "libsysevent_internal.h"

// how many times does library attempt to connect to a non blocking socket returning EINPROGRESS
#define NUM_CONNECT_ATTEMPTS 10   

//#define RUNTIME_DEBUG 1
#ifdef RUNTIME_DEBUG
char *debug_filename = "/var/log/sysevent_lib.err";
#endif  // RUNTIME_DEBUG

/*
 * This mutex serializes the get/set operations on an fd (sysevent file 
 * descriptor). Currently one mutex is used per-process. Hence operations on 
 * multiple sysevent fd created within one process will be serialized.
 * In the future this could be enhanced into a list of {fd, mutex} tuples
 */
pthread_mutex_t g_client_fd_mutex = PTHREAD_MUTEX_INITIALIZER;
char g_name[32];

/*
 * Procedure   : align
 * Purpose     : align at 4 bytes
 * Parameters  :
 *    value      : an integer to convert to a 32 bit aligned
 *                 value
 * Return Code : the word aligned integer
 */
static unsigned int align (unsigned int value)
{
   if (0 == value) {
      return(0);
   }
   while (0 != (value & 0x00000003)) {
      value++;
   }
   return(value);
}

/*
 * Procedure     : init_libsysevent
 * Purpose       : initialize this library
 * Parameters    :
 *    name          : name of the user of this library
 * Return Code   :
 */ 
void init_libsysevent(const char* const name) {
   // save the name for error messages
   snprintf(g_name, sizeof(g_name), "%s", name);
}

/*
 * Procedure     : msg_receive_internal
 * Purpose       : Receive a message if the message
 * Parameters    :
 *    fd            : The file descriptor to receive from
 *    replymsg      : A buffer to place the reply message
 *    replymsg_size : The size of the reply message buffer
 *                    On return this contains the number of bytes used
 *    who           : On return the sender as known to the sysevent daemon
 *    wait          : The maximum length of time to wait for a message
 *    error         : On return 0 if successful, errno if some failure
 * Return Code   :
 *    SE_MSG_NONE   : error
 *    >0            : The type of message returned
 */
static int msg_receive_internal (int fd, char *replymsg, unsigned int *replymsg_size, token_t *who, struct timeval *wait, int *error)
{
   *error = 0;

   if (0 > fd) {
     *replymsg_size = 0;
     *who           = TOKEN_NULL;
      return(SE_MSG_NONE);
   }
   if (*replymsg_size < sizeof(se_msg_hdr)) {
     *replymsg_size = 0;
     *who           = TOKEN_NULL;
      return(SE_MSG_NONE);
   }

   struct timeval *tptr = (NULL == wait ? NULL : wait);

   for ( ; ; ) {
      fd_set rd_set;
      FD_ZERO(&rd_set);
      FD_SET(fd, &rd_set);

      int rc = select(fd+1, &rd_set, NULL, NULL, tptr);

      if (0 == rc) {
         if (NULL == tptr) {
             continue;
         } else if (0 != tptr->tv_sec || 0 != tptr->tv_usec) {
             /*
              * Linux will return the amount of time not used for a select in the timeval.
              * This is not portable across all OSes
              */
              continue;
         } else {
            // if we got here then we have waited long enough
            *error         = EWOULDBLOCK;
            *replymsg_size = 0;
            *who           = TOKEN_NULL;
            return(SE_MSG_NONE);
         }
      } else if (0 > rc) {
         if (NULL != tptr) {
            if (EAGAIN == errno || EINTR == errno) {
               if (0 == tptr->tv_sec && 0 == tptr->tv_usec) {
                  /* Wait time has expired */
#ifdef RUNTIME_DEBUG
                  FILE *fp = fopen(debug_filename, "a+");
                  if (NULL != fp) {
                     fprintf(fp, "msg_receive_internal select error (%d) %s for %s using fd %d and Time exceeded\n", 
                                   errno, strerror(errno), NULL==g_name ? "unknown" : g_name, fd);
                     fclose(fp);
                  }
#endif  // RUNTIME_DEBUG
                  *error         = EWOULDBLOCK;
                  *replymsg_size = 0;
                  *who           = TOKEN_NULL;
                  return(SE_MSG_NONE);
               } else {
                  /* Wait time has not expired */
                  /*
                   * Linux will return the amount of time not used for a select in the timeval.
                   * This is not portable across all OSes
                   */
#ifdef RUNTIME_DEBUG
                  FILE *fp = fopen(debug_filename, "a+");
                  if (NULL != fp) {
                     fprintf(fp, "Info: msg_receive_internal select error (%d) %s for %s using fd %d. Retrying.\n", 
                                   errno, strerror(errno), NULL==g_name ? "unknown" : g_name, fd);
                     fclose(fp);
                  }
#endif  // RUNTIME_DEBUG
                  struct timespec sleep_time;
                  sleep_time.tv_sec = 0;
                  sleep_time.tv_nsec  = 100000000;  // .1 secs
                  nanosleep(&sleep_time, NULL);
                  continue;
               }
            } else {
               /* select error that is not EAGAIN or EINTR */
#ifdef RUNTIME_DEBUG
               FILE *fp = fopen(debug_filename, "a+");
               if (NULL != fp) {
                  fprintf(fp, "msg_receive_internal select error (%d) %s for %s using fd %d. Abort call.\n", 
                                errno, strerror(errno), NULL==g_name ? "unknown" : g_name, fd);
                  fclose(fp);
               }
#endif  // RUNTIME_DEBUG
               *error         = errno;
               *replymsg_size = 0;
               *who           = TOKEN_NULL;
               return(SE_MSG_NONE);
            }
         } else {
            /* There is no limit on the time to wait */
            if (EAGAIN == errno || EINTR == errno) {
#ifdef RUNTIME_DEBUG
               FILE *fp = fopen(debug_filename, "a+");
               if (NULL != fp) {
                  fprintf(fp, "msg_receive_internal select error (%d) %s for %s using fd %d and no time limit. Retrying.\n", 
                                errno, strerror(errno), NULL==g_name ? "unknown" : g_name, fd);
                  fclose(fp);
               }
#endif  // RUNTIME_DEBUG
               struct timespec sleep_time;
               sleep_time.tv_sec = 0;
               sleep_time.tv_nsec  = 100000000;  // .1 secs
               nanosleep(&sleep_time, NULL);
               continue;
            } else {
               /* fatal error */
#ifdef RUNTIME_DEBUG
               FILE *fp = fopen(debug_filename, "a+");
               if (NULL != fp) {
                  fprintf(fp, "msg_receive_internal select error (%d) %s for %s using fd %d. Abort blocking call.\n", 
                                errno, strerror(errno), NULL==g_name ? "unknown" : g_name, fd);
                  fclose(fp);
               }
#endif  // RUNTIME_DEBUG
               *error         = errno;
               *replymsg_size = 0;
               *who           = TOKEN_NULL;
               return(SE_MSG_NONE);
            }
         }
      } else {
         if (! FD_ISSET(fd, &rd_set)) {
            if (NULL == tptr) {
               continue;
            } else if (0 != tptr->tv_sec && 0 != tptr->tv_usec) {
               /*
                * Linux will return the amount of time not used for a select in the timeval.
                * This is not portable across all OSes
                */
               continue;
            } else {
              // if we got here then we have waited long enough
              *error         = EWOULDBLOCK;
              *replymsg_size = 0;
              *who           = TOKEN_NULL;
              return(SE_MSG_NONE);
            }
         } else {
            // the fd is ready to read
            break;
         } 
      }
   }

   // read in the constant size se_msg_header
   se_msg_hdr msg_hdr;
   int recv_bytes = -1;
   while (-1 == recv_bytes) {
      recv_bytes = read(fd, (void *)&msg_hdr, sizeof(se_msg_hdr));
      if ( -1 == recv_bytes && EAGAIN != errno && EWOULDBLOCK != errno && EINTR != errno) {
         *error = errno;
         *replymsg_size = 0;
         *who           = TOKEN_NULL;
         return(SE_MSG_NONE);
      }
   }

   if (sizeof(se_msg_hdr) != recv_bytes) { 
     *replymsg_size = 0;
     *who           = TOKEN_NULL;
      return(SE_MSG_NONE);
   } else if (MSG_DELIMITER != (uint32_t)ntohl(msg_hdr.poison)) {
     *replymsg_size = 0;
     *who           = TOKEN_NULL;
      return(SE_MSG_NONE);
   } else {
     *who           = htonl(msg_hdr.sender_token);
   }

   // Now we can figure out how many bytes are in the rest of the message
   // The msg footer was added by the transport layer send routing but does not appear in the msg_hdr.mbyte count
   int expected_bytes = ntohl(msg_hdr.mbytes) + sizeof(se_msg_footer) - recv_bytes;
   // The transport footer occupies the last bytes
   unsigned int msg_footer_offset = expected_bytes - sizeof(se_msg_footer);

   // if there is not enough room in the reply buffer for the message, then get rid of the message
   if (expected_bytes > (int) *replymsg_size) {
     se_buffer junk;
     int  read_bytes;
     while (0 < expected_bytes) {
        read_bytes = read(fd, junk, sizeof(junk));

        if ( -1 == read_bytes && EAGAIN != errno && EWOULDBLOCK != errno && EINTR != errno) {
           *error = errno;
           *replymsg_size = 0;
           *who           = TOKEN_NULL;
           return(SE_MSG_NONE);
        } else if (-1 == read_bytes) {
           continue;
        }

        if (0 == read_bytes) {
           expected_bytes = 0;
        } else {
           expected_bytes -= read_bytes;
        }
      }
     *replymsg_size = 0;
     *who           = TOKEN_NULL;
      return(SE_MSG_NONE);
   }

   // everything appears to be ok, so read in the rest of the message
   int  read_bytes;
   int  current_byte = 0;
   while (0 < expected_bytes) {
     read_bytes = read(fd, replymsg+current_byte, expected_bytes);
     if (-1 == read_bytes && EAGAIN != errno && EWOULDBLOCK != errno && EINTR != errno) {
        *error         = errno;
        *replymsg_size = 0;
        *who           = TOKEN_NULL;
        return(SE_MSG_NONE);
     } else if (-1 == read_bytes) {
          continue;
     }

     if (0 == read_bytes) {
        expected_bytes = 0;
     } else {
        expected_bytes -= read_bytes;
        current_byte   += read_bytes;
     }
   }

   // make sure that the msg footer is correct
   se_msg_footer *msg_footer = (se_msg_footer *)(replymsg + msg_footer_offset);
   if (MSG_DELIMITER != (uint32_t)ntohl(msg_footer->poison)) {
     *replymsg_size = 0;
     *who           = TOKEN_NULL;
      return(SE_MSG_NONE);
   } else {
      // the footer is ok, so strip it off
      msg_footer->poison = 0x00000000;
   }

   *replymsg_size = current_byte;
   return(ntohl(msg_hdr.mtype));
}

/*
 * Procedure     : SE_string2size
 * Purpose       : Given a string, return the number of bytes
 *                 the string will require in a se_msg_string
 * Parameters    :
 *   str             : The string
 * Return Code   : The number of bytes the se_msg_string holding
 *                 this string would required including the se_msg_string
 */
unsigned int SE_string2size(const char *str)
{
   if (NULL == str) {
      return(0);
   }
   return (align(strlen(str)+1) + SE_MSG_STRING_OVERHEAD);
}
 
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
 * Notes         :
 *   If str is NULL then the added string will have a length of 0
 */
int SE_msg_add_string(char *msg, unsigned int size, const char *str) 
{
   if (NULL == msg) {
      return (0);
   }
   int aligned_size      = (NULL == str ? 0 : align(strlen(str)+1));
   unsigned int msg_increase_size = (NULL == str ? SE_MSG_STRING_OVERHEAD : SE_string2size(str));  

   if  ( msg_increase_size > size ) {
      return(0);
   } else {
      memset(msg, 0, msg_increase_size);
   }

   se_msg_string *tstruct;
   tstruct       = (se_msg_string *) (msg);
   tstruct->size = htonl(aligned_size);
  /* 
   * Originally a se_msg_string had a zero length variable chat str[0]
   * which was changed to support inclusion of header files in ISO C code,
   * so instead we just know that the string starts immediately after size
   */
   //snprintf (tstruct->str, aligned_size, "%s", str);
   snprintf (((char *)(&tstruct->size))+sizeof(tstruct->size), aligned_size, "%s", str);
   return(msg_increase_size);
}

int SE_msg_add_data(char *msg, unsigned int size, const char *data, const int data_length)
{
   if (NULL == msg) {
      return (0);
   }
   int aligned_size      = (NULL == data ? 0 : data_length);
   unsigned int msg_increase_size = (NULL == data ? 0 : data_length);

   if  ( msg_increase_size > size ) {
      return(0);
   }
   else
   {
       if (msg_increase_size !=0)
       {
           memset(msg, 0, msg_increase_size);
       }
   }

   se_msg_string *tstruct;
   tstruct       = (se_msg_string *) (msg);
   tstruct->size = htonl(aligned_size);
   if (data_length == 0)
      return 0;
  /*
   * Originally a se_msg_string had a zero length variable chat str[0]
   * which was changed to support inclusion of header files in ISO C code,
   * so instead we just know that the string starts immediately after size
   */
   //snprintf (tstruct->str, aligned_size, "%s", str);
   memcpy(((char *)(&tstruct->size))+sizeof(tstruct->size),data,data_length);
   return(msg_increase_size);
}

char *SE_msg_get_data(char *msg, int *size)
{
   if (NULL == msg) {
      *size = 0;
      return (NULL);
   }

   se_msg_string *tstruct;
   tstruct = (se_msg_string *) (msg);
   // size includes the string as well as the rest of the se_msg_string
   int string_len = ntohl(tstruct->size);
   *size   = string_len;
  /*
   * Originally a se_msg_string had a zero length variable chat str[0]
   * which was changed to support inclusion of header files in ISO C code,
   * so instead we just know that the string starts mmediately after size
   */
//   return(0 == string_len ? NULL : (char *)(tstruct->str));
   return(0 == string_len ? NULL : (char *)(((char *)(&tstruct->size))+sizeof(tstruct->size)));
}


/*
 * Procedure     : SE_msg_get_string
 * Purpose       : Get a string from a SE_msg buffer. The buffer 
 *                 must be pointing at the se_msg_string containing the string
 * Parameters    :
 *    msg            : A pointer to the start of the se_msg_string
 *    size           : On return the number of bytes that the se_msg_string occupied
 * Return Code   :
 *   NULL            : problem, string not gotten or string had 0 length
 *   !NULL           : string
 *  Note         : It is possible for the returned string to be NULL but the size will be positive
 *                 because even a NULL string has a length in the message
 */
char *SE_msg_get_string(char *msg, int *size)
{
   if (NULL == msg) {
      *size = 0;
      return (NULL);
   }

   se_msg_string *tstruct;
   tstruct = (se_msg_string *) (msg);
   // size includes the string as well as the rest of the se_msg_string
   int string_len = ntohl(tstruct->size);
   *size   = (string_len + SE_MSG_STRING_OVERHEAD);
  /* 
   * Originally a se_msg_string had a zero length variable chat str[0]
   * which was changed to support inclusion of header files in ISO C code,
   * so instead we just know that the string starts mmediately after size
   */
//   return(0 == string_len ? NULL : (char *)(tstruct->str));
   return(0 == string_len ? NULL : (char *)(((char *)(&tstruct->size))+sizeof(tstruct->size)));
}

/*
 * Procedure     : mtype2str 
 * Purpose       : Print the mtype 
 * Parameters    :
 *    mtype          : The mtype to print
 * Return Code   :
 *    the string
 * Notes         :
 *    Do not save the string. It has no lifespan
 */
static char *mtype2str(int mtype)
{
   switch(ntohl(mtype)) {
      case(SE_MSG_NONE):
         return("SE_MSG_NONE");
      case(SE_MSG_OPEN_CONNECTION):
         return("SE_MSG_OPEN_CONNECTION");
      case(SE_MSG_OPEN_CONNECTION_DATA):
         return("SE_MSG_OPEN_CONNECTION_DATA");    
      case(SE_MSG_OPEN_CONNECTION_REPLY):
         return("SE_MSG_OPEN_CONNECTION_REPLY");
      case(SE_MSG_CLOSE_CONNECTION):
         return("SE_MSG_CLOSE_CONNECTION");
      case(SE_MSG_CLOSE_CONNECTION_REPLY):
         return("SE_MSG_CLOSE_CONNECTION_REPLY");
      case(SE_MSG_PING):
         return("SE_MSG_PING");
      case(SE_MSG_PING_REPLY):
         return("SE_MSG_PING_REPLY");
      case(SE_MSG_NEW_CLIENT):
         return("SE_MSG_NEW_CLIENT");
      case(SE_MSG_DIE):
         return("SE_MSG_DIE");
      case(SE_MSG_GET):
         return("SE_MSG_GET");
      case(SE_MSG_GET_DATA):
         return("SE_MSG_GET_DATA");
      case(SE_MSG_GET_REPLY):
         return("SE_MSG_GET_REPLY");
      case(SE_MSG_GET_DATA_REPLY):
         return("SE_MSG_GET_DATA_REPLY");
      case(SE_MSG_SET):
         return("SE_MSG_SET");
       case(SE_MSG_SET_DATA):
         return("SE_MSG_SET_DATA");        
      case(SE_MSG_SET_REPLY):
         return("SE_MSG_SET_REPLY");
      case(SE_MSG_SET_UNIQUE):
         return("SE_MSG_SET_UNIQUE");
      case(SE_MSG_SET_UNIQUE_REPLY):
         return("SE_MSG_SET_UNIQUE_REPLY");
      case(SE_MSG_ITERATE_GET):
         return("SE_MSG_ITERATE_GET");
      case(SE_MSG_ITERATE_GET_REPLY):
         return("SE_MSG_ITERATE_GET_REPLY");
      case(SE_MSG_DEL_UNIQUE):
         return("SE_MSG_DEL_UNIQUE");
      case(SE_MSG_NEXT_ITERATOR_GET):
         return("SE_MSG_NEXT_ITERATOR_GET");
      case(SE_MSG_NEXT_ITERATOR_GET_REPLY):
         return("SE_MSG_NEXT_ITERATOR_GET_REPLY");
      case(SE_MSG_SET_OPTIONS):
         return("SE_MSG_SET_OPTIONS");
      case(SE_MSG_SET_OPTIONS_REPLY):
         return("SE_MSG_SET_OPTIONS_REPLY");
      case(SE_MSG_SET_ASYNC_ACTION):
         return("SE_MSG_SET_ASYNC_ACTION");
      case(SE_MSG_SET_ASYNC_MESSAGE):
         return("SE_MSG_SET_ASYNC_MESSAGE");
      case(SE_MSG_SET_ASYNC_REPLY):
         return("SE_MSG_SET_ASYNC_REPLY");
      case(SE_MSG_REMOVE_ASYNC):
         return("SE_MSG_REMOVE_ASYNC");
      case(SE_MSG_REMOVE_ASYNC_REPLY):
         return("SE_MSG_REMOVE_ASYNC_REPLY");
      case(SE_MSG_EXECUTE_SERIALLY):
         return("SE_MSG_EXECUTE_SERIALLY");
      case(SE_MSG_RUN_EXTERNAL_EXECUTABLE):
         return("SE_MSG_RUN_EXTERNAL_EXECUTABLE");
      case(SE_MSG_NOTIFICATION):
         return("SE_MSG_NOTIFICATION");
      case(SE_MSG_SEND_NOTIFICATION):
         return("SE_MSG_SEND_NOTIFICATION");
      case(SE_MSG_SHOW_DATA_ELEMENTS):
         return("SE_MSG_SHOW_DATA_ELEMENTS");
      default:
         return("UNKNOWN_SE_MSG");
   }
}

/*
 * Procedure     : SE_print_message
 * Purpose       : Print a message
 * Parameters    :
 *    msg            : The message to print
 *    type           : the type of message
 * Return Code   :
 *    0              :
 */
int SE_print_message(char* inmsg, int type)
{
   switch(type) {
      case(SE_MSG_NONE):
      {
         break;
      }
      case(SE_MSG_OPEN_CONNECTION):
      case(SE_MSG_OPEN_CONNECTION_DATA):
      {
         int             id_size;
         se_open_connection_msg *msg = (se_open_connection_msg *)inmsg;
         char *data                  = (char *)&(msg->data);
         char *id                    = SE_msg_get_string(data, &id_size);

         printf("|------- SE_MSG_OPEN_CONNECTION ---------|\n");
         printf("| version       : %d\n", ntohl(msg->version));
         printf("| id_bytes      : %d\n", id_size);
         printf("| id            : %s\n", id);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_OPEN_CONNECTION_REPLY):
      {
         se_open_connection_reply_msg *msg = (se_open_connection_reply_msg *)inmsg;

         printf("|------ SE_MSG_OPEN_CONNECTION_REPLY ----|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("| token_id      : %x\n", (unsigned int)(ntohl(msg->token_id)));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_CLOSE_CONNECTION):
      {
         printf("|------- SE_MSG_CLOSE_CONNECTION --------|\n");
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_CLOSE_CONNECTION_REPLY):
      {
         printf("|----- SE_MSG_CLOSE_CONNECTION_REPLY ----|\n");
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_PING):
      {
         printf("|--------------- SE_MSG_PING ------------|\n");
         printf("| reserved      :   \n");
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_PING_REPLY):
      {
         printf("|---------- SE_MSG_PING_REPLY -----------|\n");
         printf("| reserved      :   \n");
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_DEBUG):
      {
         se_debug_msg *msg = (se_debug_msg *)inmsg;

         printf("|------------- SE_MSG_DEBUG -------------|\n");
         printf("| debug level   : 0x%x\n", ntohl(msg->level));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_NEW_CLIENT):
      {
         se_new_client_msg *msg = (se_new_client_msg *)inmsg;
         printf("|---------- SE_MSG_NEW_CLIENT -----------|\n");
         printf("| token_id      : %x\n", (unsigned int)(ntohl(msg->token_id)));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_DIE):
      {
         break;
      }
      case(SE_MSG_GET):
      {
         int       subject_size;
         se_get_msg *msg = (se_get_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *subject   = SE_msg_get_string(data, &subject_size);

         printf("|------------- SE_MSG_GET ---------------|\n");
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }      
      case(SE_MSG_GET_DATA):
      {
         int       subject_size;
         se_get_msg *msg = (se_get_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *subject   = SE_msg_get_string(data, &subject_size);

         printf("|------------- SE_MSG_GET ---------------|\n");
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_GET_REPLY):
      {
         int              subject_size;
         int              value_size;
         se_get_reply_msg *msg = (se_get_reply_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         char *value           =  SE_msg_get_string(data, &value_size);

         printf("|------------ SE_MSG_GET_REPLY ----------|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| value_bytes   : %d\n", value_size);
         printf("| subject       : %s\n", subject);
         printf("| value         : %s\n", value);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_GET_DATA_REPLY):
      {
         int              subject_size;
         int              value_size;
         se_get_reply_msg *msg = (se_get_reply_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         char *value           =  SE_msg_get_data(data, &value_size);

         printf("|------------ SE_MSG_GET_DATA_REPLY ----------|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| value_bytes   : %d\n", value_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET):
      {
         int        subject_size;
         int        value_size;
         se_set_msg *msg = (se_set_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         char *value           =  SE_msg_get_string(data, &value_size);

         printf("|-------------- SE_MSG_SET --------------|\n");
         printf("| source        : %d\n", msg->source);
         printf("| tid           : %d\n", msg->tid);
         printf("| subject_bytes : %d\n", subject_size);
         printf("| value_bytes   : %d\n", value_size);
         printf("| subject       : %s\n", subject);
         printf("| value         : %s\n", value);
         printf("|----------------------------------------|\n");
         break;
      }

      case(SE_MSG_SET_DATA):
      {
         int        subject_size;
         int        value_size;
         se_set_msg *msg = (se_set_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         char *value           =  SE_msg_get_data(data, &value_size);

         printf("|-------------- SE_MSG_SET_DATA --------------|\n");
         printf("| source        : %d\n", msg->source);
         printf("| tid           : %d\n", msg->tid);
         printf("| subject_bytes : %d\n", subject_size);
         printf("| value_bytes   : %d\n", value_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_REPLY):
      {
         se_set_reply_msg *msg = (se_set_reply_msg *)inmsg;
         printf("|----------- SE_MSG_SET_REPLY -----------|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_UNIQUE):
      {
         int        subject_size;
         int        value_size;
         se_set_unique_msg *msg = (se_set_unique_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         char *value           =  SE_msg_get_string(data, &value_size);

         printf("|------------- SE_MSG_SET_UNIQUE --------|\n");
         printf("| subject_bytes : %d\n", subject_size);
         printf("| value_bytes   : %d\n", value_size);
         printf("| subject       : %s\n", subject);
         printf("| value         : %s\n", value);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_UNIQUE_REPLY):
      {
         int              subject_size;
         se_set_unique_reply_msg *msg = (se_set_unique_reply_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);


         printf("|---------- SE_MSG_SET_UNIQUE_REPLY -----|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_ITERATE_GET):
      {
         int       subject_size;
         se_iterate_get_msg *msg = (se_iterate_get_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *subject   = SE_msg_get_string(data, &subject_size);

         printf("|----------- SE_MSG_ITERATE_GET ---------|\n");
         printf("| iterator      : %x\n", ntohl(msg->iterator));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_ITERATE_GET_REPLY):
      {
         int              subject_size;
         int              value_size;
         se_iterate_get_reply_msg *msg = (se_iterate_get_reply_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         char *value           =  SE_msg_get_string(data, &value_size);

         printf("|-------- SE_MSG_ITERATE_GET_REPLY ------|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("| iterator      : %x\n", ntohl(msg->iterator));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| value_bytes   : %d\n", value_size);
         printf("| subject       : %s\n", subject);
         printf("| value         : %s\n", value);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_DEL_UNIQUE):
      {
         int        subject_size;
         se_del_unique_msg *msg = (se_del_unique_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);

         printf("|----------- SE_MSG_DEL_UNIQUE ----------|\n");
         printf("| iterator      : %x\n", ntohl(msg->iterator));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_NEXT_ITERATOR_GET):
      {
         int              subject_size;
         se_iterate_get_iterator_msg *msg = (se_iterate_get_iterator_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);

         printf("|--------- SE_MSG_NEXT_ITERATOR_GET -----|\n");
         printf("| iterator      : %x\n", ntohl(msg->iterator));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_NEXT_ITERATOR_GET_REPLY):
      {
         int              subject_size;
         se_iterate_get_iterator_reply_msg *msg = (se_iterate_get_iterator_reply_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);

         printf("|---- SE_MSG_NEXT_ITERATOR_GET_REPLY ----|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("| iterator      : %x\n", ntohl(msg->iterator));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_OPTIONS):
      {
         int        subject_size;
         se_set_options_msg *msg = (se_set_options_msg *)inmsg;
         char *data            = (char *)&(msg->data);
         char *subject         = SE_msg_get_string(data, &subject_size);

         printf("|---------- SE_MSG_SET_OPTIONS ----------|\n");
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("| flags         : 0x%x\n", ntohl(msg->flags));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_OPTIONS_REPLY):
      {
         se_set_options_reply_msg *msg = (se_set_options_reply_msg *)inmsg;
         printf("|------ SE_MSG_SET_OPTIONS_REPLY --------|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_ASYNC_ACTION):
      {
         int               subject_size;
         int               function_size;
         int               param_size;
         char             *param;
         se_set_async_action_msg *msg  = (se_set_async_action_msg *)inmsg;
         char             *data        = (char *)&(msg->data);
         char             *subject     = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         char *function                =  SE_msg_get_string(data, &function_size);
         data += function_size;


         printf("|-------- SE_MSG_SET_ASYNC_ACTION -------|\n");
         printf("| flags         : 0x%x\n", ntohl(msg->flags));
         printf("| num_params    : %d\n", ntohl(msg->num_params));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("| function_bytes: %d\n", function_size);
         printf("| function      : %s\n", function);
         unsigned int i;
         for (i=0 ; i< (unsigned int) (ntohl(msg->num_params)); i++) {
            param = SE_msg_get_string(data, &param_size);

            printf("| param_bytes   : %d\n", param_size);
            printf("| param         : %s\n", param);
            data += param_size;
         }
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_ASYNC_MESSAGE):
      {
         int       subject_size;
         se_set_async_message_msg *msg = (se_set_async_message_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *subject   = SE_msg_get_string(data, &subject_size);

         printf("|-------- SE_MSG_SET_ASYNC_MESSAGE ------|\n");
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("| flags         : 0x%x\n", ntohl(msg->flags));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SET_ASYNC_REPLY):
      {
         se_set_async_reply_msg *msg = (se_set_async_reply_msg *)inmsg;
         printf("|------- SE_MSG_SET_ASYNC_REPLY ---------|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("| async_id      : 0x%x 0x%x\n", 
                    (msg->async_id).trigger_id, (msg->async_id).action_id );
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_REMOVE_ASYNC):
      {
         se_remove_async_msg *msg = (se_remove_async_msg *)inmsg;
         printf("|---------- SE_MSG_REMOVE_ASYNC ---------|\n");
         printf("| async_id      : 0x%x 0x%x\n", 
                    (msg->async_id).trigger_id, (msg->async_id).action_id );
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_REMOVE_ASYNC_REPLY):
      {
         se_remove_async_reply_msg *msg = (se_remove_async_reply_msg *)inmsg;
         printf("|-------- SE_MSG_REMOVE_ASYNC_REPLY -----|\n");
         printf("| status        : 0x%x\n", ntohl(msg->status));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SEND_NOTIFICATION):
      {
         int   subject_size;
         se_send_notification_msg *msg = (se_send_notification_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *subject   = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         int   value_size;
         char *value              =  SE_msg_get_string(data, &value_size);
         data += value_size;

         printf("|------- SE_MSG_SEND_NOTIFICATION -------|\n");
         printf("| source        : %d\n", msg->source);
         printf("| tid           : %d\n", msg->tid);
         printf("| token_id      : %x\n", (unsigned int)(ntohl(msg->token_id)));
         printf("| async_id      : 0x%x 0x%x\n", 
                    (msg->async_id).trigger_id, (msg->async_id).action_id );
         printf("| flags         : 0x%x\n", ntohl(msg->flags));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("| value_bytes   : %d\n", value_size);
         printf("| value         : %s\n", value);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_RUN_EXTERNAL_EXECUTABLE):
      {
         int   subject_size;
         se_run_executable_msg *msg = (se_run_executable_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *subject   = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         int   value_size;
         char *value              =  SE_msg_get_string(data, &value_size);
         data += value_size;

         printf("|---- SE_MSG_RUN_EXTERNAL_EXECUTABLE ----|\n");
         printf("| token_id      : %x\n", (unsigned int)(ntohl(msg->token_id)));
         printf("| async_id      : 0x%x 0x%x\n", 
                    (msg->async_id).trigger_id, (msg->async_id).action_id );
         printf("| flags         : 0x%x\n", ntohl(msg->flags));
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("| value_bytes   : %d\n", value_size);
         printf("| value         : %s\n", value);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_NOTIFICATION):
      {
         int   subject_size;
         se_notification_msg *msg = (se_notification_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *subject   = SE_msg_get_string(data, &subject_size);
         data += subject_size;
         int   value_size;
         char *value              =  SE_msg_get_string(data, &value_size);
         data += value_size;

         printf("|--------- SE_MSG_NOTIFICATION ----------|\n");
         printf("| source        : %d\n", msg->source);
         printf("| tid           : %d\n", msg->tid);
         printf("| async_id      : 0x%x 0x%x\n",
                    (msg->async_id).trigger_id, (msg->async_id).action_id );
         printf("| subject_bytes : %d\n", subject_size);
         printf("| subject       : %s\n", subject);
         printf("| value_bytes   : %d\n", value_size);
         printf("| value         : %s\n", value);
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_EXECUTE_SERIALLY):
      {
         se_run_serially_msg *msg = (se_run_serially_msg *)inmsg;

         printf("|------- SE_MSG_EXECUTE_SERIALLY --------|\n");
         printf("| async_id      : 0x%x 0x%x\n",
                    (msg->async_id).trigger_id, (msg->async_id).action_id );
         printf("| num_msgs      : %d\n", (msg->num_msgs));
         printf("| data          : %p\n", (msg->data));
         printf("|----------------------------------------|\n");
         break;
      }
      case(SE_MSG_SHOW_DATA_ELEMENTS):
      {
         int       filename_size;
         se_show_data_elements_msg *msg = (se_show_data_elements_msg *)inmsg;
         char *data      = (char *)&(msg->data);
         char *filename   = SE_msg_get_string(data, &filename_size);

         printf("|------ SE_MSG_SHOW_DATA_ELEMENTS -------|\n");
         printf("| filename_size : %d\n", filename_size);
         printf("| filename      : %s\n", filename);
         printf("|----------------------------------------|\n");
         break;
      }
      default:
         break;
   }
   return(0);
}

/*
 * Procedure     : SE_print_message_hdr
 * Purpose       : Print a message header and the message
 * Parameters    :
 *    msg            : The message hdr to print
 * Return Code   :
 *    0              :
 */
int  SE_print_message_hdr (char *inhdr)
{
#ifdef REALLY_PRINT_HDR
   se_msg_hdr *hdr = (se_msg_hdr *)inhdr;
   printf("|------------------ %p ------------------|\n", hdr);
   printf("| delimiter     : 0x%x\n", ntohl(hdr->poison));
   printf("| mbytes        : %d\n", ntohl(hdr->mbytes));
   printf("| mtype         : (%d) %s\n", ntohl(hdr->mtype), mtype2str(hdr->mtype));  ;
   printf("| sender        : %x\n", (unsigned int)(ntohl(hdr->sender_token)));
   SE_print_message(SE_MSG_HDR_2_BODY(hdr), ntohl(hdr->mtype)); 
   printf("|------------------------------------------------|\n");
#else
   se_msg_hdr *hdr = (se_msg_hdr *)inhdr;
   SE_print_message(SE_MSG_HDR_2_BODY(hdr), ntohl(hdr->mtype)); 
#endif
   return(0);
}

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
int  SE_msg_hdr_mbytes_fixup (se_msg_hdr *hdr)
{
   int msg_bytes = sizeof(se_msg_hdr);
   switch(ntohl(hdr->mtype)) {
      case(SE_MSG_OPEN_CONNECTION_DATA):
      case(SE_MSG_OPEN_CONNECTION):
      {
         se_open_connection_msg *msg = (se_open_connection_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string   *tstruct    = (se_msg_string *)&(msg->data);
         int             datasize    = 0;

         // datasize includes the string as well as the rest of the se_msg_string
         datasize    = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;

         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_open_connection_msg) +
                       datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_OPEN_CONNECTION_REPLY):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_open_connection_reply_msg);
         break;
      }
      case(SE_MSG_CLOSE_CONNECTION):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_close_connection_msg);
         break;
      }
      case(SE_MSG_CLOSE_CONNECTION_REPLY):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_close_connection_reply_msg);
         break;
      }
      case(SE_MSG_NEW_CLIENT):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_new_client_msg);
         break;
      }
      case(SE_MSG_DIE):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_die_msg);
         break;
      }
      case(SE_MSG_PING):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_ping_msg);
         break;
      }
      case(SE_MSG_PING_REPLY):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_ping_reply_msg);
         break;
      }
      case(SE_MSG_GET_DATA):
      case(SE_MSG_GET):
      {
         se_get_msg    *msg      = (se_get_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string *tstruct  = (se_msg_string *)&(msg->data);
         int            datasize = 0;

         // datasize includes the string as well as the rest of the se_msg_string
         datasize  = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_get_msg) +
                      datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_GET_REPLY):
      {
         se_get_reply_msg *msg     = (se_get_reply_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_get_reply_msg) +
                      datasize - sizeof(void *);
         break;
      }
       case(SE_MSG_GET_DATA_REPLY):
      {
         se_get_reply_msg *msg     = (se_get_reply_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_get_reply_msg) +
                      datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_SET):
      {
         se_set_msg *msg = (se_set_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the strings as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;


         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_msg) +
                       datasize - sizeof(void *);
         break;
      }
       case(SE_MSG_SET_DATA):
      {
         se_set_msg *msg = (se_set_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the strings as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;

         int fileread = access("/tmp/sysevent_debug", F_OK);
         if (fileread == 0)
         {
             char buf[256] = {0};
             snprintf(buf,sizeof(buf),"echo fname %s: %d >> /tmp/sys_d.txt",__FUNCTION__,datasize);
             system(buf);
         }
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_msg) +
                       datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_SET_REPLY):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_reply_msg);
         break;
      }
      case(SE_MSG_SET_UNIQUE):
      {
         se_set_unique_msg *msg = (se_set_unique_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the strings as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;


         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_unique_msg) +
                       datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_SET_UNIQUE_REPLY):
      {
         se_set_unique_reply_msg *msg     = (se_set_unique_reply_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_set_unique_reply_msg) +
                      datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_ITERATE_GET):
      {
         se_iterate_get_msg    *msg      = (se_iterate_get_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string *tstruct  = (se_msg_string *)&(msg->data);
         int            datasize = 0;

         // datasize includes the string as well as the rest of the se_msg_string
         datasize  = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_iterate_get_msg) +
                      datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_ITERATE_GET_REPLY):
      {
         se_iterate_get_reply_msg *msg     = (se_iterate_get_reply_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_iterate_get_reply_msg) +
                      datasize - sizeof(void *);
         break;
      }

      case(SE_MSG_DEL_UNIQUE):
      {
         se_del_unique_msg *msg = (se_del_unique_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;

         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_del_unique_msg) +
                       datasize - sizeof(void *);
         break;
      }

      case(SE_MSG_NEXT_ITERATOR_GET):
      {
         se_iterate_get_iterator_msg *msg     = (se_iterate_get_iterator_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_iterate_get_iterator_msg) +
                      datasize - sizeof(void *);
         break;
      }

      case(SE_MSG_NEXT_ITERATOR_GET_REPLY):
      {
         se_iterate_get_iterator_reply_msg *msg     = (se_iterate_get_iterator_reply_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_iterate_get_iterator_reply_msg) +
                      datasize - sizeof(void *);
         break;
      }

      case(SE_MSG_SET_OPTIONS):
      {
         se_set_options_msg *msg = (se_set_options_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the subject string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;

         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_options_msg) +
                       datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_SET_OPTIONS_REPLY):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_options_reply_msg);
         break;
      }
      case(SE_MSG_SET_ASYNC_ACTION):
      {
         se_set_async_action_msg *msg = (se_set_async_action_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct    = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;
         unsigned int i;
         for (i=0; i<(unsigned int)ntohl(msg->num_params); i++) {
            tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
            string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
            datasize   += string_len;
         }

         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_async_action_msg) +
                       datasize - sizeof(void *);
         break;
      }
      case (SE_MSG_SET_ASYNC_MESSAGE):
      {
         se_set_async_message_msg *msg = (se_set_async_message_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct    = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes the subject string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize   += string_len;

         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_async_message_msg) +
                       datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_SET_ASYNC_REPLY):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_set_async_reply_msg);
         break;
      }
      case(SE_MSG_REMOVE_ASYNC):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_remove_async_msg);
         break;
      }
      case(SE_MSG_REMOVE_ASYNC_REPLY):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_remove_async_reply_msg);
         break;
      }
      case(SE_MSG_SEND_NOTIFICATION_DATA):
      case(SE_MSG_SEND_NOTIFICATION):
      {
         se_send_notification_msg *msg = (se_send_notification_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes two string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_send_notification_msg) +
                      datasize - sizeof(void *);

         if(SE_MSG_SEND_NOTIFICATION_DATA == ntohl(hdr->mtype))
         {
             int fileread = access("/tmp/sysevent_debug", F_OK);
             if (fileread == 0)
             {
                 char buf[256] = {0};
                 snprintf(buf,sizeof(buf),"echo SE_MSG_SEND_NOTIFICATION_DATA fname %s: %d msgbytes %d >> /tmp/sys_d.txt",__FUNCTION__,datasize,msg_bytes);
                 system(buf);
             }
         }
         break;
      }
      case(SE_MSG_RUN_EXTERNAL_EXECUTABLE_DATA):
      case(SE_MSG_RUN_EXTERNAL_EXECUTABLE):
      {
         se_run_executable_msg *msg = (se_run_executable_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes two string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_run_executable_msg) + 
                     datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_NOTIFICATION_DATA):
      case(SE_MSG_NOTIFICATION):
      {
         se_notification_msg *msg = (se_notification_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string    *tstruct = (se_msg_string *)&(msg->data);
         int              datasize = 0;
         int              string_len;

         // datasize includes two string as well as the rest of the se_msg_string
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;
         tstruct    = (se_msg_string *) (((char *)tstruct) + string_len);
         string_len = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;
         datasize  += string_len;

         if(SE_MSG_NOTIFICATION_DATA == ntohl(hdr->mtype))
         {
             int fileread = access("/tmp/sysevent_debug", F_OK);
             if (fileread == 0)
             {
                 char buf[256] = {0};
                 snprintf(buf,sizeof(buf),"echo SE_MSG_NOTIFICATION_DATA fname %s: %d >> /tmp/sys_d.txt",__FUNCTION__,datasize);
                 system(buf);
             }
         }

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_notification_msg) +
                      datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_SHOW_DATA_ELEMENTS):
      {
         se_show_data_elements_msg    *msg      = (se_show_data_elements_msg *) SE_MSG_HDR_2_BODY(hdr);
         se_msg_string *tstruct  = (se_msg_string *)&(msg->data);
         int            datasize = 0;

         // datasize includes the string as well as the rest of the se_msg_string
         datasize  = ntohl(tstruct->size) + SE_MSG_STRING_OVERHEAD;

         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_get_msg) +
                      datasize - sizeof(void *);
         break;
      }
      case(SE_MSG_EXECUTE_SERIALLY):
      {
         msg_bytes = sizeof(se_msg_hdr) + sizeof(se_run_serially_msg);
         break;
      }
      case(SE_MSG_DEBUG):
      {
         msg_bytes   = sizeof(se_msg_hdr) + sizeof(se_debug_msg);
         break;
      }

      default:
#ifdef ENABLE_LIBSYSEVENT_DEBUG
printf("Unhandled case statement in SE_msg_hdr_mbytes_fixup 0x%x (%d)\n", hdr->mtype, hdr->mtype);
#endif
         break;
   }

   hdr->mbytes = htonl(msg_bytes);
   return(0);
}

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
char *SE_msg_prepare (char *buf, const unsigned int bufsize, const int mtype, const token_t sender)
{
   if (NULL == buf || sizeof(se_msg_hdr) > bufsize) {
      return(NULL);
   }
   if (0 == mtype) {
      return(NULL);
   }

   se_msg_hdr *hdr  = (se_msg_hdr *)buf;
   char       *body = SE_MSG_HDR_2_BODY(hdr);
   
   hdr->poison = htonl(MSG_DELIMITER);
   hdr->mbytes = htonl(sizeof(se_msg_hdr));
   hdr->mtype  = htonl(mtype);
   hdr->sender_token = htonl(sender);

   return(body);
}

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
int SE_msg_receive (int fd, char *replymsg, unsigned int *replymsg_size, token_t *who)
{
   int error = 0;
   int msgtype;
   msgtype = msg_receive_internal(fd, replymsg, replymsg_size, who, NULL, &error);
   if (SE_MSG_NONE == msgtype) {
#ifdef RUNTIME_DEBUG
      FILE *fp = fopen(debug_filename, "a+");
      if (NULL != fp) {
         if (0 == error) {
            fprintf(fp, "SE_msg_receive Got SE_MSG_NONE for %s using fd %d\n",
                         NULL==g_name ? "unknown" : g_name, fd);
         } else {
            fprintf(fp, "SE_msg_receive error (%d) %s for %s using fd %d\n",
                         error, strerror(error), NULL==g_name ? "unknown" : g_name, fd);
         }
         fclose(fp);
      }
#endif  // RUNTIME_DEBUG
   }
   return(msgtype);
}

/*
 * Procedure     : SE_minimal_blocking_msg_receive
 * Purpose       : Receive a message from a ready fd without long blocking
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
 *   This call will return SE_MSG_NONE if there is not a message (almost) immediately there
 */
int SE_minimal_blocking_msg_receive (int fd, char *replymsg, unsigned int *replymsg_size, token_t *who)
{
   struct timeval tv;
   tv.tv_sec=1;
   tv.tv_usec=0;
   int error = 0;
   int msgtype;
   msgtype = msg_receive_internal(fd, replymsg, replymsg_size, who, &tv, &error);
   if (SE_MSG_NONE == msgtype) {
#ifdef RUNTIME_DEBUG
      FILE *fp = fopen(debug_filename, "a+");
      if (NULL != fp) {
         if (0 == error) {
            // syseventd is a special case. We dont need to emit this statement because it alreadys does so if needed
            if (NULL == g_name || 0 != strcmp("syseventd", g_name)) {
               fprintf(fp, "SE_minimal_blocking_msg_receive Got SE_MSG_NONE for %s using fd %d\n",
                            NULL==g_name ? "unknown" : g_name, fd);
            }
         } else {
            fprintf(fp, "SE_minimal_blocking_msg_receive error (%d) %s for %s using fd %d\n",
                         error, strerror(error), NULL==g_name ? "unknown" : g_name, fd);
         }
         fclose(fp);
      }
#endif  // RUNTIME_DEBUG
   }
   return(msgtype);
}

/*
 * Procedure     : SE_msg_send
 * Purpose       : Send a message to the sysevent daemon
 * Parameters    :
 *    fd            : The file descriptor to send to
 *    sendmsg       : The message to send
 * Return Code   :
 *    0             : Sucess
 *   !0             : Error
 */ 
int SE_msg_send (int fd, char *sendmsg)
{
   se_msg_hdr *msg_hdr = (se_msg_hdr *)sendmsg;
   SE_msg_hdr_mbytes_fixup(msg_hdr);

   // keep track of the number of bytes in the msg (including the header)
   int bytes_to_write = ntohl(msg_hdr->mbytes);
   se_buffer send_msg_buffer;
   if (bytes_to_write + sizeof(se_msg_footer) > sizeof(send_msg_buffer)) {
      return(-2);
   } else {
      memcpy(send_msg_buffer, sendmsg, bytes_to_write);

      // add a transport message footer to help ensure message integrity during transport
      se_msg_footer footer;
      footer.poison = htonl(MSG_DELIMITER);
      memcpy(((char *)send_msg_buffer)+bytes_to_write, &footer, sizeof(footer));
      bytes_to_write += sizeof(footer);
   }

   // try to write a maximum of num_retries
   int bytes_sent = 0;
   int num_retries = 3;
   int rc;
   while (0 < bytes_to_write && 0 < num_retries) {
      rc = write(fd, send_msg_buffer+bytes_sent, bytes_to_write);
      if (0 < rc) {
         num_retries = 3;
         bytes_to_write -= rc;
         bytes_sent+=rc;
      } else if (0 == rc) {
         num_retries--;
      } else {
         struct timespec sleep_time;
         sleep_time.tv_sec = 0;
         sleep_time.tv_nsec  = 100000000;  // .1 secs
         nanosleep(&sleep_time, NULL);
         num_retries--;
      }
   }

   if (0 == bytes_to_write) {
      return(0);
   } else {
      return(-1);
   }
}

int SE_msg_send_data (int fd, char *sendmsg,int msgsize)
{
   se_msg_hdr *msg_hdr = (se_msg_hdr *)sendmsg;
   char buf_t[256] = {0};
   int fileread = access("/tmp/sysevent_debug", F_OK);
   SE_msg_hdr_mbytes_fixup(msg_hdr);

   // keep track of the number of bytes in the msg (including the header)
   int bytes_to_write = ntohl(msg_hdr->mbytes);
   unsigned int bin_size = sysevent_get_binmsg_maxsize();
   char *send_msg_buffer = sendmsg;
   if (bytes_to_write + sizeof(se_msg_footer) > bin_size) {
      return(-2);
   } else {
       if (fileread == 0)
       {
           snprintf(buf_t,sizeof(buf_t),"echo fname %s: bytestowrite %d before msg copy >> /tmp/sys_d.txt",__FUNCTION__,bytes_to_write);
           system(buf_t);

       }
      // add a transport message footer to help ensure message integrity during transport
      se_msg_footer footer;
      footer.poison = htonl(MSG_DELIMITER);
       memcpy(((char *)send_msg_buffer)+bytes_to_write, &footer, sizeof(footer));
      bytes_to_write += sizeof(footer);
   }

   // try to write a maximum of num_retries
   int bytes_sent = 0;
   int num_retries = 3;
   int rc;
   if (fileread == 0)
   {
       snprintf(buf_t,sizeof(buf_t),"echo fname before write %s: %d >> /tmp/sys_d.txt",__FUNCTION__, bytes_to_write);
       system(buf_t);
   }
   while (0 < bytes_to_write && 0 < num_retries) {
      rc = write(fd, send_msg_buffer+bytes_sent, bytes_to_write);
      if (0 < rc) {
         num_retries = 3;
         bytes_to_write -= rc;
         bytes_sent+=rc;
      } else if (0 == rc) {
         num_retries--;
      } else {
         struct timespec sleep_time;
         sleep_time.tv_sec = 0;
         sleep_time.tv_nsec  = 100000000;  // .1 secs
         nanosleep(&sleep_time, NULL);
         num_retries--;
      }
   }
   if (fileread == 0)
   {
       snprintf(buf_t,sizeof(buf_t),"echo fname after write %s: %d >> /tmp/sys_d.txt",__FUNCTION__, bytes_to_write);
       system(buf_t);
   }
   if (0 == bytes_to_write) {
      return(0);
   } else {
      return(-1);
   }
}


/*
 * Procedure     : SE_msg_send_receive
 * Purpose       : Send a message to the sysevent daemon and wait for a reply
 * Parameters    :
 *    fd            : The file descriptor to send to
 *    sendmsg       : The message to send
 *    replymsg      : A buffer to place the reply message
 *    replymsg_size : The size of the reply message buffer
 *                    On return this contains the number of bytes used
 * Return Code   :
 *    SE_MSG_NONE   : error
 *    >0            : The type of message returned
 * Notes         :
 *    This function will NOT block until it receives a reply
 */ 
int SE_msg_send_receive (int fd, char *sendmsg, char *replymsg, unsigned int *replymsg_size)
{
   int rc = SE_msg_send(fd, sendmsg);
   if (0 != rc) {
      return(SE_MSG_NONE);
   } 

   token_t who;
   int error = 0;
   int msgtype;
   struct timeval tv;
   tv.tv_sec=5;
   tv.tv_usec=0;

   msgtype = msg_receive_internal(fd, replymsg, replymsg_size, &who, &tv, &error);
   if (SE_MSG_NONE == msgtype) {
#ifdef RUNTIME_DEBUG
      FILE *fp = fopen(debug_filename, "a+");
      if (NULL != fp) {
         if (0 == error) {
            fprintf(fp, "SE_msg_send_receive Got SE_MSG_NONE for %s using fd %d\n",
                         NULL==g_name ? "unknown" : g_name, fd);
         } else {
            fprintf(fp, "SE_msg_send_receive error (%d) %s for %s using fd %d\n",
                         error, strerror(error), NULL==g_name ? "unknown" : g_name, fd);
         }
         fclose(fp);
      }
#endif  // RUNTIME_DEBUG
   }
   return(msgtype); 
}

int SE_msg_send_receive_data (int fd, char *sendmsg, int sendmsg_size, char *replymsg, unsigned int *replymsg_size)
{
   int rc = SE_msg_send_data(fd, sendmsg,sendmsg_size);
   if (0 != rc) {
      return(SE_MSG_NONE);
   }

   token_t who;
   int error = 0;
   int msgtype;
   struct timeval tv;
   tv.tv_sec=5;
   tv.tv_usec=0;

   msgtype = msg_receive_internal(fd, replymsg, replymsg_size, &who, &tv, &error);
   if (SE_MSG_NONE == msgtype) {
#ifdef RUNTIME_DEBUG
      FILE *fp = fopen(debug_filename, "a+");
      if (NULL != fp) {
         if (0 == error) {
            fprintf(fp, "SE_msg_send_receive Got SE_MSG_NONE for %s using fd %d\n",
                         NULL==g_name ? "unknown" : g_name, fd);
         } else {
            fprintf(fp, "SE_msg_send_receive error (%d) %s for %s using fd %d\n",
                         error, strerror(error), NULL==g_name ? "unknown" : g_name, fd);
         }
         fclose(fp);
      }
#endif  // RUNTIME_DEBUG
   }
   return(msgtype);
}

/*
 * Procedure     : SE_strerror
 * Purpose       : Return a string version of an error code
 * Parameters    :
 *    error         : The error
 * Return Code   :
 *   The string.
 *   Do NOT save this string. It has no lifespan
 */ 
char *SE_strerror (int error)
{
   switch(error) {
      case(0):
         return("Sysevent OK");
      case(ERR_NAME_TOO_LONG):
         return("Name used is too long");
      case(ERR_BAD_PORT):
         return("Illegal port provided");
      case(ERR_INCORRECT_VERSION):
         return("Incorrect version provided");
      case(ERR_BAD_DESTINATION):
         return("Unresolvable IP Address provided");
      case(ERR_MSG_PREPARE):
         return("Unable to prepare a msg to Server");
      case(ERR_SOCKET_OPEN):
         return("Error while opening socket");
      case(ERR_CANNOT_CONNECT):
         return("Error connecting to Server");
      case(ERR_REGISTRATION_REFUSED):
         return("Server refused registration");
      case(ERR_ALREADY_CONNECTED):
         return("Already connected to the server");
      case(ERR_NOT_CONNECTED):
         return("Not connected to the server");
      case(ERR_NO_PING_REPLY):
         return("No ping reply from the server");
      case(ERR_INSUFFICIENT_ROOM):
         return("Not enough room in the buffer. Value truncated");
      case(ERR_BAD_BUFFER):
         return("Bad input buffer size");
      case(ERR_SERVER_ERROR):
         return("Server returned bad data");
      case(ERR_MSG_TOO_LONG):
         return("Message is too long to be sent");
      case(ERR_VALUE_NOT_FOUND):
         return("Server could not find requested value");
      case(ERR_CANNOT_SET_STRING):
         return("Cannot set string in SE_msg");
      case(ERR_PARAM_NOT_FOUND):
         return("Cannot find some parameter");
      case(ERR_OUT_OF_ORDER):
         return("Unexpected message order");
      case(ERR_CORRUPTED):
         return("Message corrupted in transit");
      case(ERR_DUPLICATE_MSG):
         return("Message is duplicate of already seen message");
      default:
         return("Unknown Error");
   }
}

/*
 * Procedure     : SE_print_mtype
 * Purpose       : Return a string for a msgtype
 * Parameters    :
 *    mtype         : The msg type
 * Return Code   :
 *   The string.
 *   Do NOT save this string. It has no lifespan
 */ 
char *SE_print_mtype (int mtype)
{
   // callers to this api have their mtype in host order
   // we need to switch it to network order for mtype2str
   return (mtype2str(htonl(mtype)));
}

/*
 * Procedure     : connect_to_local_sysevent_daemon
 * Purpose       : connect to the sysevent_daemon using Unix Domain Sockets
 * Parameters    :
 *    target         : the name of the connection
 *    sockfd        : Upon return the open connected socket
 */
static int connect_to_local_sysevent_daemon(char *target, int* sockfd)
{
   *sockfd = -1;

   struct sockaddr_un se_server_addr;

   // open a UDS socket 
   if ( 0 > (*sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) ) {
      return(ERR_SOCKET_OPEN);
   }

   // can't use SOCK_NONBLOCK in socket call for some reason, so use fcntl
   int oldflags = fcntl (*sockfd, F_GETFL, 0);
   if (0 > oldflags) {
     fcntl (*sockfd, F_SETFL, O_NONBLOCK);
   } else {
     oldflags |= O_NONBLOCK;
     fcntl (*sockfd, F_SETFL, oldflags);
   }

   // connect to the server 
   int address_length;
   se_server_addr.sun_family = AF_UNIX;
   address_length = sizeof(se_server_addr.sun_family) +
                     sprintf(se_server_addr.sun_path, "%s", target);

   int rc;
   int num_tries = NUM_CONNECT_ATTEMPTS;
   while (num_tries) {
      rc = connect(*sockfd, (struct sockaddr *) &se_server_addr, address_length);
      if ( 0 > rc ) {
         if (EINPROGRESS == errno) {
            num_tries--;
            if (num_tries) {
#ifdef RUNTIME_DEBUG
               FILE *fp = fopen(debug_filename, "a+");
               if (NULL != fp) {
                  fprintf(fp, "connect_to_local_sysevent_daemon connect timed out using fd %d. Retrying\n",
                                *sockfd);
                  fclose(fp);
               }
#endif  // RUNTIME_DEBUG
               struct timespec sleep_time;
               sleep_time.tv_sec = 0;
               sleep_time.tv_nsec  = 200000000;  // .2 secs
               nanosleep(&sleep_time, NULL);
            } else {
#ifdef RUNTIME_DEBUG
               FILE *fp = fopen(debug_filename, "a+");
               if (NULL != fp) {
                  fprintf(fp, "connect_to_local_sysevent_daemon connect timed out using fd %d\n",
                                *sockfd);
                  fclose(fp);
               }
#endif  // RUNTIME_DEBUG
               return(ERR_CANNOT_CONNECT);
            }
         } else {
#ifdef RUNTIME_DEBUG
            FILE *fp = fopen(debug_filename, "a+");
            if (NULL != fp) {
               fprintf(fp, "connect_to_local_sysevent_daemon connect error (%d) %s using fd %d\n",
                             errno, strerror(errno), *sockfd);
               fclose(fp);
            }
#endif  // RUNTIME_DEBUG
            return(ERR_CANNOT_CONNECT);
         }
      } else {
         num_tries = 0;
      } 
   }
 
   return(0);
}

/*
 * Procedure     : connect_to_sysevent_daemon
 * Purpose       : connect to the sysevent_daemon
 * Parameters    :
 *    ip            : ip address to connect to. 
 *                    This may be dots and dashes or hostname
 *    port          : port to connect to
 *    sockfd        : Upon return the open connected socket
 *
 * Notes  : This procedure overrides loopback/default port with
 *           a Unix Domain Sockets connection
 */
static int connect_to_sysevent_daemon(char *ip, unsigned short port, int* sockfd)
{
   *sockfd = -1;
   struct sockaddr_in  se_server_addr;
#ifndef NO_IPV6
   struct sockaddr_in6 ipv6_server_addr;
#endif

   // make sure this is an unsigned short
   port &= 0x0000FFFF;

   int rc;
   struct addrinfo myaddr , *result, *rp;
   memset ( &myaddr , 0 , sizeof ( myaddr ));
   myaddr.ai_family   = AF_UNSPEC;
   myaddr.ai_socktype = SOCK_STREAM;
   myaddr.ai_flags    = AI_PASSIVE;

   if (0 != (rc = getaddrinfo(ip, NULL, &myaddr, &result)) ){
             return(ERR_BAD_DESTINATION);
   } else {
      int connected = 0;
      for (rp = result; !connected && NULL != rp; rp = rp->ai_next) {
#ifndef NO_IPV6
         if (AF_INET6 == rp->ai_family) {
            struct in6_addr     ipv6_server_in_addr;
            ipv6_server_in_addr = ((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;

            // is this destined for loopback interface
            // if so, redirect it to uds
            if ( 0 == memcmp(&in6addr_loopback, &ipv6_server_in_addr, sizeof(ipv6_server_in_addr)) &&
                 SE_SERVER_WELL_KNOWN_PORT == port ) {
              freeaddrinfo(result);
              return (connect_to_local_sysevent_daemon(UDS_PATH, sockfd));
              break;
            } else {
               // open a TCP socket 
               if ( 0 <= (*sockfd = socket(AF_INET6, SOCK_STREAM, 0)) ) {

                  // can't use SOCK_NONBLOCK in socket call for some reason, so use fcntl
                  int oldflags = fcntl (*sockfd, F_GETFL, 0);
                  if (0 > oldflags) {
                    fcntl (*sockfd, F_SETFL, O_NONBLOCK);
                  } else {
                    oldflags |= O_NONBLOCK;
                    fcntl (*sockfd, F_SETFL, oldflags);
                  }

                  // connect to server
                  memset(&ipv6_server_addr, 0, sizeof(ipv6_server_addr));
                  ipv6_server_addr.sin6_family     = AF_INET6;
                  ipv6_server_addr.sin6_addr       = ipv6_server_in_addr;
                  ipv6_server_addr.sin6_port       = htons(port); 

                  int rc;
                  int num_tries = NUM_CONNECT_ATTEMPTS;
                  while (num_tries) {
                      rc = connect(*sockfd, (struct sockaddr *) &ipv6_server_addr, sizeof(ipv6_server_addr));
                      if ( 0 > rc ) {
                         if (EINPROGRESS == errno) {
                            num_tries--;
                            if (num_tries) {
#ifdef RUNTIME_DEBUG
                               FILE *fp = fopen(debug_filename, "a+");
                               if (NULL != fp) {
                                  fprintf(fp, "connect_to_sysevent_daemon connect (ipv6) timed out using fd %d. Retrying\n",
                                                *sockfd);
                                  fclose(fp);
                               }
#endif  // RUNTIME_DEBUG
                               struct timespec sleep_time;
                               sleep_time.tv_sec = 0;
                               sleep_time.tv_nsec  = 200000000;  // .2 secs
                               nanosleep(&sleep_time, NULL);
                            } else {
#ifdef RUNTIME_DEBUG
                               FILE *fp = fopen(debug_filename, "a+");
                               if (NULL != fp) {
                                  fprintf(fp, "connect_to_sysevent_daemon connect (ipv6) out using fd %d\n",
                                                *sockfd);
                                  fclose(fp);
                               }
#endif  // RUNTIME_DEBUG
                               close(*sockfd);
                               *sockfd   = -1;
                               num_tries = 0;
                            }
                         } else {
#ifdef RUNTIME_DEBUG
                            FILE *fp = fopen(debug_filename, "a+");
                            if (NULL != fp) {
                               fprintf(fp, "connect_to_sysevent_daemon connect (ipv6) (%d) %s using fd %d\n",
                                   errno, strerror(errno), *sockfd);
                               fclose(fp);
                            }
#endif  // RUNTIME_DEBUG
                            close(*sockfd);
                            *sockfd   = -1;
                            num_tries = 0;
                         }
                      } else {
                         connected=1;
                         num_tries = 0;
                      }
                  }
               }
            }
         }
#endif

         if (AF_INET == rp->ai_family) {
            struct in_addr      se_server_in_addr;
            se_server_in_addr = ((struct sockaddr_in *)rp->ai_addr)->sin_addr;

            // is this destined for loopback interface
            // if so, redirect it to uds
            if ( INADDR_LOOPBACK           == ntohl(se_server_in_addr.s_addr)  && 
                 SE_SERVER_WELL_KNOWN_PORT == port) {
              freeaddrinfo(result);
              return (connect_to_local_sysevent_daemon(UDS_PATH, sockfd));
              break;
            } else {
               // open a TCP socket 
               if ( 0 <= (*sockfd = socket(AF_INET, SOCK_STREAM, 0)) ) {

                  // can't use SOCK_NONBLOCK in socket call for some reason, so use fcntl
                  int oldflags = fcntl (*sockfd, F_GETFL, 0);
                  if (0 > oldflags) {
                    fcntl (*sockfd, F_SETFL, O_NONBLOCK);
                  } else {
                    oldflags |= O_NONBLOCK;
                    fcntl (*sockfd, F_SETFL, oldflags);
                  }

                  // connect to server
                  memset(&se_server_addr, 0, sizeof(se_server_addr));
                  se_server_addr.sin_family      = AF_INET;
                  se_server_addr.sin_addr.s_addr = se_server_in_addr.s_addr;
                  se_server_addr.sin_port        = htons(port); 

                  int rc;
                  int num_tries = NUM_CONNECT_ATTEMPTS;
                  while (num_tries) {
                      rc = connect(*sockfd, (struct sockaddr *) &se_server_addr, sizeof(se_server_addr));
                      if ( 0 > rc ) {
                         if (EINPROGRESS == errno) {
                            num_tries--;
                            if (num_tries) {
#ifdef RUNTIME_DEBUG
                               FILE *fp = fopen(debug_filename, "a+");
                               if (NULL != fp) {
                                  fprintf(fp, "connect_to_sysevent_daemon connect (ipv4) timed out using fd %d. Retrying\n",
                                                *sockfd);
                                  fclose(fp);
                               }
#endif  // RUNTIME_DEBUG
                               struct timespec sleep_time;
                               sleep_time.tv_sec = 0;
                               sleep_time.tv_nsec  = 200000000;  // .2 secs
                               nanosleep(&sleep_time, NULL);
                            } else {
#ifdef RUNTIME_DEBUG
                               FILE *fp = fopen(debug_filename, "a+");
                               if (NULL != fp) {
                                  fprintf(fp, "connect_to_sysevent_daemon connect (ipv4) timed out using fd %d\n",
                                                *sockfd);
                                  fclose(fp);
                               }
#endif  // RUNTIME_DEBUG
                               close(*sockfd);
                               *sockfd   = -1;
                               num_tries = 0;
                            }
                         } else {
#ifdef RUNTIME_DEBUG
                            FILE *fp = fopen(debug_filename, "a+");
                            if (NULL != fp) {
                               fprintf(fp, "connect_to_sysevent_daemon connect (ipv4) error (%d) %s using fd %d\n",
                                   errno, strerror(errno), *sockfd);
                               fclose(fp);
                            }
#endif  // RUNTIME_DEBUG
                            close(*sockfd);
                            *sockfd   = -1;
                            num_tries = 0;
                         }
                      } else {
                         connected=1;
                         num_tries = 0;
                      }
                  }
               }
            }
         }
      }

      freeaddrinfo(result);
      if ( !connected ) {
         *sockfd = -1;
         return(ERR_CANNOT_CONNECT);
      }
   }
 
   return(0);
}

/*
 =========================================================================
                  SE Client APIs
 
 The following apis are used by se clients.

 sysevent_open must be called prior to using any of the other APIs.

 =========================================================================
 */

/*
 * Procedure     : sysevent_open
 * Purpose       : Connect to the sysevent daemon
 * Parameters    :
 *    ip            : ip address to connect to. 
 *                    This may be dots and dashes or hostname
 *    port          : port to connect to
 *    version       : version of client
 *    id            : name of client
 *    token         : opaque id for future contact
 * Return Code   :
 *    The file descriptor to use in future calls
 *    -1 if error
 */ 
int sysevent_open (char *ip, unsigned short port, int version, char *id, token_t *token)
{
   int                sockfd = -1;

   *token = TOKEN_NULL;

   // make sure this is an unsigned short
   port &= 0x0000FFFF;

   // Ensure that the input parameters are sane
   // Limit the id to 40 chars
   if (NULL == id      || 
      0 == strlen(id)  || 
     40 <=  strlen(id) ) {
      return(ERR_NAME_TOO_LONG);
   }
   if ( 0 >= port) {
      return(ERR_BAD_PORT);
   }
   if ( 1 != version) {
      return(ERR_INCORRECT_VERSION);
   }

   // prepare a open connection message
   se_buffer              send_msg_buffer;
   se_open_connection_msg *send_msg_body;
   if (NULL == 
      (send_msg_body = (se_open_connection_msg *)SE_msg_prepare (send_msg_buffer, 
                                                          sizeof(send_msg_buffer), 
                                                          SE_MSG_OPEN_CONNECTION, TOKEN_NULL)) ) {
      return(ERR_MSG_PREPARE); 
   }

   init_libsysevent(id);

   int rc;
   rc = connect_to_sysevent_daemon(ip, port, &sockfd); 
   if (0 != rc) {
      if(sockfd != -1) { /*RDKB-7132, CID-33536, close socket handle before exit*/
        close(sockfd);
      }
      return(rc);
   } 

   int   remaining_buf_bytes;
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_open_connection_msg);

   // set up the rest of the message header
   send_msg_body->version  = htonl(version);

   // set up the message body
   char *send_data_ptr = (char *)&(send_msg_body->data);
   SE_msg_add_string(send_data_ptr, remaining_buf_bytes, id);

   // send registration msg and receive the reply
   se_buffer                     reply_msg_buffer;
   se_open_connection_reply_msg *reply_msg_body = (se_open_connection_reply_msg *)reply_msg_buffer;
   unsigned int                  reply_msg_size = sizeof(reply_msg_buffer);

   int reply_msg_type = SE_msg_send_receive(sockfd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &reply_msg_size);

   // see if the registration was acceptable
   if (SE_MSG_OPEN_CONNECTION_REPLY != reply_msg_type || 0 != ntohl(reply_msg_body->status)) {
      close(sockfd);
      return(ERR_REGISTRATION_REFUSED);
   } 

   // it was acceptable and now we have an id to use in future messages
   // this is an opaque value
   
   *token = ntohl(reply_msg_body->token_id);
   return(sockfd);
}

int sysevent_open_data (char *ip, unsigned short port, int version, char *id, token_t *token)
{
   int                sockfd = -1;

   *token = TOKEN_NULL;

   // make sure this is an unsigned short
   port &= 0x0000FFFF;

   // Ensure that the input parameters are sane
   // Limit the id to 40 chars
   if (NULL == id      || 
      0 == strlen(id)  || 
     40 <=  strlen(id) ) {
      return(ERR_NAME_TOO_LONG);
   }
   if ( 0 >= port) {
      return(ERR_BAD_PORT);
   }
   if ( 1 != version) {
      return(ERR_INCORRECT_VERSION);
   }

   // prepare a open connection message
   se_buffer              send_msg_buffer;
   se_open_connection_msg *send_msg_body;
   if (NULL == 
      (send_msg_body = (se_open_connection_msg *)SE_msg_prepare (send_msg_buffer, 
                                                          sizeof(send_msg_buffer), 
                                                          SE_MSG_OPEN_CONNECTION_DATA, TOKEN_NULL)) ) {
      return(ERR_MSG_PREPARE); 
   }

   init_libsysevent(id);

   int rc;
   rc = connect_to_sysevent_daemon(ip, port, &sockfd); 
   if (0 != rc) {
      if(sockfd != -1) { /*RDKB-7132, CID-33536, close socket handle before exit*/
        close(sockfd);
      }
      return(rc);
   } 

   int   remaining_buf_bytes;
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_open_connection_msg);

   // set up the rest of the message header
   send_msg_body->version  = htonl(version);

   // set up the message body
   char *send_data_ptr = (char *)&(send_msg_body->data);
   SE_msg_add_string(send_data_ptr, remaining_buf_bytes, id);

   // send registration msg and receive the reply
   se_buffer                     reply_msg_buffer;
   se_open_connection_reply_msg *reply_msg_body = (se_open_connection_reply_msg *)reply_msg_buffer;
   unsigned int                  reply_msg_size = sizeof(reply_msg_buffer);
   int reply_msg_type = SE_msg_send_receive(sockfd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &reply_msg_size);

   // see if the registration was acceptable
   if (SE_MSG_OPEN_CONNECTION_REPLY != reply_msg_type || 0 != ntohl(reply_msg_body->status)) {
      close(sockfd);
      return(ERR_REGISTRATION_REFUSED);
   } 

   // it was acceptable and now we have an id to use in future messages
   // this is an opaque value
   
   *token = ntohl(reply_msg_body->token_id);
   char buf[256] = {0};
   snprintf(buf,sizeof(buf),"echo fname %s: fd %d >> /tmp/sys_d.txt",__FUNCTION__,sockfd);
   system(buf);


   return(sockfd);
}

int sysevent_local_open_data (char *target, int version, char *id, token_t *token)
{
   int                sockfd = -1;

   *token = TOKEN_NULL;

   // Ensure that the input parameters are sane
   // Limit the id to 40 chars
   if (NULL == id      || 
      0 == strlen(id)  || 
     40 <=  strlen(id) ) {
      return(ERR_NAME_TOO_LONG);
   }
   if ( 1 != version) {
      return(ERR_INCORRECT_VERSION);
   }

   // prepare a open connection message
   se_buffer              send_msg_buffer;
   se_open_connection_msg *send_msg_body;
   if (NULL == 
      (send_msg_body = (se_open_connection_msg *)SE_msg_prepare (send_msg_buffer, 
                                                          sizeof(send_msg_buffer), 
                                                          SE_MSG_OPEN_CONNECTION_DATA, TOKEN_NULL)) ) {
      return(ERR_MSG_PREPARE); 
   }

   init_libsysevent(id);

   int rc;
   rc = connect_to_local_sysevent_daemon(target, &sockfd);
   if (0 != rc) {
      if (sockfd != -1) { /*RDKB-7132, CID-33207, close socket handle before exit*/
         close(sockfd);
      }
      return(rc);
   }

   int   remaining_buf_bytes;
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_open_connection_msg);

   // set up the rest of the message header
   send_msg_body->version  = htonl(version);

   // set up the message body
   char *send_data_ptr = (char *)&(send_msg_body->data);
   SE_msg_add_string(send_data_ptr, remaining_buf_bytes, id);

   // send registration msg and receive the reply
   se_buffer                     reply_msg_buffer;
   se_open_connection_reply_msg *reply_msg_body = (se_open_connection_reply_msg *)reply_msg_buffer;
   unsigned int                  reply_msg_size = sizeof(reply_msg_buffer);

   int reply_msg_type = SE_msg_send_receive(sockfd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &reply_msg_size);

   // see if the registration was acceptable
   if (SE_MSG_OPEN_CONNECTION_REPLY != reply_msg_type || 0 != ntohl(reply_msg_body->status)) {
      close(sockfd);
      return(ERR_REGISTRATION_REFUSED);
   } 

   // it was acceptable and now we have an id to use in future messages
   // this is an opaque value
   
   *token = ntohl(reply_msg_body->token_id);
   char buf[256] = {0};
   snprintf(buf,sizeof(buf),"echo fname %s: fd %d >> /tmp/sys_d.txt",__FUNCTION__,sockfd);
   system(buf);

   return(sockfd);
}


/*
 * Procedure     : sysevent_local_open
 * Purpose       : Connect to the sysevent daemon using Unix Domain Socket
 * Parameters    :
 *   target         : the name of the uds to connect to   
 *    version       : version of client
 *    id            : name of client
 *    token         : opaque id for future contact
 * Return Code   :
 *    The file descriptor to use in future calls
 *    -1 if error
 */ 
int sysevent_local_open (char *target, int version, char *id, token_t *token)
{
   int                sockfd = -1;

   *token = TOKEN_NULL;

   // Ensure that the input parameters are sane
   // Limit the id to 40 chars
   if (NULL == id      || 
      0 == strlen(id)  || 
     40 <=  strlen(id) ) {
      return(ERR_NAME_TOO_LONG);
   }
   if ( 1 != version) {
      return(ERR_INCORRECT_VERSION);
   }

   // prepare a open connection message
   se_buffer              send_msg_buffer;
   se_open_connection_msg *send_msg_body;
   if (NULL == 
      (send_msg_body = (se_open_connection_msg *)SE_msg_prepare (send_msg_buffer, 
                                                          sizeof(send_msg_buffer), 
                                                          SE_MSG_OPEN_CONNECTION, TOKEN_NULL)) ) {
      return(ERR_MSG_PREPARE); 
   }

   init_libsysevent(id);

   int rc;
   rc = connect_to_local_sysevent_daemon(target, &sockfd);
   if (0 != rc) {
      if (sockfd != -1) { /*RDKB-7132, CID-33207, close socket handle before exit*/
         close(sockfd);
      }
      return(rc);
   }

   int   remaining_buf_bytes;
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_open_connection_msg);

   // set up the rest of the message header
   send_msg_body->version  = htonl(version);

   // set up the message body
   char *send_data_ptr = (char *)&(send_msg_body->data);
   SE_msg_add_string(send_data_ptr, remaining_buf_bytes, id);

   // send registration msg and receive the reply
   se_buffer                     reply_msg_buffer;
   se_open_connection_reply_msg *reply_msg_body = (se_open_connection_reply_msg *)reply_msg_buffer;
   unsigned int                  reply_msg_size = sizeof(reply_msg_buffer);

   int reply_msg_type = SE_msg_send_receive(sockfd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &reply_msg_size);

   // see if the registration was acceptable
   if (SE_MSG_OPEN_CONNECTION_REPLY != reply_msg_type || 0 != ntohl(reply_msg_body->status)) {
      close(sockfd);
      return(ERR_REGISTRATION_REFUSED);
   } 

   // it was acceptable and now we have an id to use in future messages
   // this is an opaque value
   
   *token = ntohl(reply_msg_body->token_id);
   return(sockfd);
}

/*
 * Procedure     : sysevent_close
 * Purpose       : Close a connection to the sysevent daemon
 * Parameters    : 
 *   fd             : the file descriptor to close
 *   token          : Server provided opaque value
 * Return Code   :
 *     0            : disconnected
 *    !0            : some error
 */ 
int sysevent_close (int fd, token_t token)
{
   // prepare an close_connection message
   se_buffer                send_msg_buffer;
   se_close_connection_msg *send_message_body;

   if (NULL ==
      (send_message_body = (se_close_connection_msg *)SE_msg_prepare (send_msg_buffer, 
                                                                sizeof(send_msg_buffer), 
                                                                SE_MSG_CLOSE_CONNECTION, 
                                                                token)) ) {
      return(ERR_MSG_PREPARE);
   } 
   send_message_body->reserved = (void *)htonl(0);

   // send the message, but we dont really care about the reply
   SE_msg_send(fd, send_msg_buffer);

   close(fd);

   return(0);
}

/*
 * Procedure     : sysevent_ping
 * Purpose       : Ping a connection to the sysevent daemon
 * Parameters    : 
 *   fd             : the file descriptor to ping
 *   token          : Server provided opaque value
 * Return Code   :
 *     0            : ping msg sent
 *    !0            : some error
 *  Note : We only told sysevent daemon to ping reply. We are not handling the 
 *         reply itself. That is up to the caller
 */ 
int sysevent_ping (int fd, token_t token)
{
   // prepare an ping message
   se_buffer     send_msg_buffer;
   se_ping_msg *send_message_body;

   if (NULL ==
      (send_message_body = (se_ping_msg *)SE_msg_prepare (send_msg_buffer, 
                                                                sizeof(send_msg_buffer), 
                                                                SE_MSG_PING, 
                                                                token)) ) {
      return(ERR_MSG_PREPARE);
   } 
   send_message_body->reserved = (void *)htonl(0);

   // send the message, but we dont really care about the reply
   SE_msg_send(fd, send_msg_buffer);

   return(0);
}

/*
 * Procedure     : sysevent_ping_test
 * Purpose       : Ping a connection to the sysevent daemon
 *                 AND wait for reply
 * Parameters    : 
 *   fd             : the file descriptor to ping
 *   token          : Server provided opaque value
 *   tv             : A timeval describing how long to wait
 * Return Code   :  
 *     0            : ping reply msg received
 *    !0            : some error
 *
 * Note          : This is a blocking call. It is the developer's responsibility
 *                 to specify how long to block. NULL tv = infinite block
 */
int sysevent_ping_test (int fd, token_t token, struct timeval* tv)
{
   if ( 0 != sysevent_ping (fd, token) ) {
      return(-1);
   }

   se_buffer         reply_msg_buffer;
   token_t           return_token;
   unsigned int      replymsg_size   = sizeof(reply_msg_buffer);
   int               error = 0;
   int msgtype;
   msgtype = msg_receive_internal(fd, reply_msg_buffer, &replymsg_size, &return_token, tv, &error); 
   if (SE_MSG_NONE == msgtype) {
#ifdef RUNTIME_DEBUG
      FILE *fp = fopen(debug_filename, "a+");
      if (NULL != fp) {
         if (0 == error) {
            fprintf(fp, "sysevent_ping_test Got SE_MSG_NONE for %s using fd %d\n",
                         NULL==g_name ? "unknown" : g_name, fd);
         } else {
            fprintf(fp, "sysevent_ping_test (%d) %s for %s using fd %d\n",
                         error, strerror(error), NULL==g_name ? "unknown" : g_name, fd);
         }
         fclose(fp);
      }
#endif  // RUNTIME_DEBUG
   }

   if (SE_MSG_PING_REPLY != msgtype) {
      return(-1);
   }

   return(0);
}


/*
 * Procedure     : sysevent_debug
 * Purpose       : Set sysevent daemon debug level
 * Parameters    :
 *   ip             : the name or ip address of the sysevent daemon to send msg to
 *   port           : the port of the sysevent daemon to send to
 *   level          : Debug level to set to
 * Return Code   :
 *     0            : debug msg sent
 *    !0            : some error
 */
int sysevent_debug (char *ip, unsigned short port, int level)
{
   int                sockfd = -1;

   // make sure this is an unsigned short
   port &= 0x0000FFFF;
   if ( 0 >= port) {
      return(ERR_BAD_PORT);
   }

   // prepare a debug message
   se_buffer    send_msg_buffer;
   se_debug_msg *send_msg_body;
   if (NULL ==
      (send_msg_body = (se_debug_msg *)SE_msg_prepare (send_msg_buffer,
                                                          sizeof(send_msg_buffer),
                                                          SE_MSG_DEBUG, TOKEN_NULL)) ) {
      return(ERR_MSG_PREPARE);
   }
   // set up the rest of the message header
   send_msg_body->level = htonl(level);

   int rc;
   rc = connect_to_sysevent_daemon(ip, port, &sockfd);
   if (0 != rc) {
      if (sockfd != -1) { /*RDKB-7132, CID-33027, close socket handle before exit*/
         close(sockfd);
      }
      return(rc);
   }

   SE_msg_send(sockfd, send_msg_buffer);
   close(sockfd);

   return(0);
}

/*
 * Procedure     : sysevent_get
 * Purpose       : Send a get to the sysevent daemon and receive reply
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    inbuf         : A null terminated string which is the thing to get
 *    inbytes       : The length of the string not counting terminating null
 *    outbuf        : A buffer to hold returned value
 *    outbytes      : The maximum number of bytes in outbuf
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 * Notes        : 
 *    If outbuf is not big enough to hold the reply value, then the value will
 *    be truncated to fit. An error of ERR_INSUFFICIENT_ROOM will be returned.
 *    The value will always be NULL terminated, so the outbuf must contain 
 *    enough bytes for the return value as well as the NULL byte.
 */
int sysevent_get (const int fd, const token_t token, const char *inbuf, char *outbuf, int outbytes) 
{
   se_buffer    send_msg_buffer;
   se_get_msg   *send_msg_body;
   int          inbytes;

   if (NULL == inbuf || NULL == outbuf || 0 == (inbytes = strlen(inbuf)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 == outbytes) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      outbuf[0] = '\0';
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_get_msg once it has been populated 
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_get_msg) +
                       SE_string2size(inbuf) - sizeof(void *);

   // if the se_get_msg will be too long to fit into our buffer
   // then abort
   if (send_msg_size >= sizeof(send_msg_buffer)) {
      outbuf[0] = '\0';
      return(ERR_MSG_TOO_LONG);
   } 

   // prepare the header of the se_get_msg
   if (NULL == 
      (send_msg_body = (se_get_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_GET, token)) ) {
      outbuf[0] = '\0';
      return(ERR_MSG_PREPARE); 
   } 

   // prepare the body of the se_get_msg
   int  remaining_buf_bytes;
   char *send_data_ptr = (char *)&(send_msg_body->data);
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_get_msg);
   int strsize    = SE_msg_add_string(send_data_ptr, 
                                      remaining_buf_bytes,
                                      inbuf);
   if (0 == strsize) {
      outbuf[0] = '\0';
      return(ERR_CANNOT_SET_STRING);
   }

   // send get msg and receive the get_msg_reply
   se_buffer        reply_msg_buffer;
   se_get_reply_msg *reply_msg_body = (se_get_reply_msg *)reply_msg_buffer;
   unsigned int     reply_msg_size  = sizeof(reply_msg_buffer);

   pthread_mutex_lock(&g_client_fd_mutex);
   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &reply_msg_size);
   pthread_mutex_unlock(&g_client_fd_mutex);

   // see if the get was received and returned
   if (SE_MSG_GET_REPLY != reply_msg_type) {
      outbuf[0] = '\0';
      if (SE_MSG_ERRORED == reply_msg_type) {
         return(ERR_CORRUPTED);
      } else {
         return(ERR_SERVER_ERROR);
      }
   } 
   if (0 != reply_msg_body->status) {
      outbuf[0] = '\0';
      return(ERR_SERVER_ERROR);
   } 

   // extract the subject and value from the return message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *reply_data_ptr;

   // we ignore the subject field, but future enhancements could use it
   reply_data_ptr  = (char *)&(reply_msg_body->data);
   subject_str     = SE_msg_get_string(reply_data_ptr, &subject_bytes);
   reply_data_ptr += subject_bytes;
   value_str       =  SE_msg_get_string(reply_data_ptr, &value_bytes);

   // value_bytes is a sysevent strings (and include size info)
   // so change it to the strlen
   value_bytes   = strlen(value_str);

   // make sure the caller has enough room in their buffer for the
   // value. If not truncate and notify them via return code
   if (value_bytes > (outbytes - 1)) {
      memcpy(outbuf, value_str, outbytes-1);
      outbuf[outbytes-1] = '\0';
      return(ERR_INSUFFICIENT_ROOM);
   } else {
      memcpy(outbuf, value_str, value_bytes);
      outbuf[value_bytes] = '\0';
      return(0);
   }
}

/*
 * Procedure     : sysevent_get_data
 * Purpose       : Send a get to the sysevent daemon and receive reply
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    inbuf         : A null terminated string which is the thing to get
 *    inbytes       : The length of the string not counting terminating null
 *    outbuf        : A buffer to hold returned value
 *    outbytes      : The maximum number of bytes in outbuf
 *    bufsizecopied : The actual number of bytes copied into outbuf
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 * Notes        :
 *    If outbuf is not big enough to hold the reply value, then the value will
 *    be truncated to fit. An error of ERR_INSUFFICIENT_ROOM will be returned.
 *    The value will always be NULL terminated, so the outbuf must contain
 *    enough bytes for the return value as well as the NULL byte.
 */
int sysevent_get_data(const int fd, const token_t token, const char *inbuf, char *outbuf, int outbytes, int *bufsizecopied)
{
   se_buffer    send_msg_buffer;
   se_get_msg   *send_msg_body;
   int          inbytes;

   if (NULL == inbuf || NULL == outbuf || 0 == (inbytes = strlen(inbuf)) || bufsizecopied == NULL ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 == outbytes) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      outbuf[0] = '\0';
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_get_msg once it has been populated 
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_get_msg) +
                       SE_string2size(inbuf) - sizeof(void *);

   // if the se_get_msg will be too long to fit into our buffer
   // then abort
   if (send_msg_size >= sizeof(send_msg_buffer)) {
      outbuf[0] = '\0';
      return(ERR_MSG_TOO_LONG);
   } 
   // prepare the header of the se_get_msg
   if (NULL == 
      (send_msg_body = (se_get_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_GET_DATA, token)) ) {
      outbuf[0] = '\0';
      return(ERR_MSG_PREPARE); 
   } 

   // prepare the body of the se_get_msg
   int  remaining_buf_bytes;
   char *send_data_ptr = (char *)&(send_msg_body->data);
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_get_msg);
   int strsize    = SE_msg_add_string(send_data_ptr, 
                                      remaining_buf_bytes,
                                      inbuf);
   if (0 == strsize) {
      outbuf[0] = '\0';
      return(ERR_CANNOT_SET_STRING);
   }

   unsigned int bin_size = sysevent_get_binmsg_maxsize();
   // send get msg and receive the get_msg_reply
   char *reply_msg_buffer = (char *)malloc(bin_size);
   if (!reply_msg_buffer)
         return ERR_SERVER_ERROR;
   se_get_reply_msg *reply_msg_body = (se_get_reply_msg *)reply_msg_buffer;
   unsigned int     reply_msg_size  = bin_size;

   pthread_mutex_lock(&g_client_fd_mutex);
   int reply_msg_type = SE_msg_send_receive_data(fd, 
                                            send_msg_buffer,
                                            sizeof(send_msg_buffer),                                          
                                            reply_msg_buffer, &reply_msg_size);
   pthread_mutex_unlock(&g_client_fd_mutex);

   // see if the get was received and returned
   if (SE_MSG_GET_DATA_REPLY != reply_msg_type) {
      outbuf[0] = '\0';
      free(reply_msg_buffer);
      if (SE_MSG_ERRORED == reply_msg_type) {
         return(ERR_CORRUPTED);
      } else {
         return(ERR_SERVER_ERROR);
      }
   } 
   if (0 != reply_msg_body->status) {
      outbuf[0] = '\0';
      free(reply_msg_buffer);
      return(ERR_SERVER_ERROR);
   } 

   // extract the subject and value from the return message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *reply_data_ptr;

   // we ignore the subject field, but future enhancements could use it
   reply_data_ptr  = (char *)&(reply_msg_body->data);
   subject_str     = SE_msg_get_string(reply_data_ptr, &subject_bytes);
   reply_data_ptr += subject_bytes;
   value_str       =  SE_msg_get_data(reply_data_ptr, &value_bytes);

   // make sure the caller has enough room in their buffer for the
   // value. If not truncate and notify them via return code
   if (value_bytes > outbytes) {
      memcpy(outbuf, value_str, outbytes);
      *bufsizecopied = outbytes;
      free(reply_msg_buffer);
      return(ERR_INSUFFICIENT_ROOM);
   } else {
      memcpy(outbuf, value_str, value_bytes);
      *bufsizecopied = value_bytes;
      free(reply_msg_buffer);
      return(0);
   }
}


/*
 * Procedure     : sysevent_set_private
 * Purpose       : Send a set to the sysevent daemon and receive reply
 *                 A set may change the value of a tuple
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    name          : A null terminated string which is the tuple to set
 *    value         : A null terminated string which is the value to set tuple to, or NULL
 *    source        : The source of the message
 *    tid           : A transaction id for the set
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 */
static int sysevent_set_private (const int fd, const token_t token, const char *name, const char *value, const int source, const int tid) 
{
   se_buffer     send_msg_buffer;
   se_set_msg   *send_msg_body;
   int           subbytes;
   int           valbytes;

   if (NULL == name || 0 == (subbytes = strlen(name)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   valbytes = (NULL == value ? 0 : strlen(value));

   // calculate the size of the se_set_msg once it has been populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_set_msg) +
                       SE_string2size(name) +
                       (NULL == value ? 0 : SE_string2size(value)) - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   } 

   if (NULL == 
      (send_msg_body = (se_set_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_SET, token)) ) {
      return(ERR_MSG_PREPARE); 
   }

   // prepare the body of the se_set_msg
   int   remaining_buf_bytes;
   send_msg_body->source  = htonl(source);
   send_msg_body->tid     = htonl(tid);
   char *send_data_ptr    = (char *)&(send_msg_body->data);
   remaining_buf_bytes    = sizeof(send_msg_buffer);
   remaining_buf_bytes   -=sizeof(se_msg_hdr);
   remaining_buf_bytes   -= sizeof(se_set_msg); 
   int strsize            = SE_msg_add_string(send_data_ptr,
                                        remaining_buf_bytes,
                                        name);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }
   
   remaining_buf_bytes -= strsize;
   send_data_ptr       += strsize;
      strsize              = SE_msg_add_string(send_data_ptr,
                                         remaining_buf_bytes,
                                         value);

#ifndef SET_REPLY_REQUIRED
   SE_msg_send(fd, send_msg_buffer);
#else
   // send set msg and receive the reply
   se_buffer         reply_msg_buffer;
   se_set_reply_msg *reply_msg_body = (se_set_reply_msg *)reply_msg_buffer;
   unsigned int     replymsg_size   = sizeof(reply_msg_buffer);

   pthread_mutex_lock(&g_client_fd_mutex);
   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &replymsg_size);
   pthread_mutex_unlock(&g_client_fd_mutex);
   // see if the set was received and returned
   if (SE_MSG_SET_REPLY != reply_msg_type) {
      return(ERR_SERVER_ERROR);
   } 
   if (0 != reply_msg_body->status) {
      return(ERR_SERVER_ERROR);
   } 
#endif

   return(0);
}

/*
 * Procedure     : sysevent_set
 * Purpose       : Send a set to the sysevent daemon
 *                 A set may change the value of a tuple
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    name          : A null terminated string which is the tuple to set
 *    value         : A null terminated string which is the value to set tuple to, or NULL
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 */
int sysevent_set (const int fd, const token_t token, const char *name, const char *value,  int conf_req) 
{
   return(sysevent_set_private(fd,token,name,value,0,0));
}

static int sysevent_set_data_private (const int fd, const token_t token, const char *name, const char *value, const int value_length,const int source, const int tid) 
{
//   se_buffer_data     send_msg_buffer;
   char *send_msg_buffer =  NULL;
   se_set_msg   *send_msg_body;
   int           subbytes;
   int           valbytes;
   int fileread = access("/tmp/sysevent_debug", F_OK);

   if (NULL == name || 0 == (subbytes = strlen(name)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   valbytes = (NULL == value ? 0 : value_length);

   // calculate the size of the se_set_msg once it has been populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_set_msg) +
                       SE_string2size(name) +
                       (NULL == value ? 0 : value_length) - sizeof(void *) ;
    unsigned int bin_size = sysevent_get_binmsg_maxsize();

   if (send_msg_size >= bin_size) {
      return(ERR_MSG_TOO_LONG);
   } 

   send_msg_buffer = (char*)malloc(bin_size);

   if (NULL == send_msg_buffer)
       return ERR_MSG_PREPARE;

   if (NULL == 
      (send_msg_body = (se_set_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     bin_size, 
                                                     SE_MSG_SET_DATA, token)) ) {
      free(send_msg_buffer);
      return(ERR_MSG_PREPARE); 
   }

   // prepare the body of the se_set_msg
   int   remaining_buf_bytes;
   send_msg_body->source  = htonl(source);
   send_msg_body->tid     = htonl(tid);
   char *send_data_ptr    = (char *)&(send_msg_body->data);
   remaining_buf_bytes    = bin_size;
   remaining_buf_bytes   -=sizeof(se_msg_hdr);
   remaining_buf_bytes   -= sizeof(se_set_msg); 
   int strsize            = SE_msg_add_string(send_data_ptr,
                                        remaining_buf_bytes,
                                        name);
   if (0 == strsize) {
       free(send_msg_buffer);
      return(ERR_CANNOT_SET_STRING);
   }
   
   remaining_buf_bytes -= strsize;
   send_data_ptr       += strsize;
      strsize              = SE_msg_add_data(send_data_ptr,
                                         remaining_buf_bytes,
                                         value,
                                         value_length);

      if (fileread == 0)
      {
          char buf[256] = {0};
          snprintf(buf,sizeof(buf),"echo fname %s: %d >> /tmp/sys_d.txt",__FUNCTION__,value_length);
          system(buf);
      }
#ifndef SET_REPLY_REQUIRED
   SE_msg_send_data(fd, send_msg_buffer,bin_size);
#else
   // send set msg and receive the reply
   se_buffer         reply_msg_buffer;
   se_set_reply_msg *reply_msg_body = (se_set_reply_msg *)reply_msg_buffer;
   unsigned int     replymsg_size   = sizeof(reply_msg_buffer);

   pthread_mutex_lock(&g_client_fd_mutex);
   int reply_msg_type = SE_msg_send_receive_data(fd, 
           send_msg_buffer, 
           bin_size,
           reply_msg_buffer, &replymsg_size);
   pthread_mutex_unlock(&g_client_fd_mutex);
   // see if the set was received and returned
   if (SE_MSG_SET_REPLY != reply_msg_type) {
       free(send_msg_buffer);
       return(ERR_SERVER_ERROR);
   } 
   if (0 != reply_msg_body->status) {
       free(send_msg_buffer);
       return(ERR_SERVER_ERROR);
   }
#endif
   free(send_msg_buffer);
   return(0);
}



/*
 * Procedure     : sysevent_set_data
 * Purpose       : Send a set to the sysevent daemon
 *                 A set may change the value of a tuple
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    name          : A null terminated string which is the tuple to set
 *    value         : buffer holds binary data which is the value to set tuple to, or NULL
 *    value_length  : actual size of binary data buffer
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 */
int sysevent_set_data (const int fd, const token_t token, const char *name, const char *value, int value_length) 
{
   return (sysevent_set_data_private(fd,token,name,value,value_length,0,0));
}

unsigned int sysevent_get_binmsg_maxsize()
{
    FILE *fp = fopen(SE_MAX_MSG_DATA_SIZE_READ_FILE,"r");
    if (NULL != fp) {
        unsigned int value = 0;
        fscanf(fp, "%u",&value);
        fclose(fp);
        if (value != 0)
        {    
            return value + 1024 /* additional 1k is headermsg*/;
        }
    }
    return SE_MAX_MSG_DATA_SIZE + 1024 /* additional 1k is headermsg*/;
}

/*
 * Procedure     : sysevent_unset
 * Purpose       : Send a set to the sysevent daemon
 *                 A set with NULL value of a tuple to clear existing value from volatile memory.
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    name          : A null terminated string which is the tuple to set
 *    value         : NULL by default
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 */
int sysevent_unset (const int fd, const token_t token, const char *name)
{
    return(sysevent_set_private(fd,token,name,NULL,0,0));
}


/*
 * Procedure     : sysevent_set_with_tid
 * Purpose       : Send a set to the sysevent daemon
 *                 A set may change the value of a tuple
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    name          : A null terminated string which is the tuple to set
 *    value         : A null terminated string which is the value to set tuple to, or NULL
 *    source        : The source of the original message
 *    tid           : transaction id 
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 */
int sysevent_set_with_tid (const int fd, const token_t token, const char *name, const char *value, const int source, const int tid) 
{
   return(sysevent_set_private(fd,token,name,value,source,tid));
}

/*
 * Procedure     : sysevent_set_unique
 * Purpose       : Send a set unique to the sysevent daemon 
 *                 The daemon will create a new tuple and return its name
 *                 A set unique will change the value of a tuple
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    name          : A null terminated string which is the tuple to set
 *    value         : A null terminated string which is the value to set tuple to, or NULL
 *    outbuf        : A buffer to hold returned value
 *    outbytes      : The maximum number of bytes in outbuf
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 */
int sysevent_set_unique (const int fd, const token_t token, const char *name, const char *value, char *outbuf, int outbytes) 
{
   se_buffer            send_msg_buffer;
   se_set_unique_msg   *send_msg_body;
   int                  subbytes;
   int                  valbytes;

   if (NULL == name || 0 == (subbytes = strlen(name)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   valbytes = (NULL == value ? 0 : strlen(value));

   // calculate the size of the se_set_unique_msg once it has been populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_set_unique_msg) +
                       SE_string2size(name) +
                       (NULL == value ? 0 : SE_string2size(value)) - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   } 

   if (NULL == 
      (send_msg_body = (se_set_unique_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_SET_UNIQUE, token)) ) {
      return(ERR_MSG_PREPARE); 
   }

   // prepare the body of the se_set_unique_msg
   int   remaining_buf_bytes;
   char *send_data_ptr  = (char *)&(send_msg_body->data);
   remaining_buf_bytes  = sizeof(send_msg_buffer);
   remaining_buf_bytes -=sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_set_unique_msg); 
   int strsize          = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      name);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }
   
   remaining_buf_bytes -= strsize;
   send_data_ptr       += strsize;
      strsize              = SE_msg_add_string(send_data_ptr,
                                         remaining_buf_bytes,
                                         value);

   // send set unique msg and receive the reply
   se_buffer         reply_msg_buffer;
   se_set_unique_reply_msg *reply_msg_body = (se_set_unique_reply_msg *)reply_msg_buffer;
   unsigned int     replymsg_size   = sizeof(reply_msg_buffer);


   pthread_mutex_lock(&g_client_fd_mutex);
   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &replymsg_size);
   pthread_mutex_unlock(&g_client_fd_mutex);

   // see if the set unique was received and returned
   if (SE_MSG_SET_UNIQUE_REPLY != reply_msg_type) {
      return(ERR_SERVER_ERROR);
   } 
   if (0 != reply_msg_body->status) {
      return(ERR_SERVER_ERROR);
   } 

   // extract the subject from the return message data
   int   subject_bytes;
   char *subject_str;
   char *reply_data_ptr;

   reply_data_ptr  = (char *)&(reply_msg_body->data);
   subject_str     = SE_msg_get_string(reply_data_ptr, &subject_bytes);

   // subject_bytes is a sysevent strings (and include size info)
   // so change it to the strlen
   subject_bytes = strlen(subject_str);

   // make sure the caller has enough room in their buffer for the
   // value. If not truncate and notify them via return code
   if (subject_bytes > (outbytes - 1)) {
      memcpy(outbuf, subject_str, outbytes-1);
      outbuf[outbytes-1] = '\0';
      return(ERR_INSUFFICIENT_ROOM);
   } else {
      memcpy(outbuf, subject_str, subject_bytes);
      outbuf[subject_bytes] = '\0';
      return(0);
   }
   return(0);
}

/*
 * Procedure     : sysevent_get_unique
 * Purpose       : Send a iterate_get to the sysevent daemon and receive reply
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    inbuf         : A null terminated string which is the thing to get
 *    subjectbuf        : A buffer to hold returned unique name 
 *    subjectbytes      : The maximum number of bytes in subjectbuf
 *    valuebuf        : A buffer to hold returned value     
 *    valuebytes      : The maximum number of bytes in valuebuf
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 * Notes        : 
 *    If outbuf is not big enough to hold the reply value, then the value will
 *    be truncated to fit. An error of ERR_INSUFFICIENT_ROOM will be returned.
 *    The value will always be NULL terminated, so the outbuf must contain 
 *    enough bytes for the return value as well as the NULL byte.
 */
int sysevent_get_unique (const int fd, const token_t token, const char *inbuf, unsigned int *iterator, char *subjectbuf, int subjectbytes, char *valuebuf, int valuebytes) 
{
   se_buffer             send_msg_buffer;
   se_iterate_get_msg   *send_msg_body;
   int                   inbytes;

   if (NULL == inbuf || NULL == subjectbuf || NULL == valuebuf || 0 == (inbytes = strlen(inbuf)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 == subjectbytes || 0 == valuebytes) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      valuebuf[0]   = '\0';
      subjectbuf[0] = '\0';
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_iterate_get_msg once it has been populated 
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_iterate_get_msg) +
                       SE_string2size(inbuf) - sizeof(void *);

   // if the se_get_msg will be too long to fit into our buffer
   // then abort
   if (send_msg_size >= sizeof(send_msg_buffer)) {
      valuebuf[0]   = '\0';
      subjectbuf[0] = '\0';
      return(ERR_MSG_TOO_LONG);
   } 

   // prepare the header of the sysevent_get_unique
   if (NULL == 
      (send_msg_body = (se_iterate_get_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_ITERATE_GET, token)) ) {
      valuebuf[0]   = '\0';
      subjectbuf[0] = '\0';
      return(ERR_MSG_PREPARE); 
   } 

   // prepare the body of the se_get_msg
   send_msg_body->iterator=htonl(*iterator); 
   int  remaining_buf_bytes;
   char *send_data_ptr = (char *)&(send_msg_body->data);
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_get_msg);
   int strsize    = SE_msg_add_string(send_data_ptr, 
                                      remaining_buf_bytes,
                                      inbuf);
   if (0 == strsize) {
      valuebuf[0]   = '\0';
      subjectbuf[0] = '\0';
      return(ERR_CANNOT_SET_STRING);
   }

   se_buffer        reply_msg_buffer;
   se_iterate_get_reply_msg *reply_msg_body = (se_iterate_get_reply_msg *)reply_msg_buffer;
   unsigned int     reply_msg_size  = sizeof(reply_msg_buffer);

   pthread_mutex_lock(&g_client_fd_mutex);

   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &reply_msg_size);

   pthread_mutex_unlock(&g_client_fd_mutex);

   if (SE_MSG_ITERATE_GET_REPLY != reply_msg_type) {
      valuebuf[0]   = '\0';
      subjectbuf[0] = '\0';
      return(ERR_SERVER_ERROR);
   } 
   if (0 != reply_msg_body->status) {
      valuebuf[0]   = '\0';
      subjectbuf[0] = '\0';
      return(ERR_SERVER_ERROR);
   } 

   // update the iterator
   *iterator = ntohl(reply_msg_body->iterator);

   // extract the subject and value from the return message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *reply_data_ptr;

   reply_data_ptr  = (char *)&(reply_msg_body->data);
   subject_str     = SE_msg_get_string(reply_data_ptr, &subject_bytes);
   reply_data_ptr += subject_bytes;
   value_str       =  SE_msg_get_string(reply_data_ptr, &value_bytes);
   int rc = 0;

   // value_bytes and subject_bytes are sysevent strings (and include size info)
   // so change them to the strlen
   subject_bytes = strlen(subject_str);
   value_bytes   = strlen(value_str);

   // make sure the caller has enough room in their buffer for the
   // subject. If not truncate and notify them via return code
   if (subject_bytes > (subjectbytes - 1)) {
      memcpy(subjectbuf, subject_str, subjectbytes-1);
      subjectbuf[subjectbytes-1] = '\0';
      rc = ERR_INSUFFICIENT_ROOM;
   } else {
      memcpy(subjectbuf, subject_str, subject_bytes);
      subjectbuf[subject_bytes] = '\0';
   }
   // make sure the caller has enough room in their buffer for the
   // value. If not truncate and notify them via return code
   if (value_bytes > (valuebytes - 1)) {
      memcpy(valuebuf, value_str, valuebytes-1);
      valuebuf[valuebytes-1] = '\0';
      return(ERR_INSUFFICIENT_ROOM);
   } else {
      memcpy(valuebuf, value_str, value_bytes);
      valuebuf[value_bytes] = '\0';
      return(rc);
   }
}

/*
 * Procedure     : sysevent_del_unique
 * Purpose       : Send a delete of unique element from its namespace
 * Parameters    :
 *    fd            the connection descriptor
 *    token         : The server provided opaque id of the client
 *    name          : A null terminated string which is the namespace of the thing to delete
 *    iterator      : A iterator which is describing the element to delete within the namespace
 * Return Code   :
 *    0             : 
 */   
int sysevent_del_unique(const int fd, const token_t token, const char *name, unsigned int *iterator)
{
   se_buffer            send_msg_buffer;
   se_del_unique_msg   *send_msg_body;

   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_del_unique_msg once it has been populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_del_unique_msg) +
                       SE_string2size(name) - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   }

   if (NULL ==
      (send_msg_body = (se_del_unique_msg *)SE_msg_prepare (send_msg_buffer,
                                                     sizeof(send_msg_buffer),
                                                     SE_MSG_DEL_UNIQUE, token)) ) {
      return(ERR_MSG_PREPARE);
   }

   // prepare the body of the msg
   send_msg_body->iterator=htonl(*iterator);
   int   remaining_buf_bytes;
   char *send_data_ptr  = (char *)&(send_msg_body->data);
   remaining_buf_bytes  = sizeof(send_msg_buffer);
   remaining_buf_bytes -=sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_set_unique_msg);
   int strsize          = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      name);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }
  

   SE_msg_send(fd, send_msg_buffer);

   return(0);
}

/*
 * Procedure     : sysevent_get_next_iterator
 * Purpose       : Get the next iterator for a namespace
 * Parameters    :
 *    fd            the connection descriptor
 *    token         : The server provided opaque id of the client
 *    name          : A null terminated string which is the namespace 
 *    iterator      : A iterator which is describing the current iterator. Initially set to 0
 *                    On return it contains the next iterator to the namespace
 * Return Code   :
 *    0             :
 */   
int sysevent_get_next_iterator(const int fd, const token_t token, const char *name, unsigned int *iterator)
{
   se_buffer                      send_msg_buffer;
   se_iterate_get_iterator_msg   *send_msg_body;
   int                            inbytes;

   if (NULL == name || 0 == (inbytes = strlen(name)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_iterate_get_iterator_msg once it has been populated 
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_iterate_get_msg) +
                       SE_string2size(name) - sizeof(void *);

   // if the se_get_msg will be too long to fit into our buffer
   // then abort
   if (send_msg_size >= sizeof(send_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   } 

   if (NULL == 
      (send_msg_body = (se_iterate_get_iterator_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_NEXT_ITERATOR_GET, token)) ) {
      return(ERR_MSG_PREPARE); 
   } 

   // prepare the body of the se_get_msg
   send_msg_body->iterator=htonl(*iterator); 
   int  remaining_buf_bytes;
   char *send_data_ptr = (char *)&(send_msg_body->data);
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_get_msg);
   int strsize    = SE_msg_add_string(send_data_ptr, 
                                      remaining_buf_bytes,
                                      name);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }

   se_buffer        reply_msg_buffer;
   se_iterate_get_iterator_reply_msg *reply_msg_body = (se_iterate_get_iterator_reply_msg *)reply_msg_buffer;
   unsigned int     reply_msg_size  = sizeof(reply_msg_buffer);

   pthread_mutex_lock(&g_client_fd_mutex);
   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &reply_msg_size);

   pthread_mutex_unlock(&g_client_fd_mutex);
   if (SE_MSG_NEXT_ITERATOR_GET_REPLY != reply_msg_type) {
      return(ERR_SERVER_ERROR);
   } 
   if (0 != reply_msg_body->status) {
      return(ERR_SERVER_ERROR);
   } 

   // update the iterator
   *iterator = ntohl(reply_msg_body->iterator);
   return(0);
}


/*
 * Procedure     : sysevent_set_options
 * Purpose       : Send a set options to the sysevent daemon and receive reply
 *                 A set may change the option flags of a tuple
 * Parameters    :
 *    fd            : The connection id
 *    token         : Server provided opaque value
 *    name          : A null terminated string which is the tuple to set
 *    flags         : The flags to set the option to
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 */
int sysevent_set_options (const int fd, const token_t token, const char *name, const unsigned int flags) 
{
   se_buffer             send_msg_buffer;
   se_set_options_msg   *send_msg_body;
   int                   subbytes;

   if (NULL == name || 0 == (subbytes = strlen(name)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_set_options_msg once it has been populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_set_options_msg) +
                       SE_string2size(name) - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   } 

   if (NULL == 
      (send_msg_body = (se_set_options_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_SET_OPTIONS, token)) ) {
      return(ERR_MSG_PREPARE); 
   } else {
      // prepare the body of the se_set_options_msg
      int   remaining_buf_bytes;
      char *send_data_ptr  = (char *)&(send_msg_body->data);
      remaining_buf_bytes  = sizeof(send_msg_buffer);
      remaining_buf_bytes -=sizeof(se_msg_hdr);
      remaining_buf_bytes -= sizeof(se_set_options_msg); 
      int strsize          = SE_msg_add_string(send_data_ptr,
                                         remaining_buf_bytes,
                                         name);
      if (0 == strsize) {
         return(ERR_CANNOT_SET_STRING);
      }
   
      send_msg_body->flags = htonl(flags);
   } 

#ifndef SET_REPLY_REQUIRED
   SE_msg_send(fd, send_msg_buffer);
#else
   pthread_mutex_lock(&g_client_fd_mutex);

   // send set options msg and receive the reply
   se_buffer         reply_msg_buffer;
   se_set_options_reply_msg *reply_msg_body = (se_set_options_reply_msg *)reply_msg_buffer;
   unsigned int     replymsg_size   = sizeof(reply_msg_buffer);

   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, &replymsg_size);

   pthread_mutex_unlock(&g_client_fd_mutex);

   // see if the set was received and returned
   if (SE_MSG_SET_OPTIONS_REPLY != reply_msg_type) {
      return(ERR_SERVER_ERROR);
   } 
   if (0 != reply_msg_body->status) {
      return(ERR_SERVER_ERROR);
   } 
#endif

   return(0);
}

/*
 * Procedure     : sysevent_setcallback
 * Purpose       : Declare a program to run when a given tuple changes value
 * Parameters    :
 *    fd            : The connection id
 *    token         : A Server provided opaque value
 *    flags         : action_flag_t flags to control the activation
 *    subject       : A null terminated string which is the name of the tuple
 *    function      : An execeutable to call when the tuple changes value
 *    numparams     : The number of arguments in the following char**
 *    params        : A list of 0 or more parameters to use when calling the function
 *    async_id      : On return, and id that can be used to cancel the callback
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 * Notes         :
 *    When the tuple changes value the executable will be called with all parameters given
 *    the value of parameters will be either:
 *       the exact string given as a parameter, or
 *       if the parameter begins with $ the return will be the current value
 *         of the trigger by that name. If the trigger does not exist
 *         then "NULL" will be used.
 *    For example, if the subject is trigger1, the function /bin/programA, and
 *    the parameter list is 3 params = trigger1, $trigger3, $trigger1.
 *    Assuming trigger3 has not yet been set,
 *    Then when trigger1 changes to "new_value", a process will be forked to
 *    call  /bin/programA  "trigger1" "NULL" "new_value"
 */
int sysevent_setcallback(const int fd, const token_t token, action_flag_t flags, char *subject, char *function, int numparams, char **params, async_id_t *async_id)
{
   se_buffer                send_msg_buffer;
   se_set_async_action_msg *send_msg_body;
   unsigned int             subbytes; 
   unsigned int             funcbytes;
   unsigned int             parambytes = 0;

   async_id->trigger_id = async_id->action_id = htonl(0);
   if (NULL == subject || NULL == function || 
       0 == strlen(subject) || 0 == strlen(function) ) {
      return(ERR_BAD_BUFFER);
   }
   if (NULL == params && 0 != numparams) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   // figure out how much space the se_msg_strings will take
   if (0 != numparams) {
      int i;
      for (i=0 ; i<numparams; i++) {
         parambytes += SE_string2size(params[i]); 
      }
   }
   subbytes  = SE_string2size(subject);
   funcbytes = SE_string2size(function);

   // calculate the size of the se_set_async_action_msg once it will be populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_set_async_action_msg) +
                       subbytes + funcbytes + parambytes - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) { 
      return(ERR_MSG_TOO_LONG);
   }

   if (NULL ==
      (send_msg_body = (se_set_async_action_msg *)SE_msg_prepare (send_msg_buffer, 
                                                           sizeof(send_msg_buffer), 
                                                           SE_MSG_SET_ASYNC_ACTION, token)) ) {
      return(ERR_MSG_PREPARE);
   } 

   // prepare the message
   send_msg_body->flags          = htonl(flags);
   send_msg_body->num_params     = htonl(numparams);

   int  remaining_buf_bytes;
   char *send_data_ptr = (char *)&(send_msg_body->data);
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_set_async_action_msg);
   int strsize    = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      subject);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }
   remaining_buf_bytes -= strsize;
   send_data_ptr       += strsize;
   strsize              = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      function);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }
   int i;
   for (i=0; i<numparams; i++) {
      remaining_buf_bytes -= strsize;
      send_data_ptr       += strsize;
      strsize              = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      params[i]);
      if (0 == strsize) {
         return(ERR_CANNOT_SET_STRING);
      }
   }

   // send set msg and receive the reply
   se_buffer               reply_msg_buffer;
   se_set_async_reply_msg *reply_msg_body = (se_set_async_reply_msg *)reply_msg_buffer;
   unsigned int  replymsg_size            = sizeof(reply_msg_buffer);

   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, 
                                            &replymsg_size);

   // see if the set was received and returned
   if (SE_MSG_SET_ASYNC_REPLY != reply_msg_type) {
      return(ERR_SERVER_ERROR);
   }
   if (0 != reply_msg_body->status) {
      return(ERR_SERVER_ERROR);
   } else {
      async_id->trigger_id = (reply_msg_body->async_id).trigger_id;
      async_id->action_id = (reply_msg_body->async_id).action_id;
   }

   return(0);
}

/*
 * Procedure    : sysevent_setnotification
 * Purpose      : Request a notification message to be sent when a given tuple changes value
 * Parameters   :
 *   fd            : The connection id
 *   token         : A server generated opaque value
 *   subject       :  A null terminated string which is the name of the tuple
 *   async_id      : On return, and id that can be used to cancel the notification
 *  Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 * Note         : A notification can only be sent to a client which is still connected
 */
int sysevent_setnotification(const int fd, const token_t token, char *subject, async_id_t *async_id)
{
   se_buffer                 send_msg_buffer;
   se_set_async_message_msg *send_msg_body;
   int                       subbytes; 

   async_id->trigger_id = async_id->action_id = htonl(0);
   if (NULL == subject) { 
      return(ERR_BAD_BUFFER);
   }

   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   // figure out how much space the se_msg_strings will take
   subbytes  = SE_string2size(subject);

   // calculate the size of the se_set_async_message_msg once it will be populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_set_async_message_msg) +
                       subbytes - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) { 
      return(ERR_MSG_TOO_LONG);
   }

   if (NULL ==
      (send_msg_body = (se_set_async_message_msg *)SE_msg_prepare (send_msg_buffer, 
                                                           sizeof(send_msg_buffer), 
                                                           SE_MSG_SET_ASYNC_MESSAGE, 
                                                           token)) ) {
      return(ERR_MSG_PREPARE);
   } 

   // prepare the message
   // for now there are no flags
   send_msg_body->flags = htonl(ACTION_FLAG_NONE);

   int  remaining_buf_bytes;
   char *send_data_ptr = (char *)&(send_msg_body->data);
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_set_async_message_msg);
   int strsize    = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      subject);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }

   // send set msg and receive the reply
   se_buffer               reply_msg_buffer;
   se_set_async_reply_msg *reply_msg_body = (se_set_async_reply_msg *)reply_msg_buffer;
   unsigned int  replymsg_size            = sizeof(reply_msg_buffer);

   int reply_msg_type = SE_msg_send_receive(fd, 
                                            send_msg_buffer, 
                                            reply_msg_buffer, 
                                            &replymsg_size);

   // see if the set was received and returned
   if (SE_MSG_SET_ASYNC_REPLY != reply_msg_type) {
      /*
       * For the scenario where this notification request has not yet been acked,
       * however the server already has notifications to send, we just throw
       * away the message, and wait for the real ack. But we wont wait forever
       */
      int i;
      for (i = 0; i<4; i++) {
         token_t from;
         replymsg_size  = sizeof(reply_msg_buffer);
         reply_msg_type = SE_minimal_blocking_msg_receive(fd, reply_msg_buffer, &replymsg_size, &from);
         if (SE_MSG_SET_ASYNC_REPLY == reply_msg_type) {
            goto async_reply_received;
         }
      }
      return(ERR_SERVER_ERROR);
   }

async_reply_received:
   if (0 != reply_msg_body->status) {
      return(ERR_SERVER_ERROR);
   } else {
      async_id->trigger_id = (reply_msg_body->async_id).trigger_id;
      async_id->action_id = (reply_msg_body->async_id).action_id;
   }

   return(0);
}

/*
 * Procedure     : sysevent_rmcallback
 * Purpose       : Remove an callback/notification from a trigger
 * Parameters    :
 *    fd            : The connection id
 *    token         : A server generated opaque value
 *    async_id      : The async id to remove
 * Return Code   :
 *    0             : Async action is removed
 */
int sysevent_rmcallback(const int fd, const token_t token, async_id_t async_id)
{
   se_buffer            send_msg_buffer;
   se_remove_async_msg  *send_msg_body;

   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_remove_async_msg 
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_remove_async_msg) - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) { 
      return(ERR_MSG_TOO_LONG);
   }

   if (NULL ==
      (send_msg_body = (se_remove_async_msg *)SE_msg_prepare (send_msg_buffer, 
                                                           sizeof(send_msg_buffer), 
                                                           SE_MSG_REMOVE_ASYNC, token)) ) {
      return(ERR_MSG_PREPARE);
   } 

   // prepare the message
   (send_msg_body->async_id).trigger_id = async_id.trigger_id;
   (send_msg_body->async_id).action_id  = async_id.action_id;

   // send the message but we dont care about the reply
   SE_msg_send(fd, send_msg_buffer);

   return(0);
}

/*
 * Procedure     : sysevent_rmnotification
 * Purpose       : Remove an callback/notification from a trigger
 * Parameters    :
 *    fd            : The connection id
 *    token         : A server generated opaque value
 *    async_id      : The async id to remove
 * Return Code   :
 *    0             : Async action is removed
 */
int sysevent_rmnotification(const int fd, const token_t token, async_id_t async_id)
{
   return(sysevent_rmcallback(fd, token, async_id));
}


/*
 * Procedure     : sysevent_getnotification
 * Purpose       : Wait for a notification and return the results when received
 * Parameters    :
 *    fd            : The connection id
 *    token         : A server generated opaque value
 *    namebuf       : A buffer to hold the name received
 *    namebytes     : The length of the string not counting terminating null
 *    valbuf        : A buffer to hold returned value
 *    valbytes      : On input the maximum number of bytes in outbuf
 *                    On output the actual number of bytes in outbuf not counting 
 *                    the terminating null
 *    async_id      : The async id of the action
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 * Notes        : 
 *    If a buffer is not big enough to hold the reply value, then the value will
 *    be truncated to fit. An error of ERR_INSUFFICIENT_ROOM will be returned.
 *    The value will always be NULL terminated, so the buffer must contain 
 *    enough bytes for the return value as well as the NULL byte.
 * Notes
 *   This will block
 */
int sysevent_getnotification (const int fd, const token_t token, char *namebuf, int *namebytes, char *valbuf, int *valbytes, async_id_t *async_id) 
{
   if (NULL == namebytes || NULL == valbytes) {
     return(ERR_BAD_BUFFER);
   }

   if (NULL == namebuf || NULL == valbuf) {
      *namebytes = 0;
      *valbytes  = 0;
      return(ERR_BAD_BUFFER);
   }
  if (NULL == async_id) {
      *namebytes = 0;
      *valbytes  = 0;
      return(ERR_PARAM_NOT_FOUND);
  }

   if (0 > fd) {
      *namebytes = 0;
      namebuf[0] = '\0';
      *valbytes = 0;
      valbuf[0] = '\0';
      return(ERR_NOT_CONNECTED);
   }

   if (*namebytes == 0 || *valbytes == 0)
       return ERR_INSUFFICIENT_ROOM;

   
   // receive the notification
   se_buffer            reply_msg_buffer;
   se_notification_msg *reply_msg_body = (se_notification_msg *)reply_msg_buffer;
   unsigned int         reply_msg_size  = sizeof(reply_msg_buffer);
   token_t              from = token; // not necessary to assign value
   int                  rc = 0;
   int                  reply_msg_type;

   /*
    * it is possible to get out of order messages while waiting on 
    * a notification. For example a stale async set confirmation.
    * Our policy is to ignore them unless we think the server is messed up. 
    */
   int loop;
   for (loop=0 ; loop<4; loop++) {
      reply_msg_size  = sizeof(reply_msg_buffer);
      reply_msg_type = SE_msg_receive(fd, reply_msg_buffer, &reply_msg_size, &from);
      if (SE_MSG_NOTIFICATION == reply_msg_type) {
         goto notif_reply_received;
      }
   }
   if (SE_MSG_NONE == reply_msg_type) {
      *namebytes = 0;
      namebuf[0] = '\0';
      *valbytes = 0;
      valbuf[0] = '\0';
      return(ERR_CANNOT_SET_STRING);
   }
   if (SE_MSG_NOTIFICATION != reply_msg_type) {
      *namebytes = 0;
      namebuf[0] = '\0';
      *valbytes = 0;
      valbuf[0] = '\0';
      return(ERR_OUT_OF_ORDER);
   }

notif_reply_received:
   async_id->trigger_id = (reply_msg_body->async_id).trigger_id;
   async_id->action_id = (reply_msg_body->async_id).action_id;

   // extract the subject and value from the return message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *reply_data_ptr;

   reply_data_ptr  = (char *)&(reply_msg_body->data);
   subject_str     = SE_msg_get_string(reply_data_ptr, &subject_bytes);
   reply_data_ptr += subject_bytes;
   value_str       =  SE_msg_get_string(reply_data_ptr, &value_bytes);

   // value_bytes and subject_bytes are sysevent strings (and include size info)
   // so change them to the strlen
   subject_bytes = strlen(subject_str);
   value_bytes   = strlen(value_str);

   // make sure the caller has enough room in their buffer for the name and
   // value. If not truncate and notify them via return code
   if (subject_bytes > (*namebytes - 1)) {
      memcpy(namebuf, subject_str, *namebytes-1);
      namebuf[*namebytes-1] = '\0';
      (*namebytes)--;
      rc = ERR_INSUFFICIENT_ROOM;
   } else {
      memcpy(namebuf, subject_str, subject_bytes);
      namebuf[subject_bytes] = '\0';
      *namebytes = subject_bytes;
   }

   if (value_bytes > (*valbytes - 1)) {
      memcpy(valbuf, value_str, *valbytes-1);
      valbuf[*valbytes-1] = '\0';
      (*valbytes)--;
      rc = ERR_INSUFFICIENT_ROOM;
   } else {
      memcpy(valbuf, value_str, value_bytes);
      valbuf[value_bytes] = '\0';
      *valbytes = value_bytes;
   }
   return(rc);
}

/*
 * Procedure     : sysevent_getnotification_data
 * Purpose       : Wait for a notification and return the results when received
 * Parameters    :
 *    fd            : The connection id
 *    token         : A server generated opaque value
 *    namebuf       : A buffer to hold the name received
 *    namebytes     : The length of the string not counting terminating null
 *    valbuf        : A buffer to hold returned value
 *    valbytes      : On input the maximum number of bytes in outbuf
 *                    On output the actual number of bytes in outbuf
 *    async_id      : The async id of the action
 * Return Code   :
 *    0             : Reply received
 *    !0            : Some error
 * Notes        :
 *    If a buffer is not big enough to hold the reply value, then the value will
 *    be truncated to fit. An error of ERR_INSUFFICIENT_ROOM will be returned.
 *    The value will always be NULL terminated, so the buffer must contain
 *    enough bytes for the return value as well as the NULL byte.
 * Notes
 *   This will block
 */
int sysevent_getnotification_data (const int fd, const token_t token, char *namebuf, int *namebytes, char *valbuf, int *valbytes, async_id_t *async_id) 
{
   if (NULL == namebytes || NULL == valbytes) {
     return(ERR_BAD_BUFFER);
   }
   if (NULL == namebuf || NULL == valbuf) {
      *namebytes = 0;
      *valbytes  = 0;
      return(ERR_BAD_BUFFER);
   }
  if (NULL == async_id) {
      *namebytes = 0;
      *valbytes  = 0;
      return(ERR_PARAM_NOT_FOUND);
  }

  if (*namebytes == 0 || *valbytes == 0)
      return ERR_INSUFFICIENT_ROOM;

   if (0 > fd) {
      *namebytes = 0;
      namebuf[0] = '\0';
      *valbytes = 0;
      valbuf[0] = '\0';
      return(ERR_NOT_CONNECTED);
   }

   // receive the notification
   unsigned int bin_size = sysevent_get_binmsg_maxsize();
   char *reply_msg_buffer = (char *)malloc(bin_size);   
   if (!reply_msg_buffer)
       return -1;
   se_notification_msg *reply_msg_body = (se_notification_msg *)reply_msg_buffer;
   unsigned int         reply_msg_size  = bin_size;
   token_t              from = token; // not necessary to assign value
   int                  rc = 0;
   int                  reply_msg_type;

   /*
    * it is possible to get out of order messages while waiting on 
    * a notification. For example a stale async set confirmation.
    * Our policy is to ignore them unless we think the server is messed up. 
    */
   int loop;
   for (loop=0 ; loop<4; loop++) {
      reply_msg_size  = bin_size;
      reply_msg_type = SE_msg_receive(fd, reply_msg_buffer, &reply_msg_size, &from);
      if (SE_MSG_NOTIFICATION_DATA == reply_msg_type) {
         goto notif_reply_received;
      }
   }
   if (SE_MSG_NONE == reply_msg_type) {
      *namebytes = 0;
      namebuf[0] = '\0';
      *valbytes = 0;
      valbuf[0] = '\0';
      free(reply_msg_buffer);
      return(ERR_CANNOT_SET_STRING);
   }
   if (SE_MSG_NOTIFICATION_DATA != reply_msg_type) {
      *namebytes = 0;
      namebuf[0] = '\0';
      *valbytes = 0;
      valbuf[0] = '\0';
      free(reply_msg_buffer);
      return(ERR_OUT_OF_ORDER);
   }

notif_reply_received:
   async_id->trigger_id = (reply_msg_body->async_id).trigger_id;
   async_id->action_id = (reply_msg_body->async_id).action_id;

   // extract the subject and value from the return message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *reply_data_ptr;

   reply_data_ptr  = (char *)&(reply_msg_body->data);
   subject_str     = SE_msg_get_string(reply_data_ptr, &subject_bytes);
   reply_data_ptr += subject_bytes;
   value_str       =  SE_msg_get_data(reply_data_ptr, &value_bytes);

   // value_bytes and subject_bytes are sysevent strings (and include size info)
   // so change them to the strlen
   subject_bytes = strlen(subject_str);

   // make sure the caller has enough room in their buffer for the name and
   // value. If not truncate and notify them via return code
   if (subject_bytes > (*namebytes - 1)) {
      memcpy(namebuf, subject_str, *namebytes-1);
      namebuf[*namebytes-1] = '\0';
      (*namebytes)--;
      rc = ERR_INSUFFICIENT_ROOM;
   } else {
      memcpy(namebuf, subject_str, subject_bytes);
      namebuf[subject_bytes] = '\0';
      *namebytes = subject_bytes;
   }

   if (value_bytes > *valbytes) {
      memcpy(valbuf, value_str, *valbytes);
      rc = ERR_INSUFFICIENT_ROOM;
   } else {
      memcpy(valbuf, value_str, value_bytes);
      *valbytes = value_bytes;
   }
   free(reply_msg_buffer);
   return(rc);
}


/*
 * Procedure     : sysevent_show
 * Purpose       : Tell daemon to show all data elements
 * Parameters    :
 *    fd            : The connection id
 *    token         : A server generated opaque value
 *    file         : A null terminated string which is the file to write to
 * Return Code   :
 *    0             : Success
 *    !0            : Some error
 */
int sysevent_show (const int fd, const token_t token, const char *file) 
{
   se_buffer                    send_msg_buffer;
   se_show_data_elements_msg   *send_msg_body;
   int                          inbytes;

   if (NULL == file || 0 == (inbytes = strlen(file)) ) {
      return(ERR_BAD_BUFFER);
   }
   if (0 > fd) {
      return(ERR_NOT_CONNECTED);
   }

   // calculate the size of the se_show_data_elements_msg once it has been populated 
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_show_data_elements_msg) +
                       SE_string2size(file) - sizeof(void *);

   // if the msg will be too long to fit into our buffer
   // then abort
   if (send_msg_size >= sizeof(send_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   } 

   // prepare the header of the msg
   if (NULL == 
      (send_msg_body = (se_show_data_elements_msg *)SE_msg_prepare (send_msg_buffer, 
                                                     sizeof(send_msg_buffer), 
                                                     SE_MSG_SHOW_DATA_ELEMENTS, token)) ) {
      return(ERR_MSG_PREPARE); 
   } 

   // prepare the body of the msg
   int  remaining_buf_bytes;
   char *send_data_ptr = (char *)&(send_msg_body->data);
   remaining_buf_bytes = sizeof(send_msg_buffer);
   remaining_buf_bytes -= sizeof(se_msg_hdr);
   remaining_buf_bytes -= sizeof(se_show_data_elements_msg);
   int strsize    = SE_msg_add_string(send_data_ptr, 
                                      remaining_buf_bytes,
                                      file);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }

   // send  msg . There is no reply

   SE_msg_send(fd, send_msg_buffer);
   return(0);
}
