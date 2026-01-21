/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_canbus), okay)
#include <zephyr/drivers/can.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can, LOG_LEVEL_INF);

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static int rx_counter;

const struct can_frame test_std_frame = {
	.flags   = 0,
	.id      = 0x555,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

const struct can_filter test_std_filter = {
	.flags = 0U,
	.id = 0x555,
	.mask = CAN_STD_ID_MASK
};

void tx_callback(const struct device *dev, int error, void *user_data)
{
	char *sender = (char *)user_data;

	if (error != 0) {
		LOG_ERR("Sending failed [%d]\nSender: %s", error, sender);
	}
}

void rx_callback(const struct device *dev, struct can_frame *frame, void *user_data)
{
	rx_counter++;
	LOG_INF("Received CAN frame (# %d)", rx_counter);
}

/* CAN thread */
static void can_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	atomic_inc(&started_threads);

	if (!device_is_ready(can_dev)) {
		LOG_ERR("Device %s is not ready.", can_dev->name);
		atomic_inc(&completed_threads);
		return;
	}

	/* failsafe */
	(void)can_stop(can_dev);

	/* set loopback mode */
	ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	if (ret < 0) {
		LOG_ERR("can_set_mode() returned %d", ret);
		atomic_inc(&completed_threads);
		return;
	}

	ret = can_start(can_dev);
	if (ret < 0) {
		LOG_ERR("can_start() returned %d", ret);
		atomic_inc(&completed_threads);
		return;
	}

	/* Configure CAN RX filter */
	ret = can_add_rx_filter(can_dev, rx_callback, NULL, &test_std_filter);
	if (ret < 0) {
		LOG_ERR("can_add_rx_filter() returned %d", ret);
		atomic_inc(&completed_threads);
		return;
	}

	while (rx_counter < CAN_THREAD_COUNT_MAX) {
		/* Send CAN message */
		ret = can_send(can_dev, &test_std_frame, K_FOREVER, tx_callback, "CAN thread");
		if (ret < 0) {
			LOG_ERR("can_send() returned: %d", ret);
			atomic_inc(&completed_threads);
			return;
		}

		k_msleep(CAN_THREAD_SLEEP);
	}

	ret = can_stop(can_dev);
	if (ret < 0) {
		LOG_ERR("can_stop() returned %d", ret);
		atomic_inc(&completed_threads);
		return;
	}

	LOG_INF("CAN thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_can_id, CAN_THREAD_STACKSIZE, can_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(CAN_THREAD_PRIORITY), 0, 0);

#else
#pragma message("CAN thread skipped due to missing node in the DTS")
#endif
