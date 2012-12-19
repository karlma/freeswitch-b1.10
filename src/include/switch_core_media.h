/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2012, Anthony Minessale II <anthm@freeswitch.org>
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
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthm@freeswitch.org>
 *
 * switch_core_media.c -- Core Media
 *
 */

#ifndef SWITCH_CORE_MEDIA_H
#define SWITCH_CORE_MEDIA_H

#include <switch.h>

SWITCH_BEGIN_EXTERN_C

typedef enum {
	SM_NDLB_ALLOW_BAD_IANANAME = (1 << 0),
	SM_NDLB_ALLOW_NONDUP_SDP = (1 << 1),
	SM_NDLB_ALLOW_CRYPTO_IN_AVP = (1 << 2),
	SM_NDLB_DISABLE_SRTP_AUTH = (1 << 3)
} switch_core_media_NDLB_t;

struct switch_media_handle_s;


SWITCH_DECLARE(switch_status_t) switch_media_handle_create(switch_media_handle_t **smhp, switch_core_session_t *session);
SWITCH_DECLARE(switch_media_handle_t *) switch_core_session_get_media_handle(switch_core_session_t *session);
SWITCH_DECLARE(switch_status_t) switch_core_session_clear_media_handle(switch_core_session_t *session);
SWITCH_DECLARE(switch_status_t) switch_core_session_media_handle_ready(switch_core_session_t *session);
SWITCH_DECLARE(void) switch_media_handle_set_ndlb(switch_media_handle_t *smh, switch_core_media_NDLB_t flag);
SWITCH_DECLARE(void) switch_media_handle_clear_ndlb(switch_media_handle_t *smh, switch_core_media_NDLB_t flag);
SWITCH_DECLARE(int32_t) switch_media_handle_test_ndlb(switch_media_handle_t *smh, switch_core_media_NDLB_t flag);
SWITCH_DECLARE(void) switch_core_session_check_outgoing_crypto(switch_core_session_t *session, const char *sec_var);
SWITCH_DECLARE(const char *) switch_core_sesson_local_crypto_key(switch_core_session_t *session, switch_media_type_t type);
SWITCH_DECLARE(int) switch_core_session_check_incoming_crypto(switch_core_session_t *session, 
															  const char *varname,
															  switch_media_type_t type, const char *crypto, int crypto_tag);

SWITCH_DECLARE(void) switch_core_session_apply_crypto(switch_core_session_t *session, switch_media_type_t type, const char *varname);
SWITCH_DECLARE(void) switch_core_session_get_recovery_crypto_key(switch_core_session_t *session, switch_media_type_t type, const char *varname);

SWITCH_DECLARE(void) switch_core_media_set_rtp_session(switch_core_session_t *session, switch_media_type_t type, switch_rtp_t *rtp_session);

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
