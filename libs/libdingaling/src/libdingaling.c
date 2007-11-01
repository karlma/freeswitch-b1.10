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

#ifndef  _MSC_VER
#include <config.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
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
#include "sha1.h"

#ifdef _MSC_VER
#include <io.h>
#pragma warning(disable:4127 4706)
#endif

#define microsleep(x) apr_sleep(x * 1000)
#define LDL_CAPS_VER "1.0.0.1"

static int opt_timeout = 30;

static void sha1_hash(char *out, unsigned char *in, unsigned int len);
static int b64encode(unsigned char *in, size_t ilen, unsigned char *out, size_t olen);
static void ldl_random_string(char *buf, uint16_t len, char *set);

static struct {
	unsigned int flags;
	FILE *log_stream;
	int debug;
	apr_pool_t *memory_pool;
	unsigned int id;
	ldl_logger_t logger;
	apr_hash_t *avatar_hash;
	apr_thread_mutex_t *flag_mutex;
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

typedef enum {
	CS_NEW,
	CS_START,
	CS_CONNECTED
} ldl_handle_state_t;

struct ldl_handle {
	iksparser *parser;
	iksid *acc;
	iksfilter *filter;
	char *login;
	char *password;
	char *server;
	char *status_msg;
	uint16_t port;
	int features;
	int counter;
	int job_done;
	unsigned int flags;
	apr_queue_t *queue;
	apr_queue_t *retry_queue;
	apr_hash_t *sessions;
	apr_hash_t *retry_hash;
	apr_hash_t *probe_hash;
	apr_hash_t *sub_hash;
	apr_thread_mutex_t *lock;
	apr_thread_mutex_t *flag_mutex;
	ldl_loop_callback_t loop_callback;
	ldl_session_callback_t session_callback;
	ldl_response_callback_t response_callback;
	apr_pool_t *pool;
	void *private_info;
	FILE *log_stream;
	ldl_handle_state_t state;
};

struct ldl_session {
	ldl_state_t state;
	ldl_handle_t *handle;
	char *id;
	char *initiator;
	char *them;
	char *ip;
	char *login;
	ldl_payload_t payloads[LDL_MAX_PAYLOADS];
	unsigned int payload_len;
	ldl_candidate_t candidates[LDL_MAX_CANDIDATES];
	unsigned int candidate_len;
	apr_pool_t *pool;
	apr_hash_t *variables;
	apr_time_t created;
	void *private_data;
};

static int on_disco_default(void *user_data, ikspak *pak);
static int on_vcard(void *user_data, ikspak *pak);
typedef int (*iks_filter_callback_t)(void *user_data, ikspak *pak);

struct ldl_feature {
	const char *name;
	iks_filter_callback_t callback;
};
typedef struct ldl_feature ldl_feature_t;

#define FEATURE_DISCO "http://jabber.org/protocol/disco"
#define FEATURE_DISCO_INFO "http://jabber.org/protocol/disco#info"
#define FEATURE_VERSION "jabber:iq:version"
#define FEATURE_VCARD "vcard-temp"
#define FEATURE_VOICE "http://www.google.com/xmpp/protocol/voice/v1"
#define FEATURE_LAST "jabber:iq:last"

static ldl_feature_t FEATURES[] = { 
	{ FEATURE_DISCO, on_disco_default },
	{ FEATURE_DISCO_INFO, on_disco_default },
	{ FEATURE_VERSION, on_disco_default },
	{ FEATURE_VCARD, on_vcard},
	{ FEATURE_VOICE, on_disco_default },
	{ FEATURE_LAST, on_disco_default },
	{ NULL, NULL}
};


struct ldl_avatar {
	char *path;
	char *base64;
	char hash[256];
};

typedef struct ldl_avatar ldl_avatar_t;


static void lowercase(char *str) 
{
	size_t x = 0;

	if (str) {
		for (x = 0; x < strlen(str); x++) {
			str[x] = (char)tolower((int)str[x]);
		}
	}
}

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


char *ldl_session_get_value(ldl_session_t *session, char *key)
{
	return apr_hash_get(session->variables, key, APR_HASH_KEY_STRING);
}

void ldl_session_set_value(ldl_session_t *session, const char *key, const char *val)
{
	apr_hash_set(session->variables, apr_pstrdup(session->pool, key), APR_HASH_KEY_STRING, apr_pstrdup(session->pool, val));
}

char *ldl_session_get_id(ldl_session_t *session)
{
	return session->id;
}

void ldl_session_send_msg(ldl_session_t *session, char *subject, char *body)
{
	ldl_handle_send_msg(session->handle, session->login, session->them, subject, body);
}

ldl_status ldl_session_destroy(ldl_session_t **session_p)
{
	ldl_session_t *session = *session_p;

	if (session) {
		apr_pool_t *pool = session->pool;
		apr_hash_t *hash = session->handle->sessions;

		if (globals.debug) {
			globals.logger(DL_LOG_DEBUG, "Destroyed Session %s\n", session->id);
		}

		if (session->id) {
			apr_hash_set(hash, session->id, APR_HASH_KEY_STRING, NULL);
		}

		if (session->them) {
			apr_hash_set(hash, session->them, APR_HASH_KEY_STRING, NULL);
		}

		apr_pool_destroy(pool);
		pool = NULL;
		*session_p = NULL;
		return LDL_STATUS_SUCCESS;
	}

	return LDL_STATUS_FALSE;
}


ldl_status ldl_session_create(ldl_session_t **session_p, ldl_handle_t *handle, char *id, char *them, char *me, ldl_user_flag_t flags)
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
	
	if (flags & LDL_FLAG_OUTBOUND) {
		session->initiator = apr_pstrdup(session->pool, me);
	}

	if (ldl_test_flag(handle, LDL_FLAG_COMPONENT)) {
		session->login = apr_pstrdup(session->pool, me);
	} else {
		session->login = apr_pstrdup(session->pool, handle->login);
	}

	apr_hash_set(handle->sessions, session->id, APR_HASH_KEY_STRING, session);
	apr_hash_set(handle->sessions, session->them, APR_HASH_KEY_STRING, session);
	session->handle = handle;
	session->created = apr_time_now();
	session->state = LDL_STATE_NEW;
	session->variables = apr_hash_make(session->pool);
	*session_p = session;


	if (globals.debug) {
		globals.logger(DL_LOG_DEBUG, "Created Session %s\n", id);
	}

	return LDL_STATUS_SUCCESS;
}

static ldl_status parse_session_code(ldl_handle_t *handle, char *id, char *from, char *to, iks *xml, char *xtype)
{
	ldl_session_t *session = NULL;
	ldl_signal_t dl_signal = LDL_SIGNAL_NONE;
	char *initiator = iks_find_attrib(xml, "initiator");
	char *msg = NULL;

	if (!(session = apr_hash_get(handle->sessions, id, APR_HASH_KEY_STRING))) {
		ldl_session_create(&session, handle, id, from, to, LDL_FLAG_NONE);
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

				dl_signal = LDL_SIGNAL_INITIATE;
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
								char *rate = iks_find_attrib(itag, "clockrate");
								if (name && id) {
									session->payloads[session->payload_len].name = apr_pstrdup(session->pool, name);
									session->payloads[session->payload_len].id = atoi(id);
									if (rate) {
										session->payloads[session->payload_len].rate = atoi(rate);
									} else {
                                        session->payloads[session->payload_len].rate = 8000;
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
			} else if (!strcasecmp(type, "transport-accept")) {
				dl_signal = LDL_SIGNAL_TRANSPORT_ACCEPT;
			} else if (!strcasecmp(type, "reject")) {
				dl_signal = LDL_SIGNAL_REJECT;
			} else if (!strcasecmp(type, "transport-info") || !strcasecmp(type, "candidates")) {
				char *tid = iks_find_attrib(xml, "id");
				dl_signal = LDL_SIGNAL_CANDIDATES;
				tag = iks_child (xml);
				id = type;
				if (tag && !strcasecmp(iks_name(tag), "transport")) {
					tag = iks_child(tag);
				}
				
				while(tag) {
					if (!strcasecmp(iks_name(tag), "info_element")) {
						char *name = iks_find_attrib(tag, "name");
						char *value = iks_find_attrib(tag, "value");
						if (globals.debug) {
							globals.logger(DL_LOG_DEBUG, "Info Element [%s]=[%s]\n", name, value);
						}
						ldl_session_set_value(session, name, value);
						
					} else if (!strcasecmp(iks_name(tag), "candidate") && session->candidate_len < LDL_MAX_CANDIDATES) {
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

						if (tid) {
							session->candidates[index].tid = apr_pstrdup(session->pool, tid);
						}

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
							session->candidates[index].port = (uint16_t)atoi(key);
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
				dl_signal = LDL_SIGNAL_TERMINATE;
			} else if (!strcasecmp(type, "error")) {
				dl_signal = LDL_SIGNAL_ERROR;
			}
		}
		
		xml = iks_child(xml);
	}

	if (handle->session_callback && dl_signal) {
		handle->session_callback(handle, session, dl_signal, to, from, id, msg); 
	}

	return LDL_STATUS_SUCCESS;
}

const char *marker = "TRUE";


static int on_vcard(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	char *from = iks_find_attrib(pak->x, "from");
	char *to = iks_find_attrib(pak->x, "to");

	if (handle->session_callback) {
		handle->session_callback(handle, NULL, LDL_SIGNAL_VCARD, to, from, pak->id, NULL); 
	}

	return IKS_FILTER_EAT;
}


static int on_disco_default(void *user_data, ikspak *pak)
{
	char *node = NULL;
	char *ns = NULL;
	ldl_handle_t *handle = user_data;
	iks *iq, *query, *tag;
	uint8_t send = 0;
	int x;

	if (pak && pak->query) {
		ns = iks_find_attrib(pak->query, "xmlns");
		node = iks_find_attrib(pak->query, "node");
	}

	if (pak->subtype == IKS_TYPE_RESULT) {
		globals.logger(DL_LOG_DEBUG, "FixME!!! node=[%s]\n", node?node:"");		
	} else if (pak->subtype == IKS_TYPE_GET) {
		if ((iq = iks_new("iq"))) {
			int all = 0;
			
			iks_insert_attrib(iq, "from", handle->login);
			iks_insert_attrib(iq, "to", pak->from->full);
			iks_insert_attrib(iq, "id", pak->id);
			iks_insert_attrib(iq, "type", "result");
					
			if (!(query = iks_insert (iq, "query"))) {
				goto fail;
			}
			iks_insert_attrib(query, "xmlns", ns);

			if (!strcasecmp(ns, FEATURE_LAST)) {
				iks_insert_attrib(query, "seconds", "1");
			}
			
			if (!(tag = iks_insert (query, "identity"))) {
				goto fail;
			}

			iks_insert_attrib(tag, "category", "gateway");
			//iks_insert_attrib(tag, "type", "voice");
			iks_insert_attrib(tag, "name", "LibDingaLing");
			
			if (!strcasecmp(ns, FEATURE_DISCO_INFO)) {
				if (!node) {
					all++;
				} else {
					char *p;

					if ((p = strstr(node, "caps#"))) {
						char *what = p + 5;

						if (!strcasecmp(what, "voice-v1")) {
							if (!(tag = iks_insert (query, "feature"))) {
								goto fail;
							}
							iks_insert_attrib(tag, "var", FEATURE_VOICE);
							goto done;
						}
						
					}
				}
			}
			
			for (x = 0; FEATURES[x].name; x++) {
				if (all || !ns || !strcasecmp(ns, FEATURES[x].name)) {
					if (!(tag = iks_insert (query, "feature"))) {
						goto fail;
					}
					iks_insert_attrib(tag, "var", FEATURES[x].name);
				}
			}

		done:

			apr_queue_push(handle->queue, iq);
			iq = NULL;
			send = 1;
		}
	fail:

		if (iq) {
			iks_delete(iq);
		}

		if (!send) {
			globals.logger(DL_LOG_DEBUG, "Memory Error!\n");
		}
	}

	return IKS_FILTER_EAT;
}

static int on_presence(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	char *from = iks_find_attrib(pak->x, "from");
	char *to = iks_find_attrib(pak->x, "to");
	char *type = iks_find_attrib(pak->x, "type");
	char *show = iks_find_cdata(pak->x, "show");
	char *status = iks_find_cdata(pak->x, "status");
	char id[1024];
	char *resource;
	struct ldl_buffer *buffer;
	ldl_signal_t dl_signal = LDL_SIGNAL_PRESENCE_IN;
	


    if (type && *type) {
        if (!strcasecmp(type, "unavailable")) {
            dl_signal = LDL_SIGNAL_PRESENCE_OUT;
        } else if (!strcasecmp(type, "probe")) {
            dl_signal = LDL_SIGNAL_PRESENCE_PROBE;
        }
        if (!status) {
            status = type;
        }
    } else {
        if (!status) {
            status = "Available";
        }
    }


	apr_cpystrn(id, from, sizeof(id));
	lowercase(id);

	if ((resource = strchr(id, '/'))) {
		*resource++ = '\0';
	}
	

	if (!apr_hash_get(handle->sub_hash, from, APR_HASH_KEY_STRING)) {
		iks *msg;
		apr_hash_set(handle->sub_hash, 	apr_pstrdup(handle->pool, from), APR_HASH_KEY_STRING, &marker);
		if ((msg = iks_make_s10n (IKS_TYPE_SUBSCRIBED, id, "Ding A Ling...."))) {
			apr_queue_push(handle->queue, msg);
			msg = NULL;
		}
	}

	if (resource && (strstr(resource, "talk") || strstr(resource, "telepathy")) && (buffer = apr_hash_get(handle->probe_hash, id, APR_HASH_KEY_STRING))) {
		apr_cpystrn(buffer->buf, from, buffer->len);
		fflush(stderr);
		buffer->hit = 1;
	} 
	
    if (handle->session_callback) {
        handle->session_callback(handle, NULL, dl_signal, to, id, status ? status : "n/a", show ? show : "n/a");
    }

	return IKS_FILTER_EAT;
}

static char *ldl_handle_strdup(ldl_handle_t *handle, char *str)
{
	char *dup;
	apr_size_t len;

	len = strlen(str) + 1;
	dup = apr_palloc(handle->pool, len);
	assert(dup != NULL);
	strncpy(dup, str, len);
	return dup;
}

static void ldl_strip_resource(char *in)
{
	char *p;

	if ((p = strchr(in, '/'))) {
		*p = '\0';
	}
}

static ldl_avatar_t *ldl_get_avatar(ldl_handle_t *handle, char *path, char *from)
{
	ldl_avatar_t *ap;
	uint8_t image[8192];
	unsigned char base64[9216] = "";
	int fd = -1;
	size_t bytes;
	char *key;
	//char hash[128] = "";

	if (from && (ap = (ldl_avatar_t *) apr_hash_get(globals.avatar_hash, from, APR_HASH_KEY_STRING))) {
		return ap;
	}

	if (path && from) {
		if ((ap = (ldl_avatar_t *) apr_hash_get(globals.avatar_hash, path, APR_HASH_KEY_STRING))) {
			key = ldl_handle_strdup(handle, from);
			ldl_strip_resource(key);
			apr_hash_set(globals.avatar_hash, key, APR_HASH_KEY_STRING, ap);
			return ap;
		}
	}

	if (!(path && from)) {
		return NULL;
	}

	if ((fd = open(path, O_RDONLY, 0)) < 0) {
		globals.logger(DL_LOG_ERR, "File %s does not exist!\n", path);
		return NULL;
	}
	
	bytes = read(fd, image, sizeof(image));
	close(fd);
	fd = -1;

	ap = malloc(sizeof(*ap));
	assert(ap != NULL);
	memset(ap, 0, sizeof(*ap));
	sha1_hash(ap->hash, (unsigned char *) image, (unsigned int)bytes);
	ap->path = strdup(path);

	key = ldl_handle_strdup(handle, from);
	ldl_strip_resource(key);

	b64encode((unsigned char *)image, bytes, base64, sizeof(base64));
	ap->base64 = strdup((const char *)base64);
	apr_hash_set(globals.avatar_hash, ap->path, APR_HASH_KEY_STRING, ap);
	apr_hash_set(globals.avatar_hash, key, APR_HASH_KEY_STRING, ap);
	return ap;
}


static void do_presence(ldl_handle_t *handle, char *from, char *to, char *type, char *rpid, char *message, char *avatar) 
{
	iks *pres;
	char buf[512];
	iks *tag;

	if (!strchr(from, '/')) {
		snprintf(buf, sizeof(buf), "%s/talk", from);
		from = buf;
	}

	if (ldl_test_flag(handle, LDL_FLAG_COMPONENT) && ldl_jid_domcmp(from, to)) {
		globals.logger(DL_LOG_ERR, "Refusal to send presence from and to the same domain in component mode [%s][%s]\n", from, to);
		return;
	}

	if ((pres = iks_new("presence"))) {
		iks_insert_attrib(pres, "xmlns", "jabber:client");
		if (from) {
			iks_insert_attrib(pres, "from", from);
		}
		if (to) {
			iks_insert_attrib(pres, "to", to);
		}
		if (type) {
			iks_insert_attrib(pres, "type", type);
		}

		if (rpid) {
			if ((tag = iks_insert (pres, "show"))) {
				iks_insert_cdata(tag, rpid, 0);
			}
		}

		if (message) {
			if ((tag = iks_insert (pres, "status"))) {
				iks_insert_cdata(tag, message, 0);
			}
		}

		if (message || rpid) {
			ldl_avatar_t *ap;

			if (avatar) {
				if ((ap = ldl_get_avatar(handle, avatar, from))) {
					if ((tag = iks_insert(pres, "x"))) {
						iks *hash;
						iks_insert_attrib(tag, "xmlns", "vcard-temp:x:update");
						if ((hash = iks_insert(tag, "photo"))) {
							iks_insert_cdata(hash, ap->hash, 0);
						}
					}
				}
			}

			if ((tag = iks_insert(pres, "c"))) {
				iks_insert_attrib(tag, "node", "http://www.freeswitch.org/xmpp/client/caps");
				iks_insert_attrib(tag, "ver", LDL_CAPS_VER);
				iks_insert_attrib(tag, "ext", "sidebar voice-v1");
				iks_insert_attrib(tag, "client", "libdingaling");
				iks_insert_attrib(tag, "xmlns", "http://jabber.org/protocol/caps");
			}
		}

		apr_queue_push(handle->queue, pres);
		pres = NULL;
	}
}

static void do_roster(ldl_handle_t *handle) 
{
	if (handle->session_callback) {
        handle->session_callback(handle, NULL, LDL_SIGNAL_ROSTER, NULL, handle->login, NULL, NULL);
    }
}

static int on_unsubscribe(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	char *from = iks_find_attrib(pak->x, "from");
	char *to = iks_find_attrib(pak->x, "to");

	if (handle->session_callback) {
		handle->session_callback(handle, NULL, LDL_SIGNAL_UNSUBSCRIBE, to, from, NULL, NULL);
	}

	return IKS_FILTER_EAT;
}

static int on_subscribe(void *user_data, ikspak *pak)
{
	ldl_handle_t *handle = user_data;
	char *from = iks_find_attrib(pak->x, "from");
	char *to = iks_find_attrib(pak->x, "to");
	iks *msg = NULL;
	char *id = strdup(from);
	char *r;

	if (!id) {
		return -1;
	}
	if ((r = strchr(from, '/'))) {
		*r++ = '\0';
	}

	if ((msg = iks_make_s10n (IKS_TYPE_SUBSCRIBED, id, "Ding A Ling...."))) {
		if (to && ldl_test_flag(handle, LDL_FLAG_COMPONENT)) {
			iks_insert_attrib(msg, "from", to);
		}

		apr_queue_push(handle->queue, msg);
		msg = NULL;
	}

	if ((msg = iks_make_s10n (IKS_TYPE_SUBSCRIBE, id, "Ding A Ling...."))) {

		if (to && ldl_test_flag(handle, LDL_FLAG_COMPONENT)) {
			iks_insert_attrib(msg, "from", to);
		}

		apr_queue_push(handle->queue, msg);
		msg = NULL;
	}

	if (handle->session_callback) {
		handle->session_callback(handle, NULL, LDL_SIGNAL_SUBSCRIBE, to, from, NULL, NULL);
	}

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
	char *to = iks_find_attrib(pak->x, "to");
	char *iqid = iks_find_attrib(pak->x, "id");
	char *type = iks_find_attrib(pak->x, "type");
	uint8_t is_result = strcasecmp(type, "result") ? 0 : 1;
	uint8_t is_error = strcasecmp(type, "error") ? 0 : 1;

	iks *xml;

	if (is_result) {
		iks *tag = iks_child (pak->x);
		while(tag) {
			if (!strcasecmp(iks_name(tag), "bind")) {
				char *jid = iks_find_cdata(tag, "jid");
				char *resource = strchr(jid, '/');
				if (resource) {
					resource++;
					handle->acc->resource = apr_pstrdup(handle->pool, resource);
				}
				handle->login = apr_pstrdup(handle->pool, jid);
#if 0
				if ((iq = iks_new("iq"))) {
					iks_insert_attrib(iq, "type", "get");
					iks_insert_attrib(iq, "id", "roster");
					x = iks_insert(iq,  "query");
					iks_insert_attrib(x, "xmlns", "jabber:iq:roster");
					iks_insert_attrib(x, "xmlns:gr", "google:roster");
					iks_insert_attrib(x, "gr:ext", "2");
					iks_insert_attrib(x, "gr:include", "all");
					apr_queue_push(handle->queue, iq);
					iq = NULL;
					break;
				}
#endif
			}
			tag = iks_next_tag(tag);
		}
	}

	if ((is_result || is_error) && iqid && from) {

		cancel_retry(handle, iqid);
		if (is_result) {
			if (handle->response_callback) {
				handle->response_callback(handle, iqid); 
			}
			return IKS_FILTER_EAT;
		} else if (is_error) {
			return IKS_FILTER_EAT;
		}
	}
	
	xml = iks_child (pak->x);
	while (xml) {
		char *name = iks_name(xml);
		if (!strcasecmp(name, "session")) {
			char *id = iks_find_attrib(xml, "id");
			//printf("SESSION type=%s name=%s id=%s\n", type, name, id);
			if (parse_session_code(handle, id, from, to, xml, strcasecmp(type, "error") ? NULL : type) == LDL_STATUS_SUCCESS) {
				iks *reply;
				if ((reply = iks_make_iq(IKS_TYPE_RESULT, NULL))) {
					iks_insert_attrib(reply, "to", from);
					iks_insert_attrib(reply, "from", to);
					iks_insert_attrib(reply, "id", iqid);
					apr_queue_push(handle->queue, reply);
					reply = NULL;
				}
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

	if ((msg = iks_make_pres (IKS_SHOW_AVAILABLE, handle->status_msg))) {
		ctag = iks_insert(msg, "c");
		iks_insert_attrib(ctag, "node", "http://www.freeswitch.org/xmpp/client/caps");
		iks_insert_attrib(ctag, "ver", "1.0.0.1");
		iks_insert_attrib(ctag, "ext", "sidebar voice-v1");
		iks_insert_attrib(ctag, "client", "libdingaling");
		iks_insert_attrib(ctag, "xmlns", "http://jabber.org/protocol/caps");

		apr_queue_push(handle->queue, msg);
		msg = NULL;
		ldl_set_flag_locked(handle, LDL_FLAG_READY);
	}
	return IKS_FILTER_EAT;
}

static const char c64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define B64BUFFLEN 1024

static int b64encode(unsigned char *in, size_t ilen, unsigned char *out, size_t olen) 
{
	int y=0,bytes=0;
	size_t x=0;
	unsigned int b=0,l=0;

	for(x=0;x<ilen;x++) {
		b = (b<<8) + in[x];
		l += 8;
		while (l >= 6) {
			out[bytes++] = c64[(b>>(l-=6))%64];
			if(++y!=72) {
				continue;
			}
			//out[bytes++] = '\n';
			y=0;
		}
	}

	if (l > 0) {
		out[bytes++] = c64[((b%16)<<(6-l))%64];
	}
	if (l != 0) while (l < 6) {
		out[bytes++] = '=', l += 2;
	}

	return 0;
}

static void sha1_hash(char *out, unsigned char *in, unsigned int len)
{
	SHA1Context sha;
	char *p;
	int x;
	unsigned char digest[SHA1_HASH_SIZE] = "";

	SHA1Init(&sha);
	
	SHA1Update(&sha, in, len);

	SHA1Final(&sha, digest);

	p = out;
	for (x = 0; x < SHA1_HASH_SIZE; x++) {
		p += sprintf(p, "%2.2x", digest[x]);
	}
}


static int on_stream_component(ldl_handle_t *handle, int type, iks *node)
{
	ikspak *pak = NULL;

    if (node) {
        pak = iks_packet(node);
    }

	switch (type) {
	case IKS_NODE_START:
		if (pak && handle->state == CS_NEW) {
			char secret[256] = "";
			char hash[256] = "";
			char handshake[512] = "";

			snprintf(secret, sizeof(secret), "%s%s", pak->id, handle->password);
			sha1_hash(hash, (unsigned char *) secret, (unsigned int)strlen(secret));
			snprintf(handshake, sizeof(handshake), "<handshake>%s</handshake>", hash);
			iks_send_raw(handle->parser, handshake);
			handle->state = CS_START;
			if (iks_recv(handle->parser, 1) == 2) {
				handle->state = CS_CONNECTED;
				if (!ldl_test_flag(handle, LDL_FLAG_AUTHORIZED)) {
					ldl_set_flag_locked(handle, LDL_FLAG_READY);
					do_roster(handle);
					if (handle->session_callback) {
						handle->session_callback(handle, NULL, LDL_SIGNAL_LOGIN_SUCCESS, "user", "core", "Login Success", handle->login);
					}
					globals.logger(DL_LOG_DEBUG, "XMPP authenticated\n");
					ldl_set_flag_locked(handle, LDL_FLAG_AUTHORIZED);
				}
			} else {
				globals.logger(DL_LOG_ERR, "LOGIN ERROR!\n");
				handle->state = CS_NEW;
			}
			break;
		}
		break;

	case IKS_NODE_NORMAL:
		break;

	case IKS_NODE_ERROR:
		globals.logger(DL_LOG_ERR, "NODE ERROR!\n");
		return IKS_HOOK;

	case IKS_NODE_STOP:
		globals.logger(DL_LOG_ERR, "DISCONNECTED!\n");
		return IKS_HOOK;
	}
	
    iks_filter_packet(handle->filter, pak);

	if (handle->job_done == 1) {
		return IKS_HOOK;
	}
    
	if (node) {
        iks_delete(node);
	}

	return IKS_OK;
}

static int on_stream(ldl_handle_t *handle, int type, iks *node)
{
	handle->counter = opt_timeout;


    switch (type) {
	case IKS_NODE_START:
		if (ldl_test_flag(handle, LDL_FLAG_TLS) && !iks_is_secure(handle->parser)) {
			if (iks_has_tls()) {
				iks_start_tls(handle->parser);
			} else {
				globals.logger(DL_LOG_DEBUG, "TLS NOT SUPPORTED IN THIS BUILD!\n");
			}
		}
		break;
	case IKS_NODE_NORMAL:
        if (node && strcmp("stream:features", iks_name(node)) == 0) {
			handle->features = iks_stream_features(node);
			if (ldl_test_flag(handle, LDL_FLAG_TLS) && !iks_is_secure(handle->parser))
				break;
			if (ldl_test_flag(handle, LDL_FLAG_CONNECTED)) {
				iks *t;
				if (handle->features & IKS_STREAM_BIND) {
					if ((t = iks_make_resource_bind(handle->acc))) {
						apr_queue_push(handle->queue, t);
						t = NULL;
					}
				}
				if (handle->features & IKS_STREAM_SESSION) {
					if ((t = iks_make_session())) {
						iks_insert_attrib(t, "id", "auth");
						apr_queue_push(handle->queue, t);
						t = NULL;
					}
				}
			} else {
				if (handle->features & IKS_STREAM_SASL_MD5) {
					iks_start_sasl(handle->parser, IKS_SASL_DIGEST_MD5, handle->acc->user, handle->password);
				} else if (handle->features & IKS_STREAM_SASL_PLAIN) {
					iks *x = NULL;

					if ((x = iks_new("auth"))) {
						char s[512] = "";
						char base64[1024] = "";
						uint32_t slen;

						iks_insert_attrib(x, "xmlns", IKS_NS_XMPP_SASL);
						iks_insert_attrib(x, "mechanism", "PLAIN");
						iks_insert_attrib(x, "encoding", "UTF-8");
						snprintf(s, sizeof(s), "%c%s%c%s", 0, handle->acc->user, 0, handle->password);
						slen = (uint32_t)(strlen(handle->acc->user) + strlen(handle->password) + 2);
						b64encode((unsigned char *)s, slen, (unsigned char *) base64, sizeof(base64));
						iks_insert_cdata(x, base64, 0);
						apr_queue_push(handle->queue, x);
						x = NULL;
					} else {
						globals.logger(DL_LOG_DEBUG, "Memory ERROR!\n");
						break;
					}
					
				}
			}
		} else if (node && strcmp("failure", iks_name(node)) == 0) {
			globals.logger(DL_LOG_DEBUG, "sasl authentication failed\n");
			if (handle->session_callback) {
				handle->session_callback(handle, NULL, LDL_SIGNAL_LOGIN_FAILURE, "user", "core", "Login Failure", handle->login);
			}
		} else if (node && strcmp("success", iks_name(node)) == 0) {
			globals.logger(DL_LOG_DEBUG, "XMPP server connected\n");
			iks_send_header(handle->parser, handle->acc->server);
			ldl_set_flag_locked(handle, LDL_FLAG_CONNECTED);
			if (handle->session_callback) {
				handle->session_callback(handle, NULL, LDL_SIGNAL_CONNECTED, "user", "core", "Server Connected", handle->login);
			}
		} else {
			ikspak *pak;
			if (!ldl_test_flag(handle, LDL_FLAG_AUTHORIZED)) {
				if (handle->session_callback) {
					handle->session_callback(handle, NULL, LDL_SIGNAL_LOGIN_SUCCESS, "user", "core", "Login Success", handle->login);
				}
				globals.logger(DL_LOG_DEBUG, "XMPP authenticated\n");
				ldl_set_flag_locked(handle, LDL_FLAG_AUTHORIZED);
			}
            if (node) {
                pak = iks_packet(node);
                iks_filter_packet(handle->filter, pak);
            }
			if (handle->job_done == 1) {
				return IKS_HOOK;
			}
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

	if (node) {
		iks_delete(node);
    }
	return IKS_OK;
}

static int on_msg(void *user_data, ikspak *pak)
{
	char *cmd = iks_find_cdata(pak->x, "body");
	char *to = iks_find_attrib(pak->x, "to");
	char *from = iks_find_attrib(pak->x, "from");
	char *subject = iks_find_attrib(pak->x, "subject");
	ldl_handle_t *handle = user_data;
	ldl_session_t *session = NULL;
	
	if (from) {
		session = apr_hash_get(handle->sessions, from, APR_HASH_KEY_STRING);
	}

	if (handle->session_callback) {
		handle->session_callback(handle, session, LDL_SIGNAL_MSG, to, from, subject ? subject : "N/A", cmd); 
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
	if (globals.debug) {
		if (is_incoming) {
			globals.logger(DL_LOG_INFO, "+xml:%s%s:%s", iks_is_secure(handle->parser) ? "Sec" : "", is_incoming ? "RECV" : "SEND", data);
		} else {
			globals.logger(DL_LOG_NOTICE, "+xml:%s%s:%s", iks_is_secure(handle->parser) ? "Sec" : "", is_incoming ? "RECV" : "SEND", data);
		}
						   
	}
}

static void j_setup_filter(ldl_handle_t *handle)
{
	int x = 0;

	if (handle->filter) {
		iks_filter_delete(handle->filter);
	}
	handle->filter = iks_filter_new();

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

	iks_filter_add_rule(handle->filter, on_unsubscribe, handle,
						IKS_RULE_TYPE, IKS_PAK_S10N,
						IKS_RULE_SUBTYPE, IKS_TYPE_UNSUBSCRIBE,
						IKS_RULE_DONE);

	iks_filter_add_rule(handle->filter, on_error, handle,
						IKS_RULE_TYPE, IKS_PAK_IQ,
						IKS_RULE_SUBTYPE, IKS_TYPE_ERROR, IKS_RULE_ID, "auth", IKS_RULE_DONE);

	for (x = 0; FEATURES[x].name; x++) {
		iks_filter_add_rule(handle->filter, FEATURES[x].callback, handle, 
							IKS_RULE_NS, FEATURES[x].name, IKS_RULE_DONE);
	}
}

static void ldl_flush_queue(ldl_handle_t *handle, int done)
{
	iks *msg;
	void *pop = NULL;
	unsigned int len = 0, x = 0;

	apr_thread_mutex_lock(handle->lock);

	while(apr_queue_trypop(handle->queue, &pop) == APR_SUCCESS) {
		if (pop) {
			msg = (iks *) pop;
			if (!done) iks_send(handle->parser, msg);
			iks_delete(msg);
			pop = NULL;
		} else {
			break;
		}
	}

	len = apr_queue_size(handle->retry_queue); 
	if (globals.debug && len) {
		globals.logger(DL_LOG_DEBUG, "Processing %u packets in retry queue\n", len);
	}

	pop = NULL;

	while(x < len && apr_queue_trypop(handle->retry_queue, &pop) == APR_SUCCESS) {
		if (!pop) {
			break;
		} else {
			struct packet_node *packet_node = (struct packet_node *) pop;
			apr_time_t now = apr_time_now();
			x++;

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
			if (packet_node->retries == 0 || done) {
				if (globals.debug) {
					globals.logger(DL_LOG_DEBUG, "Discarding packet %s\n", packet_node->id);
				}
				apr_hash_set(handle->retry_hash, packet_node->id, APR_HASH_KEY_STRING, NULL);
				iks_delete(packet_node->xml);
				free(packet_node);
			} else {
				apr_queue_push(handle->retry_queue, packet_node);
				packet_node = NULL;
			}
			pop = NULL;
		}
	}
	apr_thread_mutex_unlock(handle->lock);
}


static void *APR_THREAD_FUNC queue_thread(apr_thread_t *thread, void *obj)
{
	ldl_handle_t *handle = (ldl_handle_t *) obj;

	ldl_set_flag_locked(handle, LDL_FLAG_QUEUE_RUNNING);

	while (ldl_test_flag(handle, LDL_FLAG_RUNNING)) {
		ldl_flush_queue(handle, 0);

		if (handle->loop_callback(handle) != LDL_STATUS_SUCCESS) {
			int fd;

			if ((fd = iks_fd(handle->parser)) > -1) {
				shutdown(fd, 0x02);
			}
			ldl_clear_flag_locked(handle, LDL_FLAG_RUNNING);	
			break;
		}
		microsleep(100);
	}
	
	ldl_clear_flag_locked(handle, LDL_FLAG_QUEUE_RUNNING);

	return NULL;
}

static void launch_queue_thread(ldl_handle_t *handle)
{
    apr_thread_t *thread;
    apr_threadattr_t *thd_attr;;
    apr_threadattr_create(&thd_attr, handle->pool);
    apr_threadattr_detach_set(thd_attr, 1);

	apr_threadattr_stacksize_set(thd_attr, 512 * 1024);
	apr_thread_create(&thread, thd_attr, queue_thread, handle, handle->pool);

}


static void xmpp_connect(ldl_handle_t *handle, char *jabber_id, char *pass)
{
	while (ldl_test_flag(handle, LDL_FLAG_RUNNING)) {
		int e;
		char tmp[512], *sl;

		handle->parser = iks_stream_new(ldl_test_flag(handle, LDL_FLAG_COMPONENT) ? IKS_NS_COMPONENT : IKS_NS_CLIENT,
										handle,
										(iksStreamHook *) (ldl_test_flag(handle, LDL_FLAG_COMPONENT) ? on_stream_component : on_stream));


		iks_set_log_hook(handle->parser, (iksLogHook *) on_log);
			
		
		strncpy(tmp, jabber_id, sizeof(tmp)-1);
		sl = strchr(tmp, '/');

		if (!sl && !ldl_test_flag(handle, LDL_FLAG_COMPONENT)) {
			/* user gave no resource name, use the default */
			snprintf(tmp + strlen(tmp), sizeof(tmp) - strlen(tmp), "/%s", "talk");
		} else if (sl && ldl_test_flag(handle, LDL_FLAG_COMPONENT)) {
			*sl = '\0';
		}

		handle->acc = iks_id_new(iks_parser_stack(handle->parser), tmp);

		handle->password = pass;

		j_setup_filter(handle);

		e = iks_connect_via(handle->parser,
							handle->server ? handle->server : handle->acc->server,
							handle->port ? handle->port : IKS_JABBER_PORT,
							handle->acc->server);

		switch (e) {
		case IKS_OK:
			break;
		case IKS_NET_NODNS:
			globals.logger(DL_LOG_DEBUG, "hostname lookup failed\n");
			microsleep(1000);
			goto fail;
		case IKS_NET_NOCONN:
			globals.logger(DL_LOG_DEBUG, "connection failed\n");
			microsleep(1000);
			goto fail;
		default:
			globals.logger(DL_LOG_DEBUG, "io error 1 %d\n", e);
			microsleep(1000);
			goto fail;
		}

		handle->counter = opt_timeout;
		if (ldl_test_flag(handle, LDL_FLAG_TLS)) {
			launch_queue_thread(handle);
		}

		while (ldl_test_flag(handle, LDL_FLAG_RUNNING)) {
			e = iks_recv(handle->parser, 1);
			if (!ldl_test_flag(handle, LDL_FLAG_TLS) && handle->loop_callback) {
				if (handle->loop_callback(handle) != LDL_STATUS_SUCCESS) {
					ldl_clear_flag_locked(handle, LDL_FLAG_RUNNING);	
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
				globals.logger(DL_LOG_DEBUG, "io error 2 %d\n", e);
				microsleep(1000);
				goto fail;
			}

			
			if (!ldl_test_flag(handle, LDL_FLAG_TLS) && ldl_test_flag(handle, LDL_FLAG_READY)) {
				ldl_flush_queue(handle, 0);
			}

			if (!ldl_test_flag(handle, LDL_FLAG_CONNECTED)) {
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

	fail:
		iks_disconnect(handle->parser);
		iks_parser_delete(handle->parser);
		ldl_clear_flag_locked(handle, LDL_FLAG_CONNECTED);
		ldl_clear_flag_locked(handle, LDL_FLAG_AUTHORIZED);
		handle->state = CS_NEW;
	}
	ldl_clear_flag_locked(handle, LDL_FLAG_RUNNING);
	if (!ldl_test_flag(handle, LDL_FLAG_TLS)) {
		ldl_flush_queue(handle, 1);
	}
	
	while(ldl_test_flag(handle, LDL_FLAG_QUEUE_RUNNING)) {
		microsleep(100);
	}


}

static void add_elements(ldl_session_t *session, iks *tag)
{
	apr_hash_index_t *hi;

	for (hi = apr_hash_first(session->pool, session->variables); hi; hi = apr_hash_next(hi)) {
		void *val = NULL;
		const void *key = NULL;

		apr_hash_this(hi, &key, NULL, &val);
		if (val) {
			iks *var = iks_insert(tag, "info_element");
			iks_insert_attrib(var, "xmlns", "http://www.freeswitch.org/jie");
			iks_insert_attrib(var, "name", (char *) key);
			iks_insert_attrib(var, "value", (char *) val);
		}
	}
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
	iks_insert_attrib(iq, "from", session->login);
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
		apr_hash_set(handle->retry_hash, packet_node->id, APR_HASH_KEY_STRING, packet_node);
		apr_queue_push(handle->retry_queue, packet_node);
		packet_node = NULL;
	}
	apr_thread_mutex_unlock(handle->lock);

}

char *ldl_session_get_caller(ldl_session_t *session)
{
	return session->them;
}

char *ldl_session_get_callee(ldl_session_t *session)
{
	return session->login;
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

void ldl_session_accept_candidate(ldl_session_t *session, ldl_candidate_t *candidate)
{
	iks *iq, *sess, *tp;
	unsigned int myid;
    char idbuf[80];
	myid = next_id();
    snprintf(idbuf, sizeof(idbuf), "%u", myid);

	if ((iq = iks_new("iq"))) {
		if (!iks_insert_attrib(iq, "type", "set")) goto fail;
		if (!iks_insert_attrib(iq, "id", idbuf)) goto fail;
		if (!iks_insert_attrib(iq, "from", session->login)) goto fail;
		if (!iks_insert_attrib(iq, "to", session->them)) goto fail;
		if (!(sess = iks_insert (iq, "session"))) goto fail;
		if (!iks_insert_attrib(sess, "xmlns", "http://www.google.com/session")) goto fail;
		if (!iks_insert_attrib(sess, "type", "transport-accept")) goto fail;
		if (!iks_insert_attrib(sess, "id", candidate->tid)) goto fail;
		if (!iks_insert_attrib(sess, "xmlns", "http://www.google.com/session")) goto fail;
		if (!iks_insert_attrib(sess, "initiator", session->initiator ? session->initiator : session->them)) goto fail;
		if (!(tp = iks_insert (sess, "transport"))) goto fail;
		if (!iks_insert_attrib(tp, "xmlns", "http://www.google.com/transport/p2p")) goto fail;
		apr_queue_push(session->handle->queue, iq);
		iq = NULL;
	}

 fail:
	if (iq) {
		iks_delete(iq);
	}

}

void *ldl_handle_get_private(ldl_handle_t *handle)
{
	return handle->private_info;
}

char *ldl_handle_get_login(ldl_handle_t *handle)
{
	return handle->login;
}

void ldl_handle_send_presence(ldl_handle_t *handle, char *from, char *to, char *type, char *rpid, char *message, char *avatar)
{
	do_presence(handle, from, to, type, rpid, message, avatar);
}

static void ldl_random_string(char *buf, uint16_t len, char *set)
{
    char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int max;
    uint16_t x;

    if (!set) {
        set = chars;
    }

    max = (int) strlen(set);

    srand((unsigned int) time(NULL));

    for (x = 0; x < len; x++) {
        int j = (int) (max * 1.0 * rand() / (RAND_MAX + 1.0));
        buf[x] = set[j];
    }
}

#define TLEN 8192
void ldl_handle_send_vcard(ldl_handle_t *handle, char *from, char *to, char *id, char *vcard)
{
	iks *vxml = NULL, *iq = NULL;
	int e = 0;
	ldl_avatar_t *ap = NULL;
	char *text = NULL;

	ap = ldl_get_avatar(handle, NULL, from);

	if (!vcard) {
		char *ext;

		if (!ap) {
			return;
		}
		
		if ((ext = strrchr(ap->path, '.'))) {
			ext++;
		} else {
			ext = "png";
		}
		text = malloc(TLEN);
		snprintf(text, TLEN,
				 "<vCard xmlns='vcard-temp'><PHOTO><TYPE>image/%s</TYPE><BINVAL>%s</BINVAL></PHOTO></vCard>",
				 ext,
				 ap->base64
				 );
		vcard = text;
	} else {
		if (ap && (strstr(vcard, "photo") || strstr(vcard, "PHOTO"))) {
			ldl_random_string(ap->hash, sizeof(ap->hash) -1, NULL);
		}
	}


	if (!(vxml = iks_tree(vcard, 0, &e))) {
		globals.logger(DL_LOG_ERR, "Parse returned error [%d]\n", e);
		goto fail;
	}
	
	if (!(iq = iks_new("iq"))) {
		globals.logger(DL_LOG_ERR, "Memory Error\n");
		goto fail;
	}

	if (!iks_insert_attrib(iq, "to", to)) goto fail;
	if (!iks_insert_attrib(iq, "xmlns", "jabber:client")) goto fail;
	if (!iks_insert_attrib(iq,"from", from)) goto fail;
	if (!iks_insert_attrib(iq, "type", "result")) goto fail;
	if (!iks_insert_attrib(iq, "id", id)) goto fail;
	if (!iks_insert_node(iq, vxml)) goto fail;

	apr_queue_push(handle->queue, iq);

	iq = NULL;
	vxml = NULL;

 fail:

	if (iq) {
		iks_delete(iq);
	}

	if (vxml) {
		iks_delete(vxml);
	}

	if (text) {
		free(text);
	}

}

void ldl_handle_send_msg(ldl_handle_t *handle, char *from, char *to, const char *subject, const char *body)
{
	iks *msg;
	char *t, *e;
	char *bdup = NULL;
	int on = 0;
	int len = 0;
	char *my_body = strdup(body);
	assert(handle != NULL);
	assert(body != NULL);
	
	if (strchr(my_body, '<')) {
		len = (int) strlen(my_body);
		if (!(bdup = malloc(len))) {
			return;
		}

		memset(bdup, 0, len);
		
		e = bdup;
		for(t = my_body; *t; t++) {
			if (*t == '<') {
				on = 1;
			} else if (*t == '>') {
				t++;
				on = 0;
			}
			
			if (!on) {
				*e++ = *t;
			}
		}
		my_body = bdup;
	}
	
	msg = iks_make_msg(IKS_TYPE_NONE, to, my_body);
	iks_insert_attrib(msg, "type", "chat");

	if (!from) {
		from = handle->login;
	}

	iks_insert_attrib(msg, "from", from);

	if (subject) {
		iks_insert_attrib(msg, "subject", subject);
	}

	if (bdup) {	
		free(bdup);
	}

	free(my_body);

	apr_queue_push(handle->queue, msg);
	msg = NULL;
	
}

int ldl_global_debug(int on)
{
	if (on > -1) {
		globals.debug = on ? 1 : 0;
	}

	return globals.debug ? 1 : 0;
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
	unsigned int x, id = 0;


	for (x = 0; x < clen; x++) {
		char buf[512];
		iq = NULL;
		sess = NULL;
		id = 0;
		
		new_session_iq(session, &iq, &sess, &id, "transport-info");
		tag = iks_insert(sess, "transport");
		iks_insert_attrib(tag, "xmlns", "http://www.google.com/transport/p2p");

		add_elements(session, tag);
		tag = iks_insert(tag, "candidate");

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
		schedule_packet(session->handle, id, iq, LDL_RETRY);
	}


	return id;
}

char *ldl_handle_probe(ldl_handle_t *handle, char *id, char *from, char *buf, unsigned int len)
{
	iks *pres, *msg;
	char *lid = NULL, *low_id = NULL;
	struct ldl_buffer buffer;
	time_t started, elapsed, next = 0;
	char *notice = "Call Me!";
	
	buffer.buf = buf;
	buffer.len = len;
	buffer.hit = 0;

	apr_hash_set(handle->probe_hash, id, APR_HASH_KEY_STRING, &buffer);

	started = time(NULL);
	for(;;) {
		elapsed = time(NULL) - started;
		if (elapsed == next) {
			msg = iks_make_s10n (IKS_TYPE_SUBSCRIBE, id, notice); 
			iks_insert_attrib(msg, "from", from);
			apr_queue_push(handle->queue, msg);
			msg = NULL;
			
			pres = iks_new("presence");
			iks_insert_attrib(pres, "xmlns", "jabber:client");
			iks_insert_attrib(pres, "type", "probe");
			iks_insert_attrib(pres, "to", id);
			iks_insert_attrib(pres, "from", from);
			apr_queue_push(handle->queue, pres);
			pres = NULL;
			next += 5;
		}
		if (elapsed >= 17) {
			break;
		}
		if (buffer.hit) {
			lid = buffer.buf;
			break;
		}
		ldl_yield(1000);
	}

	if ((low_id = strdup(id))) {
		lowercase(id);
		apr_hash_set(handle->probe_hash, low_id, APR_HASH_KEY_STRING, NULL);
		free(low_id);
	}
	
	return lid;
}


char *ldl_handle_disco(ldl_handle_t *handle, char *id, char *from, char *buf, unsigned int len)
{
	iks *iq, *query, *msg;
	char *lid = NULL;
	struct ldl_buffer buffer;
	apr_time_t started;
	unsigned int elapsed;
	char *notice = "Call Me!";
	int again = 0;
	unsigned int myid;
    char idbuf[80];

	myid = next_id();
    snprintf(idbuf, sizeof(idbuf), "%u", myid);

	buffer.buf = buf;
	buffer.len = len;
	buffer.hit = 0;

	if ((iq = iks_new("iq"))) {
		if ((query = iks_insert(iq, "query"))) {
			iks_insert_attrib(iq, "type", "get");
			iks_insert_attrib(iq, "to", id);
			iks_insert_attrib(iq,"from", from);
			iks_insert_attrib(iq, "id", idbuf);
			iks_insert_attrib(query, "xmlns", "http://jabber.org/protocol/disco#info");
		} else {
			iks_delete(iq);
			globals.logger(DL_LOG_DEBUG, "Memory ERROR!\n");
			return NULL;
		}
	} else {
		globals.logger(DL_LOG_DEBUG, "Memory ERROR!\n");
		return NULL;
	}
	
	apr_hash_set(handle->probe_hash, id, APR_HASH_KEY_STRING, &buffer);
	msg = iks_make_s10n (IKS_TYPE_SUBSCRIBE, id, notice); 
	apr_queue_push(handle->queue, msg);
	msg = NULL;
	msg = iks_make_s10n (IKS_TYPE_SUBSCRIBED, id, notice); 
	apr_queue_push(handle->queue, msg);
	msg = NULL;
	apr_queue_push(handle->queue, iq);
	iq = NULL;

	//schedule_packet(handle, next_id(), pres, LDL_RETRY);

	started = apr_time_now();
	for(;;) {
		elapsed = (unsigned int)((apr_time_now() - started) / 1000);
		if (elapsed > 5000 && ! again) {
			msg = iks_make_s10n (IKS_TYPE_SUBSCRIBE, id, notice); 
			apr_queue_push(handle->queue, msg);
			msg = NULL;
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
	iks *iq, *sess, *tag, *payload, *tp;
	unsigned int x, id;
	

	new_session_iq(session, &iq, &sess, &id, description == LDL_DESCRIPTION_ACCEPT ? "accept" : "initiate");
	tag = iks_insert(sess, "description");
	iks_insert_attrib(tag, "xmlns", "http://www.google.com/session/phone");
	iks_insert_attrib(tag, "xml:lang", "en");
	for (x = 0; x < plen; x++) {
		char idbuf[80];
		payload = iks_insert(tag, "payload-type");
		iks_insert_attrib(payload, "xmlns", "http://www.google.com/session/phone");

		sprintf(idbuf, "%d", payloads[x].id);
		iks_insert_attrib(payload, "id", idbuf);
		iks_insert_attrib(payload, "name", payloads[x].name);
		if (payloads[x].rate) {
			sprintf(idbuf, "%d", payloads[x].rate);
			iks_insert_attrib(payload, "clockrate", idbuf);
		}
		if (payloads[x].bps) {
			sprintf(idbuf, "%d", payloads[x].bps);
			iks_insert_attrib(payload, "bitrate", idbuf);
		}
	}

	if (description == LDL_DESCRIPTION_INITIATE) {
		tp = iks_insert (sess, "transport");
		iks_insert_attrib(tp, "xmlns", "http://www.google.com/transport/p2p");
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

	apr_thread_mutex_create(&globals.flag_mutex, APR_THREAD_MUTEX_NESTED, globals.memory_pool);
	globals.log_stream = stdout;
	globals.debug = debug;
	globals.id = 300;
	globals.logger = default_logger;
	globals.avatar_hash = apr_hash_make(globals.memory_pool);
	ldl_set_flag_locked((&globals), LDL_FLAG_INIT);
	
	return LDL_STATUS_SUCCESS;
}

ldl_status ldl_global_destroy(void)
{
	if (!ldl_test_flag(&globals, LDL_FLAG_INIT)) {
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
	return (int8_t)ldl_test_flag(handle, LDL_FLAG_READY);
}

ldl_status ldl_handle_init(ldl_handle_t **handle,
						   char *login,
						   char *password,
						   char *server,
						   ldl_user_flag_t flags,
						   char *status_msg,
						   ldl_loop_callback_t loop_callback,
						   ldl_session_callback_t session_callback,
						   ldl_response_callback_t response_callback,
						   void *private_info)
{
	apr_pool_t *pool;
	assert(ldl_test_flag(&globals, LDL_FLAG_INIT));
	*handle = NULL;	

	if ((apr_pool_create(&pool, globals.memory_pool)) != LDL_STATUS_SUCCESS) {
		return LDL_STATUS_MEMERR;
	}

	if (!login) {
		globals.logger(DL_LOG_ERR, "No login supplied!\n");
		return LDL_STATUS_FALSE;
	}

	if (!password) {
		globals.logger(DL_LOG_ERR, "No password supplied!\n");
		return LDL_STATUS_FALSE;
	}
	

	if ((*handle = apr_palloc(pool, sizeof(ldl_handle_t)))) {
		ldl_handle_t *new_handle = *handle;
		memset(new_handle, 0, sizeof(ldl_handle_t));
		new_handle->log_stream = globals.log_stream;
		new_handle->login = apr_pstrdup(pool, login);
		new_handle->password = apr_pstrdup(pool, password);

		if (server) {
			char *p;

			new_handle->server = apr_pstrdup(pool, server);
			if ((p = strchr(new_handle->server, ':'))) {
				*p++ = '\0';
				new_handle->port = (uint16_t)atoi(p);
			}
		}

		if (status_msg) {
			new_handle->status_msg = apr_pstrdup(pool, status_msg);
		}

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
		new_handle->flags |= flags;
		apr_queue_create(&new_handle->queue, LDL_HANDLE_QLEN, new_handle->pool);
		apr_queue_create(&new_handle->retry_queue, LDL_HANDLE_QLEN, new_handle->pool);
		new_handle->features |= IKS_STREAM_BIND|IKS_STREAM_SESSION;

		if (new_handle->flags & LDL_FLAG_SASL_PLAIN) {
			new_handle->features |= IKS_STREAM_SASL_PLAIN;
		} else if (new_handle->flags & LDL_FLAG_SASL_MD5) {
			new_handle->features |= IKS_STREAM_SASL_MD5;
		}

		new_handle->sessions = apr_hash_make(new_handle->pool);
		new_handle->retry_hash = apr_hash_make(new_handle->pool);
		new_handle->probe_hash = apr_hash_make(new_handle->pool);
		new_handle->sub_hash = apr_hash_make(new_handle->pool);
		apr_thread_mutex_create(&new_handle->lock, APR_THREAD_MUTEX_NESTED, new_handle->pool);
		apr_thread_mutex_create(&new_handle->flag_mutex, APR_THREAD_MUTEX_NESTED, new_handle->pool);

		return LDL_STATUS_SUCCESS;
	} 
	
	return LDL_STATUS_FALSE;
}

void ldl_handle_run(ldl_handle_t *handle)
{
	ldl_set_flag_locked(handle, LDL_FLAG_RUNNING);
	xmpp_connect(handle, handle->login, handle->password);
	ldl_clear_flag_locked(handle, LDL_FLAG_RUNNING);
}

void ldl_handle_stop(ldl_handle_t *handle)
{
	ldl_clear_flag_locked(handle, LDL_FLAG_RUNNING);
}

ldl_status ldl_handle_destroy(ldl_handle_t **handle)
{
	apr_pool_t *pool = (*handle)->pool;

	ldl_flush_queue(*handle, 1);
	

	apr_pool_destroy(pool);
	*handle = NULL;
	return LDL_STATUS_SUCCESS;
}


void ldl_handle_set_log_stream(ldl_handle_t *handle, FILE *log_stream)
{
	assert(ldl_test_flag(&globals, LDL_FLAG_INIT));

	handle->log_stream = log_stream;
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
