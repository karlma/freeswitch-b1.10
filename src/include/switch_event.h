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
 * switch_event.h -- Event System
 *
 */
/*! \file switch_event.h
    \brief Event System
	
	The event system uses a backend thread and an APR threadsafe FIFO queue to accept event objects from various threads
	and allow the backend to take control and deliver the events to registered callbacks.

	The typical usage would be to bind to one or all of the events and use a callback function to react in various ways
	(see the more_xmpp_event_handler or mod_event_test modules for examples).

	Builtin events are fired by the core at various points in the execution of the application and custom events can be 
	reserved and registered so events from an external module can be rendered and handled by an another even handler module.

	If the work time to process an event in a callback is anticipated to grow beyond a very small amount of time it is reccommended
	that you impelment your own handler thread and FIFO queue so you can accept the events int the callback and queue them 
	into your own thread rather than tie up the delivery agent.  It is in to opinion of the author that such a necessity 
	should be judged on a per-use basis and therefore does not fall within the scope of this system to provide that 
	functionality at a core level.

*/

/*!
  \defgroup events Eventing Engine
  \ingroup FREESWITCH
  \{ 
*/

#ifndef SWITCH_EVENT_H
#define SWITCH_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

/*! \brief An event Header */
struct switch_event_header {
	/*! the header name */
	char *name;
	/*! the header value */
	char *value;
	struct switch_event_header *next;
};

/*! \brief A registered custom event subclass  */
struct switch_event_subclass {
	/*! the owner of the subclass */
	char *owner;
	/*! the subclass name */
	char *name;
};

/*! \brief Representation of an event */
struct switch_event {
	/*! the event id (descriptor) */
	switch_event_t event_id;
	/*! the priority of the event */
	switch_priority_t priority;
	/*! the owner of the event */
	char *owner;
	/*! the subclass of the event */
	switch_event_subclass *subclass;
	/*! the event headers */
	struct switch_event_header *headers;
	/*! the body of the event */
	char *body;
	/*! user data from the subclass provider */
	void *bind_user_data;
	/*! user data from the event sender */
	void *event_user_data;
	struct switch_event *next;
};

/*! \brief A node to store binded events */
struct switch_event_node {
	/*! the id of the node */
	char *id;
	/*! the event id enumeration to bind to */
	switch_event_t event_id;
	/*! the event subclass to bind to for custom events */
	switch_event_subclass *subclass;
	/*! a callback function to execute when the event is triggered */
	switch_event_callback_t callback;
	/*! private data */
	void *user_data;
	struct switch_event_node *next;
};

#define SWITCH_EVENT_SUBCLASS_ANY NULL

/*!
  \brief Start the eventing system
  \param pool the memory pool to use for the event system (creates a new one if NULL)
  \return SWITCH_STATUS_SUCCESS when complete
*/
SWITCH_DECLARE(switch_status) switch_event_init(switch_memory_pool *pool);

/*!
  \brief Stop the eventing system
  \return SWITCH_STATUS_SUCCESS when complete
*/
SWITCH_DECLARE(switch_status) switch_event_shutdown(void);

/*!
  \brief Create an event
  \param event a NULL pointer on which to create the event
  \param event_id the event id enumeration of the desired event
  \param subclass_name the subclass name for custom event (only valid when event_id is SWITCH_EVENT_CUSTOM)
  \return SWITCH_STATUS_SUCCESS on success
*/
SWITCH_DECLARE(switch_status) switch_event_create_subclass(switch_event **event, switch_event_t event_id, char *subclass_name);

/*!
  \brief Set the priority of an event
  \param event the event to set the priority on
  \param priority the event priority
  \return SWITCH_STATUS_SUCCESS
*/
SWITCH_DECLARE(switch_status) switch_event_set_priority(switch_event *event, switch_priority_t priority);

/*!
  \brief Retrieve a header value from an event
  \param event the event to read the header from
  \param header_name the name of the header to read
  \return the value of the requested header
*/
SWITCH_DECLARE(char *) switch_event_get_header(switch_event *event, char *header_name);

/*!
  \brief Add a header to an event
  \param event the event to add the header to
  \param stack the stack sense (stack it on the top or on the bottom)
  \param header_name the name of the header to add
  \param fmt the value of the header (varargs see standard sprintf family)
  \return SWITCH_STATUS_SUCCESS if the header was added
*/
SWITCH_DECLARE(switch_status) switch_event_add_header(switch_event *event, switch_stack_t stack, char *header_name, char *fmt, ...);

/*!
  \brief Destroy an event
  \param event pointer to the pointer to event to destroy
*/
SWITCH_DECLARE(void) switch_event_destroy(switch_event **event);

/*!
  \brief Duplicate an event
  \param event a NULL pointer on which to duplicate the event
  \param todup an event to duplicate
  \return SWITCH_STATUS_SUCCESS if the event was duplicated
*/
SWITCH_DECLARE(switch_status) switch_event_dup(switch_event **event, switch_event *todup);

/*!
  \brief Fire an event with full arguement list
  \param file the calling file
  \param func the calling function
  \param line the calling line number
  \param event the event to send (will be nulled on success)
  \param user_data optional private data to pass to the event handlers
  \return
*/
SWITCH_DECLARE(switch_status) switch_event_fire_detailed(char *file, char *func, int line, switch_event **event, void *user_data);

/*!
  \brief Bind an event callback to a specific event
  \param id an identifier token of the binder
  \param event the event enumeration to bind to
  \param subclass_name the event subclass to bind to in the case if SWITCH_EVENT_CUSTOM
  \param callback the callback functon to bind
  \param user_data optional user specific data to pass whenever the callback is invoked
  \return SWITCH_STATUS_SUCCESS if the event was binded
*/
SWITCH_DECLARE(switch_status) switch_event_bind(char *id, switch_event_t event, char *subclass_name, switch_event_callback_t callback, void *user_data);

/*!
  \brief Render the name of an event id enumeration
  \param event the event id to render the name of
  \return the rendered name
*/
SWITCH_DECLARE(char *) switch_event_name(switch_event_t event);

/*!
  \brief Reserve a subclass name for private use with a custom event
  \param owner the owner of the event name
  \param subclass_name the name to reserve
  \return SWITCH_STATUS_SUCCESS if the name was reserved
  \note There is nothing to enforce this but I reccommend using module::event_name for the subclass names

*/
SWITCH_DECLARE(switch_status) switch_event_reserve_subclass_detailed(char *owner, char *subclass_name);

/*!
  \brief Render a string representation of an event sutable for printing or network transport 
  \param event the event to render
  \param buf a string buffer to write the data to
  \param buflen the size in bytes of the buffer
  \param fmt optional body of the event (varargs see standard sprintf family)
  \return SWITCH_STATUS_SUCCESS if the operation was successful
  \note the body supplied by this function will supersede an existing body the event may have
*/
SWITCH_DECLARE(switch_status) switch_event_serialize(switch_event *event, char *buf, size_t buflen, char *fmt, ...);

/*!
  \brief Determine if the event system has been initilized
  \return SWITCH_STATUS_SUCCESS if the system is running
*/
SWITCH_DECLARE(switch_status) switch_event_running(void);

/*!
  \brief Add a body to an event
  \param event the event to add to body to
  \param fmt optional body of the event (varargs see standard sprintf family)
  \return SWITCH_STATUS_SUCCESS if the body was added to the event
  \note the body parameter can be shadowed by the switch_event_reserve_subclass_detailed function
*/
SWITCH_DECLARE(switch_status) switch_event_add_body(switch_event *event, char *fmt, ...);


/*!
  \brief Reserve a subclass assuming the owner string is the current filename
  \param subclass_name the subclass name to reserve
  \return SWITCH_STATUS_SUCCESS if the operation was successful
  \note the body supplied by this function will supersede an existing body the event may have
*/
#define switch_event_reserve_subclass(subclass_name) switch_event_reserve_subclass_detailed(__FILE__, subclass_name)

/*!
  \brief Create a new event assuming it will not be custom event and therefore hiding the unused parameters
  \param event a NULL pointer on which to create the event
  \param id the event id enumeration of the desired event
  \return SWITCH_STATUS_SUCCESS on success
*/
#define switch_event_create(event, id) switch_event_create_subclass(event, id, SWITCH_EVENT_SUBCLASS_ANY)

/*!
  \brief Deliver an event to all of the registered event listeners
  \param event the event to send (will be nulled)
  \note normaly use switch_event_fire for delivering events (only use this when you wish to deliver the event blocking on your thread)
*/
SWITCH_DECLARE(void) switch_event_deliver(switch_event **event);

/*!
  \brief Fire an event filling in most of the arguements with obvious values
  \param event the event to send (will be nulled on success)
  \return SWITCH_STATUS_SUCCESS if the operation was successful
  \note the body supplied by this function will supersede an existing body the event may have
*/
#define switch_event_fire(event) switch_event_fire_detailed(__FILE__, (char * )__FUNCTION__, __LINE__, event, NULL)

/*!
  \brief Fire an event filling in most of the arguements with obvious values and allowing user_data to be sent
  \param event the event to send (will be nulled on success)
  \param data user data to send to the event handlers
  \return SWITCH_STATUS_SUCCESS if the operation was successful
  \note the body supplied by this function will supersede an existing body the event may have
*/
#define switch_event_fire_data(event, data) switch_event_fire_detailed(__FILE__, (char * )__FUNCTION__, __LINE__, event, data)
///\}

#endif
