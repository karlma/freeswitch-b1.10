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
 * switch_channel.h -- Media Channel Interface
 * Marcel Barbulescu <marcelbarbulescu@gmail.com>
 *
 */
/** 
 * @file switch_rtp.h
 * @brief RTP
 * 
 */

#ifndef SWITCH_RTP_H
#define SWITCH_RTP_H

SWITCH_BEGIN_EXTERN_C
#define SWITCH_RTP_MAX_BUF_LEN 16384
#define SWITCH_RTP_MAX_CRYPTO_LEN 64
#define SWITCH_RTP_KEY_LEN 30
#define SWITCH_RTP_CRYPTO_KEY_32 "AES_CM_128_HMAC_SHA1_32"
#define SWITCH_RTP_CRYPTO_KEY_80 "AES_CM_128_HMAC_SHA1_80"
	typedef enum {
	SWITCH_RTP_CRYPTO_SEND,
	SWITCH_RTP_CRYPTO_RECV,
	SWITCH_RTP_CRYPTO_MAX
} switch_rtp_crypto_direction_t;

typedef enum {
	NO_CRYPTO,
	AES_CM_128_HMAC_SHA1_80, 
	AES_CM_128_HMAC_SHA1_32,
	AES_CM_128_NULL_AUTH
} switch_rtp_crypto_key_type_t;

struct switch_rtp_crypto_key {
	uint32_t index;
	switch_rtp_crypto_key_type_t type;
	unsigned char key[SWITCH_RTP_MAX_CRYPTO_LEN];
	switch_size_t keylen;
	struct switch_rtp_crypto_key *next;
};
typedef struct switch_rtp_crypto_key switch_rtp_crypto_key_t;



SWITCH_DECLARE(switch_status_t) switch_rtp_add_crypto_key(switch_rtp_t *rtp_session,
														  switch_rtp_crypto_direction_t direction,
														  uint32_t index, switch_rtp_crypto_key_type_t type, unsigned char *key, switch_size_t keylen);

///\defgroup rtp RTP (RealTime Transport Protocol)
///\ingroup core1
///\{
	 typedef void (*switch_rtp_invalid_handler_t) (switch_rtp_t *rtp_session,
												   switch_socket_t *sock, void *data, switch_size_t datalen, switch_sockaddr_t *from_addr);


SWITCH_DECLARE(void) switch_rtp_get_random(void *buf, uint32_t len);
/*! 
  \brief Initilize the RTP System
  \param pool the memory pool to use for long term allocations
  \note Generally called by the core_init
*/
SWITCH_DECLARE(void) switch_rtp_init(switch_memory_pool_t *pool);
SWITCH_DECLARE(void) switch_rtp_shutdown(void);

/*!
  \brief Set/Get RTP start port
  \param port new value (if > 0)
  \return the current RTP start port
*/
SWITCH_DECLARE(switch_port_t) switch_rtp_set_start_port(switch_port_t port);

/*!
  \brief Set/Get RTP end port
  \param port new value (if > 0)
  \return the current RTP end port
*/
SWITCH_DECLARE(switch_port_t) switch_rtp_set_end_port(switch_port_t port);

/*! 
  \brief Request a new port to be used for media
  \param ip the ip to request a port from
  \return the new port to use
*/
SWITCH_DECLARE(switch_port_t) switch_rtp_request_port(const char *ip);
SWITCH_DECLARE(void) switch_rtp_release_port(const char *ip, switch_port_t port);

SWITCH_DECLARE(switch_status_t) switch_rtp_change_interval(switch_rtp_t *rtp_session, uint32_t ms_per_packet, uint32_t samples_per_interval);
/*! 
  \brief create a new RTP session handle
  \param new_rtp_session a poiter to aim at the new session
  \param payload the IANA payload number
  \param samples_per_interval the default samples_per_interval
  \param ms_per_packet time in microseconds per packet
  \param flags flags to control behaviour
  \param timer_name timer interface to use
  \param err a pointer to resolve error messages
  \param pool a memory pool to use for the session
  \return the new RTP session or NULL on failure
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_create(switch_rtp_t **new_rtp_session,
												  switch_payload_t payload,
												  uint32_t samples_per_interval,
												  uint32_t ms_per_packet,
												  switch_rtp_flag_t flags, char *timer_name, const char **err, switch_memory_pool_t *pool);


/*!
  \brief prepare a new RTP session handle and fully initilize it
  \param rx_host the local address
  \param rx_port the local port
  \param tx_host the remote address
  \param tx_port the remote port
  \param payload the IANA payload number
  \param samples_per_interval the default samples_per_interval
  \param ms_per_packet time in microseconds per packet
  \param flags flags to control behaviour
  \param timer_name timer interface to use
  \param err a pointer to resolve error messages
  \param pool a memory pool to use for the session
  \return the new RTP session or NULL on failure
*/
SWITCH_DECLARE(switch_rtp_t *) switch_rtp_new(const char *rx_host,
											  switch_port_t rx_port,
											  const char *tx_host,
											  switch_port_t tx_port,
											  switch_payload_t payload,
											  uint32_t samples_per_interval,
											  uint32_t ms_per_packet,
											  switch_rtp_flag_t flags, char *timer_name, const char **err, switch_memory_pool_t *pool);


/*! 
  \brief Assign a remote address to the RTP session
  \param rtp_session an RTP session to assign the remote address to
  \param host the ip or fqhn of the remote address
  \param port the remote port
  \param err pointer for error messages
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_set_remote_address(switch_rtp_t *rtp_session, const char *host, switch_port_t port, 
															  switch_bool_t change_adv_addr, const char **err);

SWITCH_DECLARE(char *) switch_rtp_get_remote_host(switch_rtp_t *rtp_session);
SWITCH_DECLARE(switch_port_t) switch_rtp_get_remote_port(switch_rtp_t *rtp_session);
SWITCH_DECLARE(void) switch_rtp_reset_media_timer(switch_rtp_t *rtp_session);
SWITCH_DECLARE(void) switch_rtp_set_max_missed_packets(switch_rtp_t *rtp_session, uint32_t max);

/*! 
  \brief Assign a local address to the RTP session
  \param rtp_session an RTP session to assign the local address to
  \param host the ip or fqhn of the local address
  \param port the local port
  \param change_adv_addr change the advertised address for doing compare
  \param err pointer for error messages
  \note this call also binds the RTP session's socket to the new address
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_set_local_address(switch_rtp_t *rtp_session, const char *host, 
															 switch_port_t port, const char **err);

/*! 
  \brief Kill the socket on an existing RTP session
  \param rtp_session an RTP session to kill the socket of
*/
SWITCH_DECLARE(void) switch_rtp_kill_socket(switch_rtp_t *rtp_session);

SWITCH_DECLARE(void) switch_rtp_break(switch_rtp_t *rtp_session);

/*! 
  \brief Test if an RTP session is ready
  \param rtp_session an RTP session to test
  \return a true value if it's ready
*/
SWITCH_DECLARE(uint8_t) switch_rtp_ready(switch_rtp_t *rtp_session);

/*! 
  \brief Destroy an RTP session
  \param rtp_session an RTP session to destroy
*/
SWITCH_DECLARE(void) switch_rtp_destroy(switch_rtp_t **rtp_session);

/*! 
  \brief Acvite ICE on an RTP session
  \return SWITCH_STATUS_SUCCESS
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_activate_ice(switch_rtp_t *rtp_session, char *login, char *rlogin);

/*! 
  \brief Acvite a jitter buffer on an RTP session
  \param rtp_session the rtp session
  \param queue_frames the number of frames to delay
  \return SWITCH_STATUS_SUCCESS
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_activate_jitter_buffer(switch_rtp_t *rtp_session, uint32_t queue_frames);

/*!
  \brief Set an RTP Flag
  \param rtp_session the RTP session
  \param flags the flags to set
*/
SWITCH_DECLARE(void) switch_rtp_set_flag(switch_rtp_t *rtp_session, switch_rtp_flag_t flags);

/*!
  \brief Test an RTP Flag
  \param rtp_session the RTP session
  \param flags the flags to test
  \return TRUE or FALSE
*/
SWITCH_DECLARE(uint32_t) switch_rtp_test_flag(switch_rtp_t *rtp_session, switch_rtp_flag_t flags);

/*!
  \brief Clear an RTP Flag
  \param rtp_session the RTP session
  \param flags the flags to clear
*/
SWITCH_DECLARE(void) switch_rtp_clear_flag(switch_rtp_t *rtp_session, switch_rtp_flag_t flags);

/*! 
  \brief Retrieve the socket from an existing RTP session
  \param rtp_session the RTP session to retrieve the socket from
  \return the socket from the RTP session
*/
SWITCH_DECLARE(switch_socket_t *) switch_rtp_get_rtp_socket(switch_rtp_t *rtp_session);

/*! 
  \brief Set the default samples per interval for a given RTP session
  \param rtp_session the RTP session to set the samples per interval on
  \param samples_per_interval the new default samples per interval 
*/
SWITCH_DECLARE(void) switch_rtp_set_default_samples_per_interval(switch_rtp_t *rtp_session, uint32_t samples_per_interval);

/*! 
  \brief Get the default samples per interval for a given RTP session
  \param rtp_session the RTP session to get the samples per interval from
  \return the default samples per interval of the RTP session
*/
SWITCH_DECLARE(uint32_t) switch_rtp_get_default_samples_per_interval(switch_rtp_t *rtp_session);

/*! 
  \brief Set the default payload number for a given RTP session
  \param rtp_session the RTP session to set the payload number on
  \param payload the new default payload number 
*/
SWITCH_DECLARE(void) switch_rtp_set_default_payload(switch_rtp_t *rtp_session, switch_payload_t payload);

/*! 
  \brief Get the default payload number for a given RTP session
  \param rtp_session the RTP session to get the payload number from
  \return the default payload of the RTP session
*/
SWITCH_DECLARE(uint32_t) switch_rtp_get_default_payload(switch_rtp_t *rtp_session);


/*! 
  \brief Set a callback function to execute when an invalid RTP packet is encountered
  \param rtp_session the RTP session
  \param on_invalid the function to set
  \return 
*/
SWITCH_DECLARE(void) switch_rtp_set_invald_handler(switch_rtp_t *rtp_session, switch_rtp_invalid_handler_t on_invalid);

/*! 
  \brief Read data from a given RTP session
  \param rtp_session the RTP session to read from
  \param data the data to read
  \param datalen a pointer to the datalen
  \param payload_type the IANA payload of the packet
  \param flags flags
  \param io_flags i/o flags
  \return the number of bytes read
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_read(switch_rtp_t *rtp_session, void *data, uint32_t *datalen,
												switch_payload_t *payload_type, switch_frame_flag_t *flags, switch_io_flag_t io_flags);

/*! 
  \brief Queue RFC2833 DTMF data into an RTP Session
  \param rtp_session the rtp session to use
  \param dtmf the dtmf digits to queue
  \return SWITCH_STATUS_SUCCESS on success
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_queue_rfc2833(switch_rtp_t *rtp_session, const switch_dtmf_t *dtmf);

/*! 
  \brief Queue RFC2833 DTMF data into an RTP Session
  \param rtp_session the rtp session to use
  \param dtmf the dtmf digits to queue
  \return SWITCH_STATUS_SUCCESS on success
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_queue_rfc2833_in(switch_rtp_t *rtp_session, const switch_dtmf_t *dtmf);

/*!
  \brief Test for presence of DTMF on a given RTP session
  \param rtp_session session to test
  \return number of digits in the queue
*/
SWITCH_DECLARE(switch_size_t) switch_rtp_has_dtmf(switch_rtp_t *rtp_session);

/*!
  \brief Retrieve DTMF digits from a given RTP session
  \param rtp_session RTP session to retrieve digits from
  \param dtmf the dtmf
  \return number of bytes read into the buffer
*/
SWITCH_DECLARE(switch_size_t) switch_rtp_dequeue_dtmf(switch_rtp_t *rtp_session, switch_dtmf_t *dtmf);

/*! 
  \brief Read data from a given RTP session without copying
  \param rtp_session the RTP session to read from
  \param data a pointer to point directly to the RTP read buffer
  \param datalen a pointer to the datalen
  \param payload_type the IANA payload of the packet
  \param flags flags
  \param io_flags i/o flags
  \return the number of bytes read
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_zerocopy_read(switch_rtp_t *rtp_session,
														 void **data, uint32_t *datalen, switch_payload_t *payload_type, switch_frame_flag_t *flags,
														 switch_io_flag_t io_flags);

/*! 
  \brief Read data from a given RTP session without copying
  \param rtp_session the RTP session to read from
  \param frame a frame to populate with information
  \param io_flags i/o flags
  \return the number of bytes read
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_zerocopy_read_frame(switch_rtp_t *rtp_session, switch_frame_t *frame, switch_io_flag_t io_flags);

SWITCH_DECLARE(void) rtp_flush_read_buffer(switch_rtp_t *rtp_session);

/*!
  \brief Enable VAD on an RTP Session
  \param rtp_session the RTP session
  \param session the core session associated with the RTP session
  \param codec the codec the channel is currenty using
  \param flags flags for control
  \return SWITCH_STAUTS_SUCCESS on success
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_enable_vad(switch_rtp_t *rtp_session, switch_core_session_t *session,
													  switch_codec_t *codec, switch_vad_flag_t flags);

/*!
  \brief Disable VAD on an RTP Session
  \param rtp_session the RTP session
  \return SWITCH_STAUTS_SUCCESS on success
*/
SWITCH_DECLARE(switch_status_t) switch_rtp_disable_vad(switch_rtp_t *rtp_session);

/*! 
  \brief Write data to a given RTP session
  \param rtp_session the RTP session to write to
  \param frame the frame to write
  \return the number of bytes written
*/
SWITCH_DECLARE(int) switch_rtp_write_frame(switch_rtp_t *rtp_session, switch_frame_t *frame);

/*! 
  \brief Write data with a specified payload and sequence number to a given RTP session
  \param rtp_session the RTP session to write to
  \param data data to write
  \param datalen the size of the data
  \param m set mark bit or not
  \param payload the IANA payload number
  \param ts then number of bytes to increment the timestamp by
  \param flags frame flags
  \return the number of bytes written
*/
SWITCH_DECLARE(int) switch_rtp_write_manual(switch_rtp_t *rtp_session,
											void *data, uint32_t datalen, uint8_t m, switch_payload_t payload, uint32_t ts, switch_frame_flag_t *flags);

/*! 
  \brief Retrieve the SSRC from a given RTP session
  \param rtp_session the RTP session to retrieve from
  \return the SSRC
*/
SWITCH_DECLARE(uint32_t) switch_rtp_get_ssrc(switch_rtp_t *rtp_session);

/*! 
  \brief Associate an arbitrary data pointer with and RTP session
  \param rtp_session the RTP session to assign the pointer to
  \param private_data the private data to assign
*/
SWITCH_DECLARE(void) switch_rtp_set_private(switch_rtp_t *rtp_session, void *private_data);

/*! 
  \brief Set the payload type to consider RFC2833 DTMF
  \param rtp_session the RTP session to modify
  \param te the payload type
*/
SWITCH_DECLARE(void) switch_rtp_set_telephony_event(switch_rtp_t *rtp_session, switch_payload_t te);

/*! 
  \brief Set the payload type for comfort noise
  \param rtp_session the RTP session to modify
  \param pt the payload type
*/
SWITCH_DECLARE(void) switch_rtp_set_cng_pt(switch_rtp_t *rtp_session, switch_payload_t pt);

/*! 
  \brief Retrieve the private data from a given RTP session
  \param rtp_session the RTP session to retrieve the data from
  \return the pointer to the private data
*/
SWITCH_DECLARE(void *) switch_rtp_get_private(switch_rtp_t *rtp_session);

SWITCH_DECLARE(switch_status_t) switch_rtp_activate_stun_ping(switch_rtp_t *rtp_session, const char *stun_ip, switch_port_t stun_port,
															  uint32_t packet_count, switch_bool_t funny);

SWITCH_DECLARE(void) switch_rtp_intentional_bugs(switch_rtp_t *rtp_session, switch_rtp_bug_flag_t bugs);

/*!
  \}
*/

SWITCH_END_EXTERN_C
#endif
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
