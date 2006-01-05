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
 * switch_config.h -- Configuration File Parser
 *
 */
/*! \file switch_config.h
    \brief Basic Configuration File Parser

	This module implements a basic interface and file format parser it may be depricated in favor of database entries
	or expanded to tie to external handlers in the future as necessary.
<pre>

EXAMPLE 

[category1]
var1 => val1
var2 => val2
\# lines that begin with \# are comments
\#var3 => val3
</pre>

*/

#ifndef SWITCH_CONFIG_H
#define SWITCH_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

/*! \brief A simple file handle representing an open configuration file **/
struct switch_config {
	/*! FILE stream buffer to the opened file */
    FILE *file;
	/*! path to the file */
    char *path;
	/*! current category */
    char category[256];
	/*! buffer of current line being read */
    char buf[1024];
	/*! current line number in file */
    int lineno;
};

/*!
  \brief Open a configuration file
  \param cfg (switch_config *) config handle to use
  \param file_path path to the file
  \return 1 (true) on success 0 (false) on failure
*/
SWITCH_DECLARE(int) switch_config_open_file(switch_config *cfg, char *file_path);

/*!
  \brief Close a previously opened configuration file
  \param cfg (switch_config *) config handle to use
*/
SWITCH_DECLARE(void) switch_config_close_file(switch_config *cfg);

/*!
  \brief Retrieve next name/value pair from configuration file
  \param cfg (switch_config *) config handle to use
  \param var pointer to aim at the new variable name
  \param val pointer to aim at the new value
*/
SWITCH_DECLARE(int) switch_config_next_pair(switch_config *cfg, char **var, char **val);

#ifdef __cplusplus
}
#endif


#endif
