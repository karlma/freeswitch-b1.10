/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2011, Anthony Minessale II <anthm@freeswitch.org>
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
 * mod_httapi.c -- HT-TAPI Hypertext Telephony API
 *
 */
#include <switch.h>
#include <switch_curl.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_httapi_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_httapi_shutdown);
SWITCH_MODULE_DEFINITION(mod_httapi, mod_httapi_load, mod_httapi_shutdown, NULL);

typedef struct profile_perms_s {
	switch_byte_t set_params;
	switch_byte_t set_vars;
	switch_byte_t extended_data;
	switch_byte_t execute_apps;
	switch_byte_t expand_vars;
	struct {
		switch_byte_t enabled;
		switch_byte_t set_context;
		switch_byte_t set_dp;
		switch_byte_t set_cid_name;
		switch_byte_t set_cid_number;
		switch_byte_t full_originate;
	} dial;
	struct {
		switch_byte_t enabled;
		switch_byte_t set_profile;
	} conference;
} profile_perms_t;

struct client_s;

typedef struct action_binding_s {
	char *realm;
	char *input;
	char *action;
	char *error_file;
	char *match_digits;
	char *strip;
	struct client_s *client;
	struct action_binding_s *parent;
} action_binding_t;

typedef struct client_profile_s {
	char *name;
	char *method;
	char *url;
	char *cred;
	char *bind_local;
	int disable100continue;
	uint32_t enable_cacert_check;
	char *ssl_cert_file;
	char *ssl_key_file;
	char *ssl_key_password;
	char *ssl_version;
	char *ssl_cacert_file;
	uint32_t enable_ssl_verifyhost;
	char *cookie_file;
	switch_hash_t *vars_map;
	int auth_scheme;
	int timeout;
	profile_perms_t perms;

	struct {
		char *use_profile;
	} conference_params;

	struct {
		char *context;
		char *dp;
		switch_event_t *app_list;
		int default_allow;
	} dial_params;

} client_profile_t;

#define HTTAPI_MAX_API_BYTES 1024 * 1024
#define HTTAPI_MAX_FILE_BYTES 1024 * 1024 * 100

typedef struct client_s {
	switch_memory_pool_t *pool;
	int fd;
	switch_buffer_t *buffer;
	switch_size_t bytes;
	switch_size_t max_bytes;
	switch_event_t *headers;
	switch_event_t *params;
	switch_event_t *one_time_params;
	client_profile_t *profile;
	switch_core_session_t *session;
	switch_channel_t *channel;
	action_binding_t *matching_action_binding;
	action_binding_t *no_matching_action_binding;
	struct {
		char *action;
		char *name;
		char *file;
	} record;

	int err;
	long code;
} client_t;

typedef struct hash_node {
	switch_hash_t *hash;
	struct hash_node *next;
} hash_node_t;

static struct {
	switch_memory_pool_t *pool;
	hash_node_t *hash_root;
	hash_node_t *hash_tail;
	switch_hash_t *profile_hash;
	switch_hash_t *parse_hash;
	char cache_path[128];
	int debug;
	int not_found_expires;
	int cache_ttl;
} globals;


/* for apr_pstrcat */
#define DEFAULT_PREBUFFER_SIZE 1024 * 64

struct http_file_source;

struct http_file_context {
	int samples;
	switch_file_handle_t fh;
	char *cache_file;
	char *meta_file;
	char *lock_file;
	char *metadata;
	time_t expires;
	switch_file_t *lock_fd;
	switch_memory_pool_t *pool;
	int del_on_close;

};

typedef struct http_file_context http_file_context_t;
typedef switch_status_t (*tag_parse_t)(const char *tag_name, client_t *client, switch_xml_t tag, const char *body);

static void bind_parser(const char *tag_name, tag_parse_t handler)
{
	switch_core_hash_insert(globals.parse_hash, tag_name, (void *)(intptr_t)handler);
}


#define HTTAPI_SYNTAX "[debug_on|debug_off]"
SWITCH_STANDARD_API(httapi_api_function)
{
	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (zstr(cmd)) {
		goto usage;
	}

	if (!strcasecmp(cmd, "debug_on")) {
		globals.debug = 1;
	} else if (!strcasecmp(cmd, "debug_off")) {
		globals.debug = 0;
	} else {
		goto usage;
	}

	stream->write_function(stream, "OK\n");
	return SWITCH_STATUS_SUCCESS;

  usage:
	stream->write_function(stream, "USAGE: %s\n", HTTAPI_SYNTAX);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t digit_action_callback(switch_ivr_dmachine_match_t *match)
{
	action_binding_t *action_binding = (action_binding_t *) match->user_data;
	
	action_binding->client->matching_action_binding = action_binding;
	action_binding->match_digits = switch_core_strdup(action_binding->client->pool, match->match_digits);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t digit_nomatch_action_callback(switch_ivr_dmachine_match_t *match)
{
	action_binding_t *action_binding = (action_binding_t *) match->user_data;

	action_binding->client->no_matching_action_binding = action_binding;
	
	return SWITCH_STATUS_BREAK;
}

static switch_status_t parse_break(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	return SWITCH_STATUS_FALSE;
}


static void console_log(const char *level_str, const char *msg)
{
	switch_log_level_t level = SWITCH_LOG_DEBUG;
	if (level_str) {
		level = switch_log_str2level(level_str);
		if (level == SWITCH_LOG_INVALID) {
			level = SWITCH_LOG_DEBUG;
		}
	}
	switch_log_printf(SWITCH_CHANNEL_LOG, level, "%s", switch_str_nil(msg));
}

static void console_clean_log(const char *level_str, const char *msg)
{
	switch_log_level_t level = SWITCH_LOG_DEBUG;
	if (level_str) {
		level = switch_log_str2level(level_str);
		if (level == SWITCH_LOG_INVALID) {
			level = SWITCH_LOG_DEBUG;
		}
	}
	switch_log_printf(SWITCH_CHANNEL_LOG_CLEAN, level, "%s", switch_str_nil(msg));
}

static switch_status_t parse_log(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	const char *level = switch_xml_attr(tag, "level");
	const char *clean = switch_xml_attr(tag, "clean");
	
	if (switch_true(clean)) {
		console_clean_log(level, body);
	} else {
		console_log(level, body);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t parse_playback(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	const char *file = switch_xml_attr(tag, "file");
	const char *name = switch_xml_attr(tag, "name");
	const char *error_file = switch_xml_attr(tag, "error-file");
	const char *action = switch_xml_attr(tag, "action");
	const char *digit_timeout_ = switch_xml_attr(tag, "digit-timeout");
	const char *input_timeout_ = switch_xml_attr(tag, "input-timeout");
	const char *tts_engine = NULL;
	const char *tts_voice = NULL;
	char *loops_ = (char *) switch_xml_attr(tag, "loops");
	int loops = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_ivr_dmachine_t *dmachine = NULL;
	switch_input_args_t *args = NULL, myargs = { 0 }, nullargs = { 0 };
	long digit_timeout = 1500;
	long input_timeout = 5000;
	long tmp;
	switch_xml_t bind = NULL;
	int submit = 0;
	int input = 0;
	int speak = 0, say = 0, pause = 0, play = 0, speech = 0;
	char *sub_action = NULL;
	action_binding_t *top_action_binding = NULL;
	const char *say_lang = NULL;
	const char *say_type = NULL;
	const char *say_method = NULL;
	const char *say_gender = NULL;
	const char *sp_engine = NULL;
	const char *sp_grammar = NULL;

	if (!strcasecmp(tag_name, "say")) {
		say_lang = switch_xml_attr(tag, "language");
		say_type = switch_xml_attr(tag, "type");
		say_method = switch_xml_attr(tag, "method");
		say_gender = switch_xml_attr(tag, "gender");
		
		if (zstr(say_lang) || zstr(say_type) || zstr(say_method) || zstr(body)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "speak: missing required attributes or text! (language) (type) (method) \n");
			return SWITCH_STATUS_FALSE;
		}
		
		say = 1;
		
	} else if (!strcasecmp(tag_name, "speak")) {
		tts_engine = switch_xml_attr(tag, "engine");
		tts_voice = switch_xml_attr(tag, "voice");

		if (zstr(tts_engine)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "speak: missing engine attribute!\n");
			return SWITCH_STATUS_FALSE;
		}
		speak = 1;
	} else if (!strcasecmp(tag_name, "pause")) {
		const char *ms_ = switch_xml_attr(tag, "milliseconds");
		pause = atoi(ms_);
		if (pause < 0) pause = 1000;
	} else if (!strcasecmp(tag_name, "playback")) {
		sp_engine = switch_xml_attr(tag, "asr-engine");
		sp_grammar = switch_xml_attr(tag, "asr-grammar");

		if (sp_grammar && sp_engine) {
			speech = 1;
		} else {
			play = 1;
		}
	}


	if (zstr(name)) name = "dialed_digits";

	if (loops_) {
		loops = atoi(loops_);

		if (loops < 0) {
			loops = -1;
		}
	}

	if (digit_timeout_) {
		tmp = atol(digit_timeout_);

		if (tmp > 0) {
			digit_timeout = tmp;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid digit timeout [%s]\n", digit_timeout_);
		}
	}
	
	if (input_timeout_) {
		tmp = atol(input_timeout_);

		if (tmp > 0) {
			input_timeout = tmp;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid input timeout [%s]\n", input_timeout_);
		}
	}

	if ((bind = switch_xml_child(tag, "bind"))) {
		action_binding_t *action_binding;
		const char *realm = "default";
	

		input++;

		top_action_binding = switch_core_session_alloc(client->session, sizeof(*action_binding));
		top_action_binding->client = client;
		top_action_binding->action = (char *) action;
		top_action_binding->error_file = (char *)error_file;

		switch_ivr_dmachine_create(&dmachine, "HTTAPI", NULL, digit_timeout, 0, 
								   NULL, digit_nomatch_action_callback, top_action_binding);
		
		while(bind) {
			action_binding = switch_core_session_alloc(client->session, sizeof(*action_binding));
			action_binding->realm = (char *) realm;
			action_binding->input = bind->txt;
			action_binding->client = client;
			action_binding->action = (char *) switch_xml_attr(bind, "action");
			action_binding->strip = (char *) switch_xml_attr(bind, "strip");
			action_binding->error_file = (char *) error_file;
			action_binding->parent = top_action_binding;
			
			switch_ivr_dmachine_bind(dmachine, action_binding->realm, action_binding->input, 0, digit_action_callback, action_binding);
			bind = bind->next;
		}
		
		switch_ivr_dmachine_set_realm(dmachine, realm);
		myargs.dmachine = dmachine;
		args = &myargs;
	}

	if (zstr(file) && !input) {
		file = body;
	}

	if (zstr(file) && !(speak || say || pause)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "missing file attribute!\n");
		switch_channel_hangup(client->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return SWITCH_STATUS_FALSE;
	}

	do {
		if (speak) {
			status = switch_ivr_speak_text(client->session, tts_engine, tts_voice, (char *)body, args);
		} else if (say) {
			status = switch_ivr_say(client->session, body, say_lang, say_type, say_method, say_gender, args);
		} else if (play) {
			status = switch_ivr_play_file(client->session, NULL, file, args);
		} else if (speech) {
			char *result = NULL;

			status = switch_ivr_play_and_detect_speech(client->session, file, sp_engine, sp_grammar, &result, input_timeout, args);
			
			if (!zstr(result)) {
				switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, name, result);
				switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, "input_type", "detected_speech");
				submit = 1;
				break;
			}

			input_timeout = 0;
		} else if (pause) {
			input_timeout = pause;
			status = SWITCH_STATUS_SUCCESS;
		}

		if (!switch_channel_ready(client->channel)) {
			break;
		}

		if (!input && !pause) {
			status = switch_channel_ready(client->channel) ? SWITCH_STATUS_SUCCESS : SWITCH_STATUS_FALSE;
			submit = 1;
			break;
		}

		if (input_timeout && status == SWITCH_STATUS_SUCCESS) {
			if ((status = switch_ivr_sleep(client->session, input_timeout, SWITCH_TRUE, args)) == SWITCH_STATUS_SUCCESS) {
				status = SWITCH_STATUS_BREAK;
			}
		}

		if (status == SWITCH_STATUS_BREAK) {
			if (error_file) {
				switch_ivr_play_file(client->session, NULL, error_file, &nullargs);
				switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, name, "invalid");
				switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, "input_type", "invalid");
				status = SWITCH_STATUS_SUCCESS;
			}
		} else if (status == SWITCH_STATUS_FOUND) {
			status = SWITCH_STATUS_SUCCESS;
			submit = 1;
			break;
		} else if (status != SWITCH_STATUS_SUCCESS) {
			break;
		}
	} while (loops-- > 0);
	

	if (submit) {
		if (client->matching_action_binding) {
			if (client->matching_action_binding->match_digits) {

				if (client->matching_action_binding->strip) {
					char *pp, *p;

					for(pp = client->matching_action_binding->strip; pp && *pp; pp++) {
						if ((p = strchr(client->matching_action_binding->match_digits, *pp))) {
							*p = '\0';
						}
					}
				}
				switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, name, client->matching_action_binding->match_digits);
				switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, "input_type", "dtmf");
			}
			
			if (client->matching_action_binding->action) {
				sub_action = client->matching_action_binding->action;
			} else if (client->matching_action_binding->parent && client->matching_action_binding->parent->action) {
				sub_action = client->matching_action_binding->parent->action;
			}
		}
		
		if (!sub_action && top_action_binding && top_action_binding->action) {
			sub_action = top_action_binding->action;
		}
		
		if (sub_action && client->matching_action_binding && client->matching_action_binding->match_digits) {
			if (!strncasecmp(sub_action, "dial:", 5)) {
				char *context = NULL;
				char *dp = NULL;
				
				if (client->profile->perms.dial.set_context) {
					context = switch_core_session_strdup(client->session, sub_action + 5);
					
					if ((dp = strchr(context, ':'))) {
						*dp++ = '\0';
						if (!client->profile->perms.dial.set_dp) {
							dp = NULL;
						}
					}
				}

				switch_ivr_session_transfer(client->session, client->matching_action_binding->match_digits, dp, context);
				status = SWITCH_STATUS_FALSE;
				
			} else {
				switch_event_add_header_string(client->params, SWITCH_STACK_BOTTOM, "url", sub_action);
			}
		}
	}

	if (dmachine) {
		switch_ivr_dmachine_destroy(&dmachine);
	}

	return status;
}

static switch_status_t parse_conference(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	char *str;
	char *dup, *p;
	const char *profile_name = switch_xml_attr(tag, "profile");
	
	if (!client->profile->perms.conference.enabled) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Permission Denied!\n");
		switch_channel_hangup(client->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return SWITCH_STATUS_FALSE;
	}

	dup = switch_core_session_strdup(client->session, body);

	if ((p = strchr(dup, '@'))) {
		*p = '\0';
	}

	if (zstr(profile_name) || !client->profile->perms.conference.set_profile) {
		profile_name = client->profile->conference_params.use_profile;
	}

	str = switch_core_session_sprintf(client->session, "%s@%s", dup, profile_name);
	switch_core_session_execute_application(client->session, "conference", str);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t parse_dial(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	const char *context = NULL; 
	const char *dp = NULL;
	const char *cid_name = NULL;
	const char *cid_number = NULL;

	if (!client->profile->perms.dial.enabled) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Permission Denied!\n");
		switch_channel_hangup(client->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return SWITCH_STATUS_FALSE;
	}

	if (client->profile->perms.dial.set_context) {
		context = switch_xml_attr(tag, "context");
	}

	if (client->profile->perms.dial.set_dp) {
		dp = switch_xml_attr(tag, "dialplan");
	}

	if (client->profile->perms.dial.set_cid_name) {
		cid_name = switch_xml_attr(tag, "caller-id-name");
	}

	if (client->profile->perms.dial.set_cid_number) {
		cid_number = switch_xml_attr(tag, "caller-id-number");
	}

	if (client->profile->perms.dial.full_originate && strchr(body, '/')) {
		char *str;

		if (!cid_name) {
			cid_name = switch_channel_get_variable(client->channel, "caller_id_name");
		}
		if (!cid_number) {
			cid_number = switch_channel_get_variable(client->channel, "caller_id_number");
		}

		str = switch_core_session_sprintf(client->session, 
										  "{origination_caller_id_name='%s',origination_caller_id_number='%s'}%s", cid_name, cid_number, body);

		switch_core_session_execute_application(client->session, "bridge", str);
	} else {
		switch_ivr_session_transfer(client->session, body, dp, context);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t parse_sms(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	switch_event_t *event;
	const char *from_proto = "httapi";
	const char *to_proto = "sip";
	const char *to = switch_xml_attr(tag, "to");

	if (to && switch_event_create(&event, SWITCH_EVENT_MESSAGE) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", from_proto);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "to_proto", to_proto);

		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "from", switch_channel_get_variable(client->channel, "caller_id_number"));
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "to", to);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "type", "text/plain");
		
		if (body) {
			switch_event_add_body(event, "%s", body);
		}

		switch_core_chat_send(to_proto, event);
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing 'to' Attribute!\n");
		return SWITCH_STATUS_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}

static int check_app_perm(client_t *client, const char *app_name)
{
	const char *v;
	int r = 0;

	if (!client->profile->perms.execute_apps) {
		return 0;
	}

	if (!client->profile->dial_params.app_list) {
		return 1;
	}

	if ((v = switch_event_get_header(client->profile->dial_params.app_list, app_name))) {
		if (*v == 'd') {
			r = 0;
		} else {
			r = 1;
		}
	} else {
		r = client->profile->dial_params.default_allow;
	}
	

	return r;
}

static switch_status_t parse_execute(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	const char *app_name = switch_xml_attr(tag, "application");
	const char *data = switch_xml_attr(tag, "data");

	if (zstr(data)) data = body;

	if (!check_app_perm(client, app_name)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Permission Denied!\n");
		switch_channel_hangup(client->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return SWITCH_STATUS_FALSE;
	}

	if (zstr(app_name)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid app\n");
		switch_channel_hangup(client->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return SWITCH_STATUS_FALSE;
	} else {
		if (!client->profile->perms.expand_vars) {
			const char *p;

			for(p = data; p && *p; p++) {
				if (*p == '$') {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Expand Variables: Permission Denied!\n");
					switch_channel_hangup(client->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
					return SWITCH_STATUS_FALSE;
				}
			}
		}

		switch_core_session_execute_application(client->session, app_name, data);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t parse_hangup(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	const char *cause_str = switch_xml_attr(tag, "cause");
	switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;

	if (zstr(cause_str)) {
		cause_str = body;
	}

	if (!zstr(cause_str)) {
		cause = switch_channel_str2cause(cause_str);
	}

	switch_channel_hangup(client->channel, cause);

	return SWITCH_STATUS_FALSE;
}

static switch_status_t parse_record_call(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	const char *limit_ = switch_xml_attr(tag, "limit");
	const char *name = switch_xml_attr(tag, "name");
	const char *action = switch_xml_attr(tag, "action");
	int limit = 0;

	if (client->record.file) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (zstr(name)) name = "recorded_file";

	client->record.action = (char *) action;
	client->record.name = (char *)name;
	client->record.file = switch_core_session_sprintf(client->session, "%s%s%s.wav", 
													  SWITCH_GLOBAL_dirs.temp_dir, SWITCH_PATH_SEPARATOR, switch_core_session_get_uuid(client->session));
	
	if (limit_) {
		limit = atoi(limit_);
		if (limit < 0) limit = 0;
	}
	

	switch_ivr_record_session(client->session, client->record.file, limit, NULL);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t parse_record(const char *tag_name, client_t *client, switch_xml_t tag, const char *body)
{
	const char *file = switch_xml_attr(tag, "file");
	const char *name = switch_xml_attr(tag, "name");
	const char *error_file = switch_xml_attr(tag, "error-file");
	const char *beep_file = switch_xml_attr(tag, "beep-file");
	const char *action = switch_xml_attr(tag, "action");
	const char *sub_action = NULL;
	const char *digit_timeout_ = switch_xml_attr(tag, "digit-timeout");
	char *loops_ = (char *) switch_xml_attr(tag, "loops");
	int loops = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_ivr_dmachine_t *dmachine = NULL;
	switch_input_args_t *args = NULL, myargs = { 0 };
	long digit_timeout = 1500;
	long tmp;
	int thresh = 200;
	int silence_hits = 2;
	int record_limit = 0;
	char *tmp_record_path = NULL;
	const char *v;
	int rtmp;
	char uuid_str[SWITCH_UUID_FORMATTED_LENGTH + 1];
	char *fname = NULL;
	char *p, *ext = "wav";
	switch_xml_t bind;
	action_binding_t *top_action_binding = NULL;

	switch_uuid_str(uuid_str, sizeof(uuid_str));

	if (zstr(name)) name = "attached_file";

	if (zstr(file)) {
		return SWITCH_STATUS_FALSE;
	}
	
	fname = switch_core_strdup(client->pool, file);

	for(p = fname; p && *p; p++) {
		if (*p == '/' || *p == '\\' || *p == '`') {
			*p = '_';
		}
	}

	if ((p = strrchr(fname, '.'))) {
		*p++ = '\0';
		ext = p;
	}

	for(p = fname; p && *p; p++) {
		if (*p == '.') {
			*p = '_';
		}
	}
		
	tmp_record_path = switch_core_sprintf(client->pool, "%s%s%s_%s.%s", 
										  SWITCH_GLOBAL_dirs.temp_dir, SWITCH_PATH_SEPARATOR, uuid_str, fname, ext);

	if ((v = switch_xml_attr(tag, "limit"))) {
		if ((rtmp = atoi(v)) > -1) {
			record_limit = rtmp;
		}
	}

	if ((v = switch_xml_attr(tag, "silence-hits"))) {
		if ((rtmp = atoi(v)) > -1) {
			silence_hits = rtmp;
		}
	}

	if ((v = switch_xml_attr(tag, "threshold"))) {
		if ((rtmp = atoi(v)) > -1) {
			thresh = rtmp;
		}
	}

	if (loops_) {
		loops = atoi(loops_);

		if (loops < 0) {
			loops = -1;
		}
	}

	if (digit_timeout_) {
		tmp = atol(digit_timeout_);

		if (tmp > 0) {
			digit_timeout = tmp;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid digit timeout [%s]\n", digit_timeout_);
		}
	}
	
	if ((bind = switch_xml_child(tag, "bind"))) {
		action_binding_t *action_binding;
		const char *realm = "default";
		
		top_action_binding = switch_core_session_alloc(client->session, sizeof(*action_binding));
		top_action_binding->client = client;
		top_action_binding->action = (char *) action;
		top_action_binding->error_file = (char *)error_file;

		switch_ivr_dmachine_create(&dmachine, "HTTAPI", NULL, digit_timeout, 0, 
								   NULL, digit_nomatch_action_callback, top_action_binding);
		
		while(bind) {
			action_binding = switch_core_session_alloc(client->session, sizeof(*action_binding));
			action_binding->realm = (char *) realm;
			action_binding->input = bind->txt;
			action_binding->client = client;
			action_binding->action = (char *) switch_xml_attr(bind, "action");
			action_binding->error_file = (char *) error_file;
			action_binding->parent = top_action_binding;
			
			switch_ivr_dmachine_bind(dmachine, action_binding->realm, action_binding->input, 0, digit_action_callback, action_binding);
			bind = bind->next;
		}
		
		switch_ivr_dmachine_set_realm(dmachine, realm);
		myargs.dmachine = dmachine;
		args = &myargs;
	}

	if (beep_file) {
		status = switch_ivr_play_file(client->session, NULL, beep_file, args);
	}

	if (!switch_channel_ready(client->channel)) {
		goto end;
	}

	if (status == SWITCH_STATUS_SUCCESS) {
		switch_file_handle_t fh = { 0 };
		fh.thresh = thresh;
		fh.silence_hits = silence_hits;
		
		status = switch_ivr_record_file(client->session, &fh, tmp_record_path, args, record_limit);
	}

	if (client->matching_action_binding) {
		if (client->matching_action_binding->action) {
			sub_action = client->matching_action_binding->action;
		} else if (client->matching_action_binding->parent && client->matching_action_binding->parent->action) {
			sub_action = client->matching_action_binding->parent->action;
		}
	}

	if (!sub_action && top_action_binding && top_action_binding->action) {
		sub_action = top_action_binding->action;
	}

	if (sub_action) {
		switch_event_add_header_string(client->params, SWITCH_STACK_BOTTOM, "url", sub_action);
	}

	if (!zstr(tmp_record_path) && switch_file_exists(tmp_record_path, client->pool) == SWITCH_STATUS_SUCCESS) {
		char *key = switch_core_sprintf(client->pool, "attach_file:%s:%s.%s", name, fname, ext);
		switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, key, tmp_record_path);
		status = SWITCH_STATUS_TERM;
	}

 end:

	if (dmachine) {
		switch_ivr_dmachine_destroy(&dmachine);
	}

	return status;
}


static switch_status_t parse_xml(client_t *client)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	const void *bdata;
	switch_size_t len;

	if ((len = switch_buffer_peek_zerocopy(client->buffer, &bdata)) && switch_buffer_len(client->buffer) > len) {
		switch_xml_t xml, tag, category;
		
		if (globals.debug) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Debugging Return Data:\n%s\n", (char *)bdata);
		}

		if ((xml = switch_xml_parse_str((char *)bdata, len))) {

			if (client->profile->perms.set_params) {
				if ((category = switch_xml_child(xml, "params"))) {
					tag = category->child;
					
					while(tag) {
						if (!zstr(tag->name)) {
							char *val = tag->txt;
							if (zstr(val)) {
								val = NULL;
							}
							switch_event_add_header_string(client->params, SWITCH_STACK_BOTTOM, tag->name, val);
						}
						tag = tag->sibling;
					}
				}
			}

			if (client->profile->perms.set_vars) {
				if ((category = switch_xml_child(xml, "variables"))) {
					tag = category->child;
					
					while(tag) {
						if (!zstr(tag->name)) {
							char *val = tag->txt;
							if (zstr(val)) {
								val = NULL;
							}
							switch_channel_set_variable(client->channel, tag->name, val);
						}
						tag = tag->sibling;
					}
				}
			}

			if ((category = switch_xml_child(xml, "work"))) {
				tag = category->child;
				status = SWITCH_STATUS_SUCCESS;

				while(status == SWITCH_STATUS_SUCCESS && tag) {
					if (!zstr(tag->name)) {
						tag_parse_t handler;

						if ((handler = (tag_parse_t)(intptr_t) switch_core_hash_find(globals.parse_hash, tag->name))) {
							char *expanded = tag->txt;
							switch_event_t *templ_data;

							if (tag->txt && client->profile->perms.expand_vars) {
								switch_channel_get_variables(client->channel, &templ_data);
								switch_event_merge(templ_data, client->params);
								expanded = switch_event_expand_headers(templ_data, tag->txt);
								switch_event_destroy(&templ_data);
							}

							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Process Tag: [%s]\n", tag->name);
							handler(tag->name, client, tag, expanded);
							
							if (expanded && expanded != tag->txt) {
								free(expanded);
							}

						} else {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unsupported Tag: [%s]\n", tag->name);
							status = SWITCH_STATUS_FALSE;
						}

					}
					tag = tag->ordered;
				}
			} else {
				status = SWITCH_STATUS_FALSE;
			}

			switch_xml_free(xml);
		}
	}	

	return status;
}
	

static size_t get_header_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t realsize = (size * nmemb);
	char *val, *header = NULL;
	client_t *client = userdata;

	/* validate length... Apache and IIS won't send a header larger than 16 KB */
	if (realsize == 0 || realsize > 1024 * 16) {
		return realsize;
	}

	/* get the header, adding NULL terminator if there isn't one */
	switch_zmalloc(header, realsize + 1);
	strncpy(header, (char *)ptr, realsize);

	if ((val = strchr(header, ':'))) {
		char *cr;
		*val++ = '\0';
		while(*val == ' ') val++;

		if ((cr = strchr(val, '\r')) || (cr = strchr(val, '\r'))) {
			*cr = '\0';
		}

		switch_event_add_header_string(client->headers, SWITCH_STACK_BOTTOM, header, val);
	}
	
	switch_safe_free(header);
	return realsize;
}


static size_t file_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register unsigned int realsize = (unsigned int) (size * nmemb);
	client_t *client = data;

	client->bytes += realsize;

	if (client->bytes > client->max_bytes) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Oversized file detected [%d bytes]\n", (int) client->bytes);
		client->err = 1;
		return 0;
	}

	switch_buffer_write(client->buffer, ptr, realsize);
	
	return realsize;
}

static void client_destroy(client_t **client)
{
	if (client && *client) {
		switch_memory_pool_t *pool;
		
		switch_event_destroy(&(*client)->headers);
		switch_event_destroy(&(*client)->params);
		switch_event_destroy(&(*client)->one_time_params);
		switch_buffer_destroy(&(*client)->buffer);

		pool = (*client)->pool;
		switch_core_destroy_memory_pool(&pool);
	}
}

static void client_reset(client_t *client)
{
	if (client->headers) {
		switch_event_destroy(&client->headers);
	}

	switch_event_destroy(&client->one_time_params);
	switch_event_create(&client->one_time_params, SWITCH_EVENT_CLONE);
	client->one_time_params->flags |= EF_UNIQ_HEADERS;
	
	switch_event_create(&client->headers, SWITCH_EVENT_CLONE);


	switch_buffer_zero(client->buffer);

	client->code = 0;
	client->err = 0;
	client->matching_action_binding = NULL;
	client->no_matching_action_binding = NULL;
}

static client_t *client_create(switch_core_session_t *session, const char *profile_name, switch_event_t **params)
{
	client_t *client;
	switch_memory_pool_t *pool;
	client_profile_t *profile;

	if (zstr(profile_name)) {
		profile_name = "default";
	}

	profile = (client_profile_t *) switch_core_hash_find(globals.profile_hash, profile_name);

	if (!profile) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find profile [%s]\n", profile_name);
		return NULL;
	}

	switch_core_new_memory_pool(&pool);
	client = switch_core_alloc(pool, sizeof(*client));
	client->pool = pool;

	switch_event_create(&client->headers, SWITCH_EVENT_CLONE);
	
	client->session = session;
	client->channel = switch_core_session_get_channel(session);

	
	client->profile = profile;

	client->max_bytes = HTTAPI_MAX_API_BYTES;

	switch_buffer_create_dynamic(&client->buffer, 1024, 1024, 0);

	if (params && *params) {
		client->params = *params;
		*params = NULL;
	} else {
		switch_event_create(&client->params, SWITCH_EVENT_CLONE);
		client->params->flags |= EF_UNIQ_HEADERS;
	}

	switch_event_create(&client->one_time_params, SWITCH_EVENT_CLONE);
	client->one_time_params->flags |= EF_UNIQ_HEADERS;

	switch_event_add_header_string(client->params, SWITCH_STACK_BOTTOM, "hostname", switch_core_get_switchname());

	return client;
}


static void cleanup_attachments(client_t *client)
{
	switch_event_header_t *hp;

	for (hp = client->params->headers; hp; hp = hp->next) {
		if (!strncasecmp(hp->name, "attach_file:", 12)) {
			if (switch_file_exists(hp->value, client->pool)) {
				unlink(hp->value);
			}
		}	
	}
}

static switch_status_t httapi_sync(client_t *client)
								  
{
	switch_CURL *curl_handle = NULL;
	char *data = NULL;
	switch_curl_slist_t *headers = NULL;
	char *url = NULL;
	char *dynamic_url = NULL;
	const char *session_id = NULL;
	switch_status_t status = SWITCH_STATUS_FALSE;
	int get_style_method = 0;
	char *method = NULL;
	struct curl_httppost *formpost=NULL;
	switch_event_t *save_params = NULL;

	if (client->one_time_params && client->one_time_params->headers) {
		save_params = client->params;
		switch_event_dup(&client->params, save_params);
		switch_event_merge(client->params, client->one_time_params);
		switch_event_destroy(&client->one_time_params);
		switch_event_create(&client->one_time_params, SWITCH_EVENT_CLONE);
		client->one_time_params->flags |= EF_UNIQ_HEADERS;
	}
	
	if (!(session_id = switch_event_get_header(client->params, "HTTAPI_SESSION_ID"))) {
		if (!(session_id = switch_channel_get_variable(client->channel, "HTTAPI_SESSION_ID"))) {
			session_id = switch_core_session_get_uuid(client->session);
		}
	}

	if (client->code || client->err) {
		client_reset(client);
	}


	if (!(method = switch_event_get_header(client->params, "method"))) {
		method = client->profile->method;
	}

	if (!(url = switch_event_get_header(client->params, "url"))) {
		url = client->profile->url;
		switch_event_add_header_string(client->params, SWITCH_STACK_BOTTOM, "url", url);
	}

	get_style_method = method ? strcasecmp(method, "post") : 1;
	
	switch_event_add_header_string(client->params, SWITCH_STACK_TOP, "session_id", session_id);

	dynamic_url = switch_event_expand_headers(client->params, url);

	switch_curl_process_form_post_params(client->params, curl_handle, &formpost);

	if (formpost) {
		get_style_method = 1;
	} else {
		data = switch_event_build_param_string(client->params, NULL, client->profile->vars_map);
		switch_assert(data);

		if (get_style_method) {
			char *tmp = switch_mprintf("%s%c%s", dynamic_url, strchr(dynamic_url, '?') != NULL ? '&' : '?', data);
		
			if (dynamic_url != url) {
				free(dynamic_url);
			}
		
			dynamic_url = tmp;
		}
	}

	curl_handle = switch_curl_easy_init();

	if (session_id) {
		char *hval = switch_mprintf("HTTAPI_SESSION_ID=%s", session_id);
		switch_curl_easy_setopt(curl_handle, CURLOPT_COOKIE, hval);
		free(hval);
	}

	if (!strncasecmp(dynamic_url, "https", 5)) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
	}


	if (!zstr(client->profile->cred)) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, client->profile->auth_scheme);
		switch_curl_easy_setopt(curl_handle, CURLOPT_USERPWD, client->profile->cred);
	}

	if (client->profile->disable100continue) {
		headers = switch_curl_slist_append(headers, "Expect:");
	}

	if (client->profile->enable_cacert_check) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, TRUE);
	}

	switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	if (method != NULL && strcasecmp(method, "get") && strcasecmp(method, "post")) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, method);
	}

	if (formpost) {
		curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, formpost);
	} else {
		switch_curl_easy_setopt(curl_handle, CURLOPT_POST, !get_style_method);
	}

	switch_curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	switch_curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 10);

	if (!get_style_method && !formpost) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
	}

	switch_curl_easy_setopt(curl_handle, CURLOPT_URL, dynamic_url);
	switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, file_callback);
	switch_curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, get_header_callback);
	switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) client);
	switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *) client);
	switch_curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "mod_httapi/1.0");

	if (client->profile->timeout) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, client->profile->timeout);
		switch_curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	}

	if (client->profile->ssl_cert_file) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSLCERT, client->profile->ssl_cert_file);
	}

	if (client->profile->ssl_key_file) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSLKEY, client->profile->ssl_key_file);
	}

	if (client->profile->ssl_key_password) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSLKEYPASSWD, client->profile->ssl_key_password);
	}

	if (client->profile->ssl_version) {
		if (!strcasecmp(client->profile->ssl_version, "SSLv3")) {
			switch_curl_easy_setopt(curl_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
		} else if (!strcasecmp(client->profile->ssl_version, "TLSv1")) {
			switch_curl_easy_setopt(curl_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
		}
	}

	if (client->profile->ssl_cacert_file) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_CAINFO, client->profile->ssl_cacert_file);
	}

	if (client->profile->enable_ssl_verifyhost) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2);
	}

	if (client->profile->cookie_file) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR, client->profile->cookie_file);
		switch_curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, client->profile->cookie_file);
	}

	if (client->profile->bind_local) {
		curl_easy_setopt(curl_handle, CURLOPT_INTERFACE, client->profile->bind_local);
	}

	switch_curl_easy_perform(curl_handle);
	switch_curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &client->code);
	switch_curl_easy_cleanup(curl_handle);
	switch_curl_slist_free_all(headers);

	if (formpost) {
		curl_formfree(formpost);
	}

	if (client->err) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error encountered! [%s]\ndata: [%s]\n", client->profile->url, data);
		status = SWITCH_STATUS_FALSE;
	} else {
		status = SWITCH_STATUS_SUCCESS;
	}

	switch_safe_free(data);

	if (dynamic_url != url) {
		switch_safe_free(dynamic_url);
	}

	cleanup_attachments(client);

	if (save_params) {
		switch_event_destroy(&client->params);
		client->params = save_params;
		save_params = NULL;
	}


	return status;
}

#define ENABLE_PARAM_VALUE "enabled"
static switch_status_t do_config(void)
{
	char *cf = "httapi.conf";
	switch_xml_t cfg, xml, profiles_tag, profile_tag, param, settings, tag;
	client_profile_t *profile = NULL;
	int x = 0;
	int need_vars_map = 0;
	switch_hash_t *vars_map = NULL;

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}

	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "debug")) {
				globals.debug = switch_true(val);
			} else if (!strcasecmp(var, "file-cache-ttl")) {
				int tmp = atoi(val);

				if (tmp > -1) {
					globals.cache_ttl = tmp;
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid value [%s]for file-cache-ttl\n", val);
				}

			} else if (!strcasecmp(var, "file-not-found-expires")) {
				globals.not_found_expires = atoi(val);

				if (globals.not_found_expires < 0) {
					globals.not_found_expires = -1;
				}
			}
		}
	}

	if (!(profiles_tag = switch_xml_child(cfg, "profiles"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing <profiles> tag!\n");
		goto done;
	}

	for (profile_tag = switch_xml_child(profiles_tag, "profile"); profile_tag; profile_tag = profile_tag->next) {
		char *bname = (char *) switch_xml_attr_soft(profile_tag, "name");
		char *url = NULL;
		char *bind_local = NULL;
		char *bind_cred = NULL;
		char *method = NULL;
		int disable100continue = 1;
		int timeout = 0;
		uint32_t enable_cacert_check = 0;
		char *ssl_cert_file = NULL;
		char *ssl_key_file = NULL;
		char *ssl_key_password = NULL;
		char *ssl_version = NULL;
		char *ssl_cacert_file = NULL;
		uint32_t enable_ssl_verifyhost = 0;
		char *cookie_file = NULL;
		hash_node_t *hash_node;
		int auth_scheme = CURLAUTH_BASIC;
		need_vars_map = 0;
		vars_map = NULL;

		if (zstr(bname)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "profile tag missing name field!\n");
			continue;
		}

		if (switch_core_hash_find(globals.profile_hash, bname)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Profile %s already exists\n", (char *)bname);
			continue;
		}

		if ((tag = switch_xml_child(profile_tag, "params"))) {
			for (param = switch_xml_child(tag, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "gateway-url")) {
					if (val) {
						url = val;
					}
				} else if (!strcasecmp(var, "gateway-credentials")) {
					bind_cred = val;
				} else if (!strcasecmp(var, "auth-scheme")) {
					if (*val == '=') {
						auth_scheme = 0;
						val++;
					}

					if (!strcasecmp(val, "basic")) {
						auth_scheme |= CURLAUTH_BASIC;
					} else if (!strcasecmp(val, "digest")) {
						auth_scheme |= CURLAUTH_DIGEST;
					} else if (!strcasecmp(val, "NTLM")) {
						auth_scheme |= CURLAUTH_NTLM;
					} else if (!strcasecmp(val, "GSS-NEGOTIATE")) {
						auth_scheme |= CURLAUTH_GSSNEGOTIATE;
					} else if (!strcasecmp(val, "any")) {
						auth_scheme = CURLAUTH_ANY;
					}
				} else if (!strcasecmp(var, "disable-100-continue") && !switch_true(val)) {
					disable100continue = 0;
				} else if (!strcasecmp(var, "method")) {
					method = val;
				} else if (!strcasecmp(var, "timeout")) {
					int tmp = atoi(val);
					if (tmp >= 0) {
						timeout = tmp;
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't set a negative timeout!\n");
					}
				} else if (!strcasecmp(var, "enable-cacert-check") && switch_true(val)) {
					enable_cacert_check = 1;
				} else if (!strcasecmp(var, "ssl-cert-path")) {
					ssl_cert_file = val;
				} else if (!strcasecmp(var, "ssl-key-path")) {
					ssl_key_file = val;
				} else if (!strcasecmp(var, "ssl-key-password")) {
					ssl_key_password = val;
				} else if (!strcasecmp(var, "ssl-version")) {
					ssl_version = val;
				} else if (!strcasecmp(var, "ssl-cacert-file")) {
					ssl_cacert_file = val;
				} else if (!strcasecmp(var, "enable-ssl-verifyhost") && switch_true(val)) {
					enable_ssl_verifyhost = 1;
				} else if (!strcasecmp(var, "cookie-file")) {
					cookie_file = val;
				} else if (!strcasecmp(var, "enable-post-var")) {
					if (!vars_map && need_vars_map == 0) {
						if (switch_core_hash_init(&vars_map, globals.pool) != SWITCH_STATUS_SUCCESS) {
							need_vars_map = -1;
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Can't init params hash!\n");
							continue;
						}
						need_vars_map = 1;
					}

					if (vars_map && val) {
						if (switch_core_hash_insert(vars_map, val, ENABLE_PARAM_VALUE) != SWITCH_STATUS_SUCCESS) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Can't add %s to params hash!\n", val);
						}
					}
				} else if (!strcasecmp(var, "bind-local")) {
					bind_local = val;
				}
			}
		}

		if (!url) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Profile has no url!\n");
			if (vars_map)
				switch_core_hash_destroy(&vars_map);
			continue;
		}

		if (!(profile = switch_core_alloc(globals.pool, sizeof(*profile)))) {
			if (vars_map)
				switch_core_hash_destroy(&vars_map);
			goto done;
		}
		memset(profile, 0, sizeof(*profile));

		/* Defaults */
		profile->conference_params.use_profile = "default";
		profile->perms.set_params = 1;
		profile->perms.conference.enabled = 1;
		profile->perms.dial.enabled = 1;

		if ((tag = switch_xml_child(profile_tag, "conference"))) {
			for (param = switch_xml_child(tag, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "default-profile")) {
					profile->conference_params.use_profile = switch_core_strdup(globals.pool, val);
				}
			}
		}

		if ((tag = switch_xml_child(profile_tag, "dial"))) {
			for (param = switch_xml_child(tag, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "context")) {
					profile->dial_params.context = switch_core_strdup(globals.pool, val);
				} else if (!strcasecmp(var, "dialplan")) {
					profile->dial_params.dp = switch_core_strdup(globals.pool, val);
				}
			}
		}

		if ((tag = switch_xml_child(profile_tag, "permissions"))) {
			for (param = switch_xml_child(tag, "permission"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "all")) {
					switch_byte_t all = switch_true(val);
					memset(&profile->perms, all, sizeof(profile->perms));
				} else if (!strcasecmp(var, "none")) {
					switch_byte_t none = switch_true(val);
					memset(&profile->perms, none, sizeof(profile->perms));
				} else if (!strcasecmp(var, "set-params")) {
					profile->perms.set_params = switch_true(val);
				} else if (!strcasecmp(var, "set-vars")) {
					profile->perms.set_vars = switch_true(val);
				} else if (!strcasecmp(var, "extended-data")) {
					profile->perms.extended_data = switch_true(val);
				} else if (!strcasecmp(var, "execute-apps")) {
					profile->perms.execute_apps = switch_true(val);
					
					if (profile->perms.execute_apps) {
						switch_xml_t x_list, x_app;
						if ((x_list = switch_xml_child(param, "application-list"))) {
							char *var = (char *) switch_xml_attr_soft(param, "default");
							
							profile->dial_params.default_allow = (var && !strcasecmp(var, "allow"));
							switch_event_create(&profile->dial_params.app_list, SWITCH_EVENT_CLONE);
							profile->dial_params.app_list->flags |= EF_UNIQ_HEADERS;

							for (x_app = switch_xml_child(x_list, "application"); x_app; x_app = x_app->next) {
								const char *name = switch_xml_attr(x_app, "name");
								const char *type = switch_xml_attr(x_app, "type");

								if (zstr(type)) type = profile->dial_params.default_allow ? "deny" : "allow";

								if (name) {
									switch_event_add_header_string(profile->dial_params.app_list, SWITCH_STACK_BOTTOM, name, type);
								}
							}
						}
					}
					
				} else if (!strcasecmp(var, "expand-vars")) {
					profile->perms.expand_vars = switch_true(val);
				} else if (!strcasecmp(var, "dial")) {
					profile->perms.dial.enabled = switch_true(val);
				} else if (!strcasecmp(var, "dial-set-context")) {
					profile->perms.dial.enabled = switch_true(val);
					profile->perms.dial.set_context = switch_true(val);
				} else if (!strcasecmp(var, "dial-set-dialplan")) {
					profile->perms.dial.enabled = switch_true(val);
					profile->perms.dial.set_dp = switch_true(val);
				} else if (!strcasecmp(var, "dial-set-cid-name")) {
					profile->perms.dial.enabled = switch_true(val);
					profile->perms.dial.set_cid_name = switch_true(val);
				} else if (!strcasecmp(var, "dial-set-cid-number")) {
					profile->perms.dial.enabled = switch_true(val);
					profile->perms.dial.set_cid_number = switch_true(val);
				} else if (!strcasecmp(var, "dial-full-originate")) {
					profile->perms.dial.enabled = switch_true(val);
					profile->perms.dial.full_originate = switch_true(val);
				} else if (!strcasecmp(var, "conference")) {
					profile->perms.conference.enabled = switch_true(val);
				} else if (!strcasecmp(var, "conference-set-profile")) {
					profile->perms.conference.enabled = switch_true(val);
					profile->perms.conference.set_profile = switch_true(val);
				}

			}
		}



		profile->auth_scheme = auth_scheme;
		profile->timeout = timeout;
		profile->url = strdup(url);
		switch_assert(profile->url);

		if (bind_local != NULL) {
			profile->bind_local = strdup(bind_local);
		}
		if (method != NULL) {
			profile->method = strdup(method);
		} else {
			profile->method = NULL;
		}

		if (bind_cred) {
			profile->cred = strdup(bind_cred);
		}

		profile->disable100continue = disable100continue;
		profile->enable_cacert_check = enable_cacert_check;

		if (ssl_cert_file) {
			profile->ssl_cert_file = strdup(ssl_cert_file);
		}

		if (ssl_key_file) {
			profile->ssl_key_file = strdup(ssl_key_file);
		}

		if (ssl_key_password) {
			profile->ssl_key_password = strdup(ssl_key_password);
		}

		if (ssl_version) {
			profile->ssl_version = strdup(ssl_version);
		}

		if (ssl_cacert_file) {
			profile->ssl_cacert_file = strdup(ssl_cacert_file);
		}

		profile->enable_ssl_verifyhost = enable_ssl_verifyhost;

		if (cookie_file) {
			profile->cookie_file = strdup(cookie_file);
		}

		profile->vars_map = vars_map;

		if (vars_map) {
			switch_zmalloc(hash_node, sizeof(hash_node_t));
			hash_node->hash = vars_map;
			hash_node->next = NULL;

			if (!globals.hash_root) {
				globals.hash_root = hash_node;
				globals.hash_tail = globals.hash_root;
			}

			else {
				globals.hash_tail->next = hash_node;
				globals.hash_tail = globals.hash_tail->next;
			}

		}

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Profile [%s] JSON Function [%s]\n",
						  zstr(bname) ? "N/A" : bname, profile->url);

		profile->name = strdup(bname);

		switch_core_hash_insert(globals.profile_hash, bname, profile);

		x++;
		profile = NULL;
	}

  done:

	switch_xml_free(xml);

	return x ? SWITCH_STATUS_SUCCESS : SWITCH_STATUS_FALSE;
}

static switch_status_t my_on_reporting(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	client_t *client;
	const char *var;

	if (!(client = (client_t *) switch_channel_get_private(channel, "_HTTAPI_CLIENT_"))) {
		return status;
	}

	switch_channel_set_private(channel, "_HTTAPI_CLIENT_", NULL);

	if (client->profile->perms.extended_data) {
		switch_channel_event_set_extended_data(channel, client->one_time_params);
	}

	switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, "exiting", "true");
	
	if (client->record.file) {
		char *key = switch_core_sprintf(client->pool, "attach_file:%s:%s.wav", client->record.name, switch_core_session_get_uuid(session));
		switch_ivr_stop_record_session(client->session, client->record.file);
		switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, key, client->record.file);
	}

	var = switch_event_get_header(client->params, "url");

	if (client->record.action) {
		if (strcmp(var, client->record.action)) {
			switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, "url", client->record.action);
			httapi_sync(client);
			if (client->profile->perms.extended_data) {
				switch_channel_event_set_extended_data(channel, client->one_time_params);
			}
			switch_event_add_header_string(client->one_time_params, SWITCH_STACK_BOTTOM, "exiting", "true");
		}
	}
	
	httapi_sync(client);

	client_destroy(&client);

	return status;
}

static switch_state_handler_table_t state_handlers = {
	/*.on_init */ NULL,
	/*.on_routing */ NULL,
	/*.on_execute */ NULL,
	/*.on_hangup */ NULL,
	/*.on_exchange_media */ NULL,
	/*.on_soft_execute */ NULL,
	/*.on_consume_media */ NULL,
	/*.on_hibernate */ NULL,
	/*.on_reset */ NULL,
	/*.on_park */ NULL,
	/*.on_reporting */ my_on_reporting,
    /*.on_destroy */ NULL,
    SSH_FLAG_STICKY
};


SWITCH_STANDARD_APP(httapi_function)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	char *parsed = NULL;
	const char *profile_name = NULL;
	client_t *client;
	switch_event_t *params = NULL;
	uint32_t loops = 0, all_extended = 0;
	
	if (!zstr(data)) {
		switch_event_create_brackets((char *)data, '{', '}', ',', &params, &parsed, SWITCH_TRUE);
	}

	if ((client = (client_t *) switch_channel_get_private(channel, "_HTTAPI_CLIENT_"))) {
		if (params) {
			switch_event_merge(client->params, params);
			switch_event_destroy(&params);
		}
	} else {
		if (params) {
			profile_name = switch_event_get_header(params, "httapi_profile");
		}

		if (zstr(profile_name) && !(profile_name = switch_channel_get_variable(channel, "httapi_profile"))) {
			profile_name = "default";
		}
		
		if ((client = client_create(session, profile_name, &params))) {
			switch_channel_set_private(channel, "_HTTAPI_CLIENT_", client);
			switch_channel_add_state_handler(channel, &state_handlers);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find suitable profile\n");
			switch_event_destroy(&params);
			return;
		}
	}

	if (client->profile->perms.extended_data) {
		all_extended = switch_true(switch_event_get_header(client->params, "full_channel_data_on_every_req"));
	}

	while(switch_channel_ready(channel)) {
		switch_status_t status = SWITCH_STATUS_FALSE;

		if (client->profile->perms.extended_data && (!loops++ || all_extended)) {
			switch_channel_event_set_extended_data(channel, client->one_time_params);
		}

		if ((status = httapi_sync(client)) == SWITCH_STATUS_SUCCESS) {
			if (client->code == 200) {
				const char *ct = switch_event_get_header(client->headers, "content-type");

				if (switch_stristr("text/xml", ct)) {
					status = parse_xml(client);
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Received unsupported content-type %s\n", ct);
					break;
				}
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Received HTTP response: %ld.\n", client->code);
				break;
			}
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error %d!\n", client->err);
		}

		if (status == SWITCH_STATUS_TERM) {
			httapi_sync(client);
		}

		if (status != SWITCH_STATUS_SUCCESS) {
			break;
		}
	}


	switch_safe_free(parsed);
	
}


/* HTTP FILE INTERFACE */

static char *load_cache_data(http_file_context_t *context, const char *url)
{
	char *ext;
	char digest[SWITCH_MD5_DIGEST_STRING_SIZE] = { 0 };
	char meta_buffer[1024] = "";
	int fd;
	switch_ssize_t bytes;

	switch_md5_string(digest, (void *) url, strlen(url));
	
	if ((ext = strrchr(url, '.'))) {
		ext++;
	} else {
		ext = "wav";
	}

	context->cache_file = switch_core_sprintf(context->pool, "%s%s%s.%s", globals.cache_path, SWITCH_PATH_SEPARATOR, digest, ext);
	context->meta_file = switch_core_sprintf(context->pool, "%s.meta", context->cache_file);
	context->lock_file = switch_core_sprintf(context->pool, "%s.lock", context->cache_file);

	if (switch_file_exists(context->meta_file, context->pool) == SWITCH_STATUS_SUCCESS && ((fd = open(context->meta_file, O_RDONLY, 0)) > -1)) {
		if ((bytes = read(fd, meta_buffer, sizeof(meta_buffer))) > 0) {
			char *p;

			if ((p = strchr(meta_buffer, ':'))) {
				*p++ = '\0';
				context->expires = (time_t) atol(meta_buffer);
				context->metadata = switch_core_strdup(context->pool, p);
			}
		}
		close(fd);
	}

	return context->cache_file;
}

static size_t save_file_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register unsigned int realsize = (unsigned int) (size * nmemb);
	client_t *client = data;
	int x;

	client->bytes += realsize;
	
	

	if (client->bytes > client->max_bytes) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Oversized file detected [%d bytes]\n", (int) client->bytes);
		client->err = 1;
		return 0;
	}

	x = write(client->fd, ptr, realsize);

	if (x != (int) realsize) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Short write! %d out of %d\n", x, realsize);
	}
	return x;
}



static switch_status_t fetch_cache_data(const char *url, switch_event_t **headers, const char *save_path)
{
	switch_CURL *curl_handle = NULL;
	client_t client = { 0 };
	long code;
	switch_status_t status = SWITCH_STATUS_FALSE;

	client.fd = -1;

	if (save_path) {
		if ((client.fd = open(save_path, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
			return SWITCH_STATUS_FALSE;
		}
	}
	
	curl_handle = switch_curl_easy_init();

	switch_curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);

	if (!strncasecmp(url, "https", 5)) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
	}

	client.max_bytes = HTTAPI_MAX_FILE_BYTES;

	switch_curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	switch_curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 10);
	switch_curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	if (save_path) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, save_file_callback);
		switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &client);
	} else {
		switch_curl_easy_setopt(curl_handle, CURLOPT_HEADER, 1);
		switch_curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);
	}

	if (headers) {
		switch_event_create(&client.headers, SWITCH_EVENT_CLONE);
		if (save_path) {
			switch_curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, get_header_callback);
			switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *) &client);
		} else {
			switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, get_header_callback);
			switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &client);
		}
	}
	
	switch_curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "mod_httapi/1.0");
	switch_curl_easy_perform(curl_handle);
	switch_curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &code);
	switch_curl_easy_cleanup(curl_handle);
	
	if (client.fd > -1) {
		close(client.fd);
	}

	if (headers && client.headers) {
		switch_event_add_header(client.headers, SWITCH_STACK_BOTTOM, "http-response-code", "%ld", code);
		*headers = client.headers;
	}

	switch(code) {
	case 200:
		if (save_path) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "caching: url:%s to %s (%" SWITCH_SIZE_T_FMT " bytes)\n", url, save_path, client.bytes);
		}
		status = SWITCH_STATUS_SUCCESS;
		break;

	case 404:
		status = SWITCH_STATUS_NOTFOUND;
		break;
		
	default:
		status = SWITCH_STATUS_FALSE;
		break;
	}


	return status;
}

static switch_status_t write_meta_file(http_file_context_t *context, const char *data, switch_event_t *headers)
{
	int fd;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char write_data[1024];

	if ((fd = open(context->meta_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
		return SWITCH_STATUS_FALSE;
	}

	if (!zstr(data)) {
		int ttl = globals.cache_ttl;
		const char *cc;
		const char *p;

		if (headers && (cc = switch_event_get_header(headers, "Cache-Control"))) {
			if ((p = switch_stristr("max-age=", cc))) {
				p += 8;
				
				if (!zstr(p)) {
					ttl = atoi(p);
					if (ttl < 0) ttl = globals.cache_ttl;
				}
			}

			if (switch_stristr("no-cache", cc) || switch_stristr("no-store", cc)) {
				context->del_on_close = 1;
			}
		}

		switch_snprintf(write_data, sizeof(write_data), 
						"%" SWITCH_TIME_T_FMT ":%s",
						switch_epoch_time_now(NULL) + ttl,
						data);
		

		status = write(fd, write_data, strlen(write_data) + 1) > 0 ? SWITCH_STATUS_SUCCESS : SWITCH_STATUS_FALSE;
	}

	close(fd);

	return status;
}


static switch_status_t lock_file(http_file_context_t *context, switch_bool_t lock)
{

	switch_status_t status = SWITCH_STATUS_SUCCESS;


	if (lock) {
		if (switch_file_open(&context->lock_fd,
							 context->lock_file,
							 SWITCH_FOPEN_WRITE | SWITCH_FOPEN_CREATE | SWITCH_FOPEN_TRUNCATE,
							 SWITCH_FPROT_UREAD | SWITCH_FPROT_UWRITE, context->pool) != SWITCH_STATUS_SUCCESS) {
			return SWITCH_STATUS_FALSE;
		}
		

		if (switch_file_lock(context->lock_fd, SWITCH_FLOCK_EXCLUSIVE) != SWITCH_STATUS_SUCCESS) {
			return SWITCH_STATUS_FALSE;
		}
	} else {
		if (context->lock_fd){ 
			switch_file_close(context->lock_fd);
			status = SWITCH_STATUS_SUCCESS;
		}

		unlink(context->lock_file);
	}

	return status;
}


static switch_status_t locate_url_file(http_file_context_t *context, const char *url)
{
	switch_event_t *headers = NULL;
	int unreachable = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;
	time_t now = switch_epoch_time_now(NULL);
	char *metadata;

	load_cache_data(context, url);

	if (context->expires && now < context->expires) {
		return SWITCH_STATUS_SUCCESS;
	}

	lock_file(context, SWITCH_TRUE);

	if ((status = fetch_cache_data(url, &headers, NULL)) != SWITCH_STATUS_SUCCESS) {
		if (status == SWITCH_STATUS_NOTFOUND) {
			unreachable = 2;
			if (now - context->expires < globals.not_found_expires) {
				switch_goto_status(SWITCH_STATUS_SUCCESS, end);
			}
		} else {
			unreachable = 1;
		}
	}

	if (!unreachable && !zstr(context->metadata)) {
		metadata = switch_core_sprintf(context->pool, "%s:%s:%s:%s",
									   url,
									   switch_event_get_header_nil(headers, "last-modified"),
									   switch_event_get_header_nil(headers, "etag"),
									   switch_event_get_header_nil(headers, "content-length")
									   );

		if (!strcmp(metadata, context->metadata)) {
			write_meta_file(context, metadata, headers);
			switch_goto_status(SWITCH_STATUS_SUCCESS, end);
		}
	}

	switch_event_destroy(&headers);
	fetch_cache_data(url, &headers, context->cache_file);
	metadata = switch_core_sprintf(context->pool, "%s:%s:%s:%s",
								   url,
								   switch_event_get_header_nil(headers, "last-modified"),
								   switch_event_get_header_nil(headers, "etag"),
								   switch_event_get_header_nil(headers, "content-length")
								   );
	
	write_meta_file(context, metadata, headers);
	
	if (switch_file_exists(context->cache_file, context->pool) == SWITCH_STATUS_SUCCESS) {
		status = SWITCH_STATUS_SUCCESS;
	}

 end:

	if (status != SWITCH_STATUS_SUCCESS) {
		unlink(context->meta_file);
		unlink(context->cache_file);
	}

	lock_file(context, SWITCH_FALSE);

	switch_event_destroy(&headers);
	
	return status;
}


static switch_status_t http_file_file_seek(switch_file_handle_t *handle, unsigned int *cur_sample, int64_t samples, int whence)
{
	http_file_context_t *context = handle->private_info;

	if (!handle->seekable) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "File is not seekable\n");
		return SWITCH_STATUS_NOTIMPL;
	}

	return switch_core_file_seek(&context->fh, cur_sample, samples, whence);
}

static switch_status_t http_file_file_open(switch_file_handle_t *handle, const char *path)
{
	http_file_context_t *context;
	char *file_dup;
	switch_status_t status;

	if (switch_test_flag(handle, SWITCH_FILE_FLAG_WRITE)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "This format does not support writing!\n");
		return SWITCH_STATUS_FALSE;
	}

	context = switch_core_alloc(handle->memory_pool, sizeof(*context));
	context->pool = handle->memory_pool;
	
	file_dup = switch_core_sprintf(handle->memory_pool, "http://%s", path);
	
	if ((status = locate_url_file(context, file_dup)) != SWITCH_STATUS_SUCCESS) {
		return status;
	}

	handle->private_info = context;
	
	if ((status = switch_core_file_open(&context->fh,
										context->cache_file, 
										handle->channels, 
										handle->samplerate, 
										SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT, NULL)) != SWITCH_STATUS_SUCCESS) {
		return status;
	}

	handle->samples = context->fh.samples;
	handle->format = context->fh.format;
	handle->sections = context->fh.sections;
	handle->seekable = context->fh.seekable;
	handle->speed = context->fh.speed;
	handle->interval = context->fh.interval;

	if (switch_test_flag((&context->fh), SWITCH_FILE_NATIVE)) {
		switch_set_flag(handle, SWITCH_FILE_NATIVE);
	} else {
		switch_clear_flag(handle, SWITCH_FILE_NATIVE);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t http_file_file_close(switch_file_handle_t *handle)
{
	http_file_context_t *context = handle->private_info;

	if (switch_test_flag((&context->fh), SWITCH_FILE_OPEN)) {
		switch_core_file_close(&context->fh);
	}
	
	if (context->del_on_close) {
		if (context->cache_file) {
			unlink(context->cache_file);
			unlink(context->meta_file);
			unlink(context->lock_file);
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t http_file_file_read(switch_file_handle_t *handle, void *data, size_t *len)
{
	http_file_context_t *context = handle->private_info;
	switch_status_t status;

	if (context->samples > 0) {
		if (*len > (size_t) context->samples) {
			*len = context->samples;
		}

		context->samples -= *len;
		memset(data, 255, *len *2);
		status = SWITCH_STATUS_SUCCESS;
	} else {
		status = switch_core_file_read(&context->fh, data, len);
	}

	return status;
}

/* Registration */

static char *http_file_supported_formats[SWITCH_MAX_CODECS] = { 0 };


/* /HTTP FILE INTERFACE */

SWITCH_MODULE_LOAD_FUNCTION(mod_httapi_load)
{
	switch_api_interface_t *httapi_api_interface;
	switch_application_interface_t *app_interface;
	switch_file_interface_t *file_interface;
	
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	memset(&globals, 0, sizeof(globals));
	globals.pool = pool;
	globals.hash_root = NULL;
	globals.hash_tail = NULL;
	globals.cache_ttl = 300;
	globals.not_found_expires = 300;


	http_file_supported_formats[0] = "http";

	file_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_FILE_INTERFACE);
	file_interface->interface_name = modname;
	file_interface->extens = http_file_supported_formats;
	file_interface->file_open = http_file_file_open;
	file_interface->file_close = http_file_file_close;
	file_interface->file_read = http_file_file_read;
	file_interface->file_seek = http_file_file_seek;
	
	switch_snprintf(globals.cache_path, sizeof(globals.cache_path), "%s%shttp_file_cache", SWITCH_GLOBAL_dirs.storage_dir, SWITCH_PATH_SEPARATOR);
	switch_dir_make_recursive(globals.cache_path, SWITCH_DEFAULT_DIR_PERMS, pool);

	
	switch_core_hash_init(&globals.profile_hash, globals.pool);
	switch_core_hash_init_case(&globals.parse_hash, globals.pool, SWITCH_FALSE);

	bind_parser("execute", parse_execute);
	bind_parser("sms", parse_sms);
	bind_parser("dial", parse_dial);
	bind_parser("pause", parse_playback);
	bind_parser("hangup", parse_hangup);
	bind_parser("record", parse_record);
	bind_parser("recordCall", parse_record_call);
	bind_parser("playback", parse_playback);
	bind_parser("speak", parse_playback);
	bind_parser("say", parse_playback);
	bind_parser("conference", parse_conference);
	bind_parser("break", parse_break);
	bind_parser("log", parse_log);

	if (do_config() != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_FALSE;
	}
	
	SWITCH_ADD_API(httapi_api_interface, "httapi",
				   "HT-TAPI Hypertext Telephony API", httapi_api_function, HTTAPI_SYNTAX);

	SWITCH_ADD_APP(app_interface, "httapi", 
				   "HT-TAPI Hypertext Telephony API", 
				   "HT-TAPI Hypertext Telephony API", httapi_function, "{<param1>=<val1>}", SAF_SUPPORT_NOMEDIA);
				   


	switch_console_set_complete("add httapi debug_on");
	switch_console_set_complete("add httapi debug_off");

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_httapi_shutdown)
{
	hash_node_t *ptr = NULL;
	client_profile_t *profile;
	switch_hash_index_t *hi;
	void *val;
	const void *vvar;

	for (hi = switch_hash_first(NULL, globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, &vvar, NULL, &val);
		profile = (client_profile_t *) val;
		switch_event_destroy(&profile->dial_params.app_list);
	}


	switch_core_hash_destroy(&globals.profile_hash);	
	switch_core_hash_destroy(&globals.parse_hash);	
		
	while (globals.hash_root) {
		ptr = globals.hash_root;
		switch_core_hash_destroy(&ptr->hash);
		globals.hash_root = ptr->next;
		switch_safe_free(ptr);
	}

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


