/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/gpio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>

#define PDM_SAMPLING_RATE   CONFIG_TEST_PDM_SAMPLING_RATE
#define PDM_EXPECTED_FREQ   CONFIG_TEST_PDM_EXPECTED_FREQUENCY
#define PDM_SAMPLING_TIME   CONFIG_TEST_PDM_SAMPLING_TIME
#define PDM_READ_TIMEOUT    1000

#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE sizeof(int16_t)

#define BLOCK_SIZE(_sample_rate, _number_of_channels) \
	(BYTES_PER_SAMPLE * (_sample_rate * PDM_SAMPLING_TIME / 1000) * _number_of_channels)

#define MAX_BLOCK_SIZE   BLOCK_SIZE(PDM_SAMPLING_RATE, 1)
#define BLOCK_COUNT      4
#define SAMPLING_RATIO   PDM_SAMPLING_TIME / 1000 * BLOCK_COUNT / 2

K_MEM_SLAB_DEFINE_STATIC(mem_slab, MAX_BLOCK_SIZE, BLOCK_COUNT, 4);

#define CLOCK_INPUT_PIN	NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(pulse_counter), gpios)

volatile NRF_PDM_Type *p_reg = (NRF_PDM_Type *)DT_REG_ADDR(DT_NODELABEL(pdm_dev));

static const struct device *const pdm_dev = DEVICE_DT_GET(DT_NODELABEL(pdm_dev));
static struct pcm_stream_cfg stream_config;
static struct dmic_cfg pdm_cfg;

#if defined(NRF_TIMER00)
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER00);
#elif defined(NRF_TIMER130)
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER130);
#else
#error "No timer instance found"
#endif

static bool pdm_enabled;

static void *device_setup(void)
{
	int ret;

	ret = device_is_ready(pdm_dev);
	zassert_true(ret, "PDM device is not ready, return code = %d", ret);

	return NULL;
}

static void setup(void *unused)
{
	ARG_UNUSED(unused);

	stream_config.pcm_width = SAMPLE_BIT_WIDTH;
	stream_config.pcm_rate = PDM_SAMPLING_RATE;
	stream_config.mem_slab = &mem_slab;
	stream_config.block_size = BLOCK_SIZE(PDM_SAMPLING_RATE, 1);

	pdm_cfg.io.min_pdm_clk_freq = 1000000;
	pdm_cfg.io.max_pdm_clk_freq = 2000000;
	pdm_cfg.io.min_pdm_clk_dc   = 40;
	pdm_cfg.io.max_pdm_clk_dc   = 60;
	pdm_cfg.streams = &stream_config,
	pdm_cfg.channel.req_num_streams = 1;
	pdm_cfg.channel.req_num_chan = 1;
	pdm_cfg.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
}

static void teardown(void *unused)
{
	ARG_UNUSED(unused);

	int ret;

	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_STOP);
	zassert_true(ret >= 0, "dmic_trigger() failed, ret = %d", ret);
}

static void capture_callback(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	nrfx_timer_capture(&timer_instance, NRF_TIMER_CC_CHANNEL0);
	pdm_enabled = false;
}

K_TIMER_DEFINE(capture_timer, capture_callback, NULL);

ZTEST(pdm_prescaler, test_prescaler_not_affected_by_stop_start)
{
	int ret;
	uint32_t prescaler_1;	/* Prescaler value after applying test configuration. */
	uint32_t prescaler_temp;
	const uint32_t ITERATIONS = 5;

	uint8_t gpiote_channel;
	nrfx_gpiote_t gpiote_instance =
		GPIOTE_NRFX_INST_BY_NODE(NRF_DT_GPIOTE_NODE(DT_NODELABEL(pulse_counter), gpios));

	void *buffer;
	uint32_t size;

	ret = nrfx_gpiote_channel_alloc(&gpiote_instance, &gpiote_channel);
	zassert_true(ret == 0, "GPIOTE channel allocation failed, return code = %d", ret);

	nrfx_gpiote_trigger_config_t trigger_cfg = {
		.p_in_channel = &gpiote_channel,
		.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI,
	};

	nrf_gpio_pin_pull_t pull_cfg = NRFX_GPIOTE_DEFAULT_PULL_CONFIG;

	nrfx_gpiote_input_pin_config_t gpiote_cfg = {
		.p_pull_config = &pull_cfg,
		.p_trigger_config = &trigger_cfg,
	};

	ret = nrfx_gpiote_input_configure(&gpiote_instance, CLOCK_INPUT_PIN, &gpiote_cfg);
	zassert_true(ret == 0, "GPIOTE input configuration failed, return code = %d", ret);

	nrfx_gpiote_trigger_enable(&gpiote_instance, CLOCK_INPUT_PIN, false);

	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(
		NRFX_TIMER_BASE_FREQUENCY_GET(&timer_instance));
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.mode      = NRF_TIMER_MODE_COUNTER;

	ret = nrfx_timer_init(&timer_instance, &timer_config, NULL);
	zassert_true(ret == 0, "nrfx_timer_init() failed, ret = 0x%08X", ret);

	nrfx_timer_enable(&timer_instance);

	nrfx_gppi_handle_t gppi_handle;
	uint32_t eep = nrfx_gpiote_in_event_address_get(&gpiote_instance, CLOCK_INPUT_PIN);
	uint32_t tep = nrfx_timer_task_address_get(&timer_instance, NRF_TIMER_TASK_COUNT);

	ret = nrfx_gppi_conn_alloc(eep, tep, &gppi_handle);
	zassert_equal(ret, 0, "GPPI channel allocation failed, return code = %d", ret);
	nrfx_gppi_conn_enable(gppi_handle);

	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret >= 0, "dmic_configure() failed, ret = %d", ret);

	prescaler_1 = p_reg->PRESCALER;
	TC_PRINT("Prescaler after device configuration: %u\n", prescaler_1);
	zassert_not_equal(prescaler_1, PDM_PRESCALER_ResetValue,
		"Selected configuration sets identical PRESCALER to the default value");

	for (int i = 1; i <= ITERATIONS; i++) {
		TC_PRINT("Iteration %u\n", i);

		/* Clear clock cycles counter */
		nrfx_timer_clear(&timer_instance);

		/* When kerel timer expires:
		 *  - NRFX timer counter value is stored;
		 *  - pdm_enabled is set to false.
		 */
		pdm_enabled = true;
		k_timer_start(&capture_timer, K_SECONDS(1), K_NO_WAIT);

		ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_START);
		zassert_true(ret >= 0, "PDM start trigger failed, return code = %d", ret);

		while (pdm_enabled) {
			ret = dmic_read(pdm_dev, 0, &buffer, &size, PDM_READ_TIMEOUT);
			zassert_true(ret >= 0, "PDM read failed, return code = %d", ret);

			k_mem_slab_free(&mem_slab, buffer);
		}

		ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_STOP);
		zassert_true(ret >= 0, "PDM stop trigger failed, return code = %d", ret);

		/* Get number of PDM_CLK edges. */
		uint32_t pulses = nrfx_timer_capture_get(&timer_instance, NRF_TIMER_CC_CHANNEL0);

		/* Assert that captured frequency is within 3% margin of expected one. */
		TC_PRINT("NRFX Timer counted to %u\n", pulses);
		zassert_within(pulses, PDM_EXPECTED_FREQ, PDM_EXPECTED_FREQ / 30,
			"Captured incorrect frequency Hz. Captured pulses = %lu, expected = %lu",
			pulses, PDM_EXPECTED_FREQ);

		/* Check PRESCALER value */
		prescaler_temp = p_reg->PRESCALER;
		TC_PRINT("Prescaler after transmission: %u\n", prescaler_temp);
		zassert_equal(prescaler_1, prescaler_temp, "PRESCALER has changed");
	}

	/* Remove GPPI configuration. */
	nrfx_gppi_conn_disable(gppi_handle);
	nrfx_gppi_conn_free(eep, tep, gppi_handle);

	/* Remove GPIOTE configuration. */
	ret = nrfx_gpiote_pin_uninit(&gpiote_instance, CLOCK_INPUT_PIN);
	zexpect_true(ret == 0, "nrfx_gpiote_pin_uninit() ret %d", ret);
	nrfx_gpiote_trigger_disable(&gpiote_instance, CLOCK_INPUT_PIN);
	ret = nrfx_gpiote_channel_free(&gpiote_instance, gpiote_channel);
	zexpect_true(ret == 0, "nrfx_gpiote_channel_free() ret %d", ret);

	/* Remove NRFX Timer configuration. */
	nrfx_timer_disable(&timer_instance);
	nrfx_timer_uninit(&timer_instance);
}

ZTEST_SUITE(pdm_prescaler, NULL, device_setup, setup, teardown, NULL);
