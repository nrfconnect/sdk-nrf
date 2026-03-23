/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../common/temp_monitor.h"
#include "../common/led_control.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_CPU_LOAD
#include <zephyr/debug/cpu_load.h>
#elif defined(CONFIG_NRF_CPU_LOAD)
#include <debug/cpu_load.h>
#endif

LOG_MODULE_REGISTER(app);

/** @brief Temperature control event handler.
 *
 * Handler reports periodically about average temperature. There are additional
 * events triggered when temperature exceeds the limits.
 */
static void temp_event_handler(uint8_t type, int16_t value)
{
	int decimal = value / 100;
	uint32_t frac = ((value < 0) ? -value : value) % 100;

	switch (type) {
	case TEMP_MONITOR_MSG_AVG:
		LOG_INF("Average temperature: %d.%d", decimal, frac);
		break;
	case TEMP_MONITOR_MSG_ABOVE:
		LOG_INF("Temperature above the limit: %d.%d", decimal, frac);
		break;
	case TEMP_MONITOR_MSG_BELOW:
		LOG_INF("Temperature below the limit: %d.%d", decimal, frac);
		break;
	default:
		break;
	}
}

int main(void)
{
	/* Sample the sensor at 1000 Hz, report average every 1000 measurements. Report if
	 * temperature is outside of 20'C-28'C range.
	 */
	struct temp_monitor_start temp_monitor_params = {
		.limith = 2800, .limitl = 2000, .report_avg_period = 1000, .odr = 1000};
	static const uint32_t period_on[] = {100, 500, 800};
	static const uint32_t period_off[] = {900, 500, 200};
	uint32_t cfg_idx = 0;
	int rv;

#ifdef CONFIG_CPU_LOAD
	(void)cpu_load_get(true);
#elif defined(CONFIG_NRF_CPU_LOAD)
	cpu_load_reset();
#endif

	if (IS_ENABLED(CONFIG_SAMPLE_LOCAL_TEMP_MONITOR)) {
		printk("Sample runs modules on the application core\n");
	} else {
		printk("Sample runs modules on the VPR core\n");
	}

	temp_monitor_set_handler(temp_event_handler);
	/*return 0;*/

	while (1) {
		rv = temp_monitor_start(&temp_monitor_params);
		if (rv < 0) {
			LOG_ERR("Temperature monitor start failed, err:%d", rv);
		}

		rv = led_control_start(period_on[cfg_idx], period_off[cfg_idx]);
		if (rv < 0) {
			LOG_ERR("Led control failed, err:%d", rv);
		}
		cfg_idx = (cfg_idx == ARRAY_SIZE(period_on) - 1) ? 0 : cfg_idx + 1;

		k_msleep(2500);

		rv = temp_monitor_stop();
		if (rv < 0) {
			LOG_ERR("Temperature monitor stop failed, err:%d", rv);
		}

		rv = led_control_stop();
		if (rv < 0) {
			LOG_ERR("Led control stop failed, err:%d", rv);
		}

#ifdef CONFIG_CPU_LOAD
		int load = cpu_load_get(true);

		printk("CPU load: %d.%d\n", load / 10, load % 10);
#elif defined(CONFIG_NRF_CPU_LOAD)
		int load = cpu_load_get();

		printk("CPU load: %d.%d\n", load / 1000, load % 1000);
#endif

		k_msleep(500);
	}

	return 0;
}
