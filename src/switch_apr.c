/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005/2006, Anthony Minessale II <anthmct@yahoo.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/F
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Michael Jerris <mike@jerris.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Michael Jerris <mike@jerris.com>
 *
 *
 * switch_apr.c -- apr wrappers and extensions
 *
 */

#include <switch.h>
#ifndef WIN32
#include <switch_private.h>
#endif

/* apr headers*/
#include <apr.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_network_io.h>
#include <apr_errno.h>
#include <apr_thread_proc.h>
#include <apr_portable.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_thread_rwlock.h>
#include <apr_file_io.h>
#include <apr_poll.h>
#include <apr_dso.h>
#include <apr_strings.h>
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>

/* apr_vformatter_buff_t definition*/
#include <apr_lib.h>

/* apr-util headers */
#include <apr_queue.h>
#include <apr_uuid.h>

/* apr stubs */

/* Memory Pools */

SWITCH_DECLARE(void) switch_pool_clear(switch_memory_pool_t *p)
{
	apr_pool_clear(p);
}

/* Hash tables */

SWITCH_DECLARE(switch_hash_index_t *) switch_hash_first(switch_memory_pool_t *p, switch_hash_t *ht)
{
	return apr_hash_first(p, ht);
}

SWITCH_DECLARE(switch_hash_index_t *) switch_hash_next(switch_hash_index_t *ht)
{
	return apr_hash_next(ht);
}

SWITCH_DECLARE(void) switch_hash_this(switch_hash_index_t *hi, const void **key, switch_ssize_t *klen, void **val)
{
	apr_hash_this(hi, key, klen, val);
}

SWITCH_DECLARE(unsigned int) switch_hashfunc_default(const char *key, switch_ssize_t *klen)
{
	return apr_hashfunc_default(key, klen);
}

SWITCH_DECLARE(switch_memory_pool_t *) switch_hash_pool_get(switch_hash_t *ht)
{
	return apr_hash_pool_get(ht);
}

/* DSO functions */

SWITCH_DECLARE(switch_status_t) switch_dso_load(switch_dso_handle_t **res_handle, 
                                       const char *path, switch_memory_pool_t *ctx)
{
	return apr_dso_load(res_handle, path, ctx);
}

SWITCH_DECLARE(switch_status_t) switch_dso_unload(switch_dso_handle_t *handle)
{
	return apr_dso_unload(handle);
}

SWITCH_DECLARE(switch_status_t) switch_dso_sym(switch_dso_handle_sym_t *ressym, 
                                      switch_dso_handle_t *handle,
                                      const char *symname)
{
	return apr_dso_sym(ressym, handle, symname);
}

SWITCH_DECLARE(const char *) switch_dso_error(switch_dso_handle_t *dso, char *buf, size_t bufsize)
{
	return apr_dso_error(dso, buf, bufsize);
}


/* string functions */

SWITCH_DECLARE(switch_status_t) switch_strftime(char *s,
												switch_size_t *retsize,
												switch_size_t max,
												const char *format,
												switch_time_exp_t *tm)
{
	return apr_strftime(s, retsize, max, format, (apr_time_exp_t *)tm);
}

SWITCH_DECLARE(int) switch_snprintf(char *buf, switch_size_t len, const char *format, ...)
{
    va_list ap;
	int ret;
	va_start(ap, format);
	ret = apr_vsnprintf(buf, len, format, ap);
    va_end(ap);
	return ret;
}

SWITCH_DECLARE(int) switch_vsnprintf(char *buf, switch_size_t len, const char *format, va_list ap)
{
	return apr_vsnprintf(buf, len, format, ap);
}

SWITCH_DECLARE(char *) switch_copy_string(char *dst, const char *src, switch_size_t dst_size)
{
	return apr_cpystrn(dst, src, dst_size);
}

/* thread read write lock functions */

SWITCH_DECLARE(switch_status_t) switch_thread_rwlock_create(switch_thread_rwlock_t **rwlock,
                                                  switch_memory_pool_t *pool)
{
	return apr_thread_rwlock_create(rwlock, pool);
}

SWITCH_DECLARE(switch_status_t) switch_thread_rwlock_destroy(switch_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_destroy(rwlock);
}

SWITCH_DECLARE(switch_memory_pool_t *) switch_thread_rwlock_pool_get(switch_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_pool_get(rwlock);
}

SWITCH_DECLARE(switch_status_t) switch_thread_rwlock_rdlock(switch_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_rdlock(rwlock);
}

SWITCH_DECLARE(switch_status_t) switch_thread_rwlock_tryrdlock(switch_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_tryrdlock(rwlock);
}

SWITCH_DECLARE(switch_status_t) switch_thread_rwlock_wrlock(switch_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_wrlock(rwlock);
}

SWITCH_DECLARE(switch_status_t) switch_thread_rwlock_trywrlock(switch_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_trywrlock(rwlock);
}

SWITCH_DECLARE(switch_status_t) switch_thread_rwlock_unlock(switch_thread_rwlock_t *rwlock)
{
	return apr_thread_rwlock_unlock(rwlock);
}

/* thread mutex functions */

SWITCH_DECLARE(switch_status_t) switch_mutex_init(switch_mutex_t **lock,
												unsigned int flags,
												switch_memory_pool_t *pool)
{
	return apr_thread_mutex_create(lock, flags, pool);
}

SWITCH_DECLARE(switch_status_t) switch_mutex_destroy(switch_mutex_t *lock)
{
	return apr_thread_mutex_destroy(lock);
}

SWITCH_DECLARE(switch_status_t) switch_mutex_lock(switch_mutex_t *lock)
{
	return apr_thread_mutex_lock(lock);
}

SWITCH_DECLARE(switch_status_t) switch_mutex_unlock(switch_mutex_t *lock)
{
	return apr_thread_mutex_unlock(lock);
}

SWITCH_DECLARE(switch_status_t) switch_mutex_trylock(switch_mutex_t *lock)
{
	return apr_thread_mutex_trylock(lock);
}

/* time function stubs */

SWITCH_DECLARE(switch_time_t) switch_time_now(void)
{
	return (switch_time_t)apr_time_now();
}

SWITCH_DECLARE(switch_status_t) switch_time_exp_gmt_get(switch_time_t *result, switch_time_exp_t *input)
{
	return apr_time_exp_gmt_get(result, (apr_time_exp_t *)input);
}

SWITCH_DECLARE(switch_status_t) switch_time_exp_get(switch_time_t *result, switch_time_exp_t *input)
{
	return apr_time_exp_get(result, (apr_time_exp_t *)input);
}

SWITCH_DECLARE(switch_status_t) switch_time_exp_lt(switch_time_exp_t *result, switch_time_t input)
{
	return apr_time_exp_lt((apr_time_exp_t *)result, input);
}

SWITCH_DECLARE(switch_status_t) switch_time_exp_gmt(switch_time_exp_t *result, switch_time_t input)
{
	return apr_time_exp_gmt((apr_time_exp_t *)result, input);
}

SWITCH_DECLARE(void) switch_sleep(switch_interval_time_t t)
{
#if defined(HAVE_USLEEP)
	usleep(t);
#elif defined(WIN32)
	Sleep((DWORD)((t) / 1000));
#else
	apr_sleep(t);
#endif
}


SWITCH_DECLARE(switch_status_t) switch_rfc822_date(char *date_str, switch_time_t t)
{
	return apr_rfc822_date(date_str, t);
}

SWITCH_DECLARE(switch_time_t) switch_time_make(switch_time_t sec, int32_t usec)
{
	return ((switch_time_t)(sec) * APR_USEC_PER_SEC + (switch_time_t)(usec));
}


/* Thread condition locks */

SWITCH_DECLARE(switch_status_t) switch_thread_cond_create(switch_thread_cond_t **cond, switch_memory_pool_t *pool)
{
	return apr_thread_cond_create(cond, pool);
}

SWITCH_DECLARE(switch_status_t) switch_thread_cond_wait(switch_thread_cond_t *cond, switch_mutex_t *mutex)
{
	return apr_thread_cond_wait(cond, mutex);
}

SWITCH_DECLARE(switch_status_t) switch_thread_cond_timedwait(switch_thread_cond_t *cond, switch_mutex_t *mutex, switch_interval_time_t timeout)
{
	return apr_thread_cond_timedwait(cond, mutex, timeout);
}

SWITCH_DECLARE(switch_status_t) switch_thread_cond_signal(switch_thread_cond_t *cond)
{
	return apr_thread_cond_signal(cond);
}

SWITCH_DECLARE(switch_status_t) switch_thread_cond_broadcast(switch_thread_cond_t *cond)
{
	return apr_thread_cond_broadcast(cond);
}

SWITCH_DECLARE(switch_status_t) switch_thread_cond_destroy(switch_thread_cond_t *cond)
{
	return apr_thread_cond_destroy(cond);
}

/* file i/o stubs */

SWITCH_DECLARE(switch_status_t) switch_file_open(switch_file_t **newf, const char *fname, int32_t flag, switch_fileperms_t perm, switch_memory_pool_t *pool)
{
	return apr_file_open(newf, fname, flag, perm, pool);
}

SWITCH_DECLARE(switch_status_t) switch_file_seek(switch_file_t *thefile, switch_seek_where_t where, int64_t *offset)
{
	apr_status_t rv;
	apr_off_t off = (apr_off_t)(*offset);
	rv = apr_file_seek(thefile, where, &off);
	*offset = (int64_t)off;
	return rv;
}

SWITCH_DECLARE(switch_status_t) switch_file_close(switch_file_t *thefile)
{
	return apr_file_close(thefile);
}

SWITCH_DECLARE(switch_status_t) switch_file_remove(const char *path, switch_memory_pool_t *pool)
{
	return apr_file_remove(path, pool);
}

SWITCH_DECLARE(switch_status_t) switch_file_read(switch_file_t *thefile, void *buf, switch_size_t *nbytes)
{
	return apr_file_read(thefile, buf, nbytes);
}

SWITCH_DECLARE(switch_status_t) switch_file_write(switch_file_t *thefile, const void *buf, switch_size_t *nbytes)
{
	return apr_file_write(thefile, buf, nbytes);
}

SWITCH_DECLARE(switch_status_t) switch_file_exists(const char *filename)
{
	int32_t wanted = APR_FINFO_TYPE;
	apr_finfo_t info = {0};
	if (filename) {
		apr_stat(&info, filename, wanted, NULL);
		if (info.filetype != APR_NOFILE) {
			return SWITCH_STATUS_SUCCESS;
		}
	}
	return SWITCH_STATUS_FALSE;
}

/* thread stubs */


SWITCH_DECLARE(switch_status_t) switch_threadattr_create(switch_threadattr_t **new_attr, switch_memory_pool_t *pool)
{
	return apr_threadattr_create(new_attr, pool);
}

SWITCH_DECLARE(switch_status_t) switch_threadattr_detach_set(switch_threadattr_t *attr, int32_t on)
{
	return apr_threadattr_detach_set(attr, on);
}

SWITCH_DECLARE(switch_status_t) switch_threadattr_stacksize_set(switch_threadattr_t *attr, switch_size_t stacksize)
{
	return apr_threadattr_stacksize_set(attr, stacksize);
}

SWITCH_DECLARE(switch_status_t) switch_thread_create(switch_thread_t **new_thread, switch_threadattr_t *attr, switch_thread_start_t func, void *data, switch_memory_pool_t *cont)
{
	return apr_thread_create(new_thread, attr, func, data, cont);
}

/* socket stubs */

SWITCH_DECLARE(switch_status_t) switch_socket_create(switch_socket_t **new_sock, int family, int type, int protocol, switch_memory_pool_t *pool)
{
	return apr_socket_create(new_sock, family, type, protocol, pool);
}

SWITCH_DECLARE(switch_status_t) switch_socket_shutdown(switch_socket_t *sock, switch_shutdown_how_e how)
{
	return apr_socket_shutdown(sock, how);
}

SWITCH_DECLARE(switch_status_t) switch_socket_close(switch_socket_t *sock)
{
	return apr_socket_close(sock);
}

SWITCH_DECLARE(switch_status_t) switch_socket_bind(switch_socket_t *sock, switch_sockaddr_t *sa)
{
	return apr_socket_bind(sock, sa);
}

SWITCH_DECLARE(switch_status_t) switch_socket_listen(switch_socket_t *sock, int32_t backlog)
{
	return apr_socket_listen(sock, backlog);
}

SWITCH_DECLARE(switch_status_t) switch_socket_accept(switch_socket_t **new_sock, switch_socket_t *sock, switch_memory_pool_t *pool)
{
	return apr_socket_accept(new_sock, sock, pool);
}

SWITCH_DECLARE(switch_status_t) switch_socket_connect(switch_socket_t *sock, switch_sockaddr_t *sa)
{
	return apr_socket_connect(sock, sa);
}


SWITCH_DECLARE(switch_status_t) switch_socket_send(switch_socket_t *sock, const char *buf, switch_size_t *len)
{
	return apr_socket_send(sock, buf, len);
}

SWITCH_DECLARE(switch_status_t) switch_socket_sendto(switch_socket_t *sock, switch_sockaddr_t *where, int32_t flags, const char *buf, switch_size_t *len)
{
	return apr_socket_sendto(sock, where, flags, buf, len);
}

SWITCH_DECLARE(switch_status_t) switch_socket_recv(switch_socket_t *sock, char *buf, switch_size_t *len)
{
	return apr_socket_recv(sock, buf, len);
}

SWITCH_DECLARE(switch_status_t) switch_sockaddr_info_get(switch_sockaddr_t **sa, const char *hostname, int32_t family, switch_port_t port, int32_t flags, switch_memory_pool_t *pool)
{
	return apr_sockaddr_info_get(sa, hostname, family, port, flags, pool);
}


SWITCH_DECLARE(switch_status_t) switch_socket_opt_set(switch_socket_t *sock, int32_t opt, int32_t on)
{
	return apr_socket_opt_set(sock, opt, on);
}

SWITCH_DECLARE(switch_status_t) switch_socket_timeout_set(switch_socket_t *sock, switch_interval_time_t t)
{
	return apr_socket_timeout_set(sock, t);
}

SWITCH_DECLARE(switch_status_t) switch_sockaddr_ip_get(char **addr, switch_sockaddr_t *sa)
{
	return apr_sockaddr_ip_get(addr, sa);
}

SWITCH_DECLARE(switch_status_t) switch_mcast_join(switch_socket_t *sock, switch_sockaddr_t *join, switch_sockaddr_t *iface, switch_sockaddr_t *source)
{
	return apr_mcast_join(sock, join, iface, source);
}


/* socket functions */

SWITCH_DECLARE(char *) switch_get_addr(char *buf, switch_size_t len, switch_sockaddr_t *in)
{
    return get_addr(buf, len, &in->sa.sin.sin_addr);
}


SWITCH_DECLARE(uint16_t) switch_sockaddr_get_port(switch_sockaddr_t *sa)
{
	return sa->port;
}

SWITCH_DECLARE(int32_t) switch_sockaddr_get_family(switch_sockaddr_t *sa)
{
	return sa->family;
}

SWITCH_DECLARE(switch_status_t) switch_socket_recvfrom(switch_sockaddr_t *from, switch_socket_t *sock,
                                                    int32_t flags, char *buf, 
                                                    size_t *len)
{
	apr_status_t r;

	if ((r = apr_socket_recvfrom(from, sock, flags, buf, len)) == APR_SUCCESS) {
		from->port = ntohs(from->sa.sin.sin_port);
		/* from->ipaddr_ptr = &(from->sa.sin.sin_addr);
		 * from->ipaddr_ptr = inet_ntoa(from->sa.sin.sin_addr);
		 */
	}

	return r;

}

/* poll stubs */

SWITCH_DECLARE(switch_status_t) switch_pollset_create(switch_pollset_t **pollset,
													  uint32_t size,
													  switch_memory_pool_t *p,
													  uint32_t flags)
{
	return apr_pollset_create(pollset, size, p, flags);
}

SWITCH_DECLARE(switch_status_t) switch_pollset_add(switch_pollset_t *pollset,
												   const switch_pollfd_t *descriptor)
{
	return apr_pollset_add(pollset, descriptor);
}

SWITCH_DECLARE(switch_status_t) switch_poll(switch_pollfd_t *aprset,
											int32_t numsock,
											int32_t *nsds,
											switch_interval_time_t timeout)
{
	return apr_poll(aprset, numsock, nsds, timeout);
}

SWITCH_DECLARE(switch_status_t) switch_socket_create_pollfd(switch_pollfd_t **poll, switch_socket_t *sock,
                                                            int16_t flags, switch_memory_pool_t *pool)
{
	switch_pollset_t *pollset;

	void *ptr = NULL;

	if ((ptr = apr_palloc(pool, sizeof(switch_pollfd_t))) == 0) {
		return SWITCH_STATUS_MEMERR;
	}

	if (switch_pollset_create(&pollset, 1, pool, flags) != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_GENERR;
	}

	memset(ptr, 0, sizeof(switch_pollfd_t));
	*poll = ptr;

	(*poll)->desc_type = APR_POLL_SOCKET;
	(*poll)->reqevents = flags;
	(*poll)->desc.s = sock;
	(*poll)->client_data = sock;

	if (switch_pollset_add(pollset, *poll) != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_GENERR;
	}

	return SWITCH_STATUS_SUCCESS;
}





/* apr-util stubs */

/* UUID Handling (apr-util) */


SWITCH_DECLARE(void) switch_uuid_format(char *buffer, const switch_uuid_t *uuid)
{
	apr_uuid_format(buffer, (const apr_uuid_t *)uuid);
}

SWITCH_DECLARE(void) switch_uuid_get(switch_uuid_t *uuid)
{
	apr_uuid_get((apr_uuid_t *)uuid);
}

SWITCH_DECLARE(switch_status_t) switch_uuid_parse(switch_uuid_t *uuid, const char *uuid_str)
{
	return apr_uuid_parse((apr_uuid_t *)uuid, uuid_str);
}


/* FIFO queues (apr-util) */

SWITCH_DECLARE(switch_status_t) switch_queue_create(switch_queue_t **queue,
                                           unsigned int queue_capacity, 
                                           switch_memory_pool_t *pool)
{
	return apr_queue_create(queue, queue_capacity, pool);
}

SWITCH_DECLARE(unsigned int) switch_queue_size(switch_queue_t *queue)
{
	return apr_queue_size(queue);
}

SWITCH_DECLARE(switch_status_t) switch_queue_pop(switch_queue_t *queue, void **data)
{
	return apr_queue_pop(queue, data);
}

SWITCH_DECLARE(switch_status_t) switch_queue_push(switch_queue_t *queue, void *data)
{
	return apr_queue_push(queue, data);
}

SWITCH_DECLARE(switch_status_t) switch_queue_trypop(switch_queue_t *queue, void **data)
{
	return apr_queue_trypop(queue, data);
}

SWITCH_DECLARE(switch_status_t) switch_queue_trypush(switch_queue_t *queue, void *data)
{
	return apr_queue_trypush(queue, data);
}

#if 0
/* Utility functions */
struct switch_vasprintf_data {
    apr_vformatter_buff_t vbuff;
	switch_size_t len;
	switch_size_t block_size;
	char *buf;
};

static int vasprintf_flush(apr_vformatter_buff_t *buff)
{
    struct switch_vasprintf_data *data = (struct switch_vasprintf_data *)buff;

	char *temp;
	switch_size_t len = data->vbuff.curpos - data->buf;

	if ((temp = realloc(data->buf, data->len + data->block_size))) {
		data->buf = temp;
		data->vbuff.curpos = data->buf + len;
		data->len = data->len + data->block_size;
		data->vbuff.endpos = data->buf + data->len;
		return 0;
	}
    	
	return -1;
}
#endif

SWITCH_DECLARE(int) switch_vasprintf(char **buf, const char *format, va_list ap)
{
#if 0
	struct switch_vasprintf_data data;

	data.block_size = 1024;
	data.buf = malloc(data.block_size);

	if (data.buf == NULL) {
		*buf = NULL;
        return 0;
    }

    data.vbuff.curpos = data.buf;
    data.vbuff.endpos = data.buf + data.block_size;

    return apr_vformatter(vasprintf_flush, (apr_vformatter_buff_t *)&data, format, ap);
#endif
#ifdef HAVE_VASPRINTF
    return vasprintf(buf, format, ap);
#else
    *buf = (char *) malloc(2048);
    return vsnprintf(*buf, 2048, format, ap);
#endif
}


/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */
