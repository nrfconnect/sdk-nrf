/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_FP_ADV_H_
#define APP_FP_ADV_H_

#include <stdint.h>

/**
 * @defgroup fmdn_sample_fp_adv Locator Tag sample Fast Pair advertising module
 * @brief Locator Tag sample Fast Pair advertising module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Fast Pair advertising mode. */
enum app_fp_adv_mode {
	/* Turned off mode. */
	APP_FP_ADV_MODE_OFF,

	/* Discoverable mode. */
	APP_FP_ADV_MODE_DISCOVERABLE,

	/* Not Discoverable mode. */
	APP_FP_ADV_MODE_NOT_DISCOVERABLE,
};

/** Set the Fast Pair advertising mode.
 *
 * @param adv_mode Fast Pair advertising mode.
 */
void app_fp_adv_mode_set(enum app_fp_adv_mode adv_mode);

/** Set the suspension mode for the RPA rotations of the Fast Pair advertising set.
 *
 *  @param suspended true if the RPA cannot change on the RPA timeout.
 *                   false if the RPA can change on the RPA timeout (default).
 */
void app_fp_adv_rpa_rotation_suspend(bool suspended);

/** Set the Bluetooth identity for the Fast Pair advertising.
 *
 *  This identity shall be created with the @ref bt_id_create function
 *  that is available in the Bluetooth API.
 *  You can only set the Bluetooth identity when the Fast Pair advertising
 *  module is uninitialized. If you do not explicitly set the identity with
 *  this API call, the @ref BT_ID_DEFAULT identity is used.
 *
 *  @param id Bluetooth identity for the Fast Pair advertising.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_fp_adv_id_set(uint8_t id);

/** Get the Bluetooth identity used for the Fast Pair advertising.
 *
 *  @return Bluetooth identity for the Fast Pair advertising.
 */
uint8_t app_fp_adv_id_get(void);

/** Uninitialize the Fast Pair advertising module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_fp_adv_uninit(void);

/** Initialize the Fast Pair advertising module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_fp_adv_init(void);

/** Check if the Fast Pair advertising module is initialized.
 *
 *  @return true when the module is initialized, false otherwise.
 */
bool app_fp_adv_is_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_FP_ADV_H_ */
