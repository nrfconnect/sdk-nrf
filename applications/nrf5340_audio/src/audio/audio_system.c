/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_system.h"

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <contin_array.h>
#include <pcm_stream_channel_modifier.h>
#include <tone.h>

#include "macros_common.h"
#include "sw_codec_select.h"
#include "audio_datapath.h"
#include "audio_i2s.h"
#include "hw_codec.h"
#include "audio_usb.h"
#include "streamctrl.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_system, CONFIG_AUDIO_SYSTEM_LOG_LEVEL);

#define FIFO_TX_BLOCK_COUNT (CONFIG_FIFO_FRAME_SPLIT_NUM * CONFIG_FIFO_TX_FRAME_COUNT)
#define FIFO_RX_BLOCK_COUNT (CONFIG_FIFO_FRAME_SPLIT_NUM * CONFIG_FIFO_RX_FRAME_COUNT)

#define DEBUG_INTERVAL_NUM     1000
#define TEST_TONE_BASE_FREQ_HZ 1000

K_THREAD_STACK_DEFINE(encoder_thread_stack, CONFIG_ENCODER_STACK_SIZE);

K_MSGQ_DEFINE(audio_q_tx, sizeof(struct net_buf *), FIFO_TX_BLOCK_COUNT, sizeof(void *));
K_MSGQ_DEFINE(audio_q_rx, sizeof(struct net_buf *), FIFO_RX_BLOCK_COUNT, sizeof(void *));

NET_BUF_POOL_FIXED_DEFINE(audio_q_rx_pool, FIFO_RX_BLOCK_COUNT, FRAME_SIZE_BYTES,
			  sizeof(struct audio_metadata), NULL);
NET_BUF_POOL_FIXED_DEFINE(audio_q_tx_pool, FIFO_TX_BLOCK_COUNT, USB_BLOCK_SIZE_STEREO,
			  sizeof(struct audio_metadata), NULL);

static K_SEM_DEFINE(sem_encoder_start, 0, 1);

static struct k_thread encoder_thread_data;
static k_tid_t encoder_thread_id;

static struct k_poll_signal encoder_sig;

static struct k_poll_event encoder_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &encoder_sig);

static struct sw_codec_config sw_codec_cfg;
/* Buffer which can hold max 1 period test tone at 1000 Hz */
static int16_t test_tone_buf[CONFIG_AUDIO_SAMPLE_RATE_HZ / 1000];
static size_t test_tone_size;

static bool sample_rate_valid(uint32_t sample_rate_hz)
{
	if (sample_rate_hz == 16000 || sample_rate_hz == 24000 || sample_rate_hz == 48000) {
		return true;
	}

	return false;
}

static void audio_gateway_configure(void)
{
	if (IS_ENABLED(CONFIG_SW_CODEC_LC3)) {
		sw_codec_cfg.sw_codec = SW_CODEC_LC3;
	} else {
		ERR_CHK_MSG(-EINVAL, "No codec selected");
	}

#if (CONFIG_STREAM_BIDIRECTIONAL)
	sw_codec_cfg.decoder.audio_ch = DEVICE_LOCATION_DEFAULT;
	sw_codec_cfg.decoder.num_ch = 1;
	sw_codec_cfg.decoder.channel_mode = SW_CODEC_MONO;
#endif /* (CONFIG_STREAM_BIDIRECTIONAL) */

	if (IS_ENABLED(CONFIG_MONO_TO_ALL_RECEIVERS)) {
		sw_codec_cfg.encoder.num_ch = 1;
	} else {
		sw_codec_cfg.encoder.num_ch = 2;
	}

	sw_codec_cfg.encoder.channel_mode =
		(sw_codec_cfg.encoder.num_ch == 1) ? SW_CODEC_MONO : SW_CODEC_STEREO;
}

static void audio_headset_configure(void)
{
	if (IS_ENABLED(CONFIG_SW_CODEC_LC3)) {
		sw_codec_cfg.sw_codec = SW_CODEC_LC3;
	} else {
		ERR_CHK_MSG(-EINVAL, "No codec selected");
	}

#if (CONFIG_STREAM_BIDIRECTIONAL)
	sw_codec_cfg.encoder.audio_ch = 0;
	sw_codec_cfg.encoder.num_ch = 1;
	sw_codec_cfg.encoder.channel_mode = SW_CODEC_MONO;
#endif /* (CONFIG_STREAM_BIDIRECTIONAL) */

	enum bt_audio_location device_location;

	device_location_get(&device_location);

	sw_codec_cfg.decoder.num_ch = POPCOUNT(device_location);
	switch ((uint32_t)device_location) {
	case BT_AUDIO_LOCATION_FRONT_LEFT:
		sw_codec_cfg.decoder.audio_ch = 0;
		sw_codec_cfg.decoder.channel_mode = SW_CODEC_MONO;
		break;
	case BT_AUDIO_LOCATION_FRONT_RIGHT:
	    sw_codec_cfg.decoder.audio_ch = 1;
		sw_codec_cfg.decoder.channel_mode = SW_CODEC_MONO;
		break;
	case (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT):
		sw_codec_cfg.decoder.channel_mode = SW_CODEC_STEREO;
		break;
	default:
		LOG_ERR("Unsupported device location: 0x%08x", device_location);
		ERR_CHK(-EINVAL);
		break;
	};

	if (IS_ENABLED(CONFIG_SD_CARD_PLAYBACK)) {
		/* Need an extra decoder channel to decode data from SD card */
		sw_codec_cfg.decoder.num_ch++;
	}
}

static void encoder_thread(void *arg1, void *arg2, void *arg3)
{
	int ret;
	uint32_t audio_q_num_used;
	static uint32_t test_tone_finite_pos;
	int debug_trans_count = 0;

	while (1) {
		/* Don't start encoding until the stream needing it has started */
		ret = k_poll(&encoder_evt, 1, K_FOREVER);
		struct net_buf *audio_frame = net_buf_alloc(&audio_q_rx_pool, K_NO_WAIT);

		if (audio_frame == NULL) {
			LOG_ERR("Out of RX buffers");
			continue;
		}

		/* Get PCM data from I2S */
		/* Since one audio frame is divided into a number of
		 * blocks, we need to fetch the pointers to all of these
		 * blocks before copying it to a continuous area of memory
		 * before sending it to the encoder
		 */
		struct net_buf *audio_block = NULL;

		/* Wait for the first block to arrive */
		ret = k_msgq_get(&audio_q_rx, (void *)&audio_block, K_FOREVER);
		ERR_CHK_MSG(ret, "Failed to get audio block from RX queue");

		/* Copy metadata from the first block */
		ret = net_buf_user_data_copy(audio_frame, audio_block);
		ERR_CHK_MSG(ret, "Failed to copy user data");

		/* Extract the data from the block */
		net_buf_add_mem(audio_frame, audio_block->data, audio_block->len);

		/* Free the block */
		net_buf_unref(audio_block);
		struct audio_metadata *meta = net_buf_user_data(audio_frame);

		/* Loop until we have a full frame */
		while (meta->data_len_us < CONFIG_AUDIO_FRAME_DURATION_US) {
			ret = k_msgq_get(&audio_q_rx, (void *)&audio_block, K_FOREVER);
			ERR_CHK_MSG(ret, "Failed to get audio block from RX queue");

			struct audio_metadata *block_meta = net_buf_user_data(audio_block);

			/* Extract the data from the block */
			net_buf_add_mem(audio_frame, audio_block->data, audio_block->len);
			meta->data_len_us += block_meta->data_len_us;

			/* Free the block */
			net_buf_unref(audio_block);
		}

		if (sw_codec_cfg.encoder.enabled) {
			if (test_tone_size) {
				/* Test tone takes over audio stream */
				uint32_t num_bytes = 0;
				char tmp[FRAME_SIZE_BYTES / 2];

				ret = contin_array_create(tmp, FRAME_SIZE_BYTES / 2, test_tone_buf,
							  test_tone_size, &test_tone_finite_pos);
				ERR_CHK(ret);

				ret = pscm_copy_pad(tmp, FRAME_SIZE_BYTES / 2,
						    CONFIG_AUDIO_BIT_DEPTH_BITS, audio_frame->data,
						    &num_bytes);
				ERR_CHK(ret);

				if (audio_frame->len != num_bytes) {
					LOG_ERR("Tone wrong size: %u", num_bytes);
				}
			}

			ret = sw_codec_encode(audio_frame);
			ERR_CHK_MSG(ret, "Encode failed");
		}

		/* Print block usage */
		if (debug_trans_count == DEBUG_INTERVAL_NUM) {
			audio_q_num_used = k_msgq_num_used_get(&audio_q_rx);
			ERR_CHK(ret);
			LOG_DBG(COLOR_CYAN "RX filled: %d" COLOR_RESET, audio_q_num_used);
			debug_trans_count = 0;
		} else {
			debug_trans_count++;
		}

		if (sw_codec_cfg.encoder.enabled) {
			streamctrl_send(audio_frame);
			net_buf_unref(audio_frame);
		}
		STACK_USAGE_PRINT("encoder_thread", &encoder_thread_data);
	}
}

void audio_system_encoder_start(void)
{
	LOG_DBG("Encoder started");
	k_poll_signal_raise(&encoder_sig, 0);
}

void audio_system_encoder_stop(void)
{
	k_poll_signal_reset(&encoder_sig);
}

int audio_system_encode_test_tone_set(uint32_t freq)
{
	int ret;

	if (freq == 0) {
		test_tone_size = 0;
		return 0;
	}

	if (IS_ENABLED(CONFIG_AUDIO_TEST_TONE)) {
		ret = tone_gen(test_tone_buf, &test_tone_size, freq, CONFIG_AUDIO_SAMPLE_RATE_HZ,
			       1);
		ERR_CHK(ret);
	} else {
		LOG_ERR("Test tone is not enabled");
		return -ENXIO;
	}

	if (test_tone_size > sizeof(test_tone_buf)) {
		return -ENOMEM;
	}

	return 0;
}

int audio_system_encode_test_tone_step(void)
{
	int ret;
	static uint32_t test_tone_hz;

	if (CONFIG_AUDIO_BIT_DEPTH_BITS != 16) {
		LOG_WRN("Tone gen only supports 16 bits");
		return -ECANCELED;
	}

	if (test_tone_hz == 0) {
		test_tone_hz = TEST_TONE_BASE_FREQ_HZ;
	} else if (test_tone_hz >= TEST_TONE_BASE_FREQ_HZ * 4) {
		test_tone_hz = 0;
	} else {
		test_tone_hz = test_tone_hz * 2;
	}

	if (test_tone_hz != 0) {
		LOG_INF("Test tone set at %d Hz", test_tone_hz);
	} else {
		LOG_INF("Test tone off");
	}

	ret = audio_system_encode_test_tone_set(test_tone_hz);
	if (ret) {
		LOG_ERR("Failed to generate test tone");
		return ret;
	}

	return 0;
}

int audio_system_config_set(uint32_t encoder_sample_rate_hz, uint32_t encoder_bitrate,
			    uint32_t decoder_sample_rate_hz)
{
	if (sample_rate_valid(encoder_sample_rate_hz)) {
		sw_codec_cfg.encoder.sample_rate_hz = encoder_sample_rate_hz;
	} else if (encoder_sample_rate_hz) {
		LOG_ERR("%d is not a valid sample rate", encoder_sample_rate_hz);
		return -EINVAL;
	}

	if (sample_rate_valid(decoder_sample_rate_hz)) {
		sw_codec_cfg.decoder.enabled = true;
		sw_codec_cfg.decoder.sample_rate_hz = decoder_sample_rate_hz;
	} else if (decoder_sample_rate_hz) {
		LOG_ERR("%d is not a valid sample rate", decoder_sample_rate_hz);
		return -EINVAL;
	}

	if (encoder_bitrate) {
		sw_codec_cfg.encoder.enabled = true;
		sw_codec_cfg.encoder.bitrate = encoder_bitrate;
	}

	return 0;
}

/* This function is only used on gateway using USB as audio source and bidirectional stream */
int audio_system_decode(struct net_buf *audio_frame)
{
	int ret;
	static int debug_trans_count;
	static void *pcm_raw_data;
	size_t pcm_block_size;

	if (!sw_codec_cfg.initialized) {
		/* Throw away data */
		/* This can happen when using play/pause since there might be
		 * some packages left in the buffers
		 */
		LOG_DBG("Trying to decode while codec is not initialized");
		return -EPERM;
	}

	ret = sw_codec_decode(audio_frame, &pcm_raw_data, &pcm_block_size);
	if (ret) {
		LOG_ERR("Failed to decode");
		return ret;
	}

	/* If not enough space for a full frame, remove oldest samples to make room */
	while (k_msgq_num_free_get(&audio_q_tx) < CONFIG_FIFO_FRAME_SPLIT_NUM) {
		struct net_buf *stale_buf;

		ret = k_msgq_get(&audio_q_tx, (void *)&stale_buf, K_NO_WAIT);
		if (ret == -ENOMSG) {
			/* If there are no more blocks in FIFO, break */
			break;
		}

		net_buf_unref(stale_buf);
	}

	/* Split decoded frame into CONFIG_FIFO_FRAME_SPLIT_NUM blocks */
	for (int i = 0; i < CONFIG_FIFO_FRAME_SPLIT_NUM; i++) {
		struct net_buf *audio_block = net_buf_alloc(&audio_q_tx_pool, K_NO_WAIT);

		if (audio_block == NULL) {
			LOG_ERR("Out of TX buffers");
			return -ENOMEM;
		}

		net_buf_add_mem(audio_block, (char *)pcm_raw_data + (i * (BLOCK_SIZE_BYTES)),
				BLOCK_SIZE_BYTES);

		ret = k_msgq_put(&audio_q_tx, (void *)&audio_block, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Failed to put block onto queue");
			net_buf_unref(audio_block);
			return ret;
		}
	}
	if (debug_trans_count == DEBUG_INTERVAL_NUM) {
		uint32_t audio_q_num_used = k_msgq_num_used_get(&audio_q_tx);

		LOG_DBG(COLOR_MAGENTA "TX fill grade: %d/%d" COLOR_RESET, audio_q_num_used,
			CONFIG_FIFO_TX_FRAME_COUNT);
		debug_trans_count = 0;
	} else {
		debug_trans_count++;
	}

	return 0;
}

/**@brief Initializes the FIFOs, the codec, and starts the I2S
 */
void audio_system_start(void)
{
	int ret;

	if (CONFIG_AUDIO_DEV == HEADSET) {
		audio_headset_configure();
	} else if (CONFIG_AUDIO_DEV == GATEWAY) {
		audio_gateway_configure();
	} else {
		LOG_ERR("Invalid CONFIG_AUDIO_DEV: %d", CONFIG_AUDIO_DEV);
		ERR_CHK(-EINVAL);
	}

	ret = sw_codec_init(sw_codec_cfg);
	ERR_CHK_MSG(ret, "Failed to set up codec");

	sw_codec_cfg.initialized = true;

	if (sw_codec_cfg.encoder.enabled && encoder_thread_id == NULL) {
		encoder_thread_id = k_thread_create(
			&encoder_thread_data, encoder_thread_stack, CONFIG_ENCODER_STACK_SIZE,
			(k_thread_entry_t)encoder_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_ENCODER_THREAD_PRIO), 0, K_NO_WAIT);
		ret = k_thread_name_set(encoder_thread_id, "Encoder");
		ERR_CHK(ret);
	}

#if ((CONFIG_AUDIO_SOURCE_USB) && (CONFIG_AUDIO_DEV == GATEWAY))
	ret = audio_usb_start(&audio_q_tx, &audio_q_rx);
	ERR_CHK(ret);
#else
	if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)) {
		ret = hw_codec_default_conf_enable();
		ERR_CHK(ret);
	}

	ret = audio_datapath_start(&audio_q_rx);
	ERR_CHK(ret);
#endif /* ((CONFIG_AUDIO_SOURCE_USB) && (CONFIG_AUDIO_DEV == GATEWAY))) */
}

void audio_system_stop(void)
{
	int ret;

	if (!sw_codec_cfg.initialized) {
		LOG_WRN("Codec already unitialized");
		return;
	}

	LOG_DBG("Stopping codec");

#if ((CONFIG_AUDIO_DEV == GATEWAY) && CONFIG_AUDIO_SOURCE_USB)
	audio_usb_stop();
#else
	if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)) {
		ret = hw_codec_soft_reset();
		ERR_CHK(ret);
	}

	ret = audio_datapath_stop();
	ERR_CHK(ret);
#endif /* ((CONFIG_AUDIO_DEV == GATEWAY) && CONFIG_AUDIO_SOURCE_USB) */

	ret = sw_codec_uninit(sw_codec_cfg);
	ERR_CHK_MSG(ret, "Failed to uninit codec");
	sw_codec_cfg.initialized = false;
}

int audio_system_fifo_rx_block_drop(void)
{
	int ret;

	struct net_buf *audio_block = NULL;

	ret = k_msgq_get(&audio_q_rx, (void *)&audio_block, K_NO_WAIT);
	if (ret) {
		LOG_WRN("Failed to get last filled block");
		return -ECANCELED;
	}

	net_buf_unref(audio_block);

	LOG_DBG("Block dropped");
	return 0;
}

int audio_system_decoder_num_ch_get(void)
{
	return sw_codec_cfg.decoder.num_ch;
}

int audio_system_init(void)
{
	int ret;

#if ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_USB))
	ret = audio_usb_init();
	if (ret) {
		LOG_ERR("Failed to initialize USB: %d", ret);
		return ret;
	}
#else
	ret = audio_datapath_init();
	if (ret) {
		LOG_ERR("Failed to initialize audio datapath: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)) {
		ret = hw_codec_init();
		if (ret) {
			LOG_ERR("Failed to initialize HW codec: %d", ret);
			return ret;
		}
	}
#endif

	k_poll_signal_init(&encoder_sig);

	return 0;
}

static int cmd_audio_system_start(const struct shell *shell, size_t argc, const char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	audio_system_start();

	shell_print(shell, "Audio system started");

	return 0;
}

static int cmd_audio_system_stop(const struct shell *shell, size_t argc, const char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	audio_system_stop();

	shell_print(shell, "Audio system stopped");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(audio_system_cmd,
			       SHELL_COND_CMD(CONFIG_SHELL, start, NULL, "Start the audio system",
					      cmd_audio_system_start),
			       SHELL_COND_CMD(CONFIG_SHELL, stop, NULL, "Stop the audio system",
					      cmd_audio_system_stop),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(audio_system, &audio_system_cmd, "Audio system commands", NULL);
