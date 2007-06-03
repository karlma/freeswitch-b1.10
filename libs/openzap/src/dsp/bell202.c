
/*
 *	bell202.c
 *
 *	Copyright (c) 2005 Robert Krten.  All Rights Reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *
 *	1. Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the distribution.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 *	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 *	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *	SUCH DAMAGE.
 *
 *	This module contains a Bell-202 1200-baud FSK decoder, suitable for
 *	use in a library.  The general style of the library calls is modeled
 *	after the POSIX pthread_*() functions.
 *
 *	2005 03 20	R. Krten		created
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "bell202.h"
#include "uart.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

// local constants
#define	BELL202_MARK			1200
#define	BELL202_SPACE			2200
#define	BELL202_BAUD			1200

/*
 *	dsp_bell202_attr_init
 *
 *	Initializes the attributes structure; this must be done before the
 *	attributes structure is used.
*/

void
dsp_bell202_attr_init (dsp_bell202_attr_t *attr)
{
	memset (attr, 0, sizeof (*attr));
}

/*
 *	dsp_bell202_attr_get_bithandler
 *	dsp_bell202_attr_set_bithandler
 *	dsp_bell202_attr_get_bytehandler
 *	dsp_bell202_attr_set_bytehandler
 *	dsp_bell202_attr_getsamplerate
 *	dsp_bell202_attr_setsamplerate
 *
 *	These functions get and set their respective elements from the
 *	attributes structure.  If an error code is returned, it is just
 *	zero == ok, -1 == fail.
*/

void (*
dsp_bell202_attr_get_bithandler (dsp_bell202_attr_t *attr, void **bithandler_arg)) (void *, int)
{
	*bithandler_arg = attr -> bithandler_arg;
	return (attr -> bithandler);
}

void
dsp_bell202_attr_set_bithandler (dsp_bell202_attr_t *attr, void (*bithandler) (void *, int ), void *bithandler_arg)
{
	attr -> bithandler = bithandler;
	attr -> bithandler_arg = bithandler_arg;
}

void (*
dsp_bell202_attr_get_bytehandler (dsp_bell202_attr_t *attr, void **bytehandler_arg)) (void *, int)
{
	*bytehandler_arg = attr -> bytehandler_arg;
	return (attr -> bytehandler);
}

void
dsp_bell202_attr_set_bytehandler (dsp_bell202_attr_t *attr, void (*bytehandler) (void *, int ), void *bytehandler_arg)
{
	attr -> bytehandler = bytehandler;
	attr -> bytehandler_arg = bytehandler_arg;
}

int
dsp_bell202_attr_get_samplerate (dsp_bell202_attr_t *attr)
{
	return (attr -> sample_rate);
}

int
dsp_bell202_attr_set_samplerate (dsp_bell202_attr_t *attr, int samplerate)
{
	if (samplerate <= 0) {
		return (-1);
	}
	attr -> sample_rate = samplerate;
	return (0);
}

/*
 *	dsp_bell202_create
 *
 *	Creates a handle for subsequent use.  The handle is created to contain
 *	a context data structure for use by the sample handler function.  The
 *	function expects an initialized attributes structure, and returns the
 *	handle or a NULL if there were errors.
 *
 *	Once created, the handle can be used until it is destroyed.
*/

dsp_bell202_handle_t *
dsp_bell202_create (dsp_bell202_attr_t *attr)
{
	int						i;
	double					phi_mark, phi_space;
	dsp_bell202_handle_t	*handle;

	handle = malloc (sizeof (*handle));
	if (handle == NULL) {
		return (handle);
	}

	memset (handle, 0, sizeof (*handle));

	// fill the attributes member
	memcpy (&handle -> attr, attr, sizeof (*attr));

	// see if we can do downsampling.  We only really need 6 samples to "match"
	if (attr -> sample_rate / BELL202_MARK > 6) {
		handle -> downsampling_count = attr -> sample_rate / BELL202_MARK / 6;
	} else {
		handle -> downsampling_count = 1;
	}
	handle -> current_downsample = 1;

	// calculate the correlate size (number of samples required for slowest wave)
	handle -> corrsize = attr -> sample_rate / handle -> downsampling_count / BELL202_MARK;

	// allocate the correlation sin/cos arrays and initialize
	for (i = 0; i < 4; i++) {
		handle -> correlates [i] = malloc (sizeof (double) * handle -> corrsize);
		if (handle -> correlates [i] == NULL) {
			break;
		}
	}
	if (i != 4) {								// some failed, back out memory allocations
		dsp_bell202_destroy (handle);
		return (NULL);
	}

	// now initialize them
	phi_mark = 2. * M_PI / ((double) attr -> sample_rate / (double) handle -> downsampling_count / (double) BELL202_MARK);
	phi_space = 2. * M_PI / ((double) attr -> sample_rate / (double) handle -> downsampling_count / (double) BELL202_SPACE);

	// printf ("phi_mark is %g, phi_space is %g\n", phi_mark, phi_space);
	for (i = 0; i < handle -> corrsize; i++) {
		handle -> correlates [0][i] = sin (phi_mark * (double) i);
		handle -> correlates [1][i] = cos (phi_mark * (double) i);
		handle -> correlates [2][i] = sin (phi_space * (double) i);
		handle -> correlates [3][i] = cos (phi_space * (double) i);
		// printf ("[%2d] MS %10.4f MC %10.4f SS %10.4f SC %10.4f\n", i, handle -> correlates [0][i], handle -> correlates [1][i], handle -> correlates [2][i], handle -> correlates [3][i]);
	}

	// initialize the ring buffer
	handle -> buffer = malloc (sizeof (double) * handle -> corrsize);
	if (handle -> buffer == NULL) {				// failed; back out memory allocations
		dsp_bell202_destroy (handle);
		return (NULL);
	}
	memset (handle -> buffer, 0, sizeof (double) * handle -> corrsize);
	handle -> ringstart = 0;

	// initalize intra-cell position
	handle -> cellpos = 0;
	handle -> celladj = BELL202_BAUD / (double) attr -> sample_rate * (double) handle -> downsampling_count;

	// printf ("corrsize %d celladj %g\n", handle -> corrsize, handle -> celladj);

	// if they have provided a byte handler, add a UART to the processing chain
	if (handle -> attr.bytehandler) {
		dsp_uart_attr_t		uart_attr;
		dsp_uart_handle_t	*uart_handle;

		dsp_uart_attr_init (&uart_attr);
		dsp_uart_attr_set_bytehandler (&uart_attr, handle -> attr.bytehandler, handle -> attr.bytehandler_arg);
		uart_handle = dsp_uart_create (&uart_attr);
		if (uart_handle == NULL) {
			dsp_bell202_destroy (handle);
			return (NULL);
		}
		handle -> attr.bithandler = dsp_uart_bit_handler;
		handle -> attr.bithandler_arg = uart_handle;
	}

	return (handle);
}

/*
 *	dsp_bell202_destroy
 *
 *	Destroys a handle, releasing any associated memory.  A destroyed handle
 *	should not be used for anything.
*/

void
dsp_bell202_destroy (dsp_bell202_handle_t *handle)
{
	int		i;

	// if empty handle, just return
	if (handle == NULL) {
		return;
	}

	for (i = 0; i < 4; i++) {
		if (handle -> correlates [i] != NULL) {
			free (handle -> correlates [i]);
			handle -> correlates [i] = NULL;
		}
	}

	if (handle -> buffer != NULL) {
		free (handle -> buffer);
		handle -> buffer = NULL;
	}

	free (handle);
}

/*
 *	dsp_bell202_sample
 *
 *	This is the main processing entry point.  The function accepts a normalized
 *	sample (i.e., one whose range is between -1 and +1).  The function performs
 *	the Bell-202 FSK modem decode processing, and, if it detects a valid bit,
 *	will call the bithandler associated with the attributes structure.
 *
 *	For the Bell-202 standard, a logical zero (space) is 2200 Hz, and a logical
 *	one (mark) is 1200 Hz.
*/

void
dsp_bell202_sample (dsp_bell202_handle_t *handle, double normalized_sample)
{
	double	val;
	double	factors [4];
	int		i, j;

	// if we can avoid processing samples, do so
	if (handle -> downsampling_count != 1) {
		if (handle -> current_downsample < handle -> downsampling_count) {
			handle -> current_downsample++;
			return;												// throw this sample out
		}
		handle -> current_downsample = 1;
	}

	// store sample in buffer
	handle -> buffer [handle -> ringstart++] = normalized_sample;
	if (handle -> ringstart >= handle -> corrsize) {
		handle -> ringstart = 0;
	}

	// do the correlation calculation
	factors [0] = factors [1] = factors [2] = factors [3] = 0;	// clear out intermediate sums
	j = handle -> ringstart;
	for (i = 0; i < handle -> corrsize; i++) {
		if (j >= handle -> corrsize) {
			j = 0;
		}
		val = handle -> buffer [j];
		factors [0] += handle -> correlates [0][i] * val;
		factors [1] += handle -> correlates [1][i] * val;
		factors [2] += handle -> correlates [2][i] * val;
		factors [3] += handle -> correlates [3][i] * val;
		j++;
	}

	// store the bit (bit value is comparison of the two sets of correlate factors)
	handle -> previous_bit = handle -> current_bit;
	handle -> current_bit = (factors [0] * factors [0] + factors [1] * factors [1] > factors [2] * factors [2] + factors [3] * factors [3]);

	// printf ("Sample %10.4f factors %10.4f %10.4f %10.4f %10.4f (sum %10.4f) previous %d current %d cellpos %10.4f\n", normalized_sample, factors [0], factors [1], factors [2], factors [3], factors [0] * factors [0] + factors [1] * factors [1] - factors [2] * factors [2] - factors [3] * factors [3], handle -> previous_bit, handle -> current_bit, handle -> cellpos);
	// if there's a transition, we can synchronize the cell position
	if (handle -> previous_bit != handle -> current_bit) {
		handle -> cellpos = 0.5;								// adjust cell position to be in the middle of the cell
	}
	handle -> cellpos += handle -> celladj;						// walk the cell along

	if (handle -> cellpos > 1.0) {
		handle -> cellpos -= 1.0;
		(*handle -> attr.bithandler) (handle -> attr.bithandler_arg, handle -> current_bit);
	}
}

