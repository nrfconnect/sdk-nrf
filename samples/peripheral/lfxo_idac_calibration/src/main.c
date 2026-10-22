/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <hal/nrf_lfxo.h>

#define IDAC_BITS	 5
#define IDAC_MIN	 0
#define IDAC_MAX	 BIT_MASK(IDAC_BITS)
#define PD_SETTLE_US	 2000
#define CLEAR_TIMEOUT_MS 400
#define SETTLE_TIME_MS	 10
#define TRIGGER_TIME_MS	 2000

static void idac_set(uint8_t code)
{
	nrf_lfxo_amplitude_ctrl_t ctrl;

	nrf_lfxo_amplitude_control_get(NRF_LFXO, &ctrl);
	ctrl.idac_init = code;
	nrf_lfxo_amplitude_control_set(NRF_LFXO, &ctrl);
}

static nrf_lfxo_amplitude_t watch_amplitude(uint32_t time_ms)
{
	for (uint32_t ms = 0; ms < time_ms; ms++) {
		nrf_lfxo_amplitude_t amplitude = nrf_lfxo_amplitude_get(NRF_LFXO);

		if (amplitude != NRF_LFXO_AMPLITUDE_OK) {
			return amplitude;
		}
		k_busy_wait(USEC_PER_MSEC);
	}

	return NRF_LFXO_AMPLITUDE_OK;
}

static bool wait_amplitude_ok(uint32_t timeout_ms)
{
	for (uint32_t ms = 0; ms < timeout_ms; ms++) {
		if (nrf_lfxo_amplitude_get(NRF_LFXO) == NRF_LFXO_AMPLITUDE_OK) {
			return true;
		}
		k_busy_wait(USEC_PER_MSEC);
	}

	return false;
}

/* Successive approximation of the IDAC code that keeps the amplitude within
 * the peak detector thresholds. Before each trial, the IDAC is driven to MIN
 * or MAX until the amplitude is back within the thresholds.
 */
static int idac_search(void)
{
	int n = IDAC_BITS - 1;
	uint8_t code = BIT(n);

	idac_set(IDAC_MIN);

	while (true) {
		if (!wait_amplitude_ok(CLEAR_TIMEOUT_MS)) {
			return -ETIMEDOUT;
		}

		idac_set(code);
		k_busy_wait(SETTLE_TIME_MS * USEC_PER_MSEC);

		switch (watch_amplitude(TRIGGER_TIME_MS)) {
		case NRF_LFXO_AMPLITUDE_OK:
			return code;

		case NRF_LFXO_AMPLITUDE_TOO_HIGH:
			if ((code == 1) || ((n == 0) && !(code & BIT(0)))) {
				return -ERANGE;
			}
			code &= ~BIT(n);
			if (n > 0) {
				n--;
				code |= BIT(n);
			}
			idac_set(IDAC_MIN);
			break;

		case NRF_LFXO_AMPLITUDE_TOO_LOW:
			if (n == 0) {
				return -ERANGE;
			}
			n--;
			code |= BIT(n);
			idac_set(IDAC_MAX);
			break;
		}
	}
}

int main(void)
{
	nrf_lfxo_amplitude_ctrl_t ctrl;
	int idac;

	printk("LFXO IDAC calibration\n");

	/* Manual Pierce mode: the peak detector is forced on and IDACINIT drives the IDAC */
	nrf_lfxo_amplitude_control_get(NRF_LFXO, &ctrl);
	ctrl.pdctrl_en = false;
	nrf_lfxo_amplitude_control_set(NRF_LFXO, &ctrl);
	k_busy_wait(PD_SETTLE_US);

	idac = idac_search();
	if (idac < 0) {
		printk("IDAC search failed (%d)\n", idac);
		return 0;
	}

	printk("Add the following to the devicetree:\n");
	printk("&lfxo {\n\tidac-init = <%d>;\n};\n", idac);

	return 0;
}
