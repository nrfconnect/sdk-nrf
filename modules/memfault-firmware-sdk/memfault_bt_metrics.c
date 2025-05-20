/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>

#include <memfault/metrics/metrics.h>

#include "memfault_ncs_metrics.h"

LOG_MODULE_DECLARE(memfault_ncs_metrics, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

static atomic_t connection_count = ATOMIC_INIT(0);

#if CONFIG_MEMFAULT_NCS_STACK_METRICS
static struct memfault_ncs_metrics_thread metrics_threads[] = {
	{
		.thread_name = "BT RX",
		.key = MEMFAULT_METRICS_KEY(ncs_bt_rx_unused_stack)
	},
	{
		.thread_name = "BT TX",
		.key = MEMFAULT_METRICS_KEY(ncs_bt_tx_unused_stack)
	}
};
#endif /* CONFIG_MEMFAULT_NCS_STACK_METRICS */


void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	atomic_val_t count = atomic_get(&connection_count);

	if (conn_err == 0) {
		if (count == 0) {
			err = MEMFAULT_METRIC_TIMER_START(ncs_bt_connection_time_ms);
			if (err) {
				LOG_WRN("Failed to start memfault ncs_bt_connection_time_ms timer, "
				"err: %d", err);
			}
		}

		atomic_inc(&connection_count);
	}
}

void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;
	atomic_val_t count = atomic_get(&connection_count);

	__ASSERT_NO_MSG(count > 0);

	if (count == 1) {
		err = MEMFAULT_METRIC_TIMER_STOP(ncs_bt_connection_time_ms);
		if (err) {
			LOG_WRN("Failed to stop memfault ncs_bt_connection_time_ms timer, err: %d",
				err);
		}
	}

	atomic_dec(&connection_count);
}

BT_CONN_CB_DEFINE(conn_callback) = {
	.connected = connected,
	.disconnected = disconnected
};

void bond_process(const struct bt_bond_info *info, void *user_data)
{
	uint8_t *count = user_data;

	(*count)++;
}

#if CONFIG_MEMFAULT_NCS_STACK_METRICS
void memfault_bt_metrics_init(void)
{
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(metrics_threads); i++) {
		err = memfault_ncs_metrics_thread_add(&metrics_threads[i]);
		if (err) {
			LOG_WRN("Failed to add thread: %s for stack unused place measurement, "
				"err: %d", metrics_threads[i].thread_name, err);
		}
	}
}

#else
void memfault_bt_metrics_init(void)
{
}
#endif /* CONFIG_MEMFAULT_NCS_STACK_METRICS */

void memfault_bt_metrics_update(void)
{
	int err;
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t id_count = ARRAY_SIZE(addrs);
	uint8_t bond_count = 0;

	bt_id_get(addrs, &id_count);

	for (size_t i = BT_ID_DEFAULT; i < id_count; i++) {
		bt_foreach_bond(i, bond_process, &bond_count);
	}

	LOG_DBG("Current bond count: %u", bond_count);

	err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_bt_bond_count, bond_count);
	if (err) {
		LOG_ERR("Failed to set the ncs_bt_bond_count metric, err: %d", err);
	}

	err = MEMFAULT_METRIC_SET_UNSIGNED(ncs_bt_connection_count, atomic_get(&connection_count));
	if (err) {
		LOG_ERR("Failed to set ncs_bt_connection_count metrics, err: %d", err);
	}
}
