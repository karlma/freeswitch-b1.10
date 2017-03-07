/*
 * Copyright (c) 2017, Shane Bryldt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "blade.h"

struct blade_space_s {
	blade_handle_t *handle;
	ks_pool_t *pool;

	const char *path;
	ks_hash_t *methods;
};


KS_DECLARE(ks_status_t) blade_space_create(blade_space_t **bsP, blade_handle_t *bh, const char *path)
{
	blade_space_t *bs = NULL;
	ks_pool_t *pool = NULL;

	ks_assert(bsP);
	ks_assert(bh);
	ks_assert(path);

	pool = blade_handle_pool_get(bh);

	bs = ks_pool_alloc(pool, sizeof(blade_space_t));
	bs->handle = bh;
	bs->pool = pool;
	bs->path = path; // @todo dup and keep copy? should mostly be literals
	ks_hash_create(&bs->methods, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_NOLOCK | KS_HASH_FLAG_DUP_CHECK, bs->pool);
	ks_assert(bs);

	*bsP = bs;

	ks_log(KS_LOG_DEBUG, "Space Created: %s\n", path);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) blade_space_destroy(blade_space_t **bsP)
{
	blade_space_t *bs = NULL;
	ks_hash_iterator_t *it = NULL;

	ks_assert(bsP);
	ks_assert(*bsP);

	bs = *bsP;

	for (it = ks_hash_first(bs->methods, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
        void *key = NULL;
		blade_method_t *value = NULL;

		ks_hash_this(it, (const void **)&key, NULL, (void **)&value);
		blade_method_destroy(&value);
	}

	ks_hash_destroy(&bs->methods);

	ks_log(KS_LOG_DEBUG, "Space Destroyed: %s\n", bs->path);

	ks_pool_free(bs->pool, bsP);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(blade_handle_t *) blade_space_handle_get(blade_space_t *bs)
{
	ks_assert(bs);

	return bs->handle;
}

KS_DECLARE(const char *) blade_space_path_get(blade_space_t *bs)
{
	ks_assert(bs);

	return bs->path;
}

KS_DECLARE(ks_status_t) blade_space_methods_add(blade_space_t *bs, blade_method_t *bm)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	const char *name = NULL;
	blade_method_t *bm_old = NULL;

	ks_assert(bs);
	ks_assert(bm);

	name = blade_method_name_get(bm);
	ks_assert(name);

	ks_hash_write_lock(bs->methods);
	bm_old = ks_hash_search(bs->methods, (void *)name, KS_UNLOCKED);
	if (bm_old) ks_hash_remove(bs->methods, (void *)name);
	ret = ks_hash_insert(bs->methods, (void *)name, (void *)bm);
	ks_hash_write_unlock(bs->methods);

	if (bm_old) blade_method_destroy(&bm_old);

	return ret;
}

KS_DECLARE(ks_status_t) blade_space_methods_remove(blade_space_t *bs, blade_method_t *bm)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	const char *name = NULL;

	ks_assert(bs);
	ks_assert(bm);

	name = blade_method_name_get(bm);
	ks_assert(name);

	ks_hash_write_lock(bs->methods);
	ks_hash_remove(bs->methods, (void *)name);
	ks_hash_write_unlock(bs->methods);

	return ret;
}

KS_DECLARE(blade_method_t *) blade_space_methods_get(blade_space_t *bs, const char *name)
{
	blade_method_t *bm = NULL;

	ks_assert(bs);
	ks_assert(name);

	ks_hash_read_lock(bs->methods);
	bm = ks_hash_search(bs->methods, (void *)name, KS_UNLOCKED);
	ks_hash_read_unlock(bs->methods);

	return bm;
}


/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
