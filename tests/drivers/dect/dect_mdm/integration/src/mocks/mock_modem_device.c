/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Mock modem device for DECT L2 sink integration tests.
 * When CONFIG_MODEM_CELLULAR is enabled, dect_net_l2_sink.c uses
 * DEVICE_DT_GET(DT_ALIAS(modem)). This stub driver instantiates a device
 * for the "modem_mock" node defined in boards/native_sim.overlay so that
 * the alias resolves and the sink builds/runs without real cellular hardware.
 * Same pattern as location lib tests (nordic_wlan0 / DEVICE_DT_INST_DEFINE).
 */

#include <zephyr/device.h>

#if defined(CONFIG_MODEM_CELLULAR)

#define DT_DRV_COMPAT dect_modem_mock

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

#endif /* CONFIG_MODEM_CELLULAR */
