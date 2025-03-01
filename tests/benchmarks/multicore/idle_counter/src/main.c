/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

LOG_MODULE_REGISTER(idle_counter);

#define SHM_START_ADDR (DT_REG_ADDR(DT_NODELABEL(cpuapp_cpurad_ipc_shm)))
volatile static uint32_t *shared_var = (volatile uint32_t *)SHM_START_ADDR;
#define HOST_IS_READY	(1)
#define REMOTE_IS_READY (2)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

#define ALARM_CHANNEL_ID (0)
const struct device *const counter_dev = DEVICE_DT_GET(DT_ALIAS(counter));

static K_SEM_DEFINE(my_sem, 0, 1);

const uint32_t freq[] = {320, 256, 128, 64};

#if defined(CONFIG_GLOBAL_DOMAIN_CLOCK_FREQUENCY_SWITCHING)
/*
 * Set Global Domain frequency (HSFLL120)
 */
void set_global_domain_frequency(uint32_t freq)
{
	int err;
	int res;
	struct onoff_client cli;
	const struct device *hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(hsfll120));
	const struct nrf_clock_spec clk_spec_global_hsfll = {.frequency = MHZ(freq)};

	LOG_INF("Requested frequency [Hz]: %d", clk_spec_global_hsfll.frequency);
	sys_notify_init_spinwait(&cli.notify);
	err = nrf_clock_control_request(hsfll_dev, &clk_spec_global_hsfll, &cli);
	__ASSERT((err >= 0 && err < 3), "Wrong nrf_clock_control_request return code");
	do {
		err = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (err == -EAGAIN);
	__ASSERT(err == 0, "Wrong clock control request return code");
	__ASSERT(res == 0, "Wrong clock control request response");
}
#endif /* CONFIG_GLOBAL_DOMAIN_CLOCK_FREQUENCY_SWITCHING */

void counter_handler(const struct device *counter_dev, uint8_t chan_id, uint32_t ticks,
		     void *user_data)
{
	k_sem_give(&my_sem);
	counter_stop(counter_dev);
}

uint32_t start_timer(const struct device *counter_dev)
{
	uint32_t start_time;
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(counter_dev, CONFIG_TEST_SLEEP_DURATION_MS * 1000);
	counter_cfg.callback = counter_handler;
	counter_cfg.user_data = &counter_cfg;
	counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, &counter_cfg);

	counter_start(counter_dev);
	start_time = k_cycle_get_32();

	LOG_INF("Counter starts at %u cycle", start_time);

	return start_time;
}

void verify_timer(uint32_t start_time)
{
	uint32_t elapsed;
	int ret;

	ret = gpio_pin_set_dt(&led, 0);
	__ASSERT(ret == 0, "Unable to turn off LED");
	ret = k_sem_take(&my_sem, K_MSEC(CONFIG_TEST_SLEEP_DURATION_MS + 100));
	elapsed = k_cycle_get_32() - start_time;
	__ASSERT(ret == 0, "Timer callback not called");
	ret = gpio_pin_set_dt(&led, 1);
	__ASSERT(ret == 0, "Unable to turn on LED");
	LOG_INF("Elapsed %u cycles (%uus)", elapsed, k_cyc_to_us_ceil32(elapsed));
	__ASSERT(elapsed > k_ms_to_cyc_ceil32(CONFIG_TEST_SLEEP_DURATION_MS - 10) &&
			 elapsed < k_ms_to_cyc_ceil32(CONFIG_TEST_SLEEP_DURATION_MS + 10),
		 "expected time to elapse is 1s");
}

void sleep_with_state_indication(uint32_t sleep_duration_ms)
{
	int ret;

	ret = gpio_pin_set_dt(&led, 0);
	__ASSERT(ret == 0, "Unable to turn off LED");
	k_msleep(sleep_duration_ms);
	ret = gpio_pin_set_dt(&led, 1);
	__ASSERT(ret == 0, "Unable to turn on LED");
}

int main(void)
{
	uint32_t start_time;
	int ret;

	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret, "Error: GPIO Device not ready");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(ret == 0, "Could not configure led GPIO");

	/* Wait a bit to solve NRFS request timeout issue. */
	k_msleep(500);

	/* Synchronize Remote core with Host core */
#if !defined(CONFIG_TEST_ROLE_REMOTE)
	LOG_DBG("HOST starts");
	*shared_var = HOST_IS_READY;
	sys_cache_data_flush_range((void *)shared_var, sizeof(*shared_var));
	LOG_DBG("HOST wrote HOST_IS_READY: %u", *shared_var);
	while (*shared_var != REMOTE_IS_READY) {
		sys_cache_data_invd_range((void *)shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_DBG("HOST continues");
#else
	LOG_DBG("REMOTE starts");
	while (*shared_var != HOST_IS_READY) {
		sys_cache_data_invd_range((void *)shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_DBG("REMOTE found that HOST_IS_READY");
	*shared_var = REMOTE_IS_READY;
	sys_cache_data_flush_range((void *)shared_var, sizeof(*shared_var));
	LOG_DBG("REMOTE wrote REMOTE_IS_READY: %u", *shared_var);
	LOG_DBG("REMOTE continues");
#endif

	while (1) {
		sleep_with_state_indication(CONFIG_TEST_SLEEP_DURATION_MS);
		for (int i = 0; i < ARRAY_SIZE(freq); i++) {
			start_time = start_timer(counter_dev);
			k_msleep(100);
#if defined(CONFIG_GLOBAL_DOMAIN_CLOCK_FREQUENCY_SWITCHING)
			sleep_with_state_indication(CONFIG_TEST_SLEEP_DURATION_MS / 2 - 100);
			set_global_domain_frequency(freq[(i + 1) % ARRAY_SIZE(freq)]);
			k_msleep(10);
#endif /* CONFIG_GLOBAL_DOMAIN_CLOCK_FREQUENCY_SWITCHING */
			verify_timer(start_time);
		}
		ret = gpio_pin_set_dt(&led, 1);
		__ASSERT(ret == 0, "Unable to turn on LED");
		k_msleep(100);
	}

	return 0;
}
