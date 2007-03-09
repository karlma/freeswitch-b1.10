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
 *
 *
 * mod_dingaling.c -- Jingle Endpoint Module
 *
 */
#include <switch.h>
#include <libdingaling.h>

#define DL_CAND_WAIT 10000000
#define DL_CAND_INITIAL_WAIT 2000000

#define DL_EVENT_LOGIN_SUCCESS "dingaling::login_success"
#define DL_EVENT_LOGIN_FAILURE "dingaling::login_failure"
#define DL_EVENT_CONNECTED "dingaling::connected"
#define MDL_CHAT_PROTO "jingle"

static const char modname[] = "mod_dingaling";

static switch_memory_pool_t *module_pool = NULL;

static char sub_sql[] =
"CREATE TABLE subscriptions (\n"
"   sub_from            VARCHAR(255),\n"
"   sub_to          VARCHAR(255),\n"
"   show          VARCHAR(255),\n"
"   status          VARCHAR(255)\n"
");\n";


typedef enum {
	TFLAG_IO = (1 << 0),
	TFLAG_INBOUND = (1 << 1),
	TFLAG_OUTBOUND = (1 << 2),
	TFLAG_READING = (1 << 3),
	TFLAG_WRITING = (1 << 4),
	TFLAG_BYE = (1 << 5),
	TFLAG_VOICE = (1 << 6),
	TFLAG_RTP_READY = (1 << 7),
	TFLAG_CODEC_READY = (1 << 8),
	TFLAG_TRANSPORT = (1 << 9),
	TFLAG_ANSWER = (1 << 10),
	TFLAG_VAD_IN = ( 1 << 11),
	TFLAG_VAD_OUT = ( 1 << 12),
	TFLAG_VAD = ( 1 << 13),
	TFLAG_DO_CAND = ( 1 << 14),
	TFLAG_DO_DESC = (1 << 15),
	TFLAG_LANADDR = (1 << 16),
	TFLAG_AUTO = (1 << 17),
	TFLAG_DTMF = (1 << 18),
	TFLAG_TIMER = ( 1 << 19),
	TFLAG_TERM = ( 1 << 20),
	TFLAG_TRANSPORT_ACCEPT = (1 << 21),
	TFLAG_READY = (1 << 22),
} TFLAGS;

typedef enum {
	GFLAG_MY_CODEC_PREFS = (1 << 0)
} GFLAGS;

static struct {
	int debug;
	char *dialplan;
	char *codec_string;
	char *codec_order[SWITCH_MAX_CODECS];
	int codec_order_last;
	char *codec_rates_string;
	char *codec_rates[SWITCH_MAX_CODECS];
	int codec_rates_last;
	unsigned int flags;
	unsigned int init;
	switch_hash_t *profile_hash;
	int running;
	int handles;
    char guess_ip[80];
} globals;

struct mdl_profile {
    char *name;
    char *login;
    char *password;
    char *message;
	char *auto_reply;
    char *dialplan;
    char *ip;
    char *extip;
    char *lanaddr;
	char *server;
    char *exten;
    char *context;
	char *timer_name;
	char *dbname;
	switch_mutex_t *mutex;
    ldl_handle_t *handle;
    uint32_t flags;
    uint32_t user_flags;
};

struct private_object {
	unsigned int flags;
	switch_codec_t read_codec;
	switch_codec_t write_codec;
	switch_frame_t read_frame;
	struct mdl_profile *profile;
	switch_core_session_t *session;
	switch_caller_profile_t *caller_profile;
	unsigned short samprate;
	switch_mutex_t *mutex;
	const switch_codec_implementation_t *codecs[SWITCH_MAX_CODECS];
	unsigned int num_codecs;
	int codec_index;
	switch_rtp_t *rtp_session;
	ldl_session_t *dlsession;
	char *remote_ip;
	switch_port_t local_port;
	switch_port_t remote_port;
	char local_user[17];
	char local_pass[17];
	char *remote_user;
	char *us;
	char *them;
	unsigned int cand_id;
	unsigned int desc_id;
	unsigned int dc;
	uint32_t timestamp_send;
	int32_t timestamp_recv;
	uint32_t last_read;
	char *codec_name;
	switch_payload_t codec_num;
	switch_payload_t r_codec_num;
	uint32_t codec_rate;
	switch_time_t next_desc;
	switch_time_t next_cand;
	char *stun_ip;
	char *recip;
	char *dnis;
	uint16_t stun_port;
	switch_mutex_t *flag_mutex;
};

struct rfc2833_digit {
	char digit;
	int duration;
};


SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_dialplan, globals.dialplan)
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_codec_string, globals.codec_string)
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_codec_rates_string, globals.codec_rates_string)

static switch_status_t dl_login(char *arg, switch_core_session_t *session, switch_stream_handle_t *stream);
static switch_status_t dl_logout(char *profile_name, switch_core_session_t *session, switch_stream_handle_t *stream);
static switch_status_t channel_on_init(switch_core_session_t *session);
static switch_status_t channel_on_hangup(switch_core_session_t *session);
static switch_status_t channel_on_ring(switch_core_session_t *session);
static switch_status_t channel_on_loopback(switch_core_session_t *session);
static switch_status_t channel_on_transmit(switch_core_session_t *session);
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session, switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool);
static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, int timeout,
										  switch_io_flag_t flags, int stream_id);
static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, int timeout,
										   switch_io_flag_t flags, int stream_id);
static switch_status_t channel_kill_channel(switch_core_session_t *session, int sig);

static ldl_status handle_signalling(ldl_handle_t *handle, ldl_session_t *dlsession, ldl_signal_t dl_signal, char *to, char *from, char *subject, char *msg);
static ldl_status handle_response(ldl_handle_t *handle, char *id);
static switch_status_t load_config(void);
static int sin_callback(void *pArg, int argc, char **argv, char **columnNames);

#define is_special(s) (s && (strstr(s, "ext+") || strstr(s, "user+")))

static char *translate_rpid(char *in, char *ext)
{
	char *r = NULL;

	if (in && (strstr(in, "null") || strstr(in, "NULL"))) {
		in = NULL;
	}

	if (!in) {
		in = ext;
	}

	if (!in) {
		return NULL;
	}
	
	if (!strcasecmp(in, "busy")) {
		r = "dnd";
	}

	if (!strcasecmp(in, "unavailable")) {
		r = "dnd";
	}

	if (!strcasecmp(in, "idle")) {
		r = "away";
	}

	if (ext && !strcasecmp(ext, "idle")) {
		r = "away";
	} else if (ext && !strcasecmp(ext, "away")) {
		r = "away";
	}
	
	return r;
}

static int sub_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct mdl_profile *profile = (struct mdl_profile *) pArg;

	char *sub_from = argv[0];
	char *sub_to = argv[1];
	char *type = argv[2];
	char *rpid = argv[3];
	char *status = argv[4];
	//char *proto = argv[5];
	
	if (switch_strlen_zero(type)) {
		type = NULL;
	} else if (!strcasecmp(type, "unavailable")) {
		status = NULL;
	}
	rpid = translate_rpid(rpid, status);

	//ldl_handle_send_presence(profile->handle, sub_to, sub_from, "probe", rpid, status);
	ldl_handle_send_presence(profile->handle, sub_to, sub_from, type, rpid, status);


	return 0;
}

static int rost_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct mdl_profile *profile = (struct mdl_profile *) pArg;

	char *sub_from = argv[0];
	char *sub_to = argv[1];
	char *show = argv[2];
	char *status = argv[3];

	if (!strcasecmp(status, "n/a")) {
		if (!strcasecmp(show, "dnd")) {
			status = "Busy";
		} else if (!strcasecmp(show, "away")) {
			status = "Idle";
		}
	}

	ldl_handle_send_presence(profile->handle, sub_to, sub_from, NULL, show, status);

	return 0;
}

static void pres_event_handler(switch_event_t *event)
{
	struct mdl_profile *profile = NULL;
	switch_hash_index_t *hi;
    void *val;
	char *proto = switch_event_get_header(event, "proto");
	char *from = switch_event_get_header(event, "from");
	char *status= switch_event_get_header(event, "status");
	char *rpid = switch_event_get_header(event, "rpid");
	char *type = switch_event_get_header(event, "event_subtype");
	char *sql;
	switch_core_db_t *db;

	if (!proto) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Missing 'proto' header\n");
		return;
	}

	if (!from) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Missing 'from' header\n");
		return;
	}

	if (status && !strcasecmp(status, "n/a")) {
		status = NULL;
	}

	switch(event->event_id) {
	case SWITCH_EVENT_PRESENCE_PROBE: 
        if (proto) {
        	char *sql;
            switch_core_db_t *db;
            char *errmsg;
            char *to = switch_event_get_header(event, "to");
            char *f_host = NULL;
            if (to) {
                if ((f_host = strchr(to, '@'))) {
                    f_host++;
                }
            }

            if (f_host && (profile = switch_core_hash_find(globals.profile_hash, f_host))) {
                if (to && (sql = switch_mprintf("select * from subscriptions where sub_to='%q'", to))) {
                    if (!(db = switch_core_db_open_file(profile->dbname))) {
                        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
                        return;
                    }
                    switch_mutex_lock(profile->mutex);
                    switch_core_db_exec(db, sql, sin_callback, profile, &errmsg);
                    switch_mutex_unlock(profile->mutex);
                    switch_core_db_close(db);
                    switch_safe_free(sql);
                }
            }
        }
        return;
	case SWITCH_EVENT_PRESENCE_IN:
		if (!status) {
			status = "Available";
		}
		break;
	case SWITCH_EVENT_PRESENCE_OUT:
		type = "unavailable";
		break;
	default:
		break;
	}
	

	if (!type) {
		type = "";
	}
	if (!rpid) {
		rpid = "";
	}
	if (!status) {
		status = "Away";
	}
	

	sql = switch_mprintf("select sub_from, sub_to,'%q','%q','%q','%q' from subscriptions where sub_to like '%%%q'", 
						 type, rpid, status, proto, from);
	
	
	for (hi = switch_hash_first(switch_hash_pool_get(globals.profile_hash), globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		char *errmsg;
        switch_hash_this(hi, NULL, NULL, &val);
        profile = (struct mdl_profile *) val;

        if (!(profile->user_flags & LDL_FLAG_COMPONENT)) {
			continue;
        }


		if (sql) {
			if (!(db = switch_core_db_open_file(profile->dbname))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
				continue;
			}
			switch_mutex_lock(profile->mutex);
			switch_core_db_exec(db, sql, sub_callback, profile, &errmsg);
			switch_mutex_unlock(profile->mutex);
			switch_core_db_close(db);
		}
		

	}

	switch_safe_free(sql);
}

static switch_status_t chat_send(char *proto, char *from, char *to, char *subject, char *body, char *hint)
{
	char *user, *host, *f_user = NULL, *ffrom = NULL, *f_host = NULL, *f_resource = NULL;
	struct mdl_profile *profile = NULL;

	assert(proto != NULL);

	if (from && (f_user = strdup(from))) {
		if ((f_host = strchr(f_user, '@'))) {
			*f_host++ = '\0';
            if ((f_resource = strchr(f_host, '/'))) {
                *f_resource++ = '\0';
            }
		}
	}

	if (to && (user = strdup(to))) {
		if ((host = strchr(user, '@'))) {
			*host++ = '\0';
		}

		if (f_host && (profile = switch_core_hash_find(globals.profile_hash, f_host))) {
			
			if (!strcmp(proto, MDL_CHAT_PROTO)) {
				from = hint;
			} else {
				char *p;
				ffrom = switch_mprintf("%s+%s", proto, from);
				from = ffrom;
				if ((p = strchr(from, '/'))) {
					*p = '\0';
				}
			}
			ldl_handle_send_msg(profile->handle, from, to, NULL, body);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Profile %s\n", f_host ? f_host : "NULL");
			return SWITCH_STATUS_FALSE;
		}

		switch_safe_free(user);
		switch_safe_free(f_user);
	}

	return SWITCH_STATUS_SUCCESS;
}


static void roster_event_handler(switch_event_t *event)
{
	char *status= switch_event_get_header(event, "status");
	char *from= switch_event_get_header(event, "from");
	char *event_type = switch_event_get_header(event, "event_type");
	struct mdl_profile *profile = NULL;
	switch_hash_index_t *hi;
    void *val;
	char *sql;
	switch_core_db_t *db;

	if (status && !strcasecmp(status, "n/a")) {
		status = NULL;
	}

	if (switch_strlen_zero(event_type)) {
		event_type="presence";
	}

	if (from) {
		sql = switch_mprintf("select *,'%q' from subscriptions where sub_from='%q'", status ? status : "", from);
	} else {
		sql = switch_mprintf("select *,'%q' from subscriptions", status ? status : "");
	}

	for (hi = switch_hash_first(switch_hash_pool_get(globals.profile_hash), globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		char *errmsg;
        switch_hash_this(hi, NULL, NULL, &val);
        profile = (struct mdl_profile *) val;

        if (!(profile->user_flags & LDL_FLAG_COMPONENT)) {
			continue;
        }


		if (sql) {
			if (!(db = switch_core_db_open_file(profile->dbname))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
				continue;
			}
			switch_mutex_lock(profile->mutex);
			switch_core_db_exec(db, sql, rost_callback, profile, &errmsg);
			switch_mutex_unlock(profile->mutex);
			switch_core_db_close(db);
		}
		
	}

	switch_safe_free(sql);

}

static int so_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct mdl_profile *profile = (struct mdl_profile *) pArg;

	char *sub_from = argv[0];
	char *sub_to = argv[1];


	ldl_handle_send_presence(profile->handle, sub_to, sub_from, "unavailable", "dnd", "Bub-Bye");
	
	return 0;
}


static int sin_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct mdl_profile *profile = (struct mdl_profile *) pArg;
	switch_event_t *event;

	//char *sub_from = argv[0];
	char *sub_to = argv[1];
	
	if (is_special(sub_to)) {
		if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", profile->login);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s",  sub_to);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "rpid", "available");
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "status", "Online");
			switch_event_fire(&event);
		}
	}

	return 0;
}

static void sign_off(void)
{
	struct mdl_profile *profile = NULL;
	switch_hash_index_t *hi;
    void *val;
	char *sql;
	switch_core_db_t *db;


	sql = switch_mprintf("select * from subscriptions");
	

	for (hi = switch_hash_first(switch_hash_pool_get(globals.profile_hash), globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		char *errmsg;
        switch_hash_this(hi, NULL, NULL, &val);
        profile = (struct mdl_profile *) val;

        if (!(profile->user_flags & LDL_FLAG_COMPONENT)) {
			continue;
        }


		if (sql) {
			if (!(db = switch_core_db_open_file(profile->dbname))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
				continue;
			}
			switch_mutex_lock(profile->mutex);
			switch_core_db_exec(db, sql, so_callback, profile, &errmsg);
			switch_mutex_unlock(profile->mutex);
			switch_core_db_close(db);
		}
		
	}
	
	switch_yield(1000000);
	switch_safe_free(sql);

}

static void sign_on(struct mdl_profile *profile)
{
	char *sql;
	switch_core_db_t *db;
	char *errmsg;

	if ((sql = switch_mprintf("select * from subscriptions where sub_to like 'ext+%%' or sub_to like 'user+%%' or sub_to like 'conf+%%'"))) {
		if (!(db = switch_core_db_open_file(profile->dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
			return;
		}
		switch_mutex_lock(profile->mutex);
		switch_core_db_exec(db, sql, sin_callback, profile, &errmsg);
		switch_mutex_unlock(profile->mutex);
		switch_core_db_close(db);
		switch_safe_free(sql);
	}
}

static void terminate_session(switch_core_session_t **session, int line, switch_call_cause_t cause)
{
	if (*session) {
		switch_channel_t *channel = switch_core_session_get_channel(*session);
		switch_channel_state_t state = switch_channel_get_state(channel);
		struct private_object *tech_pvt = NULL;
		
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Terminate called from line %d state=%s\n", line, switch_channel_state_name(state));
 
		tech_pvt = switch_core_session_get_private(*session);

		if (!tech_pvt || !switch_test_flag(tech_pvt, TFLAG_READY)) {
			switch_core_session_destroy(session);
			return;
		}

		if (switch_test_flag(tech_pvt, TFLAG_TERM)) {
			/*once is enough*/
			return;
		}

		if (state < CS_HANGUP) {
			switch_channel_hangup(channel, cause);
		}
		
		switch_mutex_lock(tech_pvt->flag_mutex);
		switch_set_flag(tech_pvt, TFLAG_TERM);
		switch_set_flag(tech_pvt, TFLAG_BYE);
		switch_clear_flag(tech_pvt, TFLAG_IO);
		switch_mutex_unlock(tech_pvt->flag_mutex);

		*session = NULL;
	}

}

static void dl_logger(char *file, const char *func, int line, int level, char *fmt, ...)
{
	va_list ap;
	char data[1024];

	va_start(ap, fmt);
	
	vsnprintf(data, sizeof(data), fmt, ap);
	switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_DEBUG, "%s", data);

	va_end(ap);
}

static int get_codecs(struct private_object *tech_pvt)
{
	assert(tech_pvt != NULL);
	assert(tech_pvt->session != NULL);

	if (!tech_pvt->num_codecs) {
		if (globals.codec_string) {
			if ((tech_pvt->num_codecs = switch_loadable_module_get_codecs_sorted(tech_pvt->codecs,
																				 SWITCH_MAX_CODECS,
																				 globals.codec_order,
																				 globals.codec_order_last)) <= 0) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "NO codecs?\n");
				return 0;
			}
		} else if (((tech_pvt->num_codecs =
					 switch_loadable_module_get_codecs(switch_core_session_get_pool(tech_pvt->session), tech_pvt->codecs, SWITCH_MAX_CODECS))) <= 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "NO codecs?\n");
			return 0;
		}
	}

	return tech_pvt->num_codecs;
}



static void *SWITCH_THREAD_FUNC handle_thread_run(switch_thread_t *thread, void *obj)
{
	ldl_handle_t *handle = obj;
	struct mdl_profile *profile = NULL;



	profile = ldl_handle_get_private(handle);
	globals.handles++;
	switch_set_flag(profile, TFLAG_IO);
	ldl_handle_run(handle);
	switch_clear_flag(profile, TFLAG_IO);
	globals.handles--;
	ldl_handle_destroy(&handle);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Handle %s [%s] Destroyed\n", profile->name, profile->login);
	
	return NULL;
}

static void handle_thread_launch(ldl_handle_t *handle)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;
	
	switch_threadattr_create(&thd_attr, module_pool);
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, handle_thread_run, handle, module_pool);

}


static int activate_rtp(struct private_object *tech_pvt)
{
	switch_channel_t *channel = switch_core_session_get_channel(tech_pvt->session);
	const char *err;
	int ms = 0;
	switch_rtp_flag_t flags;

	if (switch_rtp_ready(tech_pvt->rtp_session)) {
		return 1;
	}

    if (!(tech_pvt->remote_ip && tech_pvt->remote_port)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "No valid candidates received!\n");
        return 0;
    }

	if (switch_core_codec_init(&tech_pvt->read_codec,
							   tech_pvt->codec_name,
							   NULL,
							   tech_pvt->codec_rate,
							   ms,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Can't load codec?\n");
		switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return 0;
	}
	tech_pvt->read_frame.rate = tech_pvt->read_codec.implementation->samples_per_second;
	tech_pvt->read_frame.codec = &tech_pvt->read_codec;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Set Read Codec to %s@%d\n", 
					  tech_pvt->codec_name, (int)tech_pvt->read_codec.implementation->samples_per_second);
	
	if (switch_core_codec_init(&tech_pvt->write_codec,
							   tech_pvt->codec_name,
							   NULL,
							   tech_pvt->codec_rate,
							   ms,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Can't load codec?\n");
		switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return 0;
	}
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Set Write Codec to %s@%d\n",  
					  tech_pvt->codec_name,(int)tech_pvt->write_codec.implementation->samples_per_second);
							
	switch_core_session_set_read_codec(tech_pvt->session, &tech_pvt->read_codec);
	switch_core_session_set_write_codec(tech_pvt->session, &tech_pvt->write_codec);
	

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "SETUP RTP %s:%d -> %s:%d\n", tech_pvt->profile->ip, tech_pvt->local_port, tech_pvt->remote_ip, tech_pvt->remote_port);
	
	flags = SWITCH_RTP_FLAG_GOOGLEHACK | SWITCH_RTP_FLAG_AUTOADJ;
	//flags = SWITCH_RTP_FLAG_AUTOADJ;

	if (switch_test_flag(tech_pvt->profile, TFLAG_TIMER)) {
	  flags |= SWITCH_RTP_FLAG_USE_TIMER;
	}

	flags |= SWITCH_RTP_FLAG_AUTO_CNG;

	if (!(tech_pvt->rtp_session = switch_rtp_new(tech_pvt->profile->ip,
												 tech_pvt->local_port,
												 tech_pvt->remote_ip,
												 tech_pvt->remote_port,
												 tech_pvt->codec_num,
                                                 tech_pvt->read_codec.implementation->encoded_bytes_per_frame,
												 tech_pvt->read_codec.implementation->microseconds_per_frame,
												 flags,
												 NULL,
												 tech_pvt->profile->timer_name,
												 &err, switch_core_session_get_pool(tech_pvt->session)))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "RTP ERROR %s\n", err);
		switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return 0;
	} else {
		uint8_t vad_in = switch_test_flag(tech_pvt, TFLAG_VAD_IN) ? 1 : 0;
		uint8_t vad_out = switch_test_flag(tech_pvt, TFLAG_VAD_OUT) ? 1 : 0;
		uint8_t inb = switch_test_flag(tech_pvt, TFLAG_OUTBOUND) ? 0 : 1;
		switch_rtp_activate_ice(tech_pvt->rtp_session, tech_pvt->remote_user, tech_pvt->local_user);
		if ((vad_in && inb) || (vad_out && !inb)) {
			switch_rtp_enable_vad(tech_pvt->rtp_session, tech_pvt->session, &tech_pvt->read_codec, SWITCH_VAD_FLAG_TALKING);
			switch_set_flag_locked(tech_pvt, TFLAG_VAD);
		}
	}

	return 1;
}



static int do_candidates(struct private_object *tech_pvt, int force)
{
	switch_channel_t *channel = switch_core_session_get_channel(tech_pvt->session);
	assert(channel != NULL);

	if (switch_test_flag(tech_pvt, TFLAG_DO_CAND)) {
		return 1;
	}

	tech_pvt->next_cand += DL_CAND_WAIT;
	if (switch_test_flag(tech_pvt, TFLAG_BYE)) {
		return 0;
	}
	switch_set_flag_locked(tech_pvt, TFLAG_DO_CAND);

	if (force || !switch_test_flag(tech_pvt, TFLAG_RTP_READY)) {
		ldl_candidate_t cand[1];
		char *advip = tech_pvt->profile->extip ? tech_pvt->profile->extip : tech_pvt->profile->ip;
		char *err;

		memset(cand, 0, sizeof(cand));
		switch_stun_random_string(tech_pvt->local_user, 16, NULL);
		switch_stun_random_string(tech_pvt->local_pass, 16, NULL);

		if (switch_test_flag(tech_pvt, TFLAG_LANADDR)) {
			advip = tech_pvt->profile->ip;
		}


		cand[0].port = tech_pvt->local_port;
		cand[0].address = advip;
				
		if (!strncasecmp(advip, "stun:", 5)) {
			char *stun_ip = advip + 5;
					
			if (tech_pvt->stun_ip) {
				cand[0].address = tech_pvt->stun_ip;
				cand[0].port = tech_pvt->stun_port;
			} else {
				if (!stun_ip) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Stun Failed! NO STUN SERVER!\n");
					switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
					return 0;
				}

				cand[0].address = tech_pvt->profile->ip;
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Stun Lookup Local %s:%d\n", cand[0].address, cand[0].port);
				if (switch_stun_lookup(&cand[0].address,
									   &cand[0].port,
									   stun_ip,
									   SWITCH_STUN_DEFAULT_PORT,
									   &err,
									   switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Stun Failed! %s:%d [%s]\n", stun_ip, SWITCH_STUN_DEFAULT_PORT, err);
					switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
					return 0;
				}
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Stun Success %s:%d\n", cand[0].address, cand[0].port);
			}
			cand[0].type = "stun";
			tech_pvt->stun_ip = switch_core_session_strdup(tech_pvt->session, cand[0].address);
			tech_pvt->stun_port = cand[0].port;
		} else {
			cand[0].type = "local";
		}

		cand[0].name = "rtp";
		cand[0].username = tech_pvt->local_user;
		cand[0].password = tech_pvt->local_pass;
		cand[0].pref = 1;
		cand[0].protocol = "udp";
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Send Candidate %s:%d [%s]\n", cand[0].address, cand[0].port, cand[0].username);
		tech_pvt->cand_id = ldl_session_candidates(tech_pvt->dlsession, cand, 1);
		switch_set_flag_locked(tech_pvt, TFLAG_TRANSPORT);
		switch_set_flag_locked(tech_pvt, TFLAG_RTP_READY);
	}
	switch_clear_flag_locked(tech_pvt, TFLAG_DO_CAND);
	return 1;
}

static char *lame(char *in)
{
	if (!strncasecmp(in, "ilbc", 4)) {
		return "iLBC";
	} else {
		return in;
	}
}

static int do_describe(struct private_object *tech_pvt, int force)
{
	ldl_payload_t payloads[5];

	if (!tech_pvt->session) {
		return 0;
	}

	if (switch_test_flag(tech_pvt, TFLAG_DO_DESC)) {
		return 1;
	}

	tech_pvt->next_desc += DL_CAND_WAIT;

	if (switch_test_flag(tech_pvt, TFLAG_BYE)) {
		return 0;
	}

	memset(payloads, 0, sizeof(payloads));
	switch_set_flag_locked(tech_pvt, TFLAG_DO_CAND);
	if (!get_codecs(tech_pvt)) {
		terminate_session(&tech_pvt->session, __LINE__, SWITCH_CAUSE_INCOMPATIBLE_DESTINATION);
		switch_set_flag_locked(tech_pvt, TFLAG_BYE);
		switch_clear_flag_locked(tech_pvt, TFLAG_IO);
		return 0;
	}


	if (force || !switch_test_flag(tech_pvt, TFLAG_CODEC_READY)) {
		if (tech_pvt->codec_index < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Don't have my codec yet here's one\n");
			tech_pvt->codec_name = lame(tech_pvt->codecs[0]->iananame);
			tech_pvt->codec_num = tech_pvt->codecs[0]->ianacode;
			tech_pvt->codec_rate = tech_pvt->codecs[0]->samples_per_second;
			tech_pvt->r_codec_num = tech_pvt->codecs[0]->ianacode;
			tech_pvt->codec_index = 0;
					
			payloads[0].name = lame(tech_pvt->codecs[0]->iananame);
			payloads[0].id = tech_pvt->codecs[0]->ianacode;
			payloads[0].rate = tech_pvt->codecs[0]->samples_per_second;
			payloads[0].bps = tech_pvt->codecs[0]->bits_per_second;
			
		} else {
			payloads[0].name = lame(tech_pvt->codecs[tech_pvt->codec_index]->iananame);
			payloads[0].id = tech_pvt->codecs[tech_pvt->codec_index]->ianacode;
			payloads[0].rate = tech_pvt->codecs[tech_pvt->codec_index]->samples_per_second;
			payloads[0].bps = tech_pvt->codecs[tech_pvt->codec_index]->bits_per_second;
		}

				
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Send Describe [%s@%d]\n", payloads[0].name, payloads[0].rate);
		tech_pvt->desc_id = ldl_session_describe(tech_pvt->dlsession, payloads, 1,
												 switch_test_flag(tech_pvt, TFLAG_OUTBOUND) ? LDL_DESCRIPTION_INITIATE : LDL_DESCRIPTION_ACCEPT);
		switch_set_flag_locked(tech_pvt, TFLAG_CODEC_READY);
	} 
	switch_clear_flag_locked(tech_pvt, TFLAG_DO_CAND);
	return 1;
}

static switch_status_t negotiate_media(switch_core_session_t *session)
{
	switch_status_t ret = SWITCH_STATUS_FALSE;
	switch_channel_t *channel;
	struct private_object *tech_pvt = NULL;
	switch_time_t started;
	switch_time_t now;
	unsigned int elapsed;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_set_flag_locked(tech_pvt, TFLAG_IO);

	started = switch_time_now();

	/* jingle has no ringing indication so we will just pretend that we got one */
	switch_core_session_queue_indication(session, SWITCH_MESSAGE_INDICATE_RINGING);
	switch_channel_mark_ring_ready(channel);
	
	if (switch_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
		tech_pvt->next_cand = switch_time_now() + DL_CAND_WAIT;
		tech_pvt->next_desc = switch_time_now();
	} else {
		tech_pvt->next_cand = switch_time_now() + DL_CAND_WAIT;
		tech_pvt->next_desc = switch_time_now() + DL_CAND_WAIT;
	}

	while(! (switch_test_flag(tech_pvt, TFLAG_CODEC_READY) && 
			 switch_test_flag(tech_pvt, TFLAG_RTP_READY) && 
			 switch_test_flag(tech_pvt, TFLAG_ANSWER) && 
			 switch_test_flag(tech_pvt, TFLAG_TRANSPORT_ACCEPT) &&
			 switch_test_flag(tech_pvt, TFLAG_TRANSPORT))) {
		now = switch_time_now();
		elapsed = (unsigned int)((now - started) / 1000);

		if (switch_channel_get_state(channel) >= CS_HANGUP || switch_test_flag(tech_pvt, TFLAG_BYE)) {
			goto out;
		}

		
		if (now >= tech_pvt->next_desc) {
			if (!do_describe(tech_pvt, 0)) {
				goto out;
			}
		}
		
		if (tech_pvt->next_cand && now >= tech_pvt->next_cand) {
			if (!do_candidates(tech_pvt, 0)) {
				goto out;
			}
		}
		if (elapsed > 60000) {
			terminate_session(&tech_pvt->session,  __LINE__, SWITCH_CAUSE_NORMAL_CLEARING);
			switch_set_flag_locked(tech_pvt, TFLAG_BYE);
			switch_clear_flag_locked(tech_pvt, TFLAG_IO);
			goto done;
		}
		if (switch_test_flag(tech_pvt, TFLAG_BYE) || ! switch_test_flag(tech_pvt, TFLAG_IO)) {
			goto done;
		}
		switch_yield(1000);
	}
	
	if (switch_channel_get_state(channel) >= CS_HANGUP || switch_test_flag(tech_pvt, TFLAG_BYE)) {
		goto out;
	}	

	if (!activate_rtp(tech_pvt)) {
		goto out;
	}
	
	if (switch_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
		if (!do_candidates(tech_pvt, 0)) {
			goto out;
		}
		switch_channel_answer(channel);
	} 
	ret = SWITCH_STATUS_SUCCESS;

	goto done;
	
 out:
	terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
 done:

	return ret;
}

/* 
   State methods they get called when the state changes to the specific state 
   returning SWITCH_STATUS_SUCCESS tells the core to execute the standard state method next
   so if you fully implement the state you can return SWITCH_STATUS_FALSE to skip it.
*/
static switch_status_t channel_on_init(switch_core_session_t *session)
{
	switch_channel_t *channel;
	struct private_object *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt->read_frame.buflen = SWITCH_RTP_MAX_BUF_LEN;

	switch_set_flag(tech_pvt, TFLAG_READY);

	if (negotiate_media(session) == SWITCH_STATUS_SUCCESS) {
		/* Move Channel's State Machine to RING */
		switch_channel_set_state(channel, CS_RING);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_ring(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	struct private_object *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL RING\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_execute(switch_core_session_t *session)
{

	switch_channel_t *channel = NULL;
	struct private_object *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL EXECUTE\n", switch_channel_get_name(channel));


	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_hangup(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	struct private_object *tech_pvt = NULL;
	
	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_clear_flag_locked(tech_pvt, TFLAG_IO);
	switch_clear_flag_locked(tech_pvt, TFLAG_VOICE);
	switch_set_flag_locked(tech_pvt, TFLAG_BYE);

	/* Dunno why, but if googletalk calls us for the first time, as soon as the call ends
	 they think we are offline for no reason so we send this presence packet to stop it from happening
	 We should find out why.....
	*/
	if ((tech_pvt->profile->user_flags & LDL_FLAG_COMPONENT) && is_special(tech_pvt->them)) {	
		ldl_handle_send_presence(tech_pvt->profile->handle,
								 tech_pvt->them, tech_pvt->us, NULL, NULL, "Click To Call");
	}
	if (tech_pvt->dlsession) {
		if (!switch_test_flag(tech_pvt, TFLAG_TERM)) {
			ldl_session_terminate(tech_pvt->dlsession);
			switch_set_flag_locked(tech_pvt, TFLAG_TERM);
		}
		ldl_session_destroy(&tech_pvt->dlsession);
	}

	if (switch_rtp_ready(tech_pvt->rtp_session)) {
		switch_rtp_destroy(&tech_pvt->rtp_session);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "NUKE RTP\n");
		tech_pvt->rtp_session = NULL;
	}

	if (tech_pvt->read_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->read_codec);
	}

	if (tech_pvt->write_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->write_codec);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL HANGUP\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_kill_channel(switch_core_session_t *session, int sig)
{
	switch_channel_t *channel = NULL;
	struct private_object *tech_pvt = NULL;

    if (!(channel = switch_core_session_get_channel(session))) {
        return SWITCH_STATUS_SUCCESS;
    }

    if (!(tech_pvt = switch_core_session_get_private(session))) {
        return SWITCH_STATUS_SUCCESS;
    }
    

    switch (sig) {
    case SWITCH_SIG_KILL:
        switch_clear_flag_locked(tech_pvt, TFLAG_IO);
        switch_clear_flag_locked(tech_pvt, TFLAG_VOICE);
        switch_set_flag_locked(tech_pvt, TFLAG_BYE);

        if (tech_pvt->dlsession) {
            if (!switch_test_flag(tech_pvt, TFLAG_TERM)) {
                ldl_session_terminate(tech_pvt->dlsession);
                switch_set_flag_locked(tech_pvt, TFLAG_TERM);
            }
            ldl_session_destroy(&tech_pvt->dlsession);
            
        }

        if (switch_rtp_ready(tech_pvt->rtp_session)) {
            switch_rtp_kill_socket(tech_pvt->rtp_session);
        }
        break;
	case SWITCH_SIG_BREAK:
        if (switch_rtp_ready(tech_pvt->rtp_session)) {
			switch_rtp_set_flag(tech_pvt->rtp_session, SWITCH_RTP_FLAG_BREAK);
		}
		break;
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL KILL\n", switch_channel_get_name(channel));


	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_loopback(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CHANNEL LOOPBACK\n");
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_transmit(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CHANNEL TRANSMIT\n");
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_waitfor_read(switch_core_session_t *session, int ms, int stream_id)
{
	struct private_object *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_waitfor_write(switch_core_session_t *session, int ms, int stream_id)
{
	struct private_object *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	return SWITCH_STATUS_SUCCESS;

}

static switch_status_t channel_send_dtmf(switch_core_session_t *session, char *dtmf)
{
	struct private_object *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "DTMF [%s]\n", dtmf);

	return switch_rtp_queue_rfc2833(tech_pvt->rtp_session,
									dtmf,
									100 * (tech_pvt->read_codec.implementation->samples_per_second / 1000));

}

static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, int timeout,
										  switch_io_flag_t flags, int stream_id)
{
	struct private_object *tech_pvt = NULL;
	uint32_t bytes = 0;
	switch_size_t samples = 0, frames = 0, ms = 0;
	switch_channel_t *channel = NULL;
	switch_payload_t payload = 0;
	switch_status_t status;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);


	if (!tech_pvt->rtp_session) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_test_flag(tech_pvt, TFLAG_BYE)) {
		//terminate_session(&session,  __LINE__, SWITCH_CAUSE_NORMAL_CLEARING);
		return SWITCH_STATUS_FALSE;
	}


	tech_pvt->read_frame.datalen = 0;
	switch_set_flag_locked(tech_pvt, TFLAG_READING);

	bytes = tech_pvt->read_codec.implementation->encoded_bytes_per_frame;
	samples = tech_pvt->read_codec.implementation->samples_per_frame;
	ms = tech_pvt->read_codec.implementation->microseconds_per_frame;
	tech_pvt->read_frame.datalen = 0;

	
	while (!switch_test_flag(tech_pvt, TFLAG_BYE) && switch_test_flag(tech_pvt, TFLAG_IO) && tech_pvt->read_frame.datalen == 0) {
		tech_pvt->read_frame.flags = 0;
		status = switch_rtp_zerocopy_read_frame(tech_pvt->rtp_session, &tech_pvt->read_frame);
		
		if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
			return SWITCH_STATUS_FALSE;
		}
		payload = tech_pvt->read_frame.payload;

		if (switch_rtp_has_dtmf(tech_pvt->rtp_session)) {
			char dtmf[128];
			switch_rtp_dequeue_dtmf(tech_pvt->rtp_session, dtmf, sizeof(dtmf));
			switch_channel_queue_dtmf(channel, dtmf);
			switch_set_flag_locked(tech_pvt, TFLAG_DTMF);
		}

		if (switch_test_flag(tech_pvt, TFLAG_DTMF)) {
			switch_clear_flag_locked(tech_pvt, TFLAG_DTMF);
			return SWITCH_STATUS_BREAK;
		}

		if (switch_test_flag(&tech_pvt->read_frame, SFF_CNG)) {
			tech_pvt->read_frame.datalen = tech_pvt->last_read ? tech_pvt->last_read : tech_pvt->read_codec.implementation->encoded_bytes_per_frame;
		}

		if (tech_pvt->read_frame.datalen > 0) {
            if (!switch_test_flag((&tech_pvt->read_frame), SFF_CNG)) {
                if (tech_pvt->read_codec.implementation->encoded_bytes_per_frame && bytes) {
                    bytes = tech_pvt->read_codec.implementation->encoded_bytes_per_frame;
                    frames = (tech_pvt->read_frame.datalen / bytes);
                } else {
                    frames = 1;
                }
                samples = frames * tech_pvt->read_codec.implementation->samples_per_frame;
                ms = frames * tech_pvt->read_codec.implementation->microseconds_per_frame;
                tech_pvt->timestamp_recv += (int32_t) samples;
                tech_pvt->read_frame.samples = (int) samples;
                tech_pvt->last_read = tech_pvt->read_frame.datalen;
            }
			break;
		}

		switch_yield(1000);
	}


	switch_clear_flag_locked(tech_pvt, TFLAG_READING);


	*frame = &tech_pvt->read_frame;
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, int timeout,
										   switch_io_flag_t flags, int stream_id)
{
	struct private_object *tech_pvt;
	switch_channel_t *channel = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	int bytes = 0, samples = 0, frames = 0;


	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	if (!tech_pvt->rtp_session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_test_flag(tech_pvt, TFLAG_RTP_READY)) {
		return SWITCH_STATUS_SUCCESS;
	}


	if (switch_test_flag(tech_pvt, TFLAG_BYE)) {
		return SWITCH_STATUS_FALSE;
	}

	switch_set_flag_locked(tech_pvt, TFLAG_WRITING);

	if (tech_pvt->read_codec.implementation->encoded_bytes_per_frame) {
		bytes = tech_pvt->read_codec.implementation->encoded_bytes_per_frame;
		frames = ((int) frame->datalen / bytes);
	} else {
		frames = 1;
	}

	samples = frames * tech_pvt->read_codec.implementation->samples_per_frame;
	tech_pvt->timestamp_send += samples;
	if (switch_rtp_write_frame(tech_pvt->rtp_session, frame, 0) < 0) {
		terminate_session(&session,  __LINE__, SWITCH_CAUSE_NORMAL_CLEARING);
		return SWITCH_STATUS_FALSE;
	}


	switch_clear_flag_locked(tech_pvt, TFLAG_WRITING);

	return status;
}

static switch_status_t channel_answer_channel(switch_core_session_t *session)
{
	struct private_object *tech_pvt;
	switch_channel_t *channel = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t channel_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_channel_t *channel;
	struct private_object *tech_pvt;
			
	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
			
	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (msg->message_id) {
	case SWITCH_MESSAGE_INDICATE_BRIDGE:
	  if (tech_pvt->rtp_session && switch_test_flag(tech_pvt->profile, TFLAG_TIMER)) {
			switch_rtp_clear_flag(tech_pvt->rtp_session, SWITCH_RTP_FLAG_USE_TIMER);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "De-activate timed RTP!\n");
			//switch_rtp_set_flag(tech_pvt->rtp_session, SWITCH_RTP_FLAG_TIMER_RECLOCK);
		}
		break;
	case SWITCH_MESSAGE_INDICATE_UNBRIDGE:
		if (tech_pvt->rtp_session && switch_test_flag(tech_pvt->profile, TFLAG_TIMER)) {
			switch_rtp_set_flag(tech_pvt->rtp_session, SWITCH_RTP_FLAG_USE_TIMER);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Re-activate timed RTP!\n");
			//switch_rtp_clear_flag(tech_pvt->rtp_session, SWITCH_RTP_FLAG_TIMER_RECLOCK);
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_receive_event(switch_core_session_t *session, switch_event_t *event)
{
	switch_channel_t *channel;
    struct private_object *tech_pvt;
	char *subject, *body;

    channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

    tech_pvt = switch_core_session_get_private(session);
    assert(tech_pvt != NULL);


	if (!(body = switch_event_get_body(event))) {
		body = "";
	}

	if (!(subject = switch_event_get_header(event, "subject"))) {
		subject = "None";
	}

	ldl_session_send_msg(tech_pvt->dlsession, subject, body);

	return SWITCH_STATUS_SUCCESS;
}

static const switch_state_handler_table_t channel_event_handlers = {
	/*.on_init */ channel_on_init,
	/*.on_ring */ channel_on_ring,
	/*.on_execute */ channel_on_execute,
	/*.on_hangup */ channel_on_hangup,
	/*.on_loopback */ channel_on_loopback,
	/*.on_transmit */ channel_on_transmit
};

static const switch_io_routines_t channel_io_routines = {
	/*.outgoing_channel */ channel_outgoing_channel,
	/*.answer_channel */ channel_answer_channel,
	/*.read_frame */ channel_read_frame,
	/*.write_frame */ channel_write_frame,
	/*.kill_channel */ channel_kill_channel,
	/*.waitfor_read */ channel_waitfor_read,
	/*.waitfor_write */ channel_waitfor_write,
	/*.send_dtmf */ channel_send_dtmf,
	/*.receive_message*/ channel_receive_message,
	/*.receive_event*/ channel_receive_event
};

static const switch_endpoint_interface_t channel_endpoint_interface = {
	/*.interface_name */ "dingaling",
	/*.io_routines */ &channel_io_routines,
	/*.event_handlers */ &channel_event_handlers,
	/*.private */ NULL,
	/*.next */ NULL
};



static switch_api_interface_t logout_api_interface = {
	/*.interface_name */ "dl_logout",
	/*.desc */ "DingaLing Logout",
	/*.function */ dl_logout,
	/*.syntax */ "dl_logout <profile_name>",
	/*.next */ NULL
};

static switch_api_interface_t login_api_interface = {
	/*.interface_name */ "dl_login",
	/*.desc */ "DingaLing Login",
	/*.function */ dl_login,
	/*.syntax */ "dl_login <profile_name>",
	/*.next */ &logout_api_interface
};

static const switch_chat_interface_t channel_chat_interface = {
	/*.name */ MDL_CHAT_PROTO,
	/*.chat_send */ chat_send,
};

static const switch_loadable_module_interface_t channel_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ &channel_endpoint_interface,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ NULL,
	/*.application_interface */ NULL,
	/*.api_interface */ &login_api_interface,
	/*.file_interface */ NULL,
    /*.speech_interface */ NULL,
    /*.directory_interface */ NULL,
    /*.chat_interface */ &channel_chat_interface

};


/* Make sure when you have 2 sessions in the same scope that you pass the appropriate one to the routines
   that allocate memory or you will have 1 channel with memory allocated from another channel's pool!
*/
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session, switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool)
{
	if ((*new_session = switch_core_session_request(&channel_endpoint_interface, pool)) != 0) {
		struct private_object *tech_pvt;
		switch_channel_t *channel;
		switch_caller_profile_t *caller_profile = NULL;
		struct mdl_profile *mdl_profile = NULL;
		ldl_session_t *dlsession = NULL;
		char *profile_name;
		char *callto;
		char idbuf[1024];
		char *full_id;
		char sess_id[11] = "";
		char *dnis = NULL;
		char workspace[1024] = "";
		char *p, *u, ubuf[512] = "", *user = NULL;
		char *cid_msg = NULL, *f_cid_msg = NULL;

		switch_copy_string(workspace, outbound_profile->destination_number, sizeof(workspace));
		profile_name = workspace;

		if ((callto = strchr(profile_name, '/'))) {
			*callto++ = '\0';
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Invalid URL!\n");
			terminate_session(new_session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			return SWITCH_CAUSE_INVALID_NUMBER_FORMAT;
		}

		if ((dnis = strchr(callto, ':'))) {
			*dnis++ = '\0';
		}

        for (p = callto; p && *p; p++) {
            *p = (char) tolower(*p);
        }

		if ((p = strchr(profile_name, '@'))) {
			*p++ = '\0';
			u = profile_name;
			profile_name = p;
			snprintf(ubuf, sizeof(ubuf), "%s@%s/talk", u, profile_name);
			user = ubuf;
		}

		if ((mdl_profile = switch_core_hash_find(globals.profile_hash, profile_name))) {
			if (!(mdl_profile->user_flags & LDL_FLAG_COMPONENT)) {
				user = ldl_handle_get_login(mdl_profile->handle);
			} else {
				if (!user) {
					if (strchr(outbound_profile->caller_id_number, '@')) {
						snprintf(ubuf, sizeof(ubuf), "%s/talk", outbound_profile->caller_id_number);
						user = ubuf;
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Invalid User!\n");
						terminate_session(new_session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
					}
				}
			}

			if (!ldl_handle_ready(mdl_profile->handle)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Doh! we are not logged in yet!\n");
				terminate_session(new_session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
				return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
			}
			if (!(full_id = ldl_handle_probe(mdl_profile->handle, callto, user, idbuf, sizeof(idbuf)))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Unknown Recipient!\n");
				terminate_session(new_session,  __LINE__, SWITCH_CAUSE_NO_USER_RESPONSE);
				return SWITCH_CAUSE_NO_USER_RESPONSE;
			}
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Unknown Profile!\n");
			terminate_session(new_session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
		}
		
		
		switch_core_session_add_stream(*new_session, NULL);
		if ((tech_pvt = (struct private_object *) switch_core_session_alloc(*new_session, sizeof(struct private_object))) != 0) {
			memset(tech_pvt, 0, sizeof(*tech_pvt));
			switch_mutex_init(&tech_pvt->flag_mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(*new_session));
			tech_pvt->flags |= globals.flags;
			tech_pvt->flags |= mdl_profile->flags;
			channel = switch_core_session_get_channel(*new_session);
			switch_core_session_set_private(*new_session, tech_pvt);
			tech_pvt->session = *new_session;
			tech_pvt->codec_index = -1;
			tech_pvt->local_port = switch_rtp_request_port();
			tech_pvt->recip = switch_core_session_strdup(*new_session, full_id);
			tech_pvt->dnis = switch_core_session_strdup(*new_session, dnis);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Hey where is my memory pool?\n");
			terminate_session(new_session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
		}

		if (outbound_profile) {
			char name[128];
			
			snprintf(name, sizeof(name), "DingaLing/%s", outbound_profile->destination_number);
			switch_channel_set_name(channel, name);

			caller_profile = switch_caller_profile_clone(*new_session, outbound_profile);
			switch_channel_set_caller_profile(channel, caller_profile);
			tech_pvt->caller_profile = caller_profile;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Doh! no caller profile\n");
			terminate_session(new_session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
		}

		switch_channel_set_flag(channel, CF_OUTBOUND);
		switch_set_flag_locked(tech_pvt, TFLAG_OUTBOUND);

		switch_stun_random_string(sess_id, 10, "0123456789");
		tech_pvt->us = switch_core_session_strdup(*new_session, user);
		tech_pvt->them = switch_core_session_strdup(*new_session, full_id);
		ldl_session_create(&dlsession, mdl_profile->handle, sess_id, full_id, user, LDL_FLAG_OUTBOUND);
		
		if (session) {
			switch_channel_t *calling_channel = switch_core_session_get_channel(session);
			cid_msg = switch_channel_get_variable(calling_channel, "dl_cid_msg");
		}

		if (!cid_msg) {
			f_cid_msg = switch_mprintf("Incoming Call From %s %s\n", outbound_profile->caller_id_name, outbound_profile->caller_id_number);
			cid_msg = f_cid_msg;
		}

		if (cid_msg) {
			char *them;
			them = strdup(tech_pvt->them);
			if (them) {
				char *p;
				if ((p = strchr(them, '/'))) {
					*p = '\0';
				}
				ldl_handle_send_msg(mdl_profile->handle, tech_pvt->us, them, "", cid_msg);
			}
			switch_safe_free(them);
		}
		switch_safe_free(f_cid_msg);

		tech_pvt->profile = mdl_profile;
		ldl_session_set_private(dlsession, *new_session);
		ldl_session_set_value(dlsession, "dnis", dnis);
		ldl_session_set_value(dlsession, "caller_id_name", outbound_profile->caller_id_name);
		ldl_session_set_value(dlsession, "caller_id_number", outbound_profile->caller_id_number);
		tech_pvt->dlsession = dlsession;
		if (!get_codecs(tech_pvt)) {
			terminate_session(new_session,  __LINE__, SWITCH_CAUSE_BEARERCAPABILITY_NOTAVAIL);
            return SWITCH_CAUSE_BEARERCAPABILITY_NOTAVAIL;
		}
		switch_channel_set_state(channel, CS_INIT);
		return SWITCH_CAUSE_SUCCESS;

	}

	return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;

}

SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface, char *filename)
{

	if (switch_core_new_memory_pool(&module_pool) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "OH OH no pool\n");
		return SWITCH_STATUS_TERM;
	}

	load_config();

	if (switch_event_reserve_subclass(DL_EVENT_LOGIN_SUCCESS) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!", DL_EVENT_LOGIN_SUCCESS);
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_reserve_subclass(DL_EVENT_LOGIN_FAILURE) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!", DL_EVENT_LOGIN_FAILURE);
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_reserve_subclass(DL_EVENT_CONNECTED) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!", DL_EVENT_CONNECTED);
		return SWITCH_STATUS_GENERR;
	}
	
	if (switch_event_bind((char *) modname, SWITCH_EVENT_PRESENCE_IN, SWITCH_EVENT_SUBCLASS_ANY, pres_event_handler, NULL) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_bind((char *) modname, SWITCH_EVENT_PRESENCE_PROBE, SWITCH_EVENT_SUBCLASS_ANY, pres_event_handler, NULL) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_bind((char *) modname, SWITCH_EVENT_PRESENCE_OUT, SWITCH_EVENT_SUBCLASS_ANY, pres_event_handler, NULL) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_bind((char *) modname, SWITCH_EVENT_ROSTER, SWITCH_EVENT_SUBCLASS_ANY, roster_event_handler, NULL) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = &channel_module_interface;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

static ldl_status handle_loop(ldl_handle_t *handle)
{
	if (!globals.running) {
		return LDL_STATUS_FALSE;
	}
	return LDL_STATUS_SUCCESS;
}

static switch_status_t init_profile(struct mdl_profile *profile, uint8_t login)
{
	if (profile &&
		profile->login &&
		profile->password &&
		profile->dialplan &&
		profile->message &&
		profile->ip &&
		profile->name &&
		profile->exten) {
		ldl_handle_t *handle;

		if (switch_test_flag(profile, TFLAG_TIMER) && !profile->timer_name) {
			profile->timer_name = switch_core_strdup(module_pool, "soft");			
		}

		if (login) {
			if (ldl_handle_init(&handle,
								profile->login,
								profile->password,
								profile->server,
								profile->user_flags,
								profile->message,
								handle_loop,
								handle_signalling,
								handle_response,
								profile) == LDL_STATUS_SUCCESS) {
				profile->handle = handle;
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Started Thread for %s@%s\n", profile->login, profile->dialplan);
				switch_core_hash_insert(globals.profile_hash, profile->name, profile);
				handle_thread_launch(handle);
			}
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Created Profile for %s@%s\n", profile->login, profile->dialplan);
			switch_core_hash_insert(globals.profile_hash, profile->name, profile);
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
						  "Invalid Profile\n"
						  "login[%s]\n"
						  "pass[%s]\n"
						  "dialplan[%s]\n"
						  "message[%s]\n"
						  "rtp-ip[%s]\n"
						  "name[%s]\n"
						  "exten[%s]\n",
						  profile->login,
						  profile->password,
						  profile->dialplan,
						  profile->message,
						  profile->ip,
						  profile->name,
						  profile->exten);

		return SWITCH_STATUS_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}


SWITCH_MOD_DECLARE(switch_status_t) switch_module_shutdown(void)
{
	sign_off();

	if (globals.running) {
		int x = 0;
		globals.running = 0;
		while (globals.handles > 0) {
			switch_yield(100000);
			x++;
			if(x > 10) {
				break;
			}
		}
		if (globals.init) {
			ldl_global_destroy();
		}
	}
	return SWITCH_STATUS_SUCCESS;
}


static void set_profile_val(struct mdl_profile *profile, char *var, char *val)
{
	
	if (!strcasecmp(var, "login")) {
		profile->login = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "password")) {
		profile->password = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "use-rtp-timer") && switch_true(val)) {
	  	switch_set_flag(profile, TFLAG_TIMER);
	} else if (!strcasecmp(var, "dialplan")) {
		profile->dialplan = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "auto-reply")) {
		profile->auto_reply = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "name")) {
		profile->name = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "message")) {
		profile->message = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "rtp-ip")) {
		profile->ip = switch_core_strdup(module_pool, strcasecmp(val, "auto") ? val : globals.guess_ip);
	} else if (!strcasecmp(var, "ext-rtp-ip")) {
		profile->extip = switch_core_strdup(module_pool, strcasecmp(val, "auto") ? val : globals.guess_ip);
	} else if (!strcasecmp(var, "server")) {
		profile->server = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "rtp-timer-name")) {
		profile->timer_name = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "lanaddr")) {
		profile->lanaddr = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "tls")) {
		if (switch_true(val)) {
			profile->user_flags |= LDL_FLAG_TLS;
		}
	} else if (!strcasecmp(var, "sasl")) {
		if (!strcasecmp(val, "plain")) {
			profile->user_flags |= LDL_FLAG_SASL_PLAIN;
		} else if (!strcasecmp(val, "md5")) {
			profile->user_flags |= LDL_FLAG_SASL_MD5;
		}
	} else if (!strcasecmp(var, "exten")) {
		profile->exten = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "context")) {
		profile->context = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "auto-login")) {
		if (switch_true(val)) {
			switch_set_flag(profile, TFLAG_AUTO);
		}
	} else if (!strcasecmp(var, "vad")) {
		if (!strcasecmp(val, "in")) {
			switch_set_flag(profile, TFLAG_VAD_IN);
		} else if (!strcasecmp(val, "out")) {
			switch_set_flag(profile, TFLAG_VAD_OUT);
		} else if (!strcasecmp(val, "both")) {
			switch_set_flag(profile, TFLAG_VAD_IN);
			switch_set_flag(profile, TFLAG_VAD_OUT);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invald option %s for VAD\n", val);
		}
	}
}

static switch_status_t dl_logout(char *profile_name, switch_core_session_t *session, switch_stream_handle_t *stream)
{
	struct mdl_profile *profile;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!profile_name) {
		stream->write_function(stream, "USAGE: %s\n", logout_api_interface.syntax);
		return SWITCH_STATUS_SUCCESS;
	}

	if ((profile = switch_core_hash_find(globals.profile_hash, profile_name))) {
		ldl_handle_stop(profile->handle);
		stream->write_function(stream, "OK\n");
	} else {
		stream->write_function(stream, "NO SUCH PROFILE %s\n", profile_name);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t dl_login(char *arg, switch_core_session_t *session, switch_stream_handle_t *stream)
{
	char *argv[10] = {0};
	int argc = 0;
	char *var, *val, *myarg;
	struct mdl_profile *profile = NULL;
	int x;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

    if (switch_strlen_zero(arg)) {
        stream->write_function(stream, "USAGE: %s\n", login_api_interface.syntax);
        return SWITCH_STATUS_SUCCESS;
    }

	myarg = strdup(arg);

	argc = switch_separate_string(myarg, ';', argv, (sizeof(argv) / sizeof(argv[0])));

	if (switch_strlen_zero(arg) || argc != 1) {
		stream->write_function(stream, "USAGE: %s\n", login_api_interface.syntax);
		return SWITCH_STATUS_SUCCESS;
	}

	if (!strncasecmp(argv[0], "profile=", 8)) {
		char *profile_name = argv[0] + 8;
		profile = switch_core_hash_find(globals.profile_hash, profile_name);

		if (profile) {
			if (switch_test_flag(profile, TFLAG_IO)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Profile already exists.");
				stream->write_function(stream, "Profile already exists\n");
				return SWITCH_STATUS_SUCCESS;
			}

		}
	} else {
		profile = switch_core_alloc(module_pool, sizeof(*profile));

		for(x = 0; x < argc; x++) {
			var = argv[x];
			if ((val = strchr(var, '='))) {
				*val++ = '\0';
				set_profile_val(profile, var, val);
			}
		}
	}
	

	if (profile && init_profile(profile, 1) == SWITCH_STATUS_SUCCESS) {
		stream->write_function(stream, "OK\n");
	} else {
		stream->write_function(stream, "FAIL\n");
	}

	return SWITCH_STATUS_SUCCESS;

}

static switch_status_t load_config(void)
{
	char *cf = "dingaling.conf";
	struct mdl_profile *profile = NULL;
	switch_xml_t cfg, xml, settings, param, xmlint;

	memset(&globals, 0, sizeof(globals));
	globals.running = 1;
    
    switch_find_local_ip(globals.guess_ip, sizeof(globals.guess_ip), AF_INET);

	switch_core_hash_init(&globals.profile_hash, module_pool);	

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}

	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "debug")) {
				globals.debug = atoi(val);
			} else if (!strcasecmp(var, "codec-prefs")) {
				set_global_codec_string(val);
				globals.codec_order_last =
					switch_separate_string(globals.codec_string, ',', globals.codec_order, SWITCH_MAX_CODECS);
			} else if (!strcasecmp(var, "codec-rates")) {
				set_global_codec_rates_string(val);
				globals.codec_rates_last =
					switch_separate_string(globals.codec_rates_string, ',', globals.codec_rates, SWITCH_MAX_CODECS);
			}
		}
	}
	
	if (!(xmlint = switch_xml_child(cfg, "profile"))) {
		if ((xmlint = switch_xml_child(cfg, "interface"))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "!!!!!!! DEPRICATION WARNING 'interface' is now 'profile' !!!!!!!\n");
		}
	}

	for (; xmlint; xmlint = xmlint->next) {
		char *type = (char *) switch_xml_attr_soft(xmlint, "type");
		for (param = switch_xml_child(xmlint, "param"); param; param = param->next) {
			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");

			if (!globals.init) {
				ldl_global_init(globals.debug);
				ldl_global_set_logger(dl_logger);
				globals.init = 1;
			}

			if(!profile) {
				profile = switch_core_alloc(module_pool, sizeof(*profile));
			}
			set_profile_val(profile, var, val);
		}


		if (type && !strcasecmp(type, "component")) {
			char dbname[256];
			switch_core_db_t *db;

			if (!profile->login && profile->name) {
				profile->login = switch_core_strdup(module_pool, profile->name);
			}

			switch_set_flag(profile, TFLAG_AUTO);
			profile->message = "";
			profile->user_flags |= LDL_FLAG_COMPONENT;
			switch_mutex_init(&profile->mutex, SWITCH_MUTEX_NESTED, module_pool);
			snprintf(dbname, sizeof(dbname), "dingaling_%s", profile->name);
			profile->dbname = switch_core_strdup(module_pool, dbname);

			if ((db = switch_core_db_open_file(profile->dbname))) {
				switch_core_db_test_reactive(db, "select * from subscriptions", sub_sql);
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open SQL Database!\n");
				continue;
			}
			switch_core_db_close(db);
		}

		if (profile) {
			init_profile(profile, switch_test_flag(profile, TFLAG_AUTO) ? 1 : 0);
			profile = NULL;
		}
	}

	if (profile) {
		init_profile(profile, switch_test_flag(profile, TFLAG_AUTO) ? 1 : 0);
		profile = NULL;
	}

	if (!globals.dialplan) {
		set_global_dialplan("default");
	}

	if (!globals.init) {
		ldl_global_init(globals.debug);
		ldl_global_set_logger(dl_logger);
		globals.init = 1;
	}


	switch_xml_free(xml);

	return SWITCH_STATUS_SUCCESS;
}


static void execute_sql(char *dbname, char *sql, switch_mutex_t *mutex)
{
	switch_core_db_t *db;

	if (mutex) {
		switch_mutex_lock(mutex);
	}

	if (!(db = switch_core_db_open_file(dbname))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", dbname);
		goto end;
	}
	
	switch_core_db_persistant_execute(db, sql, 25);
	switch_core_db_close(db);

 end:
	if (mutex) {
		switch_mutex_unlock(mutex);
	}
}

static void do_vcard(ldl_handle_t *handle, char *to, char *from, char *id)
{
	char *params = NULL, *real_to, *to_user, *xmlstr = NULL, *to_host = NULL;
	switch_xml_t domain, xml = NULL, user, vcard;

	if (!strncasecmp(to, "user+", 5)) {
		real_to = to + 5;
	} else {
		real_to = to;
	}


	if ((to_user = strdup(real_to))) {
		if ((to_host = strchr(to_user, '@'))) {
			*to_host++ = '\0';
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
		goto end;
	}
	
	if (!to_host) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Missing Host!\n");
		goto end;
	}

	if (!(params = switch_mprintf("to=%s@%s&from=%s&object=vcard",
								  to_user,
								  to_host,
								  from
								  ))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
		goto end;
	}

	
	if (switch_xml_locate("directory", "domain", "name", to_host, &xml, &domain, params) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "can't find domain for [%s@%s]\n", to_user, to_host);
		goto end;
	}

	if (!(user = switch_xml_find_child(domain, "user", "id", to_user))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "can't find user [%s@%s]\n", to_user, to_host);
		goto end;
	}

	if (!(vcard = switch_xml_child(user, "vcard"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "can't find <vcard> tag for user [%s@%s]\n", to_user, to_host);
		goto end;
	}

	switch_xml_set_attr(vcard, "xmlns", "vcard-tmp");

	if ((xmlstr = switch_xml_toxml(vcard))) {
		ldl_handle_send_vcard(handle, to, from, id, xmlstr);
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
	}

 end:
	if (xml) switch_xml_free(xml);
	switch_safe_free(to_user);
	switch_safe_free(params);
	switch_safe_free(xmlstr);
}

static ldl_status handle_signalling(ldl_handle_t *handle, ldl_session_t *dlsession, ldl_signal_t dl_signal, char *to, char *from, char *subject, char *msg)
{
	struct mdl_profile *profile = NULL;
	switch_core_session_t *session = NULL;
	switch_channel_t *channel = NULL;
    struct private_object *tech_pvt = NULL;
	switch_event_t *event;
	ldl_status status = LDL_STATUS_SUCCESS;
	char *sql;

	assert(handle != NULL);

	if (!(profile = ldl_handle_get_private(handle))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERROR NO PROFILE!\n");
		status = LDL_STATUS_FALSE;
		goto done;
	}

	if (!dlsession) {
		if (profile->user_flags & LDL_FLAG_COMPONENT) {
			switch(dl_signal) {
			case LDL_SIGNAL_VCARD:
				do_vcard(handle, to, from, subject);
				break;
			case LDL_SIGNAL_UNSUBSCRIBE:

				if ((sql = switch_mprintf("delete from subscriptions where sub_from='%q' and sub_to='%q';", from, to))) {
					execute_sql(profile->dbname, sql, profile->mutex);
					switch_core_db_free(sql);
				}

				break;

			case LDL_SIGNAL_SUBSCRIBE:
				
				if ((sql = switch_mprintf("delete from subscriptions where sub_from='%q' and sub_to='%q';\n"
										  "insert into subscriptions values('%q','%q','%q','%q');\n", from, to, from, to, msg, subject))) {
					execute_sql(profile->dbname, sql, profile->mutex);
					switch_core_db_free(sql);
				}

				if (is_special(to)) {
					ldl_handle_send_presence(profile->handle, to, from, NULL, NULL, "Click To Call");
				}

#if 0
				if (is_special(to)) {
					if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", profile->login);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s",  to);
						//switch_event_add_header(event, SWITCH_STACK_BOTTOM, "rpid", "unknown");
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "status", "Click To Call");
						switch_event_fire(&event);
					}
				}
#endif
				break;
			case LDL_SIGNAL_ROSTER:
				if (switch_event_create(&event, SWITCH_EVENT_ROSTER) == SWITCH_STATUS_SUCCESS) {
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s", from);
					switch_event_fire(&event);
				}
				break;
			case LDL_SIGNAL_PRESENCE_PROBE:
				if (is_special(to)) {
                    ldl_handle_send_presence(profile->handle, to, from, NULL, NULL, "Click To Call");
                } else {
					if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_PROBE) == SWITCH_STATUS_SUCCESS) {
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", profile->login);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s",  from);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "to", "%s",  to);
						switch_event_fire(&event);
					}
                }
                break;
			case LDL_SIGNAL_PRESENCE_IN:
				
				if ((sql = switch_mprintf("update subscriptions set show='%q', status='%q' where sub_from='%q'", msg, subject, from))) {
					execute_sql(profile->dbname, sql, profile->mutex);
					switch_core_db_free(sql);
				}
				
				if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", profile->login);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s",  from);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "rpid", "%s", msg);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "status", "%s", subject);
					switch_event_fire(&event);
				}

				
				if (is_special(to)) {
					ldl_handle_send_presence(profile->handle, to, from, NULL, NULL, "Click To Call");
				}
#if 0
				if (is_special(to)) {
					if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", profile->login);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s",  to);
						//switch_event_add_header(event, SWITCH_STACK_BOTTOM, "rpid", "unknown");
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "status", "Click To Call");
						switch_event_fire(&event);
					}
				}
#endif
				break;

			case LDL_SIGNAL_PRESENCE_OUT:
				
				if ((sql = switch_mprintf("update subscriptions set show='%q', status='%q' where sub_from='%q'", msg, subject, from))) {
					execute_sql(profile->dbname, sql, profile->mutex);
					switch_core_db_free(sql);
				}
				if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_OUT) == SWITCH_STATUS_SUCCESS) {
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", profile->login);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s", from);
					switch_event_fire(&event);
				}
				break;
			default:
				break;
			}
		} 

		switch(dl_signal) {
		case LDL_SIGNAL_MSG: {
			switch_chat_interface_t *ci;
			char *proto = MDL_CHAT_PROTO;
			char *pproto = NULL, *ffrom = NULL;
			char *hint;

			if (profile->auto_reply) {
				ldl_handle_send_msg(handle,
									(profile->user_flags & LDL_FLAG_COMPONENT) ? to : ldl_handle_get_login(profile->handle),
									from, "", profile->auto_reply);
			}

			if (strchr(to, '+')) {
				pproto = strdup(to);
				if ((to = strchr(pproto, '+'))) {
					*to++ = '\0';
				}
				proto = pproto;
			}

			hint = from;

			if (strchr(from, '/') && (ffrom = strdup(from))) {
				char *p;
				if ((p = strchr(ffrom, '/'))) {
					*p = '\0';
				}
				from = ffrom;
			}

			if ((ci = switch_loadable_module_get_chat_interface(proto))) {
				ci->chat_send(MDL_CHAT_PROTO, from, to, subject, msg, hint);
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Chat Interface [%s]!\n", proto);
			}

			switch_safe_free(pproto);
			switch_safe_free(ffrom);
		}
			break;
		case LDL_SIGNAL_LOGIN_SUCCESS:
			if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, DL_EVENT_LOGIN_SUCCESS) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", ldl_handle_get_login(profile->handle));
				switch_event_fire(&event);
			}
			if (profile->user_flags & LDL_FLAG_COMPONENT) {
				sign_on(profile);
			}

			break;
		case LDL_SIGNAL_LOGIN_FAILURE:
			if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, DL_EVENT_LOGIN_FAILURE) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", ldl_handle_get_login(profile->handle));
				switch_event_fire(&event);
			}
			break;
		case LDL_SIGNAL_CONNECTED:
			if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, DL_EVENT_CONNECTED) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", ldl_handle_get_login(profile->handle));
				switch_event_fire(&event);
			}
			break;
		default:
			break;
			
		}
		status = LDL_STATUS_SUCCESS;
		goto done;
	}
	

	if ((session = ldl_session_get_private(dlsession))) {
		tech_pvt = switch_core_session_get_private(session);
		assert(tech_pvt != NULL);

		channel = switch_core_session_get_channel(session);
		assert(channel != NULL);
		
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "using Existing session for %s\n", ldl_session_get_id(dlsession));

		if (switch_channel_get_state(channel) >= CS_HANGUP) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Call %s is already over\n", switch_channel_get_name(channel));
			status = LDL_STATUS_FALSE;
			goto done;
		}

	} else {
		if (dl_signal != LDL_SIGNAL_INITIATE) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Session is already dead\n");
			status = LDL_STATUS_FALSE;
			goto done;
		}
		if ((session = switch_core_session_request(&channel_endpoint_interface, NULL)) != 0) {
			switch_core_session_add_stream(session, NULL);
			
			if ((tech_pvt = (struct private_object *) switch_core_session_alloc(session, sizeof(struct private_object))) != 0) {
				memset(tech_pvt, 0, sizeof(*tech_pvt));
				switch_mutex_init(&tech_pvt->flag_mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
				tech_pvt->flags |= globals.flags;
				tech_pvt->flags |= profile->flags;
				channel = switch_core_session_get_channel(session);
				switch_core_session_set_private(session, tech_pvt);
				tech_pvt->session = session;
				tech_pvt->codec_index = -1;
				tech_pvt->profile = profile;
				tech_pvt->local_port = switch_rtp_request_port();
				switch_set_flag_locked(tech_pvt, TFLAG_ANSWER);
				tech_pvt->recip = switch_core_session_strdup(session, from);
				switch_set_flag_locked(tech_pvt, TFLAG_TRANSPORT_ACCEPT);
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Hey where is my memory pool?\n");
				terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
				status = LDL_STATUS_FALSE;
				goto done;
			}
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Creating a session for %s\n", ldl_session_get_id(dlsession));
			ldl_session_set_private(dlsession, session);
			tech_pvt->dlsession = dlsession;
			switch_channel_set_name(channel, "DingaLing/new");
			switch_channel_set_state(channel, CS_INIT);
			switch_core_session_thread_launch(session);
		} else {
			status = LDL_STATUS_FALSE;
			goto done;
		}

	}

	switch(dl_signal) {
	case LDL_SIGNAL_MSG:
		if (msg) { 
			if (*msg == '+') {
				switch_channel_queue_dtmf(channel, msg + 1);
				switch_set_flag_locked(tech_pvt, TFLAG_DTMF);
				if (switch_rtp_ready(tech_pvt->rtp_session)) {
					switch_rtp_set_flag(tech_pvt->rtp_session, SWITCH_RTP_FLAG_BREAK);
				}
			}
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "SESSION MSG [%s]\n", msg);
		}

		if (switch_event_create(&event, SWITCH_EVENT_MESSAGE) == SWITCH_STATUS_SUCCESS) {
			char *hint = NULL, *p, *freeme = NULL;

			hint = from;			
			if (strchr(from, '/')) {
				freeme = strdup(from);
				p = strchr(freeme, '/');
				*p = '\0';
				from = freeme;				
			}

			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", MDL_CHAT_PROTO);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", ldl_handle_get_login(profile->handle));
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "hint", "%s", hint);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s", from);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "to", "%s", to);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "subject", "%s", subject);
			if (msg) {
				switch_event_add_body(event, "%s", msg);
			}
			if (switch_core_session_queue_event(tech_pvt->session, &event) != SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "delivery-failure", "true");
				switch_event_fire(&event);
			}

			switch_safe_free(freeme);
		}
		break;
	case LDL_SIGNAL_TRANSPORT_ACCEPT:
		switch_set_flag_locked(tech_pvt, TFLAG_TRANSPORT_ACCEPT);
		break;
	case LDL_SIGNAL_INITIATE:
		if (dl_signal) {
			ldl_payload_t *payloads;
			unsigned int len = 0;
			int match = 0;

			if (switch_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
				if (!strcasecmp(msg, "accept")) {
					switch_set_flag_locked(tech_pvt, TFLAG_ANSWER);
					if (!do_candidates(tech_pvt, 0)) {
						terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
						status = LDL_STATUS_FALSE;
						goto done;
					}
				}
			}

			if (tech_pvt->codec_index > -1) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Already decided on a codec\n");
				break;
			}


			if (!get_codecs(tech_pvt)) {
				terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
				status = LDL_STATUS_FALSE;
				goto done;
			}

			
			if (ldl_session_get_payloads(dlsession, &payloads, &len) == LDL_STATUS_SUCCESS) {
                unsigned int x, y;
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%u payloads\n", len);
				for(x = 0; x < len; x++) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Available Payload %s %u\n", payloads[x].name, payloads[x].id);
					for(y = 0; y < tech_pvt->num_codecs; y++) {
						char *name = tech_pvt->codecs[y]->iananame;

						if (!strncasecmp(name, "ilbc", 4)) {
							name = "ilbc";
						}
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "compare %s %d/%d to %s %d/%d\n", 
                                          payloads[x].name, payloads[x].id, payloads[x].rate,
                                          name, tech_pvt->codecs[y]->ianacode, tech_pvt->codecs[y]->samples_per_second);
						if (tech_pvt->codecs[y]->ianacode > 95) {
							match = strcasecmp(name, payloads[x].name) ? 0 : 1;
						} else {
							match = (payloads[x].id == tech_pvt->codecs[y]->ianacode) ? 1 : 0;
						}
						
						if (match && payloads[x].rate == tech_pvt->codecs[y]->samples_per_second) {
							tech_pvt->codec_index = y;
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Choosing Payload index %u %s %u\n", y, payloads[x].name, payloads[x].id);
							tech_pvt->codec_name = tech_pvt->codecs[y]->iananame;
							tech_pvt->codec_num = tech_pvt->codecs[y]->ianacode;
							tech_pvt->r_codec_num = (switch_payload_t)(payloads[x].id);
							tech_pvt->codec_rate = payloads[x].rate;
							if (!switch_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
								if (!do_describe(tech_pvt, 0)) {
									terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
									status = LDL_STATUS_FALSE;
									goto done;
								}
							}
							status = LDL_STATUS_SUCCESS;
							goto done;
						}
					}
				}
				if (!match && !switch_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
					if (!do_describe(tech_pvt, 0)) {
						terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
						status = LDL_STATUS_FALSE;
						goto done;
					}
				}
			}
		}
		
		break;
	case LDL_SIGNAL_CANDIDATES:
		if (dl_signal) {
			ldl_candidate_t *candidates;
			unsigned int len = 0;
			unsigned int x;

			if (ldl_session_get_candidates(dlsession, &candidates, &len) != LDL_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Candidate Error!\n");
				switch_set_flag(tech_pvt, TFLAG_BYE);
				switch_clear_flag(tech_pvt, TFLAG_IO);
				status = LDL_STATUS_FALSE;
				goto done;
			}

				
			if (tech_pvt->remote_ip) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Already picked an IP [%s]\n", tech_pvt->remote_ip);
				break;
			}

			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%u candidates\n", len);
			for(x = 0; x < len; x++) {
				uint8_t lanaddr = 0;

				if (profile->lanaddr) {
					lanaddr = strncasecmp(candidates[x].address, profile->lanaddr, strlen(profile->lanaddr)) ? 0 : 1;
				} 

				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "candidates %s:%d\n", candidates[x].address, candidates[x].port);
					
				if (!strcasecmp(candidates[x].protocol, "udp") && (!strcasecmp(candidates[x].type, "local") || !strcasecmp(candidates[x].type, "stun")) && 
					((profile->lanaddr && lanaddr) ||
					 (strncasecmp(candidates[x].address, "10.", 3) && 
					  strncasecmp(candidates[x].address, "192.168.", 8) &&
					  strncasecmp(candidates[x].address, "127.", 4) &&
					  strncasecmp(candidates[x].address, "255.", 4) &&
					  strncasecmp(candidates[x].address, "0.", 2) &&
					  strncasecmp(candidates[x].address, "1.", 2) &&
					  strncasecmp(candidates[x].address, "2.", 2) &&
					  strncasecmp(candidates[x].address, "172.16.", 7) &&
					  strncasecmp(candidates[x].address, "172.17.", 7) &&
					  strncasecmp(candidates[x].address, "172.18.", 7) &&
					  strncasecmp(candidates[x].address, "172.19.", 7) &&
					  strncasecmp(candidates[x].address, "172.2", 5) &&
					  strncasecmp(candidates[x].address, "172.30.", 7) &&
					  strncasecmp(candidates[x].address, "172.31.", 7)  &&
					  strncasecmp(candidates[x].address, "192.0.2.", 8)  && // 192.0.0.0 - 192.0.127.255 is marked as reserved, should we filter all of them?
					  strncasecmp(candidates[x].address, "169.254.", 8)
					  ))) {
					ldl_payload_t payloads[5];
					char *exten;
					char *context;
					char *cid_name;
					char *cid_num;
					char *tmp, *t, *them = NULL;

					memset(payloads, 0, sizeof(payloads));

					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Acceptable Candidate %s:%d\n", candidates[x].address, candidates[x].port);

					if (!switch_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
						switch_set_flag_locked(tech_pvt, TFLAG_TRANSPORT_ACCEPT);
						ldl_session_accept_candidate(dlsession, &candidates[x]);
					}

					if (!(exten = ldl_session_get_value(dlsession, "dnis"))) {
						exten = profile->exten;
						/* if it's _auto_ set the extension to match the username portion of the called address */
						if (!strcmp(exten, "_auto_")) {
							if ((t = ldl_session_get_callee(dlsession))) {
								if ((them = strdup(t))) {
									char *a, *b, *p;
									if ((p = strchr(them, '/'))) {
										*p = '\0';
									}

									if ((a = strchr(them, '+')) && (b = strrchr(them, '+')) && a != b) {
										*b++ = '\0';
										switch_channel_set_variable(channel, "dl_user", them);
										switch_channel_set_variable(channel, "dl_host", b);
									}
									exten = them;
								}
							}
						}
					}
						
					if (!(context = ldl_session_get_value(dlsession, "context"))) {
						context = profile->context;
					}

					if (!(cid_name = ldl_session_get_value(dlsession, "caller_id_name"))) {
						cid_name = tech_pvt->recip;
					}

					if (!(cid_num = ldl_session_get_value(dlsession, "caller_id_number"))) {
						cid_num = tech_pvt->recip;
					}

					/* context of "_auto_" means set it to the domain */
					if (profile->context && !strcmp(profile->context, "_auto_")) {
						context = profile->name;
					}
			
					tech_pvt->them = switch_core_session_strdup(session, ldl_session_get_callee(dlsession));
					tech_pvt->us = switch_core_session_strdup(session, ldl_session_get_caller(dlsession));

					if ((tmp = strdup(tech_pvt->us))) {
						char *p, *q;

						if ((p = strchr(tmp, '@'))) {
							*p++ = '\0';
							if ((q = strchr(p, '/'))) {
								*q = '\0';
							}
							switch_channel_set_variable(channel, "dl_from_user", tmp);
							switch_channel_set_variable(channel, "dl_from_host", p);
						}

						switch_safe_free(tmp);
					}
						
					if (!tech_pvt->caller_profile) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Creating an identity for %s %s <%s> %s\n", 
										  ldl_session_get_id(dlsession), cid_name, cid_num, exten);
			
						if ((tech_pvt->caller_profile = switch_caller_profile_new(switch_core_session_get_pool(session),
																				  ldl_handle_get_login(profile->handle),
																				  profile->dialplan,
																				  cid_name,
																				  cid_num,
																				  ldl_session_get_ip(dlsession),
																				  ldl_session_get_value(dlsession, "ani"),
																				  ldl_session_get_value(dlsession, "aniii"),
																				  ldl_session_get_value(dlsession, "rdnis"),
																				  (char *)modname,
																				  context,
																				  exten)) != 0) {
							char name[128];
							snprintf(name, sizeof(name), "DingaLing/%s", tech_pvt->caller_profile->destination_number);
							switch_channel_set_name(channel, name);
							switch_channel_set_caller_profile(channel, tech_pvt->caller_profile);
						}
					}

					switch_safe_free(them);
						
					if (lanaddr) {
						switch_set_flag_locked(tech_pvt, TFLAG_LANADDR);
					}

					if (!get_codecs(tech_pvt)) {
						terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
						status = LDL_STATUS_FALSE;
						goto done;
					}

						
					tech_pvt->remote_ip = switch_core_session_strdup(session, candidates[x].address);
					ldl_session_set_ip(dlsession, tech_pvt->remote_ip);
					tech_pvt->remote_port = candidates[x].port;
					tech_pvt->remote_user = switch_core_session_strdup(session, candidates[x].username);

						
					if (!switch_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
						if (!do_candidates(tech_pvt, 0)) {
							terminate_session(&session,  __LINE__, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
							status = LDL_STATUS_FALSE;
							goto done;
						}
					}
						
					status = LDL_STATUS_SUCCESS;
					goto done;
				}
			}
		}
		break;
	case LDL_SIGNAL_REJECT:
        if (channel) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "reject %s\n", switch_channel_get_name(channel));
			terminate_session(&session,  __LINE__, SWITCH_CAUSE_CALL_REJECTED);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "End Call (Rejected)\n");
			goto done;
		}
        break;
	case LDL_SIGNAL_ERROR:
	case LDL_SIGNAL_TERMINATE:
		if (channel) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "hungup %s\n", switch_channel_get_name(channel));
			terminate_session(&session,  __LINE__, SWITCH_CAUSE_NORMAL_CLEARING);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "End Call\n");
			goto done;
		}
		break;

	default:
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERROR\n");
		break;
	}

 done:
	
	return status;
}

static ldl_status handle_response(ldl_handle_t *handle, char *id)
{
	return LDL_STATUS_SUCCESS;
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
