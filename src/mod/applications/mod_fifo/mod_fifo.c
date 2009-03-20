/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2009, Anthony Minessale II <anthm@freeswitch.org>
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
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthm@freeswitch.org>
 *
 * mod_fifo.c -- FIFO
 *
 */
#include <switch.h>
#ifdef SWITCH_HAVE_ODBC
#include <switch_odbc.h>
#endif

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_fifo_shutdown);
SWITCH_MODULE_LOAD_FUNCTION(mod_fifo_load);
SWITCH_MODULE_DEFINITION(mod_fifo, mod_fifo_load, mod_fifo_shutdown, NULL);

#define FIFO_EVENT "fifo::info"

static switch_status_t load_config(int reload, int del_all);
#define MAX_PRI 10

struct fifo_node {
	char *name;
	switch_mutex_t *mutex;
	switch_queue_t *fifo_list[MAX_PRI];
	switch_hash_t *caller_hash;
	switch_hash_t *consumer_hash;
	int caller_count;
	int consumer_count;
	switch_time_t start_waiting;
	uint32_t importance;
	switch_thread_rwlock_t *rwlock;
	switch_memory_pool_t *pool;
	int has_outbound;
	int ready;
};

typedef struct fifo_node fifo_node_t;

static switch_status_t on_dtmf(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{
	switch_core_session_t *bleg = (switch_core_session_t *) buf;

	switch (itype) {
	case SWITCH_INPUT_TYPE_DTMF:
		{
			switch_dtmf_t *dtmf = (switch_dtmf_t *) input;
			switch_channel_t *bchan = switch_core_session_get_channel(bleg);
			switch_channel_t *channel = switch_core_session_get_channel(session);

			const char *consumer_exit_key = switch_channel_get_variable(channel, "fifo_consumer_exit_key");

			if (switch_channel_test_flag(switch_core_session_get_channel(session), CF_ORIGINATOR)) {
				if ( consumer_exit_key && dtmf->digit == *consumer_exit_key ) {
					switch_channel_hangup(bchan, SWITCH_CAUSE_NORMAL_CLEARING);
					return SWITCH_STATUS_BREAK;
				}
				else if ( !consumer_exit_key && dtmf->digit == '*' ) {
					switch_channel_hangup(bchan, SWITCH_CAUSE_NORMAL_CLEARING);
					return SWITCH_STATUS_BREAK;
				} else if (dtmf->digit == '0') {
					const char *moh_a = NULL, *moh_b = NULL;

					if (!(moh_b = switch_channel_get_variable(bchan, "fifo_music"))) {
						moh_b = switch_channel_get_variable(bchan, "hold_music");
					}

					if (!(moh_a = switch_channel_get_variable(channel, "fifo_hold_music"))) {
						if (!(moh_a = switch_channel_get_variable(channel, "fifo_music"))) {
							moh_a = switch_channel_get_variable(channel, "hold_music");
						}
					}

					switch_ivr_soft_hold(session, "0", moh_a, moh_b);
					return SWITCH_STATUS_IGNORE;
				}
			}
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t moh_on_dtmf(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{
	switch (itype) {
	case SWITCH_INPUT_TYPE_DTMF:
		{
			switch_dtmf_t *dtmf = (switch_dtmf_t *) input;
			switch_channel_t *channel = switch_core_session_get_channel(session);
			const char *caller_exit_key = switch_channel_get_variable(channel, "fifo_caller_exit_key");

			if (caller_exit_key && dtmf->digit == *caller_exit_key) {
				char *bp = buf;
				*bp = dtmf->digit;
				return SWITCH_STATUS_BREAK;
			}
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

#define check_string(s) if (!switch_strlen_zero(s) && !strcasecmp(s, "undef")) { s = NULL; }

static int node_consumer_wait_count(fifo_node_t *node)
{
	int i, len = 0;

	for (i = 0; i < MAX_PRI; i++) {
		len += switch_queue_size(node->fifo_list[i]);
	}

	return len;
}

static void node_remove_uuid(fifo_node_t *node, const char *uuid)
{
	int i, len = 0, done = 0;
	void *pop = NULL;

	for (i = 0; i < MAX_PRI; i++) {
		if (!(len = switch_queue_size(node->fifo_list[i]))) {
			continue;
		}
		while (len) {
			if (switch_queue_trypop(node->fifo_list[i], &pop) == SWITCH_STATUS_SUCCESS && pop) {
				if (!done && !strcmp((char *) pop, uuid)) {
					free(pop);
					done++;
				} else {
					switch_queue_push(node->fifo_list[i], pop);
				}
			}
			len--;
		}
	}

	if (!node_consumer_wait_count(node)) {
		node->start_waiting = 0;
	}

	return;
}

#define MAX_CHIME 25
struct fifo_chime_data {
	char *list[MAX_CHIME];
	int total;
	int index;
	time_t next;
	int freq;
	int abort;
	time_t orbit_timeout;
	int do_orbit;
	char *orbit_exten;
};

typedef struct fifo_chime_data fifo_chime_data_t;

static switch_status_t caller_read_frame_callback(switch_core_session_t *session, switch_frame_t *frame, void *user_data)
{
	fifo_chime_data_t *cd = (fifo_chime_data_t *) user_data;

	if (!cd) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (cd->total && switch_epoch_time_now(NULL) >= cd->next) {
		if (cd->index == MAX_CHIME || cd->index == cd->total || !cd->list[cd->index]) {
			cd->index = 0;
		}

		if (cd->list[cd->index]) {
			switch_input_args_t args = { 0 };
			char buf[25] = "";
			switch_channel_t *channel = switch_core_session_get_channel(session);
			const char *caller_exit_key = switch_channel_get_variable(channel, "fifo_caller_exit_key");
			args.input_callback = moh_on_dtmf;
			args.buf = buf;
			args.buflen = sizeof(buf);

			if (switch_ivr_play_file(session, NULL, cd->list[cd->index], &args) != SWITCH_STATUS_SUCCESS) {
                return SWITCH_STATUS_FALSE;
            }

			if (caller_exit_key && *buf == *caller_exit_key) {
				cd->abort = 1;
				return SWITCH_STATUS_FALSE;
			}
			cd->next = switch_epoch_time_now(NULL) + cd->freq;
			cd->index++;
		}
	} else if (cd->orbit_timeout && switch_epoch_time_now(NULL) >= cd->orbit_timeout) {
		cd->do_orbit = 1;
		return SWITCH_STATUS_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t consumer_read_frame_callback(switch_core_session_t *session, switch_frame_t *frame, void *user_data)
{
	fifo_node_t *node, **node_list = (fifo_node_t **) user_data;
	int x = 0, total = 0, i = 0;
	
	for (i = 0 ;; i++) {
		if (!(node = node_list[i])) {
			break;
		}
		for (x = 0; x < MAX_PRI; x++) {
			total += switch_queue_size(node->fifo_list[x]);
		}
	}

	if (total) {
		return SWITCH_STATUS_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}

static struct {
	switch_hash_t *fifo_hash;
	switch_mutex_t *mutex;
	switch_mutex_t *sql_mutex;
	switch_memory_pool_t *pool;
	int running;
	switch_event_node_t *node;
	char hostname[256];
	char *dbname;
	char *odbc_dsn;
	int node_thread_running;
	
#ifdef SWITCH_HAVE_ODBC
	switch_odbc_handle_t *master_odbc;
#else
	void *filler1;
#endif
} globals;


static switch_status_t fifo_execute_sql(char *sql, switch_mutex_t *mutex)
{
	switch_core_db_t *db;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char *err_str;

	if (mutex) {
		switch_mutex_lock(mutex);
	}
#ifdef SWITCH_HAVE_ODBC
	if (globals.odbc_dsn) {
		SQLHSTMT stmt;
		if (switch_odbc_handle_exec(globals.master_odbc, sql, &stmt) != SWITCH_ODBC_SUCCESS) {
			
			err_str = switch_odbc_handle_get_error(globals.master_odbc, stmt);
			if (!switch_strlen_zero(err_str)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, switch_str_nil(err_str));
			}
			switch_safe_free(err_str);
			status = SWITCH_STATUS_FALSE;
		}
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	} else {
#endif
		if (!(db = switch_core_db_open_file(globals.dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", globals.dbname);
			status = SWITCH_STATUS_FALSE;
			goto end;
		}

		err_str = NULL;
		switch_core_db_exec(db, sql, NULL, NULL, &err_str);
		if (err_str) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error [%s]\n[%s]\n", sql, err_str);
			free(err_str);
		}

		switch_core_db_close(db);

#ifdef SWITCH_HAVE_ODBC
	}
#endif

  end:
	if (mutex) {
		switch_mutex_unlock(mutex);
	}

	return status;
}

static switch_bool_t fifo_execute_sql_callback(switch_mutex_t *mutex, char *sql, switch_core_db_callback_func_t callback, void *pdata)
{
	switch_bool_t ret = SWITCH_FALSE;
	switch_core_db_t *db;
	char *errmsg = NULL;

	if (mutex) {
		switch_mutex_lock(mutex);
	}

#ifdef SWITCH_HAVE_ODBC
	if (globals.odbc_dsn) {
		switch_odbc_handle_callback_exec(globals.master_odbc, sql, callback, pdata);
	} else {
#endif
		if (!(db = switch_core_db_open_file(globals.dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", globals.dbname);
			goto end;
		}

		switch_core_db_exec(db, sql, callback, pdata, &errmsg);

		if (errmsg) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "SQL ERR: [%s] %s\n", sql, errmsg);
			free(errmsg);
		}

		if (db) {
			switch_core_db_close(db);
		}
#ifdef SWITCH_HAVE_ODBC
	}
#endif

  end:
	if (mutex) {
		switch_mutex_unlock(mutex);
	}

	return ret;
}

static fifo_node_t *create_node(const char *name, uint32_t importance)
{
	fifo_node_t *node;
	int x = 0;

	if (!globals.running) {
		return NULL;
	}

	node = switch_core_alloc(globals.pool, sizeof(*node));
	node->pool = globals.pool;

	node->name = switch_core_strdup(node->pool, name);
	for (x = 0; x < MAX_PRI; x++) {
		switch_queue_create(&node->fifo_list[x], SWITCH_CORE_QUEUE_LEN, node->pool);
		switch_assert(node->fifo_list[x]);
	}
	switch_core_hash_init(&node->caller_hash, node->pool);
	switch_core_hash_init(&node->consumer_hash, node->pool);
	switch_thread_rwlock_create(&node->rwlock, node->pool);
	switch_mutex_init(&node->mutex, SWITCH_MUTEX_NESTED, node->pool);
	node->importance = importance;
	switch_core_hash_insert(globals.fifo_hash, name, node);
	return node;
}

static int node_idle_consumers(fifo_node_t *node)
{
	switch_hash_index_t *hi;
	void *val;
	const void *var;
	switch_core_session_t *session;
	switch_channel_t *channel;
	int total = 0;

	switch_mutex_lock(node->mutex);
	for (hi = switch_hash_first(NULL, node->consumer_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, &var, NULL, &val);
		session = (switch_core_session_t *) val;
		channel = switch_core_session_get_channel(session);
		if (!switch_channel_test_flag(channel, CF_BRIDGED)) {
			total++;
		}
	}
	switch_mutex_unlock(node->mutex);

	return total;
	
}

static switch_status_t hanguphook(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_state_t state = switch_channel_get_state(channel);
	const char *uuid = NULL;
	char sql[256] = "";

	if (state == CS_HANGUP || state == CS_ROUTING) {
		if ((uuid = switch_channel_get_variable(channel, "fifo_outbound_uuid"))) {
			switch_snprintf(sql, sizeof(sql), "update fifo_outbound set use_count=use_count-1, outbound_call_count=outbound_call_count+1, next_avail=%ld + lag where uuid='%s'", 
							(long)switch_epoch_time_now(NULL), uuid);
							
			fifo_execute_sql(sql, globals.sql_mutex);
		}
		switch_core_event_hook_remove_state_change(session, hanguphook);
	}
	return SWITCH_STATUS_SUCCESS;
}

struct call_helper {
	char *uuid;
	char *node_name;
	char *originate_string;
	int timeout;
	switch_memory_pool_t *pool;
};

static void *SWITCH_THREAD_FUNC o_thread_run(switch_thread_t *thread, void *obj)
{
	struct call_helper *h = (struct call_helper *) obj;

	switch_core_session_t *session = NULL;
	switch_channel_t *channel;
	switch_call_cause_t cause = SWITCH_CAUSE_NONE;
	switch_caller_extension_t *extension = NULL;
	char *app_name, *arg = NULL;
	char sql[256] = "";
	const char *member_wait = NULL;

	switch_snprintf(sql, sizeof(sql), "update fifo_outbound set use_count=use_count+1 where uuid='%s'", h->uuid);
	fifo_execute_sql(sql, globals.sql_mutex);
	
	if (switch_ivr_originate(NULL, &session, &cause, h->originate_string, h->timeout, NULL, NULL, NULL, NULL, NULL, SOF_NONE) != SWITCH_STATUS_SUCCESS) {
		switch_snprintf(sql, sizeof(sql), "update fifo_outbound set use_count=use_count-1, outbound_fail_count=outbound_fail_count+1, next_avail=%ld + lag where uuid='%s'", 
						(long)switch_epoch_time_now(NULL), h->uuid);
		fifo_execute_sql(sql, globals.sql_mutex);
		goto end;
	}
	

	channel = switch_core_session_get_channel(session);

	if ((member_wait = switch_channel_get_variable(channel, "member_wait"))) {
		if (strcasecmp(member_wait, "wait") && strcasecmp(member_wait, "nowait")) {
			member_wait = NULL;
		}
	}

	switch_channel_set_variable(channel, "fifo_outbound_uuid", h->uuid);
	switch_core_event_hook_add_state_change(session, hanguphook);
	app_name = "fifo";
	arg = switch_core_session_sprintf(session, "%s out %s", h->node_name, member_wait ? member_wait : "wait");
	extension = switch_caller_extension_new(session, app_name, arg);
	switch_caller_extension_add_application(session, extension, app_name, arg);
	switch_channel_set_caller_extension(channel, extension);
	switch_channel_set_state(channel, CS_EXECUTE);
	switch_core_session_rwunlock(session);

  end:

	switch_core_destroy_memory_pool(&h->pool);

	return NULL;
}

static int place_call_callback(void *pArg, int argc, char **argv, char **columnNames)
{

	int *need = (int *) pArg;

	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;
	switch_memory_pool_t *pool;
	struct call_helper *h;
	
	switch_core_new_memory_pool(&pool);
	h = switch_core_alloc(pool, sizeof(*h));
	h->pool = pool;
	h->uuid = switch_core_strdup(h->pool, argv[0]);
	h->node_name = switch_core_strdup(h->pool, argv[1]);
	h->originate_string = switch_core_strdup(h->pool, argv[2]);
	h->timeout = atoi(argv[5]);


	switch_threadattr_create(&thd_attr, h->pool);
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, o_thread_run, h, h->pool);

	(*need)--;
	
	return *need ? 0 : -1;
}

static void find_consumers(fifo_node_t *node)
{
	int need = node_consumer_wait_count(node);
	char *sql;

	sql = switch_mprintf("select uuid, fifo_name, originate_string, simo_count, use_count, timeout, lag, "
						 "next_avail, expires, static, outbound_call_count, outbound_fail_count, hostname "
						 "from fifo_outbound where (fifo_name = '%s') and (use_count < simo_count) and (next_avail = 0 or next_avail <= %ld) "
						 "order by outbound_call_count", node->name, (long) switch_epoch_time_now(NULL));

	switch_assert(sql);
	fifo_execute_sql_callback(globals.sql_mutex, sql, place_call_callback, &need);
	free(sql);
}

static void *SWITCH_THREAD_FUNC node_thread_run(switch_thread_t *thread, void *obj)
{
	fifo_node_t *node;
	
	globals.node_thread_running = 1;

	while(globals.node_thread_running == 1) {
		switch_hash_index_t *hi;
		void *val;
		const void *var;
		int ppl_waiting, consumer_total, idle_consumers;

		switch_mutex_lock(globals.mutex);
		for (hi = switch_hash_first(NULL, globals.fifo_hash); hi; hi = switch_hash_next(hi)) {
			switch_hash_this(hi, &var, NULL, &val);
			if ((node = (fifo_node_t *) val) && node->has_outbound && node->ready) {
				switch_mutex_lock(node->mutex);
				ppl_waiting = node_consumer_wait_count(node);
				consumer_total = node->consumer_count;
				idle_consumers = node_idle_consumers(node);

				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
				//"%s waiting %d consumer_total %d idle_consumers %d\n", node->name, ppl_waiting, consumer_total, idle_consumers);
		
				if (ppl_waiting && (!consumer_total || !idle_consumers)) {
					find_consumers(node);
				}
				switch_mutex_unlock(node->mutex);
			}
		}
		switch_mutex_unlock(globals.mutex);

		switch_yield(1000000);
	}

	globals.node_thread_running = 0;
	
	return NULL;
}

static void start_node_thread(switch_memory_pool_t *pool)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;

	switch_threadattr_create(&thd_attr, pool);
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, node_thread_run, pool, pool);
}

static int stop_node_thread(void)
{
	int sanity = 20;

	if (globals.node_thread_running) {
		globals.node_thread_running = -1;
		while(globals.node_thread_running) {
			switch_yield(500000);
			if (!--sanity) {
				return -1;
			}
		}
	}

	return 0;
}

static void send_presence(fifo_node_t *node)
{
	switch_event_t *event;
	int wait_count = 0;

	if (!globals.running) {
		return;
	}

	if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", "park");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "login", node->name);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "from", node->name);
		if ((wait_count = node_consumer_wait_count(node)) > 0) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "status", "Active (%d waiting)", wait_count);
		} else {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "status", "Idle");
		}
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "rpid", "unknown");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "event_type", "presence");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alt_event_type", "dialog");
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "event_count", "%d", 0);

		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "channel-state", wait_count > 0 ? "CS_ROUTING" : "CS_HANGUP");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "unique-id", node->name);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "answer-state", wait_count > 0 ? "early" : "terminated");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "presence-call-direction", "inbound");
		switch_event_fire(&event);
	}
}

static void pres_event_handler(switch_event_t *event)
{
	char *to = switch_event_get_header(event, "to");
	char *dup_to = NULL, *node_name;
	fifo_node_t *node;

	if (!globals.running) {
		return;
	}

	if (!to || strncasecmp(to, "park+", 5)) {
		return;
	}

	dup_to = strdup(to);
	switch_assert(dup_to);

	node_name = dup_to + 5;

	switch_mutex_lock(globals.mutex);
	if (!(node = switch_core_hash_find(globals.fifo_hash, node_name))) {
		node = create_node(node_name, 0);
	}

	send_presence(node);

	switch_mutex_unlock(globals.mutex);

	switch_safe_free(dup_to);
}

typedef enum {
	STRAT_MORE_PPL,
	STRAT_WAITING_LONGER,
} fifo_strategy_t;

#define MAX_NODES_PER_CONSUMER 25
#define FIFO_DESC "Fifo for stacking parked calls."
#define FIFO_USAGE "<fifo name>[!<importance_number>] [in [<announce file>|undef] [<music file>|undef] | out [wait|nowait] [<announce file>|undef] [<music file>|undef]]"
SWITCH_STANDARD_APP(fifo_function)
{
	int argc;
	char *mydata = NULL, *argv[5] = { 0 };
	fifo_node_t *node = NULL, *node_list[MAX_NODES_PER_CONSUMER + 1] = { 0 };
	switch_channel_t *channel = switch_core_session_get_channel(session);
	int do_wait = 1, node_count = 0, i = 0;
	const char *moh = NULL;
	const char *announce = NULL;
	switch_event_t *event = NULL;
	char date[80] = "";
	switch_time_exp_t tm;
	switch_time_t ts = switch_micro_time_now();
	switch_size_t retsize;
	char *list_string;
	int nlist_count;
	char *nlist[MAX_NODES_PER_CONSUMER];
	int consumer = 0;
	const char *arg_fifo_name = NULL;
	const char *arg_inout = NULL;
	const char *serviced_uuid = NULL;

	if (!globals.running) {
		return;
	}

	if (switch_strlen_zero(data)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No Args\n");
		return;
	}

	mydata = switch_core_session_strdup(session, data);
	switch_assert(mydata);

	argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	arg_fifo_name = argv[0];
	arg_inout = argv[1];

	if (!(arg_fifo_name && arg_inout)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "USAGE %s\n", FIFO_USAGE);
		return;
	}

	if (!strcasecmp(arg_inout, "out")) {
		consumer = 1;
	} else if (strcasecmp(arg_inout, "in")) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "USAGE %s\n", FIFO_USAGE);
		return;
	}

	list_string = switch_core_session_strdup(session, arg_fifo_name);

	if (!(nlist_count = switch_separate_string(list_string, ',', nlist, (sizeof(nlist) / sizeof(nlist[0]))))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "USAGE %s\n", FIFO_USAGE);
		return;
	}

	if (!consumer && nlist_count > 1) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "USAGE %s\n", FIFO_USAGE);
		return;
	}

	switch_mutex_lock(globals.mutex);
	for (i = 0; i < nlist_count; i++) {
		int importance = 0;
		char *p;

		if ((p = strrchr(nlist[i], '!'))) {
			*p++ = '\0';
			importance = atoi(p);
			if (importance < 0) {
				importance = 0;
			}
		}

		if (!(node = switch_core_hash_find(globals.fifo_hash, nlist[i]))) {
			node = create_node(nlist[i], importance);
		}
		node_list[node_count++] = node;
	}
	switch_mutex_unlock(globals.mutex);

	moh = switch_channel_get_variable(channel, "fifo_music");
	announce = switch_channel_get_variable(channel, "fifo_announce");

	if (consumer) {
		if (argc > 3) {
			announce = argv[3];
		}

		if (argc > 4) {
			moh = argv[4];
		}
	} else {
		if (argc > 2) {
			announce = argv[2];
		}

		if (argc > 3) {
			moh = argv[3];
		}
	}

	if (moh && !strcasecmp(moh, "silence")) {
		moh = NULL;
	}

	check_string(announce);
	check_string(moh);
	switch_assert(node);

	switch_core_media_bug_pause(session);

	if (!consumer) {
		switch_core_session_t *other_session;
		switch_channel_t *other_channel;
		const char *uuid = switch_core_session_get_uuid(session);
		const char *pri;
		char tmp[25] = "";
		int p = 0;
		int aborted = 0;
		fifo_chime_data_t cd = { {0} };
		const char *chime_list = switch_channel_get_variable(channel, "fifo_chime_list");
		const char *chime_freq = switch_channel_get_variable(channel, "fifo_chime_freq");
		const char *orbit_var = switch_channel_get_variable(channel, "fifo_orbit_exten");
		const char *orbit_ann = switch_channel_get_variable(channel, "fifo_orbit_announce");
		const char *caller_exit_key = switch_channel_get_variable(channel, "fifo_caller_exit_key");
		int freq = 30;
		int ftmp = 0;
		int to = 60;

		if (orbit_var) {
			char *ot;
			if ((cd.orbit_exten = switch_core_session_strdup(session, orbit_var))) {
				if ((ot = strchr(cd.orbit_exten, ':'))) {
					*ot++ = '\0';
					if ((to = atoi(ot)) < 0) {
						to = 60;
					}
				}
				cd.orbit_timeout = switch_epoch_time_now(NULL) + to;
			}
		}

		if (chime_freq) {
			ftmp = atoi(chime_freq);
			if (ftmp > 0) {
				freq = ftmp;
			}
		}

		switch_channel_answer(channel);

		switch_mutex_lock(node->mutex);
		node->caller_count++;

		switch_core_hash_insert(node->caller_hash, uuid, session);

		if ((pri = switch_channel_get_variable(channel, "fifo_priority"))) {
			p = atoi(pri);
		}

		if (p >= MAX_PRI) {
			p = MAX_PRI - 1;
		}

		if (!node_consumer_wait_count(node)) {
			node->start_waiting = switch_micro_time_now();
		}

		if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(channel, event);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", "push");
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "FIFO-Slot", "%d", p);
			switch_event_fire(&event);
		}
		
		switch_queue_push(node->fifo_list[p], (void *) strdup(uuid));

		if (!pri) {
			switch_snprintf(tmp, sizeof(tmp), "%d", p);
			switch_channel_set_variable(channel, "fifo_priority", tmp);
		}

		switch_mutex_unlock(node->mutex);

		ts = switch_micro_time_now();
		switch_time_exp_lt(&tm, ts);
		switch_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d %T", &tm);
		switch_channel_set_variable(channel, "fifo_status", "WAITING");
		switch_channel_set_variable(channel, "fifo_timestamp", date);
		switch_channel_set_variable(channel, "fifo_serviced_uuid", NULL);

		switch_channel_set_app_flag(channel, CF_APP_TAGGED);

		if (chime_list) {
			char *list_dup = switch_core_session_strdup(session, chime_list);
			cd.total = switch_separate_string(list_dup, ',', cd.list, (sizeof(cd.list) / sizeof(cd.list[0])));
			cd.freq = freq;
			cd.next = switch_epoch_time_now(NULL) + cd.freq;
		}

		send_presence(node);

		while (switch_channel_ready(channel)) {
			switch_input_args_t args = { 0 };
			char buf[25] = "";

			args.input_callback = moh_on_dtmf;
			args.buf = buf;
			args.buflen = sizeof(buf);

			if (cd.total || cd.orbit_timeout) {
				args.read_frame_callback = caller_read_frame_callback;
				args.user_data = &cd;
			}

			if (cd.abort || cd.do_orbit) {
				aborted = 1;
				goto abort;
			}

			if ((serviced_uuid = switch_channel_get_variable(channel, "fifo_serviced_uuid"))) {
				break;
			}

			switch_core_session_flush_private_events(session);

			if (moh) {
				switch_status_t status = switch_ivr_play_file(session, NULL, moh, &args);
				if (!SWITCH_READ_ACCEPTABLE(status)) {
					aborted = 1;
					goto abort;
				}
			} else {
				switch_ivr_collect_digits_callback(session, &args, 0);
			}

			if (caller_exit_key && *buf == *caller_exit_key) {
				aborted = 1;
				goto abort;
			}

		}

		if (!serviced_uuid && switch_channel_ready(channel)) {
			switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
		} else if ((other_session = switch_core_session_locate(serviced_uuid))) {
			int ready;
			other_channel = switch_core_session_get_channel(other_session);
			ready = switch_channel_ready(other_channel);
			switch_core_session_rwunlock(other_session);
			if (!ready) {
				switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
			}
		}

		switch_core_session_flush_private_events(session);

		if (switch_channel_ready(channel)) {
			if (announce) {
				switch_ivr_play_file(session, NULL, announce, NULL);
			}
		}

		switch_channel_clear_app_flag(channel, CF_APP_TAGGED);

	  abort:

		if (!aborted && switch_channel_ready(channel)) {
			switch_channel_set_state(channel, CS_HIBERNATE);
			goto done;
		} else {
			ts = switch_micro_time_now();
			switch_time_exp_lt(&tm, ts);
			switch_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d %T", &tm);
			switch_channel_set_variable(channel, "fifo_status", cd.do_orbit ? "TIMEOUT" : "ABORTED");
			switch_channel_set_variable(channel, "fifo_timestamp", date);

			if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
				switch_channel_event_set_data(channel, event);
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", cd.do_orbit ? "timeout" : "abort");
				switch_event_fire(&event);
			}
			switch_mutex_lock(node->mutex);
			node_remove_uuid(node, uuid);
			node->caller_count--;
			switch_core_hash_delete(node->caller_hash, uuid);
			switch_mutex_unlock(node->mutex);
			send_presence(node);

		}

		if (cd.do_orbit && cd.orbit_exten) {
			if (orbit_ann) {
				switch_ivr_play_file(session, NULL, orbit_ann, NULL);
			}
			switch_ivr_session_transfer(session, cd.orbit_exten, NULL, NULL);
		}

		goto done;

	} else {					/* consumer */
		void *pop = NULL;
		switch_frame_t *read_frame;
		switch_status_t status;
		char *uuid;
		int done = 0;
		switch_core_session_t *other_session;
		switch_input_args_t args = { 0 };
		const char *pop_order = NULL;
		int custom_pop = 0;
		int pop_array[MAX_PRI] = { 0 };
		char *pop_list[MAX_PRI] = { 0 };
		const char *fifo_consumer_wrapup_sound = NULL;
		const char *fifo_consumer_wrapup_key = NULL;
		const char *sfifo_consumer_wrapup_time = NULL;
		uint32_t fifo_consumer_wrapup_time = 0;
		switch_time_t wrapup_time_elapsed = 0, wrapup_time_started = 0, wrapup_time_remaining = 0;
		const char *my_id;
		char buf[5] = "";
		const char *strat_str = switch_channel_get_variable(channel, "fifo_strategy");
		fifo_strategy_t strat = STRAT_WAITING_LONGER;


		if (!switch_strlen_zero(strat_str)) {
			if (!strcasecmp(strat_str, "more_ppl")) {
				strat = STRAT_MORE_PPL;
			} else if (!strcasecmp(strat_str, "waiting_longer")) {
				strat = STRAT_WAITING_LONGER;
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid strategy\n");
				goto done;
			}
		}

		if (argc > 2) {
			if (!strcasecmp(argv[2], "nowait")) {
				do_wait = 0;
			} else if (strcasecmp(argv[2], "wait")) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "USAGE %s\n", FIFO_USAGE);
				goto done;
			}
		}

		if (!(my_id = switch_channel_get_variable(channel, "fifo_consumer_id"))) {
			my_id = switch_core_session_get_uuid(session);
		}

		if (do_wait) {
			for (i = 0; i < node_count; i++) {
				if (!(node = node_list[i])) {
					continue;
				}
				switch_mutex_lock(node->mutex);
				node->consumer_count++;
				switch_core_hash_insert(node->consumer_hash, switch_core_session_get_uuid(session), session);
				switch_mutex_unlock(node->mutex);
			}
			switch_channel_answer(channel);
		}

		if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(channel, event);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", "consumer_start");
			switch_event_fire(&event);
		}

		ts = switch_micro_time_now();
		switch_time_exp_lt(&tm, ts);
		switch_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d %T", &tm);
		switch_channel_set_variable(channel, "fifo_status", "WAITING");
		switch_channel_set_variable(channel, "fifo_timestamp", date);

		if ((pop_order = switch_channel_get_variable(channel, "fifo_pop_order"))) {
			char *tmp = switch_core_session_strdup(session, pop_order);
			int x;
			custom_pop = switch_separate_string(tmp, ',', pop_list, (sizeof(pop_list) / sizeof(pop_list[0])));
			if (custom_pop >= MAX_PRI) {
				custom_pop = MAX_PRI - 1;
			}

			for (x = 0; x < custom_pop; x++) {
				int temp;
				switch_assert(pop_list[x]);
				temp = atoi(pop_list[x]);
				if (temp > -1 && temp < MAX_PRI) {
					pop_array[x] = temp;
				}
			}
		}

		while (switch_channel_ready(channel)) {
			int x = 0, winner = -1;
			switch_time_t longest = (0xFFFFFFFFFFFFFFFFULL / 2);
			uint32_t importance = 0, waiting = 0, most_waiting = 0;

			pop = NULL;

			if (moh && do_wait) {
				switch_status_t moh_status;
				memset(&args, 0, sizeof(args));
				args.read_frame_callback = consumer_read_frame_callback;
				args.user_data = node_list;
				moh_status = switch_ivr_play_file(session, NULL, moh, &args);

				if (!SWITCH_READ_ACCEPTABLE(moh_status)) {
					break;
				}
			}

			for (i = 0; i < node_count; i++) {
				if (!(node = node_list[i])) {
					continue;
				}

				if ((waiting = node_consumer_wait_count(node))) {

					if (!importance || node->importance > importance) {
						if (strat == STRAT_WAITING_LONGER) {
							if (node->start_waiting < longest) {
								longest = node->start_waiting;
								winner = i;
							}
						} else {
							if (waiting > most_waiting) {
								most_waiting = waiting;
								winner = i;
							}
						}
					}

					if (node->importance > importance) {
						importance = node->importance;
					}
				}
			}

			if (winner > -1) {
				node = node_list[winner];
			} else {
				node = NULL;
			}

			if (node) {
				if (custom_pop) {
					for (x = 0; x < MAX_PRI; x++) {
						if (switch_queue_trypop(node->fifo_list[pop_array[x]], &pop) == SWITCH_STATUS_SUCCESS && pop) {
							break;
						}
					}
				} else {
					for (x = 0; x < MAX_PRI; x++) {
						if (switch_queue_trypop(node->fifo_list[x], &pop) == SWITCH_STATUS_SUCCESS && pop) {
							break;
						}
					}
				}

				if (pop && !node_consumer_wait_count(node)) {
					switch_mutex_lock(node->mutex);
					node->start_waiting = 0;
					switch_mutex_unlock(node->mutex);
				}
			}

			if (!pop) {
				if (!do_wait) {
					break;
				}

				status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);

				if (!SWITCH_READ_ACCEPTABLE(status)) {
					break;
				}

				continue;
			}

			uuid = (char *) pop;
			pop = NULL;

			if (node && (other_session = switch_core_session_locate(uuid))) {
				switch_channel_t *other_channel = switch_core_session_get_channel(other_session);
				switch_caller_profile_t *cloned_profile;
				const char *o_announce = NULL;
				const char *record_template = switch_channel_get_variable(channel, "fifo_record_template");
				char *expanded = NULL;

				if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
					switch_channel_event_set_data(other_channel, event);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", "caller_pop");
					switch_event_fire(&event);
				}

				if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
					switch_channel_event_set_data(channel, event);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", "consumer_pop");
					switch_event_fire(&event);
				}

				if ((o_announce = switch_channel_get_variable(other_channel, "fifo_override_announce"))) {
					announce = o_announce;
				}

				if (announce) {
					switch_ivr_play_file(session, NULL, announce, NULL);
				} else {
					switch_ivr_sleep(session, 500, SWITCH_TRUE, NULL);
				}

				switch_channel_set_variable(other_channel, "fifo_serviced_by", my_id);
				switch_channel_set_variable(other_channel, "fifo_serviced_uuid", switch_core_session_get_uuid(session));

				switch_channel_set_flag(other_channel, CF_BREAK);

				while (switch_channel_ready(channel) && switch_channel_ready(other_channel) && switch_channel_test_app_flag(other_channel, CF_APP_TAGGED)) {
					status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);
					if (!SWITCH_READ_ACCEPTABLE(status)) {
						break;
					}
				}

				if (!(switch_channel_ready(channel))) {
					const char *app = switch_channel_get_variable(other_channel, "current_application");
					const char *arg = switch_channel_get_variable(other_channel, "current_application_data");
					switch_caller_extension_t *extension = NULL;
					
					switch_mutex_lock(node->mutex);
					node->caller_count--;
					switch_core_hash_delete(node->caller_hash, uuid);
					switch_mutex_unlock(node->mutex);
					send_presence(node);
					

					if (app) {
						extension = switch_caller_extension_new(other_session, app, arg);
						switch_caller_extension_add_application(other_session, extension, app, arg);
						switch_channel_set_caller_extension(other_channel, extension);
						switch_channel_set_state(other_channel, CS_EXECUTE);
					} else {
						switch_channel_hangup(other_channel, SWITCH_CAUSE_NORMAL_CLEARING);
					}

					switch_core_session_rwunlock(other_session);
					break;
				}

				switch_channel_answer(channel);
				cloned_profile = switch_caller_profile_clone(other_session, switch_channel_get_caller_profile(channel));
				switch_assert(cloned_profile);
				switch_channel_set_originator_caller_profile(other_channel, cloned_profile);

				cloned_profile = switch_caller_profile_clone(session, switch_channel_get_caller_profile(other_channel));
				switch_assert(cloned_profile);
				switch_assert(cloned_profile->next == NULL);
				switch_channel_set_originatee_caller_profile(channel, cloned_profile);

				ts = switch_micro_time_now();
				switch_time_exp_lt(&tm, ts);
				switch_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d %T", &tm);
				switch_channel_set_variable(channel, "fifo_status", "TALKING");
				switch_channel_set_variable(channel, "fifo_target", uuid);
				switch_channel_set_variable(channel, "fifo_timestamp", date);

				switch_channel_set_variable(other_channel, "fifo_status", "TALKING");
				switch_channel_set_variable(other_channel, "fifo_timestamp", date);
				switch_channel_set_variable(other_channel, "fifo_target", switch_core_session_get_uuid(session));

				send_presence(node);

				if (record_template) {
					expanded = switch_channel_expand_variables(other_channel, record_template);
					switch_ivr_record_session(session, expanded, 0, NULL);
				}

				switch_core_media_bug_resume(session);
				switch_core_media_bug_resume(other_session);
				switch_ivr_multi_threaded_bridge(session, other_session, on_dtmf, other_session, session);
				switch_core_media_bug_pause(session);
				switch_core_media_bug_pause(other_session);
				
				if (record_template) {
					switch_ivr_stop_record_session(session, expanded);
					if (expanded != record_template) {
						switch_safe_free(expanded);
					}
				}

				ts = switch_micro_time_now();
				switch_time_exp_lt(&tm, ts);
				switch_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d %T", &tm);
				switch_channel_set_variable(channel, "fifo_status", "WAITING");
				switch_channel_set_variable(channel, "fifo_timestamp", date);

				switch_channel_set_variable(other_channel, "fifo_status", "DONE");
				switch_channel_set_variable(other_channel, "fifo_timestamp", date);

				switch_mutex_lock(node->mutex);
				node->caller_count--;
				switch_core_hash_delete(node->caller_hash, uuid);
				switch_mutex_unlock(node->mutex);
				send_presence(node);
				switch_core_session_rwunlock(other_session);

				if (!do_wait) {
					done = 1;
				}

				fifo_consumer_wrapup_sound = switch_channel_get_variable(channel, "fifo_consumer_wrapup_sound");
				fifo_consumer_wrapup_key = switch_channel_get_variable(channel, "fifo_consumer_wrapup_key");
				sfifo_consumer_wrapup_time = switch_channel_get_variable(channel, "fifo_consumer_wrapup_time");
				if (!switch_strlen_zero(sfifo_consumer_wrapup_time)){
					fifo_consumer_wrapup_time = atoi(sfifo_consumer_wrapup_time);
				} else {
					fifo_consumer_wrapup_time = 5000;
				}

				memset(buf, 0, sizeof(buf));

				if (fifo_consumer_wrapup_time || !switch_strlen_zero(fifo_consumer_wrapup_key)) {
					switch_channel_set_variable(channel, "fifo_status", "WRAPUP");
					if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
						switch_channel_event_set_data(channel, event);
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", "consumer_wrapup");
						switch_event_fire(&event);
					}
				}

				if (!switch_strlen_zero(fifo_consumer_wrapup_sound)) {
					memset(&args, 0, sizeof(args));
					args.buf = buf;
					args.buflen = sizeof(buf);
					switch_ivr_play_file(session, NULL, fifo_consumer_wrapup_sound, &args);
				}
				
				if (fifo_consumer_wrapup_time) {
					wrapup_time_started = switch_micro_time_now();
				}

				if (!switch_strlen_zero(fifo_consumer_wrapup_key) && strcmp(buf, fifo_consumer_wrapup_key)) {
					while(switch_channel_ready(channel)) {
						char terminator = 0;
						
						if (fifo_consumer_wrapup_time) {
							wrapup_time_elapsed = (switch_micro_time_now() - wrapup_time_started) / 1000;
							if (wrapup_time_elapsed > fifo_consumer_wrapup_time) {
								break;
							} else {
								wrapup_time_remaining = fifo_consumer_wrapup_time - wrapup_time_elapsed + 100;
							}
						}

						switch_ivr_collect_digits_count(session, buf, sizeof(buf) - 1, 1, fifo_consumer_wrapup_key, &terminator, 0, 0, (uint32_t) wrapup_time_remaining);
						if ((terminator == *fifo_consumer_wrapup_key) || !(switch_channel_ready(channel))) {
							break;
						}

					}
				} else if (fifo_consumer_wrapup_time && (switch_strlen_zero(fifo_consumer_wrapup_key) || !strcmp(buf, fifo_consumer_wrapup_key))) {
					while(switch_channel_ready(channel)) {
						wrapup_time_elapsed = (switch_micro_time_now() - wrapup_time_started) / 1000;
						if (wrapup_time_elapsed > fifo_consumer_wrapup_time) {
							break;
						}
						switch_yield(500);
					}
				}
				switch_channel_set_variable(channel, "fifo_status", "WAITING");
			}

			switch_safe_free(uuid);

			if (done) {
				break;
			}

			if (do_wait && switch_channel_ready(channel)) {
				if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
					switch_channel_event_set_data(channel, event);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", "consumer_reentrance");
					switch_event_fire(&event);
				}
			}
		}

		if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FIFO_EVENT) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(channel, event);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Name", argv[0]);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "FIFO-Action", "consumer_stop");
			switch_event_fire(&event);
		}

		if (do_wait) {
			for (i = 0; i < node_count; i++) {
				if (!(node = node_list[i])) {
					continue;
				}
				switch_mutex_lock(node->mutex);
				switch_core_hash_delete(node->consumer_hash, switch_core_session_get_uuid(session));
				node->consumer_count--;
				switch_mutex_unlock(node->mutex);
			}
		}
	}

 done:

	switch_core_media_bug_resume(session);

}

struct xml_helper {
	switch_xml_t xml;
	fifo_node_t *node;
	char *container;
	char *tag;
	int cc_off;
	int verbose;
};

static int xml_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct xml_helper *h = (struct xml_helper *) pArg;
	switch_xml_t x_tmp, x_out;
	int c_off = 0;
	char exp_buf[128] = "";
	switch_time_exp_t tm;
	switch_time_t etime = 0;
	char *expires = exp_buf;

	if (argv[7]) {
		if ((etime = atol(argv[7]))) {
			switch_size_t retsize;

			switch_time_exp_lt(&tm, switch_time_from_sec(etime));
			switch_strftime_nocheck(exp_buf, &retsize, sizeof(exp_buf), "%Y-%m-%d %T", &tm);
		} else {
			switch_set_string(exp_buf, "now");
		}
	}

	x_tmp = switch_xml_add_child_d(h->xml, h->container, h->cc_off++);

	x_out = switch_xml_add_child_d(x_tmp, h->tag, c_off++);
	switch_xml_set_attr_d(x_out, "timeout", argv[5]);
	switch_xml_set_attr_d(x_out, "simo", argv[3]);
	switch_xml_set_attr_d(x_out, "lag", argv[6]);
	switch_xml_set_attr_d(x_out, "outbound-call-count", argv[10]);
	switch_xml_set_attr_d(x_out, "outbound-fail-count", argv[11]);
	switch_xml_set_attr_d(x_out, "next-available", expires);
	
	switch_xml_set_txt_d(x_out, argv[2]);

	return 0;
}

static int xml_outbound(switch_xml_t xml, fifo_node_t *node, char *container, char *tag, int cc_off, int verbose)
{
	struct xml_helper h;
	char *sql = "select uuid, fifo_name, originate_string, simo_count, use_count, timeout, lag, next_avail, expires, static, outbound_call_count, outbound_fail_count, hostname from fifo_outbound";

	h.xml = xml;
	h.node = node;
	h.container = container;
	h.tag = tag;
	h.cc_off = cc_off;
	h.verbose = verbose;

	fifo_execute_sql_callback(globals.sql_mutex, sql, xml_callback, &h);
	
	return h.cc_off;
}

static int xml_hash(switch_xml_t xml, switch_hash_t *hash, char *container, char *tag, int cc_off, int verbose)
{
	switch_xml_t x_tmp, x_caller, x_cp, variables;
	switch_hash_index_t *hi;
	switch_core_session_t *session;
	switch_channel_t *channel;
	void *val;
	const void *var;

	x_tmp = switch_xml_add_child_d(xml, container, cc_off++);
	switch_assert(x_tmp);

	for (hi = switch_hash_first(NULL, hash); hi; hi = switch_hash_next(hi)) {
		int c_off = 0, d_off = 0;
		const char *status;
		const char *ts;

		switch_hash_this(hi, &var, NULL, &val);
		session = (switch_core_session_t *) val;
		channel = switch_core_session_get_channel(session);
		x_caller = switch_xml_add_child_d(x_tmp, tag, c_off++);
		switch_assert(x_caller);

		switch_xml_set_attr_d(x_caller, "uuid", switch_core_session_get_uuid(session));

		if ((status = switch_channel_get_variable(channel, "fifo_status"))) {
			switch_xml_set_attr_d(x_caller, "status", status);
		}

		if ((ts = switch_channel_get_variable(channel, "fifo_timestamp"))) {
			switch_xml_set_attr_d(x_caller, "timestamp", ts);
		}

		if ((ts = switch_channel_get_variable(channel, "fifo_target"))) {
			switch_xml_set_attr_d(x_caller, "target", ts);
		}

		if (!(x_cp = switch_xml_add_child_d(x_caller, "caller_profile", d_off++))) {
			abort();
		}
		if (verbose) {
			d_off += switch_ivr_set_xml_profile_data(x_cp, switch_channel_get_caller_profile(channel), d_off);

			if (!(variables = switch_xml_add_child_d(x_caller, "variables", c_off++))) {
				abort();
			}

			switch_ivr_set_xml_chan_vars(variables, channel, c_off);
		}
	}

	return cc_off;
}

static void list_node(fifo_node_t *node, switch_xml_t x_report, int *off, int verbose)
{
	switch_xml_t x_fifo;
	int cc_off = 0;
	char buffer[35];
	char *tmp = buffer;

	x_fifo = switch_xml_add_child_d(x_report, "fifo", (*off)++);;
	switch_assert(x_fifo);

	switch_xml_set_attr_d(x_fifo, "name", node->name);
	switch_snprintf(tmp, sizeof(buffer), "%d", node->consumer_count);
	switch_xml_set_attr_d(x_fifo, "consumer_count", tmp);
	switch_snprintf(tmp, sizeof(buffer), "%d", node->caller_count);
	switch_xml_set_attr_d(x_fifo, "caller_count", tmp);
	switch_snprintf(tmp, sizeof(buffer), "%d", node_consumer_wait_count(node));
	switch_xml_set_attr_d(x_fifo, "waiting_count", tmp);
	switch_snprintf(tmp, sizeof(buffer), "%u", node->importance);
	switch_xml_set_attr_d(x_fifo, "importance", tmp);
	
	cc_off = xml_outbound(x_fifo, node, "outbound", "member", cc_off, verbose);
	cc_off = xml_hash(x_fifo, node->caller_hash, "callers", "caller", cc_off, verbose);
	cc_off = xml_hash(x_fifo, node->consumer_hash, "consumers", "consumer", cc_off, verbose);
}

#define FIFO_API_SYNTAX "list|list_verbose|count|importance [<fifo name>]|reparse [del_all]"
SWITCH_STANDARD_API(fifo_api_function)
{
	int len = 0;
	fifo_node_t *node;
	char *data = NULL;
	int argc = 0;
	char *argv[5] = { 0 };
	switch_hash_index_t *hi;
	void *val;
	const void *var;
	int x = 0, verbose = 0;

	if (!globals.running) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_strlen_zero(cmd)) {
		data = strdup(cmd);
		switch_assert(data);
	}

	if (switch_strlen_zero(cmd) || (argc = switch_separate_string(data, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) < 1 || !argv[0]) {
		stream->write_function(stream, "%s\n", FIFO_API_SYNTAX);
		return SWITCH_STATUS_SUCCESS;
	}

	switch_mutex_lock(globals.mutex);
	verbose = !strcasecmp(argv[0], "list_verbose");

	if (!strcasecmp(argv[0], "reparse")) {
		load_config(1, argv[1] && !strcasecmp(argv[1], "del_all"));
		goto done;
	}

	if (!strcasecmp(argv[0], "list") || verbose) {
		char *xml_text = NULL;
		switch_xml_t x_report = switch_xml_new("fifo_report");
		switch_assert(x_report);

		if (argc < 2) {
			for (hi = switch_hash_first(NULL, globals.fifo_hash); hi; hi = switch_hash_next(hi)) {
				switch_hash_this(hi, &var, NULL, &val);
				node = (fifo_node_t *) val;
				switch_mutex_lock(node->mutex);
				list_node(node, x_report, &x, verbose);
				switch_mutex_unlock(node->mutex);
			}
		} else {
			if ((node = switch_core_hash_find(globals.fifo_hash, argv[1]))) {
				switch_mutex_lock(node->mutex);
				list_node(node, x_report, &x, verbose);
				switch_mutex_unlock(node->mutex);
			}
		}
		xml_text = switch_xml_toxml(x_report, SWITCH_FALSE);
		switch_assert(xml_text);
		stream->write_function(stream, "%s\n", xml_text);
		switch_xml_free(x_report);
		switch_safe_free(xml_text);

	} else if (!strcasecmp(argv[0], "importance")) {
		if (argv[1] && (node = switch_core_hash_find(globals.fifo_hash, argv[1]))) {
			int importance = 0;
			if (argc > 2) {
				importance = atoi(argv[2]);
				if (importance < 0) {
					importance = 0;
				}
				node->importance = importance;
			}
			stream->write_function(stream, "importance: %u\n", node->importance);
		} else {
			stream->write_function(stream, "no fifo by that name\n");
		}
	} else if (!strcasecmp(argv[0], "count")) {
		if (argc < 2) {
			for (hi = switch_hash_first(NULL, globals.fifo_hash); hi; hi = switch_hash_next(hi)) {
				switch_hash_this(hi, &var, NULL, &val);
				node = (fifo_node_t *) val;
				len = node_consumer_wait_count(node);
				switch_mutex_lock(node->mutex);
				stream->write_function(stream, "%s:%d:%d:%d\n", (char *) var, node->consumer_count, node->caller_count, len);
				switch_mutex_unlock(node->mutex);
				x++;
			}

			if (!x) {
				stream->write_function(stream, "none\n");
			}
		} else if ((node = switch_core_hash_find(globals.fifo_hash, argv[1]))) {
			len = node_consumer_wait_count(node);
			switch_mutex_lock(node->mutex);
			stream->write_function(stream, "%s:%d:%d:%d\n", argv[1], node->consumer_count, node->caller_count, len);
			switch_mutex_unlock(node->mutex);
		} else {
			stream->write_function(stream, "none\n");
		}
	} else {
		stream->write_function(stream, "-ERR Usage: %s\n", FIFO_API_SYNTAX);
	}

  done:

	switch_mutex_unlock(globals.mutex);
	return SWITCH_STATUS_SUCCESS;
}


const char outbound_sql[] = 
	"create table fifo_outbound (\n"
	" uuid varchar(255),\n"
	" fifo_name varchar(255),\n"
	" originate_string varchar(255),\n"
	" simo_count integer,\n"
	" use_count integer,\n"
	" timeout integer,\n"
	" lag integer,\n"
	" next_avail integer,\n"
	" expires integer,\n"
	" static integer,\n"
	" outbound_call_count integer,"
	" outbound_fail_count integer,"
	" hostname varchar(255)\n"
	");\n";


static switch_status_t load_config(int reload, int del_all)
{
	char *cf = "fifo.conf";
	switch_xml_t cfg, xml, fifo, fifos, member, settings, param;
	char *odbc_user = NULL;
	char *odbc_pass = NULL;
	switch_core_db_t *db;
	switch_status_t status = SWITCH_STATUS_SUCCESS;	
	char *sql;

	gethostname(globals.hostname, sizeof(globals.hostname));

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}

	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = NULL;
			char *val = NULL;

			var = (char *) switch_xml_attr_soft(param, "name");
			val = (char *) switch_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "odbc-dsn") && !switch_strlen_zero(val)) {
#ifdef SWITCH_HAVE_ODBC
				globals.odbc_dsn = switch_core_strdup(globals.pool, val);
				if ((odbc_user = strchr(globals.odbc_dsn, ':'))) {
					*odbc_user++ = '\0';
					if ((odbc_pass = strchr(odbc_user, ':'))) {
						*odbc_pass++ = '\0';
					}
				}
#else
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ODBC IS NOT AVAILABLE!\n");
#endif
			}
		}
	}


	if (switch_strlen_zero(globals.odbc_dsn) || switch_strlen_zero(odbc_user) || switch_strlen_zero(odbc_pass)) {
		globals.dbname = "fifo";
	}

#ifdef SWITCH_HAVE_ODBC
	if (globals.odbc_dsn) {
		if (!(globals.master_odbc = switch_odbc_handle_new(globals.odbc_dsn, odbc_user, odbc_pass))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
			status = SWITCH_STATUS_FALSE;
			goto done;
		}
		if (switch_odbc_handle_connect(globals.master_odbc) != SWITCH_ODBC_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
			status = SWITCH_STATUS_FALSE;
			goto done;
		}

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Connected ODBC DSN: %s\n", globals.odbc_dsn);
		if (switch_odbc_handle_exec(globals.master_odbc, "delete from fifo_outbound", NULL) != SWITCH_STATUS_SUCCESS) {
			if (switch_odbc_handle_exec(globals.master_odbc, (char *)outbound_sql, NULL) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Create SQL Database!\n");
			}
		}
	} else {
#endif
		if ((db = switch_core_db_open_file(globals.dbname))) {
			switch_core_db_test_reactive(db, "delete from fifo_outbound", NULL, (char *)outbound_sql);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open SQL Database!\n");
			status = SWITCH_STATUS_FALSE;
			goto done;
		}
		switch_core_db_close(db);
#ifdef SWITCH_HAVE_ODBC
	}
#endif

	if (reload) {
		switch_hash_index_t *hi;
		fifo_node_t *node;
		void *val;
		for (hi = switch_hash_first(NULL, globals.fifo_hash); hi; hi = switch_hash_next(hi)) {
			switch_hash_this(hi, NULL, NULL, &val);
			if ((node = (fifo_node_t *) val)) {
				node->ready = 0;
			}
		}
	}

	if (del_all) {
		sql = switch_mprintf("delete from fifo_outbound where hostname='%q'", globals.hostname);
	} else {
		sql = switch_mprintf("delete from fifo_outbound where static=1 and hostname='%q'", globals.hostname);
	}
	
	fifo_execute_sql(sql, globals.sql_mutex);
	switch_safe_free(sql);



	if ((fifos = switch_xml_child(cfg, "fifos"))) {
		for (fifo = switch_xml_child(fifos, "fifo"); fifo; fifo = fifo->next) {
			const char *name;
			const char *importance;
			int imp = 0;
			int simo_i = 1;
			int timeout_i = 60;
			int lag_i = 10;
			fifo_node_t *node;
			
			name = switch_xml_attr(fifo, "name");
			
			if ((importance = switch_xml_attr(fifo, "importance"))) {
				if ((imp = atoi(importance)) < 0) {
					imp = 0;
				}
			}				

			if (!name) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "fifo has no name!\n");
				continue;
			}

			switch_mutex_lock(globals.mutex);
			if (!(node = switch_core_hash_find(globals.fifo_hash, name))) {
				node = create_node(name, imp);
			}
			switch_mutex_unlock(globals.mutex);
			
			switch_assert(node);
			
			switch_mutex_lock(node->mutex);
			
			for (member = switch_xml_child(fifo, "member"); member; member = member->next) {
				const char *simo = switch_xml_attr_soft(member, "simo");
				const char *lag = switch_xml_attr_soft(member, "lag");
				const char *timeout = switch_xml_attr_soft(member, "timeout");
				char digest[SWITCH_MD5_DIGEST_STRING_SIZE] = { 0 };
				switch_md5_string(digest, (void *) member->txt, strlen(member->txt));

				if (simo) {
					simo_i = atoi(simo);
				}

				if (timeout) {
					if ((timeout_i = atoi(timeout)) < 10) {
						timeout_i = 60;
					}

				}

				if (lag) {
					if ((lag_i = atoi(lag)) < 0) {
						lag_i = 10;
					}
				}
				
				
				sql = switch_mprintf("insert into fifo_outbound "
									 "(uuid, fifo_name, originate_string, simo_count, use_count, timeout, lag, "
									 "next_avail, expires, static, outbound_call_count, outbound_fail_count, hostname) "
									 "values ('%q','%q','%q',%d,%d,%d,%d,0,0,1,0,0,'%q')",
									 digest, node->name, member->txt, simo_i, 0, timeout_i, lag_i, globals.hostname
									 );
				switch_assert(sql);
				fifo_execute_sql(sql, globals.sql_mutex);
				free(sql);
				
				node->has_outbound = 1;
				
			}
			node->ready = 1;
			switch_mutex_unlock(node->mutex);
			
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s configured\n", node->name);
		
		}
	}
	switch_xml_free(xml);

  done:

	if (reload) {
		switch_hash_index_t *hi;
		void *val, *pop;
		fifo_node_t *node;
		for (hi = switch_hash_first(NULL, globals.fifo_hash); hi; hi = switch_hash_next(hi)) {
			int x = 0;
			switch_hash_this(hi, NULL, NULL, &val);
			if (!(node = (fifo_node_t *) val) || node->ready) {
				continue;
			}


			if (node_consumer_wait_count(node) || node->consumer_count || node_idle_consumers(node)) {
				node->ready = 1;
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s not removed, still in use.\n", node->name);
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "%s removed.\n", node->name);
				switch_thread_rwlock_wrlock(node->rwlock);
				for (x = 0; x < MAX_PRI; x++) {
					while (switch_queue_trypop(node->fifo_list[x], &pop) == SWITCH_STATUS_SUCCESS) {
						free(pop);
					}
				}
			
				switch_core_hash_destroy(&node->caller_hash);
				switch_core_hash_destroy(&node->consumer_hash);
				switch_thread_rwlock_unlock(node->rwlock);
			}
		}
	}



	return status;
}


static void fifo_member_add(char *fifo_name, char *originate_string, int simo_count, int timeout, int lag, time_t expires)
{
	char digest[SWITCH_MD5_DIGEST_STRING_SIZE] = { 0 };
	char *sql;
	fifo_node_t *node = NULL;
	
	switch_md5_string(digest, (void *) originate_string, strlen(originate_string));
	
	sql = switch_mprintf("delete from fifo_outbound where fifo_name='%q' and uuid = '%q'", fifo_name, digest);
	switch_assert(sql);
	fifo_execute_sql(sql, globals.sql_mutex);
	free(sql);
	
	
	switch_mutex_lock(globals.mutex);
	if (!(node = switch_core_hash_find(globals.fifo_hash, fifo_name))) {
		node = create_node(fifo_name, 0);
	}
	switch_mutex_unlock(globals.mutex);

	node->has_outbound = 1;	

	sql = switch_mprintf("insert into fifo_outbound "
						 "(uuid, fifo_name, originate_string, simo_count, use_count, timeout, lag, next_avail, expires, static, outbound_call_count, outbound_fail_count, hostname) "
						 "values ('%q','%q','%q',%d,%d,%d,%d,%d,%ld,0,0,0,'%q')",
						 digest, fifo_name, originate_string, simo_count, 0, timeout, lag, 0, (long)expires, globals.hostname
						 );
	switch_assert(sql);
	fifo_execute_sql(sql, globals.sql_mutex);
	free(sql);
	
}

static void fifo_member_del(char *fifo_name, char *originate_string)
{
	char digest[SWITCH_MD5_DIGEST_STRING_SIZE] = { 0 };
	char *sql;

	switch_md5_string(digest, (void *) originate_string, strlen(originate_string));
	
	sql = switch_mprintf("delete from fifo_outbound where fifo_name='%q' and uuid = '%q' and hostname='%q'", fifo_name, digest, globals.hostname);
	switch_assert(sql);
	fifo_execute_sql(sql, globals.sql_mutex);
	free(sql);
}

#define FIFO_MEMBER_API_SYNTAX "[add <fifo_name> <originate_string> [<simo_count>] [<timeout>] [<lag>] | del <fifo_name> <originate_string>]"
SWITCH_STANDARD_API(fifo_member_api_function)
{
	char *fifo_name;
	char *originate_string;
	int simo_count = 1;
	int timeout = 60;
	int lag = 5;
	char *action;
	char *mydata = NULL, *argv[8] = { 0 };
	int argc;
	time_t expires = 0;

	if (!globals.running) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_strlen_zero(cmd)){
		stream->write_function(stream, "-USAGE: %s\n", FIFO_MEMBER_API_SYNTAX);
		return SWITCH_STATUS_SUCCESS;
	}

	mydata = strdup(cmd);
	switch_assert(mydata);

	argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc < 3) {
		stream->write_function(stream, "%s", "-ERR Invalid!\n");
		goto done;
	}
	
	action = argv[0];
	fifo_name = argv[1];
	originate_string = argv[2];
	
	if (action && !strcasecmp(action, "add")) {
		if (argc > 3) {
			simo_count = atoi(argv[3]);
		}
		if (argc > 4) {
			timeout = atoi(argv[4]);
		}
		if (argc > 5) {
			lag = atoi(argv[5]);
		}
		if (argc > 6) {
			expires = switch_epoch_time_now(NULL) + atoi(argv[6]);
		}
		if (simo_count < 0) {
			simo_count = 1;
		}
		if (timeout < 0) {
			timeout = 60;
		}
		if (lag < 0) {
			lag = 5;
		}
		
		fifo_member_add(fifo_name, originate_string, simo_count, timeout, lag, expires);
		stream->write_function(stream, "%s", "+OK\n");
	} else if (action && !strcasecmp(action, "del")) {
		fifo_member_del(fifo_name, originate_string);
		stream->write_function(stream, "%s", "+OK\n");
	} else {
		stream->write_function(stream, "%s", "-ERR Invalid!\n");
		goto done;
	}

  done:

	free(mydata);

	return SWITCH_STATUS_SUCCESS;

}

SWITCH_MODULE_LOAD_FUNCTION(mod_fifo_load)
{
	switch_application_interface_t *app_interface;
	switch_api_interface_t *commands_api_interface;

	/* create/register custom event message type */
	if (switch_event_reserve_subclass(FIFO_EVENT) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!", FIFO_EVENT);
		return SWITCH_STATUS_TERM;
	}

	/* Subscribe to presence request events */
	if (switch_event_bind_removable(modname, SWITCH_EVENT_PRESENCE_PROBE, SWITCH_EVENT_SUBCLASS_ANY, 
									pres_event_handler, NULL, &globals.node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't subscribe to presence request events!\n");
		return SWITCH_STATUS_GENERR;
	}

	switch_core_new_memory_pool(&globals.pool);
	switch_core_hash_init(&globals.fifo_hash, globals.pool);
	switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, globals.pool);
	switch_mutex_init(&globals.sql_mutex, SWITCH_MUTEX_NESTED, globals.pool);

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	SWITCH_ADD_APP(app_interface, "fifo", "Park with FIFO", FIFO_DESC, fifo_function, FIFO_USAGE, SAF_NONE);
	SWITCH_ADD_API(commands_api_interface, "fifo", "Return data about a fifo", fifo_api_function, FIFO_API_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "fifo_member", "Add members to a fifo", fifo_member_api_function, FIFO_MEMBER_API_SYNTAX);
	switch_console_set_complete("add fifo list");
	switch_console_set_complete("add fifo list_verbose");
	switch_console_set_complete("add fifo count");
	switch_console_set_complete("add fifo importance");

	globals.running = 1;

	load_config(0,1);
	start_node_thread(globals.pool);

	return SWITCH_STATUS_SUCCESS;
}

/*
  Called when the system shuts down 
*/
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_fifo_shutdown)
{
	switch_hash_index_t *hi;
	void *val, *pop;
	fifo_node_t *node;
	switch_memory_pool_t *pool = globals.pool;
	switch_mutex_t *mutex = globals.mutex;
	
	switch_event_unbind(&globals.node);
	switch_event_free_subclass(FIFO_EVENT);
	
	switch_mutex_lock(mutex);

	globals.running = 0;
	/* Cleanup */
	
	if (globals.node_thread_running) {
		stop_node_thread();
	}
		

	for (hi = switch_hash_first(NULL, globals.fifo_hash); hi; hi = switch_hash_next(hi)) {
		int x = 0;
		switch_hash_this(hi, NULL, NULL, &val);
		node = (fifo_node_t *) val;
		
		switch_thread_rwlock_wrlock(node->rwlock);
		for (x = 0; x < MAX_PRI; x++) {
			while (switch_queue_trypop(node->fifo_list[x], &pop) == SWITCH_STATUS_SUCCESS) {
				free(pop);
			}
		}

		switch_core_hash_destroy(&node->caller_hash);
		switch_core_hash_destroy(&node->consumer_hash);
		switch_thread_rwlock_unlock(node->rwlock);
	}
	switch_core_hash_destroy(&globals.fifo_hash);
	memset(&globals, 0, sizeof(globals));
	switch_mutex_unlock(mutex);
	switch_core_destroy_memory_pool(&pool);
	return SWITCH_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
