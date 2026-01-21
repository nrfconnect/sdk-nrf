/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/ztest.h>
#include "nrfx_pdm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pdm_instances);

#define SAMPLE_RATE  16000
#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE (SAMPLE_BIT_WIDTH / 8)
#define NO_OF_CHANNELS	1

/* Milliseconds to wait for a block to be captured by PCM peripheral. */
#define READ_TIMEOUT     1200

/* Driver will allocate blocks from this slab to receive audio data into them.
 * Application, after getting a given block from the driver and processing its
 * data, needs to free that block.
 */
#define AUDIO_BLOCK_SIZE   (BYTES_PER_SAMPLE * SAMPLE_RATE * NO_OF_CHANNELS / 40)
/* Driver allocates memory "in advance" therefore 2 blocks may be not enough. */
#define BLOCK_COUNT      4

K_MEM_SLAB_DEFINE_STATIC(mem_slab, AUDIO_BLOCK_SIZE, BLOCK_COUNT, 4);

/* Generate a list of devices for all instances of the "compat" */
#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_AUDIO_DMIC_NRFX_PDM
	DEVS_FOR_DT_COMPAT(nordic_nrf_pdm)
#endif
};

/* Generate a list of device's addresses for all instances of the "compat" */
#define DEVICE_DT_REG_ADDR_AND_COMMA(node_id) (NRF_PDM_Type *)DT_REG_ADDR(node_id),
#define DEV_REGS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_REG_ADDR_AND_COMMA)

typedef void (*test_func_t)(const struct device *dev);
typedef bool (*capability_func_t)(const struct device *dev);

static void setup_instance(const struct device *dev)
{
	/* Left for future test expansion. */
}

static void tear_down_instance(const struct device *dev)
{
	/* Left for future test expansion. */
}

static void test_all_instances(test_func_t func, capability_func_t capability_check)
{
	int devices_skipped = 0;

	zassert_true(ARRAY_SIZE(devices) > 0, "No device found");
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		setup_instance(devices[i]);
		TC_PRINT("\nInstance %u: ", i + 1);
		if ((capability_check == NULL) ||
		     capability_check(devices[i])) {
			TC_PRINT("Testing %s\n", devices[i]->name);
			func(devices[i]);
		} else {
			TC_PRINT("Skipped for %s\n", devices[i]->name);
			devices_skipped++;
		}
		tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
	if (devices_skipped == ARRAY_SIZE(devices)) {
		ztest_test_skip();
	}
}


/**
 * Test validates if instance can receive data.
 */
static void test_get_any_data_instance(const struct device *dev)
{
	int ret;
	void *buffer;
	uint32_t size;

	struct pcm_stream_cfg stream = {
		.pcm_rate = SAMPLE_RATE,
		.pcm_width = SAMPLE_BIT_WIDTH,
		.block_size = AUDIO_BLOCK_SIZE,
		.mem_slab  = &mem_slab,
	};

	struct dmic_cfg cfg = {
		.io = {
			/* These fields can be used to limit the PDM clock
			 * configurations that the driver is allowed to use
			 * to those supported by the microphone.
			 */
			.min_pdm_clk_freq = 1000000,
			.max_pdm_clk_freq = 3250000,
			.min_pdm_clk_dc   = 40,
			.max_pdm_clk_dc   = 60,
		},
		.streams = &stream,
		.channel = {
			.req_num_streams = 1,
			.req_num_chan = 1,
			.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT),
		},
	};

	ret = dmic_configure(dev, &cfg);
	zassert_equal(ret, 0, "Failed to configure the dmic device\n");

	ret = dmic_trigger(dev, DMIC_TRIGGER_START);
	zassert_equal(ret, 0, "Failed to trigger START\n");

	ret = dmic_read(dev, 0, &buffer, &size, READ_TIMEOUT);
	zassert_equal(ret, 0, "Failed to read data\n");

	ret = dmic_trigger(dev, DMIC_TRIGGER_STOP);
	zassert_equal(ret, 0, "Failed to trigger STOP\n");

	/* Calculate average from audio samples. */
	uint16_t *pcm_out = buffer;
	uint32_t average = 0;

	for (int j = 0; j < size/2; j++) {
		average += pcm_out[j];
	}
	average /= (size / 2);
	TC_PRINT("Average from audio samples is %u\n", average);

	k_mem_slab_free(&mem_slab, buffer);

	zassert_true(average > 0, "All samples are 0!\n");
}

static bool test_get_any_data_capable(const struct device *dev)
{
	return true;
}

ZTEST(pdm_instances, test_get_any_data)
{
	test_all_instances(test_get_any_data_instance, test_get_any_data_capable);
}

/* Check that following issue is not observed:
 * Subsequent PDM PRESCALER value not updated
 * after the first time the register is written
 * while the PDM is stopped.
 */
ZTEST(pdm_instances, test_prescaler_can_be_changed_when_pdm_is_stopped)
{
#if defined(CONFIG_SOC_NRF54H20)
	/* On nrf54H20 there is no PRESCALER register */
	ztest_test_skip();
#else
	volatile NRF_PDM_Type *p_reg[] = {
#if defined(CONFIG_AUDIO_DMIC_NRFX_PDM)
		DEV_REGS_FOR_DT_COMPAT(nordic_nrf_pdm)
#endif /* defined(CONFIG_AUDIO_DMIC_NRFX_PDM) */
	};

	zassert_true(ARRAY_SIZE(devices) > 0, "No device found");
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		TC_PRINT("Instance %u: Testing %s at %p\n",
			i + 1, devices[i]->name, p_reg[i]);

		/* Stop PDM */
		p_reg[i]->TASKS_STOP = 1;

		TC_PRINT("prescaler =");
		/* Check that PRESCALER can be modified multiple times */
		for (uint32_t val = PDM_PRESCALER_DIVISOR_Min;
			val <= PDM_PRESCALER_DIVISOR_Max; val++) {

			TC_PRINT(" %u,", val);
			p_reg[i]->PRESCALER = val;
			zassert_equal(p_reg[i]->PRESCALER, val);
		}
		TC_PRINT("\n");

		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
#endif /* defined(CONFIG_SOC_NRF54H20) */
}

static void *test_setup(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]),
			     "Device %s is not ready", devices[i]->name);
		k_object_access_grant(devices[i], k_current_get());
	}

	return NULL;
}

ZTEST_SUITE(pdm_instances, NULL, test_setup, NULL, NULL, NULL);
