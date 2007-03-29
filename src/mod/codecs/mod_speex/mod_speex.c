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
 * mod_speexcodec.c -- Speex Codec Module
 *
 */
#include <switch.h>
#include <speex/speex.h>
#include <speex/speex_preprocess.h>

static const char modname[] = "mod_speex";

static const switch_codec_settings_t default_codec_settings = {
	/*.quality */ 5,
	/*.complexity */ 5,
	/*.enhancement */ 1,
	/*.vad */ 0,
	/*.vbr */ 0,
	/*.vbr_quality */ 4,
	/*.abr */ 0,
	/*.dtx */ 0,
	/*.preproc */ 0,
	/*.pp_vad */ 0,
	/*.pp_agc */ 0,
	/*.pp_agc_level */ 8000,
	/*.pp_denoise */ 0,
	/*.pp_dereverb */ 0,
	/*.pp_dereverb_decay */ 0.4f,
	/*.pp_dereverb_level */ 0.3f,
};

struct speex_context {
	switch_codec_t *codec;
	unsigned int flags;

	/* Encoder */
	void *encoder_state;
	struct SpeexBits encoder_bits;
	unsigned int encoder_frame_size;
	int encoder_mode;
	SpeexPreprocessState *pp;

	/* Decoder */
	void *decoder_state;
	struct SpeexBits decoder_bits;
	unsigned int decoder_frame_size;
	int decoder_mode;
};

static switch_status_t switch_speex_init(switch_codec_t *codec, switch_codec_flag_t flags,
										 const switch_codec_settings_t *codec_settings)
{
	struct speex_context *context = NULL;
	int encoding, decoding;

	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!codec_settings) {
		codec_settings = &default_codec_settings;
	}

	memcpy(&codec->codec_settings, codec_settings, sizeof(codec->codec_settings));

	if (!(encoding || decoding) || ((context = switch_core_alloc(codec->memory_pool, sizeof(*context))) == 0)) {
		return SWITCH_STATUS_FALSE;
	} else {
		const SpeexMode *mode = NULL;

		context->codec = codec;
		if (codec->implementation->samples_per_second == 8000) {
			mode = &speex_nb_mode;
		} else if (codec->implementation->samples_per_second == 16000) {
			mode = &speex_wb_mode;
		} else if (codec->implementation->samples_per_second == 32000) {
			mode = &speex_uwb_mode;
		}

		if (!mode) {
			return SWITCH_STATUS_FALSE;
		}

		if (encoding) {
			speex_bits_init(&context->encoder_bits);
			context->encoder_state = speex_encoder_init(mode);
			speex_encoder_ctl(context->encoder_state, SPEEX_GET_FRAME_SIZE, &context->encoder_frame_size);
			speex_encoder_ctl(context->encoder_state, SPEEX_SET_COMPLEXITY, &codec->codec_settings.complexity);
			if (codec->codec_settings.preproc) {
				context->pp =
					speex_preprocess_state_init(context->encoder_frame_size, codec->implementation->samples_per_second);
				speex_preprocess_ctl(context->pp, SPEEX_PREPROCESS_SET_VAD, &codec->codec_settings.pp_vad);
				speex_preprocess_ctl(context->pp, SPEEX_PREPROCESS_SET_AGC, &codec->codec_settings.pp_agc);
				speex_preprocess_ctl(context->pp, SPEEX_PREPROCESS_SET_AGC_LEVEL, &codec->codec_settings.pp_agc_level);
				speex_preprocess_ctl(context->pp, SPEEX_PREPROCESS_SET_DENOISE, &codec->codec_settings.pp_denoise);
				speex_preprocess_ctl(context->pp, SPEEX_PREPROCESS_SET_DEREVERB, &codec->codec_settings.pp_dereverb);
				speex_preprocess_ctl(context->pp, SPEEX_PREPROCESS_SET_DEREVERB_DECAY,
									 &codec->codec_settings.pp_dereverb_decay);
				speex_preprocess_ctl(context->pp, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL,
									 &codec->codec_settings.pp_dereverb_level);
			}

			if (!codec->codec_settings.abr && !codec->codec_settings.vbr) {
				speex_encoder_ctl(context->encoder_state, SPEEX_SET_QUALITY, &codec->codec_settings.quality);
				if (codec->codec_settings.vad) {
					speex_encoder_ctl(context->encoder_state, SPEEX_SET_VAD, &codec->codec_settings.vad);
				}
			}
			if (codec->codec_settings.vbr) {
				speex_encoder_ctl(context->encoder_state, SPEEX_SET_VBR, &codec->codec_settings.vbr);
				speex_encoder_ctl(context->encoder_state, SPEEX_SET_VBR_QUALITY, &codec->codec_settings.vbr_quality);
			}
			if (codec->codec_settings.abr) {
				speex_encoder_ctl(context->encoder_state, SPEEX_SET_ABR, &codec->codec_settings.abr);
			}
			if (codec->codec_settings.dtx) {
				speex_encoder_ctl(context->encoder_state, SPEEX_SET_DTX, &codec->codec_settings.dtx);
			}
		}

		if (decoding) {
			speex_bits_init(&context->decoder_bits);
			context->decoder_state = speex_decoder_init(mode);
			if (codec->codec_settings.enhancement) {
				speex_decoder_ctl(context->decoder_state, SPEEX_SET_ENH, &codec->codec_settings.enhancement);
			}
		}



		codec->private_info = context;
		return SWITCH_STATUS_SUCCESS;
	}
}

static switch_status_t switch_speex_encode(switch_codec_t *codec,
										   switch_codec_t *other_codec,
										   void *decoded_data,
										   uint32_t decoded_data_len,
										   uint32_t decoded_rate,
										   void *encoded_data,
										   uint32_t * encoded_data_len, uint32_t * encoded_rate, unsigned int *flag)
{
	struct speex_context *context = codec->private_info;
	short *buf;
	int is_speech = 1;

	if (!context) {
		return SWITCH_STATUS_FALSE;
	}

	buf = decoded_data;

	if (context->pp) {
		is_speech = speex_preprocess(context->pp, buf, NULL);
	}

	if (is_speech) {
		is_speech = speex_encode_int(context->encoder_state, buf, &context->encoder_bits)
			|| !context->codec->codec_settings.dtx;
	} else {
		speex_bits_pack(&context->encoder_bits, 0, 5);
	}


	if (is_speech) {
		switch_clear_flag(context, SWITCH_CODEC_FLAG_SILENCE);
		*flag |= SWITCH_CODEC_FLAG_SILENCE_STOP;
	} else {
		if (switch_test_flag(context, SWITCH_CODEC_FLAG_SILENCE)) {
			*encoded_data_len = 0;
			*flag |= SWITCH_CODEC_FLAG_SILENCE;
			return SWITCH_STATUS_SUCCESS;
		}

		switch_set_flag(context, SWITCH_CODEC_FLAG_SILENCE);
		*flag |= SWITCH_CODEC_FLAG_SILENCE_START;
	}


	speex_bits_pack(&context->encoder_bits, 15, 5);
	*encoded_data_len = speex_bits_write(&context->encoder_bits, (char *) encoded_data, context->encoder_frame_size);
	speex_bits_reset(&context->encoder_bits);
	(*encoded_data_len)--;

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_speex_decode(switch_codec_t *codec,
										   switch_codec_t *other_codec,
										   void *encoded_data,
										   uint32_t encoded_data_len,
										   uint32_t encoded_rate,
										   void *decoded_data,
										   uint32_t * decoded_data_len, uint32_t * decoded_rate, unsigned int *flag)
{
	struct speex_context *context = codec->private_info;
	short *buf;

	if (!context) {
		return SWITCH_STATUS_FALSE;
	}


	buf = decoded_data;
	if (*flag & SWITCH_CODEC_FLAG_SILENCE) {
		speex_decode_int(context->decoder_state, NULL, buf);
	} else {
		speex_bits_read_from(&context->decoder_bits, (char *) encoded_data, (int) encoded_data_len);
		speex_decode_int(context->decoder_state, &context->decoder_bits, buf);
	}
	*decoded_data_len = codec->implementation->bytes_per_frame;

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t switch_speex_destroy(switch_codec_t *codec)
{
	int encoding, decoding;
	struct speex_context *context = codec->private_info;

	if (!context) {
		return SWITCH_STATUS_FALSE;
	}

	encoding = (codec->flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (codec->flags & SWITCH_CODEC_FLAG_DECODE);

	if (encoding) {
		speex_bits_destroy(&context->encoder_bits);
		speex_encoder_destroy(context->encoder_state);
	}

	if (decoding) {
		speex_bits_destroy(&context->decoder_bits);
		speex_decoder_destroy(context->decoder_state);
	}

	codec->private_info = NULL;

	return SWITCH_STATUS_SUCCESS;
}

/* Registration */
static const switch_codec_implementation_t speex_32k_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 102,
	/*.iananame */ "speex",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 32000,
	/*.bits_per_second */ 256000,
	/*.nanoseconds_per_frame */ 20000,
	/*.samples_per_frame */ 640,
	/*.bytes_per_frame */ 1280,
	/*.encoded_bytes_per_frame */ 0,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_speex_init,
	/*.encode */ switch_speex_encode,
	/*.decode */ switch_speex_decode,
	/*.destroy */ switch_speex_destroy
};

static const switch_codec_implementation_t speex_16k_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 99,
	/*.iananame */ "speex",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 16000,
	/*.bits_per_second */ 22000,
	/*.nanoseconds_per_frame */ 20000,
	/*.samples_per_frame */ 320,
	/*.bytes_per_frame */ 640,
	/*.encoded_bytes_per_frame */ 0,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_speex_init,
	/*.encode */ switch_speex_encode,
	/*.decode */ switch_speex_decode,
	/*.destroy */ switch_speex_destroy,
	/*.next */ &speex_32k_implementation
};

static const switch_codec_implementation_t speex_8k_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 98,
	/*.iananame */ "speex",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 11000,
	/*.nanoseconds_per_frame */ 20000,
	/*.samples_per_frame */ 160,
	/*.bytes_per_frame */ 320,
	/*.encoded_bytes_per_frame */ 0,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_speex_init,
	/*.encode */ switch_speex_encode,
	/*.decode */ switch_speex_decode,
	/*.destroy */ switch_speex_destroy,
	/*.next */ &speex_16k_implementation
};

static const switch_codec_interface_t speex_codec_interface = {
	/*.interface_name */ "speex",
	/*.implementations */ &speex_8k_implementation
};

static switch_loadable_module_interface_t speex_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ NULL,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ &speex_codec_interface,
	/*.application_interface */ NULL
};

SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface,
													   char *filename)
{
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = &speex_module_interface;

	/* indicate that the module should continue to be loaded */
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
