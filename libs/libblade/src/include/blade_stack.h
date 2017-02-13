/*
 * Copyright (c) 2007-2014, Anthony Minessale II
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

#ifndef _BLADE_STACK_H_
#define _BLADE_STACK_H_
#include <blade.h>

#define BLADE_HANDLE_TPOOL_MIN 2
#define BLADE_HANDLE_TPOOL_MAX 8
#define BLADE_HANDLE_TPOOL_STACK (1024 * 256)
#define BLADE_HANDLE_TPOOL_IDLE 10

KS_BEGIN_EXTERN_C
KS_DECLARE(ks_status_t) blade_handle_destroy(blade_handle_t **bhP);
KS_DECLARE(ks_status_t) blade_handle_create(blade_handle_t **bhP, ks_pool_t *pool, ks_thread_pool_t *tpool);
KS_DECLARE(ks_status_t) blade_handle_startup(blade_handle_t *bh, config_setting_t *config);
KS_DECLARE(ks_status_t) blade_handle_shutdown(blade_handle_t *bh);
KS_DECLARE(ks_pool_t *) blade_handle_pool_get(blade_handle_t *bh);
KS_DECLARE(ks_thread_pool_t *) blade_handle_tpool_get(blade_handle_t *bh);

KS_DECLARE(ks_status_t) blade_handle_transport_register(blade_handle_t *bh, blade_transport_callbacks_t *callbacks);
KS_DECLARE(ks_status_t) blade_handle_transport_unregister(blade_handle_t *bh, blade_transport_callbacks_t *callbacks);
						
KS_DECLARE(ks_status_t) blade_handle_message_claim(blade_handle_t *bh, blade_message_t **message, void *data, ks_size_t data_length);
KS_DECLARE(ks_status_t) blade_handle_message_discard(blade_handle_t *bh, blade_message_t **message);

KS_DECLARE(ks_bool_t) blade_handle_datastore_available(blade_handle_t *bh);
KS_DECLARE(ks_status_t) blade_handle_datastore_store(blade_handle_t *bh, const void *key, int32_t key_length, const void *data, int64_t data_length);
KS_DECLARE(ks_status_t) blade_handle_datastore_fetch(blade_handle_t *bh,
													 blade_datastore_fetch_callback_t callback,
													 const void *key,
													 int32_t key_length,
													 void *userdata);
KS_END_EXTERN_C

#endif

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
