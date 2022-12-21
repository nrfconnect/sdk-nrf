/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <modem/pdn.h>
#include <modem/lte_lc.h>

#include "message_channel.h"

/* Register log module */
LOG_MODULE_REGISTER(network, CONFIG_MQTT_SAMPLE_NETWORK_LOG_LEVEL);

/* This module does not subscribe to any channels */

static void network_task(void)
{
	enum network_status status = NETWORK_CONNECTED;

	/* Assume that the emulator is always connected to the network */
	int err = zbus_chan_pub(&NETWORK_CHAN, &status, K_SECONDS(1));

	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
}

K_THREAD_DEFINE(network_task_id,
		CONFIG_MQTT_SAMPLE_NETWORK_THREAD_STACK_SIZE,
		network_task, NULL, NULL, NULL, 3, 0, 0);
