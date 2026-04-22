/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

#if defined(CONFIG_TEST_CC_HF_CLOCK_DRIVER)

const struct nrf_clock_spec fll16m_bypass_mode = {
	.frequency = MHZ(16),
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct device *const fll16m_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m));

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
#endif /* CONFIG_TEST_CC_HF_CLOCK_DRIVER */

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

int configure_hfclock(void)
{
	int ret = 0;
#if defined(CONFIG_TEST_NRFX_HF_CLOCK_DRIVER)
	nrf_clock_hfclk_t clk_src;

	nrfx_clock_start(NRF_CLOCK_DOMAIN_HFCLK);
	if (nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, &clk_src) == false) {
		ret = -1;
	}
#else
	ret = request_clock_spec(fll16m_dev, &fll16m_bypass_mode);
#endif /* CONFIG_TEST_NRFX_HF_CLOCK_DRIVER */
	return ret;
}
