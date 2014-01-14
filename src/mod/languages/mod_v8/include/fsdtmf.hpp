/*
 * mod_v8 for FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2013-2014, Peter Olsson <peter@olssononline.se>
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
 * The Original Code is mod_v8 for FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Peter Olsson <peter@olssononline.se>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Peter Olsson <peter@olssononline.se>
 *
 * fsdtmf.hpp -- JavaScript DTMF class header
 *
 */

#ifndef FS_DTMF_H
#define FS_DTMF_H

#include "javascript.hpp"
#include <switch.h>

/* Macros for easier V8 callback definitions */
#define JS_DTMF_GET_PROPERTY_DEF(method_name) JS_GET_PROPERTY_DEF(method_name, FSDTMF)
#define JS_DTMF_SET_PROPERTY_DEF(method_name) JS_SET_PROPERTY_DEF(method_name, FSDTMF)
#define JS_DTMF_FUNCTION_DEF(method_name) JS_FUNCTION_DEF(method_name, FSDTMF)
#define JS_DTMF_GET_PROPERTY_IMPL(method_name) JS_GET_PROPERTY_IMPL(method_name, FSDTMF)
#define JS_DTMF_SET_PROPERTY_IMPL(method_name) JS_SET_PROPERTY_IMPL(method_name, FSDTMF)
#define JS_DTMF_FUNCTION_IMPL(method_name) JS_FUNCTION_IMPL(method_name, FSDTMF)

class FSDTMF : public JSBase
{
private:
	switch_dtmf_t *_dtmf;
public:
	FSDTMF(JSMain *owner) : JSBase(owner) { _dtmf = NULL; }
	FSDTMF(const v8::FunctionCallbackInfo<v8::Value>& info) : JSBase(info) { _dtmf = NULL; }
	virtual ~FSDTMF(void);
	virtual std::string GetJSClassName();

	static const js_class_definition_t *GetClassDefinition();

	/* Methods available from JavaScript */
	static void *Construct(const v8::FunctionCallbackInfo<v8::Value>& info);
	static v8::Handle<v8::Object> New(switch_dtmf_t *dtmf, const char *name, JSMain *js);
	JS_DTMF_GET_PROPERTY_DEF(GetProperty);
};

#endif /* FS_DTMF_H */

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
