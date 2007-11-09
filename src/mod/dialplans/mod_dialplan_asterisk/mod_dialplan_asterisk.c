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
 * mod_dialplan_asterisk.c -- Asterisk extensions.conf style dialplan parser.
 *
 */
#include <switch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_dialplan_asterisk_load);
SWITCH_MODULE_DEFINITION(mod_dialplan_asterisk, mod_dialplan_asterisk_load, NULL, NULL);

static switch_status_t exec_app(switch_core_session_t *session, char *app, char *arg)
{
	const switch_application_interface_t *application_interface;

	if ((application_interface = switch_loadable_module_get_application_interface(app))) {
		return switch_core_session_exec(session, application_interface, arg);
	}

	return SWITCH_STATUS_FALSE;
}


SWITCH_STANDARD_APP(dial_function)
{
	int argc;
	char *argv[4] = { 0 };
	char *mydata;
	switch_channel_t *channel;


	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	
	if (data && (mydata = switch_core_session_strdup(session, data))) {
		if ((argc = switch_separate_string(mydata, ',', argv, (sizeof(argv) / sizeof(argv[0])))) < 2) {
			goto error;
		}
		
		if (argc > 1) {
			switch_channel_set_variable(channel, "call_timeout", argv[1]);
		}
		
		switch_replace_char(argv[0], '&',',', SWITCH_FALSE);
		
		if (exec_app(session, "bridge", argv[0]) != SWITCH_STATUS_SUCCESS) {
			goto error;
		}
		
		goto ok;
	}

	
 error:
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error!\n");

 ok:
	
	return;

}



SWITCH_STANDARD_APP(goto_function)
{
	int argc;
	char *argv[3] = { 0 };
	char *mydata;
	switch_channel_t *channel;
	
	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	
	if (data && (mydata = switch_core_session_strdup(session, data))) {
		if ((argc = switch_separate_string(mydata, ',', argv, (sizeof(argv) / sizeof(argv[0])))) < 1) {
			goto error;
		}
		
		switch_ivr_session_transfer(session, argv[1], "asterisk", argv[0]);
		goto ok;
	}

	
 error:
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error!\n");

 ok:
	
	return;

}


SWITCH_STANDARD_DIALPLAN(asterisk_dialplan_hunt)
{
	switch_caller_extension_t *extension = NULL;
	switch_channel_t *channel;
	char *cf = "extensions.conf";
	switch_config_t cfg;
	char *var, *val;
	const char *context = NULL;

	if (arg) {
		cf = arg;
	}

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (!caller_profile) {
		caller_profile = switch_channel_get_caller_profile(channel);
	}
	
	if (caller_profile) {
		context = caller_profile->context ? caller_profile->context : "default";
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Obtaining Profile!\n");
		return NULL;
	}
	
	if (!switch_config_open_file(&cfg, cf)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		switch_channel_hangup(channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return NULL;
	}

	while (switch_config_next_pair(&cfg, &var, &val)) {
		if (!strcasecmp(cfg.category, context)) {
			if (!strcasecmp(var, "exten")) {
				int argc;
				char *argv[3] = { 0 };
				char *pattern = NULL;
				char *pri = NULL;
				char *app = NULL;
				char *arg = NULL;
				char expression[1024] = "";
				char substituted[2048] = "";
				char *field_data = caller_profile->destination_number;
				int proceed = 0;
				switch_regex_t *re = NULL;
				int ovector[30] = {0};
				
				argc = switch_separate_string(val, ',', argv, (sizeof(argv) / sizeof(argv[0])));
				if (argc < 3) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "parse error line %d!\n", cfg.lineno);
					continue;
				}

				pattern = argv[0];
				
				if (*pattern == '_') {
					pattern++;
					if (switch_ast2regex(pattern, expression, sizeof(expression))) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "converting [%s] to real regex [%s] you should try them!\n", 
										  pattern, expression);
					}
					if (!(proceed = switch_regex_perform(field_data, expression, &re, ovector, sizeof(ovector) / sizeof(ovector[0])))) {
						switch_regex_safe_free(re);
						continue;
					}
					
				} else {
					if (strcasecmp(pattern, caller_profile->destination_number)) {
						continue;
					}
				}
				
				switch_channel_set_variable(channel, "EXTEN", caller_profile->destination_number);
				switch_channel_set_variable(channel, "CHANNEL", switch_channel_get_name(channel));
				switch_channel_set_variable(channel, "UNIQUEID", switch_core_session_get_uuid(session));
				
				pri = argv[1];
				app = argv[2];
				
				if ((arg = strchr(app, '('))) {
					*arg++ = '\0';
					char *p = strrchr(arg, ')');
					if (p) {
							*p = '\0';
					}
				} else if ((arg = strchr(app, ','))) {
					*arg++ = '\0';
				}
				
				if (!arg) {
					arg = "";
				}

				switch_replace_char(arg, '|',',', SWITCH_FALSE);

				if (strchr(expression, '(')) {
					switch_perform_substitution(re, proceed, arg, field_data, substituted, sizeof(substituted), ovector);
					arg = substituted;
				}
				switch_regex_safe_free(re);

				if (!extension) {
					if ((extension = switch_caller_extension_new(session, caller_profile->destination_number,
																 caller_profile->destination_number)) == 0) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "memory error!\n");
						break;
					}
				}
				
				switch_caller_extension_add_application(session, extension, app, arg);
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "param '%s' not implemented at line %d!\n", var, cfg.lineno);
			}
		}
	}

	switch_config_close_file(&cfg);

	if (extension) {
		switch_channel_set_state(channel, CS_EXECUTE);
	} 

	return extension;
}


/* fake chan_sip */
switch_endpoint_interface_t *sip_endpoint_interface;
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool);
switch_io_routines_t sip_io_routines = {
	/*.outgoing_channel */ channel_outgoing_channel
};

static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool)
{
	outbound_profile->destination_number = switch_core_sprintf(outbound_profile->pool, "default/%s", outbound_profile->destination_number);
	return switch_core_session_outgoing_channel(session, "sofia", outbound_profile, new_session, pool);
}

#define WE_DONT_NEED_NO_STINKIN_KEY "true"
static char *key()
{
    return WE_DONT_NEED_NO_STINKIN_KEY;
}




SWITCH_MODULE_LOAD_FUNCTION(mod_dialplan_asterisk_load)
{
	switch_dialplan_interface_t *dp_interface;
	switch_application_interface_t *app_interface;
	char *mykey = NULL;

	if ((mykey = key())) {
		mykey = NULL;
	}
	
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	SWITCH_ADD_DIALPLAN(dp_interface, "asterisk", asterisk_dialplan_hunt);
	SWITCH_ADD_APP(app_interface, "Dial", "Dial", "Dial", dial_function, "Dial", SAF_SUPPORT_NOMEDIA);
	SWITCH_ADD_APP(app_interface, "Goto", "Goto", "Goto", goto_function, "Goto", SAF_SUPPORT_NOMEDIA);

	sip_endpoint_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_ENDPOINT_INTERFACE);
	sip_endpoint_interface->interface_name = "SIP";
	sip_endpoint_interface->io_routines = &sip_io_routines;

	
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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */
