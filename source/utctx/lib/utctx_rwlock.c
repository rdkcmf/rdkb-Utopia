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

#include "utctx_rwlock.h"
#include "utctx_internal.h"

#ifdef UTCTX_POSIX_SEM
#include <fcntl.h>
#else
#include <sys/sem.h>
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Utopia semaphore defines */
#ifdef UTCTX_POSIX_SEM
#define UTCTX_POSIX_SEMAPHORE   "utctx.semid"
#define UTCTX_POSIX_MUTEX       "utctx.mutid"
#else
#define UTCTX_SYSV_SEMAPHORE    "/tmp/utctx.semid"
#define UTCTX_SYSV_PROJID       487
#endif
#define UTCTX_SEM_RESOURCE_MAX  32


#ifdef UTCTX_POSIX_SEM
static int s_RWLock_Init(UtopiaRWLock* pLock)
{
    /* Initialize the read semaphore to 32 - allow 32 simultaneous readers */
    if ((pLock->pSemaphore = sem_open(UTCTX_POSIX_SEMAPHORE, O_CREAT, 0644, UTCTX_SEM_RESOURCE_MAX)) == SEM_FAILED)
    {
        UTCTX_LOG_ERR1("%s: Opening semaphore failed\n", __FUNCTION__);
        return 0;
    }
    if ((pLock->pMutex = sem_open(UTCTX_POSIX_MUTEX, O_CREAT, 0644, 1)) == SEM_FAILED)
    {
        UTCTX_LOG_ERR1("%s: Opening mutex failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_Close(UtopiaRWLock* pLock)
{
    if (sem_close(pLock->pSemaphore) == -1)
    {
        UTCTX_LOG_ERR1("%s: Closing semaphore failed\n", __FUNCTION__);
        return 0;
    }
    if (sem_close(pLock->pMutex) == -1)
    {
        UTCTX_LOG_ERR1("%s: Closing mutex failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_Destroy(UtopiaRWLock* pLock)
{
    /* Unused variable */
    (void)pLock;

    if (sem_unlink(UTCTX_POSIX_SEMAPHORE) == -1)
    {
        UTCTX_LOG_ERR1("%s: Destroying semaphore failed\n", __FUNCTION__);
        return 0;
    }
    if (sem_unlink(UTCTX_POSIX_MUTEX) == -1)
    {
        UTCTX_LOG_ERR1("%s: Destroying mutex failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_ReadAcquire(UtopiaRWLock* pLock)
{
    if (sem_wait(pLock->pSemaphore) == -1)
    {
        UTCTX_LOG_ERR1("%s: Acquiring read lock failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_ReadRelease(UtopiaRWLock* pLock)
{
    if (sem_post(pLock->pSemaphore) == -1)
    {
        UTCTX_LOG_ERR1("%s: Releasing read lock failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_WriteAcquire(UtopiaRWLock* pLock)
{
    int iReturn;
    int i;

    /* To avoid deadlock, let only one writer wait on the semaphore */
    if ((iReturn = sem_wait(pLock->pMutex)) == -1)
    {
        UTCTX_LOG_ERR1("%s: Acquiring write lock failed\n", __FUNCTION__);
        return 0;
    }

    /* Consume the max number of resources to ensure that no read or write is allowed until we're finished */
    for (i = 0; (i < UTCTX_SEM_RESOURCE_MAX) && (iReturn != -1); ++i)
    {
        iReturn = sem_wait(pLock->pSemaphore);
    }

    if ((iReturn == -1) || sem_post(pLock->pMutex) == -1)
    {
        UTCTX_LOG_ERR1("%s: Acquiring write lock failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_WriteRelease(UtopiaRWLock* pLock)
{
    int iReturn = 0;
    int i;

    /* Release all of the resources that we consumed in LockWrite */
    for (i = 0; (i < UTCTX_SEM_RESOURCE_MAX) && (iReturn != -1); ++i)
    {
        iReturn = sem_post(pLock->pSemaphore);
    }

    if (iReturn == -1)
    {
        UTCTX_LOG_ERR1("%s: Releasing write lock failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

#else /* #ifdef UTCTX_POSIX_SEM */

/* Value definitions of lock/unlocks */
#define UTCTX_SYSV_LOCK   -1
#define UTCTX_SYSV_UNLOCK 1

/* User-defined union for initializing semaphores */
typedef union _UtopiaSemUnion
{
    int iVal;                           /* Value for SETVAL */
    struct semid_ds *pBuf;              /* Buffer for IPC_STAT & IPC_SET */
    unsigned short int *pArray;         /* Array for GETALL & SETALL */
} UtopiaSemUnion;

/*
 * Semaphore array lock relations
 *    0 - Read/Write semaphore
 *    1 - Write mutex
 */
enum
{
    UTCTX_SYSV_SEM_NUM = 0,
    UTCTX_SYSV_MUT_NUM
};

/* System V lock/unlock helper function */
static int s_LockUnlockSysVSem(int hdSemId, int iSemNum, int iOp)
{
    struct sembuf semOps;

    memset(&semOps, 0, sizeof(struct sembuf));

    semOps.sem_num = iSemNum;
    semOps.sem_op = iOp;
    semOps.sem_flg = SEM_UNDO;

    if (semop(hdSemId, &semOps, 1) == -1)
    {
        return 0;
    }
    return 1;
}

static int s_RWLock_Init(UtopiaRWLock* pLock)
{
    int fNew = 0;
    key_t key;

    /* First off, need to check for existance of utopia sysv semaphore file */
    FILE *fp_sysv = fopen(UTCTX_SYSV_SEMAPHORE, "r");;
    if (NULL == fp_sysv)
    {
        /* Create the file */
        FILE* fp = fopen(UTCTX_SYSV_SEMAPHORE, "w");

        if (fp == 0)
        {
            UTCTX_LOG_ERR1("%s: Creating Sys V semaphore file\n", __FUNCTION__);
            return 0;
        }

        /* Write some text and close file */
        fputs("creating...", fp);
        fclose(fp);
    }
    else
    {
         fclose(fp_sysv);
    }
    

    /* Create semaphore key, unique to libutctx */
    if ((key = ftok(UTCTX_SYSV_SEMAPHORE, UTCTX_SYSV_PROJID)) == -1)
    {
        UTCTX_LOG_ERR1("%s: Could not create semaphore key\n", __FUNCTION__);
        return 0;
    }

    /* Get the semaphore, testing whether or not its new */
    if ((pLock->hdSemId = semget(key, 2, 0644 | IPC_CREAT | IPC_EXCL)) != -1)
    {
        fNew = 1;
    }
    else if ((pLock->hdSemId = semget(key, 2, 0644 | IPC_CREAT)) == -1)
    {
        UTCTX_LOG_ERR1("%s: Could not get semaphore\n", __FUNCTION__);
        return 0;
    }

    /* Initialize the semaphore if it didn't exist */
    if (fNew)
    {
        UtopiaSemUnion unSem;
        UtopiaSemUnion unMut;

        /* Initialize the semaphore resources to 32 */
        unSem.iVal = UTCTX_SEM_RESOURCE_MAX;
        if (semctl(pLock->hdSemId, UTCTX_SYSV_SEM_NUM, SETVAL, unSem) == -1)
        {
            UTCTX_LOG_ERR1("%s: Could not initialize semaphore\n", __FUNCTION__);
            return 0;
        }

        /* Initializze the mutex to 1 */
        unMut.iVal = UTCTX_SYSV_UNLOCK;
        if (semctl(pLock->hdSemId, UTCTX_SYSV_MUT_NUM, SETVAL, unMut) == -1)
        {
            UTCTX_LOG_ERR1("%s: Could not initialize mutex\n", __FUNCTION__);
            return 0;
        }
    }
    return 1;
}

static int s_RWLock_Close(UtopiaRWLock* pLock)
{
    /* This is a no-op for System V semaphores */

    /* Unused variable */
    (void)pLock;
    return 1;
}

static int s_RWLock_Destroy(UtopiaRWLock* pLock)
{
    if (semctl(pLock->hdSemId, 0, IPC_RMID) == -1)
    {
        UTCTX_LOG_ERR1("%s: Destroying system semaphore failed\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_ReadAcquire(UtopiaRWLock* pLock)
{
    if (s_LockUnlockSysVSem(pLock->hdSemId, UTCTX_SYSV_SEM_NUM, UTCTX_SYSV_LOCK) == 0)
    {
        UTCTX_LOG_ERR1("%s: Acquiring read lock\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_ReadRelease(UtopiaRWLock* pLock)
{
    if (s_LockUnlockSysVSem(pLock->hdSemId, UTCTX_SYSV_SEM_NUM, UTCTX_SYSV_UNLOCK) == 0)
    {
        UTCTX_LOG_ERR1("%s: Releasing read lock\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_WriteAcquire(UtopiaRWLock* pLock)
{
    int i;
    int iReturn;

    /* To avoid deadlock, let only one writer wait on the semaphore */
    if ((iReturn = s_LockUnlockSysVSem(pLock->hdSemId, UTCTX_SYSV_MUT_NUM, UTCTX_SYSV_LOCK)) == 0)
    {
        UTCTX_LOG_ERR1("%s: Acquiring write lock\n", __FUNCTION__);
        return 0;
    }

    /* Consume the max number of resources to ensure that no read or write is allowed until we're finished */
    for (i = 0; (i < UTCTX_SEM_RESOURCE_MAX) && iReturn; ++i)
    {
        iReturn = s_LockUnlockSysVSem(pLock->hdSemId, UTCTX_SYSV_SEM_NUM, UTCTX_SYSV_LOCK);
    }

    if (iReturn == 0 || s_LockUnlockSysVSem(pLock->hdSemId, UTCTX_SYSV_MUT_NUM, UTCTX_SYSV_UNLOCK) == 0)
    {
        UTCTX_LOG_ERR1("%s: Acquiring write lock\n", __FUNCTION__);
        return 0;
    }
    return 1;
}

static int s_RWLock_WriteRelease(UtopiaRWLock* pLock)
{
    /* Release all 32 resources at once */
    if (s_LockUnlockSysVSem(pLock->hdSemId, UTCTX_SYSV_SEM_NUM, UTCTX_SEM_RESOURCE_MAX) == 0)
    {
        UTCTX_LOG_ERR1("%s: Releasing write lock\n", __FUNCTION__);
        return 0;
    }
    return 1;
}
#endif /* #ifdef UTCTX_SYSV_SEM */


/*
 * utctx_lock.h
 */

int UtopiaRWLock_Init(UtopiaRWLock* pLock)
{
    UTCTX_LOG_DBG1("%s: Initializing...\n", __FUNCTION__);

    pLock->fReadLock = 0;
    pLock->fWriteLock = 0;

    return s_RWLock_Init(pLock);
}

int UtopiaRWLock_Destroy(UtopiaRWLock* pLock)
{
    UTCTX_LOG_DBG1("%s: Destroying...\n", __FUNCTION__);

    return s_RWLock_Destroy(pLock);
}

int UtopiaRWLock_ReadLock(UtopiaRWLock* pLock)
{
    UTCTX_LOG_DBG1("%s: Acquring read lock...\n", __FUNCTION__);

    /* If we have either a read or a write lock, return */
    if (pLock->fReadLock || pLock->fWriteLock)
    {
        return 1;
    }

    /* Attempt to acquire read lock */
    if (s_RWLock_ReadAcquire(pLock) == 0)
    {
        return 0;
    }
    else
    {
        pLock->fReadLock = 1;
        return 1;
    }
}

int UtopiaRWLock_WriteLock(UtopiaRWLock* pLock)
{
    UTCTX_LOG_DBG1("%s: Acquring write lock...\n", __FUNCTION__);

    /* Return if we already have a lock */
    if (pLock->fWriteLock)
    {
        return 1;
    }

    /* If we have a read lock, we need to first release it */
    if (pLock->fReadLock &&
        s_RWLock_ReadRelease(pLock) == 0)
    {
        return 0;
    }
    pLock->fReadLock = 0;

    /* Attempt to acquire write lock */
    if (s_RWLock_WriteAcquire(pLock) == 0)
    {
        return 0;
    }
    else
    {
        pLock->fWriteLock = 1;
        return 1;
    }
}

void UtopiaRWLock_Free(UtopiaRWLock* pLock)
{
    UTCTX_LOG_DBG1("%s: Freeing lock...\n", __FUNCTION__);

    /* Release the read lock */
    if (pLock->fReadLock)
    {
        s_RWLock_ReadRelease(pLock);
    }
    pLock->fReadLock = 0;

    /* Release the write lock */
    if (pLock->fWriteLock)
    {
        s_RWLock_WriteRelease(pLock);
    }
    pLock->fWriteLock = 0;

    /* Close the read/write semaphores */
    s_RWLock_Close(pLock);
}
