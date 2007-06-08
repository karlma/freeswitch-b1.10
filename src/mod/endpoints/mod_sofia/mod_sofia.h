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
 * Marcel Barbulescu <marcelbarbulescu@gmail.com>
 *
 *
 * mod_sofia.h -- SOFIA SIP Endpoint
 *
 */

/*Defines etc..*/
/*************************************************************************************************************************************************************/
#define IREG_SECONDS 30
#define GATEWAY_SECONDS 1

#define HAVE_APR
#include <switch.h>
#ifdef SWITCH_HAVE_ODBC
#include <switch_odbc.h>
#endif

static const char modname[] = "mod_sofia";
static const switch_state_handler_table_t noop_state_handler = { 0 };
struct sofia_gateway;
typedef struct sofia_gateway sofia_gateway_t;

struct sofia_profile;
typedef struct sofia_profile sofia_profile_t;
#define NUA_MAGIC_T sofia_profile_t

typedef struct sofia_private sofia_private_t;

struct private_object;
typedef struct private_object private_object_t;
#define NUA_HMAGIC_T sofia_private_t

#define MY_EVENT_REGISTER "sofia::register"
#define MY_EVENT_EXPIRE "sofia::expire"
#define MULTICAST_EVENT "multicast::event"
#define SOFIA_REPLACES_HEADER "_sofia_replaces_"
#define SOFIA_USER_AGENT "FreeSWITCH(mod_sofia)"
#define SOFIA_CHAT_PROTO "sip"
#define SOFIA_SIP_HEADER_PREFIX "sip_h_"
#define SOFIA_SIP_HEADER_PREFIX_T "~sip_h_"
#define SOFIA_DEFAULT_PORT "5060"

#include <sofia-sip/nua.h>
#include <sofia-sip/sip_status.h>
#include <sofia-sip/sdp.h>
#include <sofia-sip/sip_protos.h>
#include <sofia-sip/auth_module.h>
#include <sofia-sip/su_md5.h>
#include <sofia-sip/su_log.h>
#include <sofia-sip/nea.h>
#include <sofia-sip/msg_addr.h>

struct sofia_private {
	char uuid[SWITCH_UUID_FORMATTED_LENGTH + 1];
	sofia_gateway_t *gateway;
	su_home_t *home;
};



#define set_param(ptr,val) if (ptr) {free(ptr) ; ptr = NULL;} if (val) {ptr = strdup(val);}
#define set_anchor(t,m) if (t->Anchor) {delete t->Anchor;} t->Anchor = new SipMessage(m);

/* Local Structures */
/*************************************************************************************************************************************************************/
struct sip_alias_node {
	char *url;
	nua_t *nua;
	struct sip_alias_node *next;
};

typedef struct sip_alias_node sip_alias_node_t;

typedef enum {
	PFLAG_AUTH_CALLS = (1 << 0),
	PFLAG_BLIND_REG = (1 << 1),
	PFLAG_AUTH_ALL = (1 << 2),
	PFLAG_FULL_ID = (1 << 3),
	PFLAG_PRESENCE = (1 << 4),
	PFLAG_PASS_RFC2833 = (1 << 5),
	PFLAG_DISABLE_TRANSCODING = (1 << 6),
	PFLAG_REWRITE_TIMESTAMPS = (1 << 7),
	PFLAG_RUNNING = (1 << 8),
	PFLAG_RESPAWN = (1 << 9)
} PFLAGS;


typedef enum {
	PFLAG_NDLB_TO_IN_200_CONTACT = (1 << 1)
} sofia_NDLB_t;

typedef enum {
	TFLAG_IO = (1 << 0),
	TFLAG_CHANGE_MEDIA = (1 << 1),
	TFLAG_OUTBOUND = (1 << 2),
	TFLAG_READING = (1 << 3),
	TFLAG_WRITING = (1 << 4),
	TFLAG_HUP = (1 << 5),
	TFLAG_RTP = (1 << 6),
	TFLAG_BYE = (1 << 7),
	TFLAG_ANS = (1 << 8),
	TFLAG_EARLY_MEDIA = (1 << 9),
	TFLAG_SECURE = (1 << 10),
	TFLAG_VAD_IN = (1 << 11),
	TFLAG_VAD_OUT = (1 << 12),
	TFLAG_VAD = (1 << 13),
	TFLAG_TIMER = (1 << 14),
	TFLAG_READY = (1 << 15),
	TFLAG_REINVITE = (1 << 16),
	TFLAG_REFER = (1 << 17),
	TFLAG_NOHUP = (1 << 18),
	TFLAG_XFER = (1 << 19),
	TFLAG_RESERVED = (1 << 20),
	TFLAG_BUGGY_2833 = (1 << 21),
	TFLAG_SIP_HOLD = (1 << 22),
	TFLAG_INB_NOMEDIA = (1 << 23),
	TFLAG_LATE_NEGOTIATION = (1 << 24),
	TFLAG_SDP = (1 << 25),
	TFLAG_VIDEO = (1 << 26)
} TFLAGS;

struct mod_sofia_globals {
	switch_hash_t *profile_hash;
	switch_hash_t *gateway_hash;
	switch_mutex_t *hash_mutex;
	uint32_t callid;
	int32_t running;
	int32_t threads;
	switch_mutex_t *mutex;
	char guess_ip[80];
};
extern struct mod_sofia_globals mod_sofia_globals;


typedef enum {
	REG_FLAG_AUTHED = (1 << 0),
	REG_FLAG_CALLERID = (1 << 1)
} reg_flags_t;

typedef enum {
	REG_STATE_UNREGED,
	REG_STATE_TRYING,
	REG_STATE_REGISTER,
	REG_STATE_REGED,
	REG_STATE_FAILED,
	REG_STATE_EXPIRED,
	REG_STATE_NOREG,
	REG_STATE_LAST
} reg_state_t;

struct sofia_gateway {
	sofia_private_t *sofia_private;
	nua_handle_t *nh;
	sofia_profile_t *profile;
	char *name;
	char *register_scheme;
	char *register_realm;
	char *register_username;
	char *register_password;
	char *register_from;
	char *register_contact;
	char *register_to;
	char *register_proxy;
	char *register_context;
	char *expires_str;
	uint32_t freq;
	time_t expires;
	time_t retry;
	uint32_t flags;
	int32_t retry_seconds;
	reg_state_t state;
	switch_memory_pool_t *pool;
	struct sofia_gateway *next;
};


struct sofia_profile {
	int debug;
	char *name;
	char *dbname;
	char *dialplan;
	char *context;
	char *extrtpip;
	char *rtpip;
	char *sipip;
	char *extsipip;
	char *username;
	char *url;
	char *bindurl;
	char *sipdomain;
	char *timer_name;
	char *hold_music;
	char *bind_params;
	int sip_port;
	char *codec_string;
	int running;
	int codec_ms;
	int dtmf_duration;
	unsigned int flags;
	unsigned int pflags;
	unsigned int ndlb;
	uint32_t max_calls;
	uint32_t nonce_ttl;
	nua_t *nua;
	switch_memory_pool_t *pool;
	su_root_t *s_root;
	sip_alias_node_t *aliases;
	switch_payload_t te;
	switch_payload_t cng_pt;
	uint32_t codec_flags;
	switch_mutex_t *ireg_mutex;
	switch_mutex_t *gateway_mutex;
	sofia_gateway_t *gateways;
	su_home_t *home;
	switch_hash_t *profile_hash;
	switch_hash_t *chat_hash;
	switch_core_db_t *master_db;
	switch_thread_rwlock_t *rwlock;
	switch_mutex_t *flag_mutex;
	uint32_t inuse;
	uint32_t soft_max;
	time_t started;
#ifdef SWITCH_HAVE_ODBC
	char *odbc_dsn;
	char *odbc_user;
	char *odbc_pass;
	switch_odbc_handle_t *master_odbc;
#else
	void *filler1;
	void *filler2;
	void *filler3;
	void *filler4;
#endif
};


struct private_object {
	sofia_private_t *sofia_private;
	uint32_t flags;
	switch_payload_t agreed_pt;
	switch_core_session_t *session;
	switch_channel_t *channel;
	switch_frame_t read_frame;
	char *codec_order[SWITCH_MAX_CODECS];
	int codec_order_last;
	const switch_codec_implementation_t *codecs[SWITCH_MAX_CODECS];
	int num_codecs;
	switch_codec_t read_codec;
	switch_codec_t write_codec;
	uint32_t codec_ms;
	switch_caller_profile_t *caller_profile;
	uint32_t timestamp_send;
	//int32_t timestamp_recv;
	switch_rtp_t *rtp_session;
	int ssrc;
	//switch_time_t last_read;
	sofia_profile_t *profile;
	char *local_sdp_audio_ip;
	switch_port_t local_sdp_audio_port;
	char *remote_sdp_audio_ip;
	switch_port_t remote_sdp_audio_port;
	char *adv_sdp_audio_ip;
	switch_port_t adv_sdp_audio_port;
	char *proxy_sdp_audio_ip;
	switch_port_t proxy_sdp_audio_port;
	char *reply_contact;
	char *from_uri;
	char *to_uri;
	char *from_address;
	char *to_address;
	char *callid;
	char *far_end_contact;
	char *contact_url;
	char *from_str;
	char *gateway_from_str;
	char *rm_encoding;
	char *rm_fmtp;
	char *fmtp_out;
	char *remote_sdp_str;
	char *local_sdp_str;
	char *dest;
	char *dest_to;
	char *key;
	char *xferto;
	char *kick;
	char *origin;
	char *hash_key;
	char *chat_from;
	char *chat_to;
	char *e_dest;
	char *call_id;
	char *invite_contact;
	unsigned long rm_rate;
	switch_payload_t pt;
	switch_mutex_t *flag_mutex;
	switch_payload_t te;
	switch_payload_t bte;
	switch_payload_t cng_pt;
	switch_payload_t bcng_pt;
	nua_handle_t *nh;
	nua_handle_t *nh2;
	sip_contact_t *contact;
	uint32_t owner_id;
	uint32_t session_id;
	/** VIDEO **/
	switch_frame_t video_read_frame;
	switch_codec_t video_read_codec;
	switch_codec_t video_write_codec;
	switch_rtp_t *video_rtp_session;
	switch_port_t adv_sdp_video_port;
	switch_port_t local_sdp_video_port;
	char *video_rm_encoding;
	switch_payload_t video_pt;
	unsigned long video_rm_rate;
	uint32_t video_codec_ms;
	char *remote_sdp_video_ip;
	switch_port_t remote_sdp_video_port;
	char *video_rm_fmtp;
	switch_payload_t video_agreed_pt;
	char *video_fmtp_out;
	uint32_t video_count;
};

struct callback_t {
	char *val;
	switch_size_t len;
	int matches;
};

typedef enum {
	REG_REGISTER,
	REG_INVITE
} sofia_regtype_t;

typedef enum {
	AUTH_OK,
	AUTH_FORBIDDEN,
	AUTH_STALE,
} auth_res_t;


#define sofia_test_pflag(obj, flag) ((obj)->pflags & flag)
#define sofia_set_pflag(obj, flag) (obj)->pflags |= (flag)
#define sofia_set_pflag_locked(obj, flag) assert(obj->flag_mutex != NULL);\
switch_mutex_lock(obj->flag_mutex);\
(obj)->pflags |= (flag);\
switch_mutex_unlock(obj->flag_mutex);
#define sofia_clear_pflag_locked(obj, flag) switch_mutex_lock(obj->flag_mutex); (obj)->pflags &= ~(flag); switch_mutex_unlock(obj->flag_mutex);
#define sofia_clear_pflag(obj, flag) (obj)->pflags &= ~(flag)
#define sofia_copy_pflags(dest, src, flags) (dest)->pflags &= ~(flags);	(dest)->pflags |= ((src)->pflags & (flags))

/* Function Prototypes */
/*************************************************************************************************************************************************************/


switch_status_t sofia_glue_activate_rtp(private_object_t *tech_pvt);

void sofia_glue_deactivate_rtp(private_object_t *tech_pvt);

void sofia_glue_set_local_sdp(private_object_t *tech_pvt, char *ip, uint32_t port, char *sr, int force);

void sofia_glue_tech_prepare_codecs(private_object_t *tech_pvt);

void sofia_glue_attach_private(switch_core_session_t *session, sofia_profile_t *profile, private_object_t *tech_pvt, const char *channame);

switch_status_t sofia_glue_tech_choose_port(private_object_t *tech_pvt);

switch_status_t sofia_glue_do_invite(switch_core_session_t *session);

uint8_t sofia_glue_negotiate_sdp(switch_core_session_t *session, sdp_session_t *sdp);

void sofia_presence_establish_presence(sofia_profile_t *profile);

void sofia_handle_sip_i_refer(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, switch_core_session_t *session, sip_t const *sip, tagi_t tags[]);

void sofia_handle_sip_i_info(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, switch_core_session_t *session, sip_t const *sip, tagi_t tags[]);

void sofia_handle_sip_i_invite(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);

void sofia_reg_handle_sip_i_register(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);

void sofia_event_callback(nua_event_t event,
					int status,
					char const *phrase,
					nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);


void *SWITCH_THREAD_FUNC sofia_profile_thread_run(switch_thread_t *thread, void *obj);

void launch_sofia_profile_thread(sofia_profile_t *profile);

switch_status_t sofia_presence_chat_send(char *proto, char *from, char *to, char *subject, char *body, char *hint);
void sofia_glue_tech_absorb_sdp(private_object_t *tech_pvt);
switch_status_t sofia_glue_tech_media(private_object_t *tech_pvt, char *r_sdp);
char *sofia_reg_find_reg_url(sofia_profile_t *profile, const char *user, const char *host, char *val, switch_size_t len);
void event_handler(switch_event_t *event);
void sofia_presence_event_handler(switch_event_t *event);
void sofia_presence_mwi_event_handler(switch_event_t *event);
void sofia_presence_cancel(void);
switch_status_t config_sofia(int reload, char *profile_name);
void sofia_reg_auth_challange(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_regtype_t regtype, const char *realm, int stale);
auth_res_t sofia_reg_parse_auth(sofia_profile_t *profile, sip_authorization_t const *authorization, 
								const char *regstr, char *np, size_t nplen, char *ip, switch_event_t **v_event);
void sofia_reg_handle_sip_r_challenge(int status,
					 char const *phrase,
					 nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, switch_core_session_t *session, sip_t const *sip, tagi_t tags[]);
void sofia_reg_handle_sip_r_register(int status,
					char const *phrase,
					nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);
void sofia_handle_sip_i_options(int status,
				   char const *phrase,
				   nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);
void sofia_presence_handle_sip_i_publish(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);
void sofia_presence_handle_sip_i_message(int status,
				   char const *phrase,
				   nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);
void sofia_presence_handle_sip_r_subscribe(int status,
					 char const *phrase,
					 nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);
void sofia_presence_handle_sip_i_subscribe(int status,
					 char const *phrase,
					 nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sofia_private_t *sofia_private, sip_t const *sip, tagi_t tags[]);


void sofia_glue_execute_sql(sofia_profile_t *profile, switch_bool_t master, char *sql, switch_mutex_t *mutex);
void sofia_reg_check_expire(sofia_profile_t *profile, time_t now);
void sofia_reg_check_gateway(sofia_profile_t *profile, time_t now);
void sofia_reg_unregister(sofia_profile_t *profile);
switch_status_t sofia_glue_ext_address_lookup(char **ip, switch_port_t *port, char *sourceip, switch_memory_pool_t *pool);

void sofia_glue_pass_sdp(private_object_t *tech_pvt, char *sdp);
int sofia_glue_get_user_host(char *in, char **user, char **host);
switch_call_cause_t sofia_glue_sip_cause_to_freeswitch(int status);
void sofia_glue_do_xfer_invite(switch_core_session_t *session);
uint8_t sofia_reg_handle_register(nua_t *nua, sofia_profile_t *profile, nua_handle_t *nh, sip_t const *sip, 
								  sofia_regtype_t regtype, char *key, uint32_t keylen, switch_event_t **v_event);
extern const switch_endpoint_interface_t sofia_endpoint_interface;
void sofia_presence_set_chat_hash(private_object_t *tech_pvt, sip_t const *sip);
switch_status_t sofia_on_hangup(switch_core_session_t *session);
char *sofia_glue_get_url_from_contact(char *buf, uint8_t to_dup);
void sofia_presence_set_hash_key(char *hash_key, int32_t len, sip_t const *sip);
void sofia_glue_sql_close(sofia_profile_t *profile);
int sofia_glue_init_sql(sofia_profile_t *profile);
switch_bool_t sofia_glue_execute_sql_callback(sofia_profile_t *profile,
											  switch_bool_t master,
											  switch_mutex_t *mutex,
											  char *sql,
											  switch_core_db_callback_func_t callback,
											  void *pdata);
char *sofia_glue_execute_sql2str(sofia_profile_t *profile, switch_mutex_t *mutex, char *sql, char *resbuf, size_t len);
void sofia_glue_check_video_codecs(private_object_t *tech_pvt);
void sofia_glue_del_profile(sofia_profile_t *profile);

switch_status_t sofia_glue_add_profile(char *key, sofia_profile_t *profile);
void sofia_glue_release_profile__(const char *file, const char *func, int line, sofia_profile_t *profile);
#define sofia_glue_release_profile(x) sofia_glue_release_profile__(__FILE__, __SWITCH_FUNC__, __LINE__,  x)

sofia_profile_t *sofia_glue_find_profile__(const char *file, const char *func, int line, char *key);
#define sofia_glue_find_profile(x) sofia_glue_find_profile__(__FILE__, __SWITCH_FUNC__, __LINE__,  x)


switch_status_t sofia_reg_add_gateway(char *key, sofia_gateway_t *gateway);
sofia_gateway_t *sofia_reg_find_gateway__(const char *file, const char *func, int line, char *key);
#define sofia_reg_find_gateway(x) sofia_reg_find_gateway__(__FILE__, __SWITCH_FUNC__, __LINE__,  x)


void sofia_reg_release_gateway__(const char *file, const char *func, int line, sofia_gateway_t *gateway);
#define sofia_reg_release_gateway(x) sofia_reg_release_gateway__(__FILE__, __SWITCH_FUNC__, __LINE__, x);
