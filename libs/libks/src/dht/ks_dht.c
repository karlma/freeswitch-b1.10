#include "ks_dht.h"
#include "ks_dht-int.h"
#include "sodium.h"

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_alloc(ks_dht2_t **dht, ks_pool_t *pool)
{
	ks_bool_t pool_alloc = !pool;
	ks_dht2_t *d;

	ks_assert(dht);
	
	if (pool_alloc) ks_pool_open(&pool);
	*dht = d = ks_pool_alloc(pool, sizeof(ks_dht2_t));

	d->pool = pool;
	d->pool_alloc = pool_alloc;

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_prealloc(ks_dht2_t *dht, ks_pool_t *pool)
{
	ks_assert(dht);
	ks_assert(pool);

	dht->pool = pool;
	dht->pool_alloc = KS_FALSE;

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_free(ks_dht2_t *dht)
{
	ks_pool_t *pool = dht->pool;
	ks_bool_t pool_alloc = dht->pool_alloc;

	ks_dht2_deinit(dht);
	ks_pool_free(pool, dht);
	if (pool_alloc) {
		ks_pool_close(&pool);
	}

	return KS_STATUS_SUCCESS;
}
												

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_init(ks_dht2_t *dht, const ks_dht2_nodeid_raw_t *nodeid)
{
	ks_assert(dht);
	ks_assert(dht->pool);

	dht->autoroute = KS_FALSE;
	dht->autoroute_port = 0;
	
	if (ks_dht2_nodeid_prealloc(&dht->nodeid, dht->pool) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}
	
	if (ks_dht2_nodeid_init(&dht->nodeid, nodeid) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}

	ks_hash_create(&dht->registry_type, KS_HASH_MODE_DEFAULT, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK, dht->pool);
	ks_dht2_register_type(dht, "q", ks_dht2_process_query);
	ks_dht2_register_type(dht, "r", ks_dht2_process_response);
	// @todo ks_hash_insert the r/e callbacks into type registry

	ks_hash_create(&dht->registry_query, KS_HASH_MODE_DEFAULT, KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK, dht->pool);
	ks_dht2_register_query(dht, "ping", ks_dht2_process_query_ping);
	
    dht->bind_ipv4 = KS_FALSE;
	dht->bind_ipv6 = KS_FALSE;
	
	dht->endpoints = NULL;
	dht->endpoints_size = 0;
	ks_hash_create(&dht->endpoints_hash, KS_HASH_MODE_DEFAULT, KS_HASH_FLAG_RWLOCK, dht->pool);
	dht->endpoints_poll = NULL;
	
	dht->recv_buffer_length = 0;

	dht->transactionid_next = rand() % 0xFFFF;
	ks_hash_create(&dht->transactions_hash, KS_HASH_MODE_INT, KS_HASH_FLAG_RWLOCK, dht->pool);
	
	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_deinit(ks_dht2_t *dht)
{
	ks_assert(dht);

	dht->transactionid_next = 0;
	if (dht->transactions_hash) {
		ks_hash_destroy(&dht->transactions_hash);
		dht->transactions_hash = NULL;
	}
	dht->recv_buffer_length = 0;
	for (int32_t i = 0; i < dht->endpoints_size; ++i) {
		ks_dht2_endpoint_t *ep = dht->endpoints[i];
		//ks_hash_remove(dht->endpoints_hash, ep->addr.host);
		ks_dht2_endpoint_deinit(ep);
		ks_dht2_endpoint_free(ep);
	}
	dht->endpoints_size = 0;
	if (dht->endpoints) {
		ks_pool_free(dht->pool, dht->endpoints);
		dht->endpoints = NULL;
	}
	if (dht->endpoints_poll) {
		ks_pool_free(dht->pool, dht->endpoints_poll);
		dht->endpoints_poll = NULL;
	}
	if (dht->endpoints_hash) {
		ks_hash_destroy(&dht->endpoints_hash);
		dht->endpoints_hash = NULL;
	}
	dht->bind_ipv4 = KS_FALSE;
	dht->bind_ipv6 = KS_FALSE;

	if (dht->registry_type) {
		ks_hash_destroy(&dht->registry_type);
		dht->registry_type = NULL;
	}
	if (dht->registry_query) {
		ks_hash_destroy(&dht->registry_query);
		dht->registry_query = NULL;
	}

	ks_dht2_nodeid_deinit(&dht->nodeid);

	dht->autoroute = KS_FALSE;
	dht->autoroute_port = 0;
	
	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_autoroute(ks_dht2_t *dht, ks_bool_t autoroute, ks_port_t port)
{
	ks_assert(dht);

	if (!autoroute) {
		port = 0;
	} else if (port == 0) {
		return KS_STATUS_FAIL;
	}
	
	dht->autoroute = autoroute;
	dht->autoroute_port = port;
	
	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_register_type(ks_dht2_t *dht, const char *value, ks_dht2_message_callback_t callback)
{
	ks_assert(dht);
	ks_assert(value);
	ks_assert(callback);

	return ks_hash_insert(dht->registry_type, (void *)value, (void *)(intptr_t)callback) ? KS_STATUS_SUCCESS : KS_STATUS_FAIL;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_register_query(ks_dht2_t *dht, const char *value, ks_dht2_message_callback_t callback)
{
	ks_assert(dht);
	ks_assert(value);
	ks_assert(callback);

	return ks_hash_insert(dht->registry_query, (void *)value, (void *)(intptr_t)callback) ? KS_STATUS_SUCCESS : KS_STATUS_FAIL;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_bind(ks_dht2_t *dht, const ks_sockaddr_t *addr, ks_dht2_endpoint_t **endpoint)
{
	ks_dht2_endpoint_t *ep;
	ks_socket_t sock;
	int32_t epindex;
	
	ks_assert(dht);
	ks_assert(addr);
	ks_assert(addr->family == AF_INET || addr->family == AF_INET6);
	ks_assert(addr->port);

	if (endpoint) {
		*endpoint = NULL;
	}

	dht->bind_ipv4 |= addr->family == AF_INET;
	dht->bind_ipv6 |= addr->family == AF_INET6;

	// @todo start of ks_dht2_endpoint_bind
	if ((sock = socket(addr->family, SOCK_DGRAM, IPPROTO_UDP)) == KS_SOCK_INVALID) {
		return KS_STATUS_FAIL;
	}

	// @todo shouldn't ks_addr_bind take a const addr *?
	if (ks_addr_bind(sock, (ks_sockaddr_t *)addr) != KS_STATUS_SUCCESS) {
		ks_socket_close(&sock);
		return KS_STATUS_FAIL;
	}
	
	if (ks_dht2_endpoint_alloc(&ep, dht->pool) != KS_STATUS_SUCCESS) {
		ks_socket_close(&sock);
		return KS_STATUS_FAIL;
	}
	
	if (ks_dht2_endpoint_init(ep, addr, sock) != KS_STATUS_SUCCESS) {
		ks_dht2_endpoint_free(ep);
		ks_socket_close(&sock);
		return KS_STATUS_FAIL;
	}

	ks_socket_option(ep->sock, SO_REUSEADDR, KS_TRUE);
	ks_socket_option(ep->sock, KS_SO_NONBLOCK, KS_TRUE);

	// @todo end of ks_dht2_endpoint_bind
	
	epindex = dht->endpoints_size++;
	dht->endpoints = (ks_dht2_endpoint_t **)ks_pool_resize(dht->pool,
														   (void *)dht->endpoints,
														   sizeof(ks_dht2_endpoint_t *) * dht->endpoints_size);
	dht->endpoints[epindex] = ep;
	ks_hash_insert(dht->endpoints_hash, ep->addr.host, ep);
	
	dht->endpoints_poll = (struct pollfd *)ks_pool_resize(dht->pool,
														  (void *)dht->endpoints_poll,
														  sizeof(struct pollfd) * dht->endpoints_size);
	dht->endpoints_poll[epindex].fd = ep->sock;
	dht->endpoints_poll[epindex].events = POLLIN | POLLERR;

	if (endpoint) {
		*endpoint = ep;
	}

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_pulse(ks_dht2_t *dht, int32_t timeout)
{
	int32_t result;
		
	ks_assert(dht);
	ks_assert (timeout >= 0);

	// @todo why was old DHT code checking for poll descriptor resizing here?

	if (timeout == 0) {
		// @todo deal with default timeout, should return quickly but not hog the CPU polling
	}
	
	result = ks_poll(dht->endpoints_poll, dht->endpoints_size, timeout);
	if (result < 0) {
		return KS_STATUS_FAIL;
	}

	if (result == 0) {
		ks_dht2_idle(dht);
		return KS_STATUS_TIMEOUT;
	}

	for (int32_t i = 0; i < dht->endpoints_size; ++i) {
		if (dht->endpoints_poll[i].revents & POLLIN) {
			ks_sockaddr_t raddr = KS_SA_INIT;
			dht->recv_buffer_length = KS_DHT_RECV_BUFFER_SIZE;
			
			raddr.family = dht->endpoints[i]->addr.family;
			if (ks_socket_recvfrom(dht->endpoints_poll[i].fd, dht->recv_buffer, &dht->recv_buffer_length, &raddr) == KS_STATUS_SUCCESS) {
				ks_dht2_process(dht, &raddr);
			}
		}
	}

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_send(ks_dht2_t *dht, ks_sockaddr_t *raddr, ks_dht2_message_t *message)
{
	// @todo lookup standard def for IPV6 max size
	char ip[48];
	ks_dht2_endpoint_t *ep;
	// @todo calculate max IPV6 payload size?
	char buf[1000];
	ks_size_t buf_len;
	
	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(message);
	ks_assert(message->data);

	// @todo blacklist check

	ks_ip_route(ip, sizeof(ip), raddr->host);

	if (!(ep = ks_hash_search(dht->endpoints_hash, ip, KS_UNLOCKED)) && dht->autoroute) {
		ks_sockaddr_t addr;
		ks_addr_set(&addr, ip, dht->autoroute_port, raddr->family);
		if (ks_dht2_bind(dht, &addr, &ep) != KS_STATUS_SUCCESS) {
			return KS_STATUS_FAIL;
		}
	}

	if (!ep) {
		ks_log(KS_LOG_DEBUG, "No route available to %s\n", raddr->host);
		return KS_STATUS_FAIL;
	}

	buf_len = ben_encode2(buf, sizeof(buf), message->data);

	ks_log(KS_LOG_DEBUG, "Sending message to %s %d\n", raddr->host, raddr->port);
	ks_log(KS_LOG_DEBUG, "%s\n", ben_print(message->data));
	
	if (ks_socket_sendto(ep->sock, (void *)buf, &buf_len, raddr) != KS_STATUS_SUCCESS) {
		ks_log(KS_LOG_DEBUG, "Socket error\n");
		return KS_STATUS_FAIL;
	}

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_maketid(ks_dht2_t *dht)
{
	ks_assert(dht);

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_idle(ks_dht2_t *dht)
{
	ks_assert(dht);

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_process(ks_dht2_t *dht, ks_sockaddr_t *raddr)
{
	ks_dht2_message_t message;
	ks_dht2_message_callback_t callback;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(raddr);

	ks_log(KS_LOG_DEBUG, "Received message from %s %d\n", raddr->host, raddr->port);
	if (raddr->family != AF_INET && raddr->family != AF_INET6) {
		ks_log(KS_LOG_DEBUG, "Message from unsupported address family\n");
		return KS_STATUS_FAIL;
	}

	// @todo blacklist check for bad actor nodes
	
	if (ks_dht2_message_prealloc(&message, dht->pool) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}

	if (ks_dht2_message_init(&message, KS_FALSE) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}

	if (ks_dht2_message_parse(&message, dht->recv_buffer, dht->recv_buffer_length) != KS_STATUS_SUCCESS) {
		goto done;
	}
	
	if (!(callback = (ks_dht2_message_callback_t)(intptr_t)ks_hash_search(dht->registry_type, message.type, KS_UNLOCKED))) {
		ks_log(KS_LOG_DEBUG, "Message type '%s' is not registered\n", message.type);
	} else {
		ret = callback(dht, raddr, &message);
	}

 done:
	ks_dht2_message_deinit(&message);
	
	return ret;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_process_query(ks_dht2_t *dht, ks_sockaddr_t *raddr, ks_dht2_message_t *message)
{
	struct bencode *q;
	struct bencode *a;
	const char *qv;
	ks_size_t qv_len;
	char query[KS_DHT_MESSAGE_QUERY_MAX_SIZE];
	ks_dht2_message_callback_t callback;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(message);

	// @todo start of ks_dht2_message_parse_query
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
	// @todo end of ks_dht2_message_parse_query

	message->args = a;

	if (!(callback = (ks_dht2_message_callback_t)(intptr_t)ks_hash_search(dht->registry_query, query, KS_UNLOCKED))) {
		ks_log(KS_LOG_DEBUG, "Message query '%s' is not registered\n", query);
	} else {
		ret = callback(dht, raddr, message);
	}

	return ret;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_process_response(ks_dht2_t *dht, ks_sockaddr_t *raddr, ks_dht2_message_t *message)
{
	struct bencode *r;
	ks_dht2_transaction_t *transaction;
	uint32_t transactionid;
	uint16_t *tid;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(message);

	// @todo start of ks_dht2_message_parse_response
	r = ben_dict_get_by_str(message->data, "r");
	if (!r) {
		ks_log(KS_LOG_DEBUG, "Message response missing required key 'r'\n");
		return KS_STATUS_FAIL;
	}
	// todo end of ks_dht2_message_parse_response

	message->args = r;

	tid = (uint16_t *)message->transactionid;
	transactionid = ntohs(*tid);

	transaction = ks_hash_search(dht->transactions_hash, (void *)&transactionid, KS_READLOCKED);
	ks_hash_read_unlock(dht->transactions_hash);
	
	if (!transaction) {
		ks_log(KS_LOG_DEBUG, "Message response rejected with unknown transaction id %d\n", transactionid);
	} else {
		ret = transaction->callback(dht, raddr, message);
	}

	return ret;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_process_query_ping(ks_dht2_t *dht, ks_sockaddr_t *raddr, ks_dht2_message_t *message)
{
	struct bencode *id;
	const char *idv;
	ks_size_t idv_len;
	ks_dht2_nodeid_t nid;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(message);
	ks_assert(message->args);

    id = ben_dict_get_by_str(message->args, "id");
    if (!id) {
		ks_log(KS_LOG_DEBUG, "Message args missing required key 'id'\n");
		return KS_STATUS_FAIL;
	}
	
    idv = ben_str_val(id);
	idv_len = ben_str_len(id);
    if (idv_len != KS_DHT_NODEID_LENGTH) {
		ks_log(KS_LOG_DEBUG, "Message args 'id' value has an unexpected size of %d\n", idv_len);
		return KS_STATUS_FAIL;
	}

	if (ks_dht2_nodeid_prealloc(&nid, dht->pool) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}

	if (ks_dht2_nodeid_init(&nid, (const ks_dht2_nodeid_raw_t *)idv) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}

	//ks_log(KS_LOG_DEBUG, "Message query ping id is '%s'\n", id->id);
	ks_log(KS_LOG_DEBUG, "Message query ping is valid\n");

	ret = ks_dht2_send_response_ping(dht, raddr, message->transactionid, message->transactionid_length);

	ks_dht2_nodeid_deinit(&nid);

	return ret;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_process_response_ping(ks_dht2_t *dht, ks_sockaddr_t *raddr, ks_dht2_message_t *message)
{
	ks_assert(dht);
	ks_assert(raddr);
	ks_assert(message);

	return KS_STATUS_SUCCESS;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_send_query_ping(ks_dht2_t *dht, ks_sockaddr_t *raddr)
{
	uint32_t transactionid;
	ks_dht2_transaction_t *transaction = NULL;
	ks_dht2_message_t query;
	struct bencode *a;
	ks_status_t ret = KS_STATUS_FAIL;
	
	ks_assert(dht);
	ks_assert(raddr);

	// @todo atomic increment or mutex...
	transactionid = dht->transactionid_next++;

	if (ks_dht2_transaction_alloc(&transaction, dht->pool) != KS_STATUS_SUCCESS) {
		goto done;
	}

	if (ks_dht2_transaction_init(transaction, transactionid, ks_dht2_process_response_ping) != KS_STATUS_SUCCESS) {
		goto done;
	}

	if (ks_dht2_message_prealloc(&query, dht->pool) != KS_STATUS_SUCCESS) {
		goto done;
	}

	if (ks_dht2_message_init(&query, KS_TRUE) != KS_STATUS_SUCCESS) {
	    goto done;
	}

	if (ks_dht2_message_query(&query, transactionid, "ping", &a) != KS_STATUS_SUCCESS) {
		goto done;
	}

	// @todo transaction expiration and raddr

	// @todo transactions_hash mutex?
	ks_hash_insert(dht->transactions_hash, (void *)&transactionid, transaction);

	// @note a joins response.data and will be freed with it
	ben_dict_set(a, ben_blob("id", 2), ben_blob(dht->nodeid.id, KS_DHT_NODEID_LENGTH));

	ks_log(KS_LOG_DEBUG, "Sending message query ping\n");
	ret = ks_dht2_send(dht, raddr, &query);

 done:
	if (transaction && ret != KS_STATUS_SUCCESS) {
		ks_dht2_transaction_deinit(transaction);
		ks_dht2_transaction_free(transaction);
	}
	ks_dht2_message_deinit(&query);
	return ret;
}

/**
 *
 */
KS_DECLARE(ks_status_t) ks_dht2_send_response_ping(ks_dht2_t *dht,
												   ks_sockaddr_t *raddr,
												   uint8_t *transactionid,
												   ks_size_t transactionid_length)
{
	ks_dht2_message_t response;
	struct bencode *r;
	ks_status_t ret = KS_STATUS_FAIL;

	ks_assert(dht);
	ks_assert(raddr);

	if (ks_dht2_message_prealloc(&response, dht->pool) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}

	if (ks_dht2_message_init(&response, KS_TRUE) != KS_STATUS_SUCCESS) {
		return KS_STATUS_FAIL;
	}

	if (ks_dht2_message_response(&response, transactionid, transactionid_length, &r) != KS_STATUS_SUCCESS) {
		goto done;
	}

	// @note r joins response.data and will be freed with it
	ben_dict_set(r, ben_blob("id", 2), ben_blob(dht->nodeid.id, KS_DHT_NODEID_LENGTH));

	ks_log(KS_LOG_DEBUG, "Sending message response ping\n");
	ret = ks_dht2_send(dht, raddr, &response);

 done:
	ks_dht2_message_deinit(&response);
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
