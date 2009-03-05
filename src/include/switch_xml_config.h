/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2009, Anthony Minessale II <anthm@freeswitch.org>
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
 * Mathieu Rene <mathieu.rene@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Mathieu Rene <mathieu.rene@gmail.com>
 *
 *
 * switch_xml_config.h - Generic configuration parser
 *
 */

#ifndef SWITCH_XML_CONFIG_H
#define SWITCH_XML_CONFIG_H

/*! \brief Type of value to parse */
typedef enum {
	SWITCH_CONFIG_INT,			/*< (ptr=int* default=int data=NULL) Integer */
	SWITCH_CONFIG_STRING, 		/*< (ptr=[char* or char ** (for alloc)] default=char* data=switch_xml_config_string_options_t*) Zero-terminated C-string */
	SWITCH_CONFIG_YESNO, 		/*< (ptr=switch_bool_t* default=switch_bool_t data=NULL) Yes and no */
	SWITCH_CONFIG_CUSTOM, 		/*< (ptr=<custom function data> default=<custom function data> data=switch_xml_config_callback_t) Custom, get value through function pointer  */
	SWITCH_CONFIG_ENUM, 		/*< (ptr=int* default=int data=switch_xml_config_enum_item_t*) */
	SWITCH_CONFIG_FLAG,			/*< (ptr=int32_t* default=switch_bool_t data=int (flag index) */
	SWITCH_CONFIG_FLAGARRAY,	/*< (ptr=int8_t* default=switch_bool_t data=int (flag index) */
	
	/* No more past that line */
	SWITCH_CONFIG_LAST
} switch_xml_config_type_t;

typedef struct {
	char *key;					/*< The item's key or NULL if this is the last one in the list */
	switch_size_t value;		/*< The item's value */
} switch_xml_config_enum_item_t;

typedef struct {
	switch_memory_pool_t *pool; /*< If set, the string will be allocated on the pool (unless the length param is > 0, then you misread this file)*/
	int length;					/*< Length of the char array, or 0 if memory has to be allocated dynamically*/
} switch_xml_config_string_options_t;

struct switch_xml_config_item;
typedef struct switch_xml_config_item switch_xml_config_item_t;

typedef switch_status_t (*switch_xml_config_callback_t)(switch_xml_config_item_t *data);

/*!
 * \brief A configuration instruction read by switch_xml_config_parse 
*/
struct switch_xml_config_item {
	char *key;						/*< The key of the element, or NULL to indicate the end of the list */
	switch_xml_config_type_t type; 	/*< The type of variable */
	switch_bool_t reloadable; 		/*< True if the var can be changed on reload */
	void *ptr;						/*< Ptr to the var to be changed */
	void *defaultvalue; 			/*< Default value */
	void *data; 					/*< Custom data (depending on the type) */
	switch_xml_config_callback_t function;	/*< Callback (for type CUSTOM) */
} ;


#define SWITCH_CONFIG_ITEM (_key, _type, _reloadable, _ptr, _defaultvalue, _data)	{ _key, _type, _reloadable, _ptr, _defaultvalue, _data, NULL }
#define SWITCH_CONFIG_ITEM_CALLBACK (_key, _type, _reloadable, _ptr, _defaultvalue, _data)	{ _key, _type, _reloadable, _ptr, _defaultvalue, NULL, _data }
#define SWITCH_CONFIG_ITEM_END () { NULL, SWITCH_CONFIG_LAST, 0, NULL ,NULL, NULL }

/*! 
 * \brief Parses all the xml elements, following a ruleset defined by an array of switch_xml_config_item_t 
 * \param xml The first element of the list to parse
 * \param reload true to skip all non-reloadable options
 * \param options instrutions on how to parse the elements
 * \see switch_xml_config_item_t
 */
switch_status_t switch_xml_config_parse(switch_xml_t xml, int reload, switch_xml_config_item_t *options);

#endif /* !defined(SWITCH_XML_CONFIG_H) */

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
