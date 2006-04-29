#include <switch.h>
#ifdef __ICC
#pragma warning (disable:1418)
#endif



#ifdef _MSC_VER
#include <perlibs.h>
#pragma comment(lib, PERL_LIB)
#endif

void fs_core_set_globals(void)
{
	switch_core_set_globals();
}

int fs_core_init(char *path)
{
	switch_status status;

	if (switch_strlen_zero(path)) {
		path = NULL;
	}

	status = switch_core_init(path);

	return status == SWITCH_STATUS_SUCCESS ? 1 : 0;
}

int fs_core_destroy(void)
{
	switch_status status;

	status = switch_core_destroy();

	return status == SWITCH_STATUS_SUCCESS ? 1 : 0;
}

int fs_loadable_module_init(void)
{
	return switch_loadable_module_init() == SWITCH_STATUS_SUCCESS ? 1 : 0;
}

int fs_loadable_module_shutdown(void)
{
	switch_loadable_module_shutdown();
	return 1;
}

int fs_console_loop(void) 
{
	switch_console_loop();
	return 0;
}

void fs_console_log(char *msg)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, msg);
}

void fs_console_clean(char *msg)
{
	switch_log_printf(SWITCH_CHANNEL_LOG_CLEAN, SWITCH_LOG_DEBUG, msg);
}

switch_core_session_t *fs_core_session_locate(char *uuid)
{
	return switch_core_session_locate(uuid);
}

void fs_channel_answer(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_answer(channel);
}

void fs_channel_pre_answer(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_pre_answer(channel);
}

void fs_channel_hangup(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
}

void fs_channel_set_variable(switch_core_session_t *session, char *var, char *val)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_set_variable(channel, var, val);
}

void fs_channel_get_variable(switch_core_session_t *session, char *var)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_get_variable(channel, var);
}

void fs_channel_set_state(switch_core_session_t *session, char *state)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_channel_state fs_state = switch_channel_get_state(channel);

	if (!strcmp(state, "EXECUTE")) {
		fs_state = CS_EXECUTE;
	} else 	if (!strcmp(state, "TRANSMIT")) {
		fs_state = CS_TRANSMIT;
	}
	
	switch_channel_set_state(channel, fs_state);
}

int fs_ivr_play_file(switch_core_session_t *session, char *file, char *timer_name) 
{
	switch_status status;
	if (switch_strlen_zero(timer_name)) {
		timer_name = NULL;
	}
	
	status = switch_ivr_play_file(session, NULL, file, timer_name, NULL, NULL, 0);
	return status == SWITCH_STATUS_SUCCESS ? 1 : 0;
}

