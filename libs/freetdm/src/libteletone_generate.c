/* 
 * libteletone
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
 * The Original Code is libteletone
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
 * libteletone.c -- Tone Generator
 *
 *
 *
 * Exception:
 * The author hereby grants the use of this source code under the 
 * following license if and only if the source code is distributed
 * as part of the openzap library.  Any use or distribution of this
 * source code outside the scope of the openzap library will nullify the
 * following license and reinact the MPL 1.1 as stated above.
 *
 * Copyright (c) 2007, Anthony Minessale II
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

#include <libteletone.h>
#define SMAX 32767
#define SMIN -32768
#define normalize_to_16bit(n) if (n > SMAX) n = SMAX; else if (n < SMIN) n = SMIN;

#ifdef _MSC_VER
#pragma warning(disable:4706)
#endif


int teletone_set_tone(teletone_generation_session_t *ts, int index, ...)
{
	va_list ap;
	int i = 0;
	teletone_process_t x = 0;

	va_start(ap, index);
	while (i <= TELETONE_MAX_TONES && (x = va_arg(ap, teletone_process_t))) {
		ts->TONES[index].freqs[i++] = x;
	}
	va_end(ap);

	return (i > TELETONE_MAX_TONES) ? -1 : 0;
	
}

int teletone_set_map(teletone_tone_map_t *map, ...)
{
	va_list ap;
	int i = 0;
	teletone_process_t x = 0;

	va_start(ap, map);
	while (i <= TELETONE_MAX_TONES && (x = va_arg(ap, teletone_process_t))) {
		map->freqs[i++] = x;
	}
	va_end(ap);

	return (i > TELETONE_MAX_TONES) ? -1 : 0;
	
}

int teletone_init_session(teletone_generation_session_t *ts, int buflen, tone_handler handler, void *user_data)
{
	memset(ts, 0, sizeof(*ts));
	ts->rate = 8000;
	ts->channels = 1;
	ts->duration = 2000;
	ts->wait = 500;
	ts->tmp_duration = -1;
	ts->tmp_wait = -1;
	ts->handler = handler;
	ts->user_data = user_data;
	ts->volume = 1500;
	ts->decay_step = 0;
	if (buflen) {
		if ((ts->buffer = calloc(buflen, sizeof(teletone_audio_t))) == 0) {
			return -1;
		}
		ts->datalen = buflen;
	} else {
		ts->dynamic = 1024;
	}
	/* Add Standard DTMF Tones */
	teletone_set_tone(ts, '1', 697.0, 1209.0, 0.0);
	teletone_set_tone(ts, '2', 697.0, 1336.0, 0.0);
	teletone_set_tone(ts, '3', 697.0, 1477.0, 0.0);
	teletone_set_tone(ts, 'A', 697.0, 1633.0, 0.0);
	teletone_set_tone(ts, '4', 770.0, 1209.0, 0.0);
	teletone_set_tone(ts, '5', 770.0, 1336.0, 0.0);
	teletone_set_tone(ts, '6', 770.0, 1477.0, 0.0);
	teletone_set_tone(ts, 'B', 770.0, 1633.0, 0.0);
	teletone_set_tone(ts, '7', 859.0, 1209.0, 0.0);
	teletone_set_tone(ts, '8', 859.0, 1336.0, 0.0);
	teletone_set_tone(ts, '9', 859.0, 1477.0, 0.0);
	teletone_set_tone(ts, 'C', 859.0, 1633.0, 0.0);
	teletone_set_tone(ts, '*', 941.0, 1209.0, 0.0);
	teletone_set_tone(ts, '0', 941.0, 1336.0, 0.0);
	teletone_set_tone(ts, '#', 941.0, 1477.0, 0.0);
	teletone_set_tone(ts, 'D', 941.0, 1633.0, 0.0);
	
	return 0;
}

int teletone_destroy_session(teletone_generation_session_t *ts)
{
	if (ts->buffer) {
		free(ts->buffer);
		ts->buffer = NULL;
		ts->samples = 0;
	}
	return 0;
}

static int ensure_buffer(teletone_generation_session_t *ts, int need)
{
	need += ts->samples;
	need *= sizeof(teletone_audio_t);
	need *= ts->channels;

	if (need > ts->datalen) {
		ts->datalen = need + ts->dynamic;
		if (!(ts->buffer = realloc(ts->buffer, ts->datalen))) {
			return -1;
		}
	}

	return 0;
}

int teletone_mux_tones(teletone_generation_session_t *ts, teletone_tone_map_t *map)
{
	teletone_process_t period = (1.0 / ts->rate) / ts->channels;
	int i, c;
	int freqlen = 0;
	teletone_process_t tones[TELETONE_MAX_TONES];
	int decay = 0;
	int duration;
	int wait = 0;
	teletone_process_t sample;
	
	ts->samples = 0;

	duration = (ts->tmp_duration > -1) ? ts->tmp_duration : ts->duration;
	wait = (ts->tmp_wait > -1) ? ts->tmp_wait : ts->wait;

	if (map->freqs[0] > 0) {
		if (ts->decay_step) {
			if (ts->decay_factor) {
				decay = (duration - (duration / ts->decay_factor));
			} else {
				decay = 0;
			}
		}
		if (ts->volume < 0) {
			ts->volume = 0;
		}
	
		for (freqlen = 0; map->freqs[freqlen] && freqlen < TELETONE_MAX_TONES; freqlen++) {
			tones[freqlen] = (teletone_process_t) map->freqs[freqlen] * (2 * M_PI);
		}
	
		if (ts->channels > 1) {
			duration *= ts->channels;
		}

		if (ts->dynamic) {
			if (ensure_buffer(ts, duration)) {
				return -1;
			}
		}
		for (ts->samples = 0; ts->samples < ts->datalen && ts->samples < duration; ts->samples++) {
			if (ts->decay_step && !(ts->samples % ts->decay_step) && ts->volume > 0 && ts->samples > decay) {
				ts->volume += ts->decay_direction;
			}

			sample = (teletone_process_t) 128;

			for (i = 0; i < freqlen; i++) {
				sample += ((teletone_process_t) 2 * (ts->volume > 0 ? ts->volume : 1) * cos(tones[i] * ts->samples * period));
			}
			normalize_to_16bit(sample);
			ts->buffer[ts->samples] = (teletone_audio_t)sample;
			
			for (c = 1; c < ts->channels; c++) {
				ts->buffer[ts->samples+1] = ts->buffer[ts->samples];
				ts->samples++;
			}
			
		}
	}
	if (ts->dynamic) {
		if (ensure_buffer(ts, wait)) {
			return -1;
		}
	}
	for (c = 0; c < ts->channels; c++) {
		for (i = 0; i < wait && ts->samples < ts->datalen; i++) {
			ts->buffer[ts->samples++] = 0;
		}
	}

	if (ts->debug && ts->debug_stream) {
		if (map->freqs[0] <= 0) {
			fprintf(ts->debug_stream, "wait %d (%dms)\n", wait, wait / (ts->rate / 1000));
		} else {
			fprintf(ts->debug_stream, "Generate: (");

			for (i = 0; i < TELETONE_MAX_TONES && map->freqs[i]; i++) {
				fprintf(ts->debug_stream, "%s%0.2f", i == 0 ? "" : "+",map->freqs[i]);
			}
			 
			fprintf(ts->debug_stream, ") [volume %d; samples %d(%dms) x %d channel%s; wait %d(%dms); decay_factor %d; decay_step %d; wrote %d bytes]\n",
					ts->volume,
					duration,
					duration / (ts->rate / 1000), 
					ts->channels,
					ts->channels == 1 ? "" : "s",
					wait,
					wait / (ts->rate / 1000),
					ts->decay_factor,
					ts->decay_step,
					ts->samples * 2);
		}
	}	
	return ts->samples;
}

/* don't ask */
static char *my_strdup (const char *s)
{
	size_t len = strlen (s) + 1;
	void *new = malloc (len);
	
	if (new == NULL) {
		return NULL;
	}

	return (char *) memcpy (new, s, len);
}

int teletone_run(teletone_generation_session_t *ts, char *cmd)
{
	char *data = NULL, *cur = NULL, *end = NULL;
	int var = 0, LOOPING = 0;
	
	if (!cmd) {
		return -1;
	}

	do {
		if (!(data = my_strdup(cmd))) {
			return -1;
		}

		cur = data;

		while (*cur) {
			var = 0;
			if (*cur == ' ' || *cur == '\r' || *cur == '\n') {
				cur++;
				continue;
			}

			if ((end = strchr(cur, ';')) != 0) {
				*end++ = '\0';
			}
			
			if (*(cur + 1) == '=') {
				var = 1;
				switch(*cur) {
				case 'c':
					ts->channels = atoi(cur + 2);
					break;
				case 'r':
					ts->rate = atoi(cur + 2);
					break;
				case 'd':
					ts->duration = atoi(cur + 2) * (ts->rate / 1000);
					break;
				case 'v':
					ts->volume = atoi(cur + 2);
					break;
				case '>':
					ts->decay_factor = atoi(cur + 2);
					ts->decay_direction = -1;
					break;
				case '<':
					ts->decay_factor = atoi(cur + 2);
					ts->decay_direction = 1;
					break;
				case '+':
					ts->decay_step = atoi(cur + 2);
					break;
				case 'w':
					ts->wait = atoi(cur + 2) * (ts->rate / 1000);
					break;
				case 'l':
					ts->loops = atoi(cur + 2); 
					break;
				case 'L':
					if (!LOOPING) {
						ts->LOOPS = atoi(cur + 2); 
					}
					LOOPING++;
					break;
				}
			} else {
				while (*cur) {
					char *p = NULL, *e = NULL;
					teletone_tone_map_t mymap, *mapp = NULL;

					if (*cur == ' ' || *cur == '\r' || *cur == '\n') {
						cur++;
						continue;
					}
					
					ts->tmp_duration = -1;
					ts->tmp_wait = -1;

					memset(&mymap, 0, sizeof(mymap));

					if (*(cur + 1) == '(') {
						p = cur + 2;
						if (*cur) {
							char *next;
							int i = 0;
							if ((e = strchr(p, ')')) != 0) {
								*e++ = '\0';
							}
							do {
								if ((next = strchr(p, ',')) != 0) {
									*next++ = '\0';
								}
								if (i == 0) {
									ts->tmp_duration = atoi(p) * (ts->rate / 1000);
									i++;
								} else if (i == 1) {
									ts->tmp_wait = atoi(p) * (ts->rate / 1000);
									i++;
								} else {
									mymap.freqs[i++ - 2] = atof(p);
								}
								p = next;

							} while (next && (i-2) < TELETONE_MAX_TONES);
							if (i > 2 && *cur == '%') {
								mapp = &mymap;
							} else if ((i != 2 || *cur == '%')) { 
								if (ts->debug && ts->debug_stream) {
									fprintf(ts->debug_stream, "Syntax Error!\n");
								}
								goto bottom;
							}
						} 
					}

					if (*cur && !mapp) {
						if (*cur > 0 && *cur < TELETONE_TONE_RANGE) { 
							mapp = &ts->TONES[(int)*cur];
						} else if (ts->debug && ts->debug_stream) {
							fprintf(ts->debug_stream, "Map [%c] Out Of Range!\n", *cur);
						}
					}

					if (mapp) {
						if (mapp->freqs[0]) {
							if (ts->handler) {
								do {
									ts->handler(ts, mapp);
									if (ts->loops > 0) {
										ts->loops--;
									}
								} while (ts->loops);
							}
						} else if (ts->debug && ts->debug_stream) {
							fprintf(ts->debug_stream, "Ignoring Empty Map [%c]!\n", *cur);
						}
					}
					
					if (e) {
						cur = e;
					} else {
						cur++;
					}
				}
			}

			if (end) {
				cur = end;
			} else if (*cur){
				cur++;
			}
		}
	bottom:
		free(data);
		data = NULL;
		if (ts->LOOPS > 0) {
			ts->LOOPS--;
		}

	} while (ts->LOOPS);

	return 0;
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
