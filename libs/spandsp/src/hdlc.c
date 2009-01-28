/*
 * SpanDSP - a series of DSP components for telephony
 *
 * hdlc.c
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
 * $Id: hdlc.c,v 1.64 2009/01/05 13:48:31 steveu Exp $
 */

/*! \file */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "spandsp/telephony.h"
#include "spandsp/async.h"
#include "spandsp/crc.h"
#include "spandsp/bit_operations.h"
#include "spandsp/hdlc.h"
#include "spandsp/private/hdlc.h"

static void rx_special_condition(hdlc_rx_state_t *s, int condition)
{
    /* Special conditions */
    switch (condition)
    {
    case SIG_STATUS_CARRIER_UP:
    case SIG_STATUS_TRAINING_SUCCEEDED:
        /* Reset the HDLC receiver. */
        s->raw_bit_stream = 0;
        s->len = 0;
        s->num_bits = 0;
        s->flags_seen = 0;
        s->framing_ok_announced = FALSE;
        /* Fall through */
    case SIG_STATUS_TRAINING_IN_PROGRESS:
    case SIG_STATUS_TRAINING_FAILED:
    case SIG_STATUS_CARRIER_DOWN:
    case SIG_STATUS_END_OF_DATA:
        s->frame_handler(s->user_data, NULL, condition, TRUE);
        break;
    default:
        //printf("Eh!\n");
        break;
    }
}
/*- End of function --------------------------------------------------------*/

static __inline__ void octet_set_and_count(hdlc_rx_state_t *s)
{
    if (s->octet_count_report_interval == 0)
        return;
        
    /* If we are not in octet counting mode, we start it.
       If we are in octet counting mode, we update it. */
    if (s->octet_counting_mode)
    {
        if (--s->octet_count <= 0)
        {
            s->octet_count = s->octet_count_report_interval;
            s->frame_handler(s->user_data, NULL, SIG_STATUS_OCTET_REPORT, TRUE);
        }
    }
    else
    {
        s->octet_counting_mode = TRUE;
        s->octet_count = s->octet_count_report_interval;
    }
}
/*- End of function --------------------------------------------------------*/

static __inline__ void octet_count(hdlc_rx_state_t *s)
{
    if (s->octet_count_report_interval == 0)
        return;

    /* If we are not in octet counting mode, we start it.
       If we are in octet counting mode, we update it. */
    if (s->octet_counting_mode)
    {
        if (--s->octet_count <= 0)
        {
            s->octet_count = s->octet_count_report_interval;
            s->frame_handler(s->user_data, NULL, SIG_STATUS_OCTET_REPORT, TRUE);
        }
    }
}
/*- End of function --------------------------------------------------------*/

static void rx_flag_or_abort(hdlc_rx_state_t *s)
{
    if ((s->raw_bit_stream & 0x8000))
    {
        /* Hit HDLC abort */
        s->rx_aborts++;
        s->frame_handler(s->user_data, NULL, SIG_STATUS_ABORT, TRUE);
        /* If we have not yet seen enough flags, restart the count. If we
           are beyond that point, just back off one step, so we need to see
           another flag before proceeding to collect frame octets. */
        if (s->flags_seen < s->framing_ok_threshold - 1)
            s->flags_seen = 0;
        else
            s->flags_seen = s->framing_ok_threshold - 1;
        /* An abort starts octet counting */
        octet_set_and_count(s);
    }
    else
    {
        /* Hit HDLC flag */
        /* A flag clears octet counting */
        s->octet_counting_mode = FALSE;
        if (s->flags_seen >= s->framing_ok_threshold)
        {
            /* We may have a frame, or we may have back to back flags */
            if (s->len)
            {
                if (s->num_bits == 7  &&  s->len >= s->crc_bytes  &&  s->len <= s->max_frame_len)
                {
                    if ((s->crc_bytes == 2  &&  crc_itu16_check(s->buffer, s->len))
                        ||
                        (s->crc_bytes != 2  &&  crc_itu32_check(s->buffer, s->len)))
                    {
                        s->rx_frames++;
                        s->rx_bytes += s->len - s->crc_bytes;
                        s->len -= s->crc_bytes;
                        s->frame_handler(s->user_data, s->buffer, s->len, TRUE);
                    }
                    else
                    {
                        s->rx_crc_errors++;
                        if (s->report_bad_frames)
                        {
                            s->len -= s->crc_bytes;
                            s->frame_handler(s->user_data, s->buffer, s->len, FALSE);
                        }
                    }
                }
                else
                {
                    /* Frame too short or too long, or the flag is misaligned with its octets. */
                    if (s->report_bad_frames)
                    {
                        /* Don't let the length go below zero, or it will be confused
                           with one of the special conditions. */
                        if (s->len >= s->crc_bytes)
                            s->len -= s->crc_bytes;
                        else
                            s->len = 0;
                        s->frame_handler(s->user_data, s->buffer, s->len, FALSE);
                    }
                    s->rx_length_errors++;
                }
            }
        }
        else
        {
            /* Check the flags are back-to-back when testing for valid preamble. This
               greatly reduces the chances of false preamble detection, and anything
               which doesn't send them back-to-back is badly broken. */
            if (s->num_bits != 7)
            {
                /* Don't set the flags seen indicator back to zero too aggressively.
                   We want to pick up with the minimum of discarded data when there
                   is a bit error in the stream, and a bit error could emulate a
                   misaligned flag. */
                if (s->flags_seen < s->framing_ok_threshold - 1)
                    s->flags_seen = 0;
                else
                    s->flags_seen = s->framing_ok_threshold - 1;
            }
            if (++s->flags_seen >= s->framing_ok_threshold  &&  !s->framing_ok_announced)
            {
                s->frame_handler(s->user_data, NULL, SIG_STATUS_FRAMING_OK, TRUE);
                s->framing_ok_announced = TRUE;
            }
        }
    }
    s->len = 0;
    s->num_bits = 0;
}
/*- End of function --------------------------------------------------------*/

static __inline__ void hdlc_rx_put_bit_core(hdlc_rx_state_t *s)
{
    if ((s->raw_bit_stream & 0x3F00) == 0x3E00)
    {
        /* Its time to either skip a bit, for stuffing, or process a
           flag or abort */
        if ((s->raw_bit_stream & 0x4000))
            rx_flag_or_abort(s);
        return;
    }
    s->num_bits++;
    if (s->flags_seen < s->framing_ok_threshold)
    {
        if ((s->num_bits & 0x7) == 0)
            octet_count(s);
        return;
    }
    s->byte_in_progress = (s->byte_in_progress | (s->raw_bit_stream & 0x100)) >> 1;
    if (s->num_bits == 8)
    {
        /* Ensure we do not accept an overlength frame, and especially that
           we do not overflow our buffer */
        if (s->len < s->max_frame_len)
        {
            s->buffer[s->len++] = (uint8_t) s->byte_in_progress;
        }
        else
        {
            /* This is too long. Abandon the frame, and wait for the next
               flag octet. */
            s->len = sizeof(s->buffer) + 1;
            s->flags_seen = s->framing_ok_threshold - 1;
            octet_set_and_count(s);
        }
        s->num_bits = 0;
    }
}
/*- End of function --------------------------------------------------------*/

void hdlc_rx_put_bit(hdlc_rx_state_t *s, int new_bit)
{
    if (new_bit < 0)
    {
        rx_special_condition(s, new_bit);
        return;
    }
    s->raw_bit_stream = (s->raw_bit_stream << 1) | ((new_bit << 8) & 0x100);
    hdlc_rx_put_bit_core(s);
}
/*- End of function --------------------------------------------------------*/

void hdlc_rx_put_byte(hdlc_rx_state_t *s, int new_byte)
{
    int i;

    if (new_byte < 0)
    {
        rx_special_condition(s, new_byte);
        return;
    }
    s->raw_bit_stream |= new_byte;
    for (i = 0;  i < 8;  i++)
    {
        s->raw_bit_stream <<= 1;
        hdlc_rx_put_bit_core(s);
    }
}
/*- End of function --------------------------------------------------------*/

void hdlc_rx_put(hdlc_rx_state_t *s, const uint8_t buf[], int len)
{
    int i;

    for (i = 0;  i < len;  i++)
        hdlc_rx_put_byte(s, buf[i]);
}
/*- End of function --------------------------------------------------------*/

void hdlc_rx_set_max_frame_len(hdlc_rx_state_t *s, size_t max_len)
{
    max_len += s->crc_bytes;
    s->max_frame_len = (max_len <= sizeof(s->buffer))  ?  max_len  :  sizeof(s->buffer);
}
/*- End of function --------------------------------------------------------*/

void hdlc_rx_set_octet_counting_report_interval(hdlc_rx_state_t *s, int interval)
{
    s->octet_count_report_interval = interval;
}
/*- End of function --------------------------------------------------------*/

hdlc_rx_state_t *hdlc_rx_init(hdlc_rx_state_t *s,
                              int crc32,
                              int report_bad_frames,
                              int framing_ok_threshold,
                              hdlc_frame_handler_t handler,
                              void *user_data)
{
    if (s == NULL)
    {
        if ((s = (hdlc_rx_state_t *) malloc(sizeof(*s))) == NULL)
            return NULL;
    }
    memset(s, 0, sizeof(*s));
    s->frame_handler = handler;
    s->user_data = user_data;
    s->crc_bytes = (crc32)  ?  4  :  2;
    s->report_bad_frames = report_bad_frames;
    s->framing_ok_threshold = (framing_ok_threshold < 1)  ?  1  :  framing_ok_threshold;
    s->max_frame_len = sizeof(s->buffer);
    return s;
}
/*- End of function --------------------------------------------------------*/

int hdlc_rx_get_stats(hdlc_rx_state_t *s,
                      hdlc_rx_stats_t *t)
{
    t->bytes = s->rx_bytes;
    t->good_frames = s->rx_frames;
    t->crc_errors = s->rx_crc_errors;
    t->length_errors = s->rx_length_errors;
    t->aborts = s->rx_aborts;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_frame(hdlc_tx_state_t *s, const uint8_t *frame, size_t len)
{
    if (len <= 0)
    {
        s->tx_end = TRUE;
        return 0;
    }
    if (s->len + len > s->max_frame_len)
        return -1;
    if (s->progressive)
    {
        /* Only lock out if we are in the CRC section. */
        if (s->pos >= HDLC_MAXFRAME_LEN)
            return -1;
    }
    else
    {
        /* Lock out if there is anything in the buffer. */
        if (s->len)
            return -1;
    }
    memcpy(s->buffer + s->len, frame, len);
    if (s->crc_bytes == 2)
        s->crc = crc_itu16_calc(frame, len, (uint16_t) s->crc);
    else
        s->crc = crc_itu32_calc(frame, len, s->crc);
    if (s->progressive)
        s->len += len;
    else
        s->len = len;
    s->tx_end = FALSE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_flags(hdlc_tx_state_t *s, int len)
{
    /* Some HDLC applications require the ability to force a period of HDLC
       flag words. */
    if (s->pos)
        return -1;
    if (len < 0)
        s->flag_octets += -len;
    else
        s->flag_octets = len;
    s->report_flag_underflow = TRUE;
    s->tx_end = FALSE;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_abort(hdlc_tx_state_t *s)
{
    /* TODO: This is a really crude way of just fudging an abort out for simple
             test purposes. */
    s->flag_octets++;
    s->abort_octets++;
    return -1;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_corrupt_frame(hdlc_tx_state_t *s)
{
    if (s->len <= 0)
        return -1;
    s->crc ^= 0xFFFF;
    s->buffer[HDLC_MAXFRAME_LEN] ^= 0xFF;
    s->buffer[HDLC_MAXFRAME_LEN + 1] ^= 0xFF;
    s->buffer[HDLC_MAXFRAME_LEN + 2] ^= 0xFF;
    s->buffer[HDLC_MAXFRAME_LEN + 3] ^= 0xFF;
    return 0;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_get_byte(hdlc_tx_state_t *s)
{
    int i;
    int byte_in_progress;
    int txbyte;

    if (s->flag_octets > 0)
    {
        /* We are in a timed flag section (preamble, inter frame gap, etc.) */
        if (--s->flag_octets <= 0  &&  s->report_flag_underflow)
        {
            s->report_flag_underflow = FALSE;
            if (s->len == 0)
            {
                /* The timed flags have finished, there is nothing else queued to go,
                   and we have been told to report this underflow. */
                if (s->underflow_handler)
                    s->underflow_handler(s->user_data);
            }
        }
        if (s->abort_octets)
        {
            s->abort_octets = 0;
            return 0x7F;
        }   
        return s->idle_octet;
    }
    if (s->len)
    {
        if (s->num_bits >= 8)
        {
            s->num_bits -= 8;
            return (s->octets_in_progress >> s->num_bits) & 0xFF;
        }
        if (s->pos >= s->len)
        {
            if (s->pos == s->len)
            {
                s->crc ^= 0xFFFFFFFF;
                s->buffer[HDLC_MAXFRAME_LEN] = (uint8_t) s->crc;
                s->buffer[HDLC_MAXFRAME_LEN + 1] = (uint8_t) (s->crc >> 8);
                if (s->crc_bytes == 4)
                {
                    s->buffer[HDLC_MAXFRAME_LEN + 2] = (uint8_t) (s->crc >> 16);
                    s->buffer[HDLC_MAXFRAME_LEN + 3] = (uint8_t) (s->crc >> 24);
                }
                s->pos = HDLC_MAXFRAME_LEN;
            }
            else if (s->pos == HDLC_MAXFRAME_LEN + s->crc_bytes)
            {
                /* Finish off the current byte with some flag bits. If we are at the
                   start of a byte we need a at least one whole byte of flag to ensure
                   we cannot end up with back to back frames, and no flag octet at all */
                txbyte = (uint8_t) ((s->octets_in_progress << (8 - s->num_bits)) | (0x7E >> s->num_bits));
                /* Create a rotated octet of flag for idling... */
                s->idle_octet = (0x7E7E >> s->num_bits) & 0xFF;
                /* ...and the partial flag octet needed to start off the next message. */
                s->octets_in_progress = s->idle_octet >> (8 - s->num_bits);
                s->flag_octets = s->inter_frame_flags - 1;
                s->len = 0;
                s->pos = 0;
                if (s->crc_bytes == 2)
                    s->crc = 0xFFFF;
                else
                    s->crc = 0xFFFFFFFF;
                /* Report the underflow now. If there are timed flags still in progress, loading the
                   next frame right now will be harmless. */
                s->report_flag_underflow = FALSE;
                if (s->underflow_handler)
                    s->underflow_handler(s->user_data);
                /* Make sure we finish off with at least one flag octet, if the underflow report did not result
                   in a new frame being sent. */
                if (s->len == 0  &&  s->flag_octets < 2)
                    s->flag_octets = 2;
                return txbyte;
            }
        }
        byte_in_progress = s->buffer[s->pos++];
        i = bottom_bit(byte_in_progress | 0x100);
        s->octets_in_progress <<= i;
        byte_in_progress >>= i;
        for (  ;  i < 8;  i++)
        {
            s->octets_in_progress = (s->octets_in_progress << 1) | (byte_in_progress & 0x01);
            byte_in_progress >>= 1;
            if ((s->octets_in_progress & 0x1F) == 0x1F)
            {
                /* There are 5 ones - stuff */
                s->octets_in_progress <<= 1;
                s->num_bits++;
            }
        }
        /* An input byte will generate between 8 and 10 output bits */
        return (s->octets_in_progress >> s->num_bits) & 0xFF;
    }
    /* Untimed idling on flags */
    if (s->tx_end)
    {
        s->tx_end = FALSE;
        return SIG_STATUS_END_OF_DATA;
    }
    return s->idle_octet;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_get_bit(hdlc_tx_state_t *s)
{
    int txbit;

    if (s->bits == 0)
    {
        if ((s->byte = hdlc_tx_get_byte(s)) < 0)
            return s->byte;
        s->bits = 8;
    }
    s->bits--;
    txbit = (s->byte >> s->bits) & 0x01;
    return  txbit;
}
/*- End of function --------------------------------------------------------*/

int hdlc_tx_get(hdlc_tx_state_t *s, uint8_t buf[], size_t max_len)
{
    int i;
    int x;

    for (i = 0;  i < max_len;  i++)
    {
        if ((x = hdlc_tx_get_byte(s)) == SIG_STATUS_END_OF_DATA)
            return i;
        buf[i] = x;
    }
    return i;
}
/*- End of function --------------------------------------------------------*/

void hdlc_tx_set_max_frame_len(hdlc_tx_state_t *s, size_t max_len)
{
    s->max_frame_len = (max_len <= HDLC_MAXFRAME_LEN)  ?  max_len  :  HDLC_MAXFRAME_LEN;
}
/*- End of function --------------------------------------------------------*/

hdlc_tx_state_t *hdlc_tx_init(hdlc_tx_state_t *s,
                              int crc32,
                              int inter_frame_flags,
                              int progressive,
                              hdlc_underflow_handler_t handler,
                              void *user_data)
{
    if (s == NULL)
    {
        if ((s = (hdlc_tx_state_t *) malloc(sizeof(*s))) == NULL)
            return NULL;
    }
    memset(s, 0, sizeof(*s));
    s->idle_octet = 0x7E;
    s->underflow_handler = handler;
    s->user_data = user_data;
    s->inter_frame_flags = (inter_frame_flags < 1)  ?  1  :  inter_frame_flags;
    if (crc32)
    {
        s->crc_bytes = 4;
        s->crc = 0xFFFFFFFF;
    }
    else
    {
        s->crc_bytes = 2;
        s->crc = 0xFFFF;
    }
    s->progressive = progressive;
    s->max_frame_len = HDLC_MAXFRAME_LEN;
    return s;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
