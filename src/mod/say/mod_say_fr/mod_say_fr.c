/*
 * Copyright (c) 2007-2012, Anthony Minessale II
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
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthm@freeswitch.org>
 * Michael B. Murdock <mike@mmurdock.org>
 * Marc O. Chouinard <mochouinard@moctel.com>
 *
 * mod_say_fr.c -- Say for french
 *
 */

#include <switch.h>
#include <math.h>
#include <ctype.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_say_fr_load);
SWITCH_MODULE_DEFINITION(mod_say_fr, mod_say_fr_load, NULL, NULL);

#define say_num(num, meth) {											\
		char tmp[80];													\
		switch_status_t tstatus;										\
		switch_say_method_t smeth = say_args->method;					\
		switch_say_type_t stype = say_args->type;						\
		say_args->type = SST_ITEMS; say_args->method = meth;			\
		switch_snprintf(tmp, sizeof(tmp), "%u", (unsigned)num);			\
		if ((tstatus =													\
			 fr_say_general_count(session, tmp, say_args, args))		\
			!= SWITCH_STATUS_SUCCESS) {									\
			return tstatus;												\
		}																\
		say_args->method = smeth; say_args->type = stype;				\
	}																	\

#define say_file(...) {													\
		char tmp[80];													\
		switch_status_t tstatus;										\
		switch_snprintf(tmp, sizeof(tmp), __VA_ARGS__);					\
		if ((tstatus =													\
			 switch_ivr_play_file(session, NULL, tmp, args))			\
			!= SWITCH_STATUS_SUCCESS){									\
			return tstatus;												\
		}																\
		if (!switch_channel_ready(switch_core_session_get_channel(session))) { \
			return SWITCH_STATUS_FALSE;									\
		}}																\

static switch_status_t play_group(switch_say_method_t method, switch_say_gender_t gender, int a, int b, int c, char *what, switch_core_session_t *session, switch_input_args_t *args)
{
	int ftdNumber = 0;
	int itd = (b * 10) + c;

	/* Force full 2 digit playback */ 
	if (itd <= 19)
		ftdNumber = 1;
	switch (itd) {
		case 21:
		case 31:
			ftdNumber = 1;
	}
	/*switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "a=%d  b=[%d]  c=%d\n",a, b,c); */

	if (a && a !=0) {
		/*switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "a=%d  b=[%d]  c=%d\n",a, b,c);*/
		if (a != 1)
			say_file("digits/%d.wav", a);
		say_file("digits/hundred.wav");
	}

	if (b && ftdNumber == 0) {
		/*switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "a=%d  b=[%d]  c=%d\n",a, b,c);*/
		if (c == 0) {
			if (method == SSM_COUNTED) {
				say_file("digits/h-%d%d.wav", b, c);
			} else {
				say_file("digits/%d%d.wav", b, c);
			}
		} else {
			say_file("digits/%d0.wav", b);
		}
	}

	if (c || (ftdNumber == 1 && (a || b || c))) {
		/*switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "a=%d  b=[%d]  c=%d\n",a, b,c);*/
		int fVal = c;
		if (ftdNumber == 1)
			fVal = itd;
			
		if (method == SSM_COUNTED) {
			say_file("digits/h-%d.wav", fVal);
		} else {
			if (b != 1 && c == 1 && gender == SSG_FEMININE) {
				say_file("digits/%d_f.wav", fVal);
			} else {
				say_file("digits/%d.wav", fVal);
			}
		}
	}

	if (what && (a || b || c)) {
		say_file(what);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t fr_say_general_count(switch_core_session_t *session, char *tosay, switch_say_args_t *say_args, switch_input_args_t *args)
{
	int in;
	int x = 0;
	int places[9] = { 0 };
	char sbuf[128] = "";
	switch_status_t status;

	if (say_args->method == SSM_ITERATED) {
		if ((tosay = switch_strip_commas(tosay, sbuf, sizeof(sbuf)-1))) {
			char *p;
			for (p = tosay; p && *p; p++) {
				if (*p == '1' && say_args->gender == SSG_FEMININE) {
					say_file("digits/%c_f.wav", *p);
				} else {
					say_file("digits/%c.wav", *p);
				}
			}
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Parse Error!\n");
			return SWITCH_STATUS_GENERR;
		}
		return SWITCH_STATUS_SUCCESS;
	}

	if (!(tosay = switch_strip_commas(tosay, sbuf, sizeof(sbuf)-1)) || strlen(tosay) > 9) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Parse Error!\n");
		return SWITCH_STATUS_GENERR;
	}

	in = atoi(tosay);

	if (in != 0) {
		for (x = 8; x >= 0; x--) {
			int num = (int) pow(10, x);
			if ((places[(uint32_t) x] = in / num)) {
				in -= places[(uint32_t) x] * num;
			}
		}

		switch (say_args->method) {
		case SSM_COUNTED:
		case SSM_PRONOUNCED:
			if ((status = play_group(SSM_PRONOUNCED, say_args->gender, places[8], places[7], places[6], "digits/million.wav", session, args)) != SWITCH_STATUS_SUCCESS) {
				return status;
			}
			if ((status = play_group(SSM_PRONOUNCED, say_args->gender, places[5], places[4], places[3], "digits/thousand.wav", session, args)) != SWITCH_STATUS_SUCCESS) {
				return status;
			}
			if ((status = play_group(say_args->method, say_args->gender, places[2], places[1], places[0], NULL, session, args)) != SWITCH_STATUS_SUCCESS) {
				return status;
			}
			break;
		default:
			break;
		}
	} else {
		say_file("digits/0.wav");
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t fr_say_time(switch_core_session_t *session, char *tosay, switch_say_args_t *say_args, switch_input_args_t *args)
{
	int32_t t;
	switch_time_t target = 0, target_now = 0;
	switch_time_exp_t tm, tm_now;
	uint8_t say_date = 0, say_time = 0, say_year = 0, say_month = 0, say_dow = 0, say_day = 0, say_yesterday = 0, say_today = 0;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	const char *tz = switch_channel_get_variable(channel, "timezone");

	if (say_args->type == SST_TIME_MEASUREMENT) {
		int64_t hours = 0;
		int64_t minutes = 0;
		int64_t seconds = 0;
		int64_t r = 0;

		if (strchr(tosay, ':')) {
			char *tme = switch_core_session_strdup(session, tosay);
			char *p;

			if ((p = strrchr(tme, ':'))) {
				*p++ = '\0';
				seconds = atoi(p);
				if ((p = strchr(tme, ':'))) {
					*p++ = '\0';
					minutes = atoi(p);
					if (tme) {
						hours = atoi(tme);
					}
				} else {
					minutes = atoi(tme);
				}
			}
		} else {
			if ((seconds = atol(tosay)) <= 0) {
				seconds = (int64_t) switch_epoch_time_now(NULL);
			}

			if (seconds >= 60) {
				minutes = seconds / 60;
				r = seconds % 60;
				seconds = r;
			}

			if (minutes >= 60) {
				hours = minutes / 60;
				r = minutes % 60;
				minutes = r;
			}
		}

		say_args->gender = SSG_FEMININE;

		if (hours) {
			say_num(hours, SSM_PRONOUNCED);
			say_file("time/hour.wav");
		} else {
			/* TODO MINUIT */
			say_file("digits/0.wav");
			say_file("time/hour.wav");
		}

		if (minutes) {
			say_num(minutes, SSM_PRONOUNCED);
			say_file("time/minute.wav");
		} else {
			/* TODO Aucune idee quoi faire jouer icit */
			say_file("digits/0.wav");
			say_file("time/minute.wav");
		}

		if (seconds) {
			say_num(seconds, SSM_PRONOUNCED);
			say_file("time/second.wav");
		} else {
			/* TODO Aucune idee quoi faire jouer icit */
			say_file("digits/0.wav");
			say_file("time/second.wav");
		}

		return SWITCH_STATUS_SUCCESS;
	}

	if ((t = atol(tosay)) > 0) {
		target = switch_time_make(t, 0);
		target_now = switch_micro_time_now();
	} else {
		target = switch_micro_time_now();
		target_now = switch_micro_time_now();
	}

	if (tz) {
		int check = atoi(tz);
		if (check) {
			switch_time_exp_tz(&tm, target, check);
			switch_time_exp_tz(&tm_now, target_now, check);
		} else {
			switch_time_exp_tz_name(tz, &tm, target);
			switch_time_exp_tz_name(tz, &tm_now, target_now);
		}
	} else {
		switch_time_exp_lt(&tm, target);
		switch_time_exp_lt(&tm_now, target_now);
	}

	switch (say_args->type) {
	case SST_CURRENT_DATE_TIME:
		say_date = say_time = 1;
		break;
	case SST_CURRENT_DATE:
		say_date = 1;
		break;
	case SST_CURRENT_TIME:
		say_time = 1;
		break;
	case SST_SHORT_DATE_TIME:
		say_time = 1;
		if (tm.tm_year != tm_now.tm_year) {
			say_date = 1;
			break;
		}
		if (tm.tm_yday == tm_now.tm_yday) {
			say_today = 1;
			break;
		}
		if (tm.tm_yday == tm_now.tm_yday - 1) {
			say_yesterday = 1;
			break;
		}
		if (tm.tm_yday >= tm_now.tm_yday - 5) {
			say_dow = 1;
			break;
		}
		if (tm.tm_mon != tm_now.tm_mon) {
			say_month = say_day = say_dow = 1;
			break;
		}

		say_month = say_day = say_dow = 1;

		break;

	default:
		break;
	}

	if (say_today) {
		say_file("time/today.wav");
	}
	if (say_yesterday) {
		say_file("time/yesterday.wav");
	}
	if (say_dow) {
		say_file("time/day-%d.wav", tm.tm_wday);
	}

	if (say_date) {
		say_year = say_month = say_day = say_dow = 1;
		say_today = say_yesterday = 0;
	}

        if (say_day) {
		if (tm.tm_mday == 1) { /* 1 er Janvier,... 2 feb, 23 dec... */
			say_args->gender = SSG_MASCULINE;
	                say_num(tm.tm_mday, SSM_COUNTED);
		} else {
			say_args->gender = SSG_FEMININE;
			say_num(tm.tm_mday, SSM_PRONOUNCED);
		}
        }
	if (say_month) {
		say_file("time/mon-%d.wav", tm.tm_mon);
	}
	if (say_year) {
		say_args->gender = SSG_MASCULINE;
		say_num(tm.tm_year + 1900, SSM_PRONOUNCED);
	}

	if (say_time) {
		if (say_date || say_today || say_yesterday || say_dow) {
			say_file("time/at.wav");
		}
		say_args->gender = SSG_FEMININE;
		say_num(tm.tm_hour, SSM_PRONOUNCED);

		say_file("time/hour.wav");

		if (tm.tm_min) {
			say_num(tm.tm_min, SSM_PRONOUNCED);
		}

	}

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t fr_say_money(switch_core_session_t *session, char *tosay, switch_say_args_t *say_args, switch_input_args_t *args)
{
	char sbuf[16] = "";			/* enough for 999,999,999,999.99 (w/o the commas or leading $) */
	char *dollars = NULL;
	char *cents = NULL;

	if (strlen(tosay) > 15 || !(tosay = switch_strip_nonnumerics(tosay, sbuf, sizeof(sbuf)-1))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Parse Error!\n");
		return SWITCH_STATUS_GENERR;
	}

	dollars = sbuf;

	if ((cents = strchr(sbuf, '.'))) {
		*cents++ = '\0';
		if (strlen(cents) > 2) {
			cents[2] = '\0';
		}
	}

	/* If positive sign - skip over" */
	if (sbuf[0] == '+') {
		dollars++;
	}

	/* If negative say "negative" */
	if (sbuf[0] == '-') {
		say_file("currency/negative.wav");
		dollars++;
	}

	/* Say dollar amount */
	fr_say_general_count(session, dollars, say_args, args);
	if (atoi(dollars) == 1) {
		say_file("currency/dollar.wav");
	} else {
		say_file("currency/dollars.wav");
	}

	/* Say "and" */
	say_file("currency/and.wav");

	/* Say cents */
	if (cents) {
		fr_say_general_count(session, cents, say_args, args);
		if (atoi(cents) == 1) {
			say_file("currency/cent.wav");
		} else {
			say_file("currency/cents.wav");
		}
	} else {
		say_file("digits/0.wav");
		say_file("currency/cents.wav");
	}

	return SWITCH_STATUS_SUCCESS;
}



static switch_status_t fr_say(switch_core_session_t *session, char *tosay, switch_say_args_t *say_args, switch_input_args_t *args)
{
	switch_say_callback_t say_cb = NULL;

	switch (say_args->type) {
	case SST_NUMBER:
	case SST_ITEMS:
	case SST_PERSONS:
	case SST_MESSAGES:
		say_cb = fr_say_general_count;
		break;
	case SST_TIME_MEASUREMENT:
	case SST_CURRENT_DATE:
	case SST_CURRENT_TIME:
	case SST_CURRENT_DATE_TIME:
	case SST_SHORT_DATE_TIME:
		say_cb = fr_say_time;
		break;
	case SST_IP_ADDRESS:
		return switch_ivr_say_ip(session, tosay, fr_say_general_count, say_args, args);
		break;
	case SST_TELEPHONE_NUMBER:
	case SST_TELEPHONE_EXTENSION:
	case SST_URL:
	case SST_EMAIL_ADDRESS:
	case SST_POSTAL_ADDRESS:
	case SST_ACCOUNT_NUMBER:
	case SST_NAME_SPELLED:
	case SST_NAME_PHONETIC:
		return switch_ivr_say_spell(session, tosay, say_args, args);
		break;
	case SST_CURRENCY:
		say_cb = fr_say_money;
		break;
	default:
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unknown Say type=[%d]\n", say_args->type);
		break;
	}

	if (say_cb) {
		return say_cb(session, tosay, say_args, args);
	}

	return SWITCH_STATUS_FALSE;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_say_fr_load)
{
	switch_say_interface_t *say_interface;
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	say_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_SAY_INTERFACE);
	say_interface->interface_name = "fr";
	say_interface->say_function = fr_say;

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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
