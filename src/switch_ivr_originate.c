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
 * switch_ivr_originate.c -- IVR Library (originate)
 *
 */
#include <switch.h>

static const switch_state_handler_table_t originate_state_handlers;

static switch_status_t originate_on_ring(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	/* put the channel in a passive state so we can loop audio to it */

	/* clear this handler so it only works once (next time (a.k.a. Transfer) we will do the real ring state) */
	switch_channel_clear_state_handler(channel, &originate_state_handlers);

	switch_channel_set_state(channel, CS_HOLD);

	return SWITCH_STATUS_FALSE;
}

static const switch_state_handler_table_t originate_state_handlers = {
	/*.on_init */ NULL,
	/*.on_ring */ originate_on_ring,
	/*.on_execute */ NULL,
	/*.on_hangup */ NULL,
	/*.on_loopback */ NULL,
	/*.on_transmit */ NULL,
	/*.on_hold */ NULL
};


typedef enum {
	IDX_CANCEL = -2,
	IDX_NADA = -1
} abort_t;

struct key_collect {
	char *key;
	char *file;
	switch_core_session_t *session;
};

static void *SWITCH_THREAD_FUNC collect_thread_run(switch_thread_t * thread, void *obj)
{
	struct key_collect *collect = (struct key_collect *) obj;
	switch_channel_t *channel = switch_core_session_get_channel(collect->session);
	char buf[10] = "";
	char *p, term;


	if (!strcasecmp(collect->key, "exec")) {
		char *data;
		const switch_application_interface_t *application_interface;
		char *app_name, *app_data;

		if (!(data = collect->file)) {
			goto wbreak;
		}

		app_name = data;

		if ((app_data = strchr(app_name, ' '))) {
			*app_data++ = '\0';
		}

		if ((application_interface = switch_loadable_module_get_application_interface(app_name)) == 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Application %s\n", app_name);
			switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			goto wbreak;
		}

		if (!application_interface->application_function) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No Function for %s\n", app_name);
			switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			goto wbreak;
		}

		switch_core_session_exec(collect->session, application_interface, app_data);
		
		if (switch_channel_get_state(channel) < CS_HANGUP) {
			switch_channel_set_flag(channel, CF_WINNER);
		}
		goto wbreak;
	}

	if (!switch_channel_ready(channel)) {
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
			switch_ivr_collect_digits_count(collect->session, buf, sizeof(buf), 1, "", &term, 0);
		}

		for (p = buf; *p; p++) {
			if (*collect->key == *p) {
				switch_channel_set_flag(channel, CF_WINNER);
				goto wbreak;
			}
		}
	}
  wbreak:

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

static uint8_t check_channel_status(switch_channel_t **peer_channels,
									switch_core_session_t **peer_sessions,
									uint32_t len, int32_t *idx, uint32_t * hups, char *file, char *key, uint8_t early_ok, uint8_t *ring_ready)
{

	uint32_t i;
	*hups = 0;
	*idx = IDX_NADA;

	for (i = 0; i < len; i++) {
		if (!peer_channels[i]) {
			continue;
		}
		if (!*ring_ready && switch_channel_test_flag(peer_channels[i], CF_RING_READY)) {
			*ring_ready = 1;
		}
		if (switch_channel_get_state(peer_channels[i]) >= CS_HANGUP) {
			(*hups)++;
		} else if ((switch_channel_test_flag(peer_channels[i], CF_ANSWERED) ||
					(early_ok && len == 1 && switch_channel_test_flag(peer_channels[i], CF_EARLY_MEDIA))) &&
				   !switch_channel_test_flag(peer_channels[i], CF_TAGGED)) {

			if (!switch_strlen_zero(key)) {
				struct key_collect *collect;

				if ((collect = switch_core_session_alloc(peer_sessions[i], sizeof(*collect)))) {
					switch_channel_set_flag(peer_channels[i], CF_TAGGED);
					collect->key = key;
					if (!switch_strlen_zero(file)) {
						collect->file = switch_core_session_strdup(peer_sessions[i], file);
					}

					collect->session = peer_sessions[i];
					launch_collect_thread(collect);
				}
			} else {
				*idx = i;
				return 0;

			}
		} else if (switch_channel_test_flag(peer_channels[i], CF_WINNER)) {
			*idx = i;
			return 0;
		}
	}

	if (*hups == len) {
		return 0;
	} else {
		return 1;
	}

}

struct ringback {
	switch_buffer_t *audio_buffer;
	teletone_generation_session_t ts;
	switch_file_handle_t fhb;
	switch_file_handle_t *fh;
	uint8_t asis;
};

typedef struct ringback ringback_t;

static int teletone_handler(teletone_generation_session_t * ts, teletone_tone_map_t * map)
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

#define MAX_PEERS 256
SWITCH_DECLARE(switch_status_t) switch_ivr_originate(switch_core_session_t *session,
													 switch_core_session_t **bleg,
													 switch_call_cause_t *cause,
													 char *bridgeto,
													 uint32_t timelimit_sec,
													 const switch_state_handler_table_t *table,
													 char *cid_name_override, char *cid_num_override, switch_caller_profile_t *caller_profile_override)
{
	char *pipe_names[MAX_PEERS] = { 0 };
	char *data = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_channel_t *caller_channel = NULL;
	char *peer_names[MAX_PEERS] = { 0 };
	switch_core_session_t *new_session = NULL, *peer_session, *peer_sessions[MAX_PEERS] = { 0 };
	switch_caller_profile_t *new_profile = NULL, *caller_profiles[MAX_PEERS] = { 0 }, *caller_caller_profile;
	char *chan_type = NULL, *chan_data;
	switch_channel_t *peer_channel = NULL, *peer_channels[MAX_PEERS] = { 0 };
	ringback_t ringback = { 0 };
	time_t start;
	switch_frame_t *read_frame = NULL;
	switch_memory_pool_t *pool = NULL;
	int r = 0, i, and_argc = 0, or_argc = 0;
	int32_t sleep_ms = 1000, try = 0, retries = 1, idx = IDX_NADA;
	switch_codec_t write_codec = { 0 };
	switch_frame_t write_frame = { 0 };
	uint8_t fdata[1024], pass = 0;
	char key[80] = "", file[512] = "", *odata, *var;
	switch_call_cause_t reason = SWITCH_CAUSE_UNALLOCATED;
	uint8_t to = 0;
	char *var_val, *vars = NULL, *ringback_data = NULL;
	switch_codec_t *read_codec = NULL;
	uint8_t sent_ring = 0, early_ok = 1;
	switch_core_session_message_t *message = NULL;
	switch_event_t *var_event = NULL;
	uint8_t fail_on_single_reject = 0;
	uint8_t ring_ready = 0;
	char *loop_data = NULL;

	write_frame.data = fdata;

	*bleg = NULL;
	odata = strdup(bridgeto);
	data = odata;

	/* strip leading spaces */
	while (data && *data && *data == ' ') {
		data++;
	}

	if (*data == '{') {
		vars = data + 1;
		if (!(data = strchr(data, '}'))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Parse Error!\n");
			status = SWITCH_STATUS_GENERR;
			goto done;
		}
		*data++ = '\0';
	}

	/* strip leading spaces (again) */
	while (data && *data && *data == ' ') {
		data++;
	}

	if (switch_strlen_zero(data)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Parse Error!\n");
		status = SWITCH_STATUS_GENERR;
		goto done;
	}

	/* Some channel are created from an originating channel and some aren't so not all outgoing calls have a way to get params
	   so we will normalize dialstring params and channel variables (when there is an originator) into an event that we 
	   will use as a pseudo hash to consult for params as needed.
	 */
	if (switch_event_create(&var_event, SWITCH_EVENT_MESSAGE) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Memory Error!\n");
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}

	if (session) {
		switch_hash_index_t *hi;
		void *vval;
		const void *vvar;

		caller_channel = switch_core_session_get_channel(session);
		assert(caller_channel != NULL);

		/* Copy all the channel variables into the event */
		if ((hi = switch_channel_variable_first(caller_channel))) {
			for (; hi; hi = switch_hash_next(hi)) {
				switch_hash_this(hi, &vvar, NULL, &vval);
				if (vvar && vval) {
					switch_event_add_header(var_event, SWITCH_STACK_BOTTOM, (void *) vvar, "%s", (char *) vval);
				}
			}
			switch_channel_variable_last(caller_channel);
		}

	}

	if (vars) {					/* Parse parameters specified from the dialstring */
		char *var_array[1024] = { 0 };
		int var_count = 0;
		if ((var_count = switch_separate_string(vars, ',', var_array, (sizeof(var_array) / sizeof(var_array[0]))))) {
			int x = 0;
			for (x = 0; x < var_count; x++) {
				char *inner_var_array[2];
				int inner_var_count;
				if ((inner_var_count =
					 switch_separate_string(var_array[x], '=', inner_var_array, (sizeof(inner_var_array) / sizeof(inner_var_array[0])))) == 2) {

					switch_event_add_header(var_event, SWITCH_STACK_BOTTOM, inner_var_array[0], "%s", inner_var_array[1]);
					if (caller_channel) {
						switch_channel_set_variable(caller_channel, inner_var_array[0], inner_var_array[1]);
					}
				}
			}
		}
	}


	if (caller_channel) {		/* ringback is only useful when there is an originator */
		ringback_data = switch_channel_get_variable(caller_channel, "ringback");
		switch_channel_set_variable(caller_channel, "originate_disposition", "failure");
	}

	if ((var = switch_event_get_header(var_event, "group_confirm_key"))) {
		switch_copy_string(key, var, sizeof(key));
		if ((var = switch_event_get_header(var_event, "group_confirm_file"))) {
			switch_copy_string(file, var, sizeof(file));
		}
	}
	// When using the AND operator, the fail_on_single_reject flag may be set in order to indicate that a single
	// rejections should terminate the attempt rather than a timeout, answer, or rejection by all.
	if ((var = switch_event_get_header(var_event, "fail_on_single_reject")) && switch_true(var)) {
		fail_on_single_reject = 1;
	}

	if ((*file != '\0') && (!strcmp(file, "undef"))) {
		*file = '\0';
	}

	if ((var_val = switch_event_get_header(var_event, "ignore_early_media")) && switch_true(var_val)) {
		early_ok = 0;
	}

	if ((var_val = switch_event_get_header(var_event, "originate_retries")) && switch_true(var_val)) {
		int32_t tmp;
		tmp = atoi(var_val);
		if (tmp > 0 && tmp < 101) {
			retries = tmp;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
							  "Invalid originate_retries setting of %d ignored, value must be between 1 and 100\n", tmp);
		}
	}

	if ((var_val = switch_event_get_header(var_event, "originate_retry_sleep_ms")) && switch_true(var_val)) {
		int32_t tmp;
		tmp = atoi(var_val);
		if (tmp > 500 && tmp < 60000) {
			sleep_ms = tmp;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
							  "Invalid originate_retry_sleep_ms setting of %d ignored, value must be between 500 and 60000\n", tmp);
		}
	}

	if (!cid_name_override) {
		cid_name_override = switch_event_get_header(var_event, "origination_caller_id_name");
	}

	if (!cid_num_override) {
		cid_num_override = switch_event_get_header(var_event, "origination_caller_id_number");
	}



	for (try = 0; try < retries; try++) {
		switch_safe_free(loop_data);
		assert(loop_data = strdup(data));
		or_argc = switch_separate_string(loop_data, '|', pipe_names, (sizeof(pipe_names) / sizeof(pipe_names[0])));

		if (caller_channel && or_argc > 1 && !ringback_data) {
			switch_channel_ring_ready(caller_channel);
			sent_ring = 1;
		}

		for (r = 0; r < or_argc; r++) {
			uint32_t hups;
			reason = SWITCH_CAUSE_UNALLOCATED;
			memset(peer_names, 0, sizeof(peer_names));
			peer_session = NULL;
			memset(peer_sessions, 0, sizeof(peer_sessions));
			memset(peer_channels, 0, sizeof(peer_channels));
			memset(caller_profiles, 0, sizeof(caller_profiles));
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

			if (try > 0) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Originate attempt %d/%d in %d ms\n", try + 1, retries, sleep_ms);
				switch_yield(sleep_ms * 1000);
			}

			and_argc = switch_separate_string(pipe_names[r], ',', peer_names, (sizeof(peer_names) / sizeof(peer_names[0])));

			if (caller_channel && !sent_ring && and_argc > 1 && !ringback_data) {
				switch_channel_ring_ready(caller_channel);
				sent_ring = 1;
			}

			for (i = 0; i < and_argc; i++) {

				chan_type = peer_names[i];
				if ((chan_data = strchr(chan_type, '/')) != 0) {
					*chan_data = '\0';
					chan_data++;
				}

				if (session) {
					if (!switch_channel_ready(caller_channel)) {
						status = SWITCH_STATUS_FALSE;
						goto done;
					}

					caller_caller_profile = caller_profile_override ? caller_profile_override : switch_channel_get_caller_profile(caller_channel);
					new_profile = switch_caller_profile_clone(session, caller_caller_profile);
					new_profile->uuid = NULL;
					new_profile->chan_name = NULL;
					new_profile->destination_number = chan_data;

					if (cid_name_override) {
						new_profile->caller_id_name = cid_name_override;
					}
					if (cid_num_override) {
						new_profile->caller_id_number = cid_num_override;
					}

					pool = NULL;
				} else {
					if (switch_core_new_memory_pool(&pool) != SWITCH_STATUS_SUCCESS) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "OH OH no pool\n");
						status = SWITCH_STATUS_TERM;
						goto done;
					}

					if (caller_profile_override) {
						new_profile = switch_caller_profile_dup(pool, caller_profile_override);
						new_profile->destination_number = chan_data;
						new_profile->uuid = NULL;
						new_profile->chan_name = NULL;
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
																cid_name_override,
																cid_num_override,
																NULL, NULL, NULL, NULL, __FILE__, NULL,
																chan_data);
					}
				}
				
				caller_profiles[i] = NULL;
				peer_channels[i] = NULL;
				peer_sessions[i] = NULL;
				new_session = NULL;
				
				if ((reason = switch_core_session_outgoing_channel(session, chan_type, new_profile, &new_session, &pool)) != SWITCH_CAUSE_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot Create Outgoing Channel! cause: %s\n", switch_channel_cause2str(reason));
					if (pool) {
						switch_core_destroy_memory_pool(&pool);
					}
					continue;
				}

				switch_core_session_read_lock(new_session);
				pool = NULL;

				caller_profiles[i] = new_profile;
				peer_sessions[i] = new_session;
				peer_channels[i] = switch_core_session_get_channel(new_session);
				assert(peer_channels[i] != NULL);

				if (!table) {
					table = &originate_state_handlers;
				}

				if (table) {
					switch_channel_add_state_handler(peer_channels[i], table);
				}

				if (switch_core_session_running(peer_sessions[i])) {
					switch_channel_set_state(peer_channels[i], CS_RING);
				} else {
					switch_core_session_thread_launch(peer_sessions[i]);
				}
			}

			time(&start);

			for (;;) {
				uint32_t valid_channels = 0;
				for (i = 0; i < and_argc; i++) {
					int state;

					if (!peer_channels[i]) {
						continue;
					}
					valid_channels++;
					state = switch_channel_get_state(peer_channels[i]);

					if (state >= CS_HANGUP) {
						goto notready;
					} else if (state >= CS_RING) {
						goto endfor1;
					}

					if (caller_channel && !switch_channel_ready(caller_channel)) {
						goto notready;
					}

					if ((time(NULL) - start) > (time_t) timelimit_sec) {
						to++;
						idx = IDX_CANCEL;
						goto notready;
					}
					switch_yield(1000);
				}

				if (valid_channels == 0) {
					status = SWITCH_STATUS_GENERR;
					goto done;
				}

			}
		  endfor1:

			if (ringback_data && !switch_channel_test_flag(caller_channel, CF_ANSWERED)
				&& !switch_channel_test_flag(caller_channel, CF_EARLY_MEDIA)) {
				switch_channel_pre_answer(caller_channel);
			}

			if (session && (read_codec = switch_core_session_get_read_codec(session)) &&
				(ringback_data || !switch_channel_test_flag(caller_channel, CF_BYPASS_MEDIA))) {

				if (!(pass = (uint8_t) switch_test_flag(read_codec, SWITCH_CODEC_FLAG_PASSTHROUGH))) {
					if (switch_core_codec_init(&write_codec,
											   "L16",
											   NULL,
											   read_codec->implementation->samples_per_second,
											   read_codec->implementation->microseconds_per_frame / 1000,
											   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE, NULL, pool) == SWITCH_STATUS_SUCCESS) {


						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
										  "Raw Codec Activation Success L16@%uhz 1 channel %dms\n",
										  read_codec->implementation->samples_per_second, read_codec->implementation->microseconds_per_frame / 1000);
						write_frame.codec = &write_codec;
						write_frame.datalen = read_codec->implementation->bytes_per_frame;
						write_frame.samples = write_frame.datalen / 2;
						memset(write_frame.data, 255, write_frame.datalen);

						if (ringback_data) {
							char *tmp_data = NULL;

							switch_buffer_create_dynamic(&ringback.audio_buffer, 512, 1024, 0);
							switch_buffer_set_loops(ringback.audio_buffer, -1);

							if (*ringback_data == '/') {
								char *ext;

								if ((ext = strrchr(ringback_data, '.'))) {
									switch_core_session_set_read_codec(session, &write_codec);
									ext++;
								} else {
									ringback.asis++;
									write_frame.codec = read_codec;
									ext = read_codec->implementation->iananame;
									tmp_data = switch_mprintf("%s.%s", ringback_data, ext);
									ringback_data = tmp_data;
								}

								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Play Ringback File [%s]\n", ringback_data);

								ringback.fhb.channels = read_codec->implementation->number_of_channels;
								ringback.fhb.samplerate = read_codec->implementation->samples_per_second;
								if (switch_core_file_open(&ringback.fhb,
														  ringback_data,
														  read_codec->implementation->number_of_channels,
														  read_codec->implementation->samples_per_second,
														  SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT,
														  switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
									switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Playing File\n");
									switch_safe_free(tmp_data);
									goto notready;
								}
								ringback.fh = &ringback.fhb;


							} else {
								teletone_init_session(&ringback.ts, 0, teletone_handler, &ringback);
								ringback.ts.rate = read_codec->implementation->samples_per_second;
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Play Ringback Tone [%s]\n", ringback_data);
								//ringback.ts.debug = 1;
								//ringback.ts.debug_stream = switch_core_get_console();
								if (teletone_run(&ringback.ts, ringback_data)) {
									switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Playing Tone\n");
									teletone_destroy_session(&ringback.ts);
									switch_buffer_destroy(&ringback.audio_buffer);
									ringback_data = NULL;
								}
							}
							switch_safe_free(tmp_data);
						}
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Codec Error!");
						switch_channel_hangup(caller_channel, SWITCH_CAUSE_NORMAL_TEMPORARY_FAILURE);
						read_codec = NULL;
					}
				}
			}

			if (ringback_data) {
				early_ok = 0;
			}

			while ((!caller_channel || switch_channel_ready(caller_channel)) &&
				   check_channel_status(peer_channels, peer_sessions, and_argc, &idx, &hups, file, key, early_ok, &ring_ready)) {

				// When the AND operator is being used, and fail_on_single_reject is set, a hangup indicates that the call should fail.
				if ((to = (uint8_t) ((time(NULL) - start) >= (time_t) timelimit_sec))
					|| (fail_on_single_reject && hups)) {
					idx = IDX_CANCEL;
					goto notready;
				}

				if (peer_sessions[0]
					&& switch_core_session_dequeue_message(peer_sessions[0], &message) == SWITCH_STATUS_SUCCESS) {
					if (session && !ringback_data && or_argc == 1 && and_argc == 1) {	/* when there is only 1 channel to call and bridge and no ringback */
						switch_core_session_receive_message(session, message);
					}

					if (switch_test_flag(message, SCSMF_DYNAMIC)) {
						switch_safe_free(message);
					} else {
						message = NULL;
					}
				}

				/* read from the channel while we wait if the audio is up on it */
				if (session && (ringback_data || !switch_channel_test_flag(caller_channel, CF_BYPASS_MEDIA)) &&
					(switch_channel_test_flag(caller_channel, CF_ANSWERED)
					 || switch_channel_test_flag(caller_channel, CF_EARLY_MEDIA))) {
					switch_status_t status = switch_core_session_read_frame(session, &read_frame, 1000, 0);

					if (!SWITCH_READ_ACCEPTABLE(status)) {
						break;
					}

					if (ring_ready && read_frame && !pass && !switch_test_flag(read_frame, SFF_CNG)
						&& read_frame->datalen > 1) {
						if (ringback.fh) {
							uint8_t abuf[1024];
							switch_size_t mlen, olen;
							unsigned int pos = 0;

							if (ringback.asis) {
								mlen = read_frame->datalen;
							} else {
								mlen = read_frame->datalen / 2;
							}

							olen = mlen;
							switch_core_file_read(ringback.fh, abuf, &olen);

							if (olen == 0) {
								olen = mlen;
								ringback.fh->speed = 0;
								switch_core_file_seek(ringback.fh, &pos, 0, SEEK_SET);
								switch_core_file_read(ringback.fh, abuf, &olen);
								if (olen == 0) {
									break;
								}
							}
							write_frame.data = abuf;
							write_frame.datalen = (uint32_t) (ringback.asis ? olen : olen * 2);
							if (switch_core_session_write_frame(session, &write_frame, 1000, 0) != SWITCH_STATUS_SUCCESS) {
								break;
							}
						} else if (ringback.audio_buffer) {
							if ((write_frame.datalen = (uint32_t) switch_buffer_read_loop(ringback.audio_buffer,
																						  write_frame.data,
																						  write_frame.codec->implementation->bytes_per_frame)) <= 0) {
								break;
							}
						}

						if (switch_core_session_write_frame(session, &write_frame, 1000, 0) != SWITCH_STATUS_SUCCESS) {
							break;
						}
					}

				} else {
					switch_yield(1000);
				}

			}

		  notready:

			if (caller_channel && !switch_channel_ready(caller_channel)) {
				idx = IDX_CANCEL;
			}

			if (session && (ringback_data || !switch_channel_test_flag(caller_channel, CF_BYPASS_MEDIA))) {
				switch_core_session_reset(session);
			}

			for (i = 0; i < and_argc; i++) {
				if (!peer_channels[i]) {
					continue;
				}
				if (i != idx) {
					if (idx == IDX_CANCEL) {
						if (to) {
							reason = SWITCH_CAUSE_NO_ANSWER;
						} else {
							reason = SWITCH_CAUSE_ORIGINATOR_CANCEL;
						}
					} else {
						if (to) {
							reason = SWITCH_CAUSE_NO_ANSWER;
						} else if (and_argc > 1) {
							reason = SWITCH_CAUSE_LOSE_RACE;
						} else {
							reason = SWITCH_CAUSE_NO_ANSWER;
						}
					}


					switch_channel_hangup(peer_channels[i], reason);
				}
			}


			if (idx > IDX_NADA) {
				peer_session = peer_sessions[idx];
				peer_channel = peer_channels[idx];
			} else {
				status = SWITCH_STATUS_FALSE;
				goto done;
			}

			if (caller_channel) {
				if (switch_channel_test_flag(peer_channel, CF_ANSWERED)) {
					switch_channel_answer(caller_channel);
				} else if (switch_channel_test_flag(peer_channel, CF_EARLY_MEDIA)) {
					switch_channel_pre_answer(caller_channel);
				}
			}

			if (switch_channel_test_flag(peer_channel, CF_ANSWERED)
				|| switch_channel_test_flag(peer_channel, CF_EARLY_MEDIA)) {
				*bleg = peer_session;
				status = SWITCH_STATUS_SUCCESS;
			} else {
				status = SWITCH_STATUS_FALSE;
			}

		  done:
			*cause = SWITCH_CAUSE_UNALLOCATED;

			if (var_event) {
				if (peer_channel && !caller_channel) {	/* install the vars from the {} params */
					switch_event_header_t *header;
					for (header = var_event->headers; header; header = header->next) {
						switch_channel_set_variable(peer_channel, header->name, header->value);
					}
				}
				switch_event_destroy(&var_event);
			}

			if (status == SWITCH_STATUS_SUCCESS) {
				if (caller_channel) {
					switch_channel_set_variable(caller_channel, "originate_disposition", "call accepted");
				}
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Originate Resulted in Success: [%s]\n", switch_channel_get_name(peer_channel));
				*cause = SWITCH_CAUSE_SUCCESS;

			} else {
				if (peer_channel) {
					*cause = switch_channel_get_cause(peer_channel);
				} else {
					for (i = 0; i < and_argc; i++) {
						if (!peer_channels[i]) {
							continue;
						}
						*cause = switch_channel_get_cause(peer_channels[i]);
						break;
					}
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

				if (idx == IDX_CANCEL) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
									  "Originate Cancelled by originator termination Cause: %d [%s]\n", *cause, switch_channel_cause2str(*cause));

				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
									  "Originate Resulted in Error Cause: %d [%s]\n", *cause, switch_channel_cause2str(*cause));
				}
			}

			if (caller_channel) {
				switch_channel_set_variable(caller_channel, "originate_disposition", switch_channel_cause2str(*cause));
			}

			if (!pass && write_codec.implementation) {
				switch_core_codec_destroy(&write_codec);
			}

			if (ringback.fh) {
				switch_core_file_close(ringback.fh);
				ringback.fh = NULL;
				if (read_codec && !ringback.asis) {
					switch_core_session_set_read_codec(session, read_codec);
					switch_core_session_reset(session);
				}
			} else if (ringback.audio_buffer) {
				teletone_destroy_session(&ringback.ts);
				switch_buffer_destroy(&ringback.audio_buffer);
			}

			for (i = 0; i < and_argc; i++) {
				if (!peer_channels[i]) {
					continue;
				}
				if (status == SWITCH_STATUS_SUCCESS && bleg && *bleg && *bleg == peer_sessions[i]) {
					continue;
				}
				switch_core_session_rwunlock(peer_sessions[i]);
			}
			
			if (status == SWITCH_STATUS_SUCCESS) {
				goto outer_for;
			}
		}
	}
  outer_for:
	switch_safe_free(loop_data);
	switch_safe_free(odata);

	if (bleg && status != SWITCH_STATUS_SUCCESS) {
		*bleg = NULL;
	}

	return status;
}
