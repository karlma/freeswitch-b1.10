/*
 * Copyright 2008 Arsen Chaloyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPF_ACTIVITY_DETECTOR_H__
#define __MPF_ACTIVITY_DETECTOR_H__

/**
 * @file mpf_activity_detector.h
 * @brief MPF Voice Activity Detector
 */ 

#include "mpf_frame.h"
#include "mpf_codec.h"

APT_BEGIN_EXTERN_C

/** Opaque (voice) activity detector */
typedef struct mpf_activity_detector_t mpf_activity_detector_t;

/** Events of activity detector */
typedef enum {
	MPF_DETECTOR_EVENT_NONE,       /**< no event occurred */
	MPF_DETECTOR_EVENT_ACTIVITY,   /**< voice activity (transition to activity from inactivity state) */
	MPF_DETECTOR_EVENT_INACTIVITY, /**< voice inactivity (transition to inactivity from activity state) */
	MPF_DETECTOR_EVENT_NOINPUT     /**< noinput event occurred */
} mpf_detector_event_e;


/** Create activity detector */
MPF_DECLARE(mpf_activity_detector_t*) mpf_activity_detector_create(apr_pool_t *pool);

/** Set threshold of voice activity (silence) level */
MPF_DECLARE(void) mpf_activity_detector_level_set(mpf_activity_detector_t *detector, apr_size_t level_threshold);

/** Set noinput timeout */
MPF_DECLARE(void) mpf_activity_detector_noinput_timeout_set(mpf_activity_detector_t *detector, apr_size_t noinput_timeout);

/** Set transition complete timeout */
MPF_DECLARE(void) mpf_activity_detector_complete_timeout_set(mpf_activity_detector_t *detector, apr_size_t complete_timeout);

/** Process current frame, return detected event if any */
MPF_DECLARE(mpf_detector_event_e) mpf_activity_detector_process(mpf_activity_detector_t *detector, const mpf_frame_t *frame);


APT_END_EXTERN_C

#endif /*__MPF_ACTIVITY_DETECTOR_H__*/
