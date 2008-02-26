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
 * Michael Jerris <mike@jerris.com>
 * Paul D. Tinsley <pdt at jackhammer.org>
 *
 *
 * switch_core_rwlock.c -- Main Core Library (read / write locks)
 *
 */

#include <switch.h>
#include "private/switch_core_pvt.h"


SWITCH_DECLARE(switch_status_t) switch_core_session_signal_lock(switch_core_session_t *session)
{
	return switch_mutex_lock(session->signal_mutex);
}

SWITCH_DECLARE(switch_status_t) switch_core_session_signal_unlock(switch_core_session_t *session)
{
	return switch_mutex_unlock(session->signal_mutex);
}

#ifdef SWITCH_DEBUG_RWLOCKS
SWITCH_DECLARE(switch_status_t) switch_core_session_perform_read_lock(switch_core_session_t *session, const char *file, const char *func, int line)
#else
SWITCH_DECLARE(switch_status_t) switch_core_session_read_lock(switch_core_session_t *session)
#endif
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	if (session->rwlock) {
		if (switch_test_flag(session, SSF_DESTROYED)) {
			status = SWITCH_STATUS_FALSE;
#ifdef SWITCH_DEBUG_RWLOCKS
			switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_ERROR, "%s Read lock FAIL\n", switch_channel_get_name(session->channel));
#endif
		} else {
			status = (switch_status_t) switch_thread_rwlock_tryrdlock(session->rwlock);
#ifdef SWITCH_DEBUG_RWLOCKS
			switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_ERROR, "%s Read lock AQUIRED\n",
							  switch_channel_get_name(session->channel));
#endif
		}
	}

	return status;
}

#ifdef SWITCH_DEBUG_RWLOCKS
SWITCH_DECLARE(void) switch_core_session_perform_write_lock(switch_core_session_t *session, const char *file, const char *func, int line)
{

	switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_ERROR, "%s Write lock AQUIRED\n", switch_channel_get_name(session->channel));
#else
SWITCH_DECLARE(void) switch_core_session_write_lock(switch_core_session_t *session)
{
#endif
	switch_thread_rwlock_wrlock(session->rwlock);
}

#ifdef SWITCH_DEBUG_RWLOCKS
SWITCH_DECLARE(void) switch_core_session_perform_rwunlock(switch_core_session_t *session, const char *file, const char *func, int line)
{
	switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, func, line, SWITCH_LOG_ERROR, "%s Read/Write lock CLEARED\n",
					  switch_channel_get_name(session->channel));
#else
SWITCH_DECLARE(void) switch_core_session_rwunlock(switch_core_session_t *session)
{
#endif
	switch_thread_rwlock_unlock(session->rwlock);

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
