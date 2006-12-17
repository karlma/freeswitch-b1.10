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
 * switch_platform.h -- Platform Specific Header
 *
 */
/*! \file switch_platform.h
    \brief Platform Specific Header
*/
#ifndef SWITCH_PLATFORM_H
#define SWITCH_PLATFORM_H

SWITCH_BEGIN_EXTERN_C

#ifdef __ICC
#pragma warning (disable:810 869 981 279 1469 188)
#endif

#include <stdio.h>

#ifdef _MSC_VER


/* disable the following warnings 
 * C4152: non standard extension, function/data ptr conversion in expression
 * C4054: A function pointer is cast (possibly incorrectly) to a data pointer.
 * C4100: The formal parameter is not referenced in the body of the function. The unreferenced parameter is ignored. 
 * C4142: A type is redefined in a manner that has no effect on the generated code.
 * C4200: Non standard extension C zero sized array
 * C4204: nonstandard extension used : non-constant aggregate initializer 
 * C4706: assignment within conditional expression
 * C4055: type cast from data pointer to function pointer (apr carries function pointers as data pointers)
 * C4214: nonstandard extension used : bit field types other than int
 * C4819: The file contains a character that cannot be represented in the current code page
 * C4132: 'object' : const object should be initialized (fires innapropriately for prototyped forward declaration of cost var)
 */
#pragma warning(disable:4152 4054 4100 4142 4200 4204 4706 4055 4214 4819 4132 4510 4512 4610)

#if (_MSC_VER >= 1400) // VC8+
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#endif // VC8+

#if  _MSC_VER < 1300
#ifndef __FUNCTION__
#define __FUNCTION__ ""
#endif
#endif

#undef inline
#define inline __inline

#ifndef uint32_t
typedef unsigned __int8		uint8_t;
typedef unsigned __int16	uint16_t;
typedef unsigned __int32	uint32_t;
typedef unsigned __int64    uint64_t;
typedef __int8		int8_t;
typedef __int16		int16_t;
typedef __int32		int32_t;
typedef __int64		int64_t;
typedef unsigned long	in_addr_t;
#endif
#define PACKED
#include <io.h>
#else
/* packed attribute */
#ifndef PACKED
#define PACKED __attribute__ ((__packed__))
#endif
#include <limits.h>
#include <inttypes.h>
#include <sys/types.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#endif // _MSC_VER

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 4321
#endif
#ifndef __BYTE_ORDER
#ifdef SWITCH_BYTE_ORDER
#define __BYTE_ORDER SWITCH_BYTE_ORDER
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#endif

#ifdef WIN32
#if defined(SWITCH_CORE_DECLARE_STATIC)
#define SWITCH_DECLARE(type)			type __stdcall
#define SWITCH_DECLARE_NONSTD(type)		type __cdecl
#define SWITCH_DECLARE_DATA
#elif defined(FREESWITCHCORE_EXPORTS)
#define SWITCH_DECLARE(type)			__declspec(dllexport) type __stdcall
#define SWITCH_DECLARE_NONSTD(type)		__declspec(dllexport) type __cdecl
#define SWITCH_DECLARE_DATA				__declspec(dllexport)
#else
#define SWITCH_DECLARE(type)			__declspec(dllimport) type __stdcall
#define SWITCH_DECLARE_NONSTD(type)		__declspec(dllimport) type __cdecl
#define SWITCH_DECLARE_DATA				__declspec(dllimport)
#endif

#if defined(SWITCH_MOD_DECLARE_STATIC)
#define SWITCH_MOD_DECLARE(type)		type __cdecl
#elif defined(MOD_EXPORTS)
#define SWITCH_MOD_DECLARE(type)		__declspec(dllexport) type __cdecl
#else
#define SWITCH_MOD_DECLARE(type)		__declspec(dllimport) type __cdecl
#endif
#define SIGHUP SIGTERM
#else //not win32
#define SWITCH_DECLARE(type) type
#define SWITCH_DECLARE_NONSTD(type) type
#define SWITCH_MOD_DECLARE(type) type
#define SWITCH_DECLARE_DATA
#endif

#ifdef DOXYGEN
#define DoxyDefine(x) x
#else
#define DoxyDefine(x)
#endif

SWITCH_END_EXTERN_C

#endif

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:
 */
