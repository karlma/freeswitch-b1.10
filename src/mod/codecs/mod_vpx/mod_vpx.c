/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2012, Anthony Minessale II <anthm@freeswitch.org>
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
 * Seven Du <dujinfang@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Anthony Minessale II <anthm@freeswitch.org>
 * Seven Du <dujinfang@gmail.com>
 * Sam Russell <sam.h.russell@gmail.com>
 *
 * mod_vpx.c -- VP8/9 Video Codec, with transcoding
 *
 */

#include <switch.h>
#include <vpx/vpx_encoder.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8cx.h>
#include <vpx/vp8dx.h>
#include <vpx/vp8.h>

#define FPS 15
#define SLICE_SIZE 1200

SWITCH_MODULE_LOAD_FUNCTION(mod_vpx_load);
SWITCH_MODULE_DEFINITION(mod_vpx, mod_vpx_load, NULL, NULL);

struct vpx_context {
	switch_codec_t *codec;
	unsigned int flags;

	vpx_codec_enc_cfg_t	config;

	vpx_codec_ctx_t	encoder;
	vpx_image_t *pic;
	switch_bool_t force_key_frame;
	int width;
	int height;
	int bitrate;
	int fps;
	int format;
	int intra_period;
	int pts;
	int num;
	int partition_index;
	const vpx_codec_cx_pkt_t *pkt;
	int pkt_pos;
	vpx_codec_iter_t iter;
	switch_time_t last_ts;

	vpx_codec_ctx_t	decoder;
	switch_buffer_t *vpx_packet_buffer;
	int got_key_frame;
	switch_size_t last_received_timestamp;
	switch_bool_t last_received_complete_picture;
	int need_key_frame;
};
typedef struct vpx_context vpx_context_t;

static switch_status_t switch_vpx_init(switch_codec_t *codec, switch_codec_flag_t flags, const switch_codec_settings_t *codec_settings)
{
	vpx_context_t *context = NULL;
	int encoding, decoding;
	vpx_codec_ctx_t	*encoder = NULL;
	vpx_codec_ctx_t	*decoder = NULL;
	vpx_codec_enc_cfg_t *config;
	const vpx_codec_iface_t* encoder_interface = vpx_codec_vp8_cx();
	const vpx_codec_iface_t* decoder_interface = vpx_codec_vp8_dx();


	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!(encoding || decoding) || ((context = switch_core_alloc(codec->memory_pool, sizeof(*context))) == 0)) {
		return SWITCH_STATUS_FALSE;
	}
	memset(context, 0, sizeof(*context));
	context->flags = flags;
	codec->private_info = context;

	if (codec->fmtp_in) {
		codec->fmtp_out = switch_core_strdup(codec->memory_pool, codec->fmtp_in);
	}

	config = &context->config;

	if (vpx_codec_enc_config_default(encoder_interface, config, 0) != VPX_CODEC_OK) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Encoder config Error\n");
		return SWITCH_STATUS_FALSE;
	}

	context->width = 352;
	context->height = 288;
	context->bitrate = 38400;

	// settings
	config->g_profile = 1;
	config->g_w = context->width;
	config->g_h = context->height;
	config->rc_target_bitrate = context->bitrate;
	config->g_timebase.num = 1;
	config->g_timebase.den = 1000;
	config->g_error_resilient = VPX_ERROR_RESILIENT_PARTITIONS;
	config->g_lag_in_frames = 0; // 0- no frame lagging
	config->g_threads = 1;
	// rate control settings
	config->rc_dropframe_thresh = 0;
	config->rc_end_usage = VPX_CBR;
	config->g_pass = VPX_RC_ONE_PASS;
	// config->kf_mode = VPX_KF_DISABLED;
	config->kf_mode = VPX_KF_AUTO;
	// config->kf_min_dist = FPS;// Intra Period 3 seconds;
	// config->kf_max_dist = FPS * 3;
	config->rc_resize_allowed = 0;
	config->rc_min_quantizer = 2;
	config->rc_max_quantizer = 56;
	//Rate control adaptation undershoot control.
	//	This value, expressed as a percentage of the target bitrate,
	//	controls the maximum allowed adaptation speed of the codec.
	//	This factor controls the maximum amount of bits that can be
	//	subtracted from the target bitrate in order to compensate for
	//	prior overshoot.
	//	Valid values in the range 0-1000.
	config->rc_undershoot_pct = 100;
	//Rate control adaptation overshoot control.
	//	This value, expressed as a percentage of the target bitrate,
	//	controls the maximum allowed adaptation speed of the codec.
	//	This factor controls the maximum amount of bits that can be
	//	added to the target bitrate in order to compensate for prior
	//	undershoot.
	//	Valid values in the range 0-1000.
	config->rc_overshoot_pct = 15;
	//Decoder Buffer Size.
	//	This value indicates the amount of data that may be buffered
	//	by the decoding application. Note that this value is expressed
	//	in units of time (milliseconds). For example, a value of 5000
	//	indicates that the client will buffer (at least) 5000ms worth
	//	of encoded data. Use the target bitrate (rc_target_bitrate) to
	//	convert to bits/bytes, if necessary.
	config->rc_buf_sz = 1000;
	//Decoder Buffer Initial Size.
	//	This value indicates the amount of data that will be buffered
	//	by the decoding application prior to beginning playback.
	//	This value is expressed in units of time (milliseconds).
	//	Use the target bitrate (rc_target_bitrate) to convert to
	//	bits/bytes, if necessary.
	config->rc_buf_initial_sz = 500;
	//Decoder Buffer Optimal Size.
	//	This value indicates the amount of data that the encoder should
	//	try to maintain in the decoder's buffer. This value is expressed
	//	in units of time (milliseconds).
	//	Use the target bitrate (rc_target_bitrate) to convert to
	//	bits/bytes, if necessary.
	config->rc_buf_optimal_sz = 600;

	if (encoding) {
		if (vpx_codec_enc_init(&context->encoder, encoder_interface, config, 0 & VPX_CODEC_USE_OUTPUT_PARTITION) != VPX_CODEC_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Codec init error: [%d:%s]\n", encoder->err, encoder->err_detail);
			return SWITCH_STATUS_FALSE;
		}

		// The static threshold imposes a change threshold on blocks below which they will be skipped by the encoder.
		vpx_codec_control(encoder, VP8E_SET_STATIC_THRESHOLD, 100);
		//Set cpu usage, a bit lower than normal (-6) but higher than android (-12)
		vpx_codec_control(encoder, VP8E_SET_CPUUSED, -8);
		// Only one partition
		// vpx_codec_control(encoder, VP8E_SET_TOKEN_PARTITIONS, VP8_ONE_TOKENPARTITION);
		// Enable noise reduction
		vpx_codec_control(encoder, VP8E_SET_NOISE_SENSITIVITY, 0);
		//Set max data rate for Intra frames.
		//	This value controls additional clamping on the maximum size of a keyframe.
		//	It is expressed as a percentage of the average per-frame bitrate, with the
		//	special (and default) value 0 meaning unlimited, or no additional clamping
		//	beyond the codec's built-in algorithm.
		//	For example, to allocate no more than 4.5 frames worth of bitrate to a keyframe, set this to 450.
		vpx_codec_control(encoder, VP8E_SET_MAX_INTRA_BITRATE_PCT, 0);
	}

	if (decoding) {
		vp8_postproc_cfg_t ppcfg;

		if (vpx_codec_dec_init(&context->decoder, decoder_interface, NULL, VPX_CODEC_USE_POSTPROC) != VPX_CODEC_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Codec init error: [%d:%s]\n", encoder->err, encoder->err_detail);
			return SWITCH_STATUS_FALSE;
		}

		// the types of post processing to be done, should be combination of "vp8_postproc_level"
		ppcfg.post_proc_flag = VP8_DEMACROBLOCK | VP8_DEBLOCK;
		// the strength of deblocking, valid range [0, 16]
		ppcfg.deblocking_level = 3;
		// Set deblocking settings
		vpx_codec_control(decoder, VP8_SET_POSTPROC, &ppcfg);

		switch_buffer_create_dynamic(&context->vpx_packet_buffer, 512, 512, 1024000);
	}

	return SWITCH_STATUS_SUCCESS;
}

/*	http://tools.ietf.org/html/draft-ietf-payload-vp8-10

	The first octets after the RTP header are the VP8 payload descriptor, with the following structure.

	     0 1 2 3 4 5 6 7
	    +-+-+-+-+-+-+-+-+
	    |X|R|N|S|R| PID | (REQUIRED)
	    +-+-+-+-+-+-+-+-+
	X:  |I|L|T|K| RSV   | (OPTIONAL)
	    +-+-+-+-+-+-+-+-+
	I:  |M| PictureID   | (OPTIONAL)
	    +-+-+-+-+-+-+-+-+
	L:  |   TL0PICIDX   | (OPTIONAL)
	    +-+-+-+-+-+-+-+-+
	T/K:|TID|Y| KEYIDX  | (OPTIONAL)
	    +-+-+-+-+-+-+-+-+


	VP8 Payload Header

	 0 1 2 3 4 5 6 7
	+-+-+-+-+-+-+-+-+
	|Size0|H| VER |P|
	+-+-+-+-+-+-+-+-+
	|     Size1     |
	+-+-+-+-+-+-+-+-+
	|     Size2     |
	+-+-+-+-+-+-+-+-+
	| Bytes 4..N of |
	| VP8 payload   |
	:               :
	+-+-+-+-+-+-+-+-+
	| OPTIONAL RTP  |
	| padding       |
	:               :
	+-+-+-+-+-+-+-+-+
*/

static switch_status_t consume_partition(vpx_context_t *context, void *data, uint32_t *len, uint32_t *flag)
{
	if (!context->pkt) context->pkt = vpx_codec_get_cx_data(&context->encoder, &context->iter);

	*flag &= ~SFF_MARKER;

	if (context->pkt) {
		// if (context->pkt->kind == VPX_CODEC_CX_FRAME_PKT && (context->pkt->data.frame.flags & VPX_FRAME_IS_KEY) && context->pkt_pos == 0) {
		// 	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "============================Got a VP8 Key Frame size:[%d]===================================\n", (int)context->pkt->data.frame.sz);
		// }

		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "size:%d flag: %x part_id: %d pts: %lld duration:%ld\n",
		// 	(int)context->pkt->data.frame.sz, context->pkt->data.frame.flags, context->pkt->data.frame.partition_id, context->pkt->data.frame.pts, context->pkt->data.frame.duration);
	}

	if (!context->pkt || context->pkt_pos >= context->pkt->data.frame.sz - 1 || context->pkt->kind != VPX_CODEC_CX_FRAME_PKT) {
		*len = 0;
		*flag |= SFF_MARKER;
		context->pkt_pos = 0;
		context->pkt = NULL;
		return SWITCH_STATUS_SUCCESS;
	}

	if (context->pkt->data.frame.sz < SLICE_SIZE) {
		uint8_t hdr = 0x10;

		memcpy(data, &hdr, 1);
		memcpy((uint8_t *)data + 1, context->pkt->data.frame.buf, context->pkt->data.frame.sz);
		*len = context->pkt->data.frame.sz + 1;
		*flag |= SFF_MARKER;
		context->pkt = NULL;
		context->pkt_pos = 0;
		return SWITCH_STATUS_SUCCESS;
	} else {
		int left = context->pkt->data.frame.sz - context->pkt_pos;
		uint8_t *p = data;

		if (left < SLICE_SIZE) {
			p[0] = 0;
			memcpy(p+1, (uint8_t *)context->pkt->data.frame.buf + context->pkt_pos, left);
			context->pkt_pos = 0;
			context->pkt = NULL;
			*len = left + 1;
			*flag |= SFF_MARKER;
			return SWITCH_STATUS_SUCCESS;
		} else {
			uint8_t hdr = context->pkt_pos == 0 ? 0x10 : 0;

			p[0] = hdr;
			memcpy(p+1, (uint8_t *)context->pkt->data.frame.buf + context->pkt_pos, SLICE_SIZE - 1);
			context->pkt_pos += (SLICE_SIZE - 1);
			*len = SLICE_SIZE;
			return SWITCH_STATUS_MORE_DATA;
		}
	}
}

static switch_status_t switch_vpx_encode(switch_codec_t *codec, switch_image_t *img,
										  void *encoded_data, uint32_t *encoded_data_len,
										  unsigned int *flag)
{
	vpx_context_t *context = (vpx_context_t *)codec->private_info;
	uint32_t duration = 90000 / FPS;
	int width = 0;
	int height = 0;
	vpx_enc_frame_flags_t vpx_flags = 0;

	if (*flag & SFF_WAIT_KEY_FRAME) {
		context->need_key_frame = 1;
	}

	if (img == NULL) {
		return consume_partition(context, encoded_data, encoded_data_len, flag);
	}

	width = img->d_w;
	height = img->d_h;

	//switch_assert(width > 0 && (width % 4 == 0));
	//switch_assert(height > 0 && (height % 4 == 0));

	if (context->config.g_w != width || context->config.g_h != height) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "VPX reset encoder picture from %dx%d to %dx%d\n", context->config.g_w, context->config.g_h, width, height);
		context->config.g_w = width;
		context->config.g_h = height;
		if (vpx_codec_enc_config_set(&context->encoder, &context->config) != VPX_CODEC_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "VPX reset config error!");
		}
	}

	if (context->last_ts == 0) context->last_ts = switch_micro_time_now();

	if ((switch_micro_time_now() - context->last_ts) > 2 * 1000000) {
		// the config params doesn't seems work for generate regular key frames,
		// so we do some trick here to force a key frame every 2 sec
		// vpx_flags = VPX_EFLAG_FORCE_KF;
		context->last_ts = switch_micro_time_now();
	}

	if (context->need_key_frame > 0) {
		// force generate a key frame
		vpx_flags |= VPX_EFLAG_FORCE_KF;
		context->last_ts = switch_micro_time_now();
		context->need_key_frame--;
	}

	if (vpx_codec_encode(&context->encoder, (vpx_image_t *)img, context->pts, duration, vpx_flags, VPX_DL_REALTIME) != VPX_CODEC_OK) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "VP8 encode error %d:%s\n",
			context->encoder.err, context->encoder.err_detail);
		return SWITCH_STATUS_FALSE;
	}

	context->pts += duration;
	context->iter = NULL;

	return consume_partition(context, encoded_data, encoded_data_len, flag);
}

static void buffer_vpx_packets(vpx_context_t *context, switch_frame_t *frame)
{
	uint8_t *data = frame->data;
	uint8_t S;
	uint8_t DES;
	uint8_t PID;
	int len;

	if (!frame) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "no frame in codec!!\n");
		return;
	}

	DES = *data;
	data++;
	S = DES & 0x10;
	PID = DES & 0x07;

	if (DES & 0x80) { // X
		uint8_t X = *data;
		data++;
		if (X & 0x80) { // I
			uint8_t M = (*data) & 0x80;
			data++;
			if (M) data++;
		}
		if (X & 0x40) data++; // L
		if (X & 0x30) data++; // T/K
	}

	len = frame->datalen - (data - (uint8_t *)frame->data);
	if (len <= 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid packet %d\n", len);
		switch_buffer_zero(context->vpx_packet_buffer);
		return;
	}

	if (S && (PID == 0)) {
		uint8_t keyframe;

		keyframe = ((*data) & 0x01) ? 0 : 1;

		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[%d] PID: %d K:%d P:%d Inv:%d len: %d size:%d\n", frame->datalen, PID, keyframe, profile, invisible, len, size);

		if (keyframe) {
			if (!context->got_key_frame) context->got_key_frame = 1;
		}
	}

	if (!context->got_key_frame) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Waiting for key frame\n");
		switch_set_flag(frame, SFF_WAIT_KEY_FRAME);
		return;
	}

	switch_buffer_write(context->vpx_packet_buffer, data, len);
}

static switch_status_t switch_vpx_decode(switch_codec_t *codec, switch_frame_t *frame, switch_image_t **img, unsigned int *flag)
{
	vpx_context_t *context = (vpx_context_t *)codec->private_info;
	vpx_codec_ctx_t *decoder = &context->decoder;
	switch_size_t len;

	if (!decoder) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "VPX decoder is not initialized!\n");
		return SWITCH_STATUS_FALSE;
	}

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "len: %d ts: %" SWITCH_SIZE_T_FMT " mark:%d\n", frame->datalen, frame->timestamp, frame->m);

	if (context->last_received_timestamp && context->last_received_timestamp != frame->timestamp &&
		(!frame->m) && (!context->last_received_complete_picture)) {
		// possible packet loss
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Packet Loss, skip previouse received frame (to avoid crash?)\n");
		switch_buffer_zero(context->vpx_packet_buffer);
	}

	context->last_received_timestamp = frame->timestamp;
	context->last_received_complete_picture = frame->m ? SWITCH_TRUE : SWITCH_FALSE;

	buffer_vpx_packets(context, frame);

	len = switch_buffer_inuse(context->vpx_packet_buffer);

	if (frame->m && len) {
		uint8_t *data;
		vpx_codec_iter_t iter = NULL;
		int corrupted = 0;
		int err;
		//		int keyframe = 0;

		switch_buffer_peek_zerocopy(context->vpx_packet_buffer, (void *)&data);
		//		keyframe = (*data & 0x01) ? 0 : 1;

		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "buffered: %" SWITCH_SIZE_T_FMT ", key: %d\n", len, keyframe);

		err = vpx_codec_decode(decoder, data, (unsigned int)len, NULL, 0);

		if (err != VPX_CODEC_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error decoding %" SWITCH_SIZE_T_FMT " bytes, [%d:%d:%s]\n", len, err, decoder->err, decoder->err_detail);
			switch_set_flag(frame, SFF_WAIT_KEY_FRAME);
			context->got_key_frame = 0;
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "require key frame %d\n", context->got_key_frame);
			goto error;
		}

		if (vpx_codec_control(decoder, VP8D_GET_FRAME_CORRUPTED, &corrupted) != VPX_CODEC_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "VPX control error!\n");
			goto error;
		}

		*img = (switch_image_t *)vpx_codec_get_frame(decoder, &iter);

		if (!(*img) || corrupted) {
			switch_buffer_zero(context->vpx_packet_buffer);
			goto ok;
		}

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%dx%d %dx%d\n", (*img)->w,(*img)->h, (*img)->d_w, (*img)->d_h);

		switch_buffer_zero(context->vpx_packet_buffer);
	}

ok:
	return SWITCH_STATUS_SUCCESS;

error:
	switch_buffer_zero(context->vpx_packet_buffer);

	return SWITCH_STATUS_FALSE;
}

static switch_status_t switch_vpx_destroy(switch_codec_t *codec)
{
	vpx_context_t *context = (vpx_context_t *)codec->private_info;

	if (context) {
		if ((codec->flags & SWITCH_CODEC_FLAG_ENCODE)) {
			vpx_codec_destroy(&context->encoder); // TODO fix crash
		}

		if ((codec->flags & SWITCH_CODEC_FLAG_DECODE)) {
			vpx_codec_destroy(&context->decoder);
		}

		if (context->pic) {
			vpx_img_free(context->pic);
			context->pic = NULL;
		}
		if (context->vpx_packet_buffer) {
			switch_buffer_destroy(&context->vpx_packet_buffer);
			context->vpx_packet_buffer = NULL;
		}
	}
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_vpx_load)
{
	switch_codec_interface_t *codec_interface;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	SWITCH_ADD_CODEC(codec_interface, "VP8 Video");
	switch_core_codec_add_video_implementation(pool, codec_interface, 99, "VP8", NULL,
		switch_vpx_init, switch_vpx_encode, switch_vpx_decode, switch_vpx_destroy);

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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
