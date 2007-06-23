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
 * switch_event.c -- Event System
 *
 */
#include <switch.h>
#include <switch_event.h>

static switch_event_node_t *EVENT_NODES[SWITCH_EVENT_ALL + 1] = { NULL };
static switch_mutex_t *BLOCK = NULL;
static switch_mutex_t *POOL_LOCK = NULL;
static switch_mutex_t *EVENT_QUEUE_MUTEX = NULL;
static switch_mutex_t *EVENT_QUEUE_HAVEMORE_MUTEX = NULL;
static switch_thread_cond_t *EVENT_QUEUE_CONDITIONAL = NULL;
static switch_memory_pool_t *RUNTIME_POOL = NULL;
/* static switch_memory_pool_t *APOOL = NULL; */
/* static switch_memory_pool_t *BPOOL = NULL; */
static switch_memory_pool_t *THRUNTIME_POOL = NULL;
static switch_queue_t *EVENT_QUEUE[3] = { 0, 0, 0 };
static int POOL_COUNT_MAX = SWITCH_CORE_QUEUE_LEN;

static switch_hash_t *CUSTOM_HASH = NULL;
static int THREAD_RUNNING = 0;
static int EVENT_QUEUE_HAVEMORE = 0;

#if 0
static void *locked_alloc(switch_size_t len)
{
	void *mem;

	switch_mutex_lock(POOL_LOCK);
	/* <LOCKED> ----------------------------------------------- */
	mem = switch_core_alloc(THRUNTIME_POOL, len);
	switch_mutex_unlock(POOL_LOCK);
	/* </LOCKED> ---------------------------------------------- */

	return mem;
}

static void *locked_dup(char *str)
{
	char *dup;

	switch_mutex_lock(POOL_LOCK);
	/* <LOCKED> ----------------------------------------------- */
	dup = switch_core_strdup(THRUNTIME_POOL, str);
	switch_mutex_unlock(POOL_LOCK);
	/* </LOCKED> ---------------------------------------------- */

	return dup;
}

#define ALLOC(size) locked_alloc(size)
#define DUP(str) locked_dup(str)
#endif

#ifndef ALLOC
#define ALLOC(size) malloc(size)
#endif
#ifndef DUP
#define DUP(str) strdup(str)
#endif
#ifndef FREE
#define FREE(ptr) if (ptr) free(ptr)
#endif

/* make sure this is synced with the switch_event_types_t enum in switch_types.h
also never put any new ones before EVENT_ALL
*/
static char *EVENT_NAMES[] = {
	"CUSTOM",
	"CHANNEL_CREATE",
	"CHANNEL_DESTROY",
	"CHANNEL_STATE",
	"CHANNEL_ANSWER",
	"CHANNEL_HANGUP",
	"CHANNEL_EXECUTE",
	"CHANNEL_EXECUTE_COMPLETE",
	"CHANNEL_BRIDGE",
	"CHANNEL_UNBRIDGE",
	"CHANNEL_PROGRESS",
	"CHANNEL_OUTGOING",
	"CHANNEL_PARK",
	"CHANNEL_UNPARK",
	"CHANNEL_APPLICATION",
	"API",
	"LOG",
	"INBOUND_CHAN",
	"OUTBOUND_CHAN",
	"STARTUP",
	"SHUTDOWN",
	"PUBLISH",
	"UNPUBLISH",
	"TALK",
	"NOTALK",
	"SESSION_CRASH",
	"MODULE_LOAD",
	"MODULE_UNLOAD",
	"DTMF",
	"MESSAGE",
	"PRESENCE_IN",
	"PRESENCE_OUT",
	"PRESENCE_PROBE",
	"MESSAGE_WAITING",
	"MESSAGE_QUERY",
	"ROSTER",
	"CODEC",
	"BACKGROUND_JOB",
	"DETECTED_SPEECH",
	"DETECTED_TONE",
	"PRIVATE_COMMAND",
	"HEARTBEAT",
	"TRAP",
	"ADD_SCHEDULE",
	"DEL_SCHEDULE",
	"EXE_SCHEDULE",
	"RE_SCHEDULE",
	"ALL"
};


static int switch_events_match(switch_event_t *event, switch_event_node_t *node)
{
	int match = 0;


	if (node->event_id == SWITCH_EVENT_ALL) {
		match++;

		if (!node->subclass) {
			return match;
		}
	}

	if (match || event->event_id == node->event_id) {

		if (event->subclass && node->subclass) {
			if (!strncasecmp(node->subclass->name, "file:", 5)) {
				char *file_header;
				if ((file_header = switch_event_get_header(event, "file")) != 0) {
					match = strstr(node->subclass->name + 5, file_header) ? 1 : 0;
				}
			} else if (!strncasecmp(node->subclass->name, "func:", 5)) {
				char *func_header;
				if ((func_header = switch_event_get_header(event, "function")) != 0) {
					match = strstr(node->subclass->name + 5, func_header) ? 1 : 0;
				}
			} else {
				match = strstr(event->subclass->name, node->subclass->name) ? 1 : 0;
			}
		} else if ((event->subclass && !node->subclass) || (!event->subclass && !node->subclass)) {
			match = 1;
		} else {
			match = 0;
		}
	}

	return match;
}

static void *SWITCH_THREAD_FUNC switch_event_thread(switch_thread_t * thread, void *obj)
{
	switch_event_t *out_event = NULL;
	switch_queue_t *queue = NULL;
	switch_queue_t *queues[3] = { 0, 0, 0 };
	void *pop;
	int i, len[3] = { 0, 0, 0 };

	assert(thread != NULL);
	assert(obj == NULL);
	assert(POOL_LOCK != NULL);
	assert(RUNTIME_POOL != NULL);
	assert(EVENT_QUEUE_MUTEX != NULL);
	assert(EVENT_QUEUE_HAVEMORE_MUTEX != NULL);
	assert(EVENT_QUEUE_CONDITIONAL != NULL);
	THREAD_RUNNING = 1;

	queues[0] = EVENT_QUEUE[SWITCH_PRIORITY_HIGH];
	queues[1] = EVENT_QUEUE[SWITCH_PRIORITY_NORMAL];
	queues[2] = EVENT_QUEUE[SWITCH_PRIORITY_LOW];

	switch_mutex_lock(EVENT_QUEUE_MUTEX);

	for (;;) {
		int any;


		len[1] = switch_queue_size(EVENT_QUEUE[SWITCH_PRIORITY_NORMAL]);
		len[2] = switch_queue_size(EVENT_QUEUE[SWITCH_PRIORITY_LOW]);
		len[0] = switch_queue_size(EVENT_QUEUE[SWITCH_PRIORITY_HIGH]);
		any = len[1] + len[2] + len[0];


		if (!any) {
			/* lock on havemore so we are the only ones poking at it while we check it
			 * see if we saw anything in the queues or have a check again flag
			 */
			switch_mutex_lock(EVENT_QUEUE_HAVEMORE_MUTEX);
			if (!EVENT_QUEUE_HAVEMORE) {
				/* See if we need to quit */
				if (THREAD_RUNNING != 1) {
					/* give up our lock */
					switch_mutex_unlock(EVENT_QUEUE_HAVEMORE_MUTEX);

					/* game over */
					break;
				}

				/* give up our lock */
				switch_mutex_unlock(EVENT_QUEUE_HAVEMORE_MUTEX);

				/* wait until someone tells us we have something to do */
				switch_thread_cond_wait(EVENT_QUEUE_CONDITIONAL, EVENT_QUEUE_MUTEX);
			} else {
				/* Caught a race, one of the queues was updated after we looked at it 
				 * reset our flag
				 */
				EVENT_QUEUE_HAVEMORE = 0;

				/* give up our lock */
				switch_mutex_unlock(EVENT_QUEUE_HAVEMORE_MUTEX);
			}

			/* go grab some events */
			continue;
		}

		for (i = 0; i < 3; i++) {
			if (len[i]) {
				queue = queues[i];
				while (queue) {
					if (switch_queue_trypop(queue, &pop) == SWITCH_STATUS_SUCCESS) {
						out_event = pop;
						switch_event_deliver(&out_event);
					} else {
						break;
					}
				}
			}
		}

		if (THREAD_RUNNING < 0) {
			THREAD_RUNNING--;
		}
	}


	THREAD_RUNNING = 0;
	return NULL;
}

SWITCH_DECLARE(void) switch_event_deliver(switch_event_t **event)
{
	switch_event_types_t e;
	switch_event_node_t *node;

	for (e = (*event)->event_id;; e = SWITCH_EVENT_ALL) {
		for (node = EVENT_NODES[e]; node; node = node->next) {
			if (switch_events_match(*event, node)) {
				(*event)->bind_user_data = node->user_data;
				node->callback(*event);
			}
		}

		if (e == SWITCH_EVENT_ALL) {
			break;
		}
	}

	switch_event_destroy(event);
}


SWITCH_DECLARE(switch_status_t) switch_event_running(void)
{
	return THREAD_RUNNING ? SWITCH_STATUS_SUCCESS : SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(char *) switch_event_name(switch_event_types_t event)
{
	assert(BLOCK != NULL);
	assert(RUNTIME_POOL != NULL);

	return EVENT_NAMES[event];
}

SWITCH_DECLARE(switch_status_t) switch_name_event(char *name, switch_event_types_t *type)
{
	switch_event_types_t x;
	assert(BLOCK != NULL);
	assert(RUNTIME_POOL != NULL);

	for (x = 0; x <= SWITCH_EVENT_ALL; x++) {
		if ((strlen(name) > 13 && !strcasecmp(name + 13, EVENT_NAMES[x])) || !strcasecmp(name, EVENT_NAMES[x])) {
			*type = x;
			return SWITCH_STATUS_SUCCESS;
		}
	}

	return SWITCH_STATUS_FALSE;

}

SWITCH_DECLARE(switch_status_t) switch_event_reserve_subclass_detailed(char *owner, char *subclass_name)
{

	switch_event_subclass_t *subclass;

	assert(RUNTIME_POOL != NULL);
	assert(CUSTOM_HASH != NULL);

	if (switch_core_hash_find(CUSTOM_HASH, subclass_name)) {
		return SWITCH_STATUS_INUSE;
	}

	if ((subclass = switch_core_alloc(RUNTIME_POOL, sizeof(*subclass))) == 0) {
		return SWITCH_STATUS_MEMERR;
	}

	subclass->owner = switch_core_strdup(RUNTIME_POOL, owner);
	subclass->name = switch_core_strdup(RUNTIME_POOL, subclass_name);

	switch_core_hash_insert(CUSTOM_HASH, subclass->name, subclass);

	return SWITCH_STATUS_SUCCESS;

}

SWITCH_DECLARE(switch_status_t) switch_event_shutdown(void)
{
	int x = 0, last = 0;

	if (THREAD_RUNNING > 0) {
		THREAD_RUNNING = -1;

		/* lock on havemore to make sure he event thread, if currently running
		 * doesn't check the HAVEMORE flag before we set it
		 */
		switch_mutex_lock(EVENT_QUEUE_HAVEMORE_MUTEX);
		/* see if the event thread is sitting */
		if (switch_mutex_trylock(EVENT_QUEUE_MUTEX) == SWITCH_STATUS_SUCCESS) {
			/* we don't need havemore anymore, the thread was sitting already */
			switch_mutex_unlock(EVENT_QUEUE_HAVEMORE_MUTEX);

			/* wake up the event thread */
			switch_thread_cond_signal(EVENT_QUEUE_CONDITIONAL);

			/* give up our lock */
			switch_mutex_unlock(EVENT_QUEUE_MUTEX);
		} else {
			/* it wasn't waiting which means we might have updated a queue it already looked at
			 * set a flag so it knows to read the queues again
			 */
			EVENT_QUEUE_HAVEMORE = 1;

			/* variable updated, give up the mutex */
			switch_mutex_unlock(EVENT_QUEUE_HAVEMORE_MUTEX);
		}

		while (x < 100 && THREAD_RUNNING) {
			switch_yield(1000);
			if (THREAD_RUNNING == last) {
				x++;
			}
			last = THREAD_RUNNING;
		}
	}
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_event_init(switch_memory_pool_t *pool)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr;;
	switch_threadattr_create(&thd_attr, pool);
	switch_threadattr_detach_set(thd_attr, 1);

	assert(pool != NULL);
	THRUNTIME_POOL = RUNTIME_POOL = pool;

	/*
	if (switch_core_new_memory_pool(&THRUNTIME_POOL) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Could not allocate memory pool\n");
		return SWITCH_STATUS_MEMERR;
	}
	*/
	/*
	   if (switch_core_new_memory_pool(&BPOOL) != SWITCH_STATUS_SUCCESS) {
	   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Could not allocate memory pool\n");
	   return SWITCH_STATUS_MEMERR;
	   }
	 */
	/* THRUNTIME_POOL = APOOL; */
	switch_queue_create(&EVENT_QUEUE[0], POOL_COUNT_MAX + 10, THRUNTIME_POOL);
	switch_queue_create(&EVENT_QUEUE[1], POOL_COUNT_MAX + 10, THRUNTIME_POOL);
	switch_queue_create(&EVENT_QUEUE[2], POOL_COUNT_MAX + 10, THRUNTIME_POOL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Activate Eventing Engine.\n");
	switch_mutex_init(&BLOCK, SWITCH_MUTEX_NESTED, RUNTIME_POOL);
	switch_mutex_init(&POOL_LOCK, SWITCH_MUTEX_NESTED, RUNTIME_POOL);
	switch_mutex_init(&EVENT_QUEUE_MUTEX, SWITCH_MUTEX_NESTED, RUNTIME_POOL);
	switch_mutex_init(&EVENT_QUEUE_HAVEMORE_MUTEX, SWITCH_MUTEX_NESTED, RUNTIME_POOL);
	switch_thread_cond_create(&EVENT_QUEUE_CONDITIONAL, RUNTIME_POOL);
	switch_core_hash_init(&CUSTOM_HASH, RUNTIME_POOL);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, switch_event_thread, NULL, RUNTIME_POOL);

	while (!THREAD_RUNNING) {
		switch_yield(1000);
	}
	return SWITCH_STATUS_SUCCESS;

}

SWITCH_DECLARE(switch_status_t) switch_event_create_subclass(switch_event_t **event, switch_event_types_t event_id, const char *subclass_name)
{

	if (event_id != SWITCH_EVENT_CUSTOM && subclass_name) {
		return SWITCH_STATUS_GENERR;
	}

	if ((*event = ALLOC(sizeof(switch_event_t))) == 0) {
		return SWITCH_STATUS_MEMERR;
	}
	memset(*event, 0, sizeof(switch_event_t));

	(*event)->event_id = event_id;

	if (subclass_name) {
		(*event)->subclass = switch_core_hash_find(CUSTOM_HASH, subclass_name);
		switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Subclass", "%s", subclass_name);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_event_set_priority(switch_event_t *event, switch_priority_t priority)
{
	event->priority = priority;
	switch_event_add_header(event, SWITCH_STACK_TOP, "priority", "%s", switch_priority_name(priority));
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(char *) switch_event_get_header(switch_event_t *event, char *header_name)
{
	switch_event_header_t *hp;
	if (header_name) {
		for (hp = event->headers; hp; hp = hp->next) {
			if (!strcasecmp(hp->name, header_name)) {
				return hp->value;
			}
		}
	}
	return NULL;
}

SWITCH_DECLARE(char *) switch_event_get_body(switch_event_t *event)
{
	if (event) {
		return event->body;
	}

	return NULL;
}

SWITCH_DECLARE(switch_status_t) switch_event_add_header(switch_event_t *event, switch_stack_t stack, const char *header_name, const char *fmt, ...)
{
	int ret = 0;
	char data[2048];

	va_list ap;
	va_start(ap, fmt);
	ret = vsnprintf(data, sizeof(data), fmt, ap);
	va_end(ap);

	if (ret == -1) {
		return SWITCH_STATUS_MEMERR;
	} else {
		switch_event_header_t *header, *hp;

		if ((header = ALLOC(sizeof(*header))) == 0) {
			return SWITCH_STATUS_MEMERR;
		}

		memset(header, 0, sizeof(*header));

		header->name = DUP(header_name);
		header->value = DUP(data);
		if (stack == SWITCH_STACK_TOP) {
			header->next = event->headers;
			event->headers = header;
		} else {
			for (hp = event->headers; hp && hp->next; hp = hp->next);

			if (hp) {
				hp->next = header;
			} else {
				event->headers = header;
			}
		}
		return SWITCH_STATUS_SUCCESS;

	}
}


SWITCH_DECLARE(switch_status_t) switch_event_add_body(switch_event_t *event, const char *fmt, ...)
{
	int ret = 0;
	char *data;

	va_list ap;
	if (fmt) {
		va_start(ap, fmt);
		ret = switch_vasprintf(&data, fmt, ap);
		va_end(ap);

		if (ret == -1) {
			return SWITCH_STATUS_GENERR;
		} else {
			event->body = data;
			return SWITCH_STATUS_SUCCESS;
		}
	} else {
		return SWITCH_STATUS_GENERR;
	}
}

SWITCH_DECLARE(void) switch_event_destroy(switch_event_t **event)
{
	switch_event_t *ep = *event;
	switch_event_header_t *hp, *this;

	if (ep) {
		for (hp = ep->headers; hp;) {
			this = hp;
			hp = hp->next;
			FREE(this->name);
			FREE(this->value);
			FREE(this);
		}
		FREE(ep->body);
		FREE(ep);
	}
	*event = NULL;
}

SWITCH_DECLARE(switch_status_t) switch_event_dup(switch_event_t **event, switch_event_t *todup)
{
	switch_event_header_t *header, *hp, *hp2, *last = NULL;

	if (switch_event_create_subclass(event, todup->event_id, todup->subclass ? todup->subclass->name : NULL) != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_GENERR;
	}

	(*event)->subclass = todup->subclass;
	(*event)->event_user_data = todup->event_user_data;
	(*event)->bind_user_data = todup->bind_user_data;

	hp2 = (*event)->headers;

	for (hp = todup->headers; hp; hp = hp->next) {
		if ((header = ALLOC(sizeof(*header))) == 0) {
			return SWITCH_STATUS_MEMERR;
		}

		memset(header, 0, sizeof(*header));

		header->name = DUP(hp->name);
		header->value = DUP(hp->value);

		if (last) {
			last->next = header;
		} else {
			(*event)->headers = header;
		}

		last = header;
	}

	if (todup->body) {
		(*event)->body = DUP(todup->body);
	}

	(*event)->key = todup->key;

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_event_serialize(switch_event_t *event, char **str)
{
	switch_size_t len = 0;
	switch_event_header_t *hp;
	switch_size_t llen = 0, dlen = 0, blocksize = 512, encode_len = 1536, new_len = 0;
	char *buf;
	char *encode_buf = NULL;	/* used for url encoding of variables to make sure unsafe things stay out of the serialzed copy */

	*str = NULL;

	dlen = blocksize * 2;

	if (!(buf = malloc(dlen))) {
		return SWITCH_STATUS_MEMERR;
	}

	/* go ahead and give ourselves some space to work with, should save a few reallocs */
	if (!(encode_buf = malloc(encode_len))) {
		return SWITCH_STATUS_MEMERR;
	}

	/* switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "hit serialze!.\n"); */
	for (hp = event->headers; hp; hp = hp->next) {
		/*
		 * grab enough memory to store 3x the string (url encode takes one char and turns it into %XX)
		 * so we could end up with a string that is 3 times the original's length, unlikely but rather
		 * be safe than destroy the string, also add one for the null.  And try to be smart about using 
		 * the memory, allocate and only reallocate if we need more.  This avoids an alloc, free CPU
		 * destroying loop.
		 */

		new_len = (strlen(hp->value) * 3) + 1;

		if (encode_len < new_len) {
			char *tmp;
			/* switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Allocing %d was %d.\n", ((strlen(hp->value) * 3) + 1), encode_len); */
			/* we can use realloc for initial alloc as well, if encode_buf is zero it treats it as a malloc */

			/* keep track of the size of our allocation */
			encode_len = new_len;

			if (!(tmp = realloc(encode_buf, encode_len))) {
				/* oh boy, ram's gone, give back what little we grabbed and bail */
				switch_safe_free(buf);
				switch_safe_free(encode_buf);
				return SWITCH_STATUS_MEMERR;
			}

			encode_buf = tmp;
		}

		/* handle any bad things in the string like newlines : etc that screw up the serialized format */
		switch_url_encode(hp->value, encode_buf, encode_len - 1);

		llen = strlen(hp->name) + strlen(encode_buf) + 8;

		if ((len + llen) > dlen) {
			char *m;
			dlen += (blocksize + (len + llen));
			if ((m = realloc(buf, dlen))) {
				buf = m;
			} else {
				/* we seem to be out of memory trying to resize the serialize string, give back what we already have and give up */
				switch_safe_free(buf);
				switch_safe_free(encode_buf);
				return SWITCH_STATUS_MEMERR;
			}
		}

		snprintf(buf + len, dlen - len, "%s: %s\n", hp->name, encode_buf);
		len = strlen(buf);

	}

	/* we are done with the memory we used for encoding, give it back */
	switch_safe_free(encode_buf);

	if (event->body) {
		int blen = (int) strlen(event->body);
		llen = blen;

		if (blen) {
			llen += 25;
		} else {
			llen += 5;
		}

		if ((len + llen) > dlen) {
			char *m;
			dlen += (blocksize + (len + llen));
			if ((m = realloc(buf, dlen))) {
				buf = m;
			} else {
				switch_safe_free(buf);
				return SWITCH_STATUS_MEMERR;
			}
		}

		if (blen) {
			snprintf(buf + len, dlen - len, "Content-Length: %d\n\n%s", blen, event->body);
		} else {
			snprintf(buf + len, dlen - len, "\n");
		}
	} else {
		snprintf(buf + len, dlen - len, "\n");
	}

	*str = buf;

	return SWITCH_STATUS_SUCCESS;
}


static switch_xml_t add_xml_header(switch_xml_t xml, char *name, char *value, int offset)
{
	switch_xml_t header = NULL;

	if ((header = switch_xml_add_child_d(xml, "header", offset))) {
		switch_xml_set_attr_d(header, "name", name);
		switch_xml_set_attr_d(header, "value", value);
	}

	return header;
}

SWITCH_DECLARE(switch_xml_t) switch_event_xmlize(switch_event_t *event, const char *fmt,...)
{
	switch_event_header_t *hp;
	char *data = NULL, *body = NULL;
	int ret = 0;
	switch_xml_t xml = NULL;
	uint32_t off = 0;
	va_list ap;

	if (!(xml = switch_xml_new("event"))) {
		return xml;
	}

	if (fmt) {
		va_start(ap, fmt);
#ifdef HAVE_VASPRINTF
		ret = vasprintf(&data, fmt, ap);
#else
		data = (char *) malloc(2048);
		ret = vsnprintf(data, 2048, fmt, ap);
#endif
		va_end(ap);
		if (ret == -1) {
			return NULL;
		}
	}


	for (hp = event->headers; hp; hp = hp->next) {
		add_xml_header(xml, hp->name, hp->value, off++);
	}

	if (data) {
		body = data;
	} else if (event->body) {
		body = event->body;
	}

	if (body) {
		int blen = (int) strlen(body);
		char blena[25];
		snprintf(blena, sizeof(blena), "%d", blen);
		if (blen) {
			switch_xml_t xbody = NULL;

			add_xml_header(xml, "Content-Length", blena, off++);
			if ((xbody = switch_xml_add_child_d(xml, "body", off++))) {
				switch_xml_set_txt_d(xbody, body);
			}
		}
	}

	if (data) {
		free(data);
	}

	return xml;
}


SWITCH_DECLARE(switch_status_t) switch_event_fire_detailed(char *file, char *func, int line, switch_event_t **event, void *user_data)
{

	switch_time_exp_t tm;
	char date[80] = "";
	switch_size_t retsize;

	assert(BLOCK != NULL);
	assert(RUNTIME_POOL != NULL);
	assert(EVENT_QUEUE_HAVEMORE_MUTEX != NULL);
	assert(EVENT_QUEUE_MUTEX != NULL);
	assert(EVENT_QUEUE_CONDITIONAL != NULL);
	assert(RUNTIME_POOL != NULL);

	if (THREAD_RUNNING <= 0) {
		/* sorry we're closed */
		switch_event_destroy(event);
		return SWITCH_STATUS_FALSE;
	}

	switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Name", "%s", switch_event_name((*event)->event_id));
	switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Core-UUID", "%s", switch_core_get_uuid());
	switch_time_exp_lt(&tm, switch_time_now());
	switch_strftime(date, &retsize, sizeof(date), "%Y-%m-%d %T", &tm);
	switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Date-Local", "%s", date);
	switch_rfc822_date(date, switch_time_now());
	switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Date-GMT", "%s", date);
	switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Calling-File", "%s", switch_cut_path(file));
	switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Calling-Function", "%s", func);
	switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Calling-Line-Number", "%d", line);

	if ((*event)->subclass) {
		switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Subclass", "%s", (*event)->subclass->name);
		switch_event_add_header(*event, SWITCH_STACK_BOTTOM, "Event-Subclass-Owner", "%s", (*event)->subclass->owner);
	}

	if (user_data) {
		(*event)->event_user_data = user_data;
	}

	switch_queue_push(EVENT_QUEUE[(*event)->priority], *event);

	/* lock on havemore to make sure he event thread, if currently running
	 * doesn't check the HAVEMORE flag before we set it
	 */
	switch_mutex_lock(EVENT_QUEUE_HAVEMORE_MUTEX);
	/* see if the event thread is sitting */
	if (switch_mutex_trylock(EVENT_QUEUE_MUTEX) == SWITCH_STATUS_SUCCESS) {
		/* we don't need havemore anymore, the thread was sitting already */
		switch_mutex_unlock(EVENT_QUEUE_HAVEMORE_MUTEX);

		/* wake up the event thread */
		switch_thread_cond_signal(EVENT_QUEUE_CONDITIONAL);

		/* give up our lock */
		switch_mutex_unlock(EVENT_QUEUE_MUTEX);
	} else {
		/* it wasn't waiting which means we might have updated a queue it already looked at
		 * set a flag so it knows to read the queues again
		 */
		EVENT_QUEUE_HAVEMORE = 1;

		/* variable updated, give up the mutex */
		switch_mutex_unlock(EVENT_QUEUE_HAVEMORE_MUTEX);
	}


	*event = NULL;

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_event_bind(const char *id, switch_event_types_t event, char *subclass_name, switch_event_callback_t callback,
												  void *user_data)
{
	switch_event_node_t *event_node;
	switch_event_subclass_t *subclass = NULL;

	assert(BLOCK != NULL);
	assert(RUNTIME_POOL != NULL);

	if (subclass_name) {
		if ((subclass = switch_core_hash_find(CUSTOM_HASH, subclass_name)) == 0) {
			if ((subclass = switch_core_alloc(RUNTIME_POOL, sizeof(*subclass))) == 0) {
				return SWITCH_STATUS_MEMERR;
			} else {
				subclass->owner = switch_core_strdup(RUNTIME_POOL, id);
				subclass->name = switch_core_strdup(RUNTIME_POOL, subclass_name);
			}
		}
	}

	if (event <= SWITCH_EVENT_ALL && (event_node = switch_core_alloc(RUNTIME_POOL, sizeof(switch_event_node_t))) != 0) {
		switch_mutex_lock(BLOCK);
		/* <LOCKED> ----------------------------------------------- */
		event_node->id = switch_core_strdup(RUNTIME_POOL, id);
		event_node->event_id = event;
		event_node->subclass = subclass;
		event_node->callback = callback;
		event_node->user_data = user_data;

		if (EVENT_NODES[event]) {
			event_node->next = EVENT_NODES[event];
		}

		EVENT_NODES[event] = event_node;
		switch_mutex_unlock(BLOCK);
		/* </LOCKED> ----------------------------------------------- */
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_MEMERR;
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
