/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ADVERTISING_H_
#define ADVERTISING_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Invoked when advertising stops due to duration timeout (no connection).
 *
 * @note The Bluetooth stack may invoke this from a cooperative context.
 */
typedef void (*advertising_terminated_cb_t)(void);

/**
 * @brief Advertising module callbacks.
 *
 * Keep this structure for future extensions; new fields must be appended.
 */
struct advertising_cb {
	/** Called when connectable or non-connectable advertising times out. */
	advertising_terminated_cb_t terminated;
};

/**
 * @brief Initialize the advertising module.
 *
 * Must be called after @ref bt_enable. Creates resources (for example, extended advertising set)
 * when @kconfig{CONFIG_BT_EXT_ADV} is enabled; otherwise uses legacy advertising.
 *
 * Durations and intervals are taken from Kconfig:
 * - @kconfig{CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_DURATION}
 * - @kconfig{CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_DURATION}
 * - @kconfig{CONFIG_BT_POWER_PROFILING_NFC_ADV_DURATION}
 * - @kconfig{CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MIN} / _MAX
 * - @kconfig{CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MIN} / _MAX
 *
 * @param[in] cb Callbacks. Must remain valid for the lifetime of the module.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p cb is NULL or a required callback is missing.
 * @retval -EALREADY Already initialized.
 * @retval Negative errno Other error from the implementation.
 */
int advertising_init(const struct advertising_cb *cb);

/**
 * @brief Start connectable advertising.
 *
 * Stops any ongoing advertising first. Duration is
 * @kconfig{CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_DURATION}
 * (N × 10 ms units for each Zephyr API).
 *
 * @retval 0 Success.
 * @retval -EINVAL Module not initialized.
 * @retval Negative errno Error from the Bluetooth stack or implementation.
 */
int advertising_start_connectable(void);

/**
 * @brief Start connectable advertising for the NFC handover path.
 *
 * Same as @ref advertising_start_connectable except duration is
 * @kconfig{CONFIG_BT_POWER_PROFILING_NFC_ADV_DURATION}.
 *
 * @retval 0 Success.
 * @retval -EINVAL Module not initialized.
 * @retval Negative errno Error from the Bluetooth stack or implementation.
 */
int advertising_start_connectable_nfc(void);

/**
 * @brief Start non-connectable advertising.
 *
 * Stops any ongoing advertising first. Duration is
 * @kconfig{CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_DURATION}.
 *
 * @retval 0 Success.
 * @retval -EINVAL Module not initialized.
 * @retval Negative errno Error from the Bluetooth stack or implementation.
 */
int advertising_start_non_connectable(void);

/**
 * @brief Stop advertising.
 *
 * @retval 0 Success (including if advertising was already stopped).
 * @retval Negative errno Error from the Bluetooth stack or implementation.
 */
int advertising_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* ADVERTISING_H_ */
