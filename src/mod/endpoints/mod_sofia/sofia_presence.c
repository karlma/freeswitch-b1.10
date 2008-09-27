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
 * Ken Rice, Asteria Solutions Group, Inc <ken@asteriasgi.com>
 * Paul D. Tinsley <pdt at jackhammer.org>
 * Bret McDanel <trixter AT 0xdecafbad.com>
 *
 *
 * sofia_presence.c -- SOFIA SIP Endpoint (presence code)
 *
 */
#include "mod_sofia.h"

static int sofia_presence_mwi_callback(void *pArg, int argc, char **argv, char **columnNames);
static int sofia_presence_mwi_callback2(void *pArg, int argc, char **argv, char **columnNames);
static int sofia_presence_sub_reg_callback(void *pArg, int argc, char **argv, char **columnNames);
static int sofia_presence_resub_callback(void *pArg, int argc, char **argv, char **columnNames);
static int sofia_presence_sub_callback(void *pArg, int argc, char **argv, char **columnNames);



struct presence_helper {
	sofia_profile_t *profile;
	switch_event_t *event;
	switch_stream_handle_t stream;
	char last_uuid[512];
};

switch_status_t sofia_presence_chat_send(char *proto, char *from, char *to, char *subject, char *body, char *hint)
{
	char buf[256];
	char *prof = NULL, *user = NULL, *host = NULL;
	sofia_profile_t *profile = NULL;
	char *ffrom = NULL;
	nua_handle_t *msg_nh;
	char *contact;
	char *dup = NULL;
	switch_status_t status = SWITCH_STATUS_FALSE;
	const char *ct = "text/html";

	if (subject && strchr(subject, '/')) {
		ct = subject;
	}

	if (!to) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing To: header.\n");
		goto end;
	}

	dup = strdup(to);
	switch_assert(dup);
	prof = dup;

	// Do we have a user of the form profile/user[@host]?
	if ((user = strchr(prof, '/'))) {
		*user++ = '\0';
	} else {
		user = prof;
		prof = NULL;
	}  

	if ((host = strchr(user, '@'))) {
		*host++ = '\0';
		if (!prof) prof = host;
	}

	if (!host || !(profile = sofia_glue_find_profile(prof))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
						  "Chat proto [%s]\nfrom [%s]\nto [%s]\n%s\nInvalid Profile %s\n", proto, from, to,
						  body ? body : "[no body]", prof ? prof : "NULL");
		goto end;
	}

	if (!sofia_reg_find_reg_url(profile, user, host, buf, sizeof(buf))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find user. [%s][%s]\n", user, host);
		goto end;
	}

	if (!strcasecmp(proto, SOFIA_CHAT_PROTO)) {
		from = hint;
	} else {
		char *fp, *p, *fu = NULL;
		fp = strdup(from);
		if (!fp) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
			goto end;
		}

		if ((p = strchr(fp, '@'))) {
			*p = '\0';
			fu = strdup(fp);
			*p = '+';
		}

		ffrom = switch_mprintf("\"%s\" <sip:%s+%s@%s>", fu, proto, fp, profile->domain_name);
		from = ffrom;
		switch_safe_free(fu);
		switch_safe_free(fp);
	}

	status = SWITCH_STATUS_SUCCESS;
	contact = sofia_glue_get_url_from_contact(buf, 1);
	// if this cries, add contact here too, change the 1 to 0 and omit the safe_free
	msg_nh = nua_handle(profile->nua, NULL, SIPTAG_FROM_STR(from), NUTAG_URL(contact), SIPTAG_TO_STR(buf),	
						SIPTAG_CONTACT_STR(profile->url), TAG_END());

	switch_safe_free(contact);
	nua_message(msg_nh, SIPTAG_CONTENT_TYPE_STR(ct), SIPTAG_PAYLOAD_STR(body), TAG_END());


 end:

	switch_safe_free(ffrom);
	switch_safe_free(dup);


	if (profile) {
		switch_thread_rwlock_unlock(profile->rwlock);
	}

	return status;
}

void sofia_presence_cancel(void)
{
	char *sql;
	sofia_profile_t *profile;
	switch_hash_index_t *hi;
	void *val;
	struct presence_helper helper = { 0 };

	if (!mod_sofia_globals.profile_hash)
		return;
	
	if ((sql = switch_mprintf(
							  "select proto,sip_user,sip_host,sub_to_user,sub_to_host,event,contact,call_id,full_from,"
							  "full_via,expires,user_agent,accept,profile_name"
							  ",-1,'unavailable','unavailable' from sip_subscriptions where event='presence'"))) {
		switch_mutex_lock(mod_sofia_globals.hash_mutex);
		for (hi = switch_hash_first(NULL, mod_sofia_globals.profile_hash); hi; hi = switch_hash_next(hi)) {
			switch_hash_this(hi, NULL, NULL, &val);
			profile = (sofia_profile_t *) val;
			if (profile->pres_type != PRES_TYPE_FULL) {
				continue;
			}
			helper.profile = profile;
			helper.event = NULL;
			if (sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex, sql, sofia_presence_sub_callback, &helper) != SWITCH_TRUE) {
				continue;
			}
		}
		switch_safe_free(sql);
		switch_mutex_unlock(mod_sofia_globals.hash_mutex);
	}
}

void sofia_presence_establish_presence(sofia_profile_t *profile)
{

	if (sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex,
										"select sip_user,sip_host,'Registered','unknown','' from sip_registrations",
										sofia_presence_resub_callback, profile) != SWITCH_TRUE) {
		return;
	}

	if (sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex,
										"select sub_to_user,sub_to_host,'Online','unknown',proto from sip_subscriptions "
										"where proto='ext' or proto='user' or proto='conf'", sofia_presence_resub_callback, profile) != SWITCH_TRUE) {
		return;
	}
}

char *sofia_presence_translate_rpid(char *in, char *ext)
{
	char *r = in;

	if (in && (switch_stristr("null", in))) {
		in = NULL;
	}

	if (!in) {
		in = ext;
	}

	if (!in) {
		return NULL;
	}

	if (!strcasecmp(in, "dnd") || !strcasecmp(in, "idle")) {
		r = "busy";
	}

	return r;
}

struct mwi_helper {
	sofia_profile_t *profile;
	int total;
};

static void actual_sofia_presence_mwi_event_handler(switch_event_t *event)
{
	char *account, *dup_account, *yn, *host, *user;
	char *sql;
	sofia_profile_t *profile = NULL;
	switch_stream_handle_t stream = { 0 };
	switch_event_header_t *hp;
	struct mwi_helper h = { 0 };
	char *pname = NULL;

	switch_assert(event != NULL);

	if (!(account = switch_event_get_header(event, "mwi-message-account"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing required Header 'MWI-Message-Account'\n");
		return;
	}

	if (!(yn = switch_event_get_header(event, "mwi-messages-waiting"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing required Header 'MWI-Messages-Waiting'\n");
		return;
	}

	dup_account = strdup(account);
	switch_assert(dup_account != NULL);
	sofia_glue_get_user_host(dup_account, &user, &host);


	if ((pname = switch_event_get_header(event, "sofia-profile"))) {
		if (!(profile = sofia_glue_find_profile(pname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "no profile %s\n", pname);
		}
	}

	if (!profile) {
		if (!host || !(profile = sofia_glue_find_profile(host))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find profile %s\n", switch_str_nil(host));
			switch_safe_free(dup_account);
			return;
		}
	}

	if (profile->domain_name && strcasecmp(profile->domain_name, host)) {
		host = profile->domain_name;
	}

	h.profile = profile;
	h.total = 0;


	SWITCH_STANDARD_STREAM(stream);

	for (hp = event->headers; hp; hp = hp->next) {
		if (!strncasecmp(hp->name, "mwi-", 4)) {
			char *tmp = NULL;
			char *value = hp->value;
			if (!strcasecmp(hp->name, "mwi-message-account") && strncasecmp(hp->value, "sip:", 4)) {
				tmp = switch_mprintf("sip:%s", hp->value);
				value = tmp;
			}
			stream.write_function(&stream, "%s: %s\r\n", hp->name + 4, value);
			switch_safe_free(tmp);
		}
	}

	stream.write_function(&stream, "\r\n");
	
	sql = switch_mprintf("select proto,sip_user,sip_host,sub_to_user,sub_to_host,event,contact,call_id,full_from,"
						 "full_via,expires,user_agent,accept,profile_name"
						 ",'%q','%q' from sip_subscriptions where event='message-summary' "
						 "and sub_to_user='%q' and (sub_to_host='%q' or presence_hosts like '%%%q%%')",
						 stream.data, host, user, host, host);

	switch_assert(sql != NULL);
	sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex, sql, sofia_presence_mwi_callback, &h);
	
	switch_safe_free(sql);

	sql = switch_mprintf("select sip_user,sip_host,contact,profile_name,'%q' from sip_registrations where sip_user='%q' and sip_host='%q'", 
						 stream.data, user, host);



	switch_assert(sql != NULL);
	sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex, sql, sofia_presence_mwi_callback2, &h);

	switch_safe_free(sql);

	switch_safe_free(stream.data);

	switch_safe_free(dup_account);
	if (profile) {
		sofia_glue_release_profile(profile);
	}
}

static void actual_sofia_presence_event_handler(switch_event_t *event)
{
	sofia_profile_t *profile = NULL;
	switch_hash_index_t *hi;
	void *val;
	char *from = switch_event_get_header(event, "from");
	char *proto = switch_event_get_header(event, "proto");
	char *rpid = switch_event_get_header(event, "rpid");
	char *status = switch_event_get_header(event, "status");
	char *event_type = switch_event_get_header(event, "event_type");
	char *alt_event_type = switch_event_get_header(event, "alt_event_type");
	char *sql = NULL;
	char *euser = NULL, *user = NULL, *host = NULL;


	if (!mod_sofia_globals.running) {
		return;
	}

	if (rpid && !strcasecmp(rpid, "n/a")) {
		rpid = NULL;
	}

	if (status && !strcasecmp(status, "n/a")) {
		status = NULL;
	}

	if (rpid) {
		rpid = sofia_presence_translate_rpid(rpid, status);
	}

	if (!status) {
		status = "Available";

		if (rpid) {
			if (!strcasecmp(rpid, "busy")) {
				status = "Busy";
			} else if (!strcasecmp(rpid, "unavailable")) {
				status = "Idle";
			} else if (!strcasecmp(rpid, "away")) {
				status = "Idle";
			}
		}
	}

	if (!rpid) {
		rpid = "unknown";
	}

	if (event->event_id == SWITCH_EVENT_ROSTER) {
		struct presence_helper helper = { 0 };

		if (!mod_sofia_globals.profile_hash)
			return;

		if (from) {
			sql = switch_mprintf(
								 "select proto,sip_user,sip_host,sub_to_user,sub_to_host,event,contact,call_id,full_from,"
								 "full_via,expires,user_agent,accept,profile_name"
								 ",1,'%q','%q' from sip_subscriptions where event='presence' and full_from like '%%%q%%'", status, rpid, from);
		} else {
			sql = switch_mprintf("select proto,sip_user,sip_host,sub_to_user,sub_to_host,event,contact,call_id,full_from,"
								 "full_via,expires,user_agent,accept,profile_name"
								 ",1,'%q','%q' from sip_subscriptions where event='presence'", status, rpid);
		}

		switch_assert(sql != NULL);
		switch_mutex_lock(mod_sofia_globals.hash_mutex);
		for (hi = switch_hash_first(NULL, mod_sofia_globals.profile_hash); hi; hi = switch_hash_next(hi)) {
			switch_hash_this(hi, NULL, NULL, &val);
			profile = (sofia_profile_t *) val;
			if (profile->pres_type != PRES_TYPE_FULL) {
				continue;
			}
			helper.profile = profile;
			helper.event = NULL;
			sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex, sql, sofia_presence_sub_callback, &helper);
		}
		switch_mutex_unlock(mod_sofia_globals.hash_mutex);
		free(sql);
		return;
	}

	if (switch_strlen_zero(event_type)) {
		event_type = "presence";
	}

	if (switch_strlen_zero(alt_event_type)) {
		alt_event_type = "presence";
	}

	if ((user = strdup(from))) {
		if ((host = strchr(user, '@'))) {
			char *p;
			*host++ = '\0';
			if ((p = strchr(host, '/'))) {
				*p = '\0';
			}
		} else {
			switch_safe_free(user);
			return;
		}
		if ((euser = strchr(user, '+'))) {
			euser++;
		} else {
			euser = user;
		}
	} else {
		return;
	}

	switch (event->event_id) {
	case SWITCH_EVENT_PRESENCE_PROBE:
		if (proto) {
			char *to = switch_event_get_header(event, "to");
			char *probe_user, *probe_euser, *probe_host, *p;

			if (!to || !(probe_user = strdup(to))) {
				return;
			}

			if ((probe_host = strchr(probe_user, '@'))) {
				*probe_host++ = '\0';
			}
			probe_euser = probe_user;
			if ((p = strchr(probe_euser, '+'))) {
				probe_euser = (p + 1);
			}

			if (probe_euser && probe_host && (profile = sofia_glue_find_profile(probe_host))) {
				sql = switch_mprintf("select sip_registrations.sip_user, '%q', sip_registrations.status, "
									 "sip_registrations.rpid,'', sip_dialogs.uuid, sip_dialogs.state, sip_dialogs.direction, "
									 "sip_dialogs.sip_to_user, sip_dialogs.sip_to_host "
									 "from sip_registrations left join sip_dialogs on "
									 "(sip_dialogs.sip_from_user = sip_registrations.sip_user) "
									 "and sip_dialogs.sip_from_host = sip_registrations.sip_host "
									 "where sip_registrations.sip_user='%q' and "
									 "(sip_registrations.sip_host='%q' or sip_registrations.presence_hosts like '%%%q%%')", 
									 probe_host, probe_euser, probe_host, probe_host);
				switch_assert(sql);
				sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex, sql, sofia_presence_resub_callback, profile);

				sofia_glue_release_profile(profile);
				switch_safe_free(sql);
			}

			switch_safe_free(user);
			switch_safe_free(probe_user);
		}
		return;
	case SWITCH_EVENT_PRESENCE_IN:
		sql = switch_mprintf("select proto,sip_user,sip_host,sub_to_user,sub_to_host,event,contact,call_id,full_from,"
							 "full_via,expires,user_agent,accept,profile_name"
							 ",1,'%q','%q','%q' from sip_subscriptions where (event='%q' or event='%q') and sub_to_user='%q' "
							 "and (sub_to_host='%q' or presence_hosts like '%%%q%%')",
							 status, rpid, host, event_type, alt_event_type, euser, host, host);
		break;
	case SWITCH_EVENT_PRESENCE_OUT:
		sql = switch_mprintf("select proto,sip_user,sip_host,sub_to_user,sub_to_host,event,contact,call_id,full_from,"
							 "full_via,expires,user_agent,accept,profile_name"
							 ",0,'%q','%q','%q' from sip_subscriptions where (event='%q' or event='%q') "
							 "and sub_to_user='%q' and (sub_to_host='%q' or presence_hosts like '%%%q%%')",
							 status, rpid, host, event_type, alt_event_type, euser, host, host);
		break;
	default:
		break;
	}

	if (!mod_sofia_globals.profile_hash)
		goto done;

	switch_mutex_lock(mod_sofia_globals.hash_mutex);
	for (hi = switch_hash_first(NULL, mod_sofia_globals.profile_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		profile = (sofia_profile_t *) val;
		if (profile->pres_type != PRES_TYPE_FULL) {
			continue;
		}

		if (sql) {
			struct presence_helper helper = { 0 };
			helper.profile = profile;
			helper.event = event;
			SWITCH_STANDARD_STREAM(helper.stream);
			switch_assert(helper.stream.data);

			sofia_glue_execute_sql_callback(profile, SWITCH_FALSE,
											//profile->ireg_mutex,
											NULL, sql, sofia_presence_sub_callback, &helper);

			if (!switch_strlen_zero((char *) helper.stream.data)) {
				char *this = (char *) helper.stream.data;
				char *next = NULL;
				char *last = NULL;

				do {
					if ((next = strchr(this, ';'))) {
						*next++ = '\0';
						while (*next == '\n' || *next == ' ' || *next == '\r') {
							*next++ = '\0';
						}
					}

					if (!switch_strlen_zero(this) && (!last || strcmp(last, this))) {
						sofia_glue_execute_sql(profile, &this, SWITCH_FALSE);
						last = this;
					}
					this = next;
				} while (this);
			}
			switch_safe_free(helper.stream.data);
			helper.stream.data = NULL;
		}
	}
	switch_mutex_unlock(mod_sofia_globals.hash_mutex);

  done:
	switch_safe_free(sql);
	switch_safe_free(user);
}

static int EVENT_THREAD_RUNNING = 0;
static int EVENT_THREAD_STARTED = 0;

void *SWITCH_THREAD_FUNC sofia_presence_event_thread_run(switch_thread_t *thread, void *obj)
{
	void *pop;
	int done = 0;

	switch_mutex_lock(mod_sofia_globals.mutex);
	if (!EVENT_THREAD_RUNNING) {
		EVENT_THREAD_RUNNING++;
		mod_sofia_globals.threads++;
	} else {
		done = 1;
	}
	switch_mutex_unlock(mod_sofia_globals.mutex);

	if (done) {
		return NULL;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Event Thread Started\n");

	while (mod_sofia_globals.running == 1) {
		int count = 0;

		if (switch_queue_trypop(mod_sofia_globals.presence_queue, &pop) == SWITCH_STATUS_SUCCESS) {
			switch_event_t *event = (switch_event_t *) pop;

			if (!pop) {
				break;
			}

			actual_sofia_presence_event_handler(event);
			switch_event_destroy(&event);
			count++;
		}

		if (switch_queue_trypop(mod_sofia_globals.mwi_queue, &pop) == SWITCH_STATUS_SUCCESS) {
			switch_event_t *event = (switch_event_t *) pop;

			if (!pop) {
				break;
			}

			actual_sofia_presence_mwi_event_handler(event);
			switch_event_destroy(&event);
			count++;
		}

		if (!count) {
			switch_yield(100000);
		}
	}

	while (switch_queue_trypop(mod_sofia_globals.presence_queue, &pop) == SWITCH_STATUS_SUCCESS && pop) {
		switch_event_t *event = (switch_event_t *) pop;
		switch_event_destroy(&event);
	}

	while (switch_queue_trypop(mod_sofia_globals.mwi_queue, &pop) == SWITCH_STATUS_SUCCESS && pop) {
		switch_event_t *event = (switch_event_t *) pop;
		switch_event_destroy(&event);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Event Thread Ended\n");

	switch_mutex_lock(mod_sofia_globals.mutex);
	mod_sofia_globals.threads--;
	EVENT_THREAD_RUNNING = EVENT_THREAD_STARTED = 0;
	switch_mutex_unlock(mod_sofia_globals.mutex);

	return NULL;
}

void sofia_presence_event_thread_start(void)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;
	int done = 0;

	switch_mutex_lock(mod_sofia_globals.mutex);
	if (!EVENT_THREAD_STARTED) {
		EVENT_THREAD_STARTED++;
	} else {
		done = 1;
	}
	switch_mutex_unlock(mod_sofia_globals.mutex);

	if (done) {
		return;
	}

	switch_threadattr_create(&thd_attr, mod_sofia_globals.pool);
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_threadattr_priority_increase(thd_attr);
	switch_thread_create(&thread, thd_attr, sofia_presence_event_thread_run, NULL, mod_sofia_globals.pool);
}


void sofia_presence_event_handler(switch_event_t *event)
{
	switch_event_t *cloned_event;

	switch_event_dup(&cloned_event, event);
	switch_assert(cloned_event);
	switch_queue_push(mod_sofia_globals.presence_queue, cloned_event);

	if (!EVENT_THREAD_STARTED) {
		sofia_presence_event_thread_start();
	}
}

void sofia_presence_mwi_event_handler(switch_event_t *event)
{
	switch_event_t *cloned_event;

	switch_event_dup(&cloned_event, event);
	switch_assert(cloned_event);
	switch_queue_push(mod_sofia_globals.mwi_queue, cloned_event);

	if (!EVENT_THREAD_STARTED) {
		sofia_presence_event_thread_start();
	}
}


static int sofia_presence_sub_reg_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	sofia_profile_t *profile = (sofia_profile_t *) pArg;
	char *user = argv[1];
	char *host = argv[2];
	switch_event_t *event;
	char *status = NULL;
	char *event_name = argv[5];

	if (!strcasecmp(event_name, "message-summary")) {
		if (switch_event_create(&event, SWITCH_EVENT_MESSAGE_QUERY) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Message-Account", "sip:%s@%s", user, host);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "VM-Sofia-Profile", profile->name);
			switch_event_fire(&event);
		}
		return 0;
	}

	if (switch_strlen_zero(status)) {
		status = "Available";
	}
	if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_PROBE) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", SOFIA_CHAT_PROTO);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "login", profile->url);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s@%s", user, host);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "status", status);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "event_type", "presence");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "event_subtype", "probe");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto-specific-event-name", event_name);
		switch_event_fire(&event);
	}

	return 0;
}

static int sofia_presence_resub_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	sofia_profile_t *profile = (sofia_profile_t *) pArg;
	char *user = argv[0];
	char *host = argv[1];
	char *status = argv[2];
	char *rpid = argv[3];
	char *proto = argv[4];
	char *to_user = NULL;
	char *uuid = NULL;
	char *state = NULL;
	char *direction = NULL;
	switch_event_t *event;
	char to_buf[128] = "";

	if (argc > 5) {
		uuid = switch_str_nil(argv[5]);
		state = switch_str_nil(argv[6]);
		direction = switch_str_nil(argv[7]);
		if (argc > 8) {
			switch_set_string(to_buf, argv[8]);
			switch_url_decode(to_buf);
			to_user = to_buf;
		}
	}

	if (switch_strlen_zero(proto)) {
		proto = NULL;
	}

	if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", proto ? proto : SOFIA_CHAT_PROTO);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "login", profile->url);
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s@%s", user, host);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "status", status);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "rpid", rpid);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "event_type", "presence");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "alt_event_type", "dialog");
		switch_event_add_header(event, SWITCH_STACK_BOTTOM, "event_count", "%d", 0);

		if (!switch_strlen_zero(to_user)) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "to-user", to_user);
		}

		if (switch_strlen_zero(state)) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "unique-id", SOFIA_CHAT_PROTO);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "channel-state", "CS_HANGUP");
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "answer-state", "resubscribe");
		} else {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "channel-state", "CS_ROUTING");
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "unique-id", uuid);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "answer-state", state);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "astate", state);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "call-direction", direction);
		}

		switch_event_fire(&event);
	}

	return 0;
}

static char *translate_rpid(char *in)
{
	char *r = in;

	if (in && (strstr(in, "null") || strstr(in, "NULL"))) {
		in = NULL;
	}

	if (!in || !strcasecmp(in, "unknown")) {
		r = "online";
		goto end;
	}

	if (!strcasecmp(in, "busy")) {
		r = in;
		goto end;
	}

	if (!strcasecmp(in, "unavailable")) {
		r = "away";
		goto end;
	}

	if (!strcasecmp(in, "idle")) {
		r = "busy";
	}

  end:
	return r;
}


static char *gen_pidf(char *user_agent, char *id, char *url, char *open, char *rpid, char *prpid, char *status, const char **ct)
{
	if (switch_stristr("polycom", user_agent)) {
		*ct = "application/xpidf+xml";
		return switch_mprintf(
							  "<?xml version=\"1.0\"?>\n"
							  "<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n"
							  "<presence>\n"
							  " <status>\n"
							  "  <note>%s</note>\n"
							  " </status>\n"
							  " <presentity uri=\"%s;method=SUBSCRIBE\" />\n"
							  " <atom id=\"%s\">\n"
							  "  <address uri=\"%s;user=ip\" priority=\"0.800000\">\n"
							  "   <status status=\"%s\" />\n"
							  "   <msnsubstatus substatus=\"%s\" />\n"
							  "  </address>\n"
							  " </atom>\n"
							  "</presence>\n", status, id, id, url, open, prpid
							  );
	} else {
		*ct = "application/pidf+xml";
		return switch_mprintf(
							  "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?> \n"
							  "<presence xmlns='urn:ietf:params:xml:ns:pidf' \n"
							  "xmlns:dm='urn:ietf:params:xml:ns:pidf:data-model' \n"
							  "xmlns:rpid='urn:ietf:params:xml:ns:pidf:rpid' \n"
							  "xmlns:c='urn:ietf:params:xml:ns:pidf:cipid' entity='%s'>\n"
							  
							  " <tuple id='t6a5ed77e'>\n"
							  "  <status>\r\n"
							  "   <basic>%s</basic>\n"
							  "  </status>\n"
							  " </tuple>\n"
							  " <dm:person id='p06360c4a'>\n"
							  "  <rpid:activities>\r\n" 
							  "   <rpid:%s/>\n"
							  "  </rpid:activities>\n"
							  "  <dm:note>%s</dm:note>"
							  " </dm:person>\n"
							  "</presence>", id, prpid, rpid, status);
	}
}


static int sofia_presence_sub_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct presence_helper *helper = (struct presence_helper *) pArg;
	char *pl = NULL;
	char *clean_id = NULL, *id = NULL;
	uint32_t in = atoi(argv[14]);
	char *status = argv[15];
	char *rpid = argv[16];

	char *proto = argv[0];
	char *user = argv[1];
	char *host = argv[2];
	char *sub_to_user = argv[3];
	char *sub_to_host = argv[17];
	char *event = argv[5];
	char *call_id = argv[7];
	char *expires = argv[10];
	char *user_agent = argv[11];
	char *profile_name = argv[13];
	nua_handle_t *nh;
	char *to = NULL;
	char *open;
	char *prpid;
	int done = 0;
	const char *ct = "no/idea";
	time_t exptime = switch_timestamp(NULL) + 3600;
	char exp[80] = "";
	int is_dialog = 0;
	sofia_profile_t *ext_profile = NULL, *profile = helper->profile;

	
	if (profile_name && strcasecmp(profile_name, helper->profile->name)) {
        if ((ext_profile = sofia_glue_find_profile(profile_name))) {
            profile = ext_profile;
        }
    }
	

	if (expires) {
		long tmp = atol(expires);
		if (tmp > 0) {
			exptime = tmp - switch_timestamp(NULL);
		}
	}

	if (!(nh = nua_handle_by_call_id(profile->nua, call_id))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Cannot find handle for %s\n", call_id);
		goto end;
	}

	if (!rpid) {
		rpid = "unknown";
	}

	prpid = translate_rpid(rpid);

	if (in < 0) {
		done = 1;
		in = 0;
	}

	if (!strcasecmp(proto, SOFIA_CHAT_PROTO)) {
		clean_id = switch_mprintf("sip:%s@%s", sub_to_user, sub_to_host);
	} else {
		clean_id = switch_mprintf("sip:%s+%s@%s", proto, sub_to_user, sub_to_host);
	}

	if (!strcasecmp(sub_to_host, host)) {
		/* same host */
		id = switch_mprintf("sip:%s+%s@%s", proto, sub_to_user, sub_to_host);
	} else if (strcasecmp(proto, SOFIA_CHAT_PROTO)) {
		/*encapsulate */
		id = switch_mprintf("sip:%s+%s+%s@%s", proto, sub_to_user, sub_to_host, host);
	} else {
		id = switch_mprintf("sip:%s@%s", sub_to_user, sub_to_host);
	}

	to = switch_mprintf("sip:%s@%s", user, host);

	is_dialog = !strcmp(event, "dialog");

	if (helper->event) {
		switch_stream_handle_t stream = { 0 };
		const char *direction = switch_str_nil(switch_event_get_header(helper->event, "call-direction"));
		const char *uuid = switch_str_nil(switch_event_get_header(helper->event, "unique-id"));
		const char *state = switch_str_nil(switch_event_get_header(helper->event, "channel-state"));
		const char *event_status = switch_str_nil(switch_event_get_header(helper->event, "status"));
		const char *astate = switch_str_nil(switch_event_get_header(helper->event, "astate"));
		const char *answer_state = switch_str_nil(switch_event_get_header(helper->event, "answer-state"));
		const char *dft_state;
		const char *from_id = switch_str_nil(switch_event_get_header(helper->event, "Other-Leg-Caller-ID-Number"));
		const char *to_user = switch_str_nil(switch_event_get_header(helper->event, "variable_sip_to_user"));
		const char *from_user = switch_str_nil(switch_event_get_header(helper->event, "variable_sip_from_user"));
		const char *clean_to_user = NULL;
		const char *clean_from_user = NULL;
		const char *p_to_user = switch_str_nil(switch_event_get_header(helper->event, "to-user"));
#if 0
		char *buf;
		switch_event_serialize(helper->event, &buf, SWITCH_FALSE);
		switch_assert(buf);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "CHANNEL_DATA:\n%s\n", buf);
		free(buf);
#endif

		if (is_dialog) {
			SWITCH_STANDARD_STREAM(stream);
		}

		if (!strcasecmp(direction, "outbound")) {
			direction = "recipient";
			dft_state = "early";
		} else {
			direction = "initiator";
			dft_state = "confirmed";
		}

		if (!strcasecmp(state, "cs_execute") && !strstr(event_status, "hold")) {
			goto end;
		}

		if (!strcasecmp(event_status, "Registered")) {
			answer_state = "resubscribe";
		}

		if (is_dialog) {
			stream.write_function(&stream,
								  "<?xml version=\"1.0\"?>\n"
								  "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" "
								  "version=\"%s\" state=\"%s\" entity=\"%s\">\n",
								  switch_str_nil(switch_event_get_header(helper->event, "event_count")),
								  !strcasecmp(answer_state, "resubscribe") ? "partial" : "full", clean_id);
		}

		if (strcasecmp(answer_state, "resubscribe")) {

			if (!strcasecmp(state, "cs_hangup")) {
				astate = "terminated";
			} else if (switch_strlen_zero(astate)) {
				astate = switch_str_nil(switch_event_get_header(helper->event, "answer-state"));
				if (switch_strlen_zero(astate)) {
					if (is_dialog) {
						astate = dft_state;
					} else {
						astate = "terminated";
					}
				}
			}

			if (!strcasecmp(event_status, "hold")) {
				astate = "early";
			}

			if (!strcasecmp(astate, "answered")) {
				astate = "confirmed";
			}

			if (!strcasecmp(astate, "ringing")) {
				if (!strcasecmp(direction, "recipient")) {
					astate = "early";
				} else {
					astate = "confirmed";
				}
			}
			if (is_dialog) {
				stream.write_function(&stream, "<dialog id=\"%s\" direction=\"%s\">\n", uuid, direction);
				stream.write_function(&stream, "<state>%s</state>\n", astate);
			}
			if (!strcasecmp(astate, "early") || !strcasecmp(astate, "confirmed")) {

				clean_to_user = switch_mprintf("%s", sub_to_user ? sub_to_user : to_user);
				clean_from_user = switch_mprintf("%s", from_id ? from_id : from_user);

				if (is_dialog) {
					if (!switch_strlen_zero(clean_to_user) && !switch_strlen_zero(clean_from_user)) {
						stream.write_function(&stream, "<local>\n<identity display=\"%s\">sip:%s@%s</identity>\n", clean_to_user, clean_to_user, host);
						stream.write_function(&stream, "<target uri=\"sip:%s@%s\">\n", clean_to_user, host);
						stream.write_function(&stream, "<param pname=\"+sip.rendering\" pvalue=\"%s\"/>\n",
											  !strcasecmp(event_status, "hold") ? "no" : "yes");
						stream.write_function(&stream, "</target>\n</local>\n");
						stream.write_function(&stream, "<remote>\n<identity display=\"%s\">sip:%s@%s</identity>\n", clean_from_user, clean_from_user,
											  host);
						stream.write_function(&stream, "<target uri=\"sip:**%s@%s\"/>\n", clean_to_user, host);
						stream.write_function(&stream, "</remote>\n");
					} else if (!strcasecmp(proto, "park")) {
						stream.write_function(&stream, "<local>\n<identity display=\"parking\">sip:parking@%s;fifo=%s</identity>\n",
											  host, !switch_strlen_zero(clean_to_user) ? clean_to_user : "unknown");
						stream.write_function(&stream, "<target uri=\"sip:parking@%s\">\n", host);
						stream.write_function(&stream, "<param pname=\"+sip.rendering\" pvalue=\"no\"/>\n</target>\n</local>\n");
						stream.write_function(&stream, "<remote>\n<identity display=\"parking\">sip:%s</identity>\n", uuid);
						stream.write_function(&stream, "<target uri=\"sip:park+%s\"/>\n", uuid);
						stream.write_function(&stream, "</remote>\n");
					} else if (!strcasecmp(proto, "conf")) {
						stream.write_function(&stream, "<local>\n<identity display=\"conference\">sip:conference@%s;conference=%s</identity>\n",
											  host, !switch_strlen_zero(clean_to_user) ? clean_to_user : "unknown");
						stream.write_function(&stream, "<target uri=\"sip:conference@%s\">\n", host);
						stream.write_function(&stream, "<param pname=\"+sip.rendering\" pvalue=\"yes\"/>\n</target>\n</local>\n");
						stream.write_function(&stream, "<remote>\n<identity display=\"conference\">sip:%s@%s</identity>\n", uuid, host);
						stream.write_function(&stream, "<target uri=\"sip:conf+%s@%s\"/>\n", uuid, host);
						stream.write_function(&stream, "</remote>\n");
					}
				}
			}
			if (is_dialog) {
				stream.write_function(&stream, "</dialog>\n");
			}
		}

		if (is_dialog) {
			stream.write_function(&stream, "</dialog-info>\n");
			pl = stream.data;
			ct = "application/dialog-info+xml";
		}

		if (!switch_strlen_zero(astate) && !switch_strlen_zero(uuid) && helper && helper->stream.data && strcmp(helper->last_uuid, uuid)) {
			helper->stream.write_function(&helper->stream, "update sip_dialogs set state='%s' where uuid='%s';", astate, uuid);

			switch_copy_string(helper->last_uuid, uuid, sizeof(helper->last_uuid));
		}


		if (!is_dialog) {
			char status_line[256] = "";
			if (in) {
				if (!strcmp(astate, "early")) {
					switch_snprintf(status_line, sizeof(status_line), "Ring %s", switch_str_nil(from_id));
					rpid = "on-the-phone";
				} else if (!strcmp(astate, "confirmed")) {
					char *dest = switch_event_get_header(helper->event, "Caller-Destination-Number");
					if (switch_strlen_zero(from_id) && !switch_strlen_zero(dest)) {
						from_id = dest;
					}

					if (switch_strlen_zero(from_id)) {
						from_id = p_to_user;
					}
					
					if (switch_strlen_zero(from_id)) {
						switch_snprintf(status_line, sizeof(status_line), "On The Phone");
					} else {
						switch_snprintf(status_line, sizeof(status_line), "Talk %s", switch_str_nil(from_id));
					}
					rpid = "on-the-phone";
				} else if (!strcmp(astate, "terminated")) {
					if (!strcasecmp(rpid, "on-the-phone")) {
						switch_snprintf(status_line, sizeof(status_line), "Dialing");
					} else {
						switch_snprintf(status_line, sizeof(status_line), "Available");
					}
				} else {
					switch_set_string(status_line, status);
				}

				open = "open";
			} else {
				open = "closed";
			}
			
			if (!strncasecmp(status_line, "registered", 10)) {
				switch_snprintf(status_line, sizeof(status_line), "Available");
			}

			prpid = translate_rpid(rpid);
			pl = gen_pidf(user_agent, clean_id, profile->url, open, rpid, prpid, status_line, &ct);
		}

	} else {
		if (in) {
			open = "open";
		} else {
			open = "closed";
		}
		prpid = translate_rpid(rpid);
		pl = gen_pidf(user_agent, clean_id, profile->url, open, rpid, prpid, status, &ct);
	}
	
	switch_snprintf(exp, sizeof(exp), "active;expires=%ld", (long) exptime);
	nua_handle_bind(nh, &mod_sofia_globals.keep_private);
	nua_notify(nh, SIPTAG_SUBSCRIPTION_STATE_STR(exp), SIPTAG_EVENT_STR(event), SIPTAG_CONTENT_TYPE_STR(ct), SIPTAG_PAYLOAD_STR(pl), TAG_END());
	
  end:
	
	if (ext_profile) {
		sofia_glue_release_profile(ext_profile);
	}

	switch_safe_free(id);
	switch_safe_free(clean_id);
	switch_safe_free(pl);
	switch_safe_free(to);

	return 0;
}

static int sofia_presence_mwi_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	//char *proto = argv[0];
	//char *user = argv[1];
	//char *host = argv[2];
	char *sub_to_user = argv[3];
	char *sub_to_host = argv[15];
	char *event = argv[5];
	//char *contact = argv[6];
	char *call_id = argv[7];
	//char *full_from = argv[8];
	//char *full_via = argv[9];
	char *expires = argv[10];
	char *profile_name = argv[13];
	char *body = argv[14];
	char *exp;
	//sofia_profile_t *profile = NULL;
	char *id = NULL;
	nua_handle_t *nh;
	int expire_sec = atoi(expires);
	//int *total = (int *) pArg;
	struct mwi_helper *h = (struct mwi_helper *) pArg;
	sofia_profile_t *ext_profile = NULL, *profile = h->profile;
	

	if (profile_name && strcasecmp(profile_name, h->profile->name)) {
		if ((ext_profile = sofia_glue_find_profile(profile_name))) {
			profile = ext_profile;
		}
	}

	
	if (!(nh = nua_handle_by_call_id(h->profile->nua, call_id))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Cannot find handle for %s\n", call_id);
		goto end;
	}

	id = switch_mprintf("sip:%s@%s", sub_to_user, sub_to_host);
	expire_sec = (int) (expire_sec - switch_timestamp(NULL));
	if (expire_sec < 0) {
		expire_sec = 3600;
	}
	exp = switch_mprintf("active;expires=%ld", expire_sec);
	nua_handle_bind(nh, &mod_sofia_globals.keep_private);
	nua_notify(nh,
			   SIPTAG_SUBSCRIPTION_STATE_STR(exp),
			   SIPTAG_EVENT_STR(event), SIPTAG_CONTENT_TYPE_STR("application/simple-message-summary"), SIPTAG_PAYLOAD_STR(body), TAG_END());

	switch_safe_free(id);
	switch_safe_free(exp);

	h->total++;

 end:

	if (ext_profile) {
		sofia_glue_release_profile(ext_profile);
	}

	return 0;
}




static int sofia_presence_mwi_callback2(void *pArg, int argc, char **argv, char **columnNames)
{
	char *sub_to_user = argv[0];
	char *sub_to_host = argv[1];
	char *event = "message-summary";
	char *contact, *o_contact = argv[2];
	char *profile_name = argv[3];
	char *body = argv[4];
	char *id = NULL;
	nua_handle_t *nh;
	struct mwi_helper *h = (struct mwi_helper *) pArg;
	sofia_profile_t *ext_profile = NULL, *profile = h->profile;
	

	if (profile_name && strcasecmp(profile_name, h->profile->name)) {
		if ((ext_profile = sofia_glue_find_profile(profile_name))) {
			profile = ext_profile;
		}
	}

	id = switch_mprintf("sip:%s@%s", sub_to_user, sub_to_host);

	contact = sofia_glue_get_url_from_contact(o_contact, 1);

	nh = nua_handle(profile->nua, NULL, NUTAG_URL(contact), SIPTAG_FROM_STR(id), SIPTAG_TO_STR(id), SIPTAG_CONTACT_STR(h->profile->url), TAG_END());

	nua_notify(nh,
			   NUTAG_NEWSUB(1),
			   TAG_IF(strstr(o_contact, ";fs_nat"), NUTAG_PROXY(contact)),
			   SIPTAG_EVENT_STR(event), SIPTAG_CONTENT_TYPE_STR("application/simple-message-summary"), SIPTAG_PAYLOAD_STR(body), TAG_END());

	switch_safe_free(contact);
	switch_safe_free(id);

	if (ext_profile) {
		sofia_glue_release_profile(ext_profile);
	}

	return 0;
}

void sofia_presence_handle_sip_i_subscribe(int status,
										   char const *phrase,
										   nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip,
										   tagi_t tags[])
{
	if (sip) {
		long exp, exp_raw;
		sip_to_t const *to = sip->sip_to;
		sip_from_t const *from = sip->sip_from;
		sip_contact_t const *contact = sip->sip_contact;
		const char *from_user = NULL, *from_host = NULL;
		const char *to_user = NULL, *to_host = NULL;
		char *my_to_user = NULL;
		char *sql, *event = NULL;
		char *proto = "sip";
		char *d_user = NULL;
		char *contact_str = "";
		const char *call_id = NULL;
		char *to_str = NULL;
		char *full_from = NULL;
		char *full_via = NULL;
		char *full_agent = NULL;
		char *sstr;
		const char *display = "\"user\"";
		switch_event_t *sevent;
		int sub_state;
		int sent_reply = 0;
		su_addrinfo_t *my_addrinfo = msg_addrinfo(nua_current_request(nua));
		int network_port = 0;
		char network_ip[80];
		const char *contact_host, *contact_user;
		char *port;
		char new_port[25] = "";
		char *is_nat = NULL;
		const char *ipv6;

		if (!(contact && sip->sip_contact->m_url)) {
			nua_respond(nh, 481, "INVALID SUBSCRIPTION", TAG_END());
			return;
		}

		get_addr(network_ip, sizeof(network_ip), my_addrinfo->ai_addr, my_addrinfo->ai_addrlen);
		network_port = ntohs(((struct sockaddr_in *) msg_addrinfo(nua_current_request(nua))->ai_addr)->sin_port);

		tl_gets(tags, NUTAG_SUBSTATE_REF(sub_state), TAG_END());

		event = sip_header_as_string(profile->home, (void *) sip->sip_event);

		port = (char *) contact->m_url->url_port;
		contact_host = sip->sip_contact->m_url->url_host;
		contact_user = sip->sip_contact->m_url->url_user;

		display = contact->m_display;

		if (switch_strlen_zero(display)) {
			if (from) {
				display = from->a_display;
				if (switch_strlen_zero(display)) {
					display = "\"user\"";
				}
			}
		} else {
			display = "\"user\"";
		}

		if ((profile->pflags & PFLAG_AGGRESSIVE_NAT_DETECTION)) {
			if (sip && sip->sip_via) {
				const char *v_port = sip->sip_via->v_port;
				const char *v_host = sip->sip_via->v_host;

				if (v_host && sip->sip_via->v_received) {
					is_nat = "via received";
				} else if (v_host && strcmp(network_ip, v_host)) {
					is_nat = "via host";
				} else if (v_port && atoi(v_port) != network_port) {
					is_nat = "via port";
				}
			}
		}

		if (!is_nat && profile->nat_acl_count) {
			uint32_t x = 0;
			int ok = 1;
			char *last_acl = NULL;

			if (!switch_strlen_zero(contact_host)) {
				for (x = 0; x < profile->nat_acl_count; x++) {
					last_acl = profile->nat_acl[x];
					if (!(ok = switch_check_network_list_ip(contact_host, last_acl))) {
						break;
					}
				}

				if (ok) {
					is_nat = last_acl;
				}
			}
		}


		if (is_nat) {
			contact_host = network_ip;
			switch_snprintf(new_port, sizeof(new_port), ":%d", network_port);
			port = NULL;
		}


		if (port) {
			switch_snprintf(new_port, sizeof(new_port), ":%s", port);
		}

		ipv6 = strchr(contact_host, ':');
		if (contact->m_url->url_params) {
			contact_str = switch_mprintf("%s <sip:%s@%s%s%s%s;%s>%s",
										display, contact->m_url->url_user,
										ipv6 ? "[" : "",
										contact_host,
										ipv6 ? "]" : "",
										new_port,
										contact->m_url->url_params,
										is_nat ? ";nat" : "");
		} else {
			contact_str = switch_mprintf("%s <sip:%s@%s%s%s%s>%s",
										display,
										contact->m_url->url_user,
										ipv6 ? "[" : "",
										contact_host,
										ipv6 ? "]" : "",
										new_port,
										is_nat ?  ";nat" : "");
		}


		if (to) {
			to_str = switch_mprintf("sip:%s@%s", to->a_url->url_user, to->a_url->url_host);	//, to->a_url->url_port);
		}

		if (to) {
			to_user = to->a_url->url_user;
			to_host = to->a_url->url_host;
		}

		if (sip && sip->sip_from) {
			from_user = sip->sip_from->a_url->url_user;
			from_host = sip->sip_from->a_url->url_host;
		} else {
			from_user = "n/a";
			from_host = "n/a";
		}

		if (to_user && (strstr(to_user, "ext+") || strstr(to_user, "user+"))) {
			char protocol[80];
			char *p;

			switch_copy_string(protocol, to_user, sizeof(protocol));
			if ((p = strchr(protocol, '+'))) {
				*p = '\0';
			}

			if (switch_event_create(&sevent, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header_string(sevent, SWITCH_STACK_BOTTOM, "proto", protocol);
				switch_event_add_header_string(sevent, SWITCH_STACK_BOTTOM, "login", profile->name);
				switch_event_add_header(sevent, SWITCH_STACK_BOTTOM, "from", "%s@%s", to_user, to_host);
				switch_event_add_header_string(sevent, SWITCH_STACK_BOTTOM, "rpid", "active");
				switch_event_add_header_string(sevent, SWITCH_STACK_BOTTOM, "status", "Click To Call");
				switch_event_fire(&sevent);
			}

		} else {
			if (switch_event_create(&sevent, SWITCH_EVENT_PRESENCE_PROBE) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header_string(sevent, SWITCH_STACK_BOTTOM, "proto", SOFIA_CHAT_PROTO);
				switch_event_add_header_string(sevent, SWITCH_STACK_BOTTOM, "login", profile->name);
				switch_event_add_header(sevent, SWITCH_STACK_BOTTOM, "from", "%s@%s", from_user, from_host);
				switch_event_add_header(sevent, SWITCH_STACK_BOTTOM, "to", "%s@%s", to_user, to_host);
				switch_event_add_header_string(sevent, SWITCH_STACK_BOTTOM, "proto-specific-event-name", event);
				switch_event_fire(&sevent);
			}
		}

		if (to_user && strchr(to_user, '+')) {
			char *h;
			if ((proto = (d_user = strdup(to_user)))) {
				if ((my_to_user = strchr(d_user, '+'))) {
					*my_to_user++ = '\0';
					to_user = my_to_user;
					if ((h = strchr(to_user, '+')) || (h = strchr(to_user, '@'))) {
						*h++ = '\0';
						to_host = h;
					}
				}
			}

			if (!(proto && to_user && to_host)) {
				nua_respond(nh, SIP_404_NOT_FOUND, NUTAG_WITH_THIS(nua), TAG_END());
				goto end;
			}
		}

		call_id = sip->sip_call_id->i_id;
		full_from = sip_header_as_string(profile->home, (void *) sip->sip_from);
		full_via = sip_header_as_string(profile->home, (void *) sip->sip_via);

		exp_raw = (sip->sip_expires ? sip->sip_expires->ex_delta : 3600);
		exp = (long) switch_timestamp(NULL) + exp_raw;

		if (sofia_test_pflag(profile, PFLAG_MULTIREG)) {
			sql = switch_mprintf("delete from sip_subscriptions where call_id='%q'", call_id);
		} else {
			sql = switch_mprintf("delete from sip_subscriptions where "
								 "proto='%q' and sip_user='%q' and sip_host='%q' and sub_to_user='%q' and sub_to_host='%q' and event='%q' and hostname='%q'",
								 proto, from_user, from_host, to_user, to_host, event, mod_sofia_globals.hostname);
		}


		switch_mutex_lock(profile->ireg_mutex);
		switch_assert(sql != NULL);
		sofia_glue_execute_sql(profile, &sql, SWITCH_TRUE);

		if (sub_state == nua_substate_terminated) {
			sstr = switch_mprintf("terminated");
		} else {
			sip_accept_t *ap = sip->sip_accept;
			char accept[256] = "";
			full_agent = sip_header_as_string(profile->home, (void *) sip->sip_user_agent);
			while (ap) {
				switch_snprintf(accept + strlen(accept), sizeof(accept) - strlen(accept), "%s%s ", ap->ac_type, ap->ac_next ? "," : "");
				ap = ap->ac_next;
			}
			sql = switch_mprintf("insert into sip_subscriptions "
								 "(proto,sip_user,sip_host,sub_to_user,sub_to_host,presence_hosts,event,contact,call_id,full_from,"
								 "full_via,expires,user_agent,accept,profile_name,hostname) "
								 "values ('%q','%q','%q','%q','%q','%q','%q','%q','%q','%q','%q',%ld,'%q','%q','%q','%q')",
								 proto, from_user, from_host, to_user, to_host, profile->presence_hosts ? profile->presence_hosts : to_host, 
								 event, contact_str, call_id, full_from, full_via, exp, full_agent, accept, profile->name,mod_sofia_globals.hostname);
								 
			
			switch_assert(sql != NULL);
			sofia_glue_execute_sql(profile, &sql, SWITCH_TRUE);

			sstr = switch_mprintf("active;expires=%ld", exp_raw);
		}

		switch_mutex_unlock(profile->ireg_mutex);


		if (status < 200) {
			char *sticky = NULL;

			if (is_nat) {
				ipv6 = strchr(network_ip, ':');
				sticky = switch_mprintf("sip:%s@%s%s%s:%d",
										contact_user,
										ipv6 ? "[" : "",
										network_ip,
										ipv6 ? "]" : "",
										network_port);
			}

			nua_respond(nh, SIP_202_ACCEPTED, NUTAG_WITH_THIS(nua), SIPTAG_SUBSCRIPTION_STATE_STR(sstr), TAG_IF(sticky, NUTAG_PROXY(sticky)),
						//SIPTAG_FROM(sip->sip_to),
						//SIPTAG_TO(sip->sip_from),
						//SIPTAG_CONTACT_STR(contact_str),
						TAG_END());

			switch_safe_free(sticky);

		}

		sent_reply++;

#if 0

		nua_notify(nh,
				   NUTAG_NEWSUB(1),
				   SIPTAG_SUBSCRIPTION_STATE_STR(sstr), SIPTAG_EVENT_STR(event),
				   SIPTAG_CONTENT_TYPE_STR("application/simple-message-summary"),
				   SIPTAG_PAYLOAD_STR("Note: Come to ClueCon http://www.cluecon.com\n\n"), TAG_END());
#endif

		switch_safe_free(sstr);

		if ((sql = switch_mprintf(
								  "select proto,sip_user,'%q',sub_to_user,sub_to_host,event,contact,call_id,full_from,"
								  "full_via,expires,user_agent,accept,profile_name"
								  " from sip_subscriptions where sip_user='%q' and (sip_host='%q' or presence_hosts like '%%%q%%')", 
								  to_host, to_user, to_host, to_host))) {
			sofia_glue_execute_sql_callback(profile, SWITCH_FALSE, profile->ireg_mutex, sql, sofia_presence_sub_reg_callback, profile);
			
			switch_safe_free(sql);
		}
	  end:

		if (event) {
			su_free(profile->home, event);
		}

		if (full_from) {
			su_free(profile->home, full_from);
		}
		if (full_via) {
			su_free(profile->home, full_via);
		}
		if (full_agent) {
			su_free(profile->home, full_agent);
		}

		switch_safe_free(d_user);
		switch_safe_free(to_str);
		switch_safe_free(contact_str);

		if (!sent_reply) {
			nua_respond(nh, 481, "INVALID SUBSCRIPTION", TAG_END());
		}
	}

}

void sofia_presence_handle_sip_r_subscribe(int status,
										   char const *phrase,
										   nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip,
										   tagi_t tags[])
{

}

void sofia_presence_handle_sip_i_publish(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip,
										 tagi_t tags[])
{
	if (sip) {
		sip_from_t const *from = sip->sip_from;
		char *from_user = NULL;
		char *from_host = NULL;
		char *rpid = "unknown";
		sip_payload_t *payload = sip->sip_payload;
		char *event_type;

		if (from) {
			from_user = (char *) from->a_url->url_user;
			from_host = (char *) from->a_url->url_host;
		}

		if (payload) {
			switch_xml_t xml, note, person, tuple, status, basic, act;
			switch_event_t *event;
			uint8_t in = 0;
			char *sql;

			if ((xml = switch_xml_parse_str(payload->pl_data, strlen(payload->pl_data)))) {
				char *status_txt = "", *note_txt = "";

				if ((tuple = switch_xml_child(xml, "tuple")) && (status = switch_xml_child(tuple, "status"))
					&& (basic = switch_xml_child(status, "basic"))) {
					status_txt = basic->txt;
				}

				if ((person = switch_xml_child(xml, "dm:person")) && (note = switch_xml_child(person, "dm:note"))) {
					note_txt = note->txt;
				}

				if (person && (act = switch_xml_child(person, "rpid:activities"))) {
					if ((rpid = strchr(act->child->name, ':'))) {
						rpid++;
					} else {
						rpid = act->child->name;
					}
				}

				if (!strcasecmp(status_txt, "open")) {
					if (switch_strlen_zero(note_txt)) {
						note_txt = "Available";
					}
					in = 1;
				} else if (!strcasecmp(status_txt, "closed")) {
					if (switch_strlen_zero(note_txt)) {
						note_txt = "Unavailable";
					}
				}

				if ((sql =
					 switch_mprintf("update sip_registrations set status='%q',rpid='%q' where sip_user='%q' and sip_host='%q'",
									note_txt, rpid, from_user, from_host))) {
					sofia_glue_execute_sql(profile, &sql, SWITCH_TRUE);
				}

				event_type = sip_header_as_string(profile->home, (void *) sip->sip_event);

				if (in) {
					if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_IN) == SWITCH_STATUS_SUCCESS) {
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", SOFIA_CHAT_PROTO);
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "rpid", rpid);
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "login", profile->url);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s@%s", from_user, from_host);

						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "status", note_txt);
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "event_type", event_type);
						switch_event_fire(&event);
					}
				} else {
					if (switch_event_create(&event, SWITCH_EVENT_PRESENCE_OUT) == SWITCH_STATUS_SUCCESS) {
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", SOFIA_CHAT_PROTO);
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "rpid", rpid);
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "login", profile->url);
						switch_event_add_header(event, SWITCH_STACK_BOTTOM, "from", "%s@%s", from_user, from_host);

						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "event_type", event_type);
						switch_event_fire(&event);
					}
				}

				if (event_type) {
					su_free(profile->home, event_type);
				}
				switch_xml_free(xml);
			}
		}
	}
	nua_respond(nh, SIP_200_OK, NUTAG_WITH_THIS(nua), TAG_END());
}

void sofia_presence_set_hash_key(char *hash_key, int32_t len, sip_t const *sip)
{
	url_t *to = sip->sip_to->a_url;
	url_t *from = sip->sip_from->a_url;
	switch_snprintf(hash_key, len, "%s%s%s", from->url_user, from->url_host, to->url_user);
}

void sofia_presence_handle_sip_i_message(int status,
										 char const *phrase,
										 nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip,
										 tagi_t tags[])
{
	if (sip) {
		sip_from_t const *from = sip->sip_from;
		const char *from_user = NULL;
		const char *from_host = NULL;
		sip_to_t const *to = sip->sip_to;
		const char *to_user = NULL;
		const char *to_host = NULL;
		sip_subject_t const *sip_subject = sip->sip_subject;
		sip_payload_t *payload = sip->sip_payload;
		const char *subject = "n/a";
		char *msg = NULL;

		if (sip->sip_content_type && sip->sip_content_type->c_subtype) {
			if (strstr(sip->sip_content_type->c_subtype, "composing")) {
				return;
			}
		}

		if (from) {
			from_user = from->a_url->url_user;
			from_host = from->a_url->url_host;
		}

		if (to) {
			to_user = to->a_url->url_user;
			to_host = to->a_url->url_host;
		}

		if (!to_user) {
			return;
		}

		if (payload) {
			msg = payload->pl_data;
		}

		if (sip_subject) {
			subject = sip_subject->g_value;
		}

		if (nh) {
			char hash_key[512];
			private_object_t *tech_pvt;
			switch_channel_t *channel;
			switch_event_t *event;
			char *to_addr;
			char *from_addr;
			char *p;
			char *full_from;
			char proto[512] = SOFIA_CHAT_PROTO;

			full_from = sip_header_as_string(profile->home, (void *) sip->sip_from);

			if ((p = strchr(to_user, '+'))) {
				switch_copy_string(proto, to_user, sizeof(proto));
				p = strchr(proto, '+');
				*p++ = '\0';
				
				if ((to_addr = strdup(p))) {
					if ((p = strchr(to_addr, '+'))) {
						*p = '@';
					}
				}
			} else {
				to_addr = switch_mprintf("%s@%s", to_user, to_host);
			}

			from_addr = switch_mprintf("%s@%s", from_user, from_host);


			sofia_presence_set_hash_key(hash_key, sizeof(hash_key), sip);
			if ((tech_pvt = (private_object_t *) switch_core_hash_find(profile->chat_hash, hash_key))) {
				channel = switch_core_session_get_channel(tech_pvt->session);
				if (switch_event_create(&event, SWITCH_EVENT_MESSAGE) == SWITCH_STATUS_SUCCESS) {
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", SOFIA_CHAT_PROTO);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "login", profile->url);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "from", from_addr);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "hint", full_from);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "to", to_addr);
					switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "subject", "SIMPLE MESSAGE");
					if (msg) {
						switch_event_add_body(event, "%s", msg);
					}
					
					if (switch_core_session_queue_event(tech_pvt->session, &event) != SWITCH_STATUS_SUCCESS) {
						switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "delivery-failure", "true");
						switch_event_fire(&event);
					}
				}
			} else {
				switch_chat_interface_t *ci;
				if ((ci = switch_loadable_module_get_chat_interface(proto))) {
					ci->chat_send(SOFIA_CHAT_PROTO, from_addr, to_addr, "", msg, full_from);
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Chat Interface [%s]!\n", proto);
				}
			}
			switch_safe_free(to_addr);
			switch_safe_free(from_addr);
			if (full_from) {
				su_free(profile->home, full_from);
			}
		}
	}
}

void sofia_presence_set_chat_hash(private_object_t *tech_pvt, sip_t const *sip)
{
	char hash_key[256] = "";
	char buf[512];
	su_home_t *home = NULL;

	if (!tech_pvt || tech_pvt->hash_key || !sip || !sip->sip_from || !sip->sip_from->a_url || 
		!sip->sip_from->a_url->url_user || !sip->sip_from->a_url->url_host) {
		return;
	}

	if (sofia_reg_find_reg_url(tech_pvt->profile, sip->sip_from->a_url->url_user, sip->sip_from->a_url->url_host, buf, sizeof(buf))) {
		home = su_home_new(sizeof(*home));
		switch_assert(home != NULL);
		tech_pvt->chat_from = sip_header_as_string(home, (const sip_header_t *) sip->sip_to);
		tech_pvt->chat_to = switch_core_session_strdup(tech_pvt->session, buf);
		sofia_presence_set_hash_key(hash_key, sizeof(hash_key), sip);
		su_home_unref(home);
		home = NULL;
	} else {
		return;
	}
	
	switch_mutex_lock(tech_pvt->profile->flag_mutex);
	tech_pvt->hash_key = switch_core_session_strdup(tech_pvt->session, hash_key);
	switch_core_hash_insert(tech_pvt->profile->chat_hash, tech_pvt->hash_key, tech_pvt);
	switch_mutex_unlock(tech_pvt->profile->flag_mutex);
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
