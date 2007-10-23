/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005/2006, Anthony Minessale II <anthmct@yahoo.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthmct@yahoo.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthmct@yahoo.com>
 * Michael Jerris <mike@jerris.com>
 * Paul D. Tinsley <pdt at jackhammer.org>
 *
 *
 * switch_core_memory.c -- Main Core Library (memory management)
 *
 */
#include <switch.h>
#include "private/switch_core_pvt.h"
/*#define LOCK_MORE*/

static struct {
	switch_mutex_t *mem_lock;
	switch_queue_t *pool_queue; /* 8 ball break */
	switch_queue_t *pool_recycle_queue;
	switch_memory_pool_t *memory_pool;
	int pool_thread_running;
} memory_manager;

SWITCH_DECLARE(switch_memory_pool_t *) switch_core_session_get_pool(switch_core_session_t *session)
{
	assert(session != NULL);
	assert(session->pool != NULL);
	return session->pool;
}

/* **ONLY** alloc things with this function that **WILL NOT** outlive
   the session itself or expect an earth shattering KABOOM!*/
SWITCH_DECLARE(void *) switch_core_session_alloc(switch_core_session_t *session, switch_size_t memory)
{
	void *ptr = NULL;
	assert(session != NULL);
	assert(session->pool != NULL);

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

#ifdef DEBUG_ALLOC
	printf("Allocate %d\n", (int)memory);
#endif

	ptr = apr_palloc(session->pool, memory);
	assert(ptr != NULL);

	memset(ptr, 0, memory);
	
#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return ptr;
}

/* **ONLY** alloc things with these functions that **WILL NOT** need
   to be freed *EVER* ie this is for *PERMANENT* memory allocation */

SWITCH_DECLARE(void *) switch_core_permanent_alloc(switch_size_t memory)
{
	void *ptr = NULL;
	assert(memory_manager.memory_pool != NULL);

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

#ifdef DEBUG_ALLOC
	printf("Perm Allocate %d\n", (int)memory);
#endif

	ptr = apr_palloc(memory_manager.memory_pool, memory);

	assert(ptr != NULL);
	memset(ptr, 0, memory);

#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return ptr;
}

SWITCH_DECLARE(char *) switch_core_permanent_strdup(const char *todup)
{
	char *duped = NULL;
	switch_size_t len;
	assert(memory_manager.memory_pool != NULL);

	if (!todup)
		return NULL;

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

	len = strlen(todup) + 1;
	duped = apr_pstrmemdup(memory_manager.memory_pool, todup, len);
	assert(duped != NULL);

#ifdef DEBUG_ALLOC
	printf("Perm Allocate %d\n", (int)len);
#endif

#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return duped;
}

SWITCH_DECLARE(char *) switch_core_session_sprintf(switch_core_session_t *session, const char *fmt, ...)
{
	va_list ap;
	char *result = NULL;

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

	assert(session != NULL);
	assert(session->pool != NULL);
	va_start(ap, fmt);
	
	result = apr_pvsprintf(session->pool, fmt, ap);
	assert(result != NULL);
	va_end(ap);


#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return result;
}

SWITCH_DECLARE(char *) switch_core_sprintf(switch_memory_pool_t *pool, const char *fmt, ...)
{
	va_list ap;
	char *result = NULL;

	assert(pool != NULL);

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

	va_start(ap, fmt);

	result = apr_pvsprintf(pool, fmt, ap);
	assert(result != NULL);
	va_end(ap);

#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return result;
}


SWITCH_DECLARE(char *) switch_core_session_strdup(switch_core_session_t *session, const char *todup)
{
	char *duped = NULL;
	switch_size_t len;
	assert(session != NULL);
	assert(session->pool != NULL);

	if (!todup) {
		return NULL;
	}

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

	len = strlen(todup) + 1;

#ifdef DEBUG_ALLOC
	printf("Allocate %d\n", (int)len);
#endif

	duped = apr_pstrmemdup(session->pool, todup, len);
	assert(duped != NULL);


#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return duped;
}


SWITCH_DECLARE(char *) switch_core_strdup(switch_memory_pool_t *pool, const char *todup)
{
	char *duped = NULL;
	switch_size_t len;
	assert(pool != NULL);

	if (!todup) {
		return NULL;
	}

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

	len = strlen(todup) + 1;

#ifdef DEBUG_ALLOC
	printf("Allocate %d\n", (int)len);
#endif

	duped = apr_pstrmemdup(pool, todup, len);
	assert(duped != NULL);

#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return duped;
}

SWITCH_DECLARE(switch_status_t) switch_core_perform_new_memory_pool(switch_memory_pool_t **pool, const char *file, const char *func, int line)
{
	char *tmp;
	void *pop;

	switch_mutex_lock(memory_manager.mem_lock);
	assert(pool != NULL);

	if (switch_queue_trypop(memory_manager.pool_recycle_queue, &pop) == SWITCH_STATUS_SUCCESS) {
		*pool = (switch_memory_pool_t *) pop;
	} else {
		apr_pool_create(pool, NULL);
		assert(*pool != NULL);
	}

#ifdef DEBUG_ALLOC2
	printf("New Pool %s %s:%d\n", file, func, line);
#endif
	tmp = switch_core_sprintf(*pool, "%s:%d", func, line);
	apr_pool_tag(*pool, tmp);
	switch_mutex_unlock(memory_manager.mem_lock);
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_core_perform_destroy_memory_pool(switch_memory_pool_t **pool, const char *file, const char *func, int line)
{
	//char tmp[128] = "";


	assert(pool != NULL);

#ifdef DEBUG_ALLOC2
	printf("Free Pool %s %s:%d\n", file, func, line);
#endif

	if (switch_queue_push(memory_manager.pool_queue, *pool) != SWITCH_STATUS_SUCCESS) {
		apr_pool_destroy(*pool);
	}
	*pool = NULL;


	return SWITCH_STATUS_SUCCESS;
}


SWITCH_DECLARE(void *) switch_core_alloc(switch_memory_pool_t *pool, switch_size_t memory)
{
	void *ptr = NULL;

	assert(pool != NULL);

#ifdef LOCK_MORE
	switch_mutex_lock(memory_manager.mem_lock);
#endif

#ifdef DEBUG_ALLOC
	printf("Allocate %d\n", (int)memory);
	/*assert(memory < 20000);*/
#endif

	ptr = apr_palloc(pool, memory);
	assert(ptr != NULL);
	memset(ptr, 0, memory);
	

#ifdef LOCK_MORE
	switch_mutex_unlock(memory_manager.mem_lock);
#endif

	return ptr;
}


SWITCH_DECLARE(void) switch_core_memory_reclaim(void)
{
	switch_memory_pool_t *pool;
	void *pop = NULL;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Returning %d recycled memory pool(s)\n", 
					  switch_queue_size(memory_manager.pool_recycle_queue) + switch_queue_size(memory_manager.pool_queue));
	
	while (switch_queue_trypop(memory_manager.pool_recycle_queue, &pop) == SWITCH_STATUS_SUCCESS) {
		pool = (switch_memory_pool_t *) pop;
		if (!pool) {
			break;
		}
		apr_pool_destroy(pool);
	}
}

static void *SWITCH_THREAD_FUNC pool_thread(switch_thread_t * thread, void *obj)
{
	void *pop = NULL;
	switch_memory_pool_t *pool;

	memory_manager.pool_thread_running = 1;

	while (memory_manager.pool_thread_running == 1) {
		int len = switch_queue_size(memory_manager.pool_queue);

		if (len) {
			int x = len, done = 0;

			switch_yield(1000000);
			switch_mutex_lock(memory_manager.mem_lock);
			while (x > 0) {
				if (switch_queue_pop(memory_manager.pool_queue, &pop) != SWITCH_STATUS_SUCCESS) {
					done = 1;
					break;
				}
			
				if (!pop) {
					done = 1;
					break;
				}
				
				pool = (switch_memory_pool_t *) pop;
				apr_pool_clear(pool);
				if (switch_queue_trypush(memory_manager.pool_recycle_queue, pool) != SWITCH_STATUS_SUCCESS) {
					apr_pool_destroy(pool);
				}
				pool = NULL;
				x--;
			}
			switch_mutex_unlock(memory_manager.mem_lock);
			if (done) {
				goto done;
			}
		} else {
			switch_yield(100000);
		}
	}

 done:
	switch_core_memory_reclaim();

	while (switch_queue_trypop(memory_manager.pool_queue, &pop) == SWITCH_STATUS_SUCCESS) {
		pool = (switch_memory_pool_t *) pop;
		if (!pool) {
			break;
		}
		apr_pool_destroy(pool);
	}

	memory_manager.pool_thread_running = 0;
	return NULL;
}


void switch_core_memory_stop(void)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Stopping memory pool queue.\n");
	memory_manager.pool_thread_running = -1;
	while(memory_manager.pool_thread_running) {
		switch_yield(1000);
	}
}

switch_memory_pool_t *switch_core_memory_init(void)
{
	switch_thread_t *thread;
    switch_threadattr_t *thd_attr;


	memset(&memory_manager, 0, sizeof(memory_manager));

	apr_pool_create(&memory_manager.memory_pool, NULL);
	assert(memory_manager.memory_pool != NULL);
	switch_mutex_init(&memory_manager.mem_lock, SWITCH_MUTEX_NESTED, memory_manager.memory_pool);
	switch_queue_create(&memory_manager.pool_queue, 50000, memory_manager.memory_pool);
	switch_queue_create(&memory_manager.pool_recycle_queue, 50000, memory_manager.memory_pool);

	switch_threadattr_create(&thd_attr, memory_manager.memory_pool);
	switch_threadattr_detach_set(thd_attr, 1);

	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, pool_thread, NULL, memory_manager.memory_pool);
	
	while (!memory_manager.pool_thread_running) {
		switch_yield(1000);
	}

	return memory_manager.memory_pool;
}
