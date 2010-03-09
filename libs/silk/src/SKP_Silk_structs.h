

/***********************************************************************

Copyright (c) 2006-2010, Skype Limited. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, (subject to the limitations in the disclaimer below)
are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.





- Neither the name of Skype Limited, nor the names of specific
contributors, may be used to endorse or promote products derived from
this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED
BY THIS LICENSE.THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifndef SKP_SILK_STRUCTS_H
#define SKP_SILK_STRUCTS_H

#include "SKP_Silk_typedef.h"
#include "SKP_Silk_SigProc_FIX.h"
#include "SKP_Silk_define.h"

#ifdef __cplusplus
extern "C"
{
#endif


/************************************/
/* Noise shaping quantization state */
/************************************/
typedef struct {
    SKP_int16   xq[           2 * MAX_FRAME_LENGTH ]; /* Buffer for quantized output signal */
    SKP_int32   sLTP_shp_Q10[ 2 * MAX_FRAME_LENGTH ];
    SKP_int32   sLPC_Q14[ MAX_FRAME_LENGTH / NB_SUBFR + MAX_LPC_ORDER ];
    SKP_int32   sLF_AR_shp_Q12;
    SKP_int     lagPrev;
    SKP_int     sLTP_buf_idx;
    SKP_int     sLTP_shp_buf_idx;
    SKP_int32   rand_seed;
    SKP_int32   prev_inv_gain_Q16;
    SKP_int     rewhite_flag;
} SKP_Silk_nsq_state; /* FIX*/

/* Struct for Low BitRate Redundant (LBRR) information */





typedef struct {
    SKP_uint8   payload[ MAX_ARITHM_BYTES ];
    SKP_int     nBytes;                         /* Number of bytes in payload                               */
    SKP_int     usage;                          /* Tells how the payload should be used as FEC              */
} SKP_SILK_LBRR_struct;

/********************************/
/* VAD state                    */
/********************************/
typedef struct {
    SKP_int32   AnaState[ 2 ];                  /* Analysis filterbank state: 0-8 kHz                       */
    SKP_int32   AnaState1[ 2 ];                 /* Analysis filterbank state: 0-4 kHz                       */
    SKP_int32   AnaState2[ 2 ];                 /* Analysis filterbank state: 0-2 kHz                       */
    SKP_int32   XnrgSubfr[ VAD_N_BANDS ];       /* Subframe energies                                        */
    SKP_int32   NrgRatioSmth_Q8[ VAD_N_BANDS ]; /* Smoothed energy level in each band                       */
    SKP_int16   HPstate;                        /* State of differentiator in the lowest band               */
    SKP_int32   NL[ VAD_N_BANDS ];              /* Noise energy level in each band                          */
    SKP_int32   inv_NL[ VAD_N_BANDS ];          /* Inverse noise energy level in each band                  */
    SKP_int32   NoiseLevelBias[ VAD_N_BANDS ];  /* Noise level estimator bias/offset                        */
    SKP_int32   counter;                        /* Frame counter used in the initial phase                  */
} SKP_Silk_VAD_state;

/*******************************/
/* Range encoder/decoder state */
/*******************************/
typedef struct {
    SKP_int32   bufferLength;
    SKP_int32   bufferIx;
    SKP_uint32  base_Q32;
    SKP_uint32  range_Q16;
    SKP_int32   error;
    SKP_uint8   buffer[ MAX_ARITHM_BYTES ];     /* Buffer containing payload                                */
} SKP_Silk_range_coder_state;

/* Input frequency range detection struct */
typedef struct {
    SKP_int32                   S_HP_8_kHz[NB_SOS][ 2 ];        /* HP filter State */
    SKP_int32                   ConsecSmplsAboveThres;
    SKP_int32                   ActiveSpeech_ms;            /* Accumulated time with active speech */
    SKP_int                     SWB_detected;               /* Flag to indicate SWB input */
    SKP_int                     WB_detected;                /* Flag to indicate WB input */
} SKP_Silk_detect_SWB_state;

#if SWITCH_TRANSITION_FILTERING
/* Variable cut-off low-pass filter state */
typedef struct {
    SKP_int32                   In_LP_State[ 2 ];               /* Low pass filter state */
    SKP_int32                   transition_frame_no;        /* Counter which is mapped to a cut-off frequency */





    SKP_int                     mode;                       /* Operating mode, 0: switch down, 1: switch up */
} SKP_Silk_LP_state;
#endif

/* structure for one stage of MSVQ */
typedef struct {
    const SKP_int32                         nVectors;
    const SKP_int16                         *CB_NLSF_Q15;
    const SKP_int16                         *Rates_Q5;
} SKP_Silk_NLSF_CB_Stage_struct;

/* Structure containing NLSF MSVQ codebook */
typedef struct {
    const SKP_int32                         nStages;

    /* Fields for (de)quantizing */
    const SKP_Silk_NLSF_CB_Stage_struct     *CBStages;
    const SKP_int                           *NDeltaMin_Q15;

    /* Fields for arithmetic (de)coding */
    const SKP_uint16                        *CDF;
    const SKP_uint16 * const                *StartPtr;
    const SKP_int                           *MiddleIx;
} SKP_Silk_NLSF_CB_struct;

/********************************/
/* Encoder state                */
/********************************/
typedef struct {
    SKP_Silk_range_coder_state      sRC;                            /* Range coder state                                                    */
    SKP_Silk_range_coder_state      sRC_LBRR;                       /* Range coder state (for low bitrate redundancy)                       */
#if HIGH_PASS_INPUT
    SKP_int32                       In_HP_State[ 2 ];               /* High pass filter state                                               */
#endif
#if SWITCH_TRANSITION_FILTERING
    SKP_Silk_LP_state               sLP;                            /* Low pass filter state */
#endif
    SKP_Silk_VAD_state              sVAD;                           /* Voice activity detector state                                        */

    SKP_int                         LBRRprevLastGainIndex;
    SKP_int                         prev_sigtype;
    SKP_int                         typeOffsetPrev;                 /* Previous signal type and quantization offset                         */
    SKP_int                         prevLag;
    SKP_int                         prev_lagIndex;
    SKP_int                         fs_kHz;                         /* Sampling frequency (kHz)                                             */
    SKP_int                         fs_kHz_changed;                 /* Did we switch yet?                                                   */
    SKP_int                         frame_length;                   /* Frame length (samples)                                               */
    SKP_int                         subfr_length;                   /* Subframe length (samples)                                            */





    SKP_int                         la_pitch;                       /* Look-ahead for pitch analysis (samples)                              */
    SKP_int                         la_shape;                       /* Look-ahead for noise shape analysis (samples)                        */
    SKP_int32                       TargetRate_bps;                 /* Target bitrate (bps)                                                 */
    SKP_int                         PacketSize_ms;                  /* Number of milliseconds to put in each packet                         */
    SKP_int                         PacketLoss_perc;                /* Packet loss rate measured by farend                                  */
    SKP_int32                       frameCounter;
    SKP_int                         Complexity;                     /* Complexity setting: 0-> low; 1-> medium; 2->high                     */
    SKP_int                         nStatesDelayedDecision;         /* Number of states in delayed decision quantization                    */
    SKP_int                         useInterpolatedNLSFs;           /* Flag for using NLSF interpolation                                    */
    SKP_int                         shapingLPCOrder;                /* Filter order for noise shaping filters                               */
    SKP_int                         predictLPCOrder;                /* Filter order for prediction filters                                  */
    SKP_int                         pitchEstimationComplexity;      /* Complexity level for pitch estimator                                 */
    SKP_int                         pitchEstimationLPCOrder;        /* Whitening filter order for pitch estimator                           */
    SKP_int                         LTPQuantLowComplexity;          /* Flag for low complexity LTP quantization                             */
    SKP_int                         NLSF_MSVQ_Survivors;            /* Number of survivors in NLSF MSVQ                                     */
    SKP_int                         first_frame_after_reset;        /* Flag for deactivating NLSF interp. and fluc. reduction after resets  */

    /* Input/output buffering */
    SKP_int16                       inputBuf[ MAX_FRAME_LENGTH ];   /* buffer containin input signal                                        */
    SKP_int                         inputBufIx;
    SKP_int                         nFramesInPayloadBuf;            /* number of frames sitting in outputBuf                                */
    SKP_int                         nBytesInPayloadBuf;             /* number of bytes sitting in outputBuf                                 */

    /* Parameters For LTP scaling Control */
    SKP_int                         frames_since_onset;

    const SKP_Silk_NLSF_CB_struct   *psNLSF_CB[ 2 ];                /* Pointers to voiced/unvoiced NLSF codebooks */

    /* Struct for Inband LBRR */
    SKP_SILK_LBRR_struct            LBRR_buffer[ MAX_LBRR_DELAY ];
    SKP_int                         oldest_LBRR_idx;
    SKP_int                         LBRR_enabled;
    SKP_int                         LBRR_GainIncreases;             /* Number of shifts to Gains to get LBRR rate Voiced frames             */

    /* Bitrate control */
    SKP_int32                       bitrateDiff;                    /* accumulated diff. between the target bitrate and the SWB/WB limits   */

#if LOW_COMPLEXITY_ONLY
    /* state for downsampling from 24 to 16 kHz in low complexity mode */
    SKP_int16                       resample24To16state[ SigProc_Resample_2_3_coarse_NUM_FIR_COEFS - 1 ];
#else
    SKP_int32                       resample24To16state[ 11 ];      /* state for downsampling from 24 to 16 kHz                             */
#endif
    SKP_int32                       resample24To12state[ 6 ];       /* state for downsampling from 24 to 12 kHz                             */
    SKP_int32                       resample24To8state[ 7 ];        /* state for downsampling from 24 to  8 kHz                             */
    SKP_int32                       resample16To12state[ 15 ];      /* state for downsampling from 16 to 12 kHz                             */
    SKP_int32                       resample16To8state[ 6 ];        /* state for downsampling from 16 to  8 kHz                             */
#if LOW_COMPLEXITY_ONLY





    /* state for downsampling from 12 to 8 kHz in low complexity mode */
    SKP_int16                       resample12To8state[ SigProc_Resample_2_3_coarse_NUM_FIR_COEFS - 1 ];
#else
    SKP_int32                       resample12To8state[ 11 ];       /* state for downsampling from 12 to  8 kHz                             */
#endif

    /* DTX */
    SKP_int                         noSpeechCounter;                /* Counts concecutive nonactive frames, used by DTX                     */
    SKP_int                         useDTX;                         /* Flag to enable DTX                                                   */
    SKP_int                         inDTX;                          /* Flag to signal DTX period                                            */
    SKP_int                         vadFlag;                        /* Flag to indicate Voice Activity                                      */

    /* Struct for detecting SWB input */
    SKP_Silk_detect_SWB_state       sSWBdetect;


    /********************************************/
    /* Buffers etc used by the new bitstream V4 */
    /********************************************/
    SKP_int                         q[ MAX_FRAME_LENGTH * MAX_FRAMES_PER_PACKET ];      /* pulse signal buffer */
    SKP_int                         q_LBRR[ MAX_FRAME_LENGTH * MAX_FRAMES_PER_PACKET ]; /* pulse signal buffer */
    SKP_int                         sigtype[ MAX_FRAMES_PER_PACKET ];
    SKP_int                         QuantOffsetType[ MAX_FRAMES_PER_PACKET ];
    SKP_int                         extension_layer;                                    /* Add extension layer      */
    SKP_int                         bitstream_v;                                        /* Holds bitstream version  */
} SKP_Silk_encoder_state;


/************************/
/* Encoder control      */
/************************/
typedef struct {
    /* Quantization indices */
    SKP_int     lagIndex;
    SKP_int     contourIndex;
    SKP_int     PERIndex;
    SKP_int     LTPIndex[ NB_SUBFR ];
    SKP_int     NLSFIndices[ NLSF_MSVQ_MAX_CB_STAGES ];  /* NLSF path of quantized LSF vector   */
    SKP_int     NLSFInterpCoef_Q2;
    SKP_int     GainsIndices[ NB_SUBFR ];
    SKP_int32   Seed;
    SKP_int     LTP_scaleIndex;
    SKP_int     RateLevelIndex;
    SKP_int     QuantOffsetType;
    SKP_int     sigtype;

    /* Prediction and coding parameters */
    SKP_int     pitchL[ NB_SUBFR ];





    SKP_int     LBRR_usage;                     /* Low bitrate redundancy usage                             */
} SKP_Silk_encoder_control;

/* Struct for Packet Loss Concealment */
typedef struct {
    SKP_int32   pitchL_Q8;                      /* Pitch lag to use for voiced concealment                  */
    SKP_int16   LTPCoef_Q14[ LTP_ORDER ];       /* LTP coeficients to use for voiced concealment            */
    SKP_int16   prevLPC_Q12[ MAX_LPC_ORDER ];
    SKP_int     last_frame_lost;                /* Was previous frame lost                                  */
    SKP_int32   rand_seed;                      /* Seed for unvoiced signal generation                      */
    SKP_int16   randScale_Q14;                  /* Scaling of unvoiced random signal                        */
    SKP_int32   conc_energy;
    SKP_int     conc_energy_shift;
    SKP_int16   prevLTP_scale_Q14;
    SKP_int32   prevGain_Q16[ NB_SUBFR ];
    SKP_int     fs_kHz;
} SKP_Silk_PLC_struct;

/* Struct for CNG */
typedef struct {
    SKP_int32   CNG_exc_buf_Q10[ MAX_FRAME_LENGTH ];
    SKP_int     CNG_smth_NLSF_Q15[ MAX_LPC_ORDER ];
    SKP_int32   CNG_synth_state[ MAX_LPC_ORDER ];
    SKP_int32   CNG_smth_Gain_Q16;
    SKP_int32   rand_seed;
    SKP_int     fs_kHz;
} SKP_Silk_CNG_struct;

/********************************/
/* Decoder state                */
/********************************/
typedef struct {
    SKP_Silk_range_coder_state  sRC;                            /* Range coder state                                                    */
    SKP_int32       prev_inv_gain_Q16;
    SKP_int32       sLTP_Q16[ 2 * MAX_FRAME_LENGTH ];
    SKP_int32       sLPC_Q14[ MAX_FRAME_LENGTH / NB_SUBFR + MAX_LPC_ORDER ];
    SKP_int32       exc_Q10[ MAX_FRAME_LENGTH ];
    SKP_int32       res_Q10[ MAX_FRAME_LENGTH ];
    SKP_int16       outBuf[ 2 * MAX_FRAME_LENGTH ];             /* Buffer for output signal                                             */
    SKP_int         sLTP_buf_idx;                               /* LTP_buf_index                                                        */
    SKP_int         lagPrev;                                    /* Previous Lag                                                         */
    SKP_int         LastGainIndex;                              /* Previous gain index                                                  */
    SKP_int         LastGainIndex_EnhLayer;                     /* Previous gain index                                                  */
    SKP_int         typeOffsetPrev;                             /* Previous signal type and quantization offset                         */
    SKP_int32       HPState[ DEC_HP_ORDER ];                    /* HP filter state                                                      */
    const SKP_int16 *HP_A;                                      /* HP filter AR coefficients                                            */
    const SKP_int16 *HP_B;                                      /* HP filter MA coefficients                                            */
    SKP_int         fs_kHz;                                     /* Sampling frequency in kHz                                            */





    SKP_int         frame_length;                               /* Frame length (samples)                                               */
    SKP_int         subfr_length;                               /* Subframe length (samples)                                            */
    SKP_int         LPC_order;                                  /* LPC order                                                            */
    SKP_int         prevNLSF_Q15[ MAX_LPC_ORDER ];              /* Used to interpolate LSFs                                             */
    SKP_int         first_frame_after_reset;                    /* Flag for deactivating NLSF interp. and fluc. reduction after resets  */

    /* For buffering payload in case of more frames per packet */
    SKP_int         nBytesLeft;
    SKP_int         nFramesDecoded;
    SKP_int         nFramesInPacket;
    SKP_int         InternalDecoderFrames;
    SKP_int         FrameTermination;

    SKP_int32       resampleState[ 15 ];

    const SKP_Silk_NLSF_CB_struct *psNLSF_CB[ 2 ];      /* Pointers to voiced/unvoiced NLSF codebooks */

    SKP_int         sigtype[               MAX_FRAMES_PER_PACKET ];
    SKP_int         QuantOffsetType[       MAX_FRAMES_PER_PACKET ];
    SKP_int         GainsIndices[          MAX_FRAMES_PER_PACKET ][ NB_SUBFR ];
    SKP_int         GainsIndices_EnhLayer[ MAX_FRAMES_PER_PACKET ][ NB_SUBFR ];
    SKP_int         NLSFIndices[           MAX_FRAMES_PER_PACKET ][ NLSF_MSVQ_MAX_CB_STAGES ];
    SKP_int         NLSFInterpCoef_Q2[     MAX_FRAMES_PER_PACKET ];
    SKP_int         lagIndex[              MAX_FRAMES_PER_PACKET ];
    SKP_int         contourIndex[          MAX_FRAMES_PER_PACKET ];
    SKP_int         PERIndex[              MAX_FRAMES_PER_PACKET ];
    SKP_int         LTPIndex[              MAX_FRAMES_PER_PACKET ][ NB_SUBFR ];
    SKP_int         LTP_scaleIndex[        MAX_FRAMES_PER_PACKET ];
    SKP_int         Seed[                  MAX_FRAMES_PER_PACKET ];
    SKP_int         vadFlagBuf[            MAX_FRAMES_PER_PACKET ];

    /* Parameters used to investigate if inband FEC is used */
    SKP_int         vadFlag;
    SKP_int         no_fec_counter;                             /* Counts number of frames wo inband FEC                                */
    SKP_int         inband_fec_offset;                          /* 0: no FEC, 1: FEC with 1 packet offset, 2: FEC w 2 packets offset    */

    /* CNG state */
    SKP_Silk_CNG_struct sCNG;

    /* Stuff used for PLC */
    SKP_Silk_PLC_struct sPLC;
    SKP_int         lossCnt;
    SKP_int         prev_sigtype;                               /* Previous sigtype                                                     */
#ifdef USE_INTERPOLATION_PLC
    SKP_int16       prevQuant[ MAX_FRAME_LENGTH ];
    SKP_int         prevPitchL[ NB_SUBFR ];                     /* Previous Lags used                                                   */
    SKP_int16       prevLTPCoef_Q14[ NB_SUBFR * LTP_ORDER ];    /* Previous LTCoefs used                                                */
    SKP_int16       prevAR_Q12[ MAX_LPC_ORDER ];





    SKP_int         interpolDistance;                           /* Number of frames between old and new recieved packet                 */
#endif




    SKP_int                 bitstream_v;                        /* Holds bitstream version                                              */
} SKP_Silk_decoder_state;

/************************/
/* Decoder control      */
/************************/
typedef struct {
    /* prediction and coding parameters */
    SKP_int             pitchL[ NB_SUBFR ];
    SKP_int32           Gains_Q16[ NB_SUBFR ];
    SKP_int32           Seed;
    /* holds interpolated and final coefficients, 4-byte aligned */
    SKP_array_of_int16_4_byte_aligned( PredCoef_Q12[ 2 ], MAX_LPC_ORDER );
    SKP_int16           LTPCoef_Q14[ LTP_ORDER * NB_SUBFR ];
    SKP_int             LTP_scale_Q14;

    /* quantization indices */
    SKP_int             PERIndex;
    SKP_int             RateLevelIndex;
    SKP_int             QuantOffsetType;
    SKP_int             sigtype;
    SKP_int             NLSFInterpCoef_Q2;
} SKP_Silk_decoder_control;

#ifdef __cplusplus
}
#endif

#endif


