/*
 * Copyright (c) 2007, Anthony Minessale II
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

#include "openzap.h"
#include "zap_buffer.h"

static unsigned buffer_id = 0;

struct zap_buffer {
	unsigned char *data;
	unsigned char *head;
	zap_size_t used;
	zap_size_t actually_used;
	zap_size_t datalen;
	zap_size_t max_len;
	zap_size_t blocksize;
	unsigned id;
	int loops;
};


OZ_DECLARE(zap_status_t) zap_buffer_create(zap_buffer_t **buffer, zap_size_t blocksize, zap_size_t start_len, zap_size_t max_len)
{
	zap_buffer_t *new_buffer;

	new_buffer = zap_malloc(sizeof(*new_buffer));
	if (new_buffer) {
		memset(new_buffer, 0, sizeof(*new_buffer));

		if (start_len) {
			new_buffer->data = zap_malloc(start_len);
			if (!new_buffer->data) {
				zap_safe_free(new_buffer);
				return ZAP_MEMERR;
			}
			memset(new_buffer->data, 0, start_len);
		}

		new_buffer->max_len = max_len;
		new_buffer->datalen = start_len;
		new_buffer->id = buffer_id++;
		new_buffer->blocksize = blocksize;
		new_buffer->head = new_buffer->data;

		*buffer = new_buffer;
		return ZAP_SUCCESS;
	}

	return ZAP_MEMERR;
}

OZ_DECLARE(zap_size_t) zap_buffer_len(zap_buffer_t *buffer)
{

	assert(buffer != NULL);

	return buffer->datalen;

}


OZ_DECLARE(zap_size_t) zap_buffer_freespace(zap_buffer_t *buffer)
{
	assert(buffer != NULL);


	if (buffer->max_len) {
		return (zap_size_t) (buffer->max_len - buffer->used);
	}
	return 1000000;

}

OZ_DECLARE(zap_size_t) zap_buffer_inuse(zap_buffer_t *buffer)
{
	assert(buffer != NULL);

	return buffer->used;
}

OZ_DECLARE(zap_size_t) zap_buffer_seek(zap_buffer_t *buffer, zap_size_t datalen)
{
	zap_size_t reading = 0;

	assert(buffer != NULL);

	if (buffer->used < 1) {
		buffer->used = 0;
		return 0;
	} else if (buffer->used >= datalen) {
		reading = datalen;
	} else {
		reading = buffer->used;
	}

	buffer->used = buffer->actually_used - reading;
	buffer->head = buffer->data + reading;

	return reading;
}

OZ_DECLARE(zap_size_t) zap_buffer_toss(zap_buffer_t *buffer, zap_size_t datalen)
{
	zap_size_t reading = 0;

	assert(buffer != NULL);

	if (buffer->used < 1) {
		buffer->used = 0;
		return 0;
	} else if (buffer->used >= datalen) {
		reading = datalen;
	} else {
		reading = buffer->used;
	}

	buffer->used -= reading;
	buffer->head += reading;

	return buffer->used;
}

OZ_DECLARE(void) zap_buffer_set_loops(zap_buffer_t *buffer, int loops)
{
	buffer->loops = loops;
}

OZ_DECLARE(zap_size_t) zap_buffer_read_loop(zap_buffer_t *buffer, void *data, zap_size_t datalen)
{
	zap_size_t len;
	if ((len = zap_buffer_read(buffer, data, datalen)) < datalen) {
		if (buffer->loops == 0) {
			return len;
		}
		buffer->head = buffer->data;
		buffer->used = buffer->actually_used;
		len = zap_buffer_read(buffer, (char*)data + len, datalen - len);
		buffer->loops--;
	}
	return len;
}

OZ_DECLARE(zap_size_t) zap_buffer_read(zap_buffer_t *buffer, void *data, zap_size_t datalen)
{
	zap_size_t reading = 0;

	assert(buffer != NULL);
	assert(data != NULL);


	if (buffer->used < 1) {
		buffer->used = 0;
		return 0;
	} else if (buffer->used >= datalen) {
		reading = datalen;
	} else {
		reading = buffer->used;
	}

	memcpy(data, buffer->head, reading);
	buffer->used -= reading;
	buffer->head += reading;

	/* if (buffer->id == 4) printf("%u o %d = %d\n", buffer->id, (unsigned)reading, (unsigned)buffer->used); */
	return reading;
}

OZ_DECLARE(zap_size_t) zap_buffer_write(zap_buffer_t *buffer, const void *data, zap_size_t datalen)
{
	zap_size_t freespace, actual_freespace;

	assert(buffer != NULL);
	assert(data != NULL);
	assert(buffer->data != NULL);

	if (!datalen) {
		return buffer->used;
	}

	actual_freespace = buffer->datalen - buffer->actually_used;
	if (actual_freespace < datalen && (!buffer->max_len || (buffer->used + datalen <= buffer->max_len))) {
		memmove(buffer->data, buffer->head, buffer->used);
		buffer->head = buffer->data;
		buffer->actually_used = buffer->used;
	}

	freespace = buffer->datalen - buffer->used;

	/*
	  if (buffer->data != buffer->head) {
	  memmove(buffer->data, buffer->head, buffer->used);
	  buffer->head = buffer->data;
	  }
	*/
	
	if (freespace < datalen) {
		zap_size_t new_size, new_block_size;
		void *data;
		
		new_size = buffer->datalen + datalen;
		new_block_size = buffer->datalen + buffer->blocksize;

		if (new_block_size > new_size) {
			new_size = new_block_size;
		}
		buffer->head = buffer->data;
		data = realloc(buffer->data, new_size);
		if (!data) {
			return 0;
		}
		buffer->data = data;
		buffer->head = buffer->data;
		buffer->datalen = new_size;
	}
	

	freespace = buffer->datalen - buffer->used;

	if (freespace < datalen) {
		return 0;
	} else {
		memcpy(buffer->head + buffer->used, data, datalen);
		buffer->used += datalen;
		buffer->actually_used += datalen;
	}
	/* if (buffer->id == 4) printf("%u i %d = %d\n", buffer->id, (unsigned)datalen, (unsigned)buffer->used); */

	return buffer->used;
}

OZ_DECLARE(void) zap_buffer_zero(zap_buffer_t *buffer)
{
	assert(buffer != NULL);
	assert(buffer->data != NULL);

	buffer->used = 0;
	buffer->actually_used = 0;
	buffer->head = buffer->data;
}

OZ_DECLARE(zap_size_t) zap_buffer_zwrite(zap_buffer_t *buffer, const void *data, zap_size_t datalen)
{
	zap_size_t w;
	
	if (!(w = zap_buffer_write(buffer, data, datalen))) {
		zap_buffer_zero(buffer);
		return zap_buffer_write(buffer, data, datalen);
	}

	return w;
}

OZ_DECLARE(void) zap_buffer_destroy(zap_buffer_t **buffer)
{
	if (*buffer) {
		zap_safe_free((*buffer)->data);
		zap_safe_free(*buffer);
	}

	*buffer = NULL;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
