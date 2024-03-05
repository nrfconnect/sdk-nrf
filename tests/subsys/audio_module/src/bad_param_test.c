/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>

#include "fakes.h"
#include "audio_module/audio_module.h"
#include "audio_module_test_common.h"

static struct audio_module_functions mod_1_functions = {.open = NULL,
							.close = NULL,
							.configuration_set = NULL,
							.configuration_get = NULL,
							.start = NULL,
							.stop = NULL,
							.data_process = NULL};

static struct audio_module_description test_description, test_description_1, test_description_2,
	test_description_tx, test_description_rx;
static struct mod_config configuration;
static struct audio_module_configuration *config =
	(struct audio_module_configuration *)&configuration;
static struct audio_module_handle handle, handle_tx, handle_rx;
static struct mod_context context;
static uint8_t test_data[FAKE_FIFO_MSG_QUEUE_DATA_SIZE];
static struct data_fifo mod_fifo_tx, mod_fifo_rx;
static struct audio_data test_block, test_block_tx, test_block_rx;
static char mod_thread_stack[TEST_MOD_THREAD_STACK_SIZE];

/**
 * @brief Function to initialize a module's handle.
 *
 * @param description  [in/out]  Pointer to the module's description.
 * @param name         [in]      Pointer to the module's base name.
 * @param type         [in]      The type of the module.
 * @param functions    [in]      Pointer to the module's function pointer structure.
 */
static void test_initialize_description(struct audio_module_description *description, char *name,
					enum audio_module_type type,
					struct audio_module_functions *functions)
{
	description->name = name;
	description->type = type;
	description->functions = functions;
}

/**
 * @brief Function to initialize a module's handle.
 *
 * @param test_handle    [in/out]  The handle to the module instance.
 * @param instance_name  [in]      Pointer to the module's instance name.
 * @param description    [in]      Pointer to the module's description.
 * @param state          [in]      The state of the module.
 * @param fifo_tx        [in]      Pointer to the module's TX FIFO.
 * @param fifo_rx        [in]      Pointer to the module's RX FIFO.
 */
static void test_initialize_handle(struct audio_module_handle *test_handle, char *instance_name,
				   struct audio_module_description *description,
				   enum audio_module_state state, struct data_fifo *fifo_tx,
				   struct data_fifo *fifo_rx)
{
	strncpy(&test_handle->name[0], instance_name, CONFIG_AUDIO_MODULE_NAME_SIZE);
	test_handle->name[CONFIG_AUDIO_MODULE_NAME_SIZE] = '\0';
	test_handle->description = description;
	test_handle->state = state;
	test_handle->thread.msg_tx = fifo_tx;
	test_handle->thread.msg_rx = fifo_rx;
}

ZTEST(suite_audio_module_bad_param, test_number_channels_calculate_null)
{
	int ret;

	ret = audio_module_number_channels_calculate(1, NULL);
	zassert_equal(ret, -EINVAL,
		      "Calculate number of channels function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
}

ZTEST(suite_audio_module_bad_param, test_state_get_state)
{
	int ret;
	enum audio_module_state state;

	state = AUDIO_MODULE_STATE_UNDEFINED;
	handle.state = AUDIO_MODULE_STATE_UNDEFINED - 1;
	ret = audio_module_state_get(NULL, &state);
	zassert_equal(ret, -EINVAL, "Get state function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
	zassert_equal(state, AUDIO_MODULE_STATE_UNDEFINED,
		      "Get state function has changed the state returned: %d", state);

	state = AUDIO_MODULE_STATE_UNDEFINED;
	handle.state = AUDIO_MODULE_STATE_STOPPED + 1;
	ret = audio_module_state_get(NULL, &state);
	zassert_equal(ret, -EINVAL, "Get state function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
	zassert_equal(state, AUDIO_MODULE_STATE_UNDEFINED,
		      "Get state function has changed the state returned: %d", state);
}

ZTEST(suite_audio_module_bad_param, test_state_get_null)
{
	int ret;
	enum audio_module_state state;

	ret = audio_module_state_get(NULL, &state);
	zassert_equal(ret, -EINVAL, "Get state function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_state_get(&handle, NULL);
	zassert_equal(ret, -EINVAL, "Get state function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
}

ZTEST(suite_audio_module_bad_param, test_names_get_null)
{
	int ret;
	char *base_name;
	char instance_name[CONFIG_AUDIO_MODULE_NAME_SIZE];

	ret = audio_module_names_get(NULL, &base_name, &instance_name[0]);
	zassert_equal(ret, -EINVAL, "Get names function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_names_get(&handle, NULL, &instance_name[0]);
	zassert_equal(ret, -EINVAL, "Get names function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_names_get(&handle, &base_name, NULL);
	zassert_equal(ret, -EINVAL, "Get names function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	handle.state = AUDIO_MODULE_STATE_UNDEFINED;
	ret = audio_module_names_get(&handle, &base_name, &instance_name[0]);
	zassert_equal(ret, -ECANCELED,
		      "Get names function did not return successfully (%d): ret %d", -ECANCELED,
		      ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_UNDEFINED,
		      "Get names returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_data_tx_bad_state)
{
	int ret;

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_UNDEFINED,
				    NULL);

	test_initialize_handle(&handle, "TEST TX", &test_description, AUDIO_MODULE_STATE_RUNNING,
			       NULL, &mod_fifo_rx);

	test_block.data = &test_data[0];
	test_block.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -ECANCELED, "Data TX function did not return -EALREADY (%d): ret %d",
		      -ECANCELED, ret);

	test_description.type = AUDIO_MODULE_TYPE_INPUT;

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -ECANCELED, "Data TX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description.type = AUDIO_MODULE_TYPE_IN_OUT;

	handle.state = AUDIO_MODULE_STATE_UNDEFINED;

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -ECANCELED, "Data TX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle.state = AUDIO_MODULE_STATE_CONFIGURED;

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -ECANCELED, "Data TX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle.state = AUDIO_MODULE_STATE_STOPPED;

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -ECANCELED, "Data TX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_data_tx_null)
{
	int ret;

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);

	test_initialize_handle(&handle, "TEST TX", &test_description, AUDIO_MODULE_STATE_RUNNING,
			       NULL, NULL);

	test_block.data = &test_data[0];
	test_block.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_tx(NULL, &test_block, NULL);
	zassert_equal(ret, -EINVAL, "Data TX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -ECANCELED, "Data TX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle.thread.msg_rx = &mod_fifo_rx;

	ret = audio_module_data_tx(&handle, NULL, NULL);
	zassert_equal(ret, -EINVAL, "Data TX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	test_block.data = NULL;
	test_block.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -EINVAL, "Data TX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	test_block.data = &test_data[0];
	test_block.data_size = 0;

	ret = audio_module_data_tx(&handle, &test_block, NULL);
	zassert_equal(ret, -EINVAL, "Data TX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
}

ZTEST(suite_audio_module_bad_param, test_data_rx_bad_state)
{
	int ret;

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_UNDEFINED,
				    NULL);

	test_initialize_handle(&handle, "TEST RX", &test_description, AUDIO_MODULE_STATE_RUNNING,
			       &mod_fifo_tx, NULL);

	test_block.data = &test_data[0];
	test_block.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description.type = AUDIO_MODULE_TYPE_OUTPUT;

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description.type = AUDIO_MODULE_TYPE_IN_OUT;

	handle.state = AUDIO_MODULE_STATE_UNDEFINED;

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle.state = AUDIO_MODULE_STATE_CONFIGURED;

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle.state = AUDIO_MODULE_STATE_STOPPED;

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_data_rx_null)
{
	int ret;

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);

	test_initialize_handle(&handle, "TEST RX", &test_description, AUDIO_MODULE_STATE_RUNNING,
			       NULL, NULL);

	test_block.data = &test_data[0];
	test_block.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_rx(NULL, &test_block, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data RX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle.thread.msg_tx = &mod_fifo_tx;

	ret = audio_module_data_rx(&handle, NULL, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data RX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	test_block.data = NULL;
	test_block.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data RX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	test_block.data = &test_data[0];
	test_block.data_size = 0;

	ret = audio_module_data_rx(&handle, &test_block, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data RX function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
}

ZTEST(suite_audio_module_bad_param, test_data_tx_rx_bad_state)
{
	int ret;

	test_initialize_description(&test_description_tx, "Module Test TX",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);
	test_initialize_description(&test_description_rx, "Module Test RX",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle_tx, "TEST TX", &test_description_tx,
			       AUDIO_MODULE_STATE_RUNNING, &mod_fifo_tx, &mod_fifo_rx);
	test_initialize_handle(&handle_rx, "TEST RX", &test_description_rx,
			       AUDIO_MODULE_STATE_RUNNING, &mod_fifo_tx, &mod_fifo_rx);

	test_block_tx.data = &test_data[0];
	test_block_tx.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;
	test_block_rx.data = &test_data[0];
	test_block_rx.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_tx.type = AUDIO_MODULE_TYPE_INPUT;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_tx.type = AUDIO_MODULE_TYPE_IN_OUT;
	test_description_rx.type = AUDIO_MODULE_TYPE_UNDEFINED;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_rx.type = AUDIO_MODULE_TYPE_OUTPUT;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_tx.type = AUDIO_MODULE_TYPE_IN_OUT;
	test_description_rx.type = AUDIO_MODULE_TYPE_IN_OUT;

	handle_tx.state = AUDIO_MODULE_STATE_UNDEFINED;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_STATE_CONFIGURED;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_STATE_STOPPED;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_STATE_RUNNING;
	handle_rx.state = AUDIO_MODULE_STATE_UNDEFINED;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_rx.state = AUDIO_MODULE_STATE_CONFIGURED;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_rx.state = AUDIO_MODULE_STATE_STOPPED;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -ECANCELED, "Data TX/RX function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_data_tx_rx_null)
{
	int ret;

	test_initialize_description(&test_description_tx, "Module Test TX",
				    AUDIO_MODULE_TYPE_IN_OUT, NULL);
	test_initialize_description(&test_description_rx, "Module Test RX",
				    AUDIO_MODULE_TYPE_IN_OUT, NULL);

	test_initialize_handle(&handle_tx, "TEST TX", &test_description_tx,
			       AUDIO_MODULE_STATE_RUNNING, NULL, NULL);
	test_initialize_handle(&handle_rx, "TEST RX", &test_description_rx,
			       AUDIO_MODULE_STATE_RUNNING, NULL, NULL);

	test_block_tx.data = &test_data[0];
	test_block_tx.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;
	test_block_rx.data = &test_data[0];
	test_block_rx.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_tx_rx(NULL, &handle_rx, &test_block_tx, &test_block_rx, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_data_tx_rx(&handle_tx, NULL, &test_block_tx, &test_block_rx, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	handle_tx.thread.msg_tx = &mod_fifo_tx;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	handle_rx.thread.msg_rx = &mod_fifo_rx;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, NULL, &test_block_rx, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data /RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, NULL, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	test_block_tx.data = NULL;
	test_block_tx.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	test_block_tx.data = &test_data[0];
	test_block_tx.data_size = 0;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	test_block_tx.data = &test_data[0];
	test_block_tx.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;
	test_block_rx.data = NULL;
	test_block_rx.data_size = FAKE_FIFO_MSG_QUEUE_DATA_SIZE;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	test_block_rx.data = &test_data[0];
	test_block_rx.data_size = 0;

	ret = audio_module_data_tx_rx(&handle_tx, &handle_rx, &test_block_tx, &test_block_rx,
				      K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "Data TX/RX function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
}

ZTEST(suite_audio_module_bad_param, test_stop_bad_state)
{
	int ret;

	test_initialize_description(&test_description_rx, "Module Test",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle, "TEST Stop", &test_description,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);

	ret = audio_module_stop(&handle);
	zassert_equal(ret, -ECANCELED, "Stop function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_UNDEFINED,
		      "Stop returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_CONFIGURED;
	ret = audio_module_stop(&handle);
	zassert_equal(ret, -EALREADY, "Stop function did not return -EALREADY (%d): ret %d",
		      -EALREADY, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Stop returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_stop(&handle);
	zassert_equal(ret, -EALREADY, "Stop function did not return -EALREADY (%d): ret %d",
		      -EALREADY, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Stop returns with incorrect state: ret %d", ret);
}

ZTEST(suite_audio_module_bad_param, test_stop_null)
{
	int ret;

	test_initialize_description(&test_description_rx, "Module Test",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle, "TEST Stop", &test_description, AUDIO_MODULE_STATE_RUNNING,
			       NULL, NULL);

	ret = audio_module_stop(NULL);
	zassert_equal(ret, -EINVAL, "Stop function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	handle.state = AUDIO_MODULE_STATE_RUNNING;
	ret = audio_module_stop(NULL);
	zassert_equal(ret, -EINVAL, "Stop function did not return successfully (%d): ret %d",
		      -EINVAL, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_RUNNING,
		      "Stop returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_start_bad_state)
{
	int ret;

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_UNDEFINED,
				    NULL);

	test_initialize_handle(&handle, "TEST Start", &test_description,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);

	ret = audio_module_start(&handle);
	zassert_equal(ret, -ECANCELED, "Start function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_UNDEFINED,
		      "Start returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;

	ret = audio_module_start(&handle);
	zassert_equal(ret, -ECANCELED, "Start function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Start returns with incorrect state: %d", handle.state);

	test_description.type = AUDIO_MODULE_TYPE_IN_OUT;
	handle.state = AUDIO_MODULE_STATE_RUNNING;

	ret = audio_module_start(&handle);
	zassert_equal(ret, -EALREADY, "Start function did not return -EALREADY (%d): ret %d",
		      -EALREADY, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_RUNNING,
		      "Start returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_start_null)
{
	int ret;

	test_initialize_description(&test_description_rx, "Module Test",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle, "TEST Start", &test_description, AUDIO_MODULE_STATE_RUNNING,
			       NULL, NULL);

	ret = audio_module_start(NULL);
	zassert_equal(ret, -EINVAL, "Start function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_start(NULL);
	zassert_equal(ret, -EINVAL, "Start function did not return successfully (%d): ret %d",
		      -EINVAL, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Start returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_config_get_bad_state)
{
	int ret;

	test_initialize_description(&test_description_rx, "Module Test",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle, "TEST Get Config", &test_description,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);

	ret = audio_module_configuration_get(&handle, config);
	zassert_equal(ret, -ECANCELED,
		      "Configuration get function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_config_get_null)
{
	int ret;

	test_initialize_description(&test_description_rx, "Module Test",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle, "TEST Get Config", &test_description,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);

	ret = audio_module_configuration_get(NULL, NULL);
	zassert_equal(ret, -EINVAL,
		      "Configuration get function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_configuration_get(NULL, config);
	zassert_equal(ret, -EINVAL,
		      "Configuration get function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_configuration_get(&handle, NULL);
	zassert_equal(ret, -EINVAL,
		      "Configuration get function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_configuration_get(NULL, config);
	zassert_equal(ret, -EINVAL,
		      "Configuration get function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Configuration get returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_configuration_get(&handle, NULL);
	zassert_equal(ret, -EINVAL,
		      "Configuration get function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Configuration get returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_configuration_get(NULL, NULL);
	zassert_equal(ret, -EINVAL,
		      "Configuration get function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Configuration get returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_reconfig_bad_state)
{
	int ret;

	test_initialize_description(&test_description_rx, "Module Test",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle, "TEST Reconfig", &test_description,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);

	ret = audio_module_reconfigure(&handle, config);
	zassert_equal(ret, -ECANCELED,
		      "Reconfiguration function did not return -ECANCELED (%d): ret %d", -ECANCELED,
		      ret);
}

ZTEST(suite_audio_module_bad_param, test_reconfig_null)
{
	int ret;

	test_initialize_description(&test_description_rx, "Module Test",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle, "TEST Reconfig", &test_description,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);

	ret = audio_module_reconfigure(NULL, NULL);
	zassert_equal(ret, -EINVAL, "Reconfiguration function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_reconfigure(NULL, config);
	zassert_equal(ret, -EINVAL, "Reconfiguration function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_reconfigure(&handle, NULL);
	zassert_equal(ret, -EINVAL, "Reconfiguration function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_reconfigure(NULL, config);
	zassert_equal(ret, -EINVAL, "Reconfiguration function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Reconfiguration returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_reconfigure(&handle, NULL);
	zassert_equal(ret, -EINVAL, "Reconfiguration function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Reconfiguration returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_reconfigure(NULL, NULL);
	zassert_equal(ret, -EINVAL, "Reconfiguration function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Reconfiguration returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_disconnect_bad_type)
{
	int ret;

	test_initialize_description(&test_description_1, "Module Test 1",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);
	test_initialize_description(&test_description_2, "Module Test 2",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle_tx, "TEST Disconnect 1", &test_description_1,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);
	test_initialize_handle(&handle_rx, "TEST Disconnect 2", &test_description_2,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);

	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_1.type = AUDIO_MODULE_TYPE_IN_OUT;
	test_description_2.type = AUDIO_MODULE_TYPE_UNDEFINED;
	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_TYPE_UNDEFINED;
	handle_rx.state = AUDIO_MODULE_TYPE_IN_OUT;
	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_1.type = AUDIO_MODULE_TYPE_INPUT;
	test_description_2.type = AUDIO_MODULE_TYPE_INPUT;
	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_TYPE_OUTPUT;
	handle_rx.state = AUDIO_MODULE_TYPE_INPUT;
	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_TYPE_IN_OUT;
	handle_rx.state = AUDIO_MODULE_TYPE_OUTPUT;
	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_disconnect_bad_state)
{
	int ret;

	test_initialize_description(&test_description_1, "Module Test 1", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);
	test_initialize_description(&test_description_2, "Module Test 2", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);

	test_initialize_handle(&handle_tx, "TEST Disconnect 1", &test_description_1,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);
	test_initialize_handle(&handle_rx, "TEST Disconnect 2", &test_description_2,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);

	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_STATE_CONFIGURED;
	handle_rx.state = AUDIO_MODULE_STATE_UNDEFINED;
	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_STATE_UNDEFINED;
	handle_rx.state = AUDIO_MODULE_STATE_CONFIGURED;
	ret = audio_module_disconnect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Disconnect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_disconnect_null)
{
	int ret;

	test_initialize_description(&test_description_1, "Module Test 1", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);
	test_initialize_description(&test_description_2, "Module Test 2", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);

	test_initialize_handle(&handle_tx, "TEST Disconnect 1", &test_description_1,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);
	test_initialize_handle(&handle_rx, "TEST Disconnect 2", &test_description_2,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);

	ret = audio_module_disconnect(NULL, NULL, false);
	zassert_equal(ret, -EINVAL, "Disconnect set function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_disconnect(NULL, &handle_rx, false);
	zassert_equal(ret, -EINVAL, "Disconnect set function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_disconnect(&handle_tx, NULL, false);
	zassert_equal(ret, -EINVAL, "Disconnect set function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_disconnect(&handle_tx, &handle_tx, false);
	zassert_equal(ret, -EINVAL, "Disconnect set function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_module_disconnect(&handle_tx, &handle_rx, true);
	zassert_equal(ret, -EINVAL, "Disconnect set function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
}

ZTEST(suite_audio_module_bad_param, test_connect_bad_type)
{
	int ret;

	test_initialize_description(&test_description_1, "Module Test 1",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);
	test_initialize_description(&test_description_2, "Module Test 2",
				    AUDIO_MODULE_TYPE_UNDEFINED, NULL);

	test_initialize_handle(&handle_tx, "TEST Connect 1", &test_description_1,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);
	test_initialize_handle(&handle_rx, "TEST Connect 2", &test_description_2,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);

	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_1.type = AUDIO_MODULE_TYPE_IN_OUT;
	test_description_2.type = AUDIO_MODULE_TYPE_UNDEFINED;
	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_TYPE_UNDEFINED;
	handle_rx.state = AUDIO_MODULE_TYPE_IN_OUT;
	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_description_1.type = AUDIO_MODULE_TYPE_INPUT;
	test_description_2.type = AUDIO_MODULE_TYPE_INPUT;
	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_TYPE_OUTPUT;
	handle_rx.state = AUDIO_MODULE_TYPE_INPUT;
	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_TYPE_IN_OUT;
	handle_rx.state = AUDIO_MODULE_TYPE_OUTPUT;
	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_connect_bad_state)
{
	int ret;

	test_initialize_description(&test_description_1, "Module Test 1", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);
	test_initialize_description(&test_description_2, "Module Test 2", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);

	test_initialize_handle(&handle_tx, "TEST Connect 1", &test_description_1,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);
	test_initialize_handle(&handle_rx, "TEST Connect 2", &test_description_2,
			       AUDIO_MODULE_STATE_UNDEFINED, NULL, NULL);

	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_STATE_CONFIGURED;
	handle_rx.state = AUDIO_MODULE_STATE_UNDEFINED;
	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	handle_tx.state = AUDIO_MODULE_STATE_UNDEFINED;
	handle_rx.state = AUDIO_MODULE_STATE_CONFIGURED;
	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -ECANCELED, "Connect function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_connect_null)
{
	int ret;

	test_initialize_description(&test_description_1, "Module Test 1", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);
	test_initialize_description(&test_description_2, "Module Test 2", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);

	test_initialize_handle(&handle_tx, "TEST Connect 1", &test_description_1,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);
	test_initialize_handle(&handle_rx, "TEST Connect 2", &test_description_2,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);

	ret = audio_module_connect(NULL, NULL, false);
	zassert_equal(ret, -EINVAL, "Connect function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_connect(NULL, &handle_rx, false);
	zassert_equal(ret, -EINVAL, "Connect function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_connect(&handle_tx, NULL, false);
	zassert_equal(ret, -EINVAL, "Connect function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, 0, "Connect function did not return successfully: ret %d", ret);

	ret = audio_module_connect(&handle_tx, &handle_rx, false);
	zassert_equal(ret, -EALREADY, "Connect function did not return -EALREADY (%d): ret %d",
		      -EALREADY, ret);

	ret = audio_module_connect(&handle_tx, NULL, true);
	zassert_equal(ret, -EINVAL, "Connect function did not return -EALREADY (%d): ret %d",
		      -EINVAL, ret);
}

ZTEST(suite_audio_module_bad_param, test_close_bad_state)
{
	int ret;

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);

	test_initialize_handle(&handle, "TEST Close", &test_description,
			       AUDIO_MODULE_STATE_CONFIGURED, NULL, NULL);

	ret = audio_module_close(NULL);
	zassert_equal(ret, -EINVAL, "Close function did not return -EALREADY (%d): ret %d", -EINVAL,
		      ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Close returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_UNDEFINED;
	ret = audio_module_close(&handle);
	zassert_equal(ret, -ECANCELED, "Close function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_UNDEFINED,
		      "Close returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_RUNNING;
	ret = audio_module_close(&handle);
	zassert_equal(ret, -ECANCELED, " did not return -ECANCELED (%d): ret %d", -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_RUNNING,
		      "Close returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_close_null)
{
	int ret;

	ret = audio_module_close(NULL);
	zassert_equal(ret, -EINVAL, "Close function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
}

ZTEST(suite_audio_module_bad_param, test_open_bad_thread)
{
	int ret;
	char *inst_name = "TEST open";
	struct audio_module_parameters test_params_thread;

	test_params_thread.thread.stack = NULL;
	test_params_thread.thread.stack_size = TEST_MOD_THREAD_STACK_SIZE;
	test_params_thread.thread.priority = TEST_MOD_THREAD_PRIORITY;

	ret = audio_module_open(&test_params_thread, config, inst_name,
				(struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_params_thread.thread.stack = (k_thread_stack_t *)&mod_thread_stack;
	test_params_thread.thread.stack_size = 0;
	test_params_thread.thread.priority = TEST_MOD_THREAD_PRIORITY;

	ret = audio_module_open(&test_params_thread, config, inst_name,
				(struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_open_bad_description)
{
	int ret;
	char *inst_name = "TEST open";
	struct audio_module_parameters test_params_desc = {0};

	test_params_desc.description = NULL;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_initialize_description(&test_description, "Module Test", -1, &mod_1_functions);
	test_params_desc.description = &test_description;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_initialize_description(&test_description, NULL, AUDIO_MODULE_TYPE_IN_OUT,
				    &mod_1_functions);
	test_params_desc.description = &test_description;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_IN_OUT,
				    NULL);
	test_params_desc.description = &test_description;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_UNDEFINED,
				    &mod_1_functions);
	test_params_desc.description = &test_description;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	mod_1_functions.configuration_set = &test_config_set_function;
	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_UNDEFINED,
				    &mod_1_functions);
	test_params_desc.description = &test_description;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	mod_1_functions.configuration_get = &test_config_get_function;
	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_UNDEFINED,
				    &mod_1_functions);
	test_params_desc.description = &test_description;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);

	mod_1_functions.data_process = &test_data_process_function;
	test_initialize_description(&test_description, "Module Test", AUDIO_MODULE_TYPE_UNDEFINED,
				    &mod_1_functions);
	test_params_desc.description = &test_description;

	ret = audio_module_open(&test_params_desc, (struct audio_module_configuration *)&config,
				inst_name, (struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
}

ZTEST(suite_audio_module_bad_param, test_open_bad_state)
{
	int ret;
	char *inst_name = "TEST open";
	struct audio_module_description test_description = {.name = "Module 1",
							    .type = AUDIO_MODULE_TYPE_IN_OUT,
							    .functions = &mod_1_functions};
	struct audio_module_parameters test_params = {.description = &test_description};

	handle.state = AUDIO_MODULE_STATE_CONFIGURED;
	ret = audio_module_open(&test_params, config, inst_name,
				(struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Open returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_RUNNING;
	ret = audio_module_open(&test_params, config, inst_name,
				(struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_RUNNING,
		      "Open returns with incorrect state: %d", handle.state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_open(&test_params, config, inst_name,
				(struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -ECANCELED, "Open function did not return -ECANCELED (%d): ret %d",
		      -ECANCELED, ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Open returns with incorrect state: %d", handle.state);
}

ZTEST(suite_audio_module_bad_param, test_open_null)
{
	int ret;
	char *inst_name = "TEST open";
	struct audio_module_description test_description = {.name = "Module 1",
							    .type = AUDIO_MODULE_TYPE_IN_OUT,
							    .functions = &mod_1_functions};
	struct audio_module_parameters test_params = {.description = &test_description};
	struct mod_config configuration = {0};
	struct audio_module_configuration *config =
		(struct audio_module_configuration *)&configuration;
	struct audio_module_handle handle = {0};
	struct mod_context context = {0};

	ret = audio_module_open(NULL, NULL, NULL, NULL, NULL);
	zassert_equal(ret, -EINVAL, "Open function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_open(NULL, config, inst_name, (struct audio_module_context *)&context,
				&handle);
	zassert_equal(ret, -EINVAL, "Open function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_open(&test_params, NULL, inst_name,
				(struct audio_module_context *)&context, &handle);
	zassert_equal(ret, -EINVAL, "Open function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_open(&test_params, config, NULL, (struct audio_module_context *)&context,
				&handle);
	zassert_equal(ret, -EINVAL, "Open function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_open(&test_params, config, inst_name, NULL, &handle);
	zassert_equal(ret, -EINVAL, "Open function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_module_open(&test_params, config, inst_name,
				(struct audio_module_context *)&context, NULL);
	zassert_equal(ret, -EINVAL,
		      "The module open function with NULLn did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	handle.state = AUDIO_MODULE_STATE_UNDEFINED;
	ret = audio_module_open(NULL, NULL, NULL, NULL, NULL);
	zassert_equal(ret, -EINVAL, "Open function did not return -EALREADY (%d): ret %d", -EINVAL,
		      ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_UNDEFINED,
		      "Open returns with incorrect state: %d", handle.state);
}
