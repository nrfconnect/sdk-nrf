/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hid_keymap.h"
#include CONFIG_DESKTOP_HID_KEYMAP_DEF_PATH

#include <stdint.h>
#include <stdlib.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hid_keymap, CONFIG_DESKTOP_HID_KEYMAP_LOG_LEVEL);

static bool initialized;


void hid_keymap_init(void)
{
	if (IS_ENABLED(CONFIG_ASSERT) && !initialized) {
		/* Validate the order of key IDs on the key map array. */
		for (size_t i = 1; i < ARRAY_SIZE(hid_keymap); i++) {
			__ASSERT(hid_keymap[i - 1].key_id < hid_keymap[i].key_id,
				 "The hid_keymap array must be sorted by key_id!");
		}

		/* Validate if report IDs are correct. */
		for (size_t i = 0; i < ARRAY_SIZE(hid_keymap); i++) {
			__ASSERT((hid_keymap[i].report_id != REPORT_ID_RESERVED) &&
				 (hid_keymap[i].report_id < REPORT_ID_COUNT),
				 "Invalid report ID used in hid_keymap!");
		}

		initialized = true;
	}
}

/* Compare Key ID in HID Keymap entries. */
static int hid_keymap_compare(const void *a, const void *b)
{
	const struct hid_keymap *p_a = a;
	const struct hid_keymap *p_b = b;

	return (p_a->key_id - p_b->key_id);
}

/** Translate Key ID to HID report ID and HID usage ID pair. */
const struct hid_keymap *hid_keymap_get(uint16_t key_id)
{
	static const struct hid_keymap *map_cache;

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYMAP_CACHE)) {
		/* Return cached mapping if possible. */
		if (map_cache->key_id == key_id) {
			return map_cache;
		}
	}

	struct hid_keymap key = {
		.key_id = key_id
	};

	const struct hid_keymap *map = bsearch(&key,
					       hid_keymap,
					       ARRAY_SIZE(hid_keymap),
					       sizeof(key),
					       hid_keymap_compare);

	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYMAP_CACHE)) {
		/* Update cached mapping. */
		map_cache = map;
	}

	return map;
}
