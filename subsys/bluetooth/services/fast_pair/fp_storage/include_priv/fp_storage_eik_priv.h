/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_EIK_PRIV_H_
#define _FP_STORAGE_EIK_PRIV_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/settings/settings.h>

/**
 * @defgroup fp_storage_eik_priv Fast Pair storage of Ephemeral Identity Key (EIK) module private
 * @brief Private data of the Fast Pair storage of Ephemeral Identity Key (EIK) module
 *
 * Private definitions of the Fast Pair storage of Ephemeral Identity Key (EIK) module.
 * Used only by the module and by the unit test.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Settings subtree name for Fast Pair EIK storage. */
#define SETTINGS_EIK_SUBTREE_NAME "fp_eik"

/** Settings key name for EIK. */
#define SETTINGS_EIK_KEY_NAME "eik"

/** String used as a separator in settings keys. */
#define SETTINGS_NAME_SEPARATOR_STR "/"

/** Full settings key name for EIK (including subtree name). */
#define SETTINGS_EIK_FULL_NAME \
	(SETTINGS_EIK_SUBTREE_NAME SETTINGS_NAME_SEPARATOR_STR SETTINGS_EIK_KEY_NAME)

BUILD_ASSERT(SETTINGS_NAME_SEPARATOR == '/');

/** Clear storage data loaded to RAM. */
void fp_storage_eik_ram_clear(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_EIK_PRIV_H_ */
