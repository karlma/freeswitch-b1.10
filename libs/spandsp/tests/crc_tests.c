/*
 * SpanDSP - a series of DSP components for telephony
 *
 * crc_tests.c
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

/*! \page crc_tests_page CRC tests
\section crc_tests_page_sec_1 What does it do?
The CRC tests exercise the ITU-16 and ITU-32 CRC module, and verifies
correct operation.
*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spandsp.h"

int ref_len;
uint8_t buf[1000];

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

int main(int argc, char *argv[])
{
    int i;
    int j;
    int len;
    uint16_t crc16a;
    uint16_t crc16b;

    printf("CRC module tests\n");

    /* TODO: This doesn't check every function in the module */

    /* Try a few random messages through the CRC logic. */
    printf("Testing the CRC-16 routines\n");
    for (i = 0;  i < 100;  i++)
    {
        ref_len = cook_up_msg(buf);
        len = crc_itu16_append(buf, ref_len);
        if (!crc_itu16_check(buf, len))
        {
            printf("CRC-16 failure\n");
            exit(2);
        }
    }
    printf("Test passed.\n\n");

    printf("Testing the CRC-16 byte by byte and bit by bit routines\n");
    for (i = 0;  i < 100;  i++)
    {
        ref_len = cook_up_msg(buf);
        crc16a = 0xFFFF;
        crc16a = crc_itu16_calc(buf, ref_len, crc16a);

        crc16b = 0xFFFF;
        for (j = 0;  j < ref_len;  j++)
            crc16b = crc_itu16_bits(buf[j], 8, crc16b);
        if (crc16a != crc16b)
        {
            printf("CRC-16 failure\n");
            exit(2);
        }
    }
    printf("Test passed.\n\n");

    printf("Testing the CRC-32 routines\n");
    for (i = 0;  i < 100;  i++)
    {
        ref_len = cook_up_msg(buf);
        len = crc_itu32_append(buf, ref_len);
        if (!crc_itu32_check(buf, len))
        {
            printf("CRC-32 failure\n");
            exit(2);
        }
    }
    printf("Test passed.\n");
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
