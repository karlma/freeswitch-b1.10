/*
 * SpanDSP - a series of DSP components for telephony
 *
 * hdlc_tests.c
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2003 Steve Underwood
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
 */

/*! \file */

/*! \page hdlc_tests_page HDLC tests
\section hdlc_tests_page_sec_1 What does it do?
The HDLC tests exercise the HDLC module, and verifies correct operation
using both 16 and 32 bit CRCs.
*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "spandsp.h"
#include "spandsp/private/hdlc.h"

int ref_len;
uint8_t buf[1000];

int abort_reported;
int frame_handled;
int frame_failed;
int frame_len_errors;
int frame_data_errors;
int underflow_reported;
int framing_ok_reported;
int framing_ok_reports;

hdlc_rx_state_t rx;
hdlc_tx_state_t tx;

int frames_sent;
int bytes_sent;
uint64_t start;
uint64_t end;

/* Use a local random generator, so the results are consistent across platforms. We have hard coded
   correct results for a message sequence generated by this particular PRNG. */
static int my_rand(void)
{
    static int rndnum = 1234567;

    return (rndnum = 1664525U*rndnum + 1013904223U) >> 8;
}
/*- End of function --------------------------------------------------------*/

static int cook_up_msg(uint8_t *buf)
{
    int i;
    int len;

    /* Use medium length messages, with some randomised length variation. */
    /* TODO: this doesn't exercise extremely short or long messages. */
    len = (my_rand() & 0x3F) + 100;
    for (i = 0;  i < len;  i++)
        buf[i] = my_rand();
    return len;
}
/*- End of function --------------------------------------------------------*/

static void frame_handler(void *user_data, const uint8_t *pkt, int len, int ok)
{
    if (len < 0)
    {
        /* Special conditions */
        printf("HDLC rx status is %s (%d)\n", signal_status_to_str(len), len);
        switch (len)
        {
        case SIG_STATUS_FRAMING_OK:
            framing_ok_reported = TRUE;
            framing_ok_reports++;
            break;
        case SIG_STATUS_ABORT:
            abort_reported = TRUE;
            break;
        }
        return;
    }
    if (ok)
    {
        if (len != ref_len)
        {
            printf("Len error - %d %d\n", len, ref_len);
            frame_len_errors++;
            return;
        }
        if (memcmp(pkt, buf, len))
        {
            printf("Frame data error\n");
            frame_data_errors++;
            return;
        }
        frame_handled = TRUE;
    }
    else
    {
        frame_failed = TRUE;
    }
}
/*- End of function --------------------------------------------------------*/

static void underflow_handler(void *user_data)
{
    //printf("Underflow reported\n");
    underflow_reported = TRUE;
}
/*- End of function --------------------------------------------------------*/

static void check_result(void)
{
    hdlc_rx_stats_t rx_stats;

    hdlc_rx_get_stats(&rx, &rx_stats);
    printf("%lu bytes\n", rx_stats.bytes);
    printf("%lu good frames\n", rx_stats.good_frames);
    printf("%lu CRC errors\n", rx_stats.crc_errors);
    printf("%lu length errors\n", rx_stats.length_errors);
    printf("%lu aborts\n", rx_stats.aborts);
    printf("%d frame length errors\n", frame_len_errors);
    printf("%d frame data errors\n", frame_data_errors);
    printf("Test duration %" PRIu64 " ticks\n", end - start);
    if (rx_stats.bytes != bytes_sent
        ||
        rx_stats.good_frames != frames_sent
        ||
        rx_stats.crc_errors != 0
        ||
        rx_stats.length_errors != 0
        ||
        rx_stats.aborts != 0
        ||
        frame_len_errors != 0
        ||
        frame_data_errors != 0)
    {
        printf("Tests failed.\n");
        exit(2);
    }
    printf("Test passed.\n\n");
}
/*- End of function --------------------------------------------------------*/

static int test_hdlc_modes(void)
{
    int i;
    int j;
    int len;
    int nextbyte;
    int progress;
    int progress_delay;
    uint8_t bufx[100];

    /* Try sending HDLC messages with CRC-16 */
    printf("Testing with CRC-16 (byte by byte)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, FALSE, 1, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, FALSE, FALSE, 5, frame_handler, NULL);
    underflow_reported = FALSE;

    start = rdtscll();
    hdlc_tx_flags(&tx, 40);
    /* Push an initial message so we should NOT get an underflow after the preamble. */
    ref_len = cook_up_msg(buf);
    hdlc_tx_frame(&tx, buf, ref_len);
    frame_handled = FALSE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    for (i = 0;  i < 1000000;  i++)
    {
        nextbyte = hdlc_tx_get_byte(&tx);
        hdlc_rx_put_byte(&rx, nextbyte);
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            nextbyte = hdlc_tx_get_byte(&tx);
            hdlc_rx_put_byte(&rx, nextbyte);
            frames_sent++;
            bytes_sent += ref_len;
            if (!frame_handled)
            {
                printf("Frame not received.\n");
                return -1;
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, ref_len);
            frame_handled = FALSE;
        }
    }
    end = rdtscll();
    check_result();

    /* Now try sending HDLC messages with CRC-16 */
    printf("Testing with CRC-16 (chunk by chunk)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, FALSE, 1, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, FALSE, FALSE, 5, frame_handler, NULL);
    underflow_reported = FALSE;

    start = rdtscll();
    hdlc_tx_flags(&tx, 40);
    /* Push an initial message so we should NOT get an underflow after the preamble. */
    ref_len = cook_up_msg(buf);
    hdlc_tx_frame(&tx, buf, ref_len);
    frame_handled = FALSE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    for (i = 0;  i < 10000;  i++)
    {
        len = hdlc_tx_get(&tx, bufx, 100);
        hdlc_rx_put(&rx, bufx, len);
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            len = hdlc_tx_get(&tx, bufx, 100);
            hdlc_rx_put(&rx, bufx, len);
            frames_sent++;
            bytes_sent += ref_len;
            if (!frame_handled)
            {
                printf("Frame not received.\n");
                return -1;
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, ref_len);
            frame_handled = FALSE;
        }
    }
    end = rdtscll();
    check_result();

    /* Now try sending HDLC messages with CRC-16 bit by bit */
    printf("Testing with CRC-16 (bit by bit)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, FALSE, 2, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, FALSE, FALSE, 5, frame_handler, NULL);
    underflow_reported = FALSE;

    start = rdtscll();
    hdlc_tx_flags(&tx, 40);
    /* Don't push an initial message so we should get an underflow after the preamble. */
    /* Lie for the first message, as there isn't really one */
    frame_handled = TRUE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    ref_len = 0;
    for (i = 0;  i < 8*1000000;  i++)
    {
        nextbyte = hdlc_tx_get_bit(&tx);
        hdlc_rx_put_bit(&rx, nextbyte);
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            for (j = 0;  j < 20;  j++)
            {
                nextbyte = hdlc_tx_get_bit(&tx);
                hdlc_rx_put_bit(&rx, nextbyte);
            }
            if (ref_len)
            {
                frames_sent++;
                bytes_sent += ref_len;
            }
            if (!frame_handled)
            {
                printf("Frame not received.\n");
                return -1;
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, ref_len);
            frame_handled = FALSE;
        }
    }
    end = rdtscll();
    check_result();

    /* Now try sending HDLC messages with CRC-32 */
    printf("Testing with CRC-32 (byte by byte)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, TRUE, 1, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, TRUE, FALSE, 1, frame_handler, NULL);
    underflow_reported = FALSE;

    start = rdtscll();
    hdlc_tx_flags(&tx, 40);
    /* Don't push an initial message so we should get an underflow after the preamble. */
    /* Lie for the first message, as there isn't really one */
    frame_handled = TRUE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    ref_len = 0;
    for (i = 0;  i < 1000000;  i++)
    {
        nextbyte = hdlc_tx_get_byte(&tx);
        hdlc_rx_put_byte(&rx, nextbyte);
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            nextbyte = hdlc_tx_get_byte(&tx);
            hdlc_rx_put_byte(&rx, nextbyte);
            if (ref_len)
            {
                frames_sent++;
                bytes_sent += ref_len;
            }
            if (!frame_handled)
            {
                printf("Frame not received.\n");
                return -1;
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, ref_len);
            frame_handled = FALSE;
        }
    }
    end = rdtscll();
    check_result();

    /* Now try progressive mode with CRC-16 */
    printf("Testing progressive mode with CRC-16 (byte by byte)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, TRUE, 1, TRUE, underflow_handler, NULL);
    hdlc_rx_init(&rx, TRUE, FALSE, 1, frame_handler, NULL);
    underflow_reported = FALSE;

    start = rdtscll();
    hdlc_tx_flags(&tx, 40);
    /* Don't push an initial message so we should get an underflow after the preamble. */
    /* Lie for the first message, as there isn't really one */
    frame_handled = TRUE;
    frame_failed = FALSE;
    progress = 9999;
    progress_delay = 9999;
    frames_sent = 0;
    bytes_sent = 0;
    ref_len = 0;
    for (i = 0;  i < 1000000;  i++)
    {
        nextbyte = hdlc_tx_get_byte(&tx);
        hdlc_rx_put_byte(&rx, nextbyte);
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            nextbyte = hdlc_tx_get_byte(&tx);
            hdlc_rx_put_byte(&rx, nextbyte);
            if (ref_len)
            {
                frames_sent++;
                bytes_sent += ref_len;
            }
            if (!frame_handled)
            {
                printf("Frame not received.\n");
                return -1;
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, 10);
            progress = 10;
            progress_delay = 8;
            frame_handled = FALSE;
        }
        if (progress < ref_len  &&  progress_delay-- <= 0)
        {
            if (hdlc_tx_frame(&tx, buf + progress, (progress + 10 <= ref_len)  ?  10  :  ref_len - progress) < 0)
            {
                printf("Failed to add progressively\n");
                return -1;
            }
            progress += 10;
            progress_delay = 8;
        }
    }
    end = rdtscll();
    check_result();

    printf("Tests passed.\n");
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int test_hdlc_frame_length_error_handling(void)
{
    int i;
    int j;
    int nextbyte;

    printf("Testing frame length error handling using CRC-16 (bit by bit)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, FALSE, 2, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, FALSE, TRUE, 5, frame_handler, NULL);
    hdlc_rx_set_max_frame_len(&rx, 100);
    underflow_reported = FALSE;
    framing_ok_reported = FALSE;
    framing_ok_reports = 0;

    hdlc_tx_flags(&tx, 10);
    /* Don't push an initial message so we should get an underflow after the preamble. */
    /* Lie for the first message, as there isn't really one */
    frame_handled = TRUE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    ref_len = 0;
    for (i = 0;  i < 8*1000000;  i++)
    {
        nextbyte = hdlc_tx_get_bit(&tx);
        hdlc_rx_put_bit(&rx, nextbyte);
        if (framing_ok_reported)
        {
            printf("Framing OK reported at bit %d (%d)\n", i, framing_ok_reports);
            framing_ok_reported = FALSE;
        }
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            for (j = 0;  j < 20;  j++)
            {
                nextbyte = hdlc_tx_get_bit(&tx);
                hdlc_rx_put_bit(&rx, nextbyte);
                if (framing_ok_reported)
                {
                    printf("Framing OK reported at bit %d (%d) - %d\n", i, framing_ok_reports, frame_handled);
                    framing_ok_reported = FALSE;
                }
            }
            if (ref_len)
            {
                frames_sent++;
                bytes_sent += ref_len;
            }
            if (!frame_handled)
            {
                /* We should get a failure when the length reaches 101 */
                if (ref_len == 101)
                {
                    printf("Tests passed.\n");
                    return 0;
                }
                if (frame_failed)
                    printf("Frame failed.\n");
                printf("Frame not received at length %d.\n", ref_len);
                return -1;
            }
            else
            {
                /* We should get a failure when the length reaches 101 */
                if (ref_len > 100)
                {
                    printf("Tests failed.\n");
                    return -1;
                }
            }
            ref_len++;
            hdlc_tx_frame(&tx, buf, ref_len);
            frame_handled = FALSE;
        }
    }
    /* We shouldn't reach here */
    printf("Tests failed.\n");
    return -1;
}
/*- End of function --------------------------------------------------------*/

static int test_hdlc_crc_error_handling(void)
{
    int i;
    int j;
    int nextbyte;
    int corrupt;

    printf("Testing CRC error handling using CRC-16 (bit by bit)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, FALSE, 2, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, FALSE, TRUE, 5, frame_handler, NULL);
    underflow_reported = FALSE;
    framing_ok_reported = FALSE;
    framing_ok_reports = 0;

    hdlc_tx_flags(&tx, 10);
    /* Don't push an initial message so we should get an underflow after the preamble. */
    /* Lie for the first message, as there isn't really one */
    frame_handled = TRUE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    ref_len = 100;
    corrupt = FALSE;
    for (i = 0;  i < 8*1000000;  i++)
    {
        nextbyte = hdlc_tx_get_bit(&tx);
        hdlc_rx_put_bit(&rx, nextbyte);
        if (framing_ok_reported)
        {
            printf("Framing OK reported at bit %d (%d)\n", i, framing_ok_reports);
            framing_ok_reported = FALSE;
        }
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            for (j = 0;  j < 20;  j++)
            {
                nextbyte = hdlc_tx_get_bit(&tx);
                hdlc_rx_put_bit(&rx, nextbyte);
                if (framing_ok_reported)
                {
                    printf("Framing OK reported at bit %d (%d) - %d\n", i, framing_ok_reports, frame_handled);
                    framing_ok_reported = FALSE;
                }
            }
            if (ref_len)
            {
                frames_sent++;
                bytes_sent += ref_len;
            }
            if (!frame_handled)
            {
                if (!corrupt)
                {
                    printf("Frame not received when it should be correct.\n");
                    return -1;
                }
            }
            else
            {
                if (corrupt)
                {
                    printf("Frame received when it should be corrupt.\n");
                    return -1;
                }
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, ref_len);
            if ((corrupt = rand() & 1))
                hdlc_tx_corrupt_frame(&tx);
            frame_handled = FALSE;
        }
    }

    printf("Tests passed.\n");
    return 0;
}
/*- End of function --------------------------------------------------------*/

static int test_hdlc_abort_handling(void)
{
    int i;
    int j;
    int nextbyte;
    int abort;

    printf("Testing abort handling using CRC-16 (bit by bit)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, FALSE, 2, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, FALSE, TRUE, 0, frame_handler, NULL);
    underflow_reported = FALSE;
    framing_ok_reported = FALSE;
    framing_ok_reports = 0;

    hdlc_tx_flags(&tx, 10);
    /* Don't push an initial message so we should get an underflow after the preamble. */
    /* Lie for the first message, as there isn't really one */
    frame_handled = TRUE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    ref_len = 0;
    abort = FALSE;
    abort_reported = FALSE;
    for (i = 0;  i < 8*1000000;  i++)
    {
        nextbyte = hdlc_tx_get_bit(&tx);
        hdlc_rx_put_bit(&rx, nextbyte);
        if (framing_ok_reported)
        {
            printf("Framing OK reported at bit %d (%d)\n", i, framing_ok_reports);
            framing_ok_reported = FALSE;
        }
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            for (j = 0;  j < 20;  j++)
            {
                nextbyte = hdlc_tx_get_bit(&tx);
                hdlc_rx_put_bit(&rx, nextbyte);
                if (framing_ok_reported)
                {
                    printf("Framing OK reported at bit %d (%d) - %d\n", i, framing_ok_reports, frame_handled);
                    framing_ok_reported = FALSE;
                }
            }
            if (ref_len)
            {
                frames_sent++;
                bytes_sent += ref_len;
            }
            if (abort)
            {
                if (abort  &&  !abort_reported)
                {
                    printf("Abort not received when sent\n");
                    return -1;
                }
                if (frame_handled)
                {
                    if (frame_failed)
                        printf("Frame failed.\n");
                    printf("Frame received when abort sent.\n");
                    return -1;
                }
            }
            else
            {
                if (abort_reported)
                {
                    printf("Abort received when not sent\n");
                    return -1;
                }
                if (!frame_handled)
                {
                    if (frame_failed)
                        printf("Frame failed.\n");
                    printf("Frame not received.\n");
                    return -1;
                }
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, ref_len);
            if ((abort = rand() & 1))
                hdlc_tx_abort(&tx);
            frame_handled = FALSE;
            abort_reported = FALSE;
        }
    }

    printf("Tests passed.\n");
    return 0;
}
/*- End of function --------------------------------------------------------*/

#if 0
static int test_hdlc_octet_count_handling(void)
{
    int i;
    int j;
    int nextbyte;

    printf("Testing the octet_counting handling using CRC-16 (bit by bit)\n");
    frame_len_errors = 0;
    frame_data_errors = 0;
    hdlc_tx_init(&tx, FALSE, 2, FALSE, underflow_handler, NULL);
    hdlc_rx_init(&rx, FALSE, TRUE, 0, frame_handler, NULL);
    //hdlc_rx_set_max_frame_len(&rx, 50);
    hdlc_rx_set_octet_counting_report_interval(&rx, 16);
    underflow_reported = FALSE;
    framing_ok_reported = FALSE;
    framing_ok_reports = 0;

    hdlc_tx_flags(&tx, 10);
    /* Don't push an initial message so we should get an underflow after the preamble. */
    /* Lie for the first message, as there isn't really one */
    frame_handled = TRUE;
    frame_failed = FALSE;
    frames_sent = 0;
    bytes_sent = 0;
    ref_len = 0;
    for (i = 0;  i < 8*1000000;  i++)
    {
        nextbyte = hdlc_tx_get_bit(&tx);
        hdlc_rx_put_bit(&rx, nextbyte);
        if (framing_ok_reported)
        {
            printf("Framing OK reported at bit %d (%d)\n", i, framing_ok_reports);
            framing_ok_reported = FALSE;
        }
        if (underflow_reported)
        {
            underflow_reported = FALSE;
            for (j = 0;  j < 20;  j++)
            {
                nextbyte = hdlc_tx_get_bit(&tx);
                hdlc_rx_put_bit(&rx, nextbyte);
                if (framing_ok_reported)
                {
                    printf("Framing OK reported at bit %d (%d) - %d\n", i, framing_ok_reports, frame_handled);
                    framing_ok_reported = FALSE;
                }
            }
            if (ref_len)
            {
                frames_sent++;
                bytes_sent += ref_len;
            }
            if (!frame_handled)
            {
                if (frame_failed)
                    printf("Frame failed.\n");
                printf("Frame not received.\n");
                return -1;
            }
            ref_len = cook_up_msg(buf);
            hdlc_tx_frame(&tx, buf, ref_len);
            hdlc_tx_abort(&tx);
            //hdlc_tx_corrupt_frame(&tx);
            frame_handled = FALSE;
        }
    }

    printf("Tests passed.\n");
    return 0;
}
/*- End of function --------------------------------------------------------*/
#endif

static void hdlc_tests(void)
{
    printf("HDLC module tests\n");

    if (test_hdlc_modes())
    {
        printf("Tests failed\n");
        exit(2);
    }
    if (test_hdlc_frame_length_error_handling())
    {
        printf("Tests failed\n");
        exit(2);
    }
    if (test_hdlc_crc_error_handling())
    {
        printf("Tests failed\n");
        exit(2);
    }
    if (test_hdlc_abort_handling())
    {
        printf("Tests failed\n");
        exit(2);
    }
#if 0
    if (test_hdlc_octet_count_handling())
    {
        printf("Tests failed\n");
        exit(2);
    }
#endif
    printf("Tests passed.\n");
}
/*- End of function --------------------------------------------------------*/

static void decode_handler(void *user_data, const uint8_t *pkt, int len, int ok)
{
    int i;

    if (len < 0)
    {
        /* Special conditions */
        printf("HDLC rx status is %s (%d)\n", signal_status_to_str(len), len);
        return;
    }
    if (ok)
    {
        printf("Good frame, len = %d\n", len);
        printf("HDLC:  ");
        for (i = 0;  i < len;  i++)
            printf("%02X ", pkt[i]);
        printf("\n");
    }
    else
    {
        printf("Bad frame, len = %d\n", len);
    }
}
/*- End of function --------------------------------------------------------*/

static void decode_bitstream(const char *in_file_name)
{
    char buf[1024];
    int bit;
    int num;
    hdlc_rx_state_t rx;
    FILE *in;
    
    if ((in = fopen(in_file_name, "r")) == NULL)
    {
        fprintf(stderr, "Failed to open '%s'\n", in_file_name);
        exit(2);
    }

    hdlc_rx_init(&rx, FALSE, TRUE, 2, decode_handler, NULL);
    while (fgets(buf, 1024, in))
    {
        if (sscanf(buf, "Rx bit %d - %d", &num, &bit) == 2)
        {
            //printf("Rx bit %d - %d\n", num, bit);
            //printf("Rx bit %d\n", bit);
            hdlc_rx_put_bit(&rx, bit);
        }
    }
    fclose(in);
}
/*- End of function --------------------------------------------------------*/

int main(int argc, char *argv[])
{
    int opt;
    const char *in_file_name;

    in_file_name = NULL;
    while ((opt = getopt(argc, argv, "d:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            in_file_name = optarg;
            break;
        default:
            //usage();
            exit(2);
            break;
        }
    }
    if (in_file_name)
        decode_bitstream(in_file_name);
    else
        hdlc_tests();
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
