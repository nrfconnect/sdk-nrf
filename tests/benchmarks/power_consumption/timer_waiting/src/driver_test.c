/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>

#define ALARM_CHANNEL_ID (0)
const struct device *const counter_dev = DEVICE_DT_GET(DT_ALIAS(counter));
struct counter_alarm_cfg counter_cfg;

void counter_handler(const struct device *counter_dev, uint8_t chan_id,
	uint32_t ticks, void *user_data)
{
	k_busy_wait(500000);
	counter_stop(counter_dev);
}

int main(void)
{
	while (1) {
		k_msleep(1500);
		counter_start(counter_dev);

		counter_cfg.flags = 0;
		counter_cfg.ticks = counter_us_to_ticks(counter_dev, 1000000UL);
		counter_cfg.callback = counter_handler;
		counter_cfg.user_data = &counter_cfg;

		counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, &counter_cfg);
		k_msleep(1500);
	}

	return 0;
}
