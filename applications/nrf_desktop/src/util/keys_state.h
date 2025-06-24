/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _KEYS_STATE_H_
#define _KEYS_STATE_H_

/**
 * @file
 * @defgroup keys_state Keys state
 * @{
 * @brief Utility used to track state of active keys.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KEYS_MAX_CNT	CONFIG_DESKTOP_KEYS_STATE_KEY_CNT_MAX

/** @brief Structure used to track an active key. */
struct active_key {
	uint16_t id; /**< Key ID. */
	uint16_t press_cnt; /**< Keypress counter. */
};

/**@brief Keys state structure. */
struct keys_state {
	struct active_key keys[KEYS_MAX_CNT]; /**< Active keys. */
	uint8_t cnt; /**< Current number of active keys. */
	uint8_t cnt_max; /**< Maximum number of active keys. */
};

/**
 * @brief Initialize a keys state object.
 *
 * A keys state object must be initialized to track active keys.
 *
 * The function asserts if maximum number of active keys exceeds the value allowed by Kconfig
 * configuration.
 *
 * @param[in] ks		A keys state object.
 * @param[in] key_cnt_max	Maximum number of active keys.
 */
void keys_state_init(struct keys_state *ks, uint8_t key_cnt_max);

/**
 * @brief Notify keys state about a key press/release
 *
 * The utility can record multiple key presses for a given key ID. The same number of key releases
 * is needed for the utility to report release of the key.
 *
 * @param[in] ks		A keys state object.
 * @param[in] key_id		Key ID.
 * @param[in] pressed		Information if key was pressed or released.
 * @param[out] ks_changed	Information if state of the pressed keys changed.
 *
 * @retval 0 when successful.
 * @retval -ENOENT if key is released, but related key press was not recorded by the utility.
 * @retval -ENOBUFS if key is pressed and number of active keys exceeds the limit.
 */
int keys_state_key_update(struct keys_state *ks, uint16_t key_id, bool pressed, bool *ks_changed);

/**
 * @brief Clear keys state
 *
 * The function drops all of the tracked active key presses.
 *
 * @param[in] ks		A keys state object.
 */
void keys_state_clear(struct keys_state *ks);

/**
 * @brief Get keys state
 *
 * The function fills the provided array with key IDs of all active keys. The utility keeps key IDs
 * sorted in ascending order to ensure consistent results.
 *
 * The function asserts if size of the provided array is too small to handle the maximum number of
 * active keys.
 *
 * @param[in] ks	A keys state object.
 * @param[out] res	Array to be filled with key IDs of active keys.
 * @param[in] res_size	Size of the provided array.
 *
 * @return Number of keys written.
 */
size_t keys_state_keys_get(const struct keys_state *ks, uint16_t *res, size_t res_size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /*_KEYS_STATE_H_ */
