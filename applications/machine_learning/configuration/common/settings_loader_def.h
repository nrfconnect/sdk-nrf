/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This configuration file defines modules that need to be loaded before
 * calling settings_load.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */

const struct {} settings_loader_def_include_once;

#include <caf/events/module_state_event.h>


static void get_req_modules(struct module_flags *mf)
{
	module_flags_set_bit(mf, MODULE_IDX(main));
#ifdef CONFIG_CAF_BLE_ADV
	module_flags_set_bit(mf, MODULE_IDX(ble_adv));
#endif
}
