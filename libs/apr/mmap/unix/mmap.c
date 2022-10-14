/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fspr.h"
#include "fspr_private.h"
#include "fspr_general.h"
#include "fspr_strings.h"
#include "fspr_mmap.h"
#include "fspr_errno.h"
#include "fspr_arch_file_io.h"
#include "fspr_portable.h"

/* System headers required for the mmap library */
#ifdef BEOS
#include <kernel/OS.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if APR_HAS_MMAP || defined(BEOS)

static fspr_status_t mmap_cleanup(void *themmap)
{
    fspr_mmap_t *mm = themmap;
    fspr_mmap_t *next = APR_RING_NEXT(mm,link);
    int rv = 0;

    /* we no longer refer to the mmaped region */
    APR_RING_REMOVE(mm,link);
    APR_RING_NEXT(mm,link) = NULL;
    APR_RING_PREV(mm,link) = NULL;

    if (next != mm) {
        /* more references exist, so we're done */
        return APR_SUCCESS;
    }

#ifdef BEOS
    rv = delete_area(mm->area);
#else
    rv = munmap(mm->mm, mm->size);
#endif
    mm->mm = (void *)-1;

    if (rv == 0) {
        return APR_SUCCESS;
    }
    return errno;
}

APR_DECLARE(fspr_status_t) fspr_mmap_create(fspr_mmap_t **new, 
                                          fspr_file_t *file, fspr_off_t offset, 
                                          fspr_size_t size, fspr_int32_t flag, 
                                          fspr_pool_t *cont)
{
    void *mm;
#ifdef BEOS
    area_id aid = -1;
    uint32 pages = 0;
#else
    fspr_int32_t native_flags = 0;
#endif

#if APR_HAS_LARGE_FILES && defined(HAVE_MMAP64)
#define mmap mmap64
#elif APR_HAS_LARGE_FILES && SIZEOF_OFF_T == 4
    /* LFS but no mmap64: check for overflow */
    if ((fspr_int64_t)offset + size > INT_MAX)
        return APR_EINVAL;
#endif

    if (size == 0)
        return APR_EINVAL;
    
    if (file == NULL || file->filedes == -1 || file->buffered)
        return APR_EBADF;
    (*new) = (fspr_mmap_t *)fspr_pcalloc(cont, sizeof(fspr_mmap_t));
    
#ifdef BEOS
    /* XXX: mmap shouldn't really change the seek offset */
    fspr_file_seek(file, APR_SET, &offset);

    /* There seems to be some strange interactions that mean our area must
     * be set as READ & WRITE or writev will fail!  Go figure...
     * So we ignore the value in flags and always ask for both READ and WRITE
     */
    pages = (size + B_PAGE_SIZE -1) / B_PAGE_SIZE;
    aid = create_area("fspr_mmap", &mm , B_ANY_ADDRESS, pages * B_PAGE_SIZE,
        B_NO_LOCK, B_WRITE_AREA|B_READ_AREA);

    if (aid < B_NO_ERROR) {
        /* we failed to get an area we can use... */
        *new = NULL;
        return APR_ENOMEM;
    }

    if (aid >= B_NO_ERROR)
        read(file->filedes, mm, size);
    
    (*new)->area = aid;
#else

    if (flag & APR_MMAP_WRITE) {
        native_flags |= PROT_WRITE;
    }
    if (flag & APR_MMAP_READ) {
        native_flags |= PROT_READ;
    }

    mm = mmap(NULL, size, native_flags, MAP_SHARED, file->filedes, offset);

    if (mm == (void *)-1) {
        /* we failed to get an mmap'd file... */
        *new = NULL;
        return errno;
    }
#endif

    (*new)->mm = mm;
    (*new)->size = size;
    (*new)->cntxt = cont;
    APR_RING_ELEM_INIT(*new, link);

    /* register the cleanup... */
    fspr_pool_cleanup_register((*new)->cntxt, (void*)(*new), mmap_cleanup,
             fspr_pool_cleanup_null);
    return APR_SUCCESS;
}

APR_DECLARE(fspr_status_t) fspr_mmap_dup(fspr_mmap_t **new_mmap,
                                       fspr_mmap_t *old_mmap,
                                       fspr_pool_t *p)
{
    *new_mmap = (fspr_mmap_t *)fspr_pmemdup(p, old_mmap, sizeof(fspr_mmap_t));
    (*new_mmap)->cntxt = p;

    APR_RING_INSERT_AFTER(old_mmap, *new_mmap, link);

    fspr_pool_cleanup_register(p, *new_mmap, mmap_cleanup,
                              fspr_pool_cleanup_null);
    return APR_SUCCESS;
}

APR_DECLARE(fspr_status_t) fspr_mmap_delete(fspr_mmap_t *mm)
{
    return fspr_pool_cleanup_run(mm->cntxt, mm, mmap_cleanup);
}

#endif
