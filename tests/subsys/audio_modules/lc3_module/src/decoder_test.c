/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/debug/thread_analyzer.h>
#include <errno.h>

#include "fakes.h"
#include "audio_module/audio_module.h"
#include "lc3_test_common.h"
#include "lc3_decoder.h"
#include "sweep21ms_16b48khz_mono_lc3_in.h"
#include "sweep21ms_16b48khz_mono_wav_ref.h"

#define TEST_DECODER_MSG_QUEUE_SIZE	  (2)
#define TEST_DATA_OBJECTS_NUM		  (2)
#define TEST_LC3_DECODER_STACK_SIZE	  (4192)
#define TEST_LC3_DECODER_MULTI_STACK_SIZE (4192)
#define TEST_LC3_DECODER_THREAD_PRIORITY  (4)
#define TEST_DECODER_MODULES_NUM	  (2)

enum test_module_id {
	TEST_MODULE_LC3_DECODER = 0,
	TEST_MODULE_LC3_ENCODER,
	TEST_MODULES_NUM
};

enum test_lc3_decoder_module_id {
	TEST_MODULE_ID_DECODER_1 = 0,
	TEST_MODULE_ID_DECODER_2,
	TEST_MODULE_ID_DECODER_NUM
};

struct audio_module_handle handle[TEST_DECODER_MODULES_NUM];
struct lc3_decoder_context decoder_ctx[TEST_DECODER_MODULES_NUM];

K_THREAD_STACK_DEFINE(lc3_dec1_thread_stack, TEST_LC3_DECODER_MULTI_STACK_SIZE);
DATA_FIFO_DEFINE(lc3_dec1_fifo_tx, TEST_DECODER_MSG_QUEUE_SIZE,
		 sizeof(struct audio_module_message));
DATA_FIFO_DEFINE(lc3_dec1_fifo_rx, TEST_DECODER_MSG_QUEUE_SIZE,
		 sizeof(struct audio_module_message));

K_THREAD_STACK_DEFINE(lc3_dec2_thread_stack, TEST_LC3_DECODER_STACK_SIZE);
DATA_FIFO_DEFINE(lc3_dec2_fifo_tx, TEST_DECODER_MSG_QUEUE_SIZE,
		 sizeof(struct audio_module_message));
DATA_FIFO_DEFINE(lc3_dec2_fifo_rx, TEST_DECODER_MSG_QUEUE_SIZE,
		 sizeof(struct audio_module_message));

static char audio_data_memory[TEST_DEC_MULTI_BUF_SIZE * TEST_DATA_OBJECTS_NUM];
struct k_mem_slab audio_data_slab;

struct audio_data audio_data = {.meta = {.data_coding = LC3,
					 .data_len_us = TEST_LC3_FRAME_SIZE_US,
					 .sample_rate_hz = TEST_PCM_SAMPLE_RATE,
					 .bits_per_sample = TEST_PCM_BIT_DEPTH,
					 .carried_bits_pr_sample = TEST_PCM_BIT_DEPTH,
					 .reference_ts_us = 0,
					 .data_rx_ts_us = 0,
					 .bad_data = false}};

static void lc3_initialize(int duration_us)
{
	int ret;
	uint8_t enc_sample_rates = 0;
	uint8_t dec_sample_rates = 0;
	uint8_t unique_session = 0;
	LC3FrameSize_t framesize;

	/* Set unique session to 0 for using the default sharing memory setting.
	 *
	 * This could lead to higher heap consumption, but is able to manipulate
	 * different sample rate setting between encoder/decoder.
	 */

	/* Check supported sample rates for encoder */
	if (IS_ENABLED(CONFIG_LC3_ENC_SAMPLE_RATE_8KHZ_SUPPORT)) {
		enc_sample_rates |= LC3_SAMPLE_RATE_8_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_ENC_SAMPLE_RATE_16KHZ_SUPPORT)) {
		enc_sample_rates |= LC3_SAMPLE_RATE_16_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_ENC_SAMPLE_RATE_24KHZ_SUPPORT)) {
		enc_sample_rates |= LC3_SAMPLE_RATE_24_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_ENC_SAMPLE_RATE_32KHZ_SUPPORT)) {
		enc_sample_rates |= LC3_SAMPLE_RATE_32_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_ENC_SAMPLE_RATE_441KHZ_SUPPORT)) {
		enc_sample_rates |= LC3_SAMPLE_RATE_441_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_ENC_SAMPLE_RATE_48KHZ_SUPPORT)) {
		enc_sample_rates |= LC3_SAMPLE_RATE_48_KHZ;
	}

	/* Check supported sample rates for decoder */
	if (IS_ENABLED(CONFIG_LC3_DEC_SAMPLE_RATE_8KHZ_SUPPORT)) {
		dec_sample_rates |= LC3_SAMPLE_RATE_8_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_DEC_SAMPLE_RATE_16KHZ_SUPPORT)) {
		dec_sample_rates |= LC3_SAMPLE_RATE_16_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_DEC_SAMPLE_RATE_24KHZ_SUPPORT)) {
		dec_sample_rates |= LC3_SAMPLE_RATE_24_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_DEC_SAMPLE_RATE_32KHZ_SUPPORT)) {
		dec_sample_rates |= LC3_SAMPLE_RATE_32_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_DEC_SAMPLE_RATE_441KHZ_SUPPORT)) {
		dec_sample_rates |= LC3_SAMPLE_RATE_441_KHZ;
	}
	if (IS_ENABLED(CONFIG_LC3_DEC_SAMPLE_RATE_48KHZ_SUPPORT)) {
		dec_sample_rates |= LC3_SAMPLE_RATE_48_KHZ;
	}

	switch (duration_us) {
	case 7500:
		framesize = LC3FrameSize7_5Ms;
		break;
	case 10000:
		framesize = LC3FrameSize10Ms;
		break;
	default:
		return;
	}

	ret = LC3Initialize(enc_sample_rates, dec_sample_rates, framesize, unique_session, NULL,
			    NULL);
	zassert_equal(ret, 0, "LC3 initialize did not return zero");
}

static void lc3_deinitialize(void)
{
	int ret;

	ret = LC3Deinitialize();
	zassert_equal(ret, 0, "LC3 codec failed initialization");
}

static void test_lc3_decoder_mono_init(void)
{
	int ret;
	struct audio_module_parameters test_decoder_param;
	struct lc3_decoder_configuration test_decoder_config;

	for (int i = 0; i < TEST_DECODER_MODULES_NUM; i++) {
		memset(&handle[i], 0, sizeof(struct audio_module_handle));
	}

	test_decoder_config.sample_rate_hz = TEST_PCM_SAMPLE_RATE;
	test_decoder_config.bits_per_sample = TEST_SAMPLE_BIT_DEPTH;
	test_decoder_config.carried_bits_pr_sample = TEST_PCM_BIT_DEPTH;
	test_decoder_config.data_len_us = TEST_LC3_FRAME_SIZE_US;
	test_decoder_config.interleaved = false;
	test_decoder_config.locations = TEST_AUDIO_MONO_LEFT_LOCATIONS;
	test_decoder_config.bitrate_bps = TEST_LC3_BITRATE;

	/* Need a data_fifo_deinit() */
	if (lc3_dec1_fifo_rx.initialized) {
		lc3_dec1_fifo_rx.initialized = false;
	}

	if (lc3_dec1_fifo_tx.initialized) {
		lc3_dec1_fifo_tx.initialized = false;
	}

	data_fifo_init(&lc3_dec1_fifo_rx);
	data_fifo_init(&lc3_dec1_fifo_tx);

	ret = k_mem_slab_init(&audio_data_slab, &audio_data_memory[0], TEST_DEC_MONO_BUF_SIZE,
			      TEST_DATA_OBJECTS_NUM);
	zassert_equal(ret, 0, "Failed to allocate the data slab: ret = %d", ret);

	test_decoder_param.description = lc3_decoder_description;
	test_decoder_param.thread.stack = lc3_dec1_thread_stack,
	test_decoder_param.thread.stack_size = TEST_LC3_DECODER_STACK_SIZE;
	test_decoder_param.thread.priority = TEST_LC3_DECODER_THREAD_PRIORITY;
	test_decoder_param.thread.data_slab = &audio_data_slab;
	test_decoder_param.thread.data_size = TEST_DEC_MONO_BUF_SIZE;
	test_decoder_param.thread.msg_rx = &lc3_dec1_fifo_rx;
	test_decoder_param.thread.msg_tx = &lc3_dec1_fifo_tx;

	if (IS_ENABLED(TEST_THREAD_ANALYZER)) {
		thread_analyzer_print();
	}

	ret = audio_module_open(
		&test_decoder_param, (struct audio_module_configuration *)&test_decoder_config,
		"Decoder Left",
		(struct audio_module_context *)&decoder_ctx[TEST_MODULE_ID_DECODER_1],
		&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Decoder module open did not return zero");
	zassert_equal(LC3DecodeSessionClose_fake.call_count, 0,
		      "Called LC3 close decoder session %d times",
		      LC3DecodeSessionClose_fake.call_count);
	zassert_equal(LC3BitstreamBuffersize_fake.call_count, 1,
		      "Failed to call LC3 get buffer size %d times",
		      LC3BitstreamBuffersize_fake.call_count);
	zassert_equal(LC3DecodeSessionOpen_fake.call_count, 1,
		      "Failed to call LC3 open decoder session %d times",
		      LC3DecodeSessionOpen_fake.call_count);

	if (IS_ENABLED(TEST_THREAD_ANALYZER)) {
		thread_analyzer_print();
	}

	ret = audio_module_connect(&handle[TEST_MODULE_ID_DECODER_1], NULL, true);
	zassert_equal(ret, 0, "Decoder connect did not return zero");

	ret = audio_module_start(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Decoder left module start did not return zero");
}

static void test_lc3_decoder_dual_mono_init(void)
{
	int ret;
	struct audio_module_parameters test_decoder_param;
	struct lc3_decoder_configuration test_decoder_config;

	for (int i = 0; i < TEST_DECODER_MODULES_NUM; i++) {
		memset(&handle[i], 0, sizeof(struct audio_module_handle));
	}

	test_decoder_config.sample_rate_hz = TEST_PCM_SAMPLE_RATE;
	test_decoder_config.bits_per_sample = TEST_SAMPLE_BIT_DEPTH;
	test_decoder_config.carried_bits_pr_sample = TEST_PCM_BIT_DEPTH;
	test_decoder_config.data_len_us = TEST_LC3_FRAME_SIZE_US;
	test_decoder_config.interleaved = false;
	test_decoder_config.locations = TEST_AUDIO_MONO_LEFT_LOCATIONS;
	test_decoder_config.bitrate_bps = TEST_LC3_BITRATE;

	/* Need a data_fifo_deinit() */
	if (lc3_dec1_fifo_rx.initialized) {
		lc3_dec1_fifo_rx.initialized = false;
	}

	if (lc3_dec1_fifo_tx.initialized) {
		lc3_dec1_fifo_tx.initialized = false;
	}

	data_fifo_init(&lc3_dec1_fifo_rx);
	data_fifo_init(&lc3_dec1_fifo_tx);

	ret = k_mem_slab_init(&audio_data_slab, (void *)&audio_data_memory[0], 1024,
			      TEST_DATA_OBJECTS_NUM);
	zassert_equal(ret, 0, "Failed to allocate the data slab: ret = %d", ret);

	test_decoder_param.description = lc3_decoder_description;
	test_decoder_param.thread.stack_size = TEST_LC3_DECODER_STACK_SIZE;
	test_decoder_param.thread.priority = TEST_LC3_DECODER_THREAD_PRIORITY;
	test_decoder_param.thread.data_slab = &audio_data_slab;
	test_decoder_param.thread.data_size = TEST_DEC_MONO_BUF_SIZE;
	test_decoder_param.thread.stack = lc3_dec1_thread_stack,
	test_decoder_param.thread.msg_rx = &lc3_dec1_fifo_rx;
	test_decoder_param.thread.msg_tx = &lc3_dec1_fifo_tx;

	ret = audio_module_open(
		&test_decoder_param, (struct audio_module_configuration *)&test_decoder_config,
		"Decoder Left",
		(struct audio_module_context *)&decoder_ctx[TEST_MODULE_ID_DECODER_1],
		&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Decoder left module open did not return zero");
	zassert_equal(LC3DecodeSessionClose_fake.call_count, 0,
		      "Called LC3 close decoder session %d times",
		      LC3DecodeSessionClose_fake.call_count);
	zassert_equal(LC3BitstreamBuffersize_fake.call_count, 1,
		      "Failed to call LC3 get buffer size %d times",
		      LC3BitstreamBuffersize_fake.call_count);
	zassert_equal(LC3DecodeSessionOpen_fake.call_count, 1,
		      "Failed to call LC3 open decoder session %d times",
		      LC3DecodeSessionOpen_fake.call_count);

	test_decoder_config.locations = TEST_AUDIO_MONO_RIGHT_LOCATIONS;

	/* Need a data_fifo_deinit() */
	data_fifo_init(&lc3_dec2_fifo_rx);
	data_fifo_init(&lc3_dec2_fifo_tx);

	test_decoder_param.description = lc3_decoder_description;
	test_decoder_param.thread.stack_size = TEST_LC3_DECODER_STACK_SIZE;
	test_decoder_param.thread.priority = TEST_LC3_DECODER_THREAD_PRIORITY;
	test_decoder_param.thread.data_slab = &audio_data_slab;
	test_decoder_param.thread.data_size = TEST_DEC_MONO_BUF_SIZE;
	test_decoder_param.thread.stack = lc3_dec2_thread_stack,
	test_decoder_param.thread.msg_rx = &lc3_dec2_fifo_rx;
	test_decoder_param.thread.msg_tx = &lc3_dec2_fifo_tx;

	ret = audio_module_open(
		&test_decoder_param, (struct audio_module_configuration *)&test_decoder_config,
		"Decoder Right",
		(struct audio_module_context *)&decoder_ctx[TEST_MODULE_ID_DECODER_2],
		&handle[TEST_MODULE_ID_DECODER_2]);
	zassert_equal(ret, 0, "Decoder right module open did not return zero");
	zassert_equal(LC3DecodeSessionClose_fake.call_count, 0,
		      "Called LC3 close decoder session %d times",
		      LC3DecodeSessionClose_fake.call_count);
	zassert_equal(LC3BitstreamBuffersize_fake.call_count, 2,
		      "Failed to call LC3 get buffer size %d times",
		      LC3BitstreamBuffersize_fake.call_count);
	zassert_equal(LC3DecodeSessionOpen_fake.call_count, 2,
		      "Failed to call LC3 open decoder session %d times",
		      LC3DecodeSessionOpen_fake.call_count);

	ret = audio_module_connect(&handle[TEST_MODULE_ID_DECODER_1], NULL, true);
	zassert_equal(ret, 0, "Decoder connect did not return zero");

	ret = audio_module_connect(&handle[TEST_MODULE_ID_DECODER_2], NULL, true);
	zassert_equal(ret, 0, "Decoder connect did not return zero");

	ret = audio_module_start(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Left decoder module start did not return zero");

	ret = audio_module_start(&handle[TEST_MODULE_ID_DECODER_2]);
	zassert_equal(ret, 0, "Right decoder module start did not return zero");
}

static void test_lc3_decoder_multi_init(bool interleaved, uint32_t locations)
{
	int ret;
	struct audio_module_parameters test_decoder_param;
	struct lc3_decoder_configuration test_decoder_config;
	uint8_t number_channels;

	for (int i = 0; i < TEST_DECODER_MODULES_NUM; i++) {
		memset(&handle[i], 0, sizeof(struct audio_module_handle));
	}

	test_decoder_config.sample_rate_hz = TEST_PCM_SAMPLE_RATE;
	test_decoder_config.bits_per_sample = TEST_SAMPLE_BIT_DEPTH;
	test_decoder_config.carried_bits_pr_sample = TEST_PCM_BIT_DEPTH;
	test_decoder_config.data_len_us = TEST_LC3_FRAME_SIZE_US;
	test_decoder_config.interleaved = true;
	test_decoder_config.locations = locations;
	test_decoder_config.bitrate_bps = TEST_LC3_BITRATE;

	audio_module_number_channels_calculate(locations, &number_channels);

	/* Need a data_fifo_deinit() */
	if (lc3_dec1_fifo_rx.initialized) {
		lc3_dec1_fifo_rx.initialized = false;
	}

	if (lc3_dec1_fifo_tx.initialized) {
		lc3_dec1_fifo_tx.initialized = false;
	}

	data_fifo_init(&lc3_dec1_fifo_rx);
	data_fifo_init(&lc3_dec1_fifo_tx);

	ret = k_mem_slab_init(&audio_data_slab, &audio_data_memory[0],
			      TEST_DEC_MONO_BUF_SIZE * number_channels, TEST_DATA_OBJECTS_NUM);
	zassert_equal(ret, 0, "Failed to allocate the data slab: ret = %d", ret);

	test_decoder_param.description = lc3_decoder_description;
	test_decoder_param.thread.stack_size = TEST_LC3_DECODER_STACK_SIZE;
	test_decoder_param.thread.priority = TEST_LC3_DECODER_THREAD_PRIORITY;
	test_decoder_param.thread.data_slab = &audio_data_slab;
	test_decoder_param.thread.data_size = TEST_DEC_MONO_BUF_SIZE * number_channels;
	test_decoder_param.thread.stack = lc3_dec1_thread_stack,
	test_decoder_param.thread.msg_rx = &lc3_dec1_fifo_rx;
	test_decoder_param.thread.msg_tx = &lc3_dec1_fifo_tx;

	ret = audio_module_open(
		&test_decoder_param, (struct audio_module_configuration *)&test_decoder_config,
		"Decoder Multi-channel",
		(struct audio_module_context *)&decoder_ctx[TEST_MODULE_ID_DECODER_1],
		&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Decoder left module open did not return zero");
	zassert_equal(LC3Initialize_fake.call_count, 1, "Failed to call LC3 initialize %d times",
		      LC3Initialize_fake.call_count);
	zassert_equal(LC3DecodeSessionClose_fake.call_count, 0,
		      "Called LC3 close decoder session %d times",
		      LC3DecodeSessionClose_fake.call_count);
	zassert_equal(LC3BitstreamBuffersize_fake.call_count, 1,
		      "Failed to call LC3 get buffer size %d times",
		      LC3BitstreamBuffersize_fake.call_count);
	zassert_equal(LC3DecodeSessionOpen_fake.call_count, number_channels,
		      "Failed to call LC3 open decoder session %d times",
		      LC3DecodeSessionOpen_fake.call_count);

	ret = audio_module_connect(&handle[TEST_MODULE_ID_DECODER_1], NULL, true);
	zassert_equal(ret, 0, "Decoder connect did not return zero");

	ret = audio_module_start(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Decoder left module start did not return zero");
}

ZTEST(suite_lc3_decoder_functional, test_lc3_decode_mono)
{
	int ret;
	int dec_call_count = 0;
	uint8_t *data_in = &lc3_mono_in[0];
	uint8_t *data_ref = (uint8_t *)&wav_mono_ref[0];
	uint8_t pcm_out[TEST_DEC_MONO_BUF_SIZE];
	struct audio_data audio_data_tx, audio_data_rx;

	/* Fake internal empty data FIFO success */
	wav_ref = (uint8_t *)&wav_mono_ref[0];
	wav_ref_size = sizeof(wav_mono_ref);
	wav_ref_read_size = TEST_DEC_MONO_BUF_SIZE;
	LC3Initialize_fake.custom_fake = fake_LC3Initialize__succeeds;
	LC3DecodeSessionClose_fake.custom_fake = fake_LC3DecodeSessionClose__succeeds;
	LC3BitstreamBuffersize_fake.custom_fake = fake_LC3BitstreamBuffersize__succeeds;
	LC3DecodeSessionOpen_fake.custom_fake = fake_LC3DecodeSessionOpen__succeeds;
	LC3DecodeSessionData_fake.custom_fake = fake_LC3DecodeSessionData__succeeds;
	LC3Deinitialize_fake.custom_fake = fake_LC3Deinitialize__succeeds;

	lc3_initialize(TEST_LC3_FRAME_SIZE_US);
	zassert_equal(LC3Initialize_fake.call_count, 1, "Failed, called LC3 initialize %d times",
		      LC3Initialize_fake.call_count);

	test_lc3_decoder_mono_init();

	do {
		memcpy(&audio_data_tx, &audio_data, sizeof(struct audio_data));
		memcpy(&audio_data_rx, &audio_data, sizeof(struct audio_data));

		audio_data_tx.data = (void *)data_in;
		audio_data_tx.data_size = TEST_ENC_MONO_BUF_SIZE;
		audio_data_tx.meta.data_coding = LC3;
		audio_data_tx.meta.locations = TEST_AUDIO_MONO_LEFT_LOCATIONS;

		audio_data_rx.data = (void *)&pcm_out[0];
		audio_data_rx.data_size = TEST_DEC_MONO_BUF_SIZE;
		audio_data_rx.meta.data_coding = PCM;

		ret = audio_module_data_tx_rx(&handle[TEST_MODULE_ID_DECODER_1],
					      &handle[TEST_MODULE_ID_DECODER_1], &audio_data_tx,
					      &audio_data_rx, K_FOREVER);
		zassert_equal(ret, 0, "Data TX-RX did not return zero for the decoder: %d", ret);
		zassert_equal(audio_data_rx.data_size, TEST_DEC_MONO_BUF_SIZE,
			      "Decoded number of bytes incorrect: %d", audio_data_rx.data_size);
		zassert_mem_equal(&pcm_out[0], data_ref, TEST_DEC_MONO_BUF_SIZE,
				  "Decoded PCM data does not match reference PCM data");
		zassert_equal(LC3DecodeSessionData_fake.call_count, dec_call_count + 1,
			      "Failed to call LC3 decode %d times",
			      LC3DecodeSessionData_fake.call_count);

		data_in += TEST_ENC_MONO_BUF_SIZE;
		data_ref += TEST_DEC_MONO_BUF_SIZE;

		dec_call_count += 1;

	} while ((dec_call_count < (LC3_MONO_IN_SIZE / TEST_ENC_MONO_BUF_SIZE) &&
		  dec_call_count < (WAV_MONO_REF_SIZE / TEST_DEC_MONO_BUF_SAMPLES)));

	ret = audio_module_stop(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to stop module did not return zero: ret = %d", ret);

	ret = audio_module_close(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to close module did not return zero: ret = %d", ret);

	lc3_deinitialize();
	zassert_equal(LC3Deinitialize_fake.call_count, 1,
		      "Failed, called LC3 deinitialize %d times", LC3Deinitialize_fake.call_count);
}

ZTEST(suite_lc3_decoder_functional, test_lc3_decode_dual_mono)
{
	int ret;
	int dec_call_count = 0;
	uint8_t *data_l_in = (uint8_t *)&lc3_mono_in[0];
	uint8_t *data_r_in = (uint8_t *)&lc3_mono_in[0];
	uint8_t *data_l_ref = (uint8_t *)&wav_mono_ref[0];
	uint8_t *data_r_ref = (uint8_t *)&wav_mono_ref[0];
	uint8_t data_l[TEST_DEC_MONO_BUF_SIZE], data_r[TEST_DEC_MONO_BUF_SIZE];
	struct audio_data audio_data_l_tx, audio_data_l_rx;
	struct audio_data audio_data_r_tx, audio_data_r_rx;

	/* Fake internal empty data FIFO success */
	wav_ref = (uint8_t *)&wav_mono_ref[0];
	wav_ref_size = sizeof(wav_mono_ref);
	wav_ref_read_size = TEST_DEC_MONO_BUF_SIZE;
	LC3Initialize_fake.custom_fake = fake_LC3Initialize__succeeds;
	LC3DecodeSessionClose_fake.custom_fake = fake_LC3DecodeSessionClose__succeeds;
	LC3BitstreamBuffersize_fake.custom_fake = fake_LC3BitstreamBuffersize__succeeds;
	LC3DecodeSessionOpen_fake.custom_fake = fake_LC3DecodeSessionOpen__succeeds;
	LC3DecodeSessionData_fake.custom_fake = fake_LC3DecodeSessionData__succeeds;
	LC3Deinitialize_fake.custom_fake = fake_LC3Deinitialize__succeeds;

	lc3_initialize(TEST_LC3_FRAME_SIZE_US);
	zassert_equal(LC3Initialize_fake.call_count, 1, "Failed, called LC3 initialize %d times",
		      LC3Initialize_fake.call_count);

	test_lc3_decoder_dual_mono_init();

	do {
		memcpy(&audio_data_l_tx, &audio_data, sizeof(struct audio_data));
		memcpy(&audio_data_l_rx, &audio_data, sizeof(struct audio_data));

		audio_data_l_tx.data = (void *)data_l_in;
		audio_data_l_tx.data_size = TEST_ENC_MONO_BUF_SIZE;
		audio_data_l_tx.meta.data_coding = LC3;
		audio_data_l_tx.meta.locations = TEST_AUDIO_MONO_LEFT_LOCATIONS;

		memset(&data_l[0], 0, TEST_DEC_MONO_BUF_SIZE);
		audio_data_l_rx.data = (void *)&data_l[0];
		audio_data_l_rx.data_size = TEST_DEC_MONO_BUF_SIZE;

		ret = audio_module_data_tx_rx(&handle[TEST_MODULE_ID_DECODER_1],
					      &handle[TEST_MODULE_ID_DECODER_1], &audio_data_l_tx,
					      &audio_data_l_rx, K_FOREVER);
		zassert_equal(ret, 0, "Data TX-RX did not return zero for left decoder: %d", ret);
		zassert_equal(audio_data_l_rx.data_size, TEST_DEC_MONO_BUF_SIZE,
			      "Decoded number of bytes for right incorrect: %d",
			      audio_data_l_rx.data_size);
		zassert_mem_equal(&data_l[0], data_l_ref, TEST_DEC_MONO_BUF_SIZE,
				  "Decoded left PCM data does not match reference PCM data");
		zassert_equal(LC3DecodeSessionData_fake.call_count, dec_call_count + 1,
			      "Failed to call LC3 decode %d times",
			      LC3DecodeSessionData_fake.call_count);

		data_l_in += TEST_ENC_MONO_BUF_SIZE;
		data_l_ref += TEST_DEC_MONO_BUF_SIZE;

		dec_call_count += 1;

		memcpy(&audio_data_r_tx, &audio_data, sizeof(struct audio_data));
		memcpy(&audio_data_r_rx, &audio_data, sizeof(struct audio_data));

		audio_data_r_tx.data = (void *)data_r_in;
		audio_data_r_tx.data_size = TEST_ENC_MONO_BUF_SIZE;
		audio_data_r_tx.meta.data_coding = LC3;
		audio_data_r_tx.meta.locations = TEST_AUDIO_MONO_RIGHT_LOCATIONS;

		memset(&data_r[0], 0, TEST_DEC_MONO_BUF_SIZE);
		audio_data_r_rx.data = (void *)&data_r[0];
		audio_data_r_rx.data_size = TEST_DEC_MONO_BUF_SIZE;

		ret = audio_module_data_tx_rx(&handle[TEST_MODULE_ID_DECODER_2],
					      &handle[TEST_MODULE_ID_DECODER_2], &audio_data_r_tx,
					      &audio_data_r_rx, K_FOREVER);
		zassert_equal(ret, 0, "Data TX-RX did not return zero for right decoder: %d", ret);
		zassert_equal(audio_data_r_rx.data_size, TEST_DEC_MONO_BUF_SIZE,
			      "Decoded number of bytes for left incorrect: %d",
			      audio_data_r_rx.data_size);
		zassert_mem_equal(&data_r[0], data_r_ref, TEST_DEC_MONO_BUF_SIZE,
				  "Decoded right PCM data does not match reference PCM data");
		zassert_equal(LC3DecodeSessionData_fake.call_count, dec_call_count + 1,
			      "Failed to call LC3 decode %d times",
			      LC3DecodeSessionData_fake.call_count);

		data_r_in += TEST_ENC_MONO_BUF_SIZE;
		data_r_ref += TEST_DEC_MONO_BUF_SIZE;

		dec_call_count += 1;

	} while (((dec_call_count >> 1) < (LC3_MONO_IN_SIZE / TEST_ENC_MONO_BUF_SIZE) &&
		  (dec_call_count >> 1) < (WAV_MONO_REF_SIZE / TEST_DEC_MONO_BUF_SAMPLES)));

	ret = audio_module_stop(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to stop module did not return zero: ret = %d", ret);
	ret = audio_module_stop(&handle[TEST_MODULE_ID_DECODER_2]);
	zassert_equal(ret, 0, "Failed to stop module did not return zero: ret = %d", ret);

	ret = audio_module_close(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to close module did not return zero: ret = %d", ret);
	ret = audio_module_close(&handle[TEST_MODULE_ID_DECODER_2]);
	zassert_equal(ret, 0, "Failed to close module did not return zero: ret = %d", ret);

	lc3_deinitialize();
	zassert_equal(LC3Deinitialize_fake.call_count, 1,
		      "Failed, called LC3 deinitialize %d times", LC3Deinitialize_fake.call_count);
}

ZTEST(suite_lc3_decoder_functional, test_lc3_decode_stereo_deint)
{
	int ret;
	int dec_call_count = 0;
	uint8_t *data_source = (uint8_t *)&lc3_mono_in[0];
	uint8_t *data_reference = (uint8_t *)&wav_mono_ref[0];
	uint8_t data_in[TEST_ENC_STEREO_BUF_SIZE];
	uint8_t data_out[TEST_DEC_STEREO_BUF_SIZE];
	uint8_t data_ref[TEST_DEC_STEREO_BUF_SIZE];
	struct audio_data audio_data_tx, audio_data_rx;

	/* Fake internal empty data FIFO success */
	wav_ref = (uint8_t *)&wav_mono_ref[0];
	wav_ref_size = sizeof(wav_mono_ref);
	wav_ref_read_size = TEST_DEC_MONO_BUF_SIZE;
	LC3Initialize_fake.custom_fake = fake_LC3Initialize__succeeds;
	LC3DecodeSessionClose_fake.custom_fake = fake_LC3DecodeSessionClose__succeeds;
	LC3BitstreamBuffersize_fake.custom_fake = fake_LC3BitstreamBuffersize__succeeds;
	LC3DecodeSessionOpen_fake.custom_fake = fake_LC3DecodeSessionOpen__succeeds;
	LC3DecodeSessionData_fake.custom_fake = fake_LC3DecodeSessionData__succeeds;
	LC3Deinitialize_fake.custom_fake = fake_LC3Deinitialize__succeeds;

	lc3_initialize(TEST_LC3_FRAME_SIZE_US);
	zassert_equal(LC3Initialize_fake.call_count, 1, "Failed to call LC3 initialize %d times",
		      LC3Initialize_fake.call_count);

	test_lc3_decoder_multi_init(false, TEST_AUDIO_STEREO_LOCATIONS);

	do {
		memcpy(&data_in[0], data_source, TEST_ENC_MONO_BUF_SIZE);
		memcpy(&data_in[TEST_ENC_MONO_BUF_SIZE], data_source, TEST_ENC_MONO_BUF_SIZE);
		memcpy(&data_ref[0], data_reference, TEST_DEC_MONO_BUF_SIZE);
		memcpy(&data_ref[TEST_DEC_MONO_BUF_SIZE], data_reference, TEST_DEC_MONO_BUF_SIZE);

		memcpy(&audio_data_tx, &audio_data, sizeof(struct audio_data));
		memcpy(&audio_data_rx, &audio_data, sizeof(struct audio_data));

		audio_data_tx.data = &data_in[0];
		audio_data_tx.data_size = TEST_ENC_STEREO_BUF_SIZE;
		audio_data_tx.meta.data_coding = LC3;
		audio_data_tx.meta.locations = TEST_AUDIO_STEREO_LOCATIONS;

		audio_data_rx.data = (void *)&data_out[0];
		audio_data_rx.data_size = TEST_DEC_STEREO_BUF_SIZE;

		ret = audio_module_data_tx_rx(&handle[TEST_MODULE_ID_DECODER_1],
					      &handle[TEST_MODULE_ID_DECODER_1], &audio_data_tx,
					      &audio_data_rx, K_FOREVER);
		zassert_equal(ret, 0, "Data TX-RX did not return zero for left decoder: %d", ret);
		zassert_equal(audio_data_rx.data_size, TEST_DEC_STEREO_BUF_SIZE,
			      "Decoded number of bytes incorrect: %d", audio_data_rx.data_size);
		zassert_mem_equal(&data_out[0], &data_ref[0], TEST_DEC_STEREO_BUF_SIZE,
				  "Decoded PCM data does not match reference PCM data");
		zassert_equal(LC3DecodeSessionData_fake.call_count, dec_call_count + 2,
			      "Failed to call LC3 decode %d times",
			      LC3DecodeSessionData_fake.call_count);

		data_source += TEST_ENC_MONO_BUF_SIZE;
		data_reference += TEST_DEC_MONO_BUF_SIZE;

		dec_call_count += 2;

	} while (((dec_call_count >> 1) < (LC3_MONO_IN_SIZE / TEST_ENC_MONO_BUF_SIZE) &&
		  (dec_call_count >> 1) < (WAV_MONO_REF_SIZE / TEST_DEC_MONO_BUF_SAMPLES)));

	ret = audio_module_stop(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to stop module did not return zero: ret = %d", ret);

	ret = audio_module_close(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to close module did not return zero: ret = %d", ret);

	lc3_deinitialize();
	zassert_equal(LC3Deinitialize_fake.call_count, 1,
		      "Failed to call LC3 deinitialize %d times", LC3Deinitialize_fake.call_count);
}

ZTEST(suite_lc3_decoder_functional, test_lc3_decode_stereo_int)
{
	int ret;
	int dec_call_count = 0;
	uint8_t data_in[TEST_ENC_STEREO_BUF_SIZE];
	uint8_t data_out[TEST_DEC_STEREO_BUF_SIZE];
	uint8_t data_ref[TEST_DEC_STEREO_BUF_SIZE];
	struct audio_data audio_data_tx, audio_data_rx;

	/* Fake internal empty data FIFO success */
	wav_ref = (uint8_t *)&wav_mono_ref[0];
	wav_ref_size = sizeof(wav_mono_ref);
	wav_ref_read_size = TEST_DEC_MONO_BUF_SIZE;
	LC3Initialize_fake.custom_fake = fake_LC3Initialize__succeeds;
	LC3DecodeSessionClose_fake.custom_fake = fake_LC3DecodeSessionClose__succeeds;
	LC3BitstreamBuffersize_fake.custom_fake = fake_LC3BitstreamBuffersize__succeeds;
	LC3DecodeSessionOpen_fake.custom_fake = fake_LC3DecodeSessionOpen__succeeds;
	LC3DecodeSessionData_fake.custom_fake = fake_LC3DecodeSessionData__succeeds;
	LC3Deinitialize_fake.custom_fake = fake_LC3Deinitialize__succeeds;

	lc3_initialize(TEST_LC3_FRAME_SIZE_US);
	zassert_equal(LC3Initialize_fake.call_count, 1, "Failed to call LC3 initialize %d times",
		      LC3Initialize_fake.call_count);

	test_lc3_decoder_multi_init(true, TEST_AUDIO_STEREO_LOCATIONS);

	do {
		uint8_t *ref = &data_ref[0];
		uint8_t *data_source =
			(uint8_t *)&lc3_mono_in[TEST_ENC_MONO_BUF_SIZE * (dec_call_count >> 1)];
		uint8_t *data_reference =
			(uint8_t *)&wav_mono_ref[TEST_DEC_MONO_BUF_SAMPLES * (dec_call_count >> 1)];

		memcpy(&data_in[0], data_source, TEST_ENC_MONO_BUF_SIZE);
		memcpy(&data_in[TEST_ENC_MONO_BUF_SIZE], data_source, TEST_ENC_MONO_BUF_SIZE);

		/* Interleave the channel samples */
		for (uint32_t i = 0; i < TEST_DEC_MONO_BUF_SAMPLES; i++) {
			for (uint8_t j = 0; j < TEST_AUDIO_CHANNELS_STEREO; j++) {
				for (uint8_t k = 0; k < (TEST_SAMPLE_BIT_DEPTH / 8); k++) {
					*ref++ = *data_reference++;
				}

				data_reference -= (TEST_SAMPLE_BIT_DEPTH / 8);
			}

			data_reference += (TEST_SAMPLE_BIT_DEPTH / 8);
		}

		memcpy(&audio_data_tx, &audio_data, sizeof(struct audio_data));
		memcpy(&audio_data_rx, &audio_data, sizeof(struct audio_data));

		audio_data_tx.data = &data_in[0];
		audio_data_tx.data_size = TEST_ENC_STEREO_BUF_SIZE;
		audio_data_tx.meta.data_coding = LC3;
		audio_data_tx.meta.locations = TEST_AUDIO_STEREO_LOCATIONS;

		audio_data_rx.data = (void *)&data_out[0];
		audio_data_rx.data_size = TEST_DEC_STEREO_BUF_SIZE;

		ret = audio_module_data_tx_rx(&handle[TEST_MODULE_ID_DECODER_1],
					      &handle[TEST_MODULE_ID_DECODER_1], &audio_data_tx,
					      &audio_data_rx, K_FOREVER);
		zassert_equal(ret, 0, "Data TX-RX did not return zero for left decoder: %d", ret);
		zassert_equal(audio_data_rx.data_size, TEST_DEC_STEREO_BUF_SIZE,
			      "Decoded number of bytes incorrect: %d", audio_data_rx.data_size);
		zassert_mem_equal(&data_out[0], &data_ref[0], TEST_DEC_STEREO_BUF_SIZE,
				  "Decoded PCM data does not match reference PCM data");
		zassert_equal(LC3DecodeSessionData_fake.call_count, dec_call_count + 2,
			      "Failed to call LC3 decode %d times",
			      LC3DecodeSessionData_fake.call_count);

		dec_call_count += 2;

	} while (((dec_call_count >> 1) < (LC3_MONO_IN_SIZE / TEST_ENC_MONO_BUF_SIZE) &&
		  (dec_call_count >> 1) < (WAV_MONO_REF_SIZE / TEST_DEC_MONO_BUF_SAMPLES)));

	ret = audio_module_stop(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to stop module did not return zero: ret = %d", ret);

	ret = audio_module_close(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to close module did not return zero: ret = %d", ret);

	lc3_deinitialize();
	zassert_equal(LC3Deinitialize_fake.call_count, 1,
		      "Failed to call LC3 deinitialize %d times", LC3Deinitialize_fake.call_count);
}

ZTEST(suite_lc3_decoder_functional, test_zlc3_decode_multi_int)
{
	int ret;
	int dec_call_count = 0;
	uint8_t data_in[TEST_ENC_MULTI_BUF_SIZE];
	uint8_t data_out[TEST_DEC_MULTI_BUF_SIZE];
	uint8_t data_ref[TEST_DEC_MULTI_BUF_SIZE];
	struct audio_data audio_data_tx, audio_data_rx;

	/* Fake internal empty data FIFO success */
	wav_ref = (uint8_t *)&wav_mono_ref[0];
	wav_ref_size = sizeof(wav_mono_ref);
	wav_ref_read_size = TEST_DEC_MONO_BUF_SIZE;
	LC3Initialize_fake.custom_fake = fake_LC3Initialize__succeeds;
	LC3DecodeSessionClose_fake.custom_fake = fake_LC3DecodeSessionClose__succeeds;
	LC3BitstreamBuffersize_fake.custom_fake = fake_LC3BitstreamBuffersize__succeeds;
	LC3DecodeSessionOpen_fake.custom_fake = fake_LC3DecodeSessionOpen__succeeds;
	LC3DecodeSessionData_fake.custom_fake = fake_LC3DecodeSessionData__succeeds;
	LC3Deinitialize_fake.custom_fake = fake_LC3Deinitialize__succeeds;

	lc3_initialize(TEST_LC3_FRAME_SIZE_US);
	zassert_equal(LC3Initialize_fake.call_count, 1, "Failed to call LC3 initialize %d times",
		      LC3Initialize_fake.call_count);

	test_lc3_decoder_multi_init(true, TEST_AUDIO_MULTI_CHANNEL_LOCATIONS);

	do {
		uint8_t *ref = &data_ref[0];
		uint8_t *data_source =
			(uint8_t *)&lc3_mono_in[TEST_ENC_MONO_BUF_SIZE * (dec_call_count >> 1)];
		uint8_t *data_reference =
			(uint8_t *)&wav_mono_ref[TEST_DEC_MONO_BUF_SAMPLES * (dec_call_count >> 1)];

		for (int i = 0; i < TEST_AUDIO_CHANNELS_MAX; i++) {
			memcpy(&data_in[TEST_ENC_MONO_BUF_SIZE * i], data_source,
			       TEST_ENC_MONO_BUF_SIZE);
		}

		/* Interleave the channel samples */
		for (uint32_t i = 0; i < TEST_DEC_MONO_BUF_SAMPLES; i++) {
			for (uint8_t j = 0; j < TEST_AUDIO_CHANNELS_MAX; j++) {
				for (uint8_t k = 0; k < (TEST_SAMPLE_BIT_DEPTH / 8); k++) {
					*ref++ = *data_reference++;
				}

				data_reference -= (TEST_SAMPLE_BIT_DEPTH / 8);
			}

			data_reference += (TEST_SAMPLE_BIT_DEPTH / 8);
		}

		memcpy(&audio_data_tx, &audio_data, sizeof(struct audio_data));
		memcpy(&audio_data_rx, &audio_data, sizeof(struct audio_data));

		audio_data_tx.meta.data_coding = LC3;
		audio_data_tx.data = &data_in[0];
		audio_data_tx.data_size = TEST_ENC_MULTI_BUF_SIZE;
		audio_data_tx.meta.locations = TEST_AUDIO_MULTI_CHANNEL_LOCATIONS;

		audio_data_rx.data = (void *)&data_out[0];
		audio_data_rx.data_size = TEST_DEC_MULTI_BUF_SIZE;

		ret = audio_module_data_tx_rx(&handle[TEST_MODULE_ID_DECODER_1],
					      &handle[TEST_MODULE_ID_DECODER_1], &audio_data_tx,
					      &audio_data_rx, K_FOREVER);
		zassert_equal(ret, 0, "Data TX-RX did not return zero for left decoder: %d", ret);
		zassert_equal(audio_data_rx.data_size, TEST_DEC_MULTI_BUF_SIZE,
			      "Decoded number of bytes incorrect: %d", audio_data_rx.data_size);
		zassert_mem_equal(&data_out[0], &data_ref[0], TEST_DEC_MULTI_BUF_SIZE,
				  "Decoded PCM data does not match reference PCM data");
		zassert_equal(LC3DecodeSessionData_fake.call_count, dec_call_count + 2,
			      "Failed to call LC3 decode %d times",
			      LC3DecodeSessionData_fake.call_count);

		dec_call_count += 2;

	} while (((dec_call_count >> 1) < (LC3_MONO_IN_SIZE / TEST_ENC_MONO_BUF_SIZE) &&
		  (dec_call_count >> 1) < (WAV_MONO_REF_SIZE / TEST_DEC_MONO_BUF_SAMPLES)));

	ret = audio_module_stop(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to stop module did not return zero: ret = %d", ret);

	ret = audio_module_close(&handle[TEST_MODULE_ID_DECODER_1]);
	zassert_equal(ret, 0, "Failed to close module did not return zero: ret = %d", ret);

	lc3_deinitialize();
	zassert_equal(LC3Deinitialize_fake.call_count, 1,
		      "Failed to call LC3 deinitialize %d times", LC3Deinitialize_fake.call_count);
}
