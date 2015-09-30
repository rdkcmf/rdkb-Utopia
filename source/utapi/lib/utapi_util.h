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
 * Copyright (c) 2008-2009 Cisco Systems, Inc. All rights reserved.
 *
 * Cisco Systems, Inc. retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have obtained
 * a separate written license from Cisco Systems, Inc., you are not authorized
 * to utilize all or a part of this computer program for any purpose (including
 * reproduction, distribution, modification, and compilation into object code),
 * and you must immediately destroy or return to Cisco Systems, Inc. all copies
 * of this computer program.  If you are licensed by Cisco Systems, Inc., your
 * rights to utilize this computer program are limited by the terms of that
 * license.  To obtain a license, please contact Cisco Systems, Inc.
 *
 * This computer program contains trade secrets owned by Cisco Systems, Inc.
 * and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
 * maintain the confidentiality of this computer program and related information
 * and to not disclose this computer program and related information to any
 * other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
 * SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */

#ifndef __UTAPI_UTIL_H__
#define __UTAPI_UTIL_H__

extern char ulog_msg[1024];
extern int  err_rc;
extern char s_intbuf[16];
extern char s_tokenbuf[256];

/* 
 * Generic struct used to map between the various Enums and 
 * their syscfg string representations
 */
typedef struct _EnumString_Map
{
    char* pszStr;
    int iEnum;
} EnumString_Map;

/*
 * Macros methods with automatic return on error 
 * if you don't want automatic return, use the non-macro version
 */

#define UTOPIA_SET(ctx,name,value) \
                    if (0  == Utopia_Set((ctx),(name),(value))) { \
                        return (ERR_UTCTX_OP); \
                    } \

#define UTOPIA_SETINDEXED(ctx,name,index,value) \
                    if (0 == Utopia_SetIndexed((ctx),(name),(index),(value))) { \
                        return (ERR_UTCTX_OP); \
                    } \

#define UTOPIA_SETNAMED(ctx,name,prefix,value) \
                    if (0  == Utopia_SetNamed((ctx),(name),(prefix),(value))) { \
                        ulogf(ULOG_CONFIG, UL_UTAPI, "Error: setting %s_[%d] to %s", prefix, name, value); \
                        return (ERR_UTCTX_OP); \
                    } \



#define UTOPIA_SETIP(ctx,name,value) \
                    if (IsValid_IPAddr((value))) { \
                        UTOPIA_SET((ctx),(name),(value)) \
                    } else { \
                       return (ERR_INVALID_IP); \
                    } \


#define UTOPIA_VALIDATE_SET(ctx,name,value,validate_func,error) \
                    if (validate_func((value))) { \
                        UTOPIA_SET((ctx),(name),(value)) \
                    } else { \
                       return (error); \
                    } \

#define UTOPIA_UNSET(ctx,name) \
                    if (0  == Utopia_Unset((ctx),(name))) { \
                        return (ERR_UTCTX_OP); \
                    } \

#define UTOPIA_UNSETINDEXED(ctx,name,index) \
                    if (0 == Utopia_UnsetIndexed((ctx),(name),(index))) { \
                        return (ERR_UTCTX_OP); \
                    } \

/*
 * Integer sets
 */
#define UTOPIA_SETINT(ctx,name,intvalue) \
                    if (SUCCESS != (err_rc = Utopia_SetInt((ctx),(name),(intvalue)))) { \
                        return (err_rc); \
                    } \

#define UTOPIA_SETINDEXEDINT(ctx,name,index,intvalue) \
                    if (SUCCESS != (err_rc = Utopia_SetIndexedInt((ctx),(name),(index),(intvalue)))) { \
                        return (err_rc); \
                    } \

#define UTOPIA_SETNAMEDINT(ctx,name,prefix,intvalue) \
                    if (SUCCESS != (err_rc = Utopia_SetNamedInt((ctx),(name),(prefix),(intvalue)))) { \
                        return (err_rc); \
                    } \

/*
 * Boolean sets
 */
#define UTOPIA_SETBOOL(ctx,name,boolvalue) \
                    if (SUCCESS != (err_rc = Utopia_SetBool((ctx),(name),(boolvalue)))) { \
                        return (err_rc); \
                    } \

#define UTOPIA_SETINDEXEDBOOL(ctx,name,index,boolvalue) \
                    if (SUCCESS != (err_rc = Utopia_SetIndexedBool((ctx),(name),(index),(boolvalue)))) { \
                        return (err_rc); \
                    } \

#define UTOPIA_SETNAMEDBOOL(ctx,name,prefix,boolvalue) \
                    if (SUCCESS != (err_rc = Utopia_SetNamedBool((ctx),(name),(prefix),(boolvalue)))) { \
                        return (err_rc); \
                    } \


/*
 * GET macros are used ONLY on values that are always expected to be set
 * if a value is optional is syscfg, it is okay to ignore Utopia_Get method's
 * error status
 */
#define UTOPIA_GET(ctx,name,value,sz) \
                    if (0 == Utopia_Get((ctx),(name),(value),(sz))) { \
                        return (ERR_UTCTX_OP); \
                    } \

#define UTOPIA_GETINDEXED(ctx,name,index,out_value,size) \
                    if (0 == Utopia_GetIndexed((ctx),(name),(index),(out_value),(size))) { \
                        return (ERR_UTCTX_OP); \
                    } \

/*

#define UTOPIA_GETIP(ctx,name,value) \
                    if (IsValid_IPAddr((value))) { \
                        UTOPIA_GET((ctx),(name),(value)) \
                    } else { \
                       return (ERR_INVALID_IP); \
                    } \


#define UTOPIA_VALIDATE_GET(ctx,name,value,validate_func,error) \
                    if (validate_func((value))) { \
                        UTOPIA_GET((ctx),(name),(value)) \
                    } else { \
                       return (error); \
                    } \
*/

/*
 * Integer sets
 */

#define UTOPIA_GETINT(ctx,name,out_intvalue) \
                    if (SUCCESS != (err_rc = Utopia_GetInt((ctx),(name),(out_intvalue)))) { \
                        return (err_rc); \
                    } \

#define UTOPIA_GETINDEXEDINT(ctx,name,index,out_intvalue) \
                    if (SUCCESS != (err_rc = Utopia_GetIndexedInt((ctx),(name),(index),(out_intvalue)))) { \
                        return (err_rc); \
                    } \

#define UTOPIA_GETINDEXED2INT(ctx,name,index1,index2,out_value,size) \
                    if (SUCCESS != (err_rc = Utopia_GetIndexed2Int((ctx),(name),(index1),(index2),(out_intvalue)))) { \
                        return (err_rc); \
                    } \


/*
 * Integer sets
 */

#define UTOPIA_GETBOOL(ctx,name,out_boolvalue) \
                    if (SUCCESS != (err_rc = Utopia_GetBool((ctx),(name),(out_boolvalue)))) { \
                        return (err_rc); \
                    } \


#define UTOPIA_GETINDEXEDBOOL(ctx,name,index,out_boolvalue) \
                    if (SUCCESS != (err_rc = Utopia_GetIndexedBool((ctx),(name),(index),(out_boolvalue)))) { \
                        return (err_rc); \
                    } \


/*
 * Utility APIs
 */
char* s_EnumToStr (EnumString_Map* pMap, int iEnum);
int s_StrToEnum (EnumString_Map* pMap, const char *iStr);
char *chop_str (char *str, char delim);

int IsValid_IPAddr (const char *ip);
int IsValid_IPAddrLastOctet (int ipoctet);
int IsValid_Netmask (const char *ip);
int IsValid_MACAddr (const char *mac);
boolean_t IsInteger (const char *str);
int IsSameNetwork(unsigned long addr1, unsigned long addr2, unsigned long mask);
int IsLoopback(unsigned long addr);
int IsMulticast(unsigned long addr);
int IsBroadcast(unsigned long addr, unsigned long net, unsigned long mask);
int IsNetworkAddr(unsigned long addr, unsigned long net, unsigned long mask);
int IsNetmaskValid(unsigned long netmask);
void s_get_interface_mac (char *ifname, char *out_buf, int bufsz);
int s_sysevent_connect (token_t *out_se_token);
int Utopia_SetInt (UtopiaContext *ctx, UtopiaValue ixUtopia, int value);
int Utopia_SetBool (UtopiaContext *ctx, UtopiaValue ixUtopia, boolean_t value);
int Utopia_SetIndexedInt (UtopiaContext *ctx, UtopiaValue ixUtopia, int iIndex, int value);
int Utopia_SetIndexedBool (UtopiaContext *ctx, UtopiaValue ixUtopia, int iIndex, boolean_t value);
int Utopia_SetNamedInt (UtopiaContext *ctx, UtopiaValue ixUtopia, char *prefix, int value);
int Utopia_SetNamedBool (UtopiaContext *ctx, UtopiaValue ixUtopia, char *prefix, boolean_t value);
int Utopia_SetNamedLong (UtopiaContext *ctx, UtopiaValue ixUtopia, char *prefix, unsigned long value);
int Utopia_GetInt (UtopiaContext *ctx, UtopiaValue ixUtopia, int *out_int);
int Utopia_GetIndexedInt (UtopiaContext *ctx, UtopiaValue ixUtopia, int index, int *out_int);
int Utopia_GetBool (UtopiaContext *ctx, UtopiaValue ixUtopia, boolean_t *out_bool);
int Utopia_GetIndexedBool (UtopiaContext *ctx, UtopiaValue ixUtopia, int index, boolean_t *out_bool);
int Utopia_GetIndexed2Int (UtopiaContext *ctx, UtopiaValue ixUtopia, int index1, int index2, int *out_int);
int Utopia_GetIndexed2Bool (UtopiaContext *ctx, UtopiaValue ixUtopia, int index1, int index2, boolean_t *out_bool);
int Utopia_GetNamedBool (UtopiaContext *ctx, UtopiaValue ixUtopia, char *name, boolean_t *out_bool);
int Utopia_GetNamedInt (UtopiaContext *ctx, UtopiaValue ixUtopia,char *name, int *out_int);
int Utopia_GetNamedLong (UtopiaContext *ctx, UtopiaValue ixUtopia,char *name, unsigned long *out_int);

#endif // __UTAPI_UTIL_H__
