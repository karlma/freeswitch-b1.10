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

#ifndef ZAP_TYPES_H
#define ZAP_TYPES_H
#include "fsk.h"

#ifdef WIN32
#include <windows.h>
typedef HANDLE zap_socket_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8 int8_t;
typedef intptr_t zap_ssize_t;
typedef int zap_filehandle_t;
#else
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
typedef int zap_socket_t;
typedef ssize_t zap_ssize_t;
typedef int zap_filehandle_t;
#endif

typedef size_t zap_size_t;
struct zap_io_interface;

#define ZAP_COMMAND_OBJ_INT *((int *)obj)
#define ZAP_COMMAND_OBJ_CHAR_P (char *)obj
#define ZAP_FSK_MOD_FACTOR 0x10000
#define ZAP_DEFAULT_DTMF_ON 250
#define ZAP_DEFAULT_DTMF_OFF 50

typedef uint64_t zap_time_t; 

typedef enum {
	ZAP_ENDIAN_BIG = 1,
	ZAP_ENDIAN_LITTLE = -1
} zap_endian_t;

typedef enum {
	ZAP_CID_TYPE_SDMF = 0x04,
	ZAP_CID_TYPE_MDMF = 0x80
} zap_cid_type_t;

typedef enum {
	MDMF_DATETIME = 1,
	MDMF_PHONE_NUM = 2,
	MDMF_DDN = 3,
	MDMF_NO_NUM = 4,
	MDMF_PHONE_NAME = 7,
	MDMF_NO_NAME = 8,
	MDMF_ALT_ROUTE = 9,
	MDMF_INVALID = 10
} zap_mdmf_type_t;
#define MDMF_STRINGS "X", "DATETIME", "PHONE_NUM", "DDN", "NO_NUM", "X", "X", "PHONE_NAME", "NO_NAME", "ALT_ROUTE", "INVALID"
ZAP_STR2ENUM_P(zap_str2zap_mdmf_type, zap_mdmf_type2str, zap_mdmf_type_t)

#define ZAP_TONEMAP_LEN 128
typedef enum {
	ZAP_TONEMAP_NONE,
	ZAP_TONEMAP_DIAL,
	ZAP_TONEMAP_RING,
	ZAP_TONEMAP_BUSY,
	ZAP_TONEMAP_FAIL1,
	ZAP_TONEMAP_FAIL2,
	ZAP_TONEMAP_FAIL3,
	ZAP_TONEMAP_ATTN,
	ZAP_TONEMAP_CALLWAITING_CAS,
	ZAP_TONEMAP_CALLWAITING_SAS,
	ZAP_TONEMAP_CALLWAITING_ACK,
	ZAP_TONEMAP_INVALID
} zap_tonemap_t;
#define TONEMAP_STRINGS "NONE", "DIAL", "RING", "BUSY", "FAIL1", "FAIL2", "FAIL3", "ATTN", "CALLWAITING-CAS", "CALLWAITING-SAS", "CALLWAITING-ACK", "INVALID"
ZAP_STR2ENUM_P(zap_str2zap_tonemap, zap_tonemap2str, zap_tonemap_t)

typedef enum {
	ZAP_TRUNK_E1,
	ZAP_TRUNK_T1,
	ZAP_TRUNK_J1,
	ZAP_TRUNK_BRI,
	ZAP_TRUNK_FXO,
	ZAP_TRUNK_FXS,
	ZAP_TRUNK_NONE
} zap_trunk_type_t;
#define TRUNK_STRINGS "E1", "T1", "J1", "BRI", "FXO", "FXS", "NONE"
ZAP_STR2ENUM_P(zap_str2zap_trunk_type, zap_trunk_type2str, zap_trunk_type_t)

typedef enum {
	ZAP_ANALOG_START_KEWL,
	ZAP_ANALOG_START_LOOP,
	ZAP_ANALOG_START_GROUND,
	ZAP_ANALOG_START_NA
} zap_analog_start_type_t;
#define START_TYPE_STRINGS "KEWL", "LOOP", "GROUND", "NA"
ZAP_STR2ENUM_P(zap_str2zap_analog_start_type, zap_analog_start_type2str, zap_analog_start_type_t)

typedef enum {
	ZAP_OOB_DTMF, 
	ZAP_OOB_ONHOOK,
	ZAP_OOB_OFFHOOK,
	ZAP_OOB_WINK,
	ZAP_OOB_FLASH,
	ZAP_OOB_RING_START,
	ZAP_OOB_RING_STOP,
	ZAP_OOB_ALARM_TRAP,
	ZAP_OOB_ALARM_CLEAR,
	ZAP_OOB_NOOP,
	ZAP_OOB_INVALID
} zap_oob_event_t;
#define OOB_STRINGS "DTMF", "ONHOOK", "OFFHOOK", "WINK", "FLASH", "RING_START", "RING_STOP", "ALARM_TRAP", "ALARM_CLEAR", "NOOP", "INVALID"
ZAP_STR2ENUM_P(zap_str2zap_oob_event, zap_oob_event2str, zap_oob_event_t)

typedef enum {
	ZAP_ALARM_NONE = 0,
	ZAP_ALARM_RECOVER = (1 << 0),
	ZAP_ALARM_LOOPBACK = (1 << 2),
	ZAP_ALARM_YELLOW = (1 << 3),
	ZAP_ALARM_RED = (1 << 4),
	ZAP_ALARM_BLUE = (1 << 5),
	ZAP_ALARM_NOTOPEN = ( 1 << 6)
} zap_alarm_flag_t;

typedef enum {
	ZAP_SIGTYPE_NONE,
	ZAP_SIGTYPE_ISDN,
	ZAP_SIGTYPE_RBS,
	ZAP_SIGTYPE_ANALOG
} zap_signal_type_t;

typedef enum {
	ZAP_SIGEVENT_START,
	ZAP_SIGEVENT_STOP,
	ZAP_SIGEVENT_TRANSFER,
	ZAP_SIGEVENT_ANSWER,
	ZAP_SIGEVENT_UP,
	ZAP_SIGEVENT_FLASH,
	ZAP_SIGEVENT_PROGRESS,
	ZAP_SIGEVENT_PROGRESS_MEDIA,
	ZAP_SIGEVENT_NOTIFY,
	ZAP_SIGEVENT_TONE_DETECTED,
	ZAP_SIGEVENT_ALARM_TRAP,
	ZAP_SIGEVENT_ALARM_CLEAR,
	ZAP_SIGEVENT_MISC,
	ZAP_SIGEVENT_INVALID
} zap_signal_event_t;
#define SIGNAL_STRINGS "START", "STOP", "TRANSFER", "ANSWER", "UP", "FLASH", "PROGRESS", \
		"PROGRESS_MEDIA", "NOTIFY", "TONE_DETECTED", "ALARM_TRAP", "ALARM_CLEAR", "MISC", "INVALID"
ZAP_STR2ENUM_P(zap_str2zap_signal_event, zap_signal_event2str, zap_signal_event_t)

typedef enum {
	ZAP_EVENT_NONE,
	ZAP_EVENT_DTMF,
	ZAP_EVENT_OOB,
	ZAP_EVENT_COUNT
} zap_event_type_t;

typedef enum {
	ZAP_TOP_DOWN,
	ZAP_BOTTOM_UP
} zap_direction_t;

typedef enum {
	ZAP_SUCCESS,
	ZAP_FAIL,
	ZAP_MEMERR,
	ZAP_TIMEOUT,
	ZAP_NOTIMPL,
	ZAP_CHECKSUM_ERROR,
	ZAP_STATUS_COUNT
} zap_status_t;

typedef enum {
	ZAP_NO_FLAGS = 0,
	ZAP_READ =  (1 << 0),
	ZAP_WRITE = (1 << 1),
	ZAP_EVENTS = (1 << 2)
} zap_wait_flag_t;

typedef enum {
	ZAP_CODEC_ULAW = 0,
	ZAP_CODEC_ALAW = 8,
	ZAP_CODEC_SLIN = 10,
	ZAP_CODEC_NONE = (1 << 31)
} zap_codec_t;

typedef enum {
	ZAP_TONE_DTMF = (1 << 0)
} zap_tone_type_t;

typedef enum {
	ZAP_COMMAND_NOOP,
	ZAP_COMMAND_SET_INTERVAL,
	ZAP_COMMAND_GET_INTERVAL,
	ZAP_COMMAND_SET_CODEC,
	ZAP_COMMAND_GET_CODEC,
	ZAP_COMMAND_SET_NATIVE_CODEC,
	ZAP_COMMAND_GET_NATIVE_CODEC,
	ZAP_COMMAND_ENABLE_DTMF_DETECT,
	ZAP_COMMAND_DISABLE_DTMF_DETECT,
	ZAP_COMMAND_SEND_DTMF,
	ZAP_COMMAND_SET_DTMF_ON_PERIOD,
	ZAP_COMMAND_GET_DTMF_ON_PERIOD,
	ZAP_COMMAND_SET_DTMF_OFF_PERIOD,
	ZAP_COMMAND_GET_DTMF_OFF_PERIOD,
	ZAP_COMMAND_GENERATE_RING_ON,
	ZAP_COMMAND_GENERATE_RING_OFF,
	ZAP_COMMAND_OFFHOOK,
	ZAP_COMMAND_ONHOOK,
	ZAP_COMMAND_ENABLE_PROGRESS_DETECT,
	ZAP_COMMAND_DISABLE_PROGRESS_DETECT,
	ZAP_COMMAND_TRACE_INPUT,
	ZAP_COMMAND_TRACE_OUTPUT,
	ZAP_COMMAND_ENABLE_CALLERID_DETECT,
	ZAP_COMMAND_DISABLE_CALLERID_DETECT,
	ZAP_COMMAND_COUNT
} zap_command_t;

typedef enum {
	ZAP_SPAN_CONFIGURED = (1 << 0),
	ZAP_SPAN_READY = (1 << 1),
	ZAP_SPAN_STATE_CHANGE = (1 << 2)
} zap_span_flag_t;

typedef enum {
	ZAP_CHAN_TYPE_B,
	ZAP_CHAN_TYPE_DQ921,
	ZAP_CHAN_TYPE_DQ931,
	ZAP_CHAN_TYPE_FXS,
	ZAP_CHAN_TYPE_FXO,
	ZAP_CHAN_TYPE_COUNT
} zap_chan_type_t;

typedef enum {
	ZAP_CHANNEL_FEATURE_DTMF = (1 << 0),
	ZAP_CHANNEL_FEATURE_CODECS = (1 << 1),
	ZAP_CHANNEL_FEATURE_INTERVAL = (1 << 2),
	ZAP_CHANNEL_FEATURE_CALLERID = (1 << 3)
} zap_channel_feature_t;

typedef enum {
	ZAP_CHANNEL_STATE_DOWN,
	ZAP_CHANNEL_STATE_HANGUP,
	ZAP_CHANNEL_STATE_HOLD,
	ZAP_CHANNEL_STATE_DIALTONE,
	ZAP_CHANNEL_STATE_COLLECT,
	ZAP_CHANNEL_STATE_RING,
	ZAP_CHANNEL_STATE_BUSY,
	ZAP_CHANNEL_STATE_ATTN,
	ZAP_CHANNEL_STATE_GENRING,
	ZAP_CHANNEL_STATE_DIALING,
	ZAP_CHANNEL_STATE_GET_CALLERID,
	ZAP_CHANNEL_STATE_CALLWAITING,
	ZAP_CHANNEL_STATE_TERMINATING,
	ZAP_CHANNEL_STATE_RESTART,
	ZAP_CHANNEL_STATE_PROGRESS_MEDIA,
	ZAP_CHANNEL_STATE_PROGRESS,
	ZAP_CHANNEL_STATE_IDLE,
	ZAP_CHANNEL_STATE_UP,
	ZAP_CHANNEL_STATE_INVALID
} zap_channel_state_t;
#define CHANNEL_STATE_STRINGS "DOWN", "HANGUP", "HOLD", "DIALTONE", "COLLECT", \
		"RING", "BUSY", "ATTN", "GENRING", "DIALING", "GET_CALLERID", "CALLWAITING", \
		"TERMINATING", "RESTART", "PROGRESS_MEDIA", "PROGRESS", "IDLE", "UP", "INVALID"
ZAP_STR2ENUM_P(zap_str2zap_channel_state, zap_channel_state2str, zap_channel_state_t)

typedef enum {
	ZAP_CHANNEL_CONFIGURED = (1 << 0),
	ZAP_CHANNEL_READY = (1 << 1),
	ZAP_CHANNEL_OPEN = (1 << 2),
	ZAP_CHANNEL_DTMF_DETECT = (1 << 3),
	ZAP_CHANNEL_SUPRESS_DTMF = (1 << 4),
	ZAP_CHANNEL_TRANSCODE = (1 << 5),
	ZAP_CHANNEL_BUFFER = (1 << 6),
	ZAP_CHANNEL_EVENT = (1 << 7),
	ZAP_CHANNEL_INTHREAD = (1 << 8),
	ZAP_CHANNEL_WINK = (1 << 9),
	ZAP_CHANNEL_FLASH = (1 << 10),
	ZAP_CHANNEL_STATE_CHANGE = (1 << 11),
	ZAP_CHANNEL_HOLD = (1 << 12),
	ZAP_CHANNEL_INUSE = (1 << 13),
	ZAP_CHANNEL_OFFHOOK = (1 << 14),
	ZAP_CHANNEL_RINGING = (1 << 15),
	ZAP_CHANNEL_PROGRESS_DETECT = (1 << 16),
	ZAP_CHANNEL_CALLERID_DETECT = (1 << 17),
	ZAP_CHANNEL_OUTBOUND = (1 << 18),
	ZAP_CHANNEL_SUSPENDED = (1 << 19)
} zap_channel_flag_t;

typedef struct zap_channel zap_channel_t;
typedef struct zap_event zap_event_t;
typedef struct zap_sigmsg zap_sigmsg_t;
typedef struct zap_span zap_span_t;

#define ZIO_CHANNEL_OUTGOING_CALL_ARGS (zap_channel_t *zchan)
#define ZIO_SPAN_POLL_EVENT_ARGS (zap_span_t *span, uint32_t ms)
#define ZIO_SPAN_NEXT_EVENT_ARGS (zap_span_t *span, zap_event_t **event)
#define ZIO_SIGNAL_CB_ARGS (zap_sigmsg_t *sigmsg)
#define ZIO_EVENT_CB_ARGS (zap_channel_t *zchan, zap_event_t *event)
#define ZIO_CODEC_ARGS (void *data, zap_size_t max, zap_size_t *datalen)
#define ZIO_CONFIGURE_SPAN_ARGS (zap_span_t *span, const char *str, zap_chan_type_t type, char *name, char *number)
#define ZIO_CONFIGURE_ARGS (const char *category, const char *var, const char *val, int lineno)
#define ZIO_OPEN_ARGS (zap_channel_t *zchan)
#define ZIO_CLOSE_ARGS (zap_channel_t *zchan)
#define ZIO_CHANNEL_DESTROY_ARGS (zap_channel_t *zchan)
#define ZIO_COMMAND_ARGS (zap_channel_t *zchan, zap_command_t command, void *obj)
#define ZIO_WAIT_ARGS (zap_channel_t *zchan, zap_wait_flag_t *flags, int32_t to)
#define ZIO_GET_ALARMS_ARGS (zap_channel_t *zchan)
#define ZIO_READ_ARGS (zap_channel_t *zchan, void *data, zap_size_t *datalen)
#define ZIO_WRITE_ARGS (zap_channel_t *zchan, void *data, zap_size_t *datalen)

typedef zap_status_t (*zio_channel_outgoing_call_t) ZIO_CHANNEL_OUTGOING_CALL_ARGS ;
typedef zap_status_t (*zio_span_poll_event_t) ZIO_SPAN_POLL_EVENT_ARGS ;
typedef zap_status_t (*zio_span_next_event_t) ZIO_SPAN_NEXT_EVENT_ARGS ;
typedef zap_status_t (*zio_signal_cb_t) ZIO_SIGNAL_CB_ARGS ;
typedef zap_status_t (*zio_event_cb_t) ZIO_EVENT_CB_ARGS ;
typedef zap_status_t (*zio_codec_t) ZIO_CODEC_ARGS ;
typedef zap_status_t (*zio_configure_span_t) ZIO_CONFIGURE_SPAN_ARGS ;
typedef zap_status_t (*zio_configure_t) ZIO_CONFIGURE_ARGS ;
typedef zap_status_t (*zio_open_t) ZIO_OPEN_ARGS ;
typedef zap_status_t (*zio_close_t) ZIO_CLOSE_ARGS ;
typedef zap_status_t (*zio_channel_destroy_t) ZIO_CHANNEL_DESTROY_ARGS ;
typedef zap_status_t (*zio_get_alarms_t) ZIO_GET_ALARMS_ARGS ;
typedef zap_status_t (*zio_command_t) ZIO_COMMAND_ARGS ;
typedef zap_status_t (*zio_wait_t) ZIO_WAIT_ARGS ;
typedef zap_status_t (*zio_read_t) ZIO_READ_ARGS ;
typedef zap_status_t (*zio_write_t) ZIO_WRITE_ARGS ;

#define ZIO_CHANNEL_OUTGOING_CALL_FUNCTION(name) zap_status_t name ZIO_CHANNEL_OUTGOING_CALL_ARGS
#define ZIO_SPAN_POLL_EVENT_FUNCTION(name) zap_status_t name ZIO_SPAN_POLL_EVENT_ARGS
#define ZIO_SPAN_NEXT_EVENT_FUNCTION(name) zap_status_t name ZIO_SPAN_NEXT_EVENT_ARGS
#define ZIO_SIGNAL_CB_FUNCTION(name) zap_status_t name ZIO_SIGNAL_CB_ARGS
#define ZIO_EVENT_CB_FUNCTION(name) zap_status_t name ZIO_EVENT_CB_ARGS
#define ZIO_CODEC_FUNCTION(name) zap_status_t name ZIO_CODEC_ARGS
#define ZIO_CONFIGURE_SPAN_FUNCTION(name) zap_status_t name ZIO_CONFIGURE_SPAN_ARGS
#define ZIO_CONFIGURE_FUNCTION(name) zap_status_t name ZIO_CONFIGURE_ARGS
#define ZIO_OPEN_FUNCTION(name) zap_status_t name ZIO_OPEN_ARGS
#define ZIO_CLOSE_FUNCTION(name) zap_status_t name ZIO_CLOSE_ARGS
#define ZIO_CHANNEL_DESTROY_FUNCTION(name) zap_status_t name ZIO_CHANNEL_DESTROY_ARGS
#define ZIO_GET_ALARMS_FUNCTION(name) zap_status_t name ZIO_GET_ALARMS_ARGS
#define ZIO_COMMAND_FUNCTION(name) zap_status_t name ZIO_COMMAND_ARGS
#define ZIO_WAIT_FUNCTION(name) zap_status_t name ZIO_WAIT_ARGS
#define ZIO_READ_FUNCTION(name) zap_status_t name ZIO_READ_ARGS
#define ZIO_WRITE_FUNCTION(name) zap_status_t name ZIO_WRITE_ARGS

#define ZAP_PRE __FILE__, __FUNCTION__, __LINE__
#define ZAP_LOG_LEVEL_DEBUG 7
#define ZAP_LOG_LEVEL_INFO 6
#define ZAP_LOG_LEVEL_NOTICE 5
#define ZAP_LOG_LEVEL_WARNING 4
#define ZAP_LOG_LEVEL_ERROR 3
#define ZAP_LOG_LEVEL_CRIT 2
#define ZAP_LOG_LEVEL_ALERT 1
#define ZAP_LOG_LEVEL_EMERG 0

#define ZAP_LOG_DEBUG ZAP_PRE, ZAP_LOG_LEVEL_DEBUG
#define ZAP_LOG_INFO ZAP_PRE, ZAP_LOG_LEVEL_INFO
#define ZAP_LOG_NOTICE ZAP_PRE, ZAP_LOG_LEVEL_NOTICE
#define ZAP_LOG_WARNING ZAP_PRE, ZAP_LOG_LEVEL_WARNING
#define ZAP_LOG_ERROR ZAP_PRE, ZAP_LOG_LEVEL_ERROR
#define ZAP_LOG_CRIT ZAP_PRE, ZAP_LOG_LEVEL_CRIT
#define ZAP_LOG_ALERT ZAP_PRE, ZAP_LOG_LEVEL_ALERT
#define ZAP_LOG_EMERG ZAP_PRE, ZAP_LOG_LEVEL_EMERG

typedef struct zap_fsk_data_state zap_fsk_data_state_t;
typedef int (*zap_fsk_data_decoder_t)(zap_fsk_data_state_t *state);
typedef zap_status_t (*zap_fsk_write_sample_t)(int16_t *buf, zap_size_t buflen, void *user_data);
typedef void (*zap_logger_t)(char *file, const char *func, int line, int level, char *fmt, ...);
typedef struct zap_io_interface zap_io_interface_t;
typedef struct hashtable zap_hash_t;
typedef struct hashtable_itr zap_hash_itr_t;
typedef struct key zap_hash_key_t;
typedef struct value zap_hash_val_t;
typedef struct zap_bitstream zap_bitstream_t;
typedef struct zap_fsk_modulator zap_fsk_modulator_t;

#endif

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

