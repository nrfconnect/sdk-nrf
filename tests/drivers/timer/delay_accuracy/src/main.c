/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <nrfx_timer.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_TEST_START_HFXO)
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

static void clock_init(void)
{
#ifdef CONFIG_CLOCK_CONTROL_NRF
	int rv;
	struct onoff_client cli;

	sys_notify_init_spinwait(&cli.notify);
	rv = onoff_request(z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF), &cli);
	zassert_ok(rv);

	while (sys_notify_fetch_result(&cli.notify, &rv)) {
		/* pend until clock is up and running */
	}
	zassert_ok(rv);
#elif CONFIG_CLOCK_CONTROL_NRF54H_HFXO
	int rv;
	const struct device *hfxo = DEVICE_DT_GET(DT_NODELABEL(hfxo));

	rv = nrf_clock_control_request_sync(hfxo, NULL, K_MSEC(1000));
	zassert_ok(rv);
#else
	zassert_ok(false, "Should not get here.");
#endif /* CONFIG_CLOCK_CONTROL_NRF / CONFIG_CLOCK_CONTROL_NRF54H_HFXO */

	TC_PRINT("Clock has started\n");
}
#endif /* CONFIG_TEST_START_HFXO */

LOG_MODULE_REGISTER(delay_accuracy, LOG_LEVEL_INF);

nrfx_timer_t test_timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tst_timer)));

static void *suite_setup(void)
{
	int ret;

#if defined(CONFIG_TEST_START_HFXO)
	clock_init();
#endif

	/* Configure Timer. */
	uint32_t base_frequency = NRFX_TIMER_BASE_FREQUENCY_GET(&test_timer);
	nrfx_timer_config_t timer_config =
		NRFX_TIMER_DEFAULT_CONFIG(MHZ(CONFIG_TEST_TIMER_FREQUENCY_MHZ));
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.mode = NRF_TIMER_MODE_TIMER;

	TC_PRINT("Delay accuracy test on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("Timer base frequency: %d Hz\n", base_frequency);
	TC_PRINT("Timer configured frequency: %d Hz\n", timer_config.frequency);
	TC_PRINT("===================================================================\n");

	ret = nrfx_timer_init(&test_timer, &timer_config, NULL);
	zassert_true(ret == 0, "nrfx_timer_init() failed, ret = 0x%08X", ret);

	nrfx_timer_enable(&test_timer);

	return NULL;
}


static void test_k_msleep(int32_t ms)
{
	nrfx_timer_clear(&test_timer);
	k_msleep(ms);
	nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);
	uint32_t pulses = nrfx_timer_capture_get(&test_timer, NRF_TIMER_CC_CHANNEL0);
	uint32_t expected_pulses = nrfx_timer_ms_to_ticks(&test_timer, ms);
	/* Instead of using floating numbers, scale up by 100.
	 * Then, use integer division and modulo to get value
	 * with two digits after decimal point.
	 */
	uint32_t diff_percent_x100 = abs(expected_pulses - pulses) * 100 * 100 / expected_pulses;

	TC_PRINT("timer actual pulses = %u\n", pulses);
	TC_PRINT("expected pulses = %u\n", expected_pulses);
	TC_PRINT("diff = %u.%u [%%]\n", diff_percent_x100 / 100, diff_percent_x100 % 100);
	zassert_true(diff_percent_x100 < CONFIG_TEST_TOLERANCE_PERCENT * 100);
}

static void test_k_busy_wait(int32_t ms)
{
	nrfx_timer_clear(&test_timer);
	k_busy_wait(ms * 1000);
	nrfx_timer_capture(&test_timer, NRF_TIMER_CC_CHANNEL0);
	uint32_t pulses = nrfx_timer_capture_get(&test_timer, NRF_TIMER_CC_CHANNEL0);
	uint32_t expected_pulses = nrfx_timer_ms_to_ticks(&test_timer, ms);
	uint32_t diff_percent_x100 = abs(expected_pulses - pulses) * 100 * 100 / expected_pulses;

	TC_PRINT("timer actual pulses = %u\n", pulses);
	TC_PRINT("expected pulses = %u\n", expected_pulses);
	TC_PRINT("diff = %u.%u [%%]\n", diff_percent_x100 / 100, diff_percent_x100 % 100);
	zassert_true(diff_percent_x100 < CONFIG_TEST_TOLERANCE_PERCENT * 100);
}

ZTEST(delay_accuracy, test_k_busy_wait_01_sec)
{
	test_k_busy_wait(1000);
}

ZTEST(delay_accuracy, test_k_busy_wait_05_sec)
{
	test_k_busy_wait(5000);
}

ZTEST(delay_accuracy, test_k_msleep_01_sec)
{
	test_k_msleep(1000);
}

ZTEST(delay_accuracy, test_k_msleep_05_sec)
{
	test_k_msleep(5000);
}


ZTEST_SUITE(delay_accuracy, NULL, suite_setup, NULL, NULL, NULL);
