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
 * The Original Code is tone_detect.c - General telephony tone detection, and specific detection of DTMF.
 *
 *
 * The Initial Developer of the Original Code is
 * Stephen Underwood <steveu@coppice.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * The the original interface designed by Steve Underwood was preserved to retain 
 *the optimizations when considering DTMF tones though the names were changed in the interest 
 * of namespace.
 *
 * Much less efficient expansion interface was added to allow for the detection of 
 * a single arbitrary tone combination which may also exceed 2 simultaneous tones.
 * (controlled by compile time constant TELETONE_MAX_TONES)
 *
 * Copyright (C) 2006 Anthony Minessale II <anthmct@yahoo.com>
 *
 *
 * libteletone_detect.c Tone Detection Code
 *
 *
 ********************************************************************************* 
 *
 * Derived from tone_detect.c - General telephony tone detection, and specific
 * detection of DTMF.
 *
 * Copyright (C) 2001  Steve Underwood <steveu@coppice.org>
 *
 * Despite my general liking of the GPL, I place this code in the
 * public domain for the benefit of all mankind - even the slimy
 * ones who might try to proprietize my work and use it to my
 * detriment.
 */
#include <math.h>
#ifndef _MSC_VER
#include <stdint.h>
#endif
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <libteletone_detect.h>

static teletone_detection_descriptor_t dtmf_detect_row[GRID_FACTOR];
static teletone_detection_descriptor_t dtmf_detect_col[GRID_FACTOR];
static teletone_detection_descriptor_t dtmf_detect_row_2nd[GRID_FACTOR];
static teletone_detection_descriptor_t dtmf_detect_col_2nd[GRID_FACTOR];

static teletone_process_t dtmf_row[] = {697.0,  770.0,  852.0,  941.0};
static teletone_process_t dtmf_col[] = {1209.0, 1336.0, 1477.0, 1633.0};


static char dtmf_positions[] = "123A" "456B" "789C" "*0#D";

static void goertzel_init(teletone_goertzel_state_t *goertzel_state, teletone_detection_descriptor_t *tdesc) {
    goertzel_state->v2 = goertzel_state->v3 = 0.0;
    goertzel_state->fac = tdesc->fac;
}

void teletone_goertzel_update(teletone_goertzel_state_t *goertzel_state,
							  int16_t sample_buffer[],
							  int samples)
{
    int i;
    teletone_process_t v1;
    
    for (i = 0;  i < samples;  i++) {
        v1 = goertzel_state->v2;
        goertzel_state->v2 = goertzel_state->v3;
        goertzel_state->v3 = goertzel_state->fac*goertzel_state->v2 - v1 + sample_buffer[i];
    }
}

teletone_process_t teletone_goertzel_result (teletone_goertzel_state_t *goertzel_state)
{
    return goertzel_state->v3 * goertzel_state->v3 + goertzel_state->v2 * goertzel_state->v2 - goertzel_state->v2 * goertzel_state->v3 * goertzel_state->fac;
}

void teletone_dtmf_detect_init (teletone_dtmf_detect_state_t *dtmf_detect_state, int sample_rate)
{
    int i;
    teletone_process_t theta;

    dtmf_detect_state->hit1 = dtmf_detect_state->hit2 = 0;

    for (i = 0;  i < GRID_FACTOR;  i++) {
        theta = M_TWO_PI*(dtmf_row[i]/(teletone_process_t)sample_rate);
        dtmf_detect_row[i].fac = 2.0*cos(theta);

        theta = M_TWO_PI*(dtmf_col[i]/(teletone_process_t)sample_rate);
        dtmf_detect_col[i].fac = 2.0*cos(theta);
    
        theta = M_TWO_PI*(dtmf_row[i]*2.0/(teletone_process_t)sample_rate);
        dtmf_detect_row_2nd[i].fac = 2.0*cos(theta);

        theta = M_TWO_PI*(dtmf_col[i]*2.0/(teletone_process_t)sample_rate);
        dtmf_detect_col_2nd[i].fac = 2.0*cos(theta);
    
		goertzel_init (&dtmf_detect_state->row_out[i], &dtmf_detect_row[i]);
    	goertzel_init (&dtmf_detect_state->col_out[i], &dtmf_detect_col[i]);
    	goertzel_init (&dtmf_detect_state->row_out2nd[i], &dtmf_detect_row_2nd[i]);
    	goertzel_init (&dtmf_detect_state->col_out2nd[i], &dtmf_detect_col_2nd[i]);
	
		dtmf_detect_state->energy = 0.0;
    }
    dtmf_detect_state->current_sample = 0;
    dtmf_detect_state->detected_digits = 0;
    dtmf_detect_state->lost_digits = 0;
    dtmf_detect_state->digits[0] = '\0';
    dtmf_detect_state->mhit = 0;
}

void teletone_multi_tone_init(teletone_multi_tone_t *mt, teletone_tone_map_t *map)
{
	teletone_process_t theta = 0;
	int x = 0;

	if(!mt->min_samples) {
		mt->min_samples = 102;
	}

	if (!mt->positive_factor) {
		mt->positive_factor = 2;
	}

	if(!mt->negative_factor) {
		mt->negative_factor = 10;
	}

	if (!mt->hit_factor) {
		mt->hit_factor = 2;
	}

	if (!mt->sample_rate) {
		mt->sample_rate = 8000;
	}

	for(x = 0; x < TELETONE_MAX_TONES; x++) {
		if ((int) map->freqs[x] == 0) {
			break;
		}
		mt->tone_count++;
		theta = M_TWO_PI*(map->freqs[x]/(teletone_process_t)mt->sample_rate);
		mt->tdd[x].fac = 2.0 * cos(theta);
		goertzel_init (&mt->gs[x], &mt->tdd[x]);
		goertzel_init (&mt->gs2[x], &mt->tdd[x]);
	}

}

int teletone_multi_tone_detect (teletone_multi_tone_t *mt,
					   int16_t sample_buffer[],
					   int samples)
{
	int sample, limit, j, x = 0;
	teletone_process_t v1, famp;
	teletone_process_t eng_sum = 0, eng_all[TELETONE_MAX_TONES];
	int gtest = 0, see_hit = 0;

    for (sample = 0;  sample < samples;  sample = limit) {
		mt->total_samples++;

        if ((samples - sample) >= (mt->min_samples - mt->current_sample)) {
            limit = sample + (mt->min_samples - mt->current_sample);
		} else {
            limit = samples;
		}

        for (j = sample;  j < limit;  j++) {
            famp = sample_buffer[j];
			
			mt->energy += famp*famp;

			for(x = 0; x < mt->tone_count; x++) {
				v1 = mt->gs[x].v2;
				mt->gs[x].v2 = mt->gs[x].v3;
				mt->gs[x].v3 = mt->gs[x].fac * mt->gs[x].v2 - v1 + famp;
    
				v1 = mt->gs2[x].v2;
				mt->gs2[x].v2 = mt->gs2[x].v3;
				mt->gs2[x].v3 = mt->gs2[x].fac*mt->gs2[x].v2 - v1 + famp;
			}
		}

        mt->current_sample += (limit - sample);
        if (mt->current_sample < mt->min_samples) {
            continue;
		}

		eng_sum = 0;
		for(x = 0; x < mt->tone_count; x++) {
			eng_all[x] = teletone_goertzel_result (&mt->gs[x]);
			eng_sum += eng_all[x];
		}

		gtest = 0;
		for(x = 0; x < mt->tone_count; x++) {
			gtest += teletone_goertzel_result (&mt->gs2[x]) < eng_all[x] ? 1 : 0;
		}

		if (gtest >= 2 && eng_sum > 42.0 * mt->energy) {
			if(mt->negatives) {
				mt->negatives--;
			}
			mt->positives++;

			if(mt->positives >= mt->positive_factor) {
				mt->hits++;
			}
			if (mt->hits >= mt->hit_factor) {
				see_hit++;
				mt->positives = mt->negatives = mt->hits = 0;
			}
		} else {
			mt->negatives++;
			if(mt->positives) {
				mt->positives--;
			}
			if(mt->negatives > mt->negative_factor) {
				mt->positives = mt->hits = 0;
			}
		}

        /* Reinitialise the detector for the next block */
		for(x = 0; x < mt->tone_count; x++) {
			goertzel_init (&mt->gs[x], &mt->tdd[x]);
			goertzel_init (&mt->gs2[x], &mt->tdd[x]);
		}

		mt->energy = 0.0;
        mt->current_sample = 0;
    }

	return see_hit;
}


int teletone_dtmf_detect (teletone_dtmf_detect_state_t *dtmf_detect_state,
                 int16_t sample_buffer[],
                 int samples)
{
    teletone_process_t row_energy[GRID_FACTOR];
    teletone_process_t col_energy[GRID_FACTOR];
    teletone_process_t famp;
    teletone_process_t v1;
    int i;
    int j;
    int sample;
    int best_row;
    int best_col;
    int hit;
    int limit;

    hit = 0;
    for (sample = 0;  sample < samples;  sample = limit) {
        /* BLOCK_LEN is optimised to meet the DTMF specs. */
        if ((samples - sample) >= (BLOCK_LEN - dtmf_detect_state->current_sample)) {
            limit = sample + (BLOCK_LEN - dtmf_detect_state->current_sample);
		} else {
            limit = samples;
		}

        for (j = sample;  j < limit;  j++) {
			int x = 0;
            famp = sample_buffer[j];
			
			dtmf_detect_state->energy += famp*famp;

			for(x = 0; x < GRID_FACTOR; x++) {
				v1 = dtmf_detect_state->row_out[x].v2;
				dtmf_detect_state->row_out[x].v2 = dtmf_detect_state->row_out[x].v3;
				dtmf_detect_state->row_out[x].v3 = dtmf_detect_state->row_out[x].fac*dtmf_detect_state->row_out[x].v2 - v1 + famp;
    
				v1 = dtmf_detect_state->col_out[x].v2;
				dtmf_detect_state->col_out[x].v2 = dtmf_detect_state->col_out[x].v3;
				dtmf_detect_state->col_out[x].v3 = dtmf_detect_state->col_out[x].fac*dtmf_detect_state->col_out[x].v2 - v1 + famp;

				v1 = dtmf_detect_state->col_out2nd[x].v2;
				dtmf_detect_state->col_out2nd[x].v2 = dtmf_detect_state->col_out2nd[x].v3;
				dtmf_detect_state->col_out2nd[x].v3 = dtmf_detect_state->col_out2nd[x].fac*dtmf_detect_state->col_out2nd[x].v2 - v1 + famp;
        
				v1 = dtmf_detect_state->row_out2nd[x].v2;
				dtmf_detect_state->row_out2nd[x].v2 = dtmf_detect_state->row_out2nd[x].v3;
				dtmf_detect_state->row_out2nd[x].v3 = dtmf_detect_state->row_out2nd[x].fac*dtmf_detect_state->row_out2nd[x].v2 - v1 + famp;
			}

        }

        dtmf_detect_state->current_sample += (limit - sample);
        if (dtmf_detect_state->current_sample < BLOCK_LEN) {
            continue;
		}
        /* We are at the end of a DTMF detection block */
        /* Find the peak row and the peak column */
        row_energy[0] = teletone_goertzel_result (&dtmf_detect_state->row_out[0]);
        col_energy[0] = teletone_goertzel_result (&dtmf_detect_state->col_out[0]);

		for (best_row = best_col = 0, i = 1;  i < GRID_FACTOR;  i++) {
    	    row_energy[i] = teletone_goertzel_result (&dtmf_detect_state->row_out[i]);
            if (row_energy[i] > row_energy[best_row]) {
                best_row = i;
			}
    	    col_energy[i] = teletone_goertzel_result (&dtmf_detect_state->col_out[i]);
            if (col_energy[i] > col_energy[best_col]) {
                best_col = i;
			}
    	}
        hit = 0;
        /* Basic signal level test and the twist test */
        if (row_energy[best_row] >= DTMF_THRESHOLD &&
			col_energy[best_col] >= DTMF_THRESHOLD &&
            col_energy[best_col] < row_energy[best_row]*DTMF_REVERSE_TWIST &&
            col_energy[best_col]*DTMF_NORMAL_TWIST > row_energy[best_row]) {
            /* Relative peak test */
            for (i = 0;  i < GRID_FACTOR;  i++) {
                if ((i != best_col  &&  col_energy[i]*DTMF_RELATIVE_PEAK_COL > col_energy[best_col]) ||
                    (i != best_row  &&  row_energy[i]*DTMF_RELATIVE_PEAK_ROW > row_energy[best_row])) {
                    break;
                }
            }
            /* ... and second harmonic test */
            if (i >= GRID_FACTOR && (row_energy[best_row] + col_energy[best_col]) > 42.0*dtmf_detect_state->energy &&
                teletone_goertzel_result (&dtmf_detect_state->col_out2nd[best_col])*DTMF_2ND_HARMONIC_COL < col_energy[best_col] &&
                teletone_goertzel_result (&dtmf_detect_state->row_out2nd[best_row])*DTMF_2ND_HARMONIC_ROW < row_energy[best_row]) {
                hit = dtmf_positions[(best_row << 2) + best_col];
                /* Look for two successive similar results */
                /* The logic in the next test is:
                   We need two successive identical clean detects, with
				   something different preceeding it. This can work with
				   back to back differing digits. More importantly, it
				   can work with nasty phones that give a very wobbly start
				   to a digit. */
                if (hit == dtmf_detect_state->hit3  &&  dtmf_detect_state->hit3 != dtmf_detect_state->hit2) {
					dtmf_detect_state->mhit = hit;
					dtmf_detect_state->digit_hits[(best_row << 2) + best_col]++;
					dtmf_detect_state->detected_digits++;
					if (dtmf_detect_state->current_digits < TELETONE_MAX_DTMF_DIGITS) {
						dtmf_detect_state->digits[dtmf_detect_state->current_digits++] = hit;
						dtmf_detect_state->digits[dtmf_detect_state->current_digits] = '\0';
					}
					else
						{
							dtmf_detect_state->lost_digits++;
						}
				}
            }
        }
        dtmf_detect_state->hit1 = dtmf_detect_state->hit2;
        dtmf_detect_state->hit2 = dtmf_detect_state->hit3;
        dtmf_detect_state->hit3 = hit;
        /* Reinitialise the detector for the next block */
        for (i = 0;  i < GRID_FACTOR;  i++) {
       	    goertzel_init (&dtmf_detect_state->row_out[i], &dtmf_detect_row[i]);
            goertzel_init (&dtmf_detect_state->col_out[i], &dtmf_detect_col[i]);
    	    goertzel_init (&dtmf_detect_state->row_out2nd[i], &dtmf_detect_row_2nd[i]);
    	    goertzel_init (&dtmf_detect_state->col_out2nd[i], &dtmf_detect_col_2nd[i]);
        }
		dtmf_detect_state->energy = 0.0;
        dtmf_detect_state->current_sample = 0;
    }
    if ((!dtmf_detect_state->mhit) || (dtmf_detect_state->mhit != hit)) {
		dtmf_detect_state->mhit = 0;
		return(0);
    }
    return (hit);
}


int teletone_dtmf_get (teletone_dtmf_detect_state_t *dtmf_detect_state,
              char *buf,
              int max)
{
    if (max > dtmf_detect_state->current_digits) {
        max = dtmf_detect_state->current_digits;
	}
    if (max > 0) {
        memcpy (buf, dtmf_detect_state->digits, max);
        memmove (dtmf_detect_state->digits, dtmf_detect_state->digits + max, dtmf_detect_state->current_digits - max);
        dtmf_detect_state->current_digits -= max;
    }
    buf[max] = '\0';
    return  max;
}

