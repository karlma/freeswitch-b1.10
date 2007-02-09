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
 * mod_g711codec.c -- G711 Ulaw/Alaw Codec Module
 *
 */
#include <switch.h>
#include <g7xx/g711.h>


static const char modname[] = "mod_g711";


static switch_status_t switch_g711u_init(switch_codec_t *codec, switch_codec_flag_t flags,
									   const switch_codec_settings_t *codec_settings)
{
	int encoding, decoding;

	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!(encoding || decoding)) {
		return SWITCH_STATUS_FALSE;
	} else {
		return SWITCH_STATUS_SUCCESS;
	}
}


static switch_status_t switch_g711u_encode(switch_codec_t *codec,
										 switch_codec_t *other_codec,
										 void *decoded_data,
										 uint32_t decoded_data_len,
										 uint32_t decoded_rate,
										 void *encoded_data,
										 uint32_t *encoded_data_len, uint32_t *encoded_rate, unsigned int *flag)
{
	short *dbuf;
	unsigned char *ebuf;
	uint32_t i;

	dbuf = decoded_data;
	ebuf = encoded_data;

	for (i = 0; i < decoded_data_len / sizeof(short); i++) {
		ebuf[i] = linear_to_ulaw(dbuf[i]);
	}

	*encoded_data_len = i;

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_g711u_decode(switch_codec_t *codec,
										 switch_codec_t *other_codec,
										 void *encoded_data,
										 uint32_t encoded_data_len,
										 uint32_t encoded_rate,
										 void *decoded_data,
										 uint32_t *decoded_data_len, uint32_t *decoded_rate, unsigned int *flag)
{
	short *dbuf;
	unsigned char *ebuf;
	uint32_t i;

	dbuf = decoded_data;
	ebuf = encoded_data;

	if (*flag & SWITCH_CODEC_FLAG_SILENCE) {
		memset(dbuf, 0, codec->implementation->bytes_per_frame);
		*decoded_data_len = codec->implementation->bytes_per_frame;
	} else {
		for (i = 0; i < encoded_data_len; i++) {
			dbuf[i] = ulaw_to_linear(ebuf[i]);
		}

		*decoded_data_len = i * 2;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_g711u_destroy(switch_codec_t *codec)
{
	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t switch_g711a_init(switch_codec_t *codec, switch_codec_flag_t flags,
									   const switch_codec_settings_t *codec_settings)
{
	int encoding, decoding;

	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!(encoding || decoding)) {
		return SWITCH_STATUS_FALSE;
	} else {
		return SWITCH_STATUS_SUCCESS;
	}
}


static switch_status_t switch_g711a_encode(switch_codec_t *codec,
										 switch_codec_t *other_codec,
										 void *decoded_data,
										 uint32_t decoded_data_len,
										 uint32_t decoded_rate,
										 void *encoded_data,
										 uint32_t *encoded_data_len, uint32_t *encoded_rate, unsigned int *flag)
{
	short *dbuf;
	unsigned char *ebuf;
	uint32_t i;

	dbuf = decoded_data;
	ebuf = encoded_data;

	for (i = 0; i < decoded_data_len / sizeof(short); i++) {
		ebuf[i] = linear_to_alaw(dbuf[i]);
	}

	*encoded_data_len = i;

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_g711a_decode(switch_codec_t *codec,
										 switch_codec_t *other_codec,
										 void *encoded_data,
										 uint32_t encoded_data_len,
										 uint32_t encoded_rate,
										 void *decoded_data,
										 uint32_t *decoded_data_len, uint32_t *decoded_rate, unsigned int *flag)
{
	short *dbuf;
	unsigned char *ebuf;
	uint32_t i;

	dbuf = decoded_data;
	ebuf = encoded_data;

	if (*flag & SWITCH_CODEC_FLAG_SILENCE) {
		memset(dbuf, 0, codec->implementation->bytes_per_frame);
		*decoded_data_len = codec->implementation->bytes_per_frame;
	} else {
		for (i = 0; i < encoded_data_len; i++) {
			dbuf[i] = alaw_to_linear(ebuf[i]);
		}

		*decoded_data_len = i * 2;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_g711a_destroy(switch_codec_t *codec)
{
	return SWITCH_STATUS_SUCCESS;
}

/* Registration */


static const switch_codec_implementation_t g711u_8k_60ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 0,
	/*.iananame */ "PCMU",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 19200,
	/*.microseconds_per_frame */ 60000,
	/*.samples_per_frame */ 480,
	/*.bytes_per_frame */ 960,
	/*.encoded_bytes_per_frame */ 480,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711u_init,
	/*.encode */ switch_g711u_encode,
	/*.decode */ switch_g711u_decode,
	/*.destroy */ switch_g711u_destroy
};

static const switch_codec_implementation_t g711u_8k_30ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 0,
	/*.iananame */ "PCMU",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 96000,
	/*.microseconds_per_frame */ 30000,
	/*.samples_per_frame */ 240,
	/*.bytes_per_frame */ 480,
	/*.encoded_bytes_per_frame */ 240,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711u_init,
	/*.encode */ switch_g711u_encode,
	/*.decode */ switch_g711u_decode,
	/*.destroy */ switch_g711u_destroy,
	/*.next*/ &g711u_8k_60ms_implementation
};

static const switch_codec_implementation_t g711u_8k_20ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 0,
	/*.iananame */ "PCMU",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 64000,
	/*.microseconds_per_frame */ 20000,
	/*.samples_per_frame */ 160,
	/*.bytes_per_frame */ 320,
	/*.encoded_bytes_per_frame */ 160,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711u_init,
	/*.encode */ switch_g711u_encode,
	/*.decode */ switch_g711u_decode,
	/*.destroy */ switch_g711u_destroy,
	/*.next*/ &g711u_8k_30ms_implementation
};

static const switch_codec_implementation_t g711u_8k_10ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 0,
	/*.iananame */ "PCMU",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 64000,
	/*.microseconds_per_frame */ 10000,
	/*.samples_per_frame */ 80,
	/*.bytes_per_frame */ 160,
	/*.encoded_bytes_per_frame */ 80,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711u_init,
	/*.encode */ switch_g711u_encode,
	/*.decode */ switch_g711u_decode,
	/*.destroy */ switch_g711u_destroy,
	/*.next*/ &g711u_8k_20ms_implementation
};




static const switch_codec_implementation_t g711a_8k_60ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 8,
	/*.iananame */ "PCMA",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 19200,
	/*.microseconds_per_frame */ 60000,
	/*.samples_per_frame */ 480,
	/*.bytes_per_frame */ 960,
	/*.encoded_bytes_per_frame */ 480,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711u_init,
	/*.encode */ switch_g711u_encode,
	/*.decode */ switch_g711u_decode,
	/*.destroy */ switch_g711u_destroy,
};

static const switch_codec_implementation_t g711a_8k_30ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 8,
	/*.iananame */ "PCMA",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 96000,
	/*.microseconds_per_frame */ 30000,
	/*.samples_per_frame */ 240,
	/*.bytes_per_frame */ 480,
	/*.encoded_bytes_per_frame */ 240,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711u_init,
	/*.encode */ switch_g711u_encode,
	/*.decode */ switch_g711u_decode,
	/*.destroy */ switch_g711u_destroy,
    /*.next*/ &g711a_8k_60ms_implementation
};

static const switch_codec_implementation_t g711a_8k_20ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 8,
	/*.iananame */ "PCMA",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 64000,
	/*.microseconds_per_frame */ 20000,
	/*.samples_per_frame */ 160,
	/*.bytes_per_frame */ 320,
	/*.encoded_bytes_per_frame */ 160,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711a_init,
	/*.encode */ switch_g711a_encode,
	/*.decode */ switch_g711a_decode,
	/*.destroy */ switch_g711a_destroy,
    /*.next*/ &g711a_8k_30ms_implementation
};

static const switch_codec_implementation_t g711a_8k_10ms_implementation = {
	/*.codec_type */ SWITCH_CODEC_TYPE_AUDIO,
	/*.ianacode */ 8,
	/*.iananame */ "PCMA",
	/*.fmtp */ NULL,
	/*.samples_per_second */ 8000,
	/*.bits_per_second */ 64000,
	/*.microseconds_per_frame */ 10000,
	/*.samples_per_frame */ 80,
	/*.bytes_per_frame */ 160,
	/*.encoded_bytes_per_frame */ 80,
	/*.number_of_channels */ 1,
	/*.pref_frames_per_packet */ 1,
	/*.max_frames_per_packet */ 1,
	/*.init */ switch_g711u_init,
	/*.encode */ switch_g711u_encode,
	/*.decode */ switch_g711u_decode,
	/*.destroy */ switch_g711u_destroy,
    /*.next*/ &g711a_8k_20ms_implementation
};



static const switch_codec_interface_t g711a_codec_interface = {
	/*.interface_name */ "g711 alaw",
	/*.implementations */ &g711a_8k_10ms_implementation
};

static const switch_codec_interface_t g711u_codec_interface = {
	/*.interface_name */ "g711 ulaw",
	/*.implementations */ &g711u_8k_10ms_implementation,
	/*.next */ &g711a_codec_interface
};

static switch_loadable_module_interface_t g711_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ NULL,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ &g711u_codec_interface,
	/*.application_interface */ NULL
};


SWITCH_MOD_DECLARE(switch_status_t) switch_module_load(const switch_loadable_module_interface_t **module_interface, char *filename)
{
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = &g711_module_interface;

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
