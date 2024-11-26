/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/bluetooth/cs.h>

void estimate_distance(struct net_buf_simple *local_steps, struct net_buf_simple *peer_steps,
		       uint8_t n_ap, enum bt_conn_le_cs_role role);
