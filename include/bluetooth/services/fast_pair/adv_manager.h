/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_ADV_MANAGER_H_
#define BT_FAST_PAIR_ADV_MANAGER_H_

#include <stdint.h>

#include <zephyr/sys/slist.h>

/**
 * @defgroup bt_fast_pair_adv_manager Fast Pair Advertising Manager API
 * @brief Fast Pair Advertising Manager API for managing the Fast Pair advertising set
 *
 *  It is required to use the Fast Pair Advertising Manager API in the cooperative thread
 *  context. Following this requirement guarantees a proper synchronization between the
 *  user operations and the module operations.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Configuration of the Fast Pair Advertising Manager trigger. */
struct bt_fast_pair_adv_manager_trigger_config {
	/** Request activation of the pairing mode.
	 *
	 *  The pairing mode is active if at least one Fast Pair Advertising Manager trigger is
	 *  enabled while having this flag set to true.
	 *
	 *  The change of the pairing mode state always triggers the advertising payload update.
	 */
	bool pairing_mode;

	/** Request suspension of the RPA rotations used by Fast Pair advertising set.
	 *
	 *  The RPA rotation is suspended if at least one Fast Pair Advertising Manager trigger is
	 *  enabled while having this flag set to true.
	 *
	 *  Use with caution, as this flag decreases the privacy of the advertiser by freezing
	 *  its RPA rotations. It is typically used for triggers that are only active for a short
	 *  period of time.
	 */
	bool suspend_rpa;
};

/** @brief Check if the Fast Pair Advertising Manager is in the pairing mode.
 *
 *  This function can be used to synchronously check if the Fast Pair Advertising Manager is in
 *  the pairing mode. In the pairing mode, the Fast Pair advertising set uses the discoverable
 *  advertising payload. Otherwise, the not discoverable payload is used.
 *
 *  The pairing mode is active if at least one Fast Pair Advertising Manager trigger is enabled
 *  with the @ref bt_fast_pair_adv_manager_request function and is configured for the pairing mode.
 *  The @ref bt_fast_pair_adv_manager_trigger_config.pairing_mode flag is used to configure the
 *  pairing mode for the trigger.
 *
 * @retval true  If the Fast Pair Advertising Manager is configured for the pairing mode.
 * @retval false Otherwise.
 */
bool bt_fast_pair_adv_manager_is_pairing_mode(void);

/** Fast Pair Advertising Manager trigger. */
struct bt_fast_pair_adv_manager_trigger {
	/** Identifier. */
	const char *id;

	/** Configuration. */
	const struct bt_fast_pair_adv_manager_trigger_config * const config;
};

/** @brief Register a trigger in the Fast Pair Advertising Manager.
 *
 *  @param _name Trigger structure name.
 *  @param _id Trigger identifier.
 *  @param _config Trigger configuration.
 */
#define BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER(_name, _id, _config)			\
	BUILD_ASSERT(_id != NULL);							\
	static STRUCT_SECTION_ITERABLE(bt_fast_pair_adv_manager_trigger, _name) = {	\
		.id = _id,								\
		.config = _config,							\
	}

/** @brief Request turning on the Fast Pair advertising.
 *
 *  The Fast Pair advertising will be turned off only when no trigger has requested to keep
 *  the Fast Pair advertising on.
 *
 *  The Fast Pair Advertising Manager module may change its operating mode in response to the
 *  requesting trigger configuration (see the @ref bt_fast_pair_adv_manager_trigger_config).
 *  For example, this module operates in the pairing mode if at least one advertising trigger
 *  is enabled with this function and is configured for the pairing mode. For more details
 *  on how other fields from the  @ref bt_fast_pair_adv_manager_trigger_config structure affect
 *  the advertising state of the module, see the structure documentation.
 *
 *  The pairing mode state changes of the Fast Pair Advertising Manager module cause the Fast
 *  Pair advertising payload update. Apart from this exception, the activation of the trigger
 *  when the Fast Pair advertising is already enabled do not cause the advertising payload
 *  update. The @ref bt_fast_pair_adv_manager_payload_refresh can be used to refresh the payload
 *  instantly if necessary.
 *
 *  You can mark the trigger as enabled and set its configuration using the
 *  @ref bt_fast_pair_adv_manager_request function when the Fast Pair Advertising Manager module
 *  is still disabled.
 *
 *  @param trigger Trigger identifier.
 *  @param enable true to request to turn on the Fast Pair advertising.
 *		  false to remove request to turn on the Fast Pair advertising.
 */
void bt_fast_pair_adv_manager_request(struct bt_fast_pair_adv_manager_trigger *trigger,
				      bool enable);

/** @brief Refresh the Fast Pair advertising payload.
 *
 *  This function is used to manually trigger the refreshing of the Fast Pair advertising data
 *  to ensure that the advertising data is up to date.
 *
 *  Use with caution, as it updates the Fast Pair advertising payload asynchronously to the RPA
 *  address rotation and to the update of other active advertising sets (for example, the FMDN
 *  advertising set).
 */
void bt_fast_pair_adv_manager_payload_refresh(void);

/** @brief Set the Bluetooth identity for the Fast Pair advertising.
 *
 *  This identity shall be created with the bt_id_create function that is available in the
 *  Bluetooth API. You can only set the Bluetooth identity when the Fast Pair Advertising
 *  Manager module is disabled. If you do not explicitly set the identity with this API call,
 *  the BT_ID_DEFAULT identity is used.
 *
 *  @param id Bluetooth identity for the Fast Pair advertising.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_adv_manager_id_set(uint8_t id);

/** @brief Get the Bluetooth identity used for the Fast Pair advertising.
 *
 *  @return Bluetooth identity for the Fast Pair advertising.
 */
uint8_t bt_fast_pair_adv_manager_id_get(void);

/** Information callback descriptor for the Fast Pair Advertising Manager. */
struct bt_fast_pair_adv_manager_info_cb {
	/** Notify that the advertising state has changed.
	 *
	 * This information callback is used to track the state of the Fast Pair advertising set.
	 * The current state can be used to signal the application mode in the user interface
	 * (for example, on LEDs).
	 *
	 * The @ref bt_fast_pair_adv_manager_is_adv_active API function can be used to
	 * synchronously check the state of the Fast Pair advertising set.
	 *
	 *  @param active True:  Fast Pair advertising is active.
	 *                False: Fast Pair advertising is inactive.
	 */
	void (*adv_state_changed)(bool active);

	/** Internally used field for list handling. */
	sys_snode_t node;
};

/** @brief Register information callbacks for the Fast Pair Advertising Manager.
 *
 *  This function registers an instance of information callbacks. The registered instance needs
 *  to persist in the memory after this function exits, as it is used directly without the copy
 *  operation.
 *
 *  This function can only be called before enabling the Fast Pair Advertising Manager module
 *  with the @ref bt_fast_pair_adv_manager_enable API.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_fast_pair_adv_manager_info_cb_register(struct bt_fast_pair_adv_manager_info_cb *cb);

/** @brief Check if the Fast Pair advertising set is active.
 *
 *  This function can be used to synchronously check if the Fast Pair advertising set is
 *  currently being advertised. To track the advertising state asynchronously, use the
 *  @ref bt_fast_pair_adv_manager_info_cb.adv_state_changed callback.
 *
 * @retval true  If the Fast Pair advertising set is active.
 * @retval false Otherwise.
 */
bool bt_fast_pair_adv_manager_is_adv_active(void);

/** @brief Enable the Fast Pair Advertising Manager module.
 *
 *  This function enables the Fast Pair Advertising Manager module. If any trigger is enabled
 *  with the @ref bt_fast_pair_adv_manager_request function, the module activates the Fair Pair
 *  advertising in the context of this function. Provided that the module starts advertising
 *  and the @ref bt_fast_pair_adv_manager_info_cb.adv_state_changed callback is registered,
 *  the callback will be triggered in the context of this function to indicate that the Fast
 *  Pair advertising set is active.
 *
 *  This API must be called when the Fast Pair subsystem is ready (see the @ref
 *  bt_fast_pair_is_ready function).
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_adv_manager_enable(void);

/** @brief Disable the Fast Pair Advertising Manager module.
 *
 *  This function disables the Fast Pair Advertising Manager module. If the Fast Pair advertising
 *  is active, the module stops the advertising activity in the context of this function. If there
 *  is any pending Fast Pair connection, the module attempts to terminate this connection in the
 *  context of this function. Provided that the advertising is active before this API call
 *  and the @ref bt_fast_pair_adv_manager_info_cb.adv_state_changed callback is registered,
 *  the callback will be triggered in the context of this function to indicate that the Fast
 *  Pair advertising set is inactive.
 *
 *  This API should be called before the Fast Pair subsystem gets disable (see the @ref
 *  bt_fast_pair_disable function).
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_adv_manager_disable(void);

/** @brief Check if the Fast Pair Advertising Manager module is ready.
 *
 *  @return true when the module is ready, false otherwise.
 */
bool bt_fast_pair_adv_manager_is_ready(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_ADV_MANAGER_H_ */
