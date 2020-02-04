/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* This configuration file defines filters for BLE central (based on BLE short
 * names). Used only by ble_scan module.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} ble_scan_include_once;

#include "ble_event.h"

static const char *peer_type_short_name[] = {
	[PEER_TYPE_MOUSE] = "Mouse nRF52",
	[PEER_TYPE_KEYBOARD] = "Kbd nRF52",
};
