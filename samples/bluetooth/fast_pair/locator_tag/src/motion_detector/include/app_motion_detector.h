/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_MOTION_DETECTOR_H_
#define APP_MOTION_DETECTOR_H_

/**
 * @defgroup fmdn_sample_motion_detector Locator Tag sample motion detector module
 * @brief Locator Tag sample motion detector module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the motion detector module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_motion_detector_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_MOTION_DETECTOR_H_ */
