/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Basic functional tests for the SAADCT (SAADC + TIMER) driver.
 *
 * These tests verify the basic functionality of the driver:
 * configuration, starting an acquisition, and reading back the collected measurement.
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <drivers/saadct.h>
#include <nrfx_saadc.h>

#define NUM_OF_MEAS       20
#define NUM_OF_SERIES     10
#define SAMPLE_RATE_HZ    KHZ(100)
#define SAADCT_RESOLUTION NRF_SAADC_RESOLUTION_10BIT
#define RESOLUTION_BITS   10
#define MAX_SAMPLE_VALUE  (1U << RESOLUTION_BITS)

/* Value written into every sample slot before an acquisition starts. It lies
 * well outside the ADC resolution, so if the driver ever returns a series it
 * did not actually fill, check_series_values() detects the leftover poison
 * instead of silently accepting stale slab memory.
 */
#define SAMPLE_POISON	       0xA5U
#define TARGET_SERIES	       3
#define MEASUREMENT_TIMEOUT_MS 20

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
	SAADCT_CHAN_CONFIG_SE(NRFX_ANALOG_EXTERNAL_AIN6, 0),
	SAADCT_CHAN_CONFIG_SE(NRFX_ANALOG_EXTERNAL_AIN5, 1),
};

#define NUM_OF_CHANNELS    ARRAY_SIZE(test_channels)
#define SAMPLES_PER_SERIES (NUM_OF_MEAS * NUM_OF_CHANNELS)

#define MEAS_BLOCK_SIZE SAADCT_MEAS_BLOCK_SIZE(NUM_OF_MEAS, NUM_OF_CHANNELS)

K_MEM_SLAB_DEFINE(test_slab, MEAS_BLOCK_SIZE, NUM_OF_SERIES, sizeof(void *));

static atomic_t callback_count;

static void user_handler(void *context)
{
	ARG_UNUSED(context);
	atomic_inc(&callback_count);
}

#if IS_ENABLED(CONFIG_TEST_SAADCT_ONE_SHOT)
#define TEST_MODE saadct_mode_one_shot
#else
#define TEST_MODE saadct_mode_continuous

static uint32_t collected_series;
#endif

static struct saadct_config test_cfg = {
	.num_of_meas = NUM_OF_MEAS,
	.num_of_channels = NUM_OF_CHANNELS,
	.resolution = SAADCT_RESOLUTION,
	.mode = TEST_MODE,
	.sample_rate_hz = SAMPLE_RATE_HZ,
	.user_handler = user_handler,
	.user_context = NULL,
	.channels_config = test_channels,
};

#define SAADCT_NODE DT_NODELABEL(adc)
static const struct device *const saadct_dev = DEVICE_DT_GET(SAADCT_NODE);

static void check_series_values(const nrf_saadc_value_t *data)
{
	for (size_t i = 0; i < SAMPLES_PER_SERIES; i++) {
		zassert_true(abs(data[i]) < MAX_SAMPLE_VALUE,
			     "Sample %u = 0x%04x exceeds %u-bit range",
			     (unsigned int)i, (uint16_t)data[i], RESOLUTION_BITS);
	}
}

/* Pre-fill every block in the slab with an out-of-range poison byte pattern.
 * The driver hands out these blocks and is expected to overwrite the samples on
 * every completed series, so any sample slot still holding the poison afterwards
 * means the driver returned a series it never filled.
 */
static void poison_slab(struct k_mem_slab *slab)
{
	void *blocks[NUM_OF_SERIES];
	uint32_t allocated = 0;

	while (allocated < ARRAY_SIZE(blocks) &&
	       k_mem_slab_alloc(slab, &blocks[allocated], K_NO_WAIT) == 0) {
		memset(blocks[allocated], SAMPLE_POISON, MEAS_BLOCK_SIZE);
		allocated++;
	}

	for (uint32_t i = 0; i < allocated; i++) {
		k_mem_slab_free(slab, blocks[i]);
	}
}

static void *setup(void)
{
	int ret;

	zassert_true(device_is_ready(saadct_dev), "SAADCT device not ready");

	atomic_set(&callback_count, 0);

	ret = saadct_configure(saadct_dev, &test_cfg);
	zassert_ok(ret, "saadct_configure failed: %d", ret);

	return NULL;
}

#if IS_ENABLED(CONFIG_TEST_SAADCT_ONE_SHOT)

ZTEST(saadct_api, test_one_shot_single_series)
{
	int ret;
	nrf_saadc_value_t *data;

	poison_slab(&test_slab);

	ret = saadct_start(saadct_dev, &test_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* Sleep for a moment so that the sample is pending */
	k_msleep(MEASUREMENT_TIMEOUT_MS);

	/* There should be exactly 1 pending measurement result. */
	zassert_equal(saadct_pending(saadct_dev), 1,
		      "Expected exactly one pending series in one-shot mode");
	ret = saadct_get(saadct_dev, &data, K_NO_WAIT);

	zassert_ok(ret, "saadct_get failed: %d", ret);
	check_series_values(data);
	saadct_put(saadct_dev, data);

	/* One-shot must stop by itself, so no further series may appear. */
	k_msleep(50);
	zassert_equal(saadct_pending(saadct_dev), 0,
		      "One-shot produced more than one series");

	ret = saadct_get(saadct_dev, &data, K_NO_WAIT);
	zassert_equal(ret, -EIO,
		      "Expected -EIO when no measurement is available, got %d",
		      ret);
}

#else /* continuous mode */

ZTEST(saadct_api, test_continuous_acquisition)
{
	int ret;
	nrf_saadc_value_t *data;

	poison_slab(&test_slab);

	ret = saadct_start(saadct_dev, &test_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* Collect a number of series, verifying each one holds valid samples. */
	while (collected_series < TARGET_SERIES) {

		ret = saadct_get(saadct_dev, &data, K_MSEC(MEASUREMENT_TIMEOUT_MS));
		zassert_ok(ret, "saadct_get failed: %d", ret);
		check_series_values(data);
		saadct_put(saadct_dev, data);
		collected_series++;
	}

	/* Stop immediately and drain whatever series already completed. */
	ret = saadct_stop(saadct_dev, true);
	zassert_ok(ret, "saadct_stop failed: %d", ret);

	k_msleep(MEASUREMENT_TIMEOUT_MS);

	while (saadct_pending(saadct_dev) > 0) {
		ret = saadct_get(saadct_dev, &data, K_NO_WAIT);
		zassert_ok(ret, "saadct_get while draining failed: %d",
			   ret);
		check_series_values(data);
		saadct_put(saadct_dev, data);
		collected_series++;
	}

	ret = saadct_get(saadct_dev, &data, K_NO_WAIT);
	zassert_equal(ret, -EIO,
		     "Expected an error after draining all series, got %d", ret);

	zassert_true(collected_series >= TARGET_SERIES,
		     "Collected %u series, expected at least %u", collected_series, TARGET_SERIES);

	zassert_equal(atomic_get(&callback_count), (atomic_val_t)collected_series,
		      "user_handler called %ld times, expected %u (one per series)",
		      (long)atomic_get(&callback_count), collected_series);

}

#endif /* CONFIG_TEST_SAADCT_ONE_SHOT */

ZTEST_SUITE(saadct_api, NULL, setup, NULL, NULL, NULL);
