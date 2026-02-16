/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <nrfx_saadc.h>
#include <hal/nrf_saadc.h>

#define SAADC_SAMPLE_FREQUENCY 8000UL
#define INTERNAL_TIMER_FREQ    16000000UL
#define INTERNAL_TIMER_CC      (INTERNAL_TIMER_FREQ / SAADC_SAMPLE_FREQUENCY)

const nrf_saadc_channel_config_t test_channel_config = {
	.gain = NRF_SAADC_GAIN1,
	.reference = NRF_SAADC_REFERENCE_INTERNAL,
	.acq_time = 5,
	.mode = NRF_SAADC_MODE_SINGLE_ENDED,
};

ZTEST(adc_scan, test_saadc_multi_channel_config_with_interval_timer)
{
	int ret;

	const nrfx_saadc_channel_t channel_0 = {.channel_config = test_channel_config,
						.pin_p = NRFX_ANALOG_EXTERNAL_AIN0,
						.pin_n = NRFX_ANALOG_INPUT_DISABLED,
						.channel_index = 0};

	const nrfx_saadc_channel_t channel_1 = {.channel_config = test_channel_config,
						.pin_p = NRFX_ANALOG_EXTERNAL_AIN1,
						.pin_n = NRFX_ANALOG_INPUT_DISABLED,
						.channel_index = 1};

	const nrfx_saadc_channel_t channels[] = {channel_0, channel_1};

	nrfx_saadc_adv_config_t adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;

	adv_config.internal_timer_cc = INTERNAL_TIMER_CC;
	adv_config.start_on_end = false;

	ret = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
	zassert_ok(ret, "nrfx_saadc_init failed, ret = %d\n", ret);

	for (int i = 0; i < ARRAY_SIZE(channels); i++) {
		ret = nrfx_saadc_channel_config(&channels[i]);
		zassert_ok(ret, "nrfx_saadc_channel_config failed, ret = %d\n", ret);
	}

	uint32_t channel_mask = nrfx_saadc_channels_configured_get();

	ret = nrfx_saadc_advanced_mode_set(channel_mask, NRF_SAADC_RESOLUTION_10BIT, &adv_config,
					   NULL);
	zassert_ok(ret, "nrfx_saadc_advanced_mode_set failed, ret = %d\n", ret);
}

ZTEST_SUITE(adc_scan, NULL, NULL, NULL, NULL, NULL);
