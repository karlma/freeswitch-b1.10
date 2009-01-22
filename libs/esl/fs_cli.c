#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <stdio.h>
#include <stdlib.h>
#include <esl.h>
#include <signal.h>

#ifdef WIN32
#define strdup(src) _strdup(src)
#define usleep(time) Sleep(time/1000)
#define fileno _fileno
#define read _read
#include <io.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

#include <getopt.h>

#ifdef HAVE_EDITLINE
#include <histedit.h>
#endif

static char prompt_str[512] = "";

typedef struct {
	char name[128];
	char host[128];
	esl_port_t port;
	char pass[128];
	int debug;
	const char *console_fnkeys[12];
	char loglevel[128];
	int quiet;
} cli_profile_t;

static cli_profile_t profiles[128] = {{{0}}};
static cli_profile_t internal_profile = {{ 0 }};
static int pcount = 0;

static esl_handle_t *global_handle;
static cli_profile_t *global_profile;

static int process_command(esl_handle_t *handle, const char *cmd);

static int running = 1;
static int thread_running = 0;


#ifdef HAVE_EDITLINE
static char *prompt(EditLine * e)
{
    return prompt_str;
}

static EditLine *el;
static History *myhistory;
static HistEvent ev;



/*
 * If a fnkey is configured then process the command
 */
static unsigned char console_fnkey_pressed(int i)
{
	const char *c;

	assert((i > 0) && (i <= 12));

	c = global_profile->console_fnkeys[i - 1];

	/* This new line is necessary to avoid output to begin after the ">" of the CLI's prompt */
	printf("%s\n", c);
	printf("\n");
	
	if (c == NULL) {
		esl_log(ESL_LOG_ERROR, "FUNCTION KEY F%d IS NOT BOUND, please edit your config.\n", i);
		return CC_REDISPLAY;
	}

	if (process_command(global_handle, c)) {
		running = thread_running = 0;
	}

	return CC_REDISPLAY;
}

static unsigned char console_f1key(EditLine * el, int ch)
{
	return console_fnkey_pressed(1);
}
static unsigned char console_f2key(EditLine * el, int ch)
{
	return console_fnkey_pressed(2);
}
static unsigned char console_f3key(EditLine * el, int ch)
{
	return console_fnkey_pressed(3);
}
static unsigned char console_f4key(EditLine * el, int ch)
{
	return console_fnkey_pressed(4);
}
static unsigned char console_f5key(EditLine * el, int ch)
{
	return console_fnkey_pressed(5);
}
static unsigned char console_f6key(EditLine * el, int ch)
{
	return console_fnkey_pressed(6);
}
static unsigned char console_f7key(EditLine * el, int ch)
{
	return console_fnkey_pressed(7);
}
static unsigned char console_f8key(EditLine * el, int ch)
{
	return console_fnkey_pressed(8);
}
static unsigned char console_f9key(EditLine * el, int ch)
{
	return console_fnkey_pressed(9);
}
static unsigned char console_f10key(EditLine * el, int ch)
{
	return console_fnkey_pressed(10);
}
static unsigned char console_f11key(EditLine * el, int ch)
{
	return console_fnkey_pressed(11);
}
static unsigned char console_f12key(EditLine * el, int ch)
{
	return console_fnkey_pressed(12);
}

#endif

static void handle_SIGINT(int sig)
{
	if (sig);
	return;
}


#ifdef WIN32
static HANDLE hStdout;
static WORD wOldColorAttrs;
static CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

static WORD 
#else
static const char*
#endif
COLORS[] = { ESL_SEQ_DEFAULT_COLOR, ESL_SEQ_FRED, ESL_SEQ_FRED, 
			ESL_SEQ_FRED, ESL_SEQ_FMAGEN, ESL_SEQ_FCYAN, ESL_SEQ_FGREEN, ESL_SEQ_FYELLOW };

static int usage(char *name){
	printf("Usage: %s [-H <host>] [-P <port>] [-p <secret>] [-d <level>] [-x command] [profile]\n\n", name);
	printf("  -?,-h --help                    Usage Information\n");
	printf("  -H, --host=hostname             Host to connect\n");
	printf("  -P, --port=port                 Port to connect (1 - 65535)\n");
	printf("  -p, --password=FILENAME         Password\n");
	printf("  -x, --execute=command           Execute Command and Exit\n");
	printf("  -l, --loglevel=command          Log Level\n");
	printf("  -q, --quiet			          Disable logging\n");
	printf("  -d, --debug=level               Debug Level (0 - 7)\n\n");
	return 1;
}

static void *msg_thread_run(esl_thread_t *me, void *obj)
{

	esl_handle_t *handle = (esl_handle_t *) obj;

	thread_running = 1;

	while(thread_running && handle->connected) {
		esl_status_t status = esl_recv_timed(handle, 10);
		
		if (status == ESL_FAIL) {
			esl_log(ESL_LOG_WARNING, "Disconnected.\n");
			running = thread_running = 0;
		} else if (status == ESL_SUCCESS) {
			if (handle->last_event) {
				const char *type = esl_event_get_header(handle->last_event, "content-type");
				int known = 0;

				if (!esl_strlen_zero(type)) {
					if (!strcasecmp(type, "log/data")) {
						int level = 0;
						const char *lname = esl_event_get_header(handle->last_event, "log-level");
#ifdef WIN32
						DWORD len = (DWORD) strlen(handle->last_event->body);
						DWORD outbytes = 0;
#endif			
						if (lname) {
							level = atoi(lname);
						}
						
						
#ifdef WIN32
						
						SetConsoleTextAttribute(hStdout, COLORS[level]);
						WriteFile(hStdout, handle->last_event->body, len, &outbytes, NULL);
						SetConsoleTextAttribute(hStdout, wOldColorAttrs);
#else
						printf("%s%s%s", COLORS[level], handle->last_event->body, ESL_SEQ_DEFAULT_COLOR);
#endif
							
						known++;
					} else if (!strcasecmp(type, "text/disconnect-notice")) {
						running = thread_running = 0;
						known++;
					} else if (!strcasecmp(type, "text/event-plain")) {
						char *foo;
						esl_event_serialize(handle->last_ievent, &foo, ESL_FALSE);
						printf("RECV EVENT\n%s\n", foo);
						free(foo);

						known++;
					}
				}
				
				if (!known) {
					printf("INCOMING DATA [%s]\n%s", type, handle->last_event->body);
				}
			}
		}

		usleep(1000);
	}

	thread_running = 0;
	esl_log(ESL_LOG_DEBUG, "Thread Done\n");

	return NULL;
}

static int process_command(esl_handle_t *handle, const char *cmd) 
{
	if ((*cmd == '/' && cmd++) || !strncasecmp(cmd, "...", 3)) {
		
		if (!strcasecmp(cmd, "help")) {
			printf(
				   "Command                    \tDescription\n"
				   "-----------------------------------------------\n"
				   "/help                      \tHelp\n"
				   "/exit, /quit, /bye, ...    \tExit the program.\n"
				   "/event, /noevent, /nixevent\tEvent commands.\n"
				   "/log, /nolog               \tLog commands.\n"
				   "/filter                    \tFilter commands.\n"
                                   "/debug [0-7]               \tSet debug level.\n"
				   "\n"
				   );

			goto end;
		}

		if (
			!strcasecmp(cmd, "exit") ||
			!strcasecmp(cmd, "quit") ||
			!strcasecmp(cmd, "...") ||
			!strcasecmp(cmd, "bye")
			) {
			esl_log(ESL_LOG_INFO, "Goodbye!\nSee you at ClueCon http://www.cluecon.com/\n");
			return -1;
		}

		if (
			!strncasecmp(cmd, "event", 5) || 
			!strncasecmp(cmd, "noevent", 7) ||
			!strncasecmp(cmd, "nixevent", 8) ||
			!strncasecmp(cmd, "log", 3) || 
			!strncasecmp(cmd, "nolog", 5) || 
			!strncasecmp(cmd, "filter", 6)
			) {

			esl_send_recv(handle, cmd);	

			printf("%s\n", handle->last_sr_reply);

			goto end;
		}
		
		if (!strncasecmp(cmd, "debug", 5)){
			int tmp_debug = atoi(cmd+6);
			if (tmp_debug > -1 && tmp_debug < 8){
				esl_global_set_default_logger(tmp_debug);
				printf("fs_cli debug level set to %d\n", tmp_debug);
			} else {
				printf("fs_cli debug level must be 0 - 7\n");
			}
			goto end;
		}
	
		printf("Unknown command [%s]\n", cmd);
	} else {
		char cmd_str[1024] = "";

		snprintf(cmd_str, sizeof(cmd_str), "api %s\n\n", cmd);
		esl_send_recv(handle, cmd_str);
		if (handle->last_sr_event && handle->last_sr_event->body) {
			printf("%s\n", handle->last_sr_event->body);
		}
	}
	
 end:

	return 0;

}

static int get_profile(const char *name, cli_profile_t **profile)
{
	int x;

	for (x = 0; x < pcount; x++) {
		if (!strcmp(profiles[x].name, name)) {
			*profile = &profiles[x];
			return 0;
		}
	}

	return -1;
}

#ifndef HAVE_EDITLINE
static char command_buf[2048] = "";

static const char *basic_gets(int *cnt)
{
	int x = 0;

	printf("%s", prompt_str);

	memset(&command_buf, 0, sizeof(command_buf));
	for (x = 0; x < (sizeof(command_buf) - 1); x++) {
		int c = getchar();
		if (c < 0) {
			int y = read(fileno(stdin), command_buf, sizeof(command_buf) - 1);
			command_buf[y - 1] = '\0';
			break;
		}
		
		command_buf[x] = (char) c;
		
		if (command_buf[x] == '\n') {
			command_buf[x] = '\0';
			break;
		}
	}

	*cnt = x;

	return command_buf;

}
#endif


static void print_banner(FILE *stream)
{
	fprintf(stream,
			

			"           _____ ____     ____ _     ___            \n"
			"          |  ___/ ___|   / ___| |   |_ _|           \n"
			"          | |_  \\___ \\  | |   | |    | |            \n"
			"          |  _|  ___) | | |___| |___ | |            \n"
			"          |_|   |____/   \\____|_____|___|           \n"
			"\n"
			"*****************************************************\n"
			"* Anthony Minessale II, Ken Rice, Michael Jerris    *\n"
			"* FreeSWITCH (http://www.freeswitch.org)            *\n"
			"* Brought to you by ClueCon http://www.cluecon.com/ *\n"
			"*****************************************************\n"
			"\n\n"
			);
}


static void set_fn_keys(cli_profile_t *profile)
{
	profile->console_fnkeys[0] = "help";
	profile->console_fnkeys[1] = "status";
	profile->console_fnkeys[2] = "show channels";
	profile->console_fnkeys[3] = "show calls";
	profile->console_fnkeys[4] = "sofia status";
	profile->console_fnkeys[5] = "reloadxml";
	profile->console_fnkeys[6] = "/log console";
	profile->console_fnkeys[7] = "/log debug";
	profile->console_fnkeys[8] = "sofia status profile internal";
	profile->console_fnkeys[9] = "fsctl pause";
	profile->console_fnkeys[10] = "fsctl resume";
	profile->console_fnkeys[11] = "version";
}


int main(int argc, char *argv[])
{
	esl_handle_t handle = {{0}};
	int count = 0;
	const char *line = NULL;
	char cmd_str[1024] = "";
	esl_config_t cfg;
	cli_profile_t *profile = NULL;
	int rv = 0;

#ifndef WIN32
	char hfile[512] = "/etc/fs_cli_history";
	char cfile[512] = "/etc/fs_cli.conf";
	char dft_cfile[512] = "/etc/fs_cli.conf";
#else
	char hfile[512] = "fs_cli_history";
	char cfile[512] = "fs_cli.conf";
	char dft_cfile[512] = "fs_cli.conf";
#endif
	char *home = getenv("HOME");
	/* Vars for optargs */
	int opt;
	static struct option options[] = {
		{"help", 0, 0, 'h'},
		{"host", 1, 0, 'H'},
		{"port", 1, 0, 'P'},
		{"password", 1, 0, 'p'},
		{"debug", 1, 0, 'd'},
		{"execute", 1, 0, 'x'},
		{"loglevel", 1, 0, 'l'},
		{"quiet", 0, 0, 'q'},
		{0, 0, 0, 0}
	};

	char temp_host[128];
	int argv_host = 0;
	char temp_pass[128];
	int argv_pass = 0 ;
	int temp_port = 0;
	int argv_port = 0;
	int temp_log = -1;
	int argv_error = 0;
	int argv_exec = 0;
	char argv_command[256] = "";
	char argv_loglevel[128] = "";
	int argv_quiet = 0;
	

	strncpy(internal_profile.host, "127.0.0.1", sizeof(internal_profile.host));
	strncpy(internal_profile.pass, "ClueCon", sizeof(internal_profile.pass));
	strncpy(internal_profile.name, "internal", sizeof(internal_profile.name));
	internal_profile.port = 8021;
	set_fn_keys(&internal_profile);


	if (home) {
		snprintf(hfile, sizeof(hfile), "%s/.fs_cli_history", home);
		snprintf(cfile, sizeof(cfile), "%s/.fs_cli_conf", home);
	}
	
	signal(SIGINT, handle_SIGINT);

	esl_global_set_default_logger(6); /* default debug level to 6 (info) */
	
	for(;;) {
		int option_index = 0;
		opt = getopt_long(argc, argv, "H:U:P:S:p:d:x:l:qh?", options, &option_index);
		if (opt == -1) break;
		switch (opt)
		{
			case 'H':
				esl_set_string(temp_host, optarg);
				argv_host = 1;
				break;
			case 'P':
				temp_port= atoi(optarg);
				if (temp_port > 0 && temp_port < 65536){
					argv_port = 1;
				} else {
					printf("ERROR: Port must be in range 1 - 65535\n");
					argv_error = 1;
				}
				break;
			case 'p':
				esl_set_string(temp_pass, optarg);
				argv_pass = 1;
				break;
			case 'd':
				temp_log=atoi(optarg);
				if (temp_log < 0 || temp_log > 7){
					printf("ERROR: Debug level should be 0 - 7.\n");
					argv_error = 1;
				} else {
					esl_global_set_default_logger(temp_log);
				}
				break;
			case 'x':
				argv_exec = 1;
				esl_set_string(argv_command, optarg);
				break;
			case 'l':
				esl_set_string(argv_loglevel, optarg);
				break;
			case 'q':
				argv_quiet = 1;
				break;
				
			case 'h':
			case '?':
				print_banner(stdout);
				usage(argv[0]);
				return 0;
			default:
				opt = 0;
		}
	}
	
	if (argv_error){
		printf("\n");
		return usage(argv[0]);
	}

	if (!(rv = esl_config_open_file(&cfg, cfile))) {
		rv = esl_config_open_file(&cfg, dft_cfile);
	}

	if (rv) {
		char *var, *val;
		char cur_cat[128] = "";

		while (esl_config_next_pair(&cfg, &var, &val)) {
			if (strcmp(cur_cat, cfg.category)) {
				esl_set_string(cur_cat, cfg.category);
				esl_set_string(profiles[pcount].name, cur_cat);
				esl_set_string(profiles[pcount].host, "localhost");
				esl_set_string(profiles[pcount].pass, "ClueCon");
				profiles[pcount].port = 8021;
				set_fn_keys(&profiles[pcount]);
				esl_log(ESL_LOG_DEBUG, "Found Profile [%s]\n", profiles[pcount].name);
				pcount++;
			}
			
			if (!strcasecmp(var, "host")) {
				esl_set_string(profiles[pcount-1].host, val);
			} else if (!strcasecmp(var, "password")) {
				esl_set_string(profiles[pcount-1].pass, val);
			} else if (!strcasecmp(var, "port")) {
				int pt = atoi(val);
				if (pt > 0) {
					profiles[pcount-1].port = (esl_port_t)pt;
				}
			} else if (!strcasecmp(var, "debug")) {
				int dt = atoi(val);
				if (dt > -1 && dt < 8){
					 profiles[pcount-1].debug = dt;
				}	
 			} else if(!strcasecmp(var, "loglevel")) {
 				esl_set_string(profiles[pcount-1].loglevel, val);
 			} else if(!strcasecmp(var, "quiet")) {
 				profiles[pcount-1].quiet = esl_true(val);
			} else if (!strncasecmp(var, "key_F", 5)) {
				char *key = var + 5;

				if (key) {
					int i = atoi(key);
				
					if (i > 0 && i < 13) {
						profiles[pcount-1].console_fnkeys[i - 1] = strdup(val);
					}
				}
			} 
		}
		esl_config_close_file(&cfg);
	}
	
	if (optind < argc) {
		get_profile(argv[optind], &profile);
	}
	
	if (!profile) {
		if (get_profile("default", &profile)) {
			esl_log(ESL_LOG_DEBUG, "profile default does not exist using builtin profile\n");
			profile = &internal_profile;
		}
	}

	if (temp_log < 0 ) {
		esl_global_set_default_logger(profile->debug);
	}	

	if (argv_host) {
		esl_set_string(profile->host, temp_host);
	}
	if (argv_port) {
		profile->port = (esl_port_t)temp_port;
	}
	if (argv_pass) {
		esl_set_string(profile->pass, temp_pass);
	}
	
	if (*argv_loglevel) {
		esl_set_string(profile->loglevel, argv_loglevel);
		profile->quiet = 0;
	}

	esl_log(ESL_LOG_DEBUG, "Using profile %s [%s]\n", profile->name, profile->host);
	
	if (argv_host) {
		if (argv_port && profile->port != 8021) {
			snprintf(prompt_str, sizeof(prompt_str), "freeswitch@%s:%u@%s> ", profile->host, profile->port, profile->name);
		} else {
			snprintf(prompt_str, sizeof(prompt_str), "freeswitch@%s@%s> ", profile->host, profile->name);
		}
	} else {
		snprintf(prompt_str, sizeof(prompt_str), "freeswitch@%s> ", profile->name);
	}

	if (esl_connect(&handle, profile->host, profile->port, profile->pass)) {
		esl_global_set_default_logger(3);
		esl_log(ESL_LOG_ERROR, "Error Connecting [%s]\n", handle.err);
		return -1;
	}


	if (argv_exec){
		snprintf(cmd_str, sizeof(cmd_str), "api %s\n\n", argv_command);
		esl_send_recv(&handle, cmd_str);
		if (handle.last_sr_event && handle.last_sr_event->body) {
			printf("%s\n", handle.last_sr_event->body);
		}
		esl_disconnect(&handle);
		return 0;
	} 

	global_handle = &handle;
	global_profile = profile;

	esl_thread_create_detached(msg_thread_run, &handle);

#ifdef HAVE_EDITLINE
	el = el_init(__FILE__, stdout, stdout, stdout);
	el_set(el, EL_PROMPT, &prompt);
	el_set(el, EL_EDITOR, "emacs");
	myhistory = history_init();

	el_set(el, EL_ADDFN, "f1-key", "F1 KEY PRESS", console_f1key);
	el_set(el, EL_ADDFN, "f2-key", "F2 KEY PRESS", console_f2key);
	el_set(el, EL_ADDFN, "f3-key", "F3 KEY PRESS", console_f3key);
	el_set(el, EL_ADDFN, "f4-key", "F4 KEY PRESS", console_f4key);
	el_set(el, EL_ADDFN, "f5-key", "F5 KEY PRESS", console_f5key);
	el_set(el, EL_ADDFN, "f6-key", "F6 KEY PRESS", console_f6key);
	el_set(el, EL_ADDFN, "f7-key", "F7 KEY PRESS", console_f7key);
	el_set(el, EL_ADDFN, "f8-key", "F8 KEY PRESS", console_f8key);
	el_set(el, EL_ADDFN, "f9-key", "F9 KEY PRESS", console_f9key);
	el_set(el, EL_ADDFN, "f10-key", "F10 KEY PRESS", console_f10key);
	el_set(el, EL_ADDFN, "f11-key", "F11 KEY PRESS", console_f11key);
	el_set(el, EL_ADDFN, "f12-key", "F12 KEY PRESS", console_f12key);

	el_set(el, EL_BIND, "\033OP", "f1-key", NULL);
	el_set(el, EL_BIND, "\033OQ", "f2-key", NULL);
	el_set(el, EL_BIND, "\033OR", "f3-key", NULL);
	el_set(el, EL_BIND, "\033OS", "f4-key", NULL);


	el_set(el, EL_BIND, "\033[11~", "f1-key", NULL);
	el_set(el, EL_BIND, "\033[12~", "f2-key", NULL);
	el_set(el, EL_BIND, "\033[13~", "f3-key", NULL);
	el_set(el, EL_BIND, "\033[14~", "f4-key", NULL);
	el_set(el, EL_BIND, "\033[15~", "f5-key", NULL);
	el_set(el, EL_BIND, "\033[17~", "f6-key", NULL);
	el_set(el, EL_BIND, "\033[18~", "f7-key", NULL);
	el_set(el, EL_BIND, "\033[19~", "f8-key", NULL);
	el_set(el, EL_BIND, "\033[20~", "f9-key", NULL);
	el_set(el, EL_BIND, "\033[21~", "f10-key", NULL);
	el_set(el, EL_BIND, "\033[23~", "f11-key", NULL);
	el_set(el, EL_BIND, "\033[24~", "f12-key", NULL);


	if (myhistory == 0) {
		esl_log(ESL_LOG_ERROR, "history could not be initialized\n");
		goto done;
	}

	history(myhistory, &ev, H_SETSIZE, 800);
	el_set(el, EL_HIST, history, myhistory);
	history(myhistory, &ev, H_LOAD, hfile);


#endif
#ifdef WIN32
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdout != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
		wOldColorAttrs = csbiInfo.wAttributes;
	}
#endif

	if (!argv_quiet && !profile->quiet) {
		snprintf(cmd_str, sizeof(cmd_str), "log %s\n\n", profile->loglevel);	
		esl_send_recv(&handle, cmd_str);
	}

	print_banner(stdout);

	esl_log(ESL_LOG_INFO, "FS CLI Ready.\nenter /help for a list of commands.\n");

	while (running) {

#ifdef HAVE_EDITLINE
		line = el_gets(el, &count);
#else
		line = basic_gets(&count);
#endif

		if (count > 1) {
			if (!esl_strlen_zero(line)) {
				char *cmd = strdup(line);
				char *p;

#ifdef HAVE_EDITLINE
				const LineInfo *lf = el_line(el);
				char *foo = (char *) lf->buffer;
#endif

				if ((p = strrchr(cmd, '\r')) || (p = strrchr(cmd, '\n'))) {
					*p = '\0';
				}
				assert(cmd != NULL);

#ifdef HAVE_EDITLINE
				history(myhistory, &ev, H_ENTER, line);
#endif
				
				if (process_command(&handle, cmd)) {
					running = 0;
				}

#ifdef HAVE_EDITLINE
				el_deletestr(el, strlen(foo) + 1);
				memset(foo, 0, strlen(foo));
#endif
				free(cmd);
			}
		}

		usleep(1000);

	}

#ifdef HAVE_EDITLINE
 done:
	history(myhistory, &ev, H_SAVE, hfile);

	/* Clean up our memory */
	history_end(myhistory);
	el_end(el);
#endif

	esl_disconnect(&handle);
	
	thread_running = 0;

	return 0;
}
