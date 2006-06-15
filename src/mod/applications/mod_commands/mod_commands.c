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
 *
 * 
 * mod_commands.c -- Misc. Command Module
 *
 */
#include <switch.h>

static const char modname[] = "mod_commands";

static switch_status_t status_function(char *cmd, switch_stream_handle_t *stream)
{
	uint8_t html = 0;
	switch_core_time_duration_t duration;
	switch_core_measure_time(switch_core_uptime(), &duration);

	if (cmd && strstr(cmd, "html")) {
		html = 1;
		stream->write_function(stream, "<h1>FreeSWITCH Status</h1>\n<b>");
	}

	stream->write_function(stream, "UP %u year%s, %u day%s, %u hour%s, %u minute%s, %u second%s, %u millisecond%s, %u microsecond%s\n",
						   duration.yr, duration.yr == 1 ? "" : "s",
						   duration.day, duration.day == 1 ? "" : "s",
						   duration.hr, duration.hr == 1 ? "" : "s",
						   duration.min, duration.min == 1 ? "" : "s",
						   duration.sec, duration.sec == 1 ? "" : "s",
						   duration.ms, duration.ms == 1 ? "" : "s",
						   duration.mms, duration.mms == 1 ? "" : "s"
						   );

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

static switch_status_t load_function(char *mod, switch_stream_handle_t *stream)
{
	switch_loadable_module_load_module((char *) SWITCH_GLOBAL_dirs.mod_dir, (char *) mod);
	 stream->write_function(stream, "OK\n");
	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t reload_function(char *mod, switch_stream_handle_t *stream)
{
	const char *err;
	switch_xml_t xml_root;

	if ((xml_root = switch_xml_open_root(1, &err))) {
		switch_xml_free(xml_root);
	}
	
	stream->write_function(stream, "OK [%s]\n", err);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t kill_function(char *dest, switch_stream_handle_t *stream)
{
	switch_core_session_t *session = NULL;

	if ((session = switch_core_session_locate(dest))) {
		switch_channel_t *channel = switch_core_session_get_channel(session);
		switch_core_session_kill_channel(session, SWITCH_SIG_KILL);
		switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
		switch_core_session_rwunlock(session);
		stream->write_function(stream, "OK\n");
	} else {
		stream->write_function(stream, "No Such Channel!\n");
	}

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t transfer_function(char *cmd, switch_stream_handle_t *stream)
{
	switch_core_session_t *session = NULL;
	char *argv[4] = {0};
	int argc = 0;
	
	argc = switch_separate_string(cmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc < 2 || argc > 4) {
		stream->write_function(stream, "Invalid Parameters\n");
	} else {
		char *uuid = argv[0];
		char *dest = argv[1];
		char *dp = argv[2];
		char *context = argv[3];
		
		if ((session = switch_core_session_locate(uuid))) {

			if (switch_ivr_session_transfer(session, dest, dp, context) == SWITCH_STATUS_SUCCESS) {
				 stream->write_function(stream, "OK\n");
			} else {
				stream->write_function(stream, "ERROR\n");
			}

			switch_core_session_rwunlock(session);

		} else {
			stream->write_function(stream, "No Such Channel!\n");
		}
	}

	return SWITCH_STATUS_SUCCESS;
}




static switch_status_t pause_function(char *cmd, switch_stream_handle_t *stream)
{
	switch_core_session_t *session = NULL;
	char *argv[4] = {0};
	int argc = 0;
	
	argc = switch_separate_string(cmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc < 2) {
		stream->write_function(stream, "Invalid Parameters\n");
	} else {
		char *uuid = argv[0];
		char *dest = argv[1];
		
		if ((session = switch_core_session_locate(uuid))) {
			switch_channel_t *channel = switch_core_session_get_channel(session);

			if (!strcasecmp(dest, "on")) {
				switch_channel_set_flag(channel, CF_HOLD);
			} else {
				switch_channel_clear_flag(channel, CF_HOLD);
			}

			switch_core_session_rwunlock(session);

		} else {
			stream->write_function(stream, "No Such Channel!\n");
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

static int show_callback(void *pArg, int argc, char **argv, char **columnNames){
	switch_stream_handle_t *stream = (switch_stream_handle_t *) pArg;

	stream->write_function(stream, "%s|%s\n", argv[0], argv[1] ? argv[1] : "NULL");
	return 0;
}


static switch_status_t show_function(char *cmd, switch_stream_handle_t *stream)
{
	char sql[1024];
	char *errmsg;
	switch_core_db_t *db = switch_core_db_handle();

    if (!cmd) {
        sprintf (sql, "select * from interfaces");
    }
    else if ( !strcmp(cmd,"codec") || !strcmp(cmd,"application") || 
              !strcmp(cmd,"api") || !strcmp(cmd,"dialplan") || 
              !strcmp(cmd,"file") || !strcmp(cmd,"timer") 
            ) {
        sprintf (sql, "select * from interfaces where type = '%s'", cmd);
    }
    else {
        stream->write_function(stream, "Invalid interfaces type!\n");
        stream->write_function(stream, "Example:\n");
        stream->write_function(stream, "show <blank>|codec|application|api|dialplan|file|timer\n");
        return SWITCH_STATUS_SUCCESS;
    }
    
	switch_core_db_exec(db, sql, show_callback, stream, &errmsg);

	if (errmsg) {
		stream->write_function(stream, "SQL ERR [%s]\n",errmsg);
		switch_core_db_free(errmsg);
		errmsg = NULL;
	}

	switch_core_db_close(db);
	return SWITCH_STATUS_SUCCESS;
}


static switch_api_interface_t status_api_interface = {
	/*.interface_name */ "status",
	/*.desc */ "status",
	/*.function */ status_function,
	/*.next */ NULL
};

static switch_api_interface_t show_api_interface = {
	/*.interface_name */ "show",
	/*.desc */ "Show",
	/*.function */ show_function,
	/*.next */ &status_api_interface
};

static switch_api_interface_t pause_api_interface = {
	/*.interface_name */ "pause",
	/*.desc */ "Pause",
	/*.function */ pause_function,
	/*.next */ &show_api_interface
};

static switch_api_interface_t transfer_api_interface = {
	/*.interface_name */ "transfer",
	/*.desc */ "Transfer",
	/*.function */ transfer_function,
	/*.next */ &pause_api_interface
};

static switch_api_interface_t load_api_interface = {
	/*.interface_name */ "load",
	/*.desc */ "Load Modile",
	/*.function */ load_function,
	/*.next */ &transfer_api_interface
};

static switch_api_interface_t reload_api_interface = {
	/*.interface_name */ "reloadxml",
	/*.desc */ "Reload XML",
	/*.function */ reload_function,
	/*.next */ &load_api_interface,

};

static switch_api_interface_t commands_api_interface = {
	/*.interface_name */ "killchan",
	/*.desc */ "Kill Channel",
	/*.function */ kill_function,
	/*.next */ &reload_api_interface
};

static const switch_loadable_module_interface_t mod_commands_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ NULL,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ NULL,
	/*.application_interface */ NULL,
	/*.api_interface */ &commands_api_interface
};


SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface, char *filename)
{

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = &mod_commands_module_interface;


	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

