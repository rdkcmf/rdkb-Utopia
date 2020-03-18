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

#ifndef _SYSCFG_H_
#define _SYSCFG_H_

// Changing sysconfig size to 100kb for non XB3 devices

#ifdef _COSA_INTEL_XB3_ARM_
	#define SYSCFG_SZ (50 * (1024))       /* max file size - 50kb */
#else
	#define SYSCFG_SZ (100 * (1024))       /* max file size - 100kb */
#endif
#define NS_SEP "::"
#define NS_SEP_SZ 2

#define ERR_INVALID_PARAM     -1
#define ERR_NOT_INITIALIZED   -2
#define ERR_MEM_ALLOC         -3
#define ERR_INVALID_STATE     -4
#define ERR_SEMAPHORE_INIT    -5
#define ERR_NO_SPACE          -6
#define ERR_ENTRY_TOO_BIG     -7
#define ERR_SHM_CREATE        -10
#define ERR_SHM_INIT          -11
#define ERR_SHM_NO_FILE       -12
#define ERR_SHM_ATTACH        -13
#define ERR_SHM_FAILURE       -14
#define ERR_IO_FAILURE        -20
#define ERR_IO_FILE_OPEN      -21
#define ERR_IO_FILE_STAT      -22
#define ERR_IO_FILE_TOO_BIG   -23
#define ERR_IO_FILE_WRITE     -24
#define ERR_IO_MTD_OPEN       -30
#define ERR_IO_MTD_GETINFO    -31
#define ERR_IO_MTD_INVALID    -32
#define ERR_IO_MTD_BAD_MAGIC  -33
#define ERR_IO_MTD_BAD_SZ     -34
#define ERR_IO_MTD_ERASE      -35
#define ERR_IO_MTD_READ       -36
#define ERR_IO_MTD_WRITE      -37

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Procedure     : syscfg_create
 * Purpose       : Create syscfg shared memory and load entries from persistent storage
 * Parameters    :   
 *   file - filesystem 'file' where syscfg is stored
 *   mtd_device - raw /dev/mtdX device where syscfg is stored
 * Return Values :
 *    0              - success
 *    ERR_INVALID_PARAM - invalid arguments
 *    ERR_IO_FAILURE - syscfg file unavailable
 * Notes         :
 *    When both file and mtd_device specified, file based storage takes
 *    precedence. This should be called early in system bootup
 */
int syscfg_create(const char *file, long int max_file_sz, const char *mtd_device);

int syscfg_reload(const char *file);
int syscfg_commit_lock();
int syscfg_commit_unlock();

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
 *    When both file and mtd_device specified, file based storage takes
 *    precedence 
 */
int syscfg_init();

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
void syscfg_destroy();

/*
 * Procedure     : syscfg_get
 * Purpose       : Retrieve an entry from syscfg
 * Parameters    :   
 *   ns  -  namespace string (optional)
 *   name  - name string, entry to add
 *   out_val  - buffer to store output value string
 *   outbufsz  - output buffer size
 * Return Values :
 *    0 on success, -1 on error
 */
int syscfg_get(const char *ns, const char *name, char *out_value, int outbufsz);

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
int syscfg_getall(char *buf, int count, int *outsz);

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
int syscfg_set(const char *ns, const char *name, const char *value);

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
int syscfg_unset(const char *ns, const char *name);

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
int syscfg_commit();

/*
 * Procedure     : syscfg_is_match
 * Purpose       : Compare the value of an entry from syscfg 
 * Parameters    :   
 *   ns  -  namespace string (optional)
 *   name  - name string, entry to add
 *   out_val  - buffer to store output value string
 *   outbufsz  - output buffer size
 * Return Values :
 *    0 on success, -1 on error
 */
int syscfg_is_match (const char *ns, const char *name, char *value, unsigned int *out_match);

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
int syscfg_getsz (long int *used_sz, long int *max_sz);

/*
 * Procedure     : syscfg_format
 * Purpose       : SYSCFG persistent storage format
 * Parameters    :
 *   mtd_device - raw /dev/mtdX device where syscfg is stored
 *   file - filesystem 'file' to seed syscfg values (optional)
 * Return Values :
 *    0              - success
 *    ERR_INVALID_PARAM - invalid arguments
 *    ERR_IO_FAILURE - syscfg file unavailable
 * Notes :
 *    WARNING: will overwrite persistent store
 *    seed file is optional, without it only the mtd header is placed
 *    at the beginning of mtd device
 */
int syscfg_format (const char *mtd_device, const char *seed_file);

/*
 * Procedure     : syscfg_check
 * Purpose       : Checks if given flash partition is valid syscfg partition
 * Parameters    :   
 *   mtd_device  - flash mtd partition (like /dev/mtd3)
 * Return Values :
 *    0 - valid syscfg partition
 *    ERR... - error/invalid syscfg partition
 * Notes         :
 */
int syscfg_check (const char *mtd_device);

#ifdef __cplusplus
}
#endif

#endif /* _SYSCFG_H_ */
