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
 * mod_openzap.c -- OPENZAP Endpoint Module
 *
 */
#include <switch.h>
#include "openzap.h"

#ifndef __FUNCTION__
#define __FUNCTION__ __SWITCH_FUNC__
#endif

#define OPENZAP_VAR_PREFIX "openzap_"
#define OPENZAP_VAR_PREFIX_LEN 8

SWITCH_MODULE_LOAD_FUNCTION(mod_openzap_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_openzap_shutdown);
SWITCH_MODULE_DEFINITION(mod_openzap, mod_openzap_load, mod_openzap_shutdown, NULL);

switch_endpoint_interface_t *openzap_endpoint_interface;

static switch_memory_pool_t *module_pool = NULL;

typedef enum {
	ANALOG_OPTION_NONE = 0,
	ANALOG_OPTION_3WAY = (1 << 0),
	ANALOG_OPTION_CALL_SWAP = (1 << 1)
} analog_option_t;

struct span_config {
	zap_span_t *span;
	char dialplan[80];
	char context[80];
	char dial_regex[256];
	char fail_dial_regex[256];
	char hold_music[256];
	char type[256];
	analog_option_t analog_options;
};

static struct span_config SPAN_CONFIG[ZAP_MAX_SPANS_INTERFACE] = {{0}};

typedef enum {
	TFLAG_IO = (1 << 0),
	TFLAG_DTMF = (1 << 1),
	TFLAG_CODEC = (1 << 2),
	TFLAG_BREAK = (1 << 3),
	TFLAG_HOLD = (1 << 4),
	TFLAG_DEAD = (1 << 5)
} TFLAGS;

static struct {
	int debug;
	char *dialplan;
	char *codec_string;
	char *codec_order[SWITCH_MAX_CODECS];
	int codec_order_last;
	char *codec_rates_string;
	char *codec_rates[SWITCH_MAX_CODECS];
	int codec_rates_last;
	unsigned int flags;
	int fd;
	int calls;
	char hold_music[256];
	switch_mutex_t *mutex;
	analog_option_t analog_options;
} globals;

struct private_object {
	unsigned int flags;
	switch_codec_t read_codec;
	switch_codec_t write_codec;
	switch_frame_t read_frame;
	unsigned char databuf[SWITCH_RECOMMENDED_BUFFER_SIZE];
	switch_frame_t cng_frame;
	unsigned char cng_databuf[SWITCH_RECOMMENDED_BUFFER_SIZE];
	switch_core_session_t *session;
	switch_caller_profile_t *caller_profile;
	unsigned int codec;
	unsigned int codecs;
	unsigned short samprate;
	switch_mutex_t *mutex;
	switch_mutex_t *flag_mutex;
	zap_channel_t *zchan;
	uint32_t wr_error;
};

typedef struct private_object private_t;


static switch_status_t channel_on_init(switch_core_session_t *session);
static switch_status_t channel_on_hangup(switch_core_session_t *session);
static switch_status_t channel_on_routing(switch_core_session_t *session);
static switch_status_t channel_on_exchange_media(switch_core_session_t *session);
static switch_status_t channel_on_soft_execute(switch_core_session_t *session);
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, 
													switch_memory_pool_t **pool,
													switch_originate_flag_t flags);
static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id);
static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id);
static switch_status_t channel_kill_channel(switch_core_session_t *session, int sig);


static switch_core_session_t *zap_channel_get_session(zap_channel_t *channel, int32_t id)
{
	switch_core_session_t *session = NULL;

	if (id > ZAP_MAX_TOKENS) {
		return NULL;
	}

	if (!switch_strlen_zero(channel->tokens[id])) {
		if (!(session = switch_core_session_locate(channel->tokens[id]))) {
			zap_channel_clear_token(channel, channel->tokens[id]);
		}
	}

	return session;
}


static void stop_hold(switch_core_session_t *session_a, const char *uuid)
{
	switch_core_session_t *session;
	switch_channel_t *channel, *channel_a;
	;

	if (!uuid) {
		return;
	}

	if ((session = switch_core_session_locate(uuid))) {
		channel = switch_core_session_get_channel(session);

		if (switch_channel_test_flag(channel, CF_HOLD)) {
			channel_a = switch_core_session_get_channel(session_a);
			switch_ivr_unhold(session);
			switch_channel_clear_flag(channel_a, CF_SUSPEND);
			switch_channel_clear_flag(channel_a, CF_HOLD);
		} else {
			switch_channel_stop_broadcast(channel);
			switch_channel_wait_for_flag(channel, CF_BROADCAST, SWITCH_FALSE, 2000, NULL);
		}

		switch_core_session_rwunlock(session);
	}
}

static void start_hold(zap_channel_t *zchan, switch_core_session_t *session_a, const char *uuid, const char *stream)
{
	switch_core_session_t *session;
	switch_channel_t *channel, *channel_a;

	if (!uuid) {
		return;
	}
	
	
	if ((session = switch_core_session_locate(uuid))) {
		channel = switch_core_session_get_channel(session);
		if (switch_strlen_zero(stream)) {
			if (!strcasecmp(globals.hold_music, "indicate_hold")) {
				stream = "indicate_hold";
			}
			if (!strcasecmp(SPAN_CONFIG[zchan->span->span_id].hold_music, "indicate_hold")) {
				stream = "indicate_hold";
			}
		}

		if (switch_strlen_zero(stream)) {
			stream = switch_channel_get_variable(channel, SWITCH_HOLD_MUSIC_VARIABLE);
		}

		if (switch_strlen_zero(stream)) {
			stream = SPAN_CONFIG[zchan->span->span_id].hold_music;
		}

		if (switch_strlen_zero(stream)) {
			stream = globals.hold_music;
		}
		
		
		if (switch_strlen_zero(stream) && !(stream = switch_channel_get_variable(channel, SWITCH_HOLD_MUSIC_VARIABLE))) {
			stream = globals.hold_music;
		}

		if (!switch_strlen_zero(stream)) {
			if (!strcasecmp(stream, "indicate_hold")) {
				channel_a = switch_core_session_get_channel(session_a);
				switch_ivr_hold_uuid(uuid, NULL, 0);
				switch_channel_set_flag(channel_a, CF_SUSPEND);
				switch_channel_set_flag(channel_a, CF_HOLD);
			} else {
				switch_ivr_broadcast(switch_core_session_get_uuid(session), stream, SMF_ECHO_ALEG | SMF_LOOP);
			}
		}

		switch_core_session_rwunlock(session);
	}
}


static void cycle_foreground(zap_channel_t *zchan, int flash, const char *bcast) {
	uint32_t i = 0;
	switch_core_session_t *session;
	switch_channel_t *channel;
	private_t *tech_pvt;
	

	for (i = 0; i < zchan->token_count; i++) {
		if ((session = zap_channel_get_session(zchan, i))) {
			const char *buuid;
			tech_pvt = switch_core_session_get_private(session);
			channel = switch_core_session_get_channel(session);
			buuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE);

			
			if (zchan->token_count == 1 && flash) {
				if (switch_test_flag(tech_pvt, TFLAG_HOLD)) {
					stop_hold(session, buuid);
					switch_clear_flag_locked(tech_pvt, TFLAG_HOLD);
				} else {
					start_hold(zchan, session, buuid, bcast);
					switch_set_flag_locked(tech_pvt, TFLAG_HOLD);
				}
			} else if (i) {
				start_hold(zchan, session, buuid, bcast);
				switch_set_flag_locked(tech_pvt, TFLAG_HOLD);
			} else {
				stop_hold(session, buuid);
				switch_clear_flag_locked(tech_pvt, TFLAG_HOLD);
				if (!switch_channel_test_flag(channel, CF_ANSWERED)) {
					switch_channel_mark_answered(channel);
				}
			}
			switch_core_session_rwunlock(session);
		}
	}
}




static switch_status_t tech_init(private_t *tech_pvt, switch_core_session_t *session, zap_channel_t *zchan)
{
	char *dname = NULL;
	uint32_t interval = 0, srate = 8000;
	zap_codec_t codec;

	tech_pvt->zchan = zchan;
	tech_pvt->read_frame.data = tech_pvt->databuf;
	tech_pvt->read_frame.buflen = sizeof(tech_pvt->databuf);
	tech_pvt->cng_frame.data = tech_pvt->cng_databuf;
	tech_pvt->cng_frame.buflen = sizeof(tech_pvt->cng_databuf);
	tech_pvt->cng_frame.flags = SFF_CNG;
	tech_pvt->cng_frame.codec = &tech_pvt->read_codec;
	memset(tech_pvt->cng_frame.data, 255, tech_pvt->cng_frame.buflen);
	switch_mutex_init(&tech_pvt->mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
	switch_mutex_init(&tech_pvt->flag_mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
	switch_core_session_set_private(session, tech_pvt);
	tech_pvt->session = session;

	zap_channel_command(zchan, ZAP_COMMAND_GET_INTERVAL, &interval);
	zap_channel_command(zchan, ZAP_COMMAND_GET_CODEC, &codec);

	switch(codec) {
	case ZAP_CODEC_ULAW:
		{
			dname = "PCMU";
		}
		break;
	case ZAP_CODEC_ALAW:
		{
			dname = "PCMA";
		}
		break;
	case ZAP_CODEC_SLIN:
		{
			dname = "L16";
		}
		break;
	default:
		abort();
	}


	if (switch_core_codec_init(&tech_pvt->read_codec,
							   dname,
							   NULL,
							   srate,
							   interval,
							   1,
							   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
		return SWITCH_STATUS_GENERR;
	} else {
		if (switch_core_codec_init(&tech_pvt->write_codec,
								   dname,
								   NULL,
								   srate,
								   interval,
								   1,
								   SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
								   NULL, switch_core_session_get_pool(tech_pvt->session)) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Can't load codec?\n");
			switch_core_codec_destroy(&tech_pvt->read_codec);
			return SWITCH_STATUS_GENERR;
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Set codec %s %dms\n", dname, interval);
	switch_core_session_set_read_codec(tech_pvt->session, &tech_pvt->read_codec);
	switch_core_session_set_write_codec(tech_pvt->session, &tech_pvt->write_codec);
	switch_set_flag_locked(tech_pvt, TFLAG_CODEC);
	tech_pvt->read_frame.codec = &tech_pvt->read_codec;
	switch_set_flag_locked(tech_pvt, TFLAG_IO);

	return SWITCH_STATUS_SUCCESS;
	
}

static switch_status_t channel_on_init(switch_core_session_t *session)
{
	switch_channel_t *channel;
	private_t *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	
	/* Move channel's state machine to ROUTING */
	switch_channel_set_state(channel, CS_ROUTING);
	switch_mutex_lock(globals.mutex);
	globals.calls++;
	switch_mutex_unlock(globals.mutex);

	zap_channel_init(tech_pvt->zchan);

	//switch_channel_set_flag(channel, CF_ACCEPT_CNG);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_routing(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL ROUTING\n", switch_channel_get_name(channel));

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_execute(switch_core_session_t *session)
{

	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL EXECUTE\n", switch_channel_get_name(channel));


	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_hangup(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);


	zap_channel_clear_token(tech_pvt->zchan, switch_core_session_get_uuid(session));
	
	switch (tech_pvt->zchan->type) {
	case ZAP_CHAN_TYPE_FXO:
	case ZAP_CHAN_TYPE_EM:
		{

			zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_HANGUP);

		}
		break;
	case ZAP_CHAN_TYPE_FXS:
		{
			if (tech_pvt->zchan->state != ZAP_CHANNEL_STATE_BUSY && tech_pvt->zchan->state != ZAP_CHANNEL_STATE_DOWN) {
				if (tech_pvt->zchan->token_count) {
					cycle_foreground(tech_pvt->zchan, 0, NULL);
				} else {
					zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_HANGUP);
				}
			}
		}
		break;
	case ZAP_CHAN_TYPE_B:
		{
			if (tech_pvt->zchan->state != ZAP_CHANNEL_STATE_DOWN && tech_pvt->zchan->state != ZAP_CHANNEL_STATE_TERMINATING) {
				tech_pvt->zchan->caller_data.hangup_cause = switch_channel_get_cause_q850(channel);
				if (tech_pvt->zchan->caller_data.hangup_cause < 1 || tech_pvt->zchan->caller_data.hangup_cause > 127) {
					tech_pvt->zchan->caller_data.hangup_cause = ZAP_CAUSE_DESTINATION_OUT_OF_ORDER;
				}
				zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_HANGUP);
			}
		}
		break;
	default: 
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unhandled type for channel %s\n", switch_channel_get_name(channel));
		}
		break;
	}

	switch_clear_flag_locked(tech_pvt, TFLAG_IO);

	if (tech_pvt->read_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->read_codec);
	}

	if (tech_pvt->write_codec.implementation) {
		switch_core_codec_destroy(&tech_pvt->write_codec);
	}
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL HANGUP\n", switch_channel_get_name(channel));
	switch_mutex_lock(globals.mutex);
	globals.calls--;
	if (globals.calls < 0) {
		globals.calls = 0;
	}
	switch_mutex_unlock(globals.mutex);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_kill_channel(switch_core_session_t *session, int sig)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (sig) {
	case SWITCH_SIG_KILL:
		switch_clear_flag_locked(tech_pvt, TFLAG_IO);
		switch_set_flag_locked(tech_pvt, TFLAG_DEAD);
		break;
	case SWITCH_SIG_BREAK:
		switch_set_flag_locked(tech_pvt, TFLAG_BREAK);
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_exchange_media(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CHANNEL EXCHANGE_MEDIA\n");
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_soft_execute(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CHANNEL SOFT_EXECUTE\n");
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_send_dtmf(switch_core_session_t *session, const switch_dtmf_t *dtmf)
{
	private_t *tech_pvt = NULL;
	char tmp[2] = "";

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	tmp[0] = dtmf->digit;
	zap_channel_command(tech_pvt->zchan, ZAP_COMMAND_SEND_DTMF, tmp);
		
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	zap_size_t len;
	zap_wait_flag_t wflags = ZAP_READ;
	char dtmf[128] = "";
	zap_status_t status;
	int total_to;
	int chunk, do_break = 0;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
	

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	assert(tech_pvt->zchan != NULL);

	chunk = tech_pvt->zchan->effective_interval * 2;
	total_to = chunk * 2;

	if (switch_test_flag(tech_pvt, TFLAG_DEAD)) {
		return SWITCH_STATUS_FALSE;
	}

 top:

	if (switch_channel_test_flag(channel, CF_SUSPEND)) {
		do_break = 1;
	}

	if (switch_test_flag(tech_pvt, TFLAG_BREAK)) {
		switch_clear_flag_locked(tech_pvt, TFLAG_BREAK);
		do_break = 1;
	}

	if (switch_test_flag(tech_pvt, TFLAG_HOLD) || do_break) {
		switch_yield(tech_pvt->zchan->effective_interval * 1000);
		tech_pvt->cng_frame.datalen = tech_pvt->zchan->packet_len;
		tech_pvt->cng_frame.samples = tech_pvt->cng_frame.datalen;
		tech_pvt->cng_frame.flags = SFF_CNG;
		*frame = &tech_pvt->cng_frame;
		if (tech_pvt->zchan->effective_codec == ZAP_CODEC_SLIN) {
			tech_pvt->cng_frame.samples /= 2;
		}
		return SWITCH_STATUS_SUCCESS;
	}
	
	if (!switch_test_flag(tech_pvt, TFLAG_IO)) {
		goto fail;
	}

	wflags = ZAP_READ;	
	status = zap_channel_wait(tech_pvt->zchan, &wflags, chunk);
	
	if (status == ZAP_FAIL) {
		goto fail;
	}
	
	if (status == ZAP_TIMEOUT) {
		if (!switch_test_flag(tech_pvt, TFLAG_HOLD)) {
			total_to -= chunk;
			if (total_to <= 0) {
				goto fail;
			}
		}

		goto top;
	}

	if (!(wflags & ZAP_READ)) {
		goto fail;
	}

	len = tech_pvt->read_frame.buflen;
	if (zap_channel_read(tech_pvt->zchan, tech_pvt->read_frame.data, &len) != ZAP_SUCCESS) {
		goto fail;
	}

	*frame = &tech_pvt->read_frame;
	tech_pvt->read_frame.datalen = len;
	tech_pvt->read_frame.samples = tech_pvt->read_frame.datalen;

	if (tech_pvt->zchan->effective_codec == ZAP_CODEC_SLIN) {
		tech_pvt->read_frame.samples /= 2;
	}

	if (zap_channel_dequeue_dtmf(tech_pvt->zchan, dtmf, sizeof(dtmf))) {
		switch_dtmf_t _dtmf = { 0, SWITCH_DEFAULT_DTMF_DURATION };
		char *p;
		for (p = dtmf; p && *p; p++) {
			if (is_dtmf(*p)) {
				_dtmf.digit = *p;
				zap_log(ZAP_LOG_DEBUG, "queue DTMF [%c]\n", *p);
				switch_channel_queue_dtmf(channel, &_dtmf);
			}
		}
	}

	return SWITCH_STATUS_SUCCESS;

 fail:
	
	switch_clear_flag_locked(tech_pvt, TFLAG_IO);
	return SWITCH_STATUS_GENERR;
	

}

static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	zap_size_t len;
	unsigned char data[SWITCH_RECOMMENDED_BUFFER_SIZE] = {0};
	
	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	assert(tech_pvt->zchan != NULL);

	if (switch_test_flag(tech_pvt, TFLAG_DEAD)) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_test_flag(tech_pvt, TFLAG_HOLD)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (!switch_test_flag(tech_pvt, TFLAG_IO)) {
		goto fail;
	}
	
	if (switch_test_flag(frame, SFF_CNG)) {
		frame->data = data;
		frame->buflen = sizeof(data);
		if ((frame->datalen = tech_pvt->write_codec.implementation->encoded_bytes_per_packet) > frame->buflen) {
			goto fail;
		}
		memset(data, 255, frame->datalen);
	}


	len = frame->datalen;
	if (zap_channel_write(tech_pvt->zchan, frame->data, frame->buflen, &len) != ZAP_SUCCESS) {
		if (++tech_pvt->wr_error > 10) {
			goto fail;
		}
	} else {
		tech_pvt->wr_error = 0;
	}

	return SWITCH_STATUS_SUCCESS;

 fail:
	
	switch_clear_flag_locked(tech_pvt, TFLAG_IO);
	return SWITCH_STATUS_GENERR;

}


static switch_status_t channel_receive_message_b(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_channel_t *channel;
	private_t *tech_pvt;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
			
	tech_pvt = (private_t *) switch_core_session_get_private(session);
	assert(tech_pvt != NULL);
	
	switch (msg->message_id) {
	case SWITCH_MESSAGE_INDICATE_RINGING:
		{
			if (switch_channel_test_flag(channel, CF_OUTBOUND)) {
				zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_PROGRESS);
			} else {
				zap_set_state_locked_wait(tech_pvt->zchan, ZAP_CHANNEL_STATE_PROGRESS);
			}
		}
		break;
	case SWITCH_MESSAGE_INDICATE_PROGRESS:
		{
			if (switch_channel_test_flag(channel, CF_OUTBOUND)) {
				zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_PROGRESS);
				zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_MEDIA);
			} else {
				/* Don't skip messages in the ISDN call setup
				 * TODO: make the isdn stack smart enough to handle that itself
				 *       until then, this is here for safety...
				 */
				if (tech_pvt->zchan->state < ZAP_CHANNEL_STATE_PROGRESS) {
					zap_set_state_locked_wait(tech_pvt->zchan, ZAP_CHANNEL_STATE_PROGRESS);
				}
				zap_set_state_locked_wait(tech_pvt->zchan, ZAP_CHANNEL_STATE_PROGRESS_MEDIA);
			}
		}
		break;
	case SWITCH_MESSAGE_INDICATE_ANSWER:
		{
			if (switch_channel_test_flag(channel, CF_OUTBOUND)) {
				zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_ANSWERED);
			} else {
				/* Don't skip messages in the ISDN call setup
				 * TODO: make the isdn stack smart enough to handle that itself
				 *       until then, this is here for safety...
				 */
				if (tech_pvt->zchan->state < ZAP_CHANNEL_STATE_PROGRESS) {
					zap_set_state_locked_wait(tech_pvt->zchan, ZAP_CHANNEL_STATE_PROGRESS);
				}
				if (tech_pvt->zchan->state < ZAP_CHANNEL_STATE_PROGRESS_MEDIA) {
					zap_set_state_locked_wait(tech_pvt->zchan, ZAP_CHANNEL_STATE_PROGRESS_MEDIA);
				}
				zap_set_state_locked_wait(tech_pvt->zchan, ZAP_CHANNEL_STATE_UP);
			}
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_receive_message_fxo(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_channel_t *channel;
	private_t *tech_pvt;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
			
	tech_pvt = (private_t *) switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (msg->message_id) {
	case SWITCH_MESSAGE_INDICATE_PROGRESS:
	case SWITCH_MESSAGE_INDICATE_ANSWER:
		if (switch_channel_test_flag(channel, CF_OUTBOUND)) {
			zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_ANSWERED);
			zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_PROGRESS);
			zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_MEDIA);
		} else {
			zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_UP);
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_receive_message_fxs(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_channel_t *channel;
	private_t *tech_pvt;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
			
	tech_pvt = (private_t *) switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (msg->message_id) {
	case SWITCH_MESSAGE_INDICATE_PROGRESS:
	case SWITCH_MESSAGE_INDICATE_ANSWER:
		if (!switch_channel_test_flag(channel, CF_OUTBOUND)) {
			zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_ANSWERED);
			zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_PROGRESS);
			zap_set_flag_locked(tech_pvt->zchan, ZAP_CHANNEL_MEDIA);
			zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_UP);
			switch_channel_mark_answered(channel);
		}
		break;
	case SWITCH_MESSAGE_INDICATE_RINGING:
		if (!switch_channel_test_flag(channel, CF_OUTBOUND)) {
			
			if (!switch_channel_test_flag(channel, CF_ANSWERED) && 
				!switch_channel_test_flag(channel, CF_EARLY_MEDIA) &&
				!switch_channel_test_flag(channel, CF_RING_READY)
				) {
				zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_RING);
				switch_channel_mark_ring_ready(channel);
			}
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	private_t *tech_pvt;
	switch_status_t status;
	
	tech_pvt = (private_t *) switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (tech_pvt->zchan->type) {
	case ZAP_CHAN_TYPE_FXS:
	case ZAP_CHAN_TYPE_EM:
		status = channel_receive_message_fxs(session, msg);
		break;
	case ZAP_CHAN_TYPE_FXO:
		status = channel_receive_message_fxo(session, msg);
		break;
	case ZAP_CHAN_TYPE_B:
		status = channel_receive_message_b(session, msg);
		break;
	default:
		status = SWITCH_STATUS_FALSE;
		break;
	}

	return status;

}

switch_state_handler_table_t openzap_state_handlers = {
	/*.on_init */ channel_on_init,
	/*.on_routing */ channel_on_routing,
	/*.on_execute */ channel_on_execute,
	/*.on_hangup */ channel_on_hangup,
	/*.on_exchange_media */ channel_on_exchange_media,
	/*.on_soft_execute */ channel_on_soft_execute
};

switch_io_routines_t openzap_io_routines = {
	/*.outgoing_channel */ channel_outgoing_channel,
	/*.read_frame */ channel_read_frame,
	/*.write_frame */ channel_write_frame,
	/*.kill_channel */ channel_kill_channel,
	/*.send_dtmf */ channel_send_dtmf,
	/*.receive_message*/ channel_receive_message
};

/* Make sure when you have 2 sessions in the same scope that you pass the appropriate one to the routines
that allocate memory or you will have 1 channel with memory allocated from another channel's pool!
*/
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool,
													switch_originate_flag_t flags)
{

	char *dest = NULL, *data = NULL;
	int span_id = 0, chan_id = 0;
	zap_channel_t *zchan = NULL;
	switch_call_cause_t cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	char name[128];
	zap_status_t status;
	int direction = ZAP_TOP_DOWN;
	zap_caller_data_t caller_data = {{ 0 }};
	char *span_name = NULL;
	switch_event_header_t *h;
	char *argv[3];
	int argc = 0;
	const char *var;
	switch_channel_t *channel = NULL;



	if (!outbound_profile) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing caller profile\n");
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	if (switch_strlen_zero(outbound_profile->destination_number)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid dial string\n");
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}


	data = switch_core_strdup(outbound_profile->pool, outbound_profile->destination_number);

	if ((argc = switch_separate_string(data, '/', argv, (sizeof(argv) / sizeof(argv[0])))) < 2) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid dial string\n");
        return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}
	
	if (switch_is_number(argv[0])) {
		span_id = atoi(argv[0]);
	} else {
		span_name = argv[0];
	}	

	if (*argv[1] == 'A') {
		direction = ZAP_BOTTOM_UP;
	} else if (*argv[1] == 'a') {
		direction =  ZAP_TOP_DOWN;
	} else {
		chan_id = atoi(argv[1]);
	}

	if (!(dest = argv[2])) {
		dest = "";
	}

	if (!span_id && !switch_strlen_zero(span_name)) {
		zap_span_t *span;
		zap_status_t zstatus = zap_span_find_by_name(span_name, &span);
		if (zstatus == ZAP_SUCCESS && span) {
			span_id = span->span_id;
		}
	}

	if (!span_id) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing span\n");
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	if (chan_id < 0) {
		direction = ZAP_BOTTOM_UP;
		chan_id = 0;
	}
	
	if (switch_test_flag(outbound_profile, SWITCH_CPF_SCREEN)) {
		caller_data.screen = 1;
	}

	if (switch_test_flag(outbound_profile, SWITCH_CPF_HIDE_NUMBER)) {
		caller_data.pres = 1;
	}

	if (!switch_strlen_zero(dest)) {
		zap_set_string(caller_data.ani.digits, dest);
	}
	
	channel = switch_core_session_get_channel(*new_session);

	if ((var = switch_event_get_header(var_event, "openzap_outbound_ton")) || (var = switch_channel_get_variable(channel, "openzap_outbound_ton"))) {
		if (!strcasecmp(var, "national")) {
			caller_data.ani.type = Q931_TON_NATIONAL;
		}
	} else {
		caller_data.ani.type = outbound_profile->destination_number_ton;
	}

	caller_data.ani.plan = outbound_profile->destination_number_numplan;

#if 0
	if (!switch_strlen_zero(outbound_profile->rdnis)) {
		zap_set_string(caller_data.rdnis.digits, outbound_profile->rdnis);
	}
#endif

	zap_set_string(caller_data.cid_name, outbound_profile->caller_id_name);
	zap_set_string(caller_data.cid_num.digits, outbound_profile->caller_id_number);
	
	if (chan_id) {
		status = zap_channel_open(span_id, chan_id, &zchan);
	} else {
		status = zap_channel_open_any(span_id, direction, &caller_data, &zchan);
	}
	
	if (status != ZAP_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No channels available\n");
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	zap_channel_clear_vars(zchan);
	for (h = var_event->headers; h; h = h->next) {
		if (!strncasecmp(h->name, OPENZAP_VAR_PREFIX, OPENZAP_VAR_PREFIX_LEN)) {
			char *v = h->name + OPENZAP_VAR_PREFIX_LEN;
			if (!switch_strlen_zero(v)) {
				zap_channel_add_var(zchan, v, h->value);
			}
		}
	}
	
	if ((*new_session = switch_core_session_request(openzap_endpoint_interface, pool)) != 0) {
		private_t *tech_pvt;
		switch_caller_profile_t *caller_profile;

		switch_core_session_add_stream(*new_session, NULL);
		if ((tech_pvt = (private_t *) switch_core_session_alloc(*new_session, sizeof(private_t))) != 0) {
			tech_init(tech_pvt, *new_session, zchan);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Hey where is my memory pool?\n");
			switch_core_session_destroy(new_session);
			cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
			goto fail;
		}

		snprintf(name, sizeof(name), "OpenZAP/%u:%u/%s", zchan->span_id, zchan->chan_id, dest);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connect outbound channel %s\n", name);
		switch_channel_set_name(channel, name);
		zchan->caller_data = caller_data;
		caller_profile = switch_caller_profile_clone(*new_session, outbound_profile);
		switch_channel_set_caller_profile(channel, caller_profile);
		tech_pvt->caller_profile = caller_profile;
		
		
		switch_channel_set_flag(channel, CF_OUTBOUND);
		switch_channel_set_state(channel, CS_INIT);
		if (zap_channel_add_token(zchan, switch_core_session_get_uuid(*new_session), zchan->token_count) != ZAP_SUCCESS) {
			switch_core_session_destroy(new_session);
			cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
            goto fail;
		}


		if (zap_channel_outgoing_call(zchan) != ZAP_SUCCESS) {
			if (tech_pvt->read_codec.implementation) {
				switch_core_codec_destroy(&tech_pvt->read_codec);
			}
			
			if (tech_pvt->write_codec.implementation) {
				switch_core_codec_destroy(&tech_pvt->write_codec);
			}
			switch_core_session_destroy(new_session);
            cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
            goto fail;
		}

		zap_channel_init(zchan);
		
		return SWITCH_CAUSE_SUCCESS;
	}

 fail:

	if (zchan) {
		zap_channel_done(zchan);
	}

	return cause;

}

zap_status_t zap_channel_from_event(zap_sigmsg_t *sigmsg, switch_core_session_t **sp)
{
	switch_core_session_t *session = NULL;
	private_t *tech_pvt = NULL;
	switch_channel_t *channel = NULL;
	char name[128];
	
	*sp = NULL;
	
	if (!(session = switch_core_session_request(openzap_endpoint_interface, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Initilization Error!\n");
		return ZAP_FAIL;
	}
	
	switch_core_session_add_stream(session, NULL);
	
	tech_pvt = (private_t *) switch_core_session_alloc(session, sizeof(private_t));
	assert(tech_pvt != NULL);
	channel = switch_core_session_get_channel(session);
	if (tech_init(tech_pvt, session, sigmsg->channel) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Initilization Error!\n");
		switch_core_session_destroy(&session);
		return ZAP_FAIL;
	}
	
	*sigmsg->channel->caller_data.collected = '\0';
	
	if (switch_strlen_zero(sigmsg->channel->caller_data.cid_name)) {
		switch_set_string(sigmsg->channel->caller_data.cid_name, sigmsg->channel->chan_name);
	}

	if (switch_strlen_zero(sigmsg->channel->caller_data.cid_num.digits)) {
		if (!switch_strlen_zero(sigmsg->channel->caller_data.ani.digits)) {
			switch_set_string(sigmsg->channel->caller_data.cid_num.digits, sigmsg->channel->caller_data.ani.digits);
		} else {
			switch_set_string(sigmsg->channel->caller_data.cid_num.digits, sigmsg->channel->chan_number);
		}
	}

	tech_pvt->caller_profile = switch_caller_profile_new(switch_core_session_get_pool(session),
														 "OpenZAP",
														 SPAN_CONFIG[sigmsg->channel->span_id].dialplan,
														 sigmsg->channel->caller_data.cid_name,
														 sigmsg->channel->caller_data.cid_num.digits,
														 NULL,
														 sigmsg->channel->caller_data.ani.digits,
														 sigmsg->channel->caller_data.aniII,
														 sigmsg->channel->caller_data.rdnis.digits,
														 (char *) modname,
														 SPAN_CONFIG[sigmsg->channel->span_id].context,
														 sigmsg->channel->caller_data.dnis.digits);

	assert(tech_pvt->caller_profile != NULL);

	if (sigmsg->channel->caller_data.screen == 1 || sigmsg->channel->caller_data.screen == 3) {
		switch_set_flag(tech_pvt->caller_profile, SWITCH_CPF_SCREEN);
	}

	if (sigmsg->channel->caller_data.pres) {
		switch_set_flag(tech_pvt->caller_profile, SWITCH_CPF_HIDE_NAME | SWITCH_CPF_HIDE_NUMBER);
	}
	
	snprintf(name, sizeof(name), "OpenZAP/%u:%u/%s", sigmsg->channel->span_id, sigmsg->channel->chan_id, tech_pvt->caller_profile->destination_number);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connect inbound channel %s\n", name);
	switch_channel_set_name(channel, name);
	switch_channel_set_caller_profile(channel, tech_pvt->caller_profile);
		
	switch_channel_set_state(channel, CS_INIT);
	if (switch_core_session_thread_launch(session) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Error spawning thread\n");
		switch_core_session_destroy(&session);
		return ZAP_FAIL;
	}

	if (zap_channel_add_token(sigmsg->channel, switch_core_session_get_uuid(session), 0) != ZAP_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Error adding token\n");
		switch_core_session_destroy(&session);
		return ZAP_FAIL;
	}
	*sp = session;

    return ZAP_SUCCESS;
}


static ZIO_SIGNAL_CB_FUNCTION(on_fxo_signal)
{
	switch_core_session_t *session = NULL;
	switch_channel_t *channel = NULL;
	zap_status_t status;

	zap_log(ZAP_LOG_DEBUG, "got FXO sig %d:%d [%s]\n", sigmsg->channel->span_id, sigmsg->channel->chan_id, zap_signal_event2str(sigmsg->event_id));

    switch(sigmsg->event_id) {

    case ZAP_SIGEVENT_PROGRESS_MEDIA:
		{
			if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_mark_pre_answered(channel);
			}
		}
		break;
    case ZAP_SIGEVENT_STOP:
		{
			while((session = zap_channel_get_session(sigmsg->channel, 0))) {
				zap_channel_clear_token(sigmsg->channel, 0);
				channel = switch_core_session_get_channel(session);
				switch_channel_hangup(channel, sigmsg->channel->caller_data.hangup_cause);
				zap_channel_clear_token(sigmsg->channel, switch_core_session_get_uuid(session));
				switch_core_session_rwunlock(session);
			}
		}
		break;
    case ZAP_SIGEVENT_UP:
		{
			if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_mark_answered(channel);
				switch_core_session_rwunlock(session);
			}
		}
		break;
    case ZAP_SIGEVENT_START:
		{
			status = zap_channel_from_event(sigmsg, &session);
			if (status != ZAP_SUCCESS) {
				zap_set_state_locked(sigmsg->channel, ZAP_CHANNEL_STATE_DOWN);
			}
		}
		break;

	default:
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unhandled type for channel %d:%d\n",
							  sigmsg->channel->span_id, sigmsg->channel->chan_id);
		}
		break;

	}

	return ZAP_SUCCESS;
}

static ZIO_SIGNAL_CB_FUNCTION(on_fxs_signal)
{
	switch_core_session_t *session = NULL;
	switch_channel_t *channel = NULL;
	zap_status_t status = ZAP_SUCCESS;

    zap_log(ZAP_LOG_DEBUG, "got FXS sig [%s]\n", zap_signal_event2str(sigmsg->event_id));

    switch(sigmsg->event_id) {
    case ZAP_SIGEVENT_UP:
		{
			if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_mark_answered(channel);
				switch_core_session_rwunlock(session);
			}
		}
		break;
    case ZAP_SIGEVENT_PROGRESS:
		{
			if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_mark_ring_ready(channel);
				switch_core_session_rwunlock(session);
			}
		}
		break;
    case ZAP_SIGEVENT_START:
		{
			zap_clear_flag_locked(sigmsg->channel, ZAP_CHANNEL_HOLD);
			status = zap_channel_from_event(sigmsg, &session);
			if (status != ZAP_SUCCESS) {
				zap_set_state_locked(sigmsg->channel, ZAP_CHANNEL_STATE_BUSY);
			}
		}
		break;
    case ZAP_SIGEVENT_STOP:
		{
			switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;
			if (sigmsg->channel->token_count) {
				switch_core_session_t *session_a, *session_b, *session_t = NULL;
				switch_channel_t *channel_a = NULL, *channel_b = NULL;
				int digits = !switch_strlen_zero(sigmsg->channel->caller_data.collected);
				const char *br_a_uuid = NULL, *br_b_uuid = NULL;
				private_t *tech_pvt = NULL;


				if ((session_a = switch_core_session_locate(sigmsg->channel->tokens[0]))) {
					channel_a = switch_core_session_get_channel(session_a);
					br_a_uuid = switch_channel_get_variable(channel_a, SWITCH_SIGNAL_BOND_VARIABLE);

					tech_pvt = switch_core_session_get_private(session_a);
					stop_hold(session_a, switch_channel_get_variable(channel_a, SWITCH_SIGNAL_BOND_VARIABLE));
					switch_clear_flag_locked(tech_pvt, TFLAG_HOLD);
				}

				if ((session_b = switch_core_session_locate(sigmsg->channel->tokens[1]))) {
					channel_b = switch_core_session_get_channel(session_b);
					br_b_uuid = switch_channel_get_variable(channel_b, SWITCH_SIGNAL_BOND_VARIABLE);

					tech_pvt = switch_core_session_get_private(session_b);
					stop_hold(session_a, switch_channel_get_variable(channel_b, SWITCH_SIGNAL_BOND_VARIABLE));
					switch_clear_flag_locked(tech_pvt, TFLAG_HOLD);
				}

				if (channel_a && channel_b && !switch_channel_test_flag(channel_a, CF_OUTBOUND) && !switch_channel_test_flag(channel_b, CF_OUTBOUND)) {
					cause = SWITCH_CAUSE_ATTENDED_TRANSFER;
					if (br_a_uuid && br_b_uuid) {
						switch_ivr_uuid_bridge(br_a_uuid, br_b_uuid);
					} else if (br_a_uuid && digits) {
						session_t = switch_core_session_locate(br_a_uuid);
					} else if (br_b_uuid && digits) {
						session_t = switch_core_session_locate(br_b_uuid);
					}
				}
				
				if (session_t) {
					switch_ivr_session_transfer(session_t, sigmsg->channel->caller_data.collected, NULL, NULL);
					switch_core_session_rwunlock(session_t);
				}

				if (session_a) {
					switch_core_session_rwunlock(session_a);
				}

				if (session_b) {
					switch_core_session_rwunlock(session_b);
				}

				
			}

			while((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_hangup(channel, cause);
				zap_channel_clear_token(sigmsg->channel, switch_core_session_get_uuid(session));
				switch_core_session_rwunlock(session);
			}
			zap_channel_clear_token(sigmsg->channel, NULL);
			
		}
		break;

    case ZAP_SIGEVENT_ADD_CALL:
		{
			cycle_foreground(sigmsg->channel, 1, NULL);
		}
		break;
    case ZAP_SIGEVENT_FLASH:
		{

			if (zap_test_flag(sigmsg->channel, ZAP_CHANNEL_HOLD) && sigmsg->channel->token_count == 1) {
				switch_core_session_t *session;
				if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
					const char *buuid;
					switch_channel_t *channel;
					private_t *tech_pvt;
					
					tech_pvt = switch_core_session_get_private(session);
					channel = switch_core_session_get_channel(session);
					buuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE);
					zap_set_state_locked(sigmsg->channel,  ZAP_CHANNEL_STATE_UP);
					stop_hold(session, buuid);
					switch_clear_flag_locked(tech_pvt, TFLAG_HOLD);
					switch_core_session_rwunlock(session);
				}
			} else if (sigmsg->channel->token_count == 2 && (SPAN_CONFIG[sigmsg->span_id].analog_options & ANALOG_OPTION_3WAY)) {
				if (zap_test_flag(sigmsg->channel, ZAP_CHANNEL_3WAY)) {
					zap_clear_flag(sigmsg->channel, ZAP_CHANNEL_3WAY);
					if ((session = zap_channel_get_session(sigmsg->channel, 1))) {
						channel = switch_core_session_get_channel(session);
						switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
						zap_channel_clear_token(sigmsg->channel, switch_core_session_get_uuid(session));
						switch_core_session_rwunlock(session);
					}
					cycle_foreground(sigmsg->channel, 1, NULL);
				} else {
					char *cmd;
					cmd = switch_mprintf("three_way::%s", sigmsg->channel->tokens[0]);
					zap_set_flag(sigmsg->channel, ZAP_CHANNEL_3WAY);
					cycle_foreground(sigmsg->channel, 1, cmd);
					free(cmd);
				}
			} else if ((SPAN_CONFIG[sigmsg->span_id].analog_options & ANALOG_OPTION_CALL_SWAP)
					   || (SPAN_CONFIG[sigmsg->span_id].analog_options & ANALOG_OPTION_3WAY)
					   ) { 
				cycle_foreground(sigmsg->channel, 1, NULL);
				if (sigmsg->channel->token_count == 1) {
					zap_set_flag_locked(sigmsg->channel, ZAP_CHANNEL_HOLD);
					zap_set_state_locked(sigmsg->channel, ZAP_CHANNEL_STATE_DIALTONE);
				}
			}
			
		}
		break;

    case ZAP_SIGEVENT_COLLECTED_DIGIT:
		{
			char *dtmf = sigmsg->raw_data;
			char *regex = SPAN_CONFIG[sigmsg->channel->span->span_id].dial_regex;
			char *fail_regex = SPAN_CONFIG[sigmsg->channel->span->span_id].fail_dial_regex;
			
			if (switch_strlen_zero(regex)) {
				regex = NULL;
			}

			if (switch_strlen_zero(fail_regex)) {
				fail_regex = NULL;
			}

			zap_log(ZAP_LOG_DEBUG, "got DTMF sig [%s]\n", dtmf);
			switch_set_string(sigmsg->channel->caller_data.collected, dtmf);
			
			if ((regex || fail_regex) && !switch_strlen_zero(dtmf)) {
				switch_regex_t *re = NULL;
				int ovector[30];
				int match = 0;

				if (fail_regex) {
					match = switch_regex_perform(dtmf, fail_regex, &re, ovector, sizeof(ovector) / sizeof(ovector[0]));
					status = match ? ZAP_SUCCESS : ZAP_BREAK;
					switch_regex_safe_free(re);
				}

				if (status == ZAP_SUCCESS && regex) {
					match = switch_regex_perform(dtmf, regex, &re, ovector, sizeof(ovector) / sizeof(ovector[0]));
					status = match ? ZAP_BREAK : ZAP_SUCCESS;
				}
				
				switch_regex_safe_free(re);
			}

		}
		break;

	default:
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unhandled type for channel %d:%d\n",
							  sigmsg->channel->span_id, sigmsg->channel->chan_id);
		}
		break;

	}

	return status;
}

static ZIO_SIGNAL_CB_FUNCTION(on_clear_channel_signal)
{
	switch_core_session_t *session = NULL;
	switch_channel_t *channel = NULL;

	zap_log(ZAP_LOG_DEBUG, "got clear channel sig [%s]\n", zap_signal_event2str(sigmsg->event_id));

    switch(sigmsg->event_id) {
    case ZAP_SIGEVENT_START:
		{
			zap_tone_type_t tt = ZAP_TONE_DTMF;
			
			if (zap_channel_command(sigmsg->channel, ZAP_COMMAND_ENABLE_DTMF_DETECT, &tt) != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "TONE ERROR\n");
			}

			return zap_channel_from_event(sigmsg, &session);
		}
		break;
    case ZAP_SIGEVENT_STOP:
    case ZAP_SIGEVENT_RESTART:
		{	
			while((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_hangup(channel, sigmsg->channel->caller_data.hangup_cause);
				zap_channel_clear_token(sigmsg->channel, switch_core_session_get_uuid(session));
				switch_core_session_rwunlock(session);
			}
		}
		break;
    case ZAP_SIGEVENT_UP:
		{
			if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
				zap_tone_type_t tt = ZAP_TONE_DTMF;
				channel = switch_core_session_get_channel(session);
				switch_channel_mark_answered(channel);
				if (zap_channel_command(sigmsg->channel, ZAP_COMMAND_ENABLE_DTMF_DETECT, &tt) != ZAP_SUCCESS) {
					zap_log(ZAP_LOG_ERROR, "TONE ERROR\n");
				}
				switch_core_session_rwunlock(session);
			}
		}
    case ZAP_SIGEVENT_PROGRESS_MEDIA:
		{
			if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_mark_pre_answered(channel);
				switch_core_session_rwunlock(session);
			}
		}
		break;
    case ZAP_SIGEVENT_PROGRESS:
		{
			if ((session = zap_channel_get_session(sigmsg->channel, 0))) {
				channel = switch_core_session_get_channel(session);
				switch_channel_mark_ring_ready(channel);
				switch_core_session_rwunlock(session);
			}
		}
		break;

	default:
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unhandled type for channel %d:%d\n",
							  sigmsg->channel->span_id, sigmsg->channel->chan_id);
		}
		break;
	}

	return ZAP_SUCCESS;
}


static ZIO_SIGNAL_CB_FUNCTION(on_analog_signal)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	
	switch (sigmsg->channel->type) {
	case ZAP_CHAN_TYPE_FXO:
	case ZAP_CHAN_TYPE_EM:
		{
			status = on_fxo_signal(sigmsg);
		}
		break;
	case ZAP_CHAN_TYPE_FXS:
		{
			status = on_fxs_signal(sigmsg);
		}
		break;
	default: 
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unhandled type for channel %d:%d\n",
							  sigmsg->channel->span_id, sigmsg->channel->chan_id);
		}
		break;
	}

	return status;
}

static void zap_logger(const char *file, const char *func, int line, int level, const char *fmt, ...)
{
    char *data = NULL;
    va_list ap;
	
    va_start(ap, fmt);

	if (switch_vasprintf(&data, fmt, ap) != -1) {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, (char *)func, line, NULL, level, "%s", data);
		free(data);
	}
	
    va_end(ap);

}

static uint32_t enable_analog_option(const char *str, uint32_t current_options)
{
	if (!strcasecmp(str, "3-way")) {
		current_options |= ANALOG_OPTION_3WAY;
		current_options &= ~ANALOG_OPTION_CALL_SWAP;
	} else if (!strcasecmp(str, "call-swap")) {
		current_options |= ANALOG_OPTION_CALL_SWAP;
		current_options &= ~ANALOG_OPTION_3WAY;
	}
	
	return current_options;
	
}

static switch_status_t load_config(void)
{
	char *cf = "openzap.conf";
	switch_xml_t cfg, xml, settings, param, spans, myspan;

	memset(&globals, 0, sizeof(globals));
	switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, module_pool);
	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}
	
	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "debug")) {
				globals.debug = atoi(val);
			} else if (!strcasecmp(var, "hold-music")) {
				switch_set_string(globals.hold_music, val);
			} else if (!strcasecmp(var, "enable-analog-option")) {
				globals.analog_options = enable_analog_option(val, globals.analog_options);
			}
		}
	}

	if ((spans = switch_xml_child(cfg, "analog_spans"))) {
		for (myspan = switch_xml_child(spans, "span"); myspan; myspan = myspan->next) {
			char *id = (char *) switch_xml_attr(myspan, "id");
			char *name = (char *) switch_xml_attr(myspan, "name");
			zap_status_t zstatus = ZAP_FAIL;
			char *context = "default";
			char *dialplan = "XML";
			char *tonegroup = NULL;
			char *digit_timeout = NULL;
			char *max_digits = NULL;
			char *hotline = NULL;
			char *dial_regex = NULL;
			char *hold_music = NULL;
			char *fail_dial_regex = NULL;
			char *enable_callerid = "true";

			uint32_t span_id = 0, to = 0, max = 0;
			zap_span_t *span = NULL;
			analog_option_t analog_options = ANALOG_OPTION_NONE;
			
			for (param = switch_xml_child(myspan, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "tonegroup")) {
					tonegroup = val;
				} else if (!strcasecmp(var, "digit_timeout") || !strcasecmp(var, "digit-timeout")) {
					digit_timeout = val;
				} else if (!strcasecmp(var, "context")) {
					context = val;
				} else if (!strcasecmp(var, "dialplan")) {
					dialplan = val;
				} else if (!strcasecmp(var, "dial-regex")) {
					dial_regex = val;
				} else if (!strcasecmp(var, "enable-callerid")) {
					enable_callerid = val;
				} else if (!strcasecmp(var, "fail-dial-regex")) {
					fail_dial_regex = val;
				} else if (!strcasecmp(var, "hold-music")) {
					hold_music = val;
				} else if (!strcasecmp(var, "max_digits") || !strcasecmp(var, "max-digits")) {
					max_digits = val;
				} else if (!strcasecmp(var, "hotline")) {
					hotline = val;
				} else if (!strcasecmp(var, "enable-analog-option")) {
					analog_options = enable_analog_option(val, analog_options);
				}
			}
				
			if (!id && !name) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "span missing required param 'id'\n");
				continue;
			}

			
			
			if (!tonegroup) {
				tonegroup = "us";
			}
			
			if (digit_timeout) {
				to = atoi(digit_timeout);
			}

			if (max_digits) {
				max = atoi(max_digits);
			}

			if (name) {
				zstatus = zap_span_find_by_name(name, &span);
			} else {
				if (switch_is_number(id)) {
					span_id = atoi(id);
					zstatus = zap_span_find(span_id, &span);
				}

				if (zstatus != ZAP_SUCCESS) {
					zstatus = zap_span_find_by_name(id, &span);
				}
			}

			if (zstatus != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error finding OpenZAP span id:%s name:%s\n", switch_str_nil(id), switch_str_nil(name));
				continue;
			}
			
			if (!span_id) {
				span_id = span->span_id;
			}

			if (zap_configure_span("analog", span, on_analog_signal, 
								   "tonemap", tonegroup, 
								   "digit_timeout", &to,
								   "max_dialstr", &max,
								   "hotline", hotline,
								   "enable_callerid", enable_callerid,
								   TAG_END) != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error starting OpenZAP span %d\n", span_id);
				continue;
			}

			SPAN_CONFIG[span->span_id].span = span;
			switch_set_string(SPAN_CONFIG[span->span_id].context, context);
			switch_set_string(SPAN_CONFIG[span->span_id].dialplan, dialplan);
			SPAN_CONFIG[span->span_id].analog_options = analog_options | globals.analog_options;
			
			if (dial_regex) {
				switch_set_string(SPAN_CONFIG[span->span_id].dial_regex, dial_regex);
			}

			if (fail_dial_regex) {
				switch_set_string(SPAN_CONFIG[span->span_id].fail_dial_regex, fail_dial_regex);
			}

			if (hold_music) {
				switch_set_string(SPAN_CONFIG[span->span_id].hold_music, hold_music);
			}
			switch_copy_string(SPAN_CONFIG[span->span_id].type, "analog", sizeof(SPAN_CONFIG[span->span_id].type));
			zap_span_start(span);
		}
	}

	if ((spans = switch_xml_child(cfg, "analog_em_spans"))) {
		for (myspan = switch_xml_child(spans, "span"); myspan; myspan = myspan->next) {
			char *id = (char *) switch_xml_attr(myspan, "id");
			char *name = (char *) switch_xml_attr(myspan, "name");
			zap_status_t zstatus = ZAP_FAIL;
			char *context = "default";
			char *dialplan = "XML";
			char *tonegroup = NULL;
			char *digit_timeout = NULL;
			char *max_digits = NULL;
			char *dial_regex = NULL;
			char *hold_music = NULL;
			char *fail_dial_regex = NULL;
			uint32_t span_id = 0, to = 0, max = 0;
			zap_span_t *span = NULL;
			analog_option_t analog_options = ANALOG_OPTION_NONE;

			for (param = switch_xml_child(myspan, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "tonegroup")) {
					tonegroup = val;
				} else if (!strcasecmp(var, "digit_timeout") || !strcasecmp(var, "digit-timeout")) {
					digit_timeout = val;
				} else if (!strcasecmp(var, "context")) {
					context = val;
				} else if (!strcasecmp(var, "dialplan")) {
					dialplan = val;
				} else if (!strcasecmp(var, "dial-regex")) {
					dial_regex = val;
				} else if (!strcasecmp(var, "fail-dial-regex")) {
					fail_dial_regex = val;
				} else if (!strcasecmp(var, "hold-music")) {
					hold_music = val;
				} else if (!strcasecmp(var, "max_digits") || !strcasecmp(var, "max-digits")) {
					max_digits = val;
				} else if (!strcasecmp(var, "enable-analog-option")) {
					analog_options = enable_analog_option(val, analog_options);
				}
			}
				
			if (!id && !name) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "span missing required param 'id'\n");
				continue;
			}

			
			if (!tonegroup) {
				tonegroup = "us";
			}
			
			if (digit_timeout) {
				to = atoi(digit_timeout);
			}

			if (max_digits) {
				max = atoi(max_digits);
			}


			if (name) {
				zstatus = zap_span_find_by_name(name, &span);
			} else {
				if (switch_is_number(id)) {
					span_id = atoi(id);
					zstatus = zap_span_find(span_id, &span);
				}

				if (zstatus != ZAP_SUCCESS) {
					zstatus = zap_span_find_by_name(id, &span);
				}
			}

			if (zstatus != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error finding OpenZAP span id:%s name:%s\n", switch_str_nil(id), switch_str_nil(name));
				continue;
			}
			
			if (!span_id) {
				span_id = span->span_id;
			}


			if (zap_configure_span("analog_em", span, on_analog_signal, 
								   "tonemap", tonegroup, 
								   "digit_timeout", &to,
								   "max_dialstr", &max,
								   TAG_END) != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error starting OpenZAP span %d\n", span_id);
				continue;
			}

			SPAN_CONFIG[span->span_id].span = span;
			switch_set_string(SPAN_CONFIG[span->span_id].context, context);
			switch_set_string(SPAN_CONFIG[span->span_id].dialplan, dialplan);
			SPAN_CONFIG[span->span_id].analog_options = analog_options | globals.analog_options;
			
			if (dial_regex) {
				switch_set_string(SPAN_CONFIG[span->span_id].dial_regex, dial_regex);
			}

			if (fail_dial_regex) {
				switch_set_string(SPAN_CONFIG[span->span_id].fail_dial_regex, fail_dial_regex);
			}

			if (hold_music) {
				switch_set_string(SPAN_CONFIG[span->span_id].hold_music, hold_music);
			}
			switch_copy_string(SPAN_CONFIG[span->span_id].type, "analog_em", sizeof(SPAN_CONFIG[span->span_id].type));
			zap_span_start(span);
		}
	}

	if ((spans = switch_xml_child(cfg, "pri_spans"))) {
		for (myspan = switch_xml_child(spans, "span"); myspan; myspan = myspan->next) {
			char *id = (char *) switch_xml_attr(myspan, "id");
			char *name = (char *) switch_xml_attr(myspan, "name");
			zap_status_t zstatus = ZAP_FAIL;
			char *context = "default";
			char *dialplan = "XML";
			//Q921NetUser_t mode = Q931_TE;
			//Q931Dialect_t dialect = Q931_Dialect_National;
			char *mode = NULL;
			char *dialect = NULL;
			uint32_t span_id = 0;
			zap_span_t *span = NULL;
			char *tonegroup = NULL;
			char *digit_timeout = NULL;
			uint32_t to = 0;
			uint32_t opts = 0;
			int q921loglevel = -1;
			int q931loglevel = -1;
			// quick debug
			// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ID: '%s', Name:'%s'\n",id,name);

			for (param = switch_xml_child(myspan, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "tonegroup")) {
					tonegroup = val;
				} else if (!strcasecmp(var, "mode")) {
					mode = val;
				} else if (!strcasecmp(var, "dialect")) {
					dialect = val;
				} else if (!strcasecmp(var, "q921loglevel")) {
                    if ((q921loglevel = switch_log_str2level(val)) == SWITCH_LOG_INVALID) {
                        q921loglevel = -1;
                    }
				} else if (!strcasecmp(var, "q931loglevel")) {
                    if ((q931loglevel = switch_log_str2level(val)) == SWITCH_LOG_INVALID) {
                        q931loglevel = -1;
                    }
				} else if (!strcasecmp(var, "context")) {
					context = val;
				} else if (!strcasecmp(var, "suggest-channel") && switch_true(val)) {
					opts |= 1;
				} else if (!strcasecmp(var, "dialplan")) {
					dialplan = val;
				} else if (!strcasecmp(var, "digit_timeout") || !strcasecmp(var, "digit-timeout")) {
					digit_timeout = val;
				}
			}
	
			if (!id && !name) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "span missing required param 'id'\n");
				continue;
			}
			
			if (name) {
				zstatus = zap_span_find_by_name(name, &span);
			} else {
				if (switch_is_number(id)) {
					span_id = atoi(id);
					zstatus = zap_span_find(span_id, &span);
				}

				if (zstatus != ZAP_SUCCESS) {
					zstatus = zap_span_find_by_name(id, &span);
				}
			}

			if (digit_timeout) {
				to = atoi(digit_timeout);
			}
			
			if (zstatus != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error finding OpenZAP span id:%s name:%s\n", switch_str_nil(id), switch_str_nil(name));
				continue;
			}
			
			if (!span_id) {
				span_id = span->span_id;
			}
			
			if (!tonegroup) {
				tonegroup = "us";
			}
			
			if (zap_configure_span("isdn", span, on_clear_channel_signal, 
								   "mode", mode,
								   "dialect", dialect,
								   "digit_timeout", &to,
								   "opts", opts,
								   "q921loglevel", q921loglevel,
								   "q931loglevel", q931loglevel,
								   TAG_END) != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error starting OpenZAP span %d mode: %s dialect: %s error: %s\n", span_id, mode, dialect, span->last_error);
				continue;
			}

			SPAN_CONFIG[span->span_id].span = span;
			switch_copy_string(SPAN_CONFIG[span->span_id].context, context, sizeof(SPAN_CONFIG[span->span_id].context));
			switch_copy_string(SPAN_CONFIG[span->span_id].dialplan, dialplan, sizeof(SPAN_CONFIG[span->span_id].dialplan));
			switch_copy_string(SPAN_CONFIG[span->span_id].type, "isdn", sizeof(SPAN_CONFIG[span->span_id].type));

			zap_span_start(span);
		}
	}

	if ((spans = switch_xml_child(cfg, "boost_spans"))) {
		for (myspan = switch_xml_child(spans, "span"); myspan; myspan = myspan->next) {
			char *id = (char *) switch_xml_attr(myspan, "id");
			char *name = (char *) switch_xml_attr(myspan, "name");
			zap_status_t zstatus = ZAP_FAIL;
			char *context = "default";
			char *dialplan = "XML";
			uint32_t span_id = 0;
			zap_span_t *span = NULL;
			char *tonegroup = NULL;
			char *local_ip = NULL;
			int local_port = 0;
			char *remote_ip = NULL;
			int remote_port = 0;

			for (param = switch_xml_child(myspan, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "tonegroup")) {
					tonegroup = val;
				} else if (!strcasecmp(var, "local-ip")) {
					local_ip = val;
				} else if (!strcasecmp(var, "local-port")) {
					local_port = atoi(val);
				} else if (!strcasecmp(var, "remote-ip")) {
					remote_ip = val;
				} else if (!strcasecmp(var, "remote-port")) {
					remote_port = atoi(val);
				} else if (!strcasecmp(var, "context")) {
					context = val;
				} else if (!strcasecmp(var, "dialplan")) {
					dialplan = val;
				} 
			}
				
			if (!id && !name) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "span missing required param\n");
				continue;
			}

			if (!tonegroup) {
				tonegroup = "us";
			}
			
			if (name) {
				zstatus = zap_span_find_by_name(name, &span);
			} else {
				if (switch_is_number(id)) {
					span_id = atoi(id);
					zstatus = zap_span_find(span_id, &span);
				}

				if (zstatus != ZAP_SUCCESS) {
					zstatus = zap_span_find_by_name(id, &span);
				}
			}

			if (zstatus != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error finding OpenZAP span id:%s name:%s\n", switch_str_nil(id), switch_str_nil(name));
				continue;
			}
			
			if (!span_id) {
				span_id = span->span_id;
			}

			if (zap_configure_span("ss7_boost", span, on_clear_channel_signal, 
								   "local_ip", local_ip,
								   "local_port", &local_port,
								   "remote_ip", remote_ip,
								   "remote_port", &remote_port,
								   TAG_END) != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error starting OpenZAP span %d error: %s\n", span_id, span->last_error);
				continue;
			}

			SPAN_CONFIG[span->span_id].span = span;
			switch_copy_string(SPAN_CONFIG[span->span_id].context, context, sizeof(SPAN_CONFIG[span->span_id].context));
			switch_copy_string(SPAN_CONFIG[span->span_id].dialplan, dialplan, sizeof(SPAN_CONFIG[span->span_id].dialplan));

			zap_span_start(span);
			switch_copy_string(SPAN_CONFIG[span->span_id].type, "ss7 (boost)", sizeof(SPAN_CONFIG[span->span_id].type));
		}
	}



	switch_xml_free(xml);

	return SWITCH_STATUS_SUCCESS;
}


void dump_chan(zap_span_t *span, uint32_t chan_id, switch_stream_handle_t *stream)
{
	stream->write_function(stream,
						   "span_id: %u\n"
						   "chan_id: %u\n"
						   "physical_span_id: %u\n"
						   "physical_chan_id: %u\n"
						   "type: %s\n"
						   "state: %s\n"
						   "last_state: %s\n"
						   "cid_date: %s\n"
						   "cid_name: %s\n"
						   "cid_num: %s\n"
						   "ani: %s\n"
						   "aniII: %s\n"
						   "dnis: %s\n"
						   "rdnis: %s\n"
						   "cause: %s\n\n",
						   span->channels[chan_id]->span_id,
						   span->channels[chan_id]->chan_id,
						   span->channels[chan_id]->physical_span_id,
						   span->channels[chan_id]->physical_chan_id,
						   zap_chan_type2str(span->channels[chan_id]->type),
						   zap_channel_state2str(span->channels[chan_id]->state),
						   zap_channel_state2str(span->channels[chan_id]->last_state),
						   span->channels[chan_id]->caller_data.cid_date,
						   span->channels[chan_id]->caller_data.cid_name,
						   span->channels[chan_id]->caller_data.cid_num.digits,
						   span->channels[chan_id]->caller_data.ani.digits,
						   span->channels[chan_id]->caller_data.aniII,
						   span->channels[chan_id]->caller_data.dnis.digits,
						   span->channels[chan_id]->caller_data.rdnis.digits,
						   switch_channel_cause2str(span->channels[chan_id]->caller_data.hangup_cause)
						   );
}

#define OZ_SYNTAX "list || dump <span_id> [<chan_id>]"
SWITCH_STANDARD_API(oz_function)
{
	char *mycmd = NULL, *argv[10] = { 0 };
	int argc = 0;

	if (!switch_strlen_zero(cmd) && (mycmd = strdup(cmd))) {
		argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (!argc) {
		stream->write_function(stream, "%s", OZ_SYNTAX);
		goto end;
	}

	if (!strcasecmp(argv[0], "dump")) {
		if (argc < 2) {
			stream->write_function(stream, "-ERR Usage: oz dump <span_id> [<chan_id>]\n");
			goto end;
		} else {
			int32_t span_id, chan_id = 0;
			zap_span_t *span;

			span_id = atoi(argv[1]);

			if (argc > 2) {
				chan_id = atoi(argv[2]);
			}
			
			if (!(span_id && (span = SPAN_CONFIG[span_id].span))) {
				stream->write_function(stream, "-ERR invalid span\n");
			} else {
				if (chan_id) {
					dump_chan(span, chan_id, stream);
				} else {
					int j;
					
					stream->write_function(stream, "+OK\n");
					for(j = 1; j <= span->chan_count; j++) {
						dump_chan(span, j, stream);
					}

				}
			}
		}
	} else if (!strcasecmp(argv[0], "list")) {
		int j;
		for (j = 0 ; j < ZAP_MAX_SPANS_INTERFACE; j++) {
			if (SPAN_CONFIG[j].span) {
				char *flags = "none";

				if (SPAN_CONFIG[j].analog_options & ANALOG_OPTION_3WAY) {
					flags = "3way";
				} else if (SPAN_CONFIG[j].analog_options & ANALOG_OPTION_CALL_SWAP) {
					flags = "call swap";
				}

				stream->write_function(stream,
									   "+OK\n"
									   "span: %u (%s)\n"
									   "type: %s\n"
									   "chan_count: %u\n"
									   "dialplan: %s\n"
									   "context: %s\n"
									   "dial_regex: %s\n"
									   "fail_dial_regex: %s\n"
									   "hold_music: %s\n"
									   "analog_options %s\n",
									   j,
									   SPAN_CONFIG[j].span->name,
									   SPAN_CONFIG[j].type,
									   SPAN_CONFIG[j].span->chan_count,
									   SPAN_CONFIG[j].dialplan,
									   SPAN_CONFIG[j].context,
									   SPAN_CONFIG[j].dial_regex,
									   SPAN_CONFIG[j].fail_dial_regex,
									   SPAN_CONFIG[j].hold_music,
									   flags
									   );
			}
		}
	} else if (!strcasecmp(argv[0], "bounce")) {
		/* MSC testing "oz bounce" command */
		if (argc < 2) {
			stream->write_function(stream, "-ERR Usage: oz dump <span_id> [<chan_id>]\n");
			goto end;
		} else {
			int32_t span_id, chan_id = 0;
			zap_span_t *span;

			span_id = atoi(argv[1]);

			if (argc > 2) {
				chan_id = atoi(argv[2]);
			}
			
			if (!(span_id && (span = SPAN_CONFIG[span_id].span))) {
				stream->write_function(stream, "-ERR invalid span\n");
			} else {
				if (chan_id) {
					//dump_chan(span, chan_id, stream);
					zap_log(ZAP_LOG_INFO,"Bounce span: %d, chan: %d\n", span_id, chan_id);
				} else {
					int j;
					
					stream->write_function(stream, "+OK\n");
					for(j = 1; j <= span->chan_count; j++) {
						//dump_chan(span, j, stream);
						zap_log(ZAP_LOG_INFO,"Bounce span: %d, chan: %d\n", span_id, j);
					}

				}
			}
		}	
	} else {
		stream->write_function(stream, "-ERR Usage: oz list || dump <span_id> [<chan_id>]\n");
	}

 end:

	switch_safe_free(mycmd);

	return SWITCH_STATUS_SUCCESS;
}


SWITCH_STANDARD_APP(disable_ec_function)
{
	private_t *tech_pvt;
	int x = 0;

	if (!switch_core_session_check_interface(session, openzap_endpoint_interface)) {
		zap_log(ZAP_LOG_ERROR, "This application is only for OpenZAP channels.\n");
		return;
	}
	
	tech_pvt = switch_core_session_get_private(session);
	zap_channel_command(tech_pvt->zchan, ZAP_COMMAND_DISABLE_ECHOCANCEL, &x);
	zap_channel_command(tech_pvt->zchan, ZAP_COMMAND_DISABLE_ECHOTRAIN, &x);
	zap_log(ZAP_LOG_ERROR, "Echo Canceller Disabled\n");
}


SWITCH_MODULE_LOAD_FUNCTION(mod_openzap_load)
{

	switch_api_interface_t *commands_api_interface;
	switch_application_interface_t *app_interface;

	module_pool = pool;

	zap_global_set_logger(zap_logger);
	
	if (zap_global_init() != ZAP_SUCCESS) {
		zap_log(ZAP_LOG_ERROR, "Error loading OpenZAP\n");
		return SWITCH_STATUS_TERM;
	}

	if (load_config() != SWITCH_STATUS_SUCCESS) {
		zap_global_destroy();
		return SWITCH_STATUS_TERM;
	}

	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	openzap_endpoint_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_ENDPOINT_INTERFACE);
	openzap_endpoint_interface->interface_name = "openzap";
	openzap_endpoint_interface->io_routines = &openzap_io_routines;
	openzap_endpoint_interface->state_handler = &openzap_state_handlers;
	
	SWITCH_ADD_API(commands_api_interface, "oz", "OpenZAP commands", oz_function, OZ_SYNTAX);

	SWITCH_ADD_APP(app_interface, "disable_ec", "Disable Echo Canceller", "Disable Echo Canceller", disable_ec_function, "", SAF_NONE);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_openzap_shutdown)
{
	zap_global_destroy();
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
