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
 * Paul D. Tinsley <pdt at jackhammer.org>
 * Neal Horman <neal at wanlink dot com>
 * Matt Klein <mklein@nmedia.net>
 * Michael Jerris <mike@jerris.com>
 *
 * switch_ivr_play_say.c -- IVR Library (functions to play or say audio)
 *
 */
#include <switch.h>

static char *SAY_METHOD_NAMES[] = {
	"N/A",
	"PRONOUNCED",
	"ITERATED",
	"COUNTED",
	NULL
};

static char *SAY_TYPE_NAMES[] = {
	"NUMBER",
	"ITEMS",
	"PERSONS",
	"MESSAGES",
	"CURRENCY",
	"TIME_MEASUREMENT",
	"CURRENT_DATE",
	"CURRENT_TIME",
	"CURRENT_DATE_TIME",
	"TELEPHONE_NUMBER",
	"TELEPHONE_EXTENSION",
	"URL",
	"IP_ADDRESS",
	"EMAIL_ADDRESS",
	"POSTAL_ADDRESS",
	"ACCOUNT_NUMBER",
	"NAME_SPELLED",
	"NAME_PHONETIC",
	NULL
};


static switch_say_method_t get_say_method_by_name(char *name)
{
	int x = 0;
	for (x = 0; SAY_METHOD_NAMES[x]; x++) {
		if (!strcasecmp(SAY_METHOD_NAMES[x], name)) {
			break;
		}
	}

	return (switch_say_method_t) x;
}

static switch_say_type_t get_say_type_by_name(char *name)
{
	int x = 0;
	for (x = 0; SAY_TYPE_NAMES[x]; x++) {
		if (!strcasecmp(SAY_TYPE_NAMES[x], name)) {
			break;
		}
	}

	return (switch_say_type_t) x;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_phrase_macro(switch_core_session_t *session, char *macro_name, char *data, char *lang,
														switch_input_args_t *args)
{
	switch_xml_t cfg, xml = NULL, language, macros, macro, input, action;
	char *lname = NULL, *mname = NULL, hint_data[1024] = "", enc_hint[1024] = "";
	switch_status_t status = SWITCH_STATUS_GENERR;
	char *old_sound_prefix = NULL, *sound_path = NULL, *tts_engine = NULL, *tts_voice = NULL, *chan_lang = NULL;
	const char *module_name = NULL;
	switch_channel_t *channel;
	uint8_t done = 0;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (!macro_name) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No phrase macro specified.\n");
		return status;
	}

	if (!lang) {
		chan_lang = switch_channel_get_variable(channel, "default_language");
		if (!chan_lang) {
			chan_lang = "en";
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "No language specified - Using [%s]\n", chan_lang);
	} else {
		chan_lang = lang;
	}

	module_name = chan_lang;

	if (!data) {
		data = "";
	}

	switch_url_encode(data, enc_hint, sizeof(enc_hint));
	snprintf(hint_data, sizeof(hint_data), "macro_name=%s&lang=%s&data=%s&destination_number=%s", macro_name, chan_lang, enc_hint, switch_channel_get_variable(channel,"destination_number"));

	if (switch_xml_locate("phrases", NULL, NULL, NULL, &xml, &cfg, hint_data) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of phrases failed.\n");
		goto done;
	}

	if (!(macros = switch_xml_child(cfg, "macros"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "can't find macros tag.\n");
		goto done;
	}

	if (!(language = switch_xml_child(macros, "language"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "can't find language tag.\n");
		goto done;
	}

	while (language) {
		if ((lname = (char *) switch_xml_attr(language, "name")) && !strcasecmp(lname, chan_lang)) {
			const char *tmp;

			if ((tmp = switch_xml_attr(language, "module"))) {
				module_name = tmp;
			}
			break;
		}
		language = language->next;
	}

	if (!language) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "can't find language %s.\n", chan_lang);
		goto done;
	}

	
	if (!(sound_path = (char *) switch_xml_attr(language, "sound-path"))) {
		sound_path = (char *) switch_xml_attr(language, "sound_path");
	}

	if (!(tts_engine = (char *) switch_xml_attr(language, "tts-engine"))) {
		if (!(tts_engine = (char *) switch_xml_attr(language, "tts_engine"))) {
			tts_engine = switch_channel_get_variable(channel, tts_engine);
		}
	}
	
	if (!(tts_voice = (char *) switch_xml_attr(language, "tts-voice"))) {
		if (!(tts_voice = (char *) switch_xml_attr(language, "tts_voice"))) {
			tts_voice = switch_channel_get_variable(channel, tts_voice);
		}
	}

	if (sound_path) {
		old_sound_prefix = switch_channel_get_variable(channel, "sound_prefix");
		switch_channel_set_variable(channel, "sound_prefix", sound_path);
	}

	if (!(macro = switch_xml_child(language, "macro"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "can't find any macro tags.\n");
		goto done;
	}

	while (macro) {
		if ((mname = (char *) switch_xml_attr(macro, "name")) && !strcasecmp(mname, macro_name)) {
			break;
		}
		macro = macro->next;
	}

	if (!macro) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "can't find macro %s.\n", macro_name);
		goto done;
	}

	if (!(input = switch_xml_child(macro, "input"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "can't find any input tags.\n");
		goto done;
	}

	switch_channel_pre_answer(channel);

	while (input && !done) {
		char *pattern = (char *) switch_xml_attr(input, "pattern");

		if (pattern) {
			switch_regex_t *re = NULL;
			int proceed = 0, ovector[30];
			char *substituted = NULL;
			uint32_t len = 0;
			char *odata = NULL;
			char *expanded = NULL;
			switch_xml_t match = NULL;

			if ((proceed = switch_regex_perform(data, pattern, &re, ovector, sizeof(ovector) / sizeof(ovector[0])))) {
				match = switch_xml_child(input, "match");
			} else {
				match = switch_xml_child(input, "nomatch");
			}

			if (match) {
				status = SWITCH_STATUS_SUCCESS;
				for (action = switch_xml_child(match, "action"); action && status == SWITCH_STATUS_SUCCESS; action = action->next) {
					char *adata = (char *) switch_xml_attr_soft(action, "data");
					char *func = (char *) switch_xml_attr_soft(action, "function");

					if (strchr(pattern, '(') && strchr(adata, '$')) {
						len = (uint32_t) (strlen(data) + strlen(adata) + 10);
						if (!(substituted = malloc(len))) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Memory Error!\n");
							switch_regex_safe_free(re);
							switch_safe_free(expanded);
							goto done;
						}
						memset(substituted, 0, len);
						switch_perform_substitution(re, proceed, adata, data, substituted, len, ovector);
						odata = substituted;
					} else {
						odata = adata;
					}

					expanded = switch_channel_expand_variables(channel, odata);

					if (expanded == odata) {
						expanded = NULL;
					} else {
						odata = expanded;
					}

					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Handle %s:[%s] (%s:%s)\n", func, odata, chan_lang, module_name);

					if (!strcasecmp(func, "play-file")) {
						status = switch_ivr_play_file(session, NULL, odata, args);
					} else if (!strcasecmp(func, "break")) {
						done = 1;
						break;
					} else if (!strcasecmp(func, "execute")) {

					} else if (!strcasecmp(func, "say")) {
						switch_say_interface_t *si;
						if ((si = switch_loadable_module_get_say_interface(module_name))) {
							char *say_type = (char *) switch_xml_attr_soft(action, "type");
							char *say_method = (char *) switch_xml_attr_soft(action, "method");

							status = si->say_function(session, odata, get_say_type_by_name(say_type), get_say_method_by_name(say_method), args);
						} else {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid SAY Interface [%s]!\n", module_name);
						}
					} else if (!strcasecmp(func, "speak-text")) {
						char *my_tts_engine = (char *) switch_xml_attr(action, "tts-engine");
						char *my_tts_voice = (char *) switch_xml_attr(action, "tts-voice");
						
						if (!my_tts_engine) {
							my_tts_engine = tts_engine;
						}

						if (!my_tts_voice) {
							my_tts_voice = tts_voice;
						}
						if (switch_strlen_zero(tts_engine) || switch_strlen_zero(tts_voice)) {
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "TTS is not configured\n");
						} else {
							status = switch_ivr_speak_text(session, my_tts_engine, my_tts_voice, odata, args);
						}
					}
				}
			}

			switch_regex_safe_free(re);
			switch_safe_free(expanded);
			switch_safe_free(substituted);
		}

		if (status != SWITCH_STATUS_SUCCESS) {
			done = 1;
			break;
		}

		input = input->next;
	}

  done:
	if (old_sound_prefix) {
		switch_channel_set_variable(channel, "sound_prefix", old_sound_prefix);
	}
	if (xml) {
		switch_xml_free(xml);
	}
	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_record_file(switch_core_session_t *session,
													   switch_file_handle_t *fh, char *file, switch_input_args_t *args, uint32_t limit)
{
	switch_channel_t *channel;
	char dtmf[128];
	switch_file_handle_t lfh = { 0 };
	switch_frame_t *read_frame;
	switch_codec_t codec, *read_codec;
	char *codec_name;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char *p;
	const char *vval;
	time_t start = 0;
	uint32_t org_silence_hits = 0;

	if (!fh) {
		fh = &lfh;
	}

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	read_codec = switch_core_session_get_read_codec(session);
	assert(read_codec != NULL);

	fh->channels = read_codec->implementation->number_of_channels;
	fh->samplerate = read_codec->implementation->actual_samples_per_second;


	if (switch_core_file_open(fh,
							  file,
							  read_codec->implementation->number_of_channels,
							  read_codec->implementation->actual_samples_per_second,
							  SWITCH_FILE_FLAG_WRITE | SWITCH_FILE_DATA_SHORT, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		switch_core_session_reset(session);
		return SWITCH_STATUS_GENERR;
	}

	switch_channel_answer(channel);

	if ((p = switch_channel_get_variable(channel, "RECORD_TITLE"))) {
		vval = (const char *) switch_core_session_strdup(session, p);
		switch_core_file_set_string(fh, SWITCH_AUDIO_COL_STR_TITLE, vval);
		switch_channel_set_variable(channel, "RECORD_TITLE", NULL);
	}

	if ((p = switch_channel_get_variable(channel, "RECORD_COPYRIGHT"))) {
		vval = (const char *) switch_core_session_strdup(session, p);
		switch_core_file_set_string(fh, SWITCH_AUDIO_COL_STR_COPYRIGHT, vval);
		switch_channel_set_variable(channel, "RECORD_COPYRIGHT", NULL);
	}

	if ((p = switch_channel_get_variable(channel, "RECORD_SOFTWARE"))) {
		vval = (const char *) switch_core_session_strdup(session, p);
		switch_core_file_set_string(fh, SWITCH_AUDIO_COL_STR_SOFTWARE, vval);
		switch_channel_set_variable(channel, "RECORD_SOFTWARE", NULL);
	}

	if ((p = switch_channel_get_variable(channel, "RECORD_ARTIST"))) {
		vval = (const char *) switch_core_session_strdup(session, p);
		switch_core_file_set_string(fh, SWITCH_AUDIO_COL_STR_ARTIST, vval);
		switch_channel_set_variable(channel, "RECORD_ARTIST", NULL);
	}

	if ((p = switch_channel_get_variable(channel, "RECORD_COMMENT"))) {
		vval = (const char *) switch_core_session_strdup(session, p);
		switch_core_file_set_string(fh, SWITCH_AUDIO_COL_STR_COMMENT, vval);
		switch_channel_set_variable(channel, "RECORD_COMMENT", NULL);
	}

	if ((p = switch_channel_get_variable(channel, "RECORD_DATE"))) {
		vval = (const char *) switch_core_session_strdup(session, p);
		switch_core_file_set_string(fh, SWITCH_AUDIO_COL_STR_DATE, vval);
		switch_channel_set_variable(channel, "RECORD_DATE", NULL);
	}

	codec_name = "L16";
	if (switch_core_codec_init(&codec,
							   codec_name,
							   NULL,
							   read_codec->implementation->actual_samples_per_second,
							   read_codec->implementation->microseconds_per_frame / 1000,
							   read_codec->implementation->number_of_channels,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE, NULL,
							   switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activated\n");
		switch_core_session_set_read_codec(session, &codec);
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
						  "Raw Codec Activation Failed %s@%uhz %u channels %dms\n", codec_name, fh->samplerate,
						  fh->channels, read_codec->implementation->microseconds_per_frame / 1000);
		switch_core_file_close(fh);
		switch_core_session_reset(session);
		return SWITCH_STATUS_GENERR;
	}

	if (limit) {
		start = time(NULL);
	}

	if (fh->thresh) {
		if (fh->silence_hits) {
			fh->silence_hits = fh->samplerate * fh->silence_hits / read_codec->implementation->samples_per_frame;
		} else {
			fh->silence_hits = fh->samplerate * 3 / read_codec->implementation->samples_per_frame;
		}
		org_silence_hits = fh->silence_hits;
	}

	for(;;) {
		switch_size_t len;
		
		if (!switch_channel_ready(channel)) {
			status = SWITCH_STATUS_FALSE;
			break;
		}

		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			status = SWITCH_STATUS_BREAK;
			break;
		}
		
		if (switch_core_session_private_event_count(session)) {
			switch_ivr_parse_all_events(session);
		}

		if (start && (time(NULL) - start) > limit) {
			break;
		}

		if (args && (args->input_callback || args->buf || args->buflen)) {
			/*
			   dtmf handler function you can hook up to be executed when a digit is dialed during playback 
			   if you return anything but SWITCH_STATUS_SUCCESS the playback will stop.
			 */
			if (switch_channel_has_dtmf(channel)) {
				if (!args->input_callback && !args->buf) {
					status = SWITCH_STATUS_BREAK;
					break;
				}
				switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
				if (args->input_callback) {
					status = args->input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, args->buf, args->buflen);
				} else {
					switch_copy_string((char *) args->buf, dtmf, args->buflen);
					status = SWITCH_STATUS_BREAK;
				}
			}

			if (args->input_callback) {
				switch_event_t *event = NULL;

				if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
					status = args->input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, args->buf, args->buflen);
					switch_event_destroy(&event);
				}
			}


			if (status != SWITCH_STATUS_SUCCESS) {
				break;
			}
		}

		status = switch_core_session_read_frame(session, &read_frame, -1, 0);
		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}

		if (args && (args->read_frame_callback)) {
			if (args->read_frame_callback(session, read_frame, args->user_data) != SWITCH_STATUS_SUCCESS) {
				break;
			}
		}

		if (fh->thresh) {
			int16_t *fdata = (int16_t *) read_frame->data;
			uint32_t samples = read_frame->datalen / sizeof(*fdata);
			uint32_t score, count = 0, j = 0;
			double energy = 0;

			for (count = 0; count < samples; count++) {
				energy += abs(fdata[j]);
				j += read_codec->implementation->number_of_channels;
			}

			score = (uint32_t) (energy / samples);
			if (score < fh->thresh) {
				if (!--fh->silence_hits) {
					break;
				}
			} else {
				fh->silence_hits = org_silence_hits;
			}
		}

		if (!switch_test_flag(fh, SWITCH_FILE_PAUSE)) {
			len = (switch_size_t) read_frame->datalen / 2;
			if (switch_core_file_write(fh, read_frame->data, &len) != SWITCH_STATUS_SUCCESS) {
				break;
			}
		}
	}

	switch_core_session_set_read_codec(session, read_codec);
	switch_core_file_close(fh);
	switch_core_session_reset(session);
	return status;
}

static int teletone_handler(teletone_generation_session_t * ts, teletone_tone_map_t * map)
{
	switch_buffer_t *audio_buffer = ts->user_data;
	int wrote;

	if (!audio_buffer) {
		return -1;
	}

	wrote = teletone_mux_tones(ts, map);
	switch_buffer_write(audio_buffer, ts->buffer, wrote * 2);

	return 0;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_gentones(switch_core_session_t *session, char *script, int32_t loops, switch_input_args_t *args)
{
	teletone_generation_session_t ts;
	switch_buffer_t *audio_buffer;
	switch_frame_t *read_frame = NULL;
	switch_codec_t *read_codec = NULL, write_codec = { 0 };
	switch_frame_t write_frame = { 0 };
	switch_byte_t data[1024];
	switch_channel_t *channel;

	assert(session != NULL);

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_channel_pre_answer(channel);
	read_codec = switch_core_session_get_read_codec(session);
	
	if (switch_core_codec_init(&write_codec,
							   "L16",
							   NULL,
							   read_codec->implementation->actual_samples_per_second,
							   read_codec->implementation->microseconds_per_frame / 1000,
							   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE, 
							   NULL, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {

		return SWITCH_STATUS_FALSE;
	}
	memset(&ts, 0, sizeof(ts));
	write_frame.codec = &write_codec;
	write_frame.data = data;

	switch_buffer_create_dynamic(&audio_buffer, 512, 1024, 0);
	teletone_init_session(&ts, 0, teletone_handler, audio_buffer);
	ts.rate = read_codec->implementation->actual_samples_per_second;
	ts.channels = 1;
	teletone_run(&ts, script);

	if (loops) {
		switch_buffer_set_loops(audio_buffer, loops);
	}

	for(;;) {
		switch_status_t status = switch_core_session_read_frame(session, &read_frame, 1000, 0);

		if (!switch_channel_ready(channel)) {
            status = SWITCH_STATUS_FALSE;
            break;
        }


		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			status = SWITCH_STATUS_BREAK;
			break;
		}

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}

		if (read_frame->datalen < 2 || switch_test_flag(read_frame, SFF_CNG)) {
			continue;
		}

		if (args && (args->read_frame_callback)) {
			if (args->read_frame_callback(session, read_frame, args->user_data) != SWITCH_STATUS_SUCCESS) {
				break;
			}
		}

		if ((write_frame.datalen = (uint32_t) switch_buffer_read_loop(audio_buffer, write_frame.data,
																	  read_frame->codec->implementation->bytes_per_frame)) <= 0) {
			break;
		}

		write_frame.samples = write_frame.datalen / 2;

		if (switch_core_session_write_frame(session, &write_frame, 1000, 0) != SWITCH_STATUS_SUCCESS) {
			break;
		}
	}

	switch_core_codec_destroy(&write_codec);
	switch_buffer_destroy(&audio_buffer);
	teletone_destroy_session(&ts);

	return SWITCH_STATUS_SUCCESS;
}


#define FILE_STARTSAMPLES 1024 * 32
#define FILE_BLOCKSIZE 1024 * 8
#define FILE_BUFSIZE 1024 * 64

SWITCH_DECLARE(switch_status_t) switch_ivr_play_file(switch_core_session_t *session, switch_file_handle_t *fh, char *file, switch_input_args_t *args)
{
	switch_channel_t *channel;
	int16_t abuf[FILE_STARTSAMPLES];
	char dtmf[128];
	uint32_t interval = 0, samples = 0, framelen, sample_start = 0;
	uint32_t ilen = 0;
	switch_size_t olen = 0, llen = 0;
	switch_frame_t write_frame = { 0 };
	switch_timer_t timer = { 0 };
	switch_core_thread_session_t thread_session;
	switch_codec_t codec;
	switch_memory_pool_t *pool = switch_core_session_get_pool(session);
	char *codec_name;
	int stream_id = 0;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_file_handle_t lfh;
	switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
	const char *p;
	char *title = "", *copyright = "", *software = "", *artist = "", *comment = "", *date = "";
	uint8_t asis = 0;
	char *ext;
	char *prefix;
	char *timer_name;
	char *prebuf;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	prefix = switch_channel_get_variable(channel, "sound_prefix");
	timer_name = switch_channel_get_variable(channel, "timer_name");


	if (!file) {
		return SWITCH_STATUS_FALSE;
	}

	if (!strstr(file, SWITCH_URL_SEPARATOR)) {
		if (switch_is_file_path(file)) {
			char *new_file;
			uint32_t len;
			len = (uint32_t) strlen(file) + (uint32_t) strlen(prefix) + 10;
			new_file = switch_core_session_alloc(session, len);
			snprintf(new_file, len, "%s%s%s", prefix, SWITCH_PATH_SEPARATOR, file);
			file = new_file;
		}

		if ((ext = strrchr(file, '.'))) {
			ext++;
		} else {
			char *new_file;
			uint32_t len;
			ext = read_codec->implementation->iananame;
			len = (uint32_t) strlen(file) + (uint32_t) strlen(ext) + 2;
			new_file = switch_core_session_alloc(session, len);
			snprintf(new_file, len, "%s.%s", file, ext);
			file = new_file;
			asis = 1;
		}
	}


	if (!fh) {
		fh = &lfh;
		memset(fh, 0, sizeof(lfh));
	}


	if (fh->samples > 0) {
		sample_start = fh->samples;
		fh->samples = 0;
	}

	if ((prebuf = switch_channel_get_variable(channel, "stream_prebuffer"))) {
		int maybe = atoi(prebuf);
		if (maybe > 0) {
			fh->prebuf = maybe;
		}
	}
	


	if (switch_core_file_open(fh,
							  file,
							  read_codec->implementation->number_of_channels,
							  read_codec->implementation->actual_samples_per_second,
							  SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_core_session_reset(session);
		return SWITCH_STATUS_NOTFOUND;
	}
	if (switch_test_flag(fh, SWITCH_FILE_NATIVE)) {
		asis = 1;
	}


	write_frame.data = abuf;
	write_frame.buflen = sizeof(abuf);

	if (sample_start > 0) {
		uint32_t pos = 0;
		switch_core_file_seek(fh, &pos, 0, SEEK_SET);
		switch_core_file_seek(fh, &pos, sample_start, SEEK_CUR);
	}

	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_TITLE, &p) == SWITCH_STATUS_SUCCESS) {
		title = switch_core_session_strdup(session, p);
		switch_channel_set_variable(channel, "RECORD_TITLE", p);
	}

	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_COPYRIGHT, &p) == SWITCH_STATUS_SUCCESS) {
		copyright = switch_core_session_strdup(session, p);
		switch_channel_set_variable(channel, "RECORD_COPYRIGHT", p);
	}

	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_SOFTWARE, &p) == SWITCH_STATUS_SUCCESS) {
		software = switch_core_session_strdup(session, p);
		switch_channel_set_variable(channel, "RECORD_SOFTWARE", p);
	}

	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_ARTIST, &p) == SWITCH_STATUS_SUCCESS) {
		artist = switch_core_session_strdup(session, p);
		switch_channel_set_variable(channel, "RECORD_ARTIST", p);
	}

	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_COMMENT, &p) == SWITCH_STATUS_SUCCESS) {
		comment = switch_core_session_strdup(session, p);
		switch_channel_set_variable(channel, "RECORD_COMMENT", p);
	}

	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_DATE, &p) == SWITCH_STATUS_SUCCESS) {
		date = switch_core_session_strdup(session, p);
		switch_channel_set_variable(channel, "RECORD_DATE", p);
	}
#if 0
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
					  "OPEN FILE %s %uhz %u channels\n"
					  "TITLE=%s\n"
					  "COPYRIGHT=%s\n"
					  "SOFTWARE=%s\n"
					  "ARTIST=%s\n" "COMMENT=%s\n" "DATE=%s\n", file, fh->samplerate, fh->channels, title, copyright, software, artist, comment, date);
#endif

	assert(read_codec != NULL);
	interval = read_codec->implementation->microseconds_per_frame / 1000;

	if (!fh->audio_buffer) {
		switch_buffer_create_dynamic(&fh->audio_buffer, FILE_BLOCKSIZE, FILE_BUFSIZE, 0);
		if (!fh->audio_buffer) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "setup buffer failed\n");

			switch_core_file_close(fh);
			switch_core_session_reset(session);

			return SWITCH_STATUS_GENERR;
		}
	}

	if (asis) {
		write_frame.codec = read_codec;
		samples = read_codec->implementation->samples_per_frame;
		framelen = read_codec->implementation->encoded_bytes_per_frame;
	} else {
		codec_name = "L16";

		if (switch_core_codec_init(&codec,
								   codec_name,
								   NULL,
								   fh->samplerate,
								   interval, fh->channels, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE, NULL, pool) == SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG,
							  SWITCH_LOG_DEBUG, "Codec Activated %s@%uhz %u channels %dms\n", codec_name, fh->samplerate, fh->channels, interval);

			write_frame.codec = &codec;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
							  "Raw Codec Activation Failed %s@%uhz %u channels %dms\n", codec_name, fh->samplerate, fh->channels, interval);
			switch_core_file_close(fh);
			switch_core_session_reset(session);
			return SWITCH_STATUS_GENERR;
		}
		samples = codec.implementation->samples_per_frame;
		framelen = codec.implementation->bytes_per_frame;
	}


	if (timer_name) {
		uint32_t len;

		len = samples * 2;
		if (switch_core_timer_init(&timer, timer_name, interval, samples, pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "setup timer failed!\n");
			switch_core_codec_destroy(&codec);
			switch_core_file_close(fh);
			switch_core_session_reset(session);
			return SWITCH_STATUS_GENERR;
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "setup timer success %u bytes per %d ms!\n", len, interval);
	}
	write_frame.rate = fh->samplerate;

	if (timer_name) {
		/* start a thread to absorb incoming audio */
		for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
			switch_core_service_session(session, &thread_session, stream_id);
		}
	}

	ilen = samples;

	for(;;) {
		int done = 0;
		int do_speed = 1;
		int last_speed = -1;

		if (!switch_channel_ready(channel)) {
            status = SWITCH_STATUS_FALSE;
            break;
        }

		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			status = SWITCH_STATUS_BREAK;
			break;
		}

		if (switch_core_session_private_event_count(session)) {
			switch_ivr_parse_all_events(session);
		}

		if (args && (args->input_callback || args->buf || args->buflen)) {
			/*
			   dtmf handler function you can hook up to be executed when a digit is dialed during playback 
			   if you return anything but SWITCH_STATUS_SUCCESS the playback will stop.
			 */
			if (switch_channel_has_dtmf(channel)) {
				if (!args->input_callback && !args->buf) {
					status = SWITCH_STATUS_BREAK;
					done = 1;
					break;
				}
				switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
				if (args->input_callback) {
					status = args->input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, args->buf, args->buflen);
				} else {
					switch_copy_string((char *) args->buf, dtmf, args->buflen);
					status = SWITCH_STATUS_BREAK;
				}
			}

			if (args->input_callback) {
				switch_event_t *event;

				if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
					status = args->input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, args->buf, args->buflen);
					switch_event_destroy(&event);
				}
			}

			if (status != SWITCH_STATUS_SUCCESS) {
				done = 1;
				break;
			}
		}

		if (switch_test_flag(fh, SWITCH_FILE_PAUSE)) {
			memset(abuf, 0, framelen);
			olen = ilen;
			do_speed = 0;
		} else if (fh->audio_buffer && (switch_buffer_inuse(fh->audio_buffer) > (switch_size_t) (framelen))) {
			switch_buffer_read(fh->audio_buffer, abuf, framelen);
			olen = asis ? framelen : ilen;
			do_speed = 0;
		} else {
			olen = sizeof(abuf);
			if (!asis) {
				olen /= 2;
			}
			switch_core_file_read(fh, abuf, &olen);
			switch_buffer_write(fh->audio_buffer, abuf, asis ? olen : olen * 2);
			olen = switch_buffer_read(fh->audio_buffer, abuf, framelen);
			if (!asis) {
				olen /= 2;
			}
		}

		if (done || olen <= 0) {
			break;
		}
		

		if (!asis) {
			if (fh->speed > 2) {
				fh->speed = 2;
			} else if (fh->speed < -2) {
				fh->speed = -2;
			}
		}

		if (!asis && fh->audio_buffer && last_speed > -1 && last_speed != fh->speed) {
			switch_buffer_zero(fh->audio_buffer);
		}

		if (switch_test_flag(fh, SWITCH_FILE_SEEK)) {
			/* file position has changed flush the buffer */
			switch_buffer_zero(fh->audio_buffer);
			switch_clear_flag(fh, SWITCH_FILE_SEEK);
		}


		if (!asis && fh->speed && do_speed) {
			float factor = 0.25f * abs(fh->speed);
			switch_size_t newlen, supplement, step;
			short *bp = write_frame.data;
			switch_size_t wrote = 0;


			supplement = (int) (factor * olen);
			newlen = (fh->speed > 0) ? olen - supplement : olen + supplement;
			step = (fh->speed > 0) ? (newlen / supplement) : (olen / supplement);

			while ((wrote + step) < newlen) {
				switch_buffer_write(fh->audio_buffer, bp, step * 2);
				wrote += step;
				bp += step;
				if (fh->speed > 0) {
					bp++;
				} else {
					float f;
					short s;
					f = (float) (*bp + *(bp + 1) + *(bp - 1));
					f /= 3;
					s = (short) f;
					switch_buffer_write(fh->audio_buffer, &s, 2);
					wrote++;
				}
			}
			if (wrote < newlen) {
				switch_size_t r = newlen - wrote;
				switch_buffer_write(fh->audio_buffer, bp, r * 2);
				wrote += r;
			}
			last_speed = fh->speed;
			continue;
		}
		if (olen < llen) {
			uint8_t *dp = (uint8_t *) write_frame.data;
			memset(dp + (int) olen, 0, (int) (llen - olen));
			olen = llen;
		}

		write_frame.samples = (uint32_t)olen;

		if (asis) {
			write_frame.datalen = (uint32_t)olen;
		} else {
			write_frame.datalen = write_frame.samples * 2;
		}



		llen = olen;

		if (timer_name) {
			write_frame.timestamp = timer.samplecount;
		}
#ifndef WIN32
#if __BYTE_ORDER == __BIG_ENDIAN
		if (!asis) {
			switch_swap_linear(write_frame.data, (int) write_frame.datalen / 2);
		}
#endif
#endif
		stream_id = 0;

		if (fh->vol) {
			switch_change_sln_volume(write_frame.data, write_frame.datalen / 2, fh->vol);
		}

		fh->offset_pos += write_frame.samples / 2;
		status = switch_core_session_write_frame(session, &write_frame, -1, stream_id);
		
		if (status == SWITCH_STATUS_MORE_DATA) {
			status = SWITCH_STATUS_SUCCESS;
			continue;
		} else if (status != SWITCH_STATUS_SUCCESS) {
			done = 1;
			break;
		}

		if (done) {
			break;
		}


		if (timer_name) {
			if (switch_core_timer_next(&timer) != SWITCH_STATUS_SUCCESS) {
				break;
			}
		} else {				/* time off the channel (if you must) */
			switch_frame_t *read_frame;
			switch_status_t status;
			while (switch_channel_ready(channel) && switch_channel_test_flag(channel, CF_HOLD)) {
				switch_yield(10000);
			}
			status = switch_core_session_read_frame(session, &read_frame, -1, 0);

			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}

			if (args && (args->read_frame_callback)) {
				if (args->read_frame_callback(session, read_frame, args->user_data) != SWITCH_STATUS_SUCCESS) {
					break;
				}
			}
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "done playing file\n");
	switch_core_file_seek(fh, &fh->last_pos, 0, SEEK_CUR);
	
	switch_core_file_close(fh);
	switch_buffer_destroy(&fh->audio_buffer);
	if (!asis) {
		switch_core_codec_destroy(&codec);
	}
	if (timer_name) {
		/* End the audio absorbing thread */
		switch_core_thread_session_end(&thread_session);
		switch_core_timer_destroy(&timer);
	}

	switch_core_session_reset(session);
	return status;
}

SWITCH_DECLARE(switch_status_t) switch_play_and_get_digits(switch_core_session_t *session,
														   uint32_t min_digits,
														   uint32_t max_digits,
														   uint32_t max_tries,
														   uint32_t timeout,
														   char *valid_terminators,
														   char *prompt_audio_file,
														   char *bad_input_audio_file, void *digit_buffer, uint32_t digit_buffer_length,
														   char *digits_regex)
{

	char terminator;			//used to hold terminator recieved from  
	switch_channel_t *channel;	//the channel contained in session
	switch_status_t status;		//used to recieve state out of called functions

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
					  "switch_play_and_get_digits(session, %d, %d, %d, %d, %s, %s, %s, digit_buffer, %d, %s)\n",
					  min_digits, max_digits, max_tries, timeout, valid_terminators, prompt_audio_file,
					  bad_input_audio_file, digit_buffer_length, digits_regex);

	//Get the channel
	channel = switch_core_session_get_channel(session);

	//Make sure somebody is home
	assert(channel != NULL);

	//Answer the channel if it hasn't already been answered
	switch_channel_answer(channel);

	//Start pestering the user for input
	for (; (switch_channel_get_state(channel) == CS_EXECUTE) && max_tries > 0; max_tries--) {
		switch_input_args_t args = { 0 };
		//make the buffer so fresh and so clean clean
		memset(digit_buffer, 0, digit_buffer_length);

		args.buf = digit_buffer;
		args.buflen = digit_buffer_length;
		//Play the file
		status = switch_ivr_play_file(session, NULL, prompt_audio_file, &args);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "play gave up %s", (char *) digit_buffer);

		//Make sure we made it out alive
		if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
			goto done;
		}
		//we only get one digit out of playback, see if thats all we needed and what we got
		if (max_digits == 1 && status == SWITCH_STATUS_BREAK) {
			//Check the digit if we have a regex
			if (digits_regex != NULL) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Checking regex [%s] on [%s]\n", digits_regex, (char *) digit_buffer);

				//Make sure the digit is allowed
				if (switch_regex_match(digit_buffer, digits_regex) == SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Match found!\n");
					//jobs done
					break;
				} else {
					//See if a bad input prompt was specified, if so, play it
					if (strlen(bad_input_audio_file) > 0) {
						status = switch_ivr_play_file(session, NULL, bad_input_audio_file, NULL);

						//Make sure we made it out alive
						if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
							goto done;
						}
					}
				}
			} else {
				//jobs done
				break;
			}
		}

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Calling more digits try %d\n", max_tries);

		//Try to grab some more digits for the timeout period
		status = switch_ivr_collect_digits_count(session, digit_buffer, digit_buffer_length, max_digits, valid_terminators, &terminator, timeout);

		//Make sure we made it out alive
		if (status != SWITCH_STATUS_SUCCESS) {
			//Bail
			goto done;
		}
		//see if we got enough
		if (min_digits <= strlen(digit_buffer)) {
			//See if we need to test a regex
			if (digits_regex != NULL) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Checking regex [%s] on [%s]\n", digits_regex, (char *) digit_buffer);
				//Test the regex
				if (switch_regex_match(digit_buffer, digits_regex) == SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Match found!\n");
					//Jobs done
					return SWITCH_STATUS_SUCCESS;
				} else {
					//See if a bad input prompt was specified, if so, play it
					if (strlen(bad_input_audio_file) > 0) {
						status = switch_ivr_play_file(session, NULL, bad_input_audio_file, NULL);

						//Make sure we made it out alive
						if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
							goto done;
						}
					}
				}
			} else {
				//Jobs done
				return SWITCH_STATUS_SUCCESS;
			}
		}
	}

 done:
	//if we got here, we got no digits or lost the channel
	digit_buffer = "\0";
	return SWITCH_STATUS_FALSE;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_speak_text_handle(switch_core_session_t *session,
															 switch_speech_handle_t *sh,
															 switch_codec_t *codec, switch_timer_t *timer, char *text, switch_input_args_t *args)
{
	switch_channel_t *channel;
	short abuf[960];
	char dtmf[128];
	uint32_t len = 0;
	switch_size_t ilen = 0;
	switch_frame_t write_frame = { 0 };
	int x;
	int stream_id = 0;
	int done = 0;
	int lead_in_out = 10;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_speech_flag_t flags = SWITCH_SPEECH_FLAG_NONE;
	uint32_t rate = 0;
	switch_size_t extra = 0;
	char *p, *tmp = NULL;
	char *star, *pound;
	switch_size_t starlen, poundlen;
	
	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (!sh) {
		return SWITCH_STATUS_FALSE;
	}

	switch_channel_answer(channel);

	write_frame.data = abuf;
	write_frame.buflen = sizeof(abuf);

	len = sh->samples * 2;
	
	flags = 0;
	
	if (!(star = switch_channel_get_variable(channel, "star_replace"))) {
		star = "star";
	}
	if (!(pound = switch_channel_get_variable(channel, "pound_replace"))) {
		pound = "pound";
	}
	starlen = strlen(star);
	poundlen = strlen(pound);


	for(p = text; p && *p; p++) {
		if (*p == '*') {
			extra += starlen;
		} else if (*p == '#') {
			extra += poundlen;
		}
	}

	if (extra) {
		char *tp;
		switch_size_t mylen = strlen(text) + extra + 1;
		tmp = malloc(mylen);
		memset(tmp, 0, mylen);
		tp = tmp;
		for (p = text; p && *p; p++) {
			if (*p == '*') {
				strncat(tp, star, starlen);
				tp += starlen;
			} else  if (*p == '#') {
				strncat(tp, pound, poundlen);
				tp += poundlen;
			} else {
				*tp++ = *p;
			}
		}
		
		text = tmp;
	}

	switch_core_speech_feed_tts(sh, text, &flags);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Speaking text: %s\n", text);
	switch_safe_free(tmp);
	text = NULL;

	write_frame.rate = sh->rate;

	memset(write_frame.data, 0, len);
	write_frame.datalen = len;
	write_frame.samples = len / 2;
	write_frame.codec = codec;

	assert(codec->implementation != NULL);

	for (x = 0; !done && x < lead_in_out; x++) {
		switch_yield(codec->implementation->microseconds_per_frame);
		if (timer) {
			write_frame.timestamp = timer->samplecount;
		}
		if (switch_core_session_write_frame(session, &write_frame, -1, stream_id) != SWITCH_STATUS_SUCCESS) {
			done = 1;
			break;
		}
	}

	ilen = len;
	for(;;) {
		switch_event_t *event;

		if (!switch_channel_ready(channel)) {
            status = SWITCH_STATUS_FALSE;
            break;
        }

		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			status = SWITCH_STATUS_BREAK;
			break;
		}

		if (switch_core_session_dequeue_private_event(session, &event) == SWITCH_STATUS_SUCCESS) {
			switch_ivr_parse_event(session, event);
			switch_event_destroy(&event);
		}

		if (args && (args->input_callback || args->buf || args->buflen)) {
			/*
			   dtmf handler function you can hook up to be executed when a digit is dialed during playback 
			   if you return anything but SWITCH_STATUS_SUCCESS the playback will stop.
			 */
			if (switch_channel_has_dtmf(channel)) {
				if (!args->input_callback && !args->buf) {
					status = SWITCH_STATUS_BREAK;
					done = 1;
					break;
				}
				if (args->buf && !strcasecmp(args->buf, "_break_")) {
					status = SWITCH_STATUS_BREAK;
				} else {
					switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
					if (args->input_callback) {
						status = args->input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, args->buf, args->buflen);
					} else {
						switch_copy_string((char *) args->buf, dtmf, args->buflen);
						status = SWITCH_STATUS_BREAK;
					}
				}
			}

			if (args->input_callback) {
				if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
					status = args->input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, args->buf, args->buflen);
					switch_event_destroy(&event);
				}
			}

			if (status != SWITCH_STATUS_SUCCESS) {
				done = 1;
				break;
			}
		}

		if (switch_test_flag(sh, SWITCH_SPEECH_FLAG_PAUSE)) {
			if (timer) {
				if (switch_core_timer_next(timer) != SWITCH_STATUS_SUCCESS) {
					break;
				}
			} else {
				switch_frame_t *read_frame;
				switch_status_t status = switch_core_session_read_frame(session, &read_frame, -1, 0);

				while (switch_channel_ready(channel) && switch_channel_test_flag(channel, CF_HOLD)) {
					switch_yield(10000);
				}

				if (!SWITCH_READ_ACCEPTABLE(status)) {
					break;
				}

				if (args && (args->read_frame_callback)) {
					if (args->read_frame_callback(session, read_frame, args->user_data) != SWITCH_STATUS_SUCCESS) {
						break;
					}
				}
			}
			continue;
		}

		flags = SWITCH_SPEECH_FLAG_BLOCKING;
		status = switch_core_speech_read_tts(sh, abuf, &ilen, &rate, &flags);

		if (status != SWITCH_STATUS_SUCCESS) {
			for (x = 0; !done && x < lead_in_out; x++) {
				switch_yield(codec->implementation->microseconds_per_frame);
				if (timer) {
					write_frame.timestamp = timer->samplecount;
				}
				if (switch_core_session_write_frame(session, &write_frame, -1, stream_id) != SWITCH_STATUS_SUCCESS) {
					done = 1;
					break;
				}
			}
			if (status == SWITCH_STATUS_BREAK) {
				status = SWITCH_STATUS_SUCCESS;
			}
			done = 1;
		}

		if (done) {
			break;
		}

		write_frame.datalen = (uint32_t) ilen;
		write_frame.samples = (uint32_t) (ilen / 2);
		if (timer) {
			write_frame.timestamp = timer->samplecount;
		}
		if (switch_core_session_write_frame(session, &write_frame, -1, stream_id) != SWITCH_STATUS_SUCCESS) {
			done = 1;
			break;
		}

		if (done) {
			break;
		}

		if (timer) {
			if (switch_core_timer_next(timer) != SWITCH_STATUS_SUCCESS) {
				break;
			}
		} else {				/* time off the channel (if you must) */
			switch_frame_t *read_frame;
			switch_status_t status = switch_core_session_read_frame(session, &read_frame, -1, 0);

			while (switch_channel_ready(channel) && switch_channel_test_flag(channel, CF_HOLD)) {
				switch_yield(10000);
			}

			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}

			if (args && (args->read_frame_callback)) {
				if (args->read_frame_callback(session, read_frame, args->user_data) != SWITCH_STATUS_SUCCESS) {
					break;
				}
			}
		}

	}


	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "done speaking text\n");
	flags = 0;
	switch_core_speech_flush_tts(sh);
	return status;
}


struct cached_speech_handle {
	char tts_name[80];
	char voice_name[80];
	switch_speech_handle_t sh;
	switch_codec_t codec;
	switch_timer_t timer;
};
typedef struct cached_speech_handle cached_speech_handle_t;

SWITCH_DECLARE(void) switch_ivr_clear_speech_cache(switch_core_session_t *session) 
{
	cached_speech_handle_t *cache_obj = NULL;
	switch_channel_t *channel;

	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	if ((cache_obj = switch_channel_get_private(channel, SWITCH_CACHE_SPEECH_HANDLES_OBJ_NAME))) {
		switch_speech_flag_t flags = SWITCH_SPEECH_FLAG_NONE;
		if (cache_obj->timer.interval) {
			switch_core_timer_destroy(&cache_obj->timer);
		}
		switch_core_speech_close(&cache_obj->sh, &flags);
		switch_core_codec_destroy(&cache_obj->codec);
		switch_channel_set_private(channel, SWITCH_CACHE_SPEECH_HANDLES_OBJ_NAME, NULL);
	}

}


SWITCH_DECLARE(switch_status_t) switch_ivr_speak_text(switch_core_session_t *session,
													  char *tts_name, char *voice_name, char *text, switch_input_args_t *args)
{
	switch_channel_t *channel;
	uint32_t rate = 0;
	int interval = 0;
	switch_frame_t write_frame = { 0 };
	switch_timer_t ltimer, *timer;
	switch_core_thread_session_t thread_session;
	switch_codec_t lcodec, *codec;
	switch_memory_pool_t *pool = switch_core_session_get_pool(session);
	char *codec_name;
	int stream_id = 0;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_speech_handle_t lsh, *sh;
	switch_speech_flag_t flags = SWITCH_SPEECH_FLAG_NONE;
	switch_codec_t *read_codec;
	char *timer_name, *var;
	cached_speech_handle_t *cache_obj = NULL;
	int need_create = 1, need_alloc = 1;
	
	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	sh = &lsh;
	codec = &lcodec;
	timer = &ltimer;

	if ((var = switch_channel_get_variable(channel, SWITCH_CACHE_SPEECH_HANDLES_VARIABLE)) && switch_true(var)) {
		if ((cache_obj = switch_channel_get_private(channel, SWITCH_CACHE_SPEECH_HANDLES_OBJ_NAME))) {
			need_create = 0;
			if (!strcasecmp(cache_obj->tts_name, tts_name)) {
				need_alloc = 0;
			} else {
				switch_ivr_clear_speech_cache(session);
			}
		}

		if (!cache_obj) {
			cache_obj = switch_core_session_alloc(session, sizeof(*cache_obj));
		}
		if (need_alloc) {
			switch_copy_string(cache_obj->tts_name, tts_name, sizeof(cache_obj->tts_name));
			switch_copy_string(cache_obj->voice_name, voice_name, sizeof(cache_obj->voice_name));
			switch_channel_set_private(channel, SWITCH_CACHE_SPEECH_HANDLES_OBJ_NAME, cache_obj);
		}
		sh = &cache_obj->sh;
		codec = &cache_obj->codec;
		timer = &cache_obj->timer;
	}

	timer_name = switch_channel_get_variable(channel, "timer_name");

	switch_core_session_reset(session);
	read_codec = switch_core_session_get_read_codec(session);
	
	rate = read_codec->implementation->actual_samples_per_second;
	interval = read_codec->implementation->microseconds_per_frame / 1000;
	
	if (need_create) {
		memset(sh, 0, sizeof(*sh));
		if (switch_core_speech_open(sh, tts_name, voice_name, (uint32_t) rate, interval, 
									&flags, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid TTS module!\n");
			switch_core_session_reset(session);
			if (cache_obj) {
				switch_channel_set_private(channel, SWITCH_CACHE_SPEECH_HANDLES_OBJ_NAME, NULL);
			}
			return SWITCH_STATUS_FALSE;
		}
	} else if (cache_obj && strcasecmp(cache_obj->voice_name, voice_name)) {
		switch_copy_string(cache_obj->voice_name, voice_name, sizeof(cache_obj->voice_name));
		switch_core_speech_text_param_tts(sh, "voice", voice_name);
	}

	switch_channel_answer(channel);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "OPEN TTS %s\n", tts_name);


	codec_name = "L16";

	if (need_create) {
		if (switch_core_codec_init(codec,
								   codec_name,
								   NULL, (int) rate, interval, 1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE, NULL, pool) == SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activated\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activation Failed %s@%uhz 1 channel %dms\n", codec_name, rate, interval);
			flags = 0;
			switch_core_speech_close(sh, &flags);
			switch_core_session_reset(session);
			if (cache_obj) {
				switch_channel_set_private(channel, SWITCH_CACHE_SPEECH_HANDLES_OBJ_NAME, NULL);
			}
			return SWITCH_STATUS_GENERR;
		}
	}
	
	write_frame.codec = codec;

	if (timer_name) {
		if (need_create) {
			if (switch_core_timer_init(timer, timer_name, interval, (int) sh->samples, pool) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "setup timer failed!\n");
				switch_core_codec_destroy(write_frame.codec);
				flags = 0;
				switch_core_speech_close(sh, &flags);
				switch_core_session_reset(session);
				if (cache_obj) {
					switch_channel_set_private(channel, SWITCH_CACHE_SPEECH_HANDLES_OBJ_NAME, NULL);
				}
				return SWITCH_STATUS_GENERR;
			}
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "setup timer success %u bytes per %d ms!\n", sh->samples * 2, interval);
		}
		/* start a thread to absorb incoming audio */
		for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
			switch_core_service_session(session, &thread_session, stream_id);
		}
	}
	
	status = switch_ivr_speak_text_handle(session, sh, write_frame.codec, timer_name ? timer : NULL, text, args);
	flags = 0;

	if (!cache_obj) {
		switch_core_speech_close(sh, &flags);
		switch_core_codec_destroy(codec);
	}

	if (timer_name) {
		/* End the audio absorbing thread */
		switch_core_thread_session_end(&thread_session);
		if (!cache_obj) {
			switch_core_timer_destroy(timer);
		}
	}

	switch_core_session_reset(session);
	return status;
}
