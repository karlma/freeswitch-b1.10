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
 * mod_dialplan_demo.c -- Example Dialplan Module
 *
 */
#include <switch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static const char modname[] = "mod_dialplan_directory";

static struct {
	char *directory_name;
	char *host;
	char *dn;
	char *pass;
	char *base;
} globals;

SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_directory_name, globals.directory_name)
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_host, globals.host)
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_dn, globals.dn)
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_pass, globals.pass)
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_base, globals.base)

static void load_config(void)
{
	char *cf = "dialplan_directory.conf";
	switch_config cfg;
	char *var, *val;

	if (!switch_config_open_file(&cfg, cf)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return;
	}

	while (switch_config_next_pair(&cfg, &var, &val)) {
		if (!strcasecmp(cfg.category, "settings")) {
			if (!strcmp(var, "directory_name") && val) {
				set_global_directory_name(val);
			} else if (!strcmp(var, "directory_name") && val) {
				set_global_directory_name(val);
			} else if (!strcmp(var, "host") && val) {
				set_global_host(val);
			} else if (!strcmp(var, "dn") && val) {
				set_global_dn(val);
			} else if (!strcmp(var, "pass") && val) {
				set_global_pass(val);
			} else if (!strcmp(var, "base") && val) {
				set_global_base(val);
			}
		}
	}
	switch_config_close_file(&cfg);	
}

static switch_caller_extension *directory_dialplan_hunt(switch_core_session *session)
{
	switch_caller_profile *caller_profile;
	switch_caller_extension *extension = NULL;
	switch_channel *channel;
	char *var, *val;
	char filter[256];
	switch_directory_handle dh;
	char app[512];
	char *data;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	caller_profile = switch_channel_get_caller_profile(channel);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Hello %s You Dialed %s!\n", caller_profile->caller_id_name,
						  caller_profile->destination_number);
	
	
	if (! (globals.directory_name && globals.host && globals.dn && globals.base && globals.pass)) {
		return NULL;
	}

	if (switch_core_directory_open(&dh, 
								   globals.directory_name,
								   globals.host,
								   globals.dn,
								   globals.pass,
								   NULL) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't connect\n");
		return NULL;
	}

	sprintf(filter, "exten=%s", caller_profile->destination_number);

	switch_core_directory_query(&dh, globals.base, filter);
	while (switch_core_directory_next(&dh) == SWITCH_STATUS_SUCCESS) {
		while (switch_core_directory_next_pair(&dh, &var, &val) == SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "DIRECTORY VALUE [%s]=[%s]\n", var, val);
			if(!strcasecmp(var, "callflow")) {
				if (!extension) {
					if ((extension = switch_caller_extension_new(session, caller_profile->destination_number,
																  caller_profile->destination_number)) == 0) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "memory error!\n");
						goto out;
					}
				}
				switch_copy_string(app, val, sizeof(app));
				if ((data = strchr(app, ' ')) != 0) {
					*data++ = '\0';
				}
				switch_caller_extension_add_application(session, extension, app, data);
			}
		}
	}
 out:
	
	switch_core_directory_close(&dh);

	
	if (extension) {
		switch_channel_set_state(channel, CS_EXECUTE);
	} else {
		switch_channel_hangup(channel, SWITCH_CAUSE_MESSAGE_TYPE_NONEXIST);
	}

	return extension;
}


static const switch_dialplan_interface directory_dialplan_interface = {
	/*.interface_name = */ "directory",
	/*.hunt_function = */ directory_dialplan_hunt
	/*.next = NULL */
};

static const switch_loadable_module_interface directory_dialplan_module_interface = {
	/*.module_name = */ modname,
	/*.endpoint_interface = */ NULL,
	/*.timer_interface = */ NULL,
	/*.dialplan_interface = */ &directory_dialplan_interface,
	/*.codec_interface = */ NULL,
	/*.application_interface = */ NULL
};

SWITCH_MOD_DECLARE(switch_status) switch_module_load(const switch_loadable_module_interface **interface, char *filename)
{

	load_config();
	/* connect my internal structure to the blank pointer passed to me */
	*interface = &directory_dialplan_module_interface;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}
