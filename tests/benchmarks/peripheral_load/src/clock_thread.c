/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifdef CONFIG_CLOCK_CONTROL_NRF2
#include "common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock, LOG_LEVEL_INF);

#include <zephyr/devicetree.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/kernel.h>

struct test_clk_context {
	const struct device *clk_dev;
	const struct nrf_clock_spec *clk_specs;
	size_t clk_specs_size;
};

const struct nrf_clock_spec test_clk_specs_hsfll[] = {
	{
		.frequency = MHZ(128),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(320),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(64),
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
};

const struct nrf_clock_spec test_clk_specs_fll16m[] = {
	{
		.frequency = MHZ(16),
		.accuracy = 20000,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(16),
		.accuracy = 5020,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = MHZ(16),
		.accuracy = 30,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
};

static const struct test_clk_context fll16m_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m)),
		.clk_specs = test_clk_specs_fll16m,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_fll16m),
	},
};

static const struct test_clk_context cpuapp_hsfll_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_hsfll)),
		.clk_specs = test_clk_specs_hsfll,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_hsfll),
	},
};

const struct nrf_clock_spec test_clk_specs_lfclk[] = {
	{
		.frequency = 32768,
		.accuracy = 0,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = 32768,
		.accuracy = 20,
		.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
	},
	{
		.frequency = 32768,
		.accuracy = 20,
		.precision = NRF_CLOCK_CONTROL_PRECISION_HIGH,
	},
};

static const struct test_clk_context lfclk_test_clk_contexts[] = {
	{
		.clk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk)),
		.clk_specs = test_clk_specs_lfclk,
		.clk_specs_size = ARRAY_SIZE(test_clk_specs_lfclk),
	},
};

static void test_request_release_clock_spec(const struct device *clk_dev,
					    const struct nrf_clock_spec *clk_spec)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;
	bool rate_supported = true;

	LOG_INF("Clock under test: %s", clk_dev->name);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	LOG_INF("Clock control request return value: %d", ret);
	if (ret != 0) {
		LOG_ERR("Clock control request failed");
		atomic_inc(&completed_threads);
		return;
	}
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	LOG_INF("Clock control request response code: %d", res);
	if (res != 0) {
		LOG_ERR("Wrong clock control request response code");
		atomic_inc(&completed_threads);
		return;
	}
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	LOG_INF("Clock control get rate response code: %d", ret);
	if (ret == -ENOSYS) {
		LOG_INF("Clock control get rate not supported");
		rate_supported = false;
	} else if (ret != 0) {
		LOG_ERR("Clock control get rate failed");
		atomic_inc(&completed_threads);
		return;
	}
	if (rate_supported && rate != clk_spec->frequency) {
		LOG_ERR("Invalid rate: %d", rate);
		atomic_inc(&completed_threads);
		return;
	}
	k_msleep(1000);
	ret = nrf_clock_control_release(clk_dev, clk_spec);
	if (ret != ONOFF_STATE_ON) {
		LOG_ERR("Clock control release failed");
		atomic_inc(&completed_threads);
		return;
	}
}

static void test_clock_control_request(const struct test_clk_context *clk_contexts,
				       size_t contexts_size)
{
	const struct test_clk_context *clk_context;
	size_t clk_specs_size;
	const struct device *clk_dev;
	const struct nrf_clock_spec *clk_spec;

	for (size_t i = 0; i < contexts_size; i++) {
		clk_context = &clk_contexts[i];
		clk_specs_size = clk_context->clk_specs_size;

		for (size_t u = 0; u < clk_specs_size; u++) {
			clk_dev = clk_context->clk_dev;
			clk_spec = &clk_context->clk_specs[u];

			LOG_INF("Applying clock (%s) spec: frequency %d, accuracy %d, precision "
				"%d",
				clk_dev->name, clk_spec->frequency, clk_spec->accuracy,
				clk_spec->precision);
			test_request_release_clock_spec(clk_dev, clk_spec);
		}
	}
}

/* Clock thread */
static void clock_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	atomic_inc(&started_threads);

	k_msleep(100);
	test_clock_control_request(cpuapp_hsfll_test_clk_contexts,
				   ARRAY_SIZE(cpuapp_hsfll_test_clk_contexts));

	test_clock_control_request(fll16m_test_clk_contexts, ARRAY_SIZE(fll16m_test_clk_contexts));

	test_clock_control_request(lfclk_test_clk_contexts, ARRAY_SIZE(lfclk_test_clk_contexts));

	LOG_INF("Clock thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_clock_id, CLOCK_THREAD_STACKSIZE, clock_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(CLOCK_THREAD_PRIORITY), 0, 0);
#endif
