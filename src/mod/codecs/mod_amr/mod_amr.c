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
 * Brian K. West <brian.west@mac.com>
 *
 * The amr codec itself is not distributed with this module.
 *
 * mod_amr.c -- GSM-AMR Codec Module
 *
 */  
#include "switch.h"
#include "amr-nb/amr-nb.h"

/*
 * Check section 8.1 of rfc3267 for possible sdp options.
 *  
 * SDP Example 
 * 
 * a=fmtp:97 mode-set=0,2,5,7; mode-change-period=2; mode-change-neighbor=1
 *
 *                                 Class A   total speech
 *                Index   Mode       bits       bits
 *                ----------------------------------------
 *                  0     AMR 4.75   42         95
 *                  1     AMR 5.15   49        103
 *                  2     AMR 5.9    55        118
 *                  3     AMR 6.7    58        134
 *                  4     AMR 7.4    61        148
 *                  5     AMR 7.95   75        159
 *                  6     AMR 10.2   65        204
 *                  7     AMR 12.2   81        244
 *                  8     AMR SID    39         39
 *
 *        Table 1.  The number of class A bits for the AMR codec.
 *
 */

static const char modname[] = "mod_amr";

struct amr_context {
	void *encoder_state;
	void *decoder_state;
	int enc_mode;
};

enum
	{
		GENERIC_PARAMETER_AMR_MAXAL_SDUFRAMES = 0,
		GENERIC_PARAMETER_AMR_BITRATE,
		GENERIC_PARAMETER_AMR_GSMAMRCOMFORTNOISE,
		GENERIC_PARAMETER_AMR_GSMEFRCOMFORTNOISE,
		GENERIC_PARAMETER_AMR_IS_641COMFORTNOISE,
		GENERIC_PARAMETER_AMR_PDCEFRCOMFORTNOISE
	};

enum
	{
		AMR_BITRATE_475 = 0,
		AMR_BITRATE_515,
		AMR_BITRATE_590,
		AMR_BITRATE_670,
		AMR_BITRATE_740,
		AMR_BITRATE_795,
		AMR_BITRATE_1020,
		AMR_BITRATE_1220
	};

#define AMR_Mode 7

static switch_status_t switch_amr_init(switch_codec_t *codec, switch_codec_flag_t flags,
									  const switch_codec_settings_t *codec_settings) 
{
	struct amr_context *context = NULL;
	int encoding, decoding;
	int x, argc;
	char *argv[10];

	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!(encoding || decoding) || (!(context = switch_core_alloc(codec->memory_pool, sizeof(struct amr_context))))) {
		return SWITCH_STATUS_FALSE;
	} else {

		if (codec->fmtp_in) {
			argc = switch_separate_string(codec->fmtp_in, ';', argv, (sizeof(argv) / sizeof(argv[0])));
			for(x = 0; x < argc; x++) {
				char *data = argv[x];
				char *arg;
				while(*data && *data == ' ') {
					data++;
				}
				if ((arg = strchr(data, '='))) {
					*arg++ = '\0';
					//printf("Codec arg %d [%s]=[%s]\n", x, data, arg);
				}

			}
		}

		codec->fmtp_out = "octet-align=0; mode-set=7";
		
		context->enc_mode = AMR_Mode; /* start in mode 7 */
		context->encoder_state = NULL;
		context->decoder_state = NULL;

		if (encoding) {
			context->encoder_state = Encoder_Interface_init(1);
		}

		if (decoding) {
			context->decoder_state = Decoder_Interface_init();
		}

		codec->private_info = context;

		return SWITCH_STATUS_SUCCESS;
	}
}

static switch_status_t switch_amr_destroy(switch_codec_t *codec) 
{
	struct amr_context *context = codec->private_info;

	if (context->encoder_state) {
		Encoder_Interface_exit(context->encoder_state);
	}
	if (context->decoder_state) {
		Decoder_Interface_exit(context->decoder_state);
	}
	codec->private_info = NULL;
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_amr_encode(switch_codec_t *codec, 
										switch_codec_t *other_codec, 
										void *decoded_data,

										uint32_t decoded_data_len, 
										uint32_t decoded_rate, 
										void *encoded_data,

										uint32_t *encoded_data_len, 
										uint32_t *encoded_rate, 
										unsigned int *flag) 
{
	struct amr_context *context = codec->private_info;
	
	if (!context) {
		return SWITCH_STATUS_FALSE;
	}
	
	*encoded_data_len = Encoder_Interface_Encode( context->encoder_state, context->enc_mode, (int16_t *)decoded_data, (int8_t *) encoded_data, 0);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_amr_decode(switch_codec_t *codec, 
										switch_codec_t *other_codec, 
										void *encoded_data,

										uint32_t encoded_data_len, 
										uint32_t encoded_rate, 
										void *decoded_data,

										uint32_t *decoded_data_len, 
										uint32_t *decoded_rate, 
										unsigned int *flag) 
{
	struct amr_context *context = codec->private_info;

	if (!context) {
		return SWITCH_STATUS_FALSE;
	}

	Decoder_Interface_Decode( context->decoder_state, (void *)encoded_data, (void *)decoded_data, 0 );
	*decoded_data_len = codec->implementation->bytes_per_frame;
	//printf("D %d/%d\n", encoded_data_len, *decoded_data_len);
	return SWITCH_STATUS_SUCCESS;
}

/* Registration */ 

static const switch_codec_implementation_t amr_implementation = { 
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO, 
	/*.ianacode */ 96, 
	/*.iananame */ "AMR", 
	/*.fmtp */ "octet-align=0",
	/*.samples_per_second */ 8000, 
	/*.bits_per_second */ 0, 
	/*.microseconds_per_frame */ 20000, 
	/*.samples_per_frame */ 160, 
	/*.bytes_per_frame */ 320,
	/*.encoded_bytes_per_frame */ 0, 
	/*.number_of_channels */ 1, 
	/*.pref_frames_per_packet */ 1, 
	/*.max_frames_per_packet */ 1, 
	/*.init */ switch_amr_init, 
	/*.encode */ switch_amr_encode, 
	/*.decode */ switch_amr_decode, 
	/*.destroy */ switch_amr_destroy, 
};

static const switch_codec_interface_t amr_codec_interface = { 
	/*.interface_name */ "GSM-AMR", 
	/*.implementations */ &amr_implementation, 
};

static switch_loadable_module_interface_t amr_module_interface = { 
	/*.module_name */ modname, 
	/*.endpoint_interface */ NULL, 
	/*.timer_interface */ NULL, 
	/*.dialplan_interface */ NULL, 
	/*.codec_interface */ &amr_codec_interface, 
	/*.application_interface */ NULL 
};

SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface,
													 char *filename)
{
	/* connect my internal structure to the blank pointer passed to me */ 
	*module_interface = &amr_module_interface;

	/* indicate that the module should continue to be loaded */ 
	return SWITCH_STATUS_SUCCESS;
}
