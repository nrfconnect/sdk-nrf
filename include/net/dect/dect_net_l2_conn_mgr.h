/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_NET_L2_CONN_MGR_H_
#define DECT_NET_L2_CONN_MGR_H_

#include <zephyr/net/conn_mgr_connectivity_impl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Context type for generic DECT_MGMT connectivity backend.
 */
#define CONNECTIVITY_DECT_MGMT_CTX_TYPE void *

/**
 * @brief Associate the generic DECT_MGMT implementation with a network device.
 *
 * @param dev_id Network device ID.
 */
#define CONNECTIVITY_DECT_MGMT_BIND(dev_id)                                                        \
	CONN_MGR_CONN_DECLARE_PUBLIC(CONNECTIVITY_DECT_MGMT);                                      \
	CONN_MGR_BIND_CONN(dev_id, CONNECTIVITY_DECT_MGMT)

#ifdef __cplusplus
}
#endif

#endif /* DECT_NET_L2_CONN_MGR_H_ */
