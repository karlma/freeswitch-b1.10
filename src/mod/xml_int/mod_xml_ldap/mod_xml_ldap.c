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
 * Bret McDanel <trixter AT 0xdecafbad.com>
 * Justin Cassidy <xachenant@hotmail.com>
 * John Skopis <john+fs@skopis.com>
 *
 * mod_xml_ldap.c -- LDAP XML Gateway
 *
 */
#include <switch.h>
#include <stdlib.h>
#include <string.h>
#ifdef MSLDAP
#include <windows.h>
#include <winldap.h>
#include <winber.h>
#define LDAP_OPT_SUCCESS LDAP_SUCCESS
#else
#include <lber.h>
#include <ldap.h>
#endif

#define XML_LDAP_CONFIG 0
#define XML_LDAP_DIRECTORY 1
#define XML_LDAP_DIALPLAN 2
#define XML_LDAP_PHRASE 3

SWITCH_MODULE_LOAD_FUNCTION(mod_xml_ldap_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_xml_ldap_shutdown);
SWITCH_MODULE_DEFINITION(mod_xml_ldap, mod_xml_ldap_load, mod_xml_ldap_shutdown, NULL);


struct xml_binding {
  char *bindings;
  char *host;
	char *ldap_base;
  char *binddn;
  char *bindpass;
  char *queryfmt;
};

typedef struct xml_binding xml_binding_t;

struct ldap_c {
  LDAP *ld;
  LDAPMessage *msg;
  LDAPMessage *entry;
  BerElement *berkey;
  BerElement *berval;
  char *key;
  char *val;
  char **keyvals;
  char **valvals;

};

static switch_status_t xml_ldap_directory_result( void *ldap_connection, xml_binding_t *binding, switch_xml_t *xml, int *off);
static switch_status_t xml_ldap_dialplan_result( void *ldap_connection, xml_binding_t *binding, switch_xml_t *xml, int *off);


#define XML_LDAP_SYNTAX "[debug_on|debug_off]"
SWITCH_STANDARD_API(xml_ldap_function)
{
	if (session) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_strlen_zero(cmd)) {
		goto usage;
	}

	if (!strcasecmp(cmd, "debug_on")) {
	} else if (!strcasecmp(cmd, "debug_off")) {
	} else {
		goto usage;
	}

	stream->write_function(stream, "OK\n");
	return SWITCH_STATUS_SUCCESS;

usage:
	stream->write_function(stream, "USAGE: %s\n", XML_LDAP_SYNTAX);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t xml_ldap_result( void *ldap_connection, xml_binding_t *binding, switch_xml_t *xml, int *off, int qt)
{
  switch_status_t ret;
  if (qt == 1 ) ret = xml_ldap_directory_result(ldap_connection, binding, xml , off);
  else if (qt == 2 ) ret = xml_ldap_dialplan_result(ldap_connection, binding, xml , off);
  return ret;
}

static switch_status_t xml_ldap_dialplan_result( void *ldap_connection, xml_binding_t *binding, switch_xml_t *xml, int *off)
{
  return SWITCH_STATUS_FALSE;
}

static switch_status_t xml_ldap_directory_result( void *ldap_connection, xml_binding_t *binding, switch_xml_t *xml, int *off)
{
	struct ldap_c *ldap = ldap_connection;
  switch_xml_t asdf = *xml;
  switch_xml_t param, variable, params, variables;
  int i =0;
  int loff = *off;
  
  for (ldap->entry = ldap_first_entry(ldap->ld,ldap->msg); ldap->entry != NULL; ldap->entry = ldap_next_entry(ldap->ld,ldap->entry)) {
    ldap->key = ldap_first_attribute( ldap->ld, ldap->entry, &ldap->berkey );
    do {
      ldap->val = ldap_first_attribute( ldap->ld, ldap->entry, &ldap->berval );
      do {
        if(strstr(ldap->val,"value") ) {
          if(strstr(ldap->val,ldap->key) && strcmp(ldap->val,ldap->key) ) {
            if(!strcmp(ldap->key,"param")) {
              params = switch_xml_add_child_d(asdf,"params",loff++);
            }else if (!strcmp(ldap->key,"variable")) {
              variables = switch_xml_add_child_d(asdf,"variables",loff++);
            }

            if( (ldap->keyvals = ldap_get_values( ldap->ld, ldap->entry, ldap->key )) && (ldap->valvals = ldap_get_values( ldap->ld, ldap->entry, ldap->val )) ) {
              if ( ldap_count_values(ldap->valvals) == ldap_count_values(ldap->keyvals) ) {
                for ( i = 0 ; ldap->keyvals[i] != NULL && ldap->valvals[i] != NULL ; i++ ) {
                  if(!strcmp(ldap->key,"param")) {
                    param = switch_xml_add_child_d(params,"param",loff++);
                    switch_xml_set_attr_d(param,"name",ldap->keyvals[i]);
                    switch_xml_set_attr_d(param,"value",ldap->valvals[i]);
                  }
                  else if (!strcmp(ldap->key,"variable")) {
                    variable = switch_xml_add_child_d(variables,"variable",loff++);
                    switch_xml_set_attr_d(variable,"name",ldap->keyvals[i]);
                    switch_xml_set_attr_d(variable,"value",ldap->valvals[i]);
                  } 
                } 
                if ( ldap->keyvals ) ldap_value_free(ldap->keyvals);
                if ( ldap->valvals ) ldap_value_free(ldap->valvals);
              }
              else {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "seems the values %d and %d are not the same??\n",ldap_count_values ( ldap->valvals ), ldap_count_values ( ldap->keyvals )); 
              }
            }
          }
        }
        if (ldap->val) ldap_memfree(ldap->val);
        ldap->val = ldap_next_attribute(ldap->ld, ldap->entry,ldap->berval);
      } while (ldap->val != NULL);
      if (ldap->key) ldap_memfree(ldap->key);
      if (ldap->berval) ber_free(ldap->berval,0);
      ldap->key = ldap_next_attribute(ldap->ld, ldap->entry, ldap->berkey );
    } while ( ldap->key != NULL  );
    if (ldap->berkey) ber_free(ldap->berkey,0);
  }
  return SWITCH_STATUS_SUCCESS;
}


static switch_xml_t xml_ldap_search(const char *section, const char *tag_name, const char *key_name, const char *key_value, switch_event_t  *params, void *user_data)
{
  xml_binding_t *binding = (xml_binding_t *) user_data;
  switch_event_header_t *hi;
  
  switch_xml_t xml, sub; 
 
  struct ldap_c ldap_connection;
  struct ldap_c *ldap = &ldap_connection;
  
  int auth_method = LDAP_AUTH_SIMPLE;
  int desired_version = LDAP_VERSION3;
  int query_type; // see defines on top XML_LDAP_foo
  char *dir_exten, *dir_domain;
  char *filter = "(objectClass=*)";

  char *search_base = NULL;
  int off = 0;


	if (!binding) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "no bindings...sorry bud returning now\n");
		return NULL;
	}

  if(!strcmp(section,"configuration")) {
    query_type = 0;
  } 
  else if (!strcmp(section,"directory")) {
    query_type = 1;
  } 
  else if (!strcmp(section,"dialplan")) {
    query_type = 2;
  } 
  else if (!strcmp(section,"phrases")) {
    query_type = 3;
  } 


  if (params) {
    if ((hi = params->headers)) {
      for (; hi; hi = hi->next) {
        switch(query_type) {
          case XML_LDAP_CONFIG:
            break;
          case XML_LDAP_DIRECTORY:
            if (!strcmp(hi->name,"user")) dir_exten = strdup(hi->value);
            else if (!strcmp(hi->name,"domain")) dir_domain = strdup(hi->value);
            break;
          case XML_LDAP_DIALPLAN:
            break;
          case XML_LDAP_PHRASE:
            break;
        }
      }
      switch(query_type) {
        case XML_LDAP_CONFIG:
          break;
        case XML_LDAP_DIRECTORY:
          if ( dir_exten && dir_domain ) {
            xml = switch_xml_new("directory");
            switch_xml_set_attr_d(xml,"type","freeswitch/xml");
            sub = switch_xml_add_child_d(xml,"section",off++);
            switch_xml_set_attr_d(sub,"name","directory");
            sub =  switch_xml_add_child_d(sub,"domain",off++);
            switch_xml_set_attr_d(sub,"name",dir_domain);
            sub = switch_xml_add_child_d(sub,"user",off++);
            switch_xml_set_attr_d(sub,"id",dir_exten);

            search_base = switch_mprintf(binding->queryfmt,dir_exten,dir_domain,binding->ldap_base);
            
            free(dir_exten);
            dir_exten = NULL;
            free(dir_domain);
            dir_domain = NULL;
          } else { 
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "something bad happened during the query construction phase likely  exten(%s) or domain(%s) is null\n",dir_exten,dir_domain);
            goto cleanup;
          }
          break;
        case XML_LDAP_DIALPLAN:
          xml = switch_xml_new("document");
          switch_xml_set_attr_d(xml,"type","freeswitch/xml");
          sub = switch_xml_add_child_d(xml,"section",off++);
          switch_xml_set_attr_d(sub,"name","dialplan");
          sub = switch_xml_add_child_d(xml,"context",off++);
          search_base = switch_mprintf(binding->queryfmt,dir_exten,dir_domain,binding->ldap_base);
          break;
        case XML_LDAP_PHRASE:
          break;
      }
    }
    else {
      goto cleanup;
    }
  }


  if ((ldap->ld = ldap_init(binding->host, LDAP_PORT)) == NULL) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unable to connect to ldap server.%s\n", binding->host);
    goto cleanup;
  }

  if (ldap_set_option(ldap->ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version) != LDAP_OPT_SUCCESS) {
    goto cleanup;
  }

  if (ldap_bind_s(ldap->ld, binding->binddn, binding->bindpass, auth_method) != LDAP_SUCCESS) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unable to bind to ldap server %s as %s\n",binding->host, binding->binddn);
    goto cleanup;
  }

  if (ldap_search_s(ldap->ld, search_base, LDAP_SCOPE_SUBTREE, filter, NULL, 0, &ldap->msg) != LDAP_SUCCESS) {
    goto cleanup;
  }

  if (ldap_count_entries(ldap->ld, ldap->msg) <= 0) {
    goto cleanup;
  }

  if ( xml_ldap_result( &ldap_connection, binding, &sub, &off, query_type) != SWITCH_STATUS_SUCCESS ){
    goto cleanup;
  }

  cleanup:
  if (ldap->msg) ldap_msgfree(ldap->msg);
  if (ldap->ld) ldap_unbind_s(ldap->ld);
  
  switch_safe_free(search_base);

  return xml;
}


static switch_status_t do_config(void)
{
	char *cf = "xml_ldap.conf";
	switch_xml_t cfg, xml, bindings_tag, binding_tag, param;
	xml_binding_t *binding = NULL;
	int x = 0;

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}

	if (!(bindings_tag = switch_xml_child(cfg, "bindings"))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing <bindings> tag!\n");
		goto done;
	}

	for (binding_tag = switch_xml_child(bindings_tag, "binding"); binding_tag; binding_tag = binding_tag->next) {
		char *bname = (char *) switch_xml_attr_soft(binding_tag, "name");

    if (!(binding = malloc(sizeof(*binding)))) {
      goto done;
    }
    memset(binding, 0, sizeof(*binding));

    


		for (param = switch_xml_child(binding_tag, "param"); param; param = param->next) {


			char *var = (char *) switch_xml_attr_soft(param, "name");
			char *val = (char *) switch_xml_attr_soft(param, "value");
      if (!strcasecmp(var, "queryfmt")) {
         binding->bindings = (char *) switch_xml_attr_soft(param, "bindings");
         if(val) binding->queryfmt = strdup(val);
      } else if (!strcasecmp(var, "base")) {
        binding->ldap_base = strdup(val);
			} else if (!strcasecmp(var, "binddn")) {
				binding->binddn = strdup(val);
			} else if (!strcasecmp(var, "bindpass")) {
        binding->bindpass = strdup(val);
			} else if (!strcasecmp(var, "host")) {
        binding->host = strdup(val);
      }
		}

		if (!binding->ldap_base || !binding->binddn || !binding->bindpass ) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "You must define base binddn bindpass\n");
			continue;
		}


		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Binding [%s] XML Fetch Function [%s] [%s]\n",
        switch_strlen_zero(bname) ? "N/A" : bname, binding->ldap_base, binding->bindings ? binding->bindings : "all");
		switch_xml_bind_search_function(xml_ldap_search, switch_xml_parse_section_string(bname), binding);
		x++;
		binding = NULL;
	}

 done:
	switch_xml_free(xml);

	return SWITCH_STATUS_SUCCESS;
}


SWITCH_MODULE_LOAD_FUNCTION(mod_xml_ldap_load)
{
	switch_api_interface_t *xml_ldap_api_interface;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	SWITCH_ADD_API(xml_ldap_api_interface, "xml_ldap", "XML LDAP", xml_ldap_function, XML_LDAP_SYNTAX);
  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "XML LDAP module loading...\n");

	if (! (do_config() == SWITCH_STATUS_SUCCESS) ) return SWITCH_STATUS_FALSE;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_xml_ldap_shutdown)
{
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
