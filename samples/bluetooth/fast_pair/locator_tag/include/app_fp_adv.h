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

/** Fast Pair advertising trigger. */
struct app_fp_adv_trigger {
	/* Identifier. */
	const char *id;
};

/** Register a trigger for Fast Pair advertising.
 *
 *  @param _name Trigger structure name.
 *  @param _id Trigger identifier.
 */
#define APP_FP_ADV_TRIGGER_REGISTER(_name, _id)				\
	BUILD_ASSERT(_id != NULL);					\
	static STRUCT_SECTION_ITERABLE(app_fp_adv_trigger, _name) = {	\
		.id = _id,						\
	}

/** Request turning on the Fast Pair advertising.
 *
 *  The Fast Pair advertising will be turned off only when no
 *  trigger has requested to keep the Fast Pair advertising on.
 *
 *  If the current Fast Pair advertising mode is the same as the requested mode,
 *  the Fast Pair advertising payload will not be refreshed.
 *  Use the @ref app_fp_adv_payload_refresh function to manually refresh
 *  the Fast Pair advertising payload.
 *
 *  @param trigger Trigger identifier.
 *  @param enable true to request to turn on the Fast Pair advertising.
 *		  false to remove request to turn on the Fast Pair advertising.
 */
void app_fp_adv_request(struct app_fp_adv_trigger *trigger, bool enable);

/** Refresh the Fast Pair advertising payload.
 *
 *  This function is used to manually trigger the refreshing of the Fast Pair
 *  advertising data to ensure that the advertising data is up to date.
 *
 *  Use with caution, as it updates the Fast Pair advertising payload asynchronously
 *  to the RPA address rotation and to the update of the FMDN advertising payload.
 */
void app_fp_adv_payload_refresh(void);

/** Get the current Fast Pair advertising mode.
 *
 *  @return Current Fast Pair advertising mode.
 */
enum app_fp_adv_mode app_fp_adv_mode_get(void);

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

/** Initialize the Fast Pair advertising module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_fp_adv_init(void);

/** Enable the Fast Pair advertising module.
 *
 *  Can be called after the Fast Pair advertising module has been initialized using the
 *  @ref app_fp_adv_init function.
 *  Must be called after enabling the Fast Pair subsystem using the @ref bt_fast_pair_enable
 *  function.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_fp_adv_enable(void);

/** Disable the Fast Pair advertising module.
 *
 *  Can be called after the Fast Pair advertising module has been initialized using the
 *  @ref app_fp_adv_init function.
 *  Should be called before disabling the Fast Pair subsystem using the @ref bt_fast_pair_disable
 *  function.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_fp_adv_disable(void);

/** Check if the Fast Pair advertising module is ready.
 *
 *  @return true when the module is ready, false otherwise.
 */
bool app_fp_adv_is_ready(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_FP_ADV_H_ */
