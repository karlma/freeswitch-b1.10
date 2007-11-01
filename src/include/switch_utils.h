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
 * switch_utils.h -- Compatability and Helper Code
 *
 */
/*! \file switch_utils.h
    \brief Compatability and Helper Code

	Just a miscelaneaous set of general utility/helper functions.

*/
#ifndef SWITCH_UTILS_H
#define SWITCH_UTILS_H

#include <switch.h>

SWITCH_BEGIN_EXTERN_C
#define switch_bytes_per_frame(rate, interval) ((uint32_t)((float)rate / (1000.0f / (float)interval)))
#define SWITCH_SMAX 32767
#define SWITCH_SMIN -32768
#define switch_normalize_to_16bit(n) if (n > SWITCH_SMAX) n = SWITCH_SMAX / 2; else if (n < SWITCH_SMIN) n = SWITCH_SMIN / 2;
#define switch_codec2str(codec,buf,len) snprintf(buf, len, "%s@%uk@%ui", \
                                                 codec->implementation->iananame, \
                                                 codec->implementation->samples_per_second, \
                                                 codec->implementation->microseconds_per_frame / 1000)
#ifdef WIN32
#define switch_is_file_path(file) (*file == '\\' || *(file +1) == ':' || *file == '/' || strstr(file, SWITCH_URL_SEPARATOR))
#else
#define switch_is_file_path(file) ((*file == '/') || strstr(file, SWITCH_URL_SEPARATOR))
#endif

SWITCH_DECLARE(switch_status_t) switch_b64_encode(unsigned char *in, switch_size_t ilen, unsigned char *out, switch_size_t olen);

static inline switch_bool_t switch_is_digit_string(const char *s) {

	while(s && *s) {
		if (*s < 48 || *s > 57) {
			return SWITCH_FALSE;
		}
		s++;
	}

	return SWITCH_TRUE;
}

/*!
  \brief Evaluate the truthfullness of a string expression
  \param expr a string expression
  \return true or false 
*/
#define switch_true(expr)\
(expr && ( !strcasecmp(expr, "yes") ||\
!strcasecmp(expr, "on") ||\
!strcasecmp(expr, "true") ||\
!strcasecmp(expr, "enabled") ||\
!strcasecmp(expr, "active") ||\
atoi(expr))) ? SWITCH_TRUE : SWITCH_FALSE
/*!
  \brief find local ip of the box
  \param buf the buffer to write the ip adress found into
  \param len the length of the buf
  \param family the address family to return (AF_INET or AF_INET6)
  \return SWITCH_STATUS_SUCCESSS for success, otherwise failure
*/
SWITCH_DECLARE(switch_status_t) switch_find_local_ip(char *buf, int len, int family);

/*!
  \brief find the char representation of an ip adress
  \param buf the buffer to write the ip adress found into
  \param len the length of the buf
  \param in the struct in_addr * to get the adress from
  \return the ip adress string
*/
SWITCH_DECLARE(char *) get_addr(char *buf, switch_size_t len, struct in_addr *in);

#define SWITCH_STATUS_IS_BREAK(x) (x == SWITCH_STATUS_BREAK || x == 730035 || x == 35)

/*!
  \brief Return a printable name of a switch_priority_t
  \param priority the priority to get the name of
  \return the printable form of the priority
*/
SWITCH_DECLARE(const char *) switch_priority_name(switch_priority_t priority);

/*!
  \brief Return the RFC2833 character based on an event id
  \param event the event id to convert
  \return the character represented by the event or null for an invalid event
*/
SWITCH_DECLARE(char) switch_rfc2833_to_char(int event);

/*!
  \brief Return the RFC2833 event based on an key character
  \param key the charecter to encode
  \return the event id for the specified character or -1 on an invalid input
*/
SWITCH_DECLARE(unsigned char) switch_char_to_rfc2833(char key);

/*!
  \brief determine if a character is a valid DTMF key
  \param key the key to test
  \return TRUE or FALSE
 */
#define is_dtmf(key)  ((key > 47 && key < 58) || (key > 64 && key < 69) || (key > 96 && key < 101) || key == 35 || key == 42 || key == 87 || key == 119)


/*!
  \brief Test for the existance of a flag on an arbitary object
  \param obj the object to test
  \param flag the or'd list of flags to test
  \return true value if the object has the flags defined
*/
#define switch_test_flag(obj, flag) ((obj)->flags & flag)

/*!
  \brief Set a flag on an arbitrary object
  \param obj the object to set the flags on
  \param flag the or'd list of flags to set
*/
#define switch_set_flag(obj, flag) (obj)->flags |= (flag)

/*!
  \brief Set a flag on an arbitrary object while locked
  \param obj the object to set the flags on
  \param flag the or'd list of flags to set
*/
#define switch_set_flag_locked(obj, flag) assert(obj->flag_mutex != NULL);\
switch_mutex_lock(obj->flag_mutex);\
(obj)->flags |= (flag);\
switch_mutex_unlock(obj->flag_mutex);

/*!
  \brief Clear a flag on an arbitrary object
  \param obj the object to test
  \param flag the or'd list of flags to clear
*/
#define switch_clear_flag_locked(obj, flag) switch_mutex_lock(obj->flag_mutex); (obj)->flags &= ~(flag); switch_mutex_unlock(obj->flag_mutex);

/*!
  \brief Clear a flag on an arbitrary object while locked
  \param obj the object to test
  \param flag the or'd list of flags to clear
*/
#define switch_clear_flag(obj, flag) (obj)->flags &= ~(flag)

/*!
  \brief Copy flags from one arbitrary object to another
  \param dest the object to copy the flags to
  \param src the object to copy the flags from
  \param flags the flags to copy
*/
#define switch_copy_flags(dest, src, flags) (dest)->flags &= ~(flags);	(dest)->flags |= ((src)->flags & (flags))

#define switch_set_string(_dst, _src) switch_copy_string(_dst, _src, sizeof(_dst))

static inline char *switch_clean_string(char *s)
{
	char *p;
	for (p = s; p && *p; p++) {
		uint8_t x = (uint8_t) *p;
		if ((x < 32 || x > 127) && x != '\n' && x != '\r') {
			*p = ' ';
		}
	}

	return s;
}



/*!
  \brief Free a pointer and set it to NULL unless it already is NULL
  \param it the pointer
*/
#define switch_safe_free(it) if (it) {free(it);it=NULL;}


/*!
  \brief Test if one string is inside another with extra case checking
  \param s the inner string
  \param q the big string
  \return SWITCH_TRUE or SWITCH_FALSE
*/
static inline switch_bool_t switch_strstr(char *s, char *q)
{
	char *p, *S = NULL, *Q = NULL;
	switch_bool_t tf = SWITCH_FALSE;

	if (strstr(s, q)) {
		return SWITCH_TRUE;
	}

	S = strdup(s);
	
	assert(S != NULL);

	for (p = S; p && *p; p++) {
		*p = (char)toupper(*p);
	}

	if (strstr(S, q)) {
		tf = SWITCH_TRUE;
		goto done;
	}

	Q = strdup(q);
	assert(Q != NULL);

	for (p = Q; p && *p; p++) {
		*p = (char)toupper(*p);
	}
	
	if (strstr(s, Q)) {
		tf = SWITCH_TRUE;
		goto done;
	}

	if (strstr(S, Q)) {
		tf = SWITCH_TRUE;
		goto done;
	}
	
 done:
	switch_safe_free(S);
	switch_safe_free(Q);

	return tf;
}


/*!
  \brief Test for NULL or zero length string
  \param s the string to test
  \return true value if the string is NULL or zero length
*/
#define switch_strlen_zero(s) (!s || *s == '\0')

/*!
  \brief Make a null string a blank string instead
  \param s the string to test
  \return the original string or blank string.
*/
#define switch_str_nil(s) (s ? s : "")

/*!
  \brief Wait a desired number of microseconds and yield the CPU
*/
#define switch_yield(ms) switch_sleep(ms);

/*!
  \brief Converts a string representation of a date into a switch_time_t
  \param in the string
  \return the epoch time in usec
*/
SWITCH_DECLARE(switch_time_t) switch_str_time(const char *in);

/*!
  \brief Declares a function designed to set a dymaic global string
  \param fname the function name to declare
  \param vname the name of the global pointer to modify with the new function
*/
#define SWITCH_DECLARE_GLOBAL_STRING_FUNC(fname, vname) static void fname(char *string) { if (!string) return;\
		if (vname) {free(vname); vname = NULL;}vname = strdup(string);} static void fname(char *string)

/*!
  \brief Separate a string into an array based on a character delimeter
  \param buf the string to parse
  \param delim the character delimeter
  \param array the array to split the values into
  \param arraylen the max number of elements in the array
  \return the number of elements added to the array
*/
SWITCH_DECLARE(unsigned int) switch_separate_string(char *buf, char delim, char **array, int arraylen);

SWITCH_DECLARE(const char *) switch_stristr(const char *str, const char *instr);

/*!
  \brief Escape a string by prefixing a list of characters with an escape character
  \param pool a memory pool to use
  \param in the string
  \param delim the list of characters to escape
  \param esc the escape character
  \return the escaped string
*/
SWITCH_DECLARE(char *) switch_escape_char(switch_memory_pool_t *pool, char *in, const char *delim, char esc);

/*!
  \brief Wait for a socket
  \param poll the pollfd to wait on
  \param ms the number of milliseconds to wait
  \return the requested condition
*/
SWITCH_DECLARE(int) switch_socket_waitfor(switch_pollfd_t * poll, int ms);

/*!
  \brief Create a pointer to the file name in a given file path eliminating the directory name
  \return the pointer to the next character after the final / or \\ characters
*/
SWITCH_DECLARE(const char *) switch_cut_path(const char *in);

SWITCH_DECLARE(char *) switch_string_replace(const char *string, const char *search, const char *replace);
SWITCH_DECLARE(switch_status_t) switch_string_match(const char *string, size_t string_len, const char *search, size_t search_len);

#define SWITCH_READ_ACCEPTABLE(status) (status == SWITCH_STATUS_SUCCESS || status == SWITCH_STATUS_BREAK)
SWITCH_DECLARE(size_t) switch_url_encode(const char *url, char *buf, size_t len);
SWITCH_DECLARE(char *) switch_url_decode(char *s);
SWITCH_DECLARE(switch_bool_t) switch_simple_email(char *to, char *from, char *headers, char *body, char *file);

/* malloc or DIE macros */
#ifdef NDEBUG
#define switch_malloc(ptr, len) (void)( (!!(ptr = malloc(len))) || (fprintf(stderr,"ABORT! Malloc failure at: %s:%s", __FILE__, __LINE__),abort(), 0), ptr )
#define switch_zmalloc(ptr, len) (void)( (!!(ptr = malloc(len))) || (fprintf(stderr,"ABORT! Malloc failure at: %s:%s", __FILE__, __LINE__),abort(), 0), memset(ptr, 0, len))
#else
#define switch_malloc(ptr, len) (void)(assert(((ptr) = malloc((len)))),ptr)
#define switch_zmalloc(ptr, len) (void)(assert((ptr = malloc(len))),memset(ptr, 0, len))
#endif

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
