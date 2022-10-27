/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This configuration file defines modules to be initialized before calling
 * settings_load.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */

const struct {} settings_loader_def_include_once;

#include <caf/events/module_state_event.h>


static inline void get_req_modules(struct module_flags *mf)
{
	module_flags_set_bit(mf, MODULE_IDX(main));
#if CONFIG_DESKTOP_HIDS_ENABLE
	module_flags_set_bit(mf, MODULE_IDX(hids));
#endif
#if CONFIG_DESKTOP_BAS_ENABLE
	module_flags_set_bit(mf, MODULE_IDX(bas));
#endif
#ifdef CONFIG_CAF_BLE_STATE
	module_flags_set_bit(mf, MODULE_IDX(ble_state));
#endif
#if CONFIG_DESKTOP_MOTION_SENSOR_ENABLE
	module_flags_set_bit(mf, MODULE_IDX(motion));
#endif
#if CONFIG_DESKTOP_BLE_SCAN_ENABLE
	module_flags_set_bit(mf, MODULE_IDX(ble_scan));
#endif
#if CONFIG_DESKTOP_FAILSAFE_ENABLE
	module_flags_set_bit(mf, MODULE_IDX(failsafe));
#endif
}
