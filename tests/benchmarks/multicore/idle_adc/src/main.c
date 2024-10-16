/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#define ADC_NODE DT_ALIAS(adc0)
#define ADC_LOW_LEVEL 0
#define ADC_HIGH_LEVEL 4092

static const struct device *adc = DEVICE_DT_GET(ADC_NODE);
static const struct gpio_dt_spec gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sw), gpios);

int adc_setup(void)
{
	int ret;
	/* Get the number of channels defined on the DTS. */
	static const struct adc_channel_cfg channel_cfgs[] = {
DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};

	ret = adc_channel_setup(adc, &channel_cfgs[0]);
	return ret;
}

int main(void)
{
	int err;
	uint16_t channel_reading[5];
	int16_t sample_value;

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

	err = gpio_pin_configure_dt(&gpio, GPIO_OUTPUT);
	__ASSERT_NO_MSG(err == 0);
	err = adc_setup();
	__ASSERT_NO_MSG(err == 0);

	while (1) {
		gpio_pin_set_dt(&gpio, 1);
		err = adc_read(adc, &sequence);
		sample_value = channel_reading[0];
		__ASSERT_NO_MSG(sample_value == ADC_HIGH_LEVEL);
		k_sleep(K_SECONDS(1));
		gpio_pin_set_dt(&gpio, 0);
		err = adc_read(adc, &sequence);
		sample_value = channel_reading[0];
		__ASSERT_NO_MSG(sample_value == ADC_LOW_LEVEL);
		k_sleep(K_SECONDS(1));
	}
	return 0;
}
