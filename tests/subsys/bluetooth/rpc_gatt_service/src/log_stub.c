/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Stub implementation for logging module to avoid linker errors
 * when testing without full BT RPC stack
 */

#include <zephyr/logging/log.h>

/* Stub the log constant structure that's normally generated */
LOG_MODULE_REGISTER(BT_RPC, 4);
