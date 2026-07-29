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
 *   - saadct_meas_get() while a series is still in progress   -> -EAGAIN
 *   - saadct_meas_get() when no data is available / stopped   -> -EIO
 *   - saadct_start() with an exhausted memory slab            -> -ENOMEM
 *
 * The suite runs in one-shot mode: an acquisition produces exactly one series
 * and stops itself, which keeps the driver's (asynchronous) state machine
 * deterministic between tests.
 *
 * The driver initializes the underlying nrfx SAADC only once (there is no
 * de-init entry point), so saadct_configure() is called a single time in the
 * suite setup and reused by every test.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <drivers/saadct.h>
#include <nrfx_saadc.h>

#define NUM_OF_MEAS         10
#define SAMPLE_RATE_HZ      100000
#define SAADCT_RESOLUTION   NRF_SAADC_RESOLUTION_10BIT
#define SERIES_TIMEOUT_MS   3000

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

#define MEAS_BLOCK_SIZE                                                           \
	ROUND_UP(sizeof(void *) + SAMPLES_PER_SERIES * sizeof(nrf_saadc_value_t), \
		 sizeof(void *))

/* Healthy slab with enough blocks for normal operation. */
K_MEM_SLAB_DEFINE(good_slab, MEAS_BLOCK_SIZE, 8, sizeof(void *));

/* Single-block slab used to provoke the out-of-memory path. */
K_MEM_SLAB_DEFINE(enomem_slab, MEAS_BLOCK_SIZE, 1, sizeof(void *));

static struct api_saadct_config test_cfg = {
	.num_of_channels = NUM_OF_CHANNELS,
	.num_of_meas = NUM_OF_MEAS,
	.resolution = SAADCT_RESOLUTION,
	.sample_rate = SAMPLE_RATE_HZ,
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
	int64_t deadline = k_uptime_get() + SERIES_TIMEOUT_MS;

	while (k_uptime_get() < deadline) {
		while (saadct_meas_get(saadct_dev, &data) == 0) {
			saadct_meas_free(saadct_dev, data);
		}

		/* No blocks held by the driver means nothing is in flight. */
		if (k_mem_slab_num_used_get(&good_slab) == 0) {
			break;
		}
		k_msleep(2);
	}

	while (saadct_meas_get(saadct_dev, &data) == 0) {
		saadct_meas_free(saadct_dev, data);
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
	ret = saadct_meas_get(saadct_dev, &data);
	zassert_equal(ret, -EAGAIN,
			  "Expected -EAGAIN while a series is in progress, got %d",
			  ret);

	/* Cleanup (waiting for completion + drain) is done in saadct_after(). */
}

ZTEST(saadct_error_cases, test_meas_get_no_data_returns_eio)
{
	int ret;
	nrf_saadc_value_t *data;
	int64_t deadline;

	ret = saadct_start(saadct_dev, &good_slab);
	zassert_ok(ret, "saadct_start failed: %d", ret);

	/* Let the single one-shot series complete, then consume it. */
	deadline = k_uptime_get() + SERIES_TIMEOUT_MS;
	while (saadct_meas_pending(saadct_dev) == 0 && k_uptime_get() < deadline) {
		k_msleep(1);
	}
	zassert_true(saadct_meas_pending(saadct_dev) >= 1,
			 "One-shot series did not complete in time");

	ret = saadct_meas_get(saadct_dev, &data);
	zassert_ok(ret, "saadct_meas_get failed: %d", ret);
	saadct_meas_free(saadct_dev, data);

	/* All data consumed and the one-shot acquisition has stopped: further
	 * reads must report -EIO.
	 */
	ret = saadct_meas_get(saadct_dev, &data);
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
