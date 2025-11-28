/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Test configuration header - defines config values needed for unit testing
 * without full BT RPC stack
 *
 * This header is included via -include flag to ensure it's processed before
 * any source files that need these definitions.
 */

#ifndef TEST_CONFIG_H_
#define TEST_CONFIG_H_

/* Undefine first in case they're defined elsewhere, then define our test values */
#ifdef CONFIG_BT_RPC_GATT_SRV_MAX
#undef CONFIG_BT_RPC_GATT_SRV_MAX
#endif
#define CONFIG_BT_RPC_GATT_SRV_MAX 10

#ifdef CONFIG_BT_RPC_LOG_LEVEL
#undef CONFIG_BT_RPC_LOG_LEVEL
#endif
#define CONFIG_BT_RPC_LOG_LEVEL 4

#endif /* TEST_CONFIG_H_ */
