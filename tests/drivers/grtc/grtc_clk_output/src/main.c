/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>


static volatile uint32_t count_clk;

static struct gpio_callback clk_cb_data;
static const struct gpio_dt_spec gpio_clk_spec =
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), gpios, 0);

#define GRTC_NODE DT_NODELABEL(grtc)
#if DT_NODE_HAS_PROP(GRTC_NODE, clkout_fast_frequency_hz)
static struct onoff_client clk_cli;
static volatile bool clock_requested;

/* Callback for clock request. */
static void clock_started_callback(struct onoff_manager *mgr,
				   struct onoff_client *cli,
				   uint32_t state,
				   int res)
{
	(void)mgr;
	(void)cli;
	(void)state;
	(void)res;

	clock_requested = true;
}
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fll16m))
#define NODE_HFCLK      DT_NODELABEL(fll16m)
#define HFCLK_FREQUENCY DT_PROP_OR(NODE_HFCLK, frequency, 0)

static const struct device *fll16m = DEVICE_DT_GET(NODE_HFCLK);
static const struct nrf_clock_spec hfclk_spec = {
	.frequency = HFCLK_FREQUENCY,
};
#elif defined(CONFIG_SOC_SERIES_NRF54L)
static struct onoff_manager *clk_mgr;
#endif

static int hf_clock_request(void)
{
	sys_notify_init_callback(&clk_cli.notify, clock_started_callback);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fll16m))
	return nrf_clock_control_request(fll16m, &hfclk_spec, &clk_cli);
#elif defined(CONFIG_SOC_SERIES_NRF54L)
	clock_control_subsys_t subsys = CLOCK_CONTROL_NRF_SUBSYS_HF;

	clk_mgr = z_nrf_clock_control_get_onoff(subsys);
	return onoff_request(clk_mgr, &clk_cli);
#endif
	return -ENOTSUP;
}

static int hf_clock_release(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fll16m))
	return nrf_clock_control_release(fll16m, &hfclk_spec);
#elif defined(CONFIG_SOC_SERIES_NRF54L)
	return onoff_release(clk_mgr);
#endif
	return -ENOTSUP;
}
#endif /* DT_NODE_HAS_PROP(GRTC_NODE, clkout_fast_frequency_hz) */

/* ISR that counts rising edges on the CLK signal. */
static void gpio_clk_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(gpio_clk_spec.pin)) {
		count_clk++;
	}
}

/** @brief Check that there are edges on GRTC clock output pin.
 *
 * Verify that CLK signal can be output from GRTC.
 */
ZTEST(drivers_grtc_clk_out, test_grtc_clk_output)
{
	uint32_t min = CONFIG_TEST_GRTC_FREQUENCY - CONFIG_TEST_GRTC_TOLERANCE;
	uint32_t max = CONFIG_TEST_GRTC_FREQUENCY + CONFIG_TEST_GRTC_TOLERANCE;
	uint32_t temp;

	TC_PRINT("GRTC CLK test starts\n");
	TC_PRINT("Frequency = %u\n", CONFIG_TEST_GRTC_FREQUENCY);
	TC_PRINT("Tolerance = %u clock edges\n", CONFIG_TEST_GRTC_TOLERANCE);

#if DT_NODE_HAS_PROP(GRTC_NODE, clkout_fast_frequency_hz)
	int ret = hf_clock_request();

	zassert_true(ret >= 0, "Failed to request clock.");
	k_msleep(20);
	zassert_true(clock_requested == true, "Failed to request clock.");
#endif

	count_clk = 0;
	k_msleep(1000);
	temp = count_clk;

#if DT_NODE_HAS_PROP(GRTC_NODE, clkout_fast_frequency_hz)
	ret = hf_clock_release();
	zassert_true(ret >= 0, "Failed to release clock.");
#endif

	/* Verify number of edges on CLK (with some tolerance) */
	zassert_true(temp >= min,
		"CLK has %u rising edges, while at least %u is expected.",
		temp, min);

	zassert_true(temp <= max,
		"CLK has %u rising edges, while no more than %u is expected.",
		temp, max);

	TC_PRINT("In ~1 second, there were %d rising edges\n", temp);
}

static void *suite_setup(void)
{
	int ret;

	/* Configure GPIO used to count CLK edges. */
	ret = gpio_is_ready_dt(&gpio_clk_spec);
	zassert_equal(ret, true, "gpio_clk_spec not ready (ret %d)", ret);
	ret = gpio_pin_configure_dt(&gpio_clk_spec, GPIO_INPUT);
	zassert_equal(ret, 0, "failed to configure input pin gpio_clk_spec (err %d)", ret);
	ret = gpio_pin_interrupt_configure_dt(&gpio_clk_spec, GPIO_INT_EDGE_RISING);
	zassert_equal(ret, 0, "failed to configure gpio_clk_spec interrupt (err %d)", ret);
	gpio_init_callback(&clk_cb_data, gpio_clk_isr, BIT(gpio_clk_spec.pin));
	gpio_add_callback(gpio_clk_spec.port, &clk_cb_data);

	return 0;
}

ZTEST_SUITE(drivers_grtc_clk_out, NULL, suite_setup, NULL, NULL, NULL);
