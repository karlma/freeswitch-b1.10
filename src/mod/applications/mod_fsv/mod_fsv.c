/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2014, Anthony Minessale II <anthm@freeswitch.org>
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
 * Seven Du <dujinfang@gmail.com>
 *
 * mod_fsv -- FS Video File Format
 *
 *
 */

#include <switch.h>


SWITCH_MODULE_LOAD_FUNCTION(mod_fsv_load);
SWITCH_MODULE_DEFINITION(mod_fsv, mod_fsv_load, NULL, NULL);

#define VID_BIT (1 << 31)
#define VERSION 4201

struct file_header {
	int32_t version;
	char video_codec_name[32];
	char video_fmtp[128];
	uint32_t audio_rate;
	uint32_t audio_ptime;
	switch_time_t created;
};

struct record_helper {
	switch_core_session_t *session;
	switch_mutex_t *mutex;
	int fd;
	int up;
    switch_size_t shared_ts;
};

static void *SWITCH_THREAD_FUNC record_video_thread(switch_thread_t *thread, void *obj)
{
	struct record_helper *eh = obj;
	switch_core_session_t *session = eh->session;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_status_t status;
	switch_frame_t *read_frame;
	int bytes;

	eh->up = 1;
	while (switch_channel_ready(channel) && eh->up) {
		status = switch_core_session_read_video_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}

		if (switch_test_flag(read_frame, SFF_CNG)) {
			continue;
		}

		bytes = read_frame->packetlen | VID_BIT;

		switch_mutex_lock(eh->mutex);

		if (write(eh->fd, &bytes, sizeof(bytes)) != (int) sizeof(bytes)) {
			switch_mutex_unlock(eh->mutex);
			break;
		}

		if (write(eh->fd, read_frame->packet, read_frame->packetlen) != (int) read_frame->packetlen) {
			switch_mutex_unlock(eh->mutex);
			break;
		}

		switch_mutex_unlock(eh->mutex);

		switch_core_session_write_video_frame(session, read_frame, SWITCH_IO_FLAG_NONE, 0);
	}
	eh->up = 0;
	return NULL;
}

SWITCH_STANDARD_APP(record_fsv_function)
{
	switch_status_t status;
	switch_frame_t *read_frame;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	struct record_helper eh = { 0 };
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;
	int fd;
	switch_mutex_t *mutex = NULL;
	switch_codec_t codec, *vid_codec;
	switch_codec_implementation_t read_impl = { 0 };
	switch_dtmf_t dtmf = { 0 };
	int count = 0, sanity = 30;

	switch_channel_set_flag(channel, CF_VIDEO_PASSIVE);

	switch_core_session_get_read_impl(session, &read_impl);
	switch_channel_answer(channel);
	switch_core_session_refresh_video(session);

	switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, "");

	while (switch_channel_up(channel) && !switch_channel_test_flag(channel, CF_VIDEO)) {
		switch_yield(10000);

		if (count) count--;

		if (count == 0) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "%s waiting for video.\n", switch_channel_get_name(channel));
			count = 100;
			if (!--sanity) {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "%s timeout waiting for video.\n", 
								  switch_channel_get_name(channel));
				switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Got timeout while waiting for video");
				goto done;
			}
		}
	}
	
	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "%s not ready.\n", switch_channel_get_name(channel));
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Channel not ready");
		goto done;
	}

	if ((fd = open((char *) data, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRUSR | S_IWUSR)) < 0) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "Error opening file %s\n", (char *) data);
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Got error while opening file");
		goto done;
	}

	if (switch_core_codec_init(&codec,
							   "L16",
							   NULL,
							   read_impl.samples_per_second,
							   read_impl.microseconds_per_packet / 1000,
							   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Audio Codec Activation Success\n");
	} else {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Audio Codec Activation Fail\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Audio codec activation failed");
		goto end;
	}

	switch_core_session_set_read_codec(session, &codec);

	if (switch_channel_test_flag(channel, CF_VIDEO)) {
		struct file_header h;
		memset(&h, 0, sizeof(h));
		vid_codec = switch_core_session_get_video_read_codec(session);

		h.version = VERSION;
		h.created = switch_micro_time_now();
		switch_set_string(h.video_codec_name, vid_codec->implementation->iananame);
		if (vid_codec->fmtp_in) {
			switch_set_string(h.video_fmtp, vid_codec->fmtp_in);
		}
		h.audio_rate = read_impl.samples_per_second;
		h.audio_ptime = read_impl.microseconds_per_packet / 1000;

		if (write(fd, &h, sizeof(h)) != sizeof(h)) {
			switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "File write failed");
			goto end;
		}

		switch_mutex_init(&mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
		eh.mutex = mutex;
		eh.fd = fd;
		eh.session = session;
		switch_threadattr_create(&thd_attr, switch_core_session_get_pool(session));
		switch_threadattr_detach_set(thd_attr, 1);
		switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
		switch_thread_create(&thread, thd_attr, record_video_thread, &eh, switch_core_session_get_pool(session));
	}


	while (switch_channel_ready(channel)) {

		status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_SINGLE_READ, 0);

		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			eh.up = 0;
			break;
		}

		switch_ivr_parse_all_events(session);

		//check for dtmf interrupts
		if (switch_channel_has_dtmf(channel)) {
			const char * terminators = switch_channel_get_variable(channel, SWITCH_PLAYBACK_TERMINATORS_VARIABLE);
			switch_channel_dequeue_dtmf(channel, &dtmf);

			if (terminators && !strcasecmp(terminators, "none"))
			{
				terminators = NULL;
			}

			if (terminators && strchr(terminators, dtmf.digit)) {

				char sbuf[2] = {dtmf.digit, '\0'};
				switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, sbuf);
				eh.up = 0;
				break;
			}
		}

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			eh.up = 0;
			break;
		}

		if (switch_test_flag(read_frame, SFF_CNG)) {
			continue;
		}

		if (mutex) {
			switch_mutex_lock(mutex);
		}

		if (write(fd, &read_frame->datalen, sizeof(read_frame->datalen)) != sizeof(read_frame->datalen)) {
			if (mutex) {
				switch_mutex_unlock(mutex);
			}
			break;
		}

		if (write(fd, read_frame->data, read_frame->datalen) != (int) read_frame->datalen) {
			if (mutex) {
				switch_mutex_unlock(mutex);
			}
			break;
		}
        
		if (mutex) {
			switch_mutex_unlock(mutex);
		}

        switch_core_session_write_frame(session, read_frame, SWITCH_IO_FLAG_NONE, 0);
	}

	switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "OK");

  end:

	if (eh.up) {
		while (eh.up) {
			switch_cond_next();
		}
	}

	if (fd > -1) {
		close(fd);
	}


	switch_core_session_set_read_codec(session, NULL);
	switch_core_codec_destroy(&codec);

 done:

	switch_channel_clear_flag(channel, CF_VIDEO_PASSIVE);

}

SWITCH_STANDARD_APP(play_fsv_function)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_frame_t write_frame = { 0 }, vid_frame = {
	0};
	int fd = -1;
    int bytes;
	switch_codec_t codec = { 0 }, vid_codec = {
	0}, *read_vid_codec;
	unsigned char *aud_buffer;
	unsigned char *vid_buffer;
	struct file_header h;
	uint32_t ts = 0, last = 0;
	switch_timer_t timer = { 0 };
	switch_payload_t pt = 0;
	switch_dtmf_t dtmf = { 0 };
	switch_frame_t *read_frame;
	switch_codec_implementation_t read_impl = { 0 };

	switch_channel_set_flag(channel, CF_VIDEO_PASSIVE);

	switch_core_session_refresh_video(session);

	switch_core_session_get_read_impl(session, &read_impl);

	aud_buffer = switch_core_session_alloc(session, SWITCH_RECOMMENDED_BUFFER_SIZE);
	vid_buffer = switch_core_session_alloc(session, SWITCH_RECOMMENDED_BUFFER_SIZE);

	switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, "");

	if ((fd = open((char *) data, O_RDONLY | O_BINARY)) < 0) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "Error opening file %s\n", (char *) data);
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Got error while opening file");
		goto done;
	}

	if (read(fd, &h, sizeof(h)) != sizeof(h)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "Error reading file header\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Got error reading file header");
		goto end;
	}

	if (h.version != VERSION) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "File version does not match!\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "File version does not match!");
		goto end;
	}

	switch_channel_set_variable(channel, "rtp_force_video_fmtp", h.video_fmtp);
	switch_channel_answer(channel);

	if ((read_vid_codec = switch_core_session_get_video_read_codec(session))) {
		pt = read_vid_codec->agreed_pt;
	}

	write_frame.codec = &codec;
	write_frame.data = aud_buffer;
	write_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE;

	vid_frame.codec = &vid_codec;
	vid_frame.packet = vid_buffer;
	vid_frame.data = vid_buffer + 12;
	vid_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE - 12;
	switch_set_flag((&vid_frame), SFF_RAW_RTP);
	switch_set_flag((&vid_frame), SFF_PROXY_PACKET);

	if (switch_core_timer_init(&timer, "soft", read_impl.microseconds_per_packet / 1000,
							   read_impl.samples_per_packet, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Timer Activation Fail\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Timer activation failed!");
		goto end;
	}

	if (switch_core_codec_init(&codec,
							   "L16",
							   NULL,
							   h.audio_rate,
							   h.audio_ptime,
							   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Audio Codec Activation Success\n");
	} else {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Audio Codec Activation Fail\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Audio codec activation failed");
		goto end;
	}

	if (switch_core_codec_init(&vid_codec,
							   h.video_codec_name,
							   NULL,
							   0,
							   0,
							   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Video Codec Activation Success\n");
	} else {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Video Codec Activation Fail\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Video codec activation failed");
		goto end;
	}
	switch_core_session_set_read_codec(session, &codec);

	switch_core_service_session_av(session, SWITCH_FALSE, SWITCH_TRUE);
	
	while (switch_channel_ready(channel)) {

		if (read(fd, &bytes, sizeof(bytes)) != sizeof(bytes)) {
			break;
		}

		if (bytes & VID_BIT) {
			switch_rtp_hdr_t *hdr = vid_frame.packet;
			bytes &= ~VID_BIT;

			/*
			 * Frame is larger than available buffer space. This error is non-recoverable due to the
			 * structure of the .fsv format (no frame header signature to re-sync).
			 */
			if (bytes > ((int) vid_frame.buflen + 12)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Corrupt .fsv video frame header is overflowing read buffer, aborting!\n");
				break;
			}

			if ((vid_frame.packetlen = read(fd, vid_frame.packet, bytes)) != (uint32_t) bytes) {
				break;
			}

			ts = ntohl(hdr->ts);
			if (pt) {
				hdr->pt = pt;
			}
			if (switch_channel_test_flag(channel, CF_VIDEO)) {
				switch_byte_t *data = (switch_byte_t *) vid_frame.packet;

				vid_frame.m = hdr->m;
				vid_frame.timestamp = ts;
				vid_frame.data = data + 12;
				vid_frame.datalen = vid_frame.packetlen - 12;
				switch_core_session_write_video_frame(session, &vid_frame, SWITCH_IO_FLAG_NONE, 0);
			}
			if (ts && last && last != ts) {
				switch_cond_next();
			}
			last = ts;
		} else {
			/*
			 * Frame is larger than available buffer space. This error is non-recoverable due to the
			 * structure of the .fsv format (no frame header signature to re-sync).
			 */
			if (bytes > (int) write_frame.buflen) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Corrupt .fsv audio frame header is overflowing read buffer, aborting!\n");
				break;
			}

			if ((write_frame.datalen = read(fd, write_frame.data, bytes)) <= 0) {
				break;
			}
			switch_core_session_write_frame(session, &write_frame, SWITCH_IO_FLAG_NONE, 0);
			switch_core_timer_next(&timer);

			switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);

			if (switch_channel_test_flag(channel, CF_BREAK)) {
				switch_channel_clear_flag(channel, CF_BREAK);
				break;
			}

			switch_ivr_parse_all_events(session);

			//check for dtmf interrupts
			if (switch_channel_has_dtmf(channel)) {
				const char * terminators = switch_channel_get_variable(channel, SWITCH_PLAYBACK_TERMINATORS_VARIABLE);
				switch_channel_dequeue_dtmf(channel, &dtmf);

				if (terminators && !strcasecmp(terminators, "none"))
				{
					terminators = NULL;
				}								

				if (terminators && strchr(terminators, dtmf.digit)) {

					char sbuf[2] = {dtmf.digit, '\0'};
					switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, sbuf);
					break;
				}
			}
		}
		
	}

	switch_core_thread_session_end(session);

	switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "OK");

  end:

	if (timer.interval) {
		switch_core_timer_destroy(&timer);
	}


	switch_core_session_set_read_codec(session, NULL);


	if (switch_core_codec_ready(&codec)) {
		switch_core_codec_destroy(&codec);
	}

	if (switch_core_codec_ready(&vid_codec)) {
		switch_core_codec_destroy(&vid_codec);
	}

	if (fd > -1) {
		close(fd);
	}

 done:
	switch_channel_clear_flag(channel, CF_VIDEO_PASSIVE);
}

uint8_t art[14][16] = {
	{0x00, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x00},
	{0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00},
	{0x00, 0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x7E, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x00},
	{0x00, 0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x7E, 0x00},
	{0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00},
	{0x00, 0x7E, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x7E, 0x00},
	{0x00, 0x7E, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x00},
	{0x00, 0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00},
	{0x00, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x00},
	{0x00, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x7E, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00}, /*.*/
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*:*/
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /*-*/
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* */
};

void tag(void *buffer, int w, int x, int y, uint8_t n)
{
	int i = 0, j=0;
	uint8_t *p = buffer;

	if (n < 0 || n > 13) return;

	for(i=0; i<8; i++) {
		for (j=0; j<16; j++) {
			*( p + (y + j) * w + (x + i)) = (art[n][j] & 0x80 >> i) ? 0xFF : 0x00;
		}
	}
}

void text(void *buffer, int w, int x, int y, char *s)
{
	while (*s) {
		int index;

		if (x > w - 8) break;

		switch (*s) {
			case '.': index = 10; break;
			case ':': index = 11; break;
			case '-': index = 12; break;
			case ' ': index = 13; break;
			default:
				index = *s - 0x30;
		}

		tag(buffer, w, x, y, index);
		x += 8;
		s++;
	}
}

/*
	\brief play YUV file in I420 (YV12) format
*/
SWITCH_STANDARD_APP(play_yuv_function)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_frame_t vid_frame = { 0 };
	int fd = -1;
	switch_codec_t *codec = NULL;
	unsigned char *vid_buffer;
	// switch_timer_t timer = { 0 };
	switch_dtmf_t dtmf = { 0 };
	switch_frame_t *read_frame;
	uint32_t width = 0, height = 0, size;
	switch_image_t *img = NULL;
	switch_byte_t *yuv = NULL;
	switch_time_t last_video_ts = 0;
	uint32_t timestamp = 0;
	int argc;
	char *argv[3] = { 0 };
	char *mydata = switch_core_session_strdup(session, data);

	if (!switch_channel_test_flag(channel, CF_VIDEO)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Channel %s has no video\n", switch_channel_get_name(channel));
		goto done;
	}

	argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc == 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No file present\n");
		goto done;
	}

	if (argc > 1) width = atoi(argv[1]);
	if (argc > 2) height = atoi(argv[2]);

	width = width ? width : 352;
	height = height ? height : 288;
	size = width * height * 3 / 2;

	img = switch_img_alloc(NULL, SWITCH_IMG_FMT_I420, width, height, 0);

	if (!img) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Memory Error\n");
		goto end;
	}

	yuv = img->planes[SWITCH_PLANE_PACKED];

	// switch_channel_set_flag(channel, CF_VIDEO_PASSIVE);
	switch_channel_clear_flag(channel, CF_VIDEO_ECHO);

	vid_buffer = switch_core_session_alloc(session, SWITCH_RECOMMENDED_BUFFER_SIZE);

	switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, "");

	if ((fd = open(argv[0], O_RDONLY | O_BINARY)) < 0) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "Error opening file %s\n", (char *) data);
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Got error while opening file");
		goto end;
	}

	if (read(fd, yuv, size) != size) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "Error reading file\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Got error reading file");
		goto end;
	}

	close(fd);
	fd = -1;

	switch_channel_answer(channel);

	codec = switch_core_session_get_video_write_codec(session);

	vid_frame.codec = codec;
	vid_frame.packet = vid_buffer;
	vid_frame.data = vid_buffer + 12;
	vid_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE - 12;
	switch_set_flag((&vid_frame), SFF_RAW_RTP);
	// switch_set_flag((&vid_frame), SFF_PROXY_PACKET);

	while (switch_channel_ready(channel)) {
		switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);

		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			break;
		}

		switch_ivr_parse_all_events(session);

		//check for dtmf interrupts
		if (switch_channel_has_dtmf(channel)) {
			const char * terminators = switch_channel_get_variable(channel, SWITCH_PLAYBACK_TERMINATORS_VARIABLE);
			switch_channel_dequeue_dtmf(channel, &dtmf);

			if (terminators && !strcasecmp(terminators, "none")) {
				terminators = NULL;
			}

			if (terminators && strchr(terminators, dtmf.digit)) {
				char sbuf[2] = {dtmf.digit, '\0'};

				switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, sbuf);
				break;
			}
		}

		if (read_frame) switch_core_session_write_frame(session, read_frame, SWITCH_IO_FLAG_NONE, 0);

		{	/* video part */
			uint32_t encoded_data_len = 1500;
			switch_frame_t *frame = &vid_frame;
			switch_time_t now = switch_micro_time_now() / 1000;
			char ts_str[33];
			long delta;

			if (last_video_ts == 0) last_video_ts = now;

			delta = now - last_video_ts;

			if (delta > 0) {
				timestamp += delta * 90;
				last_video_ts = now;
			}

			sprintf(ts_str, "%u", timestamp);
			text(img->planes[SWITCH_PLANE_PACKED], width, 20, 20, ts_str);
			vid_frame.img = img;
			switch_core_codec_encode_video(codec, &vid_frame);

			while(encoded_data_len) {
				// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "encoded: %s [%d] flag=%d ts=%lld\n", codec->implementation->iananame, encoded_data_len, flag, last_video_ts);

				frame->timestamp = timestamp;

				if (1) { // we can remove this when ts and marker full passed in core
					/* set correct mark and ts */
					switch_rtp_hdr_t *rtp = (switch_rtp_hdr_t *)frame->packet;

					memset(rtp, 0, 12);
					rtp->version = 2;
					rtp->m = frame->m;
					rtp->ts = htonl(timestamp);
					rtp->ssrc = (uint32_t) ((intptr_t) rtp + (uint32_t) switch_epoch_time_now(NULL));
					// rtp->ssrc = 0x11223344;
				}

				switch_core_session_write_video_frame(session, frame, SWITCH_IO_FLAG_NONE, 0);

				vid_frame.datalen = 1500;
				switch_core_codec_encode_video(codec, &vid_frame);
			}
		}
	}

	switch_core_thread_session_end(session);
	switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "OK");

  end:

	if (fd > -1) {
		close(fd);
	}

	switch_img_free(img);

 done:
	// switch_channel_clear_flag(channel, CF_VIDEO_PASSIVE);
	switch_channel_set_flag(channel, CF_VIDEO_ECHO);
}


SWITCH_STANDARD_APP(decode_video_function)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_codec_t *codec = NULL;
	switch_dtmf_t dtmf = { 0 };
	switch_frame_t *frame;
	uint32_t width = 0, height = 0;
	switch_time_t last = switch_micro_time_now();
	uint32_t max_pictures = 0;
	uint32_t decoded_pictures = 0;

	if (!zstr(data)) max_pictures = atoi(data);

	if (!switch_channel_test_flag(channel, CF_VIDEO)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Channel %s has no video\n", switch_channel_get_name(channel));
		goto done;
	}

	switch_channel_set_flag(channel, CF_VIDEO_PASSIVE);
	switch_channel_clear_flag(channel, CF_VIDEO_ECHO);
	switch_channel_answer(channel);
	switch_core_session_refresh_video(session);

	switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, "");


	codec = switch_core_session_get_video_read_codec(session);
	if (!codec) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Channel has no video read codec\n");
		goto done;
	}

	while (switch_channel_ready(channel)) {
		switch_core_session_read_video_frame(session, &frame, SWITCH_IO_FLAG_NONE, 0);

		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			break;
		}

		switch_ivr_parse_all_events(session);

		//check for dtmf interrupts
		if (switch_channel_has_dtmf(channel)) {
			const char * terminators = switch_channel_get_variable(channel, SWITCH_PLAYBACK_TERMINATORS_VARIABLE);
			switch_channel_dequeue_dtmf(channel, &dtmf);

			if (terminators && !strcasecmp(terminators, "none")) {
				terminators = NULL;
			}

			if (terminators && strchr(terminators, dtmf.digit)) {
				char sbuf[2] = {dtmf.digit, '\0'};

				switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, sbuf);
				break;
			}
		}

		if (frame && frame->datalen > 0) {
			switch_core_session_write_video_frame(session, frame, SWITCH_IO_FLAG_NONE, 0);
		} else {
			continue;
		}

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "seq: %d ts: %ld len: %4d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x mark: %d %s\n",
			frame->seq, frame->timestamp, frame->datalen,
			*((uint8_t *)frame->data), *((uint8_t *)frame->data + 1),
			*((uint8_t *)frame->data + 2), *((uint8_t *)frame->data + 3),
			*((uint8_t *)frame->data + 4), *((uint8_t *)frame->data + 5),
			*((uint8_t *)frame->data + 6), *((uint8_t *)frame->data + 7),
			*((uint8_t *)frame->data + 8), *((uint8_t *)frame->data + 9),
			*((uint8_t *)frame->data + 10), frame->m, switch_test_flag(frame, SFF_CNG) ? " CNG" : "");

		if (switch_test_flag(frame, SFF_CNG) || frame->datalen < 3) {
			continue;
		}

		if ( 1 ) {	/* video part */
			switch_core_codec_decode_video(codec, frame);

			if ((switch_test_flag(frame, SFF_WAIT_KEY_FRAME))) {
				switch_time_t now = switch_micro_time_now();
				if (now - last > 3000000) {
					switch_core_session_refresh_video(session);
					last = now;
				}
				continue;
			}

			if (frame->img) {
				if (frame->img->d_w > 0 && !width) {
					width = frame->img->d_w;
					switch_channel_set_variable_printf(channel, "video_width", "%d", width);
				}

				if (frame->img->d_h > 0 && !height) {
					height = frame->img->d_h;
					switch_channel_set_variable_printf(channel, "video_height", "%d", height);
				}

				decoded_pictures++;
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "picture#%d %dx%d\n", decoded_pictures, frame->img->d_w, frame->img->d_h);

				if (max_pictures && (decoded_pictures >= max_pictures)) {
					break;
				}
			}
		}
	}

	switch_core_thread_session_end(session);
	switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "OK");

 done:
	switch_channel_clear_flag(channel, CF_VIDEO_PASSIVE);
	switch_channel_set_flag(channel, CF_VIDEO_ECHO);
}


/* File Interface*/

struct fsv_file_context {
	switch_file_t *fd;
	char *path;
	switch_mutex_t *mutex;
};

typedef struct fsv_file_context fsv_file_context;

static switch_status_t fsv_file_open(switch_file_handle_t *handle, const char *path)
{
	fsv_file_context *context;
	char *ext;
	unsigned int flags = 0;

	if ((ext = strrchr(path, '.')) == 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Format\n");
		return SWITCH_STATUS_GENERR;
	}
	ext++;

	if ((context = switch_core_alloc(handle->memory_pool, sizeof(*context))) == 0) {
		return SWITCH_STATUS_MEMERR;
	}

	switch_mutex_init(&context->mutex, SWITCH_MUTEX_NESTED, handle->memory_pool);

	if (switch_test_flag(handle, SWITCH_FILE_FLAG_WRITE)) {
		flags |= SWITCH_FOPEN_WRITE | SWITCH_FOPEN_CREATE;
		if (switch_test_flag(handle, SWITCH_FILE_WRITE_APPEND) || switch_test_flag(handle, SWITCH_FILE_WRITE_OVER)) {
			flags |= SWITCH_FOPEN_READ;
		} else {
			flags |= SWITCH_FOPEN_TRUNCATE;
		}

	}

	if (switch_test_flag(handle, SWITCH_FILE_FLAG_READ)) {
		flags |= SWITCH_FOPEN_READ;
	}

	if (switch_file_open(&context->fd, path, flags, SWITCH_FPROT_UREAD | SWITCH_FPROT_UWRITE, handle->memory_pool) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error opening %s\n", path);
		return SWITCH_STATUS_GENERR;
	}

	context->path = switch_core_strdup(handle->memory_pool, path);

	if (switch_test_flag(handle, SWITCH_FILE_WRITE_APPEND)) {
		int64_t samples = 0;
		switch_file_seek(context->fd, SEEK_END, &samples);
		handle->pos = samples;
	}

	handle->samples = 0;
	// handle->samplerate = 8000;
	// handle->channels = 1;
	handle->format = 0;
	handle->sections = 0;
	handle->seekable = 0;
	handle->speed = 0;
	handle->pos = 0;
	handle->private_info = context;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Opening File [%s] %dhz\n", path, handle->samplerate);

	if (switch_test_flag(handle, SWITCH_FILE_FLAG_WRITE)) {
		struct file_header h;
		size_t len = sizeof(h);

		memset(&h, 0, sizeof(h));

		h.version = VERSION;
		h.created = switch_micro_time_now();

		switch_set_string(h.video_codec_name, "H264"); /* FIXME: hard coded */

		h.audio_rate = handle->samplerate;
		h.audio_ptime = 20; /* FIXME: hard coded */

		if (switch_file_write(context->fd, &h, &len) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "File write failed\n");
			return SWITCH_STATUS_GENERR;
		}
	} else if (switch_test_flag(handle, SWITCH_FILE_FLAG_READ)) {
		struct file_header h;
		size_t size = sizeof(h);

		if (switch_file_read(context->fd, &h, &size) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Error reading file header\n");
			return SWITCH_STATUS_GENERR;
		}

		if (h.version != VERSION) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "File version does not match!\n");
			return SWITCH_STATUS_GENERR;
		}

		handle->samplerate = h.audio_rate;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t fsv_file_truncate(switch_file_handle_t *handle, int64_t offset)
{
	fsv_file_context *context = handle->private_info;
	switch_status_t status;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "truncate file [%s]\n", context->path);

	if ((status = switch_file_trunc(context->fd, offset)) == SWITCH_STATUS_SUCCESS) {
		handle->pos = 0;
	}

	return status;

}

static switch_status_t fsv_file_close(switch_file_handle_t *handle)
{
	fsv_file_context *context = handle->private_info;

	if (context->fd) {
		switch_file_close(context->fd);
		context->fd = NULL;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t fsv_file_seek(switch_file_handle_t *handle, unsigned int *cur_sample, int64_t samples, int whence)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Seek not implemented\n");
	return SWITCH_STATUS_FALSE;
}

static switch_status_t fsv_file_read(switch_file_handle_t *handle, void *data, size_t *len)
{
	switch_status_t status;
	size_t need = *len;
	uint32_t size;
	size_t bytes = sizeof(size);
	fsv_file_context *context = handle->private_info;

again:

	status = switch_file_read(context->fd, &size, &bytes);

	if (status != SWITCH_STATUS_SUCCESS) {
		goto end;
	}

	if (size & VID_BIT) { /* video */
		*len = size & ~VID_BIT;
		/* TODO: read video data */
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "discarding video data %d\n", (int)*len);
		status = switch_file_read(context->fd, data, len);

		if (status != SWITCH_STATUS_SUCCESS) {
			goto end;
		}

		handle->pos += *len + bytes;
		goto again;
	}

	if (size > need) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "size %u > need %u\n", (unsigned int)size, (int)need);
		goto end;
	}

	*len = size;

	status = switch_file_read(context->fd, data, len);

	*len /= 2;

end:

	return status;
}

static switch_status_t fsv_file_write(switch_file_handle_t *handle, void *data, size_t *len)
{
	uint32_t datalen = *len * sizeof(int16_t);
	size_t size;
	switch_status_t status;
	int16_t *xdata = data;
	int max_datasize = handle->samplerate / 8000 * 160;

	fsv_file_context *context = handle->private_info;

	/* switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "writing: %ld %d %x\n", (long)(*len), handle->channels, handle->flags); */

	if (*len > max_datasize) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "You are asking to write %d bytes of data which is not supported. Please set enable_file_write_buffering=false to use .fsv format\n", (int)(*len));
		return SWITCH_STATUS_GENERR;
	}

	if (handle->channels > 1) {
		int i, j;
		int32_t mixed = 0;
		for (i=0; i<*len; i++) {
			for (j = 0; j < handle->channels; j++) {
				mixed += xdata[i * handle->channels + j];
			}
			switch_normalize_to_16bit(mixed);
			xdata[i] = (uint16_t)mixed;
		}
	}

	switch_mutex_lock(context->mutex);

	size = sizeof(datalen);
	if (switch_file_write(context->fd, &datalen, &size) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "write error\n");
		return SWITCH_STATUS_FALSE;
	}

	*len = datalen;
	status =  switch_file_write(context->fd, data, len);
	switch_mutex_unlock(context->mutex);

	handle->sample_count += *len / sizeof(int16_t);

	return status;
}

static switch_status_t fsv_file_write_video(switch_file_handle_t *handle, void *data, size_t *len)
{
	uint32_t datalen = (uint32_t)*len;
	uint32_t bytes = datalen | VID_BIT;
	size_t size;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	fsv_file_context *context = handle->private_info;
	/* switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "writing: %ld\n", (long)(*len)); */

	switch_mutex_lock(context->mutex);

	size = sizeof(bytes);
	if (switch_file_write(context->fd, &bytes, &size) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "write error\n");
		return SWITCH_STATUS_FALSE;
	}

	*len = datalen;
	status =  switch_file_write(context->fd, data, len);
	switch_mutex_unlock(context->mutex);

	*len /= 2;
	handle->sample_count += *len;

	return status;
}

static switch_status_t fsv_file_set_string(switch_file_handle_t *handle, switch_audio_col_t col, const char *string)
{
	return SWITCH_STATUS_FALSE;
}

static switch_status_t fsv_file_get_string(switch_file_handle_t *handle, switch_audio_col_t col, const char **string)
{
	return SWITCH_STATUS_FALSE;
}

/* Registration */

static char *supported_formats[2] = { 0 };

SWITCH_MODULE_LOAD_FUNCTION(mod_fsv_load)
{
	switch_application_interface_t *app_interface;
	switch_file_interface_t *file_interface;

	supported_formats[0] = "fsv";

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	file_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_FILE_INTERFACE);
	file_interface->interface_name = modname;
	file_interface->extens = supported_formats;
	file_interface->file_open = fsv_file_open;
	file_interface->file_close = fsv_file_close;
	file_interface->file_truncate = fsv_file_truncate;
	file_interface->file_read = fsv_file_read;
	file_interface->file_write = fsv_file_write;
	file_interface->file_write_video = fsv_file_write_video;
	file_interface->file_seek = fsv_file_seek;
	file_interface->file_set_string = fsv_file_set_string;
	file_interface->file_get_string = fsv_file_get_string;

	SWITCH_ADD_APP(app_interface, "play_fsv", "play a fsv file", "play a fsv file", play_fsv_function, "<file>", SAF_NONE);
	SWITCH_ADD_APP(app_interface, "record_fsv", "record an fsv file", "record an fsv file", record_fsv_function, "<file>", SAF_NONE);
	SWITCH_ADD_APP(app_interface, "play_yuv", "play a yvv file", "play a yvv file in I420 format", play_yuv_function, "<file> [width] [height]", SAF_NONE);
	SWITCH_ADD_APP(app_interface, "decode_video", "decode picture", "decode picture and dump to log", decode_video_function, "[max_pictures]", SAF_NONE);

	/* indicate that the module should continue to be loaded */
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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
