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
 * structs.h
 *
 * This header file contains all the structs used in the ISAC codec
 *
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_STRUCTS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_STRUCTS_H_


#include "typedefs.h"
#include "settings.h"
#include "isac.h"

typedef struct Bitstreamstruct {

  WebRtc_UWord8   stream[STREAM_SIZE_MAX];
  WebRtc_UWord32  W_upper;
  WebRtc_UWord32  streamval;
  WebRtc_UWord32  stream_index;

} Bitstr;

typedef struct {

  double    DataBufferLo[WINLEN];
  double    DataBufferHi[WINLEN];

  double    CorrBufLo[ORDERLO+1];
  double    CorrBufHi[ORDERHI+1];

  float    PreStateLoF[ORDERLO+1];
  float    PreStateLoG[ORDERLO+1];
  float    PreStateHiF[ORDERHI+1];
  float    PreStateHiG[ORDERHI+1];
  float    PostStateLoF[ORDERLO+1];
  float    PostStateLoG[ORDERLO+1];
  float    PostStateHiF[ORDERHI+1];
  float    PostStateHiG[ORDERHI+1];

  double    OldEnergy;

} MaskFiltstr;


typedef struct {

  //state vectors for each of the two analysis filters
  double    INSTAT1[2*(QORDER-1)];
  double    INSTAT2[2*(QORDER-1)];
  double    INSTATLA1[2*(QORDER-1)];
  double    INSTATLA2[2*(QORDER-1)];
  double    INLABUF1[QLOOKAHEAD];
  double    INLABUF2[QLOOKAHEAD];

  float    INSTAT1_float[2*(QORDER-1)];
  float    INSTAT2_float[2*(QORDER-1)];
  float    INSTATLA1_float[2*(QORDER-1)];
  float    INSTATLA2_float[2*(QORDER-1)];
  float    INLABUF1_float[QLOOKAHEAD];
  float    INLABUF2_float[QLOOKAHEAD];

  /* High pass filter */
  double    HPstates[HPORDER];
  float    HPstates_float[HPORDER];

} PreFiltBankstr;


typedef struct {

  //state vectors for each of the two analysis filters
  double    STATE_0_LOWER[2*POSTQORDER];
  double    STATE_0_UPPER[2*POSTQORDER];

  /* High pass filter */
  double    HPstates1[HPORDER];
  double    HPstates2[HPORDER];

  float    STATE_0_LOWER_float[2*POSTQORDER];
  float    STATE_0_UPPER_float[2*POSTQORDER];

  float    HPstates1_float[HPORDER];
  float    HPstates2_float[HPORDER];

} PostFiltBankstr;

typedef struct {

  //data buffer for pitch filter
  double    ubuf[PITCH_BUFFSIZE];

  //low pass state vector
  double    ystate[PITCH_DAMPORDER];

  //old lag and gain
  double    oldlagp[1];
  double    oldgainp[1];

} PitchFiltstr;

typedef struct {

  //data buffer
  double    buffer[PITCH_WLPCBUFLEN];

  //state vectors
  double    istate[PITCH_WLPCORDER];
  double    weostate[PITCH_WLPCORDER];
  double    whostate[PITCH_WLPCORDER];

  //LPC window   -> should be a global array because constant
  double    window[PITCH_WLPCWINLEN];

} WeightFiltstr;

typedef struct {

  //for inital estimator
  double         dec_buffer[PITCH_CORR_LEN2 + PITCH_CORR_STEP2 +
                            PITCH_MAX_LAG/2 - PITCH_FRAME_LEN/2+2];
  double        decimator_state[2*ALLPASSSECTIONS+1];
  double        hp_state[2];

  double        whitened_buf[QLOOKAHEAD];

  double        inbuf[QLOOKAHEAD];

  PitchFiltstr  PFstr_wght;
  PitchFiltstr  PFstr;
  WeightFiltstr Wghtstr;

} PitchAnalysisStruct;



/* Have instance of struct together with other iSAC structs */
typedef struct {

  /* Previous frame length (in ms)                                    */
  WebRtc_Word32    prev_frame_length;

  /* Previous RTP timestamp from received
     packet (in samples relative beginning)                           */
  WebRtc_Word32    prev_rec_rtp_number;

  /* Send timestamp for previous packet (in ms using timeGetTime())   */
  WebRtc_UWord32    prev_rec_send_ts;

  /* Arrival time for previous packet (in ms using timeGetTime())     */
  WebRtc_UWord32    prev_rec_arr_ts;

  /* rate of previous packet, derived from RTP timestamps (in bits/s) */
  float   prev_rec_rtp_rate;

  /* Time sinse the last update of the BN estimate (in ms)            */
  WebRtc_UWord32    last_update_ts;

  /* Time sinse the last reduction (in ms)                            */
  WebRtc_UWord32    last_reduction_ts;

  /* How many times the estimate was update in the beginning          */
  WebRtc_Word32    count_tot_updates_rec;

  /* The estimated bottle neck rate from there to here (in bits/s)    */
  WebRtc_Word32  rec_bw;
  float   rec_bw_inv;
  float   rec_bw_avg;
  float   rec_bw_avg_Q;

  /* The estimated mean absolute jitter value,
     as seen on this side (in ms)                                     */
  float   rec_jitter;
  float   rec_jitter_short_term;
  float   rec_jitter_short_term_abs;
  float   rec_max_delay;
  float   rec_max_delay_avg_Q;

  /* (assumed) bitrate for headers (bps)                              */
  float   rec_header_rate;

  /* The estimated bottle neck rate from here to there (in bits/s)    */
  float    send_bw_avg;

  /* The estimated mean absolute jitter value, as seen on
     the other siee (in ms)                                           */
  float   send_max_delay_avg;

  // number of packets received since last update
  int num_pkts_rec;

  int num_consec_rec_pkts_over_30k;

  // flag for marking that a high speed network has been
  // detected downstream
  int hsn_detect_rec;

  int num_consec_snt_pkts_over_30k;

  // flag for marking that a high speed network has
  // been detected upstream
  int hsn_detect_snd;

  WebRtc_UWord32 start_wait_period;

  int in_wait_period;

  int change_to_WB;

  WebRtc_UWord32                 senderTimestamp;
  WebRtc_UWord32                 receiverTimestamp;
  //enum IsacSamplingRate incomingStreamSampFreq;
  WebRtc_UWord16                 numConsecLatePkts;
  float                        consecLatency;
  WebRtc_Word16                  inWaitLatePkts;
} BwEstimatorstr;


typedef struct {

  /* boolean, flags if previous packet exceeded B.N. */
  int    PrevExceed;
  /* ms */
  int    ExceedAgo;
  /* packets left to send in current burst */
  int    BurstCounter;
  /* packets */
  int    InitCounter;
  /* ms remaining in buffer when next packet will be sent */
  double StillBuffered;

} RateModel;


typedef struct {

  unsigned int SpaceAlloced;
  unsigned int MaxPermAlloced;
  double Tmp0[MAXFFTSIZE];
  double Tmp1[MAXFFTSIZE];
  double Tmp2[MAXFFTSIZE];
  double Tmp3[MAXFFTSIZE];
  int Perm[MAXFFTSIZE];
  int factor [NFACTOR];

} FFTstr;


/* The following strutc is used to store data from encoding, to make it
   fast and easy to construct a new bitstream with a different Bandwidth
   estimate. All values (except framelength and minBytes) is double size to
   handle 60 ms of data.
*/
typedef struct {

  /* Used to keep track of if it is first or second part of 60 msec packet */
  int         startIdx;

  /* Frame length in samples */
  WebRtc_Word16 framelength;

  /* Pitch Gain */
  int         pitchGain_index[2];

  /* Pitch Lag */
  double      meanGain[2];
  int         pitchIndex[PITCH_SUBFRAMES*2];

  /* LPC */
  int         LPCmodel[2];
  int         LPCindex_s[108*2]; /* KLT_ORDER_SHAPE = 108 */
  int         LPCindex_g[12*2];  /* KLT_ORDER_GAIN = 12 */
  double      LPCcoeffs_lo[(ORDERLO+1)*SUBFRAMES*2];
  double      LPCcoeffs_hi[(ORDERHI+1)*SUBFRAMES*2];

  /* Encode Spec */
  WebRtc_Word16 fre[FRAMESAMPLES];
  WebRtc_Word16 fim[FRAMESAMPLES];
  WebRtc_Word16 AvgPitchGain[2];

  /* Used in adaptive mode only */
  int         minBytes;

} ISAC_SaveEncData_t;


typedef struct {

  int         indexLPCShape[UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME];
  double      lpcGain[SUBFRAMES<<1];
  int         lpcGainIndex[SUBFRAMES<<1];

  Bitstr      bitStreamObj;

  WebRtc_Word16 realFFT[FRAMESAMPLES_HALF];
  WebRtc_Word16 imagFFT[FRAMESAMPLES_HALF];
} ISACUBSaveEncDataStruct;



typedef struct {

  Bitstr              bitstr_obj;
  MaskFiltstr         maskfiltstr_obj;
  PreFiltBankstr      prefiltbankstr_obj;
  PitchFiltstr        pitchfiltstr_obj;
  PitchAnalysisStruct pitchanalysisstr_obj;
  FFTstr              fftstr_obj;
  ISAC_SaveEncData_t  SaveEnc_obj;

  int                 buffer_index;
  WebRtc_Word16         current_framesamples;

  float               data_buffer_float[FRAMESAMPLES_30ms];

  int                 frame_nb;
  double              bottleneck;
  WebRtc_Word16         new_framelength;
  double              s2nr;

  /* Maximum allowed number of bits for a 30 msec packet */
  WebRtc_Word16         payloadLimitBytes30;
  /* Maximum allowed number of bits for a 30 msec packet */
  WebRtc_Word16         payloadLimitBytes60;
  /* Maximum allowed number of bits for both 30 and 60 msec packet */
  WebRtc_Word16         maxPayloadBytes;
  /* Maximum allowed rate in bytes per 30 msec packet */
  WebRtc_Word16         maxRateInBytes;

  /*---
    If set to 1 iSAC will not addapt the frame-size, if used in
    channel-adaptive mode. The initial value will be used for all rates.
    ---*/
  WebRtc_Word16         enforceFrameSize;

  /*-----
    This records the BWE index the encoder injected into the bit-stream.
    It will be used in RCU. The same BWE index of main paylaod will be in
    the redundant payload. We can not retrive it from BWE because it is
    a recursive procedure (WebRtcIsac_GetDownlinkBwJitIndexImpl) and has to be
    called only once per each encode.
    -----*/
  WebRtc_Word16         lastBWIdx;
} ISACLBEncStruct;

typedef struct {

  Bitstr                  bitstr_obj;
  MaskFiltstr             maskfiltstr_obj;
  PreFiltBankstr          prefiltbankstr_obj;
  FFTstr                  fftstr_obj;
  ISACUBSaveEncDataStruct SaveEnc_obj;

  int                     buffer_index;
  float                   data_buffer_float[MAX_FRAMESAMPLES +
                                            LB_TOTAL_DELAY_SAMPLES];
  double                  bottleneck;
  /* Maximum allowed number of bits for a 30 msec packet */
  //WebRtc_Word16        payloadLimitBytes30;
  /* Maximum allowed number of bits for both 30 and 60 msec packet */
  //WebRtc_Word16        maxPayloadBytes;
  WebRtc_Word16             maxPayloadSizeBytes;

  double                  lastLPCVec[UB_LPC_ORDER];
  WebRtc_Word16             numBytesUsed;
  WebRtc_Word16             lastJitterInfo;
} ISACUBEncStruct;



typedef struct {

  Bitstr          bitstr_obj;
  MaskFiltstr     maskfiltstr_obj;
  PostFiltBankstr postfiltbankstr_obj;
  PitchFiltstr    pitchfiltstr_obj;
  FFTstr          fftstr_obj;

} ISACLBDecStruct;

typedef struct {

  Bitstr          bitstr_obj;
  MaskFiltstr     maskfiltstr_obj;
  PostFiltBankstr postfiltbankstr_obj;
  FFTstr          fftstr_obj;

} ISACUBDecStruct;



typedef struct {

  ISACLBEncStruct ISACencLB_obj;
  ISACLBDecStruct ISACdecLB_obj;
} ISACLBStruct;


typedef struct {

  ISACUBEncStruct ISACencUB_obj;
  ISACUBDecStruct ISACdecUB_obj;
} ISACUBStruct;

/*
  This struct is used to take a snapshot of the entropy coder and LPC gains
  right before encoding LPC gains. This allows us to go back to that state
  if we like to limit the payload size.
*/
typedef struct {
  /* 6 lower-band & 6 upper-band */
  double       loFiltGain[SUBFRAMES];
  double       hiFiltGain[SUBFRAMES];
  /* Upper boundary of interval W */
  WebRtc_UWord32 W_upper;
  WebRtc_UWord32 streamval;
  /* Index to the current position in bytestream */
  WebRtc_UWord32 stream_index;
  WebRtc_UWord8  stream[3];
} transcode_obj;


typedef struct {
  // lower-band codec instance
  ISACLBStruct              instLB;
  // upper-band codec instance
  ISACUBStruct              instUB;

  // Bandwidth Estimator and model for the rate.
  BwEstimatorstr            bwestimator_obj;
  RateModel                 rate_data_obj;
  double                    MaxDelay;

  /* 0 = adaptive; 1 = instantaneous */
  WebRtc_Word16               codingMode;

  // overall bottleneck of the codec
  WebRtc_Word32               bottleneck;

  // QMF Filter state
  WebRtc_Word32               analysisFBState1[FB_STATE_SIZE_WORD32];
  WebRtc_Word32               analysisFBState2[FB_STATE_SIZE_WORD32];
  WebRtc_Word32               synthesisFBState1[FB_STATE_SIZE_WORD32];
  WebRtc_Word32               synthesisFBState2[FB_STATE_SIZE_WORD32];

  // Error Code
  WebRtc_Word16               errorCode;

  // bandwidth of the encoded audio 8, 12 or 16 kHz
  enum ISACBandwidth        bandwidthKHz;
  // Sampling rate of audio, encoder and decode,  8 or 16 kHz
  enum IsacSamplingRate encoderSamplingRateKHz;
  enum IsacSamplingRate decoderSamplingRateKHz;
  // Flag to keep track of initializations, lower & upper-band
  // encoder and decoder.
  WebRtc_Word16               initFlag;

  // Flag to to indicate signal bandwidth switch
  WebRtc_Word16               resetFlag_8kHz;

  // Maximum allowed rate, measured in Bytes per 30 ms.
  WebRtc_Word16               maxRateBytesPer30Ms;
  // Maximum allowed payload-size, measured in Bytes.
  WebRtc_Word16               maxPayloadSizeBytes;
} ISACMainStruct;

#endif /* WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_STRUCTS_H_ */
