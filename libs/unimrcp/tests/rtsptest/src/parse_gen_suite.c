/*
 * Copyright 2008 Arsen Chaloyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <apr_file_info.h>
#include <apr_file_io.h>
#include "apt_test_suite.h"
#include "apt_log.h"
#include "rtsp_stream.h"

static apt_bool_t test_stream_generate(rtsp_generator_t *generator, rtsp_message_t *message)
{
	char buffer[500];
	apt_text_stream_t stream;
	rtsp_stream_result_e result;
	apt_bool_t continuation;

	rtsp_generator_message_set(generator,message);
	do {
		apt_text_stream_init(&stream,buffer,sizeof(buffer)-1);
		continuation = FALSE;
		result = rtsp_generator_run(generator,&stream);
		if(result == RTSP_STREAM_MESSAGE_COMPLETE) {
			stream.text.length = stream.pos - stream.text.buf;
			*stream.pos = '\0';
			apt_log(APT_LOG_MARK,APT_PRIO_NOTICE,"Generated RTSP Stream [%lu bytes]\n%s",stream.text.length,stream.text.buf);
		}
		else if(result == RTSP_STREAM_MESSAGE_TRUNCATED) {
			*stream.pos = '\0';
			apt_log(APT_LOG_MARK,APT_PRIO_NOTICE,"Generated RTSP Stream [%lu bytes] continuation awaiting\n%s",stream.text.length,stream.text.buf);
			continuation = TRUE;
		}
		else {
			apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Failed to Generate RTSP Stream");
		}
	}
	while(continuation == TRUE);
	return TRUE;
}

static apt_bool_t rtsp_message_handler(void *obj, rtsp_message_t *message, rtsp_stream_result_e result)
{
	if(result == RTSP_STREAM_MESSAGE_COMPLETE) {
		/* message is completely parsed */
		rtsp_generator_t *generator = obj;
		test_stream_generate(generator,message);
	}
	return TRUE;
}

static apt_bool_t test_file_process(apt_test_suite_t *suite, const char *file_path)
{
	apr_file_t *file;
	char buffer[500];
	apt_text_stream_t stream;
	rtsp_parser_t *parser;
	rtsp_generator_t *generator;
	apr_size_t length;
	apr_size_t offset;

	apt_log(APT_LOG_MARK,APT_PRIO_INFO,"Open File [%s]",file_path);
	if(apr_file_open(&file,file_path,APR_FOPEN_READ | APR_FOPEN_BINARY,APR_OS_DEFAULT,suite->pool) != APR_SUCCESS) {
		apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Failed to Open File");
		return FALSE;
	}

	parser = rtsp_parser_create(suite->pool);
	generator = rtsp_generator_create(suite->pool);

	apt_text_stream_init(&stream,buffer,sizeof(buffer)-1);

	do {
		/* init length of the stream */
		stream.text.length = sizeof(buffer)-1;
		/* calculate offset remaining from the previous receive / if any */
		offset = stream.pos - stream.text.buf;
		/* calculate available length */
		length = stream.text.length - offset;

		if(apr_file_read(file,stream.pos,&length) != APR_SUCCESS) {
			break;
		}
		/* calculate actual length of the stream */
		stream.text.length = offset + length;
		stream.pos[length] = '\0';
		apt_log(APT_LOG_MARK,APT_PRIO_INFO,"Parse RTSP Stream [%lu bytes]\n%s",length,stream.pos);
		
		/* reset pos */
		stream.pos = stream.text.buf;
		rtsp_stream_walk(parser,&stream,rtsp_message_handler,generator);
	}
	while(apr_file_eof(file) != APR_EOF);

	apr_file_close(file);
	return TRUE;
}

static apt_bool_t test_dir_process(apt_test_suite_t *suite)
{
	apr_status_t rv;
	apr_dir_t *dir;

	const char *dir_name = "msg";
	if(apr_dir_open(&dir,dir_name,suite->pool) != APR_SUCCESS) {
		apt_log(APT_LOG_MARK,APT_PRIO_WARNING,"Cannot Open Directory [%s]",dir_name);
		return FALSE;
	}

	do {
		apr_finfo_t finfo;
		rv = apr_dir_read(&finfo,APR_FINFO_DIRENT,dir);
		if(rv == APR_SUCCESS) {
			if(finfo.filetype == APR_REG && finfo.name) {
				char *file_path;
				apr_filepath_merge(&file_path,dir_name,finfo.name,0,suite->pool);
				test_file_process(suite,file_path);
				printf("\nPress ENTER to continue\n");
				getchar();
			}
		}
	} 
	while(rv == APR_SUCCESS);

	apr_dir_close(dir);
	return TRUE;
}

static apt_bool_t parse_gen_test_run(apt_test_suite_t *suite, int argc, const char * const *argv)
{
	test_dir_process(suite);
	return TRUE;
}

apt_test_suite_t* parse_gen_test_suite_create(apr_pool_t *pool)
{
	apt_test_suite_t *suite = apt_test_suite_create(pool,"parse-gen",NULL,parse_gen_test_run);
	return suite;
}
