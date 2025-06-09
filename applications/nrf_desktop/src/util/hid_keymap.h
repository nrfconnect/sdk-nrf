/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief HID keymap header.
 */

#ifndef _HID_KEYMAP_H_
#define _HID_KEYMAP_H_

/**
 * @defgroup hid_keymap HID keymap
 * @brief Utility that handles mapping key IDs to HID usage IDs.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hid_report_desc.h"

/** @brief HID keymap entry. */
struct hid_keymap {
	uint16_t key_id;	/**< Application-specific key ID. */
	uint16_t usage_id;	/**< Assigned HID usage ID. */
	uint8_t report_id;	/**< Assigned HID report ID. */
};

/**
 * @brief Initialize HID keymap
 *
 * If assertions (@kconfig{CONFIG_ASSERT}) are enabled, the function validates if the ``hid_keymap``
 * array defined as part of the configuration is sorted ascending by key ID. The array must be
 * sorted, because HID keymap utility uses binary search to speed up searching through the array.
 *
 * The function must be called before using other HID keymap APIs. The function can be called
 * multiple times.
 */
void hid_keymap_init(void);

/**
 * @brief Get mapping from key ID to HID usage ID.
 *
 * The function maps an application-specific key ID to HID keymap entry containing HID report ID
 * and HID usage ID.
 *
 * @param[in] key_id		Application-specific key ID.
 *
 * @return HID keymap entry if mapping for the key ID is defined, NULL otherwise.
 */
const struct hid_keymap *hid_keymap_get(uint16_t key_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /*_HID_KEYMAP_H_ */
