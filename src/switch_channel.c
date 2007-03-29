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
	{"UNALLOCATED", SWITCH_CAUSE_UNALLOCATED},
	{"SUCCESS", SWITCH_CAUSE_SUCCESS},
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
	{"ORIGINATOR_CANCEL", SWITCH_CAUSE_ORIGINATOR_CANCEL},
	{"CRASH", SWITCH_CAUSE_CRASH},
	{"SYSTEM_SHUTDOWN", SWITCH_CAUSE_SYSTEM_SHUTDOWN},
	{"LOSE_RACE", SWITCH_CAUSE_LOSE_RACE},
	{"MANAGER_REQUEST", SWITCH_CAUSE_MANAGER_REQUEST},
	{"BLIND_TRANSFER", SWITCH_CAUSE_BLIND_TRANSFER},
	{"ATTENDED_TRANSFER", SWITCH_CAUSE_ATTENDED_TRANSFER},
	{"ALLOTTED_TIMEOUT", SWITCH_CAUSE_ALLOTTED_TIMEOUT},
	{NULL, 0}
};

struct switch_channel {
	char *name;
	switch_buffer_t *dtmf_buffer;
	switch_mutex_t *dtmf_mutex;
	switch_mutex_t *flag_mutex;
	switch_mutex_t *profile_mutex;
	switch_core_session_t *session;
	switch_channel_state_t state;
	uint32_t flags;
	uint32_t state_flags;
	switch_caller_profile_t *caller_profile;
	const switch_state_handler_table_t *state_handlers[SWITCH_MAX_STATE_HANDLERS];
	int state_handler_index;
	switch_hash_t *variables;
	switch_hash_t *private_hash;
	switch_call_cause_t hangup_cause;
};


SWITCH_DECLARE(char *) switch_channel_cause2str(switch_call_cause_t cause)
{
	uint8_t x;
	char *str = "UNKNOWN";

	for (x = 0; CAUSE_CHART[x].name; x++) {
		if (CAUSE_CHART[x].cause == cause) {
			str = (char *) CAUSE_CHART[x].name;
		}
	}

	return str;
}

SWITCH_DECLARE(switch_call_cause_t) switch_channel_str2cause(char *str)
{
	uint8_t x;
	switch_call_cause_t cause = SWITCH_CAUSE_UNALLOCATED;

	for (x = 0; CAUSE_CHART[x].name; x++) {
		if (!strcasecmp(CAUSE_CHART[x].name, str)) {
			cause = CAUSE_CHART[x].cause;
		}
	}
	return cause;
}

SWITCH_DECLARE(switch_call_cause_t) switch_channel_get_cause(switch_channel_t *channel)
{
	assert(channel != NULL);
	return channel->hangup_cause;
}

SWITCH_DECLARE(switch_channel_timetable_t *) switch_channel_get_timetable(switch_channel_t *channel)
{
	switch_channel_timetable_t *times = NULL;

	assert(channel != NULL);
	if (channel->caller_profile) {
		switch_mutex_lock(channel->profile_mutex);
		times = channel->caller_profile->times;
		switch_mutex_unlock(channel->profile_mutex);
	}

	return times;
}

SWITCH_DECLARE(switch_status_t) switch_channel_alloc(switch_channel_t **channel, switch_memory_pool_t *pool)
{
	assert(pool != NULL);

	if (((*channel) = switch_core_alloc(pool, sizeof(switch_channel_t))) == 0) {
		return SWITCH_STATUS_MEMERR;
	}

	switch_core_hash_init(&(*channel)->variables, pool);
	switch_core_hash_init(&(*channel)->private_hash, pool);
	switch_buffer_create_dynamic(&(*channel)->dtmf_buffer, 128, 128, 0);

	switch_mutex_init(&(*channel)->dtmf_mutex, SWITCH_MUTEX_NESTED, pool);
	switch_mutex_init(&(*channel)->flag_mutex, SWITCH_MUTEX_NESTED, pool);
	switch_mutex_init(&(*channel)->profile_mutex, SWITCH_MUTEX_NESTED, pool);
	(*channel)->hangup_cause = SWITCH_CAUSE_UNALLOCATED;

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_size_t) switch_channel_has_dtmf(switch_channel_t *channel)
{
	switch_size_t has;

	assert(channel != NULL);
	switch_mutex_lock(channel->dtmf_mutex);
	has = switch_buffer_inuse(channel->dtmf_buffer);
	switch_mutex_unlock(channel->dtmf_mutex);

	return has;
}

SWITCH_DECLARE(switch_status_t) switch_channel_queue_dtmf(switch_channel_t *channel, char *dtmf)
{
	switch_status_t status;
	register switch_size_t len, inuse;
	switch_size_t wr = 0;
	char *p;

	assert(channel != NULL);

	switch_mutex_lock(channel->dtmf_mutex);

	inuse = switch_buffer_inuse(channel->dtmf_buffer);
	len = strlen(dtmf);

	if (len + inuse > switch_buffer_len(channel->dtmf_buffer)) {
		switch_buffer_toss(channel->dtmf_buffer, strlen(dtmf));
	}

	p = dtmf;
	while (wr < len && p) {
		if (is_dtmf(*p)) {
			wr++;
		} else {
			break;
		}
		p++;
	}
	status = switch_buffer_write(channel->dtmf_buffer, dtmf, wr) ? SWITCH_STATUS_SUCCESS : SWITCH_STATUS_MEMERR;
	switch_mutex_unlock(channel->dtmf_mutex);

	return status;
}


SWITCH_DECLARE(switch_size_t) switch_channel_dequeue_dtmf(switch_channel_t *channel, char *dtmf, switch_size_t len)
{
	switch_size_t bytes;
	switch_event_t *event;

	assert(channel != NULL);

	switch_mutex_lock(channel->dtmf_mutex);
	if ((bytes = switch_buffer_read(channel->dtmf_buffer, dtmf, len)) > 0) {
		*(dtmf + bytes) = '\0';
	}
	switch_mutex_unlock(channel->dtmf_mutex);

	if (bytes && switch_event_create(&event, SWITCH_EVENT_DTMF) == SWITCH_STATUS_SUCCESS) {
		switch_channel_event_set_data(channel, event);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "DTMF-String", "%s", dtmf);
		switch_event_fire(&event);
	}

	return bytes;

}

SWITCH_DECLARE(void) switch_channel_uninit(switch_channel_t *channel)
{
	switch_buffer_destroy(&channel->dtmf_buffer);
}

SWITCH_DECLARE(switch_status_t) switch_channel_init(switch_channel_t *channel,
													switch_core_session_t *session,
													switch_channel_state_t state, uint32_t flags)
{
	assert(channel != NULL);
	channel->state = state;
	channel->flags = flags;
	channel->session = session;
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(void) switch_channel_presence(switch_channel_t *channel, char *rpid, char *status)
{
	char *id = switch_channel_get_variable(channel, "presence_id");
	switch_event_t *event;
	switch_event_types_t type = SWITCH_EVENT_PRESENCE_IN;

	if (!status) {
		type = SWITCH_EVENT_PRESENCE_OUT;
	}

	if (!id) {
		return;
	}

	if (switch_event_create(&event, type) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "proto", "%s", __FILE__);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "login", "%s", __FILE__);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s", id);
		if (type == SWITCH_EVENT_PRESENCE_IN) {
			if (!rpid) {
				rpid = "unknown";
			}
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "rpid", "%s", rpid);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "status", "%s", status);
		}
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "event_type", "presence");
		switch_event_fire(&event);
	}

}


SWITCH_DECLARE(char *) switch_channel_get_variable(switch_channel_t *channel, char *varname)
{
	char *v = NULL;
	assert(channel != NULL);

	if (!(v = switch_core_hash_find(channel->variables, varname))) {
		if (!channel->caller_profile || !(v = switch_caller_get_field_by_name(channel->caller_profile, varname))) {
			if (!strcmp(varname, "base_dir")) {
				return SWITCH_GLOBAL_dirs.base_dir;
			}
			v = switch_core_get_variable(varname);
		}
	}

	return v;
}

SWITCH_DECLARE(switch_hash_index_t *) switch_channel_variable_first(switch_channel_t *channel,
																	switch_memory_pool_t *pool)
{
	assert(channel != NULL);
	return switch_hash_first(pool, channel->variables);
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_private(switch_channel_t *channel, char *key, void *private_info)
{
	assert(channel != NULL);
	switch_core_hash_insert_dup(channel->private_hash, switch_core_session_strdup(channel->session, key), private_info);
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(void *) switch_channel_get_private(switch_channel_t *channel, char *key)
{
	assert(channel != NULL);
	return switch_core_hash_find(channel->private_hash, key);
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_name(switch_channel_t *channel, char *name)
{
	assert(channel != NULL);
	channel->name = NULL;
	if (name) {
		char *uuid = switch_core_session_get_uuid(channel->session);
		channel->name = switch_core_session_strdup(channel->session, name);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "New Chan %s [%s]\n", name, uuid);
	}
	return SWITCH_STATUS_SUCCESS;
}


SWITCH_DECLARE(char *) switch_channel_get_name(switch_channel_t *channel)
{
	assert(channel != NULL);
	return channel->name ? channel->name : "N/A";
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_variable(switch_channel_t *channel, const char *varname,
															const char *value)
{
	assert(channel != NULL);

	if (varname) {
		switch_core_hash_delete(channel->variables, varname);
		if (!switch_strlen_zero(value)) {
			switch_core_hash_insert_dup(channel->variables, varname,
										switch_core_session_strdup(channel->session, value));
		} else {
			switch_core_hash_delete(channel->variables, varname);
		}
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(switch_status_t) switch_channel_set_variable_nodup(switch_channel_t *channel, const char *varname,
																  char *value)
{
	assert(channel != NULL);

	if (varname) {
		switch_core_hash_delete(channel->variables, varname);
		if (!switch_strlen_zero(value)) {
			switch_core_hash_insert_dup(channel->variables, varname, value);
		} else {
			switch_core_hash_delete(channel->variables, varname);
		}
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(int) switch_channel_test_flag(switch_channel_t *channel, switch_channel_flag_t flags)
{
	assert(channel != NULL);
	return switch_test_flag(channel, flags) ? 1 : 0;
}

SWITCH_DECLARE(void) switch_channel_set_flag(switch_channel_t *channel, switch_channel_flag_t flags)
{
	assert(channel != NULL);
	switch_set_flag_locked(channel, flags);
}

SWITCH_DECLARE(void) switch_channel_set_state_flag(switch_channel_t *channel, switch_channel_flag_t flags)
{
	assert(channel != NULL);

	switch_mutex_lock(channel->flag_mutex);
	channel->state_flags |= flags;
	switch_mutex_unlock(channel->flag_mutex);
}

SWITCH_DECLARE(void) switch_channel_clear_flag(switch_channel_t *channel, switch_channel_flag_t flags)
{
	assert(channel != NULL);
	switch_clear_flag_locked(channel, flags);
}

SWITCH_DECLARE(switch_channel_state_t) switch_channel_get_state(switch_channel_t *channel)
{
	switch_channel_state_t state;
	assert(channel != NULL);

	switch_mutex_lock(channel->flag_mutex);
	state = channel->state;
	switch_mutex_unlock(channel->flag_mutex);

	return state;
}

SWITCH_DECLARE(uint8_t) switch_channel_ready(switch_channel_t *channel)
{
	assert(channel != NULL);

	return (!channel->hangup_cause && channel->state > CS_RING && channel->state < CS_HANGUP
			&& !switch_test_flag(channel, CF_TRANSFER)) ? 1 : 0;
}

static const char *state_names[] = {
	"CS_NEW",
	"CS_INIT",
	"CS_RING",
	"CS_TRANSMIT",
	"CS_EXECUTE",
	"CS_LOOPBACK",
	"CS_HOLD",
	"CS_HIBERNATE",
	"CS_HANGUP",
	"CS_DONE",
	NULL
};

SWITCH_DECLARE(const char *) switch_channel_state_name(switch_channel_state_t state)
{
	return state_names[state];
}


SWITCH_DECLARE(switch_channel_state_t) switch_channel_name_state(char *name)
{
	uint32_t x = 0;
	for (x = 0; state_names[x]; x++) {
		if (!strcasecmp(state_names[x], name)) {
			return (switch_channel_state_t) x;
		}
	}

	return CS_DONE;
}

SWITCH_DECLARE(switch_channel_state_t) switch_channel_perform_set_state(switch_channel_t *channel,
																		const char *file,
																		const char *func,
																		int line, switch_channel_state_t state)
{
	switch_channel_state_t last_state;
	int ok = 0;


	assert(channel != NULL);
	switch_mutex_lock(channel->flag_mutex);

	last_state = channel->state;

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
	   case CS_LOOPBACK:
	   case CS_TRANSMIT:
	   case CS_RING:
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
		switch (state) {
		default:
			ok++;
			break;
		}
		break;

	case CS_INIT:
		switch (state) {
		case CS_LOOPBACK:
		case CS_TRANSMIT:
		case CS_RING:
		case CS_EXECUTE:
		case CS_HOLD:
		case CS_HIBERNATE:
			ok++;
		default:
			break;
		}
		break;

	case CS_LOOPBACK:
		switch (state) {
		case CS_TRANSMIT:
		case CS_RING:
		case CS_EXECUTE:
		case CS_HOLD:
		case CS_HIBERNATE:
			ok++;
		default:
			break;
		}
		break;

	case CS_TRANSMIT:
		switch (state) {
		case CS_LOOPBACK:
		case CS_RING:
		case CS_EXECUTE:
		case CS_HOLD:
		case CS_HIBERNATE:
			ok++;
		default:
			break;
		}
		break;

	case CS_HOLD:
		switch (state) {
		case CS_LOOPBACK:
		case CS_RING:
		case CS_EXECUTE:
		case CS_TRANSMIT:
		case CS_HIBERNATE:
			ok++;
		default:
			break;
		}
		break;
	case CS_HIBERNATE:
		switch (state) {
		case CS_LOOPBACK:
		case CS_RING:
		case CS_EXECUTE:
		case CS_TRANSMIT:
		case CS_HOLD:
			ok++;
		default:
			break;
		}
		break;

	case CS_RING:
		switch_clear_flag(channel, CF_TRANSFER);
		switch (state) {
		case CS_LOOPBACK:
		case CS_EXECUTE:
		case CS_TRANSMIT:
		case CS_HOLD:
		case CS_HIBERNATE:
			ok++;
		default:
			break;
		}
		break;

	case CS_EXECUTE:
		switch (state) {
		case CS_LOOPBACK:
		case CS_TRANSMIT:
		case CS_RING:
		case CS_HOLD:
		case CS_HIBERNATE:
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
		if (state > CS_RING) {
			switch_channel_presence(channel, "unknown", (char *) state_names[state]);
		}
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_DEBUG, "%s State Change %s -> %s\n",
						  channel->name, state_names[last_state], state_names[state]);
		switch_mutex_lock(channel->flag_mutex);
		channel->state = state;
		switch_mutex_unlock(channel->flag_mutex);

		if (state == CS_HANGUP && !channel->hangup_cause) {
			channel->hangup_cause = SWITCH_CAUSE_NORMAL_CLEARING;
		}
		if (state < CS_HANGUP) {
			switch_event_t *event;
			if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_STATE) == SWITCH_STATUS_SUCCESS) {
				if (state == CS_RING) {
					switch_channel_event_set_data(channel, event);
				} else {
					char state_num[25];
					snprintf(state_num, sizeof(state_num), "%d", channel->state);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-State", "%s",
											(char *) switch_channel_state_name(channel->state));
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-State-Number", "%s",
											(char *) state_num);
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Name", "%s",
											switch_channel_get_name(channel));
					switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Unique-ID", "%s",
											switch_core_session_get_uuid(channel->session));
				}
				switch_event_fire(&event);
			}
		}

		if (state < CS_DONE) {
			switch_core_session_signal_state_change(channel->session);
		}

	} else {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_WARNING,
						  "%s Invalid State Change %s -> %s\n", channel->name, state_names[last_state],
						  state_names[state]);

		/* we won't tolerate an invalid state change so we can make sure we are as robust as a nice cup of dark coffee! */
		if (channel->state < CS_HANGUP) {
			/* not cool lets crash this bad boy and figure out wtf is going on */
			assert(0);
		}
	}
  done:

	if (channel->state_flags) {
		channel->flags |= channel->state_flags;
		channel->state_flags = 0;
	}

	switch_mutex_unlock(channel->flag_mutex);
	return channel->state;
}

SWITCH_DECLARE(void) switch_channel_event_set_data(switch_channel_t *channel, switch_event_t *event)
{
	switch_caller_profile_t *caller_profile, *originator_caller_profile = NULL, *originatee_caller_profile = NULL;
	switch_hash_index_t *hi;
	switch_codec_t *codec;
	void *val;
	const void *var;
	char state_num[25];

	if ((caller_profile = switch_channel_get_caller_profile(channel))) {
		originator_caller_profile = caller_profile->originator_caller_profile;
		originatee_caller_profile = caller_profile->originatee_caller_profile;
	}

	switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-State", "%s",
							switch_channel_state_name(channel->state));
	snprintf(state_num, sizeof(state_num), "%d", channel->state);
	switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-State-Number", "%s", state_num);
	switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Name", "%s", switch_channel_get_name(channel));
	switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Unique-ID", "%s",
							switch_core_session_get_uuid(channel->session));

	if ((codec = switch_core_session_get_read_codec(channel->session))) {
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Read-Codec-Name", "%s",
								switch_str_nil(codec->implementation->iananame));
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Read-Codec-Rate", "%u",
								codec->implementation->samples_per_second);
	}

	if ((codec = switch_core_session_get_write_codec(channel->session))) {
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Write-Codec-Name", "%s",
								switch_str_nil(codec->implementation->iananame));
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Channel-Write-Codec-Rate", "%u",
								codec->implementation->samples_per_second);
	}

	/* Index Caller's Profile */
	if (caller_profile) {
		switch_caller_profile_event_set_data(caller_profile, "Caller", event);
	}

	/* Index Originator's Profile */
	if (originator_caller_profile) {
		switch_caller_profile_event_set_data(originator_caller_profile, "Originator", event);
	}

	/* Index Originatee's Profile */
	if (originatee_caller_profile) {
		switch_caller_profile_event_set_data(originatee_caller_profile, "Originatee", event);
	}

	/* Index Variables */
	for (hi = switch_hash_first(switch_core_session_get_pool(channel->session), channel->variables); hi;
		 hi = switch_hash_next(hi)) {
		char buf[1024];
		switch_hash_this(hi, &var, NULL, &val);
		if (var && val) {
			snprintf(buf, sizeof(buf), "variable_%s", (char *) var);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, buf, "%s", (char *) val);
		}
	}



}

SWITCH_DECLARE(void) switch_channel_set_caller_profile(switch_channel_t *channel,
													   switch_caller_profile_t *caller_profile)
{
	assert(channel != NULL);
	assert(channel->session != NULL);
	switch_mutex_lock(channel->profile_mutex);
	assert(caller_profile != NULL);

	if (!caller_profile->uuid) {
		caller_profile->uuid =
			switch_core_session_strdup(channel->session, switch_core_session_get_uuid(channel->session));
	}

	if (!caller_profile->chan_name) {
		caller_profile->chan_name = switch_core_session_strdup(channel->session, channel->name);
	}

	if (!caller_profile->context) {
		caller_profile->chan_name = switch_core_session_strdup(channel->session, "default");
	}

	if (!channel->caller_profile) {
		switch_event_t *event;

		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_CREATE) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(channel, event);
			switch_event_fire(&event);
		}
	}

	caller_profile->times =
		(switch_channel_timetable_t *) switch_core_session_alloc(channel->session, sizeof(*caller_profile->times));
	caller_profile->times->created = switch_time_now();

	if (channel->caller_profile && channel->caller_profile->times) {
		channel->caller_profile->times->transferred = switch_time_now();
		caller_profile->times->answered = channel->caller_profile->times->answered;
	}

	caller_profile->next = channel->caller_profile;
	channel->caller_profile = caller_profile;

	switch_mutex_unlock(channel->profile_mutex);
}

SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_caller_profile(switch_channel_t *channel)
{
	switch_caller_profile_t *profile;
	assert(channel != NULL);

	switch_mutex_lock(channel->profile_mutex);
	profile = channel->caller_profile;
	switch_mutex_unlock(channel->profile_mutex);

	return profile;
}

SWITCH_DECLARE(void) switch_channel_set_originator_caller_profile(switch_channel_t *channel,
																  switch_caller_profile_t *caller_profile)
{
	assert(channel != NULL);
	if (channel->caller_profile) {
		switch_mutex_lock(channel->profile_mutex);
		caller_profile->next = channel->caller_profile->originator_caller_profile;
		channel->caller_profile->originator_caller_profile = caller_profile;
		switch_mutex_unlock(channel->profile_mutex);
	}
}

SWITCH_DECLARE(void) switch_channel_set_originatee_caller_profile(switch_channel_t *channel,
																  switch_caller_profile_t *caller_profile)
{
	assert(channel != NULL);
	if (channel->caller_profile) {
		switch_mutex_lock(channel->profile_mutex);
		caller_profile->next = channel->caller_profile->originatee_caller_profile;
		channel->caller_profile->originatee_caller_profile = caller_profile;
		switch_mutex_unlock(channel->profile_mutex);
	}
}

SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_originator_caller_profile(switch_channel_t *channel)
{
	switch_caller_profile_t *profile = NULL;
	assert(channel != NULL);

	if (channel->caller_profile) {
		switch_mutex_lock(channel->profile_mutex);
		profile = channel->caller_profile->originator_caller_profile;
		switch_mutex_unlock(channel->profile_mutex);
	}

	return profile;
}

SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_originatee_caller_profile(switch_channel_t *channel)
{
	switch_caller_profile_t *profile = NULL;
	assert(channel != NULL);

	if (channel->caller_profile) {
		switch_mutex_lock(channel->profile_mutex);
		profile = channel->caller_profile->originatee_caller_profile;
		switch_mutex_unlock(channel->profile_mutex);
	}

	return profile;
}

SWITCH_DECLARE(char *) switch_channel_get_uuid(switch_channel_t *channel)
{
	assert(channel != NULL);
	assert(channel->session != NULL);
	return switch_core_session_get_uuid(channel->session);
}

SWITCH_DECLARE(int) switch_channel_add_state_handler(switch_channel_t *channel,
													 const switch_state_handler_table_t *state_handler)
{
	int x, index;

	assert(channel != NULL);
	switch_mutex_lock(channel->flag_mutex);
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
	switch_mutex_unlock(channel->flag_mutex);
	return index;
}

SWITCH_DECLARE(const switch_state_handler_table_t *) switch_channel_get_state_handler(switch_channel_t *channel,
																					  int index)
{
	const switch_state_handler_table_t *h = NULL;

	assert(channel != NULL);

	if (index > SWITCH_MAX_STATE_HANDLERS || index > channel->state_handler_index) {
		return NULL;
	}

	switch_mutex_lock(channel->flag_mutex);
	h = channel->state_handlers[index];
	switch_mutex_unlock(channel->flag_mutex);

	return h;
}

SWITCH_DECLARE(void) switch_channel_clear_state_handler(switch_channel_t *channel,
														const switch_state_handler_table_t *state_handler)
{
	int index, i = channel->state_handler_index;
	const switch_state_handler_table_t *new_handlers[SWITCH_MAX_STATE_HANDLERS] = { 0 };


	switch_mutex_lock(channel->flag_mutex);

	assert(channel != NULL);
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

	switch_mutex_unlock(channel->flag_mutex);

}

SWITCH_DECLARE(void) switch_channel_set_caller_extension(switch_channel_t *channel,
														 switch_caller_extension_t *caller_extension)
{
	assert(channel != NULL);

	switch_mutex_lock(channel->profile_mutex);
	caller_extension->next = channel->caller_profile->caller_extension;
	channel->caller_profile->caller_extension = caller_extension;
	switch_mutex_unlock(channel->profile_mutex);
}


SWITCH_DECLARE(switch_caller_extension_t *) switch_channel_get_caller_extension(switch_channel_t *channel)
{
	switch_caller_extension_t *extension = NULL;

	assert(channel != NULL);
	switch_mutex_lock(channel->profile_mutex);
	if (channel->caller_profile) {
		extension = channel->caller_profile->caller_extension;
	}
	switch_mutex_unlock(channel->profile_mutex);
	return extension;
}


SWITCH_DECLARE(switch_channel_state_t) switch_channel_perform_hangup(switch_channel_t *channel,
																	 const char *file,
																	 const char *func,
																	 int line, switch_call_cause_t hangup_cause)
{
	assert(channel != NULL);
	switch_mutex_lock(channel->flag_mutex);

	if (channel->caller_profile && channel->caller_profile->times && !channel->caller_profile->times->hungup) {
		switch_mutex_lock(channel->profile_mutex);
		channel->caller_profile->times->hungup = switch_time_now();
		switch_mutex_unlock(channel->profile_mutex);
	}

	if (channel->state < CS_HANGUP) {
		switch_event_t *event;
		switch_channel_state_t last_state = channel->state;
		channel->state = CS_HANGUP;
		channel->hangup_cause = hangup_cause;
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_NOTICE, "Hangup %s [%s] [%s]\n",
						  channel->name, state_names[last_state], switch_channel_cause2str(channel->hangup_cause));
		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_HANGUP) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Hangup-Cause", "%s",
									switch_channel_cause2str(channel->hangup_cause));
			switch_channel_event_set_data(channel, event);
			switch_event_fire(&event);
		}

		switch_channel_set_variable(channel, "hangup_cause", switch_channel_cause2str(channel->hangup_cause));
		switch_channel_presence(channel, "unavailable", switch_channel_cause2str(channel->hangup_cause));

		switch_core_session_kill_channel(channel->session, SWITCH_SIG_KILL);
		switch_core_session_signal_state_change(channel->session);
	}

	switch_mutex_unlock(channel->flag_mutex);
	return channel->state;
}


SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_ring_ready(switch_channel_t *channel,
																	   const char *file, const char *func, int line)
{
	if (!switch_channel_test_flag(channel, CF_RING_READY)) {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_NOTICE, "Ring-Ready %s!\n",
						  channel->name);
		switch_channel_set_flag(channel, CF_RING_READY);
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}



SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_pre_answered(switch_channel_t *channel,
																		 const char *file, const char *func, int line)
{
	switch_event_t *event;

	switch_channel_mark_ring_ready(channel);

	if (!switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		char *uuid;
		switch_core_session_t *other_session;

		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_NOTICE, "Pre-Answer %s!\n",
						  channel->name);
		switch_channel_set_flag(channel, CF_EARLY_MEDIA);
		switch_channel_set_variable(channel, "endpoint_disposition", "EARLY MEDIA");
		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_PROGRESS) == SWITCH_STATUS_SUCCESS) {
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
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_pre_answer(switch_channel_t *channel,
																  const char *file, const char *func, int line)
{
	switch_core_session_message_t msg;
	char *uuid = switch_core_session_get_uuid(channel->session);
	switch_status_t status;

	assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		return SWITCH_STATUS_SUCCESS;
	}

	msg.message_id = SWITCH_MESSAGE_INDICATE_PROGRESS;
	msg.from = channel->name;
	status = switch_core_session_message_send(uuid, &msg);

	if (status == SWITCH_STATUS_SUCCESS) {
		status = switch_channel_perform_mark_pre_answered(channel, file, func, line);
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_ring_ready(switch_channel_t *channel,
																  const char *file, const char *func, int line)
{
	switch_core_session_message_t msg;
	char *uuid = switch_core_session_get_uuid(channel->session);
	switch_status_t status;

	assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		return SWITCH_STATUS_SUCCESS;
	}

	msg.message_id = SWITCH_MESSAGE_INDICATE_RINGING;
	msg.from = channel->name;
	status = switch_core_session_message_send(uuid, &msg);

	if (status == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_NOTICE, "Ring Ready %s!\n",
						  channel->name);
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_answered(switch_channel_t *channel,
																	 const char *file, const char *func, int line)
{
	switch_event_t *event;
	char *uuid;
	switch_core_session_t *other_session;

	assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (channel->caller_profile && channel->caller_profile->times) {
		switch_mutex_lock(channel->profile_mutex);
		channel->caller_profile->times->answered = switch_time_now();
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

	switch_channel_set_variable(channel, "endpoint_disposition", "ANSWER");
	switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_NOTICE, "Channel [%s] has been answered\n",
					  channel->name);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_channel_perform_answer(switch_channel_t *channel,
															  const char *file, const char *func, int line)
{
	assert(channel != NULL);

	if (channel->hangup_cause || channel->state >= CS_HANGUP) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (switch_core_session_answer_channel(channel->session) == SWITCH_STATUS_SUCCESS) {
		return switch_channel_perform_mark_answered(channel, file, func, line);
	}

	return SWITCH_STATUS_FALSE;

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

SWITCH_DECLARE(char *) switch_channel_expand_variables(switch_channel_t *channel, char *in)
{
	char *p, *c;
	char *data, *indup;
	size_t sp = 0, len = 0, olen = 0, vtype = 0, br = 0, cpos, block = 128;
	char *sub_val = NULL, *func_val = NULL;

	if (!in || !strchr(in, '$')) {
		return in;
	}

	olen = strlen(in);
	indup = strdup(in);

	if ((data = malloc(olen))) {
		memset(data, 0, olen);
		c = data;
		for (p = indup; *p; p++) {
			vtype = 0;

			if (*p == '$') {
				vtype = 1;
				if (*(p + 1) != '{') {
					vtype = 2;
				}
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
					if (!br && *e == ' ') {
						*e++ = '\0';
						sp++;
						break;
					}
					if (br == 1 && *e == '}') {
						br = 0;
						*e++ = '\0';
						break;
					}
					if (vtype == 2) {
						if (*e == '(') {
							*e++ = '\0';
							vval = e;
							br = 2;
						}
						if (br == 2 && *e == ')') {
							*e++ = '\0';
							br = 0;
							break;
						}
					}
					e++;
				}
				p = e;

				if (vtype == 1) {
					sub_val = switch_channel_get_variable(channel, vname);
				} else {
					switch_stream_handle_t stream = { 0 };

					SWITCH_STANDARD_STREAM(stream);

					if (stream.data) {
						if (switch_api_execute(vname, vval, channel->session, &stream) == SWITCH_STATUS_SUCCESS) {
							func_val = stream.data;
							sub_val = func_val;
						} else {
							free(stream.data);
						}
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
						free(data);
						free(indup);
						return in;
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
				sub_val = NULL;
				vname = NULL;
				vtype = 0;
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
