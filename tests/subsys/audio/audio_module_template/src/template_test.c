/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <errno.h>

#include "audio_module.h"
#include "audio_module_template.h"

/* Define a timeout to prevent system locking */
#define TEST_TX_RX_TIMEOUT_US (K_FOREVER)

#define CONFIG_TEXT   "Template config"
#define RECONFIG_TEXT "Template reconfig"

#define TEST_MSG_QUEUE_SIZE	   (4)
#define TEST_MOD_THREAD_STACK_SIZE (2048)
#define TEST_MOD_THREAD_PRIORITY   (4)
#define TEST_CONNECTIONS_NUM	   (5)
#define TEST_MODULES_NUM	   (TEST_CONNECTIONS_NUM - 1)
#define TEST_MOD_DATA_SIZE	   (40)
#define TEST_MSG_SIZE		   (sizeof(struct audio_module_message))
#define TEST_AUDIO_DATA_ITEMS_NUM  (20)

struct mod_config {
	int test_int1;
	int test_int2;
	int test_int3;
	int test_int4;
};

struct mod_context {
	const char *test_string;
	uint32_t test_uint32;
	struct mod_config config;
};

const char *test_instance_name = "Test instance";
struct audio_metadata test_metadata = {.data_coding = LC3,
				       .data_len_us = 10000,
				       .sample_rate_hz = 48000,
				       .bits_per_sample = 16,
				       .carried_bits_per_sample = 16,
				       .locations = 0x00000003,
				       .reference_ts_us = 0,
				       .data_rx_ts_us = 0,
				       .bad_data = false};

K_THREAD_STACK_ARRAY_DEFINE(mod_temp_stack, TEST_MODULES_NUM, TEST_MOD_THREAD_STACK_SIZE);
DATA_FIFO_DEFINE(msg_fifo_tx0, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
DATA_FIFO_DEFINE(msg_fifo_rx0, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
DATA_FIFO_DEFINE(msg_fifo_tx1, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
DATA_FIFO_DEFINE(msg_fifo_rx1, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
DATA_FIFO_DEFINE(msg_fifo_tx2, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
DATA_FIFO_DEFINE(msg_fifo_rx2, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
DATA_FIFO_DEFINE(msg_fifo_tx3, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
DATA_FIFO_DEFINE(msg_fifo_rx3, TEST_MSG_QUEUE_SIZE, TEST_MSG_SIZE);
K_MEM_SLAB_DEFINE(mod_data_slab, TEST_MOD_DATA_SIZE, TEST_MSG_QUEUE_SIZE, 4);

struct data_fifo *msg_fifo_tx_array[TEST_MODULES_NUM] = {&msg_fifo_tx0, &msg_fifo_tx1,
							 &msg_fifo_tx2, &msg_fifo_tx3};
struct data_fifo *msg_fifo_rx_array[TEST_MODULES_NUM] = {&msg_fifo_rx0, &msg_fifo_rx1,
							 &msg_fifo_rx2, &msg_fifo_rx3};

ZTEST(suite_audio_module_template, test_module_template)
{
	int ret;
	int i;
	char *base_name, instance_name[CONFIG_AUDIO_MODULE_NAME_SIZE];

	struct audio_data audio_data_tx;
	struct audio_data audio_data_rx;

	struct audio_module_parameters mod_parameters;

	struct audio_module_template_configuration configuration = {
		.sample_rate_hz = 48000, .bit_depth = 24, .module_description = CONFIG_TEXT};
	struct audio_module_template_configuration reconfiguration = {
		.sample_rate_hz = 16000, .bit_depth = 8, .module_description = RECONFIG_TEXT};
	struct audio_module_template_configuration return_configuration;

	struct audio_module_template_context context = {0};

	uint8_t test_data_in[TEST_MOD_DATA_SIZE * TEST_AUDIO_DATA_ITEMS_NUM];
	uint8_t test_data_out[TEST_MOD_DATA_SIZE * TEST_AUDIO_DATA_ITEMS_NUM];

	struct audio_module_handle handle;

	mod_parameters.description = audio_module_template_description;
	mod_parameters.thread.stack = mod_temp_stack[0];
	mod_parameters.thread.stack_size = TEST_MOD_THREAD_STACK_SIZE;
	mod_parameters.thread.priority = TEST_MOD_THREAD_PRIORITY;
	mod_parameters.thread.data_slab = &mod_data_slab;
	mod_parameters.thread.data_size = TEST_MOD_DATA_SIZE;
	mod_parameters.thread.msg_rx = &msg_fifo_rx0;
	mod_parameters.thread.msg_tx = &msg_fifo_tx0;

	ret = audio_module_open(
		&mod_parameters, (struct audio_module_configuration const *const)&configuration,
		test_instance_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, 0, "Open function did not return successfully (0): ret %d", ret);

	{
		struct audio_module_template_context *ctx =
			(struct audio_module_template_context *)handle.context;
		struct audio_module_template_configuration *test_config = &ctx->config;

		zassert_equal(test_config->bit_depth, configuration.bit_depth,
			      "Failed to configure module in the open function");
		zassert_equal(test_config->sample_rate_hz, configuration.sample_rate_hz,
			      "Failed to configure module in the open function");
		zassert_mem_equal(test_config->module_description, configuration.module_description,
				  sizeof(CONFIG_TEXT),
				  "Failed to configure module in the open function");
	}

	zassert_equal_ptr(handle.description->name, audio_module_template_description->name,
			  "Failed open for names, base name should be %s, but is %s",
			  audio_module_template_description->name, handle.description->name);
	zassert_mem_equal(&handle.name[0], test_instance_name, strlen(test_instance_name),
			  "Failed open for names, instance name should be %s, but is %s",
			  test_instance_name, handle.name);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Not in configured state after call to open the audio module");

	ret = audio_module_names_get(&handle, &base_name, &instance_name[0]);
	zassert_equal(ret, 0, "Get names function did not return successfully (0): ret %d", ret);
	zassert_mem_equal(base_name, audio_module_template_description->name,
			  strlen(audio_module_template_description->name),
			  "Failed get names, base name should be %s, but is %s",
			  audio_module_template_description->name, base_name);
	zassert_mem_equal(&instance_name[0], test_instance_name, strlen(test_instance_name),
			  "Failed get names, instance name should be %s, but is %s", handle.name,
			  &instance_name[0]);

	ret = audio_module_configuration_get(
		&handle, (struct audio_module_configuration *)&return_configuration);
	zassert_equal(ret, 0, "Configuration get function did not return successfully (0): ret %d",
		      ret);
	zassert_equal(return_configuration.bit_depth, configuration.bit_depth,
		      "Failed to get configuration");
	zassert_equal(return_configuration.sample_rate_hz, configuration.sample_rate_hz,
		      "Failed to get configuration");
	zassert_mem_equal(return_configuration.module_description, configuration.module_description,
			  sizeof(CONFIG_TEXT), "Failed to get configuration");

	ret = audio_module_connect(&handle, NULL, true);
	zassert_equal(ret, 0, "Connect function did not return successfully (0): ret %d", ret);
	zassert_equal(handle.use_tx_queue, 1, "Connect queue size is not %d, but is %d", 1,
		      handle.use_tx_queue);

	ret = audio_module_start(&handle);
	zassert_equal(ret, 0, "Start function did not return successfully (0): ret %d", ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_RUNNING,
		      "Not in running state after call to start the audio module");

	for (i = 0; i < TEST_AUDIO_DATA_ITEMS_NUM; i++) {
		audio_data_tx.data = (void *)&test_data_in[i * TEST_MOD_DATA_SIZE];
		audio_data_tx.data_size = TEST_MOD_DATA_SIZE;
		memcpy(&audio_data_tx.meta, &test_metadata, sizeof(struct audio_metadata));

		audio_data_rx.data = (void *)&test_data_out[i * TEST_MOD_DATA_SIZE];
		audio_data_rx.data_size = TEST_MOD_DATA_SIZE;

		ret = audio_module_data_tx_rx(&handle, &handle, &audio_data_tx, &audio_data_rx,
					      TEST_TX_RX_TIMEOUT_US);
		zassert_equal(ret, 0, "Data TX-RX function did not return successfully (0): ret %d",
			      ret);
		zassert_mem_equal(audio_data_tx.data, audio_data_rx.data, TEST_MOD_DATA_SIZE,
				  "Failed to process data");
		zassert_equal(audio_data_tx.data_size, audio_data_rx.data_size,
			      "Failed to process data, sizes differ");
		zassert_mem_equal(&audio_data_tx.meta, &audio_data_rx.meta,
				  sizeof(struct audio_metadata),
				  "Failed to process data, meta data differs");

		if (i == TEST_AUDIO_DATA_ITEMS_NUM / 2) {
			ret = audio_module_stop(&handle);
			zassert_equal(ret, 0,
				      "Stop function did not return successfully (0): ret %d", ret);

			ret = audio_module_reconfigure(
				&handle,
				(const struct audio_module_configuration *const)&reconfiguration);
			zassert_equal(
				ret, 0,
				"Reconfigure function did not return successfully (0): ret %d",
				ret);
			zassert_equal(reconfiguration.bit_depth, reconfiguration.bit_depth,
				      "Failed to reconfigure module");
			zassert_equal(reconfiguration.sample_rate_hz,
				      reconfiguration.sample_rate_hz,
				      "Failed to reconfigure module");
			zassert_mem_equal(reconfiguration.module_description,
					  reconfiguration.module_description, sizeof(RECONFIG_TEXT),
					  "Failed to reconfigure module");

			ret = audio_module_start(&handle);
			zassert_equal(ret, 0,
				      "Start function did not return successfully (0): ret %d",
				      ret);
			zassert_equal(handle.state, AUDIO_MODULE_STATE_RUNNING,
				      "Not in running state after call to start the audio module");
		}
	}

	ret = audio_module_disconnect(&handle, NULL, true);
	zassert_equal(ret, 0, "Disconnect function did not return successfully (0): ret %d", ret);
	zassert_equal(handle.use_tx_queue, 0, "Disconnect queue size is not %d, but is %d", 0,
		      handle.use_tx_queue);

	ret = audio_module_stop(&handle);
	zassert_equal(ret, 0, "Stop function did not return successfully (0): ret %d", ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Not in stopped state after call to stop the audio module");

	ret = audio_module_close(&handle);
	zassert_equal(ret, 0, "Close function did not return successfully (0): ret %d", ret);
}

ZTEST(suite_audio_module_template, test_module_template_stream)
{
	int ret;
	int i;
	char inst_name[CONFIG_AUDIO_MODULE_NAME_SIZE];

	struct audio_data audio_data_tx;
	struct audio_data audio_data_rx;

	struct audio_module_parameters mod_parameters;

	struct audio_module_template_configuration configuration = {
		.sample_rate_hz = 48000, .bit_depth = 16, .module_description = CONFIG_TEXT};

	struct audio_module_template_context context = {0};

	uint8_t test_data_in[TEST_MOD_DATA_SIZE * TEST_AUDIO_DATA_ITEMS_NUM];
	uint8_t test_data_out[TEST_MOD_DATA_SIZE * TEST_AUDIO_DATA_ITEMS_NUM];

	struct audio_module_handle handle[TEST_MODULES_NUM];

	for (i = 0; i < TEST_MODULES_NUM; i++) {

		memset(&handle[i], 0, sizeof(struct audio_module_handle));

		mod_parameters.description = audio_module_template_description;
		mod_parameters.thread.stack = mod_temp_stack[i];
		mod_parameters.thread.stack_size = TEST_MOD_THREAD_STACK_SIZE;
		mod_parameters.thread.priority = TEST_MOD_THREAD_PRIORITY;
		mod_parameters.thread.data_slab = &mod_data_slab;
		mod_parameters.thread.data_size = TEST_MOD_DATA_SIZE;
		mod_parameters.thread.msg_rx = msg_fifo_rx_array[i];
		mod_parameters.thread.msg_tx = msg_fifo_tx_array[i];

		ret = audio_module_open(
			&mod_parameters,
			(const struct audio_module_configuration *const)&configuration,
			&inst_name[0], (struct audio_module_context *)&context, &handle[i]);
		zassert_equal(ret, 0, "Open function did not return successfully (0): ret %d", ret);
	}

	for (i = 0; i < TEST_MODULES_NUM - 1; i++) {
		ret = audio_module_connect(&handle[i], &handle[i + 1], false);
		zassert_equal(ret, 0, "Connect function did not return successfully (0): ret %d",
			      ret);
	}

	ret = audio_module_connect(&handle[TEST_MODULES_NUM - 1], NULL, true);
	zassert_equal(ret, 0, "Connect function did not return successfully (0): ret %d", ret);

	for (i = 0; i < TEST_MODULES_NUM; i++) {
		ret = audio_module_start(&handle[i]);
		zassert_equal(ret, 0, "Start function did not return successfully (0): ret %d",
			      ret);
	}

	for (i = 0; i < TEST_AUDIO_DATA_ITEMS_NUM; i++) {
		audio_data_tx.data = (void *)&test_data_in[i * TEST_MOD_DATA_SIZE];
		audio_data_tx.data_size = TEST_MOD_DATA_SIZE;
		memcpy(&audio_data_tx.meta, &test_metadata, sizeof(struct audio_metadata));

		audio_data_rx.data = (void *)&test_data_out[i * TEST_MOD_DATA_SIZE];
		audio_data_rx.data_size = TEST_MOD_DATA_SIZE;

		ret = audio_module_data_tx_rx(&handle[0], &handle[TEST_MODULES_NUM - 1],
					      &audio_data_tx, &audio_data_rx,
					      TEST_TX_RX_TIMEOUT_US);
		zassert_equal(ret, 0, "Data TX-RX function did not return successfully (0): ret %d",
			      ret);
		zassert_mem_equal(audio_data_tx.data, audio_data_rx.data, TEST_MOD_DATA_SIZE,
				  "Failed to process data");
		zassert_equal(audio_data_tx.data_size, audio_data_rx.data_size,
			      "Failed to process data, sizes differ");
		zassert_mem_equal(&audio_data_tx.meta, &audio_data_rx.meta,
				  sizeof(struct audio_metadata),
				  "Failed to process data, meta data differs");
	}

	for (i = 1; i < TEST_MODULES_NUM; i++) {
		ret = audio_module_stop(&handle[i]);
		zassert_equal(ret, 0, "Stop function did not return successfully (0): ret %d", ret);

		ret = audio_module_close(&handle[i]);
		zassert_equal(ret, 0, "Close function did not return successfully (0): ret %d",
			      ret);
	}
}
