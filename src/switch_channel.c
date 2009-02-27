
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
 * Michael Jerris <mike@jerris.com>
 *
 *
 * switch_channel.c -- Media Channel Interface
 *
 */

#include <switch.h>
#include <switch_channel.h>

struct switch_cause_table {
	const char *name;
	switch_call_cause_t cause;
};

static struct switch_cause_table CAUSE_CHART[] = {
	{"NONE", SWITCH_CAUSE_NONE},
	{"UNALLOCATED_NUMBER", SWITCH_CAUSE_UNALLOCATED_NUMBER},
	{"NO_ROUTE_TRANSIT_NET", SWITCH_CAUSE_NO_ROUTE_TRANSIT_NET},
	{"NO_ROUTE_DESTINATION", SWITCH_CAUSE_NO_ROUTE_DESTINATION},
	{"CHANNEL_UNACCEPTABLE", SWITCH_CAUSE_CHANNEL_UNACCEPTABLE},
	{"CALL_AWARDED_DELIVERED", SWITCH_CAUSE_CALL_AWARDED_DELIVERED},
	{"NORMAL_CLEARING", SWITCH_CAUSE_NORMAL_CLEARING},
	{"USER_BUSY", SWITCH_CAUSE_USER_BUSY},
	{"NO_USER_RESPONSE", SWITCH_CAUSE_NO_USER_RESPONSE},
	{"NO_ANSWER", SWITCH_CAUSE_NO_ANSWER},
	{"SUBSCRIBER_ABSENT", SWITCH_CAUSE_SUBSCRIBER_ABSENT},
	{"CALL_REJECTED", SWITCH_CAUSE_CALL_REJECTED},
	{"NUMBER_CHANGED", SWITCH_CAUSE_NUMBER_CHANGED},
	{"REDIRECTION_TO_NEW_DESTINATION", SWITCH_CAUSE_REDIRECTION_TO_NEW_DESTINATION},
	{"EXCHANGE_ROUTING_ERROR", SWITCH_CAUSE_EXCHANGE_ROUTING_ERROR},
	{"DESTINATION_OUT_OF_ORDER", SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER},
	{"INVALID_NUMBER_FORMAT", SWITCH_CAUSE_INVALID_NUMBER_FORMAT},
	{"FACILITY_REJECTED", SWITCH_CAUSE_FACILITY_REJECTED},
	{"RESPONSE_TO_STATUS_ENQUIRY", SWITCH_CAUSE_RESPONSE_TO_STATUS_ENQUIRY},
	{"NORMAL_UNSPECIFIED", SWITCH_CAUSE_NORMAL_UNSPECIFIED},
	{"NORMAL_CIRCUIT_CONGESTION", SWITCH_CAUSE_NORMAL_CIRCUIT_CONGESTION},
	{"NETWORK_OUT_OF_ORDER", SWITCH_CAUSE_NETWORK_OUT_OF_ORDER},
	{"NORMAL_TEMPORARY_FAILURE", SWITCH_CAUSE_NORMAL_TEMPORARY_FAILURE},
	{"SWITCH_CONGESTION", SWITCH_CAUSE_SWITCH_CONGESTION},
	{"ACCESS_INFO_DISCARDED", SWITCH_CAUSE_ACCESS_INFO_DISCARDED},
	{"REQUESTED_CHAN_UNAVAIL", SWITCH_CAUSE_REQUESTED_CHAN_UNAVAIL},
	{"PRE_EMPTED", SWITCH_CAUSE_PRE_EMPTED},
	{"FACILITY_NOT_SUBSCRIBED", SWITCH_CAUSE_FACILITY_NOT_SUBSCRIBED},
	{"OUTGOING_CALL_BARRED", SWITCH_CAUSE_OUTGOING_CALL_BARRED},
	{"INCOMING_CALL_BARRED", SWITCH_CAUSE_INCOMING_CALL_BARRED},
	{"BEARERCAPABILITY_NOTAUTH", SWITCH_CAUSE_BEARERCAPABILITY_NOTAUTH},
	{"BEARERCAPABILITY_NOTAVAIL", SWITCH_CAUSE_BEARERCAPABILITY_NOTAVAIL},
	{"SERVICE_UNAVAILABLE", SWITCH_CAUSE_SERVICE_UNAVAILABLE},
	{"CHAN_NOT_IMPLEMENTED", SWITCH_CAUSE_CHAN_NOT_IMPLEMENTED},
	{"FACILITY_NOT_IMPLEMENTED", SWITCH_CAUSE_FACILITY_NOT_IMPLEMENTED},
	{"SERVICE_NOT_IMPLEMENTED", SWITCH_CAUSE_SERVICE_NOT_IMPLEMENTED},
	{"INVALID_CALL_REFERENCE", SWITCH_CAUSE_INVALID_CALL_REFERENCE},
	{"INCOMPATIBLE_DESTINATION", SWITCH_CAUSE_INCOMPATIBLE_DESTINATION},
	{"INVALID_MSG_UNSPECIFIED", SWITCH_CAUSE_INVALID_MSG_UNSPECIFIED},
	{"MANDATORY_IE_MISSING", SWITCH_CAUSE_MANDATORY_IE_MISSING},
	{"MESSAGE_TYPE_NONEXIST", SWITCH_CAUSE_MESSAGE_TYPE_NONEXIST},
	{"WRONG_MESSAGE", SWITCH_CAUSE_WRONG_MESSAGE},
	{"IE_NONEXIST", SWITCH_CAUSE_IE_NONEXIST},
	{"INVALID_IE_CONTENTS", SWITCH_CAUSE_INVALID_IE_CONTENTS},
	{"WRONG_CALL_STATE", SWITCH_CAUSE_WRONG_CALL_STATE},
	{"RECOVERY_ON_TIMER_EXPIRE", SWITCH_CAUSE_RECOVERY_ON_TIMER_EXPIRE},
	{"MANDATORY_IE_LENGTH_ERROR", SWITCH_CAUSE_MANDATORY_IE_LENGTH_ERROR},
	{"PROTOCOL_ERROR", SWITCH_CAUSE_PROTOCOL_ERROR},
	{"INTERWORKING", SWITCH_CAUSE_INTERWORKING},
	{"SUCCESS", SWITCH_CAUSE_SUCCESS},
	{"ORIGINATOR_CANCEL", SWITCH_CAUSE_ORIGINATOR_CANCEL},
	{"CRASH", SWITCH_CAUSE_CRASH},
	{"SYSTEM_SHUTDOWN", SWITCH_CAUSE_SYSTEM_SHUTDOWN},
	{"LOSE_RACE", SWITCH_CAUSE_LOSE_RACE},
	{"MANAGER_REQUEST", SWITCH_CAUSE_MANAGER_REQUEST},
	{"BLIND_TRANSFER", SWITCH_CAUSE_BLIND_TRANSFER},
	{"ATTENDED_TRANSFER", SWITCH_CAUSE_ATTENDED_TRANSFER},
	{"ALLOTTED_TIMEOUT", SWITCH_CAUSE_ALLOTTED_TIMEOUT},
	{"USER_CHALLENGE", SWITCH_CAUSE_USER_CHALLENGE},
	{"MEDIA_TIMEOUT", SWITCH_CAUSE_MEDIA_TIMEOUT},
	{"PICKED_OFF", SWITCH_CAUSE_PICKED_OFF},
	{"USER_NOT_REGISTERED", SWITCH_CAUSE_USER_NOT_REGISTERED},
	{"PROGRESS_TIMEOUT", SWITCH_CAUSE_PROGRESS_TIMEOUT},
	{NULL, 0}
};

struct switch_channel {
	char *name;
	switch_call_direction_t direction;
	switch_queue_t *dtmf_queue;
	switch_queue_t *dtmf_log_queue;
	switch_mutex_t *dtmf_mutex;
	switch_mutex_t *flag_mutex;
	switch_mutex_t *state_mutex;
	switch_mutex_t *profile_mutex;
	switch_core_session_t *session;
	switch_channel_state_t state;
	switch_channel_state_t running_state;
	uint8_t flags[CF_FLAG_MAX];
	uint8_t state_flags[CF_FLAG_MAX];
	uint32_t private_flags;
	uint32_t app_flags;
	switch_caller_profile_t *caller_profile;
	const switch_state_handler_table_t *state_handlers[SWITCH_MAX_STATE_HANDLERS];
	int state_handler_index;
	switch_event_t *variables;
	switch_hash_t *private_hash;
	switch_call_cause_t hangup_cause;
	int vi;
	int event_count;
	int profile_index;
};

SWITCH_DECLARE(const char *) switch_channel_cause2str(switch_call_cause_t cause)
{
	uint8_t x;
	const char *str = "UNKNOWN";

	for (x = 0; x < (sizeof(CAUSE_CHART) / sizeof(struct switch_cause_table)) - 1; x++) {
		if (CAUSE_CHART[x].cause == cause) {
			str = CAUSE_CHART[x].name;
			break;
		}
	}

	return str;
}

SWITCH_DECLARE(switch_call_cause_t) switch_channel_str2cause(const char *str)
{
	uint8_t x;
	switch_call_cause_t cause = SWITCH_CAUSE_NONE;

	if (*str > 47 && *str < 58) {
		cause = atoi(str);
	} else {
		for (x = 0; x < (sizeof(CAUSE_CHART) / sizeof(struct switch_cause_table)) - 1 && CAUSE_CHART[x].name; x++) {
			if (!strcasecmp(CAUSE_CHART[x].name, str)) {
				cause = CAUSE_CHART[x].cause;
				break;
			}
		}
	}
	return cause;
}

SWITCH_DECLARE(switch_call_cause_t) switch_channel_get_cause(switch_channel_t *channel)
{
	return channel->hangup_cause;
}


SWITCH_DECLARE(void) switch_channel_audio_sync(switch_channel_t *channel) 
{
	if (switch_channel_media_ready(channel)) {
		switch_core_session_message_t msg;
		msg.message_id = SWITCH_MESSAGE_INDICATE_AUDIO_SYNC;
		msg.from = channel->name;
		switch_core_session_receive_message(channel->session, &msg);
	}
}



SWITCH_DECLARE(switch_call_cause_t) switch_channel_cause_q850(switch_call_cause_t cause)
{
	if (cause <= SWITCH_CAUSE_INTERWORKING) {
		return cause;
	} else {
		return SWITCH_CAUSE_NORMAL_CLEARING;
	}
}

SWITCH_DECLARE(switch_call_cause_t) switch_channel_get_cause_q850(switch_channel_t *channel)
{
	return switch_channel_cause_q850(channel->hangup_cause);
}

SWITCH_DECLARE(switch_channel_timetable_t *) switch_channel_get_timetable(switch_channel_t *channel)
{
	switch_channel_timetable_t *times = NULL;

	if (channel->caller_profile) {
		switch_mutex_lock(channel->profile_mutex);
		times = channel->caller_profile->times;
		switch_mutex_unlock(channel->profile_mutex);
	}

	return times;
}

SWITCH_DECLARE(switch_status_t) switch_channel_alloc(switch_channel_t **channel, switch_call_direction_t direction, switch_memory_pool_t *pool)
{
	switch_assert(pool != NULL);

	if (((*channel) = switch_core_alloc(pool, sizeof(switch_channel_t))) == 0) {
		return SWITCH_STATUS_MEMERR;
	}

	switch_event_create(&(*channel)->variables, SWITCH_EVENT_GENERAL);

	switch_core_hash_init(&(*channel)->private_hash, pool);
	switch_queue_create(&(*channel)->dtmf_queue, SWITCH_DTMF_LOG_LEN, pool);
	switch_queue_create(&(*channel)->dtmf_log_queue, SWITCH_DTMF_LOG_LEN, pool);

	switch_mutex_init(&(*channel)->dtmf_mutex, SWITCH_MUTEX_NESTED, pool);
	switch_mutex_init(&(*channel)->flag_mutex, SWITCH_MUTEX_NESTED, pool);
	switch_mutex_init(&(*channel)->state_mutex, SWITCH_MUTEX_NESTED, pool);
	switch_mutex_init(&(*channel)->profile_mutex, SWITCH_MUTEX_NESTED, pool);
	(*channel)->hangup_cause = SWITCH_CAUSE_NONE;
	(*channel)->name = "";
	(*channel)->direction = direction;

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_size_t) switch_channel_has_dtmf(switch_channel_t *channel)
{
	switch_size_t has;

	switch_mutex_lock(channel->dtmf_mutex);
	has = switch_queue_size(channel->dtmf_queue);
	switch_mutex_unlock(channel->dtmf_mutex);

	return has;
}

SWITCH_DECLARE(switch_status_t) switch_channel_queue_dtmf(switch_channel_t *channel, const switch_dtmf_t *dtmf)
{
	switch_status_t status;
	void *pop;
	switch_dtmf_t new_dtmf;

	switch_assert(dtmf);

	switch_mutex_lock(channel->dtmf_mutex);
	new_dtmf = *dtmf;

	if ((status = switch_core_session_recv_dtmf(channel->session, dtmf) != SWITCH_STATUS_SUCCESS)) {
		goto done;
	}

	if (is_dtmf(new_dtmf.digit)) {
		switch_dtmf_t *dt;
		int x = 0;

		if (new_dtmf.duration > switch_core_max_dtmf_duration(0)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s EXCESSIVE DTMF DIGIT [%c] LEN [%d]\n",
							  switch_channel_get_name(channel), new_dtmf.digit, new_dtmf.duration);
			new_dtmf.duration = switch_core_max_dtmf_duration(0);
		} else if (!new_dtmf.duration) {
			new_dtmf.duration = switch_core_default_dtmf_duration(0);
		}

		switch_zmalloc(dt, sizeof(*dt));
		*dt = new_dtmf;

		while (switch_queue_trypush(channel->dtmf_queue, dt) != SWITCH_STATUS_SUCCESS) {
			if (switch_queue_trypop(channel->dtmf_queue, &pop) == SWITCH_STATUS_SUCCESS) {
				free(pop);
			}
			if (++x > 100) {
				status = SWITCH_STATUS_FALSE;
				free(dt);
				goto done;
			}
		}
	}

	status = SWITCH_STATUS_SUCCESS;

  done:

	switch_mutex_unlock(channel->dtmf_mutex);

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_channel_queue_dtmf_string(switch_channel_t *channel, const char *dtmf_string)
{
	char *p;
	switch_dtmf_t dtmf = { 0, switch_core_default_dtmf_duration(0) };
	int sent = 0, dur;
	char *string;
	int i, argc;
	char *argv[256];

	if (switch_strlen_zero(dtmf_string)) {
		return SWITCH_STATUS_FALSE;
	}

	string = switch_core_session_strdup(channel->session, dtmf_string);
	argc = switch_separate_string(string, '+', argv, (sizeof(argv) / sizeof(argv[0])));

	for (i = 0; i < argc; i++) {
		dtmf.duration = switch_core_default_dtmf_duration(0);
		dur = switch_core_default_dtmf_duration(0) / 8;
		if ((p = strchr(argv[i], '@'))) {
			*p++ = '\0';
			if ((dur = atoi(p)) > 50) {
				dtmf.duration = dur * 8;
			}
		}

		if (dtmf.duration > switch_core_max_dtmf_duration(0)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "EXCESSIVE DTMF DIGIT LEN %c %d\n", dtmf.digit, dtmf.duration);
			dtmf.duration = switch_core_max_dtmf_duration(0);
		} else if (!dtmf.duration) {
			dtmf.duration = switch_core_default_dtmf_duration(0);
		}


		for (p = argv[i]; p && *p; p++) {
			if (is_dtmf(*p)) {
				dtmf.digit = *p;
				if (switch_channel_queue_dtmf(channel, &dtmf) == SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s Queue dtmf\ndigit=%c ms=%u samples=%u\n",
									  switch_channel_get_name(channel), dtmf.digit, dur, dtmf.duration);
					sent++;
				}
			}
		}

	}

	return sent ? SWITCH_STATUS_SUCCESS : SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(switch_status_t) switch_channel_dequeue_dtmf(switch_channel_t *channel, switch_dtmf_t *dtmf)
{
	switch_event_t *event;
	void *pop;
	switch_dtmf_t *dt;
	switch_status_t status = SWITCH_STATUS_FALSE;

	switch_mutex_lock(channel->dtmf_mutex);

	if (switch_queue_trypop(channel->dtmf_queue, &pop) == SWITCH_STATUS_SUCCESS) {
		dt = (switch_dtmf_t *) pop;
		*dtmf = *dt;

		if (switch_queue_trypush(channel->dtmf_log_queue, dt) != SWITCH_STATUS_SUCCESS) {
			free(dt);			
		}
		
		dt = NULL;

		if (dtmf->duration > switch_core_max_dtmf_duration(0)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s EXCESSIVE DTMF DIGIT [%c] LEN [%d]\n",
							  switch_channel_get_name(channel), dtmf->digit, dtmf->duration);
			dtmf->duration = switch_core_max_dtmf_duration(0);
		} else if (!dtmf->duration) {
			dtmf->duration = switch_core_default_dtmf_duration(0);
		}

		status = SWITCH_STATUS_SUCCESS;
	}
	switch_mutex_unlock(channel->dtmf_mutex);

	if (status == SWITCH_STATUS_SUCCESS && switch_event_create(&event, SWITCH_EVENT_DTMF) == SWITCH_STATUS_SUCCESS) {
		switch_channel_event_set_data(channel, event);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "DTMF-Digit", "%c", dtmf->digit);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "DTMF-Duration", "%u", dtmf->duration);
		if (switch_channel_test_flag(channel, CF_DIVERT_EVENTS)) {
			switch_core_session_queue_event(channel->session, &event);
		} else {
			switch_event_fire(&event);
		}
	}

	return status;
}

SWITCH_DECLARE(switch_size_t) switch_channel_dequeue_dtmf_string(switch_channel_t *channel, char *dtmf_str, switch_size_t len)
{
	switch_size_t x = 0;
	switch_dtmf_t dtmf = { 0 };

	memset(dtmf_str, 0, len);

	while (x < len - 1 && switch_channel_dequeue_dtmf(channel, &dtmf) == SWITCH_STATUS_SUCCESS) {
		dtmf_str[x++] = dtmf.digit;
	}

	return x;

}

SWITCH_DECLARE(void) switch_channel_flush_dtmf(switch_channel_t *channel)
{
	void *pop;

	switch_mutex_lock(channel->dtmf_mutex);
	while (switch_queue_trypop(channel->dtmf_queue, &pop) == SWITCH_STATUS_SUCCESS) {
		switch_dtmf_t *dt = (switch_dtmf_t *) pop;
		if (channel->state >= CS_HANGUP || switch_queue_trypush(channel->dtmf_log_queue, dt) != SWITCH_STATUS_SUCCESS) {
			free(dt);
		}
	}
	switch_mutex_unlock(channel->dtmf_mutex);
}

SWITCH_DECLARE(void) switch_channel_uninit(switch_channel_t *channel)
{
	void *pop;
	switch_channel_flush_dtmf(channel);
	while (switch_queue_trypop(channel->dtmf_log_queue, &pop) == SWITCH_STATUS_SUCCESS) {
		switch_safe_free(pop);
	}
	switch_core_hash_destroy(&channel->private_hash);
	switch_mutex_lock(channel->profile_mutex);
	switch_event_destroy(&channel->variables);
	switch_mutex_unlock(channel->profile_mutex);
}

SWITCH_DECLARE(switch_status_t) switch_channel_init(switch_channel_t *channel, switch_core_session_t *session, switch_channel_state_t state,
													switch_channel_flag_t flag)
{
	switch_assert(channel != NULL);
	channel->state = state;
	switch_channel_set_flag(channel, flag);
	channel->session = session;
	channel->running_state = CS_NONE;
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(void) switch_channel_presence(switch_channel_t *channel, const char *rpid, const char *status, const char *id)
{
	switch_event_t *event;
	switch_event_types_t type = SWITCH_EVENT_PRESENCE_IN;

	if (!status) {
		type = SWITCH_EVENT_PRESENCE_OUT;
	}

	if (!id) {
		id = switch_channel_get_variable(channel, "presence_id");
	}

	if (!id) {
		return;
	}

	if (switch_event_create(&event, type) == SWITCH_STATUS_SUCCESS) {
		switch_channel_event_set_data(channel, event);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", __FILE__);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "login", __FILE__);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "from", id);
		if (type == SWITCH_EVENT_PRESENCE_IN) {
			if (!rpid) {
				rpid = "unknown";
			}
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "rpid", rpid);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "status", status);
		}
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "event_type", "presence");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alt_event_type", "dialog");
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "event_count", "%d", channel->event_count++);
		switch_event_fire(&event);
	}
}

SWITCH_DECLARE(const char *) switch_channel_get_variable(switch_channel_t *channel, const char *varname)
{
	const char *v = NULL;
	switch_assert(channel != NULL);

	switch_mutex_lock(channel->profile_mutex);
	if (!channel->variables || !(v = switch_event_get_header(channel->variables, (char *) varname))) {
		switch_caller_profile_t *cp = channel->caller_profile;

		if (cp) {
			if (!strncmp(varname, "aleg_", 5)) {
				cp = cp->originator_caller_profile;
				varname += 5;
			} else if (!strncmp(varname, "bleg_", 5)) {
				cp = cp->originatee_caller_profile;
				varname += 5;
			}
		}

		if (!cp || !(v = switch_caller_get_field_by_name(cp, varname))) {
			v = switch_core_get_variable(varname);
		}
	}
	switch_mutex_unlock(channel->profile_mutex);

	return v;
}

SWITCH_DECLARE(const char *) switch_channel_get_variable_partner(switch_channel_t *channel, const char *varname)
{
	const char *uuid;
	const char *val = NULL;
	switch_assert(channel != NULL);
	
	if (!switch_strlen_zero(varname)) {
		if ((uuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE))) {
			switch_core_session_t *session;
			if ((session = switch_core_session_locate(uuid))) {
				switch_channel_t *tchannel = switch_core_session_get_channel(session);
				val = switch_channel_get_variable(tchannel, varname);
				switch_core_session_rwunlock(session);
			}
		}
	}

	return val;
}


SWITCH_DECLARE(void) switch_channel_variable_last(switch_channel_t *channel)
{
	switch_assert(channel != NULL);
	if (!channel->vi) {
		return;
	}
	channel->vi = 0;
	switch_mutex_unlock(channel->profile_mutex);

}

SWITCH_DECLARE(switch_event_header_t *) switch_channel_variable_first(switch_channel_t *channel)
{
	switch_event_header_t *hi = NULL;

	switch_assert(channel != NULL);
	switch_mutex_lock(channel->profile_mutex);
	if ((hi = channel->variables->headers)) {
		channel->vi = 1;
	} else {
		switch_mutex_unlock(channel->profile_mutex);
	}

	return hi;
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_private(switch_channel_t *channel, const char *key, const void *private_info)
{
	switch_assert(channel != NULL);
	switch_core_hash_insert_locked(channel->private_hash, key, private_info, channel->profile_mutex);
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(void *) switch_channel_get_private(switch_channel_t *channel, const char *key)
{
	void *val;
	switch_assert(channel != NULL);
	val = switch_core_hash_find_locked(channel->private_hash, key, channel->profile_mutex);
	return val;
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_name(switch_channel_t *channel, const char *name)
{
	const char *old = NULL;

	switch_assert(channel != NULL);
	if (!switch_strlen_zero(channel->name)) {
		old = channel->name;
	}
	channel->name = NULL;
	if (name) {
		char *uuid = switch_core_session_get_uuid(channel->session);
		channel->name = switch_core_session_strdup(channel->session, name);
		switch_channel_set_variable(channel, SWITCH_CHANNEL_NAME_VARIABLE, name);
		if (old) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Rename Channel %s->%s [%s]\n", old, name, uuid);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "New Channel %s [%s]\n", name, uuid);
		}
	}
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(char *) switch_channel_get_name(switch_channel_t *channel)
{
	switch_assert(channel != NULL);
	return (!switch_strlen_zero(channel->name)) ? channel->name : "N/A";
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_variable_var_check(switch_channel_t *channel, 
																	  const char *varname, const char *value, switch_bool_t var_check)
{
	switch_assert(channel != NULL);

	if (channel->variables && !switch_strlen_zero(varname)) {
		switch_mutex_lock(channel->profile_mutex);
		switch_event_del_header(channel->variables, varname);
		if (!switch_strlen_zero(value)) {
			int ok = 1;
			
			if (var_check) {
				ok = !switch_string_var_check_const(value);
			}
			if (ok) {
				switch_event_add_header_string(channel->variables, SWITCH_STACK_BOTTOM, varname, value);
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Invalid data (${%s} contains a variable)\n", varname);
			}
		}
		switch_mutex_unlock(channel->profile_mutex);
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

switch_status_t switch_event_base_add_header(switch_event_t *event, switch_stack_t stack, const char *header_name, char *data);

SWITCH_DECLARE(switch_status_t) switch_channel_set_variable_printf(switch_channel_t *channel, const char *varname,  const char *fmt, ...)
{
	int ret = 0;
	char *data;
	va_list ap;
	switch_assert(channel != NULL);

	if (channel->variables && !switch_strlen_zero(varname)) {
		switch_mutex_lock(channel->profile_mutex);
		switch_event_del_header(channel->variables, varname);

		va_start(ap, fmt);
		ret = switch_vasprintf(&data, fmt, ap);
		va_end(ap);

		if (ret == -1) {
			switch_mutex_unlock(channel->profile_mutex);
			return SWITCH_STATUS_MEMERR;
		}

		switch_event_base_add_header(channel->variables, SWITCH_STACK_BOTTOM, varname, data);

		switch_mutex_unlock(channel->profile_mutex);
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}


SWITCH_DECLARE(switch_status_t) switch_channel_set_variable_partner_var_check(switch_channel_t *channel, 
																			  const char *varname, const char *value, switch_bool_t var_check)
{
	const char *uuid;
	switch_assert(channel != NULL);

	if (!switch_strlen_zero(varname)) {
		if ((uuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE))) {
			switch_core_session_t *session;
			if ((session = switch_core_session_locate(uuid))) {
				switch_channel_t *tchannel = switch_core_session_get_channel(session);
				switch_channel_set_variable_var_check(tchannel, varname, value, var_check);
				switch_core_session_rwunlock(session);
			}
			return SWITCH_STATUS_SUCCESS;
		}
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(uint32_t) switch_channel_test_flag(switch_channel_t *channel, switch_channel_flag_t flag)
{
	switch_assert(channel != NULL);
	return channel->flags[flag] ? 1 : 0;
}

SWITCH_DECLARE(switch_bool_t) switch_channel_set_flag_partner(switch_channel_t *channel, switch_channel_flag_t flag)
{
	const char *uuid;

	switch_assert(channel != NULL);

	if ((uuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE))) {
		switch_core_session_t *session;
		if ((session = switch_core_session_locate(uuid))) {
			switch_channel_set_flag(switch_core_session_get_channel(session), flag);
			switch_core_session_rwunlock(session);
			return SWITCH_TRUE;
		}
	}

	return SWITCH_FALSE;
}

SWITCH_DECLARE(switch_bool_t) switch_channel_clear_flag_partner(switch_channel_t *channel, switch_channel_flag_t flag)
{
	const char *uuid;

	switch_assert(channel != NULL);

	if ((uuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE))) {
		switch_core_session_t *session;
		if ((session = switch_core_session_locate(uuid))) {
			switch_channel_clear_flag(switch_core_session_get_channel(session), flag);
			switch_core_session_rwunlock(session);
			return SWITCH_TRUE;
		}
	}

	return SWITCH_FALSE;
}

SWITCH_DECLARE(void) switch_channel_wait_for_state(switch_channel_t *channel, switch_channel_t *other_channel, switch_channel_state_t want_state)
{
	switch_channel_state_t state, mystate, ostate;
	ostate = switch_channel_get_state(channel);

	for (;;) {
		state = switch_channel_get_running_state(other_channel);
		mystate = switch_channel_get_running_state(channel);

		if (mystate != ostate || state >= CS_HANGUP || state == want_state) {
			break;
		}
		switch_cond_next();
	}
}

SWITCH_DECLARE(switch_status_t) switch_channel_wait_for_flag(switch_channel_t *channel, 
															 switch_channel_flag_t want_flag, 
															 switch_bool_t pres, 
															 uint32_t to,
															 switch_channel_t *super_channel)
{

	if (to) {
		to++;
	}

	for (;;) {
		if (pres) {
			if (switch_channel_test_flag(channel, want_flag)) {
				break;
			}
		} else {
			if (!switch_channel_test_flag(channel, want_flag)) {
				break;
			}
		}

		switch_cond_next();

		if (super_channel && !switch_channel_ready(super_channel)) {
			return SWITCH_STATUS_FALSE;
		}

		if (!switch_channel_ready(channel)) {
			return SWITCH_STATUS_FALSE;
		}

		if (to && !--to) {
			return SWITCH_STATUS_TIMEOUT;
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(void) switch_channel_set_flag(switch_channel_t *channel, switch_channel_flag_t flag)
{
	switch_assert(channel);
	switch_assert(channel->flag_mutex);

	switch_mutex_lock(channel->flag_mutex);
	channel->flags[flag] = 1;
	switch_mutex_unlock(channel->flag_mutex);

	if (flag == CF_OUTBOUND) {
		switch_channel_set_variable(channel, "is_outbound", "true");
	}
}


SWITCH_DECLARE(void) switch_channel_set_private_flag(switch_channel_t *channel, uint32_t flags)
{
	switch_assert(channel != NULL);
	switch_mutex_lock(channel->flag_mutex);
	channel->private_flags |= flags;
	switch_mutex_unlock(channel->flag_mutex);
}

SWITCH_DECLARE(void) switch_channel_clear_private_flag(switch_channel_t *channel, uint32_t flags)
{
	switch_assert(channel != NULL);
	switch_mutex_lock(channel->flag_mutex);
	channel->private_flags &= ~flags;
	switch_mutex_unlock(channel->flag_mutex);
}

SWITCH_DECLARE(int) switch_channel_test_private_flag(switch_channel_t *channel, uint32_t flags)
{
	switch_assert(channel != NULL);
	return (channel->private_flags & flags);
}

SWITCH_DECLARE(void) switch_channel_set_app_flag(switch_channel_t *channel, uint32_t flags)
{
	switch_assert(channel != NULL);
	switch_mutex_lock(channel->flag_mutex);
	channel->app_flags |= flags;
	switch_mutex_unlock(channel->flag_mutex);
}

SWITCH_DECLARE(void) switch_channel_clear_app_flag(switch_channel_t *channel, uint32_t flags)
{
	switch_assert(channel != NULL);
	switch_mutex_lock(channel->flag_mutex);
	if (!flags) {
		channel->app_flags = 0;
	} else {
		channel->app_flags &= ~flags;
	}
	switch_mutex_unlock(channel->flag_mutex);
}

SWITCH_DECLARE(int) switch_channel_test_app_flag(switch_channel_t *channel, uint32_t flags)
{
	switch_assert(channel != NULL);
	return (channel->app_flags & flags);
}

SWITCH_DECLARE(void) switch_channel_set_state_flag(switch_channel_t *channel, switch_channel_flag_t flag)
{
	switch_assert(channel != NULL);

	switch_mutex_lock(channel->flag_mutex);
	channel->state_flags[0] = 1;
	channel->state_flags[flag] = 1;
	switch_mutex_unlock(channel->flag_mutex);
}

SWITCH_DECLARE(void) switch_channel_clear_flag(switch_channel_t *channel, switch_channel_flag_t flag)
{
	switch_assert(channel != NULL);
	switch_assert(channel->flag_mutex);

	switch_mutex_lock(channel->flag_mutex);
	channel->flags[flag] = 0;
	switch_mutex_unlock(channel->flag_mutex);
	if (flag == CF_OUTBOUND) {
		switch_channel_set_variable(channel, "is_outbound", NULL);
	}
}

SWITCH_DECLARE(switch_channel_state_t) switch_channel_get_state(switch_channel_t *channel)
{
	switch_channel_state_t state;
	switch_assert(channel != NULL);

	switch_mutex_lock(channel->state_mutex);
	state = channel->state;
	switch_mutex_unlock(channel->state_mutex);

	return state;
}

SWITCH_DECLARE(switch_channel_state_t) switch_channel_get_running_state(switch_channel_t *channel)
{
	switch_channel_state_t state;
	switch_assert(channel != NULL);

	switch_mutex_lock(channel->state_mutex);
	state = channel->running_state;
	switch_mutex_unlock(channel->state_mutex);

	return state;
}

SWITCH_DECLARE(int) switch_channel_test_ready(switch_channel_t *channel, switch_bool_t media)
{
	int ret = 0;

	switch_assert(channel != NULL);

	if (media) {
		ret = ((switch_channel_test_flag(channel, CF_ANSWERED) || 
				switch_channel_test_flag(channel, CF_EARLY_MEDIA)) && !switch_channel_test_flag(channel, CF_PROXY_MODE) && 
			   switch_core_session_get_read_codec(channel->session) && switch_core_session_get_write_codec(channel->session));

		if (!ret) return ret;
	}
	
	ret = 0;

	if (!channel->hangup_cause && channel->state > CS_ROUTING && channel->state < CS_HANGUP && channel->state != CS_RESET &&
		!switch_channel_test_flag(channel, CF_TRANSFER)) {
		ret++;
	}

	return ret;
}

static const char *state_names[] = {
	"CS_NEW",
	"CS_INIT",
	"CS_ROUTING",
	"CS_SOFT_EXECUTE",
	"CS_EXECUTE",
	"CS_EXCHANGE_MEDIA",
	"CS_PARK",
	"CS_CONSUME_MEDIA",
	"CS_HIBERNATE",
	"CS_RESET",
	"CS_HANGUP",
	"CS_DONE",
	NULL
};

SWITCH_DECLARE(const char *) switch_channel_state_name(switch_channel_state_t state)
{
	return state_names[state];
}


SWITCH_DECLARE(switch_channel_state_t) switch_channel_name_state(const char *name)
{
	uint32_t x = 0;
	for (x = 0; state_names[x]; x++) {
		if (!strcasecmp(state_names[x], name)) {
			return (switch_channel_state_t) x;
		}
	}

	return CS_DONE;
}

SWITCH_DECLARE(switch_channel_state_t) switch_channel_perform_set_running_state(switch_channel_t *channel, switch_channel_state_t state,
																				const char *file, const char *func, int line)
{
	int x;
	switch_mutex_lock(channel->state_mutex);
	switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_DEBUG, "(%s) Running State Change %s\n", channel->name, state_names[state]);
	channel->running_state = state;

	if (channel->state_flags[0]) {
		for(x = 1; x < CF_FLAG_MAX ; x++ ) {
			if (channel->state_flags[x]) {
				channel->flags[x] = 1;
				channel->state_flags[x] = 0;
			}
		}
		channel->state_flags[0] = 0;
	}

	switch_channel_clear_flag(channel, CF_TAGGED);

	if (channel->state >= CS_ROUTING) {
		switch_channel_presence(channel, "unknown", (char *) state_names[state], NULL);
	}

	if (state < CS_HANGUP) {
		switch_event_t *event;
		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_STATE) == SWITCH_STATUS_SUCCESS) {
			if (state == CS_ROUTING) {
				switch_channel_event_set_data(channel, event);
			} else {
				char state_num[25];
				switch_snprintf(state_num, sizeof(state_num), "%d", state);
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-State", (char *) switch_channel_state_name(state));
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-State-Number", (char *) state_num);
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-Name", channel->name);
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Unique-ID", switch_core_session_get_uuid(channel->session));
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Call-Direction",
											   channel->direction == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound" : "inbound");
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Presence-Call-Direction",
											   channel->direction == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound" : "inbound");
				if (switch_channel_test_flag(channel, CF_ANSWERED)) {
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "answered");
				} else if (switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "early");
				} else {
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "ringing");
				}
			}
			switch_event_fire(&event);
		}
	}

	switch_mutex_unlock(channel->state_mutex);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_channel_state_t) switch_channel_perform_set_state(switch_channel_t *channel,
																		const char *file, const char *func, int line, switch_channel_state_t state)
{
	switch_channel_state_t last_state;
	int ok = 0;

	switch_assert(channel != NULL);
	switch_assert(state <= CS_DONE);
	switch_mutex_lock(channel->state_mutex);

	last_state = channel->state;
	switch_assert(last_state <= CS_DONE);

	if (last_state == state) {
		goto done;
	}

	if (last_state >= CS_HANGUP && state < last_state) {
		goto done;
	}

	/* STUB for more dev
	   case CS_INIT:
	   switch(state) {

	   case CS_NEW:
	   case CS_INIT:
	   case CS_EXCHANGE_MEDIA:
	   case CS_SOFT_EXECUTE:
	   case CS_ROUTING:
	   case CS_EXECUTE:
	   case CS_HANGUP:
	   case CS_DONE:

	   default:
	   break;
	   }
	   break;
	 */

	switch (last_state) {
	case CS_NEW:
	case CS_RESET:
		switch (state) {
		default:
			ok++;
			break;
		}
		break;

	case CS_INIT:
		switch (state) {
		case CS_EXCHANGE_MEDIA:
		case CS_SOFT_EXECUTE:
		case CS_ROUTING:
		case CS_EXECUTE:
		case CS_PARK:
		case CS_CONSUME_MEDIA:
		case CS_HIBERNATE:
		case CS_RESET:
			ok++;
		default:
			break;
		}
		break;

	case CS_EXCHANGE_MEDIA:
		switch (state) {
		case CS_SOFT_EXECUTE:
		case CS_ROUTING:
		case CS_EXECUTE:
		case CS_PARK:
		case CS_CONSUME_MEDIA:
		case CS_HIBERNATE:
		case CS_RESET:
			ok++;
		default:
			break;
		}
		break;

	case CS_SOFT_EXECUTE:
		switch (state) {
		case CS_EXCHANGE_MEDIA:
		case CS_ROUTING:
		case CS_EXECUTE:
		case CS_PARK:
		case CS_CONSUME_MEDIA:
		case CS_HIBERNATE:
		case CS_RESET:
			ok++;
		default:
			break;
		}
		break;

	case CS_PARK:
		switch (state) {
		case CS_EXCHANGE_MEDIA:
		case CS_ROUTING:
		case CS_EXECUTE:
		case CS_SOFT_EXECUTE:
		case CS_HIBERNATE:
		case CS_RESET:
		case CS_CONSUME_MEDIA:
			ok++;
		default:
			break;
		}
		break;

	case CS_CONSUME_MEDIA:
		switch (state) {
		case CS_EXCHANGE_MEDIA:
		case CS_ROUTING:
		case CS_EXECUTE:
		case CS_SOFT_EXECUTE:
		case CS_HIBERNATE:
		case CS_RESET:
		case CS_PARK:
			ok++;
		default:
			break;
		}
		break;
	case CS_HIBERNATE:
		switch (state) {
		case CS_EXCHANGE_MEDIA:
		case CS_INIT:
		case CS_ROUTING:
		case CS_EXECUTE:
		case CS_SOFT_EXECUTE:
		case CS_PARK:
		case CS_CONSUME_MEDIA:
		case CS_RESET:
			ok++;
		default:
			break;
		}
		break;

	case CS_ROUTING:
		switch (state) {
		case CS_EXCHANGE_MEDIA:
		case CS_EXECUTE:
		case CS_SOFT_EXECUTE:
		case CS_PARK:
		case CS_CONSUME_MEDIA:
		case CS_HIBERNATE:
		case CS_RESET:
			ok++;
		default:
			break;
		}
		break;

	case CS_EXECUTE:
		switch (state) {
		case CS_EXCHANGE_MEDIA:
		case CS_SOFT_EXECUTE:
		case CS_ROUTING:
		case CS_PARK:
		case CS_CONSUME_MEDIA:
		case CS_HIBERNATE:
		case CS_RESET:
			ok++;
		default:
			break;
		}
		break;

	case CS_HANGUP:
		switch (state) {
		case CS_DONE:
			ok++;
		default:
			break;
		}
		break;

	default:
		break;

	}

	if (ok) {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_DEBUG, "(%s) State Change %s -> %s\n",
						  channel->name, state_names[last_state], state_names[state]);
		switch_mutex_lock(channel->state_mutex);
		channel->state = state;
		switch_mutex_unlock(channel->state_mutex);

		if (state == CS_HANGUP && !channel->hangup_cause) {
			channel->hangup_cause = SWITCH_CAUSE_NORMAL_CLEARING;
		}

		if (state < CS_DONE) {
			switch_core_session_signal_state_change(channel->session);
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_WARNING,
						  "(%s) Invalid State Change %s -> %s\n", channel->name, state_names[last_state], state_names[state]);
		/* we won't tolerate an invalid state change so we can make sure we are as robust as a nice cup of dark coffee! */
		/* not cool lets crash this bad boy and figure out wtf is going on */
		switch_assert(channel->state >= CS_HANGUP);
	}
  done:

	switch_mutex_unlock(channel->state_mutex);
	return channel->state;
}

SWITCH_DECLARE(void) switch_channel_event_set_basic_data(switch_channel_t *channel, switch_event_t *event)
{
	switch_caller_profile_t *caller_profile, *originator_caller_profile = NULL, *originatee_caller_profile = NULL;
	switch_codec_t *codec;
	char state_num[25];

	switch_mutex_lock(channel->profile_mutex);

	if ((caller_profile = switch_channel_get_caller_profile(channel))) {
		originator_caller_profile = caller_profile->originator_caller_profile;
		originatee_caller_profile = caller_profile->originatee_caller_profile;
	}

	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-State", switch_channel_state_name(channel->state));
	switch_snprintf(state_num, sizeof(state_num), "%d", channel->state);
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-State-Number", state_num);
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-Name", switch_channel_get_name(channel));
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Unique-ID", switch_core_session_get_uuid(channel->session));
	
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Call-Direction",
								   channel->direction == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound" : "inbound");
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Presence-Call-Direction",
								   channel->direction == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound" : "inbound");


	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "answered");
	} else if (switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "early");
	} else {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "ringing");
	}

	if ((codec = switch_core_session_get_read_codec(channel->session)) && codec->implementation) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-Read-Codec-Name", switch_str_nil(codec->implementation->iananame));
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Read-Codec-Rate", "%u", codec->implementation->actual_samples_per_second);
	}

	if ((codec = switch_core_session_get_write_codec(channel->session)) && codec->implementation) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-Write-Codec-Name", switch_str_nil(codec->implementation->iananame));
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Write-Codec-Rate", "%u", codec->implementation->actual_samples_per_second);
	}

	/* Index Caller's Profile */
	if (caller_profile) {
		switch_caller_profile_event_set_data(caller_profile, "Caller", event);
	}
	
	if (originator_caller_profile && originatee_caller_profile) {
		/* Index Originator's Profile */
		switch_caller_profile_event_set_data(originator_caller_profile, "Originator", event);
		
		/* Index Originatee's Profile */
		switch_caller_profile_event_set_data(originatee_caller_profile, "Originatee", event);
	} else {
		/* Index Originator's Profile */
		if (originator_caller_profile) {
			switch_caller_profile_event_set_data(originator_caller_profile, "Other-Leg", event);
		} else if (originatee_caller_profile) {	/* Index Originatee's Profile */
			switch_caller_profile_event_set_data(originatee_caller_profile, "Other-Leg", event);
		}
	}

	switch_mutex_unlock(channel->profile_mutex);
}

SWITCH_DECLARE(void) switch_channel_event_set_extended_data(switch_channel_t *channel, switch_event_t *event)
{
	switch_event_header_t *hi;
	int x;

	switch_mutex_lock(channel->profile_mutex);
	
	if (switch_channel_test_flag(channel, CF_VERBOSE_EVENTS) || 
		event->event_id == SWITCH_EVENT_CHANNEL_ORIGINATE ||
		event->event_id == SWITCH_EVENT_CHANNEL_UUID ||
		event->event_id == SWITCH_EVENT_CHANNEL_ANSWER ||
		event->event_id == SWITCH_EVENT_CHANNEL_PARK ||
		event->event_id == SWITCH_EVENT_CHANNEL_UNPARK ||
		event->event_id == SWITCH_EVENT_CHANNEL_BRIDGE ||
		event->event_id == SWITCH_EVENT_CHANNEL_UNBRIDGE ||
		event->event_id == SWITCH_EVENT_CHANNEL_PROGRESS ||
		event->event_id == SWITCH_EVENT_CHANNEL_PROGRESS_MEDIA ||
		event->event_id == SWITCH_EVENT_CHANNEL_HANGUP ||
		event->event_id == SWITCH_EVENT_REQUEST_PARAMS ||
		event->event_id == SWITCH_EVENT_CHANNEL_DATA ||
		event->event_id == SWITCH_EVENT_CHANNEL_EXECUTE_COMPLETE ||
		event->event_id == SWITCH_EVENT_SESSION_HEARTBEAT ||
		event->event_id == SWITCH_EVENT_CUSTOM
		) {

		x = 0;
		/* Index Variables */
		if (channel->variables) {
			for (hi = channel->variables->headers; hi; hi = hi->next) {
				char buf[1024];
				char *vvar = NULL, *vval = NULL;
				
				vvar = (char *) hi->name;
				vval = (char *) hi->value;
				x++;

				switch_assert(vvar && vval);
				switch_snprintf(buf, sizeof(buf), "variable_%s", vvar);
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, buf, vval);
			}
		}
	}

	switch_mutex_unlock(channel->profile_mutex);
}


SWITCH_DECLARE(void) switch_channel_event_set_data(switch_channel_t *channel, switch_event_t *event)
{
	switch_mutex_lock(channel->profile_mutex);
	switch_channel_event_set_basic_data(channel, event);
	switch_channel_event_set_extended_data(channel, event);
	switch_mutex_unlock(channel->profile_mutex);
}



SWITCH_DECLARE(void) switch_channel_set_caller_profile(switch_channel_t *channel, switch_caller_profile_t *caller_profile)
{
	char *uuid = NULL;
	switch_assert(channel != NULL);
	switch_assert(channel->session != NULL);
	switch_mutex_lock(channel->profile_mutex);
	switch_assert(caller_profile != NULL);

	uuid = switch_core_session_get_uuid(channel->session);

	if (!caller_profile->uuid || strcasecmp(caller_profile->uuid, uuid)) {
		caller_profile->uuid = switch_core_session_strdup(channel->session, uuid);
	}

	if (!caller_profile->chan_name || strcasecmp(caller_profile->chan_name, channel->name)) {
		caller_profile->chan_name = switch_core_session_strdup(channel->session, channel->name);
	}

	if (!caller_profile->context) {
		caller_profile->context = switch_core_session_strdup(channel->session, "default");
	}

	if (!channel->caller_profile) {
		switch_event_t *event;

		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_CREATE) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(channel, event);
			switch_event_fire(&event);
		}
	}

	caller_profile->times = (switch_channel_timetable_t *) switch_core_session_alloc(channel->session, sizeof(*caller_profile->times));
	caller_profile->times->profile_created = switch_micro_time_now();

	if (channel->caller_profile && channel->caller_profile->times) {
		channel->caller_profile->times->transferred = caller_profile->times->profile_created;
		caller_profile->times->answered = channel->caller_profile->times->answered;
		caller_profile->times->progress = channel->caller_profile->times->progress;
		caller_profile->times->progress_media = channel->caller_profile->times->progress_media;
		caller_profile->times->created = channel->caller_profile->times->created;
		caller_profile->times->hungup = channel->caller_profile->times->hungup;
		if (channel->caller_profile->caller_extension) {
			switch_caller_extension_clone(&caller_profile->caller_extension, channel->caller_profile->caller_extension, caller_profile->pool);
		}
	} else {
		caller_profile->times->created = switch_micro_time_now();
	}

	caller_profile->next = channel->caller_profile;
	channel->caller_profile = caller_profile;
	caller_profile->profile_index = switch_core_sprintf(caller_profile->pool, "%d", ++channel->profile_index);

	switch_mutex_unlock(channel->profile_mutex);
}

SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_caller_profile(switch_channel_t *channel)
{
	switch_caller_profile_t *profile;
	switch_assert(channel != NULL);
	switch_mutex_lock(channel->profile_mutex);
	profile = channel->caller_profile;
	switch_mutex_unlock(channel->profile_mutex);
	return profile;
}

SWITCH_DECLARE(void) switch_channel_set_originator_caller_profile(switch_channel_t *channel, switch_caller_profile_t *caller_profile)
{
	switch_assert(channel != NULL);
	switch_assert(channel->caller_profile != NULL);
	switch_mutex_lock(channel->profile_mutex);

	if (!caller_profile->times) {
		caller_profile->times = (switch_channel_timetable_t *) switch_core_alloc(caller_profile->pool, sizeof(*caller_profile->times));
	}

	if (channel->caller_profile) {
		caller_profile->next = channel->caller_profile->originator_caller_profile;
		channel->caller_profile->originator_caller_profile = caller_profile;
	}
	switch_assert(channel->caller_profile->originator_caller_profile->next != channel->caller_profile->originator_caller_profile);
	switch_mutex_unlock(channel->profile_mutex);
}

SWITCH_DECLARE(void) switch_channel_set_originatee_caller_profile(switch_channel_t *channel, switch_caller_profile_t *caller_profile)
{
	switch_assert(channel != NULL);
	switch_assert(channel->caller_profile != NULL);

	switch_mutex_lock(channel->profile_mutex);
	if (channel->caller_profile) {
		caller_profile->next = channel->caller_profile->originatee_caller_profile;
		channel->caller_profile->originatee_caller_profile = caller_profile;
	}
	switch_assert(channel->caller_profile->originatee_caller_profile->next != channel->caller_profile->originatee_caller_profile);
	switch_mutex_unlock(channel->profile_mutex);
}

SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_originator_caller_profile(switch_channel_t *channel)
{
	switch_caller_profile_t *profile = NULL;
	switch_assert(channel != NULL);

	switch_mutex_lock(channel->profile_mutex);
	if (channel->caller_profile) {
		profile = channel->caller_profile->originator_caller_profile;
	}
	switch_mutex_unlock(channel->profile_mutex);

	return profile;
}

SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_originatee_caller_profile(switch_channel_t *channel)
{
	switch_caller_profile_t *profile = NULL;
	switch_assert(channel != NULL);

	switch_mutex_lock(channel->profile_mutex);
	if (channel->caller_profile) {
		profile = channel->caller_profile->originatee_caller_profile;
	}
	switch_mutex_unlock(channel->profile_mutex);

	return profile;
}

SWITCH_DECLARE(char *) switch_channel_get_uuid(switch_channel_t *channel)
{
	switch_assert(channel != NULL);
	switch_assert(channel->session != NULL);
	return switch_core_session_get_uuid(channel->session);
}

SWITCH_DECLARE(int) switch_channel_add_state_handler(switch_channel_t *channel, const switch_state_handler_table_t *state_handler)
{
	int x, index;

	switch_assert(channel != NULL);
	switch_mutex_lock(channel->state_mutex);
	for (x = 0; x < SWITCH_MAX_STATE_HANDLERS; x++) {
		if (channel->state_handlers[x] == state_handler) {
			index = x;
			goto end;
		}
	}
	index = channel->state_handler_index++;

	if (channel->state_handler_index >= SWITCH_MAX_STATE_HANDLERS) {
		index = -1;
		goto end;
	}

	channel->state_handlers[index] = state_handler;

  end:
	switch_mutex_unlock(channel->state_mutex);
	return index;
}

SWITCH_DECLARE(const switch_state_handler_table_t *) switch_channel_get_state_handler(switch_channel_t *channel, int index)
{
	const switch_state_handler_table_t *h = NULL;

	switch_assert(channel != NULL);

	if (index >= SWITCH_MAX_STATE_HANDLERS || index > channel->state_handler_index) {
		return NULL;
	}

	switch_mutex_lock(channel->state_mutex);
	h = channel->state_handlers[index];
	switch_mutex_unlock(channel->state_mutex);

	return h;
}

SWITCH_DECLARE(void) switch_channel_clear_state_handler(switch_channel_t *channel, const switch_state_handler_table_t *state_handler)
{
	int index, i = channel->state_handler_index;
	const switch_state_handler_table_t *new_handlers[SWITCH_MAX_STATE_HANDLERS] = { 0 };

	switch_assert(channel != NULL);

	switch_mutex_lock(channel->state_mutex);
	channel->state_handler_index = 0;

	if (state_handler) {
		for (index = 0; index < i; index++) {
			if (channel->state_handlers[index] != state_handler) {
				new_handlers[channel->state_handler_index++] = channel->state_handlers[index];
			}
		}
	}
	for (index = 0; index < SWITCH_MAX_STATE_HANDLERS; index++) {
		channel->state_handlers[index] = NULL;
	}

	if (state_handler) {
		for (index = 0; index < channel->state_handler_index; index++) {
			channel->state_handlers[index] = new_handlers[index];
		}
	}

	switch_mutex_unlock(channel->state_mutex);
}

SWITCH_DECLARE(void) switch_channel_set_caller_extension(switch_channel_t *channel, switch_caller_extension_t *caller_extension)
{
	switch_assert(channel != NULL);

	switch_mutex_lock(channel->profile_mutex);
	caller_extension->next = channel->caller_profile->caller_extension;
	channel->caller_profile->caller_extension = caller_extension;
	switch_mutex_unlock(channel->profile_mutex);
}


SWITCH_DECLARE(switch_caller_extension_t *) switch_channel_get_caller_extension(switch_channel_t *channel)
{
	switch_caller_extension_t *extension = NULL;

	switch_assert(channel != NULL);
	switch_mutex_lock(channel->profile_mutex);
	if (channel->caller_profile) {
		extension = channel->caller_profile->caller_extension;
	}
	switch_mutex_unlock(channel->profile_mutex);
	return extension;
}


SWITCH_DECLARE(switch_channel_state_t) switch_channel_perform_hangup(switch_channel_t *channel,
																	 const char *file, const char *func, int line, switch_call_cause_t hangup_cause)
{
	switch_assert(channel != NULL);
	switch_mutex_lock(channel->state_mutex);

	if (channel->caller_profile && channel->caller_profile->times && !channel->caller_profile->times->hungup) {
		switch_mutex_lock(channel->profile_mutex);
		channel->caller_profile->times->hungup = switch_micro_time_now();
		switch_mutex_unlock(channel->profile_mutex);
	}

	switch_channel_stop_broadcast(channel);

	if (channel->state < CS_HANGUP) {
		switch_event_t *event;
		switch_channel_state_t last_state = channel->state;

		channel->state = CS_HANGUP;
		channel->hangup_cause = hangup_cause;
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_NOTICE, "Hangup %s [%s] [%s]\n",
						  channel->name, state_names[last_state], switch_channel_cause2str(channel->hangup_cause));
		
		switch_channel_set_variable(channel, "hangup_cause", switch_channel_cause2str(channel->hangup_cause));
		switch_channel_presence(channel, "unavailable", switch_channel_cause2str(channel->hangup_cause), NULL);

		switch_core_session_kill_channel(channel->session, SWITCH_SIG_KILL);
		switch_core_session_signal_state_change(channel->session);

		switch_channel_set_timestamps(channel);

		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_HANGUP) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Hangup-Cause", switch_channel_cause2str(channel->hangup_cause));
			switch_channel_event_set_data(channel, event);
			switch_event_fire(&event);
		}

	}

	switch_mutex_unlock(channel->state_mutex);
	return channel->state;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_ring_ready(switch_channel_t *channel, const char *file, const char *func, int line)
{
	const char *var;
	char *app;
	switch_event_t *event;

	if (!switch_channel_test_flag(channel, CF_RING_READY)) {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_NOTICE, "Ring-Ready %s!\n", channel->name);
		switch_channel_set_flag(channel, CF_RING_READY);
		if (channel->caller_profile && channel->caller_profile->times) {
			switch_mutex_lock(channel->profile_mutex);
			channel->caller_profile->times->progress = switch_micro_time_now();
			if (channel->caller_profile->originator_caller_profile) {
				switch_core_session_t *other_session;
				if ((other_session = switch_core_session_locate(channel->caller_profile->originator_caller_profile->uuid))) {
					switch_channel_t *other_channel;
					other_channel = switch_core_session_get_channel(other_session);
					if (other_channel->caller_profile) {
						other_channel->caller_profile->times->progress = channel->caller_profile->times->progress;
					}
					switch_core_session_rwunlock(other_session);
				}
				channel->caller_profile->originator_caller_profile->times->progress = channel->caller_profile->times->progress;
			}
			switch_mutex_unlock(channel->profile_mutex);
		}

		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_PROGRESS) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(channel, event);
			switch_event_fire(&event);
		}

		var = switch_channel_get_variable(channel, SWITCH_CHANNEL_EXECUTE_ON_RING_VARIABLE);
		if (var) {
			char *arg = NULL;
			app = switch_core_session_strdup(channel->session, var);
			if ((arg = strchr(app, ' '))) {
				*arg++ = '\0';
			}
			switch_core_session_execute_application(channel->session, app, arg);
		}


		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_pre_answered(switch_channel_t *channel, const char *file, const char *func, int line)
{
	switch_event_t *event;

	if (!switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		const char *uuid;
		switch_core_session_t *other_session;

		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_NOTICE, "Pre-Answer %s!\n", channel->name);
		switch_channel_set_flag(channel, CF_EARLY_MEDIA);
		switch_channel_set_variable(channel, SWITCH_ENDPOINT_DISPOSITION_VARIABLE, "EARLY MEDIA");
		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_PROGRESS_MEDIA) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(channel, event);
			switch_event_fire(&event);
		}
		
		if (channel->caller_profile && channel->caller_profile->times) {
			switch_mutex_lock(channel->profile_mutex);
			channel->caller_profile->times->progress_media = switch_micro_time_now();
			if (channel->caller_profile->originator_caller_profile) {
				switch_core_session_t *osession;
				if ((osession = switch_core_session_locate(channel->caller_profile->originator_caller_profile->uuid))) {
					switch_channel_t *other_channel;
					other_channel = switch_core_session_get_channel(osession);
					if (other_channel->caller_profile) {
						other_channel->caller_profile->times->progress_media = channel->caller_profile->times->progress_media;
					}
					switch_core_session_rwunlock(osession);
				}
				channel->caller_profile->originator_caller_profile->times->progress_media = channel->caller_profile->times->progress_media;
			}
			switch_mutex_unlock(channel->profile_mutex);
		}

		/* if we're the child of another channel and the other channel is in a blocking read they will never realize we have answered so send 
		   a SWITCH_SIG_BREAK to interrupt any blocking reads on that channel
		 */
		if ((uuid = switch_channel_get_variable(channel, SWITCH_ORIGINATOR_VARIABLE))
			&& (other_session = switch_core_session_locate(uuid))) {
			switch_core_session_kill_channel(other_session, SWITCH_SIG_BREAK);
			switch_core_session_rwunlock(other_session);
		}
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_pre_answer(switch_channel_t *channel, const char *file, const char *func, int line)
{
	switch_core_session_message_t msg;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	switch_assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (!switch_channel_test_flag(channel, CF_OUTBOUND)) {
		msg.message_id = SWITCH_MESSAGE_INDICATE_PROGRESS;
		msg.from = channel->name;
		status = switch_core_session_perform_receive_message(channel->session, &msg, file, func, line);
	}

	if (status == SWITCH_STATUS_SUCCESS) {
		switch_channel_perform_mark_pre_answered(channel, file, func, line);
	} else {
		switch_channel_hangup(channel, SWITCH_CAUSE_INCOMPATIBLE_DESTINATION);
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_ring_ready(switch_channel_t *channel, const char *file, const char *func, int line)
{
	switch_core_session_message_t msg;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	switch_assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (!switch_channel_test_flag(channel, CF_OUTBOUND)) {
		msg.message_id = SWITCH_MESSAGE_INDICATE_RINGING;
		msg.from = channel->name;
		status = switch_core_session_perform_receive_message(channel->session, &msg, file, func, line);
	}
	
	if (status == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_NOTICE, "Ring Ready %s!\n", channel->name);
	} else {
		switch_channel_hangup(channel, SWITCH_CAUSE_INCOMPATIBLE_DESTINATION);
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_answered(switch_channel_t *channel, const char *file, const char *func, int line)
{
	switch_event_t *event;
	const char *uuid;
	switch_core_session_t *other_session;
	const char *var;
	char *app;

	switch_assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (channel->caller_profile && channel->caller_profile->times) {
		switch_mutex_lock(channel->profile_mutex);
		channel->caller_profile->times->answered = switch_micro_time_now();
		switch_mutex_unlock(channel->profile_mutex);
	}

	switch_channel_set_flag(channel, CF_ANSWERED);

	if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_ANSWER) == SWITCH_STATUS_SUCCESS) {
		switch_channel_event_set_data(channel, event);
		switch_event_fire(&event);
	}

	/* if we're the child of another channel and the other channel is in a blocking read they will never realize we have answered so send 
	   a SWITCH_SIG_BREAK to interrupt any blocking reads on that channel
	 */
	if ((uuid = switch_channel_get_variable(channel, SWITCH_ORIGINATOR_VARIABLE))
		&& (other_session = switch_core_session_locate(uuid))) {
		switch_core_session_kill_channel(other_session, SWITCH_SIG_BREAK);
		switch_core_session_rwunlock(other_session);
	}

	if ((var = switch_channel_get_variable(channel, SWITCH_ENABLE_HEARTBEAT_EVENTS_VARIABLE))) {
		uint32_t seconds = 60;
		int tmp;

		if (switch_is_number(var)) {
			tmp = atoi(var);
			if (tmp > 0) {
				seconds = tmp;
			}
		} else if (!switch_true(var)) {
			seconds = 0;
		}

		if (seconds) {
			switch_core_session_enable_heartbeat(channel->session, seconds);
		}
	}

	switch_channel_set_variable(channel, SWITCH_ENDPOINT_DISPOSITION_VARIABLE, "ANSWER");
	switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_NOTICE, "Channel [%s] has been answered\n", channel->name);
	if ((var = switch_channel_get_variable(channel, SWITCH_CHANNEL_EXECUTE_ON_ANSWER_VARIABLE)) && !switch_strlen_zero(var)) {
		char *arg = NULL;

		app = switch_core_session_strdup(channel->session, var);

		if ((arg = strchr(app, ' '))) {
			*arg++ = '\0';
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s execute on answer: %s(%s)", channel->name, app, switch_str_nil(arg));
		switch_core_session_execute_application(channel->session, app, arg);
	}

	switch_channel_audio_sync(channel);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_answer(switch_channel_t *channel, const char *file, const char *func, int line)
{
	switch_core_session_message_t msg;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	switch_assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}
	
	if (!switch_channel_test_flag(channel, CF_OUTBOUND)) {
		msg.message_id = SWITCH_MESSAGE_INDICATE_ANSWER;
		msg.from = channel->name;
		status = switch_core_session_perform_receive_message(channel->session, &msg, file, func, line);
	}

	if (status == SWITCH_STATUS_SUCCESS) {
		switch_channel_perform_mark_answered(channel, file, func, line);
	} else {
		switch_channel_hangup(channel, SWITCH_CAUSE_INCOMPATIBLE_DESTINATION);
	}

	return status;
}

#define resize(l) {\
	char *dp;\
	olen += (len + l + block);\
	cpos = c - data;\
	if ((dp = realloc(data, olen))) {\
	data = dp;\
	c = data + cpos;\
	memset(c, 0, olen - cpos);\
	}}                           \

SWITCH_DECLARE(char *) switch_channel_expand_variables(switch_channel_t *channel, const char *in)
{
	char *p, *c = NULL;
	char *data, *indup, *endof_indup;
	size_t sp = 0, len = 0, olen = 0, vtype = 0, br = 0, cpos, block = 128;
	char *cloned_sub_val = NULL, *sub_val = NULL;
	char *func_val = NULL;
	int nv = 0;

	if (switch_strlen_zero(in)) {
		return (char *) in;
	}

	nv = switch_string_var_check_const(in);

	if (!nv) {
		return (char *) in;
	}


	nv = 0;
	olen = strlen(in) + 1;
	indup = strdup(in);
	endof_indup = end_of_p(indup) + 1;

	if ((data = malloc(olen))) {
		memset(data, 0, olen);
		c = data;
		for (p = indup; p && p < endof_indup && *p; p++) {
			vtype = 0;

			if (*p == '\\') {
				if (*(p + 1) == '$') {
					nv = 1;
				} else if (*(p + 1) == '\\') {
					*c++ = *p++;
					len++;
					continue;
				}
				p++;
			}

			if (*p == '$' && !nv) {
				if (*(p + 1)) {
					if (*(p + 1) == '{') {
						vtype = 1;
					} else {
						nv = 1;
					}
				} else {
					nv = 1;
				}
			}

			if (nv) {
				*c++ = *p;
				len++;
				nv = 0;
				continue;
			}

			if (vtype) {
				char *s = p, *e, *vname, *vval = NULL;
				size_t nlen;

				s++;

				if (vtype == 1 && *s == '{') {
					br = 1;
					s++;
				}

				e = s;
				vname = s;
				while (*e) {
					if (br == 1 && *e == '}') {
						br = 0;
						*e++ = '\0';
						break;
					}

					if (br > 0) {
						if (e != s && *e == '{') {
							br++;
						} else if (br > 1 && *e == '}') {
							br--;
						}
					}

					e++;
				}
				p = e > endof_indup ? endof_indup : e;

				if ((vval = strchr(vname, '('))) {
					e = vval - 1;
					*vval++ = '\0';
					while (*e == ' ') {
						*e-- = '\0';
					}
					e = vval;
					br = 1;
					while (e && *e) {
						if (*e == '(') {
							br++;
						} else if (br > 1 && *e == ')') {
							br--;
						} else if (br == 1 && *e == ')') {
							*e = '\0';
							break;
						}
						e++;
					}

					vtype = 2;
				}

				if (vtype == 1) {
					char *expanded = NULL;
					int offset = 0;
					int ooffset = 0;
					char *ptr;

					if ((expanded = switch_channel_expand_variables(channel, (char *) vname)) == vname) {
						expanded = NULL;
					} else {
						vname = expanded;
					}
					if ((ptr = strchr(vname, ':'))) {
						*ptr++ = '\0';
						offset = atoi(ptr);
						if ((ptr = strchr(ptr, ':'))) {
							ptr++;
							ooffset = atoi(ptr);
						}
					}

					if ((sub_val = (char *) switch_channel_get_variable(channel, vname))) {
						if (offset || ooffset) {
							cloned_sub_val = strdup(sub_val);
							switch_assert(cloned_sub_val);
							sub_val = cloned_sub_val;
						}

						if (offset >= 0) {
							if ((size_t)offset > strlen(sub_val)) {
								*sub_val = '\0';
							} else {
								sub_val += offset;
							}
						} else if ((size_t) abs(offset) <= strlen(sub_val)) {
							sub_val = cloned_sub_val + (strlen(cloned_sub_val) + offset);
						}

						if (ooffset > 0 && (size_t) ooffset < strlen(sub_val)) {
							if ((ptr = (char *) sub_val + ooffset)) {
								*ptr = '\0';
							}
						}
					}

					switch_safe_free(expanded);
				} else {
					switch_stream_handle_t stream = { 0 };
					char *expanded = NULL;

					SWITCH_STANDARD_STREAM(stream);

					if (stream.data) {
						char *expanded_vname = NULL;

						if ((expanded_vname = switch_channel_expand_variables(channel, (char *) vname)) == vname) {
							expanded_vname = NULL;
						} else {
							vname = expanded_vname;
						}

						if ((expanded = switch_channel_expand_variables(channel, vval)) == vval) {
							expanded = NULL;
						} else {
							vval = expanded;
						}

						if (switch_api_execute(vname, vval, channel->session, &stream) == SWITCH_STATUS_SUCCESS) {
							func_val = stream.data;
							sub_val = func_val;
						} else {
							free(stream.data);
						}

						switch_safe_free(expanded);
						switch_safe_free(expanded_vname);

					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
						free(data);
						free(indup);
						return (char *) in;
					}
				}
				if ((nlen = sub_val ? strlen(sub_val) : 0)) {
					if (len + nlen >= olen) {
						resize(nlen);
					}

					len += nlen;
					strcat(c, sub_val);
					c += nlen;
				}

				switch_safe_free(func_val);
				switch_safe_free(cloned_sub_val);
				sub_val = NULL;
				vname = NULL;
				vtype = 0;
				br = 0;
			}
			if (len + 1 >= olen) {
				resize(1);
			}

			if (sp) {
				*c++ = ' ';
				sp = 0;
				len++;
			}

			if (*p == '$') {
				p--;
			} else {
				*c++ = *p;
				len++;
			}
		}
	}
	free(indup);

	return data;
}

SWITCH_DECLARE(char *) switch_channel_build_param_string(switch_channel_t *channel, switch_caller_profile_t *caller_profile, const char *prefix)
{
	switch_stream_handle_t stream = { 0 };
	switch_size_t encode_len = 1024, new_len = 0;
	char *encode_buf = NULL;
	const char *prof[12] = { 0 }, *prof_names[12] = {
	0};
	char *e = NULL;
	switch_event_header_t *hi;
	uint32_t x = 0;

	SWITCH_STANDARD_STREAM(stream);

	if (prefix) {
		stream.write_function(&stream, "%s&", prefix);
	}

	encode_buf = malloc(encode_len);
	switch_assert(encode_buf);

	if (!caller_profile) {
		caller_profile = switch_channel_get_caller_profile(channel);
	}

	switch_assert(caller_profile != NULL);

	prof[0] = caller_profile->context;
	prof[1] = caller_profile->destination_number;
	prof[2] = caller_profile->caller_id_name;
	prof[3] = caller_profile->caller_id_number;
	prof[4] = caller_profile->network_addr;
	prof[5] = caller_profile->ani;
	prof[6] = caller_profile->aniii;
	prof[7] = caller_profile->rdnis;
	prof[8] = caller_profile->source;
	prof[9] = caller_profile->chan_name;
	prof[10] = caller_profile->uuid;

	prof_names[0] = "context";
	prof_names[1] = "destination_number";
	prof_names[2] = "caller_id_name";
	prof_names[3] = "caller_id_number";
	prof_names[4] = "network_addr";
	prof_names[5] = "ani";
	prof_names[6] = "aniii";
	prof_names[7] = "rdnis";
	prof_names[8] = "source";
	prof_names[9] = "chan_name";
	prof_names[10] = "uuid";

	for (x = 0; prof[x]; x++) {
		if (switch_strlen_zero(prof[x])) {
			continue;
		}
		new_len = (strlen(prof[x]) * 3) + 1;
		if (encode_len < new_len) {
			char *tmp;

			encode_len = new_len;

			if (!(tmp = realloc(encode_buf, encode_len))) {
				abort();
			}

			encode_buf = tmp;
		}
		switch_url_encode(prof[x], encode_buf, encode_len);
		stream.write_function(&stream, "%s=%s&", prof_names[x], encode_buf);
	}

	if ((hi = switch_channel_variable_first(channel))) {
		for (; hi; hi = hi->next) {
			char *var = hi->name;
			char *val = hi->value;

			new_len = (strlen((char *) var) * 3) + 1;
			if (encode_len < new_len) {
				char *tmp;

				encode_len = new_len;

				tmp = realloc(encode_buf, encode_len);
				switch_assert(tmp);
				encode_buf = tmp;
			}

			switch_url_encode((char *) val, encode_buf, encode_len);
			stream.write_function(&stream, "%s=%s&", (char *) var, encode_buf);

		}
		switch_channel_variable_last(channel);
	}

	e = (char *) stream.data + (strlen((char *) stream.data) - 1);

	if (e && *e == '&') {
		*e = '\0';
	}

	switch_safe_free(encode_buf);

	return stream.data;
}

SWITCH_DECLARE(switch_status_t) switch_channel_get_variables(switch_channel_t *channel, switch_event_t **event)
{
	switch_status_t status;
	switch_mutex_lock(channel->profile_mutex);
	status = switch_event_dup(event, channel->variables);
	switch_mutex_unlock(channel->profile_mutex);
	return status;
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_timestamps(switch_channel_t *channel)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	const char *cid_buf = NULL;
	switch_caller_profile_t *caller_profile, *ocp;
	switch_app_log_t *app_log, *ap;
	char *last_app = NULL, *last_arg = NULL;
	char start[80] = "", answer[80] = "", progress[80] = "", progress_media[80] = "", end[80] = "", tmp[80] = "", profile_start[80] = "";
	int32_t duration = 0, legbillsec = 0, billsec = 0, mduration = 0, billmsec = 0, legbillmsec = 0, progressmsec = 0, progress_mediamsec = 0;
	switch_time_t uduration = 0, legbillusec = 0, billusec = 0, progresssec = 0, progressusec = 0, progress_mediasec = 0, progress_mediausec = 0;
	time_t tt_created = 0, tt_answered = 0, tt_progress = 0, tt_progress_media = 0, tt_hungup = 0, mtt_created = 0, mtt_answered = 0, mtt_hungup =
		0, tt_prof_created, mtt_prof_created, mtt_progress = 0, mtt_progress_media = 0;
	void *pop;
	char dtstr[SWITCH_DTMF_LOG_LEN+1] = "";
	int x = 0;

	if (!(caller_profile = switch_channel_get_caller_profile(channel)) || !channel->variables) {
		return SWITCH_STATUS_FALSE;
	}

	if ((app_log = switch_core_session_get_app_log(channel->session))) {
		for (ap = app_log; ap && ap->next; ap = ap->next);
		last_app = ap->app;
		last_arg = ap->arg;
	}

	if (!(ocp = switch_channel_get_originatee_caller_profile(channel))) {
		ocp = switch_channel_get_originator_caller_profile(channel);
	}

	if (!switch_strlen_zero(caller_profile->caller_id_name)) {
		cid_buf = switch_core_session_sprintf(channel->session, "\"%s\" <%s>", caller_profile->caller_id_name,
											  switch_str_nil(caller_profile->caller_id_number));
	} else {
		cid_buf = caller_profile->caller_id_number;
	}

	while (x < SWITCH_DTMF_LOG_LEN && switch_queue_trypop(channel->dtmf_log_queue, &pop) == SWITCH_STATUS_SUCCESS) {
		switch_dtmf_t *dt = (switch_dtmf_t *) pop;

		if (dt) {
			dtstr[x++] = dt->digit;
			free(dt);
			dt = NULL;
		}
	}
	
	if (x) {
		switch_channel_set_variable(channel, "digits_dialed", dtstr);
	} else {
		switch_channel_set_variable(channel, "digits_dialed", "none");
	}

	if (caller_profile->times) {
		switch_time_exp_t tm;
		switch_size_t retsize;
		const char *fmt = "%Y-%m-%d %T";

		switch_time_exp_lt(&tm, caller_profile->times->created);
		switch_strftime_nocheck(start, &retsize, sizeof(start), fmt, &tm);
		switch_channel_set_variable(channel, "start_stamp", start);

		switch_time_exp_lt(&tm, caller_profile->times->profile_created);
		switch_strftime_nocheck(profile_start, &retsize, sizeof(profile_start), fmt, &tm);
		switch_channel_set_variable(channel, "profile_start_stamp", profile_start);

		if (caller_profile->times->answered) {
			switch_time_exp_lt(&tm, caller_profile->times->answered);
			switch_strftime_nocheck(answer, &retsize, sizeof(answer), fmt, &tm);
			switch_channel_set_variable(channel, "answer_stamp", answer);
		}

		if (caller_profile->times->progress) {
			switch_time_exp_lt(&tm, caller_profile->times->progress);
			switch_strftime_nocheck(progress, &retsize, sizeof(progress), fmt, &tm);
			switch_channel_set_variable(channel, "progress_stamp", progress);
		}

		if (caller_profile->times->progress_media) {
			switch_time_exp_lt(&tm, caller_profile->times->progress_media);
			switch_strftime_nocheck(progress_media, &retsize, sizeof(progress_media), fmt, &tm);
			switch_channel_set_variable(channel, "progress_media_stamp", progress_media);
		}

		switch_time_exp_lt(&tm, caller_profile->times->hungup);
		switch_strftime_nocheck(end, &retsize, sizeof(end), fmt, &tm);
		switch_channel_set_variable(channel, "end_stamp", end);

		tt_created = (time_t) (caller_profile->times->created / 1000000);
		mtt_created = (time_t) (caller_profile->times->created / 1000);
		tt_prof_created = (time_t) (caller_profile->times->profile_created / 1000000);
		mtt_prof_created = (time_t) (caller_profile->times->profile_created / 1000);
		switch_snprintf(tmp, sizeof(tmp), "%" TIME_T_FMT, tt_created);
		switch_channel_set_variable(channel, "start_epoch", tmp);
		switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->created);
		switch_channel_set_variable(channel, "start_uepoch", tmp);

		switch_snprintf(tmp, sizeof(tmp), "%" TIME_T_FMT, tt_prof_created);
		switch_channel_set_variable(channel, "profile_start_epoch", tmp);
		switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->profile_created);
		switch_channel_set_variable(channel, "profile_start_uepoch", tmp);

		tt_answered = (time_t) (caller_profile->times->answered / 1000000);
		mtt_answered = (time_t) (caller_profile->times->answered / 1000);
		switch_snprintf(tmp, sizeof(tmp), "%" TIME_T_FMT, tt_answered);
		switch_channel_set_variable(channel, "answer_epoch", tmp);
		switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->answered);
		switch_channel_set_variable(channel, "answer_uepoch", tmp);

		tt_progress = (time_t) (caller_profile->times->progress / 1000000);
		mtt_progress = (time_t) (caller_profile->times->progress / 1000);
		switch_snprintf(tmp, sizeof(tmp), "%" TIME_T_FMT, tt_progress);
		switch_channel_set_variable(channel, "progress_epoch", tmp);
		switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->progress);
		switch_channel_set_variable(channel, "progress_uepoch", tmp);

		tt_progress_media = (time_t) (caller_profile->times->progress_media / 1000000);
		mtt_progress_media = (time_t) (caller_profile->times->progress_media / 1000);
		switch_snprintf(tmp, sizeof(tmp), "%" TIME_T_FMT, tt_progress_media);
		switch_channel_set_variable(channel, "progress_media_epoch", tmp);
		switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->progress_media);
		switch_channel_set_variable(channel, "progress_media_uepoch", tmp);


		tt_hungup = (time_t) (caller_profile->times->hungup / 1000000);
		mtt_hungup = (time_t) (caller_profile->times->hungup / 1000);
		switch_snprintf(tmp, sizeof(tmp), "%" TIME_T_FMT, tt_hungup);
		switch_channel_set_variable(channel, "end_epoch", tmp);
		switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->hungup);
		switch_channel_set_variable(channel, "end_uepoch", tmp);

		uduration = caller_profile->times->hungup - caller_profile->times->created;
		duration = (int32_t) (tt_hungup - tt_created);
		mduration = (int32_t) (mtt_hungup - mtt_created);

		if (caller_profile->times->answered) {
			billsec = (int32_t) (tt_hungup - tt_answered);
			billmsec = (int32_t) (mtt_hungup - mtt_answered);
			billusec = caller_profile->times->hungup - caller_profile->times->answered;

			legbillsec = (int32_t) (tt_hungup - tt_prof_created);
			legbillmsec = (int32_t) (mtt_hungup - mtt_prof_created);
			legbillusec = caller_profile->times->hungup - caller_profile->times->profile_created;
		}

		if (caller_profile->times->progress) {
			progresssec = (int32_t) (tt_progress - tt_created);
			progressmsec = (int32_t) (mtt_progress - mtt_created);
			progressusec = caller_profile->times->progress - caller_profile->times->created;
		}

		if (caller_profile->times->progress_media) {
			progress_mediasec = (int32_t) (tt_progress_media - tt_created);
			progress_mediamsec = (int32_t) (mtt_progress_media - mtt_created);
			progress_mediausec = caller_profile->times->progress_media - caller_profile->times->created;
		}


	}

	switch_channel_set_variable(channel, "last_app", last_app);
	switch_channel_set_variable(channel, "last_arg", last_arg);
	switch_channel_set_variable(channel, "caller_id", cid_buf);

	switch_snprintf(tmp, sizeof(tmp), "%d", duration);
	switch_channel_set_variable(channel, "duration", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%d", billsec);
	switch_channel_set_variable(channel, "billsec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, progresssec);
	switch_channel_set_variable(channel, "progresssec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, progress_mediasec);
	switch_channel_set_variable(channel, "progress_mediasec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%d", legbillsec);
	switch_channel_set_variable(channel, "flow_billsec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%d", mduration);
	switch_channel_set_variable(channel, "mduration", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%d", billmsec);
	switch_channel_set_variable(channel, "billmsec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%d", progressmsec);
	switch_channel_set_variable(channel, "progressmsec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%d" , progressmsec);
	switch_channel_set_variable(channel, "progress_mediamsec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%d", legbillmsec);
	switch_channel_set_variable(channel, "flow_billmsec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, uduration);
	switch_channel_set_variable(channel, "uduration", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, billusec);
	switch_channel_set_variable(channel, "billusec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, progressusec);
	switch_channel_set_variable(channel, "progressusec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, progress_mediausec);
	switch_channel_set_variable(channel, "progress_mediausec", tmp);

	switch_snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, legbillusec);
	switch_channel_set_variable(channel, "flow_billusec", tmp);

	return status;
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
