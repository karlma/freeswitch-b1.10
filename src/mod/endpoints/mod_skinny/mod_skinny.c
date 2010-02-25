/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2010, Mathieu Parent <math.parent@gmail.com>
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
 * Mathieu Parent <math.parent@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Mathieu Parent <math.parent@gmail.com>
 *
 *
 * mod_skinny.c -- Skinny Call Control Protocol (SCCP) Endpoint Module
 *
 */
#include <switch.h>
#include "mod_skinny.h"
#include "skinny_protocol.h"

SWITCH_MODULE_LOAD_FUNCTION(mod_skinny_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_skinny_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_skinny_runtime);

SWITCH_MODULE_DEFINITION(mod_skinny, mod_skinny_load, mod_skinny_shutdown, mod_skinny_runtime);

switch_endpoint_interface_t *skinny_endpoint_interface;
static switch_memory_pool_t *module_pool = NULL;

skinny_globals_t globals;

/*****************************************************************************/
/* SQL TABLES */
/*****************************************************************************/
static char devices_sql[] =
	"CREATE TABLE skinny_devices (\n"
	"   name           VARCHAR(16),\n"
	"   user_id          INTEGER,\n"
	"   instance         INTEGER,\n"
	"   ip               VARCHAR(255),\n"
	"   type             INTEGER,\n"
	"   max_streams      INTEGER,\n"
	"   port             INTEGER,\n"
	"   codec_string     VARCHAR(255)\n"
	");\n";

static char buttons_sql[] =
	"CREATE TABLE skinny_buttons (\n"
	"   device_name      VARCHAR(16),\n"
	"   position         INTEGER,\n"
	"   type             INTEGER,\n"
	"   label            VARCHAR(40),\n"
	"   value            VARCHAR(24),\n"
	"   settings         VARCHAR(44)\n"
	");\n";

/*****************************************************************************/
/* PROFILES FUNCTIONS */
/*****************************************************************************/
static switch_status_t dump_profile(const skinny_profile_t *profile, switch_stream_handle_t *stream)
{
	const char *line = "=================================================================================================";
	switch_assert(profile);
	stream->write_function(stream, "%s\n", line);
	/* prefs */
	stream->write_function(stream, "Name             \t%s\n", profile->name);
	stream->write_function(stream, "Domain Name      \t%s\n", profile->domain);
	stream->write_function(stream, "IP               \t%s\n", profile->ip);
	stream->write_function(stream, "Port             \t%d\n", profile->port);
	stream->write_function(stream, "Dialplan         \t%s\n", profile->dialplan);
	stream->write_function(stream, "Context          \t%s\n", profile->context);
	stream->write_function(stream, "Keep-Alive       \t%d\n", profile->keep_alive);
	stream->write_function(stream, "Date-Format      \t%s\n", profile->date_format);
	stream->write_function(stream, "DBName           \t%s\n", profile->dbname ? profile->dbname : switch_str_nil(profile->odbc_dsn));
	stream->write_function(stream, "Debug            \t%d\n", profile->debug);
	/* stats */
	stream->write_function(stream, "CALLS-IN         \t%d\n", profile->ib_calls);
	stream->write_function(stream, "FAILED-CALLS-IN  \t%d\n", profile->ib_failed_calls);
	stream->write_function(stream, "CALLS-OUT        \t%d\n", profile->ob_calls);
	stream->write_function(stream, "FAILED-CALLS-OUT \t%d\n", profile->ob_failed_calls);
	/* listener */
	stream->write_function(stream, "Listener-Threads \t%d\n", profile->listener_threads);
	stream->write_function(stream, "%s\n", line);

	return SWITCH_STATUS_SUCCESS;
}


static skinny_profile_t *skinny_find_profile(const char *profile_name)
{
	return (skinny_profile_t *) switch_core_hash_find(globals.profile_hash, profile_name);
}

static switch_status_t skinny_profile_find_listener_by_device_name(skinny_profile_t *profile, const char *device_name, listener_t **listener)
{
	switch_mutex_lock(profile->listener_mutex);
	for (listener_t *l = profile->listeners; l; l = l->next) {
		if (!strcmp(l->device_name, device_name)) {
			*listener = l;
		}
	}
	switch_mutex_unlock(profile->listener_mutex);

	return SWITCH_STATUS_SUCCESS;
}

struct skinny_profile_find_listener_helper {
	skinny_profile_t *profile;
	listener_t *listener;
	uint32_t line;
};

static int skinny_profile_find_listener_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct skinny_profile_find_listener_helper *helper = pArg;
	skinny_profile_t *profile = helper->profile;
	char *device_name = argv[0];
	/* uint32_t position = atoi(argv[1]); */
	uint32_t relative_position = atoi(argv[2]);

	skinny_profile_find_listener_by_device_name(profile, device_name, &helper->listener);

	if(helper->listener) {
		helper->line = relative_position;
	}
	return 0;
}

static switch_status_t skinny_profile_find_listener_by_dest(skinny_profile_t *profile, const char *dest, listener_t **l, uint32_t *line)
{
	char *sql;
	struct skinny_profile_find_listener_helper helper = {0};
	helper.profile = profile;
	
	if ((sql = switch_mprintf("SELECT device_name, position, "
								"(SELECT count(*) from skinny_buttons sb2 "
									"WHERE sb2.device_name= sb1.device_name AND sb2.type='line' AND sb2.position <= sb1.position) AS relative_position "
								"FROM skinny_buttons sb1 WHERE type='line' and value='%s'",
								dest))) {
		skinny_execute_sql_callback(profile, profile->listener_mutex, sql, skinny_profile_find_listener_callback, &helper);
		switch_safe_free(sql);
	}


	*line = helper.line;
	*l = helper.listener;
	
	return SWITCH_STATUS_SUCCESS;
}

/*****************************************************************************/
/* SQL FUNCTIONS */
/*****************************************************************************/
void skinny_execute_sql(skinny_profile_t *profile, char *sql, switch_mutex_t *mutex)
{
	switch_core_db_t *db;

	if (mutex) {
		switch_mutex_lock(mutex);
	}

	if (switch_odbc_available() && profile->odbc_dsn) {
		switch_odbc_statement_handle_t stmt;
		if (switch_odbc_handle_exec(profile->master_odbc, sql, &stmt, NULL) != SWITCH_ODBC_SUCCESS) {
			char *err_str;
			err_str = switch_odbc_handle_get_error(profile->master_odbc, stmt);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, switch_str_nil(err_str));
			switch_safe_free(err_str);
		}
		switch_odbc_statement_handle_free(&stmt);
	} else {
		if (!(db = switch_core_db_open_file(profile->dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
			goto end;
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "SQL: %s\n", sql);
		switch_core_db_persistant_execute(db, sql, 1);
		switch_core_db_close(db);
	}

  end:
	if (mutex) {
		switch_mutex_unlock(mutex);
	}
}


switch_bool_t skinny_execute_sql_callback(skinny_profile_t *profile,
											  switch_mutex_t *mutex, char *sql, switch_core_db_callback_func_t callback, void *pdata)
{
	switch_bool_t ret = SWITCH_FALSE;
	switch_core_db_t *db;
	char *errmsg = NULL;

	if (mutex) {
		switch_mutex_lock(mutex);
	}

	if (switch_odbc_available() && profile->odbc_dsn) {
		switch_odbc_handle_callback_exec(profile->master_odbc, sql, callback, pdata, NULL);
	} else {
		if (!(db = switch_core_db_open_file(profile->dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
			goto end;
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "SQL: %s\n", sql);
		switch_core_db_exec(db, sql, callback, pdata, &errmsg);

		if (errmsg) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "SQL ERR: [%s] %s\n", sql, errmsg);
			free(errmsg);
		}

		if (db) {
			switch_core_db_close(db);
		}
	}

  end:

	if (mutex) {
		switch_mutex_unlock(mutex);
	}

	return ret;
}

/*****************************************************************************/
/* CHANNEL FUNCTIONS */
/*****************************************************************************/
uint32_t skinny_line_perform_set_state(listener_t *listener, const char *file, const char *func, int line, uint32_t instance, uint32_t state, uint32_t call_id)
{
	switch_assert(listener);
	
	switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, NULL, SWITCH_LOG_DEBUG,
		"Device %s, line %d State Change %s (%d) -> %s (%d) (no session)\n",
		listener->device_name, instance,
		skinny_soft_key_set2str(listener->line_state[instance]), listener->line_state[instance],
		skinny_soft_key_set2str(state), state);

	send_select_soft_keys(listener, instance, call_id, state, 0xffff);
	listener->line_state[instance] = state;
	
	return listener->line_state[instance];
}

uint32_t skinny_line_get_state(listener_t *listener, uint32_t instance)
{
	switch_assert(listener);

	return listener->line_state[instance];
}
switch_status_t skinny_tech_set_codec(private_t *tech_pvt, int force)
{
	int ms;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	int resetting = 0;

	if (!tech_pvt->iananame) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_ERROR, "No audio codec available\n");
		switch_goto_status(SWITCH_STATUS_FALSE, end);
	}

	if (switch_core_codec_ready(&tech_pvt->read_codec)) {
		if (!force) {
			switch_goto_status(SWITCH_STATUS_SUCCESS, end);
		}
		if (strcasecmp(tech_pvt->read_impl.iananame, tech_pvt->iananame) ||
			tech_pvt->read_impl.samples_per_second != tech_pvt->rm_rate ||
			tech_pvt->codec_ms != (uint32_t)tech_pvt->read_impl.microseconds_per_packet / 1000) {
			
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_DEBUG, "Changing Codec from %s@%dms to %s@%dms\n",
							  tech_pvt->read_impl.iananame, tech_pvt->read_impl.microseconds_per_packet / 1000, 
							  tech_pvt->rm_encoding, tech_pvt->codec_ms);
			
			switch_core_session_lock_codec_write(tech_pvt->session);
			switch_core_session_lock_codec_read(tech_pvt->session);
			resetting = 1;
			switch_core_codec_destroy(&tech_pvt->read_codec);
			switch_core_codec_destroy(&tech_pvt->write_codec);
		} else {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_DEBUG, "Already using %s\n", tech_pvt->read_impl.iananame);
			switch_goto_status(SWITCH_STATUS_SUCCESS, end);
		}
	}
	
	if (switch_core_codec_init(&tech_pvt->read_codec,
							   tech_pvt->iananame,
							   tech_pvt->rm_fmtp,
							   tech_pvt->rm_rate,
							   tech_pvt->codec_ms,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE | 0 /* TODO tech_pvt->profile->codec_flags */,
							   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_ERROR, "Can't load codec?\n");
		switch_goto_status(SWITCH_STATUS_FALSE, end);
	}

	if (switch_core_codec_init(&tech_pvt->write_codec,
							   tech_pvt->iananame,
							   tech_pvt->rm_fmtp,
							   tech_pvt->rm_rate,
							   tech_pvt->codec_ms,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE | 0 /* TODO tech_pvt->profile->codec_flags */,
							   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_ERROR, "Can't load codec?\n");
		switch_goto_status(SWITCH_STATUS_FALSE, end);
	}

	switch_assert(tech_pvt->read_codec.implementation);
	switch_assert(tech_pvt->write_codec.implementation);

	tech_pvt->read_impl = *tech_pvt->read_codec.implementation;
	tech_pvt->write_impl = *tech_pvt->write_codec.implementation;

	switch_core_session_set_read_impl(tech_pvt->session, tech_pvt->read_codec.implementation);
	switch_core_session_set_write_impl(tech_pvt->session, tech_pvt->write_codec.implementation);

	if (switch_rtp_ready(tech_pvt->rtp_session)) {
		switch_assert(tech_pvt->read_codec.implementation);

		if (switch_rtp_change_interval(tech_pvt->rtp_session, 
									   tech_pvt->read_impl.microseconds_per_packet,
									   tech_pvt->read_impl.samples_per_packet
									   ) != SWITCH_STATUS_SUCCESS) {
			switch_channel_t *channel = NULL;
			channel = switch_core_session_get_channel(tech_pvt->session);
			assert(channel != NULL);

			switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			switch_goto_status(SWITCH_STATUS_FALSE, end);				
		}
	}

	tech_pvt->read_frame.rate = tech_pvt->rm_rate;
	ms = tech_pvt->write_codec.implementation->microseconds_per_packet / 1000;

	if (!switch_core_codec_ready(&tech_pvt->read_codec)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_ERROR, "Can't load codec?\n");
		switch_goto_status(SWITCH_STATUS_FALSE, end);
	}

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_DEBUG, "Set Codec %s %s/%ld %d ms %d samples\n",
					  "" /* TODO switch_channel_get_name(tech_pvt->channel)*/, tech_pvt->iananame, tech_pvt->rm_rate, tech_pvt->codec_ms,
					  tech_pvt->read_impl.samples_per_packet);
	tech_pvt->read_frame.codec = &tech_pvt->read_codec;

	tech_pvt->write_codec.agreed_pt = tech_pvt->agreed_pt;
	tech_pvt->read_codec.agreed_pt = tech_pvt->agreed_pt;

	if (force != 2) {
		switch_core_session_set_read_codec(tech_pvt->session, &tech_pvt->read_codec);
		switch_core_session_set_write_codec(tech_pvt->session, &tech_pvt->write_codec);
	}

	/* TODO
	tech_pvt->fmtp_out = switch_core_session_strdup(tech_pvt->session, tech_pvt->write_codec.fmtp_out);
	*/

	/* TODO
	if (switch_rtp_ready(tech_pvt->rtp_session)) {
		switch_rtp_set_default_payload(tech_pvt->rtp_session, tech_pvt->pt);
	}
	*/

 end:
	if (resetting) {
		switch_core_session_unlock_codec_write(tech_pvt->session);
		switch_core_session_unlock_codec_read(tech_pvt->session);
	}

	return status;
}

void tech_init(private_t *tech_pvt, switch_core_session_t *session, listener_t *listener, uint32_t line)
{
	struct line_stat_res_message *button = NULL;

	switch_assert(tech_pvt);
	switch_assert(session);
	switch_assert(listener);
	
	tech_pvt->read_frame.data = tech_pvt->databuf;
	tech_pvt->read_frame.buflen = sizeof(tech_pvt->databuf);
	switch_mutex_init(&tech_pvt->mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
	switch_mutex_init(&tech_pvt->flag_mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
	tech_pvt->call_id = listener->profile->next_call_id++;
	tech_pvt->listener = listener;
	tech_pvt->line = line;
	switch_core_session_set_private(session, tech_pvt);
	tech_pvt->session = session;

	skinny_line_get(listener, line, &button);
	tech_pvt->line_name = switch_core_strdup(listener->pool, button->name);
	tech_pvt->line_shortname = switch_core_strdup(listener->pool, button->shortname);
	tech_pvt->line_displayname = switch_core_strdup(listener->pool, button->displayname);
}

/* 
   State methods they get called when the state changes to the specific state 
   returning SWITCH_STATUS_SUCCESS tells the core to execute the standard state method next
   so if you fully implement the state you can return SWITCH_STATUS_FALSE to skip it.
*/
switch_status_t channel_on_init(switch_core_session_t *session)
{
	switch_channel_t *channel;
	private_t *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
	switch_set_flag_locked(tech_pvt, TFLAG_IO);

	/* Move channel's state machine to ROUTING. This means the call is trying
	   to get from the initial start where the call because, to the point
	   where a destination has been identified. If the channel is simply
	   left in the initial state, nothing will happen. */
	switch_channel_set_state(channel, CS_ROUTING);
	switch_mutex_lock(globals.calls_mutex);
	globals.calls++;
	switch_mutex_unlock(globals.calls_mutex);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL INIT\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_on_routing(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL ROUTING\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_on_execute(switch_core_session_t *session)
{

	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL EXECUTE\n", switch_channel_get_name(channel));


	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_on_destroy(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);

	if (tech_pvt) {
		if (switch_core_codec_ready(&tech_pvt->read_codec)) {
			switch_core_codec_destroy(&tech_pvt->read_codec);
		}
		
		if (switch_core_codec_ready(&tech_pvt->write_codec)) {
			switch_core_codec_destroy(&tech_pvt->write_codec);
		}
	}

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL DESTROY\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}


switch_status_t channel_on_hangup(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	listener_t *listener = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	listener = tech_pvt->listener;
	assert(listener != NULL);
	
	switch_clear_flag_locked(tech_pvt, TFLAG_IO);
	switch_clear_flag_locked(tech_pvt, TFLAG_VOICE);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL HANGUP\n", switch_channel_get_name(channel));

	listener->session[tech_pvt->line] = NULL;

	stop_tone(listener, tech_pvt->line, tech_pvt->call_id);
	set_lamp(listener, SKINNY_BUTTON_LINE, tech_pvt->line, SKINNY_LAMP_OFF);
	clear_prompt_status(listener, tech_pvt->line, tech_pvt->call_id);

	if( skinny_line_get_state(tech_pvt->listener, tech_pvt->line) == SKINNY_KEY_SET_CONNECTED ) {
		close_receive_channel(listener,
			tech_pvt->call_id, /* uint32_t conference_id, */
			tech_pvt->party_id, /* uint32_t pass_thru_party_id, */
			tech_pvt->call_id /* uint32_t conference_id2, */
		);
		stop_media_transmission(listener,
			tech_pvt->call_id, /* uint32_t conference_id, */
			tech_pvt->party_id, /* uint32_t pass_thru_party_id, */
			tech_pvt->call_id /* uint32_t conference_id2, */
		);
		switch_mutex_lock(globals.calls_mutex);
		globals.calls--;
		if (globals.calls < 0) {
			globals.calls = 0;
		}
		switch_mutex_unlock(globals.calls_mutex);
	}
	send_call_state(listener,
		SKINNY_ON_HOOK,
		tech_pvt->line,
		tech_pvt->call_id);
	skinny_line_set_state(listener, tech_pvt->line, SKINNY_KEY_SET_ON_HOOK, tech_pvt->call_id);
	/* TODO: DefineTimeDate */
	set_speaker_mode(listener, SKINNY_SPEAKER_OFF);
	set_ringer(listener, SKINNY_RING_OFF, SKINNY_RING_FOREVER, 0);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_kill_channel(switch_core_session_t *session, int sig)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (sig) {
	case SWITCH_SIG_KILL:
		switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
		break;
	case SWITCH_SIG_BREAK:
		if (switch_rtp_ready(tech_pvt->rtp_session)) {
			switch_rtp_break(tech_pvt->rtp_session);
		}
		break;
	default:
		break;
	}
	
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s CHANNEL KILL %d\n", switch_channel_get_name(channel), sig);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_on_exchange_media(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "CHANNEL LOOPBACK\n");
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_on_soft_execute(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "CHANNEL TRANSMIT\n");
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_send_dtmf(switch_core_session_t *session, const switch_dtmf_t *dtmf)
{
	private_t *tech_pvt = switch_core_session_get_private(session);
	switch_assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "DTMF ON CALL %d [%c]\n", tech_pvt->call_id, dtmf->digit);

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	int payload = 0;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	while (!(tech_pvt->read_codec.implementation && switch_rtp_ready(tech_pvt->rtp_session))) {
		if (switch_channel_ready(channel)) {
			switch_yield(10000);
		} else {
			return SWITCH_STATUS_GENERR;
		}
	}

	tech_pvt->read_frame.datalen = 0;
	switch_set_flag_locked(tech_pvt, TFLAG_READING);

	if (switch_test_flag(tech_pvt, TFLAG_IO)) {
		switch_status_t status;

		switch_assert(tech_pvt->rtp_session != NULL);
		tech_pvt->read_frame.datalen = 0;


		while (switch_test_flag(tech_pvt, TFLAG_IO) && tech_pvt->read_frame.datalen == 0) {
			tech_pvt->read_frame.flags = SFF_NONE;

			status = switch_rtp_zerocopy_read_frame(tech_pvt->rtp_session, &tech_pvt->read_frame, flags);
			if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
				return SWITCH_STATUS_FALSE;
			}

			payload = tech_pvt->read_frame.payload;

			if (switch_rtp_has_dtmf(tech_pvt->rtp_session)) {
				switch_dtmf_t dtmf = { 0 };
				switch_rtp_dequeue_dtmf(tech_pvt->rtp_session, &dtmf);
				switch_channel_queue_dtmf(channel, &dtmf);
			}


			if (tech_pvt->read_frame.datalen > 0) {
				size_t bytes = 0;
				int frames = 1;

				if (!switch_test_flag((&tech_pvt->read_frame), SFF_CNG)) {
					if ((bytes = tech_pvt->read_codec.implementation->encoded_bytes_per_packet)) {
						frames = (tech_pvt->read_frame.datalen / bytes);
					}
					tech_pvt->read_frame.samples = (int) (frames * tech_pvt->read_codec.implementation->samples_per_packet);
				}
				break;
			}
		}
	}

	switch_clear_flag_locked(tech_pvt, TFLAG_READING);

	if (tech_pvt->read_frame.datalen == 0) {
		*frame = NULL;
		return SWITCH_STATUS_GENERR;
	}

	*frame = &tech_pvt->read_frame;

	return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	//switch_frame_t *pframe;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	if (!switch_test_flag(tech_pvt, TFLAG_IO)) {
		return SWITCH_STATUS_FALSE;
	}
#if SWITCH_BYTE_ORDER == __BIG_ENDIAN
	if (switch_test_flag(tech_pvt, TFLAG_LINEAR)) {
		switch_swap_linear(frame->data, (int) frame->datalen / 2);
	}
#endif

	switch_set_flag_locked(tech_pvt, TFLAG_WRITING);

	switch_rtp_write_frame(tech_pvt->rtp_session, frame);

	switch_clear_flag_locked(tech_pvt, TFLAG_WRITING);

	return status;

}

switch_status_t channel_answer_channel(switch_core_session_t *session)
{
	private_t *tech_pvt;
	switch_channel_t *channel = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);


	return SWITCH_STATUS_SUCCESS;
}


switch_status_t channel_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_channel_t *channel;
	private_t *tech_pvt;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = (private_t *) switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (msg->message_id) {
	case SWITCH_MESSAGE_INDICATE_ANSWER:
		{
			channel_answer_channel(session);
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

/* Make sure when you have 2 sessions in the same scope that you pass the appropriate one to the routines
   that allocate memory or you will have 1 channel with memory allocated from another channel's pool!
*/
switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool, switch_originate_flag_t flags, switch_call_cause_t *cancel_cause)
{
	switch_call_cause_t cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	switch_core_session_t *nsession = NULL;
	private_t *tech_pvt;
	
	char *profile_name, *dest;
	skinny_profile_t *profile = NULL;
	listener_t *listener = NULL;
	uint32_t line = 0;
	char name[128];
	switch_channel_t *channel;
	switch_caller_profile_t *caller_profile;

	if (!outbound_profile || zstr(outbound_profile->destination_number)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Invalid Destination\n");
		goto error;
	}

	if (!(nsession = switch_core_session_request(skinny_endpoint_interface, SWITCH_CALL_DIRECTION_OUTBOUND, pool))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Error Creating Session\n");
		goto error;
	}

	if (!(tech_pvt = (struct private_object *) switch_core_session_alloc(nsession, sizeof(*tech_pvt)))) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "Error Creating Session private object\n");
		goto error;
	}

	if(!(profile_name = switch_core_session_strdup(nsession, outbound_profile->destination_number))) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "Error Creating Session Info\n");
		goto error;
	}

	if (!(dest = strchr(profile_name, '/'))) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Invalid Skinny URL. Should be skinny/<profile>/<number>.\n");
		cause = SWITCH_CAUSE_INVALID_NUMBER_FORMAT;
		goto error;
	}
	*dest++ = '\0';

	profile = skinny_find_profile(profile_name);
	if (!(profile = skinny_find_profile(profile_name))) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Invalid Profile %s\n", profile_name);
		cause = SWITCH_CAUSE_UNALLOCATED_NUMBER;
		goto error;
	}
	
	snprintf(name, sizeof(name), "SKINNY/%s/%s", profile->name, dest);

	channel = switch_core_session_get_channel(nsession);
	switch_channel_set_name(channel, name);
	

	if ((skinny_profile_find_listener_by_dest(profile, dest, &listener, &line) != SWITCH_STATUS_SUCCESS)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Problem while retrieving listener and line for destination %s in profile %s\n", dest, profile_name);
		cause = SWITCH_CAUSE_UNALLOCATED_NUMBER;
		goto error;
	}
	
	if (!listener) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Invalid destination or phone not registred %s in profile %s\n", dest, profile_name);
		cause = SWITCH_CAUSE_UNALLOCATED_NUMBER;
		goto error;
	}

	if (line == 0) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Invalid destination or phone not registred %s in profile %s\n", dest, profile_name);
		cause = SWITCH_CAUSE_UNALLOCATED_NUMBER;
		goto error;
	}
	
	tech_init(tech_pvt, nsession, listener, line);

	caller_profile = switch_caller_profile_clone(nsession, outbound_profile);
	switch_channel_set_caller_profile(channel, caller_profile);
	tech_pvt->caller_profile = caller_profile;

	switch_channel_set_flag(channel, CF_OUTBOUND);
	switch_set_flag_locked(tech_pvt, TFLAG_OUTBOUND);

	if(tech_pvt->listener->session[tech_pvt->line]) { /* Line is busy */
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Device line is busy %s in profile %s\n", dest, profile_name);
		cause = SWITCH_CAUSE_USER_BUSY;
		goto error;
	}
	tech_pvt->listener->session[tech_pvt->line] = nsession;
	send_call_state(tech_pvt->listener, SKINNY_RING_IN, tech_pvt->line, tech_pvt->call_id);
	skinny_line_set_state(tech_pvt->listener, tech_pvt->line, SKINNY_KEY_SET_RING_IN, tech_pvt->call_id);
	display_prompt_status(tech_pvt->listener, 0, "\200\027tel", tech_pvt->line, tech_pvt->call_id);
	/* displayprinotifiymessage */
	skinny_send_call_info(nsession);
	set_lamp(tech_pvt->listener, SKINNY_BUTTON_LINE, tech_pvt->line, SKINNY_LAMP_BLINK);
	set_ringer(tech_pvt->listener, SKINNY_RING_OUTSIDE, SKINNY_RING_FOREVER, 0);

	*new_session = nsession;

	/* ?? switch_channel_mark_ring_ready(channel); */

	if (switch_channel_get_state(channel) == CS_NEW) {
		switch_channel_set_state(channel, CS_INIT);
	}

	cause = SWITCH_CAUSE_SUCCESS;
	goto done;

  error:
	if (nsession) {
		switch_core_session_destroy(&nsession);
	}
	*pool = NULL;


  done:

	if (profile) {
		if (cause == SWITCH_CAUSE_SUCCESS) {
			profile->ob_calls++;
		} else {
			profile->ob_failed_calls++;
		}
	}
	return cause;
}

switch_status_t channel_receive_event(switch_core_session_t *session, switch_event_t *event)
{
	struct private_object *tech_pvt = switch_core_session_get_private(session);
	char *body = switch_event_get_body(event);
	switch_assert(tech_pvt != NULL);

	if (!body) {
		body = "";
	}

	return SWITCH_STATUS_SUCCESS;
}



switch_state_handler_table_t skinny_state_handlers = {
	/*.on_init */ channel_on_init,
	/*.on_routing */ channel_on_routing,
	/*.on_execute */ channel_on_execute,
	/*.on_hangup */ channel_on_hangup,
	/*.on_exchange_media */ channel_on_exchange_media,
	/*.on_soft_execute */ channel_on_soft_execute,
	/*.on_consume_media*/ NULL,
	/*.on_hibernate*/ NULL,
	/*.on_reset*/ NULL,
	/*.on_park*/ NULL,
	/*.on_reporting*/ NULL,
	/*.on_destroy*/ channel_on_destroy

};

switch_io_routines_t skinny_io_routines = {
	/*.outgoing_channel */ channel_outgoing_channel,
	/*.read_frame */ channel_read_frame,
	/*.write_frame */ channel_write_frame,
	/*.kill_channel */ channel_kill_channel,
	/*.send_dtmf */ channel_send_dtmf,
	/*.receive_message */ channel_receive_message,
	/*.receive_event */ channel_receive_event
};

/*****************************************************************************/
/* LISTENER FUNCTIONS */
/*****************************************************************************/

static void add_listener(listener_t *listener)
{
	skinny_profile_t *profile;
	switch_assert(listener);
	assert(listener->profile);
	profile = listener->profile;

	switch_mutex_lock(profile->listener_mutex);
	listener->next = profile->listeners;
	profile->listeners = listener;
	switch_mutex_unlock(profile->listener_mutex);
}

static void remove_listener(listener_t *listener)
{
	listener_t *l, *last = NULL;
	skinny_profile_t *profile;
	switch_assert(listener);
	assert(listener->profile);
	profile = listener->profile;

	switch_mutex_lock(profile->listener_mutex);
	for (l = profile->listeners; l; l = l->next) {
		if (l == listener) {
			if (last) {
				last->next = l->next;
			} else {
				profile->listeners = l->next;
			}
		}
		last = l;
	}
	switch_mutex_unlock(profile->listener_mutex);
}


static void walk_listeners(skinny_listener_callback_func_t callback, void *pvt)
{
	switch_hash_index_t *hi;
	void *val;
	skinny_profile_t *profile;
	listener_t *l;
	
	/* walk listeners */
	for (hi = switch_hash_first(NULL, globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		profile = (skinny_profile_t *) val;

		switch_mutex_lock(profile->listener_mutex);
		for (l = profile->listeners; l; l = l->next) {
			callback(l, pvt);
		}
		switch_mutex_unlock(profile->listener_mutex);
	}
}

static void flush_listener(listener_t *listener, switch_bool_t flush_log, switch_bool_t flush_events)
{

	/* TODO */
}

static int dump_device_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	switch_stream_handle_t *stream = (switch_stream_handle_t *) pArg;

	char *device_name = argv[0];
	char *user_id = argv[1];
	char *instance = argv[2];
	char *ip = argv[3];
	char *type = argv[4];
	char *max_streams = argv[5];
	char *port = argv[6];
	char *codec_string = argv[7];

	const char *line = "=================================================================================================";
	stream->write_function(stream, "%s\n", line);
	stream->write_function(stream, "DeviceName    \t%s\n", switch_str_nil(device_name));
	stream->write_function(stream, "UserId        \t%s\n", user_id);
	stream->write_function(stream, "Instance      \t%s\n", instance);
	stream->write_function(stream, "IP            \t%s\n", ip);
	stream->write_function(stream, "DeviceType    \t%s\n", type);
	stream->write_function(stream, "MaxStreams    \t%s\n", max_streams);
	stream->write_function(stream, "Port          \t%s\n", port);
	stream->write_function(stream, "Codecs        \t%s\n", codec_string);
	stream->write_function(stream, "%s\n", line);

	return 0;
}

static switch_status_t dump_device(skinny_profile_t *profile, const char *device_name, switch_stream_handle_t *stream)
{
	char *sql;
	if ((sql = switch_mprintf("SELECT * FROM skinny_devices WHERE name LIKE '%s'",
			device_name))) {
		skinny_execute_sql_callback(profile, profile->listener_mutex, sql, dump_device_callback, stream);
		switch_safe_free(sql);
	}

	return SWITCH_STATUS_SUCCESS;
}


static void close_socket(switch_socket_t **sock, skinny_profile_t *profile)
{
	switch_mutex_lock(profile->sock_mutex);
	if (*sock) {
		switch_socket_shutdown(*sock, SWITCH_SHUTDOWN_READWRITE);
		switch_socket_close(*sock);
		*sock = NULL;
	}
	switch_mutex_unlock(profile->sock_mutex);
}

static switch_status_t kill_listener(listener_t *listener, void *pvt)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Killing listener.\n");
	switch_clear_flag(listener, LFLAG_RUNNING);
	close_socket(&listener->sock, listener->profile);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t kill_expired_listener(listener_t *listener, void *pvt)
{
	switch_event_t *event = NULL;

	if(listener->expire_time < switch_epoch_time_now(NULL)) {
		/* skinny::expire event */
		skinny_device_event(listener, &event, SWITCH_EVENT_CUSTOM, SKINNY_EVENT_EXPIRE);
		switch_event_fire(&event);
		return kill_listener(listener, pvt);
	}
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t keepalive_listener(listener_t *listener, void *pvt)
{
	skinny_profile_t *profile;
	switch_assert(listener);
	assert(listener->profile);
	profile = listener->profile;
	
	listener->expire_time = switch_epoch_time_now(NULL)+profile->keep_alive*110/100;

	return SWITCH_STATUS_SUCCESS;
}

static void *SWITCH_THREAD_FUNC listener_run(switch_thread_t *thread, void *obj)
{
	listener_t *listener = (listener_t *) obj;
	switch_status_t status;
	skinny_message_t *request = NULL;
	skinny_profile_t *profile;
	int destroy_pool = 1;

	switch_assert(listener);
	assert(listener->profile);
	profile = listener->profile;

	switch_mutex_lock(profile->listener_mutex);
	profile->listener_threads++;
	switch_mutex_unlock(profile->listener_mutex);
	
	switch_assert(listener != NULL);
	
	switch_socket_opt_set(listener->sock, SWITCH_SO_TCP_NODELAY, TRUE);
	switch_socket_opt_set(listener->sock, SWITCH_SO_NONBLOCK, TRUE);

	if (listener->profile->debug > 0) {
		if (zstr(listener->remote_ip)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connection Open\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connection Open from %s:%d\n", listener->remote_ip, listener->remote_port);
		}
	}

	switch_socket_opt_set(listener->sock, SWITCH_SO_NONBLOCK, TRUE);
	switch_set_flag_locked(listener, LFLAG_RUNNING);
	keepalive_listener(listener, NULL);
	add_listener(listener);


	while (globals.running && switch_test_flag(listener, LFLAG_RUNNING) && profile->listener_ready) {
		status = skinny_read_packet(listener, &request);

		if (status != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Socket Error!\n");
			switch_clear_flag_locked(listener, LFLAG_RUNNING);
			break;
		}

		if (!request) {
			continue;
		}

		if (skinny_handle_request(listener, request) != SWITCH_STATUS_SUCCESS) {
			switch_clear_flag_locked(listener, LFLAG_RUNNING);
			break;
		}

	}

	remove_listener(listener);

	if (listener->profile->debug > 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Session complete, waiting for children\n");
	}

	switch_thread_rwlock_wrlock(listener->rwlock);
	flush_listener(listener, SWITCH_TRUE, SWITCH_TRUE);

	if (listener->sock) {
		close_socket(&listener->sock, profile);
	}

	switch_thread_rwlock_unlock(listener->rwlock);

	if (listener->profile->debug > 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connection Closed\n");
	}

	for(int line = 0 ; line < SKINNY_MAX_BUTTON_COUNT ; line++) {
		if(listener->session[line]) {
			switch_channel_clear_flag(switch_core_session_get_channel(listener->session[line]), CF_CONTROLLED);
			//TODO switch_clear_flag_locked(listener, LFLAG_SESSION);
			switch_core_session_rwunlock(listener->session[line]);
			destroy_pool = 0;
		}
	}
	if(destroy_pool == 0) {
		goto no_destroy_pool;
	}
	if (listener->pool) {
		switch_memory_pool_t *pool = listener->pool;
		switch_core_destroy_memory_pool(&pool);
	}

no_destroy_pool:

	switch_mutex_lock(profile->listener_mutex);
	profile->listener_threads--;
	switch_mutex_unlock(profile->listener_mutex);

	return NULL;
}

/* Create a thread for the socket and launch it */
static void launch_listener_thread(listener_t *listener)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;

	switch_threadattr_create(&thd_attr, listener->pool);
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, listener_run, listener, listener->pool);
}

int skinny_socket_create_and_bind(skinny_profile_t *profile)
{
	switch_status_t rv;
	switch_sockaddr_t *sa;
	switch_socket_t *inbound_socket = NULL;
	listener_t *listener;
	switch_memory_pool_t *pool = NULL, *listener_pool = NULL;
	uint32_t errs = 0;

	if (switch_core_new_memory_pool(&pool) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "OH OH no pool\n");
		return SWITCH_STATUS_TERM;
	}

	while(globals.running) {
		rv = switch_sockaddr_info_get(&sa, profile->ip, SWITCH_INET, profile->port, 0, pool);
		if (rv)
			goto fail;
		rv = switch_socket_create(&profile->sock, switch_sockaddr_get_family(sa), SOCK_STREAM, SWITCH_PROTO_TCP, pool);
		if (rv)
			goto sock_fail;
		rv = switch_socket_opt_set(profile->sock, SWITCH_SO_REUSEADDR, 1);
		if (rv)
			goto sock_fail;
		rv = switch_socket_bind(profile->sock, sa);
		if (rv)
			goto sock_fail;
		rv = switch_socket_listen(profile->sock, 5);
		if (rv)
			goto sock_fail;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Socket up listening on %s:%u\n", profile->ip, profile->port);

		break;
	  sock_fail:
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error! Could not listen on %s:%u\n", profile->ip, profile->port);
		switch_yield(100000);
	}

	profile->listener_ready = 1;

	while(globals.running) {

		if (switch_core_new_memory_pool(&listener_pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "OH OH no pool\n");
			goto fail;
		}

		if ((rv = switch_socket_accept(&inbound_socket, profile->sock, listener_pool))) {
			if (!globals.running) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Shutting Down\n");
				goto end;
			} else {
				/* I wish we could use strerror_r here but its not defined everywhere =/ */
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error [%s]\n", strerror(errno));
				if (++errs > 100) {
					goto end;
				}
			}
		} else {
			errs = 0;
		}

		
		if (!(listener = switch_core_alloc(listener_pool, sizeof(*listener)))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Memory Error\n");
			break;
		}

		switch_thread_rwlock_create(&listener->rwlock, listener_pool);

		listener->sock = inbound_socket;
		listener->pool = listener_pool;
		listener_pool = NULL;
		strcpy(listener->device_name, "");
		listener->profile = profile;

		switch_mutex_init(&listener->flag_mutex, SWITCH_MUTEX_NESTED, listener->pool);

		switch_socket_addr_get(&listener->sa, SWITCH_TRUE, listener->sock);
		switch_get_addr(listener->remote_ip, sizeof(listener->remote_ip), listener->sa);
		listener->remote_port = switch_sockaddr_get_port(listener->sa);
		launch_listener_thread(listener);

	}

 end:

	close_socket(&profile->sock, profile);
	
	if (pool) {
		switch_core_destroy_memory_pool(&pool);
	}

	if (listener_pool) {
		switch_core_destroy_memory_pool(&listener_pool);
	}


  fail:
	return SWITCH_STATUS_TERM;
}

/*****************************************************************************/
/* MODULE FUNCTIONS */
/*****************************************************************************/
switch_endpoint_interface_t *skinny_get_endpoint_interface()
{
	return skinny_endpoint_interface;
}

static void skinny_profile_set(skinny_profile_t *profile, char *var, char *val)
{
	if (!var)
		return;

	if (!strcasecmp(var, "domain")) {
		profile->domain = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "ip")) {
		profile->ip = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "dialplan")) {
		profile->dialplan = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "context")) {
		profile->context = switch_core_strdup(module_pool, val);
	} else if (!strcasecmp(var, "date-format")) {
		strncpy(profile->date_format, val, 6);
	} else if (!strcasecmp(var, "odbc-dsn") && !zstr(val)) {
		if (switch_odbc_available()) {
			profile->odbc_dsn = switch_core_strdup(module_pool, val);
			if ((profile->odbc_user = strchr(profile->odbc_dsn, ':'))) {
				*profile->odbc_user++ = '\0';
				if ((profile->odbc_pass = strchr(profile->odbc_user, ':'))) {
					*profile->odbc_pass++ = '\0';
				}
			}
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ODBC IS NOT AVAILABLE!\n");
		}
	}
}

static switch_status_t load_skinny_config(void)
{
	char *cf = "skinny.conf";
	switch_xml_t xcfg, xml, xprofiles, xprofile;

	memset(&globals, 0, sizeof(globals));
	globals.running = 1;

	switch_core_hash_init(&globals.profile_hash, module_pool);

	switch_mutex_init(&globals.calls_mutex, SWITCH_MUTEX_NESTED, module_pool);
	
	if (!(xml = switch_xml_open_cfg(cf, &xcfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}

	if ((xprofiles = switch_xml_child(xcfg, "profiles"))) {
		for (xprofile = switch_xml_child(xprofiles, "profile"); xprofile; xprofile = xprofile->next) {
			char *profile_name = (char *) switch_xml_attr_soft(xprofile, "name");
			switch_xml_t xsettings = switch_xml_child(xprofile, "settings");
			if (zstr(profile_name)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
					"<profile> is missing name attribute\n");
				continue;
			}
			if (xsettings) {
				char dbname[256];
				switch_core_db_t *db;
				skinny_profile_t *profile = NULL;
				switch_xml_t param;
				
				profile = switch_core_alloc(module_pool, sizeof(skinny_profile_t));
				profile->name = profile_name;
				
				for (param = switch_xml_child(xsettings, "param"); param; param = param->next) {
					char *var = (char *) switch_xml_attr_soft(param, "name");
					char *val = (char *) switch_xml_attr_soft(param, "value");

					if (!strcmp(var, "domain")) {
						skinny_profile_set(profile, "domain", val);
					} else if (!strcmp(var, "ip")) {
						skinny_profile_set(profile, "ip", val);
					} else if (!strcmp(var, "port")) {
						profile->port = atoi(val);
					} else if (!strcmp(var, "dialplan")) {
						skinny_profile_set(profile, "dialplan", val);
					} else if (!strcmp(var, "context")) {
						skinny_profile_set(profile, "context", val);
					} else if (!strcmp(var, "keep-alive")) {
						profile->keep_alive = atoi(val);
					} else if (!strcmp(var, "date-format")) {
						skinny_profile_set(profile, "date-format", val);
					} else if (!strcmp(var, "odbc-dsn")) {
						skinny_profile_set(profile, "odbc-dsn", val);
					} else if (!strcmp(var, "debug")) {
						profile->debug = atoi(val);
					}
				} /* param */
				
				if (!profile->dialplan) {
					skinny_profile_set(profile, "dialplan","default");
				}

				if (!profile->context) {
					skinny_profile_set(profile, "context","public");
				}

				if (profile->port == 0) {
					profile->port = 2000;
				}

				switch_snprintf(dbname, sizeof(dbname), "skinny_%s", profile->name);
				profile->dbname = switch_core_strdup(module_pool, dbname);

				if (switch_odbc_available() && profile->odbc_dsn) {
					if (!(profile->master_odbc = switch_odbc_handle_new(profile->odbc_dsn, profile->odbc_user, profile->odbc_pass))) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
						continue;

					}
					if (switch_odbc_handle_connect(profile->master_odbc) != SWITCH_ODBC_SUCCESS) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
						continue;
					}

					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Connected ODBC DSN: %s\n", profile->odbc_dsn);
					switch_odbc_handle_exec(profile->master_odbc, devices_sql, NULL, NULL);
					switch_odbc_handle_exec(profile->master_odbc, buttons_sql, NULL, NULL);
				} else {
					if ((db = switch_core_db_open_file(profile->dbname))) {
						switch_core_db_test_reactive(db, "SELECT * FROM skinny_devices", NULL, devices_sql);
						switch_core_db_test_reactive(db, "SELECT * FROM skinny_buttons", NULL, buttons_sql);
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open SQL Database!\n");
						continue;
					}
					switch_core_db_close(db);
				}
				
				skinny_execute_sql_callback(profile, profile->listener_mutex, "DELETE FROM skinny_devices", NULL, NULL);
				skinny_execute_sql_callback(profile, profile->listener_mutex, "DELETE FROM skinny_buttons", NULL, NULL);

				switch_core_hash_insert(globals.profile_hash, profile->name, profile);
				profile = NULL;
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
					"Settings are missing from profile %s.\n", profile_name);
			} /* settings */
		} /* profile */
	}
	switch_xml_free(xml);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_status_profile(const char *profile_name, switch_stream_handle_t *stream)
{
	skinny_profile_t *profile;
	if ((profile = skinny_find_profile(profile_name))) {
		dump_profile(profile, stream);
	} else {
		stream->write_function(stream, "Profile not found!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_status_profile_device(const char *profile_name, const char *device_name, switch_stream_handle_t *stream)
{
	skinny_profile_t *profile;
	if ((profile = skinny_find_profile(profile_name))) {
		dump_device(profile, device_name, stream);
	} else {
		stream->write_function(stream, "Profile not found!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_profile_device_send_ringer_message(const char *profile_name, const char *device_name, const char *ring_type, const char *ring_mode, switch_stream_handle_t *stream)
{
	skinny_profile_t *profile;

	if ((profile = skinny_find_profile(profile_name))) {
		listener_t *listener = NULL;
		skinny_profile_find_listener_by_device_name(profile, device_name, &listener);
		if(listener) {
			set_ringer(listener, skinny_str2ring_type(ring_type), skinny_str2ring_mode(ring_mode), 0);
		} else {
			stream->write_function(stream, "Listener not found!\n");
		}
	} else {
		stream->write_function(stream, "Profile not found!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_profile_device_send_lamp_message(const char *profile_name, const char *device_name, const char *stimulus, const char *instance, const char *lamp_mode, switch_stream_handle_t *stream)
{
	skinny_profile_t *profile;

	if ((profile = skinny_find_profile(profile_name))) {
		listener_t *listener = NULL;
		skinny_profile_find_listener_by_device_name(profile, device_name, &listener);
		if(listener) {
			set_lamp(listener, skinny_str2button(stimulus), atoi(instance), skinny_str2lamp_mode(lamp_mode));
		} else {
			stream->write_function(stream, "Listener not found!\n");
		}
	} else {
		stream->write_function(stream, "Profile not found!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_profile_device_send_speaker_mode_message(const char *profile_name, const char *device_name, const char *speaker_mode, switch_stream_handle_t *stream)
{
	skinny_profile_t *profile;

	if ((profile = skinny_find_profile(profile_name))) {
		listener_t *listener = NULL;
		skinny_profile_find_listener_by_device_name(profile, device_name, &listener);
		if(listener) {
			set_speaker_mode(listener, skinny_str2speaker_mode(speaker_mode));
		} else {
			stream->write_function(stream, "Listener not found!\n");
		}
	} else {
		stream->write_function(stream, "Profile not found!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_profile_device_send_call_state_message(const char *profile_name, const char *device_name, const char *call_state, const char *line_instance, const char *call_id, switch_stream_handle_t *stream)
{
	skinny_profile_t *profile;

	if ((profile = skinny_find_profile(profile_name))) {
		listener_t *listener = NULL;
		skinny_profile_find_listener_by_device_name(profile, device_name, &listener);
		if(listener) {
			send_call_state(listener, skinny_str2call_state(call_state), atoi(line_instance), atoi(call_id));
		} else {
			stream->write_function(stream, "Listener not found!\n");
		}
	} else {
		stream->write_function(stream, "Profile not found!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_profile_device_send_reset_message(const char *profile_name, const char *device_name, const char *reset_type, switch_stream_handle_t *stream)
{
	skinny_profile_t *profile;

	if ((profile = skinny_find_profile(profile_name))) {
		listener_t *listener = NULL;
		skinny_profile_find_listener_by_device_name(profile, device_name, &listener);
		if(listener) {
			send_reset(listener, skinny_str2device_reset_type(reset_type));
		} else {
			stream->write_function(stream, "Listener not found!\n");
		}
	} else {
		stream->write_function(stream, "Profile not found!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(skinny_function)
{
	char *argv[1024] = { 0 };
	int argc = 0;
	char *mycmd = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	const char *usage_string = "USAGE:\n"
		"--------------------------------------------------------------------------------\n"
		"skinny help\n"
		"skinny status profile <profile_name>\n"
		"skinny status profile <profile_name> device <device_name>\n"
		"skinny profile <profile_name> device <device_name> send ResetMessage [DeviceReset|DeviceRestart]\n"
		"skinny profile <profile_name> device <device_name> send SetRingerMessage <ring_type> <ring_mode>\n"
		"skinny profile <profile_name> device <device_name> send SetLampMessage <stimulus> <instance> <lamp_mode>\n"
		"skinny profile <profile_name> device <device_name> send SetSpeakerModeMessage <speaker_mode>\n"
		"skinny profile <profile_name> device <device_name> send CallStateMessage <call_state> <line_instance> <call_id>\n"
		"--------------------------------------------------------------------------------\n";
	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (zstr(cmd)) {
		stream->write_function(stream, "%s", usage_string);
		goto done;
	}

	if (!(mycmd = strdup(cmd))) {
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}

	if (!(argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) || !argv[0]) {
		stream->write_function(stream, "%s", usage_string);
		goto done;
	}

	if (!strcasecmp(argv[0], "help")) {/* skinny help */
		stream->write_function(stream, "%s", usage_string);
		goto done;
	} else if (argc == 3 && !strcasecmp(argv[0], "status") && !strcasecmp(argv[1], "profile")) {
		/* skinny status profile <profile_name> */
		status = cmd_status_profile(argv[2], stream);
	} else if (argc == 5 && !strcasecmp(argv[0], "status") && !strcasecmp(argv[1], "profile") && !strcasecmp(argv[3], "device")) {
		/* skinny status profile <profile_name> device <device_name> */
		status = cmd_status_profile_device(argv[2], argv[4], stream);
	} else if (argc >= 6 && !strcasecmp(argv[0], "profile") && !strcasecmp(argv[2], "device") && !strcasecmp(argv[4], "send")) {
		/* skinny profile <profile_name> device <device_name> send ... */
		switch(skinny_str2message_type(argv[5])) {
			case SET_RINGER_MESSAGE:
				if(argc == 8) {
					/* SetRingerMessage <ring_type> <ring_mode> */
					status = cmd_profile_device_send_ringer_message(argv[1], argv[3], argv[6], argv[7], stream);
				}
				break;
			case SET_LAMP_MESSAGE:
				if (argc == 9) {
					/* SetLampMessage <stimulus> <instance> <lamp_mode> */
					status = cmd_profile_device_send_lamp_message(argv[1], argv[3], argv[6], argv[7], argv[8], stream);
				}
				break;
			case SET_SPEAKER_MODE_MESSAGE:
				if (argc == 7) {
					/* SetSpeakerModeMessage <speaker_mode> */
					status = cmd_profile_device_send_speaker_mode_message(argv[1], argv[3], argv[6], stream);
				}
				break;
			case CALL_STATE_MESSAGE:
				if (argc == 9) {
					/* CallStateMessage <call_state> <line_instance> <call_id> */
					status = cmd_profile_device_send_call_state_message(argv[1], argv[3], argv[6], argv[7], argv[8], stream);
				}
				break;
			case RESET_MESSAGE:
				if (argc == 7) {
					/* ResetMessage <reset_type> */
					status = cmd_profile_device_send_reset_message(argv[1], argv[3], argv[6], stream);
				}
				break;
			default:
				stream->write_function(stream, "Unhandled message %s\n", argv[5]);
		}
	} else {
		stream->write_function(stream, "Unknown Command [%s]\n", argv[0]);
	}

done:
	switch_safe_free(mycmd);
	return status;
}

static void event_handler(switch_event_t *event)
{
	if (event->event_id == SWITCH_EVENT_HEARTBEAT) {
		walk_listeners(kill_expired_listener, NULL);
	}
}

static switch_status_t skinny_list_profiles(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_console_callback_match_t *my_matches = NULL;
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_hash_index_t *hi;
	void *val;
	skinny_profile_t *profile;
	
	/* walk profiles */
	for (hi = switch_hash_first(NULL, globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		profile = (skinny_profile_t *) val;

		switch_console_push_match(&my_matches, profile->name);
	}
	
	if (my_matches) {
		*matches = my_matches;
		status = SWITCH_STATUS_SUCCESS;
	}
	
	return status;
}

struct match_helper {
	switch_console_callback_match_t *my_matches;
};

static int skinny_list_devices_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct match_helper *h = (struct match_helper *) pArg;
	char *device_name = argv[0];

	switch_console_push_match(&h->my_matches, device_name);
	return 0;
}

static switch_status_t skinny_list_devices(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	struct match_helper h = { 0 };
	switch_status_t status = SWITCH_STATUS_FALSE;
	skinny_profile_t *profile = NULL;
	char *sql;

	char *myline;
	char *argv[1024] = { 0 };
	int argc = 0;

	if (!(myline = strdup(line))) {
		status = SWITCH_STATUS_MEMERR;
		return status;
	}
	if (!(argc = switch_separate_string(myline, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) || argc != 5) {
		return status;
	}

	if(!strcasecmp(argv[1], "profile")) {/* skinny profile <profile_name> ... */
		profile = skinny_find_profile(argv[2]);
	} else if(!strcasecmp(argv[2], "profile")) {/* skinny status profile <profile_name> ... */
		profile = skinny_find_profile(argv[3]);
	}

	if(profile) {
		if ((sql = switch_mprintf("SELECT name FROM skinny_devices"))) {
			skinny_execute_sql_callback(profile, profile->listener_mutex, sql, skinny_list_devices_callback, &h);
			switch_safe_free(sql);
		}
	}
	
	if (h.my_matches) {
		*matches = h.my_matches;
		status = SWITCH_STATUS_SUCCESS;
	}
	
	return status;
}

static switch_status_t skinny_list_reset_types(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	SKINNY_PUSH_DEVICE_RESET_TYPES
	return status;
}

static switch_status_t skinny_list_stimuli(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	SKINNY_PUSH_STIMULI
	return status;
}

static switch_status_t skinny_list_ring_types(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	SKINNY_PUSH_RING_TYPES
	return status;
}

static switch_status_t skinny_list_ring_modes(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	SKINNY_PUSH_RING_MODES
	return status;
}

static switch_status_t skinny_list_stimulus_instances(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_console_callback_match_t *my_matches = NULL;
	
	switch_console_push_match(&my_matches, "<stimulus_instance>");
	switch_console_push_match(&my_matches, "0");
	
	if (my_matches) {
		*matches = my_matches;
		status = SWITCH_STATUS_SUCCESS;
	}
	return status;
}

static switch_status_t skinny_list_stimulus_modes(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	SKINNY_PUSH_LAMP_MODES
	return status;
}

static switch_status_t skinny_list_speaker_modes(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	SKINNY_PUSH_SPEAKER_MODES
	return status;
}

static switch_status_t skinny_list_call_states(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	SKINNY_PUSH_CALL_STATES
	return status;
}

static switch_status_t skinny_list_line_instances(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_console_callback_match_t *my_matches = NULL;
	
	/* TODO */
	switch_console_push_match(&my_matches, "1");
	switch_console_push_match(&my_matches, "<line_instance>");
	
	if (my_matches) {
		*matches = my_matches;
		status = SWITCH_STATUS_SUCCESS;
	}
	return status;
}

static switch_status_t skinny_list_call_ids(const char *line, const char *cursor, switch_console_callback_match_t **matches)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_console_callback_match_t *my_matches = NULL;
	
	/* TODO */
	switch_console_push_match(&my_matches, "1345");
	switch_console_push_match(&my_matches, "<call_id>");
	
	if (my_matches) {
		*matches = my_matches;
		status = SWITCH_STATUS_SUCCESS;
	}
	return status;
}

/*****************************************************************************/
SWITCH_MODULE_LOAD_FUNCTION(mod_skinny_load)
{
	switch_hash_index_t *hi;
	void *val;
	skinny_profile_t *profile;

	switch_api_interface_t *api_interface;

	module_pool = pool;

	load_skinny_config();

	/* init listeners */
	for (hi = switch_hash_first(NULL, globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		profile = (skinny_profile_t *) val;
	
		switch_mutex_init(&profile->listener_mutex, SWITCH_MUTEX_NESTED, module_pool);
		switch_mutex_init(&profile->sock_mutex, SWITCH_MUTEX_NESTED, module_pool);

	}

	if ((switch_event_bind_removable(modname, SWITCH_EVENT_HEARTBEAT, NULL, event_handler, NULL, &globals.heartbeat_node) != SWITCH_STATUS_SUCCESS)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind our heartbeat handler!\n");
		/* Not such severe to prevent loading */
	}

	if (switch_event_reserve_subclass(SKINNY_EVENT_REGISTER) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!\n", SKINNY_EVENT_REGISTER);
		return SWITCH_STATUS_TERM;
	}

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	skinny_endpoint_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_ENDPOINT_INTERFACE);
	skinny_endpoint_interface->interface_name = "skinny";
	skinny_endpoint_interface->io_routines = &skinny_io_routines;
	skinny_endpoint_interface->state_handler = &skinny_state_handlers;


	SWITCH_ADD_API(api_interface, "skinny", "Skinny Controls", skinny_function, "<cmd> <args>");
	switch_console_set_complete("add skinny help");

	switch_console_set_complete("add skinny status profile ::skinny::list_profiles");
	switch_console_set_complete("add skinny status profile ::skinny::list_profiles device ::skinny::list_devices");

	switch_console_set_complete("add skinny profile ::skinny::list_profiles device ::skinny::list_devices send ResetMessage ::skinny::list_reset_types");
	switch_console_set_complete("add skinny profile ::skinny::list_profiles device ::skinny::list_devices send SetRingerMessage ::skinny::list_ring_types ::skinny::list_ring_modes");
	switch_console_set_complete("add skinny profile ::skinny::list_profiles device ::skinny::list_devices send SetLampMessage ::skinny::list_stimuli ::skinny::list_stimulus_instances ::skinny::list_stimulus_modes");
	switch_console_set_complete("add skinny profile ::skinny::list_profiles device ::skinny::list_devices send SetSpeakerModeMessage ::skinny::list_speaker_modes");
	switch_console_set_complete("add skinny profile ::skinny::list_profiles device ::skinny::list_devices send CallStateMessage ::skinny::list_call_states ::skinny::list_line_instances ::skinny::list_call_ids");

	switch_console_add_complete_func("::skinny::list_profiles", skinny_list_profiles);
	switch_console_add_complete_func("::skinny::list_devices", skinny_list_devices);
	switch_console_add_complete_func("::skinny::list_reset_types", skinny_list_reset_types);
	switch_console_add_complete_func("::skinny::list_ring_types", skinny_list_ring_types);
	switch_console_add_complete_func("::skinny::list_ring_modes", skinny_list_ring_modes);
	switch_console_add_complete_func("::skinny::list_stimuli", skinny_list_stimuli);
	switch_console_add_complete_func("::skinny::list_stimulus_instances", skinny_list_stimulus_instances);
	switch_console_add_complete_func("::skinny::list_stimulus_modes", skinny_list_stimulus_modes);
	switch_console_add_complete_func("::skinny::list_speaker_modes", skinny_list_speaker_modes);
	switch_console_add_complete_func("::skinny::list_call_states", skinny_list_call_states);
	switch_console_add_complete_func("::skinny::list_line_instances", skinny_list_line_instances);
	switch_console_add_complete_func("::skinny::list_call_ids", skinny_list_call_ids);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_RUNTIME_FUNCTION(mod_skinny_runtime)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_hash_index_t *hi;
	void *val;
	skinny_profile_t *profile;
	
	/* launch listeners */
	for (hi = switch_hash_first(NULL, globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		profile = (skinny_profile_t *) val;
	
		status = skinny_socket_create_and_bind(profile);
		if(status != SWITCH_STATUS_SUCCESS) {
			return status;
		}
	}
	return status;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_skinny_shutdown)
{
	switch_hash_index_t *hi;
	void *val;
	skinny_profile_t *profile;
	int sanity = 0;

	switch_event_free_subclass(SKINNY_EVENT_REGISTER);
	switch_event_unbind(&globals.heartbeat_node);

	globals.running = 0;

	/* kill listeners */
	walk_listeners(kill_listener, NULL);

	/* close sockets */
	for (hi = switch_hash_first(NULL, globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		profile = (skinny_profile_t *) val;

		close_socket(&profile->sock, profile);

		while (profile->listener_threads) {
			switch_yield(100000);
			walk_listeners(kill_listener, NULL);
			if (++sanity >= 200) {
				break;
			}
		}
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
