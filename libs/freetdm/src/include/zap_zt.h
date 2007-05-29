/*
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

#ifndef ZAP_ZT_H
#define ZAP_ZT_H
#include "openzap.h"

/* Hardware interface structures and defines */
/* Based on documentation of the structures required for the hardware interface */
/* from http://wiki.freeswitch.org/wiki/Zapata_zaptel_interface */

/* Structures */

/* Used with ioctl: ZT_GET_PARAMS and ZT_SET_PARAMS */
struct zt_params {
	int chan_no;			/* Channel Number							*/
	int span_no;			/* Span Number								*/
	int chan_position;		/* Channel Position							*/
	int sig_type;			/* Signal Type (read-only)					*/
	int sig_cap;			/* Signal Cap (read-only)					*/
	int receive_offhook;	/* Receive is offhook (read-only)			*/
	int receive_bits;		/* Number of bits in receive (read-only)	*/
	int transmit_bits;		/* Number of bits in transmit (read-only)	*/
	int transmit_hook_sig;	/* Transmit Hook Signal (read-only)			*/
	int receive_hook_sig;	/* Receive Hook Signal (read-only)			*/
	int g711_type;			/* Member of zt_g711_t (read-only)			*/
	int idlebits;			/* bits for the idle state (read-only)		*/
	char chan_name[40];		/* Channel Name								*/
	int prewink_time;
	int preflash_time;
	int wink_time;
	int flash_time;
	int start_time;
	int receive_wink_time;
	int receive_flash_time;
	int debounce_time;
	int pulse_break_time;
	int pulse_make_time;
	int pulse_after_time;
};

/* Used with ioctl: ZT_CONFLINK, ZT_GETCONF and ZT_SETCONF */
struct zt_confinfo {
	int chan_no;			/* Channel Number, 0 for current */
	int conference_number;
	int conference_mode;
};

/* Used with ioctl: ZT_GETGAINS and ZT_SETGAINS */
struct zt_gains {
	int chan_no;						/* Channel Number, 0 for current	*/
	unsigned char receive_gain[256];	/* Receive gain table				*/
	unsigned char transmit_gain[256];	/* Transmit gain table				*/
};

/* Used with ioctl: ZT_SPANSTAT */
struct zt_spaninfo {
	int span_no;						/* span number (-1 to use name)				*/
	char name[20];						/* Name of span								*/
	char description[40];				/* Description of span						*/
	int alarms;							/* alarms status							*/
	int transmit_level;					/* Transmit level							*/
	int receive_level;					/* Receive level							*/
	int bpv_count;						/* Current BPV count						*/
	int crc4_count;						/* Current CRC4 error count					*/
	int ebit_count;						/* Current E-bit error count				*/
	int fas_count;						/* Current FAS error count					*/
	int irq_misses;						/* Current IRQ misses						*/
	int sync_src;						/* Span # of sync source (0 = free run)		*/
	int configured_chan_count;			/* Count of channels configured on the span	*/
	int channel_count;					/* Total count of channels on the span		*/
	int span_count;						/* Total count of zaptel spans on the system*/
};

/* Enumerations */

/* Values in zt_params structure for member g711_type */
typedef enum {
	ZT_G711_DEFAULT		= 0,	/* Default mulaw/alaw from the span */
	ZT_G711_MULAW		= 1,
	ZT_G711_ALAW		= 2
} zt_g711_t;

typedef enum {
	ZT_EVENT_NONE			= 0,
	ZT_EVENT_ONHOOK			= 1,
	ZT_EVENT_RINGOFFHOOK	= 2,
	ZT_EVENT_WINKFLASH		= 3,
	ZT_EVENT_ALARM			= 4,
	ZT_EVENT_NOALARM		= 5,
	ZT_EVENT_ABORT			= 6,
	ZT_EVENT_OVERRUN		= 7,
	ZT_EVENT_BADFCS			= 8,
	ZT_EVENT_DIALCOMPLETE	= 9, 
	ZT_EVENT_RINGERON		= 10,
	ZT_EVENT_RINGEROFF		= 11,
	ZT_EVENT_HOOKCOMPLETE	= 12,
	ZT_EVENT_BITSCHANGED	= 13,
	ZT_EVENT_PULSE_START	= 14,
	ZT_EVENT_TIMER_EXPIRED	= 15,
	ZT_EVENT_TIMER_PING		= 16,
	ZT_EVENT_POLARITY		= 17,
	ZT_EVENT_RINGBEGIN		= 18
} zt_event_t;

typedef enum {
	ZT_FLUSH_READ			= 1,
	ZT_FLUSH_WRITE			= 2,
	ZT_FLUSH_BOTH			= (ZT_FLUSH_READ | ZT_FLUSH_WRITE),
	ZT_FLUSH_EVENT			= 4,
	ZT_FLUSH_ALL			= (ZT_FLUSH_READ | ZT_FLUSH_WRITE | ZT_FLUSH_EVENT)
} zt_flush_t;

typedef enum {
	ZT_IOMUX_READ			= 1,
	ZT_IOMUX_WRITE			= 2,
	ZT_IOMUX_WRITEEMPTY		= 4,
	ZT_IOMUX_SIGEVENT		= 8,
	ZT_IOMUX_NOWAIT			= 256
} zt_iomux_t;

typedef enum {
	ZT_ONHOOK				= 0,
	ZT_OFFHOOK				= 1,
	ZT_WINK					= 2,
	ZT_FLASH				= 3,
	ZT_START				= 4,
	ZT_RING					= 5,
	ZT_RINGOFF				= 6
} zt_hookstate_t;


/* Defines */

#define		ZT_MAX_BLOCKSIZE	8192
#define		ZT_DEFAULT_MTU_MRU	2048

/* ioctl defines */

#define		ZT_CODE				'J'

#define		ZT_GET_BLOCKSIZE	_IOW  (ZT_CODE, 1, int)					/* Get Transfer Block Size. */
#define		ZT_SET_BLOCKSIZE	_IOW  (ZT_CODE, 2, int)					/* Set Transfer Block Size. */
#define		ZT_FLUSH			_IOW  (ZT_CODE, 3, int)					/* Flush Buffer(s) and stop I/O */
#define		ZT_SYNC				_IOW  (ZT_CODE, 4, int)					/* Wait for Write to Finish */
#define		ZT_GET_PARAMS		_IOR  (ZT_CODE, 5, struct zt_params)	/* Get channel parameters */
#define		ZT_SET_PARAMS		_IOW  (ZT_CODE, 6, struct zt_params)	/* Set channel parameters */
#define		ZT_HOOK				_IOW  (ZT_CODE, 7, int)					/* Set Hookswitch Status */
#define		ZT_GETEVENT			_IOR  (ZT_CODE, 8, int)					/* Get Signalling Event */
#define		ZT_IOMUX			_IOWR (ZT_CODE, 9, int)					/* Wait for something to happen (IO Mux) */
#define		ZT_SPANSTAT			_IOWR (ZT_CODE, 10, struct zt_spaninfo) /* Get Span Status */

#define		ZT_GETCONF			_IOWR (ZT_CODE, 12, struct zt_confinfo)	/* Get Conference Mode */
#define		ZT_SETCONF			_IOWR (ZT_CODE, 13, struct zt_confinfo)	/* Set Conference Mode */
#define		ZT_CONFLINK			_IOW  (ZT_CODE, 14, struct zt_confinfo)	/* Setup or Remove Conference Link */
#define		ZT_CONFDIAG			_IOR  (ZT_CODE, 15, int)				/* Display Conference Diagnostic Information on Console */

#define		ZT_GETGAINS			_IOWR (ZT_CODE, 16, struct zt_gains)	/* Get Channel audio gains */
#define		ZT_SETGAINS			_IOWR (ZT_CODE, 17, struct zt_gains)	/* Set Channel audio gains */

#define		ZT_AUDIOMODE		_IOW  (ZT_CODE, 32, int)				/* Set a clear channel into audio mode */

#define		ZT_HDLCRAWMODE		_IOW  (ZT_CODE, 36, int)				/* Set a clear channel into HDLC w/out FCS checking/calculation mode */
#define		ZT_HDLCFCSMODE		_IOW  (ZT_CODE, 37, int)				/* Set a clear channel into HDLC w/ FCS mode */

/* Specify a channel on /dev/zap/chan -- must be done before any other ioctl's and is only valid on /dev/zap/chan */
#define		ZT_SPECIFY			_IOW  (ZT_CODE, 38, int)

/* Temporarily set the law on a channel to ZT_LAW_DEFAULT, ZT_LAW_ALAW, or ZT_LAW_MULAW. Is reset on close. */
#define		ZT_SETLAW			_IOW  (ZT_CODE, 39, int)

/* Temporarily set the channel to operate in linear mode when non-zero or default law if 0 */
#define		ZT_SETLINEAR		_IOW  (ZT_CODE, 40, int)

#define		ZT_GETCONFMUTE		_IOR  (ZT_CODE, 49, int)				/* Get Conference to mute mode */



/* Openzap ZT hardware interface functions */
zap_status_t zt_init(zap_io_interface_t **zint);
zap_status_t zt_destroy(void);

#endif
