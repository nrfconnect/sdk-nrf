/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_MANAGER_PRIV_H_
#define _FP_STORAGE_MANAGER_PRIV_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/settings/settings.h>

/**
 * @defgroup fp_storage_manager_priv Fast Pair storage manager module private
 * @brief Private data of the Fast Pair storage manager module
 *
 * Private definitions of the Fast Pair storage manager module. Used only by the module and
 * by the unit test.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Settings subtree name for Fast Pair storage manager storage. */
#define SETTINGS_SM_SUBTREE_NAME "fp_sm"

/** Settings key name for reset in progress flag. */
#define SETTINGS_RESET_IN_PROGRESS_NAME "reset"

/** String used as a separator in settings keys. */
#define SETTINGS_NAME_SEPARATOR_STR "/"

/** Full settings key name for reset in progress flag (including subtree name). */
#define SETTINGS_RESET_IN_PROGRESS_FULL_NAME \
	(SETTINGS_SM_SUBTREE_NAME SETTINGS_NAME_SEPARATOR_STR SETTINGS_RESET_IN_PROGRESS_NAME)

BUILD_ASSERT(SETTINGS_NAME_SEPARATOR == '/');

/** Clear storage data loaded to RAM.
 *
 * The function is used only by fp_storage unit test.
 */
void fp_storage_manager_ram_clear(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_MANAGER_PRIV_H_ */
