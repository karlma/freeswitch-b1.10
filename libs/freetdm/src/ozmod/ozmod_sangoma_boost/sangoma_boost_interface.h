/*
 * Copyright (c) 2009, Sangoma Technologies
 * Moises Silva <moy@sangoma.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SANGOMA_BOOST_INTERFACE_H
#define SANGOMA_BOOST_INTERFACE_H

#include "zap_types.h"

/*! 
  \brief Callback used to notify signaling status changes on a channel
  \param zchan The openzap channel where the signaling status just changed
  \param status The new signaling status
 */
#define BOOST_SIG_STATUS_CB_ARGS (zap_channel_t *zchan, zap_channel_sig_status_t status)
typedef void (*boost_sig_status_cb_func_t) BOOST_SIG_STATUS_CB_ARGS;
#define BOOST_SIG_STATUS_CB_FUNCTION(name) void name BOOST_SIG_STATUS_CB_ARGS

/*! 
  \brief Write a boost msg to a boost endpoint 
  \param span The openzap span where this msg was generated
  \param msg The generic message pointer, owned by the caller
  \param msglen The length of the provided structure pointed by msg
  \return ZAP_SUCCESS or ZAP_FAIL

   The msg buffer is owned by the caller and it should
   be either t_sigboost_callstart or t_sigboost_short
   the endpoint receiving the msg will first cast to
   t_sigboost_short, check the event type, and if needed.
 */
#define BOOST_WRITE_MSG_ARGS (zap_span_t *span, void *msg, zap_size_t msglen)
typedef zap_status_t (*boost_write_msg_func_t) BOOST_WRITE_MSG_ARGS;
#define BOOST_WRITE_MSG_FUNCTION(name) zap_status_t name BOOST_WRITE_MSG_ARGS

/*! 
  \brief Set the callback to be used by a signaling module to write boost messages
  \param callback The callback to be used by the signaling module

   The provided callback will be used for the signaling boost module to notify the
   user with boost messages.
 */
#define BOOST_SET_WRITE_MSG_CB_ARGS (boost_write_msg_func_t callback)
typedef void (*boost_set_write_msg_cb_func_t) BOOST_SET_WRITE_MSG_CB_ARGS;
#define BOOST_SET_WRITE_MSG_CB_FUNCTION(name) void name BOOST_SET_WRITE_MSG_CB_ARGS

/*! 
  \brief Set signaling status callback used by the signaling module to report signaling status changes
  \param callback The callback to be used by the signaling module

   The provided callback will be used for the signaling boost module to notify the
   user with signaling link status changes.
 */
#define BOOST_SET_SIG_STATUS_CB_ARGS (boost_sig_status_cb_func_t callback)
typedef void (*boost_set_sig_status_cb_func_t) BOOST_SET_SIG_STATUS_CB_ARGS;
#define BOOST_SET_SIG_STATUS_CB_FUNCTION(name) void name BOOST_SET_SIG_STATUS_CB_ARGS

/*! 
  \brief Notify hardware status change
  \param zchan The openzap channel
  \param status The hw status 
  \return ZAP_SUCCESS or ZAP_FAIL 
 */
#define BOOST_GET_SIG_STATUS_ARGS (zap_channel_t *zchan, zap_channel_sig_status_t *status)
typedef zap_status_t (*boost_get_sig_status_func_t) BOOST_GET_SIG_STATUS_ARGS;
#define BOOST_GET_SIG_STATUS_FUNCTION(name) zap_status_t name BOOST_GET_SIG_STATUS_ARGS

/*! 
  \brief Get the signaling status on the given channel.
  \param zchan The openzap channel
  \param status The status pointer where the current signaling status will be set
 */
#define BOOST_ON_HW_LINK_STATUS_CHANGE_ARGS (zap_channel_t *zchan, zap_channel_hw_link_status_t status)
typedef void (*boost_on_hw_link_status_change_t) BOOST_ON_HW_LINK_STATUS_CHANGE_ARGS;
#define BOOST_ON_HW_LINK_STATUS_CHANGE_FUNCTION(name) void name BOOST_ON_HW_LINK_STATUS_CHANGE_ARGS

/*! 
  \brief Set the signaling status on the given channel.
  \param zchan The openzap channel
  \param status The new status for the channel
  \return ZAP_SUCCESS or ZAP_FAIL 
 */
#define BOOST_SET_SIG_STATUS_ARGS (zap_channel_t *zchan, zap_channel_sig_status_t status)
typedef zap_status_t (*boost_set_sig_status_func_t) BOOST_SET_SIG_STATUS_ARGS;
#define BOOST_SET_SIG_STATUS_FUNCTION(name) zap_status_t BOOST_SET_SIG_STATUS_ARGS

/*! 
  \brief Configure the given span signaling
  \param span The openzap span
  \param ap The list of configuration key,value pairs (must be null terminated)
  \return ZAP_SUCCESS or ZAP_FAIL 
 */
#define BOOST_CONFIGURE_SPAN_ARGS (zap_span_t *span, va_list ap) 
typedef zap_status_t (*boost_configure_span_func_t) BOOST_CONFIGURE_SPAN_ARGS;
#define BOOST_CONFIGURE_SPAN_FUNCTION(name) zap_status_t name BOOST_CONFIGURE_SPAN_ARGS

/*! 
  \brief Start the given span
  \param span The openzap span
  \return ZAP_SUCCESS or ZAP_FAIL 
 */
#define BOOST_START_SPAN_ARGS (zap_span_t *span) 
typedef zap_status_t (*boost_start_span_func_t) BOOST_START_SPAN_ARGS;
#define BOOST_START_SPAN_FUNCTION(name) zap_status_t name BOOST_START_SPAN_ARGS

/*! 
  \brief Stop the given span
  \param span The openzap span
  \return ZAP_SUCCESS or ZAP_FAIL 
 */
#define BOOST_STOP_SPAN_ARGS (zap_span_t *span) 
typedef zap_status_t (*boost_stop_span_func_t) BOOST_START_SPAN_ARGS;
#define BOOST_STOP_SPAN_FUNCTION(name) zap_status_t name BOOST_STOP_SPAN_ARGS

/*! 
  \brief The boost signaling module interface 
 */
typedef struct boost_sigmod_interface_s {
	/*! \brief Module name */
	const char *name;
	/*! \brief write boost message function */
	boost_write_msg_func_t write_msg;	
	/*! \brief set the user write boost message function */
	boost_set_write_msg_cb_func_t set_write_msg_cb;
	/*! \brief set the user signaling status function */
	boost_set_sig_status_cb_func_t set_sig_status_cb;
	/*! \brief get channel signaling status */
	boost_get_sig_status_func_t get_sig_status;
	/*! \brief set channel signaling status */
	boost_set_sig_status_func_t set_sig_status;
	/*! \brief set notify hardware link status change */
	boost_on_hw_link_status_change_t on_hw_link_status_change;
	/*! \brief configure span signaling */
	boost_configure_span_func_t configure_span;
	/*! \brief start openzap span */
	boost_start_span_func_t start_span;
	/*! \brief stop openzap span */
	boost_stop_span_func_t stop_span;
	/*! \brief private pointer for the interface user */
	void *pvt;
} boost_sigmod_interface_t;

#define BOOST_INTERFACE_NAME "boost_sigmod_interface"
/* use this in your sig boost module to declare your interface */
#define BOOST_INTERFACE boost_sigmod_interface_t BOOST_INTERFACE_NAME
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

