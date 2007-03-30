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
 * switch_channel.h -- Media Channel Interface
 *
 */
/** 
 * @file switch_channel.h
 * @brief Media Channel Interface
 * @see switch_channel
 */

#ifndef SWITCH_CHANNEL_H
#define SWITCH_CHANNEL_H

#include <switch.h>

SWITCH_BEGIN_EXTERN_C struct switch_channel_timetable {
	switch_time_t created;
	switch_time_t answered;
	switch_time_t hungup;
	switch_time_t transferred;
	struct switch_channel_timetable *next;
};

typedef struct switch_channel_timetable switch_channel_timetable_t;

/**
 * @defgroup switch_channel Channel Functions
 * @ingroup core1
 *	The switch_channel object is a private entity that belongs to a session that contains the call
 *	specific information such as the call state, variables, caller profiles and DTMF queue
 * @{
 */

/*!
  \brief Get the current state of a channel in the state engine
  \param channel channel to retrieve state from
  \return current state of channel
*/
SWITCH_DECLARE(switch_channel_state_t) switch_channel_get_state(switch_channel_t *channel);

/*!
  \brief Determine if a channel is ready for io
  \param channel channel to test
  \return true if the channel is ready
*/
SWITCH_DECLARE(uint8_t) switch_channel_ready(switch_channel_t *channel);


SWITCH_DECLARE(switch_channel_state_t) switch_channel_perform_set_state(switch_channel_t *channel,
																		const char *file, const char *func, int line, switch_channel_state_t state);

/*!
  \brief Set the current state of a channel
  \param channel channel to set state of
  \param state new state
  \return current state of channel after application of new state
*/
#define switch_channel_set_state(channel, state) switch_channel_perform_set_state(channel, __FILE__, __SWITCH_FUNC__, __LINE__, state)

/*!
  \brief return a cause code for a given string
  \param str the string to check
  \return the code
*/
SWITCH_DECLARE(switch_call_cause_t) switch_channel_str2cause(char *str);

/*!
  \brief return the cause code for a given channel
  \param channel the channel
  \return the code
*/
SWITCH_DECLARE(switch_call_cause_t) switch_channel_get_cause(switch_channel_t *channel);

/*!
  \brief return a cause string for a given cause
  \param cause the code to check
  \return the string
*/
SWITCH_DECLARE(char *) switch_channel_cause2str(switch_call_cause_t cause);

/*!
  \brief View the timetable of a channel
  \param channel channel to retrieve timetable from
  \return a pointer to the channel's timetable (created, answered, etc..)
*/
SWITCH_DECLARE(switch_channel_timetable_t *) switch_channel_get_timetable(switch_channel_t *channel);

/*!
  \brief Allocate a new channel
  \param channel NULL pointer to allocate channel to
  \param pool memory_pool to use for allocation
  \return SWITCH_STATUS_SUCCESS if successful
*/
SWITCH_DECLARE(switch_status_t) switch_channel_alloc(switch_channel_t **channel, switch_memory_pool_t *pool);

/*!
  \brief Connect a newly allocated channel to a session object and setup it's initial state
  \param channel the channel to initilize
  \param session the session to connect the channel to
  \param state the initial state of the channel
  \param flags the initial channel flags
*/
SWITCH_DECLARE(switch_status_t) switch_channel_init(switch_channel_t *channel, switch_core_session_t *session, switch_channel_state_t state,
													uint32_t flags);

/*!
  \brief Fire A presence event for the channel
  \param channel the channel to initilize
  \param rpid the rpid if for the icon to use
  \param status the status message
*/
SWITCH_DECLARE(void) switch_channel_presence(switch_channel_t *channel, char *rpid, char *status);

/*!
  \brief Uninitalize a channel
  \param channel the channel to uninit
*/
SWITCH_DECLARE(void) switch_channel_uninit(switch_channel_t *channel);

/*!
  \brief Set the given channel's caller profile
  \param channel channel to assign the profile to
  \param caller_profile the profile to assign
*/
SWITCH_DECLARE(void) switch_channel_set_caller_profile(switch_channel_t *channel, switch_caller_profile_t *caller_profile);

/*!
  \brief Retrive the given channel's caller profile
  \param channel channel to retrive the profile from
  \return the requested profile
*/
SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_caller_profile(switch_channel_t *channel);

/*!
  \brief Set the given channel's originator caller profile
  \param channel channel to assign the profile to
  \param caller_profile the profile to assign
*/
SWITCH_DECLARE(void) switch_channel_set_originator_caller_profile(switch_channel_t *channel, switch_caller_profile_t *caller_profile);

/*!
  \brief Retrive the given channel's originator caller profile
  \param channel channel to retrive the profile from
  \return the requested profile
*/
SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_originator_caller_profile(switch_channel_t *channel);

/*!
  \brief Set the given channel's originatee caller profile
  \param channel channel to assign the profile to
  \param caller_profile the profile to assign
*/
SWITCH_DECLARE(void) switch_channel_set_originatee_caller_profile(switch_channel_t *channel, switch_caller_profile_t *caller_profile);

/*!
  \brief Retrive the given channel's originatee caller profile
  \param channel channel to retrive the profile from
  \return the requested profile
*/
SWITCH_DECLARE(switch_caller_profile_t *) switch_channel_get_originatee_caller_profile(switch_channel_t *channel);


/*!
  \brief Retrive the given channel's unique id
  \param channel channel to retrive the unique id from
  \return the unique id
*/
SWITCH_DECLARE(char *) switch_channel_get_uuid(switch_channel_t *channel);

/*!
  \brief Set a variable on a given channel
  \param channel channel to set variable on
  \param varname the name of the variable
  \param value the vaule of the variable
  \returns SWITCH_STATUS_SUCCESS if successful
*/
SWITCH_DECLARE(switch_status_t) switch_channel_set_variable(switch_channel_t *channel, const char *varname, const char *value);

/*!
  \brief Set a variable on a given channel, without duplicating the value from the session pool.
  \param channel channel to set variable on
  \param varname the name of the variable
  \param value the vaule of the variable (MUST BE ALLOCATED FROM THE SESSION POOL ALREADY)
  \returns SWITCH_STATUS_SUCCESS if successful
*/
SWITCH_DECLARE(switch_status_t) switch_channel_set_variable_nodup(switch_channel_t *channel, const char *varname, char *value);

/*!
  \brief Retrieve a variable from a given channel
  \param channel channel to retrieve variable from
  \param varname the name of the variable
  \return the value of the requested variable
*/
SWITCH_DECLARE(char *) switch_channel_get_variable(switch_channel_t *channel, char *varname);

/*!
 * Start iterating over the entries in the channel variable list.
 * @param channel the channel to intterate the variales for
 * @param pool The pool to allocate the switch_hash_index_t iterator. If this
 *          pool is NULL, then an internal, non-thread-safe iterator is used.
 * @remark  Use switch_hash_next and switch_hash_this with this function to iterate all the channel variables
 */
SWITCH_DECLARE(switch_hash_index_t *) switch_channel_variable_first(switch_channel_t *channel, switch_memory_pool_t *pool);

/*!
  \brief Assign a caller extension to a given channel
  \param channel channel to assign extension to
  \param caller_extension extension to assign
*/
SWITCH_DECLARE(void) switch_channel_set_caller_extension(switch_channel_t *channel, switch_caller_extension_t *caller_extension);

/*!
  \brief Retrieve caller extension from a given channel
  \param channel channel to retrieve extension from
  \return the requested extension
*/
SWITCH_DECLARE(switch_caller_extension_t *) switch_channel_get_caller_extension(switch_channel_t *channel);

/*!
  \brief Test for presence of given flag(s) on a given channel
  \param channel channel to test 
  \param flags or'd list of channel flags to test
  \return TRUE if flags were present
*/
SWITCH_DECLARE(int) switch_channel_test_flag(switch_channel_t *channel, switch_channel_flag_t flags);

/*!
  \brief Set given flag(s) on a given channel
  \param channel channel on which to set flag(s)
  \param flags or'd list of flags to set
*/
SWITCH_DECLARE(void) switch_channel_set_flag(switch_channel_t *channel, switch_channel_flag_t flags);

/*!
  \brief Set given flag(s) on a given channel's bridge partner
  \param channel channel to derive the partner channel to set flag(s) on
  \param flags or'd list of flags to set
  \return true if the flag was set
*/
SWITCH_DECLARE(switch_bool_t) switch_channel_set_flag_partner(switch_channel_t *channel, switch_channel_flag_t flags);

/*!
  \brief Clears given flag(s) on a given channel's bridge partner
  \param channel channel to derive the partner channel to clear flag(s) from
  \param flags the flags to clear
  \return true if the flag was cleared
*/
SWITCH_DECLARE(switch_bool_t) switch_channel_clear_flag_partner(switch_channel_t *channel, switch_channel_flag_t flags);

/*!
  \brief Set given flag(s) on a given channel to be applied on the next state change
  \param channel channel on which to set flag(s)
  \param flags or'd list of flags to set
*/
SWITCH_DECLARE(void) switch_channel_set_state_flag(switch_channel_t *channel, switch_channel_flag_t flags);

/*!
  \brief Clear given flag(s) from a channel
  \param channel channel to clear flags from
  \param flags or'd list of flags to clear
*/
SWITCH_DECLARE(void) switch_channel_clear_flag(switch_channel_t *channel, switch_channel_flag_t flags);

SWITCH_DECLARE(switch_status_t) switch_channel_perform_answer(switch_channel_t *channel, const char *file, const char *func, int line);

SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_answered(switch_channel_t *channel, const char *file, const char *func, int line);

/*!
  \brief Answer a channel (initiate/acknowledge a successful connection)
  \param channel channel to answer
  \return SWITCH_STATUS_SUCCESS if channel was answered successfully
*/
#define switch_channel_answer(channel) switch_channel_perform_answer(channel, __FILE__, __SWITCH_FUNC__, __LINE__)

/*!
  \brief Mark a channel answered with no indication (for outbound calls)
  \param channel channel to mark answered
  \return SWITCH_STATUS_SUCCESS if channel was answered successfully
*/
#define switch_channel_mark_answered(channel) switch_channel_perform_mark_answered(channel, __FILE__, __SWITCH_FUNC__, __LINE__)

/*!
  \brief Mark a channel pre_answered (early media) with no indication (for outbound calls)
  \param channel channel to mark pre_answered
  \return SWITCH_STATUS_SUCCESS if channel was pre_answered successfully
*/
#define switch_channel_mark_pre_answered(channel) switch_channel_perform_mark_pre_answered(channel, __FILE__, __SWITCH_FUNC__, __LINE__)

SWITCH_DECLARE(switch_status_t) switch_channel_perform_ring_ready(switch_channel_t *channel, const char *file, const char *func, int line);
/*!
  \brief Send Ringing message to a channel
  \param channel channel to ring
  \return SWITCH_STATUS_SUCCESS if successful
*/
#define switch_channel_ring_ready(channel) switch_channel_perform_ring_ready(channel, __FILE__, __SWITCH_FUNC__, __LINE__)


SWITCH_DECLARE(switch_status_t) switch_channel_perform_pre_answer(switch_channel_t *channel, const char *file, const char *func, int line);

SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_pre_answered(switch_channel_t *channel, const char *file, const char *func, int line);

SWITCH_DECLARE(switch_status_t) switch_channel_perform_mark_ring_ready(switch_channel_t *channel, const char *file, const char *func, int line);

/*!
  \brief Indicate progress on a channel to attempt early media
  \param channel channel to pre-answer
  \return SWITCH_STATUS_SUCCESS
*/
#define switch_channel_pre_answer(channel) switch_channel_perform_pre_answer(channel, __FILE__, __SWITCH_FUNC__, __LINE__)

/*!
  \brief Indicate a channel is ready to provide ringback
  \param channel channel
  \return SWITCH_STATUS_SUCCESS
*/
#define switch_channel_mark_ring_ready(channel) switch_channel_perform_mark_ring_ready(channel, __FILE__, __SWITCH_FUNC__, __LINE__)

/*!
  \brief add a state handler table to a given channel
  \param channel channel on which to add the state handler table
  \param state_handler table of state handler functions
  \return the index number/priority of the table negative value indicates failure
*/
SWITCH_DECLARE(int) switch_channel_add_state_handler(switch_channel_t *channel, const switch_state_handler_table_t *state_handler);

/*!
  \brief clear a state handler table from a given channel
  \param channel channel from which to clear the state handler table
  \param state_handler table of state handler functions
*/
SWITCH_DECLARE(void) switch_channel_clear_state_handler(switch_channel_t *channel, const switch_state_handler_table_t *state_handler);

/*!
  \brief Retrieve an state handler tablefrom a given channel at given index level
  \param channel channel from which to retrieve the state handler table
  \param index the index of the state handler table (start from 0)
  \return given channel's state handler table at given index or NULL if requested index does not exist.
*/
SWITCH_DECLARE(const switch_state_handler_table_t *) switch_channel_get_state_handler(switch_channel_t *channel, int index);

/*!
  \brief Set private data on channel
  \param channel channel on which to set data
  \param key unique keyname to associate your private data to
  \param private_info void pointer to private data
  \return SWITCH_STATUS_SUCCESS if data was set
*/
SWITCH_DECLARE(switch_status_t) switch_channel_set_private(switch_channel_t *channel, char *key, void *private_info);

/*!
  \brief Retrieve private from a given channel
  \param channel channel to retrieve data from
  \param key unique keyname to retrieve your private data
  \return void pointer to channel's private data
*/
SWITCH_DECLARE(void *) switch_channel_get_private(switch_channel_t *channel, char *key);

/*!
  \brief Assign a name to a given channel
  \param channel channel to assign name to
  \param name name to assign
  \return SWITCH_STATUS_SUCCESS if name was assigned
*/
SWITCH_DECLARE(switch_status_t) switch_channel_set_name(switch_channel_t *channel, char *name);

/*!
  \brief Retrieve the name of a given channel
  \param channel channel to get name of
  \return the channel's name
*/
SWITCH_DECLARE(char *) switch_channel_get_name(switch_channel_t *channel);


SWITCH_DECLARE(switch_channel_state_t) switch_channel_perform_hangup(switch_channel_t *channel,
																	 const char *file, const char *func, int line, switch_call_cause_t hangup_cause);

/*!
  \brief Hangup a channel flagging it's state machine to end
  \param channel channel to hangup
  \param hangup_cause the appropriate hangup cause
  \return the resulting channel state.
*/
#define switch_channel_hangup(channel, hangup_cause) switch_channel_perform_hangup(channel, __FILE__, __SWITCH_FUNC__, __LINE__, hangup_cause)

/*!
  \brief Test for presence of DTMF on a given channel
  \param channel channel to test
  \return number of digits in the queue
*/
SWITCH_DECLARE(switch_size_t) switch_channel_has_dtmf(switch_channel_t *channel);

/*!
  \brief Queue DTMF on a given channel
  \param channel channel to queue DTMF to
  \param dtmf string of digits to queue
  \return SWITCH_STATUS_SUCCESS if successful
*/
SWITCH_DECLARE(switch_status_t) switch_channel_queue_dtmf(switch_channel_t *channel, char *dtmf);

/*!
  \brief Retrieve DTMF digits from a given channel
  \param channel channel to retrieve digits from
  \param dtmf buffer to write dtmf to
  \param len max size in bytes of the buffer
  \return number of bytes read into the buffer
*/
SWITCH_DECLARE(switch_size_t) switch_channel_dequeue_dtmf(switch_channel_t *channel, char *dtmf, switch_size_t len);

/*!
  \brief Render the name of the provided state enum
  \param state state to get name of
  \return the string representation of the state
*/
SWITCH_DECLARE(const char *) switch_channel_state_name(switch_channel_state_t state);

/*!
  \brief Render the enum of the provided state name
  \param name the name of the state
  \return the enum value (numeric)
*/
SWITCH_DECLARE(switch_channel_state_t) switch_channel_name_state(char *name);

/*!
  \brief Add information about a given channel to an event object
  \param channel channel to add information about
  \param event event to add information to
*/
SWITCH_DECLARE(void) switch_channel_event_set_data(switch_channel_t *channel, switch_event_t *event);

/*!
  \brief Expand varaibles in a string based on the variables in a paticular channel
  \param channel channel to expand the variables from
  \param in the original string
  \return the original string if no expansion takes place otherwise a new string that must be freed
  \note it's necessary to test if the return val is the same as the input and free the string if it is not.
*/
SWITCH_DECLARE(char *) switch_channel_expand_variables(switch_channel_t *channel, char *in);

/** @} */

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
