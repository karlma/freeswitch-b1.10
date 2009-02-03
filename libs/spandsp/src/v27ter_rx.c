/*
 * SpanDSP - a series of DSP components for telephony
 *
 * v27ter_rx.c - ITU V.27ter modem receive part
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2003 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: v27ter_rx.c,v 1.116 2009/02/03 16:28:40 steveu Exp $
 */

/*! \file */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include "floating_fudge.h"

#include "spandsp/telephony.h"
#include "spandsp/logging.h"
#include "spandsp/complex.h"
#include "spandsp/vector_float.h"
#include "spandsp/complex_vector_float.h"
#include "spandsp/vector_int.h"
#include "spandsp/complex_vector_int.h"
#include "spandsp/async.h"
#include "spandsp/power_meter.h"
#include "spandsp/arctan2.h"
#include "spandsp/dds.h"
#include "spandsp/complex_filters.h"

#include "spandsp/v29rx.h"
#include "spandsp/v27ter_rx.h"

#include "spandsp/private/logging.h"
#include "spandsp/private/v27ter_rx.h"

#if defined(SPANDSP_USE_FIXED_POINT)
#include "v27ter_rx_4800_fixed_rrc.h"
#include "v27ter_rx_2400_fixed_rrc.h"
#else
#include "v27ter_rx_4800_floating_rrc.h"
#include "v27ter_rx_2400_floating_rrc.h"
#endif

/* V.27ter is a DPSK modem, but this code treats it like QAM. It nails down the
   signal to a static constellation, even though dealing with differences is all
   that is necessary. */

#define CARRIER_NOMINAL_FREQ            1800.0f
#define EQUALIZER_DELTA                 0.25f

#if defined(SPANDSP_USE_FIXED_POINT)
#define FP_FACTOR                       4096
#define FP_SHIFT_FACTOR                 12
#endif

/* Segments of the training sequence */
/* V.27ter defines a long and a short sequence. FAX doesn't use the
   short sequence, so it is not implemented here. */
#define V27TER_TRAINING_SEG_3_LEN       50
#define V27TER_TRAINING_SEG_5_LEN       1074
#define V27TER_TRAINING_SEG_6_LEN       8

#define V27TER_EQUALIZER_LEN    (V27TER_EQUALIZER_PRE_LEN + 1 + V27TER_EQUALIZER_POST_LEN)

enum
{
    TRAINING_STAGE_NORMAL_OPERATION = 0,
    TRAINING_STAGE_SYMBOL_ACQUISITION,
    TRAINING_STAGE_LOG_PHASE,
    TRAINING_STAGE_WAIT_FOR_HOP,
    TRAINING_STAGE_TRAIN_ON_ABAB,
    TRAINING_STAGE_TEST_ONES,
    TRAINING_STAGE_PARKED
};

#if defined(SPANDSP_USE_FIXED_POINTx)
static const complexi16_t v27ter_constellation[8] =
{
    {((int)(FP_FACTOR* 1.414f), ((int)(FP_FACTOR* 0.0f)},       /*   0deg */
    {((int)(FP_FACTOR* 1.0f),   ((int)(FP_FACTOR* 1.0f)},       /*  45deg */
    {((int)(FP_FACTOR* 0.0f),   ((int)(FP_FACTOR* 1.414f)},     /*  90deg */
    {((int)(FP_FACTOR*-1.0f),   ((int)(FP_FACTOR* 1.0f)},       /* 135deg */
    {((int)(FP_FACTOR*-1.414f), ((int)(FP_FACTOR* 0.0f)},       /* 180deg */
    {((int)(FP_FACTOR*-1.0f),   ((int)(FP_FACTOR*-1.0f)},       /* 225deg */
    {((int)(FP_FACTOR* 0.0f),   ((int)(FP_FACTOR*-1.414f)},     /* 270deg */
    {((int)(FP_FACTOR* 1.0f),   ((int)(FP_FACTOR*-1.0f)}        /* 315deg */
};
#else
static const complexf_t v27ter_constellation[8] =
{
    { 1.414f,  0.0f},                                           /*   0deg */
    { 1.0f,    1.0f},                                           /*  45deg */
    { 0.0f,    1.414f},                                         /*  90deg */
    {-1.0f,    1.0f},                                           /* 135deg */
    {-1.414f,  0.0f},                                           /* 180deg */
    {-1.0f,   -1.0f},                                           /* 225deg */
    { 0.0f,   -1.414f},                                         /* 270deg */
    { 1.0f,   -1.0f}                                            /* 315deg */
};
#endif

SPAN_DECLARE(float) v27ter_rx_carrier_frequency(v27ter_rx_state_t *s)
{
    return dds_frequencyf(s->carrier_phase_rate);
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(float) v27ter_rx_symbol_timing_correction(v27ter_rx_state_t *s)
{
    int steps_per_symbol;
    
    steps_per_symbol = (s->bit_rate == 4800)  ?  RX_PULSESHAPER_4800_COEFF_SETS*5  :  RX_PULSESHAPER_2400_COEFF_SETS*20/3;
    return (float) s->total_baud_timing_correction/(float) steps_per_symbol;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(float) v27ter_rx_signal_power(v27ter_rx_state_t *s)
{
    return power_meter_current_dbm0(&s->power);
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(void) v27ter_rx_signal_cutoff(v27ter_rx_state_t *s, float cutoff)
{
    /* The 0.4 factor allows for the gain of the DC blocker */
    s->carrier_on_power = (int32_t) (power_meter_level_dbm0(cutoff + 2.5f)*0.4f);
    s->carrier_off_power = (int32_t) (power_meter_level_dbm0(cutoff - 2.5f)*0.4f);
}
/*- End of function --------------------------------------------------------*/

static void report_status_change(v27ter_rx_state_t *s, int status)
{
    if (s->status_handler)
        s->status_handler(s->status_user_data, status);
    else if (s->put_bit)
        s->put_bit(s->put_bit_user_data, status);
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINTx)
SPAN_DECLARE(int) v27ter_rx_equalizer_state(v27ter_rx_state_t *s, complexi16_t **coeffs)
#else
SPAN_DECLARE(int) v27ter_rx_equalizer_state(v27ter_rx_state_t *s, complexf_t **coeffs)
#endif
{
    *coeffs = s->eq_coeff;
    return V27TER_EQUALIZER_LEN;
}
/*- End of function --------------------------------------------------------*/

static void equalizer_save(v27ter_rx_state_t *s)
{
#if defined(SPANDSP_USE_FIXED_POINTx)
    cvec_copyi16(s->eq_coeff_save, s->eq_coeff, V27TER_EQUALIZER_LEN);
#else
    cvec_copyf(s->eq_coeff_save, s->eq_coeff, V27TER_EQUALIZER_LEN);
#endif
}
/*- End of function --------------------------------------------------------*/

static void equalizer_restore(v27ter_rx_state_t *s)
{
#if defined(SPANDSP_USE_FIXED_POINTx)
    cvec_copyi16(s->eq_coeff, s->eq_coeff_save, V27TER_EQUALIZER_LEN);
    cvec_zeroi16(s->eq_buf, V27TER_EQUALIZER_LEN);
    s->eq_delta = 32768.0f*EQUALIZER_DELTA/V27TER_EQUALIZER_LEN);
#else
    cvec_copyf(s->eq_coeff, s->eq_coeff_save, V27TER_EQUALIZER_LEN);
    cvec_zerof(s->eq_buf, V27TER_EQUALIZER_LEN);
    s->eq_delta = EQUALIZER_DELTA/V27TER_EQUALIZER_LEN;
#endif

    s->eq_put_step = (s->bit_rate == 4800)  ?  RX_PULSESHAPER_4800_COEFF_SETS*5/2  :  RX_PULSESHAPER_2400_COEFF_SETS*20/(3*2);
    s->eq_step = 0;
}
/*- End of function --------------------------------------------------------*/

static void equalizer_reset(v27ter_rx_state_t *s)
{
    /* Start with an equalizer based on everything being perfect. */
#if defined(SPANDSP_USE_FIXED_POINTx)
    cvec_zeroi16(s->eq_coeff, V27TER_EQUALIZER_LEN);
    s->eq_coeff[V27TER_EQUALIZER_PRE_LEN] = complex_seti16(1.414f*FP_FACTOR, 0);
    cvec_zeroi16(s->eq_buf, V27TER_EQUALIZER_LEN);
    s->eq_delta = 32768.0f*EQUALIZER_DELTA/V27TER_EQUALIZER_LEN);
#else
    cvec_zerof(s->eq_coeff, V27TER_EQUALIZER_LEN);
    s->eq_coeff[V27TER_EQUALIZER_PRE_LEN] = complex_setf(1.414f, 0.0f);
    cvec_zerof(s->eq_buf, V27TER_EQUALIZER_LEN);
    s->eq_delta = EQUALIZER_DELTA/V27TER_EQUALIZER_LEN;
#endif

    s->eq_put_step = (s->bit_rate == 4800)  ?  RX_PULSESHAPER_4800_COEFF_SETS*5/2  :  RX_PULSESHAPER_2400_COEFF_SETS*20/(3*2);
    s->eq_step = 0;
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINTx)
static __inline__ complexi16_t complex_mul_q4_12(const complexi16_t *x, const complexi16_t *y)
{
    complexi16_t z;

    z.re = ((int32_t) x->re*(int32_t) y->re - (int32_t) x->im*(int32_t) y->im) >> 12;
    z.im = ((int32_t) x->re*(int32_t) y->im + (int32_t) x->im*(int32_t) y->re) >> 12;
    return z;
}
/*- End of function --------------------------------------------------------*/
#endif

#if defined(SPANDSP_USE_FIXED_POINTx)
static __inline__ complexi16_t equalizer_get(v27ter_rx_state_t *s)
#else
static __inline__ complexf_t equalizer_get(v27ter_rx_state_t *s)
#endif
{
#if defined(SPANDSP_USE_FIXED_POINTx)
    complexi32_t zz;
    complexi16_t z;

    /* Get the next equalized value. */
    zz = cvec_circular_dot_prodi16(s->eq_buf, s->eq_coeff, V27TER_EQUALIZER_LEN, s->eq_step);
    z.re = zz.re >> FP_SHIFT_FACTOR;
    z.im = zz.im >> FP_SHIFT_FACTOR;
    return z;
#else
    /* Get the next equalized value. */
    return cvec_circular_dot_prodf(s->eq_buf, s->eq_coeff, V27TER_EQUALIZER_LEN, s->eq_step);
#endif
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINTx)
static void tune_equalizer(v27ter_rx_state_t *s, const complexi16_t *z, const complexi16_t *target)
#else
static void tune_equalizer(v27ter_rx_state_t *s, const complexf_t *z, const complexf_t *target)
#endif
{
#if defined(SPANDSP_USE_FIXED_POINTx)
    complexi16_t err;

    /* Find the x and y mismatch from the exact constellation position. */
    err.re = target->re*FP_FACTOR - z->re;
    err.im = target->im*FP_FACTOR - z->im;
    err.re = ((int32_t) err.re*(int32_t) s->eq_delta) >> 15;
    err.im = ((int32_t) err.im*(int32_t) s->eq_delta) >> 15;
    cvec_circular_lmsi16(s->eq_buf, s->eq_coeff, V27TER_EQUALIZER_LEN, s->eq_step, &err);
#else
    complexf_t err;

    /* Find the x and y mismatch from the exact constellation position. */
    err = complex_subf(target, z);
    err.re *= s->eq_delta;
    err.im *= s->eq_delta;
    cvec_circular_lmsf(s->eq_buf, s->eq_coeff, V27TER_EQUALIZER_LEN, s->eq_step, &err);
#endif
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINTx)
static __inline__ int find_quadrant(const complexi16_t *z)
#else
static __inline__ int find_quadrant(const complexf_t *z)
#endif
{
    int b1;
    int b2;

    /* Split the space along the two diagonals. */
    b1 = (z->im > z->re);
    b2 = (z->im < -z->re);
    return (b2 << 1) | (b1 ^ b2);
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINTx)
static __inline__ int find_octant(complexi16_t *z)
#else
static __inline__ int find_octant(complexf_t *z)
#endif
{
    float abs_re;
    float abs_im;
    int b1;
    int b2;
    int bits;

    /* Are we near an axis or a diagonal? */
    abs_re = fabsf(z->re);
    abs_im = fabsf(z->im);
    if (abs_im > abs_re*0.4142136f  &&  abs_im < abs_re*2.4142136f)
    {
        /* Split the space along the two axes. */
        b1 = (z->re < 0.0f);
        b2 = (z->im < 0.0f);
        bits = (b2 << 2) | ((b1 ^ b2) << 1) | 1;
    }
    else
    {
        /* Split the space along the two diagonals. */
        b1 = (z->im > z->re);
        b2 = (z->im < -z->re);
        bits = (b2 << 2) | ((b1 ^ b2) << 1);
    }
    return bits;
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINTx)
static __inline__ void track_carrier(v27ter_rx_state_t *s, const complexi16_t *z, const complexi16_t *target)
#else
static __inline__ void track_carrier(v27ter_rx_state_t *s, const complexf_t *z, const complexf_t *target)
#endif
{
#if defined(SPANDSP_USE_FIXED_POINTx)
    int32_t error;
#else
    float error;
#endif

    /* For small errors the imaginary part of the difference between the actual and the target
       positions is proportional to the phase error, for any particular target. However, the
       different amplitudes of the various target positions scale things. */
    error = z->im*target->re - z->re*target->im;

#if defined(SPANDSP_USE_FIXED_POINTx)
    error /= (float) FP_FACTOR;
    s->carrier_phase_rate += (int32_t) (s->carrier_track_i*error);
    s->carrier_phase += (int32_t) (s->carrier_track_p*error);
#else
    s->carrier_phase_rate += (int32_t) (s->carrier_track_i*error);
    s->carrier_phase += (int32_t) (s->carrier_track_p*error);
    //span_log(&s->logging, SPAN_LOG_FLOW, "Im = %15.5f   f = %15.5f\n", error, dds_frequencyf(s->carrier_phase_rate));
#endif
}
/*- End of function --------------------------------------------------------*/

static __inline__ int descramble(v27ter_rx_state_t *s, int in_bit)
{
    int out_bit;

    out_bit = (in_bit ^ (s->scramble_reg >> 5) ^ (s->scramble_reg >> 6)) & 1;
    if (s->scrambler_pattern_count >= 33)
    {
        out_bit ^= 1;
        s->scrambler_pattern_count = 0;
    }
    else
    {
        if (s->training_stage > TRAINING_STAGE_NORMAL_OPERATION  &&  s->training_stage < TRAINING_STAGE_TEST_ONES)
        {
            s->scrambler_pattern_count = 0;
        }
        else
        {
            if ((((s->scramble_reg >> 7) ^ in_bit) & ((s->scramble_reg >> 8) ^ in_bit) & ((s->scramble_reg >> 11) ^ in_bit) & 1))
                s->scrambler_pattern_count = 0;
            else
                s->scrambler_pattern_count++;
        }
    }
    s->scramble_reg <<= 1;
    if (s->training_stage > TRAINING_STAGE_NORMAL_OPERATION  &&  s->training_stage < TRAINING_STAGE_TEST_ONES)
        s->scramble_reg |= out_bit;
    else
        s->scramble_reg |= in_bit;
    return out_bit;
}
/*- End of function --------------------------------------------------------*/

static __inline__ void put_bit(v27ter_rx_state_t *s, int bit)
{
    int out_bit;

    bit &= 1;

    out_bit = descramble(s, bit);

    /* We need to strip the last part of the training before we let data
       go to the application. */
    if (s->training_stage == TRAINING_STAGE_NORMAL_OPERATION)
    {
        s->put_bit(s->put_bit_user_data, out_bit);
    }
    else
    {
        //span_log(&s->logging, SPAN_LOG_FLOW, "Test bit %d\n", out_bit);
        /* The bits during the final stage of training should be all ones. However,
           buggy modems mean you cannot rely on this. Therefore we don't bother
           testing for ones, but just rely on a constellation mismatch measurement. */
    }
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINTx)
static void decode_baud(v27ter_rx_state_t *s, complexi16_t *z)
#else
static void decode_baud(v27ter_rx_state_t *s, complexf_t *z)
#endif
{
    static const uint8_t phase_steps_4800[8] =
    {
        4, 0, 2, 6, 7, 3, 1, 5
    };
    static const uint8_t phase_steps_2400[4] =
    {
        0, 2, 3, 1
    };
    int nearest;
    int raw_bits;

    if (s->bit_rate == 2400)
    {
        nearest = find_quadrant(z);
        raw_bits = phase_steps_2400[(nearest - s->constellation_state) & 3];
        put_bit(s, raw_bits);
        put_bit(s, raw_bits >> 1);
        s->constellation_state = nearest;
        nearest <<= 1;
    }
    else
    {
        nearest = find_octant(z);
        raw_bits = phase_steps_4800[(nearest - s->constellation_state) & 7];
        put_bit(s, raw_bits);
        put_bit(s, raw_bits >> 1);
        put_bit(s, raw_bits >> 2);
        s->constellation_state = nearest;
    }
    track_carrier(s, z, &v27ter_constellation[nearest]);
    if (--s->eq_skip <= 0)
    {
        /* Once we are in the data the equalization should not need updating.
           However, the line characteristics may slowly drift. We, therefore,
           tune up on the occassional sample, keeping the compute down. */
        s->eq_skip = 100;
        tune_equalizer(s, z, &v27ter_constellation[nearest]);
    }
}
/*- End of function --------------------------------------------------------*/

static __inline__ void symbol_sync(v27ter_rx_state_t *s)
{
    float p;
    float q;

    /* This routine adapts the position of the half baud samples entering the equalizer. */

    /* Perform a Gardner test for baud alignment */
    p = s->eq_buf[(s->eq_step - 3) & V27TER_EQUALIZER_LEN].re
      - s->eq_buf[(s->eq_step - 1) & V27TER_EQUALIZER_LEN].re;
    p *= s->eq_buf[(s->eq_step - 2) & V27TER_EQUALIZER_LEN].re;

    q = s->eq_buf[(s->eq_step - 3) & V27TER_EQUALIZER_LEN].im
      - s->eq_buf[(s->eq_step - 1) & V27TER_EQUALIZER_LEN].im;
    q *= s->eq_buf[(s->eq_step - 2) & V27TER_EQUALIZER_LEN].im;

    s->gardner_integrate += (p + q > 0.0f)  ?  s->gardner_step  :  -s->gardner_step;

    if (abs(s->gardner_integrate) >= 256)
    {
        /* This integrate and dump approach avoids rapid changes of the equalizer put step.
           Rapid changes, without hysteresis, are bad. They degrade the equalizer performance
           when the true symbol boundary is close to a sample boundary. */
        //span_log(&s->logging, SPAN_LOG_FLOW, "Hop %d\n", s->gardner_integrate);
        s->eq_put_step += (s->gardner_integrate/256);
        s->total_baud_timing_correction += (s->gardner_integrate/256);
        if (s->qam_report)
            s->qam_report(s->qam_user_data, NULL, NULL, s->gardner_integrate);
        s->gardner_integrate = 0;
    }
    //span_log(&s->logging, SPAN_LOG_FLOW, "Gardner=%10.5f 0x%X\n", p, s->eq_put_step);
}
/*- End of function --------------------------------------------------------*/

#if defined(SPANDSP_USE_FIXED_POINT)
static __inline__ void process_half_baud(v27ter_rx_state_t *s, const complexi16_t *sample)
#else
static __inline__ void process_half_baud(v27ter_rx_state_t *s, const complexf_t *sample)
#endif
{
    static const int abab_pos[2] =
    {
        0, 4
    };
    complexf_t z;
    complexf_t zz;
    float p;
    int i;
    int j;
    int32_t angle;
    int32_t ang;

    /* Add a sample to the equalizer's circular buffer, but don't calculate anything
       at this time. */
#if defined(SPANDSP_USE_FIXED_POINT)
    s->eq_buf[s->eq_step].re = sample->re/(float) FP_FACTOR;
    s->eq_buf[s->eq_step].im = sample->im/(float) FP_FACTOR;
#else
    s->eq_buf[s->eq_step] = *sample;
#endif
    if (++s->eq_step >= V27TER_EQUALIZER_LEN)
        s->eq_step = 0;
        
    /* On alternate insertions we have a whole baud, and must process it. */
    if ((s->baud_half ^= 1))
        return;

    symbol_sync(s);

    z = equalizer_get(s);

    //span_log(&s->logging, SPAN_LOG_FLOW, "Equalized symbol - %15.5f %15.5f\n", z.re, z.im);
    switch (s->training_stage)
    {
    case TRAINING_STAGE_NORMAL_OPERATION:
        decode_baud(s, &z);
        break;
    case TRAINING_STAGE_SYMBOL_ACQUISITION:
        /* Allow time for the Gardner algorithm to settle the baud timing */
        /* Don't start narrowing the bandwidth of the Gardner algorithm too early.
           Some modems are a bit wobbly when they start sending the signal. Also, we start
           this analysis before our filter buffers have completely filled. */
        if (++s->training_count >= 30)
        {
            s->gardner_step = 32;
            s->training_stage = TRAINING_STAGE_LOG_PHASE;
            s->angles[0] =
            s->start_angles[0] = arctan2(z.im, z.re);
        }
        break;
    case TRAINING_STAGE_LOG_PHASE:
        /* Record the current alternate phase angle */
        angle = arctan2(z.im, z.re);
        s->angles[1] =
        s->start_angles[1] = angle;
        s->training_count = 1;
        s->training_stage = TRAINING_STAGE_WAIT_FOR_HOP;
        break;
    case TRAINING_STAGE_WAIT_FOR_HOP:
        angle = arctan2(z.im, z.re);
        /* Look for the initial ABAB sequence to display a phase reversal, which will
           signal the start of the scrambled ABAB segment */
        ang = angle - s->angles[(s->training_count - 1) & 0xF];
        s->angles[(s->training_count + 1) & 0xF] = angle;
        if ((ang > 0x20000000  ||  ang < -0x20000000)  &&  s->training_count >= 3)
        {
            /* We seem to have a phase reversal */
            /* Slam the carrier frequency into line, based on the total phase drift over the last
               section. Use the shift from the odd bits and the shift from the even bits to get
               better jitter suppression. We need to scale here, or at the maximum specified
               frequency deviation we could overflow, and get a silly answer. */
            /* Step back a few symbols so we don't get ISI distorting things. */
            i = (s->training_count - 8) & ~1;
            /* Avoid the possibility of a divide by zero */
            if (i)
            {
                j = i & 0xF;
                ang = (s->angles[j] - s->start_angles[0])/i
                    + (s->angles[j | 0x1] - s->start_angles[1])/i;
                if (s->bit_rate == 4800)
                    s->carrier_phase_rate += ang/10;
                else
                    s->carrier_phase_rate += 3*(ang/40);
            }
            span_log(&s->logging, SPAN_LOG_FLOW, "Coarse carrier frequency %7.2f (%d)\n", dds_frequencyf(s->carrier_phase_rate), s->training_count);
            /* Check if the carrier frequency is plausible */
            if (s->carrier_phase_rate < dds_phase_ratef(CARRIER_NOMINAL_FREQ - 20.0f)
                ||
                s->carrier_phase_rate > dds_phase_ratef(CARRIER_NOMINAL_FREQ + 20.0f))
            {
               span_log(&s->logging, SPAN_LOG_FLOW, "Training failed (sequence failed)\n");
               /* Park this modem */
               s->training_stage = TRAINING_STAGE_PARKED;
               report_status_change(s, SIG_STATUS_TRAINING_FAILED);
               break;
            }

            /* Make a step shift in the phase, to pull it into line. We need to rotate the equalizer
               buffer, as well as the carrier phase, for this to play out nicely. */
            angle += 0x80000000;
            p = angle*2.0f*3.14159f/(65536.0f*65536.0f);
#if defined(SPANDSP_USE_FIXED_POINTx)
            zz = complex_setf(cosf(p), -sinf(p));
            for (i = 0;  i < V27TER_EQUALIZER_LEN;  i++)
            {
                z1 = complex_setf(s->eq_buf[i].re, s->eq_buf[i].im);
                z1 = complex_mulf(&z1, &zz);
                s->eq_buf[i].re = z1.re;
                s->eq_buf[i].im = z1.im;
            }
#else
            zz = complex_setf(cosf(p), -sinf(p));
            for (i = 0;  i < V27TER_EQUALIZER_LEN;  i++)
                s->eq_buf[i] = complex_mulf(&s->eq_buf[i], &zz);
#endif
            s->carrier_phase += angle;

            s->gardner_step = 2;
            /* We have just seen the first element of the scrambled sequence so skip it. */
            s->training_bc = 1;
            s->training_bc ^= descramble(s, 1);
            descramble(s, 1);
            descramble(s, 1);
            s->training_count = 1;
            s->training_stage = TRAINING_STAGE_TRAIN_ON_ABAB;
            report_status_change(s, SIG_STATUS_TRAINING_IN_PROGRESS);
        }
        else if (++s->training_count > V27TER_TRAINING_SEG_3_LEN)
        {
            /* This is bogus. There are not this many bits in this section
               of a real training sequence. */
            span_log(&s->logging, SPAN_LOG_FLOW, "Training failed (sequence failed)\n");
            /* Park this modem */
            s->training_stage = TRAINING_STAGE_PARKED;
            report_status_change(s, SIG_STATUS_TRAINING_FAILED);
        }
        break;
    case TRAINING_STAGE_TRAIN_ON_ABAB:
        /* Train on the scrambled ABAB section */
        s->training_bc ^= descramble(s, 1);
        descramble(s, 1);
        descramble(s, 1);
        s->constellation_state = abab_pos[s->training_bc];
        track_carrier(s, &z, &v27ter_constellation[s->constellation_state]);
        tune_equalizer(s, &z, &v27ter_constellation[s->constellation_state]);

        if (++s->training_count >= V27TER_TRAINING_SEG_5_LEN)
        {
            s->constellation_state = (s->bit_rate == 4800)  ?  4  :  2;
            s->training_count = 0;
            s->training_stage = TRAINING_STAGE_TEST_ONES;
#if defined(SPANDSP_USE_FIXED_POINTx)
            s->carrier_track_i = 400;
            s->carrier_track_p = 1000000;
#else
            s->carrier_track_i = 400.0f;
            s->carrier_track_p = 1000000.0f;
#endif
        }
        break;
    case TRAINING_STAGE_TEST_ONES:
        decode_baud(s, &z);
        /* Measure the training error */
#if defined(SPANDSP_USE_FIXED_POINTx)
        z1.re = z.re/(float) FP_FACTOR;
        z1.im = z.im/(float) FP_FACTOR;
        if (s->bit_rate == 4800)
            zz = complex_subf(&z, &v27ter_constellation[s->constellation_state]);
        else
            zz = complex_subf(&z, &v27ter_constellation[s->constellation_state << 1]);
        zz = complex_subf(&z1, &zz);
        s->training_error += powerf(&zz);
#else
        if (s->bit_rate == 4800)
            zz = complex_subf(&z, &v27ter_constellation[s->constellation_state]);
        else
            zz = complex_subf(&z, &v27ter_constellation[s->constellation_state << 1]);
        s->training_error += powerf(&zz);
#endif
        if (++s->training_count >= V27TER_TRAINING_SEG_6_LEN)
        {
            /* At 4800bps the symbols are 1.08238 (Euclidian) apart.
               At 2400bps the symbols are 2.0 (Euclidian) apart. */
            if ((s->bit_rate == 4800  &&  s->training_error < V27TER_TRAINING_SEG_6_LEN*0.25f)
                ||
                (s->bit_rate == 2400  &&  s->training_error < V27TER_TRAINING_SEG_6_LEN*0.5f))
            {
                /* We are up and running */
                span_log(&s->logging, SPAN_LOG_FLOW, "Training succeeded at %dbps (constellation mismatch %f)\n", s->bit_rate, s->training_error);
                report_status_change(s, SIG_STATUS_TRAINING_SUCCEEDED);
                /* Apply some lag to the carrier off condition, to ensure the last few bits get pushed through
                   the processing. */
                s->signal_present = (s->bit_rate == 4800)  ?  90  :  120;
                s->training_stage = TRAINING_STAGE_NORMAL_OPERATION;
                equalizer_save(s);
                s->carrier_phase_rate_save = s->carrier_phase_rate;
                s->agc_scaling_save = s->agc_scaling;
            }
            else
            {
                /* Training has failed */
                span_log(&s->logging, SPAN_LOG_FLOW, "Training failed (constellation mismatch %f)\n", s->training_error);
                /* Park this modem */
                s->training_stage = TRAINING_STAGE_PARKED;
                report_status_change(s, SIG_STATUS_TRAINING_FAILED);
            }
        }
        break;
    case TRAINING_STAGE_PARKED:
        /* We failed to train! */
        /* Park here until the carrier drops. */
        break;
    }
    if (s->qam_report)
    {
#if defined(SPANDSP_USE_FIXED_POINTx)
        z1.re = z.re/(float) FP_FACTOR;
        z1.im = z.im/(float) FP_FACTOR;
        zz.re = v27ter_constellation[s->constellation_state].re;
        zz.im = v27ter_constellation[s->constellation_state].im;
        s->qam_report(s->qam_user_data,
                      &z1,
                      &zz,
                      s->constellation_state);
#else
        s->qam_report(s->qam_user_data,
                      &z,
                      &v27ter_constellation[s->constellation_state],
                      s->constellation_state);
#endif
    }
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(int) v27ter_rx(v27ter_rx_state_t *s, const int16_t amp[], int len)
{
    int i;
    int step;
    int16_t x;
    int16_t diff;
#if defined(SPANDSP_USE_FIXED_POINT)
    complexi16_t z;
    complexi16_t zz;
    complexi16_t sample;
    int32_t v;
#else
    complexf_t z;
    complexf_t zz;
    complexf_t sample;
    float v;
#endif
    int32_t power;

    if (s->bit_rate == 4800)
    {
        for (i = 0;  i < len;  i++)
        {
            s->rrc_filter[s->rrc_filter_step] = amp[i];
            if (++s->rrc_filter_step >= V27TER_RX_4800_FILTER_STEPS)
                s->rrc_filter_step = 0;

            /* There should be no DC in the signal, but sometimes there is.
               We need to measure the power with the DC blocked, but not using
               a slow to respond DC blocker. Use the most elementary HPF. */
            x = amp[i] >> 1;
            /* There could be oveflow here, but it isn't a problem in practice */
            diff = x - s->last_sample;
            power = power_meter_update(&(s->power), diff);
#if defined(IAXMODEM_STUFF)
            /* Quick power drop fudge */
            diff = abs(diff);
            if (10*diff < s->high_sample)
            {
                if (++s->low_samples > 120)
                {
                    power_meter_init(&(s->power), 4);
                    s->high_sample = 0;
                    s->low_samples = 0;
                }
            }
            else
            { 
                s->low_samples = 0;
                if (diff > s->high_sample)
                   s->high_sample = diff;
            }
#endif
            s->last_sample = x;
            //span_log(&s->logging, SPAN_LOG_FLOW, "Power = %f\n", power_meter_current_dbm0(&(s->power)));
            if (s->signal_present)
            {
                /* Look for power below turnoff threshold to turn the carrier off */
#if defined(IAXMODEM_STUFF)
                if (s->carrier_drop_pending  ||  power < s->carrier_off_power)
#else
                if (power < s->carrier_off_power)
#endif
                {
                    if (--s->signal_present <= 0)
                    {
                        /* Count down a short delay, to ensure we push the last
                           few bits through the filters before stopping. */
                        v27ter_rx_restart(s, s->bit_rate, FALSE);
                        report_status_change(s, SIG_STATUS_CARRIER_DOWN);
                        continue;
                    }
#if defined(IAXMODEM_STUFF)
                    /* Carrier has dropped, but the put_bit is pending the signal_present delay. */
                    s->carrier_drop_pending = TRUE;
#endif
                }
            }
            else
            {
                /* Look for power exceeding turnon threshold to turn the carrier on */
                if (power < s->carrier_on_power)
                    continue;
                s->signal_present = 1;
#if defined(IAXMODEM_STUFF)
                s->carrier_drop_pending = FALSE;
#endif
                report_status_change(s, SIG_STATUS_CARRIER_UP);
            }
            /* Only spend effort processing this data if the modem is not
               parked, after training failure. */
            if (s->training_stage == TRAINING_STAGE_PARKED)
                continue;
        
            /* Put things into the equalization buffer at T/2 rate. The Gardner algorithm
               will fiddle the step to align this with the symbols. */
            if ((s->eq_put_step -= RX_PULSESHAPER_4800_COEFF_SETS) <= 0)
            {
                if (s->training_stage == TRAINING_STAGE_SYMBOL_ACQUISITION)
                {
                    /* Only AGC during the initial training */
#if defined(SPANDSP_USE_FIXED_POINT)
                    s->agc_scaling = (float) FP_FACTOR*32768.0f*(1.0f/RX_PULSESHAPER_4800_GAIN)*1.414f/sqrtf(power);
#else
                    s->agc_scaling = (1.0f/RX_PULSESHAPER_4800_GAIN)*1.414f/sqrtf(power);
#endif
                }
                /* Pulse shape while still at the carrier frequency, using a quadrature
                   pair of filters. This results in a properly bandpass filtered complex
                   signal, which can be brought directly to baseband by complex mixing.
                   No further filtering, to remove mixer harmonics, is needed. */
                step = -s->eq_put_step;
                if (step > RX_PULSESHAPER_4800_COEFF_SETS - 1)
                    step = RX_PULSESHAPER_4800_COEFF_SETS - 1;
                s->eq_put_step += RX_PULSESHAPER_4800_COEFF_SETS*5/2;
#if defined(SPANDSP_USE_FIXED_POINT)
                v = vec_circular_dot_prodi16(s->rrc_filter, rx_pulseshaper_4800_re[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.re = (v*(int32_t) s->agc_scaling) >> 15;
                v = vec_circular_dot_prodi16(s->rrc_filter, rx_pulseshaper_4800_im[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.im = (v*(int32_t) s->agc_scaling) >> 15;
                z = dds_lookup_complexi16(s->carrier_phase);
                zz.re = ((int32_t) sample.re*(int32_t) z.re - (int32_t) sample.im*(int32_t) z.im) >> 15;
                zz.im = ((int32_t) -sample.re*(int32_t) z.im - (int32_t) sample.im*(int32_t) z.re) >> 15;
#else
                v = vec_circular_dot_prodf(s->rrc_filter, rx_pulseshaper_4800_re[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.re = v*s->agc_scaling;
                v = vec_circular_dot_prodf(s->rrc_filter, rx_pulseshaper_4800_im[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.im = v*s->agc_scaling;
                z = dds_lookup_complexf(s->carrier_phase);
                zz.re = sample.re*z.re - sample.im*z.im;
                zz.im = -sample.re*z.im - sample.im*z.re;
#endif
                process_half_baud(s, &zz);
            }
#if defined(SPANDSP_USE_FIXED_POINT)
            dds_advance(&s->carrier_phase, s->carrier_phase_rate);
#else
            dds_advancef(&s->carrier_phase, s->carrier_phase_rate);
#endif
        }
    }
    else
    {
        for (i = 0;  i < len;  i++)
        {
            s->rrc_filter[s->rrc_filter_step] = amp[i];
            if (++s->rrc_filter_step >= V27TER_RX_2400_FILTER_STEPS)
                s->rrc_filter_step = 0;

            /* There should be no DC in the signal, but sometimes there is.
               We need to measure the power with the DC blocked, but not using
               a slow to respond DC blocker. Use the most elementary HPF. */
            x = amp[i] >> 1;
            /* There could be oveflow here, but it isn't a problem in practice */
            diff = x - s->last_sample;
            power = power_meter_update(&(s->power), diff);
#if defined(IAXMODEM_STUFF)
            /* Quick power drop fudge */
            diff = abs(diff);
            if (10*diff < s->high_sample)
            {
                if (++s->low_samples > 120)
                {
                    power_meter_init(&(s->power), 4);
                    s->high_sample = 0;
                    s->low_samples = 0;
                }
            }
            else
            { 
                s->low_samples = 0;
                if (diff > s->high_sample)
                   s->high_sample = diff;
            }
#endif
            s->last_sample = x;
            //span_log(&s->logging, SPAN_LOG_FLOW, "Power = %f\n", power_meter_current_dbm0(&(s->power)));
            if (s->signal_present)
            {
                /* Look for power below turnoff threshold to turn the carrier off */
#if defined(IAXMODEM_STUFF)
                if (s->carrier_drop_pending  ||  power < s->carrier_off_power)
#else
                if (power < s->carrier_off_power)
#endif
                {
                    if (--s->signal_present <= 0)
                    {
                        /* Count down a short delay, to ensure we push the last
                           few bits through the filters before stopping. */
                        v27ter_rx_restart(s, s->bit_rate, FALSE);
                        report_status_change(s, SIG_STATUS_CARRIER_DOWN);
                        continue;
                    }
#if defined(IAXMODEM_STUFF)
                    /* Carrier has dropped, but the put_bit is
                       pending the signal_present delay. */
                    s->carrier_drop_pending = TRUE;
#endif
                }
            }
            else
            {
                /* Look for power exceeding turnon threshold to turn the carrier on */
                if (power < s->carrier_on_power)
                    continue;
                s->signal_present = 1;
#if defined(IAXMODEM_STUFF)
                s->carrier_drop_pending = FALSE;
#endif
                report_status_change(s, SIG_STATUS_CARRIER_UP);
            }
            /* Only spend effort processing this data if the modem is not
               parked, after training failure. */
            if (s->training_stage == TRAINING_STAGE_PARKED)
                continue;
        
            /* Put things into the equalization buffer at T/2 rate. The Gardner algorithm
               will fiddle the step to align this with the symbols. */
            if ((s->eq_put_step -= RX_PULSESHAPER_2400_COEFF_SETS) <= 0)
            {
                if (s->training_stage == TRAINING_STAGE_SYMBOL_ACQUISITION)
                {
                    /* Only AGC during the initial training */
#if defined(SPANDSP_USE_FIXED_POINT)
                    s->agc_scaling = (float) FP_FACTOR*32768.0f*(1.0f/RX_PULSESHAPER_2400_GAIN)*1.414f/sqrtf(power);
#else
                    s->agc_scaling = (1.0f/RX_PULSESHAPER_2400_GAIN)*1.414f/sqrtf(power);
#endif
                }
                /* Pulse shape while still at the carrier frequency, using a quadrature
                   pair of filters. This results in a properly bandpass filtered complex
                   signal, which can be brought directly to bandband by complex mixing.
                   No further filtering, to remove mixer harmonics, is needed. */
                step = -s->eq_put_step;
                if (step > RX_PULSESHAPER_2400_COEFF_SETS - 1)
                    step = RX_PULSESHAPER_2400_COEFF_SETS - 1;
                s->eq_put_step += RX_PULSESHAPER_2400_COEFF_SETS*20/(3*2);
#if defined(SPANDSP_USE_FIXED_POINT)
                v = vec_circular_dot_prodi16(s->rrc_filter, rx_pulseshaper_2400_re[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.re = (v*(int32_t) s->agc_scaling) >> 15;
                v = vec_circular_dot_prodi16(s->rrc_filter, rx_pulseshaper_2400_im[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.im = (v*(int32_t) s->agc_scaling) >> 15;
                z = dds_lookup_complexi16(s->carrier_phase);
                zz.re = ((int32_t) sample.re*(int32_t) z.re - (int32_t) sample.im*(int32_t) z.im) >> 15;
                zz.im = ((int32_t) -sample.re*(int32_t) z.im - (int32_t) sample.im*(int32_t) z.re) >> 15;
#else
                v = vec_circular_dot_prodf(s->rrc_filter, rx_pulseshaper_2400_re[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.re = v*s->agc_scaling;
                v = vec_circular_dot_prodf(s->rrc_filter, rx_pulseshaper_2400_im[step], V27TER_RX_FILTER_STEPS, s->rrc_filter_step);
                sample.im = v*s->agc_scaling;
                z = dds_lookup_complexf(s->carrier_phase);
                zz.re = sample.re*z.re - sample.im*z.im;
                zz.im = -sample.re*z.im - sample.im*z.re;
#endif
                process_half_baud(s, &zz);
            }
#if defined(SPANDSP_USE_FIXED_POINT)
            dds_advance(&s->carrier_phase, s->carrier_phase_rate);
#else
            dds_advancef(&s->carrier_phase, s->carrier_phase_rate);
#endif
        }
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(void) v27ter_rx_set_put_bit(v27ter_rx_state_t *s, put_bit_func_t put_bit, void *user_data)
{
    s->put_bit = put_bit;
    s->put_bit_user_data = user_data;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(void) v27ter_rx_set_modem_status_handler(v27ter_rx_state_t *s, modem_tx_status_func_t handler, void *user_data)
{
    s->status_handler = handler;
    s->status_user_data = user_data;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(logging_state_t *) v27ter_rx_get_logging_state(v27ter_rx_state_t *s)
{
    return &s->logging;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(int) v27ter_rx_restart(v27ter_rx_state_t *s, int bit_rate, int old_train)
{
    span_log(&s->logging, SPAN_LOG_FLOW, "Restarting V.27ter\n");
    if (bit_rate != 4800  &&  bit_rate != 2400)
        return -1;
    s->bit_rate = bit_rate;

#if defined(SPANDSP_USE_FIXED_POINT)
    vec_zeroi16(s->rrc_filter, sizeof(s->rrc_filter)/sizeof(s->rrc_filter[0]));
#else
    vec_zerof(s->rrc_filter, sizeof(s->rrc_filter)/sizeof(s->rrc_filter[0]));
#endif
    s->rrc_filter_step = 0;

    s->scramble_reg = 0x3C;
    s->scrambler_pattern_count = 0;
    s->training_stage = TRAINING_STAGE_SYMBOL_ACQUISITION;
    s->training_bc = 0;
    s->training_count = 0;
    s->training_error = 0.0f;
    s->signal_present = 0;
#if defined(IAXMODEM_STUFF)
    s->high_sample = 0;
    s->low_samples = 0;
    s->carrier_drop_pending = FALSE;
#endif

    s->carrier_phase = 0;
#if defined(SPANDSP_USE_FIXED_POINTx)
    s->carrier_track_i = 200000;
    s->carrier_track_p = 10000000;
#else
    s->carrier_track_i = 200000.0f;
    s->carrier_track_p = 10000000.0f;
#endif
    power_meter_init(&(s->power), 4);

    s->constellation_state = 0;

    if (s->old_train)
    {
        s->carrier_phase_rate = s->carrier_phase_rate_save;
        s->agc_scaling = s->agc_scaling_save;
        equalizer_restore(s);
    }
    else
    {
        s->carrier_phase_rate = dds_phase_ratef(CARRIER_NOMINAL_FREQ);
#if defined(SPANDSP_USE_FIXED_POINTx)
        s->agc_scaling = (float) FP_FACTOR*32768.0f*0.005f/RX_PULSESHAPER_4800_GAIN;
#else
        s->agc_scaling = 0.005f/RX_PULSESHAPER_4800_GAIN;
#endif
        equalizer_reset(s);
    }
    s->eq_skip = 0;
    s->last_sample = 0;

    s->gardner_integrate = 0;
    s->total_baud_timing_correction = 0;
    s->gardner_step = 512;
    s->baud_half = 0;

    return 0;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(v27ter_rx_state_t *) v27ter_rx_init(v27ter_rx_state_t *s, int bit_rate, put_bit_func_t put_bit, void *user_data)
{
    if (s == NULL)
    {
        if ((s = (v27ter_rx_state_t *) malloc(sizeof(*s))) == NULL)
            return NULL;
    }
    memset(s, 0, sizeof(*s));
    span_log_init(&s->logging, SPAN_LOG_NONE, NULL);
    span_log_set_protocol(&s->logging, "V.27ter RX");
    v27ter_rx_signal_cutoff(s, -45.5f);
    s->put_bit = put_bit;
    s->put_bit_user_data = user_data;

    v27ter_rx_restart(s, bit_rate, FALSE);
    return s;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(int) v27ter_rx_free(v27ter_rx_state_t *s)
{
    free(s);
    return 0;
}
/*- End of function --------------------------------------------------------*/

SPAN_DECLARE(void) v27ter_rx_set_qam_report_handler(v27ter_rx_state_t *s, qam_report_handler_t handler, void *user_data)
{
    s->qam_report = handler;
    s->qam_user_data = user_data;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
