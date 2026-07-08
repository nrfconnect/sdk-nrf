/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * SAADCT reading-correctness test using a GPIO loopback.
 *
 * Beyond checking that samples stay within the configured resolution range,
 * this test verifies that the SAADC actually measures the applied voltage: a
 * GPIO output connected to the SAADC input pin is driven high and low, and the
 * captured series is expected to read near full-scale and near zero
 * respectively.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <drivers/saadct.h>
#include <nrfx_saadc.h>

#define NUM_OF_MEAS            16
#define SAMPLE_RATE_HZ         KHZ(100)
#define SAADCT_RESOLUTION      NRF_SAADC_RESOLUTION_10BIT
#define RESOLUTION_BITS        10
#define MAX_SAMPLE_VALUE       (1U << RESOLUTION_BITS)
#define MEASUREMENT_TIMEOUT_MS 100

#define SAMPLE_LOW_EXPECTED  (nrf_saadc_value_t)0
#define SAMPLE_HIGH_EXPECTED (nrf_saadc_value_t)(MAX_SAMPLE_VALUE - 1)

#define SAMPLE_LOW_EXPECTED_ERROR  (nrf_saadc_value_t)5
#define SAMPLE_HIGH_EXPECTED_ERROR (nrf_saadc_value_t)5

/* Thresholds with generous margins around mid-scale. */
#define HIGH_THRESHOLD ((MAX_SAMPLE_VALUE * 3) / 4)   /* > 3/4 full-scale */
#define LOW_THRESHOLD  (MAX_SAMPLE_VALUE / 4)         /* < 1/4 full-scale */

/* SAADC input wired to the test GPIO by the gpio_loopback fixture. */
#define LOOPBACK_AIN DT_PROP(DT_PATH(zephyr_user), test_saadc_input)

#define SAADCT_CHAN_CONFIG_SE(_pin, _idx)                                      \
{                                                                              \
	.channel_config = {                                                    \
		.gain = NRF_SAADC_GAIN1,                                       \
		NRFX_COND_CODE_1(NRF_SAADC_HAS_CH_CONFIG_RES,                  \
				 (.resistor_p = NRF_SAADC_RESISTOR_DISABLED,   \
				  .resistor_n = NRF_SAADC_RESISTOR_DISABLED,), \
				 ())                                           \
		.reference = NRF_SAADC_REFERENCE_INTERNAL,                     \
		.conv_time = NRFX_SAADC_DEFAULT_CONV_TIME,                     \
		.acq_time = 23,                                                \
		.mode = NRF_SAADC_MODE_SINGLE_ENDED,                           \
	},                                                                     \
	.pin_p = (nrfx_analog_input_t)_pin,                                    \
	.pin_n = NRFX_ANALOG_INPUT_DISABLED,                                   \
	.channel_index = _idx,                                                 \
}

static const nrfx_saadc_channel_t test_channels[] = {
	SAADCT_CHAN_CONFIG_SE(LOOPBACK_AIN, 0),
};

#define NUM_OF_CHANNELS    ARRAY_SIZE(test_channels)
#define SAMPLES_PER_SERIES (NUM_OF_MEAS * NUM_OF_CHANNELS)

#define MEAS_BLOCK_SIZE SAADCT_MEAS_BLOCK_SIZE(NUM_OF_MEAS, NUM_OF_CHANNELS)

K_MEM_SLAB_DEFINE(test_slab, MEAS_BLOCK_SIZE, 4, sizeof(void *));

static struct saadct_config test_cfg = {
	.num_of_channels = NUM_OF_CHANNELS,
	.num_of_meas = NUM_OF_MEAS,
	.resolution = SAADCT_RESOLUTION,
	.sample_rate_hz = SAMPLE_RATE_HZ,
	.mode = saadct_mode_one_shot,
	.user_handler = NULL,
	.user_context = NULL,
	.channels_config = test_channels,
};

#define SAADCT_NODE DT_NODELABEL(adc)
static const struct device *const saadct_dev = DEVICE_DT_GET(SAADCT_NODE);

static const struct gpio_dt_spec loopback_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);

static void *saadct_setup(void)
{
	int ret;

	zassert_true(device_is_ready(saadct_dev), "SAADCT device not ready");
	zassert_true(gpio_is_ready_dt(&loopback_gpio), "Loopback GPIO not ready");

	ret = gpio_pin_configure_dt(&loopback_gpio, GPIO_OUTPUT_INACTIVE);
	zassert_ok(ret, "Failed to configure loopback GPIO: %d", ret);

	ret = saadct_configure(saadct_dev, &test_cfg);
	zassert_ok(ret, "saadct_configure failed: %d", ret);

	return NULL;
}

/*
 * Drive the loopback pin to the requested level, capture one one-shot series
 * and return it. The caller must release the buffer with saadct_put().
 */
static nrf_saadc_value_t *capture_series_at_level(int level)
{
	int ret;
	nrf_saadc_value_t *data = NULL;

	ret = gpio_pin_set_dt(&loopback_gpio, level);
	zassert_ok(ret, "Failed to set loopback GPIO: %d", ret);

	/* Allow the pad voltage to settle before sampling. */
	k_msleep(2);

	ret = saadct_start(saadct_dev, &test_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	ret = saadct_get(saadct_dev, &data, K_MSEC(MEASUREMENT_TIMEOUT_MS));
	zassert_ok(ret, "saadct_get failed: %d", ret);

	zassert_not_null(data, "meas_get returned NULL data");

	return data;
}

ZTEST_SUITE(saadct_gpio_loopback, NULL, saadct_setup, NULL, NULL, NULL);

ZTEST(saadct_gpio_loopback, test_reading_follows_gpio_high)
{
	nrf_saadc_value_t *data = capture_series_at_level(1);

	for (size_t i = 0; i < SAMPLES_PER_SERIES; i++) {
		/* Still within range... */
		zassert_within(data[i], SAMPLE_HIGH_EXPECTED, SAMPLE_HIGH_EXPECTED_ERROR,
			       "Sample %u = 0x%04x is not within range",
			       (unsigned int)i, (uint16_t)data[i]);
	}

	saadct_put(saadct_dev, data);
}

ZTEST(saadct_gpio_loopback, test_reading_follows_gpio_low)
{
	nrf_saadc_value_t *data = capture_series_at_level(0);

	for (size_t i = 0; i < SAMPLES_PER_SERIES; i++) {
		/* Still within range... */
		zassert_within(data[i], SAMPLE_LOW_EXPECTED, SAMPLE_LOW_EXPECTED_ERROR,
			       "Sample %u = 0x%04x is not within range",
			       (unsigned int)i, (uint16_t)data[i]);
	}

	saadct_put(saadct_dev, data);
}

ZTEST(saadct_gpio_loopback, test_high_reads_above_low)
{
	nrf_saadc_value_t *high = capture_series_at_level(1);
	int high_avg = 0;

	for (size_t i = 0; i < SAMPLES_PER_SERIES; i++) {
		high_avg += high[i];
	}
	high_avg /= (int)SAMPLES_PER_SERIES;
	saadct_put(saadct_dev, high);

	nrf_saadc_value_t *low = capture_series_at_level(0);
	int low_avg = 0;

	for (size_t i = 0; i < SAMPLES_PER_SERIES; i++) {
		low_avg += (int)low[i];
	}
	low_avg /= (int)SAMPLES_PER_SERIES;
	saadct_put(saadct_dev, low);

	zassert_true(high_avg > low_avg,
		     "Expected high average (%d) to exceed low average (%d)",
		     high_avg, low_avg);
}
