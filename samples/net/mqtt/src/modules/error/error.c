/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/sys/reboot.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "message_channel.h"

/* Register log module */
LOG_MODULE_REGISTER(error, CONFIG_MQTT_SAMPLE_ERROR_LOG_LEVEL);

void error_callback(const struct zbus_channel *chan)
{
	if (&FATAL_ERROR_CHAN == chan) {
		if (IS_ENABLED(CONFIG_MQTT_SAMPLE_ERROR_REBOOT_ON_FATAL)) {
			LOG_ERR("FATAL error, rebooting");
			LOG_PANIC();
			sys_reboot(0);
		}
	}
}

/* Register listener - error_handler will be called everytime a message is sent to the
 * FATAL_ERROR_CHANNEL.
 */
ZBUS_LISTENER_DEFINE(error, error_callback);
