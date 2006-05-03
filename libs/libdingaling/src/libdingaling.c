/* 
 * libDingaLing XMPP Jingle Library
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
 * The Original Code is libDingaLing XMPP Jingle Library
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
 * libdingaling.c -- Main Library Code
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <iksemel.h>
#include <apr.h>
#include <apr_network_io.h>
#include <apr_errno.h>
#include <apr_general.h>
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_thread_rwlock.h>
#include <apr_file_io.h>
#include <apr_poll.h>
#include <apr_dso.h>
#include <apr_hash.h>
#include <apr_strings.h>
#include <apr_network_io.h>
#include <apr_poll.h>
#include <apr_queue.h>
#include <apr_uuid.h>
#include <apr_strmatch.h>
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_env.h>

#include "ldl_compat.h"
#include "libdingaling.h"

#define microsleep(x) apr_sleep(x * 1000)

static int opt_timeout = 30;
static int opt_use_tls = 0;

static struct {
	unsigned int flags;
	FILE *log_stream;
	int debug;
	apr_pool_t *memory_pool;
	unsigned int id;
	ldl_logger_t logger;
} globals;

struct packet_node {
	char id[80];
	iks *xml;
	unsigned int retries;
	apr_time_t next;
};

struct ldl_buffer {
	char *buf;
	unsigned int len;
	int hit;
};

struct ldl_handle {
	iksparser *parser;
	iksid *acc;
	iksfilter *filter;
	char *login;
	char *password;
	char *status_msg;
	int features;
	int counter;
	int job_done;
	unsigned int flags;
	apr_queue_t *queue;
	apr_queue_t *retry_queue;
	apr_hash_t *sessions;
	apr_hash_t *retry_hash;
	apr_hash_t *probe_hash;
	apr_thread_mutex_t *lock;
	ldl_loop_callback_t loop_callback;
	ldl_session_callback_t session_callback;
	ldl_response_callback_t response_callback;
	apr_pool_t *pool;
	void *private_info;
	FILE *log_stream;
};

struct ldl_session {
	ldl_state_t state;
	ldl_handle_t *handle;
	char *id;
	char *initiator;
	char *them;
	char *ip;
	ldl_payload_t payloads[LDL_MAX_PAYLOADS];
	unsigned int payload_len;
	ldl_candidate_t candidates[LDL_MAX_CANDIDATES];
	unsigned int candidate_len;
	apr_pool_t *pool;
	apr_time_t created;
	void *private_data;
};


static char *cut_path(char *in)
{
	char *p, *ret = in;
	char delims[] = "/\\";
	char *i;

	for (i = delims; *i; i++) {
		p = in;
		while ((p = strchr(p, *i)) != 0) {
			ret = ++p;
		}
	}
	return ret;
}

static void default_logger(char *file, const char *func, int line, int level, char *fmt, ...)
{
	char *fp;
	char data[1024];

	va_list ap;
	
	fp = cut_path(file);

	va_start(ap, fmt);

	vsnprintf(data, sizeof(data), fmt, ap);

	fprintf(globals.log_stream, "%s:%d %s() %s", file, line, func, data);

	va_end(ap);

}

static unsigned int next_id(void)
{
	return globals.id++;
}


char *ldl_session_get_id(ldl_session_t *session)
{
	return session->id;
}

ldl_status ldl_session_destroy(ldl_session_t **session_p)
{
	ldl_session_t *session = *session_p;

	if (session) {
		apr_pool_t *pool = session->pool;

		if (globals.debug) {
			globals.logger(DL_LOG_DEBUG, "Destroyed Session %s\n", session->id);
		}
		if (session->id) {
			apr_hash_set(session->handle->sessions, session->id, APR_HASH_KEY_STRING, NULL);
		}
		if (session->them) {
			apr_hash_set(session->handle->sessions, session->them, APR_HASH_KEY_STRING, NULL);
		}
		apr_pool_destroy(pool);
		pool = NULL;
		*session_p = NULL;
		return LDL_STATUS_SUCCESS;
	}

	return LDL_STATUS_FALSE;
}


ldl_status ldl_session_create(ldl_session_t **session_p, ldl_handle_t *handle, char *id, char *them, char *me)
{
	ldl_session_t *session = NULL;
	
	if (!(session = apr_palloc(handle->pool, sizeof(ldl_session_t)))) {
		globals.logger(DL_LOG_DEBUG, "Memory ERROR!\n");
		*session_p = NULL;
		return LDL_STATUS_MEMERR;
	}
	
	memset(session, 0, sizeof(ldl_session_t));
	apr_pool_create(&session->pool, globals.memory_pool);
	session->id = apr_pstrdup(session->pool, id);
	session->them = apr_pstrdup(session->pool, them);
	if (me) {
		session->initiator = apr_pstrdup(session->pool, me);
	}
	apr_hash_set(handle->sessions, session->id, APR_HASH_KEY_STRING, session);
	apr_hash_set(handle->sessions, session->them, APR_HASH_KEY_STRING, session);
	session->handle = handle;
	session->created = apr_time_now();
	session->state = LDL_STATE_NEW;
	*session_p = session;
	
	if (globals.debug) {
		globals.logger(DL_LOG_DEBUG, "Created Session %s\n", id);
	}

	return LDL_STATUS_SUCCESS;
}

static ldl_status parse_session_code(ldl_handle_t *handle, char *id, char *from, iks *xml, char *xtype)
{
	ldl_session_t *session = NULL;
	ldl_signal_t signal = LDL_SIGNAL_NONE;
	char *initiator = iks_find_attrib(xml, "initiator");
	char *msg = NULL;

	if (!(session = apr_hash_get(handle->sessions, id, APR_HASH_KEY_STRING))) {
		ldl_session_create(&session, handle, id, from, initiator);
		if (!session) {
			return LDL_STATUS_MEMERR;
		}
	}

	if (!session) {
		if (globals.debug) {
			globals.logger(DL_LOG_DEBUG, "Non-Existent Session %s!\n", id);
		}
		return LDL_STATUS_MEMERR;
	}
	
	if (globals.debug) {
		globals.logger(DL_LOG_DEBUG, "Message for Session %s\n", id);
	}

	while(xml) {
		char *type = xtype ? xtype : iks_find_attrib(xml, "type");
		iks *tag;

		if (type) {
			
			if (!strcasecmp(type, "initiate") || !strcasecmp(type, "accept")) {

				signal = LDL_SIGNAL_INITIATE;
				if (!strcasecmp(type, "accept")) {
					msg = "accept";
				}
				if (!session->initiator && initiator) {
					session->initiator = apr_pstrdup(session->pool, initiator);
				}
				tag = iks_child (xml);
				
				while(tag) {
					if (!strcasecmp(iks_name(tag), "description")) {
						iks * itag = iks_child (tag);
						while(itag) {
							if (!strcasecmp(iks_name(itag), "payload-type") && session->payload_len < LDL_MAX_PAYLOADS) {
								char *name = iks_find_attrib(itag, "name");
								char *id = iks_find_attrib(itag, "id");
								char *rate = iks_find_attrib(itag, "rate");
								if (name && id) {
									session->payloads[session->payload_len].name = apr_pstrdup(session->pool, name);
									session->payloads[session->payload_len].id = atoi(id);
									if (rate) {
										session->payloads[session->payload_len].rate = atoi(rate);
									}
									session->payload_len++;
								
									if (globals.debug) {
										globals.logger(DL_LOG_DEBUG, "Add Payload [%s] id='%s'\n", name, id);
									}
								}
							}
							itag = iks_next_tag(itag);
						}
					}
					tag = iks_next_tag(tag);
				}
			} else if (!strcasecmp(type, "candidates")) {
				signal = LDL_SIGNAL_CANDIDATES;
				tag = iks_child (xml);
				
				while(tag) {
					if (!strcasecmp(iks_name(tag), "candidate") && session->candidate_len < LDL_MAX_CANDIDATES) {
						char *key;
						double pref = 0.0;
						int index = -1;
						if ((key = iks_find_attrib(tag, "preference"))) {
							unsigned int x;
							pref = strtod(key, NULL);
							
							for (x = 0; x < session->candidate_len; x++) {
								if (session->candidates[x].pref == pref) {
									if (globals.debug) {
										globals.logger(DL_LOG_DEBUG, "Duplicate Pref!\n");
									}
									index = x;
									break;
								}
							}
						}
						
						if (index < 0) {
							index = session->candidate_len++;
						}
						
						session->candidates[index].pref = pref;
						if ((key = iks_find_attrib(tag, "name"))) {
							session->candidates[index].name = apr_pstrdup(session->pool, key);
						}
						if ((key = iks_find_attrib(tag, "type"))) {
							session->candidates[index].type = apr_pstrdup(session->pool, key);
						}
						if ((key = iks_find_attrib(tag, "protocol"))) {
							session->candidates[index].protocol = apr_pstrdup(session->pool, key);
						}
						if ((key = iks_find_attrib(tag, "username"))) {
							session->candidates[index].username = apr_pstrdup(session->pool, key);
						}
						if ((key = iks_find_attrib(tag, "password"))) {
							session->candidates[index].password = apr_pstrdup(session->pool, key);
						}
						if ((key = iks_find_attrib(tag, "address"))) {
							session->candidates[index].address = apr_pstrdup(session->pool, key);
						}
						if ((key = iks_find_attrib(tag, "port"))) {
							session->candidates[index].port = atoi(key);
						}
						if (globals.debug) {
							globals.logger(DL_LOG_DEBUG, 
									"New Candidate %d\n"
									"name=%s\n"
									"type=%s\n"
									"protocol=%s\n"
									"username=%s\n"
									"password=%s\n"
									"address=%s\n"
									"port=%d\n"
									"pref=%0.2f\n",
									session->candidate_len,
									session->candidates[index].name,
									session->candidates[index].type,
									session->candidates[index].protocol,
									session->candidates[index].username,
									session->candidates[index].password,
									session->candidates[index].address,
									session->candidates[index].port,
									session->candidates[index].pref
									);
						}
					}
					tag = iks_next_tag(tag);
				}
			} else if (!strcasecmp(type, "terminate")) {
				signal = LDL_SIGNAL_TERMINATE;
			} else if (!strcasecmp(type, "error")) {
				signal = LDL_SIGNAL_ERROR;
			}
		}
		
		xml = iks_child(xml);
	}

	if (handle->session_callback && signal) {
		handle->session_callback(handle, session, signal, from, id, msg); 
	}

	return LDL_STATUS_SUCCESS;
}


static int on_presence(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	char *from = iks_find_attrib(pak->x, "from");
	char id[1024];
	char *resource;
	struct ldl_buffer *buffer;
	size_t x;

	apr_cpystrn(id, from, sizeof(id));
	if ((resource = strchr(id, '/'))) {
		*resource++ = '\0';
	}

	if (resource) {
		for (x = 0; x < strlen(resource); x++) {
			resource[x] = tolower(resource[x]);
		}
	}

	if (resource && strstr(resource, "talk") && (buffer = apr_hash_get(handle->probe_hash, id, APR_HASH_KEY_STRING))) {
		apr_cpystrn(buffer->buf, from, buffer->len);
		fflush(stderr);
		buffer->hit = 1;
	}

	return IKS_FILTER_EAT;
}



static int on_subscribe(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	char *from = iks_find_attrib(pak->x, "from");
	iks *msg = iks_make_s10n (IKS_TYPE_SUBSCRIBED, from, "Ding A Ling...."); 
	apr_queue_push(handle->queue, msg);
	msg = iks_make_s10n (IKS_TYPE_SUBSCRIBE, from, "Ding A Ling...."); 
	apr_queue_push(handle->queue, msg);
	return IKS_FILTER_EAT;
}

static void cancel_retry(ldl_handle_t *handle, char *id)
{
	struct packet_node *packet_node;

	apr_thread_mutex_lock(handle->lock);
	if ((packet_node = apr_hash_get(handle->retry_hash, id, APR_HASH_KEY_STRING))) {
		if (globals.debug) {
			globals.logger(DL_LOG_DEBUG, "Cancel packet %s\n", packet_node->id);
		}
		packet_node->retries = 0;
	}
	apr_thread_mutex_unlock(handle->lock);
}

static int on_commands(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	char *from = iks_find_attrib(pak->x, "from");
	//char *to = iks_find_attrib(pak->x, "to");
	char *iqid = iks_find_attrib(pak->x, "id");
	char *type = iks_find_attrib(pak->x, "type");
	iks *xml;

	//printf("XXXXX from=%s to=%s type=%s\n", from, to, type);

	if ((!strcasecmp(type, "result") || !strcasecmp(type, "error")) && iqid && from) {
		cancel_retry(handle, iqid);
		if (!strcasecmp(type, "result")) {
			if (handle->response_callback) {
				handle->response_callback(handle, iqid); 
			}
			return IKS_FILTER_EAT;
		}
	}



	
	xml = iks_child (pak->x);
	while (xml) {
		char *name = iks_name(xml);
		if (!strcasecmp(name, "session")) {
			char *id = iks_find_attrib(xml, "id");
			//printf("SESSION type=%s name=%s id=%s\n", type, name, id);
			if (parse_session_code(handle, id, from, xml, strcasecmp(type, "error") ? NULL : type) == LDL_STATUS_SUCCESS) {
				iks *reply;
				reply = iks_make_iq(IKS_TYPE_RESULT, NULL); 
				iks_insert_attrib(reply, "to", from);
				iks_insert_attrib(reply, "id", iqid);
				apr_queue_push(handle->queue, reply);
			}
		}
		xml = iks_child (xml);
	}


	return IKS_FILTER_EAT;
}


static int on_result(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	iks *msg, *ctag;

	msg = iks_make_pres (IKS_SHOW_AVAILABLE, handle->status_msg); 
	ctag = iks_insert(msg, "c");
	iks_insert_attrib(ctag, "node", "http://www.freeswitch.org/xmpp/client/caps");
	iks_insert_attrib(ctag, "ver", "1.0.0.1");
	iks_insert_attrib(ctag, "ext", "sidebar voice-v1");
	iks_insert_attrib(ctag, "client", "libdingaling");
	iks_insert_attrib(ctag, "xmlns", "http://jabber.org/protocol/caps");
	
	apr_queue_push(handle->queue, msg);
	ldl_set_flag(handle, LDL_FLAG_READY);
	return IKS_FILTER_EAT;
}

static int on_stream(ldl_handle_t *handle, int type, iks * node)
{
	handle->counter = opt_timeout;

	switch (type) {
	case IKS_NODE_START:
		if (opt_use_tls && !iks_is_secure(handle->parser)) {
			iks_start_tls(handle->parser);
		}
		break;
	case IKS_NODE_NORMAL:
		if (strcmp("stream:features", iks_name(node)) == 0) {
			handle->features = iks_stream_features(node);
			if (opt_use_tls && !iks_is_secure(handle->parser))
				break;
			if (ldl_test_flag(handle, LDL_FLAG_AUTHORIZED)) {
				iks *t;
				if (handle->features & IKS_STREAM_BIND) {
					t = iks_make_resource_bind(handle->acc);
					iks_send(handle->parser, t);
					iks_delete(t);
				}
				if (handle->features & IKS_STREAM_SESSION) {
					t = iks_make_session();
					iks_insert_attrib(t, "id", "auth");
					iks_send(handle->parser, t);
					iks_delete(t);
				}
			} else {
				if (handle->features & IKS_STREAM_SASL_MD5) {
					iks_start_sasl(handle->parser, IKS_SASL_DIGEST_MD5, handle->acc->user, handle->password);
				} else if (handle->features & IKS_STREAM_SASL_PLAIN) {
					iks_start_sasl(handle->parser, IKS_SASL_PLAIN, handle->acc->user, handle->password);
				}
			}
		} else if (strcmp("failure", iks_name(node)) == 0) {
			globals.logger(DL_LOG_DEBUG, "sasl authentication failed\n");
			if (handle->session_callback) {
				handle->session_callback(handle, NULL, LDL_SIGNAL_LOGIN_FAILURE, "core", "Login Failure", handle->login);
			}
		} else if (strcmp("success", iks_name(node)) == 0) {
			globals.logger(DL_LOG_DEBUG, "XMPP server connected\n");
			if (handle->session_callback) {
				handle->session_callback(handle, NULL, LDL_SIGNAL_LOGIN_SUCCESS, "core", "Login Success", handle->login);
			}
			iks_send_header(handle->parser, handle->acc->server);
			ldl_set_flag(handle, LDL_FLAG_AUTHORIZED);
		} else {
			ikspak *pak;

			pak = iks_packet(node);
			iks_filter_packet(handle->filter, pak);
			if (handle->job_done == 1)
				return IKS_HOOK;
		}
		break;
#if 0
	case IKS_NODE_STOP:
		globals.logger(DL_LOG_DEBUG, "server disconnected\n");
		break;

	case IKS_NODE_ERROR:
		globals.logger(DL_LOG_DEBUG, "stream error\n");
		break;
#endif

	}

	if (node)
		iks_delete(node);
	return IKS_OK;
}

static int on_msg(void *user_data, ikspak *pak)
{
	char *cmd = iks_find_cdata(pak->x, "body");
	char *from = iks_find_attrib(pak->x, "from");
	char *subject = iks_find_attrib(pak->x, "subject");
	ldl_handle_t *handle = user_data;
	ldl_session_t *session = NULL;
	
	if (from) {
		session = apr_hash_get(handle->sessions, from, APR_HASH_KEY_STRING);
	}

	if (handle->session_callback) {
		handle->session_callback(handle, session, LDL_SIGNAL_MSG, from, subject ? subject : "N/A", cmd); 
	}
	
	return 0;
}

static int on_error(void *user_data, ikspak * pak)
{
	globals.logger(DL_LOG_DEBUG, "authorization failed\n");
	return IKS_FILTER_EAT;
}

static void on_log(ldl_handle_t *handle, const char *data, size_t size, int is_incoming)
{
	if (iks_is_secure(handle->parser)) {
		fprintf(stderr, "Sec");
	}
	if (is_incoming) {
		fprintf(stderr, "RECV");
	} else {
		fprintf(stderr, "SEND");
	}
	fprintf(stderr, "[%s]\n", data);
}

static void j_setup_filter(ldl_handle_t *handle, char *login)
{
	if (handle->filter) {
		iks_filter_delete(handle->filter);
	}
	handle->filter = iks_filter_new();

	/*
	iks_filter_add_rule(handle->filter, on_msg, 0,
						IKS_RULE_TYPE, IKS_PAK_MESSAGE,
						IKS_RULE_SUBTYPE, IKS_TYPE_CHAT, IKS_RULE_FROM, login, IKS_RULE_DONE);
	*/
	iks_filter_add_rule(handle->filter, on_msg, handle, IKS_RULE_TYPE, IKS_PAK_MESSAGE, IKS_RULE_SUBTYPE, IKS_TYPE_CHAT, IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_result, handle,
						IKS_RULE_TYPE, IKS_PAK_IQ,
						IKS_RULE_SUBTYPE, IKS_TYPE_RESULT, IKS_RULE_ID, "auth", IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_presence, handle,
						IKS_RULE_TYPE, IKS_PAK_PRESENCE,
						//IKS_RULE_SUBTYPE, IKS_TYPE_SET,
						IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_commands, handle,
						IKS_RULE_TYPE, IKS_PAK_IQ,
						IKS_RULE_SUBTYPE, IKS_TYPE_SET,
						IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_commands, handle,
						IKS_RULE_TYPE, IKS_PAK_IQ,
						IKS_RULE_SUBTYPE, IKS_TYPE_RESULT,
						IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_commands, handle,
						IKS_RULE_TYPE, IKS_PAK_IQ,
						IKS_RULE_SUBTYPE, IKS_TYPE_ERROR,
						IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_subscribe, handle,
						IKS_RULE_TYPE, IKS_PAK_S10N,
						IKS_RULE_SUBTYPE, IKS_TYPE_SUBSCRIBE,
						IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_error, handle,
						IKS_RULE_TYPE, IKS_PAK_IQ,
						IKS_RULE_SUBTYPE, IKS_TYPE_ERROR, IKS_RULE_ID, "auth", IKS_RULE_DONE);
}

static void ldl_flush_queue(ldl_handle_t *handle)
{
	iks *msg;
	void *pop;
	unsigned int len = 0, x = 0;

	while(apr_queue_trypop(handle->queue, &pop) == APR_SUCCESS) {
		msg = (iks *) pop;
		iks_send(handle->parser, msg);
		iks_delete(msg);
	}

	len = apr_queue_size(handle->retry_queue); 

	if (globals.debug && len) {
		globals.logger(DL_LOG_DEBUG, "Processing %u packets in retry queue\n", len);
	}
	apr_thread_mutex_lock(handle->lock);		
	while(x < len && apr_queue_trypop(handle->retry_queue, &pop) == APR_SUCCESS) {
		struct packet_node *packet_node = (struct packet_node *) pop;
		apr_time_t now = apr_time_now();
		x++;

		//printf("%s %lld %lld %u\n", packet_node->id, packet_node->next, now, packet_node->retries);
		if (packet_node->next <= now) {
			if (packet_node->retries > 0) {
				packet_node->retries--;
				if (globals.debug) {
					globals.logger(DL_LOG_DEBUG, "Sending packet %s (%d left)\n", packet_node->id, packet_node->retries);
				}
				iks_send(handle->parser, packet_node->xml);
				packet_node->next = now + 5000000;
			}
		}
		if (packet_node->retries == 0) {
			if (globals.debug) {
				globals.logger(DL_LOG_DEBUG, "Discarding packet %s\n", packet_node->id);
			}
			apr_hash_set(handle->retry_hash, packet_node->id, APR_HASH_KEY_STRING, NULL);
			iks_delete(packet_node->xml);
			free(packet_node);
		} else {
			apr_queue_push(handle->retry_queue, packet_node);
		}
	}
	apr_thread_mutex_unlock(handle->lock);
}

static void xmpp_connect(ldl_handle_t *handle, char *jabber_id, char *pass)
{
	while (ldl_test_flag(handle, LDL_FLAG_RUNNING)) {
		int e;

		handle->parser = iks_stream_new(IKS_NS_CLIENT, handle, (iksStreamHook *) on_stream);
		if (globals.debug) {
			iks_set_log_hook(handle->parser, (iksLogHook *) on_log);
		}
		handle->acc = iks_id_new(iks_parser_stack(handle->parser), jabber_id);
		if (NULL == handle->acc->resource) {
			/* user gave no resource name, use the default */
			char tmp[512];
			sprintf(tmp, "%s@%s/%s", handle->acc->user, handle->acc->server, "dingaling");
			handle->acc = iks_id_new(iks_parser_stack(handle->parser), tmp);
		}
		handle->password = pass;

		j_setup_filter(handle, "fixme");

		e = iks_connect_tcp(handle->parser, handle->acc->server, IKS_JABBER_PORT);
		switch (e) {
		case IKS_OK:
			break;
		case IKS_NET_NODNS:
			globals.logger(DL_LOG_DEBUG, "hostname lookup failed\n");
		case IKS_NET_NOCONN:
			globals.logger(DL_LOG_DEBUG, "connection failed\n");
		default:
			globals.logger(DL_LOG_DEBUG, "io error %d\n", e);
			microsleep(500);
			continue;
		}

		handle->counter = opt_timeout;
		while (ldl_test_flag(handle, LDL_FLAG_RUNNING)) {
			e = iks_recv(handle->parser, 1);
			if (handle->loop_callback) {
				if (handle->loop_callback(handle) != LDL_STATUS_SUCCESS) {
					ldl_clear_flag(handle, LDL_FLAG_RUNNING);	
					break;
				}
			}

			if (handle->job_done) {
				break;
			}

			if (IKS_HOOK == e) {
				break;
			}

			if (IKS_OK != e) {
				globals.logger(DL_LOG_DEBUG, "io error %d\n", e);
				microsleep(500);
				break;
			}

			if (ldl_test_flag(handle, LDL_FLAG_READY)) {
				ldl_flush_queue(handle);
			}

			if (!ldl_test_flag(handle, LDL_FLAG_AUTHORIZED)) {
				if (IKS_NET_TLSFAIL == e) {
					globals.logger(DL_LOG_DEBUG, "tls handshake failed\n");
					microsleep(500);
					break;
				}

				if (handle->counter == 0) {
					globals.logger(DL_LOG_DEBUG, "network timeout\n");
					microsleep(500);
					break;
				}
			}
		}

		iks_disconnect(handle->parser);
		iks_parser_delete(handle->parser);
		ldl_clear_flag(handle, LDL_FLAG_AUTHORIZED);
	}

	ldl_clear_flag(handle, LDL_FLAG_RUNNING);
}



static ldl_status new_session_iq(ldl_session_t *session, iks **iqp, iks **sessp, unsigned int *id, char *type)
{
	iks *iq, *sess;
	unsigned int myid;
	char idbuf[80];
	
	myid = next_id();
	snprintf(idbuf, sizeof(idbuf), "%u", myid);
	iq = iks_new("iq");
	iks_insert_attrib(iq, "xmlns", "jabber:client");
	iks_insert_attrib(iq, "from", session->handle->login);
	iks_insert_attrib(iq, "to", session->them);
	iks_insert_attrib(iq, "type", "set");
	iks_insert_attrib(iq, "id", idbuf);
	sess = iks_insert (iq, "session");
	iks_insert_attrib(sess, "xmlns", "http://www.google.com/session");

	iks_insert_attrib(sess, "type", type);
	iks_insert_attrib(sess, "id", session->id);
	iks_insert_attrib(sess, "initiator", session->initiator ? session->initiator : session->them);	
	*sessp = sess;
	*iqp = iq;
	*id = myid;
	return LDL_STATUS_SUCCESS;
}

static void schedule_packet(ldl_handle_t *handle, unsigned int id, iks *xml, unsigned int retries)
{
	struct packet_node  *packet_node;
	
	apr_thread_mutex_lock(handle->lock);
	if ((packet_node = malloc(sizeof(*packet_node)))) {
		memset(packet_node, 0, sizeof(*packet_node));
		snprintf(packet_node->id, sizeof(packet_node->id), "%u", id);
		packet_node->xml = xml;
		packet_node->retries = retries;
		packet_node->next = apr_time_now();
		apr_queue_push(handle->retry_queue, packet_node);
		apr_hash_set(handle->retry_hash, packet_node->id, APR_HASH_KEY_STRING, packet_node);
	}
	apr_thread_mutex_unlock(handle->lock);

}

char *ldl_session_get_caller(ldl_session_t *session)
{
	return session->them;
}

void ldl_session_set_ip(ldl_session_t *session, char *ip)
{
	session->ip = apr_pstrdup(session->pool, ip);
}

char *ldl_session_get_ip(ldl_session_t *session)
{
	return session->ip;
}

void ldl_session_set_private(ldl_session_t *session, void *private_data)
{
	session->private_data = private_data;
}

void *ldl_session_get_private(ldl_session_t *session)
{
	return session->private_data;
}

void *ldl_handle_get_private(ldl_handle_t *handle)
{
	return handle->private_info;
}

void ldl_handle_send_msg(ldl_handle_t *handle, char *to, char *subject, char *body)
{
	iks *msg;

	assert(handle != NULL);
	assert(body != NULL);

	msg = iks_make_msg(IKS_TYPE_NONE, to, body);

	if (subject) {
		iks_insert_attrib(msg, "subject", subject);
	}

	apr_queue_push(handle->queue, msg);
	
}

void ldl_global_set_logger(ldl_logger_t logger)
{
	globals.logger = logger;
}

unsigned int ldl_session_terminate(ldl_session_t *session)
{
	iks *iq, *sess;
	unsigned int id;

	new_session_iq(session, &iq, &sess, &id, "terminate");
	schedule_packet(session->handle, id, iq, LDL_RETRY);

	return id;

}

unsigned int ldl_session_candidates(ldl_session_t *session,
								  ldl_candidate_t *candidates,
								  unsigned int clen)

{
	iks *iq, *sess, *tag;
	unsigned int x, id;

	new_session_iq(session, &iq, &sess, &id, "candidates");
	for (x = 0; x < clen; x++) {
		char buf[512];
		tag = iks_insert(sess, "candidate");
		if (candidates[x].name) {
			iks_insert_attrib(tag, "name", candidates[x].name);
		}
		if (candidates[x].address) {
			iks_insert_attrib(tag, "address", candidates[x].address);
		}
		if (candidates[x].port) {
			snprintf(buf, sizeof(buf), "%u", candidates[x].port);
			iks_insert_attrib(tag, "port", buf);
		}
		if (candidates[x].username) {
			iks_insert_attrib(tag, "username", candidates[x].username);
		}
		if (candidates[x].password) {
			iks_insert_attrib(tag, "password", candidates[x].password);
		}
		if (candidates[x].pref) {
			snprintf(buf, sizeof(buf), "%0.1f", candidates[x].pref);
			iks_insert_attrib(tag, "preference", buf);
		}
		if (candidates[x].protocol) {
			iks_insert_attrib(tag, "protocol", candidates[x].protocol);
		}
		if (candidates[x].type) {
			iks_insert_attrib(tag, "type", candidates[x].type);
		}

		iks_insert_attrib(tag, "network", "0");
		iks_insert_attrib(tag, "generation", "0");
	}
	schedule_packet(session->handle, id, iq, LDL_RETRY);

	return id;
}


char *ldl_handle_probe(ldl_handle_t *handle, char *id, char *buf, unsigned int len)
{
	iks *pres, *msg;
	char *lid = NULL;
	struct ldl_buffer buffer;
	apr_time_t started;
	unsigned int elapsed;
	char *notice = "Call Me!";
	int again = 0;

	buffer.buf = buf;
	buffer.len = len;
	buffer.hit = 0;

	pres = iks_new("presence");
	iks_insert_attrib(pres, "type", "probe");
	iks_insert_attrib(pres, "to", id);


	apr_hash_set(handle->probe_hash, id, APR_HASH_KEY_STRING, &buffer);
	msg = iks_make_s10n (IKS_TYPE_SUBSCRIBE, id, notice); 
	apr_queue_push(handle->queue, msg);
	msg = iks_make_s10n (IKS_TYPE_SUBSCRIBED, id, notice); 
	apr_queue_push(handle->queue, msg);
	apr_queue_push(handle->queue, pres);

	//schedule_packet(handle, next_id(), pres, LDL_RETRY);

	started = apr_time_now();
	for(;;) {
		elapsed = (unsigned int)((apr_time_now() - started) / 1000);
		if (elapsed > 5000 && ! again) {
			msg = iks_make_s10n (IKS_TYPE_SUBSCRIBE, id, notice); 
			apr_queue_push(handle->queue, msg);
			again++;
		}
		if (elapsed > 10000) {
			break;
		}
		if (buffer.hit) {
			lid = buffer.buf;
			break;
		}
		ldl_yield(1000);
	}

	apr_hash_set(handle->probe_hash, id, APR_HASH_KEY_STRING, NULL);
	return lid;
}


unsigned int ldl_session_describe(ldl_session_t *session,
								ldl_payload_t *payloads,
								unsigned int plen,
								ldl_description_t description)
{
	iks *iq, *sess, *tag, *payload;
	unsigned int x, id;
	

	new_session_iq(session, &iq, &sess, &id, description == LDL_DESCRIPTION_ACCEPT ? "accept" : "initiate");
	tag = iks_insert(sess, "description");
	iks_insert_attrib(tag, "xmlns", "http://www.google.com/session/phone");
	for (x = 0; x < plen; x++) {
		char idbuf[80];
		payload = iks_insert(tag, "payload-type");
		iks_insert_attrib(payload, "xmlns", "http://www.google.com/session/phone");

		sprintf(idbuf, "%d", payloads[x].id);
		iks_insert_attrib(payload, "id", idbuf);
		iks_insert_attrib(payload, "name", payloads[x].name);
		if (payloads[x].rate) {
			sprintf(idbuf, "%d", payloads[x].rate);
			iks_insert_attrib(payload, "rate", idbuf);
		}
	}

	schedule_packet(session->handle, id, iq, LDL_RETRY);

	return id;
}

ldl_state_t ldl_session_get_state(ldl_session_t *session)
{
	return session->state;
}

ldl_status ldl_session_get_candidates(ldl_session_t *session, ldl_candidate_t **candidates, unsigned int *len)
{
	if (session->candidate_len) {
		*candidates = session->candidates;
		*len = session->candidate_len;
		return LDL_STATUS_SUCCESS;
	} else {
		*candidates = NULL;
		*len = 0;
		return LDL_STATUS_FALSE;
	}
}

ldl_status ldl_session_get_payloads(ldl_session_t *session, ldl_payload_t **payloads, unsigned int *len)
{
	if (session->payload_len) {
		*payloads = session->payloads;
		*len = session->payload_len;
		return LDL_STATUS_SUCCESS;
	} else {
		*payloads = NULL;
		*len = 0;
		return LDL_STATUS_FALSE;
	}
}

ldl_status ldl_global_init(int debug)
{
	if (ldl_test_flag((&globals), LDL_FLAG_INIT)) {
		return LDL_STATUS_FALSE;
	}

	if (apr_initialize() != LDL_STATUS_SUCCESS) {
		apr_terminate();
		return LDL_STATUS_MEMERR;
	}

	memset(&globals, 0, sizeof(globals));
	if (apr_pool_create(&globals.memory_pool, NULL) != LDL_STATUS_SUCCESS) {
		globals.logger(DL_LOG_DEBUG, "Could not allocate memory pool\n");
		return LDL_STATUS_MEMERR;
	}

	globals.log_stream = stdout;
	globals.debug = debug;
	globals.id = 300;
	globals.logger = default_logger;
	ldl_set_flag(&globals, LDL_FLAG_INIT);
	
	return LDL_STATUS_SUCCESS;
}

ldl_status ldl_global_destroy(void)
{
	if (ldl_test_flag(&globals, LDL_FLAG_INIT)) {
		return LDL_STATUS_FALSE;
	}

	apr_pool_destroy(globals.memory_pool);
	ldl_clear_flag(&globals, LDL_FLAG_INIT);
	apr_terminate();

	return LDL_STATUS_SUCCESS;
}

void ldl_global_set_log_stream(FILE *log_stream)
{
	assert(ldl_test_flag(&globals, LDL_FLAG_INIT));

	globals.log_stream = log_stream;
}

int8_t ldl_handle_ready(ldl_handle_t *handle)
{
	return ldl_test_flag(handle, LDL_FLAG_READY);
}

ldl_status ldl_handle_init(ldl_handle_t **handle,
						   char *login,
						   char *password,
						   char *status_msg,
						   ldl_loop_callback_t loop_callback,
						   ldl_session_callback_t session_callback,
						   ldl_response_callback_t response_callback,
						   void *private_info)
{
	apr_pool_t *pool;
	assert(ldl_test_flag(&globals, LDL_FLAG_INIT));
	

	if ((apr_pool_create(&pool, globals.memory_pool)) != LDL_STATUS_SUCCESS) {
		return LDL_STATUS_MEMERR;
	}

	if ((*handle = apr_palloc(pool, sizeof(ldl_handle_t)))) {
		ldl_handle_t *new_handle = *handle;
		memset(new_handle, 0, sizeof(ldl_handle_t));
		new_handle->log_stream = globals.log_stream;
		new_handle->login = apr_pstrdup(pool, login);
		new_handle->password = apr_pstrdup(pool, password);
		new_handle->status_msg = apr_pstrdup(pool, status_msg);

		if (loop_callback) {
			new_handle->loop_callback = loop_callback;
		}

		if (session_callback) {
			new_handle->session_callback = session_callback;
		}

		if (response_callback) {
			new_handle->response_callback = response_callback;
		}

		new_handle->private_info = private_info;
		new_handle->pool = pool;
		apr_queue_create(&new_handle->queue, LDL_HANDLE_QLEN, new_handle->pool);
		apr_queue_create(&new_handle->retry_queue, LDL_HANDLE_QLEN, new_handle->pool);
		new_handle->features |= IKS_STREAM_BIND|IKS_STREAM_SESSION |IKS_STREAM_SASL_PLAIN;
		new_handle->sessions = apr_hash_make(new_handle->pool);
		new_handle->retry_hash = apr_hash_make(new_handle->pool);
		new_handle->probe_hash = apr_hash_make(new_handle->pool);
		apr_thread_mutex_create(&new_handle->lock, APR_THREAD_MUTEX_NESTED, new_handle->pool);

		return LDL_STATUS_SUCCESS;
	} else {
		*handle = NULL;
	}

	return LDL_STATUS_FALSE;
}

void ldl_handle_run(ldl_handle_t *handle)
{
	ldl_set_flag(handle, LDL_FLAG_RUNNING);
	xmpp_connect(handle, handle->login, handle->password);
	ldl_clear_flag(handle, LDL_FLAG_RUNNING);
}

void ldl_handle_stop(ldl_handle_t *handle)
{
	ldl_clear_flag(handle, LDL_FLAG_RUNNING);
}

ldl_status ldl_handle_destroy(ldl_handle_t **handle)
{
	apr_pool_t *pool = (*handle)->pool;

	ldl_flush_queue(*handle);
	
	apr_pool_destroy(pool);
	*handle = NULL;
	return LDL_STATUS_SUCCESS;
}


void ldl_handle_set_log_stream(ldl_handle_t *handle, FILE *log_stream)
{
	assert(ldl_test_flag(&globals, LDL_FLAG_INIT));

	handle->log_stream = log_stream;
}


