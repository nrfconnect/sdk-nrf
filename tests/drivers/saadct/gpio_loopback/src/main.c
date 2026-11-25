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

#define NUM_OF_MEAS            100
#define SAMPLE_RATE_HZ         KHZ(20)
#define MEASUREMENT_TIMEOUT_MS 50
/* Select MEASUREMENT_TIMEOUT_MS to be an integer multiple of time
 * needed to acquire NUM_OF_MEAS ADC samples at SAMPLE_RATE_HZ.
 * (which is NUM_OF_MEAS * 1/SAMPLE_RATE_HZ [sec])
 * Otherwise, test may fail due to rounding errors.
 */
#define SERIE_TIME_MS          (1000 * NUM_OF_MEAS / SAMPLE_RATE_HZ)
#define EXPECTED_SERIES_CNT    (MEASUREMENT_TIMEOUT_MS / SERIE_TIME_MS)

#define SAADCT_RESOLUTION      NRF_SAADC_RESOLUTION_10BIT
#define RESOLUTION_BITS        10
#define MAX_SAMPLE_VALUE       (1U << RESOLUTION_BITS)

#define SAMPLE_LOW_EXPECTED  (nrf_saadc_value_t)0
#define SAMPLE_HIGH_EXPECTED (nrf_saadc_value_t)(MAX_SAMPLE_VALUE - 1)

#define SAMPLE_LOW_EXPECTED_TOLERANCE  (nrf_saadc_value_t)5
#define SAMPLE_HIGH_EXPECTED_TOLERANCE (nrf_saadc_value_t)5

/* SAADC input wired to the test GPIO by the gpio_loopback fixture. */
#define LOOPBACK_AIN DT_PROP(DT_PATH(zephyr_user), test_saadc_input)

#define SAADCT_CHAN_CONFIG_SE(_pin, _idx)                                      \
{                                                                              \
	.channel_config = {                                                    \
		NRFX_COND_CODE_1(NRF_SAADC_HAS_CH_GAIN,                        \
				 (.gain = NRF_SAADC_GAIN1,),                   \
				 ())                                           \
		NRFX_COND_CODE_1(NRF_SAADC_HAS_CH_CONFIG_RES,                  \
				 (.resistor_p = NRF_SAADC_RESISTOR_DISABLED,   \
				  .resistor_n = NRF_SAADC_RESISTOR_DISABLED,), \
				 ())                                           \
		.reference = NRF_SAADC_REFERENCE_INTERNAL,                     \
		.conv_time = NRFX_SAADC_DEFAULT_CONV_TIME,                     \
		.acq_time = 319,  /* (TACQ + 1) x 125 ns */                    \
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

K_MEM_SLAB_DEFINE(test_slab, MEAS_BLOCK_SIZE, 32, 4);

static struct saadct_config test_cfg = {
	.num_of_channels = NUM_OF_CHANNELS,
	.num_of_meas = NUM_OF_MEAS,
	.resolution = SAADCT_RESOLUTION,
	.sample_rate_hz = SAMPLE_RATE_HZ,
#if defined(CONFIG_TEST_SAADCT_MODE_ONE_SHOT)
	.mode = saadct_mode_one_shot,
#elif defined(CONFIG_TEST_SAADCT_MODE_CONTINUOUS)
	.mode = saadct_mode_continuous,
#else
#error Unknown TEST_SAADCT_MODE
#endif
	.user_handler = NULL,
	.user_context = NULL,
	.channels_config = test_channels,
};

#define SAADCT_NODE DT_NODELABEL(adc)
static const struct device *const saadct_dev = DEVICE_DT_GET(SAADCT_NODE);

static const struct gpio_dt_spec loopback_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);

static void *suite_setup(void)
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

static void verify_samples(nrf_saadc_value_t *data, size_t size, uint32_t level)
{
	nrf_saadc_value_t expected;
	nrf_saadc_value_t tolerance;

	if (level) {
		expected = SAMPLE_HIGH_EXPECTED;
		tolerance = SAMPLE_HIGH_EXPECTED_TOLERANCE;
	} else {
		expected = SAMPLE_LOW_EXPECTED;
		tolerance = SAMPLE_LOW_EXPECTED_TOLERANCE;
	}

	for (size_t i = 0; i < size; i++) {
		zexpect_within(data[i], expected, tolerance,
			       "Sample %u: %d while expected is %d (+/- %u)",
			       i, data[i], expected, tolerance);
	}
}

/*
 * Drive the loopback pin to the requested level, capture one one-shot series
 * and return it. The caller must release the buffer with saadct_put().
 */
static void capture_series_at_level(int level)
{
	int ret;
	nrf_saadc_value_t *data = NULL;

	ret = gpio_pin_set_dt(&loopback_gpio, level);
	zassert_ok(ret, "Failed to set loopback GPIO: %d", ret);

	/* Allow the pad voltage to settle before sampling. */
	k_msleep(2);

#if defined(CONFIG_TEST_SAADCT_MODE_ONE_SHOT)
	ret = saadct_start(saadct_dev, &test_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	ret = saadct_get(saadct_dev, &data, K_MSEC(2 * SERIE_TIME_MS));
	zassert_ok(ret, "saadct_get failed: %d", ret);
	zassert_not_null(data, "saadct_get returned NULL data");
	verify_samples(data, SAMPLES_PER_SERIES, level);

	saadct_put(saadct_dev, data);

	/* Confirm only one series was acquired. */
	k_msleep(2 * SERIE_TIME_MS);
	zassert_ok(saadct_pending(saadct_dev), "Unexpected second series was acquired");

#elif defined(CONFIG_TEST_SAADCT_MODE_CONTINUOUS)
	uint32_t series_cnt = 0;

	ret = saadct_start(saadct_dev, &test_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* Waiting for some time to acquire expected number of series is not accurate. */
	k_msleep(MEASUREMENT_TIMEOUT_MS);

#if defined(CONFIG_TEST_SAADCT_STOP_IMMEDIATELY)
	/* saadct_stop(dev, true) immediately stops sampling. Any incomplete series is discarded.
	 * This may result in less acquired series than expected.
	 */
	saadct_stop(saadct_dev, true);
#else
	/* saadct_stop(dev, false) completes the current serie before stopping hardware.
	 * This may result in an additional serie of samples.
	 */
	saadct_stop(saadct_dev, false);
#endif /* defined(CONFIG_TEST_SAADCT_STOP_IMMEDIATELY) */

	while (true) {
		ret = saadct_get(saadct_dev, &data, K_MSEC(2 * SERIE_TIME_MS));
		if (ret == -EIO) {
			/* No more series will arrive. */
			TC_PRINT("Break after %u series\n", series_cnt);
			break;
		}

		/* Verify data in expected series. */
		zassert_ok(ret, "saadct_get returned %d", ret);
		zassert_not_null(data, "saadct_get returned NULL data");
		verify_samples(data, SAMPLES_PER_SERIES, level);

		saadct_put(saadct_dev, data);

		series_cnt++;
	}

	zassert_within(series_cnt, EXPECTED_SERIES_CNT, 1,
		      "Number of series = %d while expected is %d",
		      series_cnt, EXPECTED_SERIES_CNT);
#endif /* defined(CONFIG_TEST_SAADCT_MODE_CONTINUOUS) */
}

ZTEST(saadct_gpio_loopback, test_high_level)
{
	capture_series_at_level(1);
}

ZTEST(saadct_gpio_loopback, test_low_level)
{
	capture_series_at_level(0);
}

ZTEST(saadct_gpio_loopback, test_toggle)
{
	capture_series_at_level(0);
	capture_series_at_level(1);
	capture_series_at_level(0);
	capture_series_at_level(1);
}

ZTEST_SUITE(saadct_gpio_loopback, NULL, suite_setup, NULL, NULL, NULL);
