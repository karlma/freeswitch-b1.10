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
 * mod_spidermonkey.c -- Javascript Module
 *
 */
#ifndef HAVE_CURL
#define HAVE_CURL
#endif
#include "mod_spidermonkey.h"

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

static const char modname[] = "mod_spidermonkey";


static void session_destroy(JSContext * cx, JSObject * obj);
static JSBool session_construct(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
static JSBool session_originate(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
static switch_api_interface_t js_run_interface;

static struct {
	size_t gStackChunkSize;
	jsuword gStackBase;
	int gExitCode;
	JSBool gQuitting;
	FILE *gErrFile;
	FILE *gOutFile;
	int stackDummy;
	JSRuntime *rt;
} globals;


static JSClass global_class = {
	"Global", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};


static struct {
	switch_hash_t *mod_hash;
	switch_hash_t *load_hash;
	switch_memory_pool_t *pool;
} module_manager;

struct sm_loadable_module {
	char *filename;
	void *lib;
	const sm_module_interface_t *module_interface;
	spidermonkey_init_t spidermonkey_init;
};
typedef struct sm_loadable_module sm_loadable_module_t;

typedef enum {
	S_HUP = (1 << 0),
	S_FREE = (1 << 1)
} session_flag_t;

struct input_callback_state {
	struct js_session *session_state;
	char code_buffer[1024];
	size_t code_buffer_len;
	char ret_buffer[1024];
	int ret_buffer_len;
	int digit_count;
	JSFunction *function;
	jsval arg;
	jsval ret;
	JSContext *cx;
	JSObject *obj;
	jsrefcount saveDepth;
	void *extra;
};

struct fileio_obj {
	char *path;
	unsigned int flags;
	switch_file_t *fd;
	switch_memory_pool_t *pool;
	char *buf;
	switch_size_t buflen;
	int32 bufsize;
};

struct event_obj {
	switch_event_t *event;
	int freed;
};

/* Event Object */
/*********************************************************************************/
static JSBool event_construct(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	if (argc > 0) {
		switch_event_t *event;
		struct event_obj *eo;
		switch_event_types_t etype;
		char *ename = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));

		if ((eo = malloc(sizeof(*eo)))) {

			if (switch_name_event(ename, &etype) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Unknown event %s\n", ename);
				*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
				return JS_TRUE;
			}

			if (etype == SWITCH_EVENT_CUSTOM) {
				char *subclass_name;
				if (argc < 1) {
					*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
					return JS_TRUE;
				}

				subclass_name = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
				if (switch_event_create_subclass(&event, etype, subclass_name) != SWITCH_STATUS_SUCCESS) {
					*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
					return JS_TRUE;
				}

			} else {
				if (switch_event_create(&event, etype) != SWITCH_STATUS_SUCCESS) {
					*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
					return JS_TRUE;
				}
			}

			eo->event = event;
			eo->freed = 0;

			JS_SetPrivate(cx, obj, eo);
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}

static void event_destroy(JSContext * cx, JSObject * obj)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (eo) {
		if (!eo->freed && eo->event) {
			switch_event_destroy(&eo->event);
		}
		switch_safe_free(eo);
	}
}

static JSBool event_add_header(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (!eo || eo->freed) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argc > 1) {
		char *hname = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		char *hval = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
		switch_event_add_header(eo->event, SWITCH_STACK_BOTTOM, hname, "%s", hval);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		return JS_TRUE;
	}

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	return JS_TRUE;
}

static JSBool event_get_header(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (!eo) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argc > 0) {
		char *hname = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		char *val = switch_event_get_header(eo->event, hname);
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, val));
		return JS_TRUE;
	}

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	return JS_TRUE;
}

static JSBool event_add_body(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (!eo || eo->freed) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argc > 0) {
		char *body = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		switch_event_add_body(eo->event, "%s", body);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		return JS_TRUE;
	}

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	return JS_TRUE;
}

static JSBool event_get_body(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (!eo) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, switch_event_get_body(eo->event)));

	return JS_TRUE;
}

static JSBool event_get_type(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (!eo) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, switch_event_name(eo->event->event_id)));

	return JS_TRUE;
}

static JSBool event_serialize(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);
	char *buf;
	uint8_t isxml = 0;

	if (!eo) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argc > 0) {
		char *arg = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		if (!strcasecmp(arg, "xml")) {
			isxml++;
		}
	}

	if (isxml) {
		switch_xml_t xml;
		char *xmlstr;
		if ((xml = switch_event_xmlize(eo->event, SWITCH_VA_NONE))) {
			xmlstr = switch_xml_toxml(xml);
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, xmlstr));
			switch_xml_free(xml);
			free(xmlstr);
		} else {
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		}
	} else {
		if (switch_event_serialize(eo->event, &buf) == SWITCH_STATUS_SUCCESS) {
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buf));
			switch_safe_free(buf);
		}
	}

	return JS_TRUE;
}

static JSBool event_fire(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (eo) {
		switch_event_fire(&eo->event);
		JS_SetPrivate(cx, obj, NULL);
		switch_safe_free(eo);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		return JS_TRUE;
	}

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	return JS_TRUE;
}

static JSBool event_destroy_(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct event_obj *eo = JS_GetPrivate(cx, obj);

	if (eo) {
		if (!eo->freed) {
			switch_event_destroy(&eo->event);
		}
		JS_SetPrivate(cx, obj, NULL);
		switch_safe_free(eo);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		return JS_TRUE;
	}

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	return JS_TRUE;
}



enum event_tinyid {
	EVENT_READY
};

static JSFunctionSpec event_methods[] = {
	{"addHeader", event_add_header, 1},
	{"getHeader", event_get_header, 1},
	{"addBody", event_add_body, 1},
	{"getBody", event_get_body, 1},
	{"getType", event_get_type, 1},
	{"serialize", event_serialize, 0},
	{"fire", event_fire, 0},
	{"destroy", event_destroy_, 0},
	{0}
};


static JSPropertySpec event_props[] = {
	{"ready", EVENT_READY, JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};


static JSBool event_getProperty(JSContext * cx, JSObject * obj, jsval id, jsval * vp)
{
	JSBool res = JS_TRUE;
	switch_event_t *event = JS_GetPrivate(cx, obj);
	char *name;
	int param = 0;

	if (!event) {
		*vp = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	name = JS_GetStringBytes(JS_ValueToString(cx, id));
	/* numbers are our props anything else is a method */
	if (name[0] >= 48 && name[0] <= 57) {
		param = atoi(name);
	} else {
		return JS_TRUE;
	}

	switch (param) {
	case EVENT_READY:
		*vp = BOOLEAN_TO_JSVAL(JS_TRUE);
		break;
	}

	return res;
}

JSClass event_class = {
	"Event", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, event_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, event_destroy, NULL, NULL, NULL,
	event_construct
};




static void js_error(JSContext * cx, const char *message, JSErrorReport * report)
{
	const char *filename = __FILE__;
	int line = __LINE__;
	const char *text = "";
	char *ex = "";

	if (message && report) {
		if (report->filename) {
			filename = report->filename;
		}
		line = report->lineno;
		if (report->linebuf) {
			text = report->linebuf;
			ex = "near ";
		}
	}

	if (!message) {
		message = "(N/A)";
	}

	switch_log_printf(SWITCH_CHANNEL_ID_LOG, filename, modname, line, SWITCH_LOG_ERROR, "%s %s%s\n", ex, message, text);

}


static switch_status_t sm_load_file(char *filename)
{
	sm_loadable_module_t *module = NULL;
	switch_dso_handle_t *dso = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_dso_handle_sym_t function_handle = NULL;
	spidermonkey_init_t spidermonkey_init = NULL;
	const sm_module_interface_t *module_interface = NULL, *mp;

	int loading = 1;
	const char *err = NULL;
	char derr[512] = "";

	assert(filename != NULL);

	status = switch_dso_load(&dso, filename, module_manager.pool);

	while (loading) {
		if (status != SWITCH_STATUS_SUCCESS) {
			switch_dso_error(dso, derr, sizeof(derr));
			err = derr;
			break;
		}

		status = switch_dso_sym(&function_handle, dso, "spidermonkey_init");
		spidermonkey_init = (spidermonkey_init_t) (intptr_t) function_handle;

		if (spidermonkey_init == NULL) {
			err = "Cannot Load";
			break;
		}

		if (spidermonkey_init(&module_interface) != SWITCH_STATUS_SUCCESS) {
			err = "Module load routine returned an error";
			break;
		}

		if (!(module = (sm_loadable_module_t *) switch_core_permanent_alloc(sizeof(*module)))) {
			err = "Could not allocate memory\n";
			break;
		}

		loading = 0;
	}

	if (err) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Error Loading module %s\n**%s**\n", filename, err);
		return SWITCH_STATUS_GENERR;
	}

	module->filename = switch_core_permanent_strdup(filename);
	module->spidermonkey_init = spidermonkey_init;
	module->module_interface = module_interface;

	module->lib = dso;

	switch_core_hash_insert(module_manager.mod_hash, (char *) module->filename, (void *) module);

	for (mp = module->module_interface; mp; mp = mp->next) {
		switch_core_hash_insert(module_manager.load_hash, (char *) mp->name, (void *) mp);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Successfully Loaded [%s]\n", module->filename);

	return SWITCH_STATUS_SUCCESS;

}

static switch_status_t sm_load_module(char *dir, char *fname)
{
	switch_size_t len = 0;
	char *path;
	char *file;

#ifdef WIN32
	const char *ext = ".dll";
#elif defined (MACOSX) || defined (DARWIN)
	const char *ext = ".dylib";
#else
	const char *ext = ".so";
#endif


	if ((file = switch_core_strdup(module_manager.pool, fname)) == 0) {
		return SWITCH_STATUS_FALSE;
	}

	if (*file == '/') {
		path = switch_core_strdup(module_manager.pool, file);
	} else {
		if (strchr(file, '.')) {
			len = strlen(dir);
			len += strlen(file);
			len += 4;
			path = (char *) switch_core_alloc(module_manager.pool, len);
			snprintf(path, len, "%s%s%s", dir, SWITCH_PATH_SEPARATOR, file);
		} else {
			len = strlen(dir);
			len += strlen(file);
			len += 8;
			path = (char *) switch_core_alloc(module_manager.pool, len);
			snprintf(path, len, "%s%s%s%s", dir, SWITCH_PATH_SEPARATOR, file, ext);
		}
	}

	return sm_load_file(path);
}

static switch_status_t load_modules(void)
{
	char *cf = "spidermonkey.conf";
	switch_xml_t cfg, xml;
	unsigned int count = 0;

#ifdef WIN32
	const char *ext = ".dll";
	const char *EXT = ".DLL";
#elif defined (MACOSX) || defined (DARWIN)
	const char *ext = ".dylib";
	const char *EXT = ".DYLIB";
#else
	const char *ext = ".so";
	const char *EXT = ".SO";
#endif

	memset(&module_manager, 0, sizeof(module_manager));
	switch_core_new_memory_pool(&module_manager.pool);

	switch_core_hash_init(&module_manager.mod_hash, module_manager.pool);
	switch_core_hash_init(&module_manager.load_hash, module_manager.pool);

	if ((xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_xml_t mods, ld;

		if ((mods = switch_xml_child(cfg, "modules"))) {
			for (ld = switch_xml_child(mods, "load"); ld; ld = ld->next) {
				const char *val = switch_xml_attr_soft(ld, "module");
				if (strchr(val, '.') && !strstr(val, ext) && !strstr(val, EXT)) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Invalid extension for %s\n", val);
					continue;
				}
				sm_load_module((char *) SWITCH_GLOBAL_dirs.mod_dir, (char *) val);
				count++;
			}
		}
		switch_xml_free(xml);

	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "open of %s failed\n", cf);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t init_js(void)
{
	memset(&globals, 0, sizeof(globals));
	globals.gQuitting = JS_FALSE;
	globals.gErrFile = NULL;
	globals.gOutFile = NULL;
	globals.gStackChunkSize = 8192;
	globals.gStackBase = (jsuword) & globals.stackDummy;
	globals.gErrFile = stderr;
	globals.gOutFile = stdout;

	if (!(globals.rt = JS_NewRuntime(64L * 1024L * 1024L))) {
		return SWITCH_STATUS_FALSE;
	}

	if (load_modules() != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}

JSObject *new_js_event(switch_event_t *event, char *name, JSContext * cx, JSObject * obj)
{
	struct event_obj *eo;
	JSObject *Event = NULL;

	if ((eo = malloc(sizeof(*eo)))) {
		eo->event = event;
		eo->freed = 1;
		if ((Event = JS_DefineObject(cx, obj, name, &event_class, NULL, 0))) {
			if ((JS_SetPrivate(cx, Event, eo) && JS_DefineProperties(cx, Event, event_props) && JS_DefineFunctions(cx, Event, event_methods))) {
			}
		}
	}
	return Event;
}


static switch_status_t js_common_callback(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{
	char *dtmf = NULL;
	switch_event_t *event = NULL;
	struct input_callback_state *cb_state = buf;
	struct js_session *jss = cb_state->session_state;
	uintN argc = 0;
	jsval argv[4];
	JSObject *Event = NULL;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		return SWITCH_STATUS_FALSE;
	}

	switch (itype) {
	case SWITCH_INPUT_TYPE_EVENT:
		if ((event = (switch_event_t *) input)) {
			if ((Event = new_js_event(event, "_XX_EVENT_XX_", cb_state->cx, cb_state->obj))) {
				argv[argc++] = STRING_TO_JSVAL(JS_NewStringCopyZ(cb_state->cx, "event"));
				argv[argc++] = OBJECT_TO_JSVAL(Event);
			}
		}
		if (!Event) {
			return SWITCH_STATUS_FALSE;
		}
		break;
	case SWITCH_INPUT_TYPE_DTMF:
		dtmf = (char *) input;
		argv[argc++] = STRING_TO_JSVAL(JS_NewStringCopyZ(cb_state->cx, "dtmf"));
		argv[argc++] = STRING_TO_JSVAL(JS_NewStringCopyZ(cb_state->cx, dtmf));
		break;
	}

	if (cb_state->arg) {
		argv[argc++] = cb_state->arg;
	}

	JS_ResumeRequest(cb_state->cx, cb_state->saveDepth);
	JS_CallFunction(cb_state->cx, cb_state->obj, cb_state->function, argc, argv, &cb_state->ret);
	cb_state->saveDepth = JS_SuspendRequest(cb_state->cx);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t js_stream_input_callback(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{
	char *ret;
	switch_status_t status;
	struct input_callback_state *cb_state = buf;
	switch_file_handle_t *fh = cb_state->extra;
	struct js_session *jss = cb_state->session_state;

	if ((status = js_common_callback(session, input, itype, buf, buflen)) != SWITCH_STATUS_SUCCESS) {
		return status;
	}


	if ((ret = JS_GetStringBytes(JS_ValueToString(cb_state->cx, cb_state->ret)))) {
		if (!strncasecmp(ret, "speed", 4)) {
			char *p;

			if ((p = strchr(ret, ':'))) {
				p++;
				if (*p == '+' || *p == '-') {
					int step;
					if (!(step = atoi(p))) {
						step = 1;
					}
					fh->speed += step;
				} else {
					int speed = atoi(p);
					fh->speed = speed;
				}
				return SWITCH_STATUS_SUCCESS;
			}

			return SWITCH_STATUS_FALSE;
		} else if (!strcasecmp(ret, "pause")) {
			if (switch_test_flag(fh, SWITCH_FILE_PAUSE)) {
				switch_clear_flag(fh, SWITCH_FILE_PAUSE);
			} else {
				switch_set_flag(fh, SWITCH_FILE_PAUSE);
			}
			return SWITCH_STATUS_SUCCESS;
		} else if (!strcasecmp(ret, "restart")) {
			unsigned int pos = 0;
			fh->speed = 0;
			switch_core_file_seek(fh, &pos, 0, SEEK_SET);
			return SWITCH_STATUS_SUCCESS;
		} else if (!strncasecmp(ret, "seek", 4)) {
			switch_codec_t *codec;
			unsigned int samps = 0;
			unsigned int pos = 0;
			char *p;
			codec = switch_core_session_get_read_codec(jss->session);

			if ((p = strchr(ret, ':'))) {
				p++;
				if (*p == '+' || *p == '-') {
					int step;
					if (!(step = atoi(p))) {
						step = 1000;
					}
					if (step > 0) {
						samps = step * (codec->implementation->samples_per_second / 1000);
						switch_core_file_seek(fh, &pos, samps, SEEK_CUR);
					} else {
						samps = step * (codec->implementation->samples_per_second / 1000);
						switch_core_file_seek(fh, &pos, fh->pos - samps, SEEK_SET);
					}
				} else {
					samps = atoi(p) * (codec->implementation->samples_per_second / 1000);
					switch_core_file_seek(fh, &pos, samps, SEEK_SET);
				}
			}

			return SWITCH_STATUS_SUCCESS;
		}

		if (!strcmp(ret, "true") || !strcmp(ret, "undefined")) {
			return SWITCH_STATUS_SUCCESS;
		}

		return SWITCH_STATUS_BREAK;

	}
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t js_record_input_callback(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{
	char *ret;
	switch_status_t status;
	struct input_callback_state *cb_state = buf;
	switch_file_handle_t *fh = cb_state->extra;

	if ((status = js_common_callback(session, input, itype, buf, buflen)) != SWITCH_STATUS_SUCCESS) {
		return status;
	}

	if ((ret = JS_GetStringBytes(JS_ValueToString(cb_state->cx, cb_state->ret)))) {
		if (!strcasecmp(ret, "pause")) {
			if (switch_test_flag(fh, SWITCH_FILE_PAUSE)) {
				switch_clear_flag(fh, SWITCH_FILE_PAUSE);
			} else {
				switch_set_flag(fh, SWITCH_FILE_PAUSE);
			}
			return SWITCH_STATUS_SUCCESS;
		} else if (!strcasecmp(ret, "restart")) {
			unsigned int pos = 0;
			fh->speed = 0;
			switch_core_file_seek(fh, &pos, 0, SEEK_SET);
			return SWITCH_STATUS_SUCCESS;
		}

		if (!strcmp(ret, "true") || !strcmp(ret, "undefined")) {
			return SWITCH_STATUS_SUCCESS;
		}

		return SWITCH_STATUS_BREAK;

	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t js_collect_input_callback(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{
	char *ret;
	switch_status_t status;
	struct input_callback_state *cb_state = buf;

	if ((status = js_common_callback(session, input, itype, buf, buflen)) != SWITCH_STATUS_SUCCESS) {
		return status;
	}

	if ((ret = JS_GetStringBytes(JS_ValueToString(cb_state->cx, cb_state->ret)))) {
		if (!strcmp(ret, "true") || !strcmp(ret, "undefined")) {
			return SWITCH_STATUS_SUCCESS;
		}
	}

	return SWITCH_STATUS_BREAK;
}

static JSBool session_flush_digits(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	char buf[256];
	switch_size_t has;
	switch_channel_t *channel;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	while ((has = switch_channel_has_dtmf(channel))) {
		switch_channel_dequeue_dtmf(channel, buf, sizeof(buf));
	}

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	return JS_TRUE;
}

static JSBool session_flush_events(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_event_t *event;
	switch_channel_t *channel;

	if (!jss || !jss->session) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);


	while (switch_core_session_dequeue_event(jss->session, &event) == SWITCH_STATUS_SUCCESS) {
		switch_event_destroy(&event);
	}

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	return JS_TRUE;

}

static JSBool session_recordfile(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	char *file_name = NULL;
	void *bp = NULL;
	int len = 0;
	switch_input_callback_function_t dtmf_func = NULL;
	struct input_callback_state cb_state = { 0 };
	switch_file_handle_t fh = { 0 };
	JSFunction *function;
	int32 limit = 0;
	switch_input_args_t args = { 0 };

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	if (argc > 0) {
		file_name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		if (switch_strlen_zero(file_name)) {
			return JS_FALSE;
		}
	}
	if (argc > 1) {
		if ((function = JS_ValueToFunction(cx, argv[1]))) {
			memset(&cb_state, 0, sizeof(cb_state));
			cb_state.session_state = jss;
			cb_state.function = function;
			cb_state.cx = cx;
			cb_state.obj = obj;
			if (argc > 2) {
				cb_state.arg = argv[2];
			}

			dtmf_func = js_record_input_callback;
			bp = &cb_state;
			len = sizeof(cb_state);
		}

		if (argc > 3) {
			JS_ValueToInt32(cx, argv[3], &limit);
		}

		if (argc > 4) {
			int32 thresh;
			JS_ValueToInt32(cx, argv[4], &thresh);
			fh.thresh = thresh;
		}

		if (argc > 5) {
			int32 silence_hits;
			JS_ValueToInt32(cx, argv[5], &silence_hits);
			fh.silence_hits = silence_hits;
		}
	}


	cb_state.extra = &fh;
	cb_state.ret = BOOLEAN_TO_JSVAL(JS_FALSE);
	cb_state.saveDepth = JS_SuspendRequest(cx);
	args.input_callback = dtmf_func;
	args.buf = bp;
	args.buflen = len;
	switch_ivr_record_file(jss->session, &fh, file_name, &args, limit);
	JS_ResumeRequest(cx, cb_state.saveDepth);
	*rval = cb_state.ret;

	return JS_TRUE;
}


static JSBool session_collect_input(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	void *bp = NULL;
	int len = 0;
	int32 to = 0;
	switch_input_callback_function_t dtmf_func = NULL;
	struct input_callback_state cb_state = { 0 };
	JSFunction *function;
	switch_input_args_t args = { 0 };


	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	if (argc > 0) {
		if ((function = JS_ValueToFunction(cx, argv[0]))) {
			memset(&cb_state, 0, sizeof(cb_state));
			cb_state.function = function;

			if (argc > 1) {
				cb_state.arg = argv[1];
			}

			cb_state.session_state = jss;
			cb_state.cx = cx;
			cb_state.obj = obj;
			dtmf_func = js_collect_input_callback;
			bp = &cb_state;
			len = sizeof(cb_state);
		}
	}

	if (argc > 2) {
		JS_ValueToInt32(jss->cx, argv[2], &to);
	}

	cb_state.saveDepth = JS_SuspendRequest(cx);
	args.input_callback = dtmf_func;
	args.buf = bp;
	args.buflen = len;
	switch_ivr_collect_digits_callback(jss->session, &args, to);
	JS_ResumeRequest(cx, cb_state.saveDepth);

	*rval = cb_state.ret;

	return JS_TRUE;
}

/* session.sayphrase(phrase_name, phrase_data, language, dtmf_callback, dtmf_callback_args)*/

static JSBool session_sayphrase(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	char *phrase_name = NULL;
	char *phrase_data = NULL;
	char *phrase_lang = NULL;
	//char *input_callback = NULL;
	void *bp = NULL;
	int len = 0;
	switch_input_callback_function_t dtmf_func = NULL;
	struct input_callback_state cb_state = { 0 };
	JSFunction *function;
	switch_input_args_t args = { 0 };

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	if (argc > 0) {
		phrase_name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		if (switch_strlen_zero(phrase_name)) {
			return JS_FALSE;
		}
	} else {
		return JS_FALSE;
	}


	if (argc > 1) {
		phrase_data = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
	}

	if (argc > 2) {
		phrase_lang = JS_GetStringBytes(JS_ValueToString(cx, argv[2]));
	}

	if (argc > 3) {
		if ((function = JS_ValueToFunction(cx, argv[3]))) {
			memset(&cb_state, 0, sizeof(cb_state));
			cb_state.function = function;

			if (argc > 4) {
				cb_state.arg = argv[4];
			}

			cb_state.session_state = jss;
			cb_state.cx = cx;
			cb_state.obj = obj;
			dtmf_func = js_collect_input_callback;
			bp = &cb_state;
			len = sizeof(cb_state);
		}
	}

	cb_state.ret = BOOLEAN_TO_JSVAL(JS_FALSE);
	cb_state.saveDepth = JS_SuspendRequest(cx);
	args.input_callback = dtmf_func;
	args.buf = bp;
	args.buflen = len;

	switch_ivr_phrase_macro(jss->session, phrase_name, phrase_data, phrase_lang, &args);

	JS_ResumeRequest(cx, cb_state.saveDepth);
	*rval = cb_state.ret;

	return JS_TRUE;
}

static void check_hangup_hook(struct js_session *jss)
{
	jsval argv[2] = { 0 };
	int argc = 0;
	jsval ret;
	if (jss->on_hangup) {
		argv[argc++] = OBJECT_TO_JSVAL(jss->obj);
		JS_CallFunction(jss->cx, jss->obj, jss->on_hangup, argc, argv, &ret);
	}
}

static switch_status_t hanguphook(switch_core_session_t *session)
{
	switch_channel_t *channel;
	struct js_session *jss = NULL;

	channel = switch_core_session_get_channel(session);
	assert(channel != NULL);

	if (switch_channel_get_state(channel) != CS_EXECUTE) {
		if ((jss = switch_channel_get_private(channel, "jss"))) {
			check_hangup_hook(jss);
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

static JSBool session_hanguphook(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSFunction *function;
	struct js_session *jss;
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if ((jss = JS_GetPrivate(cx, obj)) && jss->session) {
		if (argc > 0) {
			if ((function = JS_ValueToFunction(cx, argv[0]))) {
				switch_channel_t *channel = switch_core_session_get_channel(jss->session);
				assert(channel != NULL);
				jss->on_hangup = function;
				switch_channel_set_private(channel, "jss", jss);
				switch_core_event_hook_add_state_change(jss->session, hanguphook);
				*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
			}
		}
	}

	return JS_TRUE;
}

static JSBool session_streamfile(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	char *file_name = NULL;
	//char *input_callback = NULL;
	void *bp = NULL;
	int len = 0;
	switch_input_callback_function_t dtmf_func = NULL;
	struct input_callback_state cb_state = { 0 };
	switch_file_handle_t fh = { 0 };
	JSFunction *function;
	switch_input_args_t args = { 0 };

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	if (argc > 0) {
		file_name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		if (switch_strlen_zero(file_name)) {
			return JS_FALSE;
		}
	}

	if (argc > 1) {
		if ((function = JS_ValueToFunction(cx, argv[1]))) {
			memset(&cb_state, 0, sizeof(cb_state));
			cb_state.function = function;

			if (argc > 2) {
				cb_state.arg = argv[2];
			}

			cb_state.session_state = jss;
			cb_state.cx = cx;
			cb_state.obj = obj;
			dtmf_func = js_stream_input_callback;
			bp = &cb_state;
			len = sizeof(cb_state);
		}
	}

	if (argc > 3) {
		int32 samps;
		JS_ValueToInt32(cx, argv[3], &samps);
		fh.samples = samps;
	}

	cb_state.extra = &fh;
	cb_state.ret = BOOLEAN_TO_JSVAL(JS_FALSE);
	cb_state.saveDepth = JS_SuspendRequest(cx);
	args.input_callback = dtmf_func;
	args.buf = bp;
	args.buflen = len;
	switch_ivr_play_file(jss->session, &fh, file_name, &args);
	JS_ResumeRequest(cx, cb_state.saveDepth);
	*rval = cb_state.ret;

	return JS_TRUE;
}

static JSBool session_set_variable(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (argc > 1) {
		char *var, *val;

		var = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		val = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
		switch_channel_set_variable(channel, var, val);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	} else {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	}

	return JS_TRUE;
}

static JSBool session_get_variable(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;

	if (!jss || !jss->session) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (argc > 0) {
		char *var, *val;

		var = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		val = switch_channel_get_variable(channel, var);

		if (val) {
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, val));
		} else {
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		}
	} else {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	}

	return JS_TRUE;
}


static JSBool session_speak(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	char *tts_name = NULL;
	char *voice_name = NULL;
	char *text = NULL;
	switch_codec_t *codec;
	void *bp = NULL;
	int len = 0;
	struct input_callback_state cb_state = { 0 };
	switch_input_callback_function_t dtmf_func = NULL;
	JSFunction *function;
	switch_input_args_t args = { 0 };

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	if (argc > 0) {
		tts_name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	}
	if (argc > 1) {
		voice_name = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
	}
	if (argc > 2) {
		text = JS_GetStringBytes(JS_ValueToString(cx, argv[2]));
	}
	if (argc > 3) {
		if ((function = JS_ValueToFunction(cx, argv[3]))) {
			memset(&cb_state, 0, sizeof(cb_state));
			cb_state.function = function;
			if (argc > 4) {
				cb_state.arg = argv[4];
			}

			cb_state.cx = cx;
			cb_state.obj = obj;
			cb_state.session_state = jss;
			dtmf_func = js_collect_input_callback;
			bp = &cb_state;
			len = sizeof(cb_state);
		}
	}

	if (!tts_name && text) {
		return JS_FALSE;
	}

	codec = switch_core_session_get_read_codec(jss->session);
	cb_state.ret = BOOLEAN_TO_JSVAL(JS_FALSE);
	cb_state.saveDepth = JS_SuspendRequest(cx);
	args.input_callback = dtmf_func;
	args.buf = bp;
	args.buflen = len;
	switch_ivr_speak_text(jss->session, tts_name, voice_name
						  && strlen(voice_name) ? voice_name : NULL, codec->implementation->samples_per_second, text, &args);
	JS_ResumeRequest(cx, cb_state.saveDepth);
	*rval = cb_state.ret;

	return JS_TRUE;
}

static JSBool session_get_digits(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	char *terminators = NULL;
	char buf[513] = { 0 };
	int32 digits = 0, timeout = 5000;
	switch_channel_t *channel;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	if (argc > 0) {
		char term;
		JS_ValueToInt32(cx, argv[0], &digits);

		if (digits > sizeof(buf) - 1) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Exceeded max digits of %" SWITCH_SIZE_T_FMT "\n", sizeof(buf) - 1);
			return JS_FALSE;
		}

		if (argc > 1) {
			terminators = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
		}
		if (argc > 2) {
			JS_ValueToInt32(cx, argv[2], &timeout);
		}


		switch_ivr_collect_digits_count(jss->session, buf, sizeof(buf), digits, terminators, &term, timeout);
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buf));
		return JS_TRUE;
	}

	return JS_FALSE;
}

static JSBool session_autohangup(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argv[0]) {
		JSBool tf;
		JS_ValueToBoolean(cx, argv[0], &tf);
		if (tf == JS_TRUE) {
			switch_set_flag(jss, S_HUP);
		} else {
			switch_clear_flag(jss, S_HUP);
		}
		*rval = BOOLEAN_TO_JSVAL(tf);
	}

	return JS_TRUE;
}

static JSBool session_answer(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	switch_channel_answer(channel);
	return JS_TRUE;
}


static JSBool session_cdr(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_xml_t cdr;

	/*Always a pessimist... sheesh! */
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		return JS_TRUE;
	}

	if (switch_ivr_generate_xml_cdr(jss->session, &cdr) == SWITCH_STATUS_SUCCESS) {
		char *xml_text;
		if ((xml_text = switch_xml_toxml(cdr))) {
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, xml_text));
		}
		switch_safe_free(xml_text);
		switch_xml_free(cdr);
	}

	return JS_TRUE;
}

static JSBool session_ready(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	*rval = BOOLEAN_TO_JSVAL(switch_channel_ready(channel) ? JS_TRUE : JS_FALSE);

	return JS_TRUE;
}



static JSBool session_wait_for_media(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	switch_time_t started;
	unsigned int elapsed;
	int32 timeout = 60;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	started = switch_time_now();

	if (argc > 0) {
		JS_ValueToInt32(cx, argv[0], &timeout);
	}

	for (;;) {
		if (((elapsed = (unsigned int) ((switch_time_now() - started) / 1000)) > (switch_time_t) timeout)
			|| switch_channel_get_state(channel) >= CS_HANGUP) {
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
			break;
		}

		if (switch_channel_ready(channel)
			&& (switch_channel_test_flag(channel, CF_ANSWERED) || switch_channel_test_flag(channel, CF_EARLY_MEDIA))) {
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
			break;
		}

		switch_yield(1000);
	}

	return JS_TRUE;
}


static JSBool session_wait_for_answer(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	switch_time_t started;
	unsigned int elapsed;
	int32 timeout = 60;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	started = switch_time_now();

	if (argc > 0) {
		JS_ValueToInt32(cx, argv[0], &timeout);
	}

	for (;;) {
		if (((elapsed = (unsigned int) ((switch_time_now() - started) / 1000)) > (switch_time_t) timeout)
			|| switch_channel_get_state(channel) >= CS_HANGUP) {
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
			break;
		}

		if (switch_channel_ready(channel) && switch_channel_test_flag(channel, CF_ANSWERED)) {
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
			break;
		}

		switch_yield(1000);
	}

	return JS_TRUE;
}

static JSBool session_execute(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSBool retval = JS_FALSE;
	switch_channel_t *channel;
	struct js_session *jss = JS_GetPrivate(cx, obj);

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}


	if (argc > 1) {
		const switch_application_interface_t *application_interface;
		char *app_name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		char *app_arg = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
		struct js_session *jss = JS_GetPrivate(cx, obj);
		jsrefcount saveDepth;

		if (!jss || !jss->session) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
			return JS_TRUE;
		}

		if ((application_interface = switch_loadable_module_get_application_interface(app_name))) {
			if (application_interface->application_function) {
				saveDepth = JS_SuspendRequest(cx);
				application_interface->application_function(jss->session, app_arg);
				JS_ResumeRequest(cx, saveDepth);
				retval = JS_TRUE;
			}
		}
	}

	*rval = BOOLEAN_TO_JSVAL(retval);
	return JS_TRUE;
}

static JSBool session_get_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_event_t *event;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (switch_core_session_dequeue_event(jss->session, &event) == SWITCH_STATUS_SUCCESS) {
		JSObject *Event;
		struct event_obj *eo;

		if ((eo = malloc(sizeof(*eo)))) {
			eo->event = event;
			eo->freed = 0;

			if ((Event = JS_DefineObject(cx, obj, "__event__", &event_class, NULL, 0))) {
				if ((JS_SetPrivate(cx, Event, eo) && JS_DefineProperties(cx, Event, event_props) && JS_DefineFunctions(cx, Event, event_methods))) {
					*rval = OBJECT_TO_JSVAL(Event);
					return JS_TRUE;
				}
			}
		}
	}

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	return JS_TRUE;

}

static JSBool session_send_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	JSObject *Event;
	struct event_obj *eo;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argc > 0) {
		if (JS_ValueToObject(cx, argv[0], &Event)) {
			if ((eo = JS_GetPrivate(cx, Event))) {
				if (switch_core_session_receive_event(jss->session, &eo->event) != SWITCH_STATUS_SUCCESS) {
					*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
					return JS_TRUE;
				}

				JS_SetPrivate(cx, Event, NULL);
			}
		}
	}

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	return JS_TRUE;

}


static JSBool session_hangup(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	switch_channel_t *channel;
	char *cause_name = NULL;
	switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;

	if (!jss || !jss->session) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "You must call the session.originate method before calling this method!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	channel = switch_core_session_get_channel(jss->session);
	assert(channel != NULL);

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Session is not active!\n");
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argc > 1) {
		cause_name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		cause = switch_channel_str2cause(cause_name);
	}


	switch_channel_hangup(channel, cause);
	switch_core_session_kill_channel(jss->session, SWITCH_SIG_KILL);
	return JS_TRUE;
}

#ifdef HAVE_CURL

struct config_data {
	JSContext *cx;
	JSObject *obj;
	char *name;
	int fd;
};

static size_t hash_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register size_t realsize = size * nmemb;
	char *line, lineb[2048], *nextline = NULL, *val = NULL, *p = NULL;
	jsval rval;
	struct config_data *config_data = data;
	char code[256];

	if (config_data->name) {
		switch_copy_string(lineb, (char *) ptr, sizeof(lineb));
		line = lineb;
		while (line) {
			if ((nextline = strchr(line, '\n'))) {
				*nextline = '\0';
				nextline++;
			}

			if ((val = strchr(line, '='))) {
				*val = '\0';
				val++;
				if (val[0] == '>') {
					*val = '\0';
					val++;
				}

				for (p = line; p && *p == ' '; p++);
				line = p;
				for (p = line + strlen(line) - 1; *p == ' '; p--)
					*p = '\0';
				for (p = val; p && *p == ' '; p++);
				val = p;
				for (p = val + strlen(val) - 1; *p == ' '; p--)
					*p = '\0';

				snprintf(code, sizeof(code), "~%s[\"%s\"] = \"%s\"", config_data->name, line, val);
				eval_some_js(code, config_data->cx, config_data->obj, &rval);

			}

			line = nextline;
		}
	}
	return realsize;

}



static size_t file_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register unsigned int realsize = (unsigned int) (size * nmemb);
	struct config_data *config_data = data;

	write(config_data->fd, ptr, realsize);
	return realsize;
}


static JSBool js_fetchurl_hash(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	char *url = NULL, *name = NULL;
	CURL *curl_handle = NULL;
	struct config_data config_data;

	if (argc > 1) {
		url = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		name = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));


		curl_handle = curl_easy_init();
		if (!strncasecmp(url, "https", 5)) {
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		}
		config_data.cx = cx;
		config_data.obj = obj;
		if (name) {
			config_data.name = name;
		}
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, hash_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &config_data);
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "freeswitch-js/1.0");
		curl_easy_perform(curl_handle);
		curl_easy_cleanup(curl_handle);
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error!\n");
		return JS_FALSE;
	}

	return JS_TRUE;
}



static JSBool js_fetchurl_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	char *url = NULL, *filename = NULL;
	CURL *curl_handle = NULL;
	struct config_data config_data;

	if (argc > 1) {
		url = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		filename = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));

		curl_global_init(CURL_GLOBAL_ALL);
		curl_handle = curl_easy_init();
		if (!strncasecmp(url, "https", 5)) {
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		}
		config_data.cx = cx;
		config_data.obj = obj;

		config_data.name = filename;
		if ((config_data.fd = open(filename, O_CREAT | O_RDWR | O_TRUNC)) > -1) {
			curl_easy_setopt(curl_handle, CURLOPT_URL, url);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, file_callback);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &config_data);
			curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "freeswitch-js/1.0");
			curl_easy_perform(curl_handle);
			curl_easy_cleanup(curl_handle);
			close(config_data.fd);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error!\n");
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error!\n");
	}

	return JS_TRUE;
}
#endif


/* Session Object */
/*********************************************************************************/
enum session_tinyid {
	SESSION_NAME, SESSION_STATE,
	PROFILE_DIALPLAN, PROFILE_CID_NAME, PROFILE_CID_NUM, PROFILE_IP, PROFILE_ANI, PROFILE_ANI_II, PROFILE_DEST,
	SESSION_UUID, SESSION_CAUSE
};

static JSFunctionSpec session_methods[] = {
	{"originate", session_originate, 2},
	{"setHangupHook", session_hanguphook, 1},
	{"setAutoHangup", session_autohangup, 1},
	{"sayPhrase", session_sayphrase, 1},
	{"streamFile", session_streamfile, 1},
	{"collectInput", session_collect_input, 1},
	{"recordFile", session_recordfile, 1},
	{"flushEvents", session_flush_events, 1},
	{"flushDigits", session_flush_digits, 1},
	{"speak", session_speak, 1},
	{"setVariable", session_set_variable, 1},
	{"getVariable", session_get_variable, 1},
	{"getDigits", session_get_digits, 1},
	{"answer", session_answer, 0},
	{"generateXmlCdr", session_cdr, 0},
	{"ready", session_ready, 0},
	{"waitForAnswer", session_wait_for_answer, 0},
	{"waitForMedia", session_wait_for_media, 0},
	{"getEvent", session_get_event, 0},
	{"sendEvent", session_send_event, 0},
	{"hangup", session_hangup, 0},
	{"execute", session_execute, 0},
	{0}
};


static JSPropertySpec session_props[] = {
	{"name", SESSION_NAME, JSPROP_READONLY | JSPROP_PERMANENT},
	{"state", SESSION_STATE, JSPROP_READONLY | JSPROP_PERMANENT},
	{"dialplan", PROFILE_DIALPLAN, JSPROP_READONLY | JSPROP_PERMANENT},
	{"caller_id_name", PROFILE_CID_NAME, JSPROP_READONLY | JSPROP_PERMANENT},
	{"caller_id_num", PROFILE_CID_NUM, JSPROP_READONLY | JSPROP_PERMANENT},
	{"network_addr", PROFILE_IP, JSPROP_READONLY | JSPROP_PERMANENT},
	{"ani", PROFILE_ANI, JSPROP_READONLY | JSPROP_PERMANENT},
	{"aniii", PROFILE_ANI_II, JSPROP_READONLY | JSPROP_PERMANENT},
	{"destination", PROFILE_DEST, JSPROP_READONLY | JSPROP_PERMANENT},
	{"uuid", SESSION_UUID, JSPROP_READONLY | JSPROP_PERMANENT},
	{"cause", SESSION_CAUSE, JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};

static JSBool session_getProperty(JSContext * cx, JSObject * obj, jsval id, jsval * vp)
{
	struct js_session *jss = JS_GetPrivate(cx, obj);
	int param = 0;
	switch_channel_t *channel = NULL;
	switch_caller_profile_t *caller_profile = NULL;
	char *name = NULL;

	if (jss && jss->session) {
		channel = switch_core_session_get_channel(jss->session);
		assert(channel != NULL);
		caller_profile = switch_channel_get_caller_profile(channel);
	}

	name = JS_GetStringBytes(JS_ValueToString(cx, id));

	/* numbers are our props anything else is a method */
	if (name[0] >= 48 && name[0] <= 57) {
		param = atoi(name);
	} else {
		return JS_TRUE;
	}

	if (!channel) {
		switch (param) {
		case SESSION_CAUSE:
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, switch_channel_cause2str(jss->cause)));
			break;
		default:
			*vp = BOOLEAN_TO_JSVAL(JS_FALSE);
		}
		return JS_TRUE;
	}

	switch (param) {
	case SESSION_CAUSE:
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, switch_channel_cause2str(switch_channel_get_cause(channel))));
		break;
	case SESSION_NAME:
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, switch_channel_get_name(channel)));
		break;
	case SESSION_UUID:
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, switch_channel_get_uuid(channel)));
		break;
	case SESSION_STATE:
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, switch_channel_state_name(switch_channel_get_state(channel))));
		break;
	case PROFILE_DIALPLAN:
		if (caller_profile) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, caller_profile->dialplan));
		}
		break;
	case PROFILE_CID_NAME:
		if (caller_profile) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, caller_profile->caller_id_name));
		}
		break;
	case PROFILE_CID_NUM:
		if (caller_profile) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, caller_profile->caller_id_number));
		}
		break;
	case PROFILE_IP:
		if (caller_profile) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, caller_profile->network_addr));
		}
		break;
	case PROFILE_ANI:
		if (caller_profile) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, caller_profile->ani));
		}
		break;
	case PROFILE_ANI_II:
		if (caller_profile) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, caller_profile->aniii));
		}
		break;
	case PROFILE_DEST:
		if (caller_profile) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, caller_profile->destination_number));
		}
		break;
	default:
		*vp = BOOLEAN_TO_JSVAL(JS_FALSE);
		break;

	}

	return JS_TRUE;
}

JSClass session_class = {
	"Session", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, session_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, session_destroy, NULL, NULL, NULL,
	session_construct
};


static JSObject *new_js_session(JSContext * cx, JSObject * obj, switch_core_session_t *session, struct js_session *jss, char *name, int flags)
{
	JSObject *session_obj;
	if ((session_obj = JS_DefineObject(cx, obj, name, &session_class, NULL, 0))) {
		memset(jss, 0, sizeof(struct js_session));
		jss->session = session;
		jss->flags = flags;
		jss->cx = cx;
		jss->obj = session_obj;
		if ((JS_SetPrivate(cx, session_obj, jss) &&
			 JS_DefineProperties(cx, session_obj, session_props) && JS_DefineFunctions(cx, session_obj, session_methods))) {
			return session_obj;
		}
	}

	return NULL;
}

/* Session Object */
/*********************************************************************************/

static JSBool session_construct(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = NULL;

	assert((jss = malloc(sizeof(*jss))));
	memset(jss, 0, sizeof(*jss));
	jss->cx = cx;
	jss->obj = obj;
	switch_set_flag(jss, S_FREE);

	JS_SetPrivate(cx, obj, jss);

	return JS_TRUE;
}

static JSBool session_originate(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss = NULL;
	switch_memory_pool_t *pool = NULL;

	jss = JS_GetPrivate(cx, obj);
	jss->cause = SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER;

	if (argc > 1) {
		JSObject *session_obj;
		switch_core_session_t *session = NULL, *peer_session = NULL;
		switch_caller_profile_t *caller_profile = NULL, *orig_caller_profile = NULL;
		char *dest = NULL;
		char *dialplan = NULL;
		char *cid_name = "";
		char *cid_num = "";
		char *network_addr = "";
		char *ani = "";
		char *aniii = "";
		char *rdnis = "";
		char *context = "";
		char *username = NULL;
		char *to = NULL;

		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

		if (JS_ValueToObject(cx, argv[0], &session_obj)) {
			struct js_session *old_jss = NULL;
			if ((old_jss = JS_GetPrivate(cx, session_obj))) {
				session = old_jss->session;
				orig_caller_profile = switch_channel_get_caller_profile(switch_core_session_get_channel(session));
				dialplan = orig_caller_profile->dialplan;
				cid_name = orig_caller_profile->caller_id_name;
				cid_num = orig_caller_profile->caller_id_number;
				ani = orig_caller_profile->ani;
				aniii = orig_caller_profile->aniii;
				rdnis = orig_caller_profile->rdnis;
				context = orig_caller_profile->context;
				username = orig_caller_profile->username;
			}
		}

		dest = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));

		if (!strchr(dest, '/')) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Invalid Channel String\n");
			goto done;
		}

		if (argc > 2) {
			dialplan = JS_GetStringBytes(JS_ValueToString(cx, argv[2]));
		}
		if (argc > 3) {
			context = JS_GetStringBytes(JS_ValueToString(cx, argv[3]));
		}
		if (argc > 4) {
			cid_name = JS_GetStringBytes(JS_ValueToString(cx, argv[4]));
		}
		if (argc > 5) {
			cid_num = JS_GetStringBytes(JS_ValueToString(cx, argv[5]));
		}
		if (argc > 6) {
			network_addr = JS_GetStringBytes(JS_ValueToString(cx, argv[6]));
		}
		if (argc > 7) {
			ani = JS_GetStringBytes(JS_ValueToString(cx, argv[7]));
		}
		if (argc > 8) {
			aniii = JS_GetStringBytes(JS_ValueToString(cx, argv[8]));
		}
		if (argc > 9) {
			rdnis = JS_GetStringBytes(JS_ValueToString(cx, argv[9]));
		}
		if (argc > 10) {
			username = JS_GetStringBytes(JS_ValueToString(cx, argv[10]));
		}
		if (argc > 11) {
			to = JS_GetStringBytes(JS_ValueToString(cx, argv[11]));
		}

		if (switch_core_new_memory_pool(&pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "OH OH no pool\n");
			return JS_FALSE;
		}

		caller_profile = switch_caller_profile_new(pool,
												   username, dialplan, cid_name, cid_num, network_addr, ani, aniii, rdnis, (char *) modname, context,
												   dest);

		if (switch_ivr_originate(session, &peer_session, &jss->cause, dest, to ? atoi(to) : 60, NULL, NULL, NULL, caller_profile) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Cannot Create Outgoing Channel! [%s]\n", dest);
			goto done;
		}

		jss->session = peer_session;
		jss->flags = 0;

		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Missing Args\n");
	}

  done:
	return JS_TRUE;
}




static void session_destroy(JSContext * cx, JSObject * obj)
{
	struct js_session *jss;
	switch_channel_t *channel = NULL;

	if (cx && obj) {
		if ((jss = JS_GetPrivate(cx, obj))) {
			if (jss->session) {
				channel = switch_core_session_get_channel(jss->session);
				switch_channel_set_private(channel, "jss", NULL);
			}

			if (channel && switch_test_flag(jss, S_HUP)) {
				switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
			}

			if (switch_test_flag(jss, S_FREE)) {
				free(jss);
			}
		}
	}

	return;
}

/* FileIO Object */
/*********************************************************************************/
static JSBool fileio_construct(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	switch_memory_pool_t *pool;
	switch_file_t *fd;
	char *path, *flags_str;
	unsigned int flags = 0;
	struct fileio_obj *fio;

	if (argc > 1) {
		path = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		flags_str = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));

		if (strchr(flags_str, 'r')) {
			flags |= SWITCH_FOPEN_READ;
		}
		if (strchr(flags_str, 'w')) {
			flags |= SWITCH_FOPEN_WRITE;
		}
		if (strchr(flags_str, 'c')) {
			flags |= SWITCH_FOPEN_CREATE;
		}
		if (strchr(flags_str, 'a')) {
			flags |= SWITCH_FOPEN_APPEND;
		}
		if (strchr(flags_str, 't')) {
			flags |= SWITCH_FOPEN_TRUNCATE;
		}
		if (strchr(flags_str, 'b')) {
			flags |= SWITCH_FOPEN_BINARY;
		}
		switch_core_new_memory_pool(&pool);
		if (switch_file_open(&fd, path, flags, SWITCH_FPROT_UREAD | SWITCH_FPROT_UWRITE, pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot Open File: %s\n", path);
			switch_core_destroy_memory_pool(&pool);
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
			return JS_TRUE;
		}
		fio = switch_core_alloc(pool, sizeof(*fio));
		fio->fd = fd;
		fio->pool = pool;
		fio->path = switch_core_strdup(pool, path);
		fio->flags = flags;
		JS_SetPrivate(cx, obj, fio);
		return JS_TRUE;
	}

	return JS_TRUE;
}
static void fileio_destroy(JSContext * cx, JSObject * obj)
{
	struct fileio_obj *fio = JS_GetPrivate(cx, obj);

	if (fio) {
		switch_memory_pool_t *pool;
		if (fio->fd) {
			switch_file_close(fio->fd);
		}
		pool = fio->pool;
		switch_core_destroy_memory_pool(&pool);
		pool = NULL;
	}
}

static JSBool fileio_read(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct fileio_obj *fio = JS_GetPrivate(cx, obj);
	int32 bytes = 0;
	switch_size_t read = 0;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if (!fio) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (!(fio->flags & SWITCH_FOPEN_READ)) {
		return JS_TRUE;
	}

	if (argc > 0) {
		JS_ValueToInt32(cx, argv[0], &bytes);
	}

	if (bytes) {
		if (!fio->buf || fio->bufsize < bytes) {
			fio->buf = switch_core_alloc(fio->pool, bytes);
			fio->bufsize = bytes;
		}
		read = bytes;
		switch_file_read(fio->fd, fio->buf, &read);
		fio->buflen = read;
		*rval = BOOLEAN_TO_JSVAL((read > 0) ? JS_TRUE : JS_FALSE);
	}

	return JS_TRUE;
}

static JSBool fileio_data(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct fileio_obj *fio = JS_GetPrivate(cx, obj);

	if (!fio) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, fio->buf));
	return JS_TRUE;
}

static JSBool fileio_write(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct fileio_obj *fio = JS_GetPrivate(cx, obj);
	switch_size_t wrote = 0;
	char *data = NULL;

	if (!fio) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (!(fio->flags & SWITCH_FOPEN_WRITE)) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	if (argc > 0) {
		data = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	}

	if (data) {
		wrote = strlen(data);
		*rval = BOOLEAN_TO_JSVAL((switch_file_write(fio->fd, data, &wrote) == SWITCH_STATUS_SUCCESS) ? JS_TRUE : JS_FALSE);
	}

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	return JS_TRUE;
}

enum fileio_tinyid {
	FILEIO_PATH, FILEIO_OPEN
};

static JSFunctionSpec fileio_methods[] = {
	{"read", fileio_read, 1},
	{"write", fileio_write, 1},
	{"data", fileio_data, 0},
	{0}
};


static JSPropertySpec fileio_props[] = {
	{"path", FILEIO_PATH, JSPROP_READONLY | JSPROP_PERMANENT},
	{"open", FILEIO_OPEN, JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};


static JSBool fileio_getProperty(JSContext * cx, JSObject * obj, jsval id, jsval * vp)
{
	JSBool res = JS_TRUE;
	struct fileio_obj *fio = JS_GetPrivate(cx, obj);
	char *name;
	int param = 0;

	name = JS_GetStringBytes(JS_ValueToString(cx, id));
	/* numbers are our props anything else is a method */
	if (name[0] >= 48 && name[0] <= 57) {
		param = atoi(name);
	} else {
		return JS_TRUE;
	}

	switch (param) {
	case FILEIO_PATH:
		if (fio) {
			*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, fio->path));
		} else {
			*vp = BOOLEAN_TO_JSVAL(JS_FALSE);
		}
		break;
	case FILEIO_OPEN:
		*vp = BOOLEAN_TO_JSVAL(fio ? JS_TRUE : JS_FALSE);
		break;
	}

	return res;
}

JSClass fileio_class = {
	"FileIO", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, fileio_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, fileio_destroy, NULL, NULL, NULL,
	fileio_construct
};


/* Built-In*/
/*********************************************************************************/
static JSBool js_exit(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	return JS_FALSE;
}

static JSBool js_log(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	char *level_str, *msg;
	switch_log_level_t level = SWITCH_LOG_DEBUG;
	JSScript *script = NULL;
	const char *file = __FILE__;
	int line = __LINE__;
	JSStackFrame *caller;

	caller = JS_GetScriptedCaller(cx, NULL);
	script = JS_GetFrameScript(cx, caller);

	if (script) {
		file = JS_GetScriptFilename(cx, script);
		line = JS_GetScriptBaseLineNumber(cx, script);
	}


	if (argc > 1) {
		if ((level_str = JS_GetStringBytes(JS_ValueToString(cx, argv[0])))) {
			level = switch_log_str2level(level_str);
		}

		if ((msg = JS_GetStringBytes(JS_ValueToString(cx, argv[1])))) {
			switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, "console_log", line, level, "%s", msg);
			return JS_TRUE;
		}
	} else if (argc > 0) {
		if ((msg = JS_GetStringBytes(JS_ValueToString(cx, argv[0])))) {
			switch_log_printf(SWITCH_CHANNEL_ID_LOG, file, "console_log", line, level, "%s", msg);
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}

static JSBool js_include(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	char *code;
	if (argc > 0 && (code = JS_GetStringBytes(JS_ValueToString(cx, argv[0])))) {
		if (eval_some_js(code, cx, obj, rval) < 0) {
			return JS_FALSE;
		}
		return JS_TRUE;
	}
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Arguements\n");
	return JS_FALSE;
}

static JSBool js_api_use(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	char *mod_name = NULL;

	if (argc > 0 && (mod_name = JS_GetStringBytes(JS_ValueToString(cx, argv[0])))) {
		const sm_module_interface_t *mp;

		if ((mp = switch_core_hash_find(module_manager.load_hash, mod_name))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Loading %s\n", mod_name);
			mp->spidermonkey_load(cx, obj);
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error loading %s\n", mod_name);
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Filename\n");
	}

	return JS_TRUE;

}

static JSBool js_api_execute(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{

	if (argc > 1) {
		char *cmd = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		char *arg = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
		switch_core_session_t *session = NULL;
		switch_stream_handle_t stream = { 0 };
		char retbuf[2048] = "";

		if (argc > 2) {
			JSObject *session_obj;
			struct js_session *jss;
			if (JS_ValueToObject(cx, argv[2], &session_obj)) {
				if ((jss = JS_GetPrivate(cx, session_obj))) {
					session = jss->session;
				}
			}
		}


		stream.data = retbuf;
		stream.end = stream.data;
		stream.data_size = sizeof(retbuf);
		stream.write_function = switch_console_stream_write;
		switch_api_execute(cmd, arg, session, &stream);

		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, retbuf));
	} else {
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, ""));
	}

	return JS_TRUE;
}

static JSBool js_bridge(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	struct js_session *jss_a = NULL, *jss_b = NULL;
	JSObject *session_obj_a = NULL, *session_obj_b = NULL;
	void *bp = NULL;
	int len = 0;
	switch_input_callback_function_t dtmf_func = NULL;
	struct input_callback_state cb_state = { 0 };
	JSFunction *function;

	if (argc > 1) {
		if (JS_ValueToObject(cx, argv[0], &session_obj_a)) {
			if (!(jss_a = JS_GetPrivate(cx, session_obj_a))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot Find Session [1]\n");
				return JS_FALSE;
			}
		}
		if (JS_ValueToObject(cx, argv[1], &session_obj_b)) {
			if (!(jss_b = JS_GetPrivate(cx, session_obj_b))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot Find Session [1]\n");
				return JS_FALSE;
			}
		}
	}

	if (!(jss_a && jss_b)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failure! %s %s\n", jss_a ? "y" : "n", jss_b ? "y" : "n");
		return JS_FALSE;
	}


	if (argc > 2) {
		if ((function = JS_ValueToFunction(cx, argv[2]))) {
			memset(&cb_state, 0, sizeof(cb_state));
			cb_state.function = function;

			if (argc > 3) {
				cb_state.arg = argv[3];
			}

			cb_state.cx = cx;
			cb_state.obj = obj;

			cb_state.session_state = jss_a;
			dtmf_func = js_collect_input_callback;
			bp = &cb_state;
			len = sizeof(cb_state);
		}
	}

	cb_state.saveDepth = JS_SuspendRequest(cx);
	switch_ivr_multi_threaded_bridge(jss_a->session, jss_b->session, dtmf_func, bp, bp);
	JS_ResumeRequest(cx, cb_state.saveDepth);


	return JS_TRUE;
}

/* Replace this with more robust version later */
static JSBool js_system(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	char *cmd;
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if (argc > 0 && (cmd = JS_GetStringBytes(JS_ValueToString(cx, argv[0])))) {
		*rval = INT_TO_JSVAL(system(cmd));
		return JS_TRUE;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Arguements\n");
	return JS_FALSE;
}


static JSBool js_file_unlink(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	const char *path;
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	if (argc > 0 && (path = (const char *) JS_GetStringBytes(JS_ValueToString(cx, argv[0])))) {
		if ((switch_file_remove(path, NULL)) == SWITCH_STATUS_SUCCESS) {
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		}
		return JS_TRUE;
	}
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid Arguements\n");
	return JS_FALSE;
}

static JSFunctionSpec fs_functions[] = {
	{"console_log", js_log, 2},
	{"exit", js_exit, 0},
	{"include", js_include, 1},
	{"bridge", js_bridge, 2},
	{"apiExecute", js_api_execute, 2},
	{"use", js_api_use, 1},
	{"fileDelete", js_file_unlink, 1},
	{"system", js_system, 1},
#ifdef HAVE_CURL
	{"fetchURLHash", js_fetchurl_hash, 1},
	{"fetchURLFile", js_fetchurl_file, 1},
#endif
	{0}
};


static int env_init(JSContext * cx, JSObject * javascript_object)
{

	JS_DefineFunctions(cx, javascript_object, fs_functions);

	JS_InitStandardClasses(cx, javascript_object);

	JS_InitClass(cx, javascript_object, NULL, &session_class, session_construct, 3, session_props, session_methods, session_props, session_methods);

	JS_InitClass(cx, javascript_object, NULL, &fileio_class, fileio_construct, 3, fileio_props, fileio_methods, fileio_props, fileio_methods);

	JS_InitClass(cx, javascript_object, NULL, &event_class, event_construct, 3, event_props, event_methods, event_props, event_methods);

	return 1;
}

static void js_parse_and_execute(switch_core_session_t *session, char *input_code)
{
	JSObject *javascript_global_object = NULL;
	char buf[1024], *script, *arg, *argv[512];
	int argc = 0, x = 0, y = 0;
	unsigned int flags = 0;
	struct js_session jss;
	JSContext *cx = NULL;
	jsval rval;


	if ((cx = JS_NewContext(globals.rt, globals.gStackChunkSize))) {
		JS_BeginRequest(cx);
		JS_SetErrorReporter(cx, js_error);
		javascript_global_object = JS_NewObject(cx, &global_class, NULL, NULL);
		env_init(cx, javascript_global_object);
		JS_SetGlobalObject(cx, javascript_global_object);

		/* Emaculent conception of session object into the script if one is available */
		if (session && new_js_session(cx, javascript_global_object, session, &jss, "session", flags)) {
			JS_SetPrivate(cx, javascript_global_object, session);
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Allocation Error!\n");
		return;
	}

	script = input_code;

	if ((arg = strchr(script, ' '))) {
		*arg++ = '\0';
		argc = switch_separate_string(arg, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (argc) {					/* create a js doppleganger of this argc/argv */
		snprintf(buf, sizeof(buf), "~var argv = new Array(%d);", argc);
		eval_some_js(buf, cx, javascript_global_object, &rval);
		snprintf(buf, sizeof(buf), "~var argc = %d", argc);
		eval_some_js(buf, cx, javascript_global_object, &rval);

		for (y = 0; y < argc; y++) {
			snprintf(buf, sizeof(buf), "~argv[%d] = \"%s\";", x++, argv[y]);
			eval_some_js(buf, cx, javascript_global_object, &rval);
		}
	}

	if (cx) {
		eval_some_js(script, cx, javascript_global_object, &rval);
		JS_DestroyContext(cx);
	}
}




static void *SWITCH_THREAD_FUNC js_thread_run(switch_thread_t * thread, void *obj)
{
	char *input_code = obj;

	js_parse_and_execute(NULL, input_code);

	if (input_code) {
		free(input_code);
	}

	return NULL;
}


static switch_memory_pool_t *module_pool = NULL;

static void js_thread_launch(char *text)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;

	if (!module_pool) {
		if (switch_core_new_memory_pool(&module_pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "OH OH no pool\n");
			return;
		}
	}

	switch_threadattr_create(&thd_attr, module_pool);
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, js_thread_run, strdup(text), module_pool);
}


static switch_status_t launch_async(char *text, switch_core_session_t *session, switch_stream_handle_t *stream)
{

	if (switch_strlen_zero(text)) {
		stream->write_function(stream, "USAGE: %s\n", js_run_interface.syntax);
		return SWITCH_STATUS_SUCCESS;
	}

	js_thread_launch(text);
	stream->write_function(stream, "OK\n");
	return SWITCH_STATUS_SUCCESS;
}


static const switch_application_interface_t ivrtest_application_interface = {
	/*.interface_name */ "javascript",
	/*.application_function */ js_parse_and_execute,
	/* long_desc */ "Run a javascript ivr on a channel",
	/* short_desc */ "Launch JS ivr.",
	/* syntax */ "<script> [additional_vars [...]]",
	/* flags */ SAF_NONE,
	/* should we support no media mode here?  If so, we need to detect the mode, and either disable the media functions or indicate media if/when we need */
	/*.next */ NULL
};

static switch_api_interface_t js_run_interface = {
	/*.interface_name */ "jsrun",
	/*.desc */ "run a script",
	/*.function */ launch_async,
	/*.syntax */ "jsrun <script> [additional_vars [...]]",
	/*.next */ NULL
};

static switch_loadable_module_interface_t spidermonkey_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ NULL,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ NULL,
	/*.application_interface */ &ivrtest_application_interface,
	/*.api_interface */ &js_run_interface,
	/*.file_interface */ NULL,
	/*.speech_interface */ NULL,
	/*.directory_interface */ NULL
};

static void  message_query_handler(switch_event_t *event)
{
	char *account = switch_event_get_header(event, "message-account");

	if (account) {
		char *text;

		text = switch_mprintf("%s%smwi.js", SWITCH_GLOBAL_dirs.script_dir, SWITCH_PATH_SEPARATOR);
		assert(text != NULL);

		if (switch_file_exists(text) == SWITCH_STATUS_SUCCESS) {
			js_thread_launch(text);
		}

		free(text);
	}

}

SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface, char *filename)
{
	switch_status_t status;

	if ((status = init_js()) != SWITCH_STATUS_SUCCESS) {
		return status;
	}

	if (switch_event_bind((char *) modname, SWITCH_EVENT_MESSAGE_QUERY, SWITCH_EVENT_SUBCLASS_ANY, message_query_handler, NULL)
		!= SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = &spidermonkey_module_interface;

	curl_global_init(CURL_GLOBAL_ALL);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MOD_DECLARE(switch_status_t) switch_module_shutdown(void)
{
	curl_global_cleanup();
	return SWITCH_STATUS_SUCCESS;
}

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
