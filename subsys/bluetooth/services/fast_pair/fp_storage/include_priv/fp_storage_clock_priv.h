/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_CLOCK_PRIV_H_
#define _FP_STORAGE_CLOCK_PRIV_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/settings/settings.h>

/**
 * @defgroup fp_storage_clock_priv Fast Pair storage of Beacon Clock module private
 * @brief Private data of the Fast Pair storage of Beacon Clock module
 *
 * Private definitions of the Fast Pair storage of Beacon Clock module. Used only by the module
 * and by the unit test.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Settings subtree name for Fast Pair Beacon Clock storage. */
#define SETTINGS_CLOCK_SUBTREE_NAME "fp_clk"

/** Settings key name for Beacon Clock. */
#define SETTINGS_CLOCK_KEY_NAME "clk"

/** String used as a separator in settings keys. */
#define SETTINGS_NAME_SEPARATOR_STR "/"

/** Full settings key name for Beacon Clock (including subtree name). */
#define SETTINGS_CLOCK_FULL_NAME \
	(SETTINGS_CLOCK_SUBTREE_NAME SETTINGS_NAME_SEPARATOR_STR SETTINGS_CLOCK_KEY_NAME)

BUILD_ASSERT(SETTINGS_NAME_SEPARATOR == '/');

/** Clear storage data loaded to RAM. */
void fp_storage_clock_ram_clear(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_CLOCK_PRIV_H_ */
