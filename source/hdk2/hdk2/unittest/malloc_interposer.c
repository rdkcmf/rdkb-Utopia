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
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>
#include <libxml/parser.h>

/*
 * Hooked-function helper macros
 */

#ifdef __APPLE__

#define INTERPOSED_FUNCTION(fn) interposed_##fn

#define FUNCTION_DECLARE(fn, returnType, argTypes) returnType INTERPOSED_FUNCTION(fn) argTypes __attribute__ ((visibility("hidden")));

#define FUNCTION_LOAD(fn)
#define FUNCTION_CALL(fn) fn

#else /* ndef __APPLE__ */

#define INTERPOSED_FUNCTION(fn) fn

#define FUNCTION_DECLARE(fn, returnType, argTypes) returnType (*g_pFn_##fn)argTypes = 0;
#define FUNCTION_LOAD(fn) \
    if (!g_pFn_##fn) \
    { \
        void* p = dlsym((void*)-1L /*RTLD_NEXT*/, #fn); \
        memcpy(&g_pFn_##fn, &p, sizeof(g_pFn_##fn)); \
    }

#define FUNCTION_CALL(fn) g_pFn_##fn

#endif /* def __APPLE__ */

/* c-runtime hooked functions */
FUNCTION_DECLARE(malloc, void*, (size_t))
FUNCTION_DECLARE(realloc, void*, (void*, size_t))
FUNCTION_DECLARE(free, void, (void*))
FUNCTION_DECLARE(exit, void, (int) __attribute__((noreturn)))
FUNCTION_DECLARE(mktime, time_t, (struct tm *__tp))
FUNCTION_DECLARE(localtime_r, struct tm *, (__const time_t* __restrict __timer, struct tm* __restrict __tp))
FUNCTION_DECLARE(gmtime_r, struct tm *, (__const time_t* __restrict __timer, struct tm* __restrict __tp))
#ifdef __APPLE__
/* The MAC OS X c runtime allocates a static buffers on the heap when first called for the following (which we should ignore) */
FUNCTION_DECLARE(fwrite, size_t, (const void * ptr, size_t size, size_t count, FILE* stream))
FUNCTION_DECLARE(printf, int, (const char* s, ...))
FUNCTION_DECLARE(putchar, int, (int c))
FUNCTION_DECLARE(puts, int, (const char* s))
FUNCTION_DECLARE(snprintf, int, (char* s, size_t n, const char* format, ...))
#endif

/* libxml2 hooked functions */
FUNCTION_DECLARE(xmlCreatePushParserCtxt, xmlParserCtxtPtr, (xmlSAXHandlerPtr sax, void* user_data, const char* chunk, int size, const char* filename))
FUNCTION_DECLARE(xmlParseChunk, int, (xmlParserCtxtPtr ctxt, const char* chunk, int size, int terminate))

/* libcurl hooked functions */
FUNCTION_DECLARE(curl_global_init, CURLcode, (long))
FUNCTION_DECLARE(curl_global_cleanup, void, (void))
FUNCTION_DECLARE(curl_easy_perform, CURLcode, (CURL*))


#ifdef __APPLE__

typedef struct _InterposedFunction
{
    void* fnInterposed;
    void* fnOriginal;
} InterposedFunction;

#define FUNCTION_MAP_BEGIN \
static const InterposedFunction g_interposedFunctions[] __attribute__ ((section("__DATA, __interpose"))) = \
{

#define FUNCTION_MAP_ENTRY(fn) \
    { (void*)INTERPOSED_FUNCTION(fn), (void*)fn },

#define FUNCTION_MAP_END \
};

FUNCTION_MAP_BEGIN
FUNCTION_MAP_ENTRY(malloc)
FUNCTION_MAP_ENTRY(malloc)
FUNCTION_MAP_ENTRY(realloc)
FUNCTION_MAP_ENTRY(free)
FUNCTION_MAP_ENTRY(exit)
FUNCTION_MAP_ENTRY(mktime)
FUNCTION_MAP_ENTRY(localtime_r)
FUNCTION_MAP_ENTRY(gmtime_r)
FUNCTION_MAP_ENTRY(fwrite)
FUNCTION_MAP_ENTRY(printf)
FUNCTION_MAP_ENTRY(putchar)
FUNCTION_MAP_ENTRY(puts)
FUNCTION_MAP_ENTRY(snprintf)
FUNCTION_MAP_ENTRY(xmlCreatePushParserCtxt)
FUNCTION_MAP_ENTRY(xmlParseChunk)
FUNCTION_MAP_ENTRY(curl_global_init)
FUNCTION_MAP_ENTRY(curl_global_cleanup)
FUNCTION_MAP_ENTRY(curl_easy_perform)
FUNCTION_MAP_END

#endif /* def __APPLE__ */


/*
 * Debug output functions
 */

#ifdef MALLOC_INTERPOSER_DEBUG
/* Malloc-interposer trace function */
static void Trace(const char* pszStr, ...)
{
    va_list args;
    va_start(args, pszStr);
    fprintf(stderr, __FILE__ " - ");
    vfprintf(stderr, pszStr, args);
    va_end(args);
}
#endif

/* Malloc-interposer report function */
static void Report(const char* pszStr, ...)
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

static void BlockInfo_ClearAll(MallocStats* pStats)
{
    BlockInfo* pCur = pStats->pBlockInfoHead;
    while (pCur)
    {
        BlockInfo* pToFree = pCur;
        pCur = pCur->pNext;

        FUNCTION_LOAD(free);
        FUNCTION_CALL(free)(pToFree);
    }

    pStats->pBlockInfoHead = 0;
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
 * Exported functions
 */
#ifdef __cplusplus
extern "C" void clearstats(void) throw();
void clearstats(void) throw()
#else
extern void clearstats(void);
void clearstats(void)
#endif
{
    MallocStats* pStats = LockStats();

    BlockInfo_ClearAll(pStats);

    memset(pStats, 0, sizeof(*pStats));

    UnlockStats(pStats);
}


/*
 * Interposed functions
 */

/* Interpose malloc */
#ifdef __cplusplus
void* INTERPOSED_FUNCTION(malloc)(size_t size) throw()
#else
void* INTERPOSED_FUNCTION(malloc)(size_t size)
#endif
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
    FUNCTION_LOAD(free);
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
#ifdef __cplusplus
void* INTERPOSED_FUNCTION(realloc)(void* pOld, size_t size) throw()
#else
void* INTERPOSED_FUNCTION(realloc)(void* pOld, size_t size)
#endif
{
    MallocStats* pStats = LockStats();

    void* p;
    BlockInfo* pInfo;

    /* Call the non-interposed function */
    FUNCTION_LOAD(realloc);
    FUNCTION_LOAD(free);
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
        else
        {
            Report("realloc: NULL returned for %p, %d bytes!", pOld, size);
        }
    }

    UnlockStats(pStats);

    return p;
}

/* Interpose free */
#ifdef __cplusplus
void INTERPOSED_FUNCTION(free)(void* p) throw()
#else
void INTERPOSED_FUNCTION(free)(void* p)
#endif
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
#ifdef __cplusplus
void INTERPOSED_FUNCTION(exit)(int status) throw()
#else
void INTERPOSED_FUNCTION(exit)(int status)
#endif
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
#ifdef __cplusplus
time_t INTERPOSED_FUNCTION(mktime)(struct tm* ptm) throw()
#else
time_t INTERPOSED_FUNCTION(mktime)(struct tm* ptm)
#endif
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
#ifdef __cplusplus
struct tm* INTERPOSED_FUNCTION(localtime_r)(__const time_t* pt, struct tm* ptm) throw()
#else
struct tm* INTERPOSED_FUNCTION(localtime_r)(__const time_t* pt, struct tm* ptm)
#endif
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
#ifdef __cplusplus
struct tm* INTERPOSED_FUNCTION(gmtime_r)(__const time_t* pt, struct tm* ptm) throw()
#else
struct tm* INTERPOSED_FUNCTION(gmtime_r)(__const time_t* pt, struct tm* ptm)
#endif
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

#ifdef __APPLE__
/* Interpose fwrite */
#  ifdef __cplusplus
size_t INTERPOSED_FUNCTION(fwrite)(const void* ptr, size_t size, size_t count, FILE* stream) throw()
#  else
size_t INTERPOSED_FUNCTION(fwrite)(const void* ptr, size_t size, size_t count, FILE* stream)
#  endif
{
    size_t iResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(fwrite);
    iResult = FUNCTION_CALL(fwrite)(ptr, size, count, stream);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return iResult;
}

/* Interpose printf */
#  ifdef __cplusplus
int INTERPOSED_FUNCTION(printf)(const char* s, ...) throw()
#  else
int INTERPOSED_FUNCTION(printf)(const char* s, ...)
#  endif
{
    int iResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    {
        va_list args;
        va_start(args, s);
        iResult = vprintf(s, args);
        va_end(args);
    }
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return iResult;
}

/* Interpose puts */
#  ifdef __cplusplus
int INTERPOSED_FUNCTION(puts)(const char* s) throw()
#  else
int INTERPOSED_FUNCTION(puts)(const char* s)
#  endif
{
    int iResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(puts);
    iResult = FUNCTION_CALL(puts)(s);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return iResult;
}

/* Interpose puts */
#  ifdef __cplusplus
int INTERPOSED_FUNCTION(putchar)(int c) throw()
#  else
int INTERPOSED_FUNCTION(putchar)(int c)
#  endif
{
    int iResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(putchar);
    iResult = FUNCTION_CALL(putchar)(c);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return iResult;
}

/* Interpose snprintf */
#  ifdef __cplusplus
int INTERPOSED_FUNCTION(snprintf)(char* s, size_t n, const char* format, ...) throw()
#  else
int INTERPOSED_FUNCTION(snprintf)(char* s, size_t n, const char* format, ...)
#  endif
{
    int iResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    {
        va_list args;
        va_start(args, format);
        iResult = vsnprintf(s, n, format, args);
        va_end(args);
    }
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return iResult;
}

#endif /* def __APPLE__ */

/* Interpose xmlCreatePushParserCtxt */
#ifdef __cplusplus
xmlParserCtxtPtr INTERPOSED_FUNCTION(xmlCreatePushParserCtxt)(xmlSAXHandlerPtr sax, void* user_data, const char* chunk, int size, const char* filename) throw()
#else
xmlParserCtxtPtr INTERPOSED_FUNCTION(xmlCreatePushParserCtxt)(xmlSAXHandlerPtr sax, void* user_data, const char* chunk, int size, const char* filename)
#endif
{
    xmlParserCtxtPtr parser;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(xmlCreatePushParserCtxt);
    parser = FUNCTION_CALL(xmlCreatePushParserCtxt)(sax, user_data, chunk, size, filename);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return parser;
}

/* Interpose xmlParseChunk */
#ifdef __cplusplus
int INTERPOSED_FUNCTION(xmlParseChunk)(xmlParserCtxtPtr ctxt, const char* chunk, int size, int terminate) throw()
#else
int INTERPOSED_FUNCTION(xmlParseChunk)(xmlParserCtxtPtr ctxt, const char* chunk, int size, int terminate)
#endif
{
    int iResult;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(xmlParseChunk);
    iResult = FUNCTION_CALL(xmlParseChunk)(ctxt, chunk, size, terminate);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return iResult;
}


/* Interpose curl_global_init */
#ifdef __cplusplus
CURLcode INTERPOSED_FUNCTION(curl_global_init)(long flags) throw()
#else
CURLcode INTERPOSED_FUNCTION(curl_global_init)(long flags)
#endif
{
    /*
        curl_global_init will cause memory leaks to be reported when called with the CURL_GLOBAL_SSL flag.
    */
    CURLcode code;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(curl_global_init);
    code = FUNCTION_CALL(curl_global_init)(flags);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return code;
}

/* Interpose curl_global_cleanup */
#ifdef __cplusplus
void INTERPOSED_FUNCTION(curl_global_cleanup)(void) throw()
#else
void INTERPOSED_FUNCTION(curl_global_cleanup)(void)
#endif
{
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(curl_global_cleanup);
    FUNCTION_CALL(curl_global_cleanup)();
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return;
}

/* Interpose curl_easy_perform */
#ifdef __cplusplus
CURLcode INTERPOSED_FUNCTION(curl_easy_perform)(CURL* pCURL) throw()
#else
CURLcode INTERPOSED_FUNCTION(curl_easy_perform)(CURL* pCURL)
#endif
{
    /*
        curl_easy_perform will cause memory leaks to be reported when called (presumably related to the SSL issues seen with curl_global_init) .
    */
    CURLcode code;
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats++;
        UnlockStats(pStats);
    }
    FUNCTION_LOAD(curl_easy_perform);
    code = FUNCTION_CALL(curl_easy_perform)(pCURL);
    {
        MallocStats* pStats = LockStats();
        pStats->fNoStats--;
        UnlockStats(pStats);
    }

    return code;
}
