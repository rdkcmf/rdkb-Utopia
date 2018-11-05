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

#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


/*
 * Hooked-function helper macros
 */

#define FUNCTION_DECLARE(fn, returnType, argTypes) returnType (*g_pFn_##fn)argTypes = 0;
#define FUNCTION_LOAD(fn) \
    if (!g_pFn_##fn) \
    { \
        void* p = dlsym((void*)-1L /*RTLD_NEXT*/, #fn); \
        memcpy(&g_pFn_##fn, &p, sizeof(g_pFn_##fn)); \
    }

#define FUNCTION_CALL(fn) g_pFn_##fn

/* Hooked functions */
FUNCTION_DECLARE(malloc, void*, (size_t))
FUNCTION_DECLARE(realloc, void*, (void*, size_t))
FUNCTION_DECLARE(free, void, (void*))
FUNCTION_DECLARE(exit, void, (int) __attribute__((noreturn)))
FUNCTION_DECLARE(mktime, time_t, (struct tm *__tp))
FUNCTION_DECLARE(localtime_r, struct tm *, (__const time_t* __restrict __timer, struct tm* __restrict __tp))
FUNCTION_DECLARE(gmtime_r, struct tm *, (__const time_t* __restrict __timer, struct tm* __restrict __tp))


/*
 * Debug output functions
 */

#ifdef MALLOC_INTERPOSER_DEBUG
/* Malloc-interposer trace function */
static void Trace(char* pszStr, ...)
{
    va_list args;
    va_start(args, pszStr);
    fprintf(stderr, __FILE__ " - ");
    vfprintf(stderr,pszStr, args);
    va_end(args);
}
#endif

/* Malloc-interposer report function */
static void Report(char* pszStr, ...)
{
    va_list args;
    va_start(args, pszStr);
    printf(__FILE__ " - ");
    vprintf(pszStr, args);
    va_end(args);
}


/*
 * Malloc statistics
 */

typedef struct _MallocStats
{
    struct _BlockInfo* pBlockInfoHead;
    int fNoStats;
    int cMalloc;
    int cFree;
    int cbMallocTotal;
    int cbMallocCurrent;
    int cbMallocMax;
} MallocStats;

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
MallocStats g_stats =
{
    /* pBlockInfoHead = */ 0,
    /* fNoStats = */ 0,
    /* cMalloc = */ 0,
    /* cFree = */ 0,
    /* cbMallocTotal = */ 0,
    /* cbMallocCurrent = */ 0,
    /* cbMallocMax = */ 0,
};

static MallocStats* LockStats()
{
    pthread_mutex_lock(&g_mutex);
    return &g_stats;
}

static void UnlockStats(MallocStats* pStats)
{
    /* Unused parameters */
    (void)pStats;

    pthread_mutex_unlock(&g_mutex);
}

/*
 * Interposed-block collection/info struct
 */

typedef struct _BlockInfo
{
    struct _BlockInfo* pNext;
    void* p;
    size_t size;
} BlockInfo;

static BlockInfo* BlockInfo_Add(MallocStats* pStats, void* p, size_t size)
{
    BlockInfo* pInfo;
    FUNCTION_LOAD(malloc);
    pInfo = (BlockInfo*)FUNCTION_CALL(malloc)(sizeof(BlockInfo));
    if (pInfo)
    {
        pInfo->pNext = pStats->pBlockInfoHead;
        pStats->pBlockInfoHead = pInfo;
        pInfo->p = p;
        pInfo->size = size;
    }
    return pInfo;
}

static void BlockInfo_Remove(MallocStats* pStats, void* p)
{
    BlockInfo* pPrev = 0;
    BlockInfo* pCur;
    for (pCur = pStats->pBlockInfoHead; pCur; pCur = pCur->pNext)
    {
        if (p == pCur->p)
        {
            *(pPrev ? &pPrev->pNext : &pStats->pBlockInfoHead) = pCur->pNext;
            FUNCTION_LOAD(free);
            FUNCTION_CALL(free)(pCur);
            break;
        }
        pPrev = pCur;
    }
}

static BlockInfo* BlockInfo_Get(MallocStats* pStats, void* p)
{
    BlockInfo* pInfo;
    for (pInfo = pStats->pBlockInfoHead; pInfo; pInfo = pInfo->pNext)
    {
        if (p == pInfo->p)
        {
            return pInfo;
        }
    }
    return 0;
}

static void BlockInfo_Dump(MallocStats* pStats)
{
    BlockInfo* pInfo;
    for (pInfo = pStats->pBlockInfoHead; pInfo; pInfo = pInfo->pNext)
    {
        Report("Memory leak: %p, %d bytes\n", pInfo->p, pInfo->size);
    }
}


/*
 * Interposed functions
 */

/* Interpose malloc */
void* malloc(size_t size)
{
    MallocStats* pStats = LockStats();

    void* p;
    BlockInfo* pInfo = 0;

    /* Add the block info - do this prior to malloc so as not to confuse glibc */
    if (!pStats->fNoStats)
    {
        pInfo = BlockInfo_Add(pStats, 0, size);
    }

    /* Call the non-interposed function */
    FUNCTION_LOAD(malloc);
    p = FUNCTION_CALL(malloc)(size);
#ifdef MALLOC_INTERPOSER_DEBUG
    Trace("DEBUG - malloc: %p, %u - fNoStats = %d\n", p, size, pStats->fNoStats);
#endif

    /* Collect statistics */
    if (!pStats->fNoStats)
    {
        if (p)
        {
            pInfo->p = p;

            /* Collect statistics */
            pStats->cMalloc++;
#ifdef MALLOC_INTERPOSER_DEBUG
            Trace("DEBUG - malloc: %p, %u - cMalloc++ (%d), fNoStats = %d\n", p, size, pStats->cMalloc, pStats->fNoStats);
#endif
            pStats->cbMallocTotal += size;
            pStats->cbMallocCurrent += size;
            if (pStats->cbMallocCurrent > pStats->cbMallocMax)
            {
                pStats->cbMallocMax = pStats->cbMallocCurrent;
            }
        }
        else
        {
            Report("malloc: NULL returned for %d bytes!", size);
            BlockInfo_Remove(pStats, 0);
        }
    }

    UnlockStats(pStats);

    return p;
}

/* Interpose realloc */
void* realloc(void* pOld, size_t size)
{
    MallocStats* pStats = LockStats();

    void* p;
    BlockInfo* pInfo;

    /* Call the non-interposed function */
    FUNCTION_LOAD(realloc);
    p = FUNCTION_CALL(realloc)(pOld, size);
#ifdef MALLOC_INTERPOSER_DEBUG
    Trace("DEBUG - realloc: %p, %p, %u - fNoStats = %d\n", pOld, p, size, pStats->fNoStats);
#endif

    /* Collect statistics */
    if (!pStats->fNoStats)
    {
        if (p)
        {
            /* Update the old block's info */
            pInfo = BlockInfo_Get(pStats, pOld);
            if (pInfo)
            {
                /* Update statistics */
                pStats->cbMallocTotal += size - pInfo->size;
                pStats->cbMallocCurrent += size - pInfo->size;
                if (pStats->cbMallocCurrent > pStats->cbMallocMax)
                {
                    pStats->cbMallocMax = pStats->cbMallocCurrent;
                }

                /* Update the block info */
                pInfo->p = p;
                pInfo->size = size;
            }
            else
            {
                /* Add the block info */
                pInfo = BlockInfo_Add(pStats, p, size);

                /* Update statistics */
                pStats->cMalloc++;
#ifdef MALLOC_INTERPOSER_DEBUG
                Trace("DEBUG - realloc: %p, %p, %u - cMalloc++ (%d), fNoStats = %d\n", pOld, p, size, pStats->cMalloc, pStats->fNoStats);
#endif
                pStats->cbMallocTotal += size;
                pStats->cbMallocCurrent += size;
                if (pStats->cbMallocCurrent > pStats->cbMallocMax)
                {
                    pStats->cbMallocMax = pStats->cbMallocCurrent;
                }
            }
        }
    }
    else
    {
        Report("realloc: NULL returned for %p, %d bytes!", pOld, size);
    }

    UnlockStats(pStats);

    return p;
}

/* Interpose free */
void free(void* p)
{
    MallocStats* pStats = LockStats();

    /* Collect statistics */
    if (!pStats->fNoStats && p)
    {
        /* Remove the block's info */
        BlockInfo* pInfo = BlockInfo_Get(pStats, p);
        if (pInfo)
        {
            /* Update statistics */
            pStats->cFree++;
#ifdef MALLOC_INTERPOSER_DEBUG
            Trace("DEBUG - free: %p - cFree++ (%d), fNoStats = %d\n", p, pStats->cFree, pStats->fNoStats);
#endif
            pStats->cbMallocCurrent -= pInfo->size;

            /* Remove the block info */
            BlockInfo_Remove(pStats, p);
        }
    }

    /* Call the non-interposed function */
    FUNCTION_LOAD(free);
    FUNCTION_CALL(free)(p);
#ifdef MALLOC_INTERPOSER_DEBUG
    Trace("DEBUG - free: %p - fNoStats = %d\n", p, pStats->fNoStats);
#endif

    UnlockStats(pStats);
}

/* Interpose exit */
void exit(int status)
{
    /* Report */
    {
        MallocStats* pStats = LockStats();

        Report("************************************************************\n");
        BlockInfo_Dump(pStats);
        Report("exit: malloc called %d times\n", pStats->cMalloc);
        Report("exit: free called %d times\n", pStats->cFree);
        Report("exit: malloc current = %d\n", pStats->cbMallocCurrent);
        Report("exit: malloc total = %d\n", pStats->cbMallocTotal);
        Report("exit: malloc max = %d\n", pStats->cbMallocMax);
        Report("************************************************************\n");

        UnlockStats(pStats);
    }

    /* Call the non-interposed function */
    FUNCTION_LOAD(exit);
    FUNCTION_CALL(exit)(status);
}

/*
 * mktime, localtime_r, and gmtime_r, all may allocate memory which is
 * never freed - do not track these allocations.
 */

/* Interpose mktime */
time_t mktime (struct tm* ptm)
{
    time_t result;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(mktime);
    result = FUNCTION_CALL(mktime)(ptm);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return result;
}

/* Interpose localtime_r */
struct tm* localtime_r(__const time_t* pt, struct tm* ptm)
{
    struct tm* pResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(localtime_r);
    pResult = FUNCTION_CALL(localtime_r)(pt, ptm);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return pResult;
}

/* Interpose gmtime_r */
struct tm* gmtime_r(__const time_t* pt, struct tm* ptm)
{
    struct tm* pResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(gmtime_r);
    pResult = FUNCTION_CALL(gmtime_r)(pt, ptm);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return pResult;
}
