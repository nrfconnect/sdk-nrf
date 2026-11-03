/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Boots the test mock DECT NR+ HAL once per binary and drives L2 association state for
 * RPC server ztests.
 */

#ifndef SERVER_DECT_FIXTURE_H_
#define SERVER_DECT_FIXTURE_H_

#include <stdint.h>
#include <zephyr/net/net_if.h>

/** A fixed long_rd_id usable by tests that just need *some* valid parent to associate to. */
#define SERVER_DECT_FIXTURE_PARENT_LONG_RD_ID 0x56789ABCu

/**
 * Activate mock HAL + bring up dect0 (idempotent).
 *
 * @return The DECT network interface, or NULL on failure.
 */
struct net_if *server_dect_fixture_ensure_booted(void);

/** Simulate PT parent association at L2 (dect_net_l2_parent_association_created). */
int server_dect_fixture_associate(void);

/** Release that association (dect_net_l2_association_removed). */
int server_dect_fixture_release(void);

#endif /* SERVER_DECT_FIXTURE_H_ */
