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
 * Massimo Cetra <devel@navynet.it> - Timezone functionality
 *
 *
 * softtimer.c -- Software Timer Module
 *
 */

#include <switch.h>
#include <stdio.h>
#include "private/switch_core_pvt.h"

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#define MAX_TICK UINT32_MAX - 1024
#define MS_PER_TICK 10
static switch_memory_pool_t *module_pool = NULL;

static struct {
	int32_t RUNNING;
	int32_t STARTED;
	switch_mutex_t *mutex;
} globals;

#ifdef WIN32
#undef SWITCH_MOD_DECLARE_DATA
#define SWITCH_MOD_DECLARE_DATA __declspec(dllexport)
#endif

SWITCH_MODULE_LOAD_FUNCTION(softtimer_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(softtimer_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(softtimer_runtime);
SWITCH_MODULE_DEFINITION(CORE_SOFTTIMER_MODULE, softtimer_load, softtimer_shutdown, softtimer_runtime);

#define MAX_ELEMENTS 3600
#define IDLE_SPEED 100
#define STEP_MS 1
#define STEP_MIC 1000

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

SWITCH_DECLARE(switch_time_t) switch_timestamp_now(void)
{
	return runtime.timestamp ? runtime.timestamp : switch_time_now();
}


SWITCH_DECLARE(time_t) switch_timestamp(time_t *t)
{
	time_t now = switch_timestamp_now() / APR_USEC_PER_SEC;
	if (t) {
		*t = now;
	}
	return now;
}

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
static int MONO = 1;
#else
static int MONO = 0;
#endif


SWITCH_DECLARE(void) switch_time_set_monotonic(switch_bool_t enable)
{
	MONO = enable ? 1 : 0;
	switch_time_sync();
}

static switch_time_t time_now(int64_t offset)
{
	switch_time_t now;

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	if (MONO) {
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		now = ts.tv_sec * APR_USEC_PER_SEC + (ts.tv_nsec / 1000) + offset;
	} else {
#endif
		now = switch_time_now();

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	}
#endif

	return now;
}

SWITCH_DECLARE(void) switch_time_sync(void)
{
	runtime.reference = switch_time_now();
	runtime.offset = runtime.reference - time_now(0);
	runtime.reference = time_now(runtime.offset);
}

SWITCH_DECLARE(void) switch_sleep(switch_interval_time_t t)
{

#if defined(HAVE_USLEEP)
	usleep(t);
#elif defined(WIN32)
	Sleep((DWORD) ((t) / 1000));
#else
	apr_sleep(t);
#endif

#if 0
#if defined(HAVE_CLOCK_NANOSLEEP) && defined(SWITCH_USE_CLOCK_FUNCS)
	struct timespec ts;
	ts.tv_sec = t / APR_USEC_PER_SEC;
	ts.tv_nsec = (t % APR_USEC_PER_SEC) * 1000;

	clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);

#elif defined(HAVE_USLEEP)
	usleep(t);
#elif defined(WIN32)
	Sleep((DWORD) ((t) / 1000));
#else
	apr_sleep(t);
#endif
#endif

}

static switch_status_t timer_init(switch_timer_t *timer)
{
	timer_private_t *private_info;
	int sanity = 0;

	while (globals.STARTED == 0) {
		switch_yield(100000);
		if (++sanity == 10) {
			break;
		}
	}

	if (globals.RUNNING != 1 || !globals.mutex) {
		return SWITCH_STATUS_FALSE;
	}

	if ((private_info = switch_core_alloc(timer->memory_pool, sizeof(*private_info)))) {
#if defined(WIN32)
		timeBeginPeriod(1);
#endif
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

#define check_roll() if (private_info->roll < TIMER_MATRIX[timer->interval].roll) {	\
		private_info->roll++;											\
		private_info->reference = private_info->start = TIMER_MATRIX[timer->interval].tick;	\
	}																	\


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

static switch_status_t timer_sync(switch_timer_t *timer)
{
	timer_private_t *private_info = timer->private_info;

	if (globals.RUNNING != 1 || private_info->ready == 0) {
		return SWITCH_STATUS_FALSE;
	}

	/* sync the clock */
	private_info->reference = timer->tick = TIMER_MATRIX[timer->interval].tick;

	/* apply timestamp */
	if (timer_step(timer) == SWITCH_STATUS_SUCCESS) {
		/* push the reference into the future 2 more intervals to prevent collision */
		private_info->reference += 2;
	}

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

static switch_status_t timer_check(switch_timer_t *timer, switch_bool_t step)
{
	timer_private_t *private_info = timer->private_info;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	if (globals.RUNNING != 1 || !private_info->ready) {
		return SWITCH_STATUS_SUCCESS;
	}

	check_roll();

	timer->tick = TIMER_MATRIX[timer->interval].tick;

	if (timer->tick < private_info->reference) {
		timer->diff = private_info->reference - timer->tick;
	} else {
		timer->diff = 0;
	}

	if (timer->diff) {
		status = SWITCH_STATUS_FALSE;
	} else if (step) {
		timer_step(timer);
	}


	return status;
}

static switch_status_t timer_destroy(switch_timer_t *timer)
{
	timer_private_t *private_info = timer->private_info;
	if (timer->interval < MAX_ELEMENTS) {
		switch_mutex_lock(globals.mutex);
		TIMER_MATRIX[timer->interval].count--;
		if (TIMER_MATRIX[timer->interval].count == 0) {
			TIMER_MATRIX[timer->interval].tick = 0;
		}
		switch_mutex_unlock(globals.mutex);
	}
	if (private_info) {
		private_info->ready = 0;
	}
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_RUNTIME_FUNCTION(softtimer_runtime)
{
	switch_time_t too_late = STEP_MIC * 1000;
	uint32_t current_ms = 0;
	uint32_t x, tick = 0;
	switch_time_t ts = 0, last = 0;
	int fwd_errs = 0, rev_errs = 0;

	switch_time_sync();

	globals.STARTED = globals.RUNNING = 1;
	switch_mutex_lock(runtime.throttle_mutex);
	runtime.sps = runtime.sps_total;
	switch_mutex_unlock(runtime.throttle_mutex);

	if (MONO) {
		int loops;
		for (loops = 0; loops < 3; loops++) {
			ts = time_now(0);
			/* if it returns the same value every time it won't be of much use. */
			if (ts == last) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Broken MONOTONIC Clock Detected!, Support Disabled.\n");
				MONO = 0;
				runtime.reference = switch_time_now();
				runtime.initiated = runtime.reference;
				break;
			}
			switch_yield(STEP_MIC);
			last = ts;
		}
	}

	ts = 0;
	last = 0;
	fwd_errs = rev_errs = 0;

	while (globals.RUNNING == 1) {
		runtime.reference += STEP_MIC;
		while ((ts = time_now(runtime.offset)) < runtime.reference) {
			if (ts < last) {
				if (MONO) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Virtual Migration Detected! Syncing Clock\n");
					switch_time_sync();
				} else {
					int64_t diff = (int64_t) (ts - last);
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Reverse Clock Skew Detected!\n");
					runtime.reference = switch_time_now();
					current_ms = 0;
					tick = 0;
					runtime.initiated += diff;
					rev_errs++;
				}
			} else {
				rev_errs = 0;
			}
			switch_yield(STEP_MIC);
			last = ts;
		}


		if (ts > (runtime.reference + too_late)) {
			if (MONO) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Virtual Migration Detected! Syncing Clock\n");
				switch_time_sync();
			} else {
				switch_time_t diff = ts - runtime.reference - STEP_MIC;
#ifndef WIN32
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Forward Clock Skew Detected!\n");
#endif
				fwd_errs++;
				runtime.reference = switch_time_now();
				current_ms = 0;
				tick = 0;
				runtime.initiated += diff;
			}
		} else {
			fwd_errs = 0;
		}

		if (fwd_errs > 9 || rev_errs > 9) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Auto Re-Syncing clock.\n");
			switch_time_sync();
			fwd_errs = rev_errs = 0;
		}

		runtime.timestamp = ts;
		current_ms += STEP_MS;
		tick += STEP_MS;

		if (tick >= 1000) {
			if (runtime.sps <= 0) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Over Session Rate of %d!\n", runtime.sps_total);
			}
			switch_mutex_lock(runtime.throttle_mutex);
			runtime.sps_last = runtime.sps_total - runtime.sps;
			runtime.sps = runtime.sps_total;
			switch_mutex_unlock(runtime.throttle_mutex);
			tick = 0;
		}

		if ((current_ms % MS_PER_TICK) == 0) {
			for (x = MS_PER_TICK; x <= MAX_ELEMENTS; x += MS_PER_TICK) {
				if ((current_ms % x) == 0) {
					if (TIMER_MATRIX[x].count) {
						TIMER_MATRIX[x].tick++;
						if (TIMER_MATRIX[x].tick == MAX_TICK) {
							TIMER_MATRIX[x].tick = 0;
							TIMER_MATRIX[x].roll++;
						}
					}
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

/* 
   This converts a struct tm to a switch_time_exp_t
   We have to use UNIX structures to do our exams
   and use switch_* functions for the output.
*/

static void tm2switchtime(struct tm * tm, switch_time_exp_t *xt ) 
{

	if (!xt || !tm) {
	    return;
	}
	memset( xt, 0, sizeof(xt) );

	xt->tm_sec  	= tm->tm_sec;
	xt->tm_min  	= tm->tm_min;
	xt->tm_hour 	= tm->tm_hour;
	xt->tm_mday 	= tm->tm_mday;
	xt->tm_mon  	= tm->tm_mon;
	xt->tm_year 	= tm->tm_year;
	xt->tm_wday 	= tm->tm_wday;
	xt->tm_yday 	= tm->tm_yday;
	xt->tm_isdst 	= tm->tm_isdst;
#if !defined(WIN32) && !defined(__SVR4) && !defined(__sun)

	xt->tm_gmtoff 	= tm->tm_gmtoff;
#endif

	return;
}

/* **************************************************************************
   LOADING OF THE XML DATA - HASH TABLE & MEMORY POOL MANAGEMENT
   ************************************************************************** */

typedef struct {
	switch_memory_pool_t *pool;
	switch_hash_t *hash;
} switch_timezones_list_t;

static switch_timezones_list_t TIMEZONES_LIST = { 0 };
static switch_event_node_t *NODE = NULL;

const char *switch_lookup_timezone( const char *tz_name )
{
	char *value = NULL;

	if ( tz_name && (value = switch_core_hash_find(TIMEZONES_LIST.hash, tz_name))==NULL ) {
	    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Timezone '%s' not found!\n", tz_name);
	}
	
	return value;
}

void switch_load_timezones(switch_bool_t reload)
{
	switch_xml_t xml = NULL, x_lists = NULL, x_list = NULL, cfg = NULL;
	unsigned total = 0;

	if (TIMEZONES_LIST.hash) {
		switch_core_hash_destroy(&TIMEZONES_LIST.hash);
	}

	if (TIMEZONES_LIST.pool) {
		switch_core_destroy_memory_pool(&TIMEZONES_LIST.pool);
	}

	memset(&TIMEZONES_LIST, 0, sizeof(TIMEZONES_LIST));
	switch_core_new_memory_pool(&TIMEZONES_LIST.pool);
	switch_core_hash_init(&TIMEZONES_LIST.hash, TIMEZONES_LIST.pool);

	if ((xml = switch_xml_open_cfg("timezones.conf", &cfg, NULL))) {
		if ((x_lists = switch_xml_child(cfg, "timezones"))) {
			for (x_list = switch_xml_child(x_lists, "zone"); x_list; x_list = x_list->next) {
				const char *name = switch_xml_attr(x_list, "name");
				const char *value= switch_xml_attr(x_list, "value");

				if (switch_strlen_zero(name)) {
					continue;
				}

				if (switch_strlen_zero(value)) {
					continue;
				}

				switch_core_hash_insert(TIMEZONES_LIST.hash, 
										name, 
										switch_core_strdup(TIMEZONES_LIST.pool, value) );
				total++;
			}
		}
		
		switch_xml_free(xml);
	}
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Timezone %sloaded %d definitions\n", reload ? "re" : "", total);
}

static void event_handler(switch_event_t *event)
{
	switch_load_timezones(1);
}

static void tztime(const time_t * const timep, const char *tzstring, struct tm * const tmp );

SWITCH_DECLARE(switch_status_t) switch_strftime_tz(const char *tz, const char *format, char *date, size_t len)
{
	switch_time_t thetime;
	time_t timep;

	const char *tz_name = tz;
	const char *tzdef;

	switch_size_t retsize;

	struct tm tm = { 0 };
	switch_time_exp_t stm;

	thetime = switch_timestamp_now();

	timep =  (thetime) / (int64_t) (1000000);

	if (!switch_strlen_zero(tz_name)) {
		tzdef = switch_lookup_timezone( tz_name );
	} else {
		/* We set the default timezone to GMT. */
		tz_name="GMT";
		tzdef="GMT";
	}
	
	if (tzdef) { /* The lookup of the zone may fail. */
		tztime( &timep, tzdef, &tm );
		tm2switchtime( &tm, &stm );
		switch_strftime(date, &retsize, len, switch_strlen_zero(format) ? "%Y-%m-%d %T" : format, &stm);
		if (!switch_strlen_zero_buf(date)) {
			return SWITCH_STATUS_SUCCESS;
		}
	}
	return SWITCH_STATUS_FALSE;
}

SWITCH_MODULE_LOAD_FUNCTION(softtimer_load)
{
	switch_timer_interface_t *timer_interface;
	module_pool = pool;

	memset(&globals, 0, sizeof(globals));
	switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, module_pool);

	if ((switch_event_bind_removable(modname, SWITCH_EVENT_RELOADXML, NULL, event_handler, NULL, &NODE) != SWITCH_STATUS_SUCCESS)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
	}
	switch_load_timezones(0);

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	timer_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_TIMER_INTERFACE);
	timer_interface->interface_name = "soft";
	timer_interface->timer_init = timer_init;
	timer_interface->timer_next = timer_next;
	timer_interface->timer_step = timer_step;
	timer_interface->timer_sync = timer_sync;
	timer_interface->timer_check = timer_check;
	timer_interface->timer_destroy = timer_destroy;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(softtimer_shutdown)
{

	if (globals.RUNNING) {
		switch_mutex_lock(globals.mutex);
		globals.RUNNING = -1;
		switch_mutex_unlock(globals.mutex);

		while (globals.RUNNING) {
			switch_yield(10000);
		}
	}
#if defined(WIN32)
	timeEndPeriod(1);
#endif

	if (TIMEZONES_LIST.hash) {
		switch_core_hash_destroy(&TIMEZONES_LIST.hash);
	}

	if (TIMEZONES_LIST.pool) {
		switch_core_destroy_memory_pool(&TIMEZONES_LIST.pool);
	}

	return SWITCH_STATUS_NOUNLOAD;
}




/*
 *    This file was originally written for NetBSD and is in the public domain, 
 *    so clarified as of 1996-06-05 by Arthur David Olson (arthur_david_olson@nih.gov).
 *    
 *    Iw was modified by Massimo Cetra in order to be used with Callweaver and Freeswitch.
 */

//#define TESTING_IT 1

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>


#ifdef TESTING_IT
#include <sys/time.h>
#endif


#ifndef TRUE
#define TRUE	1
#endif /* !defined TRUE */

#ifndef FALSE
#define FALSE	0
#endif /* !defined FALSE */



#ifndef TZ_MAX_TIMES
/*
** The TZ_MAX_TIMES value below is enough to handle a bit more than a
** year's worth of solar time (corrected daily to the nearest second) or
** 138 years of Pacific Presidential Election time
** (where there are three time zone transitions every fourth year).
*/
#define TZ_MAX_TIMES	370
#endif /* !defined TZ_MAX_TIMES */

#ifndef TZ_MAX_TYPES

#ifndef NOSOLAR
#define TZ_MAX_TYPES	256 /* Limited by what (unsigned char)'s can hold */
#endif /* !defined NOSOLAR */

#ifdef NOSOLAR
/*
** Must be at least 14 for Europe/Riga as of Jan 12 1995,
** as noted by Earl Chew <earl@hpato.aus.hp.com>.
*/
#define TZ_MAX_TYPES	20	/* Maximum number of local time types */
#endif /* !defined NOSOLAR */

#endif /* !defined TZ_MAX_TYPES */

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
				/* (limited by what unsigned chars can hold) */
#endif /* !defined TZ_MAX_CHARS */

#ifndef TZ_MAX_LEAPS
#define TZ_MAX_LEAPS	50	/* Maximum number of leap second corrections */
#endif /* !defined TZ_MAX_LEAPS */

#ifdef TZNAME_MAX
#define MY_TZNAME_MAX	TZNAME_MAX
#endif /* defined TZNAME_MAX */

#ifndef TZNAME_MAX
#define MY_TZNAME_MAX	255
#endif /* !defined TZNAME_MAX */


#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define JULIAN_DAY		0	/* Jn - Julian day */
#define DAY_OF_YEAR		1	/* n - day of year */
#define MONTH_NTH_DAY_OF_WEEK	2	/* Mm.n.d - month, week, day of week */

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY


#ifndef TZ_MAX_TIMES
/*
** The TZ_MAX_TIMES value below is enough to handle a bit more than a
** year's worth of solar time (corrected daily to the nearest second) or
** 138 years of Pacific Presidential Election time
** (where there are three time zone transitions every fourth year).
*/
#define TZ_MAX_TIMES	370
#endif /* !defined TZ_MAX_TIMES */

#ifndef TZDEFRULES
#define TZDEFRULES	"posixrules"
#endif /* !defined TZDEFRULES */

/*
** The DST rules to use if TZ has no rules and we can't load TZDEFRULES.
** We default to US rules as of 1999-08-17.
** POSIX 1003.1 section 8.1.1 says that the default DST rules are
** implementation dependent; for historical reasons, US rules are a
** common default.
*/
#ifndef TZDEFRULESTRING
#define TZDEFRULESTRING ",M4.1.0,M10.5.0"
#endif /* !defined TZDEFDST */

/* Unlike <ctype.h>'s isdigit, this also works if c < 0 | c > UCHAR_MAX.  */
#define is_digit(c) ((unsigned)(c) - '0' <= 9)

#define BIGGEST(a, b)	(((a) > (b)) ? (a) : (b))

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))



/*
** INITIALIZE(x)
*/

#ifndef GNUC_or_lint
#ifdef lint
#define GNUC_or_lint
#endif /* defined lint */
#ifndef lint
#ifdef __GNUC__
#define GNUC_or_lint
#endif /* defined __GNUC__ */
#endif /* !defined lint */
#endif /* !defined GNUC_or_lint */
#ifdef WIN32
#define GNUC_or_lint
#endif

#ifndef INITIALIZE
#ifdef GNUC_or_lint
#define INITIALIZE(x)	((x) = 0)
#endif /* defined GNUC_or_lint */
#ifndef GNUC_or_lint
#define INITIALIZE(x)
#endif /* !defined GNUC_or_lint */
#endif /* !defined INITIALIZE */


#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY


/* **************************************************************************
	    
   ************************************************************************** */

static const char	gmt[] = "GMT";

#define CHARS_DEF BIGGEST(BIGGEST(TZ_MAX_CHARS + 1, sizeof gmt), (2 * (MY_TZNAME_MAX + 1)))

struct rule {
	int		r_type;		/* type of rule--see below */
	int		r_day;		/* day number of rule */
	int		r_week;		/* week number of rule */
	int		r_mon;		/* month number of rule */
	long		r_time;		/* transition time of rule */
};

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* UTC offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
	int		tt_ttisstd;	/* TRUE if transition is std time */
	int		tt_ttisgmt;	/* TRUE if transition is UTC */
};

struct lsinfo {				/* leap second information */
	time_t		ls_trans;	/* transition time */
	long		ls_corr;	/* correction to apply */
};


struct state {
	int		leapcnt;
	int		timecnt;
	int		typecnt;
	int		charcnt;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[/* LINTED constant */CHARS_DEF];
	struct lsinfo	lsis[TZ_MAX_LEAPS];
};


static const int	mon_lengths[2][MONSPERYEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const int	year_lengths[2] = {
	DAYSPERNYEAR, DAYSPERLYEAR
};


/* **************************************************************************
	    
   ************************************************************************** */


/*
    Given a pointer into a time zone string, scan until a character that is not
    a valid character in a zone name is found.  Return a pointer to that
    character.
*/

static const char *getzname(register const char *strp)
{
	register char c;

	while ((c = *strp) != '\0' && !is_digit(c) && c != ',' && c != '-' &&
		c != '+')
			++strp;
	return strp;
}


/*
    Given a pointer into a time zone string, extract a number from that string.
    Check that the number is within a specified range; if it is not, return
    NULL.
    Otherwise, return a pointer to the first character not part of the number.
*/

static const char *getnum(register const char *strp, int * const nump, const int min, const int max)
{
	register char	c;
	register int	num;

	if (strp == NULL || !is_digit(c = *strp))
		return NULL;
	num = 0;
	do {
		num = num * 10 + (c - '0');
		if (num > max)
			return NULL;	/* illegal value */
		c = *++strp;
	} while (is_digit(c));
	if (num < min)
		return NULL;		/* illegal value */
	*nump = num;
	return strp;
}

/*
    Given a pointer into a time zone string, extract a number of seconds,
    in hh[:mm[:ss]] form, from the string.
    If any error occurs, return NULL.
    Otherwise, return a pointer to the first character not part of the number
    of seconds.
*/

static const char *getsecs(register const char *strp, long * const secsp)
{
	int	num;

	/*
	** `HOURSPERDAY * DAYSPERWEEK - 1' allows quasi-Posix rules like
	** "M10.4.6/26", which does not conform to Posix,
	** but which specifies the equivalent of
	** ``02:00 on the first Sunday on or after 23 Oct''.
	*/
	strp = getnum(strp, &num, 0, HOURSPERDAY * DAYSPERWEEK - 1);
	if (strp == NULL)
		return NULL;
	*secsp = num * (long) SECSPERHOUR;
	if (*strp == ':') {
		++strp;
		strp = getnum(strp, &num, 0, MINSPERHOUR - 1);
		if (strp == NULL)
			return NULL;
		*secsp += num * SECSPERMIN;
		if (*strp == ':') {
			++strp;
			/* `SECSPERMIN' allows for leap seconds.  */
			strp = getnum(strp, &num, 0, SECSPERMIN);
			if (strp == NULL)
				return NULL;
			*secsp += num;
		}
	}
	return strp;
}

/*
    Given a pointer into a time zone string, extract an offset, in
    [+-]hh[:mm[:ss]] form, from the string.
    If any error occurs, return NULL.
    Otherwise, return a pointer to the first character not part of the time.
*/

static const char *getoffset(register const char *strp, long * const offsetp)
{
	register int	neg = 0;

	if (*strp == '-') {
		neg = 1;
		++strp;
	} else if (*strp == '+')
		++strp;
	strp = getsecs(strp, offsetp);
	if (strp == NULL)
		return NULL;		/* illegal time */
	if (neg)
		*offsetp = -*offsetp;
	return strp;
}

/*
    Given a pointer into a time zone string, extract a rule in the form
    date[/time].  See POSIX section 8 for the format of "date" and "time".
    If a valid rule is not found, return NULL.
    Otherwise, return a pointer to the first character not part of the rule.
*/

static const char *getrule(const char *strp, register struct rule * const rulep)
{
	if (*strp == 'J') {
		/*
		** Julian day.
		*/
		rulep->r_type = JULIAN_DAY;
		++strp;
		strp = getnum(strp, &rulep->r_day, 1, DAYSPERNYEAR);
	} else if (*strp == 'M') {
		/*
		** Month, week, day.
		*/
		rulep->r_type = MONTH_NTH_DAY_OF_WEEK;
		++strp;
		strp = getnum(strp, &rulep->r_mon, 1, MONSPERYEAR);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_week, 1, 5);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_day, 0, DAYSPERWEEK - 1);
	} else if (is_digit(*strp)) {
		/*
		** Day of year.
		*/
		rulep->r_type = DAY_OF_YEAR;
		strp = getnum(strp, &rulep->r_day, 0, DAYSPERLYEAR - 1);
	} else	return NULL;		/* invalid format */
	if (strp == NULL)
		return NULL;
	if (*strp == '/') {
		/*
		** Time specified.
		*/
		++strp;
		strp = getsecs(strp, &rulep->r_time);
	} else	rulep->r_time = 2 * SECSPERHOUR;	/* default = 2:00:00 */
	return strp;
}


/*
    Given the Epoch-relative time of January 1, 00:00:00 UTC, in a year, the
    year, a rule, and the offset from UTC at the time that rule takes effect,
    calculate the Epoch-relative time that rule takes effect.
*/

static time_t transtime(const time_t janfirst, const int year, register const struct rule * const rulep, const long offset)
{
	register int	leapyear;
	register time_t	value;
	register int	i;
	int		d, m1, yy0, yy1, yy2, dow;

	INITIALIZE(value);
	leapyear = isleap(year);
	switch (rulep->r_type) {

	case JULIAN_DAY:
		/*
		** Jn - Julian day, 1 == January 1, 60 == March 1 even in leap
		** years.
		** In non-leap years, or if the day number is 59 or less, just
		** add SECSPERDAY times the day number-1 to the time of
		** January 1, midnight, to get the day.
		*/
		value = janfirst + (rulep->r_day - 1) * SECSPERDAY;
		if (leapyear && rulep->r_day >= 60)
			value += SECSPERDAY;
		break;

	case DAY_OF_YEAR:
		/*
		** n - day of year.
		** Just add SECSPERDAY times the day number to the time of
		** January 1, midnight, to get the day.
		*/
		value = janfirst + rulep->r_day * SECSPERDAY;
		break;

	case MONTH_NTH_DAY_OF_WEEK:
		/*
		** Mm.n.d - nth "dth day" of month m.
		*/
		value = janfirst;
		for (i = 0; i < rulep->r_mon - 1; ++i)
			value += mon_lengths[leapyear][i] * SECSPERDAY;

		/*
		** Use Zeller's Congruence to get day-of-week of first day of
		** month.
		*/
		m1 = (rulep->r_mon + 9) % 12 + 1;
		yy0 = (rulep->r_mon <= 2) ? (year - 1) : year;
		yy1 = yy0 / 100;
		yy2 = yy0 % 100;
		dow = ((26 * m1 - 2) / 10 +
			1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
		if (dow < 0)
			dow += DAYSPERWEEK;

		/*
		** "dow" is the day-of-week of the first day of the month.  Get
		** the day-of-month (zero-origin) of the first "dow" day of the
		** month.
		*/
		d = rulep->r_day - dow;
		if (d < 0)
			d += DAYSPERWEEK;
		for (i = 1; i < rulep->r_week; ++i) {
			if (d + DAYSPERWEEK >=
				mon_lengths[leapyear][rulep->r_mon - 1])
					break;
			d += DAYSPERWEEK;
		}

		/*
		** "d" is the day-of-month (zero-origin) of the day we want.
		*/
		value += d * SECSPERDAY;
		break;
	}

	/*
	** "value" is the Epoch-relative time of 00:00:00 UTC on the day in
	** question.  To get the Epoch-relative time of the specified local
	** time on that day, add the transition time and the current offset
	** from UTC.
	*/
	return value + rulep->r_time + offset;
}



/*
    Given a POSIX section 8-style TZ string, fill in the rule tables as
    appropriate.
*/

static int tzparse(const char *name, register struct state * const sp, const int lastditch)
{
	const char *			stdname;
	const char *			dstname;
	size_t				stdlen;
	size_t				dstlen;
	long				stdoffset;
	long				dstoffset;
	register time_t *		atp;
	register unsigned char *	typep;
	register char *			cp;


	INITIALIZE(dstname);
	stdname = name;

	if (lastditch) {
		stdlen = strlen(name);	/* length of standard zone name */
		name += stdlen;
		if (stdlen >= sizeof sp->chars)
			stdlen = (sizeof sp->chars) - 1;
		stdoffset = 0;
	} else {
		name = getzname(name);
		stdlen = name - stdname;
		if (stdlen < 3)
			return -1;
		if (*name == '\0')
			return -1;
		name = getoffset(name, &stdoffset);
		if (name == NULL)
			return -1;
	}

	sp->leapcnt = 0;		/* so, we're off a little */

	if (*name != '\0') {
		dstname = name;
		name = getzname(name);
		dstlen = name - dstname;	/* length of DST zone name */
		if (dstlen < 3)
			return -1;
		if (*name != '\0' && *name != ',' && *name != ';') 
		{
			name = getoffset(name, &dstoffset);
			if (name == NULL)
				return -1;
		} 
		else	
		    dstoffset = stdoffset - SECSPERHOUR;

		/* Go parsing the daylight saving stuff */
		if (*name == ',' || *name == ';') 
		{
			struct rule	start;
			struct rule	end;
			register int	year;
			register time_t	janfirst;
			time_t		starttime;
			time_t		endtime;

			++name;
			if ((name = getrule(name, &start)) == NULL)
				return -1;
			if (*name++ != ',')
				return -1;
			if ((name = getrule(name, &end)) == NULL)
				return -1;
			if (*name != '\0')
				return -1;

			sp->typecnt = 2;	/* standard time and DST */

			/*
			** Two transitions per year, from EPOCH_YEAR to 2037.
			*/
			sp->timecnt = 2 * (2037 - EPOCH_YEAR + 1);

			if (sp->timecnt > TZ_MAX_TIMES)
				return -1;

			sp->ttis[0].tt_gmtoff = -dstoffset;
			sp->ttis[0].tt_isdst = 1;
			sp->ttis[0].tt_abbrind = (int)(stdlen + 1);
			sp->ttis[1].tt_gmtoff = -stdoffset;
			sp->ttis[1].tt_isdst = 0;
			sp->ttis[1].tt_abbrind = 0;

			atp = sp->ats;
			typep = sp->types;
			janfirst = 0;

			for (year = EPOCH_YEAR; year <= 2037; ++year) {
				starttime = transtime(janfirst, year, &start,
					stdoffset);
				endtime = transtime(janfirst, year, &end,
					dstoffset);
				if (starttime > endtime) {
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
				} else {
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
				}

				janfirst += year_lengths[isleap(year)] * SECSPERDAY;
			}

		} else {
			register long	theirstdoffset;
			register long	theirdstoffset;
			register long	theiroffset;
			register int	isdst;
			register int	i;
			register int	j;

			if (*name != '\0')
				return -1;
			/*
			    Initial values of theirstdoffset and theirdstoffset.
			*/
			theirstdoffset = 0;
			for (i = 0; i < sp->timecnt; ++i) {
				j = sp->types[i];
				if (!sp->ttis[j].tt_isdst) {
					theirstdoffset =
						-sp->ttis[j].tt_gmtoff;
					break;
				}
			}
			theirdstoffset = 0;
			for (i = 0; i < sp->timecnt; ++i) {
				j = sp->types[i];
				if (sp->ttis[j].tt_isdst) {
					theirdstoffset =
						-sp->ttis[j].tt_gmtoff;
					break;
				}
			}
			/*
			** Initially we're assumed to be in standard time.
			*/
			isdst = FALSE;
			theiroffset = theirstdoffset;
			/*
			** Now juggle transition times and types
			** tracking offsets as you do.
			*/
			for (i = 0; i < sp->timecnt; ++i) {
				j = sp->types[i];
				sp->types[i] = (unsigned char)sp->ttis[j].tt_isdst;
				if (sp->ttis[j].tt_ttisgmt) {
					/* No adjustment to transition time */
				} else {
					/*
					** If summer time is in effect, and the
					** transition time was not specified as
					** standard time, add the summer time
					** offset to the transition time;
					** otherwise, add the standard time
					** offset to the transition time.
					*/
					/*
					** Transitions from DST to DDST
					** will effectively disappear since
					** POSIX provides for only one DST
					** offset.
					*/
					if (isdst && !sp->ttis[j].tt_ttisstd) {
						sp->ats[i] += dstoffset -
							theirdstoffset;
					} else {
						sp->ats[i] += stdoffset -
							theirstdoffset;
					}
				}
				theiroffset = -sp->ttis[j].tt_gmtoff;
				if (sp->ttis[j].tt_isdst)
					theirdstoffset = theiroffset;
				else	theirstdoffset = theiroffset;
			}
			/*
			** Finally, fill in ttis.
			** ttisstd and ttisgmt need not be handled.
			*/
			sp->ttis[0].tt_gmtoff = -stdoffset;
			sp->ttis[0].tt_isdst = FALSE;
			sp->ttis[0].tt_abbrind = 0;
			sp->ttis[1].tt_gmtoff = -dstoffset;
			sp->ttis[1].tt_isdst = TRUE;
			sp->ttis[1].tt_abbrind = (int)(stdlen + 1);
			sp->typecnt = 2;
		}
	} else {
		dstlen = 0;
		sp->typecnt = 1;		/* only standard time */
		sp->timecnt = 0;
		sp->ttis[0].tt_gmtoff = -stdoffset;
		sp->ttis[0].tt_isdst = 0;
		sp->ttis[0].tt_abbrind = 0;
	}

	sp->charcnt = (int)(stdlen + 1);
	if (dstlen != 0)
		sp->charcnt += (int)(dstlen + 1);
	if ((size_t) sp->charcnt > sizeof sp->chars)
		return -1;
	cp = sp->chars;
	(void) strncpy(cp, stdname, stdlen);
	cp += stdlen;
	*cp++ = '\0';
	if (dstlen != 0) {
		(void) strncpy(cp, dstname, dstlen);
		*(cp + dstlen) = '\0';
	}
	return 0;
}

/* **************************************************************************
	    
   ************************************************************************** */
#if (_MSC_VER >= 1400)			// VC8+
#define switch_assert(expr) assert(expr);__analysis_assume( expr )
#else
#define switch_assert(expr) assert(expr)
#endif

static void timesub(const time_t * const timep, const long offset, register const struct state * const sp, register struct tm * const tmp)
{
	register const struct lsinfo *	lp;
	register long			days;
	register time_t			rem;
	register int			y;
	register int			yleap;
	register const int *		ip;
	register long			corr;
	register int			hit;
	register int			i;

	switch_assert(timep != NULL);
	switch_assert(sp != NULL);
	switch_assert(tmp != NULL);

	corr = 0;
	hit = 0;
	i = (sp == NULL) ? 0 : sp->leapcnt;

	while (--i >= 0) {
		lp = &sp->lsis[i];
		if (*timep >= lp->ls_trans) {
			if (*timep == lp->ls_trans) {
				hit = ((i == 0 && lp->ls_corr > 0) ||
					(i > 0 && lp->ls_corr > sp->lsis[i - 1].ls_corr));
				if (hit)
					while (i > 0 &&
						sp->lsis[i].ls_trans ==
						sp->lsis[i - 1].ls_trans + 1 &&
						sp->lsis[i].ls_corr ==
						sp->lsis[i - 1].ls_corr + 1) {
							++hit;
							--i;
					}
			}
			corr = lp->ls_corr;
			break;
		}
	}
	days = (long)(*timep / SECSPERDAY);
	rem = *timep % SECSPERDAY;


#ifdef mc68k 
	/* If this is for CPU bugs workarounds, i would remove this anyway. Who would use it on an old mc68k ? */
	if (*timep == 0x80000000) {
		/*
		** A 3B1 muffs the division on the most negative number.
		*/
		days = -24855;
		rem = -11648;
	}
#endif

	rem += (offset - corr);
	while (rem < 0) {
		rem += SECSPERDAY;
		--days;
	}
	while (rem >= SECSPERDAY) {
		rem -= SECSPERDAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECSPERHOUR);
	rem = rem % SECSPERHOUR;
	tmp->tm_min = (int) (rem / SECSPERMIN);

	/*
	** A positive leap second requires a special
	** representation.  This uses "... ??:59:60" et seq.
	*/
	tmp->tm_sec = (int) (rem % SECSPERMIN) + hit;
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYSPERWEEK);

	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYSPERWEEK;

	y = EPOCH_YEAR;

#define LEAPS_THRU_END_OF(y)	((y) / 4 - (y) / 100 + (y) / 400)

	while (days < 0 || days >= (long) year_lengths[yleap = isleap(y)]) {
		register int	newy;

		newy = (int)(y + days / DAYSPERNYEAR);
		if (days < 0)
			--newy;
		days -= (newy - y) * DAYSPERNYEAR +
			LEAPS_THRU_END_OF(newy - 1) -
			LEAPS_THRU_END_OF(y - 1);
		y = newy;
	}

	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;

	ip = mon_lengths[yleap];

	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];

	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
#if !defined(WIN32) && !defined(__SVR4) && !defined(__sun)
	tmp->tm_gmtoff = offset;
#endif
}

/* **************************************************************************
	    
   ************************************************************************** */

static void tztime(const time_t * const timep, const char *tzstring, struct tm * const tmp )
{
	struct state 			*tzptr, 
					*sp;
	const time_t			t = *timep;
	register int			i;
	register const struct ttinfo 	*ttisp;

	if ( tzstring == NULL )
    	        tzstring = gmt;

	tzptr = (struct state *) malloc(sizeof (struct state));
	sp = tzptr;

	if (tzptr != NULL) 
	{
    
		memset(tzptr, 0, sizeof(struct state));

		(void) tzparse(tzstring, tzptr, FALSE);

		if (sp->timecnt == 0 || t < sp->ats[0]) 
		{
			i = 0;
			while (sp->ttis[i].tt_isdst)
				if (++i >= sp->typecnt) {
					i = 0;
					break;
				}
		} else {
			for (i = 1; i < sp->timecnt; ++i)
				if (t < sp->ats[i])
					break;
			i = sp->types[i - 1];	// DST begin or DST end
		}
		ttisp = &sp->ttis[i];

    		/*
		    To get (wrong) behavior that's compatible with System V Release 2.0
		    you'd replace the statement below with
		    t += ttisp->tt_gmtoff;
		    timesub(&t, 0L, sp, tmp);
		*/
		if ( tmp != NULL ) /* Just a check not to assert */
		{
	    		timesub( &t, ttisp->tt_gmtoff, sp, tmp);
			tmp->tm_isdst = ttisp->tt_isdst;
#if !defined(WIN32) && !defined(__SVR4) && !defined(__sun)
			tmp->tm_zone = &sp->chars[ttisp->tt_abbrind];
#endif
		}

		free(tzptr);
	}

}

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
