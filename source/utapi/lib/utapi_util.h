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

#ifndef __UTAPI_UTIL_H__
#define __UTAPI_UTIL_H__

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
                    { \
                        int err_rc = Utopia_SetInt((ctx),(name),(intvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

#define UTOPIA_SETINDEXEDINT(ctx,name,index,intvalue) \
                    { \
                        int err_rc = Utopia_SetIndexedInt((ctx),(name),(index),(intvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

#define UTOPIA_SETNAMEDINT(ctx,name,prefix,intvalue) \
                    { \
                        int err_rc = Utopia_SetNamedInt((ctx),(name),(prefix),(intvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

/*
 * Boolean sets
 */
#define UTOPIA_SETBOOL(ctx,name,boolvalue) \
                    { \
                        int err_rc = Utopia_SetBool((ctx),(name),(boolvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

#define UTOPIA_SETINDEXEDBOOL(ctx,name,index,boolvalue) \
                    { \
                        int err_rc = Utopia_SetIndexedBool((ctx),(name),(index),(boolvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

#define UTOPIA_SETNAMEDBOOL(ctx,name,prefix,boolvalue) \
                    { \
                        int err_rc = Utopia_SetNamedBool((ctx),(name),(prefix),(boolvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

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
                    { \
                        int err_rc = Utopia_GetInt((ctx),(name),(out_intvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

#define UTOPIA_GETINDEXEDINT(ctx,name,index,out_intvalue) \
                    { \
                        int err_rc = Utopia_GetIndexedInt((ctx),(name),(index),(out_intvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

#define UTOPIA_GETINDEXED2INT(ctx,name,index1,index2,out_value,size) \
                    { \
                        int err_rc = Utopia_GetIndexed2Int((ctx),(name),(index1),(index2),(out_intvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

/*
 * Integer sets
 */

#define UTOPIA_GETBOOL(ctx,name,out_boolvalue) \
                    { \
                        int err_rc = Utopia_GetBool((ctx),(name),(out_boolvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

#define UTOPIA_GETINDEXEDBOOL(ctx,name,index,out_boolvalue) \
                    { \
                        int err_rc = Utopia_GetIndexedBool((ctx),(name),(index),(out_boolvalue)); \
                        if (err_rc != SUCCESS) \
                            return err_rc; \
                    }

/*
 * Utility APIs
 */
char* s_EnumToStr (EnumString_Map* pMap, int iEnum);
int s_StrToEnum (EnumString_Map* pMap, const char *iStr);

int IsValid_IPAddr (const char *ip);
int IsValid_IPAddrLastOctet (int ipoctet);
int IsValid_Netmask (const char *ip);
int IsValid_MACAddr (const char *mac);
int IsValid_ULAAddress(const char *address);
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
