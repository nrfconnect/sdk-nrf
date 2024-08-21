/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_hsfll.h>
#include <math.h>

LOG_MODULE_REGISTER(idle);

/*
 * wait max 500ms with 10us intervals for hsfll freq change event
 */
#define HSFLL_FREQ_CHANGE_MAX_DELAY_MS 500UL
#define HSFLL_FREQ_CHANGE_CHECK_INTERVAL_US 10
#define HSFLL_FREQ_CHANGE_CHECK_MAX_ATTEMPTS                                                       \
	((HSFLL_FREQ_CHANGE_MAX_DELAY_MS) * (USEC_PER_MSEC) / (HSFLL_FREQ_CHANGE_CHECK_INTERVAL_US))

int main(void)
{
	unsigned int cnt = 0;

	LOG_INF("Multicore idle test on %s", CONFIG_BOARD_TARGET);

	nrf_hsfll_trim_t hsfll_trim = {};

	uint8_t freq_trim = 3;

	hsfll_trim.vsup	  = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.VSUP;
	hsfll_trim.coarse = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.COARSE[freq_trim];
	hsfll_trim.fine	  = NRF_FICR->TRIM.APPLICATION.HSFLL.TRIM.FINE[freq_trim];

	nrf_hsfll_trim_set(NRF_HSFLL, &hsfll_trim);
	nrf_barrier_w();

	nrf_hsfll_clkctrl_mult_set(NRF_HSFLL, 4); //64HMZ
	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);
	/* HSFLL task frequency change needs to be triggered twice to take effect.*/
	nrf_hsfll_task_trigger(NRF_HSFLL, NRF_HSFLL_TASK_FREQ_CHANGE);

	bool hsfll_freq_changed = false;

	NRFX_WAIT_FOR(nrf_hsfll_event_check(NRF_HSFLL, NRF_HSFLL_EVENT_FREQ_CHANGED),
		      HSFLL_FREQ_CHANGE_CHECK_MAX_ATTEMPTS,
		      HSFLL_FREQ_CHANGE_CHECK_INTERVAL_US,
		      hsfll_freq_changed);

	if (!hsfll_freq_changed) {
		return 0;
	}

	if (nrf_hsfll_clkctrl_mult_get(NRF_HSFLL) != 4) {
		return 0;
	}
	volatile uint32_t count;
	volatile float_t result;
	while (1) {
		LOG_INF("Multicore idle test iteration %u", cnt++);
		k_msleep(1000);

		for (count = 0; count < 10000; count++) {
			result = sqrtf(count);
		}
	}

	return 0;
}
