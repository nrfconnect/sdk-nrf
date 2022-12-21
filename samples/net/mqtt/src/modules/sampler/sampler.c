/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

#define FORMAT_STRING "Hello MQTT! Current uptime is: %d"

/* Register log module */
LOG_MODULE_REGISTER(sampler, CONFIG_MQTT_SAMPLE_SAMPLER_LOG_LEVEL);

/* Register subscriber */
ZBUS_SUBSCRIBER_DEFINE(sampler, CONFIG_MQTT_SAMPLE_SAMPLER_MESSAGE_QUEUE_SIZE);

static void sample(void)
{
	struct payload payload = { 0 };
	uint32_t uptime = k_uptime_get_32();
	int err, len;

	/* The payload is user defined and can be sampled from any source.
	 * Default case is to populate a string and send it on the payload channel.
	 */

	len = snprintk(payload.string, sizeof(payload.string), FORMAT_STRING, uptime);
	if ((len < 0) || (len >= sizeof(payload))) {
		LOG_ERR("Failed to construct message, error: %d", len);
		SEND_FATAL_ERROR();
		return;
	}

	err = zbus_chan_pub(&PAYLOAD_CHAN, &payload, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error:%d", err);
		SEND_FATAL_ERROR();
	}
}

static void sampler_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&sampler, &chan, K_FOREVER)) {
		if (&TRIGGER_CHAN == chan) {
			sample();
		}
	}
}

K_THREAD_DEFINE(sampler_task_id,
		CONFIG_MQTT_SAMPLE_SAMPLER_THREAD_STACK_SIZE,
		sampler_task, NULL, NULL, NULL, 3, 0, 0);
