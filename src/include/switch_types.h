/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
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
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthmct@yahoo.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthmct@yahoo.com>
 *
 * switch_types.h -- Data Types
 *
 */
/*! \file switch_types.h
    \brief Data Types
*/
#ifndef SWITCH_TYPES_H
#define SWITCH_TYPES_H

#ifdef __cplusplus
extern "C" {
#ifdef _FORMATBUG
}
#endif
#endif

#include <switch.h>
#include <switch_platform.h>

#ifdef WIN32
#define SWITCH_PATH_SEPARATOR "\\"
#else
#define SWITCH_PATH_SEPARATOR "/"
#endif

#ifndef SWITCH_PREFIX_DIR
#define SWITCH_PREFIX_DIR "."
#endif

#ifndef SWITCH_MOD_DIR
#define SWITCH_MOD_DIR SWITCH_PREFIX_DIR SWITCH_PATH_SEPARATOR "mod"
#endif

#ifndef SWITCH_CONF_DIR
#define SWITCH_CONF_DIR SWITCH_PREFIX_DIR SWITCH_PATH_SEPARATOR "conf"
#endif

#ifndef SWITCH_LOG_DIR
#define SWITCH_LOG_DIR SWITCH_PREFIX_DIR SWITCH_PATH_SEPARATOR "log"
#endif

#ifndef SWITCH_DB_DIR
#define SWITCH_DB_DIR SWITCH_PREFIX_DIR SWITCH_PATH_SEPARATOR "db"
#endif

#ifndef SWITCH_SCRIPT_DIR
#define SWITCH_SCRIPT_DIR SWITCH_PREFIX_DIR SWITCH_PATH_SEPARATOR "scripts"
#endif

struct switch_directories {
	char *base_dir;
	char *mod_dir;
	char *conf_dir;
	char *log_dir;
	char *db_dir;
	char *script_dir;
	char *temp_dir;
};

typedef struct switch_directories switch_directories;
SWITCH_DECLARE_DATA extern switch_directories SWITCH_GLOBAL_dirs;

#define SWITCH_THREAD_STACKSIZE 512 * 1024
#define SWITCH_RECCOMMENDED_BUFFER_SIZE 2048
#define SWITCH_MAX_CODECS 30
#define SWITCH_MAX_STATE_HANDLERS 30
#define SWITCH_TRUE 1
#define SWITCH_FALSE 0

/*!
  \enum switch_vad_flags
  \brief RTP Related Flags
<pre>
    SWITCH_VAD_FLAG_TALKING         - Currently Talking
    SWITCH_VAD_FLAG_EVENTS_TALK     - Fire events when talking is detected
	SWITCH_VAD_FLAG_EVENTS_NOTALK   - Fire events when not talking is detected
	SWITCH_VAD_FLAG_CNG				- Send CNG
</pre>
 */
typedef enum {
	SWITCH_VAD_FLAG_TALKING = ( 1 << 0 ),
	SWITCH_VAD_FLAG_EVENTS_TALK = ( 1 << 1 ),
	SWITCH_VAD_FLAG_EVENTS_NOTALK = ( 1 << 2 ),
	SWITCH_VAD_FLAG_CNG = ( 1 << 3)
} switch_vad_flag_t;


/*!
  \enum switch_rtp_flag_t
  \brief RTP Related Flags
<pre>
    SWITCH_RTP_FLAG_NOBLOCK       - Do not block
    SWITCH_RTP_FLAG_IO            - IO is ready
	SWITCH_RTP_FLAG_USE_TIMER     - Timeout Reads and replace with a CNG Frame
	SWITCH_RTP_FLAG_TIMER_RECLOCK - Resync the timer to the current clock on slips
	SWITCH_RTP_FLAG_SECURE        - Secure RTP
	SWITCH_RTP_FLAG_AUTOADJ       - Auto-Adjust the dest based on the source
	SWITCH_RTP_FLAG_RAW_WRITE     - Try to forward packets unscathed
	SWITCH_RTP_FLAG_GOOGLEHACK    - Convert payload from 102 to 97
	SWITCH_RTP_FLAG_VAD           - Enable VAD
</pre>
 */
typedef enum {
	SWITCH_RTP_FLAG_NOBLOCK = ( 1 << 0),
	SWITCH_RTP_FLAG_IO = (1 << 1),
	SWITCH_RTP_FLAG_USE_TIMER = (1 << 2),
	SWITCH_RTP_FLAG_TIMER_RECLOCK = (1 << 3),
	SWITCH_RTP_FLAG_SECURE = (1 << 4),
	SWITCH_RTP_FLAG_AUTOADJ = (1 << 5),
	SWITCH_RTP_FLAG_RAW_WRITE = (1 << 6),
	SWITCH_RTP_FLAG_GOOGLEHACK = (1 << 7),
	SWITCH_RTP_FLAG_VAD = (1 << 8)
} switch_rtp_flag_t;

/*!
  \enum switch_priority_t
  \brief Priority Indication
<pre>
    SWITCH_PRIORITY_NORMAL  - Normal Priority
    SWITCH_PRIORITY_LOW     - Low Priority
    SWITCH_PRIORITY_HIGH    - High Priority
</pre>
 */
typedef enum {
	SWITCH_PRIORITY_NORMAL,
	SWITCH_PRIORITY_LOW,
	SWITCH_PRIORITY_HIGH,
} switch_priority_t;

/*!
  \enum switch_ivr_option_t
  \brief Possible options related to ivr functions
<pre>
    SWITCH_IVR_OPTION_NONE  - nothing whatsoever
    SWITCH_IVR_OPTION_ASYNC - Asynchronous (do things in the background when applicable)
	SWITCH_IVR_OPTION_FILE  - string argument implies a filename
</pre>
 */
typedef enum {
	SWITCH_IVR_OPTION_NONE = 0,
	SWITCH_IVR_OPTION_ASYNC = (1 << 0),
	SWITCH_IVR_OPTION_FILE = (1 << 1)
} switch_ivr_option_t;
	
/*!
  \enum switch_core_session_message_t
  \brief Possible types of messages for inter-session communication
<pre>
	SWITCH_MESSAGE_REDIRECT_AUDIO     - Indication to redirect audio to another location if possible
	SWITCH_MESSAGE_TRANSMIT_TEXT      - A text message
	SWITCH_MESSAGE_INDICATE_PROGRESS  - indicate progress 
	SWITCH_MESSAGE_INDICATE_BRIDGE    - indicate a bridge starting
	SWITCH_MESSAGE_INDICATE_UNBRIDGE    - indicate a bridge ending
</pre>
 */
typedef enum {
	SWITCH_MESSAGE_REDIRECT_AUDIO,
	SWITCH_MESSAGE_TRANSMIT_TEXT,
	SWITCH_MESSAGE_INDICATE_PROGRESS,
	SWITCH_MESSAGE_INDICATE_BRIDGE,
	SWITCH_MESSAGE_INDICATE_UNBRIDGE
} switch_core_session_message_t;


/*!
  \enum switch_stack_t
  \brief Expression of how to stack a list
<pre>
SWITCH_STACK_BOTTOM - Stack on the bottom
SWITCH_STACK_TOP	- Stack on the top
</pre>
 */
typedef enum {
	SWITCH_STACK_BOTTOM,
	SWITCH_STACK_TOP
} switch_stack_t;

/*!
  \enum switch_status
  \brief Common return values
<pre>
    SWITCH_STATUS_SUCCESS	- General Success (common return value for most functions)
    SWITCH_STATUS_FALSE		- General Falsehood
    SWITCH_STATUS_TIMEOUT	- A Timeout has occured
    SWITCH_STATUS_RESTART	- An indication to restart the previous operation
    SWITCH_STATUS_TERM		- An indication to terminate
    SWITCH_STATUS_NOTIMPL	- An indication that requested resource is not impelemented
    SWITCH_STATUS_MEMERR	- General memory error
    SWITCH_STATUS_NOOP		- NOTHING
    SWITCH_STATUS_RESAMPLE	- An indication that a resample has occured
    SWITCH_STATUS_GENERR	- A general Error
    SWITCH_STATUS_INUSE		- An indication that requested resource is in use
	SWITCH_STATUS_BREAK     - A non-fatal break of an operation
    SWITCH_STATUS_SOCKERR  - A socket error
</pre>
 */
typedef enum {
	SWITCH_STATUS_SUCCESS,
	SWITCH_STATUS_FALSE,
	SWITCH_STATUS_TIMEOUT,
	SWITCH_STATUS_RESTART,
	SWITCH_STATUS_TERM,
	SWITCH_STATUS_NOTIMPL,
	SWITCH_STATUS_MEMERR,
	SWITCH_STATUS_NOOP,
	SWITCH_STATUS_RESAMPLE,
	SWITCH_STATUS_GENERR,
	SWITCH_STATUS_INUSE,
	SWITCH_STATUS_BREAK,
	SWITCH_STATUS_SOCKERR
} switch_status;



/*!
\enum switch_log_level
\brief Log Level Enumeration
<pre>
    SWITCH_LOG_CONSOLE          - Console
	SWITCH_LOG_DEBUG            - Debug
	SWITCH_LOG_INFO             - Info
	SWITCH_LOG_NOTICE           - Notice
	SWITCH_LOG_WARNING          - Warning
	SWITCH_LOG_ERROR            - Error
	SWITCH_LOG_CRIT             - Critical
	SWITCH_LOG_ALERT            - Alert
	SWITCH_LOG_EMERG            - Emergency
</pre>
 */
typedef enum {
	SWITCH_LOG_CONSOLE = 8,
	SWITCH_LOG_DEBUG = 7,
	SWITCH_LOG_INFO  = 6,
	SWITCH_LOG_NOTICE = 5,
	SWITCH_LOG_WARNING = 4,
	SWITCH_LOG_ERROR = 3,
	SWITCH_LOG_CRIT = 2,
	SWITCH_LOG_ALERT = 1,
	SWITCH_LOG_EMERG = 0
} switch_log_level;


/*!
\enum switch_text_channel
\brief A target to write log/debug info to
<pre>
SWITCH_CHANNEL_ID_LOG			- Write to the currently defined log
SWITCH_CHANNEL_ID_LOG_CLEAN		- Write to the currently defined log with no extra file/line/date information
SWITCH_CHANNEL_ID_EVENT			- Write to the event engine as a LOG event
</pre>
 */
typedef enum {
	SWITCH_CHANNEL_ID_LOG,
	SWITCH_CHANNEL_ID_LOG_CLEAN,
	SWITCH_CHANNEL_ID_EVENT
} switch_text_channel;

#define SWITCH_UUID_FORMATTED_LENGTH APR_UUID_FORMATTED_LENGTH 	
#define SWITCH_CHANNEL_LOG SWITCH_CHANNEL_ID_LOG, __FILE__, __FUNCTION__, __LINE__
#define SWITCH_CHANNEL_LOG_CLEAN SWITCH_CHANNEL_ID_LOG_CLEAN, __FILE__, __FUNCTION__, __LINE__
#define SWITCH_CHANNEL_EVENT SWITCH_CHANNEL_ID_EVENT, __FILE__, __FUNCTION__, __LINE__

/*!
  \enum switch_channel_state
  \brief Channel States
<pre>
CS_NEW       - Channel is newly created 
CS_INIT      - Channel has been initilized
CS_RING      - Channel is looking for a dialplan
CS_TRANSMIT  - Channel is in a passive transmit state
CS_EXECUTE   - Channel is executing it's dialplan 
CS_LOOPBACK  - Channel is in loopback
CS_HOLD		 - Channel is on hold
CS_HANGUP    - Channel is flagged for hangup and ready to end
CS_DONE      - Channel is ready to be destroyed and out of the state machine
</pre>
 */
typedef enum {
	CS_NEW,
	CS_INIT,
	CS_RING,
	CS_TRANSMIT,
	CS_EXECUTE,
	CS_LOOPBACK,
	CS_HOLD,
	CS_HANGUP,
	CS_DONE 
} switch_channel_state;


/*!
  \enum switch_channel_flag
  \brief Channel Flags

<pre>
CF_ANSWERED     = (1 <<  0) - Channel is answered
CF_OUTBOUND     = (1 <<  1) - Channel is an outbound channel
CF_EARLY_MEDIA  = (1 <<  2) - Channel is ready for audio before answer 
CF_ORIGINATOR	= (1 <<  3) - Channel is an originator
CF_TRANSFER		= (1 <<  4) - Channel is being transfered
CF_ACCEPT_CNG	= (1 <<  5) - Channel will accept CNG frames
CF_LOCK_THREAD	= (1 <<  6) - Prevent the channel thread from exiting while this flag is set
CF_BRIDGED		= (1 <<  7) - Channel in a bridge
CF_HOLD			= (1 <<  8) - Channel is on hold
</pre>
 */

typedef enum {
	CF_ANSWERED   	= (1 <<  0),
	CF_OUTBOUND   	= (1 <<  1),
	CF_EARLY_MEDIA	= (1 <<  2),
	CF_ORIGINATOR	= (1 <<  3),
	CF_TRANSFER		= (1 <<  4),
	CF_ACCEPT_CNG	= (1 <<  5),
	CF_LOCK_THREAD	= (1 <<  6),
	CF_BRIDGED		= (1 <<  7),
	CF_HOLD			= (1 <<  8)
} switch_channel_flag;


/*!
  \enum switch_frame_flag
  \brief Frame Flags

<pre>
SFF_CNG       = (1 <<  0) - Frame represents comfort noise
SFF_RAW_RTP   = (1 <<  1) - Frame has raw rtp accessible
</pre>
 */
typedef enum {
	SFF_CNG = (1 << 0),
	SFF_RAW_RTP = (1 << 1)
} switch_frame_flag;


/*!
  \enum switch_signal
  \brief Signals to send to channels
<pre>
SWITCH_SIG_KILL - Kill the channel
SWITCH_SIG_XFER - Stop the current io but leave it viable
</pre>
 */

typedef enum {
	SWITCH_SIG_NONE,
	SWITCH_SIG_KILL,
	SWITCH_SIG_XFER
} switch_signal;

/*!
  \enum switch_codec_flag
  \brief Codec related flags
<pre>
SWITCH_CODEC_FLAG_ENCODE =			(1 <<  0) - Codec can encode
SWITCH_CODEC_FLAG_DECODE =			(1 <<  1) - Codec can decode
SWITCH_CODEC_FLAG_SILENCE_START =	(1 <<  2) - Start period of silence
SWITCH_CODEC_FLAG_SILENCE_STOP =	(1 <<  3) - End period of silence
SWITCH_CODEC_FLAG_SILENCE =			(1 <<  4) - Silence
SWITCH_CODEC_FLAG_FREE_POOL =		(1 <<  5) - Free codec's pool on destruction
</pre>
*/
typedef enum {
	SWITCH_CODEC_FLAG_ENCODE =			(1 <<  0),
	SWITCH_CODEC_FLAG_DECODE =			(1 <<  1),
	SWITCH_CODEC_FLAG_SILENCE_START =	(1 <<  2),
	SWITCH_CODEC_FLAG_SILENCE_STOP =	(1 <<  3),
	SWITCH_CODEC_FLAG_SILENCE =			(1 <<  4),
	SWITCH_CODEC_FLAG_FREE_POOL =		(1 <<  5),

} switch_codec_flag;


/*!
  \enum switch_speech_flag
  \brief Speech related flags
<pre>
SWITCH_SPEECH_FLAG_TTS =			(1 <<  0) - Interface can/should convert text to speech.
SWITCH_SPEECH_FLAG_ASR =			(1 <<  1) - Interface can/should convert audio to text.
SWITCH_SPEECH_FLAG_HASTEXT =		(1 <<  2) - Interface is has text to read.
SWITCH_SPEECH_FLAG_PEEK =			(1 <<  3) - Read data but do not erase it.
SWITCH_SPEECH_FLAG_FREE_POOL =		(1 <<  4) - Free interface's pool on destruction.
SWITCH_SPEECH_FLAG_BLOCKING =       (1 <<  5) - Indicate that a blocking call is desired 
</pre>
*/
typedef enum {
	SWITCH_SPEECH_FLAG_TTS =			(1 <<  0),
	SWITCH_SPEECH_FLAG_ASR =			(1 <<  1),
	SWITCH_SPEECH_FLAG_HASTEXT =		(1 <<  2),
	SWITCH_SPEECH_FLAG_PEEK =			(1 <<  3),
	SWITCH_SPEECH_FLAG_FREE_POOL =		(1 <<  4),
	SWITCH_SPEECH_FLAG_BLOCKING =		(1 <<  5),

} switch_speech_flag;


/*!
  \enum switch_directory_flag
  \brief Directory Handle related flags
<pre>
SWITCH_DIRECTORY_FLAG_FREE_POOL =		(1 <<  0) - Free interface's pool on destruction.
</pre>
*/
typedef enum {
	SWITCH_DIRECTORY_FLAG_FREE_POOL =		(1 <<  0),

} switch_directory_flag;

/*!
  \enum switch_codec_type
  \brief Codec types
<pre>
SWITCH_CODEC_TYPE_AUDIO - Audio Codec
SWITCH_CODEC_TYPE_VIDEO - Video Codec
SWITCH_CODEC_TYPE_T38   - T38 Codec
SWITCH_CODEC_TYPE_APP   - Application Codec
</pre>
 */
typedef enum {
	SWITCH_CODEC_TYPE_AUDIO,
	SWITCH_CODEC_TYPE_VIDEO,
	SWITCH_CODEC_TYPE_T38,
	SWITCH_CODEC_TYPE_APP
} switch_codec_type;


/*!
  \enum switch_timer_flag
  \brief Timer related flags
<pre>
SWITCH_TIMER_FLAG_FREE_POOL =		(1 <<  0) - Free timer's pool on destruction
</pre>
*/
typedef enum {
		SWITCH_TIMER_FLAG_FREE_POOL =		(1 <<  0),
} switch_timer_flag;

/*!
  \enum switch_file_flag
  \brief File flags
<pre>
SWITCH_FILE_FLAG_READ =         (1 <<  0) - Open for read
SWITCH_FILE_FLAG_WRITE =        (1 <<  1) - Open for write
SWITCH_FILE_FLAG_FREE_POOL =    (1 <<  2) - Free file handle's pool on destruction
SWITCH_FILE_DATA_SHORT =        (1 <<  3) - Read data in shorts
SWITCH_FILE_DATA_INT =          (1 <<  4) - Read data in ints
SWITCH_FILE_DATA_FLOAT =        (1 <<  5) - Read data in floats
SWITCH_FILE_DATA_DOUBLE =       (1 <<  6) - Read data in doubles
SWITCH_FILE_DATA_RAW =          (1 <<  7) - Read data as is
SWITCH_FILE_PAUSE =             (1 <<  8) - Pause
</pre>
 */
typedef enum {
	SWITCH_FILE_FLAG_READ =			(1 <<  0),
	SWITCH_FILE_FLAG_WRITE =		(1 <<  1),
	SWITCH_FILE_FLAG_FREE_POOL =	(1 <<  2),
	SWITCH_FILE_DATA_SHORT =		(1 <<  3),
	SWITCH_FILE_DATA_INT =			(1 <<  4),
	SWITCH_FILE_DATA_FLOAT =		(1 <<  5),
	SWITCH_FILE_DATA_DOUBLE =		(1 <<  6),
	SWITCH_FILE_DATA_RAW =			(1 <<  7),
	SWITCH_FILE_PAUSE =				(1 <<  8)
} switch_file_flag;

typedef enum {
	SWITCH_IO_FLAG_NOOP = 0,
} switch_io_flag;

/* make sure this is synced with the EVENT_NAMES array in switch_event.c
   also never put any new ones before EVENT_ALL
*/
/*!
  \enum switch_event_t
  \brief Built-in Events

<pre>
    SWITCH_EVENT_CUSTOM				- A custom event
    SWITCH_EVENT_CHANNEL_CREATE		- A channel has been created
    SWITCH_EVENT_CHANNEL_DESTROY	- A channel has been destroyed
    SWITCH_EVENT_CHANNEL_STATE		- A channel has changed state
    SWITCH_EVENT_CHANNEL_ANSWER		- A channel has been answered
    SWITCH_EVENT_CHANNEL_HANGUP		- A channel has been hungup
    SWITCH_EVENT_CHANNEL_EXECUTE	- A channel has executed a module's application
	SWITCH_EVENT_CHANNEL_BRIDGE     - A channel has bridged to another channel
	SWITCH_EVENT_CHANNEL_UNBRIDGE   - A channel has unbridged from another channel
    SWITCH_EVENT_API				- An API call has been executed
    SWITCH_EVENT_LOG				- A LOG event has been triggered
    SWITCH_EVENT_INBOUND_CHAN		- A new inbound channel has been created
    SWITCH_EVENT_OUTBOUND_CHAN		- A new outbound channel has been created
    SWITCH_EVENT_STARTUP			- The system has been started
    SWITCH_EVENT_SHUTDOWN			- The system has been shutdown
	SWITCH_EVENT_PUBLISH			- Publish
	SWITCH_EVENT_UNPUBLISH			- UnPublish
	SWITCH_EVENT_TALK				- Talking Detected
	SWITCH_EVENT_NOTALK				- Not Talking Detected
	SWITCH_EVENT_SESSION_CRASH		- Session Crashed
    SWITCH_EVENT_ALL				- All events at once
</pre>

 */
typedef enum {
	SWITCH_EVENT_CUSTOM,
	SWITCH_EVENT_CHANNEL_CREATE,
	SWITCH_EVENT_CHANNEL_DESTROY,
	SWITCH_EVENT_CHANNEL_STATE,
	SWITCH_EVENT_CHANNEL_ANSWER,
	SWITCH_EVENT_CHANNEL_HANGUP,
	SWITCH_EVENT_CHANNEL_EXECUTE,
	SWITCH_EVENT_CHANNEL_BRIDGE,
	SWITCH_EVENT_CHANNEL_UNBRIDGE,
	SWITCH_EVENT_API,
	SWITCH_EVENT_LOG,
	SWITCH_EVENT_INBOUND_CHAN,
	SWITCH_EVENT_OUTBOUND_CHAN,
	SWITCH_EVENT_STARTUP,
	SWITCH_EVENT_SHUTDOWN,
	SWITCH_EVENT_PUBLISH,
	SWITCH_EVENT_UNPUBLISH,
	SWITCH_EVENT_TALK,
	SWITCH_EVENT_NOTALK,
	SWITCH_EVENT_SESSION_CRASH,
	SWITCH_EVENT_ALL
} switch_event_t;


typedef enum {
	SWITCH_CAUSE_UNALLOCATED = 1,
	SWITCH_CAUSE_NO_ROUTE_TRANSIT_NET = 2,
	SWITCH_CAUSE_NO_ROUTE_DESTINATION = 3,
	SWITCH_CAUSE_CHANNEL_UNACCEPTABLE = 6,
	SWITCH_CAUSE_CALL_AWARDED_DELIVERED = 7,
	SWITCH_CAUSE_NORMAL_CLEARING = 16,
	SWITCH_CAUSE_USER_BUSY = 17,
	SWITCH_CAUSE_NO_USER_RESPONSE = 18,
	SWITCH_CAUSE_NO_ANSWER = 19,
	SWITCH_CAUSE_CALL_REJECTED = 21,
	SWITCH_CAUSE_NUMBER_CHANGED = 22,
	SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER = 27,
	SWITCH_CAUSE_INVALID_NUMBER_FORMAT = 28,
	SWITCH_CAUSE_FACILITY_REJECTED = 29,
	SWITCH_CAUSE_RESPONSE_TO_STATUS_ENQUIRY = 30,
	SWITCH_CAUSE_NORMAL_UNSPECIFIED = 31,
	SWITCH_CAUSE_NORMAL_CIRCUIT_CONGESTION = 34,
	SWITCH_CAUSE_NETWORK_OUT_OF_ORDER = 38,
	SWITCH_CAUSE_NORMAL_TEMPORARY_FAILURE = 41,
	SWITCH_CAUSE_SWITCH_CONGESTION = 42,
	SWITCH_CAUSE_ACCESS_INFO_DISCARDED = 43,
	SWITCH_CAUSE_REQUESTED_CHAN_UNAVAIL = 44,
	SWITCH_CAUSE_PRE_EMPTED = 45,
	SWITCH_CAUSE_FACILITY_NOT_SUBSCRIBED = 50,
	SWITCH_CAUSE_OUTGOING_CALL_BARRED = 52,
	SWITCH_CAUSE_INCOMING_CALL_BARRED = 54,
	SWITCH_CAUSE_BEARERCAPABILITY_NOTAUTH = 57,
	SWITCH_CAUSE_BEARERCAPABILITY_NOTAVAIL = 58,
	SWITCH_CAUSE_BEARERCAPABILITY_NOTIMPL = 65,
	SWITCH_CAUSE_CHAN_NOT_IMPLEMENTED = 66,
	SWITCH_CAUSE_FACILITY_NOT_IMPLEMENTED = 69,
	SWITCH_CAUSE_INVALID_CALL_REFERENCE = 81,
	SWITCH_CAUSE_INCOMPATIBLE_DESTINATION = 88,
	SWITCH_CAUSE_INVALID_MSG_UNSPECIFIED = 95,
	SWITCH_CAUSE_MANDATORY_IE_MISSING = 96,
	SWITCH_CAUSE_MESSAGE_TYPE_NONEXIST = 97,
	SWITCH_CAUSE_WRONG_MESSAGE = 98,
	SWITCH_CAUSE_IE_NONEXIST = 99,
	SWITCH_CAUSE_INVALID_IE_CONTENTS = 100,
	SWITCH_CAUSE_WRONG_CALL_STATE = 101,
	SWITCH_CAUSE_RECOVERY_ON_TIMER_EXPIRE = 102,
	SWITCH_CAUSE_MANDATORY_IE_LENGTH_ERROR = 103,
	SWITCH_CAUSE_PROTOCOL_ERROR = 111,
	SWITCH_CAUSE_INTERWORKING = 127,
	SWITCH_CAUSE_CRASH = 500
} switch_call_cause_t;


typedef uint8_t switch_payload_t;
typedef struct switch_rtp switch_rtp;
typedef struct switch_core_session_message switch_core_session_message;
typedef struct switch_audio_resampler switch_audio_resampler;
typedef struct switch_event_header switch_event_header;
typedef struct switch_event switch_event;
typedef struct switch_event_subclass switch_event_subclass;
typedef struct switch_event_node switch_event_node;
typedef struct switch_loadable_module switch_loadable_module;
typedef struct switch_frame switch_frame;
typedef struct switch_channel switch_channel;
typedef struct switch_endpoint_interface switch_endpoint_interface;
typedef struct switch_timer_interface switch_timer_interface;
typedef struct switch_dialplan_interface switch_dialplan_interface;
typedef struct switch_codec_interface switch_codec_interface;
typedef struct switch_application_interface switch_application_interface;
typedef struct switch_api_interface switch_api_interface;
typedef struct switch_file_interface switch_file_interface;
typedef struct switch_file_handle switch_file_handle;
typedef struct switch_core_session switch_core_session;
typedef struct switch_loadable_module_interface switch_loadable_module_interface;
typedef struct switch_caller_profile switch_caller_profile;
typedef struct switch_caller_step switch_caller_step;
typedef struct switch_caller_extension switch_caller_extension;
typedef struct switch_caller_application switch_caller_application;
typedef struct switch_state_handler_table switch_state_handler_table;
typedef struct switch_timer switch_timer;
typedef struct switch_codec switch_codec;
typedef struct switch_core_thread_session switch_core_thread_session;
typedef struct switch_codec_implementation switch_codec_implementation;
typedef struct switch_io_event_hook_outgoing_channel switch_io_event_hook_outgoing_channel;
typedef struct switch_io_event_hook_answer_channel switch_io_event_hook_answer_channel;
typedef struct switch_io_event_hook_receive_message switch_io_event_hook_receive_message;
typedef struct switch_io_event_hook_queue_event switch_io_event_hook_queue_event;
typedef struct switch_io_event_hook_read_frame switch_io_event_hook_read_frame;
typedef struct switch_io_event_hook_write_frame switch_io_event_hook_write_frame;
typedef struct switch_io_event_hook_kill_channel switch_io_event_hook_kill_channel;
typedef struct switch_io_event_hook_waitfor_read switch_io_event_hook_waitfor_read;
typedef struct switch_io_event_hook_waitfor_write switch_io_event_hook_waitfor_write;
typedef struct switch_io_event_hook_send_dtmf switch_io_event_hook_send_dtmf;
typedef struct switch_io_routines switch_io_routines;
typedef struct switch_io_event_hooks switch_io_event_hooks;
typedef struct switch_buffer switch_buffer;
typedef struct switch_codec_settings switch_codec_settings;
typedef struct switch_config switch_config;
typedef struct switch_speech_handle switch_speech_handle;
typedef struct switch_speech_interface switch_speech_interface;
typedef struct switch_directory_handle switch_directory_handle;
typedef struct switch_directory_interface switch_directory_interface;
typedef void (*switch_application_function)(switch_core_session *, char *);
typedef void (*switch_event_callback_t)(switch_event *);
typedef switch_caller_extension *(*switch_dialplan_hunt_function)(switch_core_session *);
typedef switch_status (*switch_state_handler)(switch_core_session *);
typedef switch_status (*switch_outgoing_channel_hook)(switch_core_session *, switch_caller_profile *, switch_core_session *);
typedef switch_status (*switch_answer_channel_hook)(switch_core_session *);
typedef switch_status (*switch_receive_message_hook)(switch_core_session *, switch_core_session_message *);
typedef switch_status (*switch_queue_event_hook)(switch_core_session *, switch_event *);
typedef switch_status (*switch_read_frame_hook)(switch_core_session *, switch_frame **, int, switch_io_flag, int);
typedef switch_status (*switch_write_frame_hook)(switch_core_session *, switch_frame *, int, switch_io_flag, int);
typedef switch_status (*switch_kill_channel_hook)(switch_core_session *, int);
typedef switch_status (*switch_waitfor_read_hook)(switch_core_session *, int, int);
typedef switch_status (*switch_waitfor_write_hook)(switch_core_session *, int, int);
typedef switch_status (*switch_send_dtmf_hook)(switch_core_session *, char *);
typedef switch_status (*switch_api_function)(char *in, char *out, switch_size_t outlen);
typedef switch_status (*switch_dtmf_callback_function)(switch_core_session *session, char *dtmf, void *buf, unsigned int buflen);
typedef int (*switch_core_db_callback_func)(void *pArg, int argc, char **argv, char **columnNames);
typedef switch_status (*switch_module_load_t) (switch_loadable_module_interface **, char *);
typedef switch_status (*switch_module_reload_t) (void);
typedef switch_status (*switch_module_pause_t) (void);
typedef switch_status (*switch_module_resume_t) (void);
typedef switch_status (*switch_module_status_t) (void);
typedef switch_status (*switch_module_runtime_t) (void);
typedef switch_status (*switch_module_shutdown_t) (void);


/* things we don't deserve to know about */

/*! \brief A channel */
struct switch_channel;
/*! \brief A core session representing a call and all of it's resources */
struct switch_core_session;


#ifdef __cplusplus
}
#endif

#endif
