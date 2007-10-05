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
 * Brian West <brian.west@mac.com>
 * Bret McDanel <trixter AT 0xdecafbad.com>
 *
 * mod_xml_cdr.c -- XML CDR Module to files or curl
 *
 */
#include <sys/stat.h>
#include <switch.h>
#include <curl/curl.h>

static struct {
	char *cred;
	char *url;
	char *log_dir;
	char *err_log_dir;
	uint32_t delay;
	uint32_t retries;
	uint32_t shutdown;
	int ignore_cacert_check;
} globals;

SWITCH_MODULE_LOAD_FUNCTION(mod_xml_cdr_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_xml_cdr_shutdown);
SWITCH_MODULE_DEFINITION(mod_xml_cdr, mod_xml_cdr_load, NULL, NULL);

/* this function would have access to the HTML returned by the webserver, we dont need it 
 * and the default curl activity is to print to stdout, something not as desirable
 * so we have a dummy function here
 */
static void httpCallBack()
{
	return;
}

static switch_status_t my_on_hangup(switch_core_session_t *session)
{
	switch_xml_t cdr;
	char *xml_text = NULL;
	char *path = NULL;
	char *curl_xml_text = NULL;
	char *logdir = NULL;
	char *xml_text_escaped = NULL;
	int fd = -1;
	uint32_t cur_try;
	long httpRes;
	CURL *curl_handle = NULL;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (switch_ivr_generate_xml_cdr(session, &cdr) == SWITCH_STATUS_SUCCESS) {

		/* build the XML */
		if (!(xml_text = switch_xml_toxml(cdr))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
			goto error;
		}

		/* do we log to the disk no matter what? */
		/* all previous functionality is retained */

		if (!(logdir = switch_channel_get_variable(channel, "xml_cdr_base"))) {
			logdir = globals.log_dir;
		}

		if (!switch_strlen_zero(logdir)) {
			if ((path = switch_mprintf("%s/%s.cdr.xml", logdir, switch_core_session_get_uuid(session)))) {
				if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) > -1) {
					int wrote;
					wrote = write(fd, xml_text, (unsigned) strlen(xml_text));
					close(fd);
					fd = -1;
				} else {
					char ebuf[512] = { 0 };
#ifdef WIN32
					strerror_s(ebuf, sizeof(ebuf), errno);
#else
					strerror_r(errno, ebuf, sizeof(ebuf));
#endif
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error writing [%s][%s]\n", path, ebuf);
				}
				switch_safe_free(path);
			}
		}

		/* try to post it to the web server */
		if (!switch_strlen_zero(globals.url)) {
			curl_handle = curl_easy_init();

			xml_text_escaped = curl_easy_escape(curl_handle, (const char*) xml_text, (int)strlen(xml_text));

			if (switch_strlen_zero(xml_text_escaped)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
				goto error;
			}

			if (!(curl_xml_text = switch_mprintf("cdr=%s", xml_text_escaped))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
				goto error;
			}

			if (!switch_strlen_zero(globals.cred)) {
				curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
				curl_easy_setopt(curl_handle, CURLOPT_USERPWD, globals.cred);
			}

			curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, curl_xml_text);
			curl_easy_setopt(curl_handle, CURLOPT_URL, globals.url);
			curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "freeswitch-xml/1.0");
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, httpCallBack);

			if (globals.ignore_cacert_check) {
				curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
			}

			/* these were used for testing, optionally they may be enabled if someone desires
			curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 120); // tcp timeout
			curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1); // 302 recursion level
			*/

			for (cur_try = 0; cur_try < globals.retries; cur_try++) {
				if (cur_try > 0) {
					switch_yield(globals.delay * 1000000);
				}
				curl_easy_perform(curl_handle);
				curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &httpRes);
				if (httpRes == 200) {
					goto success;
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Got error [%ld] posting to web server [%s]\n",httpRes, globals.url);
				}
				
			}
			curl_easy_cleanup(curl_handle);
			curl_handle = NULL;

			/* if we are here the web post failed for some reason */
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unable to post to web server, writing to file\n");

			if ((path = switch_mprintf("%s/%s.cdr.xml", globals.err_log_dir, switch_core_session_get_uuid(session)))) {
				if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) > -1) {
					int wrote;
					wrote = write(fd, xml_text, (unsigned) strlen(xml_text));
					close(fd);
					fd = -1;
				} else {
					char ebuf[512] = { 0 };
#ifdef WIN32
					strerror_s(ebuf, sizeof(ebuf), errno);
#else
					strerror_r(errno, ebuf, sizeof(ebuf));
#endif
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error![%s]\n", ebuf);
				}
			}
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Generating Data!\n");
	}


success:
	status = SWITCH_STATUS_SUCCESS;

error:
	if (curl_handle) {	
		curl_easy_cleanup(curl_handle);
	}
	switch_safe_free(curl_xml_text);
	switch_safe_free(xml_text_escaped);
	switch_safe_free(path);
	switch_safe_free(xml_text);
	switch_xml_free(cdr);

	return status;
}


static switch_state_handler_table_t state_handlers = {
	/*.on_init */ NULL,
	/*.on_ring */ NULL,
	/*.on_execute */ NULL,
	/*.on_hangup */ my_on_hangup,
	/*.on_loopback */ NULL,
	/*.on_transmit */ NULL
};

SWITCH_MODULE_LOAD_FUNCTION(mod_xml_cdr_load)
{
	char *cf = "xml_cdr.conf";
	switch_xml_t cfg, xml, settings, param;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	/* test global state handlers */
	switch_core_add_state_handler(&state_handlers);

	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	memset(&globals,0,sizeof(globals));

	/* parse the config */
	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}

	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "cred")) {
				globals.cred = strdup(val);
			} else if (!strcasecmp(var, "url")) {
				globals.url = strdup(val);
			} else if (!strcasecmp(var, "delay")) {
				globals.delay = (uint32_t) atoi(val);
			} else if (!strcasecmp(var, "retries")) {
				globals.retries = (uint32_t) atoi(val);
			} else if (!strcasecmp(var, "log-dir")) {
				if (switch_strlen_zero(val)) {
					globals.log_dir = switch_mprintf("%s/xml_cdr", SWITCH_GLOBAL_dirs.log_dir);
				} else {
					if (switch_is_file_path(val)) {
						globals.log_dir = strdup(val);
					} else {
						globals.log_dir = switch_mprintf("%s/%s", SWITCH_GLOBAL_dirs.log_dir, val);
					}
				}
			} else if (!strcasecmp(var, "err-log-dir")) {
				if (switch_strlen_zero(val)) {
					globals.err_log_dir = switch_mprintf("%s/xml_cdr", SWITCH_GLOBAL_dirs.log_dir);
				} else {
					if (switch_is_file_path(val)) {
						globals.err_log_dir = strdup(val);
					} else {
						globals.err_log_dir = switch_mprintf("%s/%s", SWITCH_GLOBAL_dirs.log_dir, val);
					}
				}
			} else if (!strcasecmp(var, "ignore-cacert-check") && switch_true(val)) {
				globals.ignore_cacert_check = 1;
			}

			if (switch_strlen_zero(globals.err_log_dir)) {
				if (!switch_strlen_zero(globals.log_dir)) {
					globals.err_log_dir = strdup(globals.log_dir);
				} else {
					globals.err_log_dir = switch_mprintf("%s/xml_cdr", SWITCH_GLOBAL_dirs.log_dir);
				}
			}
		}
	}
	if (globals.retries < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "retries is negative, setting to 0\n");
		globals.retries = 0;
	}


	if (globals.retries && globals.delay<=0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "retries set but delay 0 setting to 5000ms\n");
		globals.delay = 5000;
	}
	
	switch_xml_free(xml);
	return status;
}


SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_xml_cdr_shutdown)
{
	
	globals.shutdown=1;
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
