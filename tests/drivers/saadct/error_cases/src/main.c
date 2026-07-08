/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Error-path tests for the SAADCT (SAADC + TIMER) driver.
 *
 * These tests verify that the driver reports the documented error codes when it
 * is used incorrectly:
 *   - saadct_get() while a series is still in progress   -> -EAGAIN
 *   - saadct_get() when no data is available / stopped   -> -EIO
 *   - saadct_start() with NULL pointer instead of a slab -> -EINVAL
 *   - saadct_start() with an exhausted memory slab       -> -ENOMEM
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <drivers/saadct.h>
#include <nrfx_saadc.h>

#define NUM_OF_MEAS            10
#define SAMPLE_RATE_HZ         KHZ(100)
#define SAADCT_RESOLUTION      NRF_SAADC_RESOLUTION_10BIT
#define MEASUREMENT_TIMEOUT_MS 100

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

#define NUM_OF_CHANNELS ARRAY_SIZE(test_channels)
#define MEAS_BLOCK_SIZE SAADCT_MEAS_BLOCK_SIZE(NUM_OF_MEAS, NUM_OF_CHANNELS)
/* Healthy slab with enough blocks for normal operation. */
K_MEM_SLAB_DEFINE(good_slab, MEAS_BLOCK_SIZE, 4, sizeof(void *));

BUILD_ASSERT(MEAS_BLOCK_SIZE == 44);

/* Single-block slab used to provoke the out-of-memory path. */
K_MEM_SLAB_DEFINE(enomem_slab, MEAS_BLOCK_SIZE, 1, sizeof(void *));

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

static void *saadct_setup(void)
{
	int ret;

	zassert_true(device_is_ready(saadct_dev), "SAADCT device not ready");

	ret = saadct_configure(saadct_dev, &test_cfg);
	zassert_ok(ret, "saadct_configure failed: %d", ret);

	return NULL;
}

/*
 * Wait for any in-flight one-shot acquisition to complete and drain every
 * collected series, so tests do not leak buffers or pending series into each
 * other.
 */
static void saadct_after(void *fixture)
{
	ARG_UNUSED(fixture);
	nrf_saadc_value_t *data;

	k_msleep(MEASUREMENT_TIMEOUT_MS);
	if (saadct_pending(saadct_dev) > 0) {
		while (saadct_get(saadct_dev, &data, K_MSEC(MEASUREMENT_TIMEOUT_MS)) == 0) {
			saadct_put(saadct_dev, data);
		}
	}
}

ZTEST_SUITE(saadct_error_cases, NULL, saadct_setup, NULL, saadct_after, NULL);

ZTEST(saadct_error_cases, test_meas_get_in_progress_returns_eagain)
{
	int ret;
	nrf_saadc_value_t *data;

	ret = saadct_start(saadct_dev, &good_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* A block has been allocated for the ongoing series but no series has
	 * completed yet, so there is nothing to read back.
	 */
	ret = saadct_get(saadct_dev, &data, K_NO_WAIT);
	zassert_equal(ret, -EAGAIN,
			  "Expected -EAGAIN while a series is in progress, got %d",
			  ret);
}

ZTEST(saadct_error_cases, test_meas_get_no_data_returns_eio)
{
	int ret;
	nrf_saadc_value_t *data;

	ret = saadct_start(saadct_dev, &good_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* Let the single one-shot series complete, then consume it. */
	k_msleep(MEASUREMENT_TIMEOUT_MS);
	zassert_true(saadct_pending(saadct_dev) == 1,
			 "One-shot series did not complete in time");

	ret = saadct_get(saadct_dev, &data, K_NO_WAIT);
	zassert_ok(ret, "saadct_get failed: %d", ret);
	saadct_put(saadct_dev, data);

	/* All data consumed and the one-shot acquisition has stopped: further
	 * reads must report -EIO.
	 */
	ret = saadct_get(saadct_dev, &data, K_NO_WAIT);
	zassert_equal(ret, -EIO,
	"Expected -EIO when no measurement is available, got %d",
			  ret);
}

ZTEST(saadct_error_cases, test_start_out_of_memory_returns_enomem)
{
	int ret;
	void *block;

	/* Exhaust the slab so the driver cannot allocate a measurement block. */
	ret = k_mem_slab_alloc(&enomem_slab, &block, K_NO_WAIT);
	zassert_ok(ret, "Failed to pre-allocate the only slab block: %d", ret);

	ret = saadct_start(saadct_dev, &enomem_slab);
	zassert_equal(ret, -ENOMEM,
			  "Expected -ENOMEM when the slab is exhausted, got %d",
			  ret);

	k_mem_slab_free(&enomem_slab, block);
}

ZTEST(saadct_error_cases, test_start_null_slab)
{
	int ret;

	ret = saadct_start(saadct_dev, NULL);
	zassert_equal(ret, -EINVAL,
			  "Expected -EINVAL when slab is NULL, got %d",
			  ret);
}
