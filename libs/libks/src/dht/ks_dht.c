#include "ks_dht.h"
#include "ks_dht-int.h"
#include "sodium.h"

KS_DECLARE(ks_status_t) ks_dht_create(ks_dht_t **dht, ks_pool_t *pool, ks_thread_pool_t *tpool)
{
	ks_bool_t pool_alloc = !pool;
	ks_dht_t *d = NULL;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);

	*dht = NULL;

	/**
	 * Create a new internally managed pool if one wasn't provided, and returns KS_STATUS_NO_MEM if pool was not created.
	 */
	if (pool_alloc) {
		ks_pool_open(&pool);
		ks_assert(pool);
	}

	/**
	 * Allocate the dht instance from the pool, and returns KS_STATUS_NO_MEM if the dht was not created.
	 */
	*dht = d = ks_pool_alloc(pool, sizeof(ks_dht_t));
	ks_assert(d);

	/**
	 * Keep track of the pool used for future allocations and cleanup.
	 * Keep track of whether the pool was created internally or not.
	 */
	d->pool = pool;
	d->pool_alloc = pool_alloc;

	/**
	 * Create a new internally managed thread pool if one wasn't provided.
	 */
	d->tpool = tpool;
	if (!tpool) {
		d->tpool_alloc = KS_TRUE;
		ks_thread_pool_create(&d->tpool, KS_DHT_TPOOL_MIN, KS_DHT_TPOOL_MAX, KS_DHT_TPOOL_STACK, KS_PRI_NORMAL, KS_DHT_TPOOL_IDLE);
		ks_assert(d->tpool);
	}

	/**
	 * Default autorouting to disabled.
	 */
	d->autoroute = KS_FALSE;
	d->autoroute_port = 0;

	/**
	 * Create the message type registry.
	 */
	ks_hash_create(&d->registry_type, KS_HASH_MODE_DEFAULT, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK, d->pool);
	ks_assert(d->registry_type);

	/**
	 * Register the message type callbacks for query (q), response (r), and error (e)
	 */
	ks_dht_register_type(d, "q", ks_dht_process_query);
	ks_dht_register_type(d, "r", ks_dht_process_response);
	ks_dht_register_type(d, "e", ks_dht_process_error);

	/**
	 * Create the message query registry.
	 */
	ks_hash_create(&d->registry_query, KS_HASH_MODE_DEFAULT, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK, d->pool);
	ks_assert(d->registry_query);

	/**
	 * Register the message query callbacks for ping, find_node, etc.
	 */
	ks_dht_register_query(d, "ping", ks_dht_process_query_ping);
	ks_dht_register_query(d, "find_node", ks_dht_process_query_findnode);
	ks_dht_register_query(d, "get", ks_dht_process_query_get);
	ks_dht_register_query(d, "put", ks_dht_process_query_put);

	/**
	 * Create the message error registry.
	 */
	ks_hash_create(&d->registry_error, KS_HASH_MODE_DEFAULT, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK, d->pool);
	ks_assert(d->registry_error);
	// @todo register 301 error for internal get/put CAS hash mismatch retry handler

	/**
	 * Default these to FALSE, binding will set them TRUE when a respective address is bound.
	 * @todo these may not be useful anymore they are from legacy code
	 */
    d->bind_ipv4 = KS_FALSE;
	d->bind_ipv6 = KS_FALSE;

	/**
	 * Initialize the data used to track endpoints to NULL, binding will handle latent allocations.
	 * The endpoints and endpoints_poll arrays are maintained in parallel to optimize polling.
	 */
	d->endpoints = NULL;
	d->endpoints_size = 0;
	d->endpoints_poll = NULL;

	/**
	 * Create the endpoints hash for fast lookup, this is used to route externally provided remote addresses when the local endpoint is unknown.
	 * This also provides the basis for autorouting to find unbound interfaces and bind them at runtime.
	 * This hash uses the host ip string concatenated with a colon and the port, ie: "123.123.123.123:123" or ipv6 equivilent
	 */
	ks_hash_create(&d->endpoints_hash, KS_HASH_MODE_DEFAULT, KS_HASH_FLAG_RWLOCK, d->pool);
	ks_assert(d->endpoints_hash);

	/**
	 * Default expirations to not be checked for one pulse.
	 */
	d->pulse_expirations = ks_time_now() + (KS_DHT_PULSE_EXPIRATIONS * 1000);

	/**
	 * Create the queue for outgoing messages, this ensures sending remains async and can be throttled when system buffers are full.
	 */
	ks_q_create(&d->send_q, d->pool, 0);
	ks_assert(d->send_q);
	
	/**
	 * If a message is popped from the queue for sending but the system buffers are too full, this is used to temporarily store the message.
	 */
	d->send_q_unsent = NULL;

	/**
	 * The dht uses a single internal large receive buffer for receiving all frames, this may change in the future to offload processing to a threadpool.
	 */
	d->recv_buffer_length = 0;

	/**
	 * Initialize the transaction id mutex, should use atomic increment instead
	 */
	ks_mutex_create(&d->tid_mutex, KS_MUTEX_FLAG_DEFAULT, d->pool);
	ks_assert(d->tid_mutex);

	/**
	 * Initialize the first transaction id randomly, this doesn't really matter.
	 */
	d->transactionid_next = 1; //rand();

	/**
	 * Create the hash to track pending transactions on queries that are pending responses.
	 * It should be impossible to receive a duplicate transaction id in the hash before it expires, but if it does an error is preferred.
	 */
	ks_hash_create(&d->transactions_hash, KS_HASH_MODE_INT, KS_HASH_FLAG_RWLOCK, d->pool);
	ks_assert(d->transactions_hash);

	/**
	 * The internal route tables will be latent allocated when binding.
	 */
	d->rt_ipv4 = NULL;
	d->rt_ipv6 = NULL;

	/**
	 * Create the hash to store searches.
	 */
	ks_hash_create(&d->search_hash, KS_HASH_MODE_ARBITRARY, KS_HASH_FLAG_NOLOCK | KS_HASH_FLAG_DUP_CHECK, d->pool);
	ks_assert(d->search_hash);

	/**
	 * The search hash uses arbitrary key size, which requires the key size be provided.
	 */
	ks_hash_set_keysize(d->search_hash, KS_DHT_NODEID_SIZE);
	
	/**
	 * The opaque write tokens require some entropy for generating which needs to change periodically but accept tokens using the last two secrets.
	 */
	d->token_secret_current = d->token_secret_previous = rand();
	d->token_secret_expiration = ks_time_now() + (KS_DHT_TOKENSECRET_EXPIRATION * 1000);

	/**
	 * Create the hash to store arbitrary data for BEP44.
	 */
	ks_hash_create(&d->storage_hash, KS_HASH_MODE_ARBITRARY, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK, d->pool);
	ks_assert(d->storage_hash);

	/**
	 * The storage hash uses arbitrary key size, which requires the key size be provided, they are the same size as nodeid's.
	 */
	ks_hash_set_keysize(d->storage_hash, KS_DHT_NODEID_SIZE);

	// done:
	if (ret != KS_STATUS_SUCCESS) {
		if (d) ks_dht_destroy(&d);
		else if (pool_alloc && pool) ks_pool_close(&pool);

		*dht = NULL;
	}
	return ret;
}

KS_DECLARE(void) ks_dht_destroy(ks_dht_t **dht)
{
	ks_dht_t *d = NULL;
	ks_pool_t *pool = NULL;
	ks_bool_t pool_alloc = KS_FALSE;
	ks_hash_iterator_t *it = NULL;
	
	ks_assert(dht);
	ks_assert(*dht);

	d = *dht;

	/**
	 * Cleanup the storage hash and it's contents if it is allocated.
	 */
	if (d->storage_hash) {
		for (it = ks_hash_first(d->storage_hash, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
			ks_dht_storageitem_t *val;

			ks_hash_this_val(it, (void **)&val);

			ks_dht_storageitem_destroy(&val);
		}
		ks_hash_destroy(&d->storage_hash);
	}

	/**
	 * Zero out the opaque write token variables.
	 */
	d->token_secret_current = 0;
	d->token_secret_previous = 0;
	d->token_secret_expiration = 0;

	/**
	 * Cleanup the search hash and it's contents if it is allocated.
	 */
	if (d->search_hash) {
		for (it = ks_hash_first(d->search_hash, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
			ks_dht_search_t *val;

			ks_hash_this_val(it, (void **)&val);
			ks_dht_search_destroy(&val);
		}
		ks_hash_destroy(&d->search_hash);
	}

	/**
	 * Cleanup the route tables if they are allocated.
	 * @todo check if endpoints need to be destroyed first to release the readlock on their node
	 */
	if (d->rt_ipv4) ks_dhtrt_deinitroute(&d->rt_ipv4);
	if (d->rt_ipv6) ks_dhtrt_deinitroute(&d->rt_ipv6);

	/**
	 * Cleanup the transactions mutex and hash if they are allocated.
	 */
	d->transactionid_next = 0;
	if (d->tid_mutex) ks_mutex_destroy(&d->tid_mutex);
	if (d->transactions_hash) ks_hash_destroy(&d->transactions_hash);

	/**
	 * Probably don't need this, recv_buffer_length is temporary and may change
	 */
	d->recv_buffer_length = 0;

	/**
	 * Cleanup the send queue and it's contents if it is allocated.
	 */
	if (d->send_q) {
		ks_dht_message_t *msg;
		while (ks_q_pop_timeout(d->send_q, (void **)&msg, 1) == KS_STATUS_SUCCESS && msg) ks_dht_message_destroy(&msg);
		ks_q_destroy(&d->send_q);
	}
	
	/**
	 * Cleanup the cached popped message if it is set.
	 */
	if (d->send_q_unsent) ks_dht_message_destroy(&d->send_q_unsent);

	/**
	 * Probably don't need this
	 */
	d->pulse_expirations = 0;

	/**
	 * Cleanup any endpoints that have been allocated.
	 */
	for (int32_t i = 0; i < d->endpoints_size; ++i) {
		ks_dht_endpoint_t *ep = d->endpoints[i];
		ks_dht_endpoint_destroy(&ep);
	}
	d->endpoints_size = 0;

	/**
	 * Cleanup the array of endpoint pointers if it is allocated.
	 */
	if (d->endpoints) {
		ks_pool_free(d->pool, d->endpoints);
		d->endpoints = NULL;
	}

	/**
	 * Cleanup the array of endpoint polling data if it is allocated.
	 */
	if (d->endpoints_poll) {
		ks_pool_free(d->pool, d->endpoints_poll);
		d->endpoints_poll = NULL;
	}

	/**
	 * Cleanup the endpoints hash if it is allocated.
	 */
	if (d->endpoints_hash) ks_hash_destroy(&d->endpoints_hash);

	/**
	 * Probably don't need this
	 */
	d->bind_ipv4 = KS_FALSE;
	d->bind_ipv6 = KS_FALSE;

	/**
	 * Cleanup the type, query, and error registries if they have been allocated.
	 */
	if (d->registry_type) ks_hash_destroy(&d->registry_type);
	if (d->registry_query) ks_hash_destroy(&d->registry_query);
	if (d->registry_error) ks_hash_destroy(&d->registry_error);

	/**
	 * Probably don't need this
	 */
	d->autoroute = KS_FALSE;
	d->autoroute_port = 0;

	/**
	 * If the thread pool was allocated internally, destroy it.
	 * If this fails, something catastrophically bad happened like memory corruption.
	 */
	if (d->tpool_alloc) ks_thread_pool_destroy(&d->tpool);
	d->tpool_alloc = KS_FALSE;

	/**
	 * Temporarily store the allocator level variables because freeing the dht instance will invalidate it.
	 */
	pool = d->pool;
	pool_alloc = d->pool_alloc;

	/**
	 * Free the dht instance from the pool, after this the dht instance memory is invalid.
	 */
	ks_pool_free(d->pool, d);

	/**
	 * At this point dht instance is invalidated so NULL the pointer.
	 */
	*dht = d = NULL;

	/**
	 * If the pool was allocated internally, destroy it using the temporary variables stored earlier.
	 * If this fails, something catastrophically bad happened like memory corruption.
	 */
	if (pool_alloc) ks_pool_close(&pool);
}
												

KS_DECLARE(void) ks_dht_autoroute(ks_dht_t *dht, ks_bool_t autoroute, ks_port_t port)
{
	ks_assert(dht);

	/**
	 * If autorouting is being disabled, port is always set to zero, otherwise if the port is zero use the DHT default port
	 */
	if (!autoroute) port = 0;
    else if (port <= 0) port = KS_DHT_DEFAULT_PORT;

	/**
	 * Set the autoroute state
	 */
	dht->autoroute = autoroute;
	dht->autoroute_port = port;
}

KS_DECLARE(ks_status_t) ks_dht_autoroute_check(ks_dht_t *dht, ks_sockaddr_t *raddr, ks_dht_endpoint_t **endpoint)
{
	// @todo lookup standard def for IPV6 max size
	char ip[48 + 1];
	ks_dht_endpoint_t *ep = NULL;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(endpoint);

	/**
	 * If the endpoint is already provided just leave it alone and return successfully.
	 */
	if (*endpoint) return KS_STATUS_SUCCESS;

	/**
	 * Use the remote address to figure out what local address we should use to attempt contacting it.
	 */
	if ((ret = ks_ip_route(ip, sizeof(ip), raddr->host)) != KS_STATUS_SUCCESS) return ret;

	/**
	 * Check if the endpoint has already been bound for the address we want to route through.
	 */
	ep = ks_hash_search(dht->endpoints_hash, ip, KS_READLOCKED);
	if ((ret = ks_hash_read_unlock(dht->endpoints_hash)) != KS_STATUS_SUCCESS) return ret;

	/**
	 * If the endpoint has not been bound, and autorouting is enabled then try to bind the new address.
	 */
	if (!ep && dht->autoroute) {
		ks_sockaddr_t addr;
		if ((ret = ks_addr_set(&addr, ip, dht->autoroute_port, raddr->family)) != KS_STATUS_SUCCESS) return ret;
		if ((ret = ks_dht_bind(dht, NULL, &addr, &ep)) != KS_STATUS_SUCCESS) return ret;
	}

	/**
	 * If no endpoint can be found to route through then all hope is lost, bail out with a failure.
	 */
	if (!ep) {
		ks_log(KS_LOG_DEBUG, "No route available to %s\n", raddr->host);
		return KS_STATUS_FAIL;
	}

	/**
	 * Reaching here means an endpoint is available, assign it and return successfully.
	 */
	*endpoint = ep;

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_dht_register_type(ks_dht_t *dht, const char *value, ks_dht_message_callback_t callback)
{
	ks_assert(dht);
	ks_assert(value);
	ks_assert(callback);

	return ks_hash_insert(dht->registry_type, (void *)value, (void *)(intptr_t)callback);
}

KS_DECLARE(ks_status_t) ks_dht_register_query(ks_dht_t *dht, const char *value, ks_dht_message_callback_t callback)
{
	ks_assert(dht);
	ks_assert(value);
	ks_assert(callback);

	return ks_hash_insert(dht->registry_query, (void *)value, (void *)(intptr_t)callback);
}

KS_DECLARE(ks_status_t) ks_dht_register_error(ks_dht_t *dht, const char *value, ks_dht_message_callback_t callback)
{
	ks_assert(dht);
	ks_assert(value);
	ks_assert(callback);

	return ks_hash_insert(dht->registry_error, (void *)value, (void *)(intptr_t)callback);
}


KS_DECLARE(ks_status_t) ks_dht_bind(ks_dht_t *dht, const ks_dht_nodeid_t *nodeid, const ks_sockaddr_t *addr, ks_dht_endpoint_t **endpoint)
{
	ks_dht_endpoint_t *ep = NULL;
	ks_socket_t sock = KS_SOCK_INVALID;
	int32_t epindex = 0;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(addr);
	ks_assert(addr->family == AF_INET || addr->family == AF_INET6);
	ks_assert(addr->port);

	/**
	 * If capturing the endpoint output, make sure it is set NULL to start with.
	 */
	if (endpoint) *endpoint = NULL;

	ep = ks_hash_search(dht->endpoints_hash, (void *)addr->host, KS_READLOCKED);
	ks_hash_read_unlock(dht->endpoints_hash);
	if (ep) {
		ks_log(KS_LOG_DEBUG, "Attempted to bind to %s more than once.\n", addr->host);
		return KS_STATUS_FAIL;
	}

	/**
	 * Legacy code, this can probably go away
	 */
	dht->bind_ipv4 |= addr->family == AF_INET;
	dht->bind_ipv6 |= addr->family == AF_INET6;

	/**
	 * Attempt to open a UDP datagram socket for the given address family.
	 */
	if ((sock = socket(addr->family, SOCK_DGRAM, IPPROTO_UDP)) == KS_SOCK_INVALID) return KS_STATUS_FAIL;

	/**
	 * Set some common socket options for non-blocking IO and forced binding when already in use
	 */
	if ((ret = ks_socket_option(sock, SO_REUSEADDR, KS_TRUE)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_socket_option(sock, KS_SO_NONBLOCK, KS_TRUE)) != KS_STATUS_SUCCESS) goto done;
	
	/**
	 * Attempt to bind the socket to the desired local address.
	 */
	// @todo shouldn't ks_addr_bind take a const addr *?
	if ((ret = ks_addr_bind(sock, (ks_sockaddr_t *)addr)) != KS_STATUS_SUCCESS) goto done;

	/**
	 * Allocate the endpoint to track the local socket.
	 */
	ks_dht_endpoint_create(&ep, dht->pool, nodeid, addr, sock);
	ks_assert(ep);

	/**
	 * Resize the endpoints array to take another endpoint pointer.
	 */
	epindex = dht->endpoints_size++;
	dht->endpoints = (ks_dht_endpoint_t **)ks_pool_resize(dht->pool,
														   (void *)dht->endpoints,
														   sizeof(ks_dht_endpoint_t *) * dht->endpoints_size);
	ks_assert(dht->endpoints);
	dht->endpoints[epindex] = ep;

	/**
	 * Add the new endpoint into the endpoints hash for quick lookups.
	 * @todo insert returns 0 when OOM, ks_pool_alloc will abort so insert can only succeed
	 */
	if ((ret = ks_hash_insert(dht->endpoints_hash, ep->addr.host, ep)) != KS_STATUS_SUCCESS) goto done;

	/**
	 * Resize the endpoints_poll array to keep in parallel with endpoints array, populate new entry with the right data.
	 */
	dht->endpoints_poll = (struct pollfd *)ks_pool_resize(dht->pool,
														  (void *)dht->endpoints_poll,
														  sizeof(struct pollfd) * dht->endpoints_size);
	ks_assert(dht->endpoints_poll);
	dht->endpoints_poll[epindex].fd = ep->sock;
	dht->endpoints_poll[epindex].events = POLLIN | POLLERR;

	/**
	 * If the route table for the family doesn't exist yet, initialize a new route table and create a local node for the endpoint.
	 */
	if (ep->addr.family == AF_INET) {
		if (!dht->rt_ipv4 && (ret = ks_dhtrt_initroute(&dht->rt_ipv4, dht->pool)) != KS_STATUS_SUCCESS) goto done;
		if ((ret = ks_dhtrt_create_node(dht->rt_ipv4,
										ep->nodeid,
										KS_DHT_LOCAL,
										ep->addr.host,
										ep->addr.port,
										&ep->node)) != KS_STATUS_SUCCESS) goto done;
	} else {
		if (!dht->rt_ipv6 && (ret = ks_dhtrt_initroute(&dht->rt_ipv6, dht->pool)) != KS_STATUS_SUCCESS) goto done;
		if ((ret = ks_dhtrt_create_node(dht->rt_ipv6,
										ep->nodeid,
										KS_DHT_LOCAL,
										ep->addr.host,
										ep->addr.port,
										&ep->node)) != KS_STATUS_SUCCESS) goto done;
	}
	/**
	 * Do not release the ep->node, keep it alive until cleanup
	 */

	/**
	 * If the endpoint output is being captured, assign it and return successfully.
	 */
	if (endpoint) *endpoint = ep;

 done:
	if (ret != KS_STATUS_SUCCESS) {
		/**
		 * If any failures occur, we need to make sure the socket is properly closed.
		 * This will be done in ks_dht_endpoint_destroy only if the socket was assigned during a successful ks_dht_endpoint_create.
		 * Then return whatever failure condition resulted in landed here.
		 */
		if (ep) {
			ks_hash_remove(dht->endpoints_hash, ep->addr.host);
			ks_dht_endpoint_destroy(&ep);
		}
		else if (sock != KS_SOCK_INVALID) ks_socket_close(&sock);

		if (endpoint) *endpoint = NULL;
	}
	return ret;
}

KS_DECLARE(void) ks_dht_pulse(ks_dht_t *dht, int32_t timeout)
{
	ks_dht_datagram_t *datagram = NULL;
	ks_sockaddr_t raddr;

	ks_assert(dht);
	ks_assert(timeout > 0);

	if (dht->send_q_unsent || ks_q_size(dht->send_q) > 0) timeout = 0;

	// @todo confirm how poll/wsapoll react to zero size and NULL array
	if (ks_poll(dht->endpoints_poll, dht->endpoints_size, timeout) > 0) {
		for (int32_t i = 0; i < dht->endpoints_size; ++i) {
			if (!(dht->endpoints_poll[i].revents & POLLIN)) continue;

			raddr = (const ks_sockaddr_t){ 0 };
			dht->recv_buffer_length = sizeof(dht->recv_buffer);
			raddr.family = dht->endpoints[i]->addr.family;
			if (ks_socket_recvfrom(dht->endpoints_poll[i].fd, dht->recv_buffer, &dht->recv_buffer_length, &raddr) != KS_STATUS_SUCCESS) continue;

			if (dht->recv_buffer_length == sizeof(dht->recv_buffer)) {
				ks_log(KS_LOG_DEBUG, "Dropped oversize datagram from %s %d\n", raddr.host, raddr.port);
				continue;
			}

			ks_dht_datagram_create(&datagram, dht->pool, dht, dht->endpoints[i], &raddr);
			ks_assert(datagram);

			if (ks_thread_pool_add_job(dht->tpool, ks_dht_process, datagram) != KS_STATUS_SUCCESS) ks_dht_datagram_destroy(&datagram);
		}
	}

	ks_dht_pulse_send(dht);

	ks_dht_pulse_expirations(dht);

	if (dht->rt_ipv4) ks_dhtrt_process_table(dht->rt_ipv4);
	if (dht->rt_ipv6) ks_dhtrt_process_table(dht->rt_ipv6);
}

KS_DECLARE(void) ks_dht_pulse_expirations(ks_dht_t *dht)
{
	ks_hash_iterator_t *it = NULL;
	ks_time_t now = ks_time_now();

	ks_assert(dht);

	if (dht->pulse_expirations > now) return;
	dht->pulse_expirations = now + (KS_DHT_PULSE_EXPIRATIONS * 1000);

	ks_hash_write_lock(dht->transactions_hash);
	for (it = ks_hash_first(dht->transactions_hash, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
		const void *key = NULL;
		ks_dht_transaction_t *value = NULL;
		ks_bool_t remove = KS_FALSE;

		ks_hash_this(it, &key, NULL, (void **)&value);
		if (value->finished) remove = KS_TRUE;
		else if (value->expiration <= now) {
			ks_log(KS_LOG_DEBUG, "Transaction has expired without response %d\n", value->transactionid);
			remove = KS_TRUE;
		}
		if (remove) {
			ks_hash_remove(dht->transactions_hash, (void *)key);
			ks_dht_transaction_destroy(&value);
		}
	}
	ks_hash_write_unlock(dht->transactions_hash);

	ks_hash_write_lock(dht->search_hash);
	for (it = ks_hash_first(dht->search_hash, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
		const void *search_key = NULL;
		ks_dht_search_t *search_value = NULL;

		ks_hash_this(it, &search_key, NULL, (void **)&search_value);

		ks_hash_write_lock(search_value->pending);
		for (ks_hash_iterator_t *i = ks_hash_first(search_value->pending, KS_UNLOCKED); i; i = ks_hash_next(&i)) {
			const void *pending_key = NULL;
			ks_dht_search_pending_t *pending_value = NULL;
			ks_bool_t pending_remove = KS_FALSE;

			ks_hash_this(i, &pending_key, NULL, (void **)&pending_value);

			if (pending_value->finished) pending_remove = KS_TRUE;
			else if (pending_value->expiration <= now) {
				char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
				char id2_buf[KS_DHT_NODEID_SIZE * 2 + 1];
				ks_log(KS_LOG_DEBUG,
					   "Search for %s pending find_node to %s has expired without response\n",
					   ks_dht_hexid(&search_value->target, id_buf),
					   ks_dht_hexid(&pending_value->nodeid, id2_buf));
				pending_remove = KS_TRUE;
			}
			if (pending_remove) {
				ks_hash_remove(search_value->pending, (void *)pending_key);
				ks_dht_search_pending_destroy(&pending_value);
			}
		}
		ks_hash_write_unlock(search_value->pending);
		if (ks_hash_count(search_value->pending) == 0) {
			for (int32_t index = 0; index < search_value->callbacks_size; ++index) search_value->callbacks[index](dht, search_value);
			ks_hash_remove(dht->search_hash, (void *)search_key);
			ks_dht_search_destroy(&search_value);
		}
	}
	ks_hash_write_unlock(dht->search_hash);

	if (dht->token_secret_expiration && dht->token_secret_expiration <= now) {
		dht->token_secret_expiration = ks_time_now() + (KS_DHT_TOKENSECRET_EXPIRATION * 1000);
		dht->token_secret_previous = dht->token_secret_current;
		dht->token_secret_current = rand();
	}
}

KS_DECLARE(void) ks_dht_pulse_send(ks_dht_t *dht)
{
	ks_dht_message_t *message;
	ks_bool_t bail = KS_FALSE;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);

	while (!bail) {
		message = NULL;
		if (dht->send_q_unsent) {
			message = dht->send_q_unsent;
			dht->send_q_unsent = NULL;
		}
		if (!message) bail = ks_q_pop_timeout(dht->send_q, (void **)&message, 1) != KS_STATUS_SUCCESS || !message;
		if (!bail) {
			bail = (ret = ks_dht_send(dht, message)) != KS_STATUS_SUCCESS;
			if (ret == KS_STATUS_BREAK) dht->send_q_unsent = message;
			else ks_dht_message_destroy(&message);
		}
	}
}

KS_DECLARE(char *) ks_dht_hexid(ks_dht_nodeid_t *id, char *buffer)
{
	char *t = buffer;

	ks_assert(id);
	ks_assert(buffer);

	memset(buffer, 0, KS_DHT_NODEID_SIZE * 2 + 1);

	for (int i = 0; i < KS_DHT_NODEID_SIZE; ++i, t += 2) sprintf(t, "%02X", id->id[i]);

	return buffer;
}

KS_DECLARE(void) ks_dht_utility_nodeid_xor(ks_dht_nodeid_t *dest, ks_dht_nodeid_t *src1, ks_dht_nodeid_t *src2)
{
	ks_assert(dest);
	ks_assert(src1);
	ks_assert(src2);

	for (int32_t i = 0; i < KS_DHT_NODEID_SIZE; ++i) dest->id[i] = src1->id[i] ^ src2->id[i];
}

KS_DECLARE(ks_status_t) ks_dht_utility_compact_addressinfo(const ks_sockaddr_t *address,
														   uint8_t *buffer,
														   ks_size_t *buffer_length,
														   ks_size_t buffer_size)
{
	ks_size_t addr_len;
	const void *paddr = NULL;
	uint16_t port = 0;

	ks_assert(address);
	ks_assert(buffer);
	ks_assert(buffer_length);
	ks_assert(buffer_size);
	ks_assert(address->family == AF_INET || address->family == AF_INET6);

	// @todo change parameters to dereferenced pointer and forward buffer pointer directly

	addr_len = address->family == AF_INET ? sizeof(uint32_t) : (sizeof(uint16_t) * 8);
	
	if (*buffer_length + addr_len + sizeof(uint16_t) > buffer_size) {
		ks_log(KS_LOG_DEBUG, "Insufficient space remaining for compacting\n");
		return KS_STATUS_NO_MEM;
	}

	if (address->family == AF_INET) {
		paddr = &address->v.v4.sin_addr; // already network byte order
		port = address->v.v4.sin_port; // already network byte order
	} else {
		paddr = &address->v.v6.sin6_addr; // already network byte order
		port = address->v.v6.sin6_port; // already network byte order
	}
	memcpy(buffer + (*buffer_length), paddr, sizeof(uint32_t));
	*buffer_length += addr_len;

	memcpy(buffer + (*buffer_length), (const void *)&port, sizeof(uint16_t));
	*buffer_length += sizeof(uint16_t);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_dht_utility_expand_addressinfo(const uint8_t *buffer,
														  ks_size_t *buffer_length,
														  ks_size_t buffer_size,
														  ks_sockaddr_t *address)
{
	ks_size_t addr_len;
	const void *paddr = NULL;
	uint16_t port = 0;

	ks_assert(buffer);
	ks_assert(buffer_length);
	ks_assert(address);
	ks_assert(address->family == AF_INET ||address->family == AF_INET6);

	// @todo change parameters to dereferenced pointer and forward buffer pointer directly

	addr_len = address->family == AF_INET ? sizeof(uint32_t) : (sizeof(uint16_t) * 8);
	if (*buffer_length + addr_len + sizeof(uint16_t) > buffer_size) return KS_STATUS_NO_MEM;

	paddr = buffer + *buffer_length;
	*buffer_length += addr_len;
	port = *((uint16_t *)(buffer + *buffer_length));
	*buffer_length += sizeof(uint16_t);

	// @todo ks_addr_set_raw second parameter should be const?
	return ks_addr_set_raw(address, (void *)paddr, port, address->family);
}

KS_DECLARE(ks_status_t) ks_dht_utility_compact_nodeinfo(const ks_dht_nodeid_t *nodeid,
														const ks_sockaddr_t *address,
														uint8_t *buffer,
														ks_size_t *buffer_length,
														ks_size_t buffer_size)
{
	ks_assert(address);
	ks_assert(buffer);
	ks_assert(buffer_length);
	ks_assert(buffer_size);
	ks_assert(address->family == AF_INET || address->family == AF_INET6);

	// @todo change parameters to dereferenced pointer and forward buffer pointer directly

	if (*buffer_length + KS_DHT_NODEID_SIZE > buffer_size) {
		ks_log(KS_LOG_DEBUG, "Insufficient space remaining for compacting\n");
		return KS_STATUS_NO_MEM;
	}

	memcpy(buffer + (*buffer_length), (void *)nodeid, KS_DHT_NODEID_SIZE);
	*buffer_length += KS_DHT_NODEID_SIZE;

	return ks_dht_utility_compact_addressinfo(address, buffer, buffer_length, buffer_size);
}

KS_DECLARE(ks_status_t) ks_dht_utility_expand_nodeinfo(const uint8_t *buffer,
													   ks_size_t *buffer_length,
													   ks_size_t buffer_size,
													   ks_dht_nodeid_t *nodeid,
													   ks_sockaddr_t *address)
{
	ks_assert(buffer);
	ks_assert(buffer_length);
	ks_assert(nodeid);
	ks_assert(address);
	ks_assert(address->family == AF_INET ||address->family == AF_INET6);

	// @todo change parameters to dereferenced pointer and forward buffer pointer directly

	if (*buffer_length + KS_DHT_NODEID_SIZE > buffer_size) return KS_STATUS_NO_MEM;

	memcpy(nodeid->id, buffer + *buffer_length, KS_DHT_NODEID_SIZE);
	*buffer_length += KS_DHT_NODEID_SIZE;

	return ks_dht_utility_expand_addressinfo(buffer, buffer_length, buffer_size, address);
}

KS_DECLARE(ks_status_t) ks_dht_utility_extract_nodeid(struct bencode *args, const char *key, ks_dht_nodeid_t **nodeid)
{
	struct bencode *id;
	const char *idv;
	ks_size_t idv_len;

	ks_assert(args);
	ks_assert(key);
	ks_assert(nodeid);

	*nodeid = NULL;

    id = ben_dict_get_by_str(args, key);
    if (!id) {
		ks_log(KS_LOG_DEBUG, "Message args missing key '%s'\n", key);
		return KS_STATUS_ARG_INVALID;
	}

    idv = ben_str_val(id);
	idv_len = ben_str_len(id);
    if (idv_len != KS_DHT_NODEID_SIZE) {
		ks_log(KS_LOG_DEBUG, "Message args '%s' value has an unexpected size of %d\n", key, idv_len);
		return KS_STATUS_ARG_INVALID;
	}

	*nodeid = (ks_dht_nodeid_t *)idv;

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_dht_utility_extract_token(struct bencode *args, const char *key, ks_dht_token_t **token)
{
	struct bencode *tok;
	const char *tokv;
	ks_size_t tokv_len;

	ks_assert(args);
	ks_assert(key);
	ks_assert(token);

	*token = NULL;

    tok = ben_dict_get_by_str(args, key);
    if (!tok) {
		ks_log(KS_LOG_DEBUG, "Message args missing key '%s'\n", key);
		return KS_STATUS_ARG_INVALID;
	}

    tokv = ben_str_val(tok);
	tokv_len = ben_str_len(tok);
    if (tokv_len != KS_DHT_TOKEN_SIZE) {
		ks_log(KS_LOG_DEBUG, "Message args '%s' value has an unexpected size of %d\n", key, tokv_len);
		return KS_STATUS_ARG_INVALID;
	}

	*token = (ks_dht_token_t *)tokv;

	return KS_STATUS_SUCCESS;
}


KS_DECLARE(ks_status_t) ks_dht_token_generate(uint32_t secret, ks_sockaddr_t *raddr, ks_dht_nodeid_t *target, ks_dht_token_t *token)
{
	SHA_CTX sha;
	uint16_t port = 0;

	ks_assert(raddr);
	ks_assert(raddr->family == AF_INET || raddr->family == AF_INET6);
	ks_assert(target);
	ks_assert(token);

	secret = htonl(secret);
	port = htons(raddr->port);

	if (!SHA1_Init(&sha) ||
		!SHA1_Update(&sha, &secret, sizeof(uint32_t)) ||
		!SHA1_Update(&sha, raddr->host, strlen(raddr->host)) ||
		!SHA1_Update(&sha, &port, sizeof(uint16_t)) ||
		!SHA1_Update(&sha, target->id, KS_DHT_NODEID_SIZE) ||
		!SHA1_Final(token->token, &sha)) return KS_STATUS_FAIL;

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_bool_t) ks_dht_token_verify(ks_dht_t *dht, ks_sockaddr_t *raddr, ks_dht_nodeid_t *target, ks_dht_token_t *token)
{
	ks_dht_token_t tok;

	if (ks_dht_token_generate(dht->token_secret_current, raddr, target, &tok) != KS_STATUS_SUCCESS) return KS_FALSE;

	if (memcmp(tok.token, token->token, KS_DHT_TOKEN_SIZE) == 0) return KS_TRUE;

	if (ks_dht_token_generate(dht->token_secret_previous, raddr, target, &tok) != KS_STATUS_SUCCESS) return KS_FALSE;

	return memcmp(tok.token, token->token, KS_DHT_TOKEN_SIZE) == 0;
}


KS_DECLARE(ks_status_t) ks_dht_send(ks_dht_t *dht, ks_dht_message_t *message)
{
	// @todo calculate max IPV6 payload size?
	char buf[1000];
	ks_size_t buf_len;

	ks_assert(dht);
	ks_assert(message);
	ks_assert(message->endpoint);
	ks_assert(message->data);

	// @todo blacklist check

	// @todo use different encode function to check if all data was encoded, do not send large incomplete messages
	buf_len = ben_encode2(buf, sizeof(buf), message->data);

	ks_log(KS_LOG_DEBUG, "Sending message to %s %d\n", message->raddr.host, message->raddr.port);
	ks_log(KS_LOG_DEBUG, "%s\n", ben_print(message->data));

	return ks_socket_sendto(message->endpoint->sock, (void *)buf, &buf_len, &message->raddr);
}


KS_DECLARE(ks_status_t) ks_dht_setup_query(ks_dht_t *dht,
										   ks_dht_endpoint_t *ep,
										   ks_sockaddr_t *raddr,
										   const char *query,
										   ks_dht_message_callback_t callback,
										   ks_dht_transaction_t **transaction,
										   ks_dht_message_t **message,
										   struct bencode **args)
{
	uint32_t transactionid;
	ks_dht_transaction_t *trans = NULL;
	ks_dht_message_t *msg = NULL;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(query);
	ks_assert(callback);
	ks_assert(message);

	if (transaction) *transaction = NULL;
	*message = NULL;

	if (!ep && (ret = ks_dht_autoroute_check(dht, raddr, &ep)) != KS_STATUS_SUCCESS) return ret;

    // @todo atomic increment
	ks_mutex_lock(dht->tid_mutex);
	transactionid = dht->transactionid_next++;
	ks_mutex_unlock(dht->tid_mutex);

	if ((ret = ks_dht_transaction_create(&trans, dht->pool, raddr, transactionid, callback)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_message_create(&msg, dht->pool, ep, raddr, KS_TRUE)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_message_query(msg, transactionid, query, args)) != KS_STATUS_SUCCESS) goto done;

	*message = msg;

	if ((ret = ks_hash_insert(dht->transactions_hash, (void *)&trans->transactionid, trans)) != KS_STATUS_SUCCESS) goto done;

	if (transaction) *transaction = trans;

	ret = KS_STATUS_SUCCESS;

 done:
	if (ret != KS_STATUS_SUCCESS) {
		if (trans) ks_dht_transaction_destroy(&trans);
		if (msg) ks_dht_message_destroy(&msg);
		*message = NULL;
	}
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_setup_response(ks_dht_t *dht,
											  ks_dht_endpoint_t *ep,
											  ks_sockaddr_t *raddr,
											  uint8_t *transactionid,
											  ks_size_t transactionid_length,
											  ks_dht_message_t **message,
											  struct bencode **args)
{
	ks_dht_message_t *msg = NULL;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(transactionid);
	ks_assert(message);

	*message = NULL;

	if (!ep && (ret = ks_dht_autoroute_check(dht, raddr, &ep)) != KS_STATUS_SUCCESS) return ret;

	if ((ret = ks_dht_message_create(&msg, dht->pool, ep, raddr, KS_TRUE)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_message_response(msg, transactionid, transactionid_length, args)) != KS_STATUS_SUCCESS) goto done;

	*message = msg;

	ret = KS_STATUS_SUCCESS;

 done:
	if (ret != KS_STATUS_SUCCESS) {
		if (msg) ks_dht_message_destroy(&msg);
		*message = NULL;
	}
	return ret;
}


KS_DECLARE(void *) ks_dht_process(ks_thread_t *thread, void *data)
{
	ks_dht_datagram_t *datagram = (ks_dht_datagram_t *)data;
	ks_dht_message_t *message = NULL;
	ks_dht_message_callback_t callback;

	ks_assert(thread);
	ks_assert(data);

	ks_log(KS_LOG_DEBUG, "Received message from %s %d\n", datagram->raddr.host, datagram->raddr.port);
	if (datagram->raddr.family != AF_INET && datagram->raddr.family != AF_INET6) {
		ks_log(KS_LOG_DEBUG, "Message from unsupported address family\n");
		goto done;
	}

	// @todo blacklist check for bad actor nodes

	if (ks_dht_message_create(&message, datagram->dht->pool, datagram->endpoint, &datagram->raddr, KS_FALSE) != KS_STATUS_SUCCESS) goto done;

	if (ks_dht_message_parse(message, datagram->buffer, datagram->buffer_length) != KS_STATUS_SUCCESS) goto done;

	callback = (ks_dht_message_callback_t)(intptr_t)ks_hash_search(datagram->dht->registry_type, message->type, KS_READLOCKED);
	ks_hash_read_unlock(datagram->dht->registry_type);

	if (!callback) ks_log(KS_LOG_DEBUG, "Message type '%s' is not registered\n", message->type);
	else callback(datagram->dht, message);

 done:
	if (message) ks_dht_message_destroy(&message);
	if (datagram) ks_dht_datagram_destroy(&datagram);
	return NULL;
}


KS_DECLARE(ks_status_t) ks_dht_process_query(ks_dht_t *dht, ks_dht_message_t *message)
{
	struct bencode *q;
	struct bencode *a;
	const char *qv;
	ks_size_t qv_len;
	char query[KS_DHT_MESSAGE_QUERY_MAX_SIZE];
	ks_dht_message_callback_t callback;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(message);

	// @todo start of ks_dht_message_parse_query
    q = ben_dict_get_by_str(message->data, "q");
    if (!q) {
		ks_log(KS_LOG_DEBUG, "Message query missing required key 'q'\n");
		return KS_STATUS_FAIL;
	}

    qv = ben_str_val(q);
	qv_len = ben_str_len(q);
    if (qv_len >= KS_DHT_MESSAGE_QUERY_MAX_SIZE) {
		ks_log(KS_LOG_DEBUG, "Message query 'q' value has an unexpectedly large size of %d\n", qv_len);
		return KS_STATUS_FAIL;
	}

	memcpy(query, qv, qv_len);
	query[qv_len] = '\0';
	ks_log(KS_LOG_DEBUG, "Message query is '%s'\n", query);

	a = ben_dict_get_by_str(message->data, "a");
	if (!a) {
		ks_log(KS_LOG_DEBUG, "Message query missing required key 'a'\n");
		return KS_STATUS_FAIL;
	}
	// @todo end of ks_dht_message_parse_query

	message->args = a;

	callback = (ks_dht_message_callback_t)(intptr_t)ks_hash_search(dht->registry_query, query, KS_READLOCKED);
	ks_hash_read_unlock(dht->registry_query);

	if (!callback) ks_log(KS_LOG_DEBUG, "Message query '%s' is not registered\n", query);
	else ret = callback(dht, message);

	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_response(ks_dht_t *dht, ks_dht_message_t *message)
{
	struct bencode *r;
	ks_dht_transaction_t *transaction;
	uint32_t *tid;
	uint32_t transactionid;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(message);

	// @todo start of ks_dht_message_parse_response
	r = ben_dict_get_by_str(message->data, "r");
	if (!r) {
		ks_log(KS_LOG_DEBUG, "Message response missing required key 'r'\n");
		return KS_STATUS_FAIL;
	}
	// @todo end of ks_dht_message_parse_response

	message->args = r;

	tid = (uint32_t *)message->transactionid;
	transactionid = ntohl(*tid);

	transaction = ks_hash_search(dht->transactions_hash, (void *)&transactionid, KS_READLOCKED);
	ks_hash_read_unlock(dht->transactions_hash);

	if (!transaction) ks_log(KS_LOG_DEBUG, "Message response rejected with unknown transaction id %d\n", transactionid);
	else if (!ks_addr_cmp(&message->raddr, &transaction->raddr)) {
		ks_log(KS_LOG_DEBUG,
			   "Message response rejected due to spoofing from %s %d, expected %s %d\n",
			   message->raddr.host,
			   message->raddr.port,
			   transaction->raddr.host,
			   transaction->raddr.port);
	} else {
		message->transaction = transaction;
		ret = transaction->callback(dht, message);
		transaction->finished = KS_TRUE;
	}

	return ret;
}


KS_DECLARE(ks_status_t) ks_dht_search(ks_dht_t *dht,
									  int family,
									  ks_dht_nodeid_t *target,
									  ks_dht_search_callback_t callback,
									  ks_dht_search_t **search)
{
	ks_bool_t locked_search = KS_FALSE;
	ks_bool_t locked_pending = KS_FALSE;
	ks_dht_search_t *s = NULL;
	ks_bool_t inserted = KS_FALSE;
	ks_bool_t allocated = KS_FALSE;
    ks_dhtrt_querynodes_t query;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(family == AF_INET || family == AF_INET6);
	ks_assert(target);

	if (search) *search = NULL;

	// @todo start write lock on search_hash and hold until after inserting
	// check hash for target to see if search already exists
	ks_hash_write_lock(dht->search_hash);
	locked_search = KS_TRUE;

	s = ks_hash_search(dht->search_hash, target->id, KS_UNLOCKED);

	// if search does not exist, create new search and store in hash by target
	if (!s) {
		if ((ret = ks_dht_search_create(&s, dht->pool, target)) != KS_STATUS_SUCCESS) goto done;
		allocated = KS_TRUE;
	} else inserted = KS_TRUE;

	// add callback regardless of whether the search is new or old
	if ((ret = ks_dht_search_callback_add(s, callback)) != KS_STATUS_SUCCESS) goto done;

	// if the search is old then bail out and return successfully
	if (!allocated) goto done;

	if ((ret = ks_hash_insert(dht->search_hash, s->target.id, s)) == KS_STATUS_SUCCESS) goto done;
	inserted = KS_TRUE;

	// lock pending before unlocking the search hash to prevent this search from being used before we finish setting it up
	ks_hash_write_lock(s->pending);
	locked_pending = KS_TRUE;

	// release search hash lock now, but pending is still locked
	ks_hash_write_unlock(dht->search_hash);
	locked_search = KS_FALSE;

	// find closest good nodes to target locally and store as the closest results
    query.nodeid = *target;
	query.type = KS_DHT_REMOTE;
	query.max = KS_DHT_SEARCH_RESULTS_MAX_SIZE;
	query.family = family;
	query.count = 0;
	ks_dhtrt_findclosest_nodes(family == AF_INET ? dht->rt_ipv4 : dht->rt_ipv6, &query);
	for (int32_t i = 0; i < query.count; ++i) {
		ks_dht_node_t *n = query.nodes[i];
		ks_dht_search_pending_t *pending = NULL;
		
		s->results[i] = n->nodeid;
		ks_dht_utility_nodeid_xor(&s->distances[i], &n->nodeid, &s->target);
		// add to pending with expiration
		if ((ret = ks_dht_search_pending_create(&pending, s->pool, &n->nodeid)) != KS_STATUS_SUCCESS) goto done;
		if ((ret = ks_hash_insert(s->pending, n->nodeid.id, pending)) != KS_STATUS_SUCCESS) {
			ks_dht_search_pending_destroy(&pending);
			goto done;
		}
		if ((ret = ks_dht_send_findnode(dht, NULL, &n->addr, target)) != KS_STATUS_SUCCESS) goto done;
		// increment here in case we end up bailing out; execute with what it has or destroy the search?
		s->results_length++;
	}
	// @todo release query nodes
	ks_hash_write_unlock(s->pending);
	locked_pending = KS_FALSE;

	if (search) *search = s;

 done:
	if (locked_search) ks_hash_write_unlock(dht->search_hash);
	if (locked_pending) ks_hash_write_unlock(s->pending);
	if (ret != KS_STATUS_SUCCESS) {
		if (!inserted && s) ks_dht_search_destroy(&s);
		*search = NULL;
	}
	return ret;
}


KS_DECLARE(ks_status_t) ks_dht_send_error(ks_dht_t *dht,
										  ks_dht_endpoint_t *ep,
										  ks_sockaddr_t *raddr,
										  uint8_t *transactionid,
										  ks_size_t transactionid_length,
										  long long errorcode,
										  const char *errorstr)
{
	ks_dht_message_t *error = NULL;
	struct bencode *e = NULL;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(transactionid);
	ks_assert(errorstr);

	if (!ep && (ret = ks_dht_autoroute_check(dht, raddr, &ep)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_message_create(&error, dht->pool, ep, raddr, KS_TRUE)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_message_error(error, transactionid, transactionid_length, &e)) != KS_STATUS_SUCCESS) goto done;

	ben_list_append(e, ben_int(errorcode));
	ben_list_append(e, ben_blob(errorstr, strlen(errorstr)));

	ks_log(KS_LOG_DEBUG, "Sending message error %d\n", errorcode);
	ks_q_push(dht->send_q, (void *)error);

 done:
	if (ret != KS_STATUS_SUCCESS && error) ks_dht_message_destroy(&error);
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_error(ks_dht_t *dht, ks_dht_message_t *message)
{
	struct bencode *e;
	struct bencode *ec;
	struct bencode *es;
	const char *et;
	ks_size_t es_len;
	long long errorcode;
	char error[KS_DHT_MESSAGE_ERROR_MAX_SIZE];
	ks_dht_transaction_t *transaction;
	uint32_t *tid;
	uint32_t transactionid;
	ks_dht_message_callback_t callback;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);

	// @todo start of ks_dht_message_parse_error
	e = ben_dict_get_by_str(message->data, "e");
	if (!e) {
		ks_log(KS_LOG_DEBUG, "Message error missing required key 'e'\n");
		return KS_STATUS_FAIL;
	}
	ec = ben_list_get(e, 0);
	es = ben_list_get(e, 1);
	es_len = ben_str_len(es);
	if (es_len >= KS_DHT_MESSAGE_ERROR_MAX_SIZE) {
		ks_log(KS_LOG_DEBUG, "Message error value has an unexpectedly large size of %d\n", es_len);
		ret = KS_STATUS_FAIL;
		goto done;
	}
	errorcode = ben_int_val(ec);
	et = ben_str_val(es);

	memcpy(error, et, es_len);
	error[es_len] = '\0';
	// @todo end of ks_dht_message_parse_error

	message->args = e;

	tid = (uint32_t *)message->transactionid;
	transactionid = ntohl(*tid);

	transaction = ks_hash_search(dht->transactions_hash, (void *)&transactionid, KS_READLOCKED);
	ks_hash_read_unlock(dht->transactions_hash);

	if (!transaction) {
		ks_log(KS_LOG_DEBUG, "Message error rejected with unknown transaction id %d\n", transactionid);
		ret = KS_STATUS_FAIL;
		goto done;
	}

	if (!ks_addr_cmp(&message->raddr, &transaction->raddr)) {
		ks_log(KS_LOG_DEBUG,
			   "Message error rejected due to spoofing from %s %d, expected %s %d\n",
			   message->raddr.host,
			   message->raddr.port,
			   transaction->raddr.host,
			   transaction->raddr.port);
		ret = KS_STATUS_FAIL;
		goto done;
	}

	transaction->finished = KS_TRUE;

	callback = (ks_dht_message_callback_t)(intptr_t)ks_hash_search(dht->registry_error, error, KS_READLOCKED);
	ks_hash_read_unlock(dht->registry_error);

	if (callback) ret = callback(dht, message);
	else ks_log(KS_LOG_DEBUG, "Message error received for transaction id %d, error %d: %s\n", transactionid, errorcode, error);

 done:
	return ret;
}


KS_DECLARE(ks_status_t) ks_dht_send_ping(ks_dht_t *dht, ks_dht_endpoint_t *ep, ks_sockaddr_t *raddr)
{
	ks_dht_message_t *message = NULL;
	struct bencode *a = NULL;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(raddr);

	if ((ret = ks_dht_setup_query(dht,
								  ep,
								  raddr,
								  "ping",
								  ks_dht_process_response_ping,
								  NULL,
								  &message,
								  &a)) != KS_STATUS_SUCCESS) goto done;

	ben_dict_set(a, ben_blob("id", 2), ben_blob(message->endpoint->nodeid.id, KS_DHT_NODEID_SIZE));

	ks_log(KS_LOG_DEBUG, "Sending message query ping\n");
	ks_q_push(dht->send_q, (void *)message);

 done:
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_query_ping(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	ks_dht_message_t *response = NULL;
	struct bencode *r = NULL;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);
	ks_assert(message->args);

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Message query ping is valid\n");

	if ((ret = ks_dht_setup_response(dht,
									 message->endpoint,
									 &message->raddr,
									 message->transactionid,
									 message->transactionid_length,
									 &response,
									 &r)) != KS_STATUS_SUCCESS) goto done;

	ben_dict_set(r, ben_blob("id", 2), ben_blob(response->endpoint->nodeid.id, KS_DHT_NODEID_SIZE));

	ks_log(KS_LOG_DEBUG, "Sending message response ping\n");
	ks_q_push(dht->send_q, (void *)response);

 done:
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_response_ping(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;
	
	ks_log(KS_LOG_DEBUG, "Touching node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_touch_node(routetable, *id)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Message response ping is reached\n");

 done:
	return ret;
}


KS_DECLARE(ks_status_t) ks_dht_send_findnode(ks_dht_t *dht, ks_dht_endpoint_t *ep, ks_sockaddr_t *raddr, ks_dht_nodeid_t *targetid)
{
	ks_dht_transaction_t *transaction = NULL;
	ks_dht_message_t *message = NULL;
	struct bencode *a = NULL;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(targetid);

	if ((ret = ks_dht_setup_query(dht,
								  ep,
								  raddr,
								  "find_node",
								  ks_dht_process_response_findnode,
								  &transaction,
								  &message,
								  &a)) != KS_STATUS_SUCCESS) goto done;

	memcpy(transaction->target.id, targetid->id, KS_DHT_NODEID_SIZE);

	ben_dict_set(a, ben_blob("id", 2), ben_blob(message->endpoint->nodeid.id, KS_DHT_NODEID_SIZE));
	ben_dict_set(a, ben_blob("target", 6), ben_blob(targetid->id, KS_DHT_NODEID_SIZE));
	// @todo produce "want" value if both families are bound

	ks_log(KS_LOG_DEBUG, "Sending message query find_node\n");
	ks_q_push(dht->send_q, (void *)message);

 done:
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_query_findnode(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	ks_dht_nodeid_t *target;
	struct bencode *want;
	ks_bool_t want4 = KS_FALSE;
	ks_bool_t want6 = KS_FALSE;
	ks_dht_message_t *response = NULL;
	struct bencode *r = NULL;
	uint8_t buffer4[1000];
	uint8_t buffer6[1000];
	ks_size_t buffer4_length = 0;
	ks_size_t buffer6_length = 0;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	ks_dhtrt_querynodes_t query;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);
	ks_assert(message->args);

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "target", &target)) != KS_STATUS_SUCCESS) goto done;

	want = ben_dict_get_by_str(message->args, "want");
	if (want) {
		// @todo use ben_list_for_each
		size_t want_len = ben_list_len(want);
		for (size_t i = 0; i < want_len; ++i) {
			struct bencode *iv = ben_list_get(want, i);
			if (!ben_cmp_with_str(iv, "n4") && dht->rt_ipv4) want4 = KS_TRUE;
			if (!ben_cmp_with_str(iv, "n6") && dht->rt_ipv6) want6 = KS_TRUE;
		}
	}

	if (!want4 && !want6) {
		want4 = message->raddr.family == AF_INET;
		want6 = message->raddr.family == AF_INET6;
	}

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Message query find_node is valid\n");


	query.nodeid = *target;
	query.type = KS_DHT_REMOTE;
	query.max = 8; // should be like KS_DHTRT_BUCKET_SIZE
	if (want4) {
		query.family = AF_INET;
		ks_dhtrt_findclosest_nodes(routetable, &query);

		for (int32_t i = 0; i < query.count; ++i) {
			ks_dht_node_t *qn = query.nodes[i];

			if ((ret = ks_dht_utility_compact_nodeinfo(&qn->nodeid,
													   &qn->addr,
													   buffer4,
													   &buffer4_length,
													   sizeof(buffer4))) != KS_STATUS_SUCCESS) goto done;

			ks_log(KS_LOG_DEBUG, "Compacted ipv4 nodeinfo for %s (%s %d)\n", ks_dht_hexid(&qn->nodeid, id_buf), qn->addr.host, qn->addr.port);
		}
	}
	if (want6) {
		query.family = AF_INET6;
		ks_dhtrt_findclosest_nodes(routetable, &query);

		for (int32_t i = 0; i < query.count; ++i) {
			ks_dht_node_t *qn = query.nodes[i];

			if ((ret = ks_dht_utility_compact_nodeinfo(&qn->nodeid,
													   &qn->addr,
													   buffer6,
													   &buffer6_length,
													   sizeof(buffer6))) != KS_STATUS_SUCCESS) goto done;

			ks_log(KS_LOG_DEBUG, "Compacted ipv6 nodeinfo for %s (%s %d)\n", ks_dht_hexid(&qn->nodeid, id_buf), qn->addr.host, qn->addr.port);
		}
	}

	if ((ret = ks_dht_setup_response(dht,
									 message->endpoint,
									 &message->raddr,
									 message->transactionid,
									 message->transactionid_length,
									 &response,
									 &r)) != KS_STATUS_SUCCESS) goto done;

	ben_dict_set(r, ben_blob("id", 2), ben_blob(response->endpoint->nodeid.id, KS_DHT_NODEID_SIZE));
	if (want4) ben_dict_set(r, ben_blob("nodes", 5), ben_blob(buffer4, buffer4_length));
	if (want6) ben_dict_set(r, ben_blob("nodes6", 6), ben_blob(buffer6, buffer6_length));

	ks_log(KS_LOG_DEBUG, "Sending message response find_node\n");
	ks_q_push(dht->send_q, (void *)response);

 done:
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_response_findnode(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	struct bencode *n;
	const uint8_t *nodes = NULL;
	const uint8_t *nodes6 = NULL;
	size_t nodes_size = 0;
	size_t nodes6_size = 0;
	size_t nodes_len = 0;
	size_t nodes6_len = 0;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_dht_search_t *search = NULL;
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);
	ks_assert(message->transaction);

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	n = ben_dict_get_by_str(message->args, "nodes");
	if (n) {
		nodes = (const uint8_t *)ben_str_val(n);
		nodes_size = ben_str_len(n);
	}
	n = ben_dict_get_by_str(message->args, "nodes6");
	if (n) {
		nodes6 = (const uint8_t *)ben_str_val(n);
		nodes6_size = ben_str_len(n);
	}

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Touching node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_touch_node(routetable, *id)) != KS_STATUS_SUCCESS) goto done;

	ks_hash_read_lock(dht->search_hash);
	search = ks_hash_search(dht->search_hash, message->transaction->target.id, KS_UNLOCKED);
	ks_hash_read_unlock(dht->search_hash);
	if (search) {
		ks_dht_search_pending_t *pending = ks_hash_search(search->pending, id->id, KS_READLOCKED);
		ks_hash_read_unlock(search->pending);
		if (pending) pending->finished = KS_TRUE;
	}

	while (nodes_len < nodes_size) {
		ks_dht_nodeid_t nid;
		ks_sockaddr_t addr;

		addr.family = AF_INET;
		if ((ret = ks_dht_utility_expand_nodeinfo(nodes, &nodes_len, nodes_size, &nid, &addr)) != KS_STATUS_SUCCESS) goto done;

		ks_log(KS_LOG_DEBUG,
			   "Expanded ipv4 nodeinfo for %s (%s %d)\n",
			   ks_dht_hexid(&nid, id_buf),
			   addr.host,
			   addr.port);

		ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(&nid, id_buf));
		ks_dhtrt_create_node(dht->rt_ipv4, nid, KS_DHT_REMOTE, addr.host, addr.port, &node);
		ks_dhtrt_release_node(node);

		if (search) {
			ks_dht_nodeid_t distance;
			int32_t results_index = -1;
			
			ks_dht_utility_nodeid_xor(&distance, &nid, &search->target);
			if (search->results_length < KS_DHT_SEARCH_RESULTS_MAX_SIZE) {
				results_index = search->results_length;
				search->results_length++;
			} else {
				for (int32_t index = 0; index < search->results_length; ++index) {
					// Check if new node is closer than this existing result
					if (memcmp(distance.id, search->distances[index].id, KS_DHT_NODEID_SIZE) < 0) {
						// If this is the first node that is further then keep it
						// Else if two or more nodes are further, and this existing result is further than the previous one then keep it
						if (results_index < 0) results_index = index;
						else if (memcmp(search->distances[index].id, search->distances[results_index].id, KS_DHT_NODEID_SIZE) > 0) results_index = index;
					}
				}
			}

			if (results_index >= 0) {
				char id2_buf[KS_DHT_NODEID_SIZE * 2 + 1];
				char id3_buf[KS_DHT_NODEID_SIZE * 2 + 1];
				ks_dht_search_pending_t *pending = NULL;

				ks_log(KS_LOG_DEBUG,
					   "Set closer node id %s (%s) in search of target id %s at results index %d\n",
					   ks_dht_hexid(&nid, id_buf),
					   ks_dht_hexid(&distance, id2_buf),
					   ks_dht_hexid(&search->target, id3_buf),
					   results_index);
				search->results[results_index] = nid;
				search->distances[results_index] = distance;

				if ((ret = ks_dht_search_pending_create(&pending, search->pool, &nid)) != KS_STATUS_SUCCESS) goto done;
				if ((ret = ks_hash_insert(search->pending, nid.id, pending)) != KS_STATUS_SUCCESS) {
					ks_dht_search_pending_destroy(&pending);
					goto done;
				}
				if ((ret = ks_dht_send_findnode(dht, NULL, &addr, &search->target)) != KS_STATUS_SUCCESS) goto done;
			}
		}
	}

	while (nodes6_len < nodes6_size) {
		ks_dht_nodeid_t nid;
		ks_sockaddr_t addr;

		addr.family = AF_INET6;
		if ((ret = ks_dht_utility_expand_nodeinfo(nodes6, &nodes6_len, nodes6_size, &nid, &addr)) != KS_STATUS_SUCCESS) goto done;

		ks_log(KS_LOG_DEBUG,
			   "Expanded ipv6 nodeinfo for %s (%s %d)\n",
			   ks_dht_hexid(&nid, id_buf),
			   addr.host,
			   addr.port);

		ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(&nid, id_buf));
		ks_dhtrt_create_node(dht->rt_ipv6, nid, KS_DHT_REMOTE, addr.host, addr.port, &node);
		ks_dhtrt_release_node(node);
	}
	// @todo repeat above for ipv6 table

	ks_log(KS_LOG_DEBUG, "Message response find_node is reached\n");

 done:
	return ret;
}


KS_DECLARE(ks_status_t) ks_dht_send_get(ks_dht_t *dht, ks_dht_endpoint_t *ep, ks_sockaddr_t *raddr, ks_dht_nodeid_t *targetid)
{
	ks_dht_message_t *message = NULL;
	struct bencode *a = NULL;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(targetid);

	if (ks_dht_setup_query(dht,
						   ep,
						   raddr,
						   "get",
						   ks_dht_process_response_get,
						   NULL,
						   &message,
						   &a) != KS_STATUS_SUCCESS) return KS_STATUS_FAIL;

	ben_dict_set(a, ben_blob("id", 2), ben_blob(message->endpoint->nodeid.id, KS_DHT_NODEID_SIZE));
	// @todo check for target item locally, set seq to item seq to prevent getting back what we already have if a newer seq is not available
	ben_dict_set(a, ben_blob("target", 6), ben_blob(targetid->id, KS_DHT_NODEID_SIZE));

	ks_log(KS_LOG_DEBUG, "Sending message query get\n");
	ks_q_push(dht->send_q, (void *)message);

	return KS_STATUS_SUCCESS;
}

KS_DECLARE(ks_status_t) ks_dht_process_query_get(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	ks_dht_nodeid_t *target;
	struct bencode *seq;
	int64_t sequence = -1;
	ks_bool_t sequence_snuffed = KS_FALSE;
	ks_dht_token_t token;
	ks_dht_storageitem_t *item = NULL;
	ks_dht_message_t *response = NULL;
	struct bencode *r = NULL;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);
	ks_assert(message->args);

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "target", &target)) != KS_STATUS_SUCCESS) goto done;

	seq = ben_dict_get_by_str(message->args, "seq");
	if (seq) sequence = ben_int_val(seq);

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Message query get is valid\n");

	ks_dht_token_generate(dht->token_secret_current, &message->raddr, target, &token);

	item = ks_hash_search(dht->storage_hash, (void *)target, KS_READLOCKED);
	ks_hash_read_unlock(dht->storage_hash);

	sequence_snuffed = item && sequence >= 0 && item->seq <= sequence;
	// @todo if sequence is provided then requester has the data so if the local sequence is lower, maybe create job to update local data from the requester?

	// @todo find closest ipv4 and ipv6 nodes to target

	// @todo compact ipv4 and ipv6 nodes into separate buffers

	if ((ret = ks_dht_setup_response(dht,
									 message->endpoint,
									 &message->raddr,
									 message->transactionid,
									 message->transactionid_length,
									 &response,
									 &r)) != KS_STATUS_SUCCESS) goto done;

	ben_dict_set(r, ben_blob("id", 2), ben_blob(response->endpoint->nodeid.id, KS_DHT_NODEID_SIZE));
	ben_dict_set(r, ben_blob("token", 5), ben_blob(token.token, KS_DHT_TOKEN_SIZE));
	if (item) {
		if (item->mutable) {
			if (!sequence_snuffed) {
				ben_dict_set(r, ben_blob("k", 1), ben_blob(item->pk.key, KS_DHT_STORAGEITEM_KEY_SIZE));
				ben_dict_set(r, ben_blob("sig", 3), ben_blob(item->sig.sig, KS_DHT_STORAGEITEM_SIGNATURE_SIZE));
			}
			ben_dict_set(r, ben_blob("seq", 3), ben_int(item->seq));
		}
		if (!sequence_snuffed) ben_dict_set(r, ben_blob("v", 1), ben_clone(item->v));
	}
	// @todo nodes, nodes6

	ks_log(KS_LOG_DEBUG, "Sending message response get\n");
	ks_q_push(dht->send_q, (void *)response);

 done:
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_response_get(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	ks_dht_token_t *token;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);

	// @todo use ks_dht_storageitem_mutable or ks_dht_storageitem_immutable if v is provided
	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	if ((ret = ks_dht_utility_extract_token(message->args, "token", &token)) != KS_STATUS_SUCCESS) goto done;

	// @todo add extract function for mutable ks_dht_storageitem_key_t
	// @todo add extract function for mutable ks_dht_storageitem_signature_t

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Touching node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_touch_node(routetable, *id)) != KS_STATUS_SUCCESS) goto done;
	// @todo add/touch bucket entries for other nodes/nodes6 returned

	ks_log(KS_LOG_DEBUG, "Message response get is reached\n");

 done:
	return ret;
}


// @todo ks_dht_send_put

KS_DECLARE(ks_status_t) ks_dht_process_query_put(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	ks_dht_message_t *response = NULL;
	struct bencode *r = NULL;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);
	ks_assert(message->args);

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Message query put is valid\n");

	if ((ret = ks_dht_setup_response(dht,
									 message->endpoint,
									 &message->raddr,
									 message->transactionid,
									 message->transactionid_length,
									 &response,
									 &r)) != KS_STATUS_SUCCESS) goto done;

	//ben_dict_set(r, ben_blob("id", 2), ben_blob(response->endpoint->nodeid.id, KS_DHT_NODEID_SIZE));

	ks_log(KS_LOG_DEBUG, "Sending message response put\n");
	ks_q_push(dht->send_q, (void *)response);

 done:
	return ret;
}

KS_DECLARE(ks_status_t) ks_dht_process_response_put(ks_dht_t *dht, ks_dht_message_t *message)
{
	ks_dht_nodeid_t *id;
	ks_dhtrt_routetable_t *routetable = NULL;
	ks_dht_node_t *node = NULL;
	char id_buf[KS_DHT_NODEID_SIZE * 2 + 1];
	ks_status_t ret = KS_STATUS_SUCCESS;

	ks_assert(dht);
	ks_assert(message);

	if ((ret = ks_dht_utility_extract_nodeid(message->args, "id", &id)) != KS_STATUS_SUCCESS) goto done;

	routetable = message->endpoint->node->table;

	ks_log(KS_LOG_DEBUG, "Creating node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_create_node(routetable, *id, KS_DHT_REMOTE, message->raddr.host, message->raddr.port, &node)) != KS_STATUS_SUCCESS) goto done;
	if ((ret = ks_dhtrt_release_node(node)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Touching node %s\n", ks_dht_hexid(id, id_buf));
	if ((ret = ks_dhtrt_touch_node(routetable, *id)) != KS_STATUS_SUCCESS) goto done;

	ks_log(KS_LOG_DEBUG, "Message response put is reached\n");

 done:
	return ret;
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
