/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/module_state_event.h>

/**
 * @file
 * @brief Configuration file for settings loader.
 *
 * This configuration file defines modules that need to be loaded before
 * calling settings_load.
 */

/**
 * @brief Structure to ensure single inclusion of this header file.
 *
 * This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} settings_loader_def_include_once;

/**
 * @brief Set required modules that need to be loaded.
 *
 * @param mf Pointer to the structure holding module flags.
 */
static void get_req_modules(struct module_flags *mf)
{
	module_flags_set_bit(mf, MODULE_IDX(main));
#ifdef CONFIG_CAF_BLE_STATE
	module_flags_set_bit(mf, MODULE_IDX(ble_state));
#endif
}
