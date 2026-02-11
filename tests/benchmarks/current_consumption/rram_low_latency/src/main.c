/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rram_low_latency, LOG_LEVEL_INF);

#include <nrf_sys_event.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/cache.h>

/* Sleep duration in us. */
#define DELAY 10000

static const struct device *counter = DEVICE_DT_GET(DT_NODELABEL(sample_counter));
static const struct gpio_dt_spec ppk_sync = GPIO_DT_SPEC_GET(DT_ALIAS(ppk_sync), gpios);

static void counter_handler(const struct device *counter_dev, uint8_t ch_id,
			    uint32_t ticks, void *user_data)
{
	k_sem_give((struct k_sem *)user_data);
}

int main(void)
{
	struct counter_alarm_cfg alarm_cfg;
	struct k_sem sem;
	int event_handle;
	int ret;

	ret = gpio_is_ready_dt(&ppk_sync);
	if (!ret) {
		LOG_ERR("ppk_sync device not ready");
	}
	__ASSERT_NO_MSG(ret == true);

	ret = gpio_pin_configure_dt(&ppk_sync, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Could not configure ppk_sync GPIO (%d)", ret);
	}
	__ASSERT_NO_MSG(ret == 0);

	ret = k_sem_init(&sem, 0, 1);
	if (ret != 0) {
		LOG_ERR("k_sem_init() has failed (%d)", ret);
	}
	__ASSERT_NO_MSG(ret == 0);

	ret = counter_start(counter);
	if (ret != 0) {
		LOG_ERR("counter_start() has failed (%d)", ret);
	}
	__ASSERT_NO_MSG(ret == 0);

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter, DELAY);
	alarm_cfg.callback = counter_handler;
	alarm_cfg.user_data = &sem;

	/* Only for CI purposes. */
	k_busy_wait(1500000);

	LOG_DBG("Entering main loop");

	while (1) {
		sys_cache_instr_invd_all();
#if defined(CONFIG_RRAMC_POWER_MODE)
		event_handle = nrf_sys_event_register(0, true);
#elif defined(CONFIG_RRAMC_PPI_WAKEUP)
		/* Event is delayed because it is registered early and not as it should just
		 * before starting. Triggering event too early may result in RRAMC going back
		 * to sleep before actual event wakes up the CPU.
		 */
		uint32_t delay_adj = 8;

		event_handle = nrf_sys_event_register(DELAY + delay_adj, true);
		if (event_handle == 32) {
			LOG_ERR("nrf_sys_event_register() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(event_handle != 32);
#else
#error Unknown RRAMC_LOW_LATENCY_METHOD
#endif
		if (event_handle < 0) {
			LOG_ERR("nrf_sys_event_register() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(event_handle >= 0);

		/* Configure timer alarm (the event). */
		ret = counter_set_channel_alarm(counter, 0, &alarm_cfg);
		if (ret != 0) {
			LOG_ERR("counter_set_channel_alarm() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret >= 0);

		/* Indicate going to sleep. */
		gpio_pin_set_dt(&ppk_sync, 0);

		LOG_DBG("Going to sleep");

		/* Wait for the event. */
		ret = k_sem_take(&sem, K_USEC(DELAY + 100));
		if (ret != 0) {
			LOG_ERR("k_sem_take() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret >= 0);

		/* Indicate PM active state */
		gpio_pin_set_dt(&ppk_sync, 1);

		LOG_DBG("Woken up");

		ret = nrf_sys_event_unregister(event_handle, false);
		if (ret != 0) {
			LOG_ERR("nrf_sys_event_unregister() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		k_busy_wait(DELAY >> 1);
	}

	return 0;
}
