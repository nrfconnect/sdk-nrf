/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#define ADC_NODE DT_ALIAS(adc0)
static const struct device *adc = DEVICE_DT_GET(ADC_NODE);

/* Get the number of channels defined on the DTS. */
static const struct adc_channel_cfg channel_cfgs[] = {
DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};

void thread_definition(void)
{
	uint16_t channel_reading[5];
	int ret;

	/* Options for the sequence sampling. */
	const struct adc_sequence_options options = {
		.extra_samplings = 1,
		.interval_us = 0,
	};

	struct adc_sequence sequence = {
		.buffer = channel_reading,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(channel_reading),
		.resolution = 12,
		.options = &options,
		.channels = 1,
	};

	ret = adc_channel_setup(adc, &channel_cfgs[0]);
	if (ret < 0) {
		printk("Issue with setting up channel, terminating thread.");
		return;
	}
	while (1) {
		ret = adc_read_async(adc, &sequence, NULL);
		if (ret < 0) {
			printk("Issue wih reading voltage, terminating thread.");
			return;
		}
	};
};
