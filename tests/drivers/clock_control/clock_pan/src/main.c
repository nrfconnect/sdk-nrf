/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrfx_clock_xo24m.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>

#define TEST_REPETITIONS   10
#define TIMER_FREQUENCY_HZ 4e6

static nrfx_timer_t test_timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tst_timer)));

uint32_t configure_test_timer(nrfx_timer_t *timer, uint32_t timer_frequency_Hz)
{
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer->p_reg);
	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(timer_frequency_Hz);

	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.mode = NRF_TIMER_MODE_TIMER;

	printk("Timer base frequency: %d Hz\n", base_frequency);
	printk("Timer configured frequency: %d Hz\n", timer_config.frequency);

	if (nrfx_timer_init(timer, &timer_config, NULL) != 0) {
		printk("Timer init failed\n");
		return -1;
	}
	nrfx_timer_enable(timer);

	return nrfx_timer_task_address_get(timer, NRF_TIMER_TASK_CAPTURE0);
}

int main(void)
{
	nrfx_gppi_handle_t gppi_handle;

	uint32_t timer_cc_before, timer_cc_after;

	uint32_t timer_task;
	uint32_t clock_event;

	clock_event = nrf_clock_event_address_get(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLK24MSTARTED);
	timer_task = configure_test_timer(&test_timer, TIMER_FREQUENCY_HZ);

	if (nrfx_gppi_conn_alloc(clock_event, timer_task, &gppi_handle)) {
		printk("Failed to allocate DPPI connection\n");
	}

	nrfx_gppi_conn_enable(gppi_handle);

	if (nrfx_clock_xo24m_init_check()) {
		printk("XO24M driver initialized\n");
	} else {
		printk("XO24M driver not initialized - initalizing\n");
		if (nrfx_clock_xo24m_init(NULL)) {
			printk("XO24M driver inti failed\n");
		} else {
			printk("XO24M driver initialized\n");
		}
	}

	k_msleep(10);

	for (uint8_t counter = 0; counter < TEST_REPETITIONS; counter++) {

		nrfx_clock_xo24m_stop();
		while (nrfx_clock_xo24m_running_check()) {
			k_msleep(1);
		}
		printk("XO24M stopped\n");

		timer_cc_before = nrfx_timer_capture_get(&test_timer, NRF_TIMER_CC_CHANNEL0);
		nrfx_clock_xo24m_start();

		while (!nrfx_clock_xo24m_running_check()) {
			k_msleep(1);
		}
		timer_cc_after = nrfx_timer_capture_get(&test_timer, NRF_TIMER_CC_CHANNEL0);
		printk("XO24M started\n");

		printk("[Run %u], NRF_CLOCK_EVENT_HFCLK24MSTARTED generated %u ns after start "
		       "call\n",
		       counter + 1,
		       (uint32_t)((timer_cc_after - timer_cc_before) / (TIMER_FREQUENCY_HZ / 1e3)));
	}

	nrfx_timer_uninit(&test_timer);
	nrfx_gppi_conn_disable(gppi_handle);
	nrfx_gppi_conn_free(clock_event, timer_task, gppi_handle);

	return 0;
}