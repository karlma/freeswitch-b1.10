/*
 * Copyright (c) 2007-2012, Anthony Minessale II
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SCGI_OOP_H_
#define _SCGI_OOP_H_
#include <scgi.h>
#ifdef __cplusplus
extern "C" { 
#endif

#define this_check(x) do { if (!this) { scgi_log(SCGI_LOG_ERROR, "object is not initalized\n"); return x;}} while(0)
#define this_check_void() do { if (!this) { scgi_log(SCGI_LOG_ERROR, "object is not initalized\n"); return;}} while(0)



class SCGIhandle {
 private:
	scgi_socket_t server_sock;
	scgi_handle_t handle;
	unsigned char buf[65536];
	char *data_buf;
	int buflen;
	int bufsize;
 public:
	SCGIhandle();
	virtual ~SCGIhandle();
	int connected();
	int socketDescriptor();
	int disconnect(void);
	int addParam(const char *name, const char *value);
	int addBody(const char *value);
	char *getBody();
	char *getParam(const char *name);
	char *sendRequest(const char *host, int port, int timeout);
	int respond(char *msg);
	int bind(const char *host, int port);
	int accept(void);
};





#ifdef __cplusplus
}
#endif

#endif
