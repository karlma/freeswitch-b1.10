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
 * mod_esf.c -- Extra SIP Functionality
 *
 */
#include <switch.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_esf_load);
SWITCH_MODULE_DEFINITION(mod_esf, mod_esf_load, NULL, NULL);

struct ls_control_packet {
	uint32_t unique_id;
	uint32_t command;
	uint32_t ip;
	uint32_t port;
};
typedef struct ls_control_packet ls_control_packet_t;

typedef enum {
	LS_START_BCAST = 6,
	LS_STOP_BCAST = 7
} ls_command_t;

typedef enum {
	SEND_TYPE_UNKNOWN = 0,
	SEND_TYPE_RTP = 1,
	SEND_TYPE_RAW = 2,
	SEND_TYPE_NOMEDIA = 3
} ls_how_t;

SWITCH_STANDARD_APP(bcast_function)
{
	switch_channel_t *channel;
	switch_socket_t *socket;
	switch_sockaddr_t *audio_addr = NULL, *control_packet_addr;
	switch_frame_t *read_frame;
	switch_status_t status;
	switch_size_t bytes;
	ls_control_packet_t control_packet;
	switch_codec_t *read_codec;
	uint32_t flags = 0;
	const char *err;
	switch_rtp_t *rtp_session = NULL;
	switch_port_t rtp_port;
	char guess_ip[25];
	ls_how_t ready = SEND_TYPE_UNKNOWN;
	int argc;
    char *mydata, *argv[5];
	char *mcast_ip = "224.168.168.168";
	switch_port_t mcast_port = 34567;
	switch_port_t mcast_control_port = 6061;
	char *mcast_port_str = "34567";


	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);
	

	if (!switch_strlen_zero((char *) data)) {
		mydata = switch_core_session_strdup(session, data);
		assert(mydata != NULL);

		argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	
		if (!switch_strlen_zero(argv[0])) {
			mcast_ip = argv[0];
		}

		if (!switch_strlen_zero(argv[1])) {
			mcast_port_str = argv[1];
			mcast_port = (switch_port_t)atoi(mcast_port_str);
		}

		if (!switch_strlen_zero(argv[2])) {
			mcast_control_port = (switch_port_t)atoi(argv[2]);
		}
	}

	
	if (switch_true(switch_channel_get_variable(channel, SWITCH_BYPASS_MEDIA_VARIABLE))) {
		switch_core_session_message_t msg = { 0 };

		ready = SEND_TYPE_NOMEDIA;
		
		switch_channel_set_variable(channel, SWITCH_REMOTE_MEDIA_IP_VARIABLE, mcast_ip);
		switch_channel_set_variable(channel, SWITCH_REMOTE_MEDIA_PORT_VARIABLE, mcast_port_str);
		
		/* special answer with the mcast addr */
		msg.from = __FILE__;
		msg.string_arg = "recvonly";
		msg.message_id = SWITCH_MESSAGE_INDICATE_BROADCAST;
		switch_core_session_receive_message(session, &msg);
	} else {
		switch_channel_answer(channel);
	} 

	read_codec = switch_core_session_get_read_codec(session);

	if (switch_socket_create(&socket, AF_INET, SOCK_DGRAM, 0, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error 1\n");
		goto fail;
	}

	if (switch_sockaddr_info_get(&control_packet_addr, mcast_ip, SWITCH_UNSPEC, 
								 mcast_control_port, 0, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error 3\n");
		goto fail;
	}


	while(!ready) {
		status = switch_core_session_read_frame(session, &read_frame, -1, 0);
		if (switch_test_flag(read_frame, SFF_CNG)) {
			continue;
		}

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			goto fail;
		}
		
		if (read_frame->packet && read_frame->packetlen) {
			ready = SEND_TYPE_RAW;
		} else {
			ready = SEND_TYPE_RTP;
		}

	}

	if (ready == SEND_TYPE_RTP) {
		switch_find_local_ip(guess_ip, sizeof(guess_ip), AF_INET);
		if (!(rtp_port = switch_rtp_request_port(guess_ip))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "RTP Port Error\n");
			goto fail;
		}

		rtp_session = switch_rtp_new(guess_ip,
									 rtp_port,
									 mcast_ip,
									 mcast_port,
									 read_codec->implementation->ianacode,
									 read_codec->implementation->samples_per_frame,
									 read_codec->implementation->microseconds_per_frame,
									 (switch_rtp_flag_t) flags,
									 NULL, "soft", &err, switch_core_session_get_pool(session));
	
		if (!switch_rtp_ready(rtp_session)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "RTP Error\n");
			goto fail;
		}
	} else if (ready == SEND_TYPE_NOMEDIA) {
		switch_yield(10000);
	} else if (ready == SEND_TYPE_RAW) {
		if (switch_sockaddr_info_get(&audio_addr, mcast_ip, SWITCH_UNSPEC, mcast_port, 0, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error 2\n");
			goto fail;
		}
	}

	control_packet.unique_id = htonl((u_long)switch_timestamp(NULL));
	control_packet.command = htonl(LS_START_BCAST);
	control_packet.ip = inet_addr(mcast_ip);
	control_packet.port = htonl(mcast_port);

	bytes = 16;
	switch_socket_sendto(socket, control_packet_addr, 0, (void *)&control_packet, &bytes);
	bytes = 16;
	switch_socket_sendto(socket, control_packet_addr, 0, (void *)&control_packet, &bytes);


	for(;;) {
		status = switch_core_session_read_frame(session, &read_frame, -1, 0);
        if (!SWITCH_READ_ACCEPTABLE(status)) {
            break;
        }
		if (switch_test_flag(read_frame, SFF_CNG)) {
			continue;
		}
		if (ready == SEND_TYPE_RTP) {
			switch_rtp_write_frame(rtp_session, read_frame);
		} else {
			bytes = read_frame->packetlen;
			switch_socket_sendto(socket, audio_addr, 0, read_frame->packet, &bytes);
		}
	}



	control_packet.unique_id = htonl((u_long)switch_timestamp(NULL));
	control_packet.command = htonl(LS_STOP_BCAST);
	bytes = 8;
	switch_socket_sendto(socket, control_packet_addr, 0, (void *)&control_packet, &bytes);
	bytes = 8;
	switch_socket_sendto(socket, control_packet_addr, 0, (void *)&control_packet, &bytes);

 fail:

	if (rtp_session && ready == SEND_TYPE_RTP && switch_rtp_ready(rtp_session)) {
		switch_rtp_destroy(&rtp_session);
	}	

	if (socket) {
		switch_socket_close(socket);
	}

	return;
}


SWITCH_MODULE_LOAD_FUNCTION(mod_esf_load)
{
	switch_application_interface_t *app_interface;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	SWITCH_ADD_APP(app_interface, "esf_page_group", NULL, NULL, bcast_function, NULL, SAF_NONE);

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
