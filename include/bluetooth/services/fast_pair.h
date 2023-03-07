/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_H_
#define BT_FAST_PAIR_H_

#include <zephyr/bluetooth/bluetooth.h>

/**
 * @defgroup bt_fast_pair Fast Pair API
 * @brief Fast Pair API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Value that denotes unknown battery level (see @ref bt_fast_pair_battery). */
#define BT_FAST_PAIR_BATTERY_LEVEL_UNKNOWN	0x7f

/** @brief Fast Pair advertising mode. Used to generate advertising packet.
 *
 * According to Fast Pair specification, when no Account Key has been saved on the device both Fast
 * Pair not discoverable advertising modes result in the same advertising data.
 */
enum bt_fast_pair_adv_mode {
	/** Fast Pair discoverable advertising. */
	BT_FAST_PAIR_ADV_MODE_DISCOVERABLE,

	/** Fast Pair not discoverable advertising, show UI indication. */
	BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_SHOW_UI_IND,

	/** Fast Pair not discoverable advertising, hide UI indication. */
	BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_HIDE_UI_IND,

	/** Number of Fast Pair advertising modes. */
	BT_FAST_PAIR_ADV_MODE_COUNT
};

/** @brief Fast Pair advertising battery mode. Used to generate advertising packet.
 *
 * Battery data can be included in advertising packet only if the Fast Pair Provider is in Fast Pair
 * not discoverable advertising mode. To prevent tracking, the Fast Pair Provider should not include
 * battery data in the advertising packet all the time.
 */
enum bt_fast_pair_adv_battery_mode {
	/** Do not advertise battery data. */
	BT_FAST_PAIR_ADV_BATTERY_MODE_NONE,

	/** Show battery data UI indication. */
	BT_FAST_PAIR_ADV_BATTERY_MODE_SHOW_UI_IND,

	/** Hide battery data UI indication. */
	BT_FAST_PAIR_ADV_BATTERY_MODE_HIDE_UI_IND,

	/** Number of Fast Pair advertising battery modes. */
	BT_FAST_PAIR_ADV_BATTERY_MODE_COUNT
};

/** @brief Index of Fast Pair battery component. */
enum bt_fast_pair_battery_comp {
	/** Left bud. */
	BT_FAST_PAIR_BATTERY_COMP_LEFT_BUD = 0,

	/** Right bud. */
	BT_FAST_PAIR_BATTERY_COMP_RIGHT_BUD = 1,

	/** Case. */
	BT_FAST_PAIR_BATTERY_COMP_BUD_CASE = 2,

	/** Number of battery values. */
	BT_FAST_PAIR_BATTERY_COMP_COUNT
};

/** @brief Fast Pair advertising config. Used to generate advertising packet. */
struct bt_fast_pair_adv_config {
	/** Fast Pair advertising mode. */
	enum bt_fast_pair_adv_mode adv_mode;

	/** Fast Pair advertising battery mode. */
	enum bt_fast_pair_adv_battery_mode adv_battery_mode;
};

/** @brief Fast Pair battery component descriptor. */
struct bt_fast_pair_battery {
	/** Battery status. True means that battery is charging and False means that battery is not
	 *  charging.
	 */
	bool charging;

	/** Battery level ranging from 0 to 100 percent. Use
	 *  @ref BT_FAST_PAIR_BATTERY_LEVEL_UNKNOWN for unknown.
	 */
	uint8_t level;
};

/** Fast Pair information callback descriptor. */
struct bt_fast_pair_info_cb {
	/** Notify that the Account Key has been written.
	 *
	 * This information can be used for example to update the Fast Pair advertising
	 * data in the not discoverable mode. Due to execution context constraints
	 * (Bluetooth RX thread), it is not recommended to block for long periods
	 * of time in this callback.
	 *
	 *  @param conn Connection object that wrote the Account Key.
	 */
	void (*account_key_written)(struct bt_conn *conn);
};

/** Get Fast Pair advertising data buffer size.
 *
 * @param[in] fp_adv_config	Fast Pair advertising config.
 *
 * @return Fast Pair advertising data buffer size in bytes if the operation was successful.
 *         Otherwise zero is returned.
 */
size_t bt_fast_pair_adv_data_size(struct bt_fast_pair_adv_config fp_adv_config);

/** Fill Bluetooth advertising packet with Fast Pair advertising data.
 *
 * Provided buffer will be used in bt_data structure. The data must be valid while the structure is
 * in use.
 *
 * The buffer size must be at least @ref bt_fast_pair_adv_data_size. Caller shall also make sure
 * that Account Key write from a connected Fast Pair Seeker would not preempt generating Fast Pair
 * not discoverable advertising data. To achieve this, this function and
 * @ref bt_fast_pair_adv_data_size should be called from context with cooperative priority.
 *
 * @param[out] adv_data		Pointer to the Bluetooth advertising data structure to be filled.
 * @param[out] buf		Pointer to the buffer used to store Fast Pair advertising data.
 * @param[in]  buf_size		Size of the buffer used to store Fast Pair advertising data.
 * @param[in]  fp_adv_config	Fast Pair advertising config.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_adv_data_fill(struct bt_data *adv_data, uint8_t *buf, size_t buf_size,
			       struct bt_fast_pair_adv_config fp_adv_config);

/** Enable or disable Fast Pair pairing mode.
 *
 * Pairing mode must be enabled if discoverable Fast Pair advertising is used.
 * Pairing mode must be disabled if not discoverable Fast Pair advertising is used.
 *
 * It is user responsibility to make sure that proper pairing mode is used before advertising is
 * started. Fast Pair pairing mode is enabled by default.
 *
 * @param[in] pairing_mode Boolean indicating if device is in pairing mode.
 */
void bt_fast_pair_set_pairing_mode(bool pairing_mode);

/** Check Account Key presence.
 *
 * Caller shall make sure that Account Key write from a connected Fast Pair Seeker would not preempt
 * this function call. Otherwise invalid value may be returned.
 *
 * The function always returns false if called before Zephyr settings are loaded.
 *
 * @return True if device already has an Account Key, false otherwise.
 */
bool bt_fast_pair_has_account_key(void);

/** Set or update specified battery value.
 *
 * Battery values are used to generate advertising packet. To include battery values in advertising
 * packet this function must be called before @ref bt_fast_pair_adv_data_fill. It is user
 * responsibility to update battery value and regenerate advertising packet when battery value
 * change.
 *
 * @param[in] battery_comp	Battery component.
 * @param[in] battery		Battery value.
 *
 * @retval 0 on success.
 * @retval -EINVAL if battery value is invalid.
 */
int bt_fast_pair_battery_set(enum bt_fast_pair_battery_comp battery_comp,
			     struct bt_fast_pair_battery battery);

/** Register Fast Pair information callbacks.
 *
 *  This function registers an instance of information callbacks. The registered instance needs to
 *  persist in the memory after this function exits, as it is used directly without the copy
 *  operation. It is possible to register only one instance of callbacks with this API.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_fast_pair_info_cb_register(const struct bt_fast_pair_info_cb *cb);

/** Perform a reset to the default factory settings for Google Fast Pair Service.
 *
 * Clears the Fast Pair storage. If the reset operation is interrupted by system reboot or power
 * outage, the reset is automatically resumed at the stage of loading the Fast Pair storage.
 * It prevents the Fast Pair storage from ending in unwanted state after the reset interruption.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_factory_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_H_ */
