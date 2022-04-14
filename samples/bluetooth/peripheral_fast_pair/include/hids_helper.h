/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HIDS_HELPER_H_
#define HIDS_HELPER_H_

/**
 * @defgroup fast_pair_sample_hids_helper Fast Pair sample HIDS API
 * @brief Fast Pair sample HIDS API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Audio volume change. */
enum hids_helper_volume_change {
	/** Decrease audio volume. */
	HIDS_HELPER_VOLUME_CHANGE_DOWN,

	/** Keep constant audio volume. */
	HIDS_HELPER_VOLUME_CHANGE_NONE,

	/** Increase audio volume. */
	HIDS_HELPER_VOLUME_CHANGE_UP,
};

/** Initialize HIDS
 *
 * Only one consumer control HID report is supported by the report map. The HID report is used to
 * control the audio volume.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int hids_helper_init(void);

/** Send HID report to control audio volume
 *
 * @param[in] volume_change Enumerator indicating if audio volume should be increased, decreased or
 *                          kept constant.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int hids_helper_volume_ctrl(enum hids_helper_volume_change volume_change);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* HIDS_HELPER_H_ */
