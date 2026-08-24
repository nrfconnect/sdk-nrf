/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <errno.h>

#include "dult_user.h"
#include "dult_user_slot.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_battery, CONFIG_DULT_LOG_LEVEL);

/** Special values for the battery level. */
#define DULT_BATTERY_LEVEL_MAX  (100U)

/** Special values for the battery type. */
#define DULT_BATTERY_TYPE_INVALID (0xFF)

/* Battery Type encoding */
enum dult_battery_type {
	/* Powered battery */
	DULT_BATTERY_TYPE_POWERED = 0x00,

	/* Non-rechargeable battery */
	DULT_BATTERY_TYPE_NON_RECHARGEABLE = 0x01,

	/* Rechargeable battery */
	DULT_BATTERY_TYPE_RECHARGEABLE = 0x02,
};

/* Battery Level encoding */
enum dult_battery_level {
	/* Full battery level */
	DULT_BATTERY_LEVEL_FULL = 0x00,

	/* Medium battery level */
	DULT_BATTERY_LEVEL_MEDIUM = 0x01,

	/* Low battery level */
	DULT_BATTERY_LEVEL_LOW = 0x02,

	/* Critical battery level */
	DULT_BATTERY_LEVEL_CRITICAL = 0x03,
};

/* Validate the battery level configuration. */
BUILD_ASSERT(CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR >=
	     CONFIG_DULT_BATTERY_LEVEL_LOW_THR);
BUILD_ASSERT(CONFIG_DULT_BATTERY_LEVEL_LOW_THR >=
	     CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR);

static bool is_enabled;

/* Per-user battery level, stored in the user slot so each registered user keeps
 * its own value and can set it during the pre-association window.
 */
static size_t battery_level_id = DULT_USER_SLOT_MEM_REF_ID_UNSET;

bool dult_battery_level_is_set(const struct dult_user *user)
{
	uint32_t val;

	if (battery_level_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		return false;
	}

	return dult_user_slot_mem_ref_val_get(user, battery_level_id, &val) == 0;
}

uint8_t dult_battery_type_encode(void)
{
	if (IS_ENABLED(CONFIG_DULT_BATTERY_TYPE_POWERED)) {
		return DULT_BATTERY_TYPE_POWERED;
	} else if (IS_ENABLED(CONFIG_DULT_BATTERY_TYPE_NON_RECHARGEABLE)) {
		return DULT_BATTERY_TYPE_NON_RECHARGEABLE;
	} else if (IS_ENABLED(CONFIG_DULT_BATTERY_TYPE_RECHARGEABLE)) {
		return DULT_BATTERY_TYPE_RECHARGEABLE;
	}

	/* Should not happen. */
	__ASSERT(0, "DULT Battery: battery type invalid configuration");
	return DULT_BATTERY_TYPE_INVALID;
}

uint8_t dult_battery_level_encode(const struct dult_user *user)
{
	uint32_t val;
	uint8_t battery_level;

	__ASSERT_NO_MSG(dult_battery_level_is_set(user));

	(void) dult_user_slot_mem_ref_val_get(user, battery_level_id, &val);
	battery_level = (uint8_t)val;

	__ASSERT(battery_level <= DULT_BATTERY_LEVEL_MAX,
		 "DULT Battery: incorrect battery level in %%");

	if (battery_level <= CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR) {
		return DULT_BATTERY_LEVEL_CRITICAL;
	} else if (battery_level <= CONFIG_DULT_BATTERY_LEVEL_LOW_THR) {
		return DULT_BATTERY_LEVEL_LOW;
	} else if (battery_level <= CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR) {
		return DULT_BATTERY_LEVEL_MEDIUM;
	} else {
		return DULT_BATTERY_LEVEL_FULL;
	}
}

int dult_battery_level_set(const struct dult_user *user, uint8_t percentage_level)
{
	int err;

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (percentage_level > DULT_BATTERY_LEVEL_MAX) {
		LOG_ERR("DULT Battery: incorrect battery level in %d %%", percentage_level);
		return -EINVAL;
	}

	if (battery_level_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		err = dult_user_slot_mem_ref_register(&battery_level_id);
		if (err) {
			return err;
		}
	}

	return dult_user_slot_mem_ref_val_set(user, battery_level_id, percentage_level);
}

int dult_battery_enable(void)
{
	if (is_enabled) {
		LOG_ERR("DULT Battery: already enabled");
		return -EALREADY;
	}

	/* The v1 API requires the battery level before enable (legacy behavior).
	 * The v2 API relies on the ANOS read-time verify instead, so enable may
	 * proceed before the level is set.
	 */
	if (!IS_ENABLED(CONFIG_DULT_API_VARIANT_V2) &&
	    !dult_battery_level_is_set(dult_user_get_associated())) {
		LOG_ERR("DULT Battery: battery level unset before the enable operation");
		return -EINVAL;
	}

	is_enabled = true;

	return 0;
}

int dult_battery_reset(void)
{
	if (!is_enabled) {
		LOG_ERR("DULT Battery: already disabled");
		return -EALREADY;
	}

	is_enabled = false;

	return 0;
}
