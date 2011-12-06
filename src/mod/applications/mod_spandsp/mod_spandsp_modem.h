/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2011, Anthony Minessale II <anthm@freeswitch.org>
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
 * The Original Code is FreeSWITCH mod_fax.
 *
 * The Initial Developer of the Original Code is
 * Massimo Cetra <devel@navynet.it>
 *
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Brian West <brian@freeswitch.org>
 * Anthony Minessale II <anthm@freeswitch.org>
 * Steve Underwood <steveu@coppice.org>
 * mod_spandsp_modem.h -- Fax modem applications provided by SpanDSP
 *
 */

#include "switch_private.h"
#if defined(HAVE_OPENPTY) || defined(HAVE_DEV_PTMX) || defined(HAVE_POSIX_OPENPT)
#define MODEM_SUPPORT 1
#if !defined(HAVE_POSIX_OPENPT) && !defined(HAVE_DEV_PTMX)
#define USE_OPENPTY 1
#endif
#ifndef _MOD_SPANDSP_MODEM_H
#define _MOD_SPANDSP_MODEM_H

#include <stdio.h>
#include <string.h>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <byteswap.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <tiffio.h>
#include <spandsp.h>


typedef enum {
	MODEM_STATE_INIT,	
	MODEM_STATE_ONHOOK,	
	MODEM_STATE_OFFHOOK,	
	MODEM_STATE_ACQUIRED,
	MODEM_STATE_RINGING,
	MODEM_STATE_ANSWERED,
	MODEM_STATE_DIALING,
	MODEM_STATE_CONNECTED,
	MODEM_STATE_HANGUP,
	MODEM_STATE_LAST
} modem_state_t;

struct modem;

typedef int (*modem_control_handler_t)(struct modem *, const char *, int);


typedef enum {
	MODEM_FLAG_RUNNING = ( 1 << 0),
	MODEM_FLAG_XOFF = ( 1 << 1)
} modem_flags;

struct modem {
	t31_state_t *t31_state;
	char digits[512];
	modem_flags flags;
	int master;
	int slave;
	char *stty;
	char devlink[128];
	int id;
	modem_state_t state;
	modem_control_handler_t control_handler;
	void *user_data;
	switch_mutex_t *mutex;
	char uuid_str[SWITCH_UUID_FORMATTED_LENGTH + 1];
	switch_time_t last_event;
	int slot;
};

typedef struct modem modem_t;

char *modem_state2name(int state);
int modem_close(struct modem *fm);
int modem_init(struct modem *fm, modem_control_handler_t control_handler);

#endif //MODEM_SUPPORT

switch_status_t modem_global_init(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool);
void modem_global_shutdown(void);

#endif //_MOD_SPANDSP_MODEM_H
