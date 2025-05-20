/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <nrf_modem_dect_phy.h>

#include "desh_print.h"
#include "dect_phy_shell.h"
#include "dect_phy_scan.h"
#include "dect_phy_rx.h"

/**************************************************************************************************/

K_MSGQ_DEFINE(dect_phy_rx_th_op_event_msgq, sizeof(struct dect_phy_common_op_event_msgq_item), 10,
	      4);

/**************************************************************************************************/

#define DECT_PHY_RX_THREAD_STACK_SIZE 1024
#define DECT_PHY_RX_THREAD_PRIORITY   5

static void dect_phy_rx_th_op_handler_thread_fn(void)
{
	struct dect_phy_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_phy_rx_th_op_event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_PHY_RX_OP_START: {
			dect_phy_scan_rx_th_run((struct dect_phy_rx_cmd_params *)event.data);
			break;
		}
		case DECT_PHY_RX_OP_RSSI_MEASURE:
			dect_phy_scan_rssi_rx_th_run(
				(struct dect_phy_rssi_scan_params *)event.data);
			break;
		case DECT_PHY_RX_OP_CUSTOM: {
			struct dect_phy_rx_th_custom_op_execute_params *params =
				(struct dect_phy_rx_th_custom_op_execute_params *)event.data;
			int ret;

			if (params->op_execution_cb != NULL) {
				ret = params->op_execution_cb(params->data);
				if (ret) {
					desh_warn("DECT_PHY_RX_OP_CUSTOM failed: err %d", ret);
				}
			}
			break;
		}
		default:
			desh_warn("DECT RX: Unknown event %d received", event.id);
			break;
		}
		k_free(event.data);
	}
}

K_THREAD_DEFINE(dect_phy_rx_th, DECT_PHY_RX_THREAD_STACK_SIZE, dect_phy_rx_th_op_handler_thread_fn,
		NULL, NULL, NULL, K_PRIO_PREEMPT(DECT_PHY_RX_THREAD_PRIORITY), 0, 0);

/**************************************************************************************************/

int dect_phy_rx_phy_measure_rssi_op_add(struct dect_phy_rssi_scan_params *rssi_params)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.data = k_malloc(sizeof(struct dect_phy_rssi_scan_params));
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, rssi_params, sizeof(struct dect_phy_rssi_scan_params));

	event.id = DECT_PHY_RX_OP_RSSI_MEASURE;
	ret = k_msgq_put(&dect_phy_rx_th_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

/**************************************************************************************************/

int dect_phy_rx_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.data = k_malloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, data, data_size);

	event.id = event_id;
	ret = k_msgq_put(&dect_phy_rx_th_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

/**************************************************************************************************/

int dect_phy_rx_msgq_custom_data_op_add(
	struct dect_phy_rx_th_custom_op_execute_params custom_op_params)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;
	struct dect_phy_rx_th_custom_op_execute_params *evt_data;

	evt_data = k_malloc(sizeof(struct dect_phy_rx_th_custom_op_execute_params));
	if (evt_data == NULL) {
		return -ENOMEM;
	}
	evt_data->data = k_malloc(custom_op_params.data_size);
	if (evt_data->data == NULL) {
		k_free(evt_data);
		return -ENOMEM;
	}
	memcpy(evt_data->data, custom_op_params.data, custom_op_params.data_size);
	evt_data->op_execution_cb = custom_op_params.op_execution_cb;
	evt_data->data_size = custom_op_params.data_size;

	event.data = evt_data;
	event.id = DECT_PHY_RX_OP_CUSTOM;
	ret = k_msgq_put(&dect_phy_rx_th_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		k_free(evt_data->data);
		return -ENOBUFS;
	}
	return 0;
}
