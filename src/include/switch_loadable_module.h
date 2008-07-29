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
 * switch_loadable_module.h -- Loadable Modules
 *
 */
/*! \file switch_loadable_module.h
    \brief Loadable Module Routines

	This module is the gateway between external modules and the core of the application.
	it contains all the access points to the various pluggable interfaces including the codecs 
	and API modules.

*/

#ifndef SWITCH_LOADABLE_MODULE_H
#define SWITCH_LOADABLE_MODULE_H

#include <switch.h>
#include <switch_module_interfaces.h>

SWITCH_BEGIN_EXTERN_C
/*!
  \defgroup mods Loadable Module Functions
  \ingroup core1
  \{ 
*/
/*! \brief The abstraction of a loadable module */
	struct switch_loadable_module_interface {
	/*! the name of the module */
	const char *module_name;
	/*! the table of endpoints the module has implmented */
	switch_endpoint_interface_t *endpoint_interface;
	/*! the table of timers the module has implmented */
	switch_timer_interface_t *timer_interface;
	/*! the table of dialplans the module has implmented */
	switch_dialplan_interface_t *dialplan_interface;
	/*! the table of codecs the module has implmented */
	switch_codec_interface_t *codec_interface;
	/*! the table of applications the module has implmented */
	switch_application_interface_t *application_interface;
	/*! the table of api functions the module has implmented */
	switch_api_interface_t *api_interface;
	/*! the table of file formats the module has implmented */
	switch_file_interface_t *file_interface;
	/*! the table of speech interfaces the module has implmented */
	switch_speech_interface_t *speech_interface;
	/*! the table of directory interfaces the module has implmented */
	switch_directory_interface_t *directory_interface;
	/*! the table of chat interfaces the module has implmented */
	switch_chat_interface_t *chat_interface;
	/*! the table of say interfaces the module has implmented */
	switch_say_interface_t *say_interface;
	/*! the table of asr interfaces the module has implmented */
	switch_asr_interface_t *asr_interface;
	/*! the table of management interfaces the module has implmented */
	switch_management_interface_t *management_interface;
	switch_memory_pool_t *pool;
};

/*!
  \brief Initilize the module backend and load all the modules
  \return SWITCH_STATUS_SUCCESS when complete
 */
SWITCH_DECLARE(switch_status_t) switch_loadable_module_init(void);

/*!
  \brief Shutdown the module backend and call the shutdown routine in all loaded modules
 */
SWITCH_DECLARE(void) switch_loadable_module_shutdown(void);

/*!
  \brief Retrieve the endpoint interface by it's registered name
  \param name the name of the endpoint
  \return the desired endpoint interface
 */
SWITCH_DECLARE(switch_endpoint_interface_t *) switch_loadable_module_get_endpoint_interface(const char *name);

/*!
  \brief Retrieve the codec interface by it's registered name
  \param name the name of the codec
  \return the desired codec interface
 */
SWITCH_DECLARE(switch_codec_interface_t *) switch_loadable_module_get_codec_interface(const char *name);

/*!
  \brief Retrieve the dialplan interface by it's registered name
  \param name the name of the dialplan
  \return the desired dialplan interface
 */
SWITCH_DECLARE(switch_dialplan_interface_t *) switch_loadable_module_get_dialplan_interface(const char *name);

/*!
  \brief build a dynamic module object and register it (for use in double embeded modules)
  \param filename the name of the modules source file
  \param switch_module_load the function to call when the module is loaded
  \param switch_module_runtime a function requested to be started in it's own thread once loaded
  \param switch_module_shutdown the function to call when the system is shutdown
  \param runtime start the runtime thread or not
  \return the resulting status
  \note only use this function if you are making a module that in turn gateways module loading to another technology
 */
SWITCH_DECLARE(switch_status_t) switch_loadable_module_build_dynamic(char *filename,
																	 switch_module_load_t switch_module_load,
																	 switch_module_runtime_t switch_module_runtime,
																	 switch_module_shutdown_t switch_module_shutdown, switch_bool_t runtime);


/*!
  \brief Retrieve the timer interface by it's registered name
  \param name the name of the timer
  \return the desired timer interface
 */
SWITCH_DECLARE(switch_timer_interface_t *) switch_loadable_module_get_timer_interface(const char *name);

/*!
  \brief Retrieve the application interface by it's registered name
  \param name the name of the application
  \return the desired application interface
 */
SWITCH_DECLARE(switch_application_interface_t *) switch_loadable_module_get_application_interface(const char *name);

/*!
  \brief Retrieve the API interface by it's registered name
  \param name the name of the API
  \return the desired API interface
 */
SWITCH_DECLARE(switch_api_interface_t *) switch_loadable_module_get_api_interface(const char *name);

/*!
  \brief Retrieve the file format interface by it's registered name
  \param name the name of the file format
  \return the desired file format interface
 */
SWITCH_DECLARE(switch_file_interface_t *) switch_loadable_module_get_file_interface(const char *name);

/*!
  \brief Retrieve the speech interface by it's registered name
  \param name the name of the speech interface
  \return the desired speech interface
 */
SWITCH_DECLARE(switch_speech_interface_t *) switch_loadable_module_get_speech_interface(const char *name);

/*!
  \brief Retrieve the asr interface by it's registered name
  \param name the name of the asr interface
  \return the desired asr interface
 */
SWITCH_DECLARE(switch_asr_interface_t *) switch_loadable_module_get_asr_interface(const char *name);

/*!
  \brief Retrieve the directory interface by it's registered name
  \param name the name of the directory interface
  \return the desired directory interface
 */
SWITCH_DECLARE(switch_directory_interface_t *) switch_loadable_module_get_directory_interface(const char *name);

/*!
  \brief Retrieve the chat interface by it's registered name
  \param name the name of the chat interface
  \return the desired chat interface
 */
SWITCH_DECLARE(switch_chat_interface_t *) switch_loadable_module_get_chat_interface(const char *name);

/*!
  \brief Retrieve the say interface by it's registered name
  \param name the name of the say interface
  \return the desired say interface
 */
SWITCH_DECLARE(switch_say_interface_t *) switch_loadable_module_get_say_interface(const char *name);

/*!
  \brief Retrieve the management interface by it's registered name
  \param relative_oid the relative oid of the management interface
  \return the desired management interface
 */
SWITCH_DECLARE(switch_management_interface_t *) switch_loadable_module_get_management_interface(const char *relative_oid);

/*!
  \brief Retrieve the list of loaded codecs into an array
  \param array the array to populate
  \param arraylen the max size in elements of the array
  \return the number of elements added to the array
 */
SWITCH_DECLARE(int) switch_loadable_module_get_codecs(const switch_codec_implementation_t **array, int arraylen);


/*!
  \brief Retrieve the list of loaded codecs into an array based on another array showing the sorted order
  \param array the array to populate
  \param arraylen the max size in elements of the array
  \param prefs the array of preferred codec names
  \param preflen the size in elements of the prefs
  \return the number of elements added to the array
  \note this function only considers codecs that are listed in the "prefs" array and ignores the rest.
*/
SWITCH_DECLARE(int) switch_loadable_module_get_codecs_sorted(const switch_codec_implementation_t **array, int arraylen, char **prefs, int preflen);

/*!
  \brief Execute a registered API command
  \param cmd the name of the API command to execute
  \param arg the optional arguement to the command
  \param session an optional session
  \param stream stream for output
  \return the status returned by the API call
*/
SWITCH_DECLARE(switch_status_t) switch_api_execute(const char *cmd, const char *arg, switch_core_session_t *session, switch_stream_handle_t *stream);

/*!
  \brief Load a module
  \param dir the directory where the module resides
  \param fname the file name of the module
  \param runtime option to start the runtime thread if it exists
  \param err pointer to error message
  \return the status
*/
SWITCH_DECLARE(switch_status_t) switch_loadable_module_load_module(char *dir, char *fname, switch_bool_t runtime, const char **err);

/*!
  \brief Check if a module is loaded
  \param mod the module name
  \return the status
*/
SWITCH_DECLARE(switch_status_t) switch_loadable_module_exists(const char *mod);

/*!
  \brief Unoad a module
  \param dir the directory where the module resides
  \param fname the file name of the module
  \param err pointer to error message
  \return the status
*/
SWITCH_DECLARE(switch_status_t) switch_loadable_module_unload_module(char *dir, char *fname, const char **err);

/* Prototypes of module interface functions */

/*!
  \brief Load a module
  \param module_interface a pointer to a pointer to aim at your module's local interface
  \param filename the path to the module's dll or so file
  \return SWITCH_STATUS_SUCCESS on a successful load
*/
SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(switch_loadable_module_interface_t **module_interface, char *filename);
SWITCH_MOD_DECLARE(switch_status_t) switch_module_runtime(void);

/*!
  \brief Shutdown a module
  \return SWITCH_STATUS_SUCCESS on a successful shutdown
*/
SWITCH_MOD_DECLARE(switch_status_t) switch_module_shutdown(void);

#define SWITCH_ADD_API(api_int, int_name, descript, funcptr, syntax_string) \
	for (;;) { \
	api_int = (switch_api_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_API_INTERFACE); \
	api_int->interface_name = int_name; \
	api_int->desc = descript; \
	api_int->function = funcptr; \
	api_int->syntax = syntax_string; \
	break; \
	}

#define SWITCH_ADD_CHAT(chat_int, int_name, funcptr) \
	for (;;) { \
	chat_int = (switch_chat_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_CHAT_INTERFACE); \
	chat_int->chat_send = funcptr; \
	chat_int->interface_name = int_name; \
	break; \
	}

#define SWITCH_ADD_APP(app_int, int_name, short_descript, long_descript, funcptr, syntax_string, app_flags) \
	for (;;) { \
	app_int = (switch_application_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_APPLICATION_INTERFACE); \
	app_int->interface_name = int_name; \
	app_int->application_function = funcptr; \
	app_int->short_desc = short_descript; \
	app_int->long_desc = long_descript; \
	app_int->syntax = syntax_string; \
	app_int->flags = app_flags; \
	break; \
	}

#define SWITCH_ADD_DIALPLAN(dp_int, int_name, funcptr) \
	for (;;) { \
	dp_int = (switch_dialplan_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_DIALPLAN_INTERFACE); \
	dp_int->hunt_function = funcptr; \
	dp_int->interface_name = int_name; \
	break; \
	}

SWITCH_DECLARE(uint32_t) switch_core_codec_next_id(void);

#define SWITCH_ADD_CODEC(codec_int, int_name) \
	for (;;) { \
		codec_int = (switch_codec_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_CODEC_INTERFACE); \
		codec_int->interface_name = switch_core_strdup(pool, int_name);	\
		codec_int->codec_id = switch_core_codec_next_id();				\
		break;															\
	}


	 static inline void switch_core_codec_add_implementation(switch_memory_pool_t *pool, switch_codec_interface_t *codec_interface,
															 /*! enumeration defining the type of the codec */
															 const switch_codec_type_t codec_type,
															 /*! the IANA code number */
															 switch_payload_t ianacode,
															 /*! the IANA code name */
															 const char *iananame,
															 /*! default fmtp to send (can be overridden by the init function) */
															 char *fmtp,
															 /*! samples transferred per second */
															 uint32_t samples_per_second,
															 /*! actual samples transferred per second for those who are not moron g722 RFC writers */
															 uint32_t actual_samples_per_second,
															 /*! bits transferred per second */
															 int bits_per_second,
															 /*! number of microseconds that denote one frame */
															 int microseconds_per_frame,
															 /*! number of samples that denote one frame */
															 uint32_t samples_per_frame,
															 /*! number of bytes that denote one frame decompressed */
															 uint32_t bytes_per_frame,
															 /*! number of bytes that denote one frame compressed */
															 uint32_t encoded_bytes_per_frame,
															 /*! number of channels represented */
															 uint8_t number_of_channels,
															 /*! number of frames to send in one netowrk packet */
															 int pref_frames_per_packet,
															 /*! max number of frames to send in one network packet */
															 int max_frames_per_packet,
															 /*! function to initialize a codec handle using this implementation */
															 switch_core_codec_init_func_t init,
															 /*! function to encode raw data into encoded data */
															 switch_core_codec_encode_func_t encode,
															 /*! function to decode encoded data into raw data */
															 switch_core_codec_decode_func_t decode,
															 /*! deinitalize a codec handle using this implementation */
															 switch_core_codec_destroy_func_t destroy)
{
	if (codec_type == SWITCH_CODEC_TYPE_VIDEO || SWITCH_ACCEPTABLE_INTERVAL(microseconds_per_frame / 1000)) {
		switch_codec_implementation_t *impl = (switch_codec_implementation_t *) switch_core_alloc(pool, sizeof(*impl));
		impl->codec_type = codec_type;
		impl->ianacode = ianacode;
		impl->iananame = switch_core_strdup(pool, iananame);
		impl->fmtp = switch_core_strdup(pool, fmtp);
		impl->samples_per_second = samples_per_second;
		impl->actual_samples_per_second = actual_samples_per_second;
		impl->bits_per_second = bits_per_second;
		impl->microseconds_per_frame = microseconds_per_frame;
		impl->samples_per_frame = samples_per_frame;
		impl->bytes_per_frame = bytes_per_frame;
		impl->encoded_bytes_per_frame = encoded_bytes_per_frame;
		impl->number_of_channels = number_of_channels;
		impl->pref_frames_per_packet = pref_frames_per_packet;
		impl->max_frames_per_packet = max_frames_per_packet;
		impl->init = init;
		impl->encode = encode;
		impl->decode = decode;
		impl->destroy = destroy;
		impl->codec_id = codec_interface->codec_id;
		impl->next = codec_interface->implementations;
		codec_interface->implementations = impl;
	}
}

///\}

#define SWITCH_DECLARE_STATIC_MODULE(init, load, run, shut) void init(void) { \
		switch_loadable_module_build_dynamic(__FILE__, load, run, shut, SWITCH_FALSE); \
	}


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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
