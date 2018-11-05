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
