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

/** Value that denotes unknown battery level (see @ref bt_fast_pair_battery_value). */
#define BT_FAST_PAIR_UNKNOWN_BATTERY_LEVEL	0x7f

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

/** @brief Fast Pair advertising info. Used to generate advertising packet. */
struct bt_fast_pair_adv_info {
	/** Fast Pair advertising mode. */
	enum bt_fast_pair_adv_mode adv_mode;

	/** Fast Pair advertising battery mode. */
	enum bt_fast_pair_adv_battery_mode adv_battery_mode;
};

/** @brief Fast Pair battery value related to single battery */
struct bt_fast_pair_battery_value {
	/** Battery status. True means that battery is charging and False means that battery is not
	 *  charging.
	 */
	bool charging;

	/** Battery level ranging from 0 to 100 percent. Use @ref BT_FAST_PAIR_UNKNOWN_BATTERY_LEVEL
	 *  for unknown.
	 */
	uint8_t level;
};

/** @brief Fast Pair battery data related to all three batteries (left bud, right bud, case). */
struct bt_fast_pair_battery_data {
	/** Left bud. */
	struct bt_fast_pair_battery_value left_bud;

	/** Right bud. */
	struct bt_fast_pair_battery_value right_bud;

	/** Case. */
	struct bt_fast_pair_battery_value bud_case;
};

/** Get Fast Pair advertising data buffer size.
 *
 * @param[in] fp_adv_info	Fast Pair advertising info.
 *
 * @return Fast Pair advertising data buffer size in bytes if the operation was successful.
 *         Otherwise zero is returned.
 */
size_t bt_fast_pair_adv_data_size(struct bt_fast_pair_adv_info fp_adv_info);

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
 * @param[in]  fp_adv_info	Fast Pair advertising info.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_adv_data_fill(struct bt_data *adv_data, uint8_t *buf, size_t buf_size,
			       struct bt_fast_pair_adv_info fp_adv_info);

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

/** Set or update battery data values.
 *
 * Battery data values may be used to generate advertising packet.
 *
 * @param[in] battery_data Battery data values.
 *
 * @retval 0 on success.
 * @retval -EINVAL if battery data is invalid.
 */
int bt_fast_pair_set_battery_data(struct bt_fast_pair_battery_data battery_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_H_ */
