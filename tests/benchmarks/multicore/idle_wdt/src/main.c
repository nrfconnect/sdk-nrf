/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_wdt, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

#define WDT_WINDOW_MAX	(200)

/* Watchdog related variables */
static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static int my_wdt_channel;

/* No init section will contain WDT_TAG if watchdog has fired */
#define WDT_TAG					(12345678U)

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
#define NOINIT_SECTION ".dtcm_noinit.test_wdt"
#else
#define NOINIT_SECTION ".noinit.test_wdt"
#endif
static volatile uint32_t wdt_status __attribute__((section(NOINIT_SECTION)));

#define SHM_START_ADDR		(DT_REG_ADDR(DT_NODELABEL(cpuapp_cpurad_ipc_shm)))
volatile static uint32_t *shared_var = (volatile uint32_t *) SHM_START_ADDR;
#define HOST_IS_READY	(1)
#define REMOTE_IS_READY	(2)

/* Variables used to make CPU active for ~1 second */
static struct k_timer my_timer;
static bool timer_expired;

static void wdt_int_cb(const struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	wdt_status = WDT_TAG;
}

void my_timer_handler(struct k_timer *dummy)
{
	timer_expired = true;
}

int main(void)
{
	int ret;
	int counter = 0;

	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret, "Error: GPIO Device not ready");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(ret == 0, "Could not configure led GPIO");

	LOG_INF("Multicore idle_wdt test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Main sleeps for %d ms", CONFIG_TEST_SLEEP_DURATION_MS);
	LOG_INF("Shared memory at %p", (void *) shared_var);

	k_timer_init(&my_timer, my_timer_handler, NULL);

	ret = device_is_ready(my_wdt_device);
	if (!ret) {
		LOG_ERR("WDT device %s is not ready", my_wdt_device->name);
	}
	__ASSERT(ret, "WDT device %s is not ready\n", my_wdt_device->name);

	/* When watchdog fires, variable wdt_status is set to the value of WDT_TAG
	 * in WDT callback wdt_int_cb(). Then, target is reset.
	 * Check value of wdt_status to prevent reset loop.
	 */
	if (wdt_status == WDT_TAG) {
		LOG_INF("Watchod has fired");
		/* Pinreset shall "recover" test to initial state */
		wdt_status = 0U;
		__ASSERT(false, "Watchod has fired\n");
	} else {
		LOG_INF("Reset wasn't due to watchdog.");
	}

	LOG_INF("wdt_status = %u", wdt_status);
	/* Clear flag that is set when the watchdog fires */
	wdt_status = 0U;
	LOG_INF("wdt_status = %u", wdt_status);

	/* Configure Watchdog */
	m_cfg_wdt0.callback = wdt_int_cb;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.min = 0U;
	m_cfg_wdt0.window.max = WDT_WINDOW_MAX;
	my_wdt_channel = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
	if (my_wdt_channel < 0) {
		LOG_ERR("wdt_install_timeout() returned %d", my_wdt_channel);
		__ASSERT(false, "wdt_install_timeout() returned %d\n", my_wdt_channel);
	}

#if defined(CONFIG_TEST_SYNCHRONIZE_CORES)
/* Synchronize Remote core with Host core */
#if !defined(CONFIG_TEST_ROLE_REMOTE)
	LOG_DBG("HOST starts");
	*shared_var = HOST_IS_READY;
	sys_cache_data_flush_range((void *) shared_var, sizeof(*shared_var));
	LOG_DBG("HOST wrote HOST_IS_READY: %u", *shared_var);
	while (*shared_var != REMOTE_IS_READY) {
		k_msleep(1);
		sys_cache_data_invd_range((void *) shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_INF("HOST continues");
#else /* !defined(CONFIG_TEST_ROLE_REMOTE) */
	LOG_DBG("REMOTE starts");
	while (*shared_var != HOST_IS_READY) {
		k_msleep(1);
		sys_cache_data_invd_range((void *) shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_DBG("REMOTE found that HOST_IS_READY");
	*shared_var = REMOTE_IS_READY;
	sys_cache_data_flush_range((void *) shared_var, sizeof(*shared_var));
	LOG_DBG("REMOTE wrote REMOTE_IS_READY: %u", *shared_var);
	LOG_INF("REMOTE continues");
#endif /* !defined(CONFIG_TEST_ROLE_REMOTE) */
#endif /* defined(CONFIG_TEST_SYNCHRONIZE_CORES) */

	/* Start Watchdog */
	ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG | WDT_OPT_PAUSE_IN_SLEEP);
	if (ret < 0) {
		LOG_ERR("wdt_setup() returned %d", ret);
		__ASSERT(false, "wdt_setup() returned %d\n", ret);
	}

	/* Run test forever */
	while (1) {
		timer_expired = false;

		/* start a one-shot timer that expires after 1 second */
		k_timer_start(&my_timer, K_MSEC(1000), K_NO_WAIT);

		/* Keep CPU active for ~ 1 second */
		while (!timer_expired) {
			ret = wdt_feed(my_wdt_device, my_wdt_channel);
			if (ret < 0) {
				LOG_ERR("wdt_feed() returned: %d", ret);
				/* Stop watchdog */
				wdt_disable(my_wdt_device);
				__ASSERT(false, "wdt_feed() returned: %d\n", ret);
			}
			k_busy_wait(10000);
			k_yield();
		}

		LOG_INF("Run %d", counter);
		counter++;

		/* Sleep / enter low power state
		 * Watchdog was started with option WDT_OPT_PAUSE_IN_SLEEP thus
		 * it shall not reset the core during sleep.
		 */
		gpio_pin_set_dt(&led, 0);
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
		gpio_pin_set_dt(&led, 1);
	}

	return 0;
}
