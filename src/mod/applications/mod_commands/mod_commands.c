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
 * Johny Kadarisman <jkr888@gmail.com>
 * Paul Tinsley <jackhammer@gmail.com>
 * Marcel Barbulescu <marcelbarbulescu@gmail.com>
 *
 * 
 * mod_commands.c -- Misc. Command Module
 *
 */
#include <switch.h>
#include <switch_version.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_commands_load);
SWITCH_MODULE_DEFINITION(mod_commands, mod_commands_load, NULL, NULL);

SWITCH_STANDARD_API(status_function)
{
	uint8_t html = 0;
	switch_core_time_duration_t duration;
	char *http = NULL;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	switch_core_measure_time(switch_core_uptime(), &duration);

	if (stream->event) {
		http = switch_event_get_header(stream->event, "http-host");
	}

	if (http || (cmd && strstr(cmd, "html"))) {
		html = 1;
		stream->write_function(stream, "<h1>FreeSWITCH Status</h1>\n<b>");
	}

	stream->write_function(stream,
						   "UP %u year%s, %u day%s, %u hour%s, %u minute%s, %u second%s, %u millisecond%s, %u microsecond%s\n",
						   duration.yr, duration.yr == 1 ? "" : "s", duration.day, duration.day == 1 ? "" : "s",
						   duration.hr, duration.hr == 1 ? "" : "s", duration.min, duration.min == 1 ? "" : "s",
						   duration.sec, duration.sec == 1 ? "" : "s", duration.ms, duration.ms == 1 ? "" : "s", duration.mms,
						   duration.mms == 1 ? "" : "s");

	stream->write_function(stream, "%"SWITCH_SIZE_T_FMT" sessions since startup\n", switch_core_session_id() - 1 );
	stream->write_function(stream, "%d sessions\n", switch_core_session_count());

	if (html) {
		stream->write_function(stream, "</b>\n");
	}

	if (cmd && strstr(cmd, "refresh=")) {
		char *refresh = strchr(cmd, '=');
		if (refresh) {
			int r;
			refresh++;
			r = atoi(refresh);
			if (r > 0) {
				stream->write_function(stream, "<META HTTP-EQUIV=REFRESH CONTENT=\"%d; URL=/api/status?refresh=%d%s\">\n", r, r, html ? "html=1" : "");
			}
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

#define CTL_SYNTAX "[hupall|pause|resume|shutdown]"
SWITCH_STANDARD_API(ctl_function)
{
	int argc;
	char *mydata, *argv[5];
	uint32_t arg = 0;

	if (switch_strlen_zero(cmd)) {
		stream->write_function(stream, "USAGE: %s\n", CTL_SYNTAX);
		return SWITCH_STATUS_SUCCESS;
	}

	if ((mydata = strdup(cmd))) {
		argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

		if (!strcmp(argv[0], "hupall")) {
			arg = 1;
			switch_core_session_ctl(SCSC_HUPALL, &arg);
		} else if (!strcmp(argv[0], "pause")) {
			arg = 1;
			switch_core_session_ctl(SCSC_PAUSE_INBOUND, &arg);
		} else if (!strcmp(argv[0], "resume")) {
			arg = 0;
			switch_core_session_ctl(SCSC_PAUSE_INBOUND, &arg);
		} else if (!strcmp(argv[0], "shutdown")) {
			arg = 0;
			switch_core_session_ctl(SCSC_SHUTDOWN, &arg);
		} else {
			stream->write_function(stream, "INVALID COMMAND\nUSAGE: fsctl [hupall|pause|resume|shutdown]\n");
			goto end;
		}

		stream->write_function(stream, "OK\n");
	  end:
		free(mydata);
	} else {
		stream->write_function(stream, "MEM ERR\n");
	}

	return SWITCH_STATUS_SUCCESS;

}

#define LOAD_SYNTAX "<mod_name>"
SWITCH_STANDARD_API(load_function)
{
	const char *err;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_strlen_zero(cmd)) {
		stream->write_function(stream, "USAGE: %s\n", LOAD_SYNTAX);
		return SWITCH_STATUS_SUCCESS;
	}

	if (switch_loadable_module_load_module((char *) SWITCH_GLOBAL_dirs.mod_dir, (char *) cmd, SWITCH_TRUE, &err) == SWITCH_STATUS_SUCCESS) {
		stream->write_function(stream, "OK\n");
	} else {
		stream->write_function(stream, "ERROR [%s]\n", err);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(unload_function)
{
	const char *err;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_strlen_zero(cmd)) {
		stream->write_function(stream, "USAGE: %s\n", LOAD_SYNTAX);
		return SWITCH_STATUS_SUCCESS;
	}

	if (switch_loadable_module_unload_module((char *) SWITCH_GLOBAL_dirs.mod_dir, (char *) cmd, &err) == SWITCH_STATUS_SUCCESS) {
		stream->write_function(stream, "OK\n");
	} else {
		stream->write_function(stream, "ERROR [%s]\n", err);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(reload_function)
{
	const char *err;
	switch_xml_t xml_root;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if ((xml_root = switch_xml_open_root(1, &err))) {
		switch_xml_free(xml_root);
	}

	stream->write_function(stream, "OK [%s]\n", err);

	return SWITCH_STATUS_SUCCESS;
}

#define KILL_SYNTAX "<uuid>"
SWITCH_STANDARD_API(kill_function)
{
	switch_core_session_t *ksession = NULL;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!cmd) {
		stream->write_function(stream, "USAGE: %s\n", KILL_SYNTAX);
	} else if ((ksession = switch_core_session_locate(cmd))) {
		switch_channel_t *channel = switch_core_session_get_channel(ksession);
		switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
		switch_core_session_rwunlock(ksession);
		stream->write_function(stream, "OK\n");
	} else {
		stream->write_function(stream, "No Such Channel!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

#define TRANSFER_SYNTAX "<uuid> <dest-exten> [<dialplan>] [<context>]"
SWITCH_STANDARD_API(transfer_function)
{
	switch_core_session_t *tsession = NULL;
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
		if (argc >= 2 && argc <= 4) {
			char *uuid = argv[0];
			char *dest = argv[1];
			char *dp = argv[2];
			char *context = argv[3];

			if ((tsession = switch_core_session_locate(uuid))) {

				if (switch_ivr_session_transfer(tsession, dest, dp, context) == SWITCH_STATUS_SUCCESS) {
					stream->write_function(stream, "OK\n");
				} else {
					stream->write_function(stream, "ERROR\n");
				}

				switch_core_session_rwunlock(tsession);

			} else {
				stream->write_function(stream, "No Such Channel!\n");
			}
			goto done;
		}
	}

	stream->write_function(stream, "USAGE: %s\n", TRANSFER_SYNTAX);

done:
	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define TONE_DETECT_SYNTAX "<uuid> <key> <tone_spec> [<flags> <timeout> <app> <args>]"
SWITCH_STANDARD_API(tone_detect_session_function)
{
	char *argv[6] = { 0 };
	int argc;
	char *mydata = NULL;
	time_t to = 0;
	switch_core_session_t *rsession;
	
	mydata = strdup(cmd);
	assert(mydata != NULL);

	if ((argc = switch_separate_string(mydata, ' ', argv, sizeof(argv) / sizeof(argv[0]))) < 3) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "INVALID ARGS!\n");
	}

	if (!(rsession = switch_core_session_locate(argv[0]))) {
		stream->write_function(stream, "-Error Cannot locate session!\n");
		return SWITCH_STATUS_SUCCESS;
	}

	if (argv[4]) {
		uint32_t mto;
		if (*argv[4] == '+') {
			if ((mto = atoi(argv[4]+1)) > 0) {
				to = time(NULL) + mto;
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "INVALID Timeout!\n");
				goto done;
			}
		} else {
			if ((to = atoi(argv[4])) < time(NULL)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "INVALID Timeout!\n");
				to = 0;
				goto done;
			}
		}
	}

	switch_ivr_tone_detect_session(rsession, argv[1], argv[2], argv[3], to, argv[5], argv[6]);
	stream->write_function(stream, "Enabling tone detection '%s' '%s' '%s'\n", argv[1], argv[2], argv[3]);

 done:

	free(mydata);
	switch_core_session_rwunlock(rsession);

	return SWITCH_STATUS_SUCCESS;
}

#define SCHED_TRANSFER_SYNTAX "[+]<time> <uuid> <extension> [<dialplan>] [<context>]"
SWITCH_STANDARD_API(sched_transfer_function)
{
	switch_core_session_t *tsession = NULL;
	char *mycmd = NULL, *argv[6] = { 0 };
	int argc = 0;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 2 || argc > 5) {
		stream->write_function(stream, "USAGE: %s\n", SCHED_TRANSFER_SYNTAX);
	} else {
		char *uuid = argv[1];
		char *dest = argv[2];
		char *dp = argv[3];
		char *context = argv[4];
		time_t when;

		if (*argv[0] == '+') {
			when = time(NULL) + atol(argv[0] + 1);
		} else {
			when = atol(argv[0]);
		}

		if ((tsession = switch_core_session_locate(uuid))) {
			switch_ivr_schedule_transfer(when, uuid, dest, dp, context);
			stream->write_function(stream, "OK\n");
			switch_core_session_rwunlock(tsession);
		} else {
			stream->write_function(stream, "No Such Channel!\n");
		}
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define SCHED_HANGUP_SYNTAX "[+]<time> <uuid> [<cause>]"
SWITCH_STANDARD_API(sched_hangup_function)
{
	switch_core_session_t *hsession = NULL;
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 1) {
		stream->write_function(stream, "USAGE: %s\n", SCHED_HANGUP_SYNTAX);
	} else {
		char *uuid = argv[1];
		char *cause_str = argv[2];
		time_t when;
		switch_call_cause_t cause = SWITCH_CAUSE_ALLOTTED_TIMEOUT;

		if (*argv[0] == '+') {
			when = time(NULL) + atol(argv[0] + 1);
		} else {
			when = atol(argv[0]);
		}

		if (cause_str) {
			cause = switch_channel_str2cause(cause_str);
		}

		if ((hsession = switch_core_session_locate(uuid))) {
			switch_ivr_schedule_hangup(when, uuid, cause, SWITCH_FALSE);
			stream->write_function(stream, "OK\n");
			switch_core_session_rwunlock(hsession);
		} else {
			stream->write_function(stream, "No Such Channel!\n");
		}
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define MEDIA_SYNTAX "<uuid>"
SWITCH_STANDARD_API(uuid_media_function)
{
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (session) {
		return status;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 1) {
		stream->write_function(stream, "USAGE: %s\n", MEDIA_SYNTAX);
	} else {
		if (!strcmp(argv[0], "off")) {
			status = switch_ivr_nomedia(argv[1], SMF_REBRIDGE);
		} else {
			status = switch_ivr_media(argv[0], SMF_REBRIDGE);
		}
	}

	if (status == SWITCH_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK Success\n");
	} else {
		stream->write_function(stream, "-ERR Operation Failed\n");
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define BROADCAST_SYNTAX "<uuid> <path> [aleg|bleg|both]"
SWITCH_STANDARD_API(uuid_broadcast_function)
{
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (session) {
		return status;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 2) {
		stream->write_function(stream, "USAGE: %s\n", BROADCAST_SYNTAX);
	} else {
		switch_media_flag_t flags = SMF_NONE;

		if (argv[2]) {
			if (!strcmp(argv[2], "both")) {
				flags |= (SMF_ECHO_ALEG | SMF_ECHO_BLEG);
			} else if (!strcmp(argv[2], "aleg")) {
				flags |= SMF_ECHO_ALEG;
			} else if (!strcmp(argv[2], "bleg")) {
				flags |= SMF_ECHO_BLEG;
			}
		} else {
			flags |= SMF_ECHO_ALEG;
		}

		status = switch_ivr_broadcast(argv[0], argv[1], flags);
		stream->write_function(stream, "+OK Message Sent\n");
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define SCHED_BROADCAST_SYNTAX "[+]<time> <uuid> <path> [aleg|bleg|both]"
SWITCH_STANDARD_API(sched_broadcast_function)
{
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (session) {
		return status;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 3) {
		stream->write_function(stream, "USAGE: %s\n", SCHED_BROADCAST_SYNTAX);
	} else {
		switch_media_flag_t flags = SMF_NONE;
		time_t when;

		if (*argv[0] == '+') {
			when = time(NULL) + atol(argv[0] + 1);
		} else {
			when = atol(argv[0]);
		}

		if (argv[3]) {
			if (!strcmp(argv[3], "both")) {
				flags |= (SMF_ECHO_ALEG | SMF_ECHO_BLEG);
			} else if (!strcmp(argv[3], "aleg")) {
				flags |= SMF_ECHO_ALEG;
			} else if (!strcmp(argv[3], "bleg")) {
				flags |= SMF_ECHO_BLEG;
			}
		} else {
			flags |= SMF_ECHO_ALEG;
		}

		status = switch_ivr_schedule_broadcast(when, argv[1], argv[2], flags);
		stream->write_function(stream, "+OK Message Scheduled\n");
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define HOLD_SYNTAX "<uuid>"
SWITCH_STANDARD_API(uuid_hold_function)
{
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (session) {
		return status;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 1) {
		stream->write_function(stream, "USAGE: %s\n", HOLD_SYNTAX);
	} else {
		if (!strcmp(argv[0], "off")) {
			status = switch_ivr_unhold_uuid(argv[1]);
		} else {
			status = switch_ivr_hold_uuid(argv[0]);
		}
	}

	if (status == SWITCH_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK Success\n");
	} else {
		stream->write_function(stream, "-ERR Operation Failed\n");
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define UUID_SYNTAX "<uuid> <other_uuid>"
SWITCH_STANDARD_API(uuid_bridge_function)
{
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc != 2) {
		stream->write_function(stream, "USAGE: %s\n", UUID_SYNTAX);
	} else {
		if (switch_ivr_uuid_bridge(argv[0], argv[1]) != SWITCH_STATUS_SUCCESS) {
			stream->write_function(stream, "Invalid uuid\n");
		}
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define SESS_REC_SYNTAX "<uuid> [start|stop] <path> [<limit>]"
SWITCH_STANDARD_API(session_record_function)
{
	switch_core_session_t *rsession = NULL;
	char *mycmd = NULL, *argv[4] = { 0 };
	char *uuid = NULL, *action = NULL, *path = NULL;
	int argc = 0;
	uint32_t limit = 0;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_strlen_zero(cmd)) {
		goto usage;
	}

	if (!(mycmd = strdup(cmd))) {
		goto usage;
	}

	if ((argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) < 3) {
		goto usage;
	}

	uuid = argv[0];
	action = argv[1];
	path = argv[2];
	limit = argv[3] ? atoi(argv[3]) : 0;
	
	if (!(rsession = switch_core_session_locate(uuid))) {
		stream->write_function(stream, "-Error Cannot locate session!\n");
		return SWITCH_STATUS_SUCCESS;
	}
	
	if (switch_strlen_zero(action) || switch_strlen_zero(path)) {
		goto usage;
	}

	if (!strcasecmp(action, "start")) {
		switch_ivr_record_session(rsession, path, limit, NULL);
	} else if (!strcasecmp(action, "stop")) {
		switch_ivr_stop_record_session(rsession, path);
	} else {
		goto usage;
	}

	goto done;

  usage:

	stream->write_function(stream, "USAGE: %s\n", SESS_REC_SYNTAX);
	switch_safe_free(mycmd);


  done:

	if (rsession) {
		switch_core_session_rwunlock(rsession);
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}


SWITCH_STANDARD_API(session_displace_function)
{
	switch_core_session_t *rsession = NULL;
	char *mycmd = NULL, *argv[5] = { 0 };
	char *uuid = NULL, *action = NULL, *path = NULL;
	int argc = 0;
	uint32_t limit = 0;
	char *flags = NULL;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_strlen_zero(cmd)) {
		goto usage;
	}

	if (!(mycmd = strdup(cmd))) {
		goto usage;
	}

	if ((argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) < 3) {
		goto usage;
	}

	uuid = argv[0];
	action = argv[1];
	path = argv[2];
	limit = argv[3] ? atoi(argv[3]) : 0;
	flags = argv[4];

	if (!(rsession = switch_core_session_locate(uuid))) {
		stream->write_function(stream, "-Error Cannot locate session!\n");
		return SWITCH_STATUS_SUCCESS;
	}
	
	if (switch_strlen_zero(action) || switch_strlen_zero(path)) {
		goto usage;
	}

	if (!strcasecmp(action, "start")) {
		switch_ivr_displace_session(rsession, path, limit, flags);
	} else if (!strcasecmp(action, "stop")) {
		switch_ivr_stop_displace_session(rsession, path);
	} else {
		goto usage;
	}

	goto done;

  usage:

	stream->write_function(stream, "INVALID SYNTAX\n");
	switch_safe_free(mycmd);


  done:

	if (rsession) {
		switch_core_session_rwunlock(rsession);
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define PAUSE_SYNTAX "<uuid> <on|off>"
SWITCH_STANDARD_API(pause_function)
{
	switch_core_session_t *psession = NULL;
	char *mycmd = NULL, *argv[4] = { 0 };
	int argc = 0;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 2) {
		stream->write_function(stream, "USAGE: %s\n", PAUSE_SYNTAX);
	} else {
		char *uuid = argv[0];
		char *dest = argv[1];

		if ((psession = switch_core_session_locate(uuid))) {
			switch_channel_t *channel = switch_core_session_get_channel(psession);

			if (!strcasecmp(dest, "on")) {
				switch_channel_set_flag(channel, CF_HOLD);
			} else {
				switch_channel_clear_flag(channel, CF_HOLD);
			}

			switch_core_session_rwunlock(psession);

		} else {
			stream->write_function(stream, "No Such Channel!\n");
		}
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

#define ORIGINATE_SYNTAX "<call url> <exten>|&<application_name>(<app_args>) [<dialplan>] [<context>] [<cid_name>] [<cid_num>] [<timeout_sec>]"
SWITCH_STANDARD_API(originate_function)
{
	switch_channel_t *caller_channel;
	switch_core_session_t *caller_session = NULL;
	char *mycmd = NULL, *argv[7] = { 0 };
	int i = 0, x, argc = 0;
	char *aleg, *exten, *dp, *context, *cid_name, *cid_num;
	uint32_t timeout = 60;
	switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;
	uint8_t machine = 1;

	if (session) {
		stream->write_function(stream, "Illegal Usage\n");
		return SWITCH_STATUS_SUCCESS;
	}

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (switch_strlen_zero(cmd) || argc < 2 || argc > 7) {
		stream->write_function(stream, "USAGE: %s\n", ORIGINATE_SYNTAX);
		switch_safe_free(mycmd);
		return SWITCH_STATUS_SUCCESS;
	}

	for (x = 0; x < argc; x++) {
		if (!strcasecmp(argv[x], "undef")) {
			argv[x] = NULL;
		}
	}

	if (!strcasecmp(argv[0], "machine")) {
		machine = 1;
		i++;
	}

	aleg = argv[i++];
	exten = argv[i++];
	dp = argv[i++];
	context = argv[i++];
	cid_name = argv[i++];
	cid_num = argv[i++];

	if (!dp) {
		dp = "XML";
	}

	if (!context) {
		context = "default";
	}

	if (argv[6]) {
		timeout = atoi(argv[6]);
	}

	if (switch_ivr_originate(NULL, &caller_session, &cause, aleg, timeout, NULL, cid_name, cid_num, NULL) != SWITCH_STATUS_SUCCESS) {
		if (machine) {
			stream->write_function(stream, "fail: %s\n", switch_channel_cause2str(cause));
		} else {
			stream->write_function(stream, "Cannot Create Outgoing Channel! [%s] cause: %s\n", aleg, switch_channel_cause2str(cause));
		}
		switch_safe_free(mycmd);
		return SWITCH_STATUS_SUCCESS;
	}

	caller_channel = switch_core_session_get_channel(caller_session);
	assert(caller_channel != NULL);
	switch_channel_clear_state_handler(caller_channel, NULL);

	if (*exten == '&' && *(exten + 1)) {
		switch_caller_extension_t *extension = NULL;
		char *app_name = switch_core_session_strdup(caller_session, (exten + 1));
		char *arg = NULL, *e;

		if ((e = strchr(app_name, ')'))) {
			*e = '\0';
		}

		if ((arg = strchr(app_name, '('))) {
			*arg++ = '\0';
		}

		if ((extension = switch_caller_extension_new(caller_session, app_name, arg)) == 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "memory error!\n");
			switch_channel_hangup(caller_channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			switch_safe_free(mycmd);
			return SWITCH_STATUS_MEMERR;
		}
		switch_caller_extension_add_application(caller_session, extension, app_name, arg);
		switch_channel_set_caller_extension(caller_channel, extension);
		switch_channel_set_state(caller_channel, CS_EXECUTE);
	} else {
		switch_ivr_session_transfer(caller_session, exten, dp, context);
	}

	if (machine) {
		stream->write_function(stream, "success: %s\n", switch_core_session_get_uuid(caller_session));
	} else {
		stream->write_function(stream, "Created Session: %s\n", switch_core_session_get_uuid(caller_session));
	}

	if (caller_session) {
		switch_core_session_rwunlock(caller_session);
	}

	switch_safe_free(mycmd);
	return SWITCH_STATUS_SUCCESS;
}

static void sch_api_callback(switch_scheduler_task_t *task)
{
	char *cmd, *arg = NULL;
	switch_stream_handle_t stream = { 0 };

	assert(task);

	cmd = (char *) task->cmd_arg;

	if ((arg = strchr(cmd, ' '))) {
		*arg++ = '\0';
	}

	SWITCH_STANDARD_STREAM(stream);
	switch_api_execute(cmd, arg, NULL, &stream);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Command %s(%s):\n%s\n", cmd, arg, switch_str_nil((char *) stream.data));
	switch_safe_free(stream.data);
}

SWITCH_STANDARD_API(sched_del_function)
{
	uint32_t cnt = 0;
	
	if (switch_is_digit_string(cmd)) {
		int64_t tmp;
		tmp = (uint32_t) atoi(cmd);
		if (tmp > 0) {
			cnt = switch_scheduler_del_task_id((uint32_t)tmp);
		}
	} else {
		cnt = switch_scheduler_del_task_group(cmd);
	}

	stream->write_function(stream, "DELETED: %u\n", cnt);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(xml_wrap_api_function)
{
	char *dcommand, *edata = NULL, *send = NULL, *command, *arg = NULL;
	switch_stream_handle_t mystream = { 0 };
	int encoded = 0, elen = 0;
	
	if ((dcommand = strdup(cmd))) {
		if (!strncasecmp(dcommand, "encoded ", 8)) {
			encoded++;
			command = dcommand + 8;
		} else {
			command = dcommand;
		}

		if ((arg = strchr(command, ' '))) {
			*arg++ = '\0';
		}
		SWITCH_STANDARD_STREAM(mystream);
		switch_api_execute(command, arg, NULL, &mystream);

		if (mystream.data) {
			if (encoded) {
				elen = (int) strlen(mystream.data) * 3;
				edata = malloc(elen);
				assert(edata != NULL);
				memset(edata, 0, elen);
				switch_url_encode(mystream.data, edata, elen);
				send = edata;
			} else {
				send = mystream.data;
			}
		}

		stream->write_function(stream, 
							   "<result>\n"
							   "  <row id=\"1\">\n"
							   "    <data>%s</data>\n"
							   "  </row>\n"
							   "</result>\n",
							   send ? send : "ERROR"
							   );
		switch_safe_free(mystream.data);
		switch_safe_free(edata);
		free(dcommand);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(sched_api_function)
{
	char *tm = NULL, *dcmd, *group;
	time_t when;

	assert(cmd != NULL);
	tm = strdup(cmd);
	assert(tm != NULL);

	if ((group = strchr(tm, ' '))) {
		uint32_t id;

		*group++ = '\0';

		if ((dcmd = strchr(group, ' '))) {
			*dcmd++ = '\0';

			if (*tm == '+') {
				when = time(NULL) + atol(tm + 1);
			} else {
				when = atol(tm);
			}
		
			id = switch_scheduler_add_task(when, sch_api_callback, (char *) __SWITCH_FUNC__, group, 0, strdup(dcmd), SSHF_FREE_ARG);
			stream->write_function(stream, "ADDED: %u\n", id);
			goto good;
		} 
	}

	stream->write_function(stream, "Invalid syntax\n");

 good:

	switch_safe_free(tm);

	return SWITCH_STATUS_SUCCESS;
}


struct holder {
	switch_stream_handle_t *stream;
	char *http;
	char *delim;
	uint32_t count;
	int print_title;
	switch_xml_t xml;
	int rows;
};

static int show_as_xml_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct holder *holder = (struct holder *) pArg;
	switch_xml_t row, field;
	int x, f_off = 0;
	char id[50];

	if (holder->count == 0) {
		if (!(holder->xml = switch_xml_new("result"))) {
			return -1;
		}
	}

	if (!(row = switch_xml_add_child_d(holder->xml, "row", holder->rows++))) {
		return -1;
	}

	snprintf(id, sizeof(id), "%d", holder->rows);
	switch_xml_set_attr_d(row, "row_id", id);

	for(x = 0; x < argc; x++) {
		char *name = columnNames[x];
		char *val = switch_str_nil(argv[x]);

		if (!name) {
			name = "undefined";
		}

		if ((field = switch_xml_add_child_d(row, name, f_off++))) {
			switch_xml_set_txt_d(field, val);
		} else {
			return -1;
		}
	}

	holder->count++;

	return 0;
}

static int show_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct holder *holder = (struct holder *) pArg;
	int x;


	if (holder->print_title && holder->count == 0) {
		if (holder->http) {
			holder->stream->write_function(holder->stream, "\n<tr>");
		}

		for (x = 0; x < argc; x++) {
			char *name = columnNames[x];


			if (!name) {
				name = "undefined";
			}

			if (holder->http) {
				holder->stream->write_function(holder->stream, "<td>");
				holder->stream->write_function(holder->stream, "<b>%s</b>%s", name, x == (argc - 1) ? "</td></tr>\n" : "</td><td>");
			} else {
				holder->stream->write_function(holder->stream, "%s%s", name, x == (argc - 1) ? "\n" : holder->delim);
			}
		}
	}

	if (holder->http) {
		holder->stream->write_function(holder->stream, "<tr bgcolor=%s>", holder->count % 2 == 0 ? "eeeeee" : "ffffff");
	}

	for (x = 0; x < argc; x++) {
		char *val = switch_str_nil(argv[x]);

		if (holder->http) {
			holder->stream->write_function(holder->stream, "<td>");
			holder->stream->write_function(holder->stream, "%s%s", val, x == (argc - 1) ? "</td></tr>\n" : "</td><td>");
		} else {
			holder->stream->write_function(holder->stream, "%s%s", val, x == (argc - 1) ? "\n" : holder->delim);
		}
	}

	holder->count++;
	return 0;
}

#define SHOW_SYNTAX "codec|application|api|dialplan|file|timer|calls|channels"
SWITCH_STANDARD_API(show_function)
{
	char sql[1024];
	char *errmsg;
	switch_core_db_t *db = switch_core_db_handle();
	struct holder holder = { 0 };
	int help = 0;
	char *mydata = NULL, *argv[6] = {0};
	int argc;
	char *command = NULL, *as = NULL;

	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (cmd && (mydata = strdup(cmd))) {
		argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
		command = argv[0];
		if (argv[2] && !strcasecmp(argv[1], "as")) {
			as = argv[2];
		}
	}
	
	if (!as && stream->event) {
		holder.http = switch_event_get_header(stream->event, "http-host");
	}

	holder.print_title = 1;

	// If you changes the field qty or order of any of these select
	// statmements, you must also change show_callback and friends to match!
	if (!command) {
		stream->write_function(stream, "USAGE: %s\n", SHOW_SYNTAX);
		return SWITCH_STATUS_SUCCESS;
	} else if (!strcmp(command, "codec") || !strcmp(command, "dialplan") || !strcmp(command, "file") || !strcmp(command, "timer")) {
		sprintf(sql, "select type, name from interfaces where type = '%s'", command);
	} else if (!strcmp(command, "tasks")) {
		sprintf(sql, "select * from %s", command);
	} else if (!strcmp(command, "application") || !strcmp(command, "api")) {
		sprintf(sql, "select name, description, syntax from interfaces where type = '%s' and description != ''", command);
	} else if (!strcmp(command, "calls")) {
		sprintf(sql, "select * from calls");
	} else if (!strcmp(command, "channels")) {
		sprintf(sql, "select * from channels");
	} else if (!strncasecmp(command, "help", 4)) {
		char *cmdname = NULL;

		help = 1;
		holder.print_title = 0;
		if ((cmdname = strchr(command, ' ')) != 0) {
			*cmdname++ = '\0';
			snprintf(sql, sizeof(sql) - 1, "select name, syntax, description from interfaces where type = 'api' and name = '%s'", cmdname);
		} else {
			snprintf(sql, sizeof(sql) - 1, "select name, syntax, description from interfaces where type = 'api'");
		}
	} else {
		stream->write_function(stream, "USAGE: %s\n", SHOW_SYNTAX);
		return SWITCH_STATUS_SUCCESS;
	}

	holder.stream = stream;
	holder.count = 0;

	if (holder.http) {
		holder.stream->write_function(holder.stream, "<table cellpadding=1 cellspacing=4 border=1>\n");
	}

	if (!as) {
		as = "delim";
		holder.delim = ",";
	}

	if (!strcasecmp(as, "delim") || !strcasecmp(as, "csv")) {
		if (switch_strlen_zero(holder.delim)) {
			if (!(holder.delim = argv[3])) {
				holder.delim = ",";
			}
		}
		switch_core_db_exec(db, sql, show_callback, &holder, &errmsg);
		if (holder.http) {
			holder.stream->write_function(holder.stream, "</table>");
		}

		if (errmsg) {
			stream->write_function(stream, "SQL ERR [%s]\n", errmsg);
			switch_core_db_free(errmsg);
			errmsg = NULL;
		} else if (help) {
			if (holder.count == 0)
				stream->write_function(stream, "No such command.\n");
		} else {
			stream->write_function(stream, "\n%u total.\n", holder.count);
		}
	} else if (!strcasecmp(as, "xml")) {
		switch_core_db_exec(db, sql, show_as_xml_callback, &holder, &errmsg);

		if (holder.xml) {
			char count[50];
			char *xmlstr;
			snprintf(count, sizeof(count), "%d", holder.count);
			switch_xml_set_attr_d(holder.xml, "row_count", count);
			xmlstr = switch_xml_toxml(holder.xml);

			if (xmlstr) {
				holder.stream->write_function(holder.stream, "%s", xmlstr);
				free(xmlstr);
			} else {
				holder.stream->write_function(holder.stream, "<result row_count=\"0\"/>\n");
			}
		} else {
			holder.stream->write_function(holder.stream, "<result row_count=\"0\"/>\n");
		}
	} else {
		holder.stream->write_function(holder.stream, "Cannot find format %s\n", as);
	}

	switch_safe_free(mydata);
	switch_core_db_close(db);
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(version_function)
{
	char version_string[1024];
	snprintf(version_string, sizeof(version_string) - 1, "FreeSwitch Version %s\n", SWITCH_VERSION_FULL);

	stream->write_function(stream, version_string);
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(help_function)
{
	char showcmd[1024];
	int all = 0;
	if (switch_strlen_zero(cmd)) {
		sprintf(showcmd, "help");
		all = 1;
	} else {
		snprintf(showcmd, sizeof(showcmd) - 1, "help %s", cmd);
	}

	if (all) {
		stream->write_function(stream, "\nValid Commands:\n\n");
	}

	show_function(showcmd, session, stream);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_commands_load)
{
	switch_api_interface_t *commands_api_interface;
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	SWITCH_ADD_API(commands_api_interface, "originate", "Originate a Call", originate_function, ORIGINATE_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "tone_detect", "Start Tone Detection on a channel", tone_detect_session_function, TONE_DETECT_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "killchan", "Kill Channel", kill_function, KILL_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "reloadxml", "Reload XML", reload_function, "");
	SWITCH_ADD_API(commands_api_interface, "unload", "Unload Module", unload_function, LOAD_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "load", "Load Module", unload_function, LOAD_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "transfer", "Transfer Module", transfer_function, TRANSFER_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "pause", "Pause", pause_function, PAUSE_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "show", "Show", show_function, SHOW_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "status", "status", status_function, "");
	SWITCH_ADD_API(commands_api_interface, "uuid_bridge", "uuid_bridge", uuid_bridge_function, UUID_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "session_displace", "session displace", session_displace_function, "<uuid> [start|stop] <path> [<limit>] [mux]");
	SWITCH_ADD_API(commands_api_interface, "session_record", "session record", session_record_function, SESS_REC_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "broadcast", "broadcast", uuid_broadcast_function, BROADCAST_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "hold", "hold", uuid_hold_function, HOLD_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "media", "media", uuid_media_function, MEDIA_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "fsctl", "control messages", ctl_function, CTL_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "help", "Show help for all the api commands", help_function, "");
	SWITCH_ADD_API(commands_api_interface, "version", "Show version of the switch", version_function, "");
	SWITCH_ADD_API(commands_api_interface, "sched_hangup", "Schedule a running call to hangup", sched_hangup_function, SCHED_HANGUP_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "sched_broadcast", "Schedule a broadcast event to a running call", sched_broadcast_function, SCHED_BROADCAST_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "sched_transfer", "Schedule a broadcast event to a running call", sched_transfer_function, SCHED_TRANSFER_SYNTAX);
	SWITCH_ADD_API(commands_api_interface, "sched_api", "Schedule an api command", sched_api_function, "[+]<time> <group_name> <command_string>");
	SWITCH_ADD_API(commands_api_interface, "sched_del", "Delete a Scheduled task", sched_del_function, "<task_id>|<group_id>");
	SWITCH_ADD_API(commands_api_interface, "xml_wrap", "Wrap another api command in xml", xml_wrap_api_function, "<command> <args>");

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_NOUNLOAD;
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
