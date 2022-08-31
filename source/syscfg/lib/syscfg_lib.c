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

/* _GNU_SOURCE is needed for strchrnul() and program_invocation_short_name */

//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#ifdef SC_SYSV_SEM
#include <sys/sem.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <ulog/ulog.h>
#include "syscfg_lib.h"   // internal interface
#include "syscfg.h"       // external interface used by users
#include "safec_lib_common.h"

//#define VERBOSE_DEBUG

/*
 * Global data structures
 */
static syscfg_shm_ctx *syscfg_ctx = NULL;
static int syscfg_initialized = 0;

static char name_p[MAX_NAME_LEN+1];                      // internal temp name buffer

static int load_from_file (const char *fname);
static int commit_to_file (const char *fname);

/******************************************************************************
 *                External syscfg library access apis
 *****************************************************************************/

/*
 * Procedure     : syscfg_get
 * Purpose       : Retrieve an entry from syscfg
 * Parameters    :   
 *   ns  -  namespace string (optional)
 *   name  - name string, entry to add
 *   out_val  - buffer to store output value string
 *   outbufsz  - output buffer size
 * Return Values :
 *    0 - success
 *    ERR_xxx - various errors codes dependening on the failure
 */
int syscfg_get (const char *ns, const char *name, char *out_val, int outbufsz)
{
    char *val;
    size_t len;

    if (NULL == name || NULL == out_val) {
        if (out_val != NULL) {
            out_val[0] = 0;
        }
        return -1;
    }

    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            out_val[0] = 0;
            return rc;
        }
    }

    val = _syscfg_get(ns, name);

    if (val == NULL) {
        out_val[0] = 0;
        return -1;
    }

    len = strlen(val);

    if (len >= outbufsz) {
        memcpy(out_val, val, outbufsz - 1);
        out_val[outbufsz - 1] = 0;
#if defined (VERBOSE_DEBUG)
        fprintf(stderr, "syscfg_get: %s outbufsz too small (%d < %d) (%s: lr %p)\n", name, outbufsz, (int) len + 1, program_invocation_short_name, __builtin_extract_return_addr (__builtin_return_address (0)));
#endif
    }
    else {
        memcpy(out_val, val, len + 1);
    }

    return 0;
}

/*
 * Procedure     : syscfg_set
 * Purpose       : Adds an entry to syscfg
 * Parameters    :   
 *   ns  -  namespace string (optional)
 *   name  - name string, entry to add
 *   value  - value string to associate with name
 * Return Values :
 *    0 - success
 *    ERR_xxx - various errors codes dependening on the failure
 * Notes         :
 *    Only changes syscfg hash table, persistent store contents
 *    not changed until 'commit' operation
 */
int syscfg_set_ns (const char *ns, const char *name, const char *value)
{
    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            return rc;
        }
    }
    if (NULL == name || NULL == value) {
        return ERR_INVALID_PARAM;
    }

    return _syscfg_set(ns, name, value, 0);
}

int syscfg_set_ns_commit (const char *ns, const char *name, const char *value)
{
    int result = syscfg_set_ns (ns, name, value);
    if (result == 0)
        result = syscfg_commit();
    return result;
}

int syscfg_set_ns_u (const char *ns, const char *name, unsigned long value)
{
    char buf[sizeof(long)*3];
    sprintf (buf, "%lu", value);
    return syscfg_set_ns (ns, name, buf);
}

int syscfg_set_ns_u_commit (const char *ns, const char *name, unsigned long value)
{
    int result = syscfg_set_ns_u (ns, name, value);
    if (result == 0)
        result = syscfg_commit();
    return result;
}

int syscfg_set_nns (const char *name, const char *value)
{
    return syscfg_set_ns (NULL, name, value);
}

int syscfg_set_nns_commit (const char *name, const char *value)
{
    return syscfg_set_ns_commit (NULL, name, value);
}

int syscfg_set_nns_u (const char *name, unsigned long value)
{
    return syscfg_set_ns_u (NULL, name, value);
}

int syscfg_set_nns_u_commit (const char *name, unsigned long value)
{
    return syscfg_set_ns_u_commit (NULL, name, value);
}

/*
 * Procedure     : syscfg_getall
 * Purpose       : Retrieve all entries from syscfg
 * Parameters    :   
 *   buf  -  output buffer to store syscfg entries
 *   bufsz  - size of output buffer
 *   outsz  - number of bytes return into given buffer
 * Return Values :
 *    0       - on success
 *    ERR_xxx - various errors codes dependening on the failure
 * Notes         :
 *    useful for clients to dump the whole syscfg data
 */
int syscfg_getall (char *buf, int bufsz, int *outsz)
{
    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            return rc;
        }
    }
    if (NULL == buf) {
        return ERR_INVALID_PARAM;
    }

    *outsz = _syscfg_getall(buf, bufsz);
    return 0;
}

/*
 * Procedure     : syscfg_unset
 * Purpose       : Remove an entry from syscfg
 * Parameters    :   
 *   ns  -  namespace string (optional)
 *   name  - name string, entry to remove
 * Return Values :
 *    0 - success
 *    ERR_xxx - various errors codes dependening on the failure
 * Notes         :
 *    Only changes syscfg hash table, persistent store contents
 *    not changed until 'commit' operation
 */
int syscfg_unset (const char *ns, const char *name)
{
    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            return rc;
        }
    }
    if (NULL == name) {
        return ERR_INVALID_PARAM;
    }

    return _syscfg_unset(ns, name, 0);
}

/*
 * Procedure     : syscfg_is_match
 * Purpose       : Compare the value of an entry from syscfg 
 * Parameters    :   
 *   ns  -  namespace string (optional)
 *   name  - name string, entry to add
 *   value  - value string to be matched
 *   out_match  - 1 is match, 0 if not
 * Return Values :
 *    0 - success
 *    ERR_xxx - various errors codes dependening on the failure
 */
int syscfg_is_match (const char *ns, const char *name, char *value, unsigned int *out_match)
{
    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            return rc;
        }
    }

    if (NULL == name || NULL == value || NULL == out_match) {
        return -1;
    }

    char *val = _syscfg_get(ns, name);
    if (val) {
        *out_match = (0 == strcmp(val, value)) ? 1 : 0;
        return 0;
    } 
    *out_match = 0;
    return 0;
}


/*
 * Procedure     : syscfg_getsz
 * Purpose       : Get current & maximum peristent storage size 
 *                 of syscfg content
 * Parameters    : 
 *                 used_sz - return buffer of used size
 *                 max_sz - return buffer of max size
 * Return Values :
 *    0 - success
 *    ERR_xxx - various errors codes dependening on the failure
 */
int syscfg_getsz (long int *used_sz, long int *max_sz)
{
    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            return rc;
        }
    }

    return _syscfg_getsz(used_sz, max_sz);
}

/*
 * Procedure     : syscfg_commit
 * Purpose       : commits current stats of syscfg hash table data
 *                 to persistent store
 * Parameters    :   
 *   None
 * Return Values :
 *    0 - success
 *    ERR_IO_xxx - various IO errors dependening on the failure
 * Notes         :
 *    WARNING: will overwrite persistent store
 *    Persistent store location specified during syscfg_create() is cached 
 *    in syscfg shared memory and used as the target for commit
 */
int syscfg_commit (void)
{
    syscfg_shm_ctx *ctx;
    int rc;

    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            return rc;
        }
    }
  
    ctx = syscfg_ctx;

    write_lock(ctx);
    commit_lock(ctx);

    rc = commit_to_file(ctx->cb.store_path);

    commit_unlock(ctx);
    write_unlock(ctx);
    
    #if !defined (_ARRIS_XB6_PRODUCT_REQ_)
        ulog(ULOG_SYSTEM, UL_SYSCFG, "commit to store");
    #endif
    return rc;
}

/*
 * Procedure     : syscfg_destroy
 * Purpose       : Destroy syscfg shared memory context
 * Parameters    :   
 *   None
 * Return Values :
 *    0              - success
 *    ERR_INVALID_PARAM - invalid arguments
 *    ERR_IO_FAILURE - syscfg file unavailable
 * Notes         :
 *   syscfg destroy should happen only during system shutdown.
 *   *NEVER* call this API in any other scenario!!
 */
void syscfg_destroy (void)
{
    if (syscfg_initialized == 0) {
        int rc = syscfg_init();
        if (rc != 0) {
            return;
        }
    }

    if (syscfg_initialized) {
        _syscfg_destroy();
        syscfg_initialized = 0;
    }
}

/*
 * Procedure     : syscfg_create
 * Purpose       : SYSCFG initialization from persistent storage
 * Parameters    :   
 *   file - filesystem 'file' where syscfg is stored
 * Return Values :
 *    0              - success
 *    ERR_INVALID_PARAM - invalid arguments
 *    ERR_IO_FAILURE - syscfg file unavailable
 * Notes         :
 */
int syscfg_create (const char *file, long int max_file_sz)
{
    if (file == NULL) {
        return ERR_INVALID_PARAM;
    }
    ulog_LOG_Info("Enter in function %s \n", __FUNCTION__);

    store_info_t store_info;
    int rc, shmid = -1;

    store_info.type = STORE_FILE;
    strncpy(store_info.path, file, sizeof(store_info.path)-1);
    store_info.path[sizeof(store_info.path)-1] = '\0';
    store_info.max_size = (max_file_sz > 0) ? max_file_sz : DEFAULT_MAX_FILE_SZ;
    store_info.hdr_size = 0;

    ulog_LOG_Info("creating shared memory with store type %d, path %s, size %ld, hdr size %d \n", store_info.type, store_info.path, store_info.max_size, store_info.hdr_size);

    syscfg_ctx = (syscfg_shm_ctx *) syscfg_shm_create(&store_info, &shmid);
    if (NULL == syscfg_ctx) {
        ulog_LOG_Err("Error creating shared memory");
        return ERR_SHM_CREATE;
    }

    // ready for operation
    syscfg_initialized = 1;

    rc = load_from_file(store_info.path);

    if (0 != rc) {
        ulog_LOG_Err("Error loading from store");
    }

    shmdt(syscfg_ctx);

    return 0;
}

int syscfg_reload(const char *file) {
    int rc = 0;

    if (NULL == file)
        return ERR_INVALID_PARAM;

    ulog_LOG_Info("Enter in function %s \n", __FUNCTION__);

    rc = load_from_file(file);
    if (0 != rc)
        ulog_LOG_Err("Error reloading config file %s", file);

    return rc;
}

int syscfg_commit_lock() {
    syscfg_shm_ctx *ctx = syscfg_ctx;
    return commit_lock(ctx);
}

int syscfg_commit_unlock() {
    syscfg_shm_ctx *ctx = syscfg_ctx;
    return commit_unlock(ctx);
}

/*
 * Procedure     : syscfg_init
 * Purpose       : Initialization to attach current process to syscfg
 *                 shared memory based context
 * Parameters    :   
 * Return Values :
 *    0              - success
 *    ERR_INVALID_PARAM - invalid arguments
 *    ERR_IO_FAILURE - syscfg file unavailable
 * Notes         :
 */
int syscfg_init (void)
{
    syscfg_shm_ctx *ctx;
    int rc;

    if (syscfg_initialized) {
        return 0;
    }

    ulog_init();

    ctx = NULL;
    rc = syscfg_shm_init(&ctx);
    if (rc || NULL == ctx) {
	ulog_LOG_Dbg("Error initializing shared memor");
        return rc;
    }

    syscfg_ctx = ctx;
    syscfg_initialized = 1;

    return 0;
}

/******************************************************************************
 *                Internal utility routines
 *****************************************************************************/

/*
 * Procedure     : syscfg_parse
 * Purpose       : parses one name=value at buffer 
 * Parameters    :   
 *   str   - input buffer
 *   name  - pointer to mem location of name
 *   value - pointer to mem location of value
 * Return Values :
 *    buf - points to the terminating char past the 'value' string
 *    name - malloc'd buffer with 'name' string
 *    value - malloc'd buffer with 'value' string
 * Notes         :
 *   caller need to free 'name' & 'value' 
 */
static char *syscfg_parse (const char *str, char **name, char **value)
{
    char *n, *p;
    int len;

    if (NULL == str || NULL == name || NULL == value) {
        return NULL;
    }

    *name = *value = NULL;

    if ((n = strchr(str,'='))) {
        len = n - str;
        *name = malloc(len+1);
        if (*name) {
            memcpy(*name, str, len);
            (*name)[len] = '\0';
            n++;
            p = strchrnul(n,'\n');
            if (p) {
                len = p - n;
                *value = malloc(len+1);
                if (*value) {
                    memcpy(*value, n, len);
                    (*value)[len] = '\0';
                    return p;
                }
            }
        }
    }

    if (*name) {
        free(*name);
        *name = NULL;
    }
    return NULL;
}

// dbj2 hash: hash * 33 + str[i]
static unsigned int hash (const char *str)
{
    unsigned int hash = 5381 % SYSCFG_HASH_TABLE_SZ;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }

    return hash % SYSCFG_HASH_TABLE_SZ;
}

/******************************************************************************
 *                Internal lock apis
 *****************************************************************************/

#ifdef SC_POSIX_SEM

static int lock_init (syscfg_shm_ctx *ctx)
{
    shm_cb *cb = &(ctx->cb);

    int err;

    pthread_mutexattr_t mattr;

    pthread_mutexattr_init(&mattr);

    // set up the atributes for a robust mutex
    err = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
    if (err) {
        ulog_errorf(ULOG_SYSTEM, UL_SYSCFG, "pthread_mutexattr_setprotocol error %d: %s\n",
            err, strerror(err));
        return ERR_SEMAPHORE_INIT;
    }
    err = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
    if (err) {
        ulog_errorf(ULOG_SYSTEM, UL_SYSCFG, "pthread_mutexattr_setrobust error %d: %s\n",
            err, strerror(err));
        return ERR_SEMAPHORE_INIT;
    }

    // Build the mutexes using the above attributes
    err = pthread_mutex_init(&cb->write_lock, &mattr);
    if (err) {
        ulog_errorf(ULOG_SYSTEM, UL_SYSCFG, "pthread_mutex_init in write_lock error %d: %s\n", err, strerror(err));
        return ERR_SEMAPHORE_INIT;
    }

    err = pthread_mutex_init(&cb->read_lock, &mattr);
    if (err) {
        ulog_errorf(ULOG_SYSTEM, UL_SYSCFG, "pthread_mutex_init in read_lock error %d: %s\n", err, strerror(err));
        return ERR_SEMAPHORE_INIT;
    }

    err = pthread_mutex_init(&cb->commit_lock, &mattr);
    if (err) {
        ulog_errorf(ULOG_SYSTEM, UL_SYSCFG, "pthread_mutex_init in commit_lock error %d: %s\n", err, strerror(err));
        return ERR_SEMAPHORE_INIT;
    }

    return 0;
}

static inline int read_lock (syscfg_shm_ctx *ctx)
{
    int err = pthread_mutex_lock(&ctx->cb.read_lock);

    if (err == 0) {
        //ulog(ULOG_SYSTEM, UL_SYSCFG, "Process %d locked read mutex\n", (int) getpid());
    } else if (err == EOWNERDEAD) {
        FILE *consolefp = NULL;
        if((consolefp = fopen (LOG_FILE, "a+") )) {
                fprintf(consolefp, "SYSCFG_ERROR:Process %d got EOWNERDEAD for read mutex\n", (int) getpid());
                fclose(consolefp);
        }
       ulog_errorf(ULOG_SYSTEM, UL_SYSCFG, "Process %d got EOWNERDEAD for read mutex\n", (int) getpid());
        err = pthread_mutex_consistent(&ctx->cb.read_lock);
        //ulog(ULOG_SYSTEM, UL_SYSCFG, "Process %d locked read mutex\n", (int) getpid());

    }

    return err;
}

static inline int read_unlock (syscfg_shm_ctx *ctx)
{
    return pthread_mutex_unlock(&ctx->cb.read_lock);
}

static inline int write_lock (syscfg_shm_ctx *ctx)
{
    int err = pthread_mutex_lock(&ctx->cb.write_lock);

    if (err == 0) {
        //ulog(ULOG_SYSTEM, UL_SYSCFG, "Process %d locked write mutex\n", (int) getpid());
    } else if (err == EOWNERDEAD) {
        FILE *consolefp = NULL;
        if((consolefp = fopen (LOG_FILE, "a+") )) {
                fprintf(consolefp, "SYSCFG_ERROR:Process %d got EOWNERDEAD for write mutex\n", (int) getpid());
                fclose(consolefp);
        }
       ulog_errorf(ULOG_SYSTEM, UL_SYSCFG, "Process %d got EOWNERDEAD for write mutex\n", (int) getpid());
       err = pthread_mutex_consistent(&ctx->cb.write_lock);
        //ulog(ULOG_SYSTEM, UL_SYSCFG, "Process %d locked write mutex\n", (int) getpid());
    }

    return err;
}

static inline int write_unlock (syscfg_shm_ctx *ctx)
{
    return pthread_mutex_unlock(&ctx->cb.write_lock);
}

static inline int commit_lock (syscfg_shm_ctx *ctx)
{
    int err = pthread_mutex_lock(&ctx->cb.commit_lock);

    if (err == 0) {
        //ulog(ULOG_SYSTEM, UL_SYSCFG,"Process %d locked commit mutex\n", (int) getpid());
    } else if (err == EOWNERDEAD) {
        FILE *consolefp = NULL;
        if((consolefp = fopen (LOG_FILE, "a+") )) {
                fprintf(consolefp, "SYSCFG_ERROR:Process %d got EOWNERDEAD for commit mutex\n", (int) getpid());
                fclose(consolefp);
        }
        ulog_errorf(ULOG_SYSTEM, UL_SYSCFG,"Process %d got EOWNERDEAD for commit mutex\n", (int) getpid());
        err = pthread_mutex_consistent(&ctx->cb.commit_lock);
        //ulog(ULOG_SYSTEM, UL_SYSCFG,"Process %d locked commit mutex\n", (int) getpid());
    }

    return err;
}

static inline int commit_unlock (syscfg_shm_ctx *ctx)
{
    return pthread_mutex_unlock(&ctx->cb.commit_lock);
}

static int lock_destroy (syscfg_shm_ctx *ctx)
{
    pthread_mutex_destroy(&ctx->cb.read_lock);
    pthread_mutex_destroy(&ctx->cb.write_lock);
    pthread_mutex_destroy(&ctx->cb.commit_lock);

    return 0;
}

#elif SC_SYSV_SEM

union semun {
    int val;                           // value for SETVAL
    struct semid_ds *buf;              // buffer for IPC_STAT & IPC_SET
    unsigned short int *array;         // array for GETALL & SETALL
    struct seminfo *__buf;             // buffer for IPC_INFO
};

// sem array lock relations,
//    0 - read lock
//    1 - write lock
//    2 - commit lock

enum {
    READ_SEM_NUM,
    WRITE_SEM_NUM,
    COMMIT_SEM_NUM
};

static int lock_init (syscfg_shm_ctx *ctx)
{
    int semid;

    key_t key= ftok(SYSCFG_SHM_FILE, SYSCFG_SHM_PROJID);
    if (-1 == (semid = semget(key, 3, 0600 | IPC_CREAT | IPC_EXCL))) {
        return -1;
    }

    union semun semval;
    semval.val = 1;

    if (-1 == semctl(semid, READ_SEM_NUM, SETVAL, semval)) {
        return -1;
    }
    if (-1 == semctl(semid, WRITE_SEM_NUM, SETVAL, semval)) {
        return -1;
    }
    if (-1 == semctl(semid, COMMIT_SEM_NUM, SETVAL, semval)) {
        return -1;
    }
    ctx->cb.semid = semid;

    return 0;
}

static int lock_sysv_sem (int semid, int sem_num)
{
    struct sembuf sops;

    memset(&sops, 0, sizeof(struct sembuf));

    sops.sem_num = sem_num;
    sops.sem_op = -1; // decr by 1
    sops.sem_flg = 0; // wait till lock is acquired 

    if (-1 == semop(semid, &sops, 1)) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "locking. semop failed");
        return -1;
    }
    return 0;
}

static int unlock_sysv_sem (int semid, int sem_num)
{
    struct sembuf sops;

    memset(&sops, 0, sizeof(struct sembuf));

    sops.sem_num = sem_num;
    sops.sem_op = 1; // incr by 1
    sops.sem_flg = 0; // wait till lock is acquired 

    if (-1 == semop(semid, &sops, 1)) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "unlocking. semop failed");
        return -1;
    }
    return 0;
}

static inline int read_lock (syscfg_shm_ctx *ctx)
{
    return lock_sysv_sem((ctx)->cb.semid, READ_SEM_NUM);
}

static inline int read_unlock (syscfg_shm_ctx *ctx)
{
    return unlock_sysv_sem((ctx)->cb.semid, READ_SEM_NUM);
}

static inline int write_lock (syscfg_shm_ctx *ctx)
{
    return lock_sysv_sem((ctx)->cb.semid, WRITE_SEM_NUM);
}

static inline int write_unlock (syscfg_shm_ctx *ctx)
{
    return unlock_sysv_sem((ctx)->cb.semid, WRITE_SEM_NUM);
}

static inline int commit_lock (syscfg_shm_ctx *ctx)
{
    return lock_sysv_sem((ctx)->cb.semid, COMMIT_SEM_NUM);
}

static inline int commit_unlock (syscfg_shm_ctx *ctx)
{
    return unlock_sysv_sem((ctx)->cb.semid, COMMIT_SEM_NUM);
}

static int lock_destroy (syscfg_shm_ctx *ctx)
{
    if (-1 == semctl(ctx->cb.semid, 0, IPC_RMID)) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "system lock destroy failed");
        return -1;
    }
    ctx->cb.semid = -1;
    return 0;
}

#endif

static int rw_lock (syscfg_shm_ctx *ctx)
{
    int rc = read_lock(ctx);
    if (0 == rc) {
        rc = write_lock(ctx);
        if (0 == rc) {
            return 0; // all success
        } else {
            // write lock failed, rollback read lock
            read_unlock(ctx);
        }
    }
    return -1;
}

static int rw_unlock (syscfg_shm_ctx *ctx)
{
    read_unlock(ctx);
    write_unlock(ctx);
    return 0;
}


/******************************************************************************
 *                Internal syscfg library access apis
 *****************************************************************************/

static int make_ht_entry (const char *name, int namelen, const char *value, shmoff_t *out_offset)
{
    syscfg_shm_ctx *ctx = syscfg_ctx;
    char *p_entry_name, *p_entry_value;
    int valuelen = strlen(value);
    int rc = 0;

    int size = namelen + 1 +
               valuelen + 1 +
               sizeof(ht_entry);

    rc = shm_malloc(ctx, size, out_offset);
    if (0 != rc) {
        return rc;
    }

    shmoff_t ht_entry_offset = *out_offset;
    if (ht_entry_offset) {
        ht_entry *entry = HT_ENTRY(ctx,ht_entry_offset);
        entry->name_sz = namelen + 1;
        entry->value_sz = valuelen + 1;
        entry->next = 0;
        p_entry_name = HT_ENTRY_NAME(ctx,ht_entry_offset);
        memcpy(p_entry_name, name, namelen + 1);
        p_entry_value = HT_ENTRY_VALUE(ctx,ht_entry_offset);
        memcpy(p_entry_value, value, valuelen + 1);
    }

    return rc;
}

static void _syscfg_destroy (void)
{
    
    syscfg_shm_destroy(syscfg_ctx);
    syscfg_ctx = NULL;
}

static char* _syscfg_get (const char *ns, const char *name)
{
    int index;
    syscfg_shm_ctx *ctx = syscfg_ctx;

    rw_lock(ctx);

    if (ns) {
        snprintf(name_p, sizeof(name_p), "%s" NS_SEP "%s", ns, name);
    } else {
        snprintf(name_p, sizeof(name_p), "%s", name);
    }

    index = hash(name_p);

    shmoff_t entryoffset = ctx->ht[index];

    while (entryoffset && strcmp(HT_ENTRY_NAME(ctx,entryoffset), name_p)) {
        entryoffset = HT_ENTRY_NEXT(ctx,entryoffset);
    }

    rw_unlock(ctx);

    if (entryoffset) {
        return HT_ENTRY_VALUE(ctx,entryoffset);
    }

    return NULL;
}

/*
 *  Description:
 *     Check current used storage if set can be saved to persistent store
 *     Incr the used count accordingly
 *  Returns
 *         1 - if syscfg storage has enough free space to store the tuple
 *             incr current used storage (in bytes)
 *         0 - otherwise, not enough storage
 *  Note
 *         Since this routine manipulated ctx object, it should be protected
 *         
 */
static int check_decr_store_sz (syscfg_shm_ctx *ctx, int namelen, const char *value)
{
    shm_cb *cb = &(ctx->cb);
    int entry_sz = 0;

    // Size is calculated as "sizeof(name) + sizeof('=') + sizeof(value) + sizeof('\n')
    // note: name here includes namespace and NS_SEP string

    entry_sz = namelen;
    entry_sz += (value) ? strlen(value) : 0;
    entry_sz += 2;

    if ((cb->used_store_size + entry_sz) > cb->max_store_size) {
        // not enough storage space
        return 0;
    } else {
        // we have room, go ahead and decr
        cb->used_store_size += entry_sz;
        return 1;
    }
}

static void incr_store_sz (syscfg_shm_ctx *ctx, int namelen, const char *value)
{
    shm_cb *cb = &(ctx->cb);
    int entry_sz = 0;

    // Size is calculated as "sizeof(name) + sizeof('=') + sizeof(value) + sizeof('\n')
    // note: name here includes namespace and NS_SEP string

    entry_sz = namelen;
    entry_sz += (value) ? strlen(value) : 0;
    entry_sz += 2;

    // we have room, go ahead and decr
    cb->used_store_size -= entry_sz;
}

static int _syscfg_getsz (long int *used_sz, long int *max_sz)
{
    syscfg_shm_ctx *ctx = syscfg_ctx;
    shm_cb *cb = &(ctx->cb);

    write_lock(ctx);
    if (used_sz) {
        *used_sz = cb->used_store_size;
    }
    if (max_sz) {
        *max_sz = cb->max_store_size;
    }
    write_unlock(ctx);

    return 0;
}


/*
 * replace value by unsetting existing entry and setting new entry
 * reason being we might need a new shm_malloc item of different size
 * TODO-OPTIMIZE: do this only if new value doesn't fit current mm_item
 *
 * nolock - indicates if write lock already acquired by caller, hence it need
 *          not be locked (actually shd not be, bcoz it will cause a deadlock)
 */
static int _syscfg_set (const char *ns, const char *name, const char *value, int nolock)
{
    int index, rc = 0;
    syscfg_shm_ctx *ctx = syscfg_ctx;
    size_t namelen;

    if (!nolock) {
        rw_lock(ctx);
    }

    if (ns) {
        namelen = snprintf(name_p, sizeof(name_p), "%s" NS_SEP "%s", ns, name);
    } else {
        namelen = snprintf(name_p, sizeof(name_p), "%s", name);
    }

    if (namelen >= sizeof(name_p))
        namelen = sizeof(name_p) - 1;

    index = hash(name_p);

    if (0 == ctx->ht[index]) {
        if (0 == check_decr_store_sz(ctx, namelen, value)) {
            if (!nolock) {
                rw_unlock(ctx);
            }
            return ERR_NO_SPACE;
        }
        shmoff_t ht_offset;
        rc = make_ht_entry(name_p, namelen, value, &ht_offset);
        if (0 == rc) {
            ctx->ht[index] = ht_offset;
        } else {
            incr_store_sz(ctx, namelen, value);
        }
        if (!nolock) {
            rw_unlock(ctx);
        }
        return rc;
    }

    // handle collision
    shmoff_t entryoffset, prev_offset;

    entryoffset = prev_offset = ctx->ht[index];
    while (strcmp(HT_ENTRY_NAME(ctx,entryoffset), name_p) && HT_ENTRY_NEXT(ctx, entryoffset)) {
        entryoffset = HT_ENTRY_NEXT(ctx,entryoffset);
    }

    if (0 == strcmp(HT_ENTRY_NAME(ctx,entryoffset), name_p)) {
        /*
         * recursion, explicitly set to no-lock as write lock is already acquired
         */
        _syscfg_unset(ns, name, 1);
        rc = _syscfg_set(ns, name, value, 1);
    } else { 
        // new entry, attach it to end of linked list
        if (0 == check_decr_store_sz(ctx, namelen, value)) {
            if (!nolock) {
                rw_unlock(ctx);
            }
            return ERR_NO_SPACE;
        }
        shmoff_t ht_offset;
        rc = make_ht_entry(name_p, namelen, value, &ht_offset);
        if (0 == rc) {
            HT_ENTRY_NEXT(ctx,entryoffset) = ht_offset;
        } else {
            incr_store_sz(ctx, namelen, value);
        }
    }

    if (!nolock) {
        rw_unlock(ctx);
    }
    return rc;
}

/*
 * locked - indicates if write lock already acquired by caller, hence it need
 *          not be locked (actually shd not be, bcoz it will cause a deadlock)
 */
static int _syscfg_unset (const char *ns, const char *name, int nolock)
{
    int index;
    syscfg_shm_ctx *ctx = syscfg_ctx;
    size_t namelen;

    if (!nolock) {
        rw_lock(ctx);
    }

    if (ns) {
        namelen = snprintf(name_p, sizeof(name_p), "%s" NS_SEP "%s", ns, name);
    } else {
        namelen = snprintf(name_p, sizeof(name_p), "%s", name);
    }

    if (namelen >= sizeof(name_p))
        namelen = sizeof(name_p) - 1;

    index = hash(name_p);

    if (0 == ctx->ht[index]) {
        // doesn't exist, nothing to do
        if (!nolock) {
            rw_unlock(ctx);
        }
        return 0;
    }

    shmoff_t entryoffset = ctx->ht[index];
    shmoff_t prev = 0;

    while (entryoffset && strcmp(HT_ENTRY_NAME(ctx,entryoffset), name_p)) {
        prev = entryoffset;
        entryoffset = HT_ENTRY_NEXT(ctx,entryoffset);
    }
    if (0 == entryoffset) {
        // doesn't exist, nothing to do
        if (!nolock) {
            rw_unlock(ctx);
        }
        return 0;
    }
    incr_store_sz(ctx, namelen, HT_ENTRY_VALUE(ctx,entryoffset));
    if (prev) {
        HT_ENTRY_NEXT(ctx,prev) = HT_ENTRY_NEXT(ctx,entryoffset);
    } else {
        ctx->ht[index] = HT_ENTRY_NEXT(ctx,entryoffset);
    }
    shm_free(ctx, entryoffset);

    if (!nolock) {
        rw_unlock(ctx);
    }
    return 0;
}

static int _syscfg_getall (char *buf, int bufsz)
{
    int i;
    unsigned int len = bufsz;
    syscfg_shm_ctx *ctx = syscfg_ctx;
    shmoff_t entry;

    rw_lock(ctx);
    for (i = 0; i < SYSCFG_HASH_TABLE_SZ; i++) {
        entry = ctx->ht[i];
        // check if space left for 'name' '=' 'value' + null char
        while (entry && len >= (HT_ENTRY_NAMESZ(ctx,entry) + HT_ENTRY_VALUESZ(ctx,entry))) {
            len -= sprintf(buf + (bufsz - len),
                           "%s=%s", HT_ENTRY_NAME(ctx,entry), HT_ENTRY_VALUE(ctx,entry)) + 1;
            entry = HT_ENTRY_NEXT(ctx,entry);
        }
    }
    rw_unlock(ctx);

    return (bufsz - len);
}



/******************************************************************************
 *          shared-memory create, initialize and attach/detach APIs
 *****************************************************************************/


static void shm_mm_init (syscfg_shm_ctx *ctx);

/*
 * initialize control block
 */
/* CID 68025: Big parameter passed by value */
static int shm_cb_init (syscfg_shm_ctx *ctx, int shmid, store_info_t *store_info)
{
    shm_cb *cb = &(ctx->cb);

    memset(cb, 0, sizeof(shm_cb));

    cb->magic = SYSCFG_SHM_MAGIC;
    cb->version = (SYSCFG_SHM_VERSION_MAJOR << 4) | (SYSCFG_SHM_VERSION_MINOR);
    cb->size = SYSCFG_SHM_SIZE;
    cb->shmid = shmid;
    cb->store_type = store_info->type;
    /* CID 135644 : BUFFER_SIZE_WARNING */
    strncpy(cb->store_path, store_info->path, sizeof(cb->store_path)-1);
    cb->store_path[sizeof(cb->store_path)-1] = '\0';
    cb->max_store_size = store_info->max_size;
    cb->used_store_size = store_info->hdr_size;

    int rc = lock_init(ctx);
    if (rc) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Error creating system locks");
    }

    return rc;
}

static int syscfg_shm_init (syscfg_shm_ctx **out_ctx)
{
    int rc, shmid;
    struct stat sbuf;

    if (-1 == (rc = stat(SYSCFG_SHM_FILE, &sbuf))) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Shared memory file not found");
        return ERR_SHM_NO_FILE;
    }

    syscfg_shm_ctx *ctx = (syscfg_shm_ctx *) syscfg_shm_attach(&shmid);
    if (NULL == ctx) {
        return ERR_SHM_ATTACH;
    }

    if (ctx->cb.magic != SYSCFG_SHM_MAGIC) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Magic check failed. Invalid shared memory context");
        return ERR_SHM_ATTACH;
    }

    *out_ctx = ctx;
    return 0;
}

/* CID 63267:Big parameter passed by value */
static void *syscfg_shm_create (store_info_t *store_info, int *out_shmid)
{
    int shmid;
    key_t key;
    struct stat sbuf;

 
    /*CID: 135594 Time of check time of use*/
    // precreate file 
    FILE *fp = fopen(SYSCFG_SHM_FILE, "w");
    if (NULL == fp) {
        return NULL;
    }

    if (-1 != fstat(fileno(fp), &sbuf)) {
        void *p_shm = syscfg_shm_attach(&shmid);
        if (p_shm) {
            ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Old shm instance still present. destroy it before creating new one");
	    fclose(fp);
            return NULL;
        } else {
            ulog_error(ULOG_SYSTEM, UL_SYSCFG, "WARN: Unexpected shared memory file during creation"); 
        }
    }
    fputs("creating", fp);
    fclose(fp);


    // get unique key using file path and proj id
    if (-1 == (key= ftok(SYSCFG_SHM_FILE, SYSCFG_SHM_PROJID))) {
        return NULL;
    }

    if (-1 == (shmid = shmget(key, SYSCFG_SHM_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR))) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "shared memory creation failed"); 
        return NULL;
    }

    // attach to shm to initialize syscfg structures
    syscfg_shm_ctx *ctx = shmat(shmid, NULL, 0);
    if (NULL == ctx) {
        return NULL;
    }

    int rc = shm_cb_init(ctx, shmid, store_info);
    if (rc) {
        ulog_error(ULOG_SYSTEM, UL_SYSCFG, "shared memory control block init failed"); 
    }

    shm_mm_init(ctx);

    fp = fopen(SYSCFG_SHM_FILE, "w");
    if (fp) {
        fprintf(fp, "%d", shmid);
        fclose(fp);
    }

    *out_shmid = shmid;

    syscfg_ctx = ctx;
    syscfg_initialized = 1;

    return ctx;
}

static int syscfg_shm_getid (void)
{
    key_t key;

    // get unique key using file path and proj id
    key = ftok(SYSCFG_SHM_FILE, SYSCFG_SHM_PROJID);
    if (-1 == key) {
        printf("Error: couldn't get shm key from file name %s and proj id %d\n",
               SYSCFG_SHM_FILE, SYSCFG_SHM_PROJID);
        return -1;
    }
    
    int shmid = shmget(key, 0, 0);
    if (-1 == shmid) {
        return -1;
    }

    return shmid;
}

static void *syscfg_shm_attach (int *shmid)
{
    void *shm_p;

    if (-1 == (*shmid = syscfg_shm_getid())) {
        return NULL;
    }
    if ((void *) -1 == (shm_p = shmat(*shmid, NULL, 0))) {
        return NULL;
    }
    return shm_p;
}

// mark shared memory for deletion
// CHECK: shd we wait on all read, write & commit to open?
// most probably not as those procs would've attached to
// shm (and hence will be still valid). Also if we wait, it might take
// a long time as those other operations might take a while
// UI should warn the user (html, cli, etc) that this is a dangerous 
// operation to do and might take some time (please wait blah blah ...)
// CHECK: when shd we call sem_destroy for all our semaphores?
//
static void syscfg_shm_destroy (syscfg_shm_ctx *ctx)
{
    if (NULL == ctx) {
        return;
    }

    lock_destroy(ctx);

    struct shmid_ds buf;

    int rc = shmctl(ctx->cb.shmid, IPC_RMID, &buf);
    if (-1 == rc) {
    }
    shmdt(ctx);

    unlink(SYSCFG_SHM_FILE);
}

/******************************************************************************
 *                shared-memory memory management APIs
 *****************************************************************************/

    /*
     * data block has 3 offsets of interest
     *   db_start - data block start points to beginning of shm context's data block
     *              it never changes
     *   db_end   - data block end points to end of shm context's data block.
     *              it rarely changes, only when shm is resized or remapped 
     *              for need of more space
     *   db_cur   - data block current points to an offset between start and end
     *              that is current used. this actively changes as new elements are
     *              allocated from datablock. it always monetonically increased and never
     *              decrease. Initially db_cur points to beginning of data block (db_start)
     *
     *   db_start  -->  |------------|
     *                  |xxxxxxxxxxxx| 
     *                  |xxxxxxxxxxxx| 
     *                  |xxxxxxxxxxxx| 
     *                  |xxxxxxxxxxxx| 
     *                  |xxxxxxxxxxxx| 
     *   db_cur    -->  |------------|
     *                  |000000000000| 
     *                  |000000000000| 
     *                  |000000000000| 
     *                  |000000000000| 
     *   db_end    -->  |------------|
     */

static void shm_mm_init (syscfg_shm_ctx *ctx)
{
    int shm_metadata_sz = sizeof(shm_cb) +
                          sizeof(shm_mm) +
                          (sizeof(shmoff_t) * SYSCFG_HASH_TABLE_SZ);
    shm_mm *mm = &(ctx->mm);

    mm->db_size = ctx->cb.size - shm_metadata_sz;
    mm->db_start = shm_metadata_sz;
    mm->db_end = mm->db_start + mm->db_size;
    mm->db_cur = mm->db_start;


    shm_free_table *ft = mm->ft;

#define MIN_ITEM_SZ 32
#define MAX_ITEM_SZ 2048

    // free list for 32 byte entries
    ft[0].size = MIN_ITEM_SZ; 
    ft[0].mf = 8; 
    ft[0].head = 0; 

    // free list for 64 byte entries
    ft[1].size = 64; 
    ft[1].mf = 4; 
    ft[1].head = 0; 

    // free list for 128 byte entries
    ft[2].size = 128; 
    ft[2].mf = 2; 
    ft[2].head = 0; 

    // free list for 256 byte entries
    ft[3].size = 256; 
    ft[3].mf = 2; 
    ft[3].head = 0; 

    // free list for 512 byte entries
    ft[4].size = 512; 
    ft[4].mf = 1; 
    ft[4].head = 0; 

    // free list for 1024 byte entries
    ft[5].size = 1024; 
    ft[5].mf = 1; 
    ft[5].head = 0; 

    // free list for 2048 byte entries
    ft[6].size = MAX_ITEM_SZ; 
    ft[6].mf = 1; 
    ft[6].head = 0; 
}

int shm_malloc (syscfg_shm_ctx *ctx, int size, shmoff_t *out_offset)
{
    int i;
    shmoff_t item;

    // shm_mm *mm = &(ctx->mm);
    // shm_free_table *ft = mm->ft;
    shm_free_table *ft = ctx->mm.ft;

    for(i=0; i < NUM_BUCKETS; i++) {
        if (ft[i].size > (size + MM_OVERHEAD)) {
            if (ft[i].head) {
                item = ft[i].head;
                ft[i].head = MM_ITEM_NEXT(ctx, item);
                MM_ITEM_NEXT(ctx, item) = 0;
                *out_offset = (item + MM_OVERHEAD);
                return 0;
            } else {
                int ct = make_mm_items(ctx, &ft[i]);
                if (0 == ct) {
                    ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Error: shm_malloc failed, insufficient space?");
                    return ERR_MEM_ALLOC;
                } else {
                    // now that we've more items, call us again
                    return shm_malloc(ctx, size, out_offset);
                }
            }
        }
    }
    
    ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Error: shm_malloc failed, size too big?");
    return ERR_ENTRY_TOO_BIG;
}

/*
 * offset is past mm_overhead
 * clear mm_item's data portion before putting back to free list
 */
void shm_free (syscfg_shm_ctx *ctx, shmoff_t offset)
{
    shmoff_t old_head, mmoffset;
    mm_item *item;

    shm_free_table *ft = ctx->mm.ft;

    mmoffset = (offset - MM_OVERHEAD);
    
    item = MM_ITEM(ctx,mmoffset);

    memset(((char *) item + MM_OVERHEAD), 0, item->size - MM_OVERHEAD);

    int i;
    for(i=0; i < NUM_BUCKETS; i++) {
        if (ft[i].size == MM_ITEM_SIZE(ctx, mmoffset)) {
            old_head = ft[i].head;
            ft[i].head = mmoffset;
            MM_ITEM_NEXT(ctx,mmoffset) = old_head;
        }
    }
}

/*
 * returns number of mm_items made
 * 0 on error, > 0 on success
 * Note, argument "ft" should be treated as one shm_free_table entry, NOT as ft array
 */
static int make_mm_items (syscfg_shm_ctx *ctx, shm_free_table *ft)
{
    int ct;
    shmoff_t item;

    if (ft->size < MIN_ITEM_SZ || ft->size > MAX_ITEM_SZ) { 
        // WARNING: bad shm mm bucket 
        return 0;
    }
    if (ft->head) {
        // item already available
        // WARNING: why do you want more items?
        return 0;
    }

    for(ct=0; (ct < ft->mf) && ((ctx->mm.db_cur + ft->size) < ctx->mm.db_end); ct++) {
        item = ctx->mm.db_cur;
        MM_ITEM_SIZE(ctx, item) = ft->size;
        MM_ITEM_NEXT(ctx, item) = ft->head;
        ctx->mm.db_cur += ft->size;
        ft->head = item;
    }

    return ct;
}

/*
 * Advisory read and write lock
 * Note: fd should be opened with RDWR mode
 */
static void _syscfg_file_lock (int fd)
{
    struct flock fl;

    memset(&fl, 0, sizeof(fl));

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; // whole file, even if it grows
    /* CID 70977: Unchecked return value from library */
    if (fcntl(fd, F_SETLKW, &fl) == -1)
    {
	close(fd);
	return;
    }
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; // whole file, even if it grows
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        close(fd);
    }
}

/*
 * Unlock read and write 
 * Note: fd should be opened with RDWR mode
 */
static void _syscfg_file_unlock (int fd)
{
    struct flock fl;

    memset(&fl, 0, sizeof(fl));

    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; // whole file, even if it grows
    /* CID 63229: Unchecked return value from library */
     if (fcntl(fd, F_SETLKW, &fl) == -1)
     {
         close(fd);
     }
}

static int load_from_file (const char *fname)
{
    int fd;
    ssize_t count;
    char *inbuf = NULL, *buf = NULL;
    char *name = NULL, *value = NULL;

    fd = open(fname, O_RDONLY);
    if (-1 == fd) {
        return ERR_IO_FILE_OPEN;
    }
    inbuf = malloc(SYSCFG_SZ);
    if (NULL == inbuf) {
        close(fd); /*RDKB-7135, CID-33110, free unused resources before exit*/
        return ERR_MEM_ALLOC;
    }

    count = read(fd, inbuf, SYSCFG_SZ);
    close(fd);

    if (count <= 0) {
        free(inbuf);
        return 1;
    }

    buf = inbuf;
    /*CID 135472 String not null terminated */
    buf[count] = '\0';
    do {
        buf = syscfg_parse(buf, &name, &value);
        if (name && value) {
            syscfg_set(NULL, name, value);
            free(name);
            name = NULL; /*RDKB-7135, CID-33405, set null after free*/
            free(value);
            value = NULL; /*RDKB-7135, CID-33137, set null after free*/
        }

        // skip any special chars leftover
        if (buf && *buf == '\n') {
            buf++;
        }
    } while (buf);

    free(inbuf);

    return 0;
}

/* Taking backup of file */
static int backup_file (const char *bkupFile, const char *localFile)
{
   int fd_from = open(localFile, O_RDONLY);
   int rc=0;
  if(fd_from < 0)
  {
    ulog_error(ULOG_SYSTEM, UL_SYSCFG,"opening localfile failed during db backup");
    return -1;
  }
  struct stat Stat;
  if(fstat(fd_from, &Stat)<0)
  {
    ulog_error(ULOG_SYSTEM, UL_SYSCFG, "fstat call failed during db backup");

    close(fd_from);
    return -1;
  }
  void *mem = mmap(NULL, Stat.st_size, PROT_READ, MAP_SHARED, fd_from, 0);
  if(mem == MAP_FAILED)
  {
    	ulog_error(ULOG_SYSTEM, UL_SYSCFG, "mmap failed during db backup");
        close(fd_from);
        return -1;
  }

  int fd_to = open(bkupFile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if(fd_to < 0)
  {
    	ulog_error(ULOG_SYSTEM, UL_SYSCFG, "creat sys call failed during db backup");
	rc = munmap(mem,Stat.st_size);
	if ( rc != 0 ){
		
    		ulog_error(ULOG_SYSTEM, UL_SYSCFG, "munmap failed");
	}
        close(fd_from);
        return -1;
  }
  ssize_t nwritten = write(fd_to, mem, Stat.st_size);
  if (msync(mem, Stat.st_size, MS_SYNC)) {
      ulog_error(ULOG_SYSTEM, UL_SYSCFG, "msync call failed during db backup");
      fprintf(stderr, "%s msync FAILED, errno:%d\n", __func__, errno);
  }

  if(nwritten < Stat.st_size)
  {
    	ulog_error(ULOG_SYSTEM, UL_SYSCFG, "write system call failed during db backup");

	rc = munmap(mem,Stat.st_size);
        if ( rc != 0 ){

                ulog_error(ULOG_SYSTEM, UL_SYSCFG, "munmap failed");
        }

	close(fd_from);
        close(fd_to);
        return -1;
  }

  rc = munmap(mem,Stat.st_size);
  if ( rc != 0 ){
       ulog_error(ULOG_SYSTEM, UL_SYSCFG, "munmap failed");
  }

  if(close(fd_to) < 0) {
        fd_to = -1;
    	ulog_error(ULOG_SYSTEM, UL_SYSCFG, "closing file descriptor failed during db backup");

  	close(fd_from);
        return -1;
  }
  close(fd_from);

  /* Success! */
  return 0;
}

/*
 * Notes
 *    syscfg space is locked by the caller (for write & commit)
 */
static int commit_to_file (const char *fname)
{
    int fd;
    int i, ct;
    char buf[2*MAX_ITEM_SZ];
    int ret=0;
    syscfg_shm_ctx *ctx = syscfg_ctx;

    fd = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        return ERR_IO_FILE_OPEN;
    }
    _syscfg_file_lock(fd);

    shmoff_t entry;

    for (i = 0; i < SYSCFG_HASH_TABLE_SZ; i++) {
        entry = ctx->ht[i];
        while (entry) {
            ct = snprintf(buf, sizeof(buf), "%s=%s\n",
                          HT_ENTRY_NAME(ctx,entry), HT_ENTRY_VALUE(ctx,entry));
            write(fd, buf, ct);
            entry = HT_ENTRY_NEXT(ctx,entry);
        }
    }
    _syscfg_file_unlock(fd);

    close(fd);

   ret = access(SYSCFG_BKUP_FILE, F_OK);
   if ( ret == 0 ) { 
   	ret=backup_file(SYSCFG_BKUP_FILE,fname);
   	if (ret == -1)
   	{
    		ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Backing up of syscfg failed");
		// retrying again to take db back up
		ret=0;
        	ret=backup_file(SYSCFG_BKUP_FILE,fname);
		if ( ret == -1){
			ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Retry of backing up syscfg also failed");
	        	return ret;
		}
   	}
   }

   ret = access(SYSCFG_NEW_FILE, F_OK);
   if ( ret == 0 ) {
 	  ret=backup_file(SYSCFG_NEW_FILE,fname);
	  if (ret == -1)
   	  {
    		ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Backing up of syscfg failed");
		// retrying again to take db back up
		ret=0;
        	ret=backup_file(SYSCFG_NEW_FILE,fname);
		if ( ret == -1){
			ulog_error(ULOG_SYSTEM, UL_SYSCFG, "Retry of backing up syscfg also failed");
	        	return ret;
	        }
   	  }
   }
   return 0;
}
