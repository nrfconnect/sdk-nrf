/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_H_
#define _DULT_H_

#include <stdint.h>
#include <stddef.h>

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

/** DULT firmware version. */
struct dult_firmware_version {
	/** Major firmware version. */
	uint16_t major;

	/** Minor firmware version. */
	uint8_t minor;

	/** Firmware revision. */
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
	struct dult_firmware_version firmware_version;
};

/** @brief Register DULT user.
 *
 *  The function must be called before calling any other functions from the DULT user API.
 *  Only one DULT user can be registered at a time. Only the registered DULT user is
 *  allowed to use other DULT APIs.
 *
 *  @param user	Structure containing user information.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_register(const struct dult_user *user);

/** @brief Set the current battery level.
 *
 *  This function sets the current battery level. The battery level is an optional
 *  feature in the DULT specification and this API must not be used when the
 *  CONFIG_DULT_BATTERY Kconfig is disabled.
 *
 *  If the CONFIG_DULT_BATTERY Kconfig is enabled, this function must be called to
 *  initialize the battery level after registering the DULT user with @ref dult_user_register
 *  and before enabling DULT with @ref dult_enable function. Subsequent calls to update
 *  the battery level are allowed in the enabled mode.
 *
 *  To keep the battery level information accurate, the user should set the battery level
 *  to the new value with the help of this API as soon as the device battery level changes.
 *
 *  The exact mapping of the battery percentage to the battery level as defined by the
 *  DULT specification in the ANOS is implementation-specific. The mapping configuration
 *  is controlled by the following Kconfig options:
 *  CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR, CONFIG_DULT_BATTERY_LEVEL_LOW_THR
 *  and CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR.
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

/** Minimum duration in milliseconds for the DULT sound action. */
#define DULT_SOUND_DURATION_MIN_MS (5000U)

/** DULT sound source types. */
enum dult_sound_src {
	/** Sound source type originating from the Bluetooth accessory non-owner service. */
	DULT_SOUND_SRC_BT_GATT,

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
	 *              used in this callback: @ref DULT_SOUND_SRC_BT_GATT.
	 */
	void (*sound_start)(enum dult_sound_src src);

	/** @brief Request the user to stop the sound action.
	 *
	 *  This callback is triggered to notify the upper layer about the request
	 *  to stop sound action. If the upper layer changes its sound state in
	 *  response to this request (as described by the @ref dult_sound_state_param
	 *  structure), it then calls the @ref dult_sound_state_update API.
	 *
	 *  @param src	Sound source type. Only the DULT internal sources are
	 *              used in this callback: @ref DULT_SOUND_SRC_BT_GATT.
	 */
	void (*sound_stop)(enum dult_sound_src src);
};

/** @brief Register DULT sound callback structure.
 *
 *  This function must be called after registering the DULT user with @ref dult_user_register
 *  and before enabling DULT with @ref dult_enable function.
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
 *  This function can only be called if the DULT user was previously registered with the
 *  @ref dult_user_register API. By default, the DULT near-owner state is set to "near-owner"
 *  (see @ref DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER) on boot and when the @ref dult_reset API
 *  is called.
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
 *  @param user	User structure used to authenticate the user.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_enable(const struct dult_user *user);

/** @brief Reset DULT.
 *
 *  This function can only be called by the DULT user previously registered with the
 *  @ref dult_user_register function. After the device boot-up, no DULT user is registered.
 *  Calling this function unregisters the registered DULT user structure and callbacks.
 *
 *  @param user	User structure used to authenticate the user.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_reset(const struct dult_user *user);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_H_ */
