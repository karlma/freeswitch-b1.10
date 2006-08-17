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
 * switch_ivr_api.c -- IVR Library
 *
 */
#include <switch.h>
#include <switch_ivr.h>

static const switch_state_handler_table_t audio_bridge_peer_state_handlers;

SWITCH_DECLARE(switch_status_t) switch_ivr_sleep(switch_core_session_t *session, uint32_t ms)
{
	switch_channel_t *channel;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_time_t start, now, done = switch_time_now() + (ms * 1000);
	switch_frame_t *read_frame;
	int32_t left, elapsed;

	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	start = switch_time_now();

	for(;;) {
		now = switch_time_now();
		elapsed = (int32_t)((now - start) / 1000);
		left = ms - elapsed;

		if (!switch_channel_ready(channel)) {
			status = SWITCH_STATUS_FALSE;
			break;
		}

		if (now > done || left <= 0) {
			break;
		}

		if (switch_channel_test_flag(channel, CF_SERVICE)) {
			switch_yield(1000);
		} else {
			status = switch_core_session_read_frame(session, &read_frame, left, 0);
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
		}
	}
	

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_collect_digits_callback(switch_core_session_t *session,
																 switch_input_callback_function_t input_callback,
																 void *buf,
																 unsigned int buflen)
{
	switch_channel_t *channel;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	if (!input_callback) {
		return SWITCH_STATUS_GENERR;
	}

	while(switch_channel_ready(channel)) {
		switch_frame_t *read_frame;
		switch_event_t *event;

		char dtmf[128];

		if (switch_channel_has_dtmf(channel)) {
			switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
			status = input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, buf, buflen);
		}

		if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
			status = input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, buf, buflen);			
			switch_event_destroy(&event);
		}

		if (status != SWITCH_STATUS_SUCCESS) {
			break;
		}

		if (switch_channel_test_flag(channel, CF_SERVICE)) {
			switch_yield(1000);
		} else {
			status = switch_core_session_read_frame(session, &read_frame, -1, 0);
		}

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}
	}

	return status;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_collect_digits_count(switch_core_session_t *session,
																char *buf,
																unsigned int buflen,
																unsigned int maxdigits,
																const char *terminators,
																char *terminator,
																unsigned int timeout)
{
	unsigned int i = 0, x =  (unsigned int) strlen(buf);
	switch_channel_t *channel;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_time_t started = 0;
	unsigned int elapsed;

	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	*terminator = '\0';

	if (terminators) {
		for (i = 0 ; i < x; i++) {
			if (strchr(terminators, buf[i])) {
				*terminator = buf[i];
				return SWITCH_STATUS_SUCCESS;
			}
		}
	}

	if (timeout) {
		started = switch_time_now();
	}

	while(switch_channel_ready(channel)) {
		switch_frame_t *read_frame;

		if (timeout) {
			elapsed = (unsigned int)((switch_time_now() - started) / 1000);
			if (elapsed >= timeout) {
				break;
			}
		}
		
		if (switch_channel_has_dtmf(channel)) {
			char dtmf[128];
			switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
			for(i =0 ; i < (unsigned int) strlen(dtmf); i++) {

				if (strchr(terminators, dtmf[i])) {
					*terminator = dtmf[i];
					return SWITCH_STATUS_SUCCESS;
				}

				buf[x++] = dtmf[i];
				buf[x] = '\0';
				if (x >= buflen || x >= maxdigits) {
					return SWITCH_STATUS_SUCCESS;
				}
			}
		}

		if (switch_channel_test_flag(channel, CF_SERVICE)) {
			switch_yield(1000);
		} else {
			status = switch_core_session_read_frame(session, &read_frame, -1, 0);
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
		}
	}

	return status;
}



SWITCH_DECLARE(switch_status_t) switch_ivr_record_file(switch_core_session_t *session, 
													 switch_file_handle_t *fh,
													 char *file,
													 switch_input_callback_function_t input_callback,
													 void *buf,
													 unsigned int buflen)
{
	switch_channel_t *channel;
    char dtmf[128];
	switch_file_handle_t lfh;
	switch_frame_t *read_frame;
	switch_codec_t codec, *read_codec;
	char *codec_name;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char *p;
	const char *vval;

	if (!fh) {
		fh = &lfh;
	}
	memset(fh, 0, sizeof(*fh));

	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	read_codec = switch_core_session_get_read_codec(session);
	assert(read_codec != NULL);

	fh->channels = read_codec->implementation->number_of_channels;
	fh->samplerate = read_codec->implementation->samples_per_second;


	if (switch_core_file_open(fh,
							  file,
							  SWITCH_FILE_FLAG_WRITE | SWITCH_FILE_DATA_SHORT,
							  switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
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
							   read_codec->implementation->samples_per_second,
							   read_codec->implementation->microseconds_per_frame / 1000,
							   read_codec->implementation->number_of_channels,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activated\n");
		switch_core_session_set_read_codec(session, &codec);		
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Raw Codec Activation Failed %s@%uhz %u channels %dms\n",
							  codec_name, fh->samplerate, fh->channels, read_codec->implementation->microseconds_per_frame / 1000);
		switch_core_file_close(fh);
		switch_core_session_reset(session);
		return SWITCH_STATUS_GENERR;
	}
	

	while(switch_channel_ready(channel)) {
		switch_size_t len;
		switch_event_t *event;

		if (input_callback || buf) {
			/*
			  dtmf handler function you can hook up to be executed when a digit is dialed during playback 
			  if you return anything but SWITCH_STATUS_SUCCESS the playback will stop.
			*/
			if (switch_channel_has_dtmf(channel)) {
				switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
				if (input_callback) {
					status = input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, buf, buflen);
				} else {
					switch_copy_string((char *)buf, dtmf, buflen);
					status = SWITCH_STATUS_BREAK;
				}
			}

			if (input_callback) {
				if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
					status = input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, buf, buflen);			
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
		if (!switch_test_flag(fh, SWITCH_FILE_PAUSE)) {
			len = (switch_size_t) read_frame->datalen / 2;
			switch_core_file_write(fh, read_frame->data, &len);
		}
	}

	switch_core_session_set_read_codec(session, read_codec);
	switch_core_file_close(fh);
	switch_core_session_reset(session);
	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_play_file(switch_core_session_t *session, 
												   switch_file_handle_t *fh,
												   char *file,
												   char *timer_name,
												   switch_input_callback_function_t input_callback,
												   void *buf,
												   unsigned int buflen)
{
	switch_channel_t *channel;
	short abuf[960];
	char dtmf[128];
	uint32_t interval = 0, samples = 0;
	uint32_t ilen = 0;
	switch_size_t olen = 0;
	switch_frame_t write_frame = {0};
	switch_timer_t timer;
	switch_core_thread_session_t thread_session;
	switch_codec_t codec;
	switch_memory_pool_t *pool = switch_core_session_get_pool(session);
	char *codec_name;
	int stream_id;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_file_handle_t lfh;
	switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
	const char *p;
	char *title = "", *copyright = "", *software = "", *artist = "", *comment = "", *date = "";
	
	if (!fh) {
		fh = &lfh;
		memset(fh, 0, sizeof(lfh));
	}

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (switch_core_file_open(fh,
							  file,
							  SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT,
							  switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_core_session_reset(session);
		return SWITCH_STATUS_NOTFOUND;
	}


	write_frame.data = abuf;
	write_frame.buflen = sizeof(abuf);

	
	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_TITLE, &p) == SWITCH_STATUS_SUCCESS) {
		title = (char *) switch_core_session_strdup(session, (char *)p);
		switch_channel_set_variable(channel, "RECORD_TITLE", (char *)p);
	}
	
	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_COPYRIGHT, &p) == SWITCH_STATUS_SUCCESS) {
		copyright = (char *) switch_core_session_strdup(session, (char *)p);
		switch_channel_set_variable(channel, "RECORD_COPYRIGHT", (char *)p);
	}
	
	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_SOFTWARE, &p) == SWITCH_STATUS_SUCCESS) {
		software = (char *) switch_core_session_strdup(session, (char *)p);
		switch_channel_set_variable(channel, "RECORD_SOFTWARE", (char *)p);
	}
	
	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_ARTIST, &p) == SWITCH_STATUS_SUCCESS) {
		artist = (char *) switch_core_session_strdup(session, (char *)p);
		switch_channel_set_variable(channel, "RECORD_ARTIST", (char *)p);
	}
	
	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_COMMENT, &p) == SWITCH_STATUS_SUCCESS) {
		comment = (char *) switch_core_session_strdup(session, (char *)p);
		switch_channel_set_variable(channel, "RECORD_COMMENT", (char *)p);
	}
	
	if (switch_core_file_get_string(fh, SWITCH_AUDIO_COL_STR_DATE, &p) == SWITCH_STATUS_SUCCESS) {
		date = (char *) switch_core_session_strdup(session, (char *)p);
		switch_channel_set_variable(channel, "RECORD_DATE", (char *)p);
	}
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
					  "OPEN FILE %s %uhz %u channels\n"
					  "TITLE=%s\n"
					  "COPYRIGHT=%s\n"
					  "SOFTWARE=%s\n"
					  "ARTIST=%s\n"
					  "COMMENT=%s\n"
					  "DATE=%s\n", file, fh->samplerate, fh->channels,
					  title,
					  copyright,
					  software,
					  artist,
					  comment,
					  date);

	assert(read_codec != NULL);
	interval = read_codec->implementation->microseconds_per_frame / 1000;


	codec_name = "L16";

	if (switch_core_codec_init(&codec,
							   codec_name,
							   fh->samplerate,
							   interval,
							   fh->channels,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, pool) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activated\n");

		write_frame.codec = &codec;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activation Failed %s@%uhz %u channels %dms\n",
							  codec_name, fh->samplerate, fh->channels, interval);
		switch_core_file_close(fh);
		switch_core_session_reset(session);
		return SWITCH_STATUS_GENERR;
	}

	samples = codec.implementation->bytes_per_frame / 2;

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
	while(switch_channel_ready(channel)) {
		int done = 0;
		int do_speed = 1;
		int last_speed = -1;
		switch_event_t *event;
	
		if (input_callback || buf) {
			/*
			  dtmf handler function you can hook up to be executed when a digit is dialed during playback 
			  if you return anything but SWITCH_STATUS_SUCCESS the playback will stop.
			*/
			if (switch_channel_has_dtmf(channel)) {
				switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
				if (input_callback) {
					status = input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, buf, buflen);
				} else {
					switch_copy_string((char *)buf, dtmf, buflen);
					status = SWITCH_STATUS_BREAK;
				}
			}

			if (input_callback) {
				if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
					status = input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, buf, buflen);			
					switch_event_destroy(&event);
				}
			}
			
			if (status != SWITCH_STATUS_SUCCESS) {
				done = 1;
				break;
			}
		}
		
		if (switch_test_flag(fh, SWITCH_FILE_PAUSE)) {
			memset(abuf, 0, ilen * 2);
			olen = ilen;
            do_speed = 0;
		} else if (fh->audio_buffer && (switch_buffer_inuse(fh->audio_buffer) > (switch_size_t)(ilen * 2))) {
			switch_buffer_read(fh->audio_buffer, abuf, ilen * 2);
			olen = ilen;
			do_speed = 0;
		} else {
			olen = ilen;
			switch_core_file_read(fh, abuf, &olen);
		}

		if (done || olen <= 0) {
			break;
		}

		if (fh->speed > 2) {
			fh->speed = 2;
		} else if (fh->speed < -2) {
			fh->speed = -2;
		}
		
		if (fh->audio_buffer && last_speed > -1 && last_speed != fh->speed) {
			switch_buffer_zero(fh->audio_buffer);
		}

		
		if (fh->speed && do_speed) {
			float factor = 0.25f * abs(fh->speed);
			switch_size_t newlen, supplement, step;
			short *bp = write_frame.data;
			switch_size_t wrote = 0;
			
			if (!fh->audio_buffer) {
				switch_buffer_create(fh->memory_pool, &fh->audio_buffer, SWITCH_RECCOMMENDED_BUFFER_SIZE);
			} 
			
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
					f = (float)(*bp + *(bp+1) + *(bp-1));
					f /= 3;
					s = (short) f;
					switch_buffer_write(fh->audio_buffer, &s, 2);
					wrote++;
				}
			}
			if (wrote < newlen) {
				switch_size_t r = newlen - wrote;
				switch_buffer_write(fh->audio_buffer, bp, r*2);
				wrote += r;
			}
			last_speed = fh->speed;
			continue;
		} 

		write_frame.datalen = (uint32_t)(olen * 2);
		write_frame.samples = (uint32_t)olen;

#ifndef WIN32
#if __BYTE_ORDER == __BIG_ENDIAN
		switch_swap_linear(write_frame.data, (int) write_frame.datalen / 2);
#endif
#endif
		for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
			status = switch_core_session_write_frame(session, &write_frame, -1, stream_id);

			if (status == SWITCH_STATUS_MORE_DATA) {
				status = SWITCH_STATUS_SUCCESS;
				continue;
			} else if (status != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Bad Write\n");
				done = 1;
				break;
			}
			if (done) {
				break;
			}
		}
		
		if (timer_name) {
			if (switch_core_timer_next(&timer) < 0) {
				break;
			}
		} else { /* time off the channel (if you must) */
			switch_frame_t *read_frame;
			switch_status_t status; 
			while (switch_channel_test_flag(channel, CF_HOLD)) {
				switch_yield(10000);
			}
			status = switch_core_session_read_frame(session, &read_frame, -1, 0);
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "done playing file\n");
	switch_core_file_close(fh);
	switch_core_codec_destroy(&codec);

	if (timer_name) {
		/* End the audio absorbing thread */
		switch_core_thread_session_end(&thread_session);
		switch_core_timer_destroy(&timer);
	}

	switch_core_session_reset(session);
	return status;
}




SWITCH_DECLARE(switch_status_t) switch_ivr_speak_text_handle(switch_core_session_t *session, 
															 switch_speech_handle_t *sh,
															 switch_codec_t *codec,
															 switch_timer_t *timer,
															 switch_input_callback_function_t input_callback,
															 char *text,
															 void *buf,
															 unsigned int buflen)
{
	switch_channel_t *channel;
	short abuf[960];
	char dtmf[128];
	uint32_t len = 0;
	switch_size_t ilen = 0;
	switch_frame_t write_frame = {0};
	int x;
	int stream_id;
	int done = 0;
	int lead_in_out = 10;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_speech_flag_t flags = SWITCH_SPEECH_FLAG_TTS;
	uint32_t rate = 0, samples = 0;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (!sh) {
		return SWITCH_STATUS_FALSE;
	}
	
	switch_channel_answer(channel);

	write_frame.data = abuf;
	write_frame.buflen = sizeof(abuf);

    samples = (uint32_t)(sh->rate / 50);
    len = samples * 2;

	flags = 0;
	switch_sleep(200000);
	switch_core_speech_feed_tts(sh, text, &flags);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Speaking text: %s\n", text);

	write_frame.rate = sh->rate;

	memset(write_frame.data, 0, len);
	write_frame.datalen = len;
	write_frame.samples = len / 2;
	write_frame.codec = codec;

	for( x = 0; !done && x < lead_in_out; x++) {
		for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
			if (switch_core_session_write_frame(session, &write_frame, -1, stream_id) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Bad Write\n");
				done = 1;
				break;
			}
		}
	}

	ilen = len;
	while(switch_channel_ready(channel)) {
		switch_event_t *event;

		if (input_callback || buf) {
			/*
			  dtmf handler function you can hook up to be executed when a digit is dialed during playback 
			  if you return anything but SWITCH_STATUS_SUCCESS the playback will stop.
			*/
			if (switch_channel_has_dtmf(channel)) {
				if (buf && !strcasecmp(buf, "_break_")) {
					status = SWITCH_STATUS_BREAK;
				} else {
					switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
					if (input_callback) {
						status = input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, buf, buflen);
					} else {
						switch_copy_string((char *)buf, dtmf, buflen);
						status = SWITCH_STATUS_BREAK;
					}
				}
			}

			if (input_callback) {
				if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
					status = input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, buf, buflen);			
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
				if ((x = switch_core_timer_next(timer)) < 0) {
					break;
				}
			} else {
				switch_frame_t *read_frame;
				switch_status_t status = switch_core_session_read_frame(session, &read_frame, -1, 0);

				while (switch_channel_test_flag(channel, CF_HOLD)) {
					switch_yield(10000);
				}
			
				if (!SWITCH_READ_ACCEPTABLE(status)) {
					break;
				}
			}
			continue;
		}

		flags = SWITCH_SPEECH_FLAG_BLOCKING;
		status = switch_core_speech_read_tts(sh,
											 abuf,
											 &ilen,
											 &rate,
											 &flags);

		if (status != SWITCH_STATUS_SUCCESS) {
			for( x = 0; !done && x < lead_in_out; x++) {
				for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
					if (switch_core_session_write_frame(session, &write_frame, -1, stream_id) != SWITCH_STATUS_SUCCESS) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Bad Write\n");
						done = 1;
						break;
					}
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

		write_frame.datalen = (uint32_t)ilen;
		write_frame.samples = (uint32_t)(ilen / 2);

		for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
			if (switch_core_session_write_frame(session, &write_frame, -1, stream_id) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Bad Write\n");
				done = 1;
				break;
			}

			if (done) {
				break;
			}
		}

		if (timer) {
			if ((x = switch_core_timer_next(timer)) < 0) {
				break;
			}
		} else { /* time off the channel (if you must) */
			switch_frame_t *read_frame;
			switch_status_t status = switch_core_session_read_frame(session, &read_frame, -1, 0);

			while (switch_channel_test_flag(channel, CF_HOLD)) {
				switch_yield(10000);
			}
			
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
		}

	}
	
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "done speaking text\n");
	flags = 0;	
	switch_core_speech_flush_tts(sh);
	return status;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_speak_text(switch_core_session_t *session, 
													  char *tts_name,
													  char *voice_name,
													  char *timer_name,
													  uint32_t rate,
													  switch_input_callback_function_t input_callback,
													  char *text,
													  void *buf,
													  unsigned int buflen)
{
	switch_channel_t *channel;
	int interval = 0;
	uint32_t samples = 0;
	uint32_t len = 0;
	switch_frame_t write_frame = {0};
	switch_timer_t timer;
	switch_core_thread_session_t thread_session;
	switch_codec_t codec;
	switch_memory_pool_t *pool = switch_core_session_get_pool(session);
	char *codec_name;
	int stream_id;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_speech_handle_t sh;
	switch_speech_flag_t flags = SWITCH_SPEECH_FLAG_TTS;


	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);


	memset(&sh, 0, sizeof(sh));
	if (switch_core_speech_open(&sh,
								tts_name,
								voice_name,
								(unsigned int)rate,
								&flags,
								switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid TTS module!\n");
		switch_core_session_reset(session);
		return SWITCH_STATUS_FALSE;
	}
		
	switch_channel_answer(channel);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "OPEN TTS %s\n", tts_name);
	
	interval = 20;
	samples = (uint32_t)(rate / 50);
	len = samples * 2;

	codec_name = "L16";

	if (switch_core_codec_init(&codec,
							   codec_name,
							   (int)rate,
							   interval,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, pool) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activated\n");
		write_frame.codec = &codec;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activation Failed %s@%uhz 1 channel %dms\n",
							  codec_name, rate, interval);
		flags = 0;
		switch_core_speech_close(&sh, &flags);
		switch_core_session_reset(session);
		return SWITCH_STATUS_GENERR;
	}

	if (timer_name) {
		if (switch_core_timer_init(&timer, timer_name, interval, (int)samples, pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "setup timer failed!\n");
			switch_core_codec_destroy(&codec);
			flags = 0;
			switch_core_speech_close(&sh, &flags);

			switch_core_session_reset(session);
			return SWITCH_STATUS_GENERR;
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "setup timer success %u bytes per %d ms!\n", len, interval);

		/* start a thread to absorb incoming audio */
		for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
			switch_core_service_session(session, &thread_session, stream_id);
		}
	}

	switch_ivr_speak_text_handle(session, &sh, &codec, timer_name ? &timer : NULL, input_callback, text, buf, buflen);
	flags = 0;	
	switch_core_speech_close(&sh, &flags);
	switch_core_codec_destroy(&codec);

	if (timer_name) {
		/* End the audio absorbing thread */
		switch_core_thread_session_end(&thread_session);
		switch_core_timer_destroy(&timer);
	}

	switch_core_session_reset(session);
	return status;
}


/* Bridge Related Stuff*/
/*********************************************************************************/
struct audio_bridge_data {
	switch_core_session_t *session_a;
	switch_core_session_t *session_b;
	int running;
};

static void *audio_bridge_thread(switch_thread_t *thread, void *obj)
{
	switch_core_thread_session_t *his_thread, *data = obj;
	int *stream_id_p;
	int stream_id = 0, ans_a = 0, ans_b = 0;
	switch_input_callback_function_t input_callback;
	switch_core_session_message_t msg = {0};
	void *user_data;

	switch_channel_t *chan_a, *chan_b;
	switch_frame_t *read_frame;
	switch_core_session_t *session_a, *session_b;

	assert(!thread || thread);

	session_a = data->objs[0];
	session_b = data->objs[1];

	stream_id_p = data->objs[2];
	input_callback = (switch_input_callback_function_t) data->objs[3];
	user_data = data->objs[4];
	his_thread = data->objs[5];

	if (stream_id_p) {
		stream_id = *stream_id_p;
	}

	chan_a = switch_core_session_get_channel(session_a);
	chan_b = switch_core_session_get_channel(session_b);


	ans_a = switch_channel_test_flag(chan_a, CF_ANSWERED);
	ans_b = switch_channel_test_flag(chan_b, CF_ANSWERED);

	switch_channel_set_flag(chan_a, CF_BRIDGED);

	while (data->running > 0 && his_thread->running > 0) {
		switch_channel_state_t b_state = switch_channel_get_state(chan_b);
		switch_status_t status;
		switch_event_t *event;

		switch (b_state) {
		case CS_HANGUP:
		case CS_DONE:
			data->running = -1;
			continue;
		default:
			break;
		}

		if (switch_channel_test_flag(chan_a, CF_TRANSFER)) {
			break;
		}

		if (!switch_channel_test_flag(chan_a, CF_HOLD)) {
			/* If this call is running on early media and it answers for real, pass it along... */
			if (!ans_b && switch_channel_test_flag(chan_a, CF_ANSWERED)) {
				if (!switch_channel_test_flag(chan_b, CF_ANSWERED)) {
					switch_channel_answer(chan_b);
				}
				ans_b++;
			}

			if (!ans_a && switch_channel_test_flag(chan_b, CF_ANSWERED)) {
				if (!switch_channel_test_flag(chan_a, CF_ANSWERED)) {
					switch_channel_answer(chan_a);
				}
				ans_a++;
			}
			


			/* if 1 channel has DTMF pass it to the other */
			if (switch_channel_has_dtmf(chan_a)) {
				char dtmf[128];
				switch_channel_dequeue_dtmf(chan_a, dtmf, sizeof(dtmf));
				switch_core_session_send_dtmf(session_b, dtmf);

				if (input_callback) {
					if (input_callback(session_a, dtmf, SWITCH_INPUT_TYPE_DTMF, user_data, 0) != SWITCH_STATUS_SUCCESS) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s ended call via DTMF\n", switch_channel_get_name(chan_a));
						data->running = -1;
						break;
					}
				}
			}

			if (switch_core_session_dequeue_event(session_a, &event) == SWITCH_STATUS_SUCCESS) {
				if (input_callback) {
					status = input_callback(session_a, event, SWITCH_INPUT_TYPE_EVENT, user_data, 0);
				}

				if (switch_core_session_receive_event(session_b, &event) != SWITCH_STATUS_SUCCESS) {
					switch_event_destroy(&event);
				}

			}
 
		}

		/* read audio from 1 channel and write it to the other */
		status = switch_core_session_read_frame(session_a, &read_frame, -1, stream_id);

		if (SWITCH_READ_ACCEPTABLE(status)) {
			if (status != SWITCH_STATUS_BREAK) {
				if (switch_core_session_write_frame(session_b, read_frame, -1, stream_id) != SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "write: %s Bad Frame....[%u] Bubye!\n", switch_channel_get_name(chan_b), read_frame->datalen);
					data->running = -1;
				}
			} 
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "read: %s Bad Frame.... Bubye!\n", switch_channel_get_name(chan_a));
			data->running = -1;
		}

		//switch_yield(1000);
	}

	
	msg.message_id = SWITCH_MESSAGE_INDICATE_UNBRIDGE;
	msg.from = __FILE__;
	switch_core_session_receive_message(session_a, &msg);

	data->running = 0;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "BRIDGE THREAD DONE [%s]\n", switch_channel_get_name(chan_a));

	if (switch_channel_test_flag(chan_a, CF_ORIGINATOR)) {
		if (!switch_channel_test_flag(chan_b, CF_TRANSFER)) {
			switch_core_session_kill_channel(session_b, SWITCH_SIG_KILL);
			switch_channel_hangup(chan_b, SWITCH_CAUSE_NORMAL_CLEARING);
		}
		switch_channel_clear_flag(chan_a, CF_ORIGINATOR);
		his_thread->running = 0;
	}

	switch_channel_clear_flag(chan_a, CF_BRIDGED);
	data->running = 0;
	return NULL;
}

static switch_status_t audio_bridge_on_loopback(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	void *arg;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if ((arg = switch_channel_get_private(channel))) {
		switch_channel_set_private(channel, NULL);
		audio_bridge_thread(NULL, (void *) arg);
	} else {
		switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
	}
	switch_channel_clear_state_handler(channel, &audio_bridge_peer_state_handlers);
		
	return SWITCH_STATUS_FALSE;
}


static switch_status_t audio_bridge_on_ring(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CUSTOM RING\n");

	/* put the channel in a passive state so we can loop audio to it */
	switch_channel_set_state(channel, CS_HOLD);
	return SWITCH_STATUS_FALSE;
}

static switch_status_t audio_bridge_on_hold(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CUSTOM HOLD\n");

	/* put the channel in a passive state so we can loop audio to it */
	return SWITCH_STATUS_FALSE;
}

static const switch_state_handler_table_t audio_bridge_peer_state_handlers = {
	/*.on_init */ NULL,
	/*.on_ring */ audio_bridge_on_ring,
	/*.on_execute */ NULL,
	/*.on_hangup */ NULL,
	/*.on_loopback */ audio_bridge_on_loopback,
	/*.on_transmit */ NULL,
	/*.on_hold */ audio_bridge_on_hold,
};

struct key_collect {
	char *key;
	char *file;
	switch_core_session_t *session;
};


static void *SWITCH_THREAD_FUNC collect_thread_run(switch_thread_t *thread, void *obj)
{
	struct key_collect *collect = (struct key_collect *) obj;
	switch_channel_t *channel = switch_core_session_get_channel(collect->session);
	char buf[10] = "";
	char *p, term;

	while(switch_channel_ready(channel)) {
		memset(buf, 0, sizeof(buf));

		if (collect->file) {
			switch_ivr_play_file(collect->session, NULL, collect->file, NULL, NULL, buf, sizeof(buf));
		} else {
			switch_ivr_collect_digits_count(collect->session, buf, sizeof(buf), 1, "", &term, 0);
		}

		for(p = buf; *p; p++) {
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
									uint32_t len,
									int32_t *idx,
									char *file,
									char *key)
{

	int32_t i;
	*idx = -1;
	
	if (len == 1) {
		*idx = 0;
		return (switch_channel_get_state(peer_channels[0]) < CS_HANGUP &&
				!switch_channel_test_flag(peer_channels[0], CF_ANSWERED) &&
				!switch_channel_test_flag(peer_channels[0], CF_EARLY_MEDIA)) ? 1 : 0;
	} else {
		int32_t hups = 0;

		for (i = 0; i < len; i++) {
			if (!peer_channels[i]) {
				continue;
			}
			if (switch_channel_get_state(peer_channels[i]) >= CS_HANGUP) {
				hups++;
			} else if (switch_channel_test_flag(peer_channels[i], CF_ANSWERED) && !switch_channel_test_flag(peer_channels[i], CF_TAGGED)) {

				if (key) {
					struct key_collect *collect;

					if ((collect = switch_core_session_alloc(peer_sessions[i], sizeof(*collect)))) {
						switch_channel_set_flag(peer_channels[i], CF_TAGGED);
						collect->key = key;
						if (file) {
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

		if (hups == len) {
			return 0;
		} else {
			return 1;
		}
	}
}

#define MAX_PEERS 256
SWITCH_DECLARE(switch_status_t) switch_ivr_originate(switch_core_session_t *session,
													 switch_core_session_t **bleg,
													 char *bridgeto,
													 uint32_t timelimit_sec,
													 const switch_state_handler_table_t *table,
													 char *cid_name_override,
													 char *cid_num_override)
										  
{
	char *peer_names[MAX_PEERS];
	switch_core_session_t *peer_session, *peer_sessions[MAX_PEERS];
	switch_caller_profile_t *caller_profiles[MAX_PEERS], *caller_caller_profile;
	char *chan_type = NULL, *chan_data;
	switch_channel_t *peer_channel = NULL, *peer_channels[MAX_PEERS];
	time_t start;
	switch_frame_t *read_frame = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_channel_t *caller_channel = NULL;
	switch_memory_pool_t *pool;
	char *data = NULL;
	int i, argc;
	int32_t idx = -1, ccount = 0;
	switch_codec_t write_codec = {0};
	switch_frame_t write_frame = {0};
	uint8_t err = 0, fdata[1024];
	char *file = NULL, *key = NULL, *odata, *var;

	write_frame.data = fdata;
	
	*bleg = NULL;
	odata = strdup(bridgeto);
	data = odata;

	if (!strncasecmp(data, "confirm=", 8)) {
		data += 8;
		file = data;
		if ((data = strchr(file, ';'))) {
			*data++ = '\0';
			if ((key = strchr(file, ':'))) {
				*key++ = '\0';
			} else {
				err++;
			}
		} else {
			err++;
		}
	}

	if (err) {
		status = SWITCH_STATUS_GENERR;
		goto done;
	}
	
	if (session) {
		caller_channel = switch_core_session_get_channel(session);
		assert(caller_channel != NULL);



		if ((var = switch_channel_get_variable(caller_channel, "group_confirm_key"))) {
			key = switch_core_session_strdup(session, var);
			if ((var = switch_channel_get_variable(caller_channel, "group_confirm_file"))) {
				file = switch_core_session_strdup(session, var);
			}
		}
	}
	


	if (file && !strcmp(file, "undef")) {
		file = NULL;
	}

	argc = switch_separate_string(data, '&', peer_names, (sizeof(peer_names) / sizeof(peer_names[0])));
	
	for (i = 0; i < argc; i++) {
		
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

			caller_caller_profile = switch_channel_get_caller_profile(caller_channel);

			if (!cid_name_override) {
				cid_name_override = caller_caller_profile->caller_id_name;
			}
			if (!cid_num_override) {
				cid_num_override = caller_caller_profile->caller_id_number;
			}

			caller_profiles[i] = switch_caller_profile_new(switch_core_session_get_pool(session),
														   caller_caller_profile->username,
														   caller_caller_profile->dialplan,
														   cid_name_override,
														   cid_num_override,
														   caller_caller_profile->network_addr, 
														   NULL,
														   NULL, 
														   caller_caller_profile->rdnis,
														   caller_caller_profile->source,
														   caller_caller_profile->context,
														   chan_data);
			pool = NULL;
		} else {
			if (!cid_name_override) {
				cid_name_override = "FreeSWITCH";
			}
			if (!cid_num_override) {
				cid_num_override = "0000000000";
			}

			if (switch_core_new_memory_pool(&pool) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "OH OH no pool\n");
				status = SWITCH_STATUS_TERM;
				goto done;
			}

			caller_profiles[i] = switch_caller_profile_new(pool,
														   NULL,
														   NULL,
														   cid_name_override,
														   cid_num_override,
														   NULL,
														   NULL, 
														   NULL,
														   NULL,
														   __FILE__,
														   NULL,
														   chan_data);
		}


		if (switch_core_session_outgoing_channel(session, chan_type, caller_profiles[i], &peer_sessions[i], pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot Create Outgoing Channel!\n");
			if (pool) {
				switch_core_destroy_memory_pool(&pool);
			}
			caller_profiles[i] = NULL;
			peer_channels[i] = NULL;
			peer_sessions[i] = NULL;
			continue;
		} 
		pool = NULL;
	
		peer_channels[i] = switch_core_session_get_channel(peer_sessions[i]);
		assert(peer_channels[i] != NULL);
		
		if (!table) {
			table = &audio_bridge_peer_state_handlers;
		}

		switch_channel_add_state_handler(peer_channels[i], table);

		if (switch_core_session_runing(peer_sessions[i])) {
			switch_channel_set_state(peer_channels[i], CS_RING);
		} else {
			switch_core_session_thread_launch(peer_sessions[i]);
		}
	}

	time(&start);
	for (i = 0; i < argc; i++) {
		if (peer_channels[i]) {
			ccount++;
		}
	}
	
	if (ccount == 1) {
		key = file = NULL;
	}

	for (;;) {
		for (i = 0; i < argc; i++) {
			int state;

			if (!peer_channels[i]) {
				continue;
			}

			state = switch_channel_get_state(peer_channels[i]);

			if (state >= CS_RING) {
				goto endfor1;
			}
		
			if (caller_channel && !switch_channel_ready(caller_channel)) {
				break;
			}
		
			if ((time(NULL) - start) > (time_t)timelimit_sec) {
				break;
			}
			switch_yield(1000);
		}
	}
 endfor1:

	if (session) {
		switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
		switch_channel_pre_answer(caller_channel);

		if (switch_core_codec_init(&write_codec,
								   "L16",
								   read_codec->implementation->samples_per_second,
								   read_codec->implementation->microseconds_per_frame / 1000,
								   1,
								   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
								   NULL,
								   pool) == SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activation Success L16@%uhz 1 channel %dms\n",
							  read_codec->implementation->samples_per_second,
							  read_codec->implementation->microseconds_per_frame / 1000);
			write_frame.codec = &write_codec;
			write_frame.datalen = read_codec->implementation->bytes_per_frame;
			write_frame.samples = write_frame.datalen / 2;
			memset(write_frame.data, 255, write_frame.datalen);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Codec Error!");
			switch_channel_hangup(caller_channel, SWITCH_CAUSE_NORMAL_TEMPORARY_FAILURE);
		}
	}

	while ((!caller_channel || switch_channel_ready(caller_channel)) && 
		   check_channel_status(peer_channels, peer_sessions, argc, &idx, file, key) && ((time(NULL) - start) < (time_t)timelimit_sec)) {
		
		/* read from the channel while we wait if the audio is up on it */
		if (session && (switch_channel_test_flag(caller_channel, CF_ANSWERED) || switch_channel_test_flag(caller_channel, CF_EARLY_MEDIA))) {
			switch_status_t status = switch_core_session_read_frame(session, &read_frame, 1000, 0);
			
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
			if (read_frame) {
				if (switch_core_session_write_frame(session, &write_frame, 1000, 0) != SWITCH_STATUS_SUCCESS) {
					break;
				}
			}

		} else {
			switch_yield(1000);
		}
		
	}

	switch_core_session_reset(session);

	for (i = 0; i < argc; i++) {
		if (!peer_channels[i]) {
			continue;
		}
		if (i != idx) {
			switch_channel_hangup(peer_channels[i], SWITCH_CAUSE_LOSE_RACE);
		}
	}


	if (idx > -1) {
		peer_session = peer_sessions[idx];
		peer_channel = peer_channels[idx];
	} else {
		status = SWITCH_STATUS_FALSE;
		goto done;
	}

	if (caller_channel && switch_channel_test_flag(peer_channel, CF_ANSWERED)) {
		switch_channel_answer(caller_channel);
	}

	if (switch_channel_test_flag(peer_channel, CF_ANSWERED) || switch_channel_test_flag(peer_channel, CF_EARLY_MEDIA)) {
		switch_core_session_read_lock(peer_session);
		*bleg = peer_session;
		status = SWITCH_STATUS_SUCCESS;
	} else {
		switch_channel_hangup(peer_channel, SWITCH_CAUSE_NO_ANSWER);
		status = SWITCH_STATUS_FALSE;
	}


 done:
	if (odata) {
		free(odata);
	}
	if (write_codec.implementation) {
		switch_core_codec_destroy(&write_codec);
	}
	return status;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_multi_threaded_bridge(switch_core_session_t *session, 
																 switch_core_session_t *peer_session,
																 switch_input_callback_function_t input_callback,
																 void *session_data,
																 void *peer_session_data)
	 

															   
{
	switch_core_thread_session_t *this_audio_thread, *other_audio_thread;
	switch_channel_t *caller_channel, *peer_channel;
	int stream_id = 0;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	caller_channel = switch_core_session_get_channel(session);
	assert(caller_channel != NULL);

	switch_channel_set_flag(caller_channel, CF_ORIGINATOR);

	peer_channel = switch_core_session_get_channel(peer_session);
	assert(peer_channel != NULL);

	other_audio_thread = switch_core_session_alloc(peer_session, sizeof(switch_core_thread_session_t));
	this_audio_thread = switch_core_session_alloc(peer_session, sizeof(switch_core_thread_session_t));

	other_audio_thread->objs[0] = session;
	other_audio_thread->objs[1] = peer_session;
	other_audio_thread->objs[2] = &stream_id;
	other_audio_thread->objs[3] = (void *) input_callback;
	other_audio_thread->objs[4] = session_data;
	other_audio_thread->objs[5] = this_audio_thread;
	other_audio_thread->running = 5;

	this_audio_thread->objs[0] = peer_session;
	this_audio_thread->objs[1] = session;
	this_audio_thread->objs[2] = &stream_id;
	this_audio_thread->objs[3] = (void *) input_callback;
	this_audio_thread->objs[4] = peer_session_data;
	this_audio_thread->objs[5] = other_audio_thread;
	this_audio_thread->running = 2;

	switch_channel_add_state_handler(peer_channel, &audio_bridge_peer_state_handlers);

	if (switch_channel_test_flag(peer_channel, CF_ANSWERED)) {
		switch_channel_answer(caller_channel);
	}

	if (switch_channel_test_flag(peer_channel, CF_ANSWERED) || switch_channel_test_flag(peer_channel, CF_EARLY_MEDIA)) {
		switch_event_t *event;
		switch_core_session_message_t msg = {0};
		
		switch_channel_set_state(peer_channel, CS_HOLD);

		if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_BRIDGE) == SWITCH_STATUS_SUCCESS) {
			switch_channel_event_set_data(caller_channel, event);
			switch_event_fire(&event);
		}
		
		msg.message_id = SWITCH_MESSAGE_INDICATE_BRIDGE;
		msg.from = __FILE__;
		msg.pointer_arg = session;

		switch_core_session_receive_message(peer_session, &msg);
		msg.pointer_arg = peer_session;
		switch_core_session_receive_message(session, &msg);

		if (switch_core_session_read_lock(peer_session) == SWITCH_STATUS_SUCCESS) {
			switch_channel_set_private(peer_channel, other_audio_thread);
			switch_channel_set_state(peer_channel, CS_LOOPBACK);
			audio_bridge_thread(NULL, (void *) this_audio_thread);

			if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_UNBRIDGE) == SWITCH_STATUS_SUCCESS) {
				switch_channel_event_set_data(caller_channel, event);
				switch_event_fire(&event);
			}
		
			this_audio_thread->objs[0] = NULL;
			this_audio_thread->objs[1] = NULL;
			this_audio_thread->objs[2] = NULL;
			this_audio_thread->objs[3] = NULL;
			this_audio_thread->objs[4] = NULL;
			this_audio_thread->objs[5] = NULL;
			this_audio_thread->running = 2;
			switch_core_session_rwunlock(peer_session);
		} else {
			status = SWITCH_STATUS_FALSE;
		}
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	if (status != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Bridge Failed %s->%s\n", 
						  switch_channel_get_name(caller_channel),
						  switch_channel_get_name(peer_channel)
						  );
		switch_channel_hangup(peer_channel, SWITCH_CAUSE_NO_ANSWER);
	}

	return status;
}



SWITCH_DECLARE(switch_status_t) switch_ivr_session_transfer(switch_core_session_t *session, char *extension, char *dialplan, char *context)
{
	switch_channel_t *channel;
	switch_caller_profile_t *profile, *new_profile;

	assert(session != NULL);
	assert(extension != NULL);

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if ((profile = switch_channel_get_caller_profile(channel))) {
		new_profile = switch_caller_profile_clone(session, profile);
		new_profile->destination_number = switch_core_session_strdup(session, extension);

		if (dialplan) {
			new_profile->dialplan = switch_core_session_strdup(session, dialplan);
		} else {
			dialplan = new_profile->dialplan;
		}

		if (context) {
			new_profile->context = switch_core_session_strdup(session, context);
		} else {
			context = new_profile->context;
		}

		switch_channel_set_caller_profile(channel, new_profile);
		switch_channel_set_flag(channel, CF_TRANSFER);
		switch_channel_set_state(channel, CS_RING);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Transfer %s to %s[%s@%s]\n", 
						  switch_channel_get_name(channel), dialplan, extension, context); 
		return SWITCH_STATUS_SUCCESS;
	} 

	return SWITCH_STATUS_FALSE;
}


