/*
 * SpanDSP - a series of DSP components for telephony
 *
 * t31_tests.c - Tests for the T.31 modem.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2004 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: t31_tests.c,v 1.69 2009/01/23 16:07:14 steveu Exp $
 */

/*! \file */

/*! \page t31_tests_page T.31 tests
\section t31_tests_page_sec_1 What does it do?
*/

/* Enable the following definition to enable direct probing into the FAX structures */
//#define WITH_SPANDSP_INTERNALS

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(HAVE_FL_FL_H)  &&  defined(HAVE_FL_FL_CARTESIAN_H)  &&  defined(HAVE_FL_FL_AUDIO_METER_H)
#define ENABLE_GUI
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <audiofile.h>

//#if defined(WITH_SPANDSP_INTERNALS)
#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES
//#endif

#include "spandsp.h"
#include "spandsp/t30_fcf.h"
#include "spandsp-sim.h"

#if defined(ENABLE_GUI)
#include "media_monitor.h"
#endif

#define INPUT_FILE_NAME         "../test-data/itu/fax/itu1.tif"
#define OUTPUT_FILE_NAME        "t31.tif"
#define OUTPUT_WAVE_FILE_NAME   "t31_tests.wav"

#define DLE 0x10
#define ETX 0x03
#define SUB 0x1A

#define MANUFACTURER            "www.soft-switch.org"

#define SAMPLES_PER_CHUNK 160

struct command_response_s
{
    const char *command;
    int len_command;
    const char *response;
    int len_response;
};

g1050_state_t *path_a_to_b;
g1050_state_t *path_b_to_a;

double when = 0.0;

#define EXCHANGE(a,b) {a, sizeof(a) - 1, b, sizeof(b) - 1}
#define RESPONSE(b) {"", 0, b, sizeof(b) - 1}
#define FAST_RESPONSE(b) {NULL, -1, b, sizeof(b) - 1}
#define FAST_SEND(b) {(const char *) 1, -2, b, sizeof(b) - 1}

static const struct command_response_s fax_send_test_seq[] =
{
    EXCHANGE("ATE0\r", "ATE0\r\r\nOK\r\n"),
    EXCHANGE("AT+FCLASS=1\r", "\r\nOK\r\n"),
    EXCHANGE("ATD123456789\r", "\r\nCONNECT\r\n"),
    //<NSF frame>         AT+FRH=3 is implied when dialing in the AT+FCLASS=1 state
    //RESPONSE("\xFF\x03\x10\x03"),
    //RESPONSE("\r\nOK\r\n"),
    //EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<CSI frame data>
    RESPONSE("\xFF\x03\x40\x31\x31\x31\x31\x31\x31\x31\x31\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x1e\x46\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<DIS frame data>
    RESPONSE("\xFF\x13\x80\x00\xEE\xF8\x80\x80\x91\x80\x80\x80\x18\x78\x57\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    //EXCHANGE("AT+FRH=3\r", "\r\nNO CARRIER\r\n"),
    EXCHANGE("AT+FTH=3\r", "\r\nCONNECT\r\n"),
    //<TSI frame data>
    EXCHANGE("\xFF\x03\x43\x32\x32\x32\x32\x32\x32\x32\x32\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x10\x03", "\r\nCONNECT\r\n"),
    //<DCS frame data>
    EXCHANGE("\xFF\x13\x83\x01\xC6\x80\x80\x80\x80\x01\xFD\x13\x10\x03", "\r\nOK\r\n"),
    //Do a wait for timed silence at this point, or there won't be one in the tests
    EXCHANGE("AT+FRS=7\r", "\r\nOK\r\n"),
    //EXCHANGE("AT+FTS=8;+FTM=96\r", "\r\nCONNECT\r\n"),
    EXCHANGE("AT+FTS=8\r", "\r\nOK\r\n"),
    EXCHANGE("AT+FTM=96\r", "\r\nCONNECT\r\n"),
    //<TCF data pattern>
    FAST_SEND("\r\nOK\r\n"),
    EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<CFR frame data>
    RESPONSE("\xFF\x13\x84\xEA\x7D\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    EXCHANGE("AT+FTM=96\r", "\r\nCONNECT\r\n"),
    //<page image data>
    FAST_SEND("\r\nOK\r\n"),
    //EXCHANGE("AT+FTS=8;+FTH=3\r", "\r\nCONNECT\r\n"),
    EXCHANGE("AT+FTS=8\r", "\r\nOK\r\n"),
    EXCHANGE("AT+FTH=3\r", "\r\nCONNECT\r\n"),
    //<EOP frame data>
    EXCHANGE("\xFF\x13\x2E\x10\x03", "\r\nOK\r\n"),
    EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<MCF frame data>
    RESPONSE("\xFF\x13\x8C\xA2\xF1\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    EXCHANGE("AT+FTH=3\r", "\r\nCONNECT\r\n"),
    // <DCN frame data>
    EXCHANGE("\xFF\x13\xFB\x10\x03", "\r\nOK\r\n"),
    EXCHANGE("ATH0\r", "\r\nOK\r\n")
};

static const struct command_response_s fax_receive_test_seq[] =
{
    EXCHANGE("ATE0\r", "ATE0\r\r\nOK\r\n"),
    EXCHANGE("AT+FCLASS=1\r", "\r\nOK\r\n"),
    RESPONSE("\r\nRING\r\n"),
    EXCHANGE("ATA\r", "\r\nCONNECT\r\n"),
    //<CSI frame data>
    EXCHANGE("\xFF\x03\x40\x32\x32\x32\x32\x32\x32\x32\x32\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x10\x03", "\r\nCONNECT\r\n"),
    //<DIS frame data>
    EXCHANGE("\xFF\x13\x80\x01\xCE\xF4\x80\x80\x81\x80\x80\x80\x18\x10\x03", "\r\nOK\r\n"),
    EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<TSI frame data>
    RESPONSE("\xFF\x03\x43\x31\x31\x31\x31\x31\x31\x31\x31\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xAA\x1F\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<DCS frame data>
    RESPONSE("\xFF\x13\x83\x00\xC6\x74\x53\x00\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    EXCHANGE("AT+FRM=96\r", "\r\nCONNECT\r\n"),
    //<TCF data>
    FAST_RESPONSE(NULL),
    RESPONSE("\r\nNO CARRIER\r\n"),
    EXCHANGE("AT+FTH=3\r", "\r\nCONNECT\r\n"),
    //<CFR frame data>
    EXCHANGE("\xFF\x13\x84\x10\x03", "\r\nOK\r\n"),
    EXCHANGE("AT+FRM=96\r", "\r\nCONNECT\r\n"),
    //<page image data>
    FAST_RESPONSE(NULL),
    RESPONSE("\r\nNO CARRIER\r\n"),
    EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<EOP frame data>
    RESPONSE("\xFF\x13\x2F\x33\x66\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    EXCHANGE("AT+FTH=3\r", "\r\nCONNECT\r\n"),
    //<MCF frame data>
    EXCHANGE("\xFF\x13\x8C\x10\x03", "\r\nOK\r\n"),
    EXCHANGE("AT+FRH=3\r", "\r\nCONNECT\r\n"),
    //<DCN frame data>
    RESPONSE("\xFF\x13\xfb\x9a\xf6\x10\x03"),
    RESPONSE("\r\nOK\r\n"),
    EXCHANGE("ATH0\r", "\r\nOK\r\n")
};

char *decode_test_file = NULL;
int countdown = 0;
int command_response_test_step = -1;
char response_buf[1000];
int response_buf_ptr = 0;
int answered = FALSE;
int kick = FALSE;
int dled = FALSE;
int done = FALSE;

static const struct command_response_s *fax_test_seq;

int test_seq_ptr = 0;

t31_state_t *t31_state;

static int phase_b_handler(t30_state_t *s, void *user_data, int result)
{
    int i;
    
    i = (int) (intptr_t) user_data;
    printf("%c: Phase B handler on channel %c - (0x%X) %s\n", i, i, result, t30_frametype(result));
    return T30_ERR_OK;
}
/*- End of function --------------------------------------------------------*/

static int phase_d_handler(t30_state_t *s, void *user_data, int result)
{
    int i;
    t30_stats_t t;
    const char *u;

    i = (int) (intptr_t) user_data;
    printf("%c: Phase D handler on channel %c - (0x%X) %s\n", i, i, result, t30_frametype(result));
    t30_get_transfer_statistics(s, &t);
    printf("%c: Phase D: bit rate %d\n", i, t.bit_rate);
    printf("%c: Phase D: ECM %s\n", i, (t.error_correcting_mode)  ?  "on"  :  "off");
    printf("%c: Phase D: pages transferred %d\n", i, t.pages_transferred);
    printf("%c: Phase D: image size %d x %d\n", i, t.width, t.length);
    printf("%c: Phase D: image resolution %d x %d\n", i, t.x_resolution, t.y_resolution);
    printf("%c: Phase D: bad rows %d\n", i, t.bad_rows);
    printf("%c: Phase D: longest bad row run %d\n", i, t.longest_bad_row_run);
    printf("%c: Phase D: coding method %s\n", i, t4_encoding_to_str(t.encoding));
    printf("%c: Phase D: image size %d\n", i, t.image_size);
    if ((u = t30_get_tx_ident(s)))
        printf("%c: Phase D: local ident '%s'\n", i, u);
    if ((u = t30_get_rx_ident(s)))
        printf("%c: Phase D: remote ident '%s'\n", i, u);
#if defined(WITH_SPANDSP_INTERNALS)
    printf("%c: Phase D: bits per row - min %d, max %d\n", i, s->t4.min_row_bits, s->t4.max_row_bits);
#endif
    return T30_ERR_OK;
}
/*- End of function --------------------------------------------------------*/

static void phase_e_handler(t30_state_t *s, void *user_data, int result)
{
    int i;
    
    i = (intptr_t) user_data;
    printf("Phase E handler on channel %c\n", i);
    //exit(0);
}
/*- End of function --------------------------------------------------------*/

static int modem_call_control(t31_state_t *s, void *user_data, int op, const char *num)
{
    switch (op)
    {
    case AT_MODEM_CONTROL_ANSWER:
        printf("\nModem control - Answering\n");
        answered = TRUE;
        break;
    case AT_MODEM_CONTROL_CALL:
        printf("\nModem control - Dialing '%s'\n", num);
        t31_call_event(t31_state, AT_CALL_EVENT_CONNECTED);
        break;
    case AT_MODEM_CONTROL_HANGUP:
        printf("\nModem control - Hanging up\n");
        done = TRUE;
        break;
    case AT_MODEM_CONTROL_OFFHOOK:
        printf("\nModem control - Going off hook\n");
        break;
    case AT_MODEM_CONTROL_DTR:
        printf("\nModem control - DTR %d\n", (int) (intptr_t) num);
        break;
    case AT_MODEM_CONTROL_RTS:
        printf("\nModem control - RTS %d\n", (int) (intptr_t) num);
        break;
    case AT_MODEM_CONTROL_CTS:
        printf("\nModem control - CTS %d\n", (int) (intptr_t) num);
        break;
    case AT_MODEM_CONTROL_CAR:
        printf("\nModem control - CAR %d\n", (int) (intptr_t) num);
        break;
    case AT_MODEM_CONTROL_RNG:
        printf("\nModem control - RNG %d\n", (int) (intptr_t) num);
        break;
    case AT_MODEM_CONTROL_DSR:
        printf("\nModem control - DSR %d\n", (int) (intptr_t) num);
        break;
    default:
        printf("\nModem control - operation %d\n", op);
        break;
    }
    /*endswitch*/
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int at_tx_handler(at_state_t *s, void *user_data, const uint8_t *buf, size_t len)
{
    size_t i;

    i = 0;
    if (fax_test_seq[test_seq_ptr].command == NULL)
    {
        /* TCF or non-ECM image data expected */
        for (  ;  i < len;  i++)
        {
            if (dled)
            {
                if (buf[i] == ETX)
                {
                    printf("\nFast data ended\n");
                    response_buf_ptr = 0;
                    response_buf[response_buf_ptr] = '\0';
                    test_seq_ptr++;
                    if (fax_test_seq[test_seq_ptr].command)
                        kick = TRUE;
                    break;
                }
                dled = FALSE;
            }
            else
            {
                if (buf[i] == DLE)
                    dled = TRUE;
            }
        }
        i++;
        if (i >= len)
            return 0;
    }
    for (  ;  i < len;  i++)
    {
        response_buf[response_buf_ptr++] = buf[i];
        putchar(buf[i]);
    }
    response_buf[response_buf_ptr] = '\0';
    printf("Expected ");
    for (i = 0;  i < response_buf_ptr;  i++)
        printf("%02x ", fax_test_seq[test_seq_ptr].response[i] & 0xFF);
    printf("\n");
    printf("Response ");
    for (i = 0;  i < response_buf_ptr;  i++)
        printf("%02x ", response_buf[i] & 0xFF);
    printf("\n");
printf("Match %d against %d\n", response_buf_ptr, fax_test_seq[test_seq_ptr].len_response);
    if (response_buf_ptr >= fax_test_seq[test_seq_ptr].len_response
        &&
        memcmp(fax_test_seq[test_seq_ptr].response, response_buf, fax_test_seq[test_seq_ptr].len_response) == 0)
    {
        printf("\nMatched\n");
        test_seq_ptr++;
        response_buf_ptr = 0;
        response_buf[response_buf_ptr] = '\0';
        if (fax_test_seq[test_seq_ptr].command)
            kick = TRUE;
        else
            dled = FALSE;
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int t38_tx_packet_handler(t38_core_state_t *s, void *user_data, const uint8_t *buf, int len, int count)
{
    t31_state_t *t;
    int i;

    /* This routine queues messages between two instances of T.38 processing */
    t = (t31_state_t *) user_data;
    span_log(&s->logging, SPAN_LOG_FLOW, "Send seq %d, len %d, count %d\n", s->tx_seq_no, len, count);

    for (i = 0;  i < count;  i++)
    {
        if (g1050_put(path_a_to_b, buf, len, s->tx_seq_no, when) < 0)
            printf("Lost packet %d\n", s->tx_seq_no);
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int t31_tx_packet_handler(t38_core_state_t *s, void *user_data, const uint8_t *buf, int len, int count)
{
    //t38_terminal_state_t *t;
    int i;

    /* This routine queues messages between two instances of T.38 processing */
    //t = (t38_terminal_state_t *) user_data;
    span_log(&s->logging, SPAN_LOG_FLOW, "Send seq %d, len %d, count %d\n", s->tx_seq_no, len, count);

    for (i = 0;  i < count;  i++)
    {
        if (g1050_put(path_b_to_a, buf, len, s->tx_seq_no, when) < 0)
            printf("Lost packet %d\n", s->tx_seq_no);
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int t38_tests(int use_gui, int test_sending, int model_no, int speed_pattern_no)
{
    t38_terminal_state_t *t38_state;
    int fast_send;
    int fast_blocks;
    uint8_t fast_buf[1000];
    int msg_len;
    uint8_t msg[1024];
    int t38_version;
    int without_pacing;
    int use_tep;
    int seq_no;
    double tx_when;
    double rx_when;
    t30_state_t *t30;
    t38_core_state_t *t38_core;
    logging_state_t *logging;

    t38_version = 1;
    without_pacing = FALSE;
    use_tep = FALSE;

    srand48(0x1234567);
    if ((path_a_to_b = g1050_init(model_no, speed_pattern_no, 100, 33)) == NULL)
    {
        fprintf(stderr, "Failed to start IP network path model\n");
        exit(2);
    }
    if ((path_b_to_a = g1050_init(model_no, speed_pattern_no, 100, 33)) == NULL)
    {
        fprintf(stderr, "Failed to start IP network path model\n");
        exit(2);
    }
    if ((t31_state = t31_init(NULL, at_tx_handler, NULL, modem_call_control, NULL, t31_tx_packet_handler, NULL)) == NULL)
    {
        fprintf(stderr, "    Cannot start the T.31 T.38 modem\n");
        exit(2);
    }
    logging = t31_get_logging_state(t31_state);
    span_log_set_level(logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(logging, "T.31");

    t38_core = t31_get_t38_core_state(t31_state);
    span_log_set_level(&t38_core->logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(&t38_core->logging, "T.31");

    span_log_set_level(&t31_state->at_state.logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(&t31_state->at_state.logging, "T.31");

    t31_set_mode(t31_state, TRUE);
    t38_set_t38_version(t38_core, t38_version);

    if (test_sending)
    {
        if ((t38_state = t38_terminal_init(NULL, FALSE, t38_tx_packet_handler, t31_state)) == NULL)
        {
            fprintf(stderr, "Cannot start the T.38 channel\n");
            exit(2);
        }
        t30 = t38_terminal_get_t30_state(t38_state);
        t30_set_rx_file(t30, OUTPUT_FILE_NAME, -1);
        fax_test_seq = fax_send_test_seq;
        countdown = 0;
    }
    else
    {
        if ((t38_state = t38_terminal_init(NULL, TRUE, t38_tx_packet_handler, t31_state)) == NULL)
        {
            fprintf(stderr, "Cannot start the T.38 channel\n");
            exit(2);
        }
        t30 = t38_terminal_get_t30_state(t38_state);
        t30_set_tx_file(t30, INPUT_FILE_NAME, -1, -1);
        fax_test_seq = fax_receive_test_seq;
        countdown = 250;
    }

    t30 = t38_terminal_get_t30_state(t38_state);
    t38_core = t38_terminal_get_t38_core_state(t38_state);
    t38_set_t38_version(t38_core, t38_version);
    t38_terminal_set_config(t38_state, without_pacing);
    t38_terminal_set_tep_mode(t38_state, use_tep);

    t30_set_tx_ident(t30, "11111111");
    t30_set_supported_modems(t30, T30_SUPPORT_V27TER | T30_SUPPORT_V29 | T30_SUPPORT_V17);
    //t30_set_tx_nsf(t30, (const uint8_t *) "\x50\x00\x00\x00Spandsp\x00", 12);
    t30_set_phase_b_handler(t30, phase_b_handler, (void *) 'A');
    t30_set_phase_d_handler(t30, phase_d_handler, (void *) 'A');
    t30_set_phase_e_handler(t30, phase_e_handler, (void *) 'A');

    logging = t38_terminal_get_logging_state(t38_state);
    span_log_set_level(logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(logging, "T.38");

    t38_core = t38_terminal_get_t38_core_state(t38_state);
    span_log_set_level(&t38_core->logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(&t38_core->logging, "T.38");

    logging = t30_get_logging_state(t30);
    span_log_set_level(logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(logging, "T.38");

    fast_send = FALSE;
    fast_blocks = 0;
    kick = TRUE;
#if defined(ENABLE_GUI)
    if (use_gui)
        start_media_monitor();
#endif
    while (!done)
    {
        logging = t38_terminal_get_logging_state(t38_state);
        span_log_bump_samples(logging, SAMPLES_PER_CHUNK);
        t38_core = t38_terminal_get_t38_core_state(t38_state);
        logging = t38_core_get_logging_state(t38_core);
        span_log_bump_samples(logging, SAMPLES_PER_CHUNK);
        t30 = t38_terminal_get_t30_state(t38_state);
        logging = t30_get_logging_state(t30);
        span_log_bump_samples(logging, SAMPLES_PER_CHUNK);

        logging = t31_get_logging_state(t31_state);
        span_log_bump_samples(logging, SAMPLES_PER_CHUNK);
        t38_core = t31_get_t38_core_state(t31_state);
        logging = t38_core_get_logging_state(t38_core);
        span_log_bump_samples(logging, SAMPLES_PER_CHUNK);
        span_log_bump_samples(&t31_state->at_state.logging, SAMPLES_PER_CHUNK);

        t38_terminal_send_timeout(t38_state, SAMPLES_PER_CHUNK);
        t31_t38_send_timeout(t31_state, SAMPLES_PER_CHUNK);

        when += (float) SAMPLES_PER_CHUNK/(float) SAMPLE_RATE;

        if (kick)
        {
            kick = FALSE;
            if (fax_test_seq[test_seq_ptr].command > (const char *) 1)
            {
                if (fax_test_seq[test_seq_ptr].command[0])
                {
                    printf("%s\n", fax_test_seq[test_seq_ptr].command);
                    t31_at_rx(t31_state, fax_test_seq[test_seq_ptr].command, fax_test_seq[test_seq_ptr].len_command);
                }
            }
            else
            {
                printf("Fast send\n");
                fast_send = TRUE;
                fast_blocks = 100;
            }
        }
        if (fast_send)
        {
            /* Send fast modem data */
            memset(fast_buf, 0, 36);
            if (fast_blocks == 1)
            {
                /* Insert EOLs */
                fast_buf[35] = ETX;
                fast_buf[34] = DLE;
                fast_buf[31] =
                fast_buf[28] =
                fast_buf[25] =
                fast_buf[22] =
                fast_buf[19] =
                fast_buf[16] = 1;
            }
            t31_at_rx(t31_state, (char *) fast_buf, 36);
            if (--fast_blocks == 0)
                fast_send = FALSE;
        }
        if (countdown)
        {
            if (answered)
            {
                countdown = 0;
                t31_call_event(t31_state, AT_CALL_EVENT_ANSWERED);
            }
            else if (--countdown == 0)
            {
                t31_call_event(t31_state, AT_CALL_EVENT_ALERTING);
                countdown = 250;
            }
        }
        while ((msg_len = g1050_get(path_a_to_b, msg, 1024, when, &seq_no, &tx_when, &rx_when)) >= 0)
        {
#if defined(ENABLE_GUI)
            if (use_gui)
                media_monitor_rx(seq_no, tx_when, rx_when);
#endif
            t38_core = t31_get_t38_core_state(t31_state);
            t38_core_rx_ifp_packet(t38_core, msg, msg_len, seq_no);
        }
        while ((msg_len = g1050_get(path_b_to_a, msg, 1024, when, &seq_no, &tx_when, &rx_when)) >= 0)
        {
#if defined(ENABLE_GUI)
            if (use_gui)
                media_monitor_rx(seq_no, tx_when, rx_when);
#endif
            t38_core = t38_terminal_get_t38_core_state(t38_state);
            t38_core_rx_ifp_packet(t38_core, msg, msg_len, seq_no);
        }
#if defined(ENABLE_GUI)
        if (use_gui)
            media_monitor_update_display();
#endif
    }
    t38_terminal_release(t38_state);
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int t30_tests(int log_audio, int test_sending)
{
    int k;
    int outframes;
    fax_state_t *fax_state;
    int16_t t30_amp[SAMPLES_PER_CHUNK];
    int16_t t31_amp[SAMPLES_PER_CHUNK];
    int16_t silence[SAMPLES_PER_CHUNK];
    int16_t out_amp[2*SAMPLES_PER_CHUNK];
    int t30_len;
    int t31_len;
    AFfilehandle wave_handle;
    AFfilehandle in_handle;
    int fast_send;
    int fast_blocks;
    uint8_t fast_buf[1000];
    t30_state_t *t30;
    logging_state_t *logging;

    wave_handle = AF_NULL_FILEHANDLE;
    if (log_audio)
    {
        if ((wave_handle = afOpenFile_telephony_write(OUTPUT_WAVE_FILE_NAME, 2)) == AF_NULL_FILEHANDLE)
        {
            fprintf(stderr, "    Cannot create wave file '%s'\n", OUTPUT_WAVE_FILE_NAME);
            exit(2);
        }
    }

    memset(silence, 0, sizeof(silence));
 
    in_handle = NULL;
    if (decode_test_file)
    {
        if ((in_handle = afOpenFile_telephony_read(decode_test_file, 1)) == AF_NULL_FILEHANDLE)
        {
            fprintf(stderr, "    Cannot create wave file '%s'\n", decode_test_file);
            exit(2);
        }
    }

    if ((t31_state = t31_init(NULL, at_tx_handler, NULL, modem_call_control, NULL, NULL, NULL)) == NULL)
    {
        fprintf(stderr, "    Cannot start the T.31 FAX modem\n");
        exit(2);
    }
    logging = t31_get_logging_state(t31_state);
    span_log_set_level(logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(logging, "T.31");

    if (test_sending)
    {
        fax_state = fax_init(NULL, FALSE);
        t30 = fax_get_t30_state(fax_state);
        t30_set_rx_file(t30, OUTPUT_FILE_NAME, -1);
        fax_test_seq = fax_send_test_seq;
        countdown = 0;
    }
    else
    {
        fax_state = fax_init(NULL, TRUE);
        t30 = fax_get_t30_state(fax_state);
        t30_set_tx_file(t30, INPUT_FILE_NAME, -1, -1);
        fax_test_seq = fax_receive_test_seq;
        countdown = 250;
    }
    
    t30_set_tx_ident(t30, "11111111");
    t30_set_supported_modems(t30, T30_SUPPORT_V27TER | T30_SUPPORT_V29 | T30_SUPPORT_V17);
    t30_set_phase_b_handler(t30, phase_b_handler, (void *) 'A');
    t30_set_phase_d_handler(t30, phase_d_handler, (void *) 'A');
    t30_set_phase_e_handler(t30, phase_e_handler, (void *) 'A');
    memset(t30_amp, 0, sizeof(t30_amp));
    
    logging = t30_get_logging_state(t30);
    span_log_set_level(logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(logging, "FAX");

    logging = fax_get_logging_state(fax_state);
    span_log_set_level(logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG | SPAN_LOG_SHOW_SAMPLE_TIME);
    span_log_set_tag(logging, "FAX");

    fast_send = FALSE;
    fast_blocks = 0;
    kick = TRUE;
    while (!done)
    {
        if (kick)
        {
            kick = FALSE;
            if (fax_test_seq[test_seq_ptr].command > (const char *) 1)
            {
                if (fax_test_seq[test_seq_ptr].command[0])
                {
                    printf("%s\n", fax_test_seq[test_seq_ptr].command);
                    t31_at_rx(t31_state, fax_test_seq[test_seq_ptr].command, fax_test_seq[test_seq_ptr].len_command);
                }
            }
            else
            {
                printf("Fast send\n");
                fast_send = TRUE;
                fast_blocks = 100;
            }
        }
        if (fast_send)
        {
            /* Send fast modem data */
            memset(fast_buf, 0, 36);
            if (fast_blocks == 1)
            {
                /* Insert EOLs */
                fast_buf[35] = ETX;
                fast_buf[34] = DLE;
                fast_buf[31] =
                fast_buf[28] =
                fast_buf[25] =
                fast_buf[22] =
                fast_buf[19] =
                fast_buf[16] = 1;
            }
            t31_at_rx(t31_state, (char *) fast_buf, 36);
            if (--fast_blocks == 0)
                fast_send = FALSE;
        }
        t30_len = fax_tx(fax_state, t30_amp, SAMPLES_PER_CHUNK);
        /* The receive side always expects a full block of samples, but the
           transmit side may not be sending any when it doesn't need to. We
           may need to pad with some silence. */
        if (t30_len < SAMPLES_PER_CHUNK)
        {
            memset(t30_amp + t30_len, 0, sizeof(int16_t)*(SAMPLES_PER_CHUNK - t30_len));
            t30_len = SAMPLES_PER_CHUNK;
        }
        if (log_audio)
        {
            for (k = 0;  k < t30_len;  k++)
                out_amp[2*k] = t30_amp[k];
        }
        if (t31_rx(t31_state, t30_amp, t30_len))
            break;
        if (countdown)
        {
            if (answered)
            {
                countdown = 0;
                t31_call_event(t31_state, AT_CALL_EVENT_ANSWERED);
            }
            else if (--countdown == 0)
            {
                t31_call_event(t31_state, AT_CALL_EVENT_ALERTING);
                countdown = 250;
            }
        }

        t31_len = t31_tx(t31_state, t31_amp, SAMPLES_PER_CHUNK);
        if (t31_len < SAMPLES_PER_CHUNK)
        {
            memset(t31_amp + t31_len, 0, sizeof(int16_t)*(SAMPLES_PER_CHUNK - t31_len));
            t31_len = SAMPLES_PER_CHUNK;
        }
        if (log_audio)
        {
            for (k = 0;  k < t31_len;  k++)
                out_amp[2*k + 1] = t31_amp[k];
        }
        if (fax_rx(fax_state, t31_amp, SAMPLES_PER_CHUNK))
            break;

        if (log_audio)
        {
            outframes = afWriteFrames(wave_handle, AF_DEFAULT_TRACK, out_amp, SAMPLES_PER_CHUNK);
            if (outframes != SAMPLES_PER_CHUNK)
                break;
        }
    }
    if (decode_test_file)
    {
        if (afCloseFile(in_handle) != 0)
        {
            fprintf(stderr, "    Cannot close wave file '%s'\n", decode_test_file);
            exit(2);
        }
    }
    if (log_audio)
    {
        if (afCloseFile(wave_handle) != 0)
        {
            fprintf(stderr, "    Cannot close wave file '%s'\n", OUTPUT_WAVE_FILE_NAME);
            exit(2);
        }
    }
    return 0;
}
/*- End of function --------------------------------------------------------*/

int main(int argc, char *argv[])
{
    int log_audio;
    int t38_mode;
    int test_sending;
    int use_gui;
    int opt;

    decode_test_file = NULL;
    log_audio = FALSE;
    test_sending = FALSE;
    t38_mode = FALSE;
    use_gui = FALSE;
    while ((opt = getopt(argc, argv, "d:glrst")) != -1)
    {
        switch (opt)
        {
        case 'd':
            decode_test_file = optarg;
            break;
        case 'g':
#if defined(ENABLE_GUI)
            use_gui = TRUE;
#else
            fprintf(stderr, "Graphical monitoring not available\n");
            exit(2);
#endif
            break;
        case 'l':
            log_audio = TRUE;
            break;
        case 'r':
            test_sending = FALSE;
            break;
        case 's':
            test_sending = TRUE;
            break;
        case 't':
            t38_mode = TRUE;
            break;
        default:
            //usage();
            exit(2);
            break;
        }
    }

    if (t38_mode)
        t38_tests(use_gui, test_sending, 0, 1);
    else
        t30_tests(log_audio, test_sending);
    if (done)
    {
        printf("Tests passed\n");
    }
    return  0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
