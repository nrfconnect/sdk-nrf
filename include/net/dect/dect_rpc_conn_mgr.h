/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_RPC_CONN_MGR_H_
#define DECT_RPC_CONN_MGR_H_

#include <zephyr/net/conn_mgr_connectivity_impl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Context type for DECT NR+ RPC client connectivity backend. */
#define CONNECTIVITY_DECT_RPC_MGMT_CTX_TYPE void *

/**
 * @brief Associate the DECT NR+ RPC client conn_mgr backend with the RPC net device.
 *
 * @param dev_id Network device ID (@c dect_rpc from @c NET_DEVICE_INIT).
 */
#define CONNECTIVITY_DECT_RPC_MGMT_BIND(dev_id)                                                  \
	CONN_MGR_CONN_DECLARE_PUBLIC(CONNECTIVITY_DECT_RPC_MGMT);                                  \
	CONN_MGR_BIND_CONN(dev_id, CONNECTIVITY_DECT_RPC_MGMT)

#ifdef __cplusplus
}
#endif

#endif /* DECT_RPC_CONN_MGR_H_ */
