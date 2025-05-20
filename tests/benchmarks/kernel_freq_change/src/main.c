/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

static void set_clock_frequency(uint32_t freq_mhz, const struct device *clk_dev)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;
	const struct nrf_clock_spec clk_spec = {.frequency = freq_mhz};

	TC_PRINT("Clock under test: %s\n", clk_dev->name);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, &clk_spec, &cli);
	TC_PRINT("nrf_clock_control_request return code: %d\n", ret);
	zassert_between_inclusive(ret, 0, 2);
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	TC_PRINT("Clock control request return value: %d\n", ret);
	TC_PRINT("Clock control request response code: %d\n", res);
	zassert_ok(ret);
	zassert_ok(res);
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	if (ret != -ENOSYS) {
		zassert_ok(ret);
		zassert_equal(rate, clk_spec.frequency);
	}
}

static void release_clock(uint32_t freq_mhz, const struct device *clk_dev)
{
	int ret = 0;
	const struct nrf_clock_spec clk_spec = {.frequency = freq_mhz};

	ret = nrf_clock_control_release(clk_dev, &clk_spec);
	TC_PRINT("nrf_clock_control_release return code: %d\n", ret);
	zassert_equal(ret, ONOFF_STATE_ON);
}

const struct device *global_hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(hsfll120));
#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
const struct device *local_hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_hsfll));
#endif

extern void test_ms_time_duration(void);
extern void kernel_init_objects(void);
extern void test_k_sleep(void);
extern void test_busy_wait(void);
extern void test_timer_status_get_anytime(void);
extern void test_timer_remaining(void);
extern void test_timer_k_define(void);
extern void *setup_timer_api(void);

ZTEST(clock, test_kernel_common_gd_320MHz)
{
	set_clock_frequency(MHZ(320), global_hsfll_dev);
	test_ms_time_duration();
	release_clock(MHZ(320), global_hsfll_dev);
}

ZTEST(clock, test_kernel_common_gd_256MHz)
{
	set_clock_frequency(MHZ(256), global_hsfll_dev);
	test_ms_time_duration();
	release_clock(MHZ(256), global_hsfll_dev);
}

ZTEST(clock, test_kernel_common_gd_128MHz)
{
	set_clock_frequency(MHZ(128), global_hsfll_dev);
	test_ms_time_duration();
	release_clock(MHZ(128), global_hsfll_dev);
}

ZTEST(clock, test_kernel_common_gd_64MHz)
{
	set_clock_frequency(MHZ(64), global_hsfll_dev);
	test_ms_time_duration();
	release_clock(MHZ(64), global_hsfll_dev);
}

ZTEST(clock, test_kernel_common)
{
	test_ms_time_duration();
}

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
ZTEST(clock, test_kernel_common_ld_64MHz)
{
	set_clock_frequency(MHZ(64), local_hsfll_dev);
	test_ms_time_duration();
	release_clock(MHZ(64), local_hsfll_dev);
}

ZTEST(clock, test_kernel_common_ld_128MHz)
{
	set_clock_frequency(MHZ(128), local_hsfll_dev);
	test_ms_time_duration();
	release_clock(MHZ(128), local_hsfll_dev);
}

ZTEST(clock, test_kernel_common_ld_320MHz)
{
	set_clock_frequency(MHZ(320), local_hsfll_dev);
	test_ms_time_duration();
	release_clock(MHZ(320), local_hsfll_dev);
}
#endif

ZTEST(clock, test_kernel_context_gd_320MHz)
{
	set_clock_frequency(MHZ(320), global_hsfll_dev);
	test_k_sleep();
	test_busy_wait();
	release_clock(MHZ(320), global_hsfll_dev);
}

ZTEST(clock, test_kernel_context_gd_256MHz)
{
	set_clock_frequency(MHZ(256), global_hsfll_dev);
	test_k_sleep();
	test_busy_wait();
	release_clock(MHZ(256), global_hsfll_dev);
}

ZTEST(clock, test_kernel_context_gd_128MHz)
{
	set_clock_frequency(MHZ(128), global_hsfll_dev);
	test_k_sleep();
	test_busy_wait();
	release_clock(MHZ(128), global_hsfll_dev);
}

ZTEST(clock, test_kernel_context_gd_64MHz)
{
	set_clock_frequency(MHZ(64), global_hsfll_dev);
	test_k_sleep();
	test_busy_wait();
	release_clock(MHZ(64), global_hsfll_dev);
}

ZTEST(clock, test_kernel_context)
{
	test_k_sleep();
	test_busy_wait();
}

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
ZTEST(clock, test_kernel_context_ld_64MHz)
{
	set_clock_frequency(MHZ(64), local_hsfll_dev);
	test_k_sleep();
	test_busy_wait();
	release_clock(MHZ(64), local_hsfll_dev);
}

ZTEST(clock, test_kernel_context_ld_128MHz)
{
	set_clock_frequency(MHZ(128), local_hsfll_dev);
	test_k_sleep();
	test_busy_wait();
	release_clock(MHZ(128), local_hsfll_dev);
}

ZTEST(clock, test_kernel_context_ld_320MHz)
{
	set_clock_frequency(MHZ(320), local_hsfll_dev);
	test_k_sleep();
	test_busy_wait();
	release_clock(MHZ(320), local_hsfll_dev);
}
#endif

ZTEST(clock, test_kernel_timer_gd_320MHz)
{
	set_clock_frequency(MHZ(320), global_hsfll_dev);
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
	release_clock(MHZ(320), global_hsfll_dev);
}

ZTEST(clock, test_kernel_timer_gd_256MHz)
{
	set_clock_frequency(MHZ(256), global_hsfll_dev);
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
	release_clock(MHZ(256), global_hsfll_dev);
}

ZTEST(clock, test_kernel_timer_gd_128MHz)
{
	set_clock_frequency(MHZ(128), global_hsfll_dev);
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
	release_clock(MHZ(128), global_hsfll_dev);
}

ZTEST(clock, test_kernel_timer_gd_64MHz)
{
	set_clock_frequency(MHZ(64), global_hsfll_dev);
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
	release_clock(MHZ(64), global_hsfll_dev);
}

ZTEST(clock, test_kernel_timer)
{
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
}

#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
ZTEST(clock, test_kernel_timer_ld_64MHz)
{
	set_clock_frequency(MHZ(64), local_hsfll_dev);
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
	release_clock(MHZ(64), local_hsfll_dev);
}

ZTEST(clock, test_kernel_timer_ld_128MHz)
{
	set_clock_frequency(MHZ(128), local_hsfll_dev);
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
	release_clock(MHZ(128), local_hsfll_dev);
}

ZTEST(clock, test_kernel_timer_ld_320MHz)
{
	set_clock_frequency(MHZ(320), local_hsfll_dev);
	test_timer_k_define();
	test_timer_remaining();
	test_timer_status_get_anytime();
	release_clock(MHZ(320), local_hsfll_dev);
}
#endif

static bool once;

static void *clock_setup(void)
{
	kernel_init_objects();
	setup_timer_api();

	/** wait for clock initialization */
	if (!once) {
		k_msleep(2000);
		once = true;
	}
	return NULL;
}

ZTEST_SUITE(clock, NULL, clock_setup, NULL, NULL, NULL);
