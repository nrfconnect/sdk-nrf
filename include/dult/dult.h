/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_DULT_H_
#define _DULT_DULT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/sys/slist.h>

/**
 * @defgroup dult Detecting Unwanted Location Trackers
 * @brief API for Detecting Unwanted Location Trackers specification implementation
 *
 * The Detecting Unwanted Location Trackers module can be used by location trackers devices to
 * comply with the Detecting Unwanted Location Trackers specification
 * (https://datatracker.ietf.org/doc/draft-detecting-unwanted-location-trackers/). The specification
 * can be used with various location tracking networks. The API is not fully thread-safe and
 * should be called from cooperative thread context.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Length of DULT product data array (bytes). See the @a product_data field in the @ref dult_user
 *  structure.
 */
#define DULT_PRODUCT_DATA_LEN	8

/** DULT Network ID value. Used in the @a network_id field that is part of the @ref dult_user
 *  structure.
 */
enum dult_network_id {
	DULT_NETWORK_ID_APPLE = 0x01,
	DULT_NETWORK_ID_GOOGLE = 0x02,
};

/** Bit positions in bitmask that encodes capabilities of the DULT accessory. */
enum dult_accessory_capability {
	/** Bit position for the play sound capability. */
	DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS = 0,

	/** Bit position for the motion detector unwanted tracking capability. */
	DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS = 1,

	/** Bit position for the identifier lookup by NFC capability. */
	DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_NFC_BIT_POS = 2,

	/** Bit position for the identifier lookup by BLE capability. */
	DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS = 3,
};

/** DULT version. Used to describe the firmware and network version. */
struct dult_version {
	/** Major version. */
	uint16_t major;

	/** Minor version. */
	uint8_t minor;

	/** Revision. */
	uint8_t revision;
};

/** DULT user. */
struct dult_user {
	/** Pointer to the product data array. The array length must be equal to
	 *  @ref DULT_PRODUCT_DATA_LEN.
	 */
	const uint8_t *product_data;

	/** Manufacturer name. */
	const char *manufacturer_name;

	/** Model name. */
	const char *model_name;

	/** Accessory category. */
	uint8_t accessory_category;

	/** Accessory capabilities bitmask. See @ref dult_accessory_capability for more details. */
	uint32_t accessory_capabilities;

	/** Network ID. See @ref dult_network_id for more details. */
	enum dult_network_id network_id;

	/** Firmware version. */
	struct dult_version firmware_version;

	/** Network version.
	 *
	 *  When non-NULL, the value is returned in response to the
	 *  OPTIONAL Get_Network_Version (0x000D) opcode of the Accessory
	 *  Non-owner Service. When NULL, the opcode is reported as
	 *  unsupported.
	 */
	const struct dult_version *network_version;
};

/** @brief Register DULT user.
 *
 *  The function must be called before calling any other functions from the DULT user API.
 *
 *  By default (@kconfig{CONFIG_DULT_MULTI_USER} disabled), the DULT subsystem supports only one
 *  user at the time.
 *  The lifetime of the registered user is limited by the @ref dult_reset API.
 *
 *  When the @kconfig{CONFIG_DULT_MULTI_USER} Kconfig is enabled, the DULT subsystem supports
 *  up to @kconfig{CONFIG_DULT_MULTI_USER_MAX} users that can be registered at the same time.
 *  The lifetime of the registered user when there is a multi-user support is limited by
 *  the @ref dult_user_unregister API.
 *
 *  @param user	Structure containing user information.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_register(const struct dult_user *user);

/** @brief Unregister a DULT user.
 *
 *  Used only in the multi-user (@kconfig{CONFIG_DULT_MULTI_USER}) configurations.
 *  Calling this function unregisters the registered DULT user structure and callbacks.
 *
 *  @param user	User structure used to authenticate the user.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_unregister(const struct dult_user *user);

/** @brief Set the current battery level.
 *
 *  This function sets the current battery level. The battery level is an optional
 *  feature in the DULT specification and this API must not be used when the
 *  @kconfig{CONFIG_DULT_BATTERY} Kconfig is disabled.
 *
 *  If the @kconfig{CONFIG_DULT_BATTERY} Kconfig is enabled, call this function after registering
 *  the DULT user with @ref dult_user_register. In single-user builds
 *  (@kconfig{CONFIG_DULT_MULTI_USER} disabled) the battery level is mandatory and must be set
 *  before the first @ref dult_enable that follows registration. In multi-user builds
 *  (@kconfig{CONFIG_DULT_MULTI_USER}) setting it is optional; until it is set, the ANOS
 *  Get_Battery_Level operation is answered as invalid. Subsequent calls to update the battery
 *  level are allowed in the enabled mode.
 *
 *  To keep the battery level information accurate, the user should set the battery level
 *  to the new value with the help of this API as soon as the device battery level changes.
 *
 *  The battery level is stored per registered user, so it can be set during the pre-association
 *  window and each locator network keeps its own value. The accessory reports the currently
 *  associated user's value. In single-user builds (@kconfig{CONFIG_DULT_MULTI_USER} disabled) it
 *  is cleared by @ref dult_reset, which is the terminal teardown for that configuration. In
 *  multi-user builds (@kconfig{CONFIG_DULT_MULTI_USER}) it is preserved across @ref dult_reset so
 *  it does not have to be set again before each subsequent @ref dult_enable, and it is cleared by
 *  @ref dult_user_unregister.
 *
 *  The exact mapping of the battery percentage to the battery level as defined by the
 *  DULT specification in the ANOS is implementation-specific. The mapping configuration
 *  is controlled by the following Kconfig options:
 *  @kconfig{CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR}, @kconfig{CONFIG_DULT_BATTERY_LEVEL_LOW_THR}
 *  and @kconfig{CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR}.
 *
 *  @param user	            User structure used to authenticate the user.
 *  @param percentage_level Battery level as a percentage [0-100%]
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_battery_level_set(const struct dult_user *user, uint8_t percentage_level);

/** DULT identifier read state callback. */
struct dult_id_read_state_cb {
	/** @brief Get identifier payload.
	 *
	 *  This callback is called to get the DULT user-specific identifier payload.
	 *
	 *  @param[out]    buf	Pointer to the buffer used to store identifier payload.
	 *  @param[in,out] len	Pointer to the length (in bytes) of the buffer used to store
	 *			identifier payload. A negative error code shall be returned
	 *			if this value is too small. If the operation was successful, the
	 *			length of the identifier payload shall be written to this pointer.
	 *
	 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is
	 *	    returned.
	 */
	int (*payload_get)(uint8_t *buf, size_t *len);

	/** @brief Identifier read state exited.
	 *
	 *  This callback is called to indicate that the identifier read state has been exited.
	 *  Identifier read state can be entered by calling the
	 *  @ref dult_id_read_state_enter API.
	 */
	void (*exited)(void);
};

/** @brief Register DULT identifier read state callback structure.
 *
 *  This function must be called after registering the DULT user with @ref dult_user_register and
 *  before enabling DULT with @ref dult_enable function.
 *
 *  In single-user builds (@kconfig{CONFIG_DULT_MULTI_USER} disabled) the callback is cleared
 *  by @ref dult_reset, which is the terminal teardown for that configuration; it must be
 *  registered again after a subsequent @ref dult_user_register. In multi-user builds
 *  (@kconfig{CONFIG_DULT_MULTI_USER}) the callback is preserved across @ref dult_reset calls
 *  and cleared only by @ref dult_user_unregister.
 *
 *  @param user	User structure used to authenticate the user.
 *  @param cb	Identifier read state callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_id_read_state_cb_register(const struct dult_user *user,
				   const struct dult_id_read_state_cb *cb);

/** @brief Enter DULT identifier read state.
 *
 *  This function can only be called if DULT was previously enabled with the
 *  @ref dult_enable API.
 *
 *  @param user		User structure used to authenticate the user.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_id_read_state_enter(const struct dult_user *user);

/** Minimum duration in milliseconds for the DULT sound action originating from the Bluetooth
 *  accessory non-owner service (see @ref DULT_SOUND_SRC_BT_GATT).
 */
#define DULT_SOUND_DURATION_BT_GATT_MIN_MS (5000U)

/** DULT sound source types. */
enum dult_sound_src {
	/** Sound source type originating from the Bluetooth accessory non-owner service. */
	DULT_SOUND_SRC_BT_GATT,

	/** Sound source type originating from the Motion detector.
	 *  Used only when the @kconfig{CONFIG_DULT_MOTION_DETECTOR} is enabled.
	 */
	DULT_SOUND_SRC_MOTION_DETECTOR,

	/** External source type originating from the unknown location to the DULT module. */
	DULT_SOUND_SRC_EXTERNAL,
};

/** DULT sound callback. */
struct dult_sound_cb {
	/** @brief Request the user to start the sound action.
	 *
	 *  This callback is triggered to notify the upper layer about the request
	 *  to start sound action. If the upper layer changes its sound state in
	 *  response to this request (as described by the @ref dult_sound_state_param
	 *  structure), it then calls the @ref dult_sound_state_update API.
	 *
	 *  @param src	Sound source type. Only the DULT internal sources are
	 *              used in this callback: @ref DULT_SOUND_SRC_BT_GATT,
	 *              @ref DULT_SOUND_SRC_MOTION_DETECTOR.
	 */
	void (*sound_start)(enum dult_sound_src src);

	/** @brief Request the user to stop the sound action.
	 *
	 *  This callback is triggered to notify the upper layer about the request
	 *  to stop sound action. If the upper layer changes its sound state in
	 *  response to this request (as described by the @ref dult_sound_state_param
	 *  structure), it then calls the @ref dult_sound_state_update API.
	 *
	 *  @param src	Sound source type. Only the DULT internal source originating
	 *		from the Bluetooth accessory non-owner service
	 *		(@ref DULT_SOUND_SRC_BT_GATT) is used in this callback.
	 */
	void (*sound_stop)(enum dult_sound_src src);
};

/** @brief Register DULT sound callback structure.
 *
 *  This function must be called after registering the DULT user with @ref dult_user_register
 *  and before enabling DULT with @ref dult_enable function.
 *
 *  In single-user builds (@kconfig{CONFIG_DULT_MULTI_USER} disabled) the callback is cleared
 *  by @ref dult_reset, which is the terminal teardown for that configuration; it must be
 *  registered again after a subsequent @ref dult_user_register. In multi-user builds
 *  (@kconfig{CONFIG_DULT_MULTI_USER}) the callback is preserved across @ref dult_reset calls
 *  and cleared only by @ref dult_user_unregister.
 *
 *  @param user	User structure used to authenticate the user.
 *  @param cb	Sound callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error
 *          code is returned.
 */
int dult_sound_cb_register(const struct dult_user *user, const struct dult_sound_cb *cb);

/** Sound state parameters */
struct dult_sound_state_param {
	/** Sound state change flag.
	 *  True when the sound state is activated.
	 *  False: when the sound state is deactivated.
	 */
	bool active;

	/** Sound source type.
	 *  The source can change during the sound-playing operation. For example, the
	 *  @ref DULT_SOUND_SRC_EXTERNAL source can override the @ref DULT_SOUND_SRC_BT_GATT
	 *  source). In the typical flow, the source for sound activation is also the source
	 *  for sound deactivation.
	 */
	enum dult_sound_src src;
};

/** @brief Update the sound state.
 *
 *  This function should be called by the upper layer on each sound state change
 *  as defined by the @ref dult_sound_state_param structure. All fields defined in the
 *  linked structure compose the sound state. The sound state change should be reported
 *  as soon as possible with this API, so that the DULT module is able to track the state
 *  in the real-time.
 *
 *  This API is typically used to respond to the callbacks defined by the @ref dult_sound_cb
 *  structure. Each callback requests a specific action and the upper layer can accept
 *  a request by changing the sound state with this API. The upper layer is the ultimate
 *  owner of the sound state and only notifies the DULT module about each change.
 *
 *  This function can be used to change the sound state asynchronously as it is often
 *  impossible to execute sound playing action on the speaker device in the context of
 *  the requesting callbacks (as defined in the @ref dult_sound_cb structure). Asynchronous
 *  support is also necessary to report sound state changes triggered by an external source
 *  unknown to the DULT module (see @ref DULT_SOUND_SRC_EXTERNAL source type).
 *
 *  @param user  User structure used to authenticate the user.
 *  @param param Sound state parameters.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error
 *          code is returned.
 */
int dult_sound_state_update(const struct dult_user *user,
			    const struct dult_sound_state_param *param);

/** @brief Motion detector callback structure.
 *
 *  Used only when the @kconfig{CONFIG_DULT_MOTION_DETECTOR} Kconfig option is enabled.
 */
struct dult_motion_detector_cb {
	/** @brief Request the user to start the motion detector.
	 *
	 *  This callback is called to start the motion detector
	 *  activity. From now on, the motion detector events are polled
	 *  periodically with the @ref period_expired API.
	 *  The motion detector activity stops when the
	 *  @ref stop is called.
	 */
	void (*start)(void);

	/** @brief Notify the user that the motion detector period has expired.
	 *
	 *  This callback is called at the end of each
	 *  motion detector period. The @ref start function
	 *  indicates the beginning of the first motion detector period.
	 *  The next period is started as soon as the previous period expires.
	 *  The user should notify the DULT module if motion was detected
	 *  in the previous period. The return value of this callback
	 *  is used to pass this information.
	 *
	 *  @return true to indicate detected motion in the last period,
	 *  otherwise false.
	 */
	bool (*period_expired)(void);

	/** @brief Notify the user that the motion detector can be stopped.
	 *
	 *  This callback is called to notify the user that the motion
	 *  detector is no longer used by the DULT module. It concludes
	 *  the motion detector activity that was started by the
	 *  @ref start callback.
	 */
	void (*stop)(void);
};

/** @brief Register motion detector callbacks.
 *
 *  This function registers callbacks to handle motion detector activities defined
 *  in the Motion detector feature from the DULT specification. This API can only
 *  be used when the @kconfig{CONFIG_DULT_MOTION_DETECTOR} Kconfig option is
 *  enabled. If this configuration is active, this function must be called after
 *  registering the DULT user with @ref dult_user_register and before enabling
 *  DULT with @ref dult_enable function.
 *
 *  In single-user builds (@kconfig{CONFIG_DULT_MULTI_USER} disabled) the callback is cleared
 *  by @ref dult_reset, which is the terminal teardown for that configuration; it must be
 *  registered again after a subsequent @ref dult_user_register. In multi-user builds
 *  (@kconfig{CONFIG_DULT_MULTI_USER}) the callback is preserved across @ref dult_reset calls
 *  and cleared only by @ref dult_user_unregister.
 *
 *  @param user	User structure used to authenticate the user.
 *  @param cb Motion detector callback structure.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_motion_detector_cb_register(const struct dult_user *user,
				     const struct dult_motion_detector_cb *cb);

/** Modes of the DULT near-owner state. */
enum dult_near_owner_state_mode {
	/** Separated mode of the near-owner state. */
	DULT_NEAR_OWNER_STATE_MODE_SEPARATED,

	/** Near-owner mode of the near-owner state. */
	DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER,

	/** Mode count for the near-owner state. */
	DULT_NEAR_OWNER_STATE_MODE_COUNT,
};

/** @brief Set the mode of the DULT near-owner state.
 *
 *  This function can be called by any registered DULT user (see @ref dult_user_register),
 *  including before it becomes the associated user, so the near-owner state can be preset ahead
 *  of association. The value is stored per user; the accessory uses the currently associated
 *  user's value.
 *
 *  Until set, the near-owner state defaults to "near-owner"
 *  (see @ref DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER). In single-user builds
 *  (@kconfig{CONFIG_DULT_MULTI_USER} disabled) it is cleared by @ref dult_reset, which is the
 *  terminal teardown for that configuration. In multi-user builds
 *  (@kconfig{CONFIG_DULT_MULTI_USER}) it is preserved across @ref dult_reset and cleared by
 *  @ref dult_user_unregister; the user is responsible for setting it as needed after a subsequent
 *  association.
 *
 *  @param user	User structure used to authenticate the user.
 *  @param mode	Mode of the DULT near-owner state.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_near_owner_state_set(const struct dult_user *user, enum dult_near_owner_state_mode mode);

/** @brief Enable DULT.
 *
 *  This function shall be used only after calling the @ref dult_user_register
 *  and registering all of the required callbacks.
 *
 *  When the @kconfig{CONFIG_DULT_MULTI_USER} Kconfig is enabled, enabling one user first
 *  evicts every other registered user. This is done to ensure that only one user is enabled at a
 *  time. The DULT notifies every registered user about the arbitration outcome (including the
 *  evicted ones) via the @ref dult_multi_user_cb.ownership_claimed callback.
 *  These notifications are delivered asynchronously. Toggling the association
 *  (repeated @ref dult_enable / @ref dult_reset) multiple times within a single
 *  execution context without yielding may drop cancelling transitions, so intermediate
 *  states can be lost.
 *  However, even in such a case, the end association state will still be reported correctly, and
 *  the callback listener will never get two consecutive @ref dult_multi_user_cb.ownership_claimed
 *  or two consecutive @ref dult_multi_user_cb.ownership_released callbacks.
 *  Toggling the association states with the @ref dult_enable and @ref dult_reset functions in a
 *  single execution context without yielding is strongly discouraged.
 *
 *  @param user	User structure used to authenticate the user.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_enable(const struct dult_user *user);

/** @brief Reset DULT.
 *
 *  This function can only be called by the DULT user previously registered with the
 *  @ref dult_user_register function.
 *
 *  By default (@kconfig{CONFIG_DULT_MULTI_USER} disabled), calling this function unregisters
 *  the registered DULT user structure and callbacks.
 *
 *  When the @kconfig{CONFIG_DULT_MULTI_USER} Kconfig is enabled, releases the currently associated
 *  user but keeps the DULT user and its callbacks registered, so the @ref dult_enable function can
 *  be called again to re-enable the user. Use the @ref dult_user_unregister function to fully
 *  unregister the DULT user and return it to the unregistered state.
 *
 *  In multi-user builds, releasing the association triggers the
 *  @ref dult_multi_user_cb.ownership_released notification for every registered user, mirroring
 *  the @ref dult_multi_user_cb.ownership_claimed notification emitted by @ref dult_enable.
 *  These notifications are delivered asynchronously. Toggling the association
 *  (repeated @ref dult_enable / @ref dult_reset) multiple times within a single
 *  execution context without yielding may drop cancelling transitions, so intermediate
 *  states can be lost.
 *  However, even in such a case, the end association state will still be reported correctly, and
 *  the callback listener will never get two consecutive @ref dult_multi_user_cb.ownership_claimed
 *  or two consecutive @ref dult_multi_user_cb.ownership_released callbacks.
 *  Toggling the association states with the @ref dult_enable and @ref dult_reset functions in a
 *  single execution context without yielding is strongly discouraged.
 *
 *  @param user	User structure used to authenticate the user.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_reset(const struct dult_user *user);

/** @brief Check if the DULT user is the associated user.
 *
 *  The associated user is the user that successfully called the @ref dult_enable function and has
 *  not released the association with the @ref dult_reset function. At most one user is associated
 *  at a time.
 *
 *  The returned value is a snapshot of the current state. The return values of the
 *  @ref dult_enable and @ref dult_reset functions remain authoritative for the association
 *  outcome.
 *
 *  @param user	User structure used to authenticate the user.
 *
 *  @return True if the provided user is the associated user. Otherwise, False is returned.
 */
bool dult_user_is_associated(const struct dult_user *user);

/** @brief Check whether any DULT user is currently associated.
 *
 *  @return True if some user has called the @ref dult_enable function and has not yet been reset.
 */
bool dult_is_any_associated(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_DULT_H_ */
