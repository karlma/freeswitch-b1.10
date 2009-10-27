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
 * Travis Cross <tc@traviscross.com>
 *
 * switch_ivr_originate.c -- IVR Library (originate)
 *
 */

#include <switch.h>

static const switch_state_handler_table_t originate_state_handlers;

static switch_status_t originate_on_consume_media_transmit(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);

	/* testing out not using this, we probably don't need it anymore now that we have audio sync */
#if 0
	if (!switch_channel_test_flag(channel, CF_PROXY_MODE)) {
		while (switch_channel_get_state(channel) == CS_CONSUME_MEDIA && !switch_channel_test_flag(channel, CF_TAGGED)) {
			if (!switch_channel_media_ready(channel)) {
				switch_yield(10000);
			} else {
				switch_ivr_sleep(session, 10, SWITCH_FALSE, NULL);
			}
		}
	}
#endif

	switch_channel_clear_state_handler(channel, &originate_state_handlers);

	return SWITCH_STATUS_FALSE;
}

static switch_status_t originate_on_routing(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);

	if (switch_channel_get_state(channel) == CS_ROUTING) {
		/* put the channel in a passive state until it is answered */
		switch_channel_set_state(channel, CS_CONSUME_MEDIA);
	}

	return SWITCH_STATUS_FALSE;
}

static const switch_state_handler_table_t originate_state_handlers = {
	/*.on_init */ NULL,
	/*.on_routing */ originate_on_routing,
	/*.on_execute */ NULL,
	/*.on_hangup */ NULL,
	/*.on_exchange_media */ NULL,
	/*.on_soft_execute */ originate_on_consume_media_transmit,
	/*.on_consume_media */ originate_on_consume_media_transmit
};


typedef struct {
	switch_core_session_t *peer_session;
	switch_channel_t *peer_channel;
	switch_caller_profile_t *caller_profile;
	uint8_t ring_ready;
	uint8_t early_media;
	uint8_t answered;
	uint32_t per_channel_timelimit_sec;
	uint32_t per_channel_progress_timelimit_sec;
	uint32_t per_channel_delay_start;
} originate_status_t;


typedef struct {
	switch_core_session_t *session;
	int32_t idx;
	uint32_t hups;
	char file[512];
	char key[80];
	uint8_t early_ok;
	uint8_t ring_ready;
	uint8_t instant_ringback;
	uint8_t sent_ring;
	uint8_t progress;
	uint8_t return_ring_ready;
	uint8_t monitor_early_media_ring;
	uint8_t monitor_early_media_fail;
	uint8_t gen_ringback;
	uint8_t ignore_early_media;
	uint8_t ignore_ring_ready;
	int monitor_early_media_ring_count;
	int monitor_early_media_ring_total;
	int cancel_timeout;
	int continue_on_timeout;
} originate_global_t;



typedef enum {
	IDX_KEY_CANCEL = -4,
	IDX_TIMEOUT = -3,
	IDX_CANCEL = -2,
	IDX_NADA = -1
} abort_t;

struct key_collect {
	char *key;
	char *file;
	switch_core_session_t *session;
};

static void *SWITCH_THREAD_FUNC collect_thread_run(switch_thread_t *thread, void *obj)
{
	struct key_collect *collect = (struct key_collect *) obj;
	switch_channel_t *channel = switch_core_session_get_channel(collect->session);
	char buf[10] = SWITCH_BLANK_STRING;
	char *p, term;

	if (collect->session) {
		if (switch_core_session_read_lock(collect->session) != SWITCH_STATUS_SUCCESS) {
			return NULL;
		}
	} else {
		return NULL;
	}
	
	switch_ivr_sleep(collect->session, 0, SWITCH_TRUE, NULL);
	
	if (!strcasecmp(collect->key, "exec")) {
		char *data;
		switch_application_interface_t *application_interface;
		char *app_name, *app_data;

		if (!(data = collect->file)) {
			goto wbreak;
		}

		app_name = data;

		if ((app_data = strchr(app_name, ' '))) {
			*app_data++ = '\0';
		}

		if ((application_interface = switch_loadable_module_get_application_interface(app_name)) == 0) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(collect->session), SWITCH_LOG_ERROR, "Invalid Application %s\n", app_name);
			switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			UNPROTECT_INTERFACE(application_interface);
			goto wbreak;
		}

		if (!application_interface->application_function) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(collect->session), SWITCH_LOG_ERROR, "No Function for %s\n", app_name);
			switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			goto wbreak;
		}

		switch_core_session_exec(collect->session, application_interface, app_data);

		if (switch_channel_up(channel)) {
			switch_channel_set_flag(channel, CF_WINNER);
		}
		goto wbreak;
	}

	if (!switch_channel_up(channel)) {
		switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		goto wbreak;
	}

	while (switch_channel_ready(channel)) {
		memset(buf, 0, sizeof(buf));

		if (collect->file) {
			switch_input_args_t args = { 0 };
			args.buf = buf;
			args.buflen = sizeof(buf);
			switch_ivr_play_file(collect->session, NULL, collect->file, &args);
		} else {
			switch_ivr_collect_digits_count(collect->session, buf, sizeof(buf), 1, SWITCH_BLANK_STRING, &term, 0, 0, 0);
		}

		for (p = buf; *p; p++) {
			if (*collect->key == *p) {
				switch_channel_set_flag(channel, CF_WINNER);
				goto wbreak;
			}
		}
	}
 wbreak:

	switch_core_session_rwunlock(collect->session);

	return NULL;
}

static void launch_collect_thread(struct key_collect *collect)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;

	switch_threadattr_create(&thd_attr, switch_core_session_get_pool(collect->session));
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, collect_thread_run, collect, switch_core_session_get_pool(collect->session));
}

static int check_per_channel_timeouts(originate_global_t *oglobals,
									  originate_status_t *originate_status, 
									  int max,
									  time_t start)
{
	int x = 0,i,delayed_channels=0,active_channels=0;
	uint32_t early_exit_time=0, delayed_min=0;

	time_t elapsed = switch_epoch_time_now(NULL) - start;

	if (oglobals->cancel_timeout > 0) {
		return 0;
	}
	for (i = 0; i < max; i++) {
		if (originate_status[i].peer_channel && switch_channel_get_state(originate_status[i].peer_channel) != CS_DESTROY && switch_channel_get_state(originate_status[i].peer_channel) != CS_REPORTING) {
			if (originate_status[i].per_channel_delay_start) {
				delayed_channels++;
			} else {
				active_channels++;
			}
		}
	}

	if (active_channels == 0 && delayed_channels) {
		for (i = 0; i < max; i++) {
			if ( originate_status[i].peer_channel && originate_status[i].per_channel_delay_start && (! delayed_min || delayed_min > originate_status[i].per_channel_delay_start) ) {
				delayed_min = originate_status[i].per_channel_delay_start;
			}
		}
		early_exit_time = delayed_min - (uint32_t) elapsed;
	}
	for (i = 0; i < max; i++) {
		if (originate_status[i].peer_channel && originate_status[i].per_channel_delay_start && (elapsed > originate_status[i].per_channel_delay_start || active_channels == 0) ) {
			if (active_channels == 0) {
				if (originate_status[i].per_channel_timelimit_sec) {
					if (originate_status[i].per_channel_timelimit_sec > early_exit_time) /* IN theory this check is not needed ( should just be if !0 then -= with no else), if its not 0 it should always be greater.... */
						originate_status[i].per_channel_timelimit_sec -= early_exit_time;
					else
						originate_status[i].per_channel_timelimit_sec = 1;
				}
				if (originate_status[i].per_channel_progress_timelimit_sec) {
					if (originate_status[i].per_channel_progress_timelimit_sec > early_exit_time) /* IN theory this check is not needed ( should just be if !0 then -= with no else), if its not 0 it should always be greater.... */
						originate_status[i].per_channel_progress_timelimit_sec -= early_exit_time;
					else
						originate_status[i].per_channel_progress_timelimit_sec = 1;
				}
				originate_status[i].per_channel_delay_start -= delayed_min;
			} else {
				originate_status[i].per_channel_delay_start = 0;
			}

			if (! originate_status[i].per_channel_delay_start) {
				switch_channel_clear_flag(originate_status[i].peer_channel, CF_BLOCK_STATE);
			}
		}

		if (originate_status[i].peer_channel && switch_channel_up(originate_status[i].peer_channel)) {
			if (originate_status[i].per_channel_progress_timelimit_sec && elapsed > originate_status[i].per_channel_progress_timelimit_sec &&
				!(
				  switch_channel_test_flag(originate_status[i].peer_channel, CF_RING_READY) ||
				  switch_channel_test_flag(originate_status[i].peer_channel, CF_ANSWERED) ||
				  (!oglobals->monitor_early_media_ring && switch_channel_test_flag(originate_status[i].peer_channel, CF_EARLY_MEDIA))
				  )
				) {
				switch_channel_hangup(originate_status[i].peer_channel, SWITCH_CAUSE_PROGRESS_TIMEOUT);
				x++;
			}
			if (originate_status[i].per_channel_timelimit_sec && elapsed > originate_status[i].per_channel_timelimit_sec) {
				switch_channel_hangup(originate_status[i].peer_channel, SWITCH_CAUSE_ALLOTTED_TIMEOUT);
				x++;
			}
		}
	}

	return x;
}

static switch_bool_t monitor_callback(switch_core_session_t *session, const char *app, const char *data)
{
	if (app) {
		switch_channel_t *channel = switch_core_session_get_channel(session);
		if (!strcmp(app, "fail")) {
			const char *bd = switch_channel_get_variable(channel, "monitor_fail_dispo");
			if (!bd) {
				bd = "monitor_early_media_fail";
			}
			switch_channel_set_variable(channel, "originate_disposition", bd);
			switch_channel_hangup(channel, data ? switch_channel_str2cause(data) : SWITCH_CAUSE_USER_BUSY);
		} else if (!strcmp(app, "ring")) {
			originate_global_t *oglobals = (originate_global_t *) switch_channel_get_private(channel, "_oglobals_");
			const char *bd = switch_channel_get_variable(channel, "monitor_ring_dispo");
			if (!bd) {
				bd = "monitor_early_media_ring";
			}
			switch_channel_set_variable(channel, "originate_disposition", bd);
			
			if (oglobals) {
				if (oglobals->monitor_early_media_ring_total && ++oglobals->monitor_early_media_ring_count < oglobals->monitor_early_media_ring_total) {
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Ring %d/%d\n", 
									  oglobals->monitor_early_media_ring_count, oglobals->monitor_early_media_ring_total);
					return SWITCH_TRUE;
				}
	
				switch_channel_set_private(channel, "_oglobals_", NULL);
				
				if (!oglobals->progress) {
					oglobals->progress = 1;
				}
			
				if (!oglobals->ring_ready && !oglobals->ignore_ring_ready) {
					oglobals->ring_ready = 1;
				}
				
				if (!oglobals->ignore_early_media && !oglobals->early_ok) {
					oglobals->early_ok = 1;
				}
			}
		}
	}

	return SWITCH_FALSE;
}

static uint8_t check_channel_status(originate_global_t *oglobals, originate_status_t *originate_status, uint32_t len)
{

	uint32_t i;
	uint8_t rval = 0;
	switch_channel_t *caller_channel = NULL;
	int pindex = -1;
	char bug_key[256] = "";

	oglobals->hups = 0;
	oglobals->idx = IDX_NADA;


	if (oglobals->session) {
		caller_channel = switch_core_session_get_channel(oglobals->session);
		if (switch_channel_test_flag(caller_channel, CF_XFER_ZOMBIE)) {
			caller_channel = NULL;
		}
	}
	

	for (i = 0; i < len; i++) {
		switch_channel_state_t state;
		if (!(originate_status[i].peer_channel && originate_status[i].peer_session)) {
			oglobals->hups++;
			continue;
		}

		if (switch_channel_test_flag(originate_status[i].peer_channel, CF_RING_READY)) {
			if (!originate_status[i].ring_ready) {
				originate_status[i].ring_ready = 1;
			}

			if (!oglobals->ring_ready) {
				oglobals->ring_ready = 1;
				if (caller_channel && !oglobals->ignore_ring_ready) {
					if (len == 1) {
						switch_channel_pass_callee_id(originate_status[0].peer_channel, caller_channel);
					}
					switch_channel_ring_ready(caller_channel);
					oglobals->sent_ring = 1;
				}
			}
		}

		if (switch_channel_test_flag(originate_status[i].peer_channel, CF_EARLY_MEDIA)) {
			if (!oglobals->sent_ring && oglobals->ignore_early_media == 2 && len == 1 && caller_channel && !oglobals->ignore_ring_ready) {
				switch_channel_pass_callee_id(originate_status[0].peer_channel, caller_channel);
				switch_channel_ring_ready(caller_channel);
				oglobals->sent_ring = 1;
			}

			if (!originate_status[i].early_media) {
				originate_status[i].early_media = 1;
				if (oglobals->early_ok) {
					pindex = i;
				}
				
				if (oglobals->monitor_early_media_fail) {
					const char *var = switch_channel_get_variable(originate_status[i].peer_channel, "monitor_early_media_fail");
					if (!zstr(var)) {
						char *fail_array[128] = {0};
						int fail_count = 0;
						char *fail_data = strdup(var);
						int fx;
						int y = 0;

						switch_assert(fail_data);
						fail_count = switch_separate_string(fail_data, '!', fail_array, (sizeof(fail_array) / sizeof(fail_array[0])));
					
						for(fx = 0; fx < fail_count; fx++) {
							char *cause = fail_array[fx];
							int hits = 2;
							char *p, *q;
						
							if (!(p = strchr(cause, ':'))) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_ERROR, "Parse Error\n");
								continue;
							}
							*p++ = '\0';
						

							if (!p) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_ERROR, "Parse Error\n");
								continue;
							}
						

							if (!(hits = atoi(p))) {
								hits = 2;
							}
						

							if (!(p = strchr(p, ':'))) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_ERROR, "Parse Error\n");
								continue;
							}
							*p++ = '\0';
						
							if (!p) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_ERROR, "Parse Error\n");
								continue;
							}

							for(q = p; q && *q; q++) {
								if (*q == '+') {
									*q = ',';
								}
							}
							switch_snprintf(bug_key, sizeof(bug_key), "monitor_early_media_fail_%d", ++y);
							switch_ivr_tone_detect_session(originate_status[i].peer_session,
														   bug_key,
														   p, "r", 0, 
														   hits, 
														   "fail", 
														   cause, monitor_callback);
						
						}
					
						switch_safe_free(fail_data);
					
					}
				}
				
				if (oglobals->monitor_early_media_ring) {
					const char *var = switch_channel_get_variable(originate_status[i].peer_channel, "monitor_early_media_ring");
					const char *var_total = switch_channel_get_variable(originate_status[i].peer_channel, "monitor_early_media_ring_total");
					if (!zstr(var)) {
						char *ring_array[128] = {0};
						int ring_count = 0;
						char *ring_data = strdup(var);
						int fx;
						int y = 0;

						switch_assert(ring_data);
						ring_count = switch_separate_string(ring_data, '!', ring_array, (sizeof(ring_array) / sizeof(ring_array[0])));
					
						for(fx = 0; fx < ring_count; fx++) {
							int hits = 2;
							char *p = ring_array[fx], *q;
						
							if (!p) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_ERROR, "Parse Error\n");
								continue;
							}

							if (!(hits = atoi(p))) {
								hits = 2;
							}
						
							if (!(p = strchr(p, ':'))) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_ERROR, "Parse Error\n");
								continue;
							}
							*p++ = '\0';
						
							if (!p) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_ERROR, "Parse Error\n");
								continue;
							}

							for(q = p; q && *q; q++) {
								if (*q == '+') {
									*q = ',';
								}
							}
						
							switch_channel_set_private(originate_status[i].peer_channel, "_oglobals_", oglobals);
							switch_snprintf(bug_key, sizeof(bug_key), "monitor_early_media_ring_%d", ++y);
							switch_ivr_tone_detect_session(originate_status[i].peer_session,
														   bug_key,
														   p, "r", 0, hits, "ring", NULL, monitor_callback);
							
						}
					
						if (var_total) {
							int tmp = atoi(var_total);
							if (tmp > 0 && tmp < 100) {
								switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(originate_status[i].peer_session), SWITCH_LOG_DEBUG, "%s setting ring total to %d\n", 
												  switch_channel_get_name(originate_status[i].peer_channel), tmp);
								oglobals->monitor_early_media_ring_total = tmp;
							}
						}

						switch_safe_free(ring_data);
					
					}
				}
			}
			
			if (!oglobals->monitor_early_media_ring) {

				if (!oglobals->progress) {
					oglobals->progress = 1;
				}
				
				if (!oglobals->ring_ready && !oglobals->ignore_ring_ready) {
					oglobals->ring_ready = 1;
					
				}
			}
		}
		
		switch_ivr_parse_all_events(originate_status[i].peer_session);

		state = switch_channel_get_state(originate_status[i].peer_channel);
		if (state >= CS_HANGUP || state == CS_RESET || switch_channel_test_flag(originate_status[i].peer_channel, CF_TRANSFER) ||
			switch_channel_test_flag(originate_status[i].peer_channel, CF_REDIRECT) ||
			switch_channel_test_flag(originate_status[i].peer_channel, CF_BRIDGED) || 
			!switch_channel_test_flag(originate_status[i].peer_channel, CF_ORIGINATING)
			) {
			(oglobals->hups)++;
		} else if ((switch_channel_test_flag(originate_status[i].peer_channel, CF_ANSWERED) ||
					(oglobals->early_ok && switch_channel_test_flag(originate_status[i].peer_channel, CF_EARLY_MEDIA)) ||
					(oglobals->ring_ready && oglobals->return_ring_ready && len == 1 && 
					 switch_channel_test_flag(originate_status[i].peer_channel, CF_RING_READY))
					)
				   && !switch_channel_test_flag(originate_status[i].peer_channel, CF_TAGGED)
				   ) {
			
			if (!zstr(oglobals->key)) {
				struct key_collect *collect;

				if (oglobals->cancel_timeout < 0) {
					oglobals->cancel_timeout = 1;
				}

				if ((collect = switch_core_session_alloc(originate_status[i].peer_session, sizeof(*collect)))) {
					switch_channel_set_flag(originate_status[i].peer_channel, CF_TAGGED);
					if (!zstr(oglobals->key)) {
						collect->key = switch_core_session_strdup(originate_status[i].peer_session, oglobals->key);
					}
					if (!zstr(oglobals->file)) {
						collect->file = switch_core_session_strdup(originate_status[i].peer_session, oglobals->file);
					}
					switch_channel_audio_sync(originate_status[i].peer_channel);
					collect->session = originate_status[i].peer_session;
					launch_collect_thread(collect);
				}
			} else {
				oglobals->idx = i;
				pindex = (uint32_t) i;
				rval = 0;
				goto end;

			}
		} else if (switch_channel_test_flag(originate_status[i].peer_channel, CF_WINNER)) {
			oglobals->idx = i;
			rval = 0;
			pindex = (uint32_t) i;
			goto end;
		}
	}

	if (oglobals->hups == len) {
		rval = 0;
	} else {
		rval = 1;
	}

 end:

	if (pindex > -1 && caller_channel && switch_channel_ready(caller_channel) && !switch_channel_media_ready(caller_channel)) {
		const char *var = switch_channel_get_variable(caller_channel, "inherit_codec");
		if (switch_true(var)) {
			switch_codec_implementation_t impl = { 0 };
			char tmp[128] = "";

			
			if (switch_core_session_get_read_impl(originate_status[pindex].peer_session, &impl) == SWITCH_STATUS_SUCCESS) {
				switch_snprintf(tmp, sizeof(tmp), "%s@%uh@%ui", impl.iananame, impl.samples_per_second, impl.microseconds_per_packet / 1000);
				switch_channel_set_variable(caller_channel, "absolute_codec_string", tmp);
				switch_log_printf(SWITCH_CHANNEL_CHANNEL_LOG(caller_channel), SWITCH_LOG_DEBUG, "Setting codec string on %s to %s\n", switch_channel_get_name(caller_channel), tmp);
			} else {
				switch_log_printf(SWITCH_CHANNEL_CHANNEL_LOG(originate_status[pindex].peer_channel), SWITCH_LOG_WARNING, "Error inheriting codec.  Channel %s has no read codec yet.\n", 
								  switch_channel_get_name(originate_status[pindex].peer_channel));
			}

		}
	}
	

	return rval;

}

struct ringback {
	switch_buffer_t *audio_buffer;
	teletone_generation_session_t ts;
	switch_file_handle_t fhb;
	switch_file_handle_t *fh;
	int silence;
	uint8_t asis;
};

typedef struct ringback ringback_t;

static int teletone_handler(teletone_generation_session_t *ts, teletone_tone_map_t *map)
{
	ringback_t *tto = ts->user_data;
	int wrote;

	if (!tto) {
		return -1;
	}
	wrote = teletone_mux_tones(ts, map);
	switch_buffer_write(tto->audio_buffer, ts->buffer, wrote * 2);

	return 0;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_wait_for_answer(switch_core_session_t *session, switch_core_session_t *peer_session)
{
	switch_channel_t *caller_channel = NULL;
	switch_channel_t *peer_channel = switch_core_session_get_channel(peer_session);
	const char *ringback_data = NULL;
	switch_frame_t write_frame = { 0 };
	switch_codec_t write_codec = { 0 };
	switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
	uint8_t pass = 0;
	ringback_t ringback = { 0 };
	switch_frame_t *read_frame = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	int timelimit = 60;
	const char *var;
	switch_time_t start = 0;
	const char *cancel_key = NULL;
	switch_channel_state_t wait_state = 0;

	switch_assert(peer_channel);
	
	if (switch_channel_get_state(peer_channel) == CS_RESET) {
		switch_channel_set_state(peer_channel, CS_SOFT_EXECUTE);
	}

	if (session) {
		caller_channel = switch_core_session_get_channel(session);
	}

	if ((switch_channel_test_flag(peer_channel, CF_ANSWERED) || switch_channel_test_flag(peer_channel, CF_EARLY_MEDIA))) {
		goto end;
	}

	switch_zmalloc(write_frame.data, SWITCH_RECOMMENDED_BUFFER_SIZE);
	write_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE;

	if (caller_channel && (var = switch_channel_get_variable(caller_channel, SWITCH_CALL_TIMEOUT_VARIABLE))) {
		timelimit = atoi(var);
		if (timelimit < 0) {
			timelimit = 60;
		}
	}

	timelimit *= 1000000;
	start = switch_micro_time_now();

	if (caller_channel) {
		cancel_key = switch_channel_get_variable(caller_channel, "origination_cancel_key");

		if (switch_channel_test_flag(caller_channel, CF_ANSWERED)) {
			ringback_data = switch_channel_get_variable(caller_channel, "transfer_ringback");
		}
		
		if (!ringback_data) {
			ringback_data = switch_channel_get_variable(caller_channel, "ringback");
		}

		if (switch_channel_test_flag(caller_channel, CF_PROXY_MODE) || switch_channel_test_flag(caller_channel, CF_PROXY_MEDIA)) {
			ringback_data = NULL;
		} else if (zstr(ringback_data)) {
			if ((var = switch_channel_get_variable(caller_channel, SWITCH_SEND_SILENCE_WHEN_IDLE_VARIABLE))) {
				int sval = atoi(var);

				if (sval) {
					ringback_data = switch_core_session_sprintf(session, "silence:%d", sval);
				}
			}
		}
	}


	if (read_codec && ringback_data) {
		if (switch_is_file_path(ringback_data)) {
			if (!(strrchr(ringback_data, '.') || strstr(ringback_data, SWITCH_URL_SEPARATOR))) {
				ringback.asis++;
			}
		}


		
		if (!ringback.asis) {
			if ((pass = (uint8_t) switch_test_flag(read_codec, SWITCH_CODEC_FLAG_PASSTHROUGH))) {
				goto no_ringback;
			}

			if (switch_core_codec_init(&write_codec,
									   "L16",
									   NULL,
									   read_codec->implementation->actual_samples_per_second,
									   read_codec->implementation->microseconds_per_packet / 1000,
									   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE, NULL,
									   switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Codec Error!\n");
				if (caller_channel) {
					switch_channel_hangup(caller_channel, SWITCH_CAUSE_NORMAL_TEMPORARY_FAILURE);
				}
				read_codec = NULL;
				goto done;
			} else {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG,
								  "Raw Codec Activation Success L16@%uhz 1 channel %dms\n",
								  read_codec->implementation->actual_samples_per_second, read_codec->implementation->microseconds_per_packet / 1000);
				
				write_frame.codec = &write_codec;
				write_frame.datalen = read_codec->implementation->decoded_bytes_per_packet;
				write_frame.samples = write_frame.datalen / 2;
				memset(write_frame.data, 255, write_frame.datalen);
				switch_core_session_set_read_codec(session, &write_codec);
			}
		}
		
		if (switch_channel_test_flag(caller_channel, CF_DISABLE_RINGBACK)) {
			ringback_data = NULL;
		}

		if (ringback_data) {
			char *tmp_data = NULL;

			if (switch_is_file_path(ringback_data)) {
				char *ext;

				if (ringback.asis) {
					write_frame.codec = read_codec;
					ext = read_codec->implementation->iananame;
					tmp_data = switch_mprintf("%s.%s", ringback_data, ext);
					ringback_data = tmp_data;
				}

				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Play Ringback File [%s]\n", ringback_data);

				ringback.fhb.channels = read_codec->implementation->number_of_channels;
				ringback.fhb.samplerate = read_codec->implementation->actual_samples_per_second;
				if (switch_core_file_open(&ringback.fhb,
										  ringback_data,
										  read_codec->implementation->number_of_channels,
										  read_codec->implementation->actual_samples_per_second,
										  SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT, NULL) != SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Error Playing File\n");
					switch_safe_free(tmp_data);
					goto done;
				}
				ringback.fh = &ringback.fhb;
			} else {
				if (!strncasecmp(ringback_data, "silence", 7)) {
					const char *p = ringback_data + 7;
					if (*p == ':') {
						p++;
						if (p) {
							ringback.silence = atoi(p);
						}
					}
					if (ringback.silence <= 0) {
						ringback.silence = 400;
					}
				} else {
					switch_buffer_create_dynamic(&ringback.audio_buffer, 512, 1024, 0);
					switch_buffer_set_loops(ringback.audio_buffer, -1);
							
					teletone_init_session(&ringback.ts, 0, teletone_handler, &ringback);
					ringback.ts.rate = read_codec->implementation->actual_samples_per_second;
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Play Ringback Tone [%s]\n", ringback_data);
					if (teletone_run(&ringback.ts, ringback_data)) {
						switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Error Playing Tone\n");
						teletone_destroy_session(&ringback.ts);
						switch_buffer_destroy(&ringback.audio_buffer);
						ringback_data = NULL;
					}
				}
			}
			switch_safe_free(tmp_data);
		}
	}

 no_ringback:

	if (caller_channel) {
		wait_state = switch_channel_get_state(caller_channel);
	}

	while (switch_channel_ready(peer_channel) && !switch_channel_media_ready(peer_channel)) {
		int diff = (int) (switch_micro_time_now() - start);

		if (caller_channel && cancel_key) {
			if (switch_channel_has_dtmf(caller_channel)) {
				switch_dtmf_t dtmf = { 0, 0 };
				if (switch_channel_dequeue_dtmf(caller_channel, &dtmf) == SWITCH_STATUS_SUCCESS) {
					if (dtmf.digit == *cancel_key) {
						status = SWITCH_STATUS_FALSE;
						goto done;
					}
				}
			}
		}

		if (caller_channel && switch_channel_get_state(caller_channel) != wait_state) {
			goto done;
		}

		if (diff > timelimit) {
			status = SWITCH_STATUS_TIMEOUT;
			goto done;
		}

		if (switch_channel_media_ready(caller_channel)) {
			status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
		} else {
			read_frame = NULL;
		}
		
		if (read_frame && !pass) {

			if (ringback.fh) {
				switch_size_t mlen, olen;
				unsigned int pos = 0;
				
				if (ringback.asis) {
					mlen = write_frame.codec->implementation->encoded_bytes_per_packet;
				} else {
					mlen = write_frame.codec->implementation->samples_per_packet;
				}

				olen = mlen;
				//if (ringback.fh->resampler && ringback.fh->resampler->rfactor > 1) {
				//olen = (switch_size_t) (olen * ringback.fh->resampler->rfactor);
				//}
				switch_core_file_read(ringback.fh, write_frame.data, &olen);

				if (olen == 0) {
					olen = mlen;
					ringback.fh->speed = 0;
					switch_core_file_seek(ringback.fh, &pos, 0, SEEK_SET);
					switch_core_file_read(ringback.fh, write_frame.data, &olen);
					if (olen == 0) {
						break;
					}
				}
				write_frame.datalen = (uint32_t) (ringback.asis ? olen : olen * 2);
			} else if (ringback.audio_buffer) {
				if ((write_frame.datalen = (uint32_t) switch_buffer_read_loop(ringback.audio_buffer,
																			  write_frame.data,
																			  write_frame.codec->implementation->decoded_bytes_per_packet)) <= 0) {
					break;
				}
			} else if (ringback.silence) {
				write_frame.datalen = write_frame.codec->implementation->decoded_bytes_per_packet;
				switch_generate_sln_silence((int16_t *) write_frame.data, write_frame.datalen / 2, ringback.silence);
			}

			if ((ringback.fh || ringback.silence || ringback.audio_buffer) && write_frame.codec && write_frame.datalen) {
				if (switch_core_session_write_frame(session, &write_frame, SWITCH_IO_FLAG_NONE, 0) != SWITCH_STATUS_SUCCESS) {
					break;
				}
			}
		} else {
			switch_cond_next();
		}
	}

 done:

	if (ringback.fh) {
		switch_core_file_close(ringback.fh);
		ringback.fh = NULL;
	} else if (ringback.audio_buffer) {
		teletone_destroy_session(&ringback.ts);
		switch_buffer_destroy(&ringback.audio_buffer);
	}

	switch_core_session_reset(session, SWITCH_TRUE, SWITCH_TRUE);

	if (switch_core_codec_ready(&write_codec)) {
		switch_core_codec_destroy(&write_codec);
	}

	switch_safe_free(write_frame.data);

 end:

	if (!switch_channel_media_ready(peer_channel)) {
		if (switch_channel_up(peer_channel)) {
			switch_channel_hangup(peer_channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		}
		status = SWITCH_STATUS_FALSE;
	}
	
	if (caller_channel && !switch_channel_ready(caller_channel)) {
		status = SWITCH_STATUS_FALSE;
	}

	return status;
}

SWITCH_DECLARE(void) switch_process_import(switch_core_session_t *session, switch_channel_t *peer_channel, const char *varname)
{
	const char *import, *val;
	switch_channel_t *caller_channel;

	switch_assert(session && peer_channel);
	caller_channel = switch_core_session_get_channel(session);
	
	if ((import = switch_channel_get_variable(caller_channel, varname))) {
		char *mydata = switch_core_session_strdup(session, import);
		int i, argc;
		char *argv[64] = { 0 };

		if ((argc = switch_separate_string(mydata, ',', argv, (sizeof(argv) / sizeof(argv[0]))))) {
			for(i = 0; i < argc; i++) {
				if ((val = switch_channel_get_variable(peer_channel, argv[i]))) {
					switch_channel_set_variable(caller_channel, argv[i], val);
				}
			}
		}
	}
}


#define peer_eligible(_peer) (_peer && !(switch_channel_test_flag(_peer, CF_TRANSFER) || \
										 switch_channel_test_flag(_peer, CF_REDIRECT) || \
										 switch_channel_test_flag(_peer, CF_BRIDGED) || \
										 switch_channel_get_state(_peer) == CS_RESET || \
										 !switch_channel_test_flag(_peer, CF_ORIGINATING)))

#define MAX_PEERS 128
SWITCH_DECLARE(switch_status_t) switch_ivr_originate(switch_core_session_t *session,
													 switch_core_session_t **bleg,
													 switch_call_cause_t *cause,
													 const char *bridgeto,
													 uint32_t timelimit_sec,
													 const switch_state_handler_table_t *table,
													 const char *cid_name_override,
													 const char *cid_num_override,
													 switch_caller_profile_t *caller_profile_override, 
													 switch_event_t *ovars,
													 switch_originate_flag_t flags
													 )
{
	originate_status_t originate_status[MAX_PEERS] = { { 0 } }; 
	switch_originate_flag_t dftflags = SOF_NONE, myflags = dftflags;
	char *pipe_names[MAX_PEERS] = { 0 };
	char *data = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_channel_t *caller_channel = NULL;
	char *peer_names[MAX_PEERS] = { 0 };
	switch_core_session_t *new_session = NULL, *peer_session;
	switch_caller_profile_t *new_profile = NULL, *caller_caller_profile;
	char *chan_type = NULL, *chan_data;
	switch_channel_t *peer_channel = NULL;
	ringback_t ringback = { 0 };
	time_t start;
	switch_frame_t *read_frame = NULL;
	switch_memory_pool_t *pool = NULL;
	int r = 0, i, and_argc = 0, or_argc = 0;
	int32_t sleep_ms = 1000, try = 0, retries = 1;
	switch_codec_t write_codec = { 0 };
	switch_frame_t write_frame = { 0 };
	uint8_t pass = 0;
	char *odata, *var;
	switch_call_cause_t reason = SWITCH_CAUSE_NONE;
	switch_call_cause_t force_reason = SWITCH_CAUSE_NONE;
	uint8_t to = 0;
	char *var_val, *vars = NULL;
	int var_block_count = 0;
	char *e = NULL;
	const char *ringback_data = NULL;
	switch_codec_t *read_codec = NULL;
	switch_event_t *var_event = NULL;
	uint8_t fail_on_single_reject = 0;
	char *fail_on_single_reject_var = NULL;
	char *loop_data = NULL;
	uint32_t progress_timelimit_sec = 0;
	const char *cid_tmp, *lc;
	originate_global_t oglobals = { 0 };
	int cdr_total = 0;
	int local_clobber = 0;
	const char *cancel_key = NULL;
	const char *holding = NULL;

	if (session) {
		const char *to_var;
		caller_channel = switch_core_session_get_channel(session);
		switch_channel_set_flag(caller_channel, CF_ORIGINATOR);
		oglobals.session = session;


		if ((to_var = switch_channel_get_variable(caller_channel, SWITCH_CALL_TIMEOUT_VARIABLE))) {
			timelimit_sec = atoi(to_var);
		}

		if (switch_true(switch_channel_get_variable(caller_channel, SWITCH_PROXY_MEDIA_VARIABLE))) {
			switch_channel_set_flag(caller_channel, CF_PROXY_MEDIA);
		}

		if (switch_channel_test_flag(caller_channel, CF_PROXY_MODE)
			|| (switch_true(switch_channel_get_variable(caller_channel, SWITCH_BYPASS_MEDIA_VARIABLE)))) {
			if (!switch_channel_test_flag(caller_channel, CF_ANSWERED)
				&& !switch_channel_test_flag(caller_channel, CF_EARLY_MEDIA)) {
				switch_channel_set_flag(caller_channel, CF_PROXY_MODE);
			} else {
				if (switch_channel_test_flag(caller_channel, CF_PROXY_MODE)) {
					switch_ivr_media(switch_core_session_get_uuid(session), SMF_REBRIDGE);
					switch_channel_set_flag(caller_channel, CF_PROXY_MODE);
				} else {
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Channel is already up, delaying proxy mode 'till both legs are answered.\n");
					switch_channel_set_variable(caller_channel, "bypass_media_after_bridge", "true");
					switch_channel_set_variable(caller_channel, SWITCH_BYPASS_MEDIA_VARIABLE, NULL);
					switch_channel_clear_flag(caller_channel, CF_PROXY_MODE);
				}
			}
		}
	}

	if (timelimit_sec <= 0) {
		timelimit_sec = 60;
	}

	
	oglobals.idx = IDX_NADA;
	oglobals.early_ok = 1;



	*bleg = NULL;

	switch_zmalloc(write_frame.data, SWITCH_RECOMMENDED_BUFFER_SIZE);
	write_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE;

	odata = strdup(bridgeto);

	if (!odata) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Memory Error!\n");
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}

	data = odata;

	/* strip leading spaces */
	while (data && *data && *data == ' ') {
		data++;
	}

	/* extract channel variables, allowing multiple sets of braces */
	while (*data == '{') {
		if (!var_block_count) {
			e = switch_find_end_paren(data, '{', '}');
			if (!e || !*e) {
				goto var_extract_error;
			}
			vars = data + 1;
			*e = '\0';
			data = e + 1;
		} else {
			int j = 0, k = 0;
			if (e) {
				*e = ',';
			}
			e = switch_find_end_paren(data, '{', '}');
			if (!e || !*e) {
				goto var_extract_error;
			}
			/* swallow the opening bracket */
			while ((data + k) && *(data + k)) {
				j = k; k++;
				/* note that this affects vars[] */
				data[j] = data[k];
			}
			*(--e) = '\0';
			data = e + 1;
		}
		var_block_count++;
		continue;
    
	var_extract_error:
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Parse Error!\n");
		status = SWITCH_STATUS_GENERR;
		goto done;
	}

	/* strip leading spaces (again) */
	while (data && *data && *data == ' ') {
		data++;
	}

	if (zstr(data)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "No origination URL specified!\n");
		status = SWITCH_STATUS_GENERR;
		goto done;
	}

	/* Some channel are created from an originating channel and some aren't so not all outgoing calls have a way to get params
	   so we will normalize dialstring params and channel variables (when there is an originator) into an event that we 
	   will use as a pseudo hash to consult for params as needed.
	*/

	if (ovars) {
		var_event = ovars;
	} else {
		if (switch_event_create_plain(&var_event, SWITCH_EVENT_CHANNEL_DATA) != SWITCH_STATUS_SUCCESS) {
			abort();
		}
	}

	if (vars) {					/* Parse parameters specified from the dialstring */
		char *var_array[1024] = { 0 };
		int var_count = 0;
		if ((var_count = switch_separate_string(vars, ',', var_array, (sizeof(var_array) / sizeof(var_array[0]))))) {
			int x = 0;
			for (x = 0; x < var_count; x++) {
				char *inner_var_array[2] = { 0 };
				int inner_var_count;
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "variable string %d = [%s]\n", x, var_array[x]);
				if ((inner_var_count =
					 switch_separate_string(var_array[x], '=', inner_var_array, (sizeof(inner_var_array) / sizeof(inner_var_array[0])))) == 2) {
					switch_event_del_header(var_event, inner_var_array[0]);
					switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, inner_var_array[0], inner_var_array[1]);
				}
			}
		}
	}

	if (oglobals.session) {
		switch_event_header_t *hi;
		const char *cdr_total_var;

		if ((cdr_total_var = switch_channel_get_variable(caller_channel, "failed_xml_cdr_total"))) {
			int tmp = atoi(cdr_total_var);
			if (tmp > 0) {
				cdr_total = tmp;
			}
		}
		

		/* Copy all the missing applicable channel variables from A-leg into the event */
		if ((hi = switch_channel_variable_first(caller_channel))) {
			for (; hi; hi = hi->next) {
				int ok = 0;
				if (!strcasecmp((char *) hi->name, "group_confirm_key")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "group_confirm_file")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "group_confirm_cancel_timeout")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "forked_dial")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "fail_on_single_reject")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "ignore_early_media")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "originate_continue_on_timeout")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "ignore_ring_ready")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "monitor_early_media_ring")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "monitor_early_media_ring_total")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "monitor_early_media_fail")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "return_ring_ready")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "ring_ready")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "instant_ringback")) {
					ok = 1;
				} else if (!strcasecmp((char *) hi->name, "progress_timeout")) {
					ok = 1;
				}

				if (ok && !switch_event_get_header(var_event, hi->name)) {
					switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, (char *) hi->name, (char *) hi->value);
				}
			}
			switch_channel_variable_last(caller_channel);
		}
		/*
		  if ((hi = switch_channel_variable_first(caller_channel))) {
		  for (; hi; hi = switch_hash_next(hi)) {
		  switch_hash_this(hi, &vvar, NULL, &vval);
		  if (vvar && vval) {
		  switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, (void *) vvar, (char *) vval);
		  }
		  }
		  switch_channel_variable_last(caller_channel);
		  }
		*/
	}

	if (caller_channel) {		/* ringback is only useful when there is an originator */
		ringback_data = NULL;
		cancel_key = switch_channel_get_variable(caller_channel, "origination_cancel_key");

		if (switch_channel_test_flag(caller_channel, CF_ANSWERED)) {
			ringback_data = switch_channel_get_variable(caller_channel, "transfer_ringback");
		}

		if (!ringback_data) {
			ringback_data = switch_channel_get_variable(caller_channel, "ringback");
		}

		switch_channel_set_variable(caller_channel, "originate_disposition", "failure");

		if (switch_channel_test_flag(caller_channel, CF_PROXY_MODE) || switch_channel_test_flag(caller_channel, CF_PROXY_MEDIA)) {
			ringback_data = NULL;
		} else if (zstr(ringback_data)) {
			const char *vvar;
			
			if ((vvar = switch_channel_get_variable(caller_channel, SWITCH_SEND_SILENCE_WHEN_IDLE_VARIABLE))) {
				int sval = atoi(vvar);

				if (sval) {
					ringback_data = switch_core_session_sprintf(oglobals.session, "silence:%d", sval);
				}

			}
		}
	}

#if 0
	/* changing behaviour ignore_early_media=true must also be explicitly set for previous behaviour */
	if (ringback_data) {
		oglobals.early_ok = 0;
	}
#endif

	if (switch_true(switch_event_get_header(var_event, "group_confirm_cancel_timeout"))) {
		oglobals.cancel_timeout = -1;
	}

	if ((var = switch_event_get_header(var_event, "group_confirm_key"))) {
		switch_copy_string(oglobals.key, var, sizeof(oglobals.key));
		if ((var = switch_event_get_header(var_event, "group_confirm_file"))) {
			switch_copy_string(oglobals.file, var, sizeof(oglobals.file));
		}
	}
	/* When using the AND operator, the fail_on_single_reject flag may be set in order to indicate that a single
	   rejections should terminate the attempt rather than a timeout, answer, or rejection by all. 
	   If the value is set to 'true' any fail cause will end the attempt otherwise it can contain a comma (,) separated
	   list of cause names which should be considered fatal
	*/
	if ((var = switch_event_get_header(var_event, "fail_on_single_reject")) && switch_true(var)) {
		fail_on_single_reject_var = strdup(var);
		fail_on_single_reject = 1;
	}

	if ((*oglobals.file != '\0') && (!strcmp(oglobals.file, "undef"))) {
		*oglobals.file = '\0';
	}

	if ((var_val = switch_event_get_header(var_event, "ignore_early_media"))) {
		if (switch_true(var_val)) {
			oglobals.early_ok = 0;
			oglobals.ignore_early_media = 1;
		} else if (!strcmp(var_val, "ring_ready")) {
			oglobals.early_ok = 0;
			oglobals.ignore_early_media = 2;
		}
	}

	if ((var_val = switch_event_get_header(var_event, "originate_continue_on_timeout")) && switch_true(var_val)) {
		oglobals.continue_on_timeout = 1;
	}

	if ((var_val = switch_event_get_header(var_event, "ignore_ring_ready")) && switch_true(var_val)) {
		oglobals.ignore_ring_ready = 1;
	}

	if ((var_val = switch_event_get_header(var_event, "monitor_early_media_ring"))) {
		oglobals.early_ok = 0;
		oglobals.monitor_early_media_ring = 1;
	}

	if ((var_val = switch_event_get_header(var_event, "monitor_early_media_fail"))) {
		oglobals.early_ok = 0;
		oglobals.monitor_early_media_fail = 1;
	}

	if ((var_val = switch_event_get_header(var_event, "return_ring_ready")) && switch_true(var_val)) {
		oglobals.return_ring_ready = 1;
	}

	if ((var_val = switch_event_get_header(var_event, "ring_ready")) && switch_true(var_val)) {
		oglobals.ring_ready = 1;
	}

	if ((var_val = switch_event_get_header(var_event, "instant_ringback")) && switch_true(var_val)) {
		oglobals.instant_ringback = 1;
	}

	if ((var_val = switch_event_get_header(var_event, "originate_timeout"))) {
		int tmp = atoi(var_val);
		if (tmp > 0) {
			timelimit_sec = (uint32_t) tmp;
		}
	}

	if ((var_val = switch_event_get_header(var_event, "progress_timeout"))) {
		int tmp = atoi(var_val);
		if (tmp > 0) {
			progress_timelimit_sec = (uint32_t) tmp;
		}
	}

	if ((var_val = switch_event_get_header(var_event, "originate_retries")) && switch_true(var_val)) {
		int32_t tmp;
		tmp = atoi(var_val);
		if (tmp > 0 && tmp < 101) {
			retries = tmp;
		} else {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING,
							  "Invalid originate_retries setting of %d ignored, value must be between 1 and 100\n", tmp);
		}
	}

	if ((var_val = switch_event_get_header(var_event, "originate_retry_sleep_ms")) && switch_true(var_val)) {
		int32_t tmp;
		tmp = atoi(var_val);
		if (tmp >= 500 && tmp <= 60000) {
			sleep_ms = tmp;
		} else {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING,
							  "Invalid originate_retry_sleep_ms setting of %d ignored, value must be between 500 and 60000\n", tmp);
		}
	}

	if ((cid_tmp = switch_event_get_header(var_event, "origination_caller_id_name"))) {
		cid_name_override = cid_tmp;
	}

	if (cid_name_override) {
		switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, "origination_caller_id_name", cid_name_override);
	} else {
		cid_name_override = switch_event_get_header(var_event, "origination_caller_id_name");
	}

	if ((cid_tmp = switch_event_get_header(var_event, "origination_caller_id_number"))) {
		cid_num_override = cid_tmp;
	}

	if (cid_num_override) {
		switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, "origination_caller_id_number", cid_num_override);
	} else {
		cid_num_override = switch_event_get_header(var_event, "origination_caller_id_number");
	}

	if (cid_num_override) {
		dftflags |= SOF_NO_EFFECTIVE_CID_NUM;
	}

	if (cid_name_override) {
		dftflags |= SOF_NO_EFFECTIVE_CID_NAME;
	}

	if (!progress_timelimit_sec) {
		progress_timelimit_sec = timelimit_sec;
	}

	for (try = 0; try < retries; try++) {
		switch_safe_free(loop_data);
		loop_data = strdup(data);
		switch_assert(loop_data);
		or_argc = switch_separate_string(loop_data, '|', pipe_names, (sizeof(pipe_names) / sizeof(pipe_names[0])));

		if ((flags & SOF_NOBLOCK) && or_argc > 1) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Only calling the first element in the list in this mode.\n");
			or_argc = 1;
		}
		
		for (r = 0; r < or_argc; r++) {
			char *p, *end = NULL;
			const char *var_begin, *var_end;
			oglobals.hups = 0;
			
			reason = SWITCH_CAUSE_NONE;
			memset(peer_names, 0, sizeof(peer_names));
			peer_session = NULL;
			memset(originate_status, 0, sizeof(originate_status));
			new_profile = NULL;
			new_session = NULL;
			chan_type = NULL;
			chan_data = NULL;
			peer_channel = NULL;
			start = 0;
			read_frame = NULL;
			pool = NULL;
			pass = 0;
			var = NULL;
			to = 0;
			oglobals.sent_ring = 0;
			oglobals.progress = 0;
			myflags = dftflags;
			
			if (try > 0) {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "Originate attempt %d/%d in %d ms\n", try + 1, retries, sleep_ms);
				if (caller_channel) {
					switch_ivr_sleep(oglobals.session, sleep_ms, SWITCH_TRUE, NULL);
					if (!switch_channel_ready(caller_channel)) {
						status = SWITCH_STATUS_FALSE;
						goto done;
					}
				} else {
					switch_yield(sleep_ms * 1000);
				}
			}
			
			p = pipe_names[r];
			while(p && *p) {
				if (*p == '[') {
					end = switch_find_end_paren(p, '[', ']');
				}

				if (end && p < end && *p == ',') {
					*p = '|';
				}

				if (p == end) {
					end = NULL;
				}

				p++;
			}
			
			and_argc = switch_separate_string(pipe_names[r], ',', peer_names, (sizeof(peer_names) / sizeof(peer_names[0])));

			if ((flags & SOF_NOBLOCK) && and_argc > 1) {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Only calling the first element in the list in this mode.\n");
				and_argc = 1;
			}

			for (i = 0; i < and_argc; i++) {
				char *vdata;
				const char *current_variable;
				char variable_buffer[512] = "";
				end = NULL;
				chan_type = peer_names[i];

				while (chan_type && *chan_type && *chan_type == ' ') {
					chan_type++;
				}

				vdata = chan_type;
				end = switch_find_end_paren(vdata, '[', ']');

				if (end) {
					vdata++;
					*end++ = '\0';
					chan_type = end;
				} else {
					vdata = NULL;
				}

				if ((chan_data = strchr(chan_type, '/')) != 0) {
					*chan_data = '\0';
					chan_data++;
				}

				if (oglobals.session) {
					if (!switch_channel_ready(caller_channel)) {
						status = SWITCH_STATUS_FALSE;
						goto done;
					}

					caller_caller_profile = caller_profile_override ? caller_profile_override : switch_channel_get_caller_profile(caller_channel);
					new_profile = switch_caller_profile_clone(oglobals.session, caller_caller_profile);
					new_profile->uuid = SWITCH_BLANK_STRING;
					new_profile->chan_name = SWITCH_BLANK_STRING;
					new_profile->destination_number = switch_core_strdup(new_profile->pool, chan_data);

					if (cid_name_override) {
						new_profile->caller_id_name = switch_core_strdup(new_profile->pool, cid_name_override);
					}
					if (cid_num_override) {
						new_profile->caller_id_number = switch_core_strdup(new_profile->pool, cid_num_override);
					}

					pool = NULL;
				} else {
					switch_core_new_memory_pool(&pool);

					if (caller_profile_override) {
						new_profile = switch_caller_profile_dup(pool, caller_profile_override);
						new_profile->destination_number = switch_core_strdup(new_profile->pool, switch_str_nil(chan_data));
						new_profile->uuid = SWITCH_BLANK_STRING;
						new_profile->chan_name = SWITCH_BLANK_STRING;
					} else {
						if (!cid_name_override) {
							cid_name_override = "FreeSWITCH";
						}
						if (!cid_num_override) {
							cid_num_override = "0000000000";
						}

						new_profile = switch_caller_profile_new(pool,
																NULL,
																NULL,
																cid_name_override, cid_num_override, NULL, NULL, NULL, NULL, __FILE__, NULL, chan_data);
					}
				}

				originate_status[i].caller_profile = NULL;
				originate_status[i].peer_channel = NULL;
				originate_status[i].peer_session = NULL;
				new_session = NULL;

				if (and_argc > 1 || or_argc > 1) {
					myflags |= SOF_FORKED_DIAL;
				} else if (var_event) {
					const char *vvar;
					if ((vvar = switch_event_get_header(var_event, "forked_dial")) && switch_true(vvar)) {
						myflags |= SOF_FORKED_DIAL;
					}
				}
				
				/* only valid in [] since it's unique to each channel leg */

				if (vdata && (var_begin = switch_stristr("origination_uuid=", vdata))) {
					char tmp[512] = "";
					var_begin += strlen("origination_uuid=");
					var_end = strchr(var_begin, '|');

					if (var_end) {
						strncpy(tmp, var_begin, var_end-var_begin);
					} else {
						strncpy(tmp, var_begin, strlen(var_begin));
					}

					switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, "origination_uuid", tmp);
				}

				/* The rest are valid in both {} and [] with [] taking precedence */

				current_variable = NULL;
				
				if (vdata && (var_begin = switch_stristr("origination_caller_id_number=", vdata))) {
					var_begin += strlen("origination_caller_id_number=");
					var_end = strchr(var_begin, '|');

					if (var_end) {
						strncpy(variable_buffer, var_begin, var_end-var_begin);
					} else {
						strncpy(variable_buffer, var_begin, strlen(var_begin));
					}

					current_variable = variable_buffer;
				}
				
				if (current_variable || (current_variable = switch_event_get_header(var_event, "origination_caller_id_number"))) {
					new_profile->caller_id_number = switch_core_strdup(new_profile->pool, current_variable);
					myflags |= SOF_NO_EFFECTIVE_CID_NUM;
				}

				current_variable = NULL;
				
				if (vdata && (var_begin = switch_stristr("origination_caller_id_name=", vdata))) {
					var_begin += strlen("origination_caller_id_name=");
					var_end = strchr(var_begin, '|');

					if (var_end) {
						strncpy(variable_buffer, var_begin, var_end-var_begin);
					} else {
						strncpy(variable_buffer, var_begin, strlen(var_begin));
					}

					current_variable = variable_buffer;
				}
				
				if (current_variable || (current_variable = switch_event_get_header(var_event, "origination_caller_id_name"))) {
					new_profile->caller_id_name = switch_core_strdup(new_profile->pool, current_variable);
					myflags |= SOF_NO_EFFECTIVE_CID_NAME;
				}
				
				current_variable = NULL;
				
				if (vdata && (var_begin = switch_stristr("origination_privacy=", vdata))) {
					var_begin += strlen("origination_privacy=");
					var_end = strchr(var_begin, '|');

					if (var_end) {
						strncpy(variable_buffer, var_begin, var_end-var_begin);
					} else {
						strncpy(variable_buffer, var_begin, strlen(var_begin));
					}

					current_variable = variable_buffer;
				}
				
				if (current_variable || (current_variable = switch_event_get_header(var_event, "origination_privacy"))) {
					new_profile->flags = SWITCH_CPF_NONE;
					
					if (switch_stristr("screen", current_variable)) {
						switch_set_flag(new_profile, SWITCH_CPF_SCREEN);
					}

					if (switch_stristr("hide_name", current_variable)) {
						switch_set_flag(new_profile, SWITCH_CPF_HIDE_NAME);
					}

					if (switch_stristr("hide_number", current_variable)) {
						switch_set_flag(new_profile, SWITCH_CPF_HIDE_NUMBER);
					}
				}

				current_variable = NULL;
				switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, "originate_early_media", oglobals.early_ok ? "true" : "false");

				if ((reason = switch_core_session_outgoing_channel(oglobals.session, var_event, chan_type, 
																   new_profile, &new_session, &pool, myflags)) != SWITCH_CAUSE_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Cannot create outgoing channel of type [%s] cause: [%s]\n", 
									  chan_type, switch_channel_cause2str(reason));
					
					if (pool) {
						switch_core_destroy_memory_pool(&pool);
					}
					continue;
				}

				if (switch_core_session_read_lock(new_session) != SWITCH_STATUS_SUCCESS) {
					status = SWITCH_STATUS_FALSE;
					goto done;
				}
				pool = NULL;

				originate_status[i].caller_profile = new_profile;
				originate_status[i].peer_session = new_session;
				originate_status[i].peer_channel = switch_core_session_get_channel(new_session);
				switch_channel_set_flag(originate_status[i].peer_channel, CF_ORIGINATING);

				if ((lc = switch_event_get_header(var_event, "local_var_clobber"))) {
					local_clobber = switch_true(lc);
				}

				if (local_clobber) {
					if (var_event) {
						switch_event_t *event;
						switch_event_header_t *header;
						/* install the vars from the {} params */
						for (header = var_event->headers; header; header = header->next) {
							switch_channel_set_variable(originate_status[i].peer_channel, header->name, header->value);
						}
						switch_event_create(&event, SWITCH_EVENT_CHANNEL_ORIGINATE);
						switch_assert(event);
						switch_channel_event_set_data(originate_status[i].peer_channel, event);
						switch_event_fire(&event);
					}
				}

				if (vdata) {
					char *var_array[1024] = { 0 };
					int var_count = 0;
					if ((var_count = switch_separate_string(vdata, '|', var_array, (sizeof(var_array) / sizeof(var_array[0]))))) {
						int x = 0;
						for (x = 0; x < var_count; x++) {
							char *inner_var_array[2] = { 0 };
							int inner_var_count;
							if ((inner_var_count =
								 switch_separate_string(var_array[x], '=',
														inner_var_array, (sizeof(inner_var_array) / sizeof(inner_var_array[0])))) == 2) {

								switch_channel_set_variable(originate_status[i].peer_channel, inner_var_array[0], inner_var_array[1]);
							}
						}
					}
				}

				if (!local_clobber) {
					if (var_event) {
						switch_event_t *event;
						switch_event_header_t *header;
						/* install the vars from the {} params */
						for (header = var_event->headers; header; header = header->next) {
							switch_channel_set_variable(originate_status[i].peer_channel, header->name, header->value);
						}
						switch_event_create(&event, SWITCH_EVENT_CHANNEL_ORIGINATE);
						switch_assert(event);
						switch_channel_event_set_data(originate_status[i].peer_channel, event);
						switch_event_fire(&event);
					}
				}

				if (originate_status[i].peer_channel) {
					const char *vvar;
					
					if ((vvar = switch_channel_get_variable(originate_status[i].peer_channel, "origination_callee_id_name"))) {
						switch_channel_set_profile_var(originate_status[i].peer_channel, "callee_id_name", vvar);
					}

					if ((vvar = switch_channel_get_variable(originate_status[i].peer_channel, "origination_callee_id_number"))) {
						switch_channel_set_profile_var(originate_status[i].peer_channel, "callee_id_number", vvar);
					}

					if ((vvar = switch_channel_get_variable(originate_status[i].peer_channel, "leg_timeout"))) {
						int val = atoi(vvar);
						if (val > 0) {
							switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s Setting leg timeout to %d\n", 
											  switch_channel_get_name(originate_status[i].peer_channel), val);
							originate_status[i].per_channel_timelimit_sec = (uint32_t) val;
						}
					}
					
					if ((vvar = switch_channel_get_variable(originate_status[i].peer_channel, "leg_progress_timeout"))) {
						int val = atoi(vvar);
						if (val > 0) {
							switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s Setting leg progress timeout to %d\n", 
											  switch_channel_get_name(originate_status[i].peer_channel), val);
							originate_status[i].per_channel_progress_timelimit_sec = (uint32_t) val;
						}
					}

					if ((vvar = switch_channel_get_variable(originate_status[i].peer_channel, "leg_delay_start"))) {
						int val = atoi(vvar);
						if (val > 0) {
							switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s Setting leg delay start to %d\n", 
											  switch_channel_get_name(originate_status[i].peer_channel), val);
							originate_status[i].per_channel_delay_start = (uint32_t) val;

							if (originate_status[i].per_channel_progress_timelimit_sec != 0) {
								originate_status[i].per_channel_progress_timelimit_sec += originate_status[i].per_channel_delay_start;
							}

							if (originate_status[i].per_channel_timelimit_sec != 0) {
								originate_status[i].per_channel_timelimit_sec += originate_status[i].per_channel_delay_start;
							}
						}
					}
				}

				if (table) {
					switch_channel_add_state_handler(originate_status[i].peer_channel, table);
				}

				switch_channel_add_state_handler(originate_status[i].peer_channel, &originate_state_handlers);

				if ((flags & SOF_NOBLOCK) && originate_status[i].peer_session) {
					status = SWITCH_STATUS_SUCCESS;
					*bleg = originate_status[i].peer_session;
					*cause = SWITCH_CAUSE_SUCCESS;
					goto outer_for;
				}

				if (!switch_core_session_running(originate_status[i].peer_session)) {
					if (originate_status[i].per_channel_delay_start) {
						switch_channel_set_flag(originate_status[i].peer_channel, CF_BLOCK_STATE);
					}
					switch_core_session_thread_launch(originate_status[i].peer_session);
				}
			}

			switch_epoch_time_now(&start);

			for (;;) {
				uint32_t valid_channels = 0;
				for (i = 0; i < and_argc; i++) {
					int state;
					time_t elapsed;

					if (!originate_status[i].peer_channel) {
						continue;
					}

					state = switch_channel_get_state(originate_status[i].peer_channel);

					if (state < CS_HANGUP) {
						valid_channels++;
					} else {
						continue;
					}

					if (state >= CS_ROUTING) {
						goto endfor1;
					}

					if (caller_channel && !switch_channel_ready(caller_channel)) {
						goto notready;
					}
					
					elapsed = switch_epoch_time_now(NULL) - start;
				
					if (elapsed > (time_t) timelimit_sec) {
						to++;
						oglobals.idx = IDX_TIMEOUT;
						goto notready;
					}

					if (!oglobals.sent_ring && !oglobals.ignore_ring_ready && 
						!oglobals.progress && (progress_timelimit_sec && elapsed > (time_t) progress_timelimit_sec)) {
						to++;
						oglobals.idx = IDX_TIMEOUT;
						goto notready;
					}
					
					switch_cond_next();
				}

				check_per_channel_timeouts(&oglobals, originate_status, and_argc, start);


				if (valid_channels == 0) {
					status = SWITCH_STATUS_GENERR;
					goto done;
				}

			}

		endfor1:

			if (caller_channel) {
				if (switch_channel_test_flag(caller_channel, CF_PROXY_MODE) || 
					switch_channel_test_flag(caller_channel, CF_PROXY_MEDIA) || switch_channel_test_flag(caller_channel, CF_DISABLE_RINGBACK)) {
					ringback_data = NULL;
				}
			}

			if (ringback_data && !switch_channel_test_flag(caller_channel, CF_ANSWERED)
				&& !switch_channel_test_flag(caller_channel, CF_EARLY_MEDIA)) {
				if ((status = switch_channel_pre_answer(caller_channel)) != SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_CHANNEL_LOG(caller_channel), SWITCH_LOG_DEBUG, "%s Media Establishment Failed.\n", switch_channel_get_name(caller_channel));
					goto done;
				}
			}

			if (oglobals.session && (read_codec = switch_core_session_get_read_codec(oglobals.session)) && ringback_data) {
				if (switch_is_file_path(ringback_data)) {
					if (!(strrchr(ringback_data, '.') || strstr(ringback_data, SWITCH_URL_SEPARATOR))) {
						ringback.asis++;
					}
				}
				
				if (!ringback.asis) {
					if ((pass = (uint8_t) switch_test_flag(read_codec, SWITCH_CODEC_FLAG_PASSTHROUGH))) {
						ringback_data = NULL;
						goto no_ringback;
					}

					if (switch_core_codec_init(&write_codec,
											   "L16",
											   NULL,
											   read_codec->implementation->actual_samples_per_second,
											   read_codec->implementation->microseconds_per_packet / 1000,
											   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE, NULL,
											   switch_core_session_get_pool(oglobals.session)) == SWITCH_STATUS_SUCCESS) {


						switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(oglobals.session), SWITCH_LOG_DEBUG,
										  "Raw Codec Activation Success L16@%uhz 1 channel %dms\n",
										  read_codec->implementation->actual_samples_per_second,
										  read_codec->implementation->microseconds_per_packet / 1000);
						write_frame.codec = &write_codec;
						write_frame.datalen = read_codec->implementation->decoded_bytes_per_packet;
						write_frame.samples = write_frame.datalen / 2;
						memset(write_frame.data, 255, write_frame.datalen);
						switch_core_session_set_read_codec(oglobals.session, &write_codec);
					} else {
						switch_log_printf(SWITCH_CHANNEL_CHANNEL_LOG(caller_channel), SWITCH_LOG_ERROR, "Codec Error!\n");
						switch_channel_hangup(caller_channel, SWITCH_CAUSE_NORMAL_TEMPORARY_FAILURE);
						read_codec = NULL;
						goto done;
					}
				}	

				if (ringback_data) {
					char *tmp_data = NULL;

					oglobals.gen_ringback = 1;

					if (switch_is_file_path(ringback_data)) {
						char *ext;

						if (ringback.asis) {
							write_frame.codec = read_codec;
							ext = read_codec->implementation->iananame;
							tmp_data = switch_mprintf("%s.%s", ringback_data, ext);
							ringback_data = tmp_data;
						}

						switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(oglobals.session), SWITCH_LOG_DEBUG, "Play Ringback File [%s]\n", ringback_data);

						ringback.fhb.channels = read_codec->implementation->number_of_channels;
						ringback.fhb.samplerate = read_codec->implementation->actual_samples_per_second;
						if (switch_core_file_open(&ringback.fhb,
												  ringback_data,
												  read_codec->implementation->number_of_channels,
												  read_codec->implementation->actual_samples_per_second,
												  SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT, NULL) != SWITCH_STATUS_SUCCESS) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Playing File\n");
							switch_safe_free(tmp_data);
							goto notready;
						}
						ringback.fh = &ringback.fhb;

					} else if (!strncasecmp(ringback_data, "silence", 7)) {
						const char *c = ringback_data + 7;
						if (*c == ':') {
							c++;
							if (c) {
								ringback.silence = atoi(c);
							}
						}
						if (ringback.silence <= 0) {
							ringback.silence = 400;
						}
					} else {
						switch_buffer_create_dynamic(&ringback.audio_buffer, 512, 1024, 0);
						switch_buffer_set_loops(ringback.audio_buffer, -1);

						teletone_init_session(&ringback.ts, 0, teletone_handler, &ringback);
						ringback.ts.rate = read_codec->implementation->actual_samples_per_second;
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Play Ringback Tone [%s]\n", ringback_data);
						/* ringback.ts.debug = 1;
						   ringback.ts.debug_stream = switch_core_get_console();
						*/
						if (teletone_run(&ringback.ts, ringback_data)) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Playing Tone\n");
							teletone_destroy_session(&ringback.ts);
							switch_buffer_destroy(&ringback.audio_buffer);
							ringback_data = NULL;
						}
					}
					switch_safe_free(tmp_data);
				}
			}
			
			
		no_ringback:

#if 0
			/* changing behaviour ignore_early_media=true must also be explicitly set for previous behaviour */
			if (ringback_data) {
				oglobals.early_ok = 0;
			}
#endif

			while ((!caller_channel || switch_channel_ready(caller_channel) || switch_channel_test_flag(caller_channel, CF_XFER_ZOMBIE)) && 
				   check_channel_status(&oglobals, originate_status, and_argc)) {
				time_t elapsed = switch_epoch_time_now(NULL) - start;
				
				check_per_channel_timeouts(&oglobals, originate_status, and_argc, start);

				if (oglobals.session) {
					switch_ivr_parse_all_events(oglobals.session);
				}

				if (!oglobals.sent_ring && !oglobals.progress && (progress_timelimit_sec && elapsed > (time_t) progress_timelimit_sec)) {
					oglobals.idx = IDX_TIMEOUT;
					goto notready;
				}

				if ((to = (uint8_t) (elapsed >= (time_t) timelimit_sec)) || (fail_on_single_reject && oglobals.hups)) {
					int ok = 0;
					
					if (fail_on_single_reject_var && !switch_true(fail_on_single_reject_var)) {
						ok = 1;
						for (i = 0; i < and_argc; i++) {
							switch_channel_t *pchannel;
							const char *cause_str;
							
							if (!originate_status[i].peer_session) {
								continue;
							}
							pchannel = switch_core_session_get_channel(originate_status[i].peer_session);

							if (switch_channel_down(pchannel)) {
								cause_str = switch_channel_cause2str(switch_channel_get_cause(pchannel));
								if (switch_stristr(cause_str, fail_on_single_reject_var)) {
									ok = 0;
									break;
								}
							}
						}
					}
					if (!ok) {
						oglobals.idx = IDX_TIMEOUT;
						goto notready;
					}
				}
				
				/* read from the channel while we wait if the audio is up on it */
				if (oglobals.session &&
					!switch_channel_test_flag(caller_channel, CF_PROXY_MODE) &&
					!switch_channel_test_flag(caller_channel, CF_PROXY_MEDIA) &&
					!switch_channel_test_flag(caller_channel, CF_XFER_ZOMBIE) &&
					(ringback_data
					 || (switch_channel_test_flag(caller_channel, CF_ANSWERED) || switch_channel_test_flag(caller_channel, CF_EARLY_MEDIA)))) {
					
					switch_status_t tstatus = SWITCH_STATUS_SUCCESS;
					int silence = 0;

					if (caller_channel && cancel_key) {
						if (switch_channel_has_dtmf(caller_channel)) {
							switch_dtmf_t dtmf = { 0, 0 };
							if (switch_channel_dequeue_dtmf(caller_channel, &dtmf) == SWITCH_STATUS_SUCCESS) {
								if (dtmf.digit == *cancel_key) {
									oglobals.idx = IDX_KEY_CANCEL;
									goto notready;
								}
							}
						}
					}

					if (switch_channel_media_ready(caller_channel)) {
						tstatus = switch_core_session_read_frame(oglobals.session, &read_frame, SWITCH_IO_FLAG_NONE, 0);
						if (!SWITCH_READ_ACCEPTABLE(tstatus)) {
							if (switch_channel_test_flag(caller_channel, CF_XFER_ZOMBIE)) {
								continue;
							}
							break;
						}
					} else {
						read_frame = NULL;
					}
					
					if ((oglobals.ring_ready || oglobals.instant_ringback) && read_frame && !pass) {
						if (ringback.fh) {
							switch_size_t mlen, olen;
							unsigned int pos = 0;

							if (ringback.asis) {
								mlen = write_frame.codec->implementation->encoded_bytes_per_packet;
							} else {
								mlen = write_frame.codec->implementation->samples_per_packet;
							}

							olen = mlen;
							
							//if (ringback.fh->resampler && ringback.fh->resampler->rfactor > 1) {
							//olen = (switch_size_t) (olen * ringback.fh->resampler->rfactor);
							//}
							
							switch_core_file_read(ringback.fh, write_frame.data, &olen);

							if (olen == 0) {
								olen = mlen;
								ringback.fh->speed = 0;
								switch_core_file_seek(ringback.fh, &pos, 0, SEEK_SET);
								switch_core_file_read(ringback.fh, write_frame.data, &olen);
								if (olen == 0) {
									break;
								}
							}
							write_frame.datalen = (uint32_t) (ringback.asis ? olen : olen * 2);
						} else if (ringback.audio_buffer) {
							if ((write_frame.datalen = (uint32_t) switch_buffer_read_loop(ringback.audio_buffer,
																						  write_frame.data,
																						  write_frame.codec->implementation->decoded_bytes_per_packet)) <= 0) {
								break;
							}
						} else if (ringback.silence) {
							silence = ringback.silence;
						}
					} else {
						silence = 600;
					}
					
					if ((ringback.fh || silence || ringback.audio_buffer) && write_frame.codec && write_frame.datalen) {
						if (silence) {
							write_frame.datalen = write_frame.codec->implementation->decoded_bytes_per_packet;
							switch_generate_sln_silence((int16_t *) write_frame.data, write_frame.datalen / 2, silence);
						}
						
						if (switch_core_session_write_frame(oglobals.session, &write_frame, SWITCH_IO_FLAG_NONE, 0) != SWITCH_STATUS_SUCCESS) {
							if (switch_channel_test_flag(caller_channel, CF_XFER_ZOMBIE)) {
								continue;
							}
							break;
						}
					}
					
				} else {
					switch_cond_next();
				}
				
			}

		  notready:

			if (caller_channel) {
				const char *soft_holding = switch_channel_get_variable(caller_channel, SWITCH_SOFT_HOLDING_UUID_VARIABLE);
				holding = switch_channel_get_variable(caller_channel, SWITCH_HOLDING_UUID_VARIABLE);
				switch_channel_set_variable(caller_channel, SWITCH_HOLDING_UUID_VARIABLE, NULL);

				if (soft_holding && switch_channel_test_flag(caller_channel, CF_XFER_ZOMBIE)) {
					holding = soft_holding;
					switch_channel_set_variable(caller_channel, SWITCH_SOFT_HOLDING_UUID_VARIABLE, NULL);
				}
			}
			
			if (caller_channel && !switch_channel_ready(caller_channel) && !switch_channel_test_flag(caller_channel, CF_XFER_ZOMBIE)) {
				oglobals.idx = IDX_CANCEL;
			}

			if (oglobals.session && (ringback_data || !(switch_channel_test_flag(caller_channel, CF_PROXY_MODE) &&
											   switch_channel_test_flag(caller_channel, CF_PROXY_MEDIA)))) {
				switch_core_session_reset(oglobals.session, SWITCH_FALSE, SWITCH_TRUE);
			}

			if (holding) {
				if (oglobals.idx > IDX_NADA) {
					peer_session = originate_status[oglobals.idx].peer_session;
					peer_channel = originate_status[oglobals.idx].peer_channel;
					originate_status[oglobals.idx].peer_channel = NULL;
				} else if (and_argc == 1) {
					peer_session = originate_status[0].peer_session;
					peer_channel = originate_status[0].peer_channel;
					originate_status[0].peer_channel = NULL;
				} else {
					for (i = 0; i < and_argc; i++) {
						if (!peer_eligible(originate_status[i].peer_channel)) {
							continue;
						}
						if (switch_channel_media_ready(originate_status[i].peer_channel)) {
							peer_session = originate_status[i].peer_session;
							peer_channel = originate_status[i].peer_channel;
							originate_status[i].peer_channel = NULL;
							goto end_search;
						}
					}
					for (i = 0; i < and_argc; i++) {
						if (!peer_eligible(originate_status[i].peer_channel)) {
							continue;
						}
						if (switch_channel_up(originate_status[i].peer_channel)) {
							peer_session = originate_status[i].peer_session;
							peer_channel = originate_status[i].peer_channel;
							originate_status[i].peer_channel = NULL;
							break;
						}
					}
				}
				
			end_search:

				if (peer_channel && switch_channel_down(peer_channel)) {
					switch_core_session_rwunlock(peer_session);
					peer_session = NULL;
					peer_channel = NULL;

				}
				
				if (oglobals.idx == IDX_TIMEOUT || to || oglobals.idx == IDX_KEY_CANCEL || oglobals.idx == IDX_CANCEL || 
					(!peer_session && oglobals.idx == IDX_NADA)) {
					const char *dest = NULL;
					const char *context = NULL;
					const char *dialplan = NULL;
					switch_core_session_t *holding_session;
					
					if (caller_channel) {
						if (zstr(context)) {
							context = switch_channel_get_variable(caller_channel, "context"); 
						}
						if (zstr(dialplan)) {
							dialplan = switch_channel_get_variable(caller_channel, "dialplan"); 
						}
					}
					
					if (zstr(context)) {
						context = "default";
					}

					if (zstr(context)) {
						dialplan = "XML";
					}

					if ((holding_session = switch_core_session_locate(holding))) {
						switch_channel_t *holding_channel = switch_core_session_get_channel(holding_session);
						switch_status_t mstatus = SWITCH_STATUS_FALSE;
						
						if (caller_channel) {
							if ((mstatus = switch_channel_caller_extension_masquerade(caller_channel, holding_channel, 1)) == SWITCH_STATUS_SUCCESS) {
								switch_channel_restart(holding_channel);
							}
						}

						if (mstatus != SWITCH_STATUS_SUCCESS) {
							if (peer_channel) {
								dest = switch_channel_get_variable(peer_channel, "destination_number");
								context = switch_channel_get_variable(peer_channel, "context");
								dialplan = switch_channel_get_variable(peer_channel, "dialplan");
							} else if (caller_channel) {
								dest = switch_channel_get_variable(caller_channel, "destination_number");
							}
							if (dest) {
								switch_ivr_session_transfer(holding_session, dest, dialplan, context);
							}
						}

						switch_core_session_rwunlock(holding_session);
						holding = NULL;
						holding_session = NULL;
					}

					if (peer_channel) {
						switch_channel_hangup(peer_channel, SWITCH_CAUSE_ATTENDED_TRANSFER);
						switch_core_session_rwunlock(peer_session);
					}
					force_reason = SWITCH_CAUSE_ATTENDED_TRANSFER;
				} else {
					if (peer_channel && switch_channel_ready(peer_channel)) {
						force_reason = SWITCH_CAUSE_ATTENDED_TRANSFER;
						switch_ivr_uuid_bridge(holding, switch_core_session_get_uuid(peer_session));
						oglobals.idx = IDX_NADA;
						if (caller_channel && switch_channel_up(caller_channel)) {
							switch_channel_hangup(caller_channel, SWITCH_CAUSE_ATTENDED_TRANSFER);
						}
						caller_channel = NULL;
						oglobals.session = NULL;
						session = NULL;
						switch_core_session_rwunlock(peer_session);
					} else {
						switch_core_session_t *holding_session;

						if ((holding_session = switch_core_session_locate(holding))) {
							switch_channel_t *holding_channel = switch_core_session_get_channel(holding_session);
							if (caller_channel && switch_channel_ready(caller_channel)) {
								switch_ivr_uuid_bridge(holding, switch_core_session_get_uuid(session));
							} else {
								switch_channel_hangup(holding_channel, SWITCH_CAUSE_NORMAL_UNSPECIFIED);
							}
							switch_core_session_rwunlock(holding_session);
						}
					}
				}

				peer_session = NULL;
				peer_channel = NULL;
				
			}
			
			for (i = 0; i < and_argc; i++) {
				if (!peer_eligible(originate_status[i].peer_channel)) {
					continue;
				}
				
				if (i != oglobals.idx) {
					holding = NULL;

					if (oglobals.idx == IDX_TIMEOUT || to) {
						reason = SWITCH_CAUSE_NO_ANSWER;
					} else {
						if (oglobals.idx == IDX_CANCEL) {
							reason = SWITCH_CAUSE_ORIGINATOR_CANCEL;
						} else {
							if (and_argc > 1) {
								reason = SWITCH_CAUSE_LOSE_RACE;
							} else {
								reason = SWITCH_CAUSE_NO_ANSWER;
							}
						}
					}
					if (switch_channel_up(originate_status[i].peer_channel)) {
						if (caller_channel && i == 0) {
                            holding = switch_channel_get_variable(caller_channel, SWITCH_SOFT_HOLDING_UUID_VARIABLE);
                            switch_channel_set_variable(caller_channel, SWITCH_SOFT_HOLDING_UUID_VARIABLE, NULL);
                        }
						
						if (holding && oglobals.idx != IDX_TIMEOUT && oglobals.idx != IDX_KEY_CANCEL) {
                            switch_ivr_uuid_bridge(holding, switch_core_session_get_uuid(originate_status[i].peer_session));
                        } else {
							switch_channel_hangup(originate_status[i].peer_channel, reason);
						}
					}
				}
			}
			
			if (oglobals.idx > IDX_NADA) {
				peer_session = originate_status[oglobals.idx].peer_session;
				peer_channel = originate_status[oglobals.idx].peer_channel;
			} else {
				status = SWITCH_STATUS_FALSE;
				if (caller_channel && peer_channel) {
					switch_process_import(oglobals.session, peer_channel, "import");
				}
				peer_channel = NULL;
				goto done;
			}

			if (caller_channel) {
				if (switch_channel_test_flag(peer_channel, CF_ANSWERED)) {
					switch_channel_pass_callee_id(peer_channel, caller_channel);
					status = switch_channel_answer(caller_channel);
				} else if (switch_channel_test_flag(peer_channel, CF_EARLY_MEDIA)) {
					status = switch_channel_pre_answer(caller_channel);
				} else {
					status = SWITCH_STATUS_SUCCESS;
				}

				if (status != SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_CHANNEL_LOG(peer_channel), SWITCH_LOG_DEBUG, "%s Media Establishment Failed.\n", switch_channel_get_name(caller_channel));
					switch_channel_hangup(peer_channel, SWITCH_CAUSE_INCOMPATIBLE_DESTINATION);
				}
			}

			if (switch_channel_test_flag(peer_channel, CF_ANSWERED) ||
				(oglobals.early_ok && switch_channel_test_flag(peer_channel, CF_EARLY_MEDIA)) ||
				(oglobals.return_ring_ready && switch_channel_test_flag(peer_channel, CF_RING_READY))
				) {
				*bleg = peer_session;

				if (oglobals.monitor_early_media_ring || oglobals.monitor_early_media_fail) {
					switch_ivr_stop_tone_detect_session(peer_session);
					switch_channel_set_private(peer_channel, "_oglobals_", NULL);
				}

				status = SWITCH_STATUS_SUCCESS;
			} else {
				status = SWITCH_STATUS_FALSE;
			}

		  done:
		
			*cause = SWITCH_CAUSE_NONE;

			if (caller_channel && !switch_channel_ready(caller_channel)) {
				status = SWITCH_STATUS_FALSE;
			}

			if (status == SWITCH_STATUS_SUCCESS) {
				if (caller_channel) {
					switch_channel_set_variable(caller_channel, "originate_disposition", "call accepted");
					if (peer_channel) {
						switch_process_import(oglobals.session, peer_channel, "import");
					}
				}
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(oglobals.session), SWITCH_LOG_DEBUG, "Originate Resulted in Success: [%s]\n", switch_channel_get_name(peer_channel));
				*cause = SWITCH_CAUSE_SUCCESS;

			} else {
				const char *cdr_var = NULL;
				switch_xml_t cdr;
				char *xml_text;
				char buf[128] = "", buf2[128] = "";

				if (caller_channel) {
					cdr_var = switch_channel_get_variable(caller_channel, "failed_xml_cdr_prefix");
				}

				if (peer_channel) {
					*cause = switch_channel_get_cause(peer_channel);					
				} else {
					for (i = 0; i < and_argc; i++) {
						if (!originate_status[i].peer_channel) {
							continue;
						}
						*cause = switch_channel_get_cause(originate_status[i].peer_channel);
						break;
					}
				}
				
				if (cdr_var) {
					for (i = 0; i < and_argc; i++) {
						switch_channel_t *channel;

						if (!originate_status[i].peer_session) {
                            continue;
                        }

						channel = switch_core_session_get_channel(originate_status[i].peer_session);
						
						switch_channel_wait_for_state_timeout(channel, CS_REPORTING, 5000);
						
						if (!switch_channel_test_flag(channel, CF_TIMESTAMP_SET)) {
							switch_channel_set_timestamps(channel);
						}
						
						if (switch_ivr_generate_xml_cdr(originate_status[i].peer_session, &cdr) == SWITCH_STATUS_SUCCESS) {
							if ((xml_text = switch_xml_toxml(cdr, SWITCH_FALSE))) {
								switch_snprintf(buf, sizeof(buf), "%s_%d", cdr_var, ++cdr_total);
								switch_channel_set_variable(caller_channel, buf, xml_text);
								switch_safe_free(xml_text);
							}
							switch_xml_free(cdr);
							cdr = NULL;
						}

					}
					switch_snprintf(buf, sizeof(buf), "%s_total", cdr_var);
					switch_snprintf(buf2, sizeof(buf2), "%d", cdr_total ? cdr_total : 0);
					switch_channel_set_variable(caller_channel, buf, buf2);
				}

				if (!*cause) {
					if (reason) {
						*cause = reason;
					} else if (caller_channel) {
						*cause = switch_channel_get_cause(caller_channel);
					} else {
						*cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
					}
				}

				if (*cause == SWITCH_CAUSE_SUCCESS || *cause == SWITCH_CAUSE_NONE) {
					*cause = SWITCH_CAUSE_ORIGINATOR_CANCEL;
				}

				if (oglobals.idx == IDX_CANCEL) {
					*cause = SWITCH_CAUSE_ORIGINATOR_CANCEL;
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(oglobals.session), SWITCH_LOG_DEBUG,
									  "Originate Cancelled by originator termination Cause: %d [%s]\n", *cause, switch_channel_cause2str(*cause));

				} else if (oglobals.idx == IDX_TIMEOUT) {
					*cause = SWITCH_CAUSE_NO_ANSWER;
				} else {
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(oglobals.session), SWITCH_LOG_DEBUG,
									  "Originate Resulted in Error Cause: %d [%s]\n", *cause, switch_channel_cause2str(*cause));
				}
			}

			if (caller_channel) {
				switch_channel_set_variable(caller_channel, "originate_disposition", switch_channel_cause2str(*cause));
			}

			if (ringback.fh) {
				switch_core_file_close(ringback.fh);
				ringback.fh = NULL;
			} else if (ringback.audio_buffer) {
				teletone_destroy_session(&ringback.ts);
				switch_buffer_destroy(&ringback.audio_buffer);
			}

			if (oglobals.session) {
				switch_core_session_reset(oglobals.session, SWITCH_FALSE, SWITCH_TRUE);
			}

			if (switch_core_codec_ready(&write_codec)) {
				switch_core_codec_destroy(&write_codec);
			}

			for (i = 0; i < and_argc; i++) {
				switch_channel_state_t state;

				if (!originate_status[i].peer_channel) {
					continue;
				}
				
				if (status == SWITCH_STATUS_SUCCESS) { 
					switch_channel_clear_flag(originate_status[i].peer_channel, CF_ORIGINATING);
					if (bleg && *bleg && *bleg == originate_status[i].peer_session) {
						continue;
					}
				} else if ((state=switch_channel_get_state(originate_status[i].peer_channel)) < CS_HANGUP && 
						   switch_channel_test_flag(originate_status[i].peer_channel, CF_ORIGINATING)) {
					if (!(state == CS_RESET || switch_channel_test_flag(originate_status[i].peer_channel, CF_TRANSFER) || 
						  switch_channel_test_flag(originate_status[i].peer_channel, CF_REDIRECT) ||
						  switch_channel_test_flag(originate_status[i].peer_channel, CF_BRIDGED))) {
						switch_channel_hangup(originate_status[i].peer_channel, *cause);
					}
				}
				switch_channel_clear_flag(originate_status[i].peer_channel, CF_ORIGINATING);

				switch_core_session_rwunlock(originate_status[i].peer_session);
			}

			if (status == SWITCH_STATUS_SUCCESS) {
				goto outer_for;
			} else {
				int ok = 0;

				if (fail_on_single_reject_var && !switch_true(fail_on_single_reject_var)) {
					ok = 1;
					for (i = 0; i < and_argc; i++) {
						switch_channel_t *pchannel;
						const char *cause_str;
						
						if (!originate_status[i].peer_session) {
							continue;
						}
						pchannel = switch_core_session_get_channel(originate_status[i].peer_session);

						if (switch_channel_down(pchannel)) {
							cause_str = switch_channel_cause2str(switch_channel_get_cause(pchannel));
							if (switch_stristr(cause_str, fail_on_single_reject_var)) {
								ok = 0;
								break;
							}
						}
					}
				}
				if (ok) {
					goto outer_for;
				}

				if (to && !oglobals.continue_on_timeout) {
					goto outer_for;
				}
			}
		}
	}
  outer_for:
	switch_safe_free(loop_data);
	switch_safe_free(odata);

	if (bleg && status != SWITCH_STATUS_SUCCESS) {
		*bleg = NULL;
	}

	if (*bleg) {
		if (session && caller_channel && !switch_channel_get_originatee_caller_profile(caller_channel)) {
			switch_caller_profile_t *cloned_profile, *peer_profile = switch_channel_get_caller_profile(switch_core_session_get_channel(*bleg));
			
			if (peer_profile) {
				if ((cloned_profile = switch_caller_profile_clone(session, peer_profile)) != 0) {
					switch_channel_set_originatee_caller_profile(caller_channel, cloned_profile);
				}
			}
		}

		switch_ivr_sleep(*bleg, 0, SWITCH_TRUE, NULL);
	}

	if (oglobals.session && status == SWITCH_STATUS_SUCCESS) {
		switch_ivr_sleep(oglobals.session, 0, SWITCH_TRUE, NULL);
	}

	if (var_event && var_event != ovars) {
		switch_event_destroy(&var_event);
	}

	switch_safe_free(write_frame.data);
	switch_safe_free(fail_on_single_reject_var);
	
	if (caller_channel) {
		switch_channel_clear_flag(caller_channel, CF_ORIGINATOR);
		switch_channel_clear_flag(caller_channel, CF_XFER_ZOMBIE);
	}

	if (force_reason != SWITCH_CAUSE_NONE) {
		*cause = force_reason;
	}

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
