/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_RX_H
#define DECT_PHY_RX_H

#include <zephyr/kernel.h>
#include <stdint.h>

#include "dect_phy_shell.h"

#define DECT_PHY_RX_OP_RSSI_MEASURE 1
#define DECT_PHY_RX_OP_START	    2
#define DECT_PHY_RX_OP_CUSTOM	    3

int dect_phy_rx_phy_measure_rssi_op_add(struct dect_phy_rssi_scan_params *rssi_params);
int dect_phy_rx_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size);

/**************************************************************************************************/

typedef int (*dect_phy_rx_th_custom_op_execute_callback_t)(void *data);

struct dect_phy_rx_th_custom_op_execute_params {
	dect_phy_rx_th_custom_op_execute_callback_t op_execution_cb;
	void *data;
	size_t data_size;
};
int dect_phy_rx_msgq_custom_data_op_add(
	struct dect_phy_rx_th_custom_op_execute_params custom_op_params);

#endif /* DECT_PHY_RX_H */
