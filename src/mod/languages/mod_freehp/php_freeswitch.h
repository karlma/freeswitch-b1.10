/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.29
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

/*
  +----------------------------------------------------------------------+
  | PHP version 4.0                                                      |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors:                                                             |
  |                                                                      |
  +----------------------------------------------------------------------+
 */


#ifndef PHP_FREESWITCH_H
#define PHP_FREESWITCH_H

extern zend_module_entry freeswitch_module_entry;
#define phpext_freeswitch_ptr &freeswitch_module_entry

#ifdef PHP_WIN32
# define PHP_FREESWITCH_API __declspec(dllexport)
#else
# define PHP_FREESWITCH_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(freeswitch);
PHP_MSHUTDOWN_FUNCTION(freeswitch);
PHP_RINIT_FUNCTION(freeswitch);
PHP_RSHUTDOWN_FUNCTION(freeswitch);
PHP_MINFO_FUNCTION(freeswitch);

ZEND_NAMED_FUNCTION(_wrap_fs_core_set_globals);
ZEND_NAMED_FUNCTION(_wrap_fs_core_init);
ZEND_NAMED_FUNCTION(_wrap_fs_core_destroy);
ZEND_NAMED_FUNCTION(_wrap_fs_loadable_module_init);
ZEND_NAMED_FUNCTION(_wrap_fs_loadable_module_shutdown);
ZEND_NAMED_FUNCTION(_wrap_fs_console_loop);
ZEND_NAMED_FUNCTION(_wrap_fs_consol_log);
ZEND_NAMED_FUNCTION(_wrap_fs_consol_clean);
ZEND_NAMED_FUNCTION(_wrap_fs_core_session_locate);
ZEND_NAMED_FUNCTION(_wrap_fs_channel_answer);
ZEND_NAMED_FUNCTION(_wrap_fs_channel_pre_answer);
ZEND_NAMED_FUNCTION(_wrap_fs_channel_hangup);
ZEND_NAMED_FUNCTION(_wrap_fs_channel_set_variable);
ZEND_NAMED_FUNCTION(_wrap_fs_channel_get_variable);
ZEND_NAMED_FUNCTION(_wrap_fs_channel_set_state);
ZEND_NAMED_FUNCTION(_wrap_fs_ivr_play_file);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_ivr_record_file);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_ivr_sleep);
ZEND_NAMED_FUNCTION(_wrap_fs_ivr_play_file2);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_ivr_collect_digits_callback);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_ivr_collect_digits_count);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_ivr_originate);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_ivr_session_transfer);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_ivr_speak_text);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_channel_get_variable);
ZEND_NAMED_FUNCTION(_wrap_fs_switch_channel_set_variable);
#endif /* PHP_FREESWITCH_H */
