/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2013, Anthony Minessale II <anthm@freeswitch.org>
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
 * Seven Du <dujinfang@gmail.com>
 *
 * mod_av -- FS Video File Format
 *
 */

/* compile: need to remove the -pedantic option from modmake.rules to compile */

#include <switch.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>

/* use libx264 by default, comment out to use the ffmpeg/avcodec wrapper */
#define H264_CODEC_USE_LIBX264
#define SLICE_SIZE SWITCH_DEFAULT_VIDEO_SIZE

#ifdef H264_CODEC_USE_LIBX264
#include <x264.h>
#endif

#define FPS 15 // frame rate

SWITCH_MODULE_LOAD_FUNCTION(mod_av_load);
SWITCH_MODULE_DEFINITION(mod_av, mod_av_load, NULL, NULL);

/*  ff_avc_find_startcode is not exposed in the ffmpeg lib but you can use it
	Either include the avc.h which available in the ffmpeg source, or
	just add the declaration like we does following to avoid include that whole avc.h
	The function is implemented in avc.h, guess we'll get rid of this later if we can directly use libx264

#include <libavformat/avc.h>
*/

const uint8_t *ff_avc_find_startcode(const uint8_t *p, const uint8_t *end);


/* codec interface */

#define H264_NALU_BUFFER_SIZE 65536
#define MAX_NALUS 100

typedef struct our_h264_nalu_s
{
	const uint8_t *start;
	const uint8_t *eat;
	uint32_t len;
} our_h264_nalu_t;

typedef struct h264_codec_context_s {
	switch_buffer_t *nalu_buffer;
	AVCodec *decoder;
	AVCodec *encoder;
	AVCodecContext *decoder_ctx;
	AVCodecContext *encoder_ctx;
	int got_pps; /* if pps packet received */
	int64_t pts;
	int got_encoded_output;
	AVFrame *encoder_avframe;
	AVPacket encoder_avpacket;
	our_h264_nalu_t nalus[MAX_NALUS];
	int nalu_current_index;
	switch_size_t last_received_timestamp;
	switch_bool_t last_received_complete_picture;
	switch_image_t *img;
	int need_key_frame;
	switch_bool_t nalu_28_start;

#ifdef H264_CODEC_USE_LIBX264
	/*x264*/

	x264_t *x264_handle;
	x264_param_t x264_params;
	x264_nal_t *x264_nals;
	int x264_nal_count;
	int cur_nalu_index;
#endif

} h264_codec_context_t;

#ifdef H264_CODEC_USE_LIBX264

static switch_status_t init_x264(h264_codec_context_t *context, uint32_t width, uint32_t height)
{
	x264_t *xh = context->x264_handle;
	x264_param_t *xp = &context->x264_params;
	int ret = 0;

	if (xh) {
		xp->i_width = width;
		xp->i_height = height;
		ret = x264_encoder_reconfig(xh, xp);

		if (ret == 0) return SWITCH_STATUS_SUCCESS;

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Cannot Reset error:%d\n", ret);
		return SWITCH_STATUS_FALSE;
	}

	// x264_param_default(xp);
	x264_param_default_preset(xp, "veryfast", "zerolatency");
	// xp->i_level_idc = 31; // baseline
	// CPU Flags
	// xp->i_threads  = 1;//X264_SYNC_LOOKAHEAD_AUTO;
	// xp->i_lookahead_threads = X264_SYNC_LOOKAHEAD_AUTO;
	// Video Properties
	xp->i_width	  = width;
	xp->i_height  = height;
	xp->i_frame_total = 0;
	xp->i_keyint_max = FPS * 10;
	// Bitstream parameters
	xp->i_bframe	 = 0;
	// xp->i_frame_reference = 0;
	// xp->b_open_gop	= 0;
	// xp->i_bframe_pyramid = 0;
	// xp->i_bframe_adaptive = X264_B_ADAPT_NONE;

	//xp->vui.i_sar_width = 1080;
	//xp->vui.i_sar_height = 720;
	// xp->i_log_level	 = X264_LOG_DEBUG;
	xp->i_log_level	 = X264_LOG_NONE;
	// Rate control Parameters
	xp->rc.i_bitrate = 378;//kbps
	// Muxing parameters
	xp->i_fps_den  = 1;
	xp->i_fps_num  = FPS;
	xp->i_timebase_den = xp->i_fps_num;
	xp->i_timebase_num = xp->i_fps_den;
	xp->i_slice_max_size = SLICE_SIZE;
	//Set Profile 0=baseline other than 1=MainProfile
	x264_param_apply_profile(xp, x264_profile_names[0]);
	xh = x264_encoder_open(xp);

	if (!xh) return SWITCH_STATUS_FALSE;

	// copy params back to xp;
	x264_encoder_parameters(xh, xp);
	context->x264_handle = xh;
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t nalu_slice(h264_codec_context_t *context, switch_frame_t *frame)
{
	int nalu_len;
	uint8_t *buffer;
	int start_code_len = 3;
	x264_nal_t *nal = &context->x264_nals[context->cur_nalu_index];
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	frame->m = 0;

	if (context->cur_nalu_index >= context->x264_nal_count) {
		frame->datalen = 0;
		frame->m = 0;
		context->cur_nalu_index = 0;
		return SWITCH_STATUS_NOTFOUND;
	}

	if (nal->b_long_startcode) start_code_len++;

	nalu_len = nal->i_payload - start_code_len;
	buffer = nal->p_payload + start_code_len;

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "nalu:%d/%d nalu_len:%d\n",
		// context->cur_nalu_index, context->x264_nal_count, nalu_len);

	switch_assert(nalu_len > 0);

	// if ((*buffer & 0x1f) == 7) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Got SPS\n");

	memcpy(frame->data, buffer, nalu_len);
	frame->datalen = nalu_len;
	if (context->cur_nalu_index == context->x264_nal_count - 1) {
		frame->m = 1;
	} else {
		status = SWITCH_STATUS_MORE_DATA;
	}

	context->cur_nalu_index++;

	return status;
}

#endif

static uint8_t ff_input_buffer_padding[FF_INPUT_BUFFER_PADDING_SIZE] = { 0 };

static void buffer_h264_nalu(h264_codec_context_t *context, switch_frame_t *frame)
{
	uint8_t nalu_type = 0;
	uint8_t *data = frame->data;
	uint8_t nalu_hdr = *data;
	uint8_t sync_bytes[] = {0, 0, 0, 1};
	switch_buffer_t *buffer = context->nalu_buffer;

	nalu_type = nalu_hdr & 0x1f;

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "nalu=%02x mark=%d seq=%d ts=%" SWITCH_SIZE_T_FMT " len=%d\n", nalu_hdr, frame->m, frame->seq, frame->timestamp, frame->datalen);

	if (!context->got_pps && nalu_type != 7) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "waiting pps\n");
		switch_set_flag(frame, SFF_WAIT_KEY_FRAME);
		return;
	}

	if (!context->got_pps) context->got_pps = 1;

	/* hack for phones sending sps/pps with frame->m = 1 such as grandstream */
	if ((nalu_type == 7 || nalu_type == 8) && frame->m) frame->m = SWITCH_FALSE;

	if (nalu_type == 28) { // 0x1c FU-A
		int start = *(data + 1) & 0x80;
		int end = *(data + 1) & 0x40;

		nalu_type = *(data + 1) & 0x1f;

		if (start && end) return;

		if (start) {
			if (context->nalu_28_start) {
				context->nalu_28_start = 0;
				switch_buffer_zero(buffer);
			}
		} else if (end) {
			context->nalu_28_start = 0;
		} else if (!context->nalu_28_start) {
			return;
		}

		if (start) {
			uint8_t nalu_idc = (nalu_hdr & 0x60) >> 5;
			nalu_type |= (nalu_idc << 5);

			switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
			switch_buffer_write(buffer, &nalu_type, 1);
			context->nalu_28_start = 1;
		}

		switch_buffer_write(buffer, (void *)(data + 2), frame->datalen - 2);
	} else {
		switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
		switch_buffer_write(buffer, frame->data, frame->datalen);
		context->nalu_28_start = 0;
	}

	if (frame->m) {
		switch_buffer_write(buffer, ff_input_buffer_padding, sizeof(ff_input_buffer_padding));
		context->nalu_28_start = 0;
	}
}

#ifndef H264_CODEC_USE_LIBX264

static switch_status_t consume_nalu(h264_codec_context_t *context, switch_frame_t *frame)
{
	our_h264_nalu_t *nalu = &context->nalus[context->nalu_current_index];

	if (!nalu->start) {
		frame->datalen = 0;
		frame->m = 1;
		if (context->encoder_avpacket.size > 0) av_free_packet(&context->encoder_avpacket);
		if (context->encoder_avframe->data) av_freep(&context->encoder_avframe->data[0]);
		context->nalu_current_index = 0;
		return SWITCH_STATUS_SUCCESS;
	}

	assert(nalu->len);

	if (nalu->len <= SLICE_SIZE) {
		uint8_t nalu_hdr = *(uint8_t *)(nalu->start);
		uint8_t nalu_type = nalu_hdr & 0x1f;

		memcpy(frame->data, nalu->start, nalu->len);
		frame->datalen = nalu->len;
		context->nalu_current_index++;
		if (nalu_type == 6 || nalu_type == 7 || nalu_type == 8) {
			frame->m = 0;
			return SWITCH_STATUS_MORE_DATA;
		}

		frame->m = 1;
		return SWITCH_STATUS_SUCCESS;
	} else {
		uint8_t nalu_hdr = *(uint8_t *)(nalu->start);
		uint8_t nri = nalu_hdr & 0x60;
		uint8_t nalu_type = nalu_hdr & 0x1f;
		int left = nalu->len - (nalu->eat - nalu->start);
		uint8_t *p = frame->data;

		if (left <= (1400 - 2)) {
			p[0] = nri | 28; // FU-A
			p[1] = 0x40 | nalu_type;
			memcpy(p+2, nalu->eat, left);
			nalu->eat += left;
			frame->datalen = left + 2;
			frame->m = 1;
			context->nalu_current_index++;
			return SWITCH_STATUS_SUCCESS;
		} else {
			uint8_t start = nalu->start == nalu->eat ? 0x80 : 0;

			p[0] = nri | 28; // FU-A
			p[1] = start | nalu_type;
			if (start) nalu->eat++;
			memcpy(p+2, nalu->eat, 1400 - 2);
			nalu->eat += (1400 - 2);
			frame->datalen = 1400;
			return SWITCH_STATUS_MORE_DATA;
		}
	}
}

static switch_status_t open_encoder(h264_codec_context_t *context, uint32_t width, uint32_t height)
{

	context->encoder_ctx->width = width;
	context->encoder_ctx->height = height;

	if (avcodec_is_open(context->encoder_ctx)) avcodec_close(context->encoder_ctx);

	if (avcodec_open2(context->encoder_ctx, context->encoder, NULL) < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not open codec\n");
		return SWITCH_STATUS_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}
#endif

static switch_status_t switch_h264_init(switch_codec_t *codec, switch_codec_flag_t flags, const switch_codec_settings_t *codec_settings)
{
	int encoding, decoding;

	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!(encoding || decoding)) {
		return SWITCH_STATUS_FALSE;
	} else {
		h264_codec_context_t *context = NULL;
		if (codec->fmtp_in) {
			codec->fmtp_out = switch_core_strdup(codec->memory_pool, codec->fmtp_in);
		}

		context = switch_core_alloc(codec->memory_pool, sizeof(h264_codec_context_t));
		switch_assert(context);
		memset(context, 0, sizeof(*context));

		if (decoding) {
			context->decoder = avcodec_find_decoder(AV_CODEC_ID_H264);

			if (!context->decoder) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find codec AV_CODEC_ID_H264\n");
				goto error;
			}

			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "codec: id=%d %s\n", context->decoder->id, context->decoder->long_name);

			context->decoder_ctx = avcodec_alloc_context3(context->decoder);

			if (avcodec_open2(context->decoder_ctx, context->decoder, NULL) < 0) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error openning codec\n");
				goto error;
			}
		}

		if (encoding) {
			context->encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (!context->encoder) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Cannot find AV_CODEC_ID_H264 decoder\n");
				goto skip;
			}

			context->encoder_ctx = avcodec_alloc_context3(context->encoder);
			if (!context->encoder_ctx) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not allocate video encoder context\n");
				goto error;
			}

			context->encoder_ctx->bit_rate = 400000;
			context->encoder_ctx->width = 352;
			context->encoder_ctx->height = 288;
			/* frames per second */
			context->encoder_ctx->time_base= (AVRational){1, FPS};
			context->encoder_ctx->gop_size = FPS * 3; /* emit one intra frame every 3 seconds */
			context->encoder_ctx->max_b_frames = 0;
			context->encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			context->encoder_ctx->thread_count = switch_core_cpu_count();
			context->encoder_ctx->rtp_payload_size = SLICE_SIZE;
			av_opt_set(context->encoder_ctx->priv_data, "preset", "fast", 0);
		}

skip:

		switch_buffer_create_dynamic(&(context->nalu_buffer), H264_NALU_BUFFER_SIZE, H264_NALU_BUFFER_SIZE * 8, 0);
		codec->private_info = context;

		return SWITCH_STATUS_SUCCESS;
	}

error:
	// todo, do some clean up
	return SWITCH_STATUS_FALSE;
}

static void __attribute__((unused)) fill_avframe(AVFrame *pict, switch_image_t *img)
{
	int i;
	uint8_t *y = img->planes[0];
	uint8_t *u = img->planes[1];
	uint8_t *v = img->planes[2];

	/* Y */
	for (i = 0; i < pict->height; i++) {
		memcpy(&pict->data[0][i * pict->linesize[0]], y + i * img->stride[0], pict->width);
	}

	/* U/V */
	for(i = 0; i < pict->height / 2; i++) {
		memcpy(&pict->data[1][i * pict->linesize[1]], u + i * img->stride[1], pict->width / 2);
		memcpy(&pict->data[2][i * pict->linesize[2]], v + i * img->stride[2], pict->width / 2);
	}

}

#ifndef H264_CODEC_USE_LIBX264

static switch_status_t switch_h264_encode(switch_codec_t *codec, switch_frame_t *frame)
{
	h264_codec_context_t *context = (h264_codec_context_t *)codec->private_info;
	AVCodecContext *avctx= context->encoder_ctx;
	int ret;
	int *got_output = &context->got_encoded_output;
	AVFrame *avframe;
	AVPacket *pkt = &context->encoder_avpacket;
	uint32_t width = 0;
	uint32_t height = 0;
	switch_image_t *img = frame->img;

	switch_assert(frame);
	frame->m = 0;

	if (frame->datalen < SWITCH_DEFAULT_VIDEO_SIZE) return SWITCH_STATUS_FALSE;

	if (frame->flags & SFF_SAME_IMAGE) {
		// read from nalu buffer
		return consume_nalu(context, frame);
	}

	width = img->d_w;
	height = img->d_h;

	if (!avcodec_is_open(avctx)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "initializing encoder %dx%d\n", width, height);
		open_encoder(context, width, height);
	}

	if (avctx->width != width || avctx->height != height) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "picture size changed from %dx%d to %dx%d, reinitializing encoder",
			avctx->width, avctx->height, width, height);
		open_encoder(context, width, height);
	}

	av_init_packet(pkt);
	pkt->data = NULL;      // packet data will be allocated by the encoder
	pkt->size = 0;

	if (!context->encoder_avframe) context->encoder_avframe = avcodec_alloc_frame();

	avframe = context->encoder_avframe;

	if (!avframe) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error allocate frame!\n");
		goto error;
	}

	avframe->format = avctx->pix_fmt;
	avframe->width  = avctx->width;
	avframe->height = avctx->height;

	/* the image can be allocated by any means and av_image_alloc() is
	 * just the most convenient way if av_malloc() is to be used */
	ret = av_image_alloc(avframe->data, avframe->linesize, avctx->width, avctx->height, avctx->pix_fmt, 32);
	if (ret < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not allocate raw picture buffer\n");
		goto error;
	}

	if (*got_output) { // Could be more delayed frames
		ret = avcodec_encode_video2(avctx, pkt, NULL, got_output);
		if (ret < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Encoding Error %d\n", ret);
			goto error;
		}

		if (*got_output) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Encoded frame %llu (size=%5d) nalu_type=0x%x %d\n", context->pts, pkt->size, *((uint8_t *)pkt->data +4), *got_output);
			goto process;
		}
	}

	fill_avframe(avframe, img);

	avframe->pts = context->pts++;

	/* encode the image */
	ret = avcodec_encode_video2(avctx, pkt, avframe, got_output);

	if (ret < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Encoding Error %d\n", ret);
		goto error;
	}

process:

	if (*got_output) {
		const uint8_t *p = pkt->data;
		int i = 0;

		/* split into nalus */
		memset(context->nalus, 0, sizeof(context->nalus));

		while ((p = ff_avc_find_startcode(p, pkt->data+pkt->size)) < (pkt->data + pkt->size)) {
			if (!context->nalus[i].start) {
				while (!(*p++)) ; /* eat the sync bytes, what ever 0 0 1 or 0 0 0 1 */
				context->nalus[i].start = p;
				context->nalus[i].eat = p;
			} else {
				context->nalus[i].len = p - context->nalus[i].start;
				while (!(*p++)) ; /* eat the sync bytes, what ever 0 0 1 or 0 0 0 1 */
				i++;
				context->nalus[i].start = p;
				context->nalus[i].eat = p;
			}
			if (i >= MAX_NALUS - 2) break;
		}

		context->nalus[i].len = p - context->nalus[i].start;
		context->nalu_current_index = 0;
		return consume_nalu(context, frame);
	}

error:
	frame->datalen = 0;
	return SWITCH_STATUS_FALSE;
}

#endif

#ifdef H264_CODEC_USE_LIBX264

static switch_status_t switch_h264_encode(switch_codec_t *codec,
										  switch_frame_t *frame)
{
	h264_codec_context_t *context = (h264_codec_context_t *)codec->private_info;
	uint32_t width = 0;
	uint32_t height = 0;
	x264_picture_t pic = { 0 }, pic_out = { 0 };
	int result;
	switch_image_t *img = frame->img;
	void *encoded_data = frame->data;
	uint32_t *encoded_data_len = &frame->datalen;
	unsigned int *flag = &frame->flags;

	if (*flag & SFF_WAIT_KEY_FRAME) context->need_key_frame = 1;

	//if (*encoded_data_len < SWITCH_DEFAULT_VIDEO_SIZE) return SWITCH_STATUS_FALSE;

	if (!context) return SWITCH_STATUS_FALSE;

	if (!context->x264_handle) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "initializing x264 handle %dx%d\n", width, height);
		init_x264(context, width, height);
	}

	if (frame->flags & SFF_SAME_IMAGE) {
		return nalu_slice(context, frame);
	}

	width = img->d_w;
	height = img->d_h;

	if (context->x264_params.i_width != width || context->x264_params.i_height != height) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "picture size changed from %dx%d to %dx%d, reinitializing encoder\n",
			context->x264_params.i_width, context->x264_params.i_width, width, height);
		init_x264(context, width, height);
	}

	switch_assert(encoded_data);

	x264_picture_init(&pic);
	pic.img.i_csp = X264_CSP_I420;
	pic.img.i_plane = 3;
	pic.img.i_stride[0] = img->stride[0];
	pic.img.i_stride[1] = img->stride[1];
	pic.img.i_stride[2] = img->stride[2];
	pic.img.plane[0] = img->planes[0];
	pic.img.plane[1] = img->planes[1];
	pic.img.plane[2] = img->planes[2];
	// pic.i_pts = (context->pts++);
	// pic.i_pts = (context->pts+=90000/FPS);
	pic.i_pts = 0;

	if (context->need_key_frame) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "H264 KEYFRAME GENERATED\n");
		//pic.i_type = X264_TYPE_IDR;
		pic.i_type = X264_TYPE_KEYFRAME;
		context->need_key_frame = 0;
	}

	result = x264_encoder_encode(context->x264_handle, &context->x264_nals, &context->x264_nal_count, &pic, &pic_out);

	if (result < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "encode error\n");
		goto error;
	}

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "encode result:%d nal_count:%d daylayed: %d, max_delayed: %d\n", result, context->x264_nal_count, x264_encoder_delayed_frames(context->x264_handle), x264_encoder_maximum_delayed_frames(context->x264_handle));

	if (0) { //debug
		int i;
		x264_nal_t *nals = context->x264_nals;

		for (i = 0; i < context->x264_nal_count; i++) {
			// int start_code_len = 3 + nals[i].b_long_startcode;
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "encoded: %d %d %d %d| %d %d %d %x %x %x %x %x %x\n",
				nals[i].i_type, nals[i].i_ref_idc, nals[i].i_payload, nals[i].b_long_startcode, *(nals[i].p_payload), *(nals[i].p_payload + 1), *(nals[i].p_payload + 2), *(nals[i].p_payload+3), *(nals[i].p_payload + 4), *(nals[i].p_payload + 5), *(nals[i].p_payload + 6), *(nals[i].p_payload + 7), *(nals[i].p_payload + 8));
		}
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Got ACL count : %d, Encoder output dts:%ld\n", context->x264_nal_count, (long)pic_out.i_dts);
	}

	context->cur_nalu_index = 0;
	return nalu_slice(context, frame);

error:

	*encoded_data_len = 0;
	return SWITCH_STATUS_NOTFOUND;
}

#endif

static switch_status_t switch_h264_decode(switch_codec_t *codec, switch_frame_t *frame)
{
	h264_codec_context_t *context = (h264_codec_context_t *)codec->private_info;
	AVCodecContext *avctx= context->decoder_ctx;

	switch_assert(frame);

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "len: %d ts: %u mark:%d\n", frame->datalen, ntohl(frame->timestamp), frame->m);

	//if (context->last_received_timestamp && context->last_received_timestamp != frame->timestamp &&
	//	(!frame->m) && (!context->last_received_complete_picture)) {
		// possible packet loss
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Packet Loss, skip privousely received packets\n");
	//	switch_buffer_zero(context->nalu_buffer);
	//}

	context->last_received_timestamp = frame->timestamp;
	context->last_received_complete_picture = frame->m ? SWITCH_TRUE : SWITCH_FALSE;

	buffer_h264_nalu(context, frame);

	if (frame->m) {
		uint32_t size = switch_buffer_inuse(context->nalu_buffer);
		AVPacket pkt = { 0 };
		AVFrame *picture;
		int got_picture = 0;
		int decoded_len;

		if (size > FF_INPUT_BUFFER_PADDING_SIZE) {
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;
			switch_buffer_peek_zerocopy(context->nalu_buffer, (const void **)&pkt.data);

			pkt.size = size - FF_INPUT_BUFFER_PADDING_SIZE;
			picture = av_frame_alloc();
			assert(picture);
			decoded_len = avcodec_decode_video2(avctx, picture, &got_picture, &pkt);

			// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "buffer: %d got pic: %d len: %d [%dx%d]\n", size, got_picture, decoded_len, avctx->width, avctx->height);

			if (got_picture && decoded_len > 0) {
				int width = avctx->width;
				int height = avctx->height;

				if (!context->img) {
					context->img = switch_img_wrap(NULL, SWITCH_IMG_FMT_I420, width, height, 0, picture->data[0]);
					assert(context->img);
				}
				context->img->w = picture->linesize[0];
				context->img->h = picture->linesize[1];
				context->img->d_w = width;
				context->img->d_h = height;
				context->img->planes[0] = picture->data[0];
				context->img->planes[1] = picture->data[1];
				context->img->planes[2] = picture->data[2];
				context->img->stride[0] = picture->linesize[0];
				context->img->stride[1] = picture->linesize[1];
				context->img->stride[2] = picture->linesize[2];

				frame->img = context->img;
			}

			av_frame_free(&picture);
			av_free_packet(&pkt);
		}

		switch_buffer_zero(context->nalu_buffer);
		context->nalu_28_start = 0;
		//switch_set_flag(frame, SFF_USE_VIDEO_TIMESTAMP);
		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_h264_control(switch_codec_t *codec, 
										   switch_codec_control_command_t cmd, 
										   switch_codec_control_type_t ctype,
										   void *cmd_data,
										   switch_codec_control_type_t *rtype,
										   void **ret_data) {



	h264_codec_context_t *context = (h264_codec_context_t *)codec->private_info;

	switch(cmd) {
	case SCC_VIDEO_REFRESH:
		context->need_key_frame = 1;		
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t switch_h264_destroy(switch_codec_t *codec)
{
	h264_codec_context_t *context = (h264_codec_context_t *)codec->private_info;

	if (!context) return SWITCH_STATUS_SUCCESS;

	switch_buffer_destroy(&context->nalu_buffer);
	if (context->decoder_ctx) {
		if (avcodec_is_open(context->decoder_ctx)) avcodec_close(context->decoder_ctx);
		av_free(context->decoder_ctx);
	}

	switch_img_free(&context->img);

	if (context->encoder_ctx) {
		if (avcodec_is_open(context->encoder_ctx)) avcodec_close(context->encoder_ctx);
		av_free(context->encoder_ctx);
	}

	if (context->encoder_avframe) {
		av_frame_free(&context->encoder_avframe);
	}

#ifdef H264_CODEC_USE_LIBX264
	if (context->x264_handle) {
		x264_encoder_close(context->x264_handle);
	}
#endif

	return SWITCH_STATUS_SUCCESS;
}

/* end of codec interface */

/* App interface */

// a wrapper around a single output AVStream
typedef struct OutputStream {
	AVStream *st;
	AVFrame *frame;
	AVFrame *tmp_frame;
	int64_t next_pts;
	struct SwrContext *swr_ctx;
	int width;
	int height;
} OutputStream;

typedef struct record_helper_s {
	switch_mutex_t *mutex;
	AVFormatContext *oc;
	OutputStream *video_st;
	switch_timer_t *timer;
	int in_callback;
} record_helper_t;

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

	printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		   av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
		   av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
		   av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
		   pkt->stream_index);
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	if (0) log_packet(fmt_ctx, pkt);
	return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
static switch_status_t add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
{
	AVCodecContext *c;
	switch_status_t status = SWITCH_STATUS_FALSE;

	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
		return status;
	}

	ost->st = avformat_new_stream(oc, *codec);
	if (!ost->st) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not allocate stream\n");
		return status;
	}
	ost->st->id = oc->nb_streams - 1;
	c = ost->st->codec;
	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "id:%d den:%d num:%d\n", ost->st->id, ost->st->time_base.den, ost->st->time_base.num);

	switch ((*codec)->type) {
	case AVMEDIA_TYPE_AUDIO:
		c->sample_fmt  = (*codec)->sample_fmts ?
			(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		c->bit_rate    = 64000;
		c->sample_rate = 44100; // todo: not hardcodec?
		c->channels    = 1;
		// c->channel_layout = AV_CH_LAYOUT_STEREO;
		c->channel_layout = AV_CH_LAYOUT_MONO; // todo: support stereo
		break;

	case AVMEDIA_TYPE_VIDEO:
		c->codec_id = codec_id;
		c->bit_rate = 400000;
		/* Resolution must be a multiple of two. */
		c->width    = ost->width;
		c->height   = ost->height;
		c->time_base.den = 1000;
		c->time_base.num = 1;
		c->gop_size      = 30; /* emit one intra frame every x frames at most */
		c->pix_fmt       = AV_PIX_FMT_YUV420P;
		c->thread_count  = switch_core_cpu_count();

		if (codec_id == AV_CODEC_ID_VP8) {
			av_set_options_string(c, "quality=realtime", "=", ":");
		}
		break;
	default:
		break;
	}

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	return SWITCH_STATUS_SUCCESS;
}

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture) return NULL;

	picture->format = pix_fmt;
	picture->width  = width;
	picture->height = height;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not allocate frame data.\n");
		return NULL;
	}

	return picture;
}

static switch_status_t open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost)
{
	int ret;
	AVCodecContext *c = ost->st->codec;
	switch_status_t status = SWITCH_STATUS_FALSE;

	/* open the codec */
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not open video codec: %s\n", av_err2str(ret));
		return status;
	}

	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	switch_assert(ost->frame);

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "pix_fmt: %d\n", c->pix_fmt);
	switch_assert(c->pix_fmt == AV_PIX_FMT_YUV420P); // always I420 for NOW

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost)
{
	AVCodecContext *c;
	int ret;
	switch_status_t status = SWITCH_STATUS_FALSE;

	c = ost->st->codec;

	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not open audio codec: %s\n", av_err2str(ret));
		return status;
	}

	ost->frame = av_frame_alloc();
	switch_assert(ost->frame);

	ost->frame->sample_rate    = c->sample_rate;
	ost->frame->format         = AV_SAMPLE_FMT_S16;
	ost->frame->channel_layout = c->channel_layout;

	if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE) {
		ost->frame->nb_samples = 10000;
	} else {
		ost->frame->nb_samples = c->frame_size;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "sample_rate: %d nb_samples: %d\n", ost->frame->sample_rate, ost->frame->nb_samples);

	if (c->sample_fmt != AV_SAMPLE_FMT_S16) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "sample_fmt %d != AV_SAMPLE_FMT_S16, start resampler\n", c->sample_fmt);

		ost->swr_ctx = swr_alloc();

		if (!ost->swr_ctx) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not allocate resampler context\n");
			return status;
		}

		/* set options */
		av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
		av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
		av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
		av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
		av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
		av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

		if ((ret = swr_init(ost->swr_ctx)) < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to initialize the resampling context\n");
			return status;
		}
	}

	ret = av_frame_get_buffer(ost->frame, 0);
	if (ret < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not allocate audio frame.\n");
		return status;
	}

	if (ost->swr_ctx) {
		ost->tmp_frame = av_frame_alloc();
		switch_assert(ost->tmp_frame);

		ost->tmp_frame->sample_rate    = c->sample_rate;
		ost->tmp_frame->format         = c->sample_fmt;
		ost->tmp_frame->channel_layout = c->channel_layout;
		ost->tmp_frame->nb_samples     = ost->frame->nb_samples;

		ret = av_frame_get_buffer(ost->tmp_frame, 0);
		if (ret < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not allocate audio frame.\n");
			return status;
		}
	}

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t video_read_callback(switch_core_session_t *session, switch_frame_t *frame, void *user_data)
{
	record_helper_t *eh = user_data;
	switch_status_t status = SWITCH_STATUS_FALSE;
	AVPacket pkt = { 0 };
	int got_packet;
	int ret;

	eh->in_callback = 1;

	av_init_packet(&pkt);

	if (frame->img) {
		ret = av_frame_make_writable(eh->video_st->frame);
		if (ret < 0) goto end;

		fill_avframe(eh->video_st->frame, frame->img);
		switch_core_timer_sync(eh->timer);

		if (eh->video_st->frame->pts == eh->timer->samplecount) {
			// never use the same pts, or the encoder coughs
			eh->video_st->frame->pts++;
		} else {
			eh->video_st->frame->pts = eh->timer->samplecount;
		}
		// eh->video_st->frame->pts = switch_time_now() / 1000 - eh->video_st->next_pts;
		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "pts: %lld\n", eh->video_st->frame->pts);

		/* encode the image */
		ret = avcodec_encode_video2(eh->video_st->st->codec, &pkt, eh->video_st->frame, &got_packet);

		if (ret < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Encoding Error %d\n", ret);
			goto end;
		}

		if (got_packet) {
			ret = write_frame(eh->oc, &eh->video_st->st->codec->time_base, eh->video_st->st, &pkt);
			av_free_packet(&pkt);
		}

		if (ret < 0) goto end;
	}

	status = SWITCH_STATUS_SUCCESS;
end:
	eh->in_callback = 0;

	return status;
}

static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
	avcodec_close(ost->st->codec);
	av_frame_free(&ost->frame);
	if (ost->tmp_frame) av_frame_free(&ost->tmp_frame);
}

SWITCH_STANDARD_APP(record_av_function)
{
	switch_status_t status;
	switch_frame_t *read_frame;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	record_helper_t eh = { 0 };
	switch_timer_t timer = { 0 };
	switch_mutex_t *mutex = NULL;
	switch_codec_t codec;//, *vid_codec;
	switch_codec_implementation_t read_impl = { 0 };
	switch_dtmf_t dtmf = { 0 };
	switch_buffer_t *buffer = NULL;
	int inuse = 0, bytes = 0;
	switch_vid_params_t vid_params = { 0 };

	OutputStream video_st = { 0 }, audio_st = { 0 };
	AVOutputFormat *fmt;
	AVFormatContext *oc = NULL;
	AVCodec *audio_codec, *video_codec;
	int has_audio = 0, has_video = 0;
	int ret;

	switch_channel_answer(channel);
	switch_core_session_get_read_impl(session, &read_impl);
	switch_core_session_request_video_refresh(session);

	switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, "");

	if (!switch_channel_ready(channel)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_CRIT, "%s not ready.\n", switch_channel_get_name(channel));
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Channel not ready");
		goto done;
	}

	switch_channel_set_flag(channel, CF_VIDEO_DECODED_READ);
	switch_channel_wait_for_flag(channel, CF_VIDEO_READY, SWITCH_TRUE, 10000, NULL);
	switch_core_media_get_vid_params(session, &vid_params);
	switch_channel_set_flag(channel, CF_VIDEO_ECHO);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "video size: %dx%d\n", vid_params.width, vid_params.height);

	if (switch_core_codec_init(&codec,
							   "L16",
							   NULL,
							   // read_impl.samples_per_second,
							   44100, // todo: not hard coded?
							   read_impl.microseconds_per_packet / 1000,
							   1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
							   NULL, switch_core_session_get_pool(session)) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Audio Codec Activation Success\n");
	} else {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Audio Codec Activation Fail\n");
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "Audio codec activation failed");
		goto end;
	}

	switch_buffer_create_dynamic(&buffer, 8192, 65536, 0);

	av_register_all();
	avformat_alloc_output_context2(&oc, NULL, NULL, data);

	if (!oc) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Could not deduce output format from file extension\n");
		goto end;
	}

	fmt = oc->oformat;

	if (fmt->video_codec != AV_CODEC_ID_NONE &&
		switch_channel_test_flag(channel, CF_VIDEO) &&
		vid_params.width > 0 && vid_params.height > 0) {

		char codec_str[256];
		const AVCodecDescriptor *desc;

		desc = avcodec_descriptor_get(fmt->video_codec);

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "use video codec: [%d] %s (%s)\n", fmt->video_codec, desc->name, desc->long_name);

		video_st.width = vid_params.width;
		video_st.height = vid_params.height;
		video_st.next_pts = switch_time_now() / 1000;
		if (add_stream(&video_st, oc, &video_codec, fmt->video_codec) == SWITCH_STATUS_SUCCESS &&
			open_video(oc, video_codec, &video_st) == SWITCH_STATUS_SUCCESS) {
			avcodec_string(codec_str, sizeof(codec_str), video_st.st->codec, 1);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "use video codec implementation %s\n", codec_str);
			has_video = 1;
		}
	}

	if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
		if (open_audio(oc, audio_codec, &audio_st) != SWITCH_STATUS_SUCCESS) {
			goto end;
		}

		has_audio = 1;
	}

	av_dump_format(oc, 0, data, 1);

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&oc->pb, data, AVIO_FLAG_WRITE);
		if (ret < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not open '%s': %s\n", data, av_err2str(ret));
			goto end;
		}
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(oc, NULL);
	if (ret < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error occurred when opening output file: %s\n", av_err2str(ret));
		goto end;
	}

	if (has_video) {
		switch_mutex_init(&mutex, SWITCH_MUTEX_NESTED, switch_core_session_get_pool(session));
		eh.mutex = mutex;
		eh.video_st = &video_st;
		eh.oc = oc;
		if (switch_core_timer_init(&timer, "soft", 1, 1, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Timer Activation Fail\n");
			goto end;
		}
		eh.timer = &timer;
		switch_core_session_set_video_read_callback(session, video_read_callback, (void *)&eh);
	}

	switch_core_session_set_read_codec(session, &codec);

	while (switch_channel_ready(channel)) {

		status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_SINGLE_READ, 0);

		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			break;
		}

		switch_ivr_parse_all_events(session);

		//check for dtmf interrupts
		if (switch_channel_has_dtmf(channel)) {
			const char * terminators = switch_channel_get_variable(channel, SWITCH_PLAYBACK_TERMINATORS_VARIABLE);
			switch_channel_dequeue_dtmf(channel, &dtmf);

			if (terminators && !strcasecmp(terminators, "none"))
			{
				terminators = NULL;
			}

			if (terminators && strchr(terminators, dtmf.digit)) {

				char sbuf[2] = {dtmf.digit, '\0'};
				switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, sbuf);
				break;
			}
		}

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}

		if (switch_test_flag(read_frame, SFF_CNG)) {
			continue;
		}

		if (mutex) switch_mutex_lock(mutex);

		switch_buffer_write(buffer, read_frame->data, read_frame->datalen);
		bytes = audio_st.frame->nb_samples * 2 * audio_st.st->codec->channels;
		inuse = switch_buffer_inuse(buffer);

		while (inuse >= bytes) {
			AVPacket pkt = { 0 };
			int got_packet = 0;

			av_init_packet(&pkt);

			// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "inuse: %d samples: %d bytes: %d\n", inuse, audio_st.frame->nb_samples, bytes);

			if (audio_st.swr_ctx) { // need resample
				int sample_rate = audio_st.st->codec->sample_rate;
				int dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_st.swr_ctx, sample_rate) + audio_st.frame->nb_samples,
										sample_rate, sample_rate, AV_ROUND_UP);
				switch_assert(dst_nb_samples == audio_st.frame->nb_samples);


				av_frame_make_writable(audio_st.frame);
				av_frame_make_writable(audio_st.tmp_frame);
				switch_buffer_read(buffer, audio_st.frame->data[0], bytes);
				/* convert to destination format */
				ret = swr_convert(audio_st.swr_ctx, audio_st.tmp_frame->data, dst_nb_samples,
						(const uint8_t **)audio_st.frame->data, audio_st.frame->nb_samples);
				if (ret < 0) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error while converting %d samples\n", dst_nb_samples);
					continue;
				}

				audio_st.tmp_frame->pts = audio_st.next_pts;
				audio_st.next_pts  += audio_st.frame->nb_samples;

				ret = avcodec_encode_audio2(audio_st.st->codec, &pkt, audio_st.tmp_frame, &got_packet);
			} else {
				av_frame_make_writable(audio_st.frame);
				switch_buffer_read(buffer, audio_st.frame->data[0], bytes);
				audio_st.frame->pts = audio_st.next_pts;
				audio_st.next_pts  += audio_st.frame->nb_samples;

				ret = avcodec_encode_audio2(audio_st.st->codec, &pkt, audio_st.frame, &got_packet);
			}

			if (ret < 0) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Error encoding audio frame: %s\n", av_err2str(ret));
				continue;
			}

			if (got_packet) {
				// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "got pkt: %d\n", pkt.size);
				if (1) ret = write_frame(oc, &audio_st.st->codec->time_base, audio_st.st, &pkt);
				if (ret < 0) {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error while writing audio frame: %s\n", av_err2str(ret));
					goto end;
				}
			}

			inuse = switch_buffer_inuse(buffer);
		}

		if (mutex) switch_mutex_unlock(mutex);

		switch_core_session_write_frame(session, read_frame, SWITCH_IO_FLAG_NONE, 0);
	}

	switch_core_session_set_video_read_callback(session, NULL, NULL);

	while(eh.in_callback) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "waiting callback func to end\n");
		switch_yield(100000);
	}

	if (has_video) {
		AVPacket pkt = { 0 };
		int got_packet = 0;
		int ret = 0;

	again:
		av_init_packet(&pkt);
		ret = avcodec_encode_video2(video_st.st->codec, &pkt, NULL, &got_packet);

		if (ret < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Encoding Error %d\n", ret);
			goto end;
		}

		if (got_packet) {
			ret = write_frame(oc, &video_st.st->codec->time_base, video_st.st, &pkt);
			av_free_packet(&pkt);
			goto again;
		}
	}

	av_write_trailer(oc);
	switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "OK");

  end:

	if (oc) {
		if (has_video) close_stream(oc, &video_st);
		if (has_audio) close_stream(oc, &audio_st);

		if (!(fmt->flags & AVFMT_NOFILE)) {
			avio_close(oc->pb);
		}
		/* free the stream */
		avformat_free_context(oc);
	}

	if (timer.interval) {
		switch_core_timer_destroy(&timer);
	}

	switch_core_media_end_video_function(session);
	switch_core_session_set_read_codec(session, NULL);
	switch_core_codec_destroy(&codec);

 done:
	switch_core_session_video_reset(session);
}
/* end of App interface */


/* API interface */

static char get_media_type_char(enum AVMediaType type)
{
	switch (type) {
		case AVMEDIA_TYPE_VIDEO:    return 'V';
		case AVMEDIA_TYPE_AUDIO:    return 'A';
		case AVMEDIA_TYPE_DATA:     return 'D';
		case AVMEDIA_TYPE_SUBTITLE: return 'S';
		case AVMEDIA_TYPE_ATTACHMENT:return 'T';
		default:                    return '?';
	}
}

static const AVCodec *next_codec_for_id(enum AVCodecID id, const AVCodec *prev,
										int encoder)
{
	while ((prev = av_codec_next(prev))) {
		if (prev->id == id &&
			(encoder ? av_codec_is_encoder(prev) : av_codec_is_decoder(prev)))
			return prev;
	}
	return NULL;
}

static int compare_codec_desc(const void *a, const void *b)
{
	const AVCodecDescriptor * const *da = a;
	const AVCodecDescriptor * const *db = b;

	return (*da)->type != (*db)->type ? (*da)->type - (*db)->type :
		   strcmp((*da)->name, (*db)->name);
}

static unsigned get_codecs_sorted(const AVCodecDescriptor ***rcodecs)
{
	const AVCodecDescriptor *desc = NULL;
	const AVCodecDescriptor **codecs;
	unsigned nb_codecs = 0, i = 0;

	while ((desc = avcodec_descriptor_next(desc)))
		nb_codecs++;
	if (!(codecs = av_malloc(nb_codecs * sizeof(*codecs)))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "MEM Error!\n");
		return 0;
	}
	desc = NULL;
	while ((desc = avcodec_descriptor_next(desc)))
		codecs[i++] = desc;
	switch_assert(i == nb_codecs);
	qsort(codecs, nb_codecs, sizeof(*codecs), compare_codec_desc);
	*rcodecs = codecs;
	return nb_codecs;
}

static void print_codecs_for_id(switch_stream_handle_t *stream, enum AVCodecID id, int encoder)
{
	const AVCodec *codec = NULL;

	stream->write_function(stream, " (%s: ", encoder ? "encoders" : "decoders");

	while ((codec = next_codec_for_id(id, codec, encoder)))
		stream->write_function(stream, "%s ", codec->name);

	stream->write_function(stream, ")");
}

static int is_device(const AVClass *avclass)
{
#if 0
	if (!avclass) return 0;


	return  avclass->category == AV_CLASS_CATEGORY_DEVICE_VIDEO_OUTPUT ||
			avclass->category == AV_CLASS_CATEGORY_DEVICE_VIDEO_INPUT ||
			avclass->category == AV_CLASS_CATEGORY_DEVICE_AUDIO_OUTPUT ||
			avclass->category == AV_CLASS_CATEGORY_DEVICE_AUDIO_INPUT ||
			avclass->category == AV_CLASS_CATEGORY_DEVICE_OUTPUT ||
			avclass->category == AV_CLASS_CATEGORY_DEVICE_INPUT;
#endif

	return 0;

}

void show_formats(switch_stream_handle_t *stream) {
	AVInputFormat *ifmt  = NULL;
	AVOutputFormat *ofmt = NULL;
	const char *last_name;
	// int is_dev;

	stream->write_function(stream, "============= File Formats ==============================:\n"
		   " D. = Demuxing supported\n"
		   " .M = Muxing supported\n"
		   "----------------------\n");

	last_name = "000";

	for (;;) {
		int decode = 0;
		int encode = 0;
		int is_dev = 0;
		const char *name      = NULL;
		const char *long_name = NULL;

		while ((ofmt = av_oformat_next(ofmt))) {
			is_dev = is_device(ofmt->priv_class);

			if ((name == NULL || strcmp(ofmt->name, name) < 0) &&
				strcmp(ofmt->name, last_name) > 0) {
				name      = ofmt->name;
				long_name = ofmt->long_name;
				encode    = 1;
			}
		}

		while ((ifmt = av_iformat_next(ifmt))) {
			is_dev = is_device(ifmt->priv_class);

			if ((name == NULL || strcmp(ifmt->name, name) < 0) &&
				strcmp(ifmt->name, last_name) > 0) {
				name      = ifmt->name;
				long_name = ifmt->long_name;
				encode    = 0;
			}

			if (name && strcmp(ifmt->name, name) == 0) decode = 1;
		}

		if (name == NULL) break;

		last_name = name;

		stream->write_function(stream, "%s%s%s %-15s %s\n",
			is_dev ? "*" : " ",
			decode ? "D" : " ",
			encode ? "M" : " ",
			name, long_name ? long_name:" ");
	}

}

void show_codecs(switch_stream_handle_t *stream)
{
	const AVCodecDescriptor **codecs = NULL;
	unsigned i, nb_codecs = get_codecs_sorted(&codecs);

	stream->write_function(stream, "================ Codecs ===============================:\n"
		   " V..... = Video\n"
		   " A..... = Audio\n"
		   " S..... = Subtitle\n"
		   " .F.... = Frame-level multithreading\n"
		   " ..S... = Slice-level multithreading\n"
		   " ...X.. = Codec is experimental\n"
		   " ....B. = Supports draw_horiz_band\n"
		   " .....D = Supports direct rendering method 1\n"
		   " ----------------------------------------------\n\n");

	for (i = 0; i < nb_codecs; i++) {
		const AVCodecDescriptor *desc = codecs[i];
		const AVCodec *codec = NULL;

		stream->write_function(stream, " ");
		stream->write_function(stream, avcodec_find_decoder(desc->id) ? "D" : ".");
		stream->write_function(stream, avcodec_find_encoder(desc->id) ? "E" : ".");

		stream->write_function(stream, "%c", get_media_type_char(desc->type));
		stream->write_function(stream, (desc->props & AV_CODEC_PROP_INTRA_ONLY) ? "I" : ".");
		stream->write_function(stream, (desc->props & AV_CODEC_PROP_LOSSY)      ? "L" : ".");
		stream->write_function(stream, (desc->props & AV_CODEC_PROP_LOSSLESS)   ? "S" : ".");

		stream->write_function(stream, " %-20s %s", desc->name, desc->long_name ? desc->long_name : "");

		/* print decoders/encoders when there's more than one or their
		 * names are different from codec name */
		while ((codec = next_codec_for_id(desc->id, codec, 0))) {
			if (strcmp(codec->name, desc->name)) {
				print_codecs_for_id(stream ,desc->id, 0);
				break;
			}
		}
		codec = NULL;
		while ((codec = next_codec_for_id(desc->id, codec, 1))) {
			if (strcmp(codec->name, desc->name)) {
				print_codecs_for_id(stream, desc->id, 1);
				break;
			}
		}

		stream->write_function(stream, "\n");

	}

	av_free(codecs);
}

SWITCH_STANDARD_API(av_api_function)
{
	if (zstr(cmd)) {
		show_codecs(stream);
		stream->write_function(stream, "\n");
		show_formats(stream);
	} else {
		if (!strcmp(cmd, "show formats")) {
			show_formats(stream);
		} else if (!strcmp(cmd, "show codecs")) {
			show_codecs(stream);
		} else {
			stream->write_function(stream, "Usage: ffmpeg show <formats|codecs>");
		}
	}

	return SWITCH_STATUS_SUCCESS;
}

static void log_callback(void *ptr, int level, const char *fmt, va_list vl)
{
	switch_log_level_t switch_level = SWITCH_LOG_DEBUG;

	switch(level) {
		case AV_LOG_QUIET:   switch_level = SWITCH_LOG_CONSOLE; break;
		case AV_LOG_PANIC:   switch_level = SWITCH_LOG_ERROR;   break;
		case AV_LOG_FATAL:   switch_level = SWITCH_LOG_ERROR;   break;
		case AV_LOG_ERROR:   switch_level = SWITCH_LOG_ERROR;   break;
		case AV_LOG_WARNING: switch_level = SWITCH_LOG_WARNING; break;
		case AV_LOG_INFO:    switch_level = SWITCH_LOG_INFO;    break;
		case AV_LOG_VERBOSE: switch_level = SWITCH_LOG_INFO;    break;
		case AV_LOG_DEBUG:   switch_level = SWITCH_LOG_DEBUG;   break;
		default: break;
	}

	// switch_level = SWITCH_LOG_ERROR; // hardcoded for debug
	switch_log_vprintf(SWITCH_CHANNEL_LOG, switch_level, fmt, vl);
}

static char *supported_formats[SWITCH_MAX_CODECS] = { 0 };

SWITCH_MODULE_LOAD_FUNCTION(mod_av_load)
{
	switch_codec_interface_t *codec_interface;
	switch_api_interface_t *api_interface;
	switch_application_interface_t *app_interface;

	supported_formats[0] = "mp4";
	supported_formats[1] = "ts";

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	SWITCH_ADD_CODEC(codec_interface, "H264 Video");
	switch_core_codec_add_video_implementation(pool, codec_interface, 99, "H264", NULL,
											   switch_h264_init, switch_h264_encode, switch_h264_decode, switch_h264_control, switch_h264_destroy);

	SWITCH_ADD_API(api_interface, "av", "av information", av_api_function, "show <formats|codecs>");

	SWITCH_ADD_APP(app_interface, "record_av", "record video using libavformat", "record video using libavformat", record_av_function, "<file>", SAF_NONE);

	av_log_set_callback(log_callback);
	av_log_set_level(AV_LOG_DEBUG);
	av_register_all();

	av_log(NULL, AV_LOG_INFO, "%s %d\n", "av_log callback installed, level=", av_log_get_level());

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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
