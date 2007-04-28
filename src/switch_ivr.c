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
 * Paul D. Tinsley <pdt at jackhammer.org>
 * Neal Horman <neal at wanlink dot com>
 * Matt Klein <mklein@nmedia.net>
 * Michael Jerris <mike@jerris.com>
 *
 * switch_ivr.c -- IVR Library
 *
 */
#include <switch.h>
#include <switch_ivr.h>



SWITCH_DECLARE(switch_status_t) switch_ivr_sleep(switch_core_session_t *session, uint32_t ms)
{
	switch_channel_t *channel;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_time_t start, now, done = switch_time_now() + (ms * 1000);
	switch_frame_t *read_frame;
	int32_t left, elapsed;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	start = switch_time_now();

	for (;;) {
		now = switch_time_now();
		elapsed = (int32_t) ((now - start) / 1000);
		left = ms - elapsed;

		if (!switch_channel_ready(channel)) {
			status = SWITCH_STATUS_FALSE;
			break;
		}

		if (now > done || left <= 0) {
			break;
		}

		if (switch_channel_test_flag(channel, CF_SERVICE) ||
			(!switch_channel_test_flag(channel, CF_ANSWERED) && !switch_channel_test_flag(channel, CF_EARLY_MEDIA))) {
			switch_yield(1000);
		} else {
			status = switch_core_session_read_frame(session, &read_frame, left, 0);
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
		}
	}


	return status;
}


static void *SWITCH_THREAD_FUNC unicast_thread_run(switch_thread_t *thread, void *obj)
{
	switch_unicast_conninfo_t *conninfo = (switch_unicast_conninfo_t *) obj;
	switch_size_t len;
	switch_channel_t *channel;

	if (!conninfo) {
		return NULL;
	}

	channel = switch_core_session_get_channel(conninfo->session);
	
	while(switch_test_flag(conninfo, SUF_READY) && switch_test_flag(conninfo, SUF_THREAD_RUNNING)) {
		len = conninfo->write_frame.buflen;
		if (switch_socket_recv(conninfo->socket, conninfo->write_frame.data, &len) != SWITCH_STATUS_SUCCESS || len == 0) {
			break;
		}
		conninfo->write_frame.datalen = (uint32_t)len;
		conninfo->write_frame.samples = conninfo->write_frame.datalen / 2;
		switch_core_session_write_frame(conninfo->session, &conninfo->write_frame, -1, conninfo->stream_id);
	}
	
	switch_clear_flag_locked(conninfo, SUF_READY);
	switch_clear_flag_locked(conninfo, SUF_THREAD_RUNNING);

	return NULL;
}

static void unicast_thread_launch(switch_unicast_conninfo_t *conninfo)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;

	switch_threadattr_create(&thd_attr, switch_core_session_get_pool(conninfo->session));
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_set_flag_locked(conninfo, SUF_THREAD_RUNNING);
	switch_thread_create(&thread, thd_attr, unicast_thread_run, conninfo, switch_core_session_get_pool(conninfo->session));
}

SWITCH_DECLARE(switch_status_t) switch_ivr_deactivate_unicast(switch_core_session_t *session)
{
	switch_channel_t *channel;
	switch_unicast_conninfo_t *conninfo;
	int sanity = 0;
	
	channel = switch_core_session_get_channel(session);
    assert(channel != NULL);

	if (switch_channel_test_flag(channel, CF_UNICAST)) {
		if ((conninfo = switch_channel_get_private(channel, "unicast"))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Shutting down unicast connection\n");
			switch_clear_flag_locked(conninfo, SUF_READY);
			switch_socket_shutdown(conninfo->socket, SWITCH_SHUTDOWN_READWRITE);
			while(switch_test_flag(conninfo, SUF_THREAD_RUNNING)) {
				switch_yield(10000);
				if (++sanity >= 10000) {
					break;
				}
			}
			if (conninfo->read_codec.implementation) {
				switch_core_codec_destroy(&conninfo->read_codec);
			}
			switch_socket_close(conninfo->socket);
		}
		switch_channel_clear_flag(channel, CF_UNICAST);
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
	
}

SWITCH_DECLARE(switch_status_t) switch_ivr_activate_unicast(switch_core_session_t *session, 
															char *local_ip,
															switch_port_t local_port,
															char *remote_ip,
															switch_port_t remote_port,
															char *transport,
															char *flags)
{
	switch_channel_t *channel;
	switch_unicast_conninfo_t *conninfo;
	switch_codec_t *read_codec;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	conninfo = switch_core_session_alloc(session, sizeof(*conninfo));
	assert(conninfo != NULL);

	conninfo->local_ip = switch_core_session_strdup(session, local_ip);
	conninfo->local_port = local_port;

	conninfo->remote_ip = switch_core_session_strdup(session, remote_ip);
	conninfo->remote_port = remote_port;
	conninfo->session = session;
	
	if (!strcasecmp(transport, "udp")) {
		conninfo->type = AF_INET;
		conninfo->transport = SOCK_DGRAM;
	} else if (!strcasecmp(transport, "tcp")) {
		conninfo->type = AF_INET;
		conninfo->transport = SOCK_STREAM;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid transport %s\n", transport);
		goto fail;
	}

	if (flags) {
		if (strstr(flags, "native")) {
			switch_set_flag(conninfo, SUF_NATIVE);
		}
	}

	switch_mutex_init(&conninfo->flag_mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
	
	read_codec = switch_core_session_get_read_codec(session);
	
	if (!switch_test_flag(conninfo, SUF_NATIVE)) {
		if (switch_core_codec_init(&conninfo->read_codec,
								   "L16",
								   NULL,
								   read_codec->implementation->samples_per_second,
								   read_codec->implementation->microseconds_per_frame / 1000,
								   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
								   NULL, switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
							  "Raw Codec Activation Success L16@%uhz 1 channel %dms\n",
							  read_codec->implementation->samples_per_second, read_codec->implementation->microseconds_per_frame / 1000);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Raw Codec Activation Failed L16@%uhz 1 channel %dms\n",
							  read_codec->implementation->samples_per_second, read_codec->implementation->microseconds_per_frame / 1000);
			goto fail;
		}
	}

	conninfo->write_frame.data = conninfo->write_frame_data;
	conninfo->write_frame.buflen = sizeof(conninfo->write_frame_data);
	conninfo->write_frame.codec = switch_test_flag(conninfo, SUF_NATIVE) ? read_codec : &conninfo->read_codec;
	

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "connect %s:%d->%s:%d\n",
					  conninfo->local_ip, conninfo->local_port, conninfo->remote_ip, conninfo->remote_port);

	if (switch_sockaddr_info_get(&conninfo->local_addr,
								 conninfo->local_ip, SWITCH_UNSPEC, conninfo->local_port, 0, 
								 switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		goto fail;
	}
	
	if (switch_sockaddr_info_get(&conninfo->remote_addr, 
								 conninfo->remote_ip, SWITCH_UNSPEC, conninfo->remote_port, 0,
								 switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		goto fail;
	}
	
	if (switch_socket_create(&conninfo->socket, AF_INET, SOCK_DGRAM, 0, switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		if (switch_socket_bind(conninfo->socket, conninfo->local_addr) != SWITCH_STATUS_SUCCESS) {
			goto fail;
		}
	} else {
		goto fail;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Created unicast connection %s:%d->%s:%d\n",
					  conninfo->local_ip, conninfo->local_port, conninfo->remote_ip, conninfo->remote_port);
	switch_channel_set_private(channel, "unicast", conninfo);
	switch_channel_set_flag(channel, CF_UNICAST);
	switch_set_flag_locked(conninfo, SUF_READY);
	return SWITCH_STATUS_SUCCESS;

 fail:

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Failure creating unicast connection %s:%d->%s:%d\n",
					  conninfo->local_ip, conninfo->local_port, conninfo->remote_ip, conninfo->remote_port);
	return SWITCH_STATUS_FALSE;	


}

SWITCH_DECLARE(switch_status_t) switch_ivr_parse_event(switch_core_session_t *session, switch_event_t *event)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	char *cmd = switch_event_get_header(event, "call-command");
	unsigned long cmd_hash;
	switch_ssize_t hlen = SWITCH_HASH_KEY_STRING;
	unsigned long CMD_EXECUTE = switch_hashfunc_default("execute", &hlen);
	unsigned long CMD_HANGUP = switch_hashfunc_default("hangup", &hlen);
	unsigned long CMD_NOMEDIA = switch_hashfunc_default("nomedia", &hlen);
	unsigned long CMD_UNICAST = switch_hashfunc_default("unicast", &hlen);

	assert(channel != NULL);
	assert(event != NULL);

	if (switch_strlen_zero(cmd)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Command!\n");
		return SWITCH_STATUS_FALSE;
	}

	hlen = (switch_size_t) strlen(cmd);
	cmd_hash = switch_hashfunc_default(cmd, &hlen);

	switch_channel_set_flag(channel, CF_EVENT_PARSE);


	if (cmd_hash == CMD_EXECUTE) {
		const switch_application_interface_t *application_interface;
		char *app_name = switch_event_get_header(event, "execute-app-name");
		char *app_arg = switch_event_get_header(event, "execute-app-arg");
		char *loop_h = switch_event_get_header(event, "loops");
		int loops = 1;
	
		if (loop_h) {
			loops = atoi(loop_h);
		}

		if (app_name && app_arg) {
			if ((application_interface = switch_loadable_module_get_application_interface(app_name))) {
				if (application_interface->application_function) {
					int x;
					switch_channel_set_flag(channel, CF_BROADCAST);
					for (x = 0; x < loops || loops < 0; x++) {
						switch_core_session_exec(session, application_interface, app_arg);
						if (!switch_channel_test_flag(channel, CF_BROADCAST)) {
							break;
						}
					}
					switch_channel_clear_flag(channel, CF_BROADCAST);					
				}
			}
		}
	} else if (cmd_hash == CMD_UNICAST) {
		char *local_ip = switch_event_get_header(event, "local_ip");
		char *local_port = switch_event_get_header(event, "local_port");
		char *remote_ip = switch_event_get_header(event, "remote_ip");
		char *remote_port = switch_event_get_header(event, "remote_port");
		char *transport = switch_event_get_header(event, "transport");
		char *flags = switch_event_get_header(event, "flags");

		if (switch_strlen_zero(local_ip)) {
			local_ip = "127.0.0.1";
		}
		if (switch_strlen_zero(remote_ip)) {
			remote_ip = "127.0.0.1";
		}
		if (switch_strlen_zero(local_port)) {
			local_port = "8025";
		}
		if (switch_strlen_zero(remote_port)) {
			remote_port = "8026";
		}
		if (switch_strlen_zero(transport)) {
			transport = "udp";
		}

		switch_ivr_activate_unicast(session, local_ip, (switch_port_t)atoi(local_port), remote_ip, (switch_port_t)atoi(remote_port), transport, flags);

	} else if (cmd_hash == CMD_HANGUP) {
		char *cause_name = switch_event_get_header(event, "hangup-cause");
		switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;

		if (cause_name) {
			cause = switch_channel_str2cause(cause_name);
		}

		switch_channel_hangup(channel, cause);
	} else if (cmd_hash == CMD_NOMEDIA) {
		char *uuid = switch_event_get_header(event, "nomedia-uuid");
		switch_ivr_nomedia(uuid, SMF_REBRIDGE);
	}


	switch_channel_clear_flag(channel, CF_EVENT_PARSE);
	return SWITCH_STATUS_SUCCESS;

}

SWITCH_DECLARE(switch_status_t) switch_ivr_park(switch_core_session_t *session, switch_input_args_t *args)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_channel_t *channel;
	switch_frame_t *read_frame;
	int stream_id = 0;
	switch_event_t *event;
	switch_unicast_conninfo_t *conninfo = NULL;
	switch_codec_t *read_codec = switch_core_session_get_read_codec(session);
	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_channel_answer(channel);

	if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_PARK) == SWITCH_STATUS_SUCCESS) {
		switch_channel_event_set_data(channel, event);
		switch_event_fire(&event);
	}

	switch_channel_set_flag(channel, CF_CONTROLLED);
	while (switch_channel_ready(channel) && switch_channel_test_flag(channel, CF_CONTROLLED)) {

		if ((status = switch_core_session_read_frame(session, &read_frame, -1, stream_id)) == SWITCH_STATUS_SUCCESS) {
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
			
			if (switch_channel_test_flag(channel, CF_UNICAST)) {
				if (!conninfo) {
					if (!(conninfo = switch_channel_get_private(channel, "unicast"))) {
						switch_channel_clear_flag(channel, CF_UNICAST);
					}
					
					if (conninfo) {
						unicast_thread_launch(conninfo);
					}
				}
				
				if (conninfo) {
					switch_size_t len = 0;
					uint32_t flags = 0;
					switch_byte_t decoded[SWITCH_RECOMMENDED_BUFFER_SIZE];
					uint32_t rate = read_codec->implementation->samples_per_second;
					uint32_t dlen = sizeof(decoded);
					switch_status_t status;
					switch_byte_t *sendbuf = NULL;
					uint32_t sendlen = 0;

					if (switch_test_flag(read_frame, SFF_CNG)) {
						sendlen = read_codec->implementation->bytes_per_frame;
						memset(decoded, 255, sendlen);
						sendbuf = decoded;
						status = SWITCH_STATUS_SUCCESS;
					} else {
						if (switch_test_flag(conninfo, SUF_NATIVE)) {
							status = SWITCH_STATUS_NOOP;
						} else {
							status = switch_core_codec_decode(
															  read_codec,
															  &conninfo->read_codec,
															  read_frame->data,
															  read_frame->datalen,
															  read_codec->implementation->samples_per_second,
															  decoded, &dlen, &rate, &flags);
						}
						switch (status) {
						case SWITCH_STATUS_NOOP:
						case SWITCH_STATUS_BREAK:
							sendbuf = read_frame->data;
							sendlen = read_frame->datalen;
							status = SWITCH_STATUS_SUCCESS;
							break;
						case SWITCH_STATUS_SUCCESS:
							sendbuf = decoded;
							sendlen = dlen;
							status = SWITCH_STATUS_SUCCESS;
							break;
						default:
							switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Codec Error\n");
							switch_ivr_deactivate_unicast(session);
							break;
						}
					}

					if (status == SWITCH_STATUS_SUCCESS) {
						len = sendlen;
						if (switch_socket_sendto(conninfo->socket, conninfo->remote_addr, 0, (void *)sendbuf, &len) != SWITCH_STATUS_SUCCESS) {
							switch_ivr_deactivate_unicast(session);
						}
					}
				}
			}
			
			if (switch_core_session_dequeue_private_event(session, &event) == SWITCH_STATUS_SUCCESS) {
				switch_ivr_parse_event(session, event);
				switch_channel_event_set_data(switch_core_session_get_channel(session), event);
				switch_event_fire(&event);
			}

			if (switch_channel_has_dtmf(channel)) {
				char dtmf[128];
				switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
				if (args && args->input_callback) {
					if ((status = args->input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, args->buf, args->buflen)) != SWITCH_STATUS_SUCCESS) {
						break;
					}
				}
			}

			if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
				if (args && args->input_callback) {
					if ((status = args->input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, args->buf, args->buflen)) != SWITCH_STATUS_SUCCESS) {
						break;
					}
				} else {
					switch_channel_event_set_data(channel, event);
					switch_event_fire(&event);
				}
			}
		}

	}
	switch_channel_clear_flag(channel, CF_CONTROLLED);

	if (switch_event_create(&event, SWITCH_EVENT_CHANNEL_UNPARK) == SWITCH_STATUS_SUCCESS) {
		switch_channel_event_set_data(channel, event);
		switch_event_fire(&event);
	}

	if (switch_channel_test_flag(channel, CF_UNICAST)) {
		switch_ivr_deactivate_unicast(session);
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_collect_digits_callback(switch_core_session_t *session, switch_input_args_t *args, uint32_t timeout)
{
	switch_channel_t *channel;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_time_t started = 0;
	uint32_t elapsed;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (!args->input_callback) {
		return SWITCH_STATUS_GENERR;
	}

	if (timeout) {
		started = switch_time_now();
	}

	while (switch_channel_ready(channel)) {
		switch_frame_t *read_frame;
		switch_event_t *event;
		char dtmf[128];

		if (timeout) {
			elapsed = (uint32_t) ((switch_time_now() - started) / 1000);
			if (elapsed >= timeout) {
				break;
			}
		}

		if (switch_core_session_dequeue_private_event(session, &event) == SWITCH_STATUS_SUCCESS) {
			switch_ivr_parse_event(session, event);
			switch_event_destroy(&event);
		}

		if (switch_channel_has_dtmf(channel)) {
			switch_channel_dequeue_dtmf(channel, dtmf, sizeof(dtmf));
			status = args->input_callback(session, dtmf, SWITCH_INPUT_TYPE_DTMF, args->buf, args->buflen);
		}

		if (switch_core_session_dequeue_event(session, &event) == SWITCH_STATUS_SUCCESS) {
			status = args->input_callback(session, event, SWITCH_INPUT_TYPE_EVENT, args->buf, args->buflen);
			switch_event_destroy(&event);
		}

		if (status != SWITCH_STATUS_SUCCESS) {
			break;
		}

		if (switch_channel_test_flag(channel, CF_SERVICE)) {
			switch_yield(1000);
		} else {
			status = switch_core_session_read_frame(session, &read_frame, -1, 0);
		}

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}
	}

	return status;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_collect_digits_count(switch_core_session_t *session,
																char *buf,
																uint32_t buflen,
																uint32_t maxdigits, const char *terminators, char *terminator, uint32_t timeout)
{
	uint32_t i = 0, x = (uint32_t) strlen(buf);
	switch_channel_t *channel;
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_time_t started = 0;
	uint32_t elapsed;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (terminator != NULL)
		*terminator = '\0';

	if (!switch_strlen_zero(terminators)) {
		for (i = 0; i < x; i++) {
			if (strchr(terminators, buf[i]) && terminator != NULL) {
				*terminator = buf[i];
				return SWITCH_STATUS_SUCCESS;
			}
		}
	}

	if (timeout) {
		started = switch_time_now();
	}

	while (switch_channel_ready(channel)) {
		switch_frame_t *read_frame;
		switch_event_t *event;

		if (timeout) {
			elapsed = (uint32_t) ((switch_time_now() - started) / 1000);
			if (elapsed >= timeout) {
				break;
			}
		}

		if (switch_core_session_dequeue_private_event(session, &event) == SWITCH_STATUS_SUCCESS) {
			switch_ivr_parse_event(session, event);
			switch_event_destroy(&event);
		}

		if (switch_channel_has_dtmf(channel)) {
			char dtmf[128];

			switch_channel_dequeue_dtmf(channel, dtmf, maxdigits);
			for (i = 0; i < (uint32_t) strlen(dtmf); i++) {

				if (!switch_strlen_zero(terminators) && strchr(terminators, dtmf[i]) && terminator != NULL) {
					*terminator = dtmf[i];
					return SWITCH_STATUS_SUCCESS;
				}

				buf[x++] = dtmf[i];
				buf[x] = '\0';
				if (x >= buflen || x >= maxdigits) {
					return SWITCH_STATUS_SUCCESS;
				}
			}
		}

		if (switch_channel_test_flag(channel, CF_SERVICE)) {
			switch_yield(1000);
		} else {
			status = switch_core_session_read_frame(session, &read_frame, -1, 0);
			if (!SWITCH_READ_ACCEPTABLE(status)) {
				break;
			}
		}
	}

	return status;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_hold(switch_core_session_t *session)
{
	switch_core_session_message_t msg = { 0 };
	switch_channel_t *channel;

	msg.message_id = SWITCH_MESSAGE_INDICATE_HOLD;
	msg.from = __FILE__;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_channel_set_flag(channel, CF_HOLD);
	switch_channel_set_flag(channel, CF_SUSPEND);

	switch_core_session_receive_message(session, &msg);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_hold_uuid(char *uuid)
{
	switch_core_session_t *session;

	if ((session = switch_core_session_locate(uuid))) {
		switch_ivr_hold(session);
		switch_core_session_rwunlock(session);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_unhold(switch_core_session_t *session)
{
	switch_core_session_message_t msg = { 0 };
	switch_channel_t *channel;

	msg.message_id = SWITCH_MESSAGE_INDICATE_UNHOLD;
	msg.from = __FILE__;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	switch_channel_clear_flag(channel, CF_HOLD);
	switch_channel_clear_flag(channel, CF_SUSPEND);

	switch_core_session_receive_message(session, &msg);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_unhold_uuid(char *uuid)
{
	switch_core_session_t *session;

	if ((session = switch_core_session_locate(uuid))) {
		switch_ivr_unhold(session);
		switch_core_session_rwunlock(session);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_media(char *uuid, switch_media_flag_t flags)
{
	char *other_uuid = NULL;
	switch_channel_t *channel, *other_channel = NULL;
	switch_core_session_t *session, *other_session;
	switch_core_session_message_t msg = { 0 };
	switch_status_t status = SWITCH_STATUS_GENERR;
	uint8_t swap = 0;

	msg.message_id = SWITCH_MESSAGE_INDICATE_MEDIA;
	msg.from = __FILE__;

	if ((session = switch_core_session_locate(uuid))) {
		channel = switch_core_session_get_channel(session);
		assert(channel != NULL);
		if ((flags & SMF_REBRIDGE) && !switch_channel_test_flag(channel, CF_ORIGINATOR)) {
			swap = 1;
		}

		if (switch_channel_test_flag(channel, CF_NOMEDIA)) {
			status = SWITCH_STATUS_SUCCESS;
			switch_channel_clear_flag(channel, CF_NOMEDIA);
			switch_core_session_receive_message(session, &msg);

			if ((flags & SMF_REBRIDGE)
				&& (other_uuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BRIDGE_VARIABLE))
				&& (other_session = switch_core_session_locate(other_uuid))) {
				other_channel = switch_core_session_get_channel(other_session);
				assert(other_channel != NULL);
				switch_core_session_receive_message(other_session, &msg);
				switch_channel_clear_state_handler(other_channel, NULL);
				switch_core_session_rwunlock(other_session);
			}
			if (other_channel) {
				switch_channel_clear_state_handler(channel, NULL);
			}
		}

		switch_core_session_rwunlock(session);

		if (other_channel) {
			if (swap) {
				switch_ivr_uuid_bridge(other_uuid, uuid);
			} else {
				switch_ivr_uuid_bridge(uuid, other_uuid);
			}
		}
	}

	return status;
}


SWITCH_DECLARE(switch_status_t) switch_ivr_nomedia(char *uuid, switch_media_flag_t flags)
{
	char *other_uuid;
	switch_channel_t *channel, *other_channel = NULL;
	switch_core_session_t *session, *other_session = NULL;
	switch_core_session_message_t msg = { 0 };
	switch_status_t status = SWITCH_STATUS_GENERR;
	uint8_t swap = 0;

	msg.message_id = SWITCH_MESSAGE_INDICATE_NOMEDIA;
	msg.from = __FILE__;

	if ((session = switch_core_session_locate(uuid))) {
		status = SWITCH_STATUS_SUCCESS;
		channel = switch_core_session_get_channel(session);
		assert(channel != NULL);

		if ((flags & SMF_REBRIDGE) && !switch_channel_test_flag(channel, CF_ORIGINATOR)) {
			swap = 1;
		}

		if ((flags & SMF_FORCE) || !switch_channel_test_flag(channel, CF_NOMEDIA)) {
			switch_channel_set_flag(channel, CF_NOMEDIA);
			switch_core_session_receive_message(session, &msg);
			if ((flags & SMF_REBRIDGE) && (other_uuid = switch_channel_get_variable(channel, SWITCH_BRIDGE_VARIABLE)) &&
				(other_session = switch_core_session_locate(other_uuid))) {
				other_channel = switch_core_session_get_channel(other_session);
				assert(other_channel != NULL);
				switch_core_session_receive_message(other_session, &msg);
				switch_channel_clear_state_handler(other_channel, NULL);

			}
			if (other_channel) {
				switch_channel_clear_state_handler(channel, NULL);
				if (swap) {
					switch_ivr_signal_bridge(other_session, session);
				} else {
					switch_ivr_signal_bridge(session, other_session);
				}
				switch_core_session_rwunlock(other_session);
			}
		}
		switch_core_session_rwunlock(session);
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_session_transfer(switch_core_session_t *session, char *extension, char *dialplan, char *context)
{
	switch_channel_t *channel;
	switch_caller_profile_t *profile, *new_profile;
	switch_core_session_message_t msg = { 0 };
	switch_core_session_t *other_session;
	switch_channel_t *other_channel = NULL;
	char *uuid = NULL;

	assert(session != NULL);
	assert(extension != NULL);

	switch_core_session_reset(session);


	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	/* clear all state handlers */
	switch_channel_clear_state_handler(channel, NULL);

	if ((profile = switch_channel_get_caller_profile(channel))) {
		new_profile = switch_caller_profile_clone(session, profile);
		new_profile->destination_number = switch_core_session_strdup(session, extension);

		if (!switch_strlen_zero(dialplan)) {
			new_profile->dialplan = switch_core_session_strdup(session, dialplan);
		} else {
			dialplan = new_profile->dialplan;
		}

		if (!switch_strlen_zero(context)) {
			new_profile->context = switch_core_session_strdup(session, context);
		} else {
			context = new_profile->context;
		}


		switch_channel_set_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE, NULL);
		if ((uuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BOND_VARIABLE))
			&& (other_session = switch_core_session_locate(uuid))) {
			switch_channel_set_variable(other_channel, SWITCH_SIGNAL_BOND_VARIABLE, NULL);
			switch_core_session_rwunlock(other_session);
		}

		if ((uuid = switch_channel_get_variable(channel, SWITCH_SIGNAL_BRIDGE_VARIABLE))
			&& (other_session = switch_core_session_locate(uuid))) {
			other_channel = switch_core_session_get_channel(other_session);
			assert(other_channel != NULL);

			switch_channel_set_variable(channel, SWITCH_SIGNAL_BRIDGE_VARIABLE, NULL);
			switch_channel_set_variable(other_channel, SWITCH_SIGNAL_BRIDGE_VARIABLE, NULL);

			switch_channel_set_variable(channel, SWITCH_BRIDGE_VARIABLE, NULL);
			switch_channel_set_variable(other_channel, SWITCH_BRIDGE_VARIABLE, NULL);

			switch_channel_hangup(other_channel, SWITCH_CAUSE_BLIND_TRANSFER);
			switch_ivr_media(uuid, SMF_NONE);

			switch_core_session_rwunlock(other_session);
		}

		switch_channel_set_caller_profile(channel, new_profile);
		switch_channel_set_flag(channel, CF_TRANSFER);
		switch_channel_set_state(channel, CS_RING);

		msg.message_id = SWITCH_MESSAGE_INDICATE_TRANSFER;
		msg.from = __FILE__;
		switch_core_session_receive_message(session, &msg);

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Transfer %s to %s[%s@%s]\n", switch_channel_get_name(channel), dialplan, extension,
						  context);
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_transfer_variable(switch_core_session_t *sessa, switch_core_session_t *sessb, char *var)
{
	switch_channel_t *chana = switch_core_session_get_channel(sessa);
	switch_channel_t *chanb = switch_core_session_get_channel(sessb);
	char *val = NULL;
	uint8_t prefix = 0;

	if (var && *var == '~') {
		var++;
		prefix = 1;
	}


	if (var && !prefix) {
		if ((val = switch_channel_get_variable(chana, var))) {
			switch_channel_set_variable(chanb, var, val);
		}
	} else {
		switch_hash_index_t *hi;
		void *vval;
		const void *vvar;

		if ((hi = switch_channel_variable_first(chana, switch_core_session_get_pool(sessa)))) {
			for (; hi; hi = switch_hash_next(hi)) {
				switch_hash_this(hi, &vvar, NULL, &vval);
				if (vvar && vval && (!prefix || (var && !strncmp((char *) vvar, var, strlen(var))))) {
					switch_channel_set_variable(chanb, (char *) vvar, (char *) vval);
				}
			}
			switch_channel_variable_last(chana);
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

/******************************************************************************************************/

struct switch_ivr_digit_stream_parser {
	int pool_auto_created;
	switch_memory_pool_t *pool;
	switch_hash_t *hash;
	switch_size_t maxlen;
	switch_size_t minlen;
	char terminator;
	unsigned int digit_timeout_ms;
};

struct switch_ivr_digit_stream {
	char *digits;
	switch_time_t last_digit_time;
};

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_parser_new(switch_memory_pool_t *pool, switch_ivr_digit_stream_parser_t ** parser)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (parser != NULL) {
		int pool_auto_created = 0;

		// if the caller didn't provide a pool, make one
		if (pool == NULL) {
			switch_core_new_memory_pool(&pool);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "created a memory pool\n");
			if (pool != NULL) {
				pool_auto_created = 1;
			}
		}
		// if we have a pool, make a parser object
		if (pool != NULL) {
			*parser = (switch_ivr_digit_stream_parser_t *) switch_core_alloc(pool, sizeof(switch_ivr_digit_stream_parser_t));
		}
		// if we have parser object, initialize it for the caller
		if (*parser != NULL) {
			memset(*parser, 0, sizeof(switch_ivr_digit_stream_parser_t));
			(*parser)->pool_auto_created = pool_auto_created;
			(*parser)->pool = pool;
			(*parser)->digit_timeout_ms = 1000;
			switch_core_hash_init(&(*parser)->hash, (*parser)->pool);

			status = SWITCH_STATUS_SUCCESS;
		} else {
			status = SWITCH_STATUS_MEMERR;
			// if we can't create a parser object,clean up the pool if we created it
			if (pool != NULL && pool_auto_created) {
				switch_core_destroy_memory_pool(&pool);
			}
		}
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_parser_destroy(switch_ivr_digit_stream_parser_t * parser)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (parser != NULL) {
		if (parser->hash != NULL) {
			switch_core_hash_destroy(parser->hash);
			parser->hash = NULL;
		}
		// free the memory pool if we created it
		if (parser->pool_auto_created && parser->pool != NULL) {
			status = switch_core_destroy_memory_pool(&parser->pool);
		}
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_new(switch_ivr_digit_stream_parser_t * parser, switch_ivr_digit_stream_t ** stream)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	// if we have a paser object memory pool and a stream object pointer that is null
	if (parser != NULL && parser->pool && stream != NULL && *stream == NULL) {
		*stream = (switch_ivr_digit_stream_t *) switch_core_alloc(parser->pool, sizeof(switch_ivr_digit_stream_t));
		if (*stream != NULL) {
			memset(*stream, 0, sizeof(switch_ivr_digit_stream_t));
			status = SWITCH_STATUS_SUCCESS;
		}
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_destroy(switch_ivr_digit_stream_t * stream)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (stream == NULL && stream->digits != NULL) {
		free(stream->digits);
		stream->digits = NULL;
		status = SWITCH_STATUS_SUCCESS;
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_parser_set_event(switch_ivr_digit_stream_parser_t * parser, char *digits, void *data)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (parser != NULL && digits != NULL && *digits && parser->hash != NULL) {

		status = switch_core_hash_insert_dup(parser->hash, digits, data);
		if (status == SWITCH_STATUS_SUCCESS) {
			switch_size_t len = strlen(digits);

			// if we don't have a terminator, then we have to try and
			// figure out when a digit set is completed, therefore we
			// keep track of the min and max digit lengths
			if (parser->terminator == '\0') {
				if (len > parser->maxlen) {
					parser->maxlen = len;
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "max len %u\n", (uint32_t) parser->maxlen);
				}
				if (parser->minlen == 0 || len < parser->minlen) {
					parser->minlen = len;
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "min len %u\n", (uint32_t) parser->minlen);
				}
			} else {
				// since we have a terminator, reset min and max
				parser->minlen = 0;
				parser->maxlen = 0;
			}

		}
	}
	if (status != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "unable to add hash for '%s'\n", digits);
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_parser_del_event(switch_ivr_digit_stream_parser_t * parser, char *digits)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (parser != NULL && digits != NULL && *digits) {
		status = switch_core_hash_delete(parser->hash, digits);
	}

	if (status != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "unable to del hash for '%s'\n", digits);
	}

	return status;
}

SWITCH_DECLARE(void *) switch_ivr_digit_stream_parser_feed(switch_ivr_digit_stream_parser_t * parser, switch_ivr_digit_stream_t * stream, char digit)
{
	void *result = NULL;

	if (parser != NULL && stream != NULL) {
		switch_size_t len = (stream->digits != NULL ? strlen(stream->digits) : 0);

		// handle new digit arrivals
		if (digit != '\0') {

			// if it's not a terminator digit, add it to the collected digits
			if (digit != parser->terminator) {
				// if collected digits length >= the max length of the keys
				// in the hash table, then left shift the digit string
				if (len > 0 && parser->maxlen != 0 && len >= parser->maxlen) {
					char *src = stream->digits + 1;
					char *dst = stream->digits;

					while (*src) {
						*(dst++) = *(src++);
					}
					*dst = digit;
				} else {
					stream->digits = realloc(stream->digits, len + 2);
					*(stream->digits + (len++)) = digit;
					*(stream->digits + len) = '\0';
					stream->last_digit_time = switch_time_now() / 1000;
				}
			}
		}
		// don't allow collected digit string testing if there are varying sized keys until timeout
		if (parser->maxlen - parser->minlen > 0 && (switch_time_now() / 1000) - stream->last_digit_time < parser->digit_timeout_ms) {
			len = 0;
		}
		// if we have digits to test
		if (len) {
			result = switch_core_hash_find(parser->hash, stream->digits);
			// if we matched the digit string, or this digit is the terminator
			// reset the collected digits for next digit string
			if (result != NULL || parser->terminator == digit) {
				free(stream->digits);
				stream->digits = NULL;
			}
		}
	}

	return result;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_reset(switch_ivr_digit_stream_t * stream)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (stream != NULL && stream->digits != NULL) {
		free(stream->digits);
		stream->digits = NULL;
		stream->last_digit_time = 0;
		status = SWITCH_STATUS_SUCCESS;
	}

	return status;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_digit_stream_parser_set_terminator(switch_ivr_digit_stream_parser_t * parser, char digit)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (parser != NULL) {
		parser->terminator = digit;
		// since we have a terminator, reset min and max
		parser->minlen = 0;
		parser->maxlen = 0;
		status = SWITCH_STATUS_SUCCESS;
	}

	return status;
}

static int set_profile_data(switch_xml_t xml, switch_caller_profile_t *caller_profile, int off)
{
	switch_xml_t param;

	if (!(param = switch_xml_add_child_d(xml, "username", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->username);

	if (!(param = switch_xml_add_child_d(xml, "dialplan", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->dialplan);

	if (!(param = switch_xml_add_child_d(xml, "caller_id_name", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->caller_id_name);

	if (!(param = switch_xml_add_child_d(xml, "ani", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->ani);

	if (!(param = switch_xml_add_child_d(xml, "aniii", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->aniii);

	if (!(param = switch_xml_add_child_d(xml, "caller_id_number", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->caller_id_number);

	if (!(param = switch_xml_add_child_d(xml, "network_addr", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->network_addr);

	if (!(param = switch_xml_add_child_d(xml, "rdnis", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->rdnis);

	if (!(param = switch_xml_add_child_d(xml, "destination_number", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->destination_number);

	if (!(param = switch_xml_add_child_d(xml, "uuid", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->uuid);

	if (!(param = switch_xml_add_child_d(xml, "source", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->source);

	if (!(param = switch_xml_add_child_d(xml, "context", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->context);

	if (!(param = switch_xml_add_child_d(xml, "chan_name", off++))) {
		return -1;
	}
	switch_xml_set_txt(param, caller_profile->chan_name);

	return 0;
}

SWITCH_DECLARE(switch_status_t) switch_ivr_generate_xml_cdr(switch_core_session_t *session, switch_xml_t * xml_cdr)
{
	switch_channel_t *channel;
	switch_caller_profile_t *caller_profile;
	switch_hash_index_t *hi;
	void *vval;
	const void *vvar;
	switch_xml_t variable, variables, cdr, x_caller_profile, x_caller_extension, x_times, time_tag, 
		x_application, x_callflow, x_inner_extension, x_apps;
	switch_app_log_t *app_log;
	
	char tmp[512];
	int cdr_off = 0, v_off = 0;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (!(cdr = switch_xml_new("cdr"))) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (!(variables = switch_xml_add_child_d(cdr, "variables", cdr_off++))) {
		goto error;
	}

	if ((app_log = switch_core_session_get_app_log(session))) {
		int app_off = 0;
		switch_app_log_t *ap;
		
		if (!(x_apps = switch_xml_add_child_d(cdr, "app_log", cdr_off++))) {
			goto error;
		}
		for(ap = app_log; ap; ap = ap->next) {

			if (!(x_application = switch_xml_add_child_d(x_apps, "application", app_off++))) {
				goto error;
			}
			
			switch_xml_set_attr_d(x_application, "app_name", ap->app);
			switch_xml_set_attr_d(x_application, "app_data", ap->arg);
		}
	}

	if (((hi = switch_channel_variable_first(channel, switch_core_session_get_pool(session))))) {
		for (; hi; hi = switch_hash_next(hi)) {
			switch_hash_this(hi, &vvar, NULL, &vval);
			if (vvar && vval) {
				if ((variable = switch_xml_add_child_d(variables, (char *) vvar, v_off++))) {
					char *data;
					char *value = (char *) vval;
					switch_size_t dlen = strlen(value) * 3;
					
					if ((data = switch_core_session_alloc(session, dlen))) {
						switch_url_encode(value, data, dlen);
						switch_xml_set_txt_d(variable, data);
					}
				}
			}
		}
		switch_channel_variable_last(channel);
	}

	caller_profile = switch_channel_get_caller_profile(channel);

	while (caller_profile) {
		int cf_off = 0;

		if (!(x_callflow = switch_xml_add_child_d(cdr, "callflow", cdr_off++))) {
			goto error;
		}
		switch_xml_set_attr_d(x_callflow, "dialplan", caller_profile->dialplan);

		if (caller_profile->caller_extension) {
			switch_caller_application_t *ap;
			int app_off = 0;

			if (!(x_caller_extension = switch_xml_add_child_d(x_callflow, "extension", cf_off++))) {
				goto error;
			}
			switch_xml_set_attr_d(x_caller_extension, "name", caller_profile->caller_extension->extension_name);
			switch_xml_set_attr_d(x_caller_extension, "number", caller_profile->caller_extension->extension_number);
			if (caller_profile->caller_extension->current_application) {
				switch_xml_set_attr_d(x_caller_extension, "current_app", caller_profile->caller_extension->current_application->application_name);
			}

			for (ap = caller_profile->caller_extension->applications; ap; ap = ap->next) {
				if (!(x_application = switch_xml_add_child_d(x_caller_extension, "application", app_off++))) {
					goto error;
				}
				if (ap == caller_profile->caller_extension->current_application) {
					switch_xml_set_attr_d(x_application, "last_executed", "true");
				}
				switch_xml_set_attr_d(x_application, "app_name", ap->application_name);
				switch_xml_set_attr_d(x_application, "app_data", ap->application_data);
			}

			if (caller_profile->caller_extension->children) {
				switch_caller_profile_t *cp = NULL;
				int i_off = 0;
				for (cp = caller_profile->caller_extension->children; cp; cp = cp->next) {
					switch_caller_application_t *ap;
					int app_off = 0;
					
					if (!cp->caller_extension) {
						continue;
					}
					if (!(x_inner_extension = switch_xml_add_child_d(x_caller_extension, "sub_extensions", i_off++))) {
						goto error;
					}				

					if (!(x_caller_extension = switch_xml_add_child_d(x_inner_extension, "extension", cf_off++))) {
						goto error;
					}
					switch_xml_set_attr_d(x_caller_extension, "name", cp->caller_extension->extension_name);
					switch_xml_set_attr_d(x_caller_extension, "number", cp->caller_extension->extension_number);
					switch_xml_set_attr_d(x_caller_extension, "dialplan", cp->dialplan);
					if (cp->caller_extension->current_application) {
						switch_xml_set_attr_d(x_caller_extension, "current_app", cp->caller_extension->current_application->application_name);
					}
				
					for (ap = cp->caller_extension->applications; ap; ap = ap->next) {
						if (!(x_application = switch_xml_add_child_d(x_caller_extension, "application", app_off++))) {
							goto error;
						}
						if (ap == cp->caller_extension->current_application) {
							switch_xml_set_attr_d(x_application, "last_executed", "true");
						}
						switch_xml_set_attr_d(x_application, "app_name", ap->application_name);
						switch_xml_set_attr_d(x_application, "app_data", ap->application_data);
					}
				}
			}

		}


		if (!(x_caller_profile = switch_xml_add_child_d(x_callflow, "caller_profile", cf_off++))) {
			goto error;
		}
		set_profile_data(x_caller_profile, caller_profile, 0);

		if (caller_profile->originator_caller_profile) {
			if (!(x_caller_profile = switch_xml_add_child_d(x_callflow, "originator_caller_profile", cf_off++))) {
				goto error;
			}
			set_profile_data(x_caller_profile, caller_profile->originator_caller_profile, 0);
		}

		if (caller_profile->originatee_caller_profile) {
			if (!(x_caller_profile = switch_xml_add_child_d(x_callflow, "originatee_caller_profile", cf_off++))) {
				goto error;
			}
			set_profile_data(x_caller_profile, caller_profile->originatee_caller_profile, 0);
		}


		if (caller_profile->times) {
			int t_off = 0;
			if (!(x_times = switch_xml_add_child_d(x_callflow, "times", cf_off++))) {
				goto error;
			}
			if (!(time_tag = switch_xml_add_child_d(x_times, "created_time", t_off++))) {
				goto error;
			}
			snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->created);
			switch_xml_set_txt_d(time_tag, tmp);

			if (!(time_tag = switch_xml_add_child_d(x_times, "answered_time", t_off++))) {
				goto error;
			}
			snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->answered);
			switch_xml_set_txt_d(time_tag, tmp);

			if (!(time_tag = switch_xml_add_child_d(x_times, "hangup_time", t_off++))) {
				goto error;
			}
			snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->hungup);
			switch_xml_set_txt_d(time_tag, tmp);

			if (!(time_tag = switch_xml_add_child_d(x_times, "transfer_time", t_off++))) {
				goto error;
			}
			snprintf(tmp, sizeof(tmp), "%" SWITCH_TIME_T_FMT, caller_profile->times->transferred);
			switch_xml_set_txt_d(time_tag, tmp);
		}

		caller_profile = caller_profile->next;
	}




	*xml_cdr = cdr;

	return SWITCH_STATUS_SUCCESS;

  error:

	if (cdr) {
		switch_xml_free(cdr);
	}

	return SWITCH_STATUS_FALSE;
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
