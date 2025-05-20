/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc, LOG_LEVEL_INF);

#include <zephyr/drivers/adc.h>
#include "common.h"

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx)	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};

/* Get the number of channels defined on the DTS. */
#define CHANNEL_COUNT ARRAY_SIZE(adc_channels)

/* ADC thread */
static void adc_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
	uint16_t average[CHANNEL_COUNT] = { 0 };
	int32_t average_mv[CHANNEL_COUNT];

	atomic_inc(&started_threads);

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			LOG_ERR("ADC controller devivce %s is not ready",
				adc_channels[i].dev->name);
			atomic_inc(&completed_threads);
			return;
		}

		ret = adc_channel_setup_dt(&adc_channels[i]);
		if (ret < 0) {
			LOG_ERR("Could not setup channel #%d (%d)", i, ret);
			atomic_inc(&completed_threads);
			return;
		}
	}

	for (int i = 0; i < (ADC_THREAD_COUNT_MAX * ADC_THREAD_OVERSAMPLING); i++) {
		/* For each ADC channel */
		for (size_t i = 0U; i < CHANNEL_COUNT; i++) {

			/* Read ADC channel */
			ret = adc_sequence_init_dt(&adc_channels[i], &sequence);
			if (ret < 0) {
				LOG_ERR("adc_sequence_init_dt(%d) failed (%d)", i, ret);
				atomic_inc(&completed_threads);
				return;
			}

			ret = adc_read_dt(&adc_channels[i], &sequence);
			if (ret < 0) {
				LOG_ERR("adc_read_dt(%d) failed (%d)", i, ret);
				atomic_inc(&completed_threads);
				return;
			}

			/* Calculate running average */
			average[i] = (average[i] + buf) >> 1;
			average_mv[i] = average[i];

			ret = adc_raw_to_millivolts_dt(&adc_channels[i], &average_mv[i]);
			if (ret < 0) {
				LOG_ERR("adc_raw_to_millivolts_dt(%d) failed (%d)", i, ret);
				atomic_inc(&completed_threads);
				return;
			}
		}

		/* Print result */
		if ((i % ADC_THREAD_OVERSAMPLING) == 0) {
			LOG_INF("CH0 = %d (%d mv), CH1 = %d (%d mv)",
				average[0], average_mv[0], average[1], average_mv[1]);
		}

		k_msleep(ADC_THREAD_SLEEP / ADC_THREAD_OVERSAMPLING);
	}
	LOG_INF("ADC thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_adc_id, ADC_THREAD_STACKSIZE, adc_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(ADC_THREAD_PRIORITY), 0, 0);
