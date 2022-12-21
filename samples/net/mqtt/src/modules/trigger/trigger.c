/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#if CONFIG_DK_LIBRARY
#include <dk_buttons_and_leds.h>
#endif /* CONFIG_DK_LIBRARY */

#include "message_channel.h"

/* Register log module */
LOG_MODULE_REGISTER(trigger, CONFIG_MQTT_SAMPLE_TRIGGER_LOG_LEVEL);

static void message_send(void)
{
	int not_used = -1;
	int err;

	err = zbus_chan_pub(&TRIGGER_CHAN, &not_used, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
}

#if CONFIG_DK_LIBRARY
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states) {
		message_send();
	}
}
#endif /* CONFIG_DK_LIBRARY */

static void trigger_task(void)
{
#if CONFIG_DK_LIBRARY
	int err = dk_buttons_init(button_handler);

	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
		SEND_FATAL_ERROR();
		return;
	}
#endif /* CONFIG_DK_LIBRARY */

	while (true) {
		message_send();
		k_sleep(K_SECONDS(CONFIG_MQTT_SAMPLE_TRIGGER_TIMEOUT_SECONDS));
	}
}

K_THREAD_DEFINE(trigger_task_id,
		CONFIG_MQTT_SAMPLE_TRIGGER_THREAD_STACK_SIZE,
		trigger_task, NULL, NULL, NULL, 3, 0, 0);
