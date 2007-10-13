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
 * mod_voicemail.c -- Voicemail Module
 *
 */
#include <switch.h>

#ifdef SWITCH_HAVE_ODBC
#include <switch_odbc.h>
#endif

#ifdef _MSC_VER /* compilers are stupid sometimes */
#define TRY_CODE(code) for(;;) {status = code; if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) { goto end; } break;}
#else
#define TRY_CODE(code) do {status = code; if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) { goto end; } break;} while(status)
#endif

SWITCH_MODULE_LOAD_FUNCTION(mod_voicemail_load);
SWITCH_MODULE_DEFINITION(mod_voicemail, mod_voicemail_load, NULL, NULL);

static struct {
	switch_hash_t *profile_hash;
    int debug;
    switch_memory_pool_t *pool;
} globals;

struct vm_profile {
    char *name;
    char *dbname;
	char *odbc_dsn;
	char *odbc_user;
	char *odbc_pass;
    char terminator_key[2];
    char play_new_messages_key[2];
    char play_saved_messages_key[2];

    char main_menu_key[2];    
    char config_menu_key[2];
    char record_greeting_key[2];
    char choose_greeting_key[2];
    char record_name_key[2];
    
    char record_file_key[2];
    char listen_file_key[2];
    char save_file_key[2];
    char delete_file_key[2];
    char undelete_file_key[2];
    char email_key[2];
    char pause_key[2];
    char restart_key[2];
    char ff_key[2];
    char rew_key[2];
    char urgent_key[2];
    char file_ext[10];
    char *tone_spec;
    uint32_t digit_timeout;
    uint32_t max_login_attempts;
    uint32_t max_record_len;
    switch_mutex_t *mutex;
#ifdef SWITCH_HAVE_ODBC
	switch_odbc_handle_t *master_odbc;
#else
	void *filler1;
#endif
};
typedef struct vm_profile vm_profile_t;

static switch_status_t vm_execute_sql(vm_profile_t *profile, char *sql, switch_mutex_t *mutex)
{
	switch_core_db_t *db;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

	if (mutex) {
		switch_mutex_lock(mutex);
	}

#ifdef SWITCH_HAVE_ODBC
    if (profile->odbc_dsn) {
		SQLHSTMT stmt;
		if (switch_odbc_handle_exec(profile->master_odbc, sql, &stmt) != SWITCH_ODBC_SUCCESS) {
			char *err_str;
			err_str = switch_odbc_handle_get_error(profile->master_odbc, stmt);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, switch_str_nil(err_str));
			switch_safe_free(err_str);
            status = SWITCH_STATUS_FALSE;
		}
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	} else {
#endif
		if (!(db = switch_core_db_open_file(profile->dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
            status = SWITCH_STATUS_FALSE;
			goto end;
		}

		status = switch_core_db_persistant_execute(db, sql, 25);
		switch_core_db_close(db);

#ifdef SWITCH_HAVE_ODBC
    }
#endif


 end:
	if (mutex) {
		switch_mutex_unlock(mutex);
	}

    return status;
}


static switch_bool_t vm_execute_sql_callback(vm_profile_t *profile,
											  switch_mutex_t *mutex,
											  char *sql,
											  switch_core_db_callback_func_t callback,
											  void *pdata)
{
	switch_bool_t ret = SWITCH_FALSE;
	switch_core_db_t *db;
	char *errmsg = NULL;
	
	if (mutex) {
        switch_mutex_lock(mutex);
    }


#ifdef SWITCH_HAVE_ODBC
    if (profile->odbc_dsn) {
		switch_odbc_handle_callback_exec(profile->master_odbc, sql, callback, pdata);
	} else {
#endif



		if (!(db = switch_core_db_open_file(profile->dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", profile->dbname);
			goto end;
		}

	
		switch_core_db_exec(db, sql, callback, pdata, &errmsg);

		if (errmsg) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "SQL ERR: [%s] %s\n", sql, errmsg);
			free(errmsg);
		}

		if (db) {
			switch_core_db_close(db);
		}

#ifdef SWITCH_HAVE_ODBC
    }
#endif


 end:

	if (mutex) {
        switch_mutex_unlock(mutex);
    }
	


	return ret;

}


static char vm_sql[] =
	"CREATE TABLE voicemail_data (\n"
	"   created_epoch INTEGER(8),\n"
	"   read_epoch    INTEGER(8),\n" 
	"   user          VARCHAR(255),\n" 
	"   domain        VARCHAR(255),\n" 
	"   uuid          VARCHAR(255),\n" 
	"   cid_name      VARCHAR(255),\n" 
	"   cid_number    VARCHAR(255),\n" 
	"   in_folder     VARCHAR(255),\n" 
	"   file_path     VARCHAR(255),\n" 
	"   flags         VARCHAR(255),\n"
	"   read_flags    VARCHAR(255)\n" 
	");\n";


static char vm_pref_sql[] =
	"CREATE TABLE voicemail_prefs (\n"
	"   user            VARCHAR(255),\n" 
	"   domain          VARCHAR(255),\n" 
	"   name_path       VARCHAR(255),\n" 
	"   greeting_path VARCHAR(255)\n" 
	");\n";




static switch_status_t load_config(void)
{
	char *cf = "voicemail.conf";
	vm_profile_t *profile = NULL;
	switch_xml_t cfg, xml, settings, param, x_profile, x_profiles;

	memset(&globals, 0, sizeof(globals));
    switch_core_new_memory_pool(&globals.pool);
	switch_core_hash_init(&globals.profile_hash, globals.pool);

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}
    
	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");
            
			if (!strcasecmp(var, "debug")) {
				globals.debug = atoi(val);
			}
		}
	}

	if (!(x_profiles = switch_xml_child(cfg, "profiles"))) {
        goto end;
    }

    for (x_profile = switch_xml_child(x_profiles, "profile"); x_profile; x_profile = x_profile->next) {
        char *name = (char *) switch_xml_attr_soft(x_profile, "name");
        char *odbc_dsn = NULL, *odbc_user = NULL, *odbc_pass = NULL;
        char *terminator_key = "#";
        char *play_new_messages_key = "1";
        char *play_saved_messages_key = "2";
        
        char *main_menu_key = "0";    
        char *config_menu_key = "5";
        char *record_greeting_key = "1";
        char *choose_greeting_key = "2";
        char *record_name_key = "3";
        
        char *record_file_key = "3";
        char *listen_file_key = "1";
        char *save_file_key = "2";
        char *delete_file_key = "7";
        char *undelete_file_key = "8";
        char *email_key = "4";
        char *pause_key = "0";
        char *restart_key = "1";
        char *ff_key = "6";
        char *rew_key = "4";
        char *urgent_key = "*";
        char *tone_spec = "%(1000, 0, 640)";
        char *file_ext = "wav";

        switch_core_db_t *db;
        uint32_t timeout = 10000, max_login_attempts = 3, max_record_len = 300;
        
        db = NULL;
        for (param = switch_xml_child(x_profile, "param"); param; param = param->next) {
            char *var, *val;

            var = (char *) switch_xml_attr_soft(param, "name");
            val = (char *) switch_xml_attr_soft(param, "value");

            if (!strcasecmp(var, "terminator-key") && !switch_strlen_zero(val)) {
                terminator_key = val;
            } else if (!strcasecmp(var, "play-new-messages-key") && !switch_strlen_zero(val)) {
                play_new_messages_key = val;
            } else if (!strcasecmp(var, "play-saved-messages-key") && !switch_strlen_zero(val)) {
                play_saved_messages_key = val;
            } else if (!strcasecmp(var, "main-menu-key") && !switch_strlen_zero(val)) {
                main_menu_key = val;
            } else if (!strcasecmp(var, "config-menu-key") && !switch_strlen_zero(val)) {
                config_menu_key = val;
            } else if (!strcasecmp(var, "record-greeting-key") && !switch_strlen_zero(val)) {
                record_greeting_key = val;
            } else if (!strcasecmp(var, "choose-greeting-key") && !switch_strlen_zero(val)) {
                choose_greeting_key = val;
            } else if (!strcasecmp(var, "record-name-key") && !switch_strlen_zero(val)) {
                record_name_key = val;
            } else if (!strcasecmp(var, "listen-file-key") && !switch_strlen_zero(val)) {
                listen_file_key = val;
            } else if (!strcasecmp(var, "save-file-key") && !switch_strlen_zero(val)) {
                save_file_key = val;
            } else if (!strcasecmp(var, "delete-file-key") && !switch_strlen_zero(val)) {
                delete_file_key = val;
            } else if (!strcasecmp(var, "undelete-file-key") && !switch_strlen_zero(val)) {
                undelete_file_key = val;
            } else if (!strcasecmp(var, "email-key") && !switch_strlen_zero(val)) {
                email_key = val;
            } else if (!strcasecmp(var, "pause-key") && !switch_strlen_zero(val)) {
                pause_key = val;
            } else if (!strcasecmp(var, "restart-key") && !switch_strlen_zero(val)) {
                restart_key = val;
            } else if (!strcasecmp(var, "ff-key") && !switch_strlen_zero(val)) {
                ff_key = val;
            } else if (!strcasecmp(var, "rew-key") && !switch_strlen_zero(val)) {
                rew_key = val;
            } else if (!strcasecmp(var, "urgent-key") && !switch_strlen_zero(val)) {
                urgent_key = val;
            } else if (!strcasecmp(var, "file-extension")) {
                file_ext = val;
            } else if (!strcasecmp(var, "tone-spec")) {
                tone_spec = val;
            } else if (!strcasecmp(var, "digit-timeout")) {
                int tmp = 0;
                if (!switch_strlen_zero(val)) {
                    tmp = atoi(val);
                }
                if (tmp >= 1000 && tmp <= 30000) {
                    timeout = tmp;
                } else {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "invalid timeout value [%s] must be between 1000 and 30000 ms\n", val);
                }
            } else if (!strcasecmp(var, "max-login-attempts")) {
                int tmp = 0;
                if (!switch_strlen_zero(val)) {
                    tmp = atoi(val);
                }
                if (tmp > 0 && tmp < 11) {
                    max_login_attempts = tmp;
                } else {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "invalid attempts [%s] must be between 1 and 10 ms\n", val);
                }
            } else if (!strcasecmp(var, "max-record-len")) {
                int tmp = 0;
                if (!switch_strlen_zero(val)) {
                    tmp = atoi(val);
                }
                if (tmp > 0 && tmp < 10000) {
                    max_record_len = tmp;
                } else {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "invalid attempts [%s] must be between 1 and 10000s\n", val);
                }
            } else if (!strcasecmp(var, "odbc-dsn")) {
#ifdef SWITCH_HAVE_ODBC
                odbc_dsn = switch_core_strdup(globals.pool, val);
                if ((odbc_user = strchr(odbc_dsn, ':'))) {
                    *odbc_user++ = '\0';
                }
                if ((odbc_pass = strchr(odbc_user, ':'))) {
                    *odbc_pass++ = '\0';
                }
#else
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ODBC IS NOT AVAILABLE!\n");
#endif
            }
        }
        
        if (switch_strlen_zero(name)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No name specified.\n");
        } else {
            profile = switch_core_alloc(globals.pool, sizeof(*profile));
            profile->name = switch_core_strdup(globals.pool, name);

            if (!switch_strlen_zero(odbc_dsn) && !switch_strlen_zero(odbc_user) && !switch_strlen_zero(odbc_pass)) {
                profile->odbc_dsn = odbc_dsn;
                profile->odbc_user = odbc_user;
                profile->odbc_pass = odbc_pass;
            } else {
                profile->dbname = switch_core_sprintf(globals.pool, "voicemail_%s", name);
            }
#ifdef SWITCH_HAVE_ODBC
			if (profile->odbc_dsn) {
				if (!(profile->master_odbc = switch_odbc_handle_new(profile->odbc_dsn, profile->odbc_user, profile->odbc_pass))) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
					continue;
					
				}
				if (switch_odbc_handle_connect(profile->master_odbc) != SWITCH_ODBC_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
					continue;
				}

				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Connected ODBC DSN: %s\n", profile->odbc_dsn);
				switch_odbc_handle_exec(profile->master_odbc, vm_sql, NULL);
				switch_odbc_handle_exec(profile->master_odbc, vm_pref_sql, NULL);
			} else {
#endif
				if ((db = switch_core_db_open_file(profile->dbname))) {
					switch_core_db_test_reactive(db, "select * from voicemail_data", vm_sql);
					switch_core_db_test_reactive(db, "select * from voicemail_prefs", vm_pref_sql);
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open SQL Database!\n");
					continue;
				}
				switch_core_db_close(db);
#ifdef SWITCH_HAVE_ODBC
			}            
#endif
            profile->digit_timeout = timeout;
            profile->max_login_attempts = max_login_attempts;
            profile->max_record_len = max_record_len;
            *profile->terminator_key = *terminator_key;
            *profile->play_new_messages_key = *play_new_messages_key;
            *profile->play_saved_messages_key = *play_saved_messages_key;
            *profile->main_menu_key = *main_menu_key;
            *profile->config_menu_key = *config_menu_key;
            *profile->record_greeting_key = *record_greeting_key;
            *profile->choose_greeting_key = *choose_greeting_key;
            *profile->record_name_key = *record_name_key;
            *profile->record_file_key = *record_file_key;
            *profile->listen_file_key = *listen_file_key;
            *profile->save_file_key = *save_file_key;
            *profile->delete_file_key = *delete_file_key;
            *profile->undelete_file_key = *undelete_file_key;
            *profile->email_key = *email_key;
            *profile->pause_key = *pause_key;
            *profile->restart_key = *restart_key;
            *profile->ff_key = *ff_key;
            *profile->rew_key = *rew_key;
            *profile->urgent_key = *urgent_key;

            profile->tone_spec = switch_core_strdup(globals.pool, tone_spec);
            switch_copy_string(profile->file_ext, file_ext, sizeof(profile->file_ext));
            switch_mutex_init(&profile->mutex, SWITCH_MUTEX_NESTED, globals.pool);
            
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Added Profile %s\n", profile->name);
            switch_core_hash_insert(globals.profile_hash, profile->name, profile);
            
        }
        
    }
        
 end:

	switch_xml_free(xml);

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t cancel_on_dtmf(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{


	switch (itype) {
	case SWITCH_INPUT_TYPE_DTMF:
        {
            char *dtmf = (char *) input;
            if (buf && buflen) {
                switch_copy_string(buf, dtmf, buflen);
            }
            return SWITCH_STATUS_BREAK;
        }
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}


struct call_control {
    vm_profile_t *profile;
    switch_file_handle_t *fh;
    char buf[4];
    int noexit;
};
typedef struct call_control cc_t;

static switch_status_t control_playback(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{


	switch (itype) {
	case SWITCH_INPUT_TYPE_DTMF:
        {
            char *dtmf = (char *) input;
            cc_t *cc = (cc_t *) buf;
            switch_file_handle_t *fh = cc->fh;
            uint32_t pos = 0;

            if (!cc->noexit && (*dtmf == *cc->profile->delete_file_key || *dtmf == *cc->profile->save_file_key || *dtmf == *cc->profile->terminator_key)) {
                *cc->buf = *dtmf;
                return SWITCH_STATUS_BREAK;
            }

            if (!fh) {
                return SWITCH_STATUS_SUCCESS;
            }

            if (*dtmf == *cc->profile->pause_key) {
                if (switch_test_flag(fh, SWITCH_FILE_PAUSE)) {
                    switch_clear_flag(fh, SWITCH_FILE_PAUSE);
                } else {
                    switch_set_flag(fh, SWITCH_FILE_PAUSE);
                }
                return SWITCH_STATUS_SUCCESS;
            }

            if (*dtmf == *cc->profile->restart_key) {
                unsigned int pos = 0;
                fh->speed = 0;
                switch_core_file_seek(fh, &pos, 0, SEEK_SET);
                return SWITCH_STATUS_SUCCESS;
            }

            if (*dtmf == *cc->profile->ff_key) {
                int samps = 24000;
                switch_core_file_seek(fh, &pos, samps, SEEK_CUR);
                return SWITCH_STATUS_SUCCESS;
            }

            if (*dtmf == *cc->profile->rew_key) {
                int samps = 24000;
                switch_core_file_seek(fh, &pos, fh->pos - samps, SEEK_SET);
                return SWITCH_STATUS_SUCCESS;
            }

        }
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}

struct prefs_callback {
    char name_path[255];
    char greeting_path[255];
};
typedef struct prefs_callback prefs_callback_t;

static int prefs_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	prefs_callback_t *cbt = (prefs_callback_t *) pArg;

	switch_copy_string(cbt->name_path, argv[2], sizeof(cbt->name_path));
	switch_copy_string(cbt->greeting_path, argv[3], sizeof(cbt->greeting_path));
    
	return 0;
}


typedef enum {
    VM_CHECK_START,
    VM_CHECK_AUTH,
    VM_CHECK_MENU,
    VM_CHECK_CONFIG,
    VM_CHECK_PLAY_MESSAGES,
    VM_CHECK_FOLDER_SUMMARY,
    VM_CHECK_LISTEN
} vm_check_state_t;


#define VM_ACK_MACRO "voicemail_ack"
#define VM_SAY_DATE_MACRO "voicemail_say_date"
#define VM_PLAY_GREETING_MACRO "voicemail_play_greeting"
#define VM_SAY_MESSAGE_NUMBER_MACRO "voicemail_say_message_number"
#define VM_SAY_NUMBER_MACRO "voicemail_say_number"
#define VM_SAY_PHONE_NUMBER_MACRO "voicemail_say_phone_number"
#define VM_SAY_NAME_MACRO "voicemail_say_name"

#define VM_RECORD_MESSAGE_MACRO "voicemail_record_message"
#define VM_CHOOSE_GREETING_MACRO "voicemail_choose_greeting"
#define VM_CHOOSE_GREETING_FAIL_MACRO "voicemail_choose_greeting_fail"
#define VM_CHOOSE_GREETING_SELECTED_MACRO "voicemail_greeting_selected"
#define VM_RECORD_GREETING_MACRO "voicemail_record_greeting"
#define VM_RECORD_NAME_MACRO "voicemail_record_name"
#define VM_LISTEN_FILE_CHECK_MACRO "voicemail_listen_file_check"
#define VM_RECORD_FILE_CHECK_MACRO "voicemail_record_file_check"
#define VM_RECORD_URGENT_CHECK_MACRO "voicemail_record_urgent_check"
#define VM_MENU_MACRO "voicemail_menu"
#define VM_CONFIG_MENU_MACRO "voicemail_config_menu"
#define VM_ENTER_ID_MACRO "voicemail_enter_id"
#define VM_ENTER_PASS_MACRO "voicemail_enter_pass"
#define VM_FAIL_AUTH_MACRO "voicemail_fail_auth"
#define VM_ABORT_MACRO "voicemail_abort"
#define VM_HELLO_MACRO "voicemail_hello"
#define VM_GOODBYE_MACRO "voicemail_goodbye"
#define VM_MESSAGE_COUNT_MACRO "voicemail_message_count"
#define URGENT_FLAG_STRING "A_URGENT"
#define NORMAL_FLAG_STRING "B_NORMAL"


static switch_status_t vm_macro_get(switch_core_session_t *session,
                                    char *macro,
                                    char *macro_arg,
                                    char *buf,
                                    switch_size_t buflen,
                                    switch_size_t maxlen,
                                    char *term_chars,
                                    char *terminator_key,
                                    uint32_t timeout)
{
    switch_input_args_t args = { 0 }, *ap = NULL;
    switch_channel_t *channel;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_size_t bslen;

    channel = switch_core_session_get_channel(session);
    assert(channel != NULL);
    
    if (buf && buflen) {
        memset(buf, 0, buflen);
        args.input_callback = cancel_on_dtmf;
        args.buf = buf;
        args.buflen = sizeof(buf);
        ap = &args;
    }

    status = switch_ivr_phrase_macro(session, macro, macro_arg, NULL, ap);
    
    if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
        if (buf) {
            memset(buf, 0, buflen);
        }
        return status;
    }

    if (!buf) {
        return status;
    }

    bslen = strlen(buf);

    if (maxlen == 0 || maxlen > buflen - 1) {
        maxlen = buflen -1;
    }
    if (bslen < maxlen) {
        status = switch_ivr_collect_digits_count(session, buf + bslen, buflen, maxlen - bslen, term_chars, terminator_key, timeout);
    }

    return status;
}

struct callback {
    char *buf;
    size_t len;
    int matches;
};
typedef struct callback callback_t;

static int sql2str_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	callback_t *cbt = (callback_t *) pArg;

	switch_copy_string(cbt->buf, argv[0], cbt->len);
	cbt->matches++;
	return 0;
}


static int unlink_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    if (argv[0]) {
        unlink(argv[0]);
    }
	return 0;
}


typedef enum {
    MSG_NONE,
    MSG_NEW,
    MSG_SAVED
} msg_type_t;

static uint32_t DEFAULT_DIR_PERMS = SWITCH_FPROT_UREAD | SWITCH_FPROT_UWRITE | SWITCH_FPROT_UEXECUTE | SWITCH_FPROT_GREAD | SWITCH_FPROT_GEXECUTE;


static switch_status_t create_file(switch_core_session_t *session, vm_profile_t *profile, char *macro_name, char *file_path)
{
    switch_channel_t *channel;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_file_handle_t fh = { 0 };
    switch_input_args_t args = { 0 };
    char term;
    char input[10] = "" , key_buf[80] = "";
    cc_t cc = { 0 };

    channel = switch_core_session_get_channel(session);
    assert(channel != NULL);
    

    while(switch_channel_ready(channel)) {

        
        switch_channel_set_variable(channel, "RECORD_COMMENT", macro_name);




        
        snprintf(key_buf, sizeof(key_buf), "%s:%s:%s", 
                 profile->listen_file_key,
                 profile->save_file_key,
                 profile->record_file_key);


    record_file:
        args.input_callback = cancel_on_dtmf;
        TRY_CODE(switch_ivr_phrase_macro(session, macro_name, NULL, NULL, NULL));
        TRY_CODE(switch_ivr_gentones(session, profile->tone_spec, 0, NULL));

        memset(&fh, 0, sizeof(fh));
        fh.thresh = 200;
        fh.silence_hits = 2;


        switch_ivr_record_file(session, &fh, file_path, &args, profile->max_record_len);
        status = SWITCH_STATUS_SUCCESS;

    play_file:
        memset(&fh, 0, sizeof(fh));
        args.input_callback = control_playback;
        memset(&cc, 0, sizeof(cc));
        cc.profile = profile;
        cc.fh = &fh;
        args.buf = &cc;
        switch_ivr_play_file(session, &fh, file_path, &args);
        
        
        while(switch_channel_ready(channel)) {
            if (*cc.buf) {
                *input = *cc.buf;
                *(input+1) = '\0';
                status = SWITCH_STATUS_SUCCESS;
                *cc.buf = '\0';
            } else {
                status = vm_macro_get(session, VM_RECORD_FILE_CHECK_MACRO, 
                                      key_buf, input, sizeof(input), 1, profile->terminator_key, &term, profile->digit_timeout);
            }

            if (!strcmp(input, profile->listen_file_key)) {
                goto play_file;
            } else if (!strcmp(input, profile->record_file_key)) {
                goto record_file;
            } else {
                TRY_CODE(switch_ivr_phrase_macro(session, VM_ACK_MACRO, "saved", NULL, NULL));
                goto end;
            }
        }
    }
    
 end:

    return status;
}


struct listen_callback {
    char created_epoch[255];
    char read_epoch[255];
    char user[255];
    char domain[255];
    char uuid[255];
    char cid_name[255];
    char cid_number[255];
    char in_folder[255];
    char file_path[255];
    char flags[255];
    char read_flags[255];
    char *email;
    int index;
    int want;
    msg_type_t type;
};
typedef struct listen_callback listen_callback_t;

static int listen_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	listen_callback_t *cbt = (listen_callback_t *) pArg;

    if (cbt->index++ != cbt->want) {
        return 0;
    }

    switch_copy_string(cbt->created_epoch, argv[0], 255);
	switch_copy_string(cbt->read_epoch, argv[1], 255);
	switch_copy_string(cbt->user, argv[2], 255);
	switch_copy_string(cbt->domain, argv[3], 255);
	switch_copy_string(cbt->uuid, argv[4], 255);
	switch_copy_string(cbt->cid_name, argv[5], 255);
	switch_copy_string(cbt->cid_number, argv[6], 255);
	switch_copy_string(cbt->in_folder, argv[7], 255);
	switch_copy_string(cbt->file_path, argv[8], 255);
	switch_copy_string(cbt->flags, argv[9], 255);
	switch_copy_string(cbt->read_flags, argv[10], 255);
    
    
	return -1;
}


static switch_status_t listen_file(switch_core_session_t *session, vm_profile_t *profile, listen_callback_t *cbt)
{
    switch_channel_t *channel;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_input_args_t args = { 0 };
    char term;
    char input[10] = "" , key_buf[80] = "";
    switch_file_handle_t fh = { 0 };
    cc_t cc = { 0 };
    channel = switch_core_session_get_channel(session);
    assert(channel != NULL);
    

    if(switch_channel_ready(channel)) {
        
        args.input_callback = cancel_on_dtmf;


        snprintf(key_buf, sizeof(key_buf), "%s:%s:%s:%s", 
                 profile->listen_file_key,
                 profile->save_file_key,
                 profile->delete_file_key,
                 profile->email_key);


        snprintf(input, sizeof(input), "%s:%d", cbt->type == MSG_NEW ? "new" : "saved", cbt->want+1);
        memset(&cc, 0, sizeof(cc));
        cc.profile = profile;
        args.buf = &cc;
        args.input_callback = control_playback;
        TRY_CODE(switch_ivr_phrase_macro(session, VM_SAY_MESSAGE_NUMBER_MACRO, input, NULL, &args));
        if (!*cc.buf) {
            TRY_CODE(switch_ivr_phrase_macro(session, VM_SAY_DATE_MACRO, cbt->created_epoch, NULL, &args));
        }
    play_file:

        if (!*cc.buf) {
            memset(&fh, 0, sizeof(fh));
            args.input_callback = control_playback;
            memset(&cc, 0, sizeof(cc));
            cc.profile = profile;
            cc.fh = &fh;
            args.buf = &cc;
            TRY_CODE(switch_ivr_play_file(session, NULL, cbt->file_path, &args));
        }

        if (switch_channel_ready(channel)) {
            if (*cc.buf) {
                *input = *cc.buf;
                *(input+1) = '\0';
                status = SWITCH_STATUS_SUCCESS;
                *cc.buf = '\0';
            } else {
                status = vm_macro_get(session, VM_LISTEN_FILE_CHECK_MACRO, 
                                      key_buf, input, sizeof(input), 1, profile->terminator_key, &term, profile->digit_timeout);
            }
            if (!strcmp(input, profile->listen_file_key)) {
                goto play_file;
            } else if (!strcmp(input, profile->delete_file_key) || !strcmp(input, profile->email_key)) {
                char *sql = switch_mprintf("update voicemail_data set flags='delete' where uuid='%s'", cbt->uuid);
                vm_execute_sql(profile, sql, profile->mutex);
                switch_safe_free(sql);
                if (!strcmp(input, profile->email_key) && !switch_strlen_zero(cbt->email)) {
                    switch_event_t *event;
                    char *from = switch_core_session_sprintf(session, "%s@%s", cbt->user, cbt->domain);
                    char *headers;
                    char *body;
                    int priority = 3;
                    
                    if (!strcasecmp(cbt->read_flags, URGENT_FLAG_STRING)) {
                        priority = 1;
                    }
                    headers = switch_core_session_sprintf(session, 
                                                          "From: FreeSWITCH mod_voicemail <%s@%s>\nSubject: Voicemail from %s %s\nX-Priority: %d", 
                                                          cbt->user, cbt->domain, cbt->cid_name, cbt->cid_number, priority);
                    
                    if (switch_event_create(&event, SWITCH_EVENT_MESSAGE) == SWITCH_STATUS_SUCCESS) {
                        switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Message-Type", "forwarded-voicemail");
                        switch_event_serialize(event, &body);
                        switch_event_fire(&event);
                    } else {
                        body = switch_mprintf("Voicemail from %s %s", 
                                              cbt->cid_name, cbt->cid_number);
                    }

                    //TBD add better formatting to the body
                    switch_simple_email(cbt->email, from, headers, body, cbt->file_path);
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Sending message to %s\n", cbt->email);
                    switch_safe_free(body);
                    TRY_CODE(switch_ivr_phrase_macro(session, VM_ACK_MACRO, "emailed", NULL, NULL));
                } else {
                    TRY_CODE(switch_ivr_phrase_macro(session, VM_ACK_MACRO, "deleted", NULL, NULL));
                }
            } else {
                char *sql = switch_mprintf("update voicemail_data set flags='save' where uuid='%s'", cbt->uuid);
                vm_execute_sql(profile, sql, profile->mutex);
                switch_safe_free(sql);
                TRY_CODE(switch_ivr_phrase_macro(session, VM_ACK_MACRO, "saved", NULL, NULL));
            }
        }
    }
    
 end:

    return status;
}

static void message_count(vm_profile_t *profile, char *myid, char *domain_name, char *myfolder, 
                          int *total_new_messages, int *total_saved_messages, int *total_new_urgent_messages, int *total_saved_urgent_messages)
{
    char msg_count[80] = "";
    callback_t cbt = { 0 };
    char sql[256];
    
    cbt.buf = msg_count;
    cbt.len = sizeof(msg_count);
                

    snprintf(sql, sizeof(sql), 
             "select count(*) from voicemail_data where user='%s' and domain='%s' and in_folder='%s' and read_epoch=0", 
             myid,
             domain_name,
             myfolder);
    vm_execute_sql_callback(profile, profile->mutex, sql, sql2str_callback, &cbt);
    *total_new_messages = atoi(msg_count);

    snprintf(sql, sizeof(sql), 
             "select count(*) from voicemail_data where user='%s' and domain='%s' and in_folder='%s' and read_epoch=0 and read_flags='%s'", 
             myid,
             domain_name,
             myfolder, 
             URGENT_FLAG_STRING);
    vm_execute_sql_callback(profile, profile->mutex, sql, sql2str_callback, &cbt);
    *total_new_urgent_messages = atoi(msg_count);

    snprintf(sql, sizeof(sql), 
             "select count(*) from voicemail_data where user='%s' and domain='%s' and in_folder='%s' and read_epoch!=0", 
             myid,
             domain_name,
             myfolder);
    vm_execute_sql_callback(profile, profile->mutex, sql, sql2str_callback, &cbt);
    *total_saved_messages = atoi(msg_count);


    snprintf(sql, sizeof(sql), 
             "select count(*) from voicemail_data where user='%s' and domain='%s' and in_folder='%s' and read_epoch!=0 and read_flags='%s'", 
             myid,
             domain_name,
             myfolder,
             URGENT_FLAG_STRING);
    vm_execute_sql_callback(profile, profile->mutex, sql, sql2str_callback, &cbt);
    *total_saved_urgent_messages = atoi(msg_count);

}


static void voicemail_check_main(switch_core_session_t *session, char *profile_name, char *domain_name, char *id, int auth)
{
    vm_check_state_t vm_check_state = VM_CHECK_START;
    switch_channel_t *channel;
    vm_profile_t *profile;
    switch_xml_t x_domain, x_domain_root, x_user, x_params, x_param;
    switch_status_t status;
    char pass_buf[80] = "", *mypass = NULL, id_buf[80] = "", *myid = id, *myfolder = NULL;
    const char *thepass = NULL;
    char term = 0;
    uint32_t timeout, attempts = 0;
    int failed = 0;
    msg_type_t play_msg_type = MSG_NONE;
    char *dir_path = NULL, *file_path = NULL;
    int total_new_messages = 0;
    int total_saved_messages = 0;
    int total_new_urgent_messages = 0;
    int total_saved_urgent_messages = 0;
    int heard_auto_saved = 0, heard_auto_new = 0;
    char *email_vm = NULL;
    channel = switch_core_session_get_channel(session);
    assert(channel != NULL);    

    
    if (!(profile = switch_core_hash_find(globals.profile_hash, profile_name))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error invalid profile %s", profile_name);
        return;
    }
    
    x_user = x_domain = x_domain_root = NULL;

    timeout = profile->digit_timeout;
    attempts = profile->max_login_attempts;
    
    status = switch_ivr_phrase_macro(session, VM_HELLO_MACRO, NULL, NULL, NULL);

    while(switch_channel_ready(channel)) {

        switch_ivr_sleep(session, 100);
        
        switch(vm_check_state) {
        case VM_CHECK_START:
            {
                total_new_messages = 0;
                total_saved_messages = 0;
                total_new_urgent_messages = 0;
                total_saved_urgent_messages = 0;
                heard_auto_saved = 0;
                heard_auto_new = 0;
                play_msg_type = MSG_NONE;
                attempts = profile->max_login_attempts;
                myid = id;
                mypass = NULL;
                myfolder = "inbox";
                vm_check_state = VM_CHECK_AUTH;
                if (x_domain_root) {
                    switch_xml_free(x_domain_root);
                }
                x_user = x_domain = x_domain_root = NULL;
            }
            break;
        case VM_CHECK_FOLDER_SUMMARY:
            {
                char msg_count[80] = "";
                switch_channel_set_variable(channel, "voicemail_current_folder", myfolder);
                message_count(profile, myid, domain_name, myfolder, &total_new_messages, &total_saved_messages, 
                              &total_new_urgent_messages, &total_saved_urgent_messages);

                if (total_new_urgent_messages > 0) {
                    snprintf(msg_count, sizeof(msg_count), "%d:urgent-new:%s", total_new_urgent_messages, total_new_urgent_messages == 1 ? "" : "s");
                    if ((status = switch_ivr_phrase_macro(session, VM_MESSAGE_COUNT_MACRO, msg_count, NULL, NULL)) != SWITCH_STATUS_SUCCESS) {
                        goto end;
                    }
                }
                if (total_new_messages > 0) {
                    snprintf(msg_count, sizeof(msg_count), "%d:new:%s", total_new_messages, total_new_messages == 1 ? "" : "s");
                    if ((status = switch_ivr_phrase_macro(session, VM_MESSAGE_COUNT_MACRO, msg_count, NULL, NULL)) != SWITCH_STATUS_SUCCESS) {
                        goto end;
                    }
                }
                
                if (!heard_auto_new && total_new_messages > 0) {
                    heard_auto_new = 1;
                    play_msg_type = MSG_NEW;
                    vm_check_state = VM_CHECK_PLAY_MESSAGES;
                    continue;
                }
                
                if (total_saved_urgent_messages > 0) {
                    snprintf(msg_count, sizeof(msg_count), "%d:urgent-saved:%s", total_saved_urgent_messages, total_saved_urgent_messages == 1 ? "" : "s");
                    if ((status = switch_ivr_phrase_macro(session, VM_MESSAGE_COUNT_MACRO, msg_count, NULL, NULL)) != SWITCH_STATUS_SUCCESS) {
                        goto end;
                    }
                }

                if (total_saved_messages > 0) {
                    snprintf(msg_count, sizeof(msg_count), "%d:saved:%s", total_saved_messages, total_saved_messages == 1 ? "" : "s");
                    if ((status = switch_ivr_phrase_macro(session, VM_MESSAGE_COUNT_MACRO, msg_count, NULL, NULL)) != SWITCH_STATUS_SUCCESS) {
                        goto end;
                    }
                }
                
                if (!heard_auto_saved && total_saved_messages > 0) {
                    heard_auto_saved = 1;
                    play_msg_type = MSG_SAVED;
                    vm_check_state = VM_CHECK_PLAY_MESSAGES;
                    continue;
                }
                
                vm_check_state = VM_CHECK_MENU;
            }
            break;
        case VM_CHECK_PLAY_MESSAGES:
            {
                listen_callback_t cbt;
                char sql[256];
                int cur_message, total_messages;
                message_count(profile, myid, domain_name, myfolder, &total_new_messages, &total_saved_messages, 
                              &total_new_urgent_messages, &total_saved_urgent_messages);
                memset(&cbt, 0, sizeof(cbt));
                cbt.email = email_vm;
                switch(play_msg_type) {
                case MSG_NEW:
                    {
                        snprintf(sql, sizeof(sql),
                                 "select * from voicemail_data where user='%s' and domain='%s' and read_epoch=0 order by read_flags", myid, domain_name);
                        total_messages = total_new_messages;
                        heard_auto_new = heard_auto_saved = 1;
                    }
                    break;
                case MSG_SAVED:
                default:
                    {
                        snprintf(sql, sizeof(sql),
                                 "select * from voicemail_data where user='%s' and domain='%s' and read_epoch !=0 order by read_flags", myid, domain_name);
                        total_messages = total_saved_messages;
                        heard_auto_new = heard_auto_saved = 1;                        
                    }
                    break;
                }
                for (cur_message = 0; cur_message < total_messages; cur_message++) {
                    cbt.index = 0;
                    cbt.want = cur_message;
                    cbt.type = play_msg_type;
                    vm_execute_sql_callback(profile, profile->mutex, sql, listen_callback, &cbt);
                    status = listen_file(session, profile, &cbt);
                    if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
                        break;
                    }
                }
                snprintf(sql, sizeof(sql), "update voicemail_data set read_epoch=%ld where user='%s' and domain='%s' and flags='save'", 
                         (long)time(NULL), myid, domain_name);
                vm_execute_sql(profile, sql, profile->mutex);
                snprintf(sql, sizeof(sql), "select file_path from voicemail_data where user='%s' and domain='%s' and flags='delete'", myid, domain_name);
                vm_execute_sql_callback(profile, profile->mutex, sql, unlink_callback, NULL);
                snprintf(sql, sizeof(sql), "delete from voicemail_data where user='%s' and domain='%s' and flags='delete'", myid, domain_name);
                vm_execute_sql(profile, sql, profile->mutex);
                vm_check_state = VM_CHECK_FOLDER_SUMMARY;
            }
            break;
        case VM_CHECK_CONFIG:
            {
                char *sql = NULL;
                char input[10] = "";
                char key_buf[80] = "";
                callback_t cbt = { 0 };
                char msg_count[80] = "";
                cc_t cc = { 0 };
                cbt.buf = msg_count;
                cbt.len = sizeof(msg_count);
                sql = switch_mprintf("select count(*) from voicemail_prefs where user='%q' and domain = '%q'", myid, domain_name);
                vm_execute_sql_callback(profile, profile->mutex, sql, sql2str_callback, &cbt);
                switch_safe_free(sql);
                if (switch_strlen_zero(msg_count) || !atoi(msg_count)) {
                    sql = switch_mprintf("insert into voicemail_prefs values('%q','%q','','')", myid, domain_name);
                    vm_execute_sql(profile, sql, profile->mutex);
                    switch_safe_free(sql);
                }

                snprintf(key_buf, sizeof(key_buf), "%s:%s:%s:%s", 
                         profile->record_greeting_key, 
                         profile->choose_greeting_key, 
                         profile->record_name_key,
                         profile->main_menu_key);
                

                TRY_CODE(vm_macro_get(session, VM_CONFIG_MENU_MACRO, key_buf, input, sizeof(input), 1, 
                                      profile->terminator_key, &term, timeout));
                if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
                    goto end;
                }
                
                if (!strcmp(input, profile->main_menu_key)) {
                    vm_check_state = VM_CHECK_MENU;
                } else if (!strcmp(input, profile->choose_greeting_key)) {
                    int num;
                    switch_input_args_t args = { 0 };
                    args.input_callback = cancel_on_dtmf;
                    
                    TRY_CODE(vm_macro_get(session, VM_CHOOSE_GREETING_MACRO, key_buf, input, sizeof(input), 1, 
                                          profile->terminator_key, &term, timeout));
                    
                    num = atoi(input);
                    file_path = switch_mprintf("%s%sgreeting_%d.%s", dir_path, SWITCH_PATH_SEPARATOR, num, profile->file_ext);
                    if (num < 1 || num > 3) {
                        status = SWITCH_STATUS_FALSE;
                    } else {
                        switch_file_handle_t fh = { 0 };
                        memset(&fh, 0, sizeof(fh));
                        args.input_callback = control_playback;
                        memset(&cc, 0, sizeof(cc));
                        cc.profile = profile;
                        cc.fh = &fh;
                        cc.noexit = 1;
                        args.buf = &cc;
                        status = switch_ivr_play_file(session, NULL, file_path, &args);
                    }
                    if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
                        TRY_CODE(switch_ivr_phrase_macro(session, VM_CHOOSE_GREETING_FAIL_MACRO, NULL, NULL, NULL));
                    } else {
                        TRY_CODE(switch_ivr_phrase_macro(session, VM_CHOOSE_GREETING_SELECTED_MACRO, input, NULL, NULL));
                        sql = switch_mprintf("update voicemail_prefs set greeting_path='%s' where user='%s' and domain='%s'", file_path, myid, domain_name);
                        vm_execute_sql(profile, sql, profile->mutex);
                        switch_safe_free(sql);
                    }
                    switch_safe_free(file_path);
                } else if (!strcmp(input, profile->record_greeting_key)) {
                    int num;
                    TRY_CODE(vm_macro_get(session, VM_CHOOSE_GREETING_MACRO, key_buf, input, sizeof(input), 1, 
                                          profile->terminator_key, &term, timeout));
                    
                    num = atoi(input);
                    if (num < 1 || num > 3) {
                        TRY_CODE(switch_ivr_phrase_macro(session, VM_CHOOSE_GREETING_FAIL_MACRO, NULL, NULL, NULL));
                    } else {
                        file_path = switch_mprintf("%s%sgreeting_%d.%s", dir_path, SWITCH_PATH_SEPARATOR, num, profile->file_ext);
                        TRY_CODE(create_file(session, profile, VM_RECORD_GREETING_MACRO, file_path));
                        sql = switch_mprintf("update voicemail_prefs set greeting_path='%s' where user='%s' and domain='%s'", file_path, myid, domain_name);
                        vm_execute_sql(profile, sql, profile->mutex);
                        switch_safe_free(sql);
                        switch_safe_free(file_path);
                    }

                } else if (!strcmp(input, profile->record_name_key)) {
                    file_path = switch_mprintf("%s%srecorded_name.%s", dir_path, SWITCH_PATH_SEPARATOR, profile->file_ext);
                    TRY_CODE(create_file(session, profile, VM_RECORD_NAME_MACRO, file_path));
                    sql = switch_mprintf("update voicemail_prefs set name_path='%s' where user='%s' and domain='%s'", file_path, myid, domain_name);
                    vm_execute_sql(profile, sql, profile->mutex);
                    switch_safe_free(file_path);
                    switch_safe_free(sql);
                }

                continue;
            }
            break;
        case VM_CHECK_MENU:
            {
                char input[10] = "";
                char key_buf[80] = "";
                play_msg_type = MSG_NONE;
                
                snprintf(key_buf, sizeof(key_buf), "%s:%s:%s:%s", 
                         profile->play_new_messages_key, 
                         profile->play_saved_messages_key,
                         profile->config_menu_key,
                         profile->terminator_key);
                
                status = vm_macro_get(session, VM_MENU_MACRO, key_buf, input, sizeof(input), 1, 
                                      profile->terminator_key, &term, timeout);
                if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
                    goto end;
                }
                
                if (!strcmp(input, profile->play_new_messages_key)) {
                    play_msg_type = MSG_NEW;
                } else if (!strcmp(input, profile->play_saved_messages_key)) {
                    play_msg_type = MSG_SAVED;
                } else if (!strcmp(input, profile->terminator_key)) {
                    goto end;
                } else if (!strcmp(input, profile->config_menu_key)) {
                    vm_check_state = VM_CHECK_CONFIG;
                }
                
                if (play_msg_type) {
                    vm_check_state = VM_CHECK_PLAY_MESSAGES;
                }


                continue;
            }
            break;
        case VM_CHECK_AUTH:
            {
                if (!attempts) {
                    failed = 1;
                    goto end;
                }

                attempts--;

                if (!myid) {
                    status = vm_macro_get(session, VM_ENTER_ID_MACRO, profile->terminator_key, id_buf, sizeof(id_buf), 0, 
                                          profile->terminator_key, &term, timeout);
                    if (status != SWITCH_STATUS_SUCCESS) {
                        goto end;
                    }

                    if (switch_strlen_zero(id_buf)) {
                        continue;
                    } else {
                        myid = id_buf;
                    } 
                }

                if (!x_user) {
                    int ok = 1;
                    char *xtra = switch_mprintf("mailbox=%s", myid);
                    
                    assert(xtra);

                    if (switch_xml_locate_user(myid, domain_name, switch_channel_get_variable(channel, "network_addr"), 
                                               &x_domain_root, &x_domain, &x_user, xtra) != SWITCH_STATUS_SUCCESS) {
                        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "can't find user [%s@%s]\n", myid, domain_name);
                        ok = 0;
                    }

                    switch_safe_free(xtra);

                    if (!ok) {
                        goto failed;
                    }
                }
                
                if (!mypass) {
                    if (auth) {
                        mypass = "OK";
                    } else {
                        status = vm_macro_get(session, VM_ENTER_PASS_MACRO, profile->terminator_key, 
                                              pass_buf, sizeof(pass_buf), 0, profile->terminator_key, &term, timeout);
                        if (status != SWITCH_STATUS_SUCCESS) {
                            goto end;
                        }
                        if (switch_strlen_zero(pass_buf)) {
                            continue;
                        } else {
                            mypass = pass_buf;
                        }
                    }
                }
                
                if (!(x_params = switch_xml_child(x_user, "params"))) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "can't find params for user [%s@%s]\n", myid, domain_name);
                    goto failed;
                }
                
                thepass = NULL;
                for (x_param = switch_xml_child(x_params, "param"); x_param; x_param = x_param->next) {
                    const char *var = switch_xml_attr_soft(x_param, "name");
                    const char *val = switch_xml_attr_soft(x_param, "value");
                    
                    if (!strcasecmp(var, "password")) {
                        thepass = val;
                    } else if (!strcasecmp(var, "vm-password")) {
                        thepass = val;
                    } else if (!strcasecmp(var, "vm-mailto")) {
                        email_vm = switch_core_session_strdup(session, val);
                    }
                }
                switch_xml_free(x_domain_root);
                x_domain_root = NULL;

                if (auth || !thepass || (thepass && mypass && !strcmp(thepass, mypass))) {
                    if (!dir_path) {
                        dir_path = switch_core_session_sprintf(session, "%s%svoicemail%s%s%s%s%s%s", SWITCH_GLOBAL_dirs.storage_dir, 
                                                               SWITCH_PATH_SEPARATOR,
                                                               SWITCH_PATH_SEPARATOR,
                                                               profile->name,
                                                               SWITCH_PATH_SEPARATOR,
                                                               domain_name,
                                                               SWITCH_PATH_SEPARATOR,
                                                               myid);
                        
                        
                        if (switch_dir_make_recursive(dir_path, DEFAULT_DIR_PERMS, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
                            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating %s", dir_path);
                            return;
                        }
                    }


                    vm_check_state = VM_CHECK_FOLDER_SUMMARY;
                    //vm_check_state = VM_CHECK_MENU;
                } else {
                    goto failed;
                }
                
                continue;
                
            failed:
                status = switch_ivr_phrase_macro(session, VM_FAIL_AUTH_MACRO, NULL, NULL, NULL);
                myid = id;
                mypass = NULL;
                continue;
            }
            break;
        default:
            break;
        }
    }
    
 end:

    if (switch_channel_ready(channel)) {
        if (failed) {
            status = switch_ivr_phrase_macro(session, VM_ABORT_MACRO, NULL, NULL, NULL);
        }
        status = switch_ivr_phrase_macro(session, VM_GOODBYE_MACRO, NULL, NULL, NULL);
    }

    if (x_domain_root) {
        switch_xml_free(x_domain_root);
    }

}
    

static switch_status_t voicemail_leave_main(switch_core_session_t *session, char *profile_name, char *domain_name, char *id)
{
    switch_channel_t *channel;
    char *myfolder = "inbox";
    char sql[256];
    prefs_callback_t cbt;
    vm_profile_t *profile;
    char *uuid = switch_core_session_get_uuid(session);
    char *file_path = NULL;
    char *dir_path = NULL;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    switch_caller_profile_t *caller_profile;
    switch_file_handle_t fh = { 0 };
    switch_input_args_t args = { 0 };
    char *email_vm = NULL;
    int send_mail = 0;
    cc_t cc = { 0 };
    char *read_flags = NORMAL_FLAG_STRING;
    int priority = 3;

    memset(&cbt, 0, sizeof(cbt));
    if (!(profile = switch_core_hash_find(globals.profile_hash, profile_name))) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error invalid profile %s", profile_name);
        return SWITCH_STATUS_FALSE;
    }

    channel = switch_core_session_get_channel(session);
    assert(channel != NULL);
    
    caller_profile = switch_channel_get_caller_profile(channel);
    dir_path = switch_core_session_sprintf(session, "%s%svoicemail%s%s%s%s%s%s", SWITCH_GLOBAL_dirs.storage_dir, 
                                           SWITCH_PATH_SEPARATOR,
                                           SWITCH_PATH_SEPARATOR,
                                           profile->name,
                                           SWITCH_PATH_SEPARATOR,
                                           domain_name,
                                           SWITCH_PATH_SEPARATOR,
                                           id);
    if (switch_dir_make_recursive(dir_path, DEFAULT_DIR_PERMS, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating %s", dir_path);
        goto end;
    }


    if (id) {
        int ok = 1;
        char *xtra = switch_mprintf("mailbox=%s", id);
        switch_xml_t x_domain, x_domain_root, x_user, x_params, x_param;

        assert(xtra);
        x_user = x_domain = x_domain_root = NULL;
        if (switch_xml_locate_user(id, domain_name, switch_channel_get_variable(channel, "network_addr"), 
                                   &x_domain_root, &x_domain, &x_user, xtra) == SWITCH_STATUS_SUCCESS) {
            if ((x_params = switch_xml_child(x_user, "params"))) {
                for (x_param = switch_xml_child(x_params, "param"); x_param; x_param = x_param->next) {
                    const char *var = switch_xml_attr_soft(x_param, "name");
                    const char *val = switch_xml_attr_soft(x_param, "value");
                    
                    if (!strcasecmp(var, "vm-mailto")) {
                        email_vm = switch_core_session_strdup(session, val);
                    } else if (!strcasecmp(var, "vm-email-all-messages")) {
                        send_mail = switch_true(var);
                    }
                }
            }

        } else {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "can't find user [%s@%s]\n", id, domain_name);
            ok = 0;
        }
        

        switch_safe_free(xtra);
        switch_xml_free(x_domain_root);
        if (!ok) {
            goto end;
        }
    }

    snprintf(sql, sizeof(sql), 
             "select * from voicemail_prefs where user='%s' and domain='%s'", 
             id,
             domain_name);
    vm_execute_sql_callback(profile, profile->mutex, sql, prefs_callback, &cbt);
    
    file_path = switch_mprintf("%s%smsg_%s.%s", dir_path, SWITCH_PATH_SEPARATOR, uuid, profile->file_ext);

    memset(&fh, 0, sizeof(fh));
    args.input_callback = control_playback;
    memset(&cc, 0, sizeof(cc));
    cc.profile = profile;
    cc.fh = &fh;
    cc.noexit = 1;
    args.buf = &cc;

    if (!switch_strlen_zero(cbt.greeting_path)) {
        TRY_CODE(switch_ivr_play_file(session, NULL, cbt.greeting_path, NULL));
    } else {
        if (!switch_strlen_zero(cbt.name_path)) {
            TRY_CODE(switch_ivr_play_file(session, NULL, cbt.name_path, NULL));
        }
        TRY_CODE(switch_ivr_phrase_macro(session, VM_PLAY_GREETING_MACRO, id, NULL, NULL));
    }

        status = create_file(session, profile, VM_RECORD_MESSAGE_MACRO, file_path);

        if ((status == SWITCH_STATUS_SUCCESS || status == SWITCH_STATUS_BREAK) && switch_channel_ready(channel)) {
            char input[10] = "", key_buf[80] = "", term = 0;

            snprintf(key_buf, sizeof(key_buf), "%s:%s", 
                     profile->urgent_key,
                     profile->terminator_key);

            vm_macro_get(session, VM_RECORD_URGENT_CHECK_MACRO, key_buf, input, sizeof(input), 1, profile->terminator_key, &term, profile->digit_timeout);
            if (*profile->urgent_key == *input) {
                read_flags = URGENT_FLAG_STRING;
                priority = 1;
                TRY_CODE(switch_ivr_phrase_macro(session, VM_ACK_MACRO, "marked-urgent", NULL, NULL));
            } else {
                TRY_CODE(switch_ivr_phrase_macro(session, VM_ACK_MACRO, "saved", NULL, NULL));
            }
        }

        if(!send_mail) {
            char *usql;
            switch_event_t *event;
            char *mwi_id = NULL;
            int total_new_messages = 0;
            int total_saved_messages = 0;
            int total_new_urgent_messages = 0;
            int total_saved_urgent_messages = 0;
        
            usql = switch_mprintf("insert into voicemail_data values(%ld,0,'%q','%q','%q','%q','%q','%q','%q','','%q')", (long)time(NULL),
                                  id, domain_name, uuid, caller_profile->caller_id_name, caller_profile->caller_id_number, myfolder, file_path, read_flags);
            vm_execute_sql(profile, usql, profile->mutex);
            switch_safe_free(usql);

            message_count(profile, id, domain_name, myfolder, &total_new_messages, &total_saved_messages,
                          &total_new_urgent_messages, &total_saved_urgent_messages);

            if (switch_event_create(&event, SWITCH_EVENT_MESSAGE_WAITING) == SWITCH_STATUS_SUCCESS) {
                mwi_id = switch_mprintf("%s@%s", id, domain_name);
                switch_event_add_header(event, SWITCH_STACK_BOTTOM, "MWI-Messages-Waiting", "yes");
                switch_event_add_header(event, SWITCH_STACK_BOTTOM, "MWI-Message-Account", mwi_id);
                switch_event_add_header(event, SWITCH_STACK_BOTTOM, "MWI-Voice-Message", "%d/%d (%d/%d)", 
                                        total_new_messages, total_saved_messages, total_new_urgent_messages, total_saved_urgent_messages);
                switch_event_fire(&event);
                switch_safe_free(mwi_id);
            }    
        }
        
 end:
    
    if (send_mail && !switch_strlen_zero(email_vm)) {
        switch_event_t *event;
        char *from = switch_core_session_sprintf(session, "%s@%s", id, domain_name);
        char *body;
        char *headers;
        
        headers = switch_core_session_sprintf(session, "From: FreeSWITCH mod_voicemail <%s@%s>\nSubject: Voicemail from %s %s\nX-Priority: %d", 
                                              id, domain_name,
                                              caller_profile->caller_id_name, 
                                              caller_profile->caller_id_number,
                                              priority);


        
        if (switch_event_create(&event, SWITCH_EVENT_MESSAGE) == SWITCH_STATUS_SUCCESS) {
            switch_channel_event_set_data(channel, event);
            switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Message-Type", "voicemail");
            switch_event_serialize(event, &body);
            switch_event_fire(&event);
        } else {
            body = switch_mprintf("Voicemail from %s %s", 
                                  caller_profile->caller_id_name, caller_profile->caller_id_number);
        }

        //TBD add better formatting to the body
        switch_simple_email(email_vm, from, headers, body, file_path);
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Sending message to %s\n", email_vm);
        switch_safe_free(body);
        unlink(file_path);
    }

    switch_safe_free(file_path);
    
    if (switch_channel_ready(channel)) {
        status = switch_ivr_phrase_macro(session, VM_GOODBYE_MACRO, NULL, NULL, NULL);
    }

    return status;


}


#define VM_DESC "voicemail"
#define VM_USAGE "[check|auth] <profile_name> <domain_name> [<id>]"

SWITCH_STANDARD_APP(voicemail_function)
{
	int argc = 0;
	char *argv[4] = { 0 };
	char *mydata = NULL;
    char *profile_name = NULL;
    char *domain_name = NULL;
    char *id = NULL;
    char *auth_var = NULL;
    int x = 0, check = 0, auth = 0;
    switch_channel_t *channel;

    channel = switch_core_session_get_channel(session);
    assert(channel != NULL);
    
    if (switch_dir_make_recursive(SWITCH_GLOBAL_dirs.storage_dir, DEFAULT_DIR_PERMS, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating %s", SWITCH_GLOBAL_dirs.storage_dir);
        return;
    }
    
    if (!switch_strlen_zero(data)) {
        mydata = switch_core_session_strdup(session, data);
        argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
    }

    if (argv[x] && !strcasecmp(argv[x], "check")) {
        check++;
        x++;
    }

    if (argv[x] && !strcasecmp(argv[x], "auth")) {
        auth++;
        x++;
    }

    if (argv[x]) {
        profile_name = argv[x++];
    }

    if (argv[x]) {
        domain_name = argv[x++];
    }

    if (argv[x]) {
        id = argv[x++];
    }


    if ((auth_var = switch_channel_get_variable(channel, "voicemail_authorized")) && switch_true(auth_var)) {
        auth = 1;
    }

    if (switch_strlen_zero(profile_name)) {
        profile_name = switch_channel_get_variable(channel, "voicemail_profile_name");
    }

    if (switch_strlen_zero(domain_name)) {
        domain_name = switch_channel_get_variable(channel, "voicemail_domain_name");
    }

    if (switch_strlen_zero(id)) {
        id = switch_channel_get_variable(channel, "voicemail_id");
    }

    if (switch_strlen_zero(profile_name) || switch_strlen_zero(domain_name)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Usage: %s\n", VM_USAGE);
        return;
    }
    
    if (check) {
        voicemail_check_main(session, profile_name, domain_name, id, auth);
    } else {
        voicemail_leave_main(session, profile_name, domain_name, id);
    }
}

static void  message_query_handler(switch_event_t *event)
{
	char *account = switch_event_get_header(event, "message-account");
    int sent = 0;
    switch_event_t *new_event = NULL;

	if (account) {
        switch_hash_index_t *hi;
        int total_new_messages = 0;
        int total_saved_messages = 0;
        int total_new_urgent_messages = 0;
        int total_saved_urgent_messages = 0;
        void *val;
        vm_profile_t *profile;
        char *id, *domain;

        if (!strncasecmp(account, "sip:", 4)) {
            id = strdup(account + 4);
        } else {
            id = strdup(account);
        }
        assert(id);

        if ((domain = strchr(id, '@'))) {
            *domain++ = '\0';
            for (hi = switch_hash_first(NULL, globals.profile_hash); hi; hi = switch_hash_next(hi)) {
                switch_hash_this(hi, NULL, NULL, &val);
                profile = (vm_profile_t *) val;
                total_new_messages =  total_saved_messages = 0;
                message_count(profile, id, domain, "inbox", &total_new_messages, &total_saved_messages,
                              &total_new_urgent_messages, &total_saved_urgent_messages);
                if (total_new_messages || total_saved_messages) {
                    if (switch_event_create(&new_event, SWITCH_EVENT_MESSAGE_WAITING) == SWITCH_STATUS_SUCCESS) {
                        switch_event_add_header(new_event, SWITCH_STACK_BOTTOM, "MWI-Messages-Waiting", "yes");
                        switch_event_add_header(new_event, SWITCH_STACK_BOTTOM, "MWI-Message-Account", account);
                        switch_event_add_header(new_event, SWITCH_STACK_BOTTOM, "MWI-Voice-Message", "%d/%d (%d/%d)", 
                                                total_new_messages, total_saved_messages, total_new_urgent_messages, total_saved_urgent_messages);
                        switch_event_fire(&new_event);
                        sent++;
                    }
                }
            }
        }
        
        switch_safe_free(id);

	}

    if (!sent) {
        if (switch_event_create(&new_event, SWITCH_EVENT_MESSAGE_WAITING) == SWITCH_STATUS_SUCCESS) {
            switch_event_add_header(new_event, SWITCH_STACK_BOTTOM, "MWI-Messages-Waiting", "no");
            switch_event_add_header(new_event, SWITCH_STACK_BOTTOM, "MWI-Message-Account", account);
            switch_event_fire(&new_event);
        }
    }


}

SWITCH_MODULE_LOAD_FUNCTION(mod_voicemail_load)
{
	switch_application_interface_t *app_interface;
    switch_status_t status;

    if ((status = load_config()) != SWITCH_STATUS_SUCCESS) {
        return status;
    }
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	SWITCH_ADD_APP(app_interface, "voicemail", "Voicemail", VM_DESC, voicemail_function, VM_USAGE, SAF_NONE);


	if (switch_event_bind((char *) modname, SWITCH_EVENT_MESSAGE_QUERY, SWITCH_EVENT_SUBCLASS_ANY, message_query_handler, NULL)
		!= SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
    
	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_NOUNLOAD;
}

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
