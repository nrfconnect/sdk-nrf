/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <debug/cpu_load.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <hal/nrf_power.h>

#ifdef DPPI_PRESENT
#include <nrfx_dppi.h>

static void timer_handler(nrf_timer_event_t event_type, void *context)
{
	/*empty*/
}
#endif

#define FULL_LOAD 100000
#define SMALL_LOAD 3000

ZTEST(cpu_load, test_cpu_load)
{
	int err;
	uint32_t load;

	if (IS_ENABLED(CONFIG_SOC_NRF9160)) {
		/* Wait for LF clock stabilization. */
		k_busy_wait(500000);
	}

#ifdef DPPI_PRESENT
	static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(1);
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer.p_reg);
	nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
	uint8_t ch;
	uint32_t evt = nrf_power_event_address_get(NRF_POWER,
						NRF_POWER_EVENT_SLEEPENTER);
	uint32_t tsk = nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT);

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
#endif

	err = cpu_load_init();
	zassert_equal(err, 0, "Unexpected err:%d", err);

	/* Busy wait for 10 ms */
	k_busy_wait(10000);

	load = cpu_load_get();
	zassert_equal(load, FULL_LOAD, "Unexpected load:%d", load);

	k_sleep(K_MSEC(10));
	load = cpu_load_get();
	zassert_true(load < FULL_LOAD, "Unexpected load:%d", load);

	cpu_load_reset();
	k_sleep(K_MSEC(10));
	load = cpu_load_get();
	zassert_true(load < SMALL_LOAD, "Unexpected load:%d", load);
}

ZTEST_SUITE(cpu_load, NULL, NULL, NULL, NULL, NULL);
