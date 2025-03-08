/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dmic_sample);

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
#define AUDIO_BLOCK_SIZE   (BYTES_PER_SAMPLE * SAMPLE_RATE * NO_OF_CHANNELS / 10)
/* Why driver fails when BLOCK_COUNT is decreased to 2? */
#define BLOCK_COUNT      4
K_MEM_SLAB_DEFINE_STATIC(mem_slab, AUDIO_BLOCK_SIZE, BLOCK_COUNT, 4);

int main(void)
{
	const struct device *const dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));
	int ret;
	int loop_counter = 1;
	void *buffer;
	uint32_t size;

	if (!device_is_ready(dmic_dev)) {
		LOG_ERR("%s is not ready", dmic_dev->name);
		return 0;
	}

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
			.req_num_chan = 1,
			.req_num_streams = 1,
			.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT),
		},
	};

	ret = dmic_configure(dmic_dev, &cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure the driver: %d", ret);
		return ret;
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("START trigger failed: %d", ret);
		return ret;
	}

	while (1) {
		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%d - read failed: %d", loop_counter, ret);
			return ret;
		}

		/* Print buffer on serial in binary mode */
		unsigned char pcm_l, pcm_h;
		int j;

		uint16_t *pcm_out = buffer;
		// uint8_t *pcm_out = buffer;

		for (j = 2; j < size/2; j++) {
			pcm_l = (char)(pcm_out[j] & 0xFF);
			pcm_h = (char)((pcm_out[j] >> 8) & 0xFF);

			z_impl_k_str_out(&pcm_l, 1);
			z_impl_k_str_out(&pcm_h, 1);
		}

		// for (j = 0; j < size; j+=2) {
		// 	pcm_l = (char)pcm_out[j];
		// 	pcm_h = (char)pcm_out[j+1];

		// 	z_impl_k_str_out(&pcm_l, 1);
		// 	z_impl_k_str_out(&pcm_h, 1);
		// }

		k_mem_slab_free(&mem_slab, buffer);

		loop_counter++;
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		LOG_ERR("STOP trigger failed: %d", ret);
		return ret;
	}

	return 0;
}
