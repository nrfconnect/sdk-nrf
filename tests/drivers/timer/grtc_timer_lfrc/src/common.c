/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

#if defined(CONFIG_SOC_NRF54H20)
const struct device *const lfclk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk));
const struct device *const fll16m_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m));

const struct nrf_clock_spec lfclk_lfrc_mode = {
	.frequency = 32768,
	.accuracy = 0,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec lfclk_synth_mode = {
	.frequency = 32768,
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec fll16m_bypass_mode = {
	.frequency = MHZ(16),
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

#else
const struct device *const hfclock = DEVICE_DT_GET(DT_NODELABEL(clock));
#endif /* CONFIG_SOC_NRF54H20 */

#if defined(CONFIG_SOC_NRF54H20)
int request_clock_spec(const struct device *clk_dev, const struct nrf_clock_spec *clk_spec)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;

	printk("Clock: %s, requesting frequency=%d, accuracy=%d, precision=%d\n", clk_dev->name,
	       clk_spec->frequency, clk_spec->accuracy, clk_spec->precision);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	if (!(ret >= 0 && ret <= 2)) {
		return -1;
	}
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	if (ret != 0) {
		return -2;
	}
	if (res != 0) {
		return -3;
	}
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	if (ret != -ENOSYS) {
		if (ret != 0) {
			return -4;
		}
		if (rate != clk_spec->frequency) {
			return -5;
		};
		k_busy_wait(REQUEST_SERVING_WAIT_TIME_US);
	}

	return 0;
}
#endif /* CONFIG_SOC_NRF54H20 */
