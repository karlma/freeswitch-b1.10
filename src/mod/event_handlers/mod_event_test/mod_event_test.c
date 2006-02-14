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
 * mod_event_test.c -- Framework Demo Module
 *
 */
#include <switch.h>

static const char modname[] = "mod_event_test";

//#define TORTURE_ME


static void event_handler(switch_event *event)
{
	char buf[1024];

	switch (event->event_id) {
	case SWITCH_EVENT_LOG:
		return;
		break;
	default:
		switch_event_serialize(event, buf, sizeof(buf), NULL);
		switch_console_printf(SWITCH_CHANNEL_CONSOLE, "\nEVENT\n--------------------------------\n%s\n", buf);
		break;
	}
}



static switch_loadable_module_interface event_test_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ NULL,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ NULL,
	/*.application_interface */ NULL
};

#define MY_EVENT_COOL "test::cool"


#ifdef TORTURE_ME
#define TTHREADS 500
static int THREADS = 0;

static void *torture_thread(switch_thread *thread, void *obj)
{
	int y = 0;
	int z = 0;
	switch_core_thread_session *ts = obj;
	switch_event *event;

	z = THREADS++;

	while (THREADS > 0) {
		int x;
		for (x = 0; x < 1; x++) {
			if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, MY_EVENT_COOL) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, "event_info", "hello world %d %d", z, y++);
				switch_event_fire(&event);
			}
		}
		switch_yield(100000);
	}

	if (ts->pool) {
		switch_memory_pool *pool = ts->pool;
		switch_core_destroy_memory_pool(&pool);
	}

	switch_console_printf(SWITCH_CHANNEL_CONSOLE, "Thread Ended\n");
}

SWITCH_MOD_DECLARE(switch_status) switch_module_shutdown(void)
{
	THREADS = -1;
	switch_yield(100000);
	return SWITCH_STATUS_SUCCESS;
}
#endif


SWITCH_MOD_DECLARE(switch_status) switch_module_load(const switch_loadable_module_interface **interface, char *filename)
{
	/* connect my internal structure to the blank pointer passed to me */
	*interface = &event_test_module_interface;

	if (switch_event_reserve_subclass(MY_EVENT_COOL) != SWITCH_STATUS_SUCCESS) {
		switch_console_printf(SWITCH_CHANNEL_CONSOLE, "Couldn't register subclass!");
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_bind((char *) modname, SWITCH_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY, event_handler, NULL) !=
		SWITCH_STATUS_SUCCESS) {
		switch_console_printf(SWITCH_CHANNEL_CONSOLE, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

#ifdef TORTURE_ME
	if (1) {
		int x = 0;
		for (x = 0; x < TTHREADS; x++) {
			switch_core_launch_thread(torture_thread, NULL);
		}
	}
#endif

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}
