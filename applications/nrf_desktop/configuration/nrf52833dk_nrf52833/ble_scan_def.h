/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This configuration file defines filters for BLE central (based on BLE
 * names). Used only by ble_scan module.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} ble_scan_include_once;

#include "ble_event.h"

static const char *peer_name[] = {
	[PEER_TYPE_MOUSE] = "Mouse nRF52 Desktop",
	[PEER_TYPE_KEYBOARD] = "Keyboard nRF52 Desktop",
};
