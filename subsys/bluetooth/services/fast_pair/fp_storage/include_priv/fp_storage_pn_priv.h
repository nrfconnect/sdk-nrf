/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_PN_PRIV_H_
#define _FP_STORAGE_PN_PRIV_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/settings/settings.h>

/**
 * @defgroup fp_storage_pn_priv Fast Pair storage of Personalized Name module private
 * @brief Private data of the Fast Pair storage of Personalized Name module
 *
 * Private definitions of the Fast Pair storage of Personalized Name module. Used only by the module
 * and by the unit test.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Settings subtree name for Fast Pair Personalized Name storage. */
#define SETTINGS_PN_SUBTREE_NAME "fp_pn"

/** Settings key name for Personalized Name. */
#define SETTINGS_PN_KEY_NAME "pn"

/** String used as a separator in settings keys. */
#define SETTINGS_NAME_SEPARATOR_STR "/"

/** Full settings key name for Personalized Name (including subtree name). */
#define SETTINGS_PN_FULL_NAME \
	(SETTINGS_PN_SUBTREE_NAME SETTINGS_NAME_SEPARATOR_STR SETTINGS_PN_KEY_NAME)

BUILD_ASSERT(SETTINGS_NAME_SEPARATOR == '/');

/** Clear storage data loaded to RAM. */
void fp_storage_pn_ram_clear(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_PN_PRIV_H_ */
