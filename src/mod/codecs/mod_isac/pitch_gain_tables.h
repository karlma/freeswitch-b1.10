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
 * pitch_gain_tables.h
 *
 * This file contains tables for the pitch filter side-info in the entropy coder.
 *
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_PITCH_GAIN_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_PITCH_GAIN_TABLES_H_

#include "typedefs.h"

/* header file for coding tables for the pitch filter side-info in the entropy coder */
/********************* Pitch Filter Gain Coefficient Tables ************************/
/* cdf for quantized pitch filter gains */
extern const WebRtc_UWord16 WebRtcIsac_kQPitchGainCdf[255];

/* index limits and ranges */
extern const WebRtc_Word16 WebRtcIsac_kIndexLowerLimitGain[3];

extern const WebRtc_Word16 WebRtcIsac_kIndexUpperLimitGain[3];
extern const WebRtc_UWord16 WebRtcIsac_kIndexMultsGain[2];

/* mean values of pitch filter gains */
//(Y)
extern const WebRtc_Word16 WebRtcIsac_kQMeanGain1Q12[144];
extern const WebRtc_Word16 WebRtcIsac_kQMeanGain2Q12[144];
extern const WebRtc_Word16 WebRtcIsac_kQMeanGain3Q12[144];
extern const WebRtc_Word16 WebRtcIsac_kQMeanGain4Q12[144];
//(Y)

/* size of cdf table */
extern const WebRtc_UWord16 WebRtcIsac_kQCdfTableSizeGain[1];

#endif /* WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_PITCH_GAIN_TABLES_H_ */
