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
 * Michael Jerris <mike@jerris.com>
 *
 * mod_ilbc.c -- ilbc Codec Module
 *
 */  
#include "switch.h"
#include "iLBC_encode.h"
#include "iLBC_decode.h"

static const char modname[] = "mod_ilbc";

struct ilbc_context {
	iLBC_Enc_Inst_t encoder;
	iLBC_Dec_Inst_t decoder;
};

static switch_status switch_ilbc_init(switch_codec *codec, switch_codec_flag flags,
									  const struct switch_codec_settings *codec_settings) 
{
	struct ilbc_context *context;
	int encoding, decoding;

	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!(encoding || decoding)) {
		return SWITCH_STATUS_FALSE;
	} else {
		context = switch_core_alloc(codec->memory_pool, sizeof(*context));
		if (encoding)
			initEncode(&context->encoder, 30);
		if (decoding)
			initDecode(&context->decoder, 30, 0);
	}

	codec->private_info = context;
	return SWITCH_STATUS_SUCCESS;
}


static switch_status switch_ilbc_destroy(switch_codec *codec) 
{
	codec->private_info = NULL;
	return SWITCH_STATUS_SUCCESS;
}

static switch_status switch_ilbc_encode(switch_codec *codec, 
										switch_codec *other_codec, 
										void *decoded_data,

										size_t decoded_data_len, 
										int decoded_rate, 
										void *encoded_data,

										size_t *encoded_data_len, 
										int *encoded_rate, 
										unsigned int *flag) 
{
	struct ilbc_context *context = codec->private_info;
	int cbret = 0;

	if (!context) {
		return SWITCH_STATUS_FALSE;
	}
	if (decoded_data_len % 320 == 0) {
		unsigned int new_len = 0;
		ilbc_signal * ddp = decoded_data;
		ilbc_byte * edp = encoded_data;
		int x;
		int loops = (int) decoded_data_len / 320;
		for (x = 0; x < loops && new_len < *encoded_data_len; x++) {
			iLBC_encode(context->encoder, ddp, edp);
			edp += 33;
			ddp += 160;
			new_len += 33;
		}
		if (new_len <= *encoded_data_len) {
			*encoded_data_len = new_len;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "buffer overflow!!! %u >= %u\n", new_len, *encoded_data_len);
			return SWITCH_STATUS_FALSE;
		}
	}
	return SWITCH_STATUS_SUCCESS;
}

static switch_status switch_ilbc_decode(switch_codec *codec, 
										switch_codec *other_codec, 
										void *encoded_data,

										size_t encoded_data_len, 
										int encoded_rate, 
										void *decoded_data,

										size_t *decoded_data_len, 
										int *decoded_rate, 
										unsigned int *flag) 
{
	struct ilbc_context *context = codec->private_info;

	if (!context) {
		return SWITCH_STATUS_FALSE;
	}

	if (encoded_data_len % 33 == 0) {
		int loops = (int) encoded_data_len / 33;
		ilbc_byte * edp = encoded_data;
		ilbc_signal * ddp = decoded_data;
		int x;
		unsigned int new_len = 0;

		for (x = 0; x < loops && new_len < *decoded_data_len; x++) {
			iLBC_decode(context->decoder, edp, ddp);
			ddp += 160;
			edp += 33;
			new_len += 320;
		}
		if (new_len <= *decoded_data_len) {
			*decoded_data_len = new_len;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "buffer overflow!!!\n");
			return SWITCH_STATUS_FALSE;
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "yo this frame is an odd size [%d]\n", encoded_data_len);
	}
	return SWITCH_STATUS_SUCCESS;
}


/* Registration */ 

static const switch_codec_implementation ilbc_8k_implementation = { 
		/*.samples_per_second */ 8000, 
		/*.bits_per_second */ 13200, 
		/*.microseconds_per_frame */ 20000, 
		/*.samples_per_frame */ 160, 
		/*.bytes_per_frame */ 320, 
		/*.encoded_bytes_per_frame */ 33, 
		/*.number_of_channels */ 1, 
		/*.pref_frames_per_packet */ 1, 
		/*.max_frames_per_packet */ 1, 
		/*.init */ switch_ilbc_init, 
		/*.encode */ switch_ilbc_encode, 
		/*.decode */ switch_ilbc_decode, 
		/*.destroy */ switch_ilbc_destroy, 
};


static const switch_codec_interface ilbc_codec_interface = { 
		/*.interface_name */ "ilbc", 
		/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO, 
		/*.ianacode */ 3, 
		/*.iananame */ "ilbc", 
		/*.implementations */ &ilbc_8k_implementation, 
};


static switch_loadable_module_interface ilbc_module_interface = { 
		/*.module_name */ modname, 
		/*.endpoint_interface */ NULL, 
		/*.timer_interface */ NULL, 
		/*.dialplan_interface */ NULL, 
		/*.codec_interface */ &ilbc_codec_interface, 
		/*.application_interface */ NULL 
};



SWITCH_MOD_DECLARE(switch_status) switch_module_load(const switch_loadable_module_interface **interface,
														char *filename)
{
	
		/* connect my internal structure to the blank pointer passed to me */ 
		*interface = &ilbc_module_interface;
	

		/* indicate that the module should continue to be loaded */ 
		return SWITCH_STATUS_SUCCESS;

}







