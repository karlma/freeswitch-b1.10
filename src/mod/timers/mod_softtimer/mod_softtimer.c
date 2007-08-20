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
 * mod_softtimer.c -- Software Timer Module
 *
 */
#include <switch.h>
#include <stdio.h>

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#define MAX_TICK UINT32_MAX - 1024


static switch_memory_pool_t *module_pool = NULL;

static struct {
	int32_t RUNNING;
	int32_t STARTED;
	switch_mutex_t *mutex;
} globals;

SWITCH_MODULE_LOAD_FUNCTION(mod_softtimer_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_softtimer_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_softtimer_runtime);
SWITCH_MODULE_DEFINITION(mod_softtimer, mod_softtimer_load, mod_softtimer_shutdown, mod_softtimer_runtime);

#define MAX_ELEMENTS 1000

struct timer_private {
	switch_size_t reference;
	switch_size_t start;
	uint32_t roll;
	uint32_t ready;
};
typedef struct timer_private timer_private_t;

struct timer_matrix {
	switch_size_t tick;
	uint32_t count;
	uint32_t roll;
};
typedef struct timer_matrix timer_matrix_t;

static timer_matrix_t TIMER_MATRIX[MAX_ELEMENTS + 1];

#define IDLE_SPEED 100


static switch_status_t timer_init(switch_timer_t *timer)
{
	timer_private_t *private_info;
	int sanity = 0;

	while(globals.STARTED == 0) {
		switch_yield(100000);
		if (++sanity == 10) {
			break;
		}
	}

	if (globals.RUNNING != 1 || !globals.mutex) {
		return SWITCH_STATUS_FALSE;
	}

	if ((private_info = switch_core_alloc(timer->memory_pool, sizeof(*private_info)))) {
		switch_mutex_lock(globals.mutex);
		TIMER_MATRIX[timer->interval].count++;
		switch_mutex_unlock(globals.mutex);
		timer->private_info = private_info;
		private_info->start = private_info->reference = TIMER_MATRIX[timer->interval].tick;
		private_info->roll = TIMER_MATRIX[timer->interval].roll;
		private_info->ready = 1;
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_MEMERR;
}


#define check_roll() if (private_info->roll < TIMER_MATRIX[timer->interval].roll) {\
		private_info->roll++;\
		private_info->reference = private_info->start = TIMER_MATRIX[timer->interval].tick;\
	}\



static switch_status_t timer_step(switch_timer_t *timer)
{
	timer_private_t *private_info = timer->private_info;
	uint64_t samples;

	if (globals.RUNNING != 1 || private_info->ready == 0) {
		return SWITCH_STATUS_FALSE;
	}

	check_roll();
	samples = timer->samples * (private_info->reference - private_info->start);

	if (samples > UINT32_MAX) {
		private_info->start = private_info->reference;
		samples = timer->samples;
	}

	timer->samplecount = (uint32_t) samples;
	private_info->reference++;

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t timer_next(switch_timer_t *timer)
{
	timer_private_t *private_info = timer->private_info;

	timer_step(timer);

	while (globals.RUNNING == 1 && private_info->ready && TIMER_MATRIX[timer->interval].tick < private_info->reference) {
		check_roll();
		switch_yield(1000);
	}

	if (globals.RUNNING == 1) {
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

static switch_status_t timer_check(switch_timer_t *timer)
{
	timer_private_t *private_info = timer->private_info;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_size_t diff;

	if (globals.RUNNING != 1 || !private_info->ready) {
		return SWITCH_STATUS_SUCCESS;
	}

	check_roll();

	if (TIMER_MATRIX[timer->interval].tick < private_info->reference) {
		diff = private_info->reference - TIMER_MATRIX[timer->interval].tick;
	} else {
		diff = 0;
	}

	if (diff) {
		status = SWITCH_STATUS_FALSE;
	} else {
		timer_step(timer);
	}

	return status;
}


static switch_status_t timer_destroy(switch_timer_t *timer)
{
	timer_private_t *private_info = timer->private_info;
	switch_mutex_lock(globals.mutex);
	TIMER_MATRIX[timer->interval].count--;
	if (TIMER_MATRIX[timer->interval].count == 0) {
		TIMER_MATRIX[timer->interval].tick = 0;
	}
	switch_mutex_unlock(globals.mutex);
	private_info->ready = 0;
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_softtimer_load)
{
	switch_timer_interface_t *timer_interface;
	module_pool = pool;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	timer_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_TIMER_INTERFACE);
	timer_interface->interface_name = "soft";
	timer_interface->timer_init = timer_init;
	timer_interface->timer_next = timer_next;
	timer_interface->timer_step = timer_step;
	timer_interface->timer_check = timer_check;
	timer_interface->timer_destroy = timer_destroy;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

/* I cant resist setting this to 10ms, we dont even run anything smaller than 20ms so this is already 
   twice the granularity we need, we'll change it if we need anything smaller
*/

#define STEP_MS 1
#define STEP_MIC 1000

SWITCH_MODULE_RUNTIME_FUNCTION(mod_softtimer_runtime)
{
	switch_time_t reference = switch_time_now();
	uint32_t current_ms = 0;
	uint32_t x;

	memset(&globals, 0, sizeof(globals));
	switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, module_pool);
	
	globals.STARTED = globals.RUNNING = 1;
	
	while (globals.RUNNING == 1) {
		reference += STEP_MIC;

		while (switch_time_now() < reference) {
			switch_yield(STEP_MIC);
		}

		current_ms += STEP_MS;

		for (x = 0; x < MAX_ELEMENTS; x++) {
			int i = x, index;
			if (i == 0) {
				i = 1;
			}

			index = (current_ms % i == 0) ? i : 0;

			if (TIMER_MATRIX[index].count) {
				TIMER_MATRIX[index].tick++;
				if (TIMER_MATRIX[index].tick == MAX_TICK) {
					TIMER_MATRIX[index].tick = 0;
					TIMER_MATRIX[index].roll++;
				}
			}
		}

		if (current_ms == MAX_ELEMENTS) {
			current_ms = 0;
		}
	}

	switch_mutex_lock(globals.mutex);
	globals.RUNNING = 0;
	switch_mutex_unlock(globals.mutex);

	return SWITCH_STATUS_TERM;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_softtimer_shutdown)
{

	if (globals.RUNNING) {
		switch_mutex_lock(globals.mutex);
		globals.RUNNING = -1;
		switch_mutex_unlock(globals.mutex);

		while (globals.RUNNING) {
			switch_yield(10000);
		}
	}

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
