/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>
#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <debug/cpu_load.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_dppi.h>
#include <hal/nrf_power.h>

#define FULL_LOAD 100000
#define SMALL_LOAD 3000

static void timer_handler(nrf_timer_event_t event_type, void *context)
{
	/*empty*/
}

void test_cpu_load(void)
{
	int err;
	u32_t load;
	static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(1);

	if (IS_ENABLED(CONFIG_SOC_NRF9160)) {
		/* Wait for LF clock stabilization. */
		k_busy_wait(500000);
	}

	if (IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)) {
		nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG;
		u8_t ch;
		u32_t evt =
			nrf_power_event_address_get(NRF_POWER,
						    NRF_POWER_EVENT_SLEEPENTER);
		u32_t tsk =
		     nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT);

		config.frequency = NRF_TIMER_FREQ_1MHz;
		config.bit_width = NRF_TIMER_BIT_WIDTH_32;

		err = nrfx_timer_init(&timer, &config, timer_handler);
		zassert_equal(err, NRFX_SUCCESS, "Unexpected error:%d", err);

		err = nrfx_dppi_channel_alloc(&ch);
		zassert_equal(err, NRFX_SUCCESS, "Unexpected error:%d", err);

		nrfx_gppi_channel_endpoints_setup(ch, evt, tsk);
		nrfx_gppi_channels_enable(BIT(ch));

		if (!IS_ENABLED(CONFIG_CPU_LOAD_USE_SHARED_DPPI_CHANNELS)) {
			err = cpu_load_init();
			zassert_equal(err, -ENODEV, "Unexpected err:%d", err);

			nrfx_gppi_channels_disable(BIT(ch));
			nrfx_gppi_event_endpoint_clear(ch, evt);
			nrfx_gppi_task_endpoint_clear(ch, tsk);
			err = nrfx_dppi_channel_free(ch);
		}
	}

	err = cpu_load_init();
	zassert_equal(err, 0, "Unexpected err:%d", err);

	/* Busy wait for 10 ms */
	k_busy_wait(10000);

	load = cpu_load_get();
	zassert_equal(load, FULL_LOAD, "Unexpected load:%d", load);

	k_sleep(10);
	load = cpu_load_get();
	zassert_true(load < FULL_LOAD, "Unexpected load:%d", load);

	cpu_load_reset();
	k_sleep(10);
	load = cpu_load_get();
	zassert_true(load < SMALL_LOAD, "Unexpected load:%d", load);
}

void test_main(void)
{
	ztest_test_suite(cpu_load,
		ztest_unit_test(test_cpu_load)
	);
	ztest_run_test_suite(cpu_load);
}
