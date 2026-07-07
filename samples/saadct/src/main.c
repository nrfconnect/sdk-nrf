/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/saadct.h>

LOG_MODULE_REGISTER(saadct_sample, LOG_LEVEL_INF);

#define NUMBER_OF_MEASUREMENTS 100
#define NUMBER_OF_SERIES       10
#define ACQ_TIME               23

#if NRF_SAADC_HAS_CH_CONFIG_RES
#define SAADCT_RESISTOR_CFG                                                                        \
	.resistor_p = NRF_SAADC_RESISTOR_DISABLED,                                                 \
	.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
#else
#define SAADCT_RESISTOR_CFG
#endif

#define SAADCT_CHAN_DEFAULT_CONFIG_SE(_pin, _tacq, _gain, _ref)                                    \
	{                                                                                          \
		.channel_config = {                                                                \
			.gain = _gain,                                                             \
			SAADCT_RESISTOR_CFG                                                        \
			.reference = _ref,                                                         \
			.conv_time = NRFX_SAADC_DEFAULT_CONV_TIME,                                 \
			.acq_time = _tacq,                                                         \
			.mode = NRF_SAADC_MODE_SINGLE_ENDED,                                       \
		},                                                                                 \
		.pin_p = (nrfx_analog_input_t)_pin,                                                \
		.pin_n = NRFX_ANALOG_INPUT_DISABLED,                                               \
		.channel_index = (__COUNTER__ - SAADCT_CHAN_IDX_BASE - 1),                         \
	}

/* Anchor for auto-numbering channel indices sequentially from 0. */
enum {
	SAADCT_CHAN_IDX_BASE = __COUNTER__,
};

static const nrfx_saadc_channel_t saadct_chan_cfg[] = {
	SAADCT_CHAN_DEFAULT_CONFIG_SE(NRFX_ANALOG_EXTERNAL_AIN6, ACQ_TIME, NRF_SAADC_GAIN1,
				      NRF_SAADC_REFERENCE_INTERNAL),
	SAADCT_CHAN_DEFAULT_CONFIG_SE(NRFX_ANALOG_EXTERNAL_AIN5, ACQ_TIME, NRF_SAADC_GAIN1,
				      NRF_SAADC_REFERENCE_INTERNAL),
};

K_MEM_SLAB_DEFINE(my_meas_slab,
		SAADCT_MEAS_BLOCK_SIZE(NUMBER_OF_MEASUREMENTS, ARRAY_SIZE(saadct_chan_cfg)),
		128, 4);

static const struct device *const saadct_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

static atomic_t handler_cnt;

static void saadct_handler(void *context)
{
	ARG_UNUSED(context);
	atomic_inc(&handler_cnt);
}

static struct saadct_config saadct_cfg = {
	.num_of_channels = ARRAY_SIZE(saadct_chan_cfg),
	.num_of_meas = NUMBER_OF_MEASUREMENTS,
	.resolution = NRF_SAADC_RESOLUTION_10BIT,
	.mode = saadct_mode_continuous,
	.sample_rate_hz = 10000,
	.user_handler = saadct_handler,
	.user_context = NULL,
	.channels_config = saadct_chan_cfg,
};

int main(void)
{
	nrf_saadc_value_t *samples;
	int ret;
	uint32_t series_cnt = 0;
	const uint32_t samples_per_series =
		NUMBER_OF_MEASUREMENTS * ARRAY_SIZE(saadct_chan_cfg);

	LOG_INF("SAADCT sample start");

	if (!device_is_ready(saadct_dev)) {
		LOG_ERR("SAADCT device is not ready");
		return 0;
	}

	ret = saadct_configure(saadct_dev, &saadct_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure SAADCT: %d", ret);
		return 0;
	}

	ret = saadct_start(saadct_dev, &my_meas_slab);
	if (ret < 0) {
		LOG_ERR("Failed to start SAADCT: %d", ret);
		return 0;
	}

	k_msleep(100);
	saadct_stop(saadct_dev, false);
	LOG_INF("Stop requested, pending series: %u", saadct_pending(saadct_dev));

	while (series_cnt < NUMBER_OF_SERIES) {
		ret = saadct_get(saadct_dev, &samples, K_FOREVER);
		if (ret == -EIO) {
			/* No more series will arrive. */
			break;
		}

		if (ret < 0) {
			LOG_WRN("saadct_get returned %d after %u series", ret, series_cnt);
			break;
		}

		LOG_INF("series[%u]: first=0x%04x last=0x%04x", series_cnt, samples[0],
			samples[samples_per_series - 1]);
		saadct_put(saadct_dev, samples);
		series_cnt++;
	}

	while (saadct_pending(saadct_dev) > 0) {
		ret = saadct_get(saadct_dev, &samples, K_FOREVER);
		if (ret < 0) {
			LOG_WRN("saadct_get returned %d while draining", ret);
			break;
		}

		LOG_INF("drain series[%u]: first=0x%04x last=0x%04x", series_cnt, samples[0],
			samples[samples_per_series - 1]);
		saadct_put(saadct_dev, samples);
		series_cnt++;
	}

	LOG_INF("Measurement finished, series read: %u, callbacks: %u", series_cnt,
		(unsigned int)atomic_get(&handler_cnt));

	while (true) {
		k_msleep(100);
	}
}
