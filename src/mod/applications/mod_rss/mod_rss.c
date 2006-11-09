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
 * mod_rss.c -- RSS Browser
 *
 */
#include <switch.h>

static const char modname[] = "mod_rss";

typedef enum {
	SFLAG_INSTRUCT = (1 << 0),
	SFLAG_INFO = (1 << 1),
	SFLAG_MAIN = (1 << 2)
} SFLAGS;

/* helper object */
struct dtmf_buffer {
	int32_t index;
	uint32_t flags;
	int32_t speed;
	char voice[80];
	switch_speech_handle_t *sh;
};

#define TTS_MEAN_SPEED 170
#define TTS_MAX_ENTRIES 99
#define TTS_DEFAULT_ENGINE "cepstral"
#define TTS_DEFAULT_VOICE "david"

#define MATCH_COUNT

struct rss_entry {
	uint8_t inuse;
	char *title_txt;
	char *description_txt;
	char *subject_txt;
	char *dept_txt;
};

#ifdef MATCH_COUNT
static uint32_t match_count(char *str, uint32_t max)
{
	char tstr[80] = "";
	uint32_t matches = 0, x = 0;
	uint32_t len = (uint32_t)strlen(str);

	for (x = 0; x < max ; x++) {
		snprintf(tstr, sizeof(tstr), "%u", x);
		if (!strncasecmp(str, tstr, len)) {
			matches++;
		}
	}
	return matches;
}
#endif


/*
  dtmf handler function you can hook up to be executed when a digit is dialed during playback 
  if you return anything but SWITCH_STATUS_SUCCESS the playback will stop.
*/
static switch_status_t on_dtmf(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)

{
	switch (itype) {
	case SWITCH_INPUT_TYPE_DTMF: {
		char *dtmf = (char *) input;
		struct dtmf_buffer *dtb;
		dtb = (struct dtmf_buffer *) buf;
	
		switch(*dtmf) {
		case '#':
			switch_set_flag(dtb, SFLAG_MAIN);
			return SWITCH_STATUS_BREAK;
		case '6':
			dtb->index++;
			return SWITCH_STATUS_BREAK;
		case '4':
			dtb->index--;
			return SWITCH_STATUS_BREAK;
		case '*':
			if (switch_test_flag(dtb->sh, SWITCH_SPEECH_FLAG_PAUSE)) {
				switch_clear_flag(dtb->sh, SWITCH_SPEECH_FLAG_PAUSE);
			} else {
				switch_set_flag(dtb->sh, SWITCH_SPEECH_FLAG_PAUSE);
			}
			break;
		case '5':
			switch_core_speech_text_param_tts(dtb->sh, "voice", "next");
			switch_set_flag(dtb, SFLAG_INFO);
			return SWITCH_STATUS_BREAK;
			break;
		case '9':
			switch_core_speech_text_param_tts(dtb->sh, "voice", dtb->voice);
			switch_set_flag(dtb, SFLAG_INFO);
			return SWITCH_STATUS_BREAK;
			break;
		case '2':
			if (dtb->speed < 260) {
				dtb->speed += 30;
				switch_core_speech_numeric_param_tts(dtb->sh, "speech/rate", dtb->speed);
				switch_set_flag(dtb, SFLAG_INFO);
				return SWITCH_STATUS_BREAK;
			}
			break;
		case '7':
			dtb->speed = TTS_MEAN_SPEED;
			switch_core_speech_numeric_param_tts(dtb->sh, "speech/rate", dtb->speed);
			switch_set_flag(dtb, SFLAG_INFO);
			return SWITCH_STATUS_BREAK;
		case '8':
			if (dtb->speed > 80) {
				dtb->speed -= 30;
				switch_core_speech_numeric_param_tts(dtb->sh, "speech/rate", dtb->speed);
				switch_set_flag(dtb, SFLAG_INFO);
				return SWITCH_STATUS_BREAK;
			}
			break;
		case '0':
			switch_set_flag(dtb, SFLAG_INSTRUCT);
			return SWITCH_STATUS_BREAK;
		}
	}
		break;
	default:
		break;
	}
	return SWITCH_STATUS_SUCCESS;
}

static void rss_function(switch_core_session_t *session, char *data)
{
	switch_channel_t *channel;
	switch_status_t status;
	const char *err = NULL;
	struct dtmf_buffer dtb = {0};
	switch_xml_t xml = NULL, item, xchannel = NULL;
	struct rss_entry entries[TTS_MAX_ENTRIES] = {{0}};
	uint32_t i = 0;
	char *title_txt = "", *description_txt = "", *rights_txt = "";
	switch_codec_t speech_codec, *codec = switch_core_session_get_read_codec(session);
	char *engine = TTS_DEFAULT_ENGINE;
	char *voice = TTS_DEFAULT_VOICE;
	char *timer_name = NULL;
	switch_speech_handle_t sh;
    switch_speech_flag_t flags = SWITCH_SPEECH_FLAG_NONE;
	switch_core_thread_session_t thread_session;
	uint32_t rate, interval = 20;
	int stream_id = 0;
	switch_timer_t timer = {0}, *timerp = NULL;
	uint32_t last;
	char *mydata = NULL;
	char *filename = NULL;
	char *argv[3], *feed_list[TTS_MAX_ENTRIES] = {0} , *feed_names[TTS_MAX_ENTRIES] = {0};
	int argc, feed_index = 0;
	char *cf = "rss.conf";
    switch_xml_t cfg, cxml, feeds, feed;
	char buf[1024];
	int32_t jumpto = -1;
	uint32_t matches = 0;

	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);


	if (!(cxml = switch_xml_open_cfg(cf, &cfg, NULL))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
        return;
    }

	if ((feeds = switch_xml_child(cfg, "feeds"))) {
		for (feed = switch_xml_child(feeds, "feed"); feed; feed = feed->next) {
			char *name = (char *) switch_xml_attr_soft(feed, "name");

			if (!name) {
				name = "Error No Name.";
			}

			feed_names[feed_index] = switch_core_session_strdup(session, name);
			feed_list[feed_index] = switch_core_session_strdup(session, feed->txt);
			feed_index++;
		}
	}

	switch_xml_free(cxml);
	
	switch_channel_answer(channel);



	if (!switch_strlen_zero(data)) {
		if ((mydata = switch_core_session_strdup(session, data))) {
			argc = switch_separate_string(mydata, ' ', argv, sizeof(argv)/sizeof(argv[0]));

			if (argv[0]) {
				engine = argv[0];
				if (argv[1]) {
					voice = argv[1];
					if (argv[2]) {
						jumpto = atoi(argv[2]);
					}
				}
			}
		}
	}

	if (!feed_index) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "No Feeds Specified!\n");
		return;
	}

	if (codec) {
		rate = codec->implementation->samples_per_second;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Codec Error!\n");
		return;
	}

	memset(&sh, 0, sizeof(sh));
	if (switch_core_speech_open(&sh,
								engine,
								voice,
								rate,
								&flags,
								switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid TTS module!\n");
		return;
	}
	
	if (switch_core_codec_init(&speech_codec,
							   "L16",
							   NULL,
							   (int)rate,
							   interval,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL,
							   switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activated\n");
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activation Failed L16@%uhz 1 channel %dms\n", rate, interval);
		flags = 0;
		switch_core_speech_close(&sh, &flags);
		return;
	}
	
	if (timer_name) {
		if (switch_core_timer_init(&timer, timer_name, interval, (int)(rate / 50), switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "setup timer failed!\n");
			switch_core_codec_destroy(&speech_codec);
			flags = 0;
			switch_core_speech_close(&sh, &flags);
			return;
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "setup timer success %u bytes per %d ms!\n", (rate / 50)*2, interval);

		/* start a thread to absorb incoming audio */
		for (stream_id = 0; stream_id < switch_core_session_get_stream_count(session); stream_id++) {
			switch_core_service_session(session, &thread_session, stream_id);
		}
		timerp = &timer;
	}


	while (switch_channel_ready(channel)) {
		int32_t len = 0, idx = 0;
		char cmd[3];
	main_menu:
		filename = NULL;
		len = idx = 0;
		*cmd = '\0';
		title_txt = description_txt = rights_txt = "";

		if (jumpto > -1) {
			snprintf(cmd, sizeof(cmd), "%d", jumpto);
			jumpto = -1;
		} else {
			switch_core_speech_flush_tts(&sh);
#ifdef MATCH_COUNT
			snprintf(buf + len, sizeof(buf) - len, 
					 ",<break time=\"500ms\"/>Main Menu. <break time=\"600ms\"/> "
					 "Select one of the following news sources, or press 0 to exit. "
					 ",<break time=\"600ms\"/>");
#else
			snprintf(buf + len, sizeof(buf) - len, 
					 ",<break time=\"500ms\"/>Main Menu. <break time=\"600ms\"/> "
					 "Select one of the following news sources, followed by the pound key or press 0 to exit. "
					 ",<break time=\"600ms\"/>");
#endif
			len = (int32_t)strlen(buf);

			for (idx = 0; idx < feed_index; idx++) {
				snprintf(buf + len, sizeof(buf) - len, "%d: %s. <break time=\"600ms\"/>", idx + 1, feed_names[idx]);
				len = (int32_t)strlen(buf);
			}


			snprintf(buf + len, sizeof(buf) - len, "<break time=\"2000ms\"/>");
			len = (int32_t)strlen(buf);

			status = switch_ivr_speak_text_handle(session,
												  &sh,
												  &speech_codec,
												  timerp,
												  NULL,
												  buf,
												  cmd,
												  sizeof(cmd));
			if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
				goto finished;
			}
		}
		if (!switch_strlen_zero(cmd)) {
			int32_t i;
			char *p;

			if (strchr(cmd, '0')) {
				break;
			}

			if ((p=strchr(cmd, '#'))) {
				*p = '\0';
#ifdef MATCH_COUNT
				/* Hmmm... I know there are no more matches so I don't *need* them to press pound but 
				   I already told them to press it.  Will this confuse people or not?  Let's make em press 
				   pound unless this define is enabled for now.
				*/
			} else if (match_count(cmd, feed_index) > 1) {
#else
			} else {
#endif
				char term;
				char *cp;
				int blen = sizeof(cmd) - (int)strlen(cmd);

				cp = cmd + blen;
				switch_ivr_collect_digits_count(session, cp, blen, blen, "#", &term, 5000);
			}
			
			i = atoi(cmd) - 1;

			if (i > -1 && i < feed_index) {
				filename = feed_list[i];
			} else if (matches > 1) {
				
			} else {
				status = switch_ivr_speak_text_handle(session,
													  &sh,
													  &speech_codec,
													  timerp,
													  NULL,
													  "I'm sorry. That is an Invalid Selection. ",
													  NULL,
													  0);
				if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
					goto finished;
				}
			}
		}
	
		if (!filename) {
			continue;
		}
		


		if (!(xml = switch_xml_parse_file(filename))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", filename);
			goto finished;
		}

		err = switch_xml_error(xml);

		if (!switch_strlen_zero(err)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Error [%s]\n", err);
			goto finished;
		}

		if ((xchannel = switch_xml_child(xml, "channel"))) {
			switch_xml_t title, description, rights;

			if ((title = switch_xml_child(xchannel, "title"))) {
				title_txt = title->txt;
			}
			
			if ((description = switch_xml_child(xchannel, "description"))) {
				description_txt = description->txt;
			}

			if ((rights = switch_xml_child(xchannel, "dc:rights"))) {
				rights_txt = rights->txt;
			}
		}


		if (!(item = switch_xml_child(xml, "item"))) {
			if (xchannel) {
				item = switch_xml_child(xchannel, "item");
			}
		}

		memset(entries, 0, sizeof(entries));

		for (i = 0; item; item = item->next) {
			switch_xml_t title, description, subject, dept;
			char *p;

			entries[i].inuse = 1;
			entries[i].title_txt = NULL;
			entries[i].description_txt = NULL;
			entries[i].subject_txt = NULL;
			entries[i].dept_txt = NULL;


			if ((title = switch_xml_child(item, "title"))) {
				entries[i].title_txt = title->txt;
			}
		
			if ((description = switch_xml_child(item, "description"))) {
				char *t, *e;
				entries[i].description_txt = description->txt;
				for(;;) {
					if (!(t = strchr(entries[i].description_txt, '<'))) {
						break;
					}
					if (!(e = strchr(t, '>'))) {
						break;
					}
				
					memset(t, 32, ++e-t);
				}
			}

			if ((subject = switch_xml_child(item, "dc:subject"))) {
				entries[i].subject_txt = subject->txt;
			}

			if ((dept = switch_xml_child(item, "slash:department"))) {
				entries[i].dept_txt = dept->txt;
			}

			if (entries[i].description_txt && (p = strchr(entries[i].description_txt, '<'))) {
				*p = '\0';
			}

#ifdef _STRIP_SOME_CHARS_
			for(p = entries[i].description_txt; *p; p++) {
				if (*p == '\'' || *p == '"' || *p == ':') {
					*p = ' ';
				}
			}
#endif

			i++;
		}

		if (switch_channel_ready(channel)) {
			switch_time_exp_t tm;
			char date[80] = "";
			switch_size_t retsize;
			char cmd[5] = "";

			switch_time_exp_lt(&tm, switch_time_now());
			switch_strftime(date, &retsize, sizeof(date), "%I:%M %p", &tm);


			snprintf(buf, sizeof(buf), ",<break time=\"500ms\"/>%s. %s. %s. local time: %s, Press 0 for options, 5 to change voice, or pound to return to the main menu. ", 
					 title_txt, description_txt, rights_txt, date);
			status = switch_ivr_speak_text_handle(session,
												  &sh,
												  &speech_codec,
												  timerp,
												  NULL,
												  buf,
												  cmd,
												  sizeof(cmd));
			if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
				goto finished;
			}
			if (!switch_strlen_zero(cmd)) {
				switch (*cmd) {
				case '0':
					switch_set_flag(&dtb, SFLAG_INSTRUCT);
					break;
				case '#':
					goto main_menu;
					break;
				}
			}
		}

		for(last = 0; last < TTS_MAX_ENTRIES; last++) {
			if (!entries[last].inuse) {
				last--;
				break;
			}
		}

		dtb.index = 0;
		dtb.sh = &sh;
		dtb.speed = TTS_MEAN_SPEED;
		switch_set_flag(&dtb, SFLAG_INFO);
		switch_copy_string(dtb.voice, voice, sizeof(dtb.voice));
		while(entries[0].inuse && switch_channel_ready(channel)) {
			while(switch_channel_ready(channel)) {
				uint8_t cont = 0;

				if (dtb.index >= TTS_MAX_ENTRIES) {
					dtb.index = 0;
				}
				if (dtb.index < 0) {
					dtb.index = last;
				}

				if (!entries[dtb.index].inuse) {
					dtb.index = 0;
					continue;
				}
				if (switch_channel_ready(channel)) {
					char buf[1024] = "";
					uint32_t len = 0;

					if (switch_test_flag(&dtb, SFLAG_MAIN)) {
						switch_clear_flag(&dtb, SFLAG_MAIN);
						goto main_menu;
					}
					if (switch_test_flag(&dtb, SFLAG_INFO)) {
						switch_clear_flag(&dtb, SFLAG_INFO);
						snprintf(buf + len, sizeof(buf) - len,
								 "%s %s. I am speaking at %u words per minute. ",
								 sh.engine,
								 sh.voice,
								 dtb.speed
								 );
						len = (uint32_t)strlen(buf);
					}

					if (switch_test_flag(&dtb, SFLAG_INSTRUCT)) {
						switch_clear_flag(&dtb, SFLAG_INSTRUCT);
						cont = 1;
						snprintf(buf + len, sizeof(buf) - len,
								 "Press star to pause or resume speech. "
								 "To go to the next item, press six. "
								 "To go back, press 4. "
								 "Press two to go faster, eight to slow down, or 7 to resume normal speed. "
								 "To change voices, press five. To restore the original voice press 9. "
								 "To hear these options again, press zero or press pound to return to the main menu. ");
					} else {
						snprintf(buf + len, sizeof(buf) - len, "Story %d. ", dtb.index + 1);
						len = (uint32_t)strlen(buf);

						if (entries[dtb.index].subject_txt) {
							snprintf(buf + len, sizeof(buf) - len, "Subject: %s. ", entries[dtb.index].subject_txt);
							len = (uint32_t)strlen(buf);
						}
				
						if (entries[dtb.index].dept_txt) {
							snprintf(buf + len, sizeof(buf) - len, "From the %s department. ", entries[dtb.index].dept_txt);
							len = (uint32_t)strlen(buf);
						}

						if (entries[dtb.index].title_txt) {
							snprintf(buf + len, sizeof(buf) - len, "%s", entries[dtb.index].title_txt);
							len = (uint32_t)strlen(buf);
						}
					}
					switch_core_speech_flush_tts(&sh);
					status = switch_ivr_speak_text_handle(session,
														  &sh,
														  &speech_codec,
														  timerp,
														  on_dtmf,
														  buf,
														  &dtb,
														  sizeof(dtb));
					if (status == SWITCH_STATUS_BREAK) {
						continue;
					} else if (status != SWITCH_STATUS_SUCCESS) {
						goto finished;
					}

					if (cont) {
						cont = 0;
						continue;
					}
					
					if (entries[dtb.index].description_txt) {
						status = switch_ivr_speak_text_handle(session,
															  &sh,
															  &speech_codec,
															  timerp,
															  on_dtmf,
															  entries[dtb.index].description_txt,
															  &dtb,
															  sizeof(dtb));
					}
					if (status == SWITCH_STATUS_BREAK) {
						continue;
					} else if (status != SWITCH_STATUS_SUCCESS) {
						goto finished;
					}
				}

				dtb.index++;
			}
		}
	}

 finished:
	switch_core_speech_close(&sh, &flags);
	switch_core_codec_destroy(&speech_codec);

	if (timerp) {
		/* End the audio absorbing thread */
		switch_core_thread_session_end(&thread_session);
		switch_core_timer_destroy(&timer);
	}


	switch_xml_free(xml);

	switch_core_session_reset(session);
}

static const switch_application_interface_t rss_application_interface = {
	/*.interface_name */ "rss",
	/*.application_function */ rss_function,
	NULL, NULL, NULL,
	/*.next*/ NULL
};


static switch_loadable_module_interface_t rss_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ NULL,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ NULL,
	/*.application_interface */ &rss_application_interface,
	/*.api_interface */ NULL,
	/*.file_interface */ NULL,
	/*.speech_interface */ NULL,
	/*.directory_interface */ NULL
};


SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface, char *filename)
{
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = &rss_module_interface;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

