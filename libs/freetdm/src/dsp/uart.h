
/*
 *	uart.h
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
 *	This module contains the manifest constants and declarations for
 *	the UART module.
 *
 *	2005 06 19	R. Krten		created
*/

#ifndef	__UART_H__
#define	__UART_H__

typedef struct dsp_uart_attr_s
{
	void				(*bytehandler) (void *, int);	// byte handler
	void				*bytehandler_arg;				// arbitrary ID passed to bytehandler as first argument
}	dsp_uart_attr_t;

typedef struct
{
	dsp_uart_attr_t		attr;
	int					have_start;						// wait for start bit to show up
	int					data;							// data buffer
	int					nbits;							// number of bits accumulated so far
}	dsp_uart_handle_t;

/*
 *	Function prototypes
 *
 *	General calling order is:
 *		a) create the attributes structure (dsp_uart_attr_init)
 *		b) initialize fields in the attributes structure (dsp_uart_attr_set_*)
 *		c) create a Bell-202 handle (dsp_uart_create)
 *		d) feed samples through the handler (dsp_uart_sample)
*/

extern	void					dsp_uart_attr_init (dsp_uart_attr_t *attributes);

extern  void					(*dsp_uart_attr_get_bithandler (dsp_uart_attr_t *attributes, void **bithandler_arg)) (void *, int);
extern	void					dsp_uart_attr_set_bithandler (dsp_uart_attr_t *attributes, void (*bithandler) (void *, int ), void *bithandler_arg);
extern  void					(*dsp_uart_attr_get_bytehandler (dsp_uart_attr_t *attributes, void **bytehandler_arg)) (void *, int);
extern	void					dsp_uart_attr_set_bytehandler (dsp_uart_attr_t *attributes, void (*bytehandler) (void *, int ), void *bytehandler_arg);
extern	int						dsp_uart_attr_get_samplerate (dsp_uart_attr_t *attributes);
extern	int						dsp_uart_attr_set_samplerate (dsp_uart_attr_t *attributes, int samplerate);

extern	dsp_uart_handle_t *		dsp_uart_create (dsp_uart_attr_t *attributes);
extern	void					dsp_uart_destroy (dsp_uart_handle_t *handle);

extern	void					dsp_uart_sample (dsp_uart_handle_t *handle, double normalized_sample);

extern	void					dsp_uart_bit_handler (void *handle, int bit);

#endif	// __UART_H__

