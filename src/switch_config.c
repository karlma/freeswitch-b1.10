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
 * switch_config.c -- Configuration File Parser
 *
 */
#include <switch_config.h>

#ifndef SWITCH_CONF_DIR
#ifdef WIN32
#define SWITCH_CONF_DIR ".\\conf"
#else
#define SWITCH_CONF_DIR "/usr/local/freeswitch/conf"
#endif
#endif

SWITCH_DECLARE(int) switch_config_open_file(switch_config *cfg, char *file_path)
{
	FILE *f;
	char *path = NULL;
	char path_buf[1024];

	if (file_path[0] == '/') {
		path = file_path;
	} else {
		snprintf(path_buf, sizeof(path_buf), "%s/%s", SWITCH_CONF_DIR, file_path);
		path = path_buf;
	}

	if (!path || !(f = fopen(path, "r"))) {
		return 0;
	}

	memset(cfg, 0, sizeof(*cfg));
	cfg->file = f;
	cfg->path = path;

	return 1;
}


SWITCH_DECLARE(void) switch_config_close_file(switch_config *cfg)
{

	if (cfg->file) {
		fclose(cfg->file);
	}

	memset(cfg, 0, sizeof(*cfg));
}



SWITCH_DECLARE(int) switch_config_next_pair(switch_config *cfg, char **var, char **val)
{
	int ret = 0;
	char *p, *end;

	*var = *val = NULL;

	for (;;) {
		cfg->lineno++;

		if (!fgets(cfg->buf, sizeof(cfg->buf), cfg->file)) {
			ret = 0;
			break;
		}

		*var = cfg->buf;

		if (**var == '[' && (end = strchr(*var, ']'))) {
			*end = '\0';
			(*var)++;
			switch_copy_string(cfg->category, *var, sizeof(cfg->category));
			cfg->catno++;
			continue;
		}

		if (**var == '#' || **var == '\n' || **var == '\r') {
			continue;
		}

		if ((end = strchr(*var, '#'))) {
			*end = '\0';
			end--;
		} else if ((end = strchr(*var, '\n'))) {
			if (*(end - 1) == '\r') {
				end--;
			}
			*end = '\0';
		}

		p = *var;
		while ((*p == ' ' || *p == '\t') && p != end) {
			*p = '\0';
			p++;
		}
		*var = p;

		if (!(*val = strchr(*var, '='))) {
			ret = -1;
			//log_printf(0, server.log, "Invalid syntax on %s: line %d\n", cfg->path, cfg->lineno);
			continue;
		} else {
			p = *val - 1;
			*(*val) = '\0';
			(*val)++;
			if (*(*val) == '>') {
				*(*val) = '\0';
				(*val)++;
			}

			while ((*p == ' ' || *p == '\t') && p != *var) {
				*p = '\0';
				p--;
			}

			p = *val;
			while ((*p == ' ' || *p == '\t') && p != end) {
				*p = '\0';
				p++;
			}
			*val = p;
			ret = 1;
			break;
		}
	}

	return ret;

}
