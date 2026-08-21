/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/pulse_meas.h>

LOG_MODULE_REGISTER(pulse_meas_sample, LOG_LEVEL_INF);

#define NUMBER_OF_MEASUREMENTS 16
#define NUMBER_OF_SERIES       10

K_MEM_SLAB_DEFINE(my_meas_slab, PULSE_MEAS_BLOCK_SIZE(NUMBER_OF_MEASUREMENTS), 128, 4);

static const struct device *const pulse_meas_dev = DEVICE_DT_GET(DT_NODELABEL(pulse_meas));

static atomic_t handler_cnt;

static void pulse_meas_handler(void *context)
{
	ARG_UNUSED(context);
	atomic_inc(&handler_cnt);
}

static struct pulse_meas_config pulse_meas_cfg = {
	.num_of_meas = NUMBER_OF_MEASUREMENTS,
	.pulse_type = PULSE_MEAS_PULSE_POSITIVE,
	.mode = PULSE_MEAS_MODE_CONTINUOUS,
	.pull_config = NRF_GPIO_PIN_NOPULL,
	.user_handler = pulse_meas_handler,
	.user_context = NULL,
};

int main(void)
{
	uint32_t *widths;
	int ret;
	uint32_t series_cnt = 0;

	printk("Pulse width measurement sample start\n");

	if (!device_is_ready(pulse_meas_dev)) {
		LOG_ERR("Pulse width measurement device is not ready");
		return 0;
	}

	ret = pulse_meas_configure(pulse_meas_dev, &pulse_meas_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure pulse width measurement: %d", ret);
		return 0;
	}

	ret = pulse_meas_start(pulse_meas_dev, &my_meas_slab);
	if (ret < 0) {
		LOG_ERR("Failed to start pulse width measurement: %d", ret);
		return 0;
	}

	k_msleep(100);
	pulse_meas_stop(pulse_meas_dev, false);
	printk("Stop requested, pending series: %u\n", pulse_meas_pending(pulse_meas_dev));

	while (series_cnt < NUMBER_OF_SERIES) {
		ret = pulse_meas_get(pulse_meas_dev, &widths);
		if (ret == -EAGAIN) {
			k_msleep(1);
			continue;
		}

		if (ret < 0) {
			if (pulse_meas_pending(pulse_meas_dev) == 0 && series_cnt > 0) {
				break;
			}

			if (ret == -EIO && series_cnt > 0) {
				break;
			}

			LOG_WRN("pulse_meas_get returned %d after %u series", ret, series_cnt);
			break;
		}

		printk("series[%u]: first=%u us last=%u us\n", series_cnt, widths[0],
		       widths[NUMBER_OF_MEASUREMENTS - 1]);
		pulse_meas_put(pulse_meas_dev, widths);
		series_cnt++;
	}

	while (pulse_meas_pending(pulse_meas_dev) > 0) {
		ret = pulse_meas_get(pulse_meas_dev, &widths);
		if (ret == -EAGAIN) {
			k_msleep(1);
			continue;
		}

		if (ret < 0) {
			LOG_WRN("pulse_meas_get returned %d while draining", ret);
			break;
		}

		printk("drain series[%u]: first=%u us last=%u us\n", series_cnt, widths[0],
		       widths[NUMBER_OF_MEASUREMENTS - 1]);
		pulse_meas_put(pulse_meas_dev, widths);
		series_cnt++;
	}

	printk("Measurement finished, series read: %u, callbacks: %u\n", series_cnt,
	       (unsigned int)atomic_get(&handler_cnt));

	while (true) {
		k_msleep(100);
	}
}
