/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "adc_test.h"

LOG_MODULE_REGISTER(adc_test, LOG_LEVEL_INF);

#define ADC_NODE DT_NODELABEL(adc)

static int16_t adc_sample_buffer[ADC_BUFFER_MAX_SIZE] DMM_MEMORY_SECTION(ADC_NODE);

static const struct device *adc = DEVICE_DT_GET(ADC_NODE);
static struct adc_channel_cfg channel_cfgs[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

extern atomic_t started_threads;

const struct adc_sequence_options options = {
	.interval_us = 100,
	.extra_samplings = ADC_BUFFER_MAX_SIZE - 1,
};

struct adc_sequence sequence = {
	.options = &options, .buffer = adc_sample_buffer, .buffer_size = sizeof(adc_sample_buffer)};

static int setup(void)
{
	int ret;

	ret = adc_is_ready_dt(&adc_channels[0]);
	if (ret != 1) {
		LOG_ERR("ADC device not ready: %d", ret);
		return -1;
	}

	ret = adc_channel_setup(adc, &channel_cfgs[0]);
	if (ret != 0) {
		LOG_ERR("ADC channel setup failed: %d", ret);
		return -2;
	}

	ret = adc_sequence_init_dt(&adc_channels[0], &sequence);
	if (ret != 0) {
		LOG_ERR("ADC sequence init failed: %d", ret);
		return -3;
	}

	return 0;
}

static void adc_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("ADC setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("ADC load thread started");
	while (1) {
		ret = adc_read_dt(&adc_channels[0], &sequence);
		if (ret != 0) {
			LOG_ERR("ADC read failed: %d", ret);
		}
		k_msleep(ADC_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(adc_load_thread, ADC_THREAD_STACKSIZE, adc_load_thread_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(ADC_THREAD_PRIORITY), 0, 0);
