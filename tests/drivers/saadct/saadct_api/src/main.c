/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Basic functional tests for the SAADCT (SAADC + TIMER) driver.
 *
 * These tests exercise the happy path of the public driver API declared in
 * <saadct.h>: configuring the driver, starting an acquisition, reading back the
 * collected measurement series and releasing the buffers.
 *
 * The driver keeps global state and initializes the underlying nrfx SAADC only
 * once (there is no de-init entry point), therefore a single test image can
 * only call saadct_configure() once. The two operating modes (continuous and
 * one-shot) are consequently built and run as two separate images selected by
 * CONFIG_SAADCT_TEST_ONE_SHOT.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <drivers/saadct.h>
#include <nrfx_saadc.h>

/* Acquisition parameters. Kept small so a series completes quickly. */
#define NUM_OF_MEAS         20
#define NUM_OF_SERIES       10
#define SAMPLE_RATE_HZ      100000
#define SAADCT_RESOLUTION   NRF_SAADC_RESOLUTION_10BIT
#define RESOLUTION_BITS     10
#define MAX_SAMPLE_VALUE    (1U << RESOLUTION_BITS)

/* Value written into every sample slot before an acquisition starts. It lies
 * well outside the ADC resolution, so if the driver ever returns a series it
 * did not actually fill, check_series_values() detects the leftover poison
 * instead of silently accepting stale (often zero) slab memory.
 */
#define SAMPLE_POISON       0xFFU

/* How many series we expect to collect before declaring the continuous
 * acquisition healthy, and the wall-clock budget allowed to collect them.
 */
#define TARGET_SERIES       3
#define COLLECT_TIMEOUT_MS  3000

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

#define NUM_OF_CHANNELS      ARRAY_SIZE(test_channels)
#define SAMPLES_PER_SERIES   (NUM_OF_MEAS * NUM_OF_CHANNELS)

#define MEAS_BLOCK_SIZE SAADCT_MEAS_BLOCK_SIZE(SAMPLES_PER_SERIES, NUM_OF_CHANNELS)

K_MEM_SLAB_DEFINE(test_slab, MEAS_BLOCK_SIZE, NUM_OF_SERIES, sizeof(void *));

static atomic_t callback_count;

static void user_handler(void *context)
{
	ARG_UNUSED(context);
	atomic_inc(&callback_count);
}

#if IS_ENABLED(CONFIG_SAADCT_TEST_ONE_SHOT)
#define TEST_MODE saadct_mode_one_shot
#else
#define TEST_MODE saadct_mode_continuous

static uint32_t collected_series;
#endif

static struct api_saadct_config test_cfg = {
	.num_of_channels = NUM_OF_CHANNELS,
	.num_of_meas = NUM_OF_MEAS,
	.resolution = SAADCT_RESOLUTION,
	.sample_rate = SAMPLE_RATE_HZ,
	.mode = TEST_MODE,
	.user_handler = user_handler,
	.user_context = NULL,
	.channels_config = test_channels,
};

#define SAADCT_NODE DT_NODELABEL(adc)
static const struct device *const saadct_dev = DEVICE_DT_GET(SAADCT_NODE);

#define ABS(a) ((a) < 0 ? -(a) : (a))

static void check_series_values(const nrf_saadc_value_t *data)
{
	for (size_t i = 0; i < SAMPLES_PER_SERIES; i++) {
		zassert_true(ABS(data[i]) < MAX_SAMPLE_VALUE,
			     "Sample %u = 0x%04x exceeds %u-bit range",
			     (unsigned int)i, (uint16_t)data[i], RESOLUTION_BITS);
	}
}

/* Pre-fill every block in the slab with an out-of-range poison byte pattern.
 * The driver hands out these blocks and is expected to overwrite the samples on
 * every completed series, so any sample slot still holding the poison afterwards
 * means the driver returned a series it never filled.
 *
 * Rather than depending on the driver's internal block layout, each block is
 * allocated and fully memset, then freed back to the slab. This avoids touching
 * the slab's free-list bookkeeping (which lives inside free blocks): allocation
 * hands out usable memory, and the subsequent free only rewrites the block's
 * first word, leaving the poisoned sample region intact.
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

static void *saadct_setup(void)
{
	int ret;

	zassert_true(device_is_ready(saadct_dev), "SAADCT device not ready");

	atomic_set(&callback_count, 0);

	ret = saadct_configure(saadct_dev, &test_cfg);
	zassert_ok(ret, "saadct_configure failed: %d", ret);

	return NULL;
}

ZTEST_SUITE(saadct_api, NULL, saadct_setup, NULL, NULL, NULL);

#if IS_ENABLED(CONFIG_SAADCT_TEST_ONE_SHOT)

ZTEST(saadct_api, test_one_shot_single_series)
{
	int ret;
	nrf_saadc_value_t *data;
	int64_t deadline;

	poison_slab(&test_slab);

	ret = saadct_start(saadct_dev, &test_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* Wait for the single series to complete. */
	deadline = k_uptime_get() + COLLECT_TIMEOUT_MS;
	while (saadct_meas_pending(saadct_dev) == 0 &&
	       k_uptime_get() < deadline) {
		k_msleep(1);
	}

	zassert_equal(saadct_meas_pending(saadct_dev), 1,
		      "Expected exactly one pending series in one-shot mode");

	ret = saadct_meas_get(saadct_dev, &data);
	zassert_ok(ret, "saadct_meas_get failed: %d", ret);
	check_series_values(data);
	saadct_meas_free(saadct_dev, data);

	/* One-shot must stop by itself, so no further series may appear. */
	k_msleep(50);
	zassert_equal(saadct_meas_pending(saadct_dev), 0,
		      "One-shot produced more than one series");

	ret = saadct_meas_get(saadct_dev, &data);
	zassert_equal(ret, -EIO,
		      "Expected -EIO when no measurement is available, got %d",
		      ret);
}

#else /* continuous mode */

ZTEST(saadct_api, test_continuous_acquisition)
{
	int ret;
	nrf_saadc_value_t *data;
	int64_t deadline;
	uint32_t previous_pending;

	poison_slab(&test_slab);

	ret = saadct_start(saadct_dev, &test_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* Collect a number of series, verifying each one holds valid samples. */
	deadline = k_uptime_get() + COLLECT_TIMEOUT_MS;
	while (collected_series < TARGET_SERIES && k_uptime_get() < deadline) {
		if (saadct_meas_pending(saadct_dev) == 0) {
			k_usleep(50);
			continue;
		}

		ret = saadct_meas_get(saadct_dev, &data);
		zassert_ok(ret, "saadct_meas_get failed: %d", ret);
		check_series_values(data);
		saadct_meas_free(saadct_dev, data);
		collected_series++;
	}

	zassert_true(collected_series >= TARGET_SERIES,
		     "Collected only %u series (expected >= %u)",
		     collected_series, TARGET_SERIES);

	/* Stop immediately and drain whatever series already completed. */
	ret = saadct_stop(saadct_dev, false);
	zassert_ok(ret, "saadct_stop failed: %d", ret);

	previous_pending = saadct_meas_pending(saadct_dev);
	while (saadct_meas_pending(saadct_dev) > 0) {
		uint32_t now_pending = saadct_meas_pending(saadct_dev);

		zassert_true(now_pending <= previous_pending,
			     "Pending count grew after immediate stop");
		previous_pending = now_pending;

		ret = saadct_meas_get(saadct_dev, &data);
		zassert_ok(ret, "saadct_meas_get while draining failed: %d",
			   ret);
		saadct_meas_free(saadct_dev, data);
	}

	/* All completed series have been consumed: the pending counter is back
	 * to zero and no further complete series can be read.
	 */
	zassert_equal(saadct_meas_pending(saadct_dev), 0,
		      "Pending count non-zero after draining");

	ret = saadct_meas_get(saadct_dev, &data);
	zassert_true(ret < 0,
		     "Expected an error after draining all series, got %d", ret);
}

ZTEST(saadct_api, test_user_callback_invoked)
{
	/* The configured user_handler must be called once per completed series.
	 * This depends on test_continuous_acquisition having collected series
	 * first (tests run in definition order).
	 */
	zassert_true(collected_series > 0,
		     "No series were collected, cannot check callback");

	zassert_true(atomic_get(&callback_count) > 0,
		     "user_handler was never invoked");

	zassert_equal(atomic_get(&callback_count), (atomic_val_t)TARGET_SERIES,
		      "user_handler called %ld times, expected %u (one per series)",
		      (long)atomic_get(&callback_count), TARGET_SERIES);
}

#endif /* CONFIG_SAADCT_TEST_ONE_SHOT */
