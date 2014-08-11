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

/*
 * utctx.h - Utopia context.
 */

#ifndef __UTCTX_H__
#define __UTCTX_H__

#include "utctx_rwlock.h"


/*
 * Struct   : Utopia context
 * Purpose  : Stores transaction list, events to be triggered and sysevent handles
 */
typedef struct _UtopiaContext
{
    /* Utopia transaction list head pointer */
    void* pHead;

    /* HDK_Utopia_Event bitmask */
    unsigned int bfEvents;

    /* SysEvent handle and token */
    int iEventHandle;
    unsigned long uiEventToken;

    /* UtopiaRWLock handle */
    UtopiaRWLock rwLock;
} UtopiaContext;

/*
 * Procedure     : Utopia_Init
 * Purpose       : Initialize Utopia context
 * Parameters    :
 *   pUtopiaCtx -  UtopiaContext pointer
 * Return Values :
 *    1 on success, 0 on error
 */
extern int Utopia_Init(UtopiaContext* pUtopiaCtx);

/*
 * Procedure     : Utopia_Free
 * Purpose       : Commit all values stored in the transaction if fCommit is true, free up context memory
 * Parameters    :
 *   pUtopiaCtx -  UtopiaContext pointer
 *   fCommit - Commit transaction is true
 * Return Values :
 */
extern void Utopia_Free(UtopiaContext* pUtopiaCtx, int fCommit);

#endif /* __UTCTX_H__ */
