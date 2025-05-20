/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_MOTION_DETECTOR_H_
#define _DULT_MOTION_DETECTOR_H_

#include <stdint.h>
#include <stddef.h>

#include "dult.h"

/**
 * @defgroup dult_motion_detector Detecting Unwanted Location Trackers motion detector
 * @brief Private API for DULT specification implementation of the motion detector module
 *
 * Used only when the CONFIG_DULT_MOTION_DETECTOR Kconfig option is enabled.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief DULT motion detector sound callback structure. */
struct dult_motion_detector_sound_cb {
	/** @brief Start sound action.
	 *
	 *  This callback is called to notify the upper layer that the sound action shall be
	 *  started due to the motion detector requirements.
	 */
	void (*sound_start)(void);
};

/** @brief Register DULT motion detector sound callback structure.
 *
 *  @param cb Sound callback structure.
 */
void dult_motion_detector_sound_cb_register(const struct dult_motion_detector_sound_cb *cb);

/** @brief Notify the DULT motion detector module about the sound state change.
 *
 *  @param active True when the play sound action is in progress.
 *                False: otherwise.
 *  @param native True when the play sound action was triggered by this module.
 *                False: otherwise.
 */
void dult_motion_detector_sound_state_change_notify(bool active, bool native);

/** @brief Enable DULT motion detector.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_motion_detector_enable(void);

/** @brief Reset DULT motion detector.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_motion_detector_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_MOTION_DETECTOR_H_ */
