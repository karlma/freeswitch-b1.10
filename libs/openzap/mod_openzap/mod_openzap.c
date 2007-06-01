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
#include "zap_analog.h"
#include "zap_isdn.h"

static const char modname[] = "mod_openzap";

static switch_memory_pool_t *module_pool = NULL;
static int running = 1;

struct span_config {
	zap_span_t *span;
	char dialplan[80];
	char context[80];
};

static struct span_config SPAN_CONFIG[ZAP_MAX_SPANS_INTERFACE] = {0};

typedef enum {
	TFLAG_IO = (1 << 0),
	TFLAG_DTMF = (1 << 1),
	TFLAG_CODEC = (1 << 2),
	TFLAG_BREAK = (1 << 3),
	TFLAG_HOLD = (1 << 4)
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
	switch_mutex_t *mutex;
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
	int32_t token_id;
	uint32_t wr_error;
};

typedef struct private_object private_t;


static switch_status_t channel_on_init(switch_core_session_t *session);
static switch_status_t channel_on_hangup(switch_core_session_t *session);
static switch_status_t channel_on_ring(switch_core_session_t *session);
static switch_status_t channel_on_loopback(switch_core_session_t *session);
static switch_status_t channel_on_transmit(switch_core_session_t *session);
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool);
static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, int timeout, switch_io_flag_t flags, int stream_id);
static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, int timeout, switch_io_flag_t flags, int stream_id);
static switch_status_t channel_kill_channel(switch_core_session_t *session, int sig);


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

	switch_set_flag_locked(tech_pvt, TFLAG_IO);
	
	/* Move Channel's State Machine to RING */
	switch_channel_set_state(channel, CS_RING);
	switch_mutex_lock(globals.mutex);
	globals.calls++;
	switch_mutex_unlock(globals.mutex);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_ring(switch_core_session_t *session)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s CHANNEL RING\n", switch_channel_get_name(channel));

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

	zap_channel_clear_token(tech_pvt->zchan, tech_pvt->token_id);
	
	switch (tech_pvt->zchan->type) {
	case ZAP_CHAN_TYPE_FXO:
		{
			if (tech_pvt->zchan->state != ZAP_CHANNEL_STATE_DOWN) {
				zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_HANGUP);
			}
		}
		break;
	case ZAP_CHAN_TYPE_FXS:
		{
			if (tech_pvt->zchan->state != ZAP_CHANNEL_STATE_DOWN) {
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
		break;
	case SWITCH_SIG_BREAK:
		switch_set_flag_locked(tech_pvt, TFLAG_BREAK);
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_loopback(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CHANNEL LOOPBACK\n");
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_on_transmit(switch_core_session_t *session)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "CHANNEL TRANSMIT\n");
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_waitfor_read(switch_core_session_t *session, int ms, int stream_id)
{
	private_t *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_waitfor_write(switch_core_session_t *session, int ms, int stream_id)
{
	private_t *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	return SWITCH_STATUS_SUCCESS;

}

static switch_status_t channel_send_dtmf(switch_core_session_t *session, char *dtmf)
{
	private_t *tech_pvt = NULL;

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	zap_channel_command(tech_pvt->zchan, ZAP_COMMAND_SEND_DTMF, dtmf);
		
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, int timeout, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	uint32_t len;
	zap_wait_flag_t wflags = ZAP_READ;
	char dtmf[128] = "";
	zap_status_t status;
	int total_to = timeout;
	int chunk, do_break = 0;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	assert(tech_pvt->zchan != NULL);

	chunk = tech_pvt->zchan->effective_interval * 2;

 top:

	if (switch_test_flag(tech_pvt, TFLAG_BREAK)) {
		switch_clear_flag_locked(tech_pvt, TFLAG_BREAK);
		do_break = 1;
	}

	if (switch_test_flag(tech_pvt, TFLAG_HOLD) || do_break) {
		switch_yield(tech_pvt->zchan->effective_interval * 1000);
		*frame = &tech_pvt->cng_frame;
		tech_pvt->cng_frame.datalen = tech_pvt->zchan->packet_len;
		tech_pvt->cng_frame.samples = tech_pvt->cng_frame.datalen;
		if (tech_pvt->zchan->effective_codec == ZAP_CODEC_SLIN) {
			tech_pvt->cng_frame.samples /= 2;
		}
		return SWITCH_STATUS_SUCCESS;
	}
	
	if (!switch_test_flag(tech_pvt, TFLAG_IO)) {
		return SWITCH_STATUS_FALSE;
	}

	status = zap_channel_wait(tech_pvt->zchan, &wflags, chunk);

	if (status == ZAP_FAIL) {
		return SWITCH_STATUS_GENERR;
	}
	
	if (status == ZAP_TIMEOUT) {
		if (timeout > 0 && !switch_test_flag(tech_pvt, TFLAG_HOLD)) {
			total_to -= chunk;
			if (total_to <= 0) {
				return SWITCH_STATUS_BREAK;
			}
		}

		goto top;
	}

	if (!(wflags & ZAP_READ)) {
		return SWITCH_STATUS_GENERR;
	}

	len = tech_pvt->read_frame.buflen;
	if (zap_channel_read(tech_pvt->zchan, tech_pvt->read_frame.data, &len) != ZAP_SUCCESS) {
		return SWITCH_STATUS_GENERR;
	}

	*frame = &tech_pvt->read_frame;
	tech_pvt->read_frame.datalen = len;
	tech_pvt->read_frame.samples = tech_pvt->read_frame.datalen;

	if (tech_pvt->zchan->effective_codec == ZAP_CODEC_SLIN) {
		tech_pvt->read_frame.samples /= 2;
	}

	if (zap_channel_dequeue_dtmf(tech_pvt->zchan, dtmf, sizeof(dtmf))) {
		switch_channel_queue_dtmf(channel, dtmf);
	}

	return SWITCH_STATUS_SUCCESS;

}

static switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, int timeout, switch_io_flag_t flags, int stream_id)
{
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	zap_size_t len;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	tech_pvt = switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	assert(tech_pvt->zchan != NULL);

	if (switch_test_flag(tech_pvt, TFLAG_HOLD)) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (!switch_test_flag(tech_pvt, TFLAG_IO)) {
		return SWITCH_STATUS_FALSE;
	}
	
	len = frame->datalen;
	if (zap_channel_write(tech_pvt->zchan, frame->data, frame->buflen, &len) != ZAP_SUCCESS) {
		if (++tech_pvt->wr_error > 10) {
			return SWITCH_STATUS_GENERR;
		}
	} else {
		tech_pvt->wr_error = 0;
	}

	return SWITCH_STATUS_SUCCESS;

}

static switch_status_t channel_receive_message_b(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	return SWITCH_STATUS_FALSE;
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
		if (!switch_channel_test_flag(channel, CF_OUTBOUND)) {
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
			zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_UP);
		}
		break;
	case SWITCH_MESSAGE_INDICATE_RINGING:
		zap_set_state_locked(tech_pvt->zchan, ZAP_CHANNEL_STATE_RING);
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t channel_receive_message(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	private_t *tech_pvt;
			
	tech_pvt = (private_t *) switch_core_session_get_private(session);
	assert(tech_pvt != NULL);

	switch (tech_pvt->zchan->type) {
	case ZAP_CHAN_TYPE_FXS:
		return channel_receive_message_fxs(session, msg);
	case ZAP_CHAN_TYPE_FXO:
		return channel_receive_message_fxo(session, msg);
	case ZAP_CHAN_TYPE_B:
		return channel_receive_message_b(session, msg);
	default:
		return SWITCH_STATUS_FALSE;
	}
}

static const switch_state_handler_table_t channel_event_handlers = {
	/*.on_init */ channel_on_init,
	/*.on_ring */ channel_on_ring,
	/*.on_execute */ channel_on_execute,
	/*.on_hangup */ channel_on_hangup,
	/*.on_loopback */ channel_on_loopback,
	/*.on_transmit */ channel_on_transmit
};

static const switch_io_routines_t channel_io_routines = {
	/*.outgoing_channel */ channel_outgoing_channel,
	/*.read_frame */ channel_read_frame,
	/*.write_frame */ channel_write_frame,
	/*.kill_channel */ channel_kill_channel,
	/*.waitfor_read */ channel_waitfor_read,
	/*.waitfor_write */ channel_waitfor_write,
	/*.send_dtmf */ channel_send_dtmf,
	/*.receive_message*/ channel_receive_message
};

static const switch_endpoint_interface_t channel_endpoint_interface = {
	/*.interface_name */ "openzap",
	/*.io_routines */ &channel_io_routines,
	/*.event_handlers */ &channel_event_handlers,
	/*.private */ NULL,
	/*.next */ NULL
};

static const switch_loadable_module_interface_t channel_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ &channel_endpoint_interface,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ NULL,
	/*.application_interface */ NULL
};


/* Make sure when you have 2 sessions in the same scope that you pass the appropriate one to the routines
that allocate memory or you will have 1 channel with memory allocated from another channel's pool!
*/
static switch_call_cause_t channel_outgoing_channel(switch_core_session_t *session,
													switch_caller_profile_t *outbound_profile,
													switch_core_session_t **new_session, switch_memory_pool_t **pool)
{

	char *p, *dest;
	int span_id = 0, chan_id = 0;
	zap_channel_t *zchan = NULL;
	switch_call_cause_t cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	char name[128];
	zap_status_t status;

	if (!outbound_profile) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing caller profile\n");
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	if ((p = strchr(outbound_profile->destination_number, '/'))) {
		dest = p + 1;
		span_id = atoi(outbound_profile->destination_number);
		if ((p = strchr(dest, '/'))) {
			chan_id = atoi(dest);
			dest = p + 1;
		}
	}

	if (!span_id) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing span\n");
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}
	
	if (chan_id) {
		status = zap_channel_open(span_id, chan_id, &zchan);
	} else {
		status = zap_channel_open_any(span_id, ZAP_TOP_DOWN, &zchan);
		
	}

	if (status != ZAP_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No channels available\n");
		return SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
	}

	if ((*new_session = switch_core_session_request(&channel_endpoint_interface, pool)) != 0) {
		private_t *tech_pvt;
		switch_channel_t *channel;
		switch_caller_profile_t *caller_profile;

		switch_core_session_add_stream(*new_session, NULL);
		if ((tech_pvt = (private_t *) switch_core_session_alloc(*new_session, sizeof(private_t))) != 0) {
			channel = switch_core_session_get_channel(*new_session);
			tech_init(tech_pvt, *new_session, zchan);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Hey where is my memory pool?\n");
			switch_core_session_destroy(new_session);
			cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
			goto fail;
		}

		snprintf(name, sizeof(name), "OPENZAP/%s", dest);
		switch_channel_set_name(channel, name);
		zap_set_string(zchan->caller_data.ani, dest);
		caller_profile = switch_caller_profile_clone(*new_session, outbound_profile);
		switch_channel_set_caller_profile(channel, caller_profile);
		tech_pvt->caller_profile = caller_profile;

		
		switch_channel_set_flag(channel, CF_OUTBOUND);
		switch_channel_set_state(channel, CS_INIT);
		if ((tech_pvt->token_id = zap_channel_add_token(zchan, switch_core_session_get_uuid(*new_session))) < 0) {
			switch_core_session_destroy(new_session);
			cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;
            goto fail;
		}

		zap_channel_outgoing_call(zchan);
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
	
	if (!(session = switch_core_session_request(&channel_endpoint_interface, NULL))) {
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
		
	tech_pvt->caller_profile = switch_caller_profile_new(switch_core_session_get_pool(session),
														 "OpenZAP",
														 SPAN_CONFIG[sigmsg->channel->span_id].dialplan,
														 sigmsg->channel->chan_name,
														 sigmsg->channel->chan_number,
														 NULL,
														 sigmsg->channel->chan_number,
														 NULL,
														 NULL,
														 (char *) modname,
														 SPAN_CONFIG[sigmsg->channel->span_id].context,
														 sigmsg->channel->caller_data.dnis);
	assert(tech_pvt->caller_profile != NULL);
		
	snprintf(name, sizeof(name), "OpenZAP/%s", tech_pvt->caller_profile->destination_number);
	switch_channel_set_name(channel, name);
	switch_channel_set_caller_profile(channel, tech_pvt->caller_profile);
		
	switch_channel_set_state(channel, CS_INIT);
	if (switch_core_session_thread_launch(session) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Error spawning thread\n");
		switch_core_session_destroy(&session);
		return ZAP_FAIL;
	}

	if ((tech_pvt->token_id = zap_channel_add_token(sigmsg->channel, switch_core_session_get_uuid(session))) < 0) {
		switch_core_session_destroy(&session);
		return ZAP_FAIL;
	}
	*sp = session;

    return ZAP_SUCCESS;
}

static switch_core_session_t *zap_channel_get_session(zap_channel_t *channel, int32_t id)
{
	switch_core_session_t *session = NULL;

	if (id > ZAP_MAX_TOKENS) {
		return NULL;
	}

	if (!switch_strlen_zero(channel->tokens[id])) {
		session = switch_core_session_locate(channel->tokens[id]);
	}

	return session;
}

static ZIO_SIGNAL_CB_FUNCTION(on_fxo_signal)
{
	switch_core_session_t *session = NULL;
	switch_channel_t *channel = NULL;
	zap_status_t status;

	zap_log(ZAP_LOG_DEBUG, "got fxo sig [%s]\n", zap_signal_event2str(sigmsg->event_id));

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
    case ZAP_SIGEVENT_START:
		{
			status = zap_channel_from_event(sigmsg, &session);
			if (status != ZAP_SUCCESS) {
				zap_set_state_locked(sigmsg->channel, ZAP_CHANNEL_STATE_DOWN);
			}
		}
		break;
	}

	return ZAP_SUCCESS;
}

static ZIO_SIGNAL_CB_FUNCTION(on_fxs_signal)
{
	switch_core_session_t *session = NULL;
	switch_channel_t *channel = NULL;
	private_t *tech_pvt = NULL;
	zap_status_t status;

    zap_log(ZAP_LOG_DEBUG, "got fxs sig [%s]\n", zap_signal_event2str(sigmsg->event_id));

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
    case ZAP_SIGEVENT_START:
		{
			status = zap_channel_from_event(sigmsg, &session);
			if (status != ZAP_SUCCESS) {
				zap_set_state_locked(sigmsg->channel, ZAP_CHANNEL_STATE_BUSY);
			}
		}
		break;
    case ZAP_SIGEVENT_STOP:
		{
			while((session = zap_channel_get_session(sigmsg->channel, 0))) {
				zap_channel_clear_token(sigmsg->channel, 0);
				channel = switch_core_session_get_channel(session);
				switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
				switch_core_session_rwunlock(session);
			}
		}
		break;

    case ZAP_SIGEVENT_FLASH:
		{
			uint32_t i = 0;
			for (i = 0; i < sigmsg->channel->token_count; i++) {
				if ((session = zap_channel_get_session(sigmsg->channel, i))) {
					tech_pvt = switch_core_session_get_private(session);
					if (sigmsg->channel->token_count == 1) {
						if (switch_test_flag(tech_pvt, TFLAG_HOLD)) {
							switch_clear_flag_locked(tech_pvt, TFLAG_HOLD);
						} else {
							switch_set_flag_locked(tech_pvt, TFLAG_HOLD);
						}
					} else if (i) {
						switch_set_flag_locked(tech_pvt, TFLAG_HOLD);
					} else {
						switch_clear_flag_locked(tech_pvt, TFLAG_HOLD);
					}
					switch_core_session_rwunlock(session);
				}
			}
		}
		break;
	}

	return status;
}

static ZIO_SIGNAL_CB_FUNCTION(on_zap_signal)
{
	switch (sigmsg->channel->type) {
	case ZAP_CHAN_TYPE_FXO:
		{
			on_fxo_signal(sigmsg);
		}
		break;
	case ZAP_CHAN_TYPE_FXS:
		{
			on_fxs_signal(sigmsg);
		}
		break;
	default: 
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unhandled type for channel %d:%d\n",
							  sigmsg->channel->span_id, sigmsg->channel->chan_id);
		}
		break;
	}
}

static void zap_logger(char *file, const char *func, int line, int level, char *fmt, ...)
{
    char *data = NULL;
    va_list ap;
	
    va_start(ap, fmt);

	if (switch_vasprintf(&data, fmt, ap) != -1) {
		switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, level, data);
		free(data);
	}
	
    va_end(ap);

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
			}
		}
	}

	if ((spans = switch_xml_child(cfg, "analog_spans"))) {
		for (myspan = switch_xml_child(spans, "span"); myspan; myspan = myspan->next) {
			char *id = (char *) switch_xml_attr_soft(myspan, "id");
			char *context = "default";
			char *dialplan = "XML";
			char *tonegroup = NULL;
			char *digit_timeout = NULL;
			char *max_digits = NULL;
			uint32_t span_id = 0, to = 0, max = 0;
			zap_span_t *span = NULL;

			for (param = switch_xml_child(myspan, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "tonegroup")) {
					tonegroup = val;
				} else if (!strcasecmp(var, "digit_timeout")) {
					digit_timeout = val;
				} else if (!strcasecmp(var, "context")) {
					context = val;
				} else if (!strcasecmp(var, "dialplan")) {
					dialplan = val;
				} else if (!strcasecmp(var, "max_digits")) {
					digit_timeout = val;
				}
			}
				
			if (!id) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "span missing required param 'id'\n");
			}

			span_id = atoi(id);
			
			if (!tonegroup) {
				tonegroup = "us";
			}
			
			if (digit_timeout) {
				to = atoi(digit_timeout);
			}

			if (max_digits) {
				max = atoi(max_digits);
			}

			if (zap_span_find(span_id, &span) != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error finding OpenZAP span %d\n", span_id);
				continue;
			}

			if (zap_analog_configure_span(span, tonegroup, to, max, on_zap_signal) != ZAP_SUCCESS) {
				zap_log(ZAP_LOG_ERROR, "Error starting OpenZAP span %d\n", span_id);
				continue;
			}

			SPAN_CONFIG[span->span_id].span = span;
			switch_copy_string(SPAN_CONFIG[span->span_id].context, context, sizeof(SPAN_CONFIG[span->span_id].context));
			switch_copy_string(SPAN_CONFIG[span->span_id].dialplan, dialplan, sizeof(SPAN_CONFIG[span->span_id].dialplan));

			
			zap_analog_start(span);
		}
	}


	switch_xml_free(xml);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface, char *filename)
{

	if (switch_core_new_memory_pool(&module_pool) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "OH OH no pool\n");
		return SWITCH_STATUS_TERM;
	}

	zap_global_set_logger(zap_logger);
	
	if (zap_global_init() != ZAP_SUCCESS) {
		zap_log(ZAP_LOG_ERROR, "Error loading OpenZAP\n");
		return SWITCH_STATUS_TERM;
	}

	if (load_config() != SWITCH_STATUS_SUCCESS) {
		zap_global_destroy();
		return SWITCH_STATUS_TERM;
	}

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = &channel_module_interface;

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
