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
 * mod_codec_gsm.c -- gsm Codec Module
 *
 */  
#include "switch.h"
#include "gsm.h"
static const char modname[] = "mod_gsm";
struct gsm_context {
	gsm encoder;
	gsm decoder;
};
static switch_status_t switch_gsm_init(switch_codec_t *codec, switch_codec_flag_t flags,
									   const switch_codec_settings_t *codec_settings) 
{
	struct gsm_context *context;
	int encoding, decoding;
	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);
	if (!(encoding || decoding)) {
		return SWITCH_STATUS_FALSE;
	} else {
		context = switch_core_alloc(codec->memory_pool, sizeof(*context));
		if (encoding)
			context->encoder = gsm_create();
		if (decoding)
			context->decoder = gsm_create();
	}
	codec->private_info = context;
	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t switch_gsm_destroy(switch_codec_t *codec) 
{
	struct gsm_context *context = codec->private_info;
	int encoding = (codec->flags & SWITCH_CODEC_FLAG_ENCODE);
	int decoding = (codec->flags & SWITCH_CODEC_FLAG_DECODE);
	if (encoding)
		gsm_destroy(context->encoder);
	if (decoding)
		gsm_destroy(context->decoder);
	codec->private_info = NULL;
	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t switch_gsm_encode(switch_codec_t *codec, switch_codec_t *other_codec, void *decoded_data,
										  uint32_t decoded_data_len, uint32_t decoded_rate, void *encoded_data,
										  uint32_t *encoded_data_len, uint32_t *encoded_rate, unsigned int *flag) 
{
	struct gsm_context *context = codec->private_info;
	if (!context) {
		return SWITCH_STATUS_FALSE;
	}
	if (decoded_data_len % 320 == 0) {
		uint32_t new_len = 0;
		gsm_signal * ddp = decoded_data;
		gsm_byte * edp = encoded_data;
		int x;
		int loops = (int) decoded_data_len / 320;
		for (x = 0; x < loops && new_len < *encoded_data_len; x++) {
			gsm_encode(context->encoder, ddp, edp);
			edp += 33;
			ddp += 160;
			new_len += 33;
		}
		if (new_len <= *encoded_data_len) {
			*encoded_data_len = new_len;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "buffer overflow!!! %u >= %u\n", new_len, *encoded_data_len);
			return SWITCH_STATUS_FALSE;
		}
	}
	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t switch_gsm_decode(switch_codec_t *codec, switch_codec_t *other_codec, void *encoded_data,
										  uint32_t encoded_data_len, uint32_t encoded_rate, void *decoded_data,
										  uint32_t *decoded_data_len, uint32_t *decoded_rate, unsigned int *flag) 
{
	struct gsm_context *context = codec->private_info;
	if (!context) {
		return SWITCH_STATUS_FALSE;
	}

	if (encoded_data_len % 33 == 0) {
		int loops = (int) encoded_data_len / 33;
		gsm_byte * edp = encoded_data;
		gsm_signal * ddp = decoded_data;
		int x;
		uint32_t new_len = 0;

		for (x = 0; x < loops && new_len < *decoded_data_len; x++) {
			gsm_decode(context->decoder, edp, ddp);
			ddp += 160;
			edp += 33;
			new_len += 320;
		}
		if (new_len <= *decoded_data_len) {
			*decoded_data_len = new_len;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "buffer overflow!!! %u %u\n", new_len, *decoded_data_len);
			return SWITCH_STATUS_FALSE;
		}
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "yo this frame is an odd size [%u]\n", encoded_data_len);
	}
	return SWITCH_STATUS_SUCCESS;
}


/* Registration */ 
static const switch_codec_implementation_t gsm_8k_implementation = { 
		/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO, 
		/*.ianacode */ 3, 
		/*.iananame */ "GSM", 
		/*.fmtp */ NULL,
		/*.samples_per_second */ 8000, 
		/*.bits_per_second */ 13200, 
		/*.microseconds_per_frame */ 20000, 
		/*.samples_per_frame */ 160, 
		/*.bytes_per_frame */ 320, 
		/*.encoded_bytes_per_frame */ 33, 
		/*.number_of_channels */ 1, 
		/*.pref_frames_per_packet */ 1, 
		/*.max_frames_per_packet */ 1, 
		/*.init */ switch_gsm_init, 
		/*.encode */ switch_gsm_encode, 
		/*.decode */ switch_gsm_decode, 
		/*.destroy */ switch_gsm_destroy, 
};
static const switch_codec_interface_t gsm_codec_interface = { 
		/*.interface_name */ "gsm", 
		/*.implementations */ &gsm_8k_implementation, 
};
static switch_loadable_module_interface_t gsm_module_interface = { 
		/*.module_name */ modname, 
		/*.endpoint_interface */ NULL, 
		/*.timer_interface */ NULL, 
		/*.dialplan_interface */ NULL, 
		/*.codec_interface */ &gsm_codec_interface, 
		/*.application_interface */ NULL 
};
SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface,
														char *filename)
{
	
		/* connect my internal structure to the blank pointer passed to me */ 
		*module_interface = &gsm_module_interface;
	
		/* indicate that the module should continue to be loaded */ 
		return SWITCH_STATUS_SUCCESS;
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
