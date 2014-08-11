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
 * utctx_rwlock.h - Utopia read/write lock
 */

#ifndef __UTCTX_RWLOCK_H__
#define __UTCTX_RWLOCK_H__

#include <semaphore.h>


/* Utopia Read/Write lock struct */
typedef struct _UtopiaRWLock
{
    /* Read lock obtained */
    int fReadLock;

    /* Write lock obtained */
    int fWriteLock;

#ifdef UTCTX_POSIX_SEM
    /* Read-write semaphore */
    sem_t* pSemaphore;

    /* Mutex protecting read-write semaphore for writes */
    sem_t* pMutex;
#else
    int hdSemId;
#endif
} UtopiaRWLock;


/*
 * Procedure     : UtopiaRWLock_Init
 * Purpose       : Initialize Utopia rwlock
 * Parameters    :
 *   pLock -  UtopiaRWLock pointer
 * Return Values :
 *    1 on success, 0 on error
 */
extern int UtopiaRWLock_Init(UtopiaRWLock* pLock);

/*
 * Procedure     : UtopiaRWLock_Destroy
 * Purpose       : Destroy Utopia rwlock
 * Parameters    :
 *   pLock -  UtopiaRWLock pointer
 * Return Values :
 *    1 on success, 0 on error
 */
extern int UtopiaRWLock_Destroy(UtopiaRWLock* pLock);

/*
 * Procedure     : UtopiaRWLock_ReadLock
 * Purpose       : Acquires a read lock if there isn't already a read or a write lock.
 * Parameters    :
 *   pLock -  UtopiaRWLock pointer
 * Return Values :
 *    1 on success, 0 on error
 */
extern int UtopiaRWLock_ReadLock(UtopiaRWLock* pLock);

/*
 * Procedure     : UtopiaRWLock_WriteLock
 * Purpose       : Acquires a write lock if there isn't already a write lock. If there is a read lock, it will be released.
 * Parameters    :
 *   pLock -  UtopiaRWLock pointer
 * Return Values :
 *    1 on success, 0 on error
 */
extern int UtopiaRWLock_WriteLock(UtopiaRWLock* pLock);

/*
 * Procedure     : UtopiaRWLock_Free
 * Purpose       : Release locks and free up resources
 * Parameters    :
 *   pLock -  UtopiaRWLock pointer
 * Return Values :
 */
extern void UtopiaRWLock_Free(UtopiaRWLock* pLock);

#endif /* __UTCTX_RWLOCK_H__ */
