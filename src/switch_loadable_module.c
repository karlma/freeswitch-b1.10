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
 * switch_loadable_module.c -- Loadable Modules
 *
 */
#include <switch.h>
#include <ctype.h>

struct switch_loadable_module {
	char *filename;
	const switch_loadable_module_interface_t *module_interface;
	void *lib;
	switch_module_load_t switch_module_load;
	switch_module_runtime_t switch_module_runtime;
	switch_module_shutdown_t switch_module_shutdown;
};

struct switch_loadable_module_container {
	switch_hash_t *module_hash;
	switch_hash_t *endpoint_hash;
	switch_hash_t *codec_hash;
	switch_hash_t *dialplan_hash;
	switch_hash_t *timer_hash;
	switch_hash_t *application_hash;
	switch_hash_t *api_hash;
	switch_hash_t *file_hash;
	switch_hash_t *speech_hash;
	switch_hash_t *directory_hash;
	switch_memory_pool_t *pool;
};

static struct switch_loadable_module_container loadable_modules;

static void *switch_loadable_module_exec(switch_thread_t *thread, void *obj)
{


	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_core_thread_session_t *ts = obj;
	switch_loadable_module_t *module = ts->objs[0];
	int restarts;

	assert(thread != NULL);
	assert(module != NULL);

	for (restarts = 0; status != SWITCH_STATUS_TERM; restarts++) {
		status = module->switch_module_runtime();
	}
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Thread ended for %s\n", module->module_interface->module_name);

	if (ts->pool) {
		switch_memory_pool_t *pool = ts->pool;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroying Pool for %s\n", module->module_interface->module_name);
		switch_core_destroy_memory_pool(&pool);
	}
	switch_yield(1000000);
	return NULL;
}





static switch_status_t switch_loadable_module_process(char *key, switch_loadable_module_t *new_module)
{
	switch_event_t *event;

	switch_core_hash_insert(loadable_modules.module_hash, key, new_module);

	if (new_module->module_interface->endpoint_interface) {
		const switch_endpoint_interface_t *ptr;
		for (ptr = new_module->module_interface->endpoint_interface; ptr; ptr = ptr->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding Endpoint '%s'\n", ptr->interface_name);
			switch_core_hash_insert(loadable_modules.endpoint_hash, (char *) ptr->interface_name, (void *) ptr);
		}
	}

	if (new_module->module_interface->codec_interface) {
		const switch_codec_implementation_t *impl;
		const switch_codec_interface_t *ptr;

		for (ptr = new_module->module_interface->codec_interface; ptr; ptr = ptr->next) {
			for (impl = ptr->implementations; impl; impl = impl->next) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
								  "Adding Codec '%s' (%s) %dkhz %dms\n",
								  ptr->iananame,
								  ptr->interface_name,
								  impl->samples_per_second, impl->microseconds_per_frame / 1000);
			}
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "codec");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->interface_name);
				switch_event_fire(&event);
			}
			switch_core_hash_insert(loadable_modules.codec_hash, (char *) ptr->iananame, (void *) ptr);
		}
	}

	if (new_module->module_interface->dialplan_interface) {
		const switch_dialplan_interface_t *ptr;

		for (ptr = new_module->module_interface->dialplan_interface; ptr; ptr = ptr->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding Dialplan '%s'\n", ptr->interface_name);
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "dialplan");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->interface_name);
				switch_event_fire(&event);
			}
			switch_core_hash_insert(loadable_modules.dialplan_hash, (char *) ptr->interface_name, (void *) ptr);
		}
	}

	if (new_module->module_interface->timer_interface) {
		const switch_timer_interface_t *ptr;

		for (ptr = new_module->module_interface->timer_interface; ptr; ptr = ptr->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding Timer '%s'\n", ptr->interface_name);
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "timer");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->interface_name);
				switch_event_fire(&event);
			}
			switch_core_hash_insert(loadable_modules.timer_hash, (char *) ptr->interface_name, (void *) ptr);
		}
	}

	if (new_module->module_interface->application_interface) {
		const switch_application_interface_t *ptr;

		for (ptr = new_module->module_interface->application_interface; ptr; ptr = ptr->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding Application '%s'\n", ptr->interface_name);
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "application");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->interface_name);
				switch_event_fire(&event);
			}
			switch_core_hash_insert(loadable_modules.application_hash,
									(char *) ptr->interface_name, (void *) ptr);
		}
	}

	if (new_module->module_interface->api_interface) {
		const switch_api_interface_t *ptr;

		for (ptr = new_module->module_interface->api_interface; ptr; ptr = ptr->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding API Function '%s'\n", ptr->interface_name);
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "api");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->interface_name);
				switch_event_fire(&event);
			}
			switch_core_hash_insert(loadable_modules.api_hash, (char *) ptr->interface_name, (void *) ptr);
		}
	}

	if (new_module->module_interface->file_interface) {
		const switch_file_interface_t *ptr;

		for (ptr = new_module->module_interface->file_interface; ptr; ptr = ptr->next) {
			int i;
			for (i = 0; ptr->extens[i]; i++) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding File Format '%s'\n", ptr->extens[i]);
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "file");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->extens[i]);
				switch_event_fire(&event);
			}
				switch_core_hash_insert(loadable_modules.file_hash, (char *) ptr->extens[i], (void *) ptr);
			}
		}
	}

	if (new_module->module_interface->speech_interface) {
		const switch_speech_interface_t *ptr;

		for (ptr = new_module->module_interface->speech_interface; ptr; ptr = ptr->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding Speech interface '%s'\n", ptr->interface_name);
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "speech");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->interface_name);
				switch_event_fire(&event);
			}
			switch_core_hash_insert(loadable_modules.speech_hash, (char *) ptr->interface_name, (void *) ptr);
		}
	}

	if (new_module->module_interface->directory_interface) {
		const switch_directory_interface_t *ptr;

		for (ptr = new_module->module_interface->directory_interface; ptr; ptr = ptr->next) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Adding Directory interface '%s'\n", ptr->interface_name);
			if (switch_event_create(&event, SWITCH_EVENT_MODULE_LOAD) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "type", "directory");
				switch_event_add_header(event, SWITCH_STACK_BOTTOM, "name", "%s", ptr->interface_name);
				switch_event_fire(&event);
			}
			switch_core_hash_insert(loadable_modules.directory_hash, (char *) ptr->interface_name, (void *) ptr);
		}
	}
	

	return SWITCH_STATUS_SUCCESS;

}

static switch_status_t switch_loadable_module_load_file(char *filename, switch_loadable_module_t **new_module)
{
	switch_loadable_module_t *module = NULL;
	apr_dso_handle_t *dso = NULL;
	apr_status_t status = SWITCH_STATUS_SUCCESS;
	apr_dso_handle_sym_t function_handle = NULL;
	switch_module_load_t load_func_ptr = NULL;
	int loading = 1;
	const char *err = NULL;
	switch_loadable_module_interface_t *module_interface = NULL;
	char derr[512] = "";

	assert(filename != NULL);

	*new_module = NULL;
	status = apr_dso_load(&dso, filename, loadable_modules.pool);

	while (loading) {
		if (status != APR_SUCCESS) {
			apr_dso_error(dso, derr, sizeof(derr));
			err = derr;
			break;
		}

		status = apr_dso_sym(&function_handle, dso, "switch_module_load");
		load_func_ptr = (switch_module_load_t) function_handle;

		if (load_func_ptr == NULL) {
			err = "Cannot Load";
			break;
		}

		if (load_func_ptr(&module_interface, filename) != SWITCH_STATUS_SUCCESS) {
			err = "Module load routine returned an error";
			module_interface = NULL;
			break;
		}

		if ((module = switch_core_permenant_alloc(sizeof(switch_loadable_module_t))) == 0) {
			err = "Could not allocate memory\n";
			break;
		}

		loading = 0;
	}

	if (err) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Error Loading module %s\n**%s**\n", filename, err);
		return SWITCH_STATUS_GENERR;
	}

	module->filename = switch_core_permenant_strdup(filename);
	module->module_interface = module_interface;
	module->switch_module_load = load_func_ptr;

	if ((status = apr_dso_sym(&function_handle, dso, "switch_module_shutdown")) == APR_SUCCESS) {
		module->switch_module_shutdown = (switch_module_shutdown_t) function_handle;
	}

	if ((status = apr_dso_sym(&function_handle, dso, "switch_module_runtime")) == APR_SUCCESS) {
		module->switch_module_runtime = (switch_module_runtime_t) function_handle;
	}

	module->lib = dso;

	if (module->switch_module_runtime) {
		switch_core_launch_thread(switch_loadable_module_exec, module, loadable_modules.pool);
	}

	*new_module = module;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Successfully Loaded [%s]\n", module_interface->module_name);

	return SWITCH_STATUS_SUCCESS;

}

SWITCH_DECLARE(switch_status_t) switch_loadable_module_load_module(char *dir, char *fname)
{
	switch_size_t len = 0;
	char *path;
	char *file;
	switch_loadable_module_t *new_module = NULL;
	switch_status_t status;

#ifdef WIN32
	const char *ext = ".dll";
#elif defined (MACOSX) || defined (DARWIN)
	const char *ext = ".dylib";
#else
	const char *ext = ".so";
#endif


	if ((file = switch_core_strdup(loadable_modules.pool, fname)) == 0) {
		return SWITCH_STATUS_FALSE;
	}

	if (*file == '/') {
		path = switch_core_strdup(loadable_modules.pool, file);
	} else {
		if (strchr(file, '.')) {
			len = strlen(dir);
			len += strlen(file);
			len += 4;
			path = (char *) switch_core_alloc(loadable_modules.pool, len);
			snprintf(path, len, "%s%s%s", dir, SWITCH_PATH_SEPARATOR, file);
		} else {
			len = strlen(dir);
			len += strlen(file);
			len += 8;
			path = (char *) switch_core_alloc(loadable_modules.pool, len);
			snprintf(path, len, "%s%s%s%s", dir, SWITCH_PATH_SEPARATOR, file, ext);
		}
	}
	
	if ((status = switch_loadable_module_load_file(path, &new_module) == SWITCH_STATUS_SUCCESS)) {
		return switch_loadable_module_process((char *) file, new_module);
	} else {
		return status;
	}
}

SWITCH_DECLARE(switch_status_t) switch_loadable_module_build_dynamic(char *filename,
																   switch_module_load_t switch_module_load,
																   switch_module_runtime_t switch_module_runtime,
																   switch_module_shutdown_t switch_module_shutdown)
{ 
	switch_loadable_module_t *module = NULL; 
	switch_module_load_t load_func_ptr = NULL; 
	int loading = 1; 
	const char *err = NULL; 
	switch_loadable_module_interface_t *module_interface = NULL; 

	if ((module = switch_core_permenant_alloc(sizeof(switch_loadable_module_t))) == 0) { 
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Couldn't allocate memory\n"); 
		return SWITCH_STATUS_GENERR; 
	} 

	while (loading) { 
  		load_func_ptr = (switch_module_load_t) switch_module_load; 
  
		if (load_func_ptr == NULL) { 
			err = "Cannot Load"; 
			break; 
		} 
  
		if (load_func_ptr(&module_interface, filename) != SWITCH_STATUS_SUCCESS) { 
			err = "Module load routine returned an error"; 
			module_interface = NULL; 
			break; 
		} 
  
		if ((module = switch_core_permenant_alloc(sizeof(switch_loadable_module_t))) == 0) { 
			err = "Could not allocate memory\n"; 
			break; 
		} 
  
		loading = 0; 
	}
  
	if (err) { 
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Error Loading module %s\n**%s**\n", filename, err); 
		return SWITCH_STATUS_GENERR; 
	} 
  
	module->filename = switch_core_permenant_strdup(filename); 
	module->module_interface = module_interface; 
	module->switch_module_load = load_func_ptr; 
  
	if (switch_module_shutdown) { 
		module->switch_module_shutdown = switch_module_shutdown; 
	} 
	if (switch_module_runtime) { 
		module->switch_module_runtime = switch_module_runtime; 
	} 
	if (module->switch_module_runtime) { 
		switch_core_launch_thread(switch_loadable_module_exec, module, loadable_modules.pool); 
	} 
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Successfully Loaded [%s]\n", module_interface->module_name); 
	return switch_loadable_module_process((char *) module->filename, module);
} 

#ifdef WIN32
static void switch_loadable_module_path_init()
{
	char *path =NULL, *working =NULL;
	apr_dir_t *perl_dir_handle = NULL;

	apr_env_get(&path, "path", loadable_modules.pool);
	apr_filepath_get(&working, APR_FILEPATH_NATIVE , loadable_modules.pool);

	if (apr_dir_open(&perl_dir_handle, ".\\perl", loadable_modules.pool) == APR_SUCCESS) {
			apr_dir_close(perl_dir_handle);
			apr_env_set("path", 
						apr_pstrcat(loadable_modules.pool, path, ";", working, "\\perl", NULL), 
						loadable_modules.pool);
	}
}
#endif

SWITCH_DECLARE(switch_status_t) switch_loadable_module_init()
{

	apr_finfo_t finfo = {0};
	apr_dir_t *module_dir_handle = NULL;
	apr_int32_t finfo_flags = APR_FINFO_DIRENT | APR_FINFO_TYPE | APR_FINFO_NAME;
	char *cf = "modules.conf";
	char *pcf = "post_load_modules.conf";
	switch_xml_t cfg, xml;
	unsigned char all = 0;
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

	memset(&loadable_modules, 0, sizeof(loadable_modules));
	switch_core_new_memory_pool(&loadable_modules.pool);


#ifdef WIN32
	switch_loadable_module_path_init();
#endif

	switch_core_hash_init(&loadable_modules.module_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.endpoint_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.codec_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.timer_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.application_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.api_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.file_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.speech_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.directory_hash, loadable_modules.pool);
	switch_core_hash_init(&loadable_modules.dialplan_hash, loadable_modules.pool);

	if ((xml = switch_xml_open_cfg(cf, &cfg))) {
		switch_xml_t mods, ld;

		if ((mods = switch_xml_child(cfg, "modules"))) {
			for (ld = switch_xml_child(mods, "load"); ld; ld = ld->next) {
				const char *val = switch_xml_attr(ld, "module");
				if (strchr(val, '.') && !strstr(val, ext) && !strstr(val, EXT)) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Invalid extension for %s\n", val);
					continue;
				}
				switch_loadable_module_load_module((char *) SWITCH_GLOBAL_dirs.mod_dir, (char *) val);
				count++;
			}
		}
		switch_xml_free(xml);
		
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "open of %s failed\n", cf);
	}

	if ((xml = switch_xml_open_cfg(pcf, &cfg))) {
		switch_xml_t mods, ld;

		if ((mods = switch_xml_child(cfg, "modules"))) {
			for (ld = switch_xml_child(mods, "load"); ld; ld = ld->next) {
				const char *val = switch_xml_attr(ld, "module");
				if (strchr(val, '.') && !strstr(val, ext) && !strstr(val, EXT)) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Invalid extension for %s\n", val);
					continue;
				}
				switch_loadable_module_load_module((char *) SWITCH_GLOBAL_dirs.mod_dir, (char *) val);
				count++;
			}
		}
		switch_xml_free(xml);
		
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "open of %s failed\n", pcf);
	}

	if (!count) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "No modules loaded assuming 'load all'\n");
		all = 1;
	}
	
	if (all) {
		if (apr_dir_open(&module_dir_handle, SWITCH_GLOBAL_dirs.mod_dir, loadable_modules.pool) != APR_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Can't open directory: %s\n", SWITCH_GLOBAL_dirs.mod_dir);
			return SWITCH_STATUS_GENERR;
		}

		while (apr_dir_read(&finfo, finfo_flags, module_dir_handle) == APR_SUCCESS) {
			const char *fname = finfo.fname;

			if (finfo.filetype != APR_REG) {
				continue;
			}

			if (!fname) {
				fname = finfo.name;
			}

			if (!fname) {
				continue;
			}

			if (!strstr(fname, ext) && !strstr(fname, EXT)) {
				continue;
			}

			switch_loadable_module_load_module((char *) SWITCH_GLOBAL_dirs.mod_dir, (char *) fname);
		}
		apr_dir_close(module_dir_handle);
	}


	return SWITCH_STATUS_SUCCESS;
}

SWITCH_DECLARE(void) switch_loadable_module_shutdown(void)
{
	switch_hash_index_t *hi;
	void *val;
	switch_loadable_module_t *module;

	for (hi = switch_hash_first(loadable_modules.pool, loadable_modules.module_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		module = (switch_loadable_module_t *) val;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CONSOLE, "Checking %s\t", module->module_interface->module_name);
		if (module->switch_module_shutdown) {
			switch_log_printf(SWITCH_CHANNEL_LOG_CLEAN, SWITCH_LOG_CONSOLE, "(yes)\n");
			module->switch_module_shutdown();
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG_CLEAN, SWITCH_LOG_CONSOLE, "(no)\n");
		}
	}

}

SWITCH_DECLARE(switch_endpoint_interface_t *) switch_loadable_module_get_endpoint_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.endpoint_hash, name);
}

SWITCH_DECLARE(switch_codec_interface_t *) switch_loadable_module_get_codec_interface(char *name)
{
	char altname[256] = "";
	switch_codec_interface_t *codec;
	switch_size_t x;
	
	if (!(codec = switch_core_hash_find(loadable_modules.codec_hash, name))) {
		for(x = 0; x < strlen(name); x++) {
			altname[x] = (char)toupper(name[x]);
		}
		if (!(codec = switch_core_hash_find(loadable_modules.codec_hash, altname))) {
			for(x = 0; x < strlen(name); x++) {
				altname[x] = (char)tolower(name[x]);
			}
			codec = switch_core_hash_find(loadable_modules.codec_hash, altname);
		}
	}
	return codec;
}

SWITCH_DECLARE(switch_dialplan_interface_t *) switch_loadable_module_get_dialplan_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.dialplan_hash, name);
}

SWITCH_DECLARE(switch_timer_interface_t *) switch_loadable_module_get_timer_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.timer_hash, name);
}

SWITCH_DECLARE(switch_application_interface_t *) switch_loadable_module_get_application_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.application_hash, name);
}

SWITCH_DECLARE(switch_api_interface_t *) switch_loadable_module_get_api_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.api_hash, name);
}

SWITCH_DECLARE(switch_file_interface_t *) switch_loadable_module_get_file_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.file_hash, name);
}

SWITCH_DECLARE(switch_speech_interface_t *) switch_loadable_module_get_speech_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.speech_hash, name);
}

SWITCH_DECLARE(switch_directory_interface_t *) switch_loadable_module_get_directory_interface(char *name)
{
	return switch_core_hash_find(loadable_modules.directory_hash, name);
}

SWITCH_DECLARE(int) switch_loadable_module_get_codecs(switch_memory_pool_t *pool, switch_codec_interface_t **array,
													  int arraylen)
{
	switch_hash_index_t *hi;
	void *val;
	int i = 0;

	for (hi = switch_hash_first(pool, loadable_modules.codec_hash); hi; hi = switch_hash_next(hi)) {
		switch_hash_this(hi, NULL, NULL, &val);
		array[i++] = val;
		if (i > arraylen) {
			break;
		}
	}

	return i;

}

SWITCH_DECLARE(int) switch_loadable_module_get_codecs_sorted(switch_codec_interface_t **array,
															 int arraylen, char **prefs, int preflen)
{
	int x, i = 0;
	switch_codec_interface_t *codec_interface;

	for (x = 0; x < preflen; x++) {
		if ((codec_interface = switch_loadable_module_get_codec_interface(prefs[x])) != 0 ) {
			array[i++] = codec_interface;
			if (i > arraylen) {
				break;
			}
		}
	}

	return i;
}

SWITCH_DECLARE(switch_status_t) switch_api_execute(char *cmd, char *arg, char *retbuf, switch_size_t len)
{
	switch_api_interface_t *api;
	switch_status_t status;
	switch_event_t *event;

	if ((api = switch_loadable_module_get_api_interface(cmd)) != 0) {
		status = api->function(arg, retbuf, len);
	} else {
		status = SWITCH_STATUS_FALSE;
		snprintf(retbuf, len, "INVALID COMMAND [%s]", cmd);
	}

	if (switch_event_create(&event, SWITCH_EVENT_API) == SWITCH_STATUS_SUCCESS) {
		if (cmd) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "API-Command", cmd);
		}
		if (arg) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "API-Command-Arguement", arg);
		}
		switch_event_add_body(event, retbuf);
		switch_event_fire(&event);
	}


	return status;
}
