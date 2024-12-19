/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_LOCATOR_TAG_FP_ADV_H_
#define BT_FAST_PAIR_LOCATOR_TAG_FP_ADV_H_

#include <stdint.h>

/**
 * @defgroup bt_fast_pair_locator_tag_fp_adv Fast Pair Locator Tag advertising API
 * @brief Fast Pair Locator Tag API for managing the Fast Pair advertising set
 *
 *  It is required to use the Fast Pair Locator Tag advertising API in the cooperative thread
 *  context (for example, system workqueue thread). Following this requirement guarantees
 *  a proper synchronization between the user operations and the module operations.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Fast Pair Locator Tag advertising mode. */
enum bt_fast_pair_locator_tag_fp_adv_mode {
	/** Turned off mode. */
	BT_FAST_PAIR_LOCATOR_TAG_FP_ADV_MODE_OFF,

	/** Discoverable mode. */
	BT_FAST_PAIR_LOCATOR_TAG_FP_ADV_MODE_DISCOVERABLE,

	/** Not Discoverable mode. */
	BT_FAST_PAIR_LOCATOR_TAG_FP_ADV_MODE_NOT_DISCOVERABLE,
};

/** @brief Get the current Fast Pair advertising mode.
 *
 *  @return Current Fast Pair advertising mode.
 */
enum bt_fast_pair_locator_tag_fp_adv_mode bt_fast_pair_locator_tag_fp_adv_mode_get(void);

/** Fast Pair Locator Tag advertising trigger. */
struct bt_fast_pair_locator_tag_fp_adv_trigger {
	/** Identifier. */
	const char *id;
};

/** @brief Register a trigger for Fast Pair Locator Tag advertising.
 *
 *  @param _name Trigger structure name.
 *  @param _id Trigger identifier.
 */
#define BT_FAST_PAIR_LOCATOR_TAG_FP_ADV_TRIGGER_REGISTER(_name, _id)				\
	BUILD_ASSERT(_id != NULL);								\
	static STRUCT_SECTION_ITERABLE(bt_fast_pair_locator_tag_fp_adv_trigger, _name) = {	\
		.id = _id,									\
	}

/** @brief Request turning on the Fast Pair Locator Tag advertising.
 *
 *  The Fast Pair advertising will be turned off only when no trigger has requested to keep
 *  the Fast Pair advertising on.
 *
 *  If the current Fast Pair advertising mode is the same as the requested mode, the advertising
 *  payload will not be refreshed. Use the @ref bt_fast_pair_locator_tag_fp_adv_payload_refresh
 *  function to manually refresh the Fast Pair advertising payload.
 *
 *  @param trigger Trigger identifier.
 *  @param enable true to request to turn on the Fast Pair advertising.
 *		  false to remove request to turn on the Fast Pair advertising.
 */
void bt_fast_pair_locator_tag_fp_adv_request(
	struct bt_fast_pair_locator_tag_fp_adv_trigger *trigger, bool enable);

/** @brief Refresh the Fast Pair advertising payload.
 *
 *  This function is used to manually trigger the refreshing of the Fast Pair advertising data
 *  to ensure that the advertising data is up to date.
 *
 *  Use with caution, as it updates the Fast Pair advertising payload asynchronously
 *  to the RPA address rotation and to the update of the FMDN advertising payload.
 */
void bt_fast_pair_locator_tag_fp_adv_payload_refresh(void);

/** @brief Set the Bluetooth identity for the Fast Pair advertising.
 *
 *  This identity shall be created with the bt_id_create function that is available in the
 *  Bluetooth API. You can only set the Bluetooth identity when the Fast Pair advertising
 *  module is uninitialized. If you do not explicitly set the identity with this API call,
 *  the BT_ID_DEFAULT identity is used.
 *
 *  @param id Bluetooth identity for the Fast Pair advertising.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_locator_tag_fp_adv_id_set(uint8_t id);

/** @brief Get the Bluetooth identity used for the Fast Pair advertising.
 *
 *  @return Bluetooth identity for the Fast Pair advertising.
 */
uint8_t bt_fast_pair_locator_tag_fp_adv_id_get(void);

/** Fast Pair advertising information callback descriptor. */
struct bt_fast_pair_locator_tag_fp_adv_info_cb {
	/** Notify that the advertising state has changed.
	 *
	 * This information callback is used to track the state of the Fast Pair advertising set.
	 * The current state can be used to signal the application mode in the user interface
	 * (for example, on LEDs).
	 *
	 *  @param enable True:  Fast Pair advertising is enabled.
	 *                False: Fast Pair advertising is disabled.
	 */
	void (*state_changed)(bool enabled);
};

/** @brief Register Fast Pair advertising information callbacks.
 *
 *  This function registers an instance of information callbacks. The registered instance needs
 *  to persist in the memory after this function exits, as it is used directly without the copy
 *  operation. It is possible to register only one instance of callbacks with this API.
 *
 *  This function can only be called before enabling Fast Pair with
 *  the @ref bt_fast_pair_locator_tag_fp_adv_enable API.
 *
 *  This function must be called in the cooperative thread context.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_fast_pair_locator_tag_fp_adv_info_cb_register(
	const struct bt_fast_pair_locator_tag_fp_adv_info_cb *cb);

/** @brief Enable the Fast Pair advertising module.
 *
 *  Can be called after the Fast Pair advertising module has been initialized using the
 *  @ref bt_fast_pair_locator_tag_fp_adv_init function.
 *
 *  Must be called after enabling the Fast Pair subsystem using the @ref bt_fast_pair_enable
 *  function.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_locator_tag_fp_adv_enable(void);

/** @brief Disable the Fast Pair advertising module.
 *
 *  Can be called after the Fast Pair advertising module has been initialized using the
 *  @ref bt_fast_pair_locator_tag_fp_adv_init function.
 *
 *  Should be called before disabling the Fast Pair subsystem using the @ref bt_fast_pair_disable
 *  function.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_locator_tag_fp_adv_disable(void);

/** @brief Check if the Fast Pair advertising module is ready.
 *
 *  @return true when the module is ready, false otherwise.
 */
bool bt_fast_pair_locator_tag_fp_adv_is_ready(void);

/** @brief Initialize the Fast Pair advertising module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_locator_tag_fp_adv_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_LOCATOR_TAG_FP_ADV_H_ */
