/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
#if defined(CONFIG_HAS_NORDIC_DMM)
#include <dmm.h>
#endif

#define PDM_SAMPLING_RATE   CONFIG_TEST_PDM_SAMPLING_RATE
#define PDM_SAMPLING_RATE_DUMMY  32000
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

#if CONFIG_TEST_USE_DMM
struct k_mem_slab mem_slab;
char __aligned(WB_UP(4)) mem_slab_buffer[BLOCK_COUNT * WB_UP(MAX_BLOCK_SIZE)]
					 DMM_MEMORY_SECTION(DT_NODELABEL(pdm_dev));
#else
K_MEM_SLAB_DEFINE_STATIC(mem_slab, MAX_BLOCK_SIZE, BLOCK_COUNT, 4);
#endif

#define CLOCK_INPUT_PIN	NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(pulse_counter), gpios)

static const struct device *const pdm_dev = DEVICE_DT_GET(DT_NODELABEL(pdm_dev));
static const nrfx_gpiote_t gpiote_instance = NRFX_GPIOTE_INSTANCE(
					     NRF_DT_GPIOTE_INST(
					     DT_NODELABEL(pulse_counter), gpios));
static struct pcm_stream_cfg stream_config, stream_config_dummy;
static struct dmic_cfg pdm_cfg, pdm_cfg_dummy;

#if CONFIG_NRFX_TIMER00
static const nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(00);
#elif CONFIG_NRFX_TIMER130
static const nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(130);
#else
#error "No timer instance found"
#endif

void timer_handler(nrf_timer_event_t event, void *p_context)
{
	ARG_UNUSED(event);
	ARG_UNUSED(p_context);
}

static void *device_setup(void)
{
	int ret;

	ret = device_is_ready(pdm_dev);
	zassert_true(ret, "PDM device is not ready, return code = %d", ret);

#if CONFIG_TEST_USE_DMM
	ret = k_mem_slab_init(&mem_slab, mem_slab_buffer, WB_UP(MAX_BLOCK_SIZE), BLOCK_COUNT);
	zassert_true(ret >= 0, "Memory slab initialization failed, return code = %d", ret);
#endif

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

	/* Dummy cfg to test that reconfiguration works */
	stream_config_dummy.pcm_width = SAMPLE_BIT_WIDTH;
	stream_config_dummy.pcm_rate = PDM_SAMPLING_RATE_DUMMY;
	stream_config_dummy.mem_slab = &mem_slab;
	stream_config_dummy.block_size = BLOCK_SIZE(PDM_SAMPLING_RATE, 1);

	pdm_cfg_dummy.io.min_pdm_clk_freq = 1000000;
	pdm_cfg_dummy.io.max_pdm_clk_freq = 2000000;
	pdm_cfg_dummy.io.min_pdm_clk_dc   = 40;
	pdm_cfg_dummy.io.max_pdm_clk_dc   = 60;
	pdm_cfg_dummy.streams = &stream_config_dummy,
	pdm_cfg_dummy.channel.req_num_streams = 1;
	pdm_cfg_dummy.channel.req_num_chan = 1;
	pdm_cfg_dummy.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
}

static void teardown(void *unused)
{
	ARG_UNUSED(unused);

	int ret;

	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_STOP);
	zassert_true(ret >= 0, "PDM start trigger failed, return code = %d", ret);
}

static void capture_callback(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	nrfx_timer_capture(&timer_instance, NRF_TIMER_CC_CHANNEL0);
}

K_TIMER_DEFINE(capture_timer, capture_callback, NULL);

static void pdm_transfer(const struct device *pdm_dev,
			   struct dmic_cfg *pdm_cfg,
			   size_t block_count)
{
	int ret;

#if PDM_SAMPLING_RATE != PDM_SAMPLING_RATE_DUMMY
	/* Dummy configuration to test if device can be reconfigured.
	 *
	 * When both configurations are the same, apply it only once
	 * to test that single configuration also works.
	 */
	ret = dmic_configure(pdm_dev, &pdm_cfg_dummy);
	zassert_true(ret >= 0, "PDM configuration failed, return code = %d", ret);
#endif

	ret = dmic_configure(pdm_dev, pdm_cfg);
	zassert_true(ret >= 0, "PDM configuration failed, return code = %d", ret);

	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_START);
	zassert_true(ret >= 0, "PDM start trigger failed, return code = %d", ret);

	/* Count clock cycles during 50% of transfer time. */
	k_timer_start(&capture_timer, K_MSEC(BLOCK_COUNT / 2 * PDM_SAMPLING_TIME), K_NO_WAIT);

	void *buffer;
	uint32_t size;

	ret = dmic_read(pdm_dev, 0, &buffer, &size, PDM_READ_TIMEOUT);
	zassert_true(ret >= 0, "PDM read failed, return code = %d", ret);

	k_mem_slab_free(&mem_slab, buffer);

	k_msleep(BLOCK_COUNT / 2 * PDM_SAMPLING_TIME);

	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_STOP);
	zassert_true(ret >= 0, "PDM stop trigger failed, return code = %d", ret);
}

ZTEST(pdm_loopback, test_pdm_configure)
{
	int ret;

	/* Not enough channels. */
	pdm_cfg.channel.req_num_chan = 0;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret == -EINVAL, "PDM configuration should fail, return code = %d", ret);

	/* Too many channels. */
	pdm_cfg.channel.req_num_chan = 3;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret == -EINVAL, "PDM configuration should fail, return code = %d", ret);

	/* Streams not equal to 1. */
	pdm_cfg.channel.req_num_chan = 1;
	pdm_cfg.channel.req_num_streams = 0;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret == -EINVAL, "PDM configuration should fail, return code = %d", ret);

	/* Streams not equal to 1. */
	pdm_cfg.channel.req_num_chan = 1;
	pdm_cfg.channel.req_num_streams = 2;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret == -EINVAL, "PDM configuration should fail, return code = %d", ret);

	/* Invalid sample bit width. */
	pdm_cfg.channel.req_num_streams = 1;
	pdm_cfg.streams->pcm_width = 32;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret == -EINVAL, "PDM configuration should fail, return code = %d", ret);

	/* Invalid sample rate, unable to provide suitable clock. */
	pdm_cfg.streams->pcm_width = SAMPLE_BIT_WIDTH;
	pdm_cfg.streams->pcm_rate = 100;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret == -EINVAL, "PDM configuration should fail, return code = %d", ret);

	/* Proper configuration. */
	pdm_cfg.streams->pcm_rate = PDM_SAMPLING_RATE;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret >= 0, "PDM configuration failed, return code = %d", ret);

	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_START);
	zassert_true(ret >= 0, "PDM start trigger failed, return code = %d", ret);

	/* Device is running, unable to configure. */
	pdm_cfg.streams->pcm_rate = PDM_SAMPLING_RATE;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret == -EBUSY, "PDM configuration should fail, return code = %d", ret);
}

ZTEST(pdm_loopback, test_start_trigger)
{
	int ret;

	/* Disable device. */
	pdm_cfg.streams->pcm_rate = 0;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret >= 0, "PDM configuration failed, return code = %d", ret);

	/* Device not configured. */
	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_START);
	zassert_true(ret == -EIO, "PDM start trigger should fail, return code = %d", ret);

	pdm_cfg.streams->pcm_rate = PDM_SAMPLING_RATE;
	ret = dmic_configure(pdm_dev, &pdm_cfg);
	zassert_true(ret >= 0, "PDM configuration failed, return code = %d", ret);

	/* Proper start. */
	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_START);
	zassert_true(ret >= 0, "PDM start trigger failed, return code = %d", ret);

	/* Multiple start triggers should return no error. */
	ret = dmic_trigger(pdm_dev, DMIC_TRIGGER_START);
	zassert_true(ret >= 0, "PDM start trigger failed, return code = %d", ret);
}

ZTEST(pdm_loopback, test_pdm_clk_frequency)
{
	int ret;

	uint8_t gpiote_channel;

	ret = nrfx_gpiote_channel_alloc(&gpiote_instance, &gpiote_channel);
	zassert_true(ret == NRFX_SUCCESS,
		     "GPIOTE channel allocation failed, return code = 0x%08X", ret);

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
	zassert_true(ret == NRFX_SUCCESS,
		     "GPIOTE input configuration failed, return code = 0x%08X", ret);

	nrfx_gpiote_trigger_enable(&gpiote_instance, CLOCK_INPUT_PIN, false);

	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(
					   NRFX_TIMER_BASE_FREQUENCY_GET(&timer_instance));
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.mode      = NRF_TIMER_MODE_COUNTER;

	ret = nrfx_timer_init(&timer_instance, &timer_config, timer_handler);
	zassert_true(ret == NRFX_SUCCESS,
		     "TIMER initialization failed, return code = 0x%08X", ret);

	nrfx_timer_enable(&timer_instance);

	uint8_t gppi_channel;

	ret = nrfx_gppi_channel_alloc(&gppi_channel);

	zassert_true(ret == NRFX_SUCCESS,
			    "GPPI channel allocation failed, return code = 0x%08X", ret);
	nrfx_gppi_channel_endpoints_setup(gppi_channel,
					  nrfx_gpiote_in_event_address_get(&gpiote_instance,
									   CLOCK_INPUT_PIN),
					  nrfx_timer_task_address_get(&timer_instance,
								      NRF_TIMER_TASK_COUNT));
	nrfx_gppi_channels_enable(BIT(gppi_channel));

	pdm_transfer(pdm_dev, &pdm_cfg, BLOCK_COUNT);

	uint32_t pulses = nrfx_timer_capture_get(&timer_instance, NRF_TIMER_CC_CHANNEL0);

	/* Assert that captured frequency is within 3% margin of expected one. */
	zassert_within(pulses, PDM_EXPECTED_FREQ * SAMPLING_RATIO,
		       PDM_EXPECTED_FREQ * SAMPLING_RATIO / 30,
		       "Captured incorrect frequency Hz. Captured pulses = %lu, expected = %lu",
		       pulses, PDM_EXPECTED_FREQ * SAMPLING_RATIO);
}

ZTEST_SUITE(pdm_loopback, NULL, device_setup, setup, teardown, NULL);
