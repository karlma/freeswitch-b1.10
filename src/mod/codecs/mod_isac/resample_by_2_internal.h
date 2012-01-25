/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
 * This header file contains some internal resampling functions.
 *
 */

#ifndef WEBRTC_SPL_RESAMPLE_BY_2_INTERNAL_H_
#define WEBRTC_SPL_RESAMPLE_BY_2_INTERNAL_H_

#include "typedefs.h"

/*******************************************************************
 * resample_by_2_fast.c
 * Functions for internal use in the other resample functions
 ******************************************************************/
void WebRtcSpl_DownBy2IntToShort(WebRtc_Word32 *in, WebRtc_Word32 len, WebRtc_Word16 *out,
                                 WebRtc_Word32 *state);

void WebRtcSpl_DownBy2ShortToInt(const WebRtc_Word16 *in, WebRtc_Word32 len,
                                 WebRtc_Word32 *out, WebRtc_Word32 *state);

void WebRtcSpl_UpBy2ShortToInt(const WebRtc_Word16 *in, WebRtc_Word32 len,
                               WebRtc_Word32 *out, WebRtc_Word32 *state);

void WebRtcSpl_UpBy2IntToInt(const WebRtc_Word32 *in, WebRtc_Word32 len, WebRtc_Word32 *out,
                             WebRtc_Word32 *state);

void WebRtcSpl_UpBy2IntToShort(const WebRtc_Word32 *in, WebRtc_Word32 len,
                               WebRtc_Word16 *out, WebRtc_Word32 *state);

void WebRtcSpl_LPBy2ShortToInt(const WebRtc_Word16* in, WebRtc_Word32 len,
                               WebRtc_Word32* out, WebRtc_Word32* state);

void WebRtcSpl_LPBy2IntToInt(const WebRtc_Word32* in, WebRtc_Word32 len, WebRtc_Word32* out,
                             WebRtc_Word32* state);

#endif // WEBRTC_SPL_RESAMPLE_BY_2_INTERNAL_H_
