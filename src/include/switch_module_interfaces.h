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
 *
 * switch_module_interfaces.h -- Module Interface Definitions
 *
 */
/*! \file switch_module_interfaces.h
    \brief Module Interface Definitions

	This module holds the definition of data abstractions used to implement various pluggable 
	interfaces and pluggable event handlers.

*/
#ifndef SWITCH_MODULE_INTERFACES_H
#define SWITCH_MODULE_INTERFACES_H

#include <switch.h>
#include "switch_resample.h"

SWITCH_BEGIN_EXTERN_C
/*! \brief A table of functions to execute at various states 
*/

typedef enum {
	SWITCH_SHN_ON_INIT,
	SWITCH_SHN_ON_RING,
	SWITCH_SHN_ON_EXECUTE,
	SWITCH_SHN_ON_HANGUP,
	SWITCH_SHN_ON_LOOPBACK,
	SWITCH_SHN_ON_TRANSMIT,
	SWITCH_SHN_ON_HOLD,
	SWITCH_SHN_ON_HIBERNATE,
	SWITCH_SHN_ON_RESET,
	SWITCH_SHN_ON_PARK
} switch_state_handler_name_t;

struct switch_state_handler_table {
	/*! executed when the state changes to init */
	switch_state_handler_t on_init;
	/*! executed when the state changes to routing */
	switch_state_handler_t on_routing;
	/*! executed when the state changes to execute */
	switch_state_handler_t on_execute;
	/*! executed when the state changes to hangup */
	switch_state_handler_t on_hangup;
	/*! executed when the state changes to exchange_media */
	switch_state_handler_t on_exchange_media;
	/*! executed when the state changes to soft_execute */
	switch_state_handler_t on_soft_execute;
	/*! executed when the state changes to consume_media */
	switch_state_handler_t on_consume_media;
	/*! executed when the state changes to hibernate */
	switch_state_handler_t on_hibernate;
	/*! executed when the state changes to reset */
	switch_state_handler_t on_reset;
	/*! executed when the state changes to park */
	switch_state_handler_t on_park;
	void *padding[10];
};

struct switch_stream_handle {
	switch_stream_handle_write_function_t write_function;
	switch_stream_handle_raw_write_function_t raw_write_function;
	void *data;
	void *end;
	switch_size_t data_size;
	switch_size_t data_len;
	switch_size_t alloc_len;
	switch_size_t alloc_chunk;
	switch_event_t *event;
};

struct switch_io_event_hooks;



typedef switch_call_cause_t (*switch_io_outgoing_channel_t)
(switch_core_session_t *, switch_caller_profile_t *, switch_core_session_t **, switch_memory_pool_t **, switch_originate_flag_t);
typedef switch_status_t (*switch_io_read_frame_t) (switch_core_session_t *, switch_frame_t **, switch_io_flag_t, int);
typedef switch_status_t (*switch_io_write_frame_t) (switch_core_session_t *, switch_frame_t *, switch_io_flag_t, int);
typedef switch_status_t (*switch_io_kill_channel_t) (switch_core_session_t *, int);
typedef switch_status_t (*switch_io_send_dtmf_t) (switch_core_session_t *, const switch_dtmf_t *);
typedef switch_status_t (*switch_io_receive_message_t) (switch_core_session_t *, switch_core_session_message_t *);
typedef switch_status_t (*switch_io_receive_event_t) (switch_core_session_t *, switch_event_t *);
typedef switch_status_t (*switch_io_state_change_t) (switch_core_session_t *);
typedef	switch_status_t (*switch_io_read_video_frame_t) (switch_core_session_t *, switch_frame_t **, switch_io_flag_t, int);
typedef switch_status_t (*switch_io_write_video_frame_t) (switch_core_session_t *, switch_frame_t *, switch_io_flag_t, int);
typedef switch_call_cause_t (*switch_io_resurrect_session_t)(switch_core_session_t **, switch_memory_pool_t **, void *);

typedef enum {
	SWITCH_IO_OUTGOING_CHANNEL,
	SWITCH_IO_READ_FRAME,
	SWITCH_IO_WRITE_FRAME,
	SWITCH_IO_KILL_CHANNEL,
	SWITCH_IO_SEND_DTMF,
	SWITCH_IO_RECEIVE_MESSAGE,
	SWITCH_IO_RECEIVE_EVENT,
	SWITCH_IO_STATE_CHANGE,
	SWITCH_IO_READ_VIDEO_FRAME,
	SWITCH_IO_WRITE_VIDEO_FRAME,
	SWITCH_IO_RESURRECT_SESSION
} switch_io_routine_name_t;

/*! \brief A table of i/o routines that an endpoint interface can implement */
struct switch_io_routines {
	/*! creates an outgoing session from given session, caller profile */
	switch_io_outgoing_channel_t outgoing_channel;
	/*! read a frame from a session */
	switch_io_read_frame_t read_frame;
	/*! write a frame to a session */
	switch_io_write_frame_t write_frame;
	/*! send a kill signal to the session's channel */
	switch_io_kill_channel_t kill_channel;
	/*! send a string of DTMF digits to a session's channel */
	switch_io_send_dtmf_t send_dtmf;
	/*! receive a message from another session */
	switch_io_receive_message_t receive_message;
	/*! queue a message for another session */
	switch_io_receive_event_t receive_event;
	/*! change a sessions channel state */
	switch_io_state_change_t state_change;
	/*! read a video frame from a session */
	switch_io_read_video_frame_t read_video_frame;
	/*! write a video frame to a session */
	switch_io_write_video_frame_t write_video_frame;
	/*! resurrect a session */
	switch_io_resurrect_session_t resurrect_session;
	void *padding[10];
};

/*! \brief Abstraction of an module endpoint interface
  This is the glue between the abstract idea of a "channel" and what is really going on under the
  hood.	 Each endpoint module fills out one of these tables and makes it available when a channel
  is created of it's paticular type.
*/

struct switch_endpoint_interface {
	/*! the interface's name */
	const char *interface_name;

	/*! channel abstraction methods */
	switch_io_routines_t *io_routines;

	/*! state machine methods */
	switch_state_handler_table_t *state_handler;

	/*! private information */
	void *private_info;

	/* to facilitate linking */
	struct switch_endpoint_interface *next;
};

/*! \brief Abstract handler to a timer module */
struct switch_timer {
	/*! time interval expressed in milliseconds */
	int interval;
	/*! flags to control behaviour */
	uint32_t flags;
	/*! sample count to increment by on each cycle */
	unsigned int samples;
	/*! current sample count based on samples parameter */
	uint32_t samplecount;
	/*! the timer interface provided from a loadable module */
	switch_timer_interface_t *timer_interface;
	/*! the timer's memory pool */
	switch_memory_pool_t *memory_pool;
	/*! private data for loadable modules to store information */
	void *private_info;
	/*! remaining time from last call to _check()*/
	switch_size_t diff;
	switch_size_t tick;
};

typedef enum {
	SWITCH_TIMER_FUNC_TIMER_INIT,
	SWITCH_TIMER_FUNC_TIMER_NEXT,
	SWITCH_TIMER_FUNC_TIMER_STEP,
	SWITCH_TIMER_FUNC_TIMER_SYNC,
	SWITCH_TIMER_FUNC_TIMER_CHECK,
	SWITCH_TIMER_FUNC_TIMER_DESTROY
} switch_timer_func_name_t;

/*! \brief A table of functions that a timer module implements */
struct switch_timer_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function to allocate the timer */
	switch_status_t (*timer_init) (switch_timer_t *);
	/*! function to wait for one cycle to pass */
	switch_status_t (*timer_next) (switch_timer_t *);
	/*! function to step the timer one step */
	switch_status_t (*timer_step) (switch_timer_t *);
	/*! function to reset the timer  */
	switch_status_t (*timer_sync) (switch_timer_t *);
	/*! function to check if the current step has expired */
	switch_status_t (*timer_check) (switch_timer_t *, switch_bool_t);
	/*! function to deallocate the timer */
	switch_status_t (*timer_destroy) (switch_timer_t *);
	struct switch_timer_interface *next;
};

/*! \brief Abstract interface to a dialplan module */
struct switch_dialplan_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! the function to read an extension and set a channels dialpan */
	switch_dialplan_hunt_function_t hunt_function;
	struct switch_dialplan_interface *next;
};

/*! \brief Abstract interface to a file format module */
struct switch_file_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function to open the file */
	switch_status_t (*file_open) (switch_file_handle_t *, const char *file_path);
	/*! function to close the file */
	switch_status_t (*file_close) (switch_file_handle_t *);
	/*! function to read from the file */
	switch_status_t (*file_read) (switch_file_handle_t *, void *data, switch_size_t *len);
	/*! function to write from the file */
	switch_status_t (*file_write) (switch_file_handle_t *, void *data, switch_size_t *len);
	/*! function to seek to a certian position in the file */
	switch_status_t (*file_seek) (switch_file_handle_t *, unsigned int *cur_pos, int64_t samples, int whence);
	/*! function to set meta data */
	switch_status_t (*file_set_string) (switch_file_handle_t *fh, switch_audio_col_t col, const char *string);
	/*! function to get meta data */
	switch_status_t (*file_get_string) (switch_file_handle_t *fh, switch_audio_col_t col, const char **string);
	/*! list of supported file extensions */
	char **extens;
	struct switch_file_interface *next;
};

/*! an abstract representation of a file handle (some parameters based on compat with libsndfile) */
struct switch_file_handle {
	/*! the interface of the module that implemented the current file type */
	const switch_file_interface_t *file_interface;
	/*! flags to control behaviour */
	uint32_t flags;
	/*! a file descriptor if neceessary */
	switch_file_t *fd;
	/*! samples position of the handle */
	unsigned int samples;
	/*! the current samplerate */
	uint32_t samplerate;
	/*! the current native samplerate */
	uint32_t native_rate;
	/*! the number of channels */
	uint8_t channels;
	/*! integer representation of the format */
	unsigned int format;
	/*! integer representation of the sections */
	unsigned int sections;
	/*! is the file seekable */
	int seekable;
	/*! the sample count of the file */
	switch_size_t sample_count;
	/*! the speed of the file playback */
	int speed;
	/*! the handle's memory pool */
	switch_memory_pool_t *memory_pool;
	/*! pre-buffer x bytes for streams */
	uint32_t prebuf;
	/*! private data for the format module to store handle specific info */
	uint32_t interval;
	void *private_info;
	char *handler;
	int64_t pos;
	switch_buffer_t *audio_buffer;
	switch_buffer_t *sp_audio_buffer;
	uint32_t thresh;
	uint32_t silence_hits;
	uint32_t offset_pos;
	uint32_t last_pos;
	int32_t vol;
	switch_audio_resampler_t *resampler;
	switch_buffer_t *buffer;
	switch_byte_t *dbuf;
	switch_size_t dbuflen;
	const char *file;
	const char *func;
	int line;
};

/*! \brief Abstract interface to an asr module */
struct switch_asr_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function to open the asr interface */
	switch_status_t (*asr_open) (switch_asr_handle_t *ah, const char *codec, int rate, const char *dest, switch_asr_flag_t *flags);
	/*! function to load a grammar to the asr interface */
	switch_status_t (*asr_load_grammar) (switch_asr_handle_t *ah, const char *grammar, const char *path);
	/*! function to unload a grammar to the asr interface */
	switch_status_t (*asr_unload_grammar) (switch_asr_handle_t *ah, const char *grammar);
	/*! function to close the asr interface */
	switch_status_t (*asr_close) (switch_asr_handle_t *ah, switch_asr_flag_t *flags);
	/*! function to feed audio to the ASR */
	switch_status_t (*asr_feed) (switch_asr_handle_t *ah, void *data, unsigned int len, switch_asr_flag_t *flags);
	/*! function to resume the ASR */
	switch_status_t (*asr_resume) (switch_asr_handle_t *ah);
	/*! function to pause the ASR */
	switch_status_t (*asr_pause) (switch_asr_handle_t *ah);
	/*! function to read results from the ASR */
	switch_status_t (*asr_check_results) (switch_asr_handle_t *ah, switch_asr_flag_t *flags);
	/*! function to read results from the ASR */
	switch_status_t (*asr_get_results) (switch_asr_handle_t *ah, char **xmlstr, switch_asr_flag_t *flags);
	struct switch_asr_interface *next;
};

/*! an abstract representation of an asr speech interface. */
struct switch_asr_handle {
	/*! the interface of the module that implemented the current speech interface */
	const switch_asr_interface_t *asr_interface;
	/*! flags to control behaviour */
	uint32_t flags;
	/*! The Name */
	char *name;
	/*! The Codec */
	char *codec;
	/*! The Rate */
	uint32_t rate;
	char *grammar;
	/*! module specific param*/
	char *param;
	/*! the handle's memory pool */
	switch_memory_pool_t *memory_pool;
	/*! private data for the format module to store handle specific info */
	void *private_info;
};

/*! \brief Abstract interface to a speech module */
struct switch_speech_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function to open the speech interface */
	switch_status_t (*speech_open) (switch_speech_handle_t *sh, const char *voice_name, int rate, switch_speech_flag_t *flags);
	/*! function to close the speech interface */
	switch_status_t (*speech_close) (switch_speech_handle_t *, switch_speech_flag_t *flags);
	/*! function to feed audio to the ASR */
	switch_status_t (*speech_feed_tts) (switch_speech_handle_t *sh, char *text, switch_speech_flag_t *flags);
	/*! function to read audio from the TTS */
	switch_status_t (*speech_read_tts) (switch_speech_handle_t *sh, void *data, switch_size_t *datalen, uint32_t * rate, switch_speech_flag_t *flags);
	void (*speech_flush_tts) (switch_speech_handle_t *sh);
	void (*speech_text_param_tts) (switch_speech_handle_t *sh, char *param, const char *val);
	void (*speech_numeric_param_tts) (switch_speech_handle_t *sh, char *param, int val);
	void (*speech_float_param_tts) (switch_speech_handle_t *sh, char *param, double val);

	struct switch_speech_interface *next;
};


/*! an abstract representation of a asr/tts speech interface. */
struct switch_speech_handle {
	/*! the interface of the module that implemented the current speech interface */
	const switch_speech_interface_t *speech_interface;
	/*! flags to control behaviour */
	uint32_t flags;
	/*! The Name */
	char *name;
	/*! The Rate */
	uint32_t rate;
	uint32_t speed;
	uint32_t samples;
	char voice[80];
	char *engine;
	/*! module specific param*/
	char *param;
	/*! the handle's memory pool */
	switch_memory_pool_t *memory_pool;
	/*! private data for the format module to store handle specific info */
	void *private_info;
};

/*! \brief Abstract interface to a say module */
struct switch_say_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function to pass down to the module */
	switch_say_callback_t say_function;
	struct switch_say_interface *next;
};

/*! \brief Abstract interface to a chat module */
struct switch_chat_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function to open the directory interface */
	switch_status_t (*chat_send) (char *proto, char *from, char *to, char *subject, char *body, char *hint);
	struct switch_chat_interface *next;
};

/*! \brief Abstract interface to a management module */
struct switch_management_interface {
	/*! the name of the interface */
	const char *relative_oid;
	/*! function to open the directory interface */
	switch_status_t (*management_function) (char *relative_oid, switch_management_action_t action, char *data, switch_size_t datalen);
	struct switch_management_interface *next;
};

/*! \brief Abstract interface to a directory module */
struct switch_directory_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function to open the directory interface */
	switch_status_t (*directory_open) (switch_directory_handle_t *dh, char *source, char *dsn, char *passwd);
	/*! function to close the directory interface */
	switch_status_t (*directory_close) (switch_directory_handle_t *dh);
	/*! function to query the directory interface */
	switch_status_t (*directory_query) (switch_directory_handle_t *dh, char *base, char *query);
	/*! function to advance to the next record */
	switch_status_t (*directory_next) (switch_directory_handle_t *dh);
	/*! function to advance to the next name/value pair in the current record */
	switch_status_t (*directory_next_pair) (switch_directory_handle_t *dh, char **var, char **val);

	struct switch_directory_interface *next;
};

/*! an abstract representation of a directory interface. */
struct switch_directory_handle {
	/*! the interface of the module that implemented the current directory interface */
	const switch_directory_interface_t *directory_interface;
	/*! flags to control behaviour */
	uint32_t flags;

	/*! the handle's memory pool */
	switch_memory_pool_t *memory_pool;
	/*! private data for the format module to store handle specific info */
	void *private_info;
};


/* nobody has more setting than speex so we will let them set the standard */
/*! \brief Various codec settings (currently only relevant to speex) */
struct switch_codec_settings {
	/*! desired quality */
	int quality;
	/*! desired complexity */
	int complexity;
	/*! desired enhancement */
	int enhancement;
	/*! desired vad level */
	int vad;
	/*! desired vbr level */
	int vbr;
	/*! desired vbr quality */
	float vbr_quality;
	/*! desired abr level */
	int abr;
	/*! desired dtx setting */
	int dtx;
	/*! desired preprocessor settings */
	int preproc;
	/*! preprocessor vad settings */
	int pp_vad;
	/*! preprocessor gain control settings */
	int pp_agc;
	/*! preprocessor gain level */
	float pp_agc_level;
	/*! preprocessor denoise level */
	int pp_denoise;
	/*! preprocessor dereverb settings */
	int pp_dereverb;
	/*! preprocessor dereverb decay level */
	float pp_dereverb_decay;
	/*! preprocessor dereverb level */
	float pp_dereverb_level;
};

/*! an abstract handle to a codec module */
struct switch_codec {
	/*! the codec interface table this handle uses */
	const switch_codec_interface_t *codec_interface;
	/*! the specific implementation of the above codec */
	const switch_codec_implementation_t *implementation;
	/*! fmtp line from remote sdp */
	char *fmtp_in;
	/*! fmtp line for local sdp */
	char *fmtp_out;
	/*! codec settings for this handle */
	switch_codec_settings_t codec_settings;
	/*! flags to modify behaviour */
	uint32_t flags;
	/*! the handle's memory pool */
	switch_memory_pool_t *memory_pool;
	/*! private data for the codec module to store handle specific info */
	void *private_info;
	switch_payload_t agreed_pt;
};

/*! \brief A table of settings and callbacks that define a paticular implementation of a codec */
struct switch_codec_implementation {
	/*! enumeration defining the type of the codec */
	switch_codec_type_t codec_type;
	/*! the IANA code number */
	switch_payload_t ianacode;
	/*! the IANA code name */
	char *iananame;
	/*! default fmtp to send (can be overridden by the init function) */
	char *fmtp;
	/*! samples transferred per second */
	uint32_t samples_per_second;
	/*! actual samples transferred per second for those who are not moron g722 RFC writers*/
	uint32_t actual_samples_per_second;
	/*! bits transferred per second */
	int bits_per_second;
	/*! number of microseconds that denote one frame */
	int microseconds_per_frame;
	/*! number of samples that denote one frame */
	uint32_t samples_per_frame;
	/*! number of bytes that denote one frame decompressed */
	uint32_t bytes_per_frame;
	/*! number of bytes that denote one frame compressed */
	uint32_t encoded_bytes_per_frame;
	/*! number of channels represented */
	uint8_t number_of_channels;
	/*! number of frames to send in one netowrk packet */
	int pref_frames_per_packet;
	/*! max number of frames to send in one network packet */
	int max_frames_per_packet;
	/*! function to initialize a codec handle using this implementation */
    switch_core_codec_init_func_t init;
	/*! function to encode raw data into encoded data */
    switch_core_codec_encode_func_t encode;
	/*! function to decode encoded data into raw data */
    switch_core_codec_decode_func_t decode;
	/*! deinitalize a codec handle using this implementation */
    switch_core_codec_destroy_func_t destroy;
	uint32_t codec_id;
	struct switch_codec_implementation *next;
};

/*! \brief Top level module interface to implement a series of codec implementations */
struct switch_codec_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! a list of codec implementations related to the codec */
	switch_codec_implementation_t *implementations;
	uint32_t codec_id;
	struct switch_codec_interface *next;
};

/*! \brief A module interface to implement an application */
struct switch_application_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! function the application implements */
	switch_application_function_t application_function;
	/*! the long winded description of the application */
	const char *long_desc;
	/*! the short and sweet description of the application */
	const char *short_desc;
	/*! an example of the application syntax */
	const char *syntax;
	/*! flags to control behaviour */
	uint32_t flags;
	struct switch_application_interface *next;
};

/*! \brief A module interface to implement an api function */
struct switch_api_interface {
	/*! the name of the interface */
	const char *interface_name;
	/*! a description of the api function */
	const char *desc;
	/*! function the api call uses */
	switch_api_function_t function;
	/*! an example of the api syntax */
	const char *syntax;
	struct switch_api_interface *next;
};

SWITCH_END_EXTERN_C
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
