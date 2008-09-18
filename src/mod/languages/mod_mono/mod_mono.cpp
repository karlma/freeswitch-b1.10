/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application - mod_mono
 * Copyright (C) 2008, Michael Giagnocavo <mgg@packetrino.com>
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
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application - mod_mono
 *
 * The Initial Developer of the Original Code is
 * Michael Giagnocavo <mgg@packetrino.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Michael Giagnocavo <mgg@packetrino.com>
 * David Brazier <David.Brazier@360crm.co.uk>
 *
 * mod_mono.cpp -- FreeSWITCH mod_mono main class
 *
 * Most of mod_mono is implmented in the mod_mono_managed Loader class. 
 * The native code just handles getting the Mono runtime up and down
 * and passing pointers into managed code.
 */  
	
#include <switch.h>
	
SWITCH_BEGIN_EXTERN_C 
 
#include "freeswitch_mono.h"
#include <glib.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/debug-helpers.h>
	
#ifdef WIN32
#include <shlobj.h>
#define EXPORT __declspec(dllexport)
#else
#define EXPORT 
#endif	

#define MOD_MONO_MANAGED_DLL "mod_mono_managed.dll"

#define MOD_MONO_MANAGED_ASM_NAME "mod_mono_managed"
#define MOD_MONO_MANAGED_ASM_V1 1
#define MOD_MONO_MANAGED_ASM_V2 0
#define MOD_MONO_MANAGED_ASM_V3 0
#define MOD_MONO_MANAGED_ASM_V4 0

mod_mono_globals globals = 
{ 0 };

SWITCH_MODULE_LOAD_FUNCTION(mod_mono_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_mono_shutdown);
SWITCH_MODULE_DEFINITION(mod_mono, mod_mono_load, mod_mono_shutdown, NULL);

SWITCH_STANDARD_API(monorun_api_function);	/* ExecuteBackground */
SWITCH_STANDARD_API(mono_api_function);	/* Execute */
SWITCH_STANDARD_APP(mono_app_function);	/* Run */


// Sets up delegates (and anything else needed) on the MonoSession object
// Called via internalcall
SWITCH_MOD_DECLARE(void) InitMonoSession(MonoSession * session, MonoObject * dtmfDelegate, MonoObject * hangupDelegate) 
{
	switch_assert(session);
	if (!session) {
		return;
	}
	session->setDTMFCallback(NULL, "");
	session->setHangupHook(NULL);
	session->dtmfDelegateHandle = mono_gchandle_new(dtmfDelegate, FALSE);
	session->hangupDelegateHandle = mono_gchandle_new(hangupDelegate, FALSE);
}

switch_status_t setMonoDirs() 
{
#ifdef WIN32	
	// Win32 Mono installs can't figure out their own path
	// Guys in #mono say we should just deploy all the libs we need
	// We'll first check for Program Files\Mono to allow people to use the symlink dir for a specific version.
	// Then we'll check HKEY_LOCAL_MACHINE\SOFTWARE\Novell\Mono\2.0\FrameworkAssemblyDirectory and MonoConfigDir
	// After that, we'll scan program files for a Mono-* dir.
	char progFilesPath[MAX_PATH];
	char libPath[MAX_PATH];
	char etcPath[MAX_PATH];
	char findPath[MAX_PATH];
	bool found = false;

	SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, progFilesPath);

	{ // Check PF\Mono directly
		DWORD attr;
		switch_snprintf(findPath, MAX_PATH, "%s\\Mono", progFilesPath);
		attr = GetFileAttributes(findPath);
		found = (attr != INVALID_FILE_ATTRIBUTES && ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
		if (found) {
			switch_snprintf(libPath, MAX_PATH, "%s\\lib", findPath);
			switch_snprintf(etcPath, MAX_PATH, "%s\\etc", findPath);
		}
	}

	if(!found) 
	{   // Check registry
		DWORD size = MAX_PATH;
		if (ERROR_SUCCESS == RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Novell\\Mono\\2.0", "FrameworkAssemblyDirectory", RRF_RT_REG_SZ, NULL, &libPath, &size)) {
			size = MAX_PATH;
			if (ERROR_SUCCESS == RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Novell\\Mono\\2.0", "MonoConfigDir", RRF_RT_REG_SZ, NULL, &etcPath, &size)) {
				found = true;
			}
		}
	}

	if (!found)
	{ // Scan program files for Mono-2something
		HANDLE hFind;
		WIN32_FIND_DATA findData;
		switch_snprintf(findPath, MAX_PATH, "%s\\Mono-2*", progFilesPath);
		hFind = FindFirstFile(findPath, &findData);
		if (hFind == INVALID_HANDLE_VALUE) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error looking for Mono in Program Files.\n");
			return SWITCH_STATUS_FALSE;
		}

		while ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
			if (FindNextFile(hFind, &findData) == 0) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not find Mono directory in Program Files.\n");
				FindClose(hFind);
				return SWITCH_STATUS_FALSE;
			}
		}
		switch_snprintf(libPath, MAX_PATH, "%s\\%s\\lib", progFilesPath, findData.cFileName);
		switch_snprintf(etcPath, MAX_PATH, "%s\\%s\\etc", progFilesPath, findData.cFileName);
		FindClose(hFind);
	}

	/* Got it */ 
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Using Mono paths '%s' and '%s'.\n", libPath, etcPath);
	mono_set_dirs(libPath, etcPath);
	return SWITCH_STATUS_SUCCESS;

#else
	// On other platforms, it should just work if it hasn't been relocated
	mono_set_dirs(NULL, NULL);
	return SWITCH_STATUS_SUCCESS;

#endif	/* 
	*/
}

switch_status_t loadModMonoManaged() 
{
	/* Find and load mod_mono_managed.exe */ 
	char filename[256];

	switch_snprintf(filename, 256, "%s%s%s", SWITCH_GLOBAL_dirs.mod_dir, SWITCH_PATH_SEPARATOR, MOD_MONO_MANAGED_DLL);
	globals.domain = mono_jit_init(filename);

	/* Already got a Mono domain? */
	if ((globals.domain = mono_get_root_domain())) {
		mono_thread_attach(globals.domain);
		globals.embedded = SWITCH_TRUE;
	} else {
		if (!(globals.domain = mono_jit_init(filename))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "mono_jit_init failed.\n");
			return SWITCH_STATUS_FALSE;
		}
	}

	/* Already loaded? */
	MonoAssemblyName name;
	name.name = MOD_MONO_MANAGED_ASM_NAME;
	name.major = MOD_MONO_MANAGED_ASM_V1;
	name.minor = MOD_MONO_MANAGED_ASM_V2;
	name.revision = MOD_MONO_MANAGED_ASM_V3;
	name.build = MOD_MONO_MANAGED_ASM_V4;
	name.culture = "";
	name.hash_value = "";

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Calling mono_assembly_loaded");

	if (!(globals.mod_mono_asm = mono_assembly_loaded(&name))) {
		/* Open the assembly */ 
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Calling mono_domain_assembly_open");
		globals.mod_mono_asm = mono_domain_assembly_open(globals.domain, filename);
		if (!globals.mod_mono_asm) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "mono_domain_assembly_open failed.\n");
			return SWITCH_STATUS_FALSE;
		}
	}

	return SWITCH_STATUS_SUCCESS;

}

MonoMethod * getMethod(const char *name, MonoClass * klass) 
{
	MonoMethodDesc * desc;
	MonoMethod * method;

	desc = mono_method_desc_new(name, TRUE);
	method = mono_method_desc_search_in_class(desc, klass);

	if (!method) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not find %s method.\n", name);
		return NULL;
	}

	return method;
}

switch_status_t findLoader() 
{
	/* Find loader class and methods */ 
	MonoClass * loaderClass;
	MonoImage * img = mono_assembly_get_image(globals.mod_mono_asm);

	if (!(loaderClass = mono_class_from_name(img, "FreeSWITCH", "Loader"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not find FreeSWITCH.Loader class.\n");
		return SWITCH_STATUS_FALSE;
	}

	if (!(globals.loadMethod = getMethod("FreeSWITCH.Loader:Load()", loaderClass))) {
		return SWITCH_STATUS_FALSE;
	}

	if (!(globals.unloadMethod = getMethod("FreeSWITCH.Loader:Unload()", loaderClass))) {
		return SWITCH_STATUS_FALSE;
	}

	if (!(globals.runMethod = getMethod("FreeSWITCH.Loader:Run(string,intptr)", loaderClass))) {
		return SWITCH_STATUS_FALSE;
	}

	if (!(globals.executeMethod = getMethod("FreeSWITCH.Loader:Execute(string,intptr,intptr)", loaderClass))) {
		return SWITCH_STATUS_FALSE;
	}

	if (!(globals.executeBackgroundMethod = getMethod("FreeSWITCH.Loader:ExecuteBackground(string)", loaderClass))) {
		return SWITCH_STATUS_FALSE;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Found all loader functions.\n");
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_mono_load) 
{
	/* connect my internal structure to the blank pointer passed to me */ 
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	globals.pool = pool;

	if (setMonoDirs() != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_FALSE;
	}

	if (loadModMonoManaged() != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_FALSE;
	}

	if (findLoader() != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_FALSE;
	}

	/* Not sure if this is necesary on the loading thread */ 
	mono_thread_attach(globals.domain);

	mono_add_internal_call("FreeSWITCH.Native.MonoSession::InitMonoSession", (void *)InitMonoSession);

	/* Run loader */ 
	MonoObject * objResult;
	MonoObject * exception = NULL;

	objResult = mono_runtime_invoke(globals.loadMethod, NULL, NULL, &exception);

	if (exception) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Load threw an exception.\n");
		mono_print_unhandled_exception(exception);
		return SWITCH_STATUS_FALSE;
	}

	if (*(int *) mono_object_unbox(objResult)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Load completed successfully.\n");
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Load did not return true.\n");
		return SWITCH_STATUS_FALSE;
	}

	/* We're good to register */ 
	switch_api_interface_t *api_interface;
	switch_application_interface_t *app_interface;

	SWITCH_ADD_API(api_interface, "monorun", "Run a module (ExecuteBackground)", monorun_api_function, "<module> [<args>]");
	SWITCH_ADD_API(api_interface, "mono", "Run a module as an API function (Execute)", mono_api_function, "<module> [<args>]");
	SWITCH_ADD_APP(app_interface, "mono", "Run Mono IVR", "Run a Mono IVR on a channel", mono_app_function, "<modulename> [<args>]", SAF_NONE);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(monorun_api_function) 
{
	// TODO: Should we be detaching after all this?
	mono_thread_attach(globals.domain);

	if (switch_strlen_zero(cmd)) {
		stream->write_function(stream, "-ERR no args specified!\n");
		return SWITCH_STATUS_SUCCESS;
	}

	// ExecuteBackground(string command)
	void *args[1];

	args[0] = mono_string_new(globals.domain, cmd);
	MonoObject * exception = NULL;
	MonoObject * objResult = mono_runtime_invoke(globals.executeBackgroundMethod, NULL, args, &exception);

	if (exception) {
		stream->write_function(stream, "-ERR FreeSWITCH.Loader.ExecuteBackground threw an exception.\n");
		mono_print_unhandled_exception(exception);
		return SWITCH_STATUS_SUCCESS;
	}

	if (*(int *) mono_object_unbox(objResult)) {
		stream->write_function(stream, "+OK\n");
	} else {
		stream->write_function(stream, "-ERR ExecuteBackground returned false (unknown module?).\n");
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_API(mono_api_function) 
{
	mono_thread_attach(globals.domain);

	if (switch_strlen_zero(cmd)) {
		stream->write_function(stream, "-ERR no args specified!\n");
		return SWITCH_STATUS_SUCCESS;
	}

	// Method is: Execute(string command, IntPtr streamPtr, IntPtr eventPtr)
	void *args[3];

	args[0] = mono_string_new(globals.domain, cmd);
	args[1] = &stream;			// Address of the arguments
	args[2] = &(stream->param_event);

	MonoObject * exception = NULL;
	MonoObject * objResult = mono_runtime_invoke(globals.executeMethod, NULL, args, &exception);

	if (exception) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Exception trying to execute mono %s.\n", cmd);
		mono_print_unhandled_exception(exception);
	}

	if (!(*(int *) mono_object_unbox(objResult))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Execute failed for %s (unknown module?).\n", cmd);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_APP(mono_app_function) 
{
	mono_thread_attach(globals.domain);

	if (switch_strlen_zero(data)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No args specified!\n");
	}

	// bool Run(string command, IntPtr sessionHandle)
	void *args[2];

	args[0] = mono_string_new(globals.domain, data);
	args[1] = &session;

	MonoObject * exception = NULL;
	MonoObject * objResult = mono_runtime_invoke(globals.runMethod, NULL, args, &exception);

	if (exception) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Exception trying to execute application mono %s.\n", data);
		mono_print_unhandled_exception(exception);
	}

	if (!(*(int *) mono_object_unbox(objResult))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Application run failed for %s (unknown module?).\n", data);
	}
} 

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_mono_shutdown) 
{
	MonoObject * ex;

	mono_thread_attach(globals.domain);
	mono_runtime_invoke(globals.unloadMethod, NULL, NULL, &ex);

	if (ex) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Exception occurred in Loader::Unload.\n");
		mono_print_unhandled_exception(ex);
	}

	if (!globals.embedded) {
		mono_jit_cleanup(globals.domain);
	}

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_END_EXTERN_C
