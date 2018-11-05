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

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

/* Number of simultaneous read locks allowed (defined in utctx_rwlock.c) */
#define UTCTX_SEM_RESOURCE_MAX  32

/* Unittest timeout (minutes) - should be sufficient enough time for all reads/writes to run */
#define UNITTEST_TIMEOUT 2

/* The number of simultaneous reader/writers that will compete for lock */
#define SIM_PROCESSES    16

/* Min/Max sleep times (microseconds) */
#define READ_SLEEP_MIN   50000
#define READ_SLEEP_MAX   370000
#define WRITE_SLEEP_MIN  530000
#define WRITE_SLEEP_MAX  890000

/* Number of reads/writes performed by each process */
#define READS_PERFORMED  50
#define WRITES_PERFORMED 13

/* Min/Max read/write times (microseconds) */
#define READ_TIME_MIN    7300
#define READ_TIME_MAX    11300
#define WRITE_TIME_MIN   73000
#define WRITE_TIME_MAX   113000

/* Sleep for a random amount of time in range y to z */
#define SLEEP_RANDOM_RANGE(y,z)                     \
    {                                               \
        int x = rand();                             \
        x = ((x % z) < y ? (x % z) : (x % z) + y);  \
        usleep(x);                                  \
    }                                               \

/* Macro that sets x to be the number of microseconds elapsed today */
#define MICROSECS_TODAY(x)                      \
    {                                           \
        struct timeval tv;                      \
        gettimeofday(&tv, NULL);                \
        x = (tv.tv_sec % 86400);                \
        x *= 1000000;                           \
        x += tv.tv_usec;                        \
    }                                           \

/* Semaphore related defines */
#define RWLOCK_UNITTEST_MUTEX "rwlock_unittest.semid"
#define RWLOCK_UNITTEST_SHMEM "rwlock_unittest.shmid"

/* Global shared memory pointer */
typedef struct _ShMemStruct
{
    /* Count of the number of processes which have finished */
    int iProcsDone;

    /* lock/unlock times for every read/write performed in each process */
    long long int pppReadTimes[SIM_PROCESSES / 2][READS_PERFORMED][2];
    long long int pppWriteTimes[SIM_PROCESSES / 2][WRITES_PERFORMED][2];
} ShMemStruct;

static ShMemStruct* g_pShMemStruct;

/*
 * Reader helper function, which acquires a read lock every
 * READ_SLEEP_MIN to READ_SLEEP_MAX usecs and holds the
 * lock for READ_TIME_MIN to READ_TIME_MAX usecs
 */
static void s_reader(int iProcId, sem_t* pSem)
{
    int i;

    /*
     * Seed the random number generator so that each process
     * produces a different sequence of psuedo-random numbers
     */
    srand(iProcId + 1);

    for (i = 0; i < READS_PERFORMED; ++i)
    {
        long long int lliLocked;
        long long int lliUnLocked;
        UtopiaRWLock rwLock;

        UtopiaRWLock_Init(&rwLock);
        UtopiaRWLock_ReadLock(&rwLock);

        /* Get time lock acquired in microseconds */
        MICROSECS_TODAY(lliLocked);

        SLEEP_RANDOM_RANGE(READ_TIME_MIN, READ_TIME_MAX);

        /* Get time lock release in microseconds */
        MICROSECS_TODAY(lliUnLocked);

        UtopiaRWLock_Free(&rwLock);

        /* Lock the shared memory struct */
        sem_wait(pSem);
        g_pShMemStruct->pppReadTimes[iProcId / 2][i][0] = lliLocked;
        g_pShMemStruct->pppReadTimes[iProcId / 2][i][1] = lliUnLocked;
        sem_post(pSem);

        SLEEP_RANDOM_RANGE(READ_SLEEP_MIN, READ_SLEEP_MAX);
    }
}

/*
 * Helper function, which acquires UTCTX_SEM_RESOURCE_MAX / 2 read locks and
 * then crashes, before releasing the locks.
 */
static void s_reader_crash(void)
{
    int i;
    UtopiaRWLock rwLock;

    UtopiaRWLock_Init(&rwLock);

    /* Acquire some read locks */
    for (i = 0; i < UTCTX_SEM_RESOURCE_MAX / 2; ++i)
    {
        /* Acquire utopia read lock */
        UtopiaRWLock_ReadLock(&rwLock);
    }

    /* Oops! */
    *(int*)0 = 0;

    UtopiaRWLock_Free(&rwLock);
}

/*
 * Writer helper function, which acquires a write lock every
 * WRITE_SLEEP_MIN to WRITE_SLEEP_MAX usecs and holds the
 * lock for WRITE_TIME_MIN to WRITE_TIME_MAX usecs
 */
static void s_writer(int iProcId, sem_t* pSem)
{
    int i;

    /*
     * Seed the random number generator so that each process
     * produces a different sequence of psuedo-random numbers
     */
    srand(iProcId + 1);

    for (i = 0; i < WRITES_PERFORMED; ++i)
    {
        long long int lliLocked;
        long long int lliUnLocked;
        UtopiaRWLock rwLock;

        /* Acquire utopia write lock */
        UtopiaRWLock_Init(&rwLock);
        UtopiaRWLock_WriteLock(&rwLock);

        /* Get time lock acquired in microseconds */
        MICROSECS_TODAY(lliLocked);

        SLEEP_RANDOM_RANGE(WRITE_TIME_MIN, WRITE_TIME_MAX);

        /* Get time lock release in microseconds */
        MICROSECS_TODAY(lliUnLocked);

        UtopiaRWLock_Free(&rwLock);

        /* Lock the shared memory struct */
        sem_wait(pSem);
        g_pShMemStruct->pppWriteTimes[iProcId / 2][i][0] = lliLocked;
        g_pShMemStruct->pppWriteTimes[iProcId / 2][i][1] = lliUnLocked;
        sem_post(pSem);

        SLEEP_RANDOM_RANGE(WRITE_SLEEP_MIN, WRITE_SLEEP_MAX);
    }
}

/*
 * Writer helper function, which acquires a write lock and
 * then exits without releasing the lock.
 */
static void s_writer_exit(void)
{
    UtopiaRWLock rwLock;

    /* Acquire utopia write lock */
    UtopiaRWLock_Init(&rwLock);
    UtopiaRWLock_WriteLock(&rwLock);

    exit(0);
}

/* Helper function to initialize shared memory and mutex */
static void s_init_shmem(int* pfdShMem, sem_t** ppSem)
{
    int i;

    /* Initialize the procs_done mutex */
    *ppSem = sem_open(RWLOCK_UNITTEST_MUTEX, O_CREAT, 0644, 1);

    /* Attempt to open the shm instance */
    *pfdShMem = shm_open(RWLOCK_UNITTEST_SHMEM, O_CREAT | O_RDWR, 0644);

    /* Set the size of the shm */
    i = ftruncate(*pfdShMem, sizeof(ShMemStruct));
    (void)i;

    /* Map the g_ProcsDone int to the shared memory */
    g_pShMemStruct =  mmap(0, sizeof(ShMemStruct), PROT_READ | PROT_WRITE, MAP_SHARED, *pfdShMem, 0);

    /* Initialize value ShMemStruct values */
    {
        int i, j;

        g_pShMemStruct->iProcsDone = 0;

        /* Initialize all of the lock/unlock times to -1 */
        for(i = 0; i < (SIM_PROCESSES / 2); ++i)
        {
            for(j = 0; j < READS_PERFORMED; ++j)
            {
                g_pShMemStruct->pppReadTimes[i][j][0] = -1;
                g_pShMemStruct->pppReadTimes[i][j][1] = -1;
            }
        }
        for(i = 0; i < (SIM_PROCESSES / 2); ++i)
        {
            for(j = 0; j < WRITES_PERFORMED; ++j)
            {
                g_pShMemStruct->pppWriteTimes[i][j][0] = -1;
                g_pShMemStruct->pppWriteTimes[i][j][0] = -1;
            }
        }
    }
}

/* Helper function to free shared memory and mutex */
static void s_free_shmem(int fdShMem, sem_t* pSem)
{
    sem_close(pSem);
    close(fdShMem);

    sem_unlink(RWLOCK_UNITTEST_MUTEX);
    shm_unlink(RWLOCK_UNITTEST_SHMEM);
}


/*
 * rwlock_unittest.c - UtopiaRWLock unittest application
 */

int main(void)
{
    int fdShMem;
    int fProcsFinished = 0;
    int i;
    pid_t pid;
    sem_t* pSem;

    /* First off, spawn a couple of processes that will not release their locks */
    pid = fork();

    if (pid == -1)
    {
        printf("Failed: Reader-crash fork failed");
        exit(-1);
    }
    else if (pid == 0)
    {
        s_reader_crash();
    }

    pid = fork();

    if (pid == -1)
    {
        printf("Failed: Writer-exit fork failed");
        exit(-1);
    }
    else if (pid == 0)
    {
        s_writer_exit();
    }

    /* Initialize the procs_done shared memory and mutex */
    s_init_shmem(&fdShMem, &pSem);

    /* Next, spawn multiple reader and writer processes */
    for (i = 0; i < SIM_PROCESSES; ++i)
    {
        pid = fork();

        if (pid == -1)
        {
            printf("Failed: Reader/Writer fork failed");
            exit(-1);
        }
        /* Call the reader for even number iterations and writer for odd */
        else if (pid == 0)
        {
            if (i & 0x01)
            {
                s_writer(i, pSem);
            }
            else
            {
                s_reader(i, pSem);
            }

            /* Acquire a lock for g_ProcsDone mutex before we increment value */
            sem_wait(pSem);

            /* Increment the global processes done counter */
            g_pShMemStruct->iProcsDone += 1;

            /* Release lock */
            sem_post(pSem);

            /* Exit the child process */
            exit(0);
        }
    }

    /*
     * Loop until g_ProcsDone is equal to SIM_PROCESSES or
     * 240 * UNITTEST_TIMEOUT loops complete (~1 minute * UNITTEST_TIMEOUT),
     * in which case we assume that one of the locks is starving
     * and fail the test
     */
    for(i = 0; i < 240 * UNITTEST_TIMEOUT; ++i)
    {
        /* Acquire a lock for g_ProcsDone mutex before we read value */
        sem_wait(pSem);

        if (g_pShMemStruct->iProcsDone == SIM_PROCESSES)
        {
            fProcsFinished = 1;
        }

        /* Release lock */
        sem_post(pSem);

        if (fProcsFinished)
        {
            break;
        }

        /* Sleep for 1/4 second */
        usleep(250000);
    }

    /* Free up the shared memory mutex now that it's no longer needed */
    s_free_shmem(fdShMem, pSem);

    if (fProcsFinished == 0)
    {
        printf("Failed: Not all processes finished after %d minutes", UNITTEST_TIMEOUT);
        exit(-1);
    }

    /* Now validate that no reads were occurring during any write */
    {
        int i, j, k, l;

        /* Validate no read occurred during a write */
        for(i = 0; i < (SIM_PROCESSES / 2); ++i)
        {
            for(j = 0; j < READS_PERFORMED; ++j)
            {
                for(k = 0; k < (SIM_PROCESSES / 2); ++k)
                {
                    for(l = 0; l < WRITES_PERFORMED; ++l)
                    {
                        if ((g_pShMemStruct->pppReadTimes[i][j][0] > g_pShMemStruct->pppWriteTimes[k][l][0] &&
                             g_pShMemStruct->pppReadTimes[i][j][0] < g_pShMemStruct->pppWriteTimes[k][l][1]) ||
                            (g_pShMemStruct->pppReadTimes[i][j][1] > g_pShMemStruct->pppWriteTimes[k][l][0] &&
                             g_pShMemStruct->pppReadTimes[i][j][1] < g_pShMemStruct->pppWriteTimes[k][l][1]))
                        {
                            printf("Failed: A read occurred during a write");
                            exit(-1);
                        }
                    }
                }
            }
        }

        /* Validate no write occurred during another write */
        for(i = 0; i < (SIM_PROCESSES / 2); ++i)
        {
            for(j = 0; j < WRITES_PERFORMED; ++j)
            {
                for(k = 0; k < (SIM_PROCESSES / 2); ++k)
                {
                    for(l = 0; l < WRITES_PERFORMED; ++l)
                    {
                        if ((i != k || j != l) &&
                            ((g_pShMemStruct->pppWriteTimes[i][j][0] > g_pShMemStruct->pppWriteTimes[k][l][0] &&
                              g_pShMemStruct->pppWriteTimes[i][j][0] < g_pShMemStruct->pppWriteTimes[k][l][1]) ||
                             (g_pShMemStruct->pppWriteTimes[i][j][1] > g_pShMemStruct->pppWriteTimes[k][l][0] &&
                              g_pShMemStruct->pppWriteTimes[i][j][1] < g_pShMemStruct->pppWriteTimes[k][l][1])))
                        {
                            printf("Failed: A write occurred during another write");
                            exit(-1);
                        }
                    }
                }
            }
        }
    }

    exit(0);
}
