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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <mtd/mtd-user.h>
#include <ulog/ulog.h>
#include "syscfg.h"

#define SYSCFG_MTD_MAGIC     0xDADEBEEF
/*
 * WARNING: should be same as SYSCFG flash partition size defined in the kernel
 * For broadcom board, this is defined in arch/mips/brcm-boards/bcm947xx/setup.c
 */
#define SYSCFG_MTD_SIZE      (32*1024)

#define SYSCFG_MTD_CONTENT_SIZE (SYSCFG_MTD_SIZE - sizeof(syscfg_mtd_hdr_t))

/*
 * Current version 0.1
 */
#define SYSCFG_MTD_VERSION_MAJOR 0
#define SYSCFG_MTD_VERSION_MINOR 1

/*
 * syscfg mtd header placed at the beginning of mtd region
 */
typedef struct {
    int  magic;
    int  size;         // size of valid contents within mtd-device excluding this header
    uint version;
} syscfg_mtd_hdr_t;

#define SYSCFG_TMP_FILE_PREFIX "/tmp/.syscfg.tmp"

extern int commit_to_file (const char *fname);
extern int load_from_file (const char *fname);


/*
 * Write file into mtd-device after a syscfg mtd header
 * If file is NULL, just place the syscfg mtd header
 *
 * As of now this routine handles NAND and NOR flash types 
 * (and MTD_RAM for testing)
 * The whole flash region is erased before writing
 * A small syscfg_mtd header is placed at the beginning 
 * of the mtd region. It contains metadata like syscfg 
 * mtd size, this is used while reading off this space
 */
int mtd_write_from_file (const char *mtd_device, const char *file)
{
    struct stat file_info;
    syscfg_mtd_hdr_t mtd_hdr;
    int in_fd = 0, mtd_fd = 0, rc, content_size = 0;

    if (file) {
        // Input file 
        if (-1 == (in_fd = open(file, O_RDONLY))) {
            // perror("mtd_write: input file open()");
            rc = ERR_IO_FILE_OPEN;
            goto err_mtd_write;
        }
        if (-1 == fstat(in_fd, &file_info)) {
            // perror("mtd_write: input file stat()");
            rc = ERR_IO_FILE_STAT;
            goto err_mtd_write;
        }
        content_size = file_info.st_size;
        if (content_size < 0 || content_size > SYSCFG_MTD_CONTENT_SIZE) {
            rc = ERR_IO_FILE_TOO_BIG;
            // fprintf(stderr, "%s: Error invalid syscfg file size %d\n", __FUNCTION__, content_size);
            goto err_mtd_write;
        }
    }

    // Output mtd device
    if (-1 == (mtd_fd = open(mtd_device, O_WRONLY | O_SYNC))) {
        // perror("mtd_write: mtd open()");
        rc = ERR_IO_MTD_OPEN;
        goto err_mtd_write;
    }

    struct mtd_info_user mtd_info;
    memset(&mtd_info, 0, sizeof(mtd_info));

    if (-1 == ioctl(mtd_fd, MEMGETINFO, &mtd_info)) {
        // perror("mtd_write: mtd getinfo ioctl()");
        rc = ERR_IO_MTD_GETINFO;
        goto err_mtd_write;
    }
    if (MTD_NORFLASH != mtd_info.type && MTD_NANDFLASH != mtd_info.type && MTD_RAM != mtd_info.type) {
        // fprintf(stderr, "%s: Error not NOR/NAND/RAM flash. type=%d\n", __FUNCTION__, mtd_info.type);
        rc = ERR_IO_MTD_INVALID;
        goto err_mtd_write;
    }

    struct erase_info_user erase_info;
    erase_info.start = 0;
    erase_info.length = mtd_info.size;
    if (-1 == ioctl(mtd_fd, MEMERASE, &erase_info)) {
        // perror("mtd_write: mtd erase ioctl()");
        rc = ERR_IO_MTD_ERASE;
        goto err_mtd_write;
    }

    memset(&mtd_hdr, 0, sizeof(mtd_hdr));
    mtd_hdr.size = content_size;
    mtd_hdr.magic = SYSCFG_MTD_MAGIC;
    mtd_hdr.version = (SYSCFG_MTD_VERSION_MAJOR << 4) | (SYSCFG_MTD_VERSION_MINOR);

    if (-1 == write(mtd_fd, &mtd_hdr, sizeof(mtd_hdr))) {
        // perror("mtd_write: mtd header write()");
        rc = ERR_IO_MTD_WRITE;
        goto err_mtd_write;
    }

    if (file) {
        char buf[8*1024];
        int n, ct;
    
        while ((n = read(in_fd, buf, sizeof(buf))) > 0) {
            ct = write(mtd_fd, buf, n);
            if (-1 == ct) {
                // perror("mtd_write: mtd buffer write()");
                rc = ERR_IO_MTD_WRITE;
                goto err_mtd_write;
            }
            if (ct != n) {
                fprintf(stderr, "%s: Error writing to mtd %s, only %d off %d written\n",
                        __FUNCTION__, mtd_device, ct, n);
            }
        }
    }

    close(in_fd);
    close(mtd_fd);
    return 0;

err_mtd_write:
    /*RDKB-7134, CID-33439, CID-33312; free resources before exit*/
    if (in_fd != -1) close(in_fd);
    if (mtd_fd != -1) close(mtd_fd);
    return rc;
}

int mtd_read_to_file (const char *mtd_device, const char *file)
{
    int mtd_fd = 0, out_fd = 0, rc = 0;
    syscfg_mtd_hdr_t mtd_hdr;
    struct mtd_info_user mtd_info;

    if (-1 == (mtd_fd = open(mtd_device, O_RDONLY | O_SYNC))) {
        // perror("mtd_read: mtd open()");
        rc = ERR_IO_MTD_OPEN;
        goto err_mtd_read;
    }

    if (-1 == ioctl(mtd_fd, MEMGETINFO, &mtd_info)) {
        // perror("mtd_read: mtd getinfo ioctl()");
        rc = ERR_IO_MTD_GETINFO;
        goto err_mtd_read;
    }
    if (MTD_NORFLASH != mtd_info.type && MTD_NANDFLASH != mtd_info.type && MTD_RAM != mtd_info.type) {
        // fprintf(stderr, "%s: Error not NOR/NAND/RAM flash. type=%d\n", __FUNCTION__, mtd_info.type);
        rc = ERR_IO_MTD_INVALID;
        goto err_mtd_read;
    }

    if (-1 == read(mtd_fd, &mtd_hdr, sizeof(mtd_hdr))) {
        // perror("mtd_read: mtd read()");
        rc = ERR_IO_MTD_READ;
        goto err_mtd_read;
    }


    if (SYSCFG_MTD_MAGIC != mtd_hdr.magic) {
        rc = ERR_IO_MTD_BAD_MAGIC;
        goto err_mtd_read;
    }
    if (mtd_hdr.size < 0 || mtd_hdr.size > SYSCFG_MTD_CONTENT_SIZE) {
        rc = ERR_IO_MTD_BAD_SZ;
        // fprintf(stderr, "%s: Error invalid mtd syscfg size %d\n", __FUNCTION__, mtd_hdr.size);
        goto err_mtd_read;
    }
    if (-1 == (out_fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR))) {
        // perror("mtd_read: out file open()");
        rc = ERR_IO_FILE_OPEN;
        goto err_mtd_read;
    }

    char buf[8*1024];
    int ct, n, size = mtd_hdr.size;

    do {
        n = read(mtd_fd, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        ct = write(out_fd, buf, n);
        if (-1 == ct) {
            fprintf(stderr, "%s: Error writing to file %s\n", __FUNCTION__, file);
            rc = ERR_IO_FILE_WRITE;
            goto err_mtd_read;
        }
        if (ct != n) {
            fprintf(stderr, "%s: Error writing to file %s, only %d off %d written\n", __FUNCTION__, file, ct, n);
        }
        size -= n;
    } while (size > 0);

    close(out_fd);
    close(mtd_fd); 
    return 0;

err_mtd_read:
    /*RDKB-7134, CID-33466, CID-33299; free resources before exit*/
    if (mtd_fd != -1) close(mtd_fd);
    if (out_fd != -1) close(out_fd);
    return rc;
}

/*
 *  Returns
 *   0 - valid syscfg mtd header
 *   rc - failure code, invalid syscfg mdr header
 */
int mtd_hdr_check (const char *mtd_device)
{
    int mtd_fd = 0, rc = 0;
    syscfg_mtd_hdr_t mtd_hdr;
    struct mtd_info_user mtd_info;

    if (-1 == (mtd_fd = open(mtd_device, O_RDONLY | O_SYNC))) {
        // perror("mtd_read: mtd open()");
        rc = ERR_IO_MTD_OPEN;
        goto err_mtd_check;
    }
    if (-1 == ioctl(mtd_fd, MEMGETINFO, &mtd_info)) {
        // perror("mtd_read: mtd getinfo ioctl()");
        rc = ERR_IO_MTD_GETINFO;
        goto err_mtd_check;
    }

    if (MTD_NORFLASH != mtd_info.type && MTD_NANDFLASH != mtd_info.type && MTD_RAM != mtd_info.type) {
        // fprintf(stderr, "%s: Error not NOR/NAND/RAM flash. type=%d\n", __FUNCTION__, mtd_info.type);
        rc = ERR_IO_MTD_INVALID;
        goto err_mtd_check;
    }

    if (-1 == read(mtd_fd, &mtd_hdr, sizeof(mtd_hdr))) {
        // perror("mtd_read: mtd read()");
        rc = ERR_IO_MTD_READ;
        goto err_mtd_check;
    }

    if (SYSCFG_MTD_MAGIC != mtd_hdr.magic) {
        rc = ERR_IO_MTD_BAD_MAGIC;
        // fprintf(stderr, "%s: Error mtd device %s not a valid syscfg store\n", __FUNCTION__, mtd_device);
        goto err_mtd_check;
    }
    if (mtd_hdr.size < 0 || mtd_hdr.size > SYSCFG_MTD_CONTENT_SIZE) {
        rc = ERR_IO_MTD_BAD_SZ;
        // fprintf(stderr, "%s: Error invalid mtd syscfg size %d\n", __FUNCTION__, mtd_hdr.size);
        goto err_mtd_check;
    }
    close(mtd_fd); 
    return 0;

err_mtd_check:
    /*RDKB-7134, CID-33089; free resources before exit*/
    if (mtd_fd != -1 ) close(mtd_fd);
    return rc;
}

int mtd_get_hdrsize ()
{
    return sizeof(syscfg_mtd_hdr_t);
}

long int mtd_get_devicesize ()
{
    return (SYSCFG_MTD_CONTENT_SIZE);
}

/*
 * Loading syscfg data from flash device
 *
 * This is a two step process. First syscfg data is
 * read from flash and written to temporary filesystem 
 * based file (excluding any sycfg mtd metadata).
 * Then regular file based syscfg parsing is done
 * on the temp file to initialize/populate the hash table
 */
int load_from_mtd (const char *mtd_device)
{
    int rc = 0;
    char tmpfile[32];

    srand((unsigned int)time(NULL));
    snprintf(tmpfile, sizeof(tmpfile), "%s%d", SYSCFG_TMP_FILE_PREFIX, rand());

    unlink(tmpfile);

    rc = mtd_read_to_file(mtd_device, tmpfile);
    if (0 == rc) {
        rc = load_from_file(tmpfile);
    }

    unlink(tmpfile);
    return rc;
}


/*
 * Commit syscfg data to flash device
 *
 * This is a two step process. First syscfg data is
 * written to filesystem file (using tmp file)
 * and this file is written to flash device along with
 * some metadata (syscfg mtd header)
 */
int commit_to_mtd (const char *mtd_device)
{
    char tmpfile[32];
    int rc = 0;

    srand((unsigned int)time(NULL));
    snprintf(tmpfile, sizeof(tmpfile), "%s%d", SYSCFG_TMP_FILE_PREFIX, rand());

    unlink(tmpfile);

    rc = commit_to_file(tmpfile);
    if (0 == rc) {
        rc = mtd_write_from_file(mtd_device, tmpfile);
    }
    
    unlink(tmpfile);
    return rc;
}

