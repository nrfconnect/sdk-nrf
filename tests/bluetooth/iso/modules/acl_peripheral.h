/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ACL_PERIPHERAL_H
#define ACL_PERIPHERAL_H

#include <stdint.h>

typedef void (*sim_acl_recv_cb_t)(uint32_t last_count, uint32_t counts_fail,
				  uint32_t counts_success);

int acl_peripheral_init(sim_acl_recv_cb_t sim_acl_recv_cb);

#endif /* ACL_PERIPHERAL_H */
