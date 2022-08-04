/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DM_RPC_COMMON_H_
#define DM_RPC_COMMON_H_

#include <zephyr/bluetooth/addr.h>
#include <nrf_dm.h>

/** @brief Command IDs used in inter-core communication. */
enum dm_rpc_cmd {
	DM_INIT_RPC_CMD,
	DM_REQUEST_ADD_RPC_CMD,
	DM_END_PROCESS_RPC_CMD,
};

/** @brief Data structure shared between cores.
 *         Network core allocates and populates it with data.
 *         Application core performs calculations based on the data.
 */
struct dm_rpc_process_data {
	nrf_dm_report_t report;
	bt_addr_le_t bt_addr;
};

#endif /* DM_RPC_COMMON_H_ */
