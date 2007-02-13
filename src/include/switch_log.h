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
 * switch_log.h -- Logger
 *
 */
/*! \file switch_log.h
    \brief Simple Log

	Logging Routines
*/

#ifndef SWITCH_LOG_H
#define SWITCH_LOG_H

#include <switch.h>

SWITCH_BEGIN_EXTERN_C

///\defgroup log Logger Routines
///\ingroup core1
///\{


/*! \brief Log Data
 */
typedef struct {
	/*! The complete log message */
	char *data;
	/*! The file where the message originated */	
	char *file;
	/*! The line number where the message originated */	
	uint32_t line;
	/*! The function where the message originated */	
	char *func;
	/*! The log level of the message */	
	switch_log_level_t level;
	/*! The time when the log line was sent */
	switch_time_t timestamp;
	/*! A pointer to where the actual content of the message starts (skipping past the preformatted portion) */	
	char *content;
} switch_log_node_t;

typedef switch_status_t (*switch_log_function_t)(const switch_log_node_t *node, switch_log_level_t level);


/*! 
  \brief Initilize the logging engine
  \param pool the memory pool to use
  \note to be called at application startup by the core
*/
SWITCH_DECLARE(switch_status_t) switch_log_init(switch_memory_pool_t *pool);

/*! 
  \brief Shut down the logging engine
  \note to be called at application termination by the core
*/
SWITCH_DECLARE(switch_status_t) switch_log_shutdown(void);


/*! 
  \brief Write log data to the logging engine
  \param channel the log channel to write to
  \param file the current file
  \param func the current function
  \param line the current line
  \param level the current log level
  \param fmt desired format
  \param ... variable args
  \note there are channel macros to supply the first 4 parameters
*/
SWITCH_DECLARE(void) switch_log_printf(switch_text_channel_t channel, const char *file, const char *func, int line, switch_log_level_t level, char *fmt, ...) PRINTF_FUNCTION(6,7);

/*! 
  \brief Shut down  the logging engine
  \note to be called at application termination by the core
*/
SWITCH_DECLARE(switch_status_t) switch_log_bind_logger(switch_log_function_t function, switch_log_level_t level);

/*! 
  \brief Return the name of the specified log level
  \param level the level
  \return the name of the log level
*/
SWITCH_DECLARE(const char *) switch_log_level2str(switch_log_level_t level);

/*! 
  \brief Return the level number of the specified log level name
  \param str the name of the level
  \return the log level
*/
SWITCH_DECLARE(switch_log_level_t) switch_log_str2level(const char *str);

///\}
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
