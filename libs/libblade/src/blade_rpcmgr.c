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

struct blade_rpcmgr_s {
	blade_handle_t *handle;

	ks_hash_t *corerpcs; // method, blade_rpc_t*
	ks_hash_t *protocolrpcs; // method, blade_rpc_t*

	ks_hash_t *requests; // id, blade_rpc_request_t*
	ks_time_t requests_ttlcheck;
	ks_mutex_t* requests_ttlcheck_lock;
};


static void blade_rpcmgr_cleanup(void *ptr, void *arg, ks_pool_cleanup_action_t action, ks_pool_cleanup_type_t type)
{
	blade_rpcmgr_t *brpcmgr = (blade_rpcmgr_t *)ptr;
	ks_hash_iterator_t *it = NULL;

	ks_assert(brpcmgr);

	switch (action) {
	case KS_MPCL_ANNOUNCE:
		break;
	case KS_MPCL_TEARDOWN:
		while ((it = ks_hash_first(brpcmgr->protocolrpcs, KS_UNLOCKED)) != NULL) {
			void *key = NULL;
			blade_rpc_t *value = NULL;

			ks_hash_this(it, (const void **)&key, NULL, (void **)&value);
			ks_hash_remove(brpcmgr->protocolrpcs, key);

			blade_rpc_destroy(&value); // must call destroy to close the method pool, using FREE_VALUE on the hash would attempt to free the method from the wrong pool
		}
		while ((it = ks_hash_first(brpcmgr->corerpcs, KS_UNLOCKED)) != NULL) {
			void *key = NULL;
			blade_rpc_t *value = NULL;

			ks_hash_this(it, (const void **)&key, NULL, (void **)&value);
			ks_hash_remove(brpcmgr->corerpcs, key);

			blade_rpc_destroy(&value); // must call destroy to close the rpc pool, using FREE_VALUE on the hash would attempt to free the rpc from the wrong pool
		}
		break;
	case KS_MPCL_DESTROY:
		break;
	}
}

KS_DECLARE(ks_status_t) blade_rpcmgr_create(blade_rpcmgr_t **brpcmgrP, blade_handle_t *bh)
{
	ks_pool_t *pool = NULL;
	blade_rpcmgr_t *brpcmgr = NULL;

	ks_assert(brpcmgrP);
	
	ks_pool_open(&pool);
	ks_assert(pool);

	brpcmgr = ks_pool_alloc(pool, sizeof(blade_rpcmgr_t));
	brpcmgr->handle = bh;

	ks_hash_create(&brpcmgr->corerpcs, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY, pool);
	ks_assert(brpcmgr->corerpcs);

	ks_hash_create(&brpcmgr->protocolrpcs, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY, pool);
	ks_assert(brpcmgr->protocolrpcs);

	ks_hash_create(&brpcmgr->requests, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY, pool);
	ks_assert(brpcmgr->requests);

	ks_mutex_create(&brpcmgr->requests_ttlcheck_lock, KS_MUTEX_FLAG_NON_RECURSIVE, pool);
	ks_assert(brpcmgr->requests_ttlcheck_lock);

	ks_pool_set_cleanup(brpcmgr, NULL, blade_rpcmgr_cleanup);

	*brpcmgrP = brpcmgr;

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) blade_rpcmgr_destroy(blade_rpcmgr_t **brpcmgrP)
{
	blade_rpcmgr_t *brpcmgr = NULL;
	ks_pool_t *pool;

	ks_assert(brpcmgrP);
	ks_assert(*brpcmgrP);

	brpcmgr = *brpcmgrP;
	*brpcmgrP = NULL;

	pool = ks_pool_get(brpcmgr);

	ks_pool_close(&pool);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(blade_handle_t *) blade_rpcmgr_handle_get(blade_rpcmgr_t *brpcmgr)
{
	ks_assert(brpcmgr);

	return brpcmgr->handle;
}

KS_DECLARE(blade_rpc_t *) blade_rpcmgr_corerpc_lookup(blade_rpcmgr_t *brpcmgr, const char *method)
{
	blade_rpc_t *brpc = NULL;

	ks_assert(brpcmgr);
	ks_assert(method);

	brpc = (blade_rpc_t *)ks_hash_search(brpcmgr->corerpcs, (void *)method, KS_READLOCKED);
	// @todo if (brpc) blade_rpc_read_lock(brpc);
	ks_hash_read_unlock(brpcmgr->corerpcs);

	return brpc;
}

KS_DECLARE(ks_status_t) blade_rpcmgr_corerpc_add(blade_rpcmgr_t *brpcmgr, blade_rpc_t *brpc)
{
	char *key = NULL;

	ks_assert(brpcmgr);
	ks_assert(brpc);

	key = ks_pstrdup(ks_pool_get(brpcmgr), blade_rpc_method_get(brpc));
	ks_hash_insert(brpcmgr->corerpcs, (void *)key, (void *)brpc);

	ks_log(KS_LOG_DEBUG, "CoreRPC Added: %s\n", key);

	return KS_STATUS_SUCCESS;

}

KS_DECLARE(ks_status_t) blade_rpcmgr_corerpc_remove(blade_rpcmgr_t *brpcmgr, blade_rpc_t *brpc)
{
	const char *method = NULL;

	ks_assert(brpcmgr);
	ks_assert(brpc);

	method = blade_rpc_method_get(brpc);
	ks_hash_remove(brpcmgr->corerpcs, (void *)method);

	ks_log(KS_LOG_DEBUG, "CoreRPC Removed: %s\n", method);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(blade_rpc_t *) blade_rpcmgr_protocolrpc_lookup(blade_rpcmgr_t *brpcmgr, const char *method, const char *protocol)
{
	blade_rpc_t *brpc = NULL;
	char *key = NULL;

	ks_assert(brpcmgr);
	ks_assert(method);
	ks_assert(protocol);

	key = ks_psprintf(ks_pool_get(brpcmgr), "%s:%s", protocol, method);
	brpc = ks_hash_search(brpcmgr->protocolrpcs, (void *)key, KS_READLOCKED);
	// @todo if (brpc) blade_rpc_read_lock(brpc);
	ks_hash_read_unlock(brpcmgr->protocolrpcs);

	ks_pool_free(&key);

	return brpc;
}

KS_DECLARE(ks_status_t) blade_rpcmgr_protocolrpc_add(blade_rpcmgr_t *brpcmgr, blade_rpc_t *brpc)
{
	const char *method = NULL;
	const char *protocol = NULL;
	char *key = NULL;

	ks_assert(brpcmgr);
	ks_assert(brpc);

	method = blade_rpc_method_get(brpc);
	ks_assert(method);

	protocol = blade_rpc_protocol_get(brpc);
	ks_assert(protocol);

	key = ks_psprintf(ks_pool_get(brpcmgr), "%s:%s", protocol, method);
	ks_assert(key);

	ks_hash_insert(brpcmgr->protocolrpcs, (void *)key, (void *)brpc);

	ks_log(KS_LOG_DEBUG, "ProtocolRPC Added: %s\n", key);

return KS_STATUS_SUCCESS;

}

KS_DECLARE(ks_status_t) blade_rpcmgr_protocolrpc_remove(blade_rpcmgr_t *brpcmgr, blade_rpc_t *brpc)
{
	const char *method = NULL;
	const char *protocol = NULL;
	char *key = NULL;

	ks_assert(brpcmgr);
	ks_assert(brpc);

	method = blade_rpc_method_get(brpc);
	ks_assert(method);

	protocol = blade_rpc_protocol_get(brpc);
	ks_assert(protocol);

	key = ks_psprintf(ks_pool_get(brpcmgr), "%s:%s", protocol, method);
	ks_assert(key);

	ks_hash_remove(brpcmgr->protocolrpcs, (void *)key);

	ks_log(KS_LOG_DEBUG, "ProtocolRPC Removed: %s\n", key);

	ks_pool_free(&key);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(blade_rpc_request_t *) blade_rpcmgr_request_lookup(blade_rpcmgr_t *brpcmgr, const char *id)
{
	blade_rpc_request_t *brpcreq = NULL;

	ks_assert(brpcmgr);
	ks_assert(id);

	brpcreq = (blade_rpc_request_t *)ks_hash_search(brpcmgr->requests, (void *)id, KS_READLOCKED);
	// @todo if (brpcreq) blade_rpc_request_read_lock(brpcreq);
	ks_hash_read_unlock(brpcmgr->requests);

	return brpcreq;
}

KS_DECLARE(ks_status_t) blade_rpcmgr_request_add(blade_rpcmgr_t *brpcmgr, blade_rpc_request_t *brpcreq)
{
	char *key = NULL;

	ks_assert(brpcmgr);
	ks_assert(brpcreq);

	key = ks_pstrdup(ks_pool_get(brpcmgr), blade_rpc_request_messageid_get(brpcreq));
	ks_hash_insert(brpcmgr->requests, (void *)key, (void *)brpcreq);

	ks_log(KS_LOG_DEBUG, "Request Added: %s\n", key);

	return KS_STATUS_SUCCESS;

}

KS_DECLARE(ks_status_t) blade_rpcmgr_request_remove(blade_rpcmgr_t *brpcmgr, blade_rpc_request_t *brpcreq)
{
	const char *id = NULL;

	ks_assert(brpcmgr);
	ks_assert(brpcreq);

	id = blade_rpc_request_messageid_get(brpcreq);
	ks_hash_remove(brpcmgr->requests, (void *)id);

	ks_log(KS_LOG_DEBUG, "Request Removed: %s\n", id);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) blade_rpcmgr_request_timeouts(blade_rpcmgr_t *brpcmgr)
{
	blade_session_t *loopback = NULL;

	ks_assert(brpcmgr);

	// All this stuff is to ensure that the requests hash is not locked up when it does not need to be,
	// and this will also ensure that sessions will not lock up if any other session is trying to deal
	// with timeouts already
	if (ks_mutex_trylock(brpcmgr->requests_ttlcheck_lock) != KS_STATUS_SUCCESS) return KS_STATUS_SUCCESS;

	if (brpcmgr->requests_ttlcheck > ks_time_now()) {
		ks_mutex_unlock(brpcmgr->requests_ttlcheck_lock);
		return KS_STATUS_SUCCESS;
	}

	ks_hash_write_lock(brpcmgr->requests);

	// Give a one second delay between timeout checking
	brpcmgr->requests_ttlcheck = ks_time_now() + KS_USEC_PER_SEC;

	ks_mutex_unlock(brpcmgr->requests_ttlcheck_lock);

	// Now find all the expired requests and send out loopback error responses, which will invoke request removal when received and processed
	for (ks_hash_iterator_t *it = ks_hash_first(brpcmgr->requests, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
		const char *key = NULL;
		blade_rpc_request_t *value = NULL;
		cJSON *res = NULL;

		ks_hash_this(it, (const void **)&key, NULL, (void **)&value);

		if (!blade_rpc_request_expired(value)) continue;

		if (!loopback) loopback = blade_sessionmgr_loopback_lookup(blade_handle_sessionmgr_get(brpcmgr->handle));

		ks_log(KS_LOG_DEBUG, "Request (%s) TTL timeout\n", key);

		blade_rpc_error_raw_create(&res, NULL, blade_rpc_request_messageid_get(value), -32000, "Request timed out");
		// @todo may want to include requester-nodeid and responder-nodeid into the error block when they are present in the original request
		// even though a response or error without them is treated as locally processed, it may be useful to know who the request was attempted with
		// when multiple options are available such as a blade.execute where the protocol has multiple possible controllers to pick from
		blade_session_send(loopback, res, 0, NULL, NULL);

		cJSON_Delete(res);
	}

	if (loopback) blade_session_read_unlock(loopback);

	ks_hash_write_unlock(brpcmgr->requests);

	return KS_STATUS_SUCCESS;
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
