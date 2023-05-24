/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <errno.h>

#include "fakes.h"
#include "audio_module/audio_module.h"
#include "audio_module_test_common.h"

K_THREAD_STACK_DEFINE(mod_stack, TEST_MOD_THREAD_STACK_SIZE);
K_MEM_SLAB_DEFINE(data_slab, TEST_MOD_DATA_SIZE, FAKE_FIFO_MSG_QUEUE_SIZE, 4);

static const char *test_base_name = "Test base name";

static struct mod_context mod_context, test_mod_context;
static struct mod_config mod_config = {
	.test_int1 = 5, .test_int2 = 4, .test_int3 = 3, .test_int4 = 2};
static const struct audio_module_functions ft_null = {.open = NULL,
						      .close = NULL,
						      .configuration_set = NULL,
						      .configuration_get = NULL,
						      .start = NULL,
						      .stop = NULL,
						      .data_process = NULL};
static const struct audio_module_functions ft_pop = {.open = test_open_function,
						     .close = test_close_function,
						     .configuration_set = test_config_set_function,
						     .configuration_get = test_config_get_function,
						     .start = test_start_function,
						     .stop = test_stop_function,
						     .data_process = test_data_process_function};
static struct audio_module_description mod_description = {
	.name = "Test base name", .type = AUDIO_MODULE_TYPE_IN_OUT, .functions = &ft_null};
static struct audio_module_description test_from_description, test_to_description;
static struct audio_module_parameters mod_parameters = {
	.description = &mod_description,
	.thread = {.stack = mod_stack,
		   .stack_size = TEST_MOD_THREAD_STACK_SIZE,
		   .priority = TEST_MOD_THREAD_PRIORITY,
		   .data_slab = &data_slab,
		   .data_size = TEST_MOD_DATA_SIZE}};
struct audio_module_parameters test_mod_parameters;
static struct audio_module_handle handle;
static struct mod_context *handle_context;
static struct data_fifo mod_fifo_tx, mod_fifo_rx;

/**
 * @brief Set the minimum for a handle.
 *
 * @param test_handle  [in/out]  The handle to the module instance.
 * @param state        [in]      Pointer to the module's state.
 * @param name         [in]      Pointer to the module's base name.
 * @param description  [in/out]  Pointer to the modules description.
 */
static void test_handle_set(struct audio_module_handle *test_handle, enum audio_module_state state,
			    char *name, struct audio_module_description *description)
{
	test_handle->state = state;
	description->name = name;
	test_handle->description = description;
}

/**
 * @brief Simple test thread with handle NULL.
 *
 * @param test_handle  [in]  The handle to the module instance.
 *
 * @return 0 if successful, error value.
 */
static int test_thread_handle(struct audio_module_handle const *const test_handle)
{
	/* Execute thread */
	while (1) {
		zassert_not_null(test_handle, NULL, "handle is NULL!");

		k_sleep(K_MSEC(100));
	}

	return 0;
}

/**
 * @brief Simple function to start a test thread with handle.
 *
 * @param test_handle  [in/out]  The handle to the module instance.
 *
 * @return 0 if successful, error value.
 */
static int thread_start(struct audio_module_handle *test_handle)
{
	int ret;

	test_handle->thread_id = k_thread_create(
		&test_handle->thread_data, test_handle->thread.stack,
		test_handle->thread.stack_size, (k_thread_entry_t)test_thread_handle, test_handle,
		NULL, NULL, K_PRIO_PREEMPT(test_handle->thread.priority), 0, K_FOREVER);

	ret = k_thread_name_set(test_handle->thread_id, test_handle->name);
	if (ret) {
		return ret;
	}

	k_thread_start(test_handle->thread_id);

	return 0;
}

/**
 * @brief Initialize a handle.
 *
 * @param test_handle    [in/out]  The handle to the module instance.
 * @param description    [in]      Pointer to the module's description.
 * @param context        [in/out]  Pointer to the module's context.
 * @param configuration  [in]      Pointer to the module's configuration.
 */
static void test_initialize_handle(struct audio_module_handle *test_handle,
				   struct audio_module_description const *const description,
				   struct mod_context *context,
				   struct mod_config const *const configuration)
{
	memset(test_handle, 0, sizeof(struct audio_module_handle));
	memcpy(&test_handle->name[0], TEST_INSTANCE_NAME, CONFIG_AUDIO_MODULE_NAME_SIZE);
	test_handle->description = description;
	test_handle->state = AUDIO_MODULE_STATE_CONFIGURED;
	test_handle->dest_count = 0;

	if (configuration != NULL) {
		memcpy(&context->config, configuration, sizeof(struct mod_config));
	}

	if (context != NULL) {
		test_handle->context = (struct audio_module_context *)context;
	}
}

/**
 * @brief Initialize a list of connections.
 *
 * @param handle_from   [in/out]  The handle for the module to initialize the list.
 * @param handles_to    [in/out]  Pointer to an array of handles to initialize the list with.
 * @param list_size     [in]      The number of handles to initialize the list with.
 * @param use_tx_queue  [in]      The state to set for use_tx_queue in the handle.
 *
 * @return Number of destinations.
 */
static int test_initialize_connection_list(struct audio_module_handle *handle_from,
					   struct audio_module_handle *handles_to, size_t list_size,
					   bool use_tx_queue)
{
	for (int i = 0; i < list_size; i++) {
		sys_slist_append(&handle_from->handle_dest_list, &handles_to->node);
		handle_from->dest_count += 1;
		handles_to += 1;
	}

	handle_from->use_tx_queue = use_tx_queue;

	if (handle_from->use_tx_queue) {
		handle_from->dest_count += 1;
	}

	return handle_from->dest_count;
}

/**
 * @brief Test a list of connections, both that the handle is in the list and that the list is in
 *        the correct order.
 *
 * @param test_handle   [in]  The handle for the module to test the list.
 * @param handles_to    [in]  Pointer to an array of handles that should be in the list and are in
 *                            the order expected.
 * @param list_size     [in]  The number of handles expected in the list.
 * @param use_tx_queue  [in]  The expected state of use_tx_queue in the handle.
 *
 * @return 0 if successful, assert otherwise.
 */
static int test_list(struct audio_module_handle *test_handle,
		     struct audio_module_handle *handles_to, size_t list_size, bool use_tx_queue)
{
	struct audio_module_handle *handle_get;
	int i = 0;

	zassert_equal(test_handle->dest_count, list_size,
		      "List is the incorrect size, it is %d but should be %d",
		      test_handle->dest_count, list_size);

	zassert_equal(test_handle->use_tx_queue, use_tx_queue,
		      "List is the incorrect, use_tx_queue flag is %d but should be %d",
		      test_handle->use_tx_queue, use_tx_queue);

	SYS_SLIST_FOR_EACH_CONTAINER(&test_handle->handle_dest_list, handle_get, node) {
		zassert_equal_ptr(&handles_to[i], handle_get, "List is incorrect for item %d", i);
		i += 1;
	}

	return 0;
}

/**
 * @brief Test the module's close.
 *
 * @param test_fnct    [in]  Pointer to the audio module's function pointer structure.
 * @param test_type    [in]  The type of the audio module.
 * @param fifo_rx_set  [in]  Flag to indicate if the audio modules RX data FIFO is to be
 * initialised.
 * @param fifo_tx_set  [in]  Flag to indicate if the audio modules TX data FIFO is to be
 * initialised.
 * @param test_state   [in]  The module's state.
 */
static void test_close(const struct audio_module_functions *test_fnct,
		       enum audio_module_type test_type, bool fifo_rx_set, bool fifo_tx_set,
		       enum audio_module_state test_state)
{
	int ret;
	int empty_call_count = 0;
	char *test_inst_name = "TEST instance 1";
	struct audio_module_description test_mod_description;
	struct audio_module_functions test_mod_functions;

	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	mod_description.functions = test_fnct;
	memcpy(&test_mod_functions, test_fnct, sizeof(struct audio_module_functions));

	test_context_set(&test_mod_context, &mod_config);
	mod_context = test_mod_context;

	mod_description.type = test_type;
	test_mod_description = mod_description;
	test_mod_parameters = mod_parameters;

	memset(&handle, 0, sizeof(struct audio_module_handle));

	memcpy(&handle.name, test_inst_name, sizeof(*test_inst_name));
	handle.description = &mod_description;
	handle.use_tx_queue = true;
	handle.dest_count = 1;
	handle.thread.stack = mod_stack;
	handle.thread.stack_size = TEST_MOD_THREAD_STACK_SIZE;
	handle.thread.priority = TEST_MOD_THREAD_PRIORITY;
	handle.thread.data_slab = &data_slab;
	handle.thread.data_size = TEST_MOD_DATA_SIZE;
	handle.context = (struct audio_module_context *)&mod_context;

	/* Fake internal empty data FIFO success */
	data_fifo_init_fake.custom_fake = fake_data_fifo_init__succeeds;
	data_fifo_empty_fake.custom_fake = fake_data_fifo_empty__succeeds;

	if (fifo_rx_set) {
		data_fifo_deinit(&mod_fifo_rx);
		data_fifo_init(&mod_fifo_rx);
		handle.thread.msg_rx = &mod_fifo_rx;

		empty_call_count++;
	} else {
		handle.thread.msg_rx = NULL;
	}

	if (fifo_tx_set) {
		data_fifo_deinit(&mod_fifo_tx);
		data_fifo_init(&mod_fifo_tx);
		handle.thread.msg_tx = &mod_fifo_tx;

		empty_call_count++;
	} else {
		handle.thread.msg_tx = NULL;
	}

	thread_start(&handle);

	handle.state = test_state;

	ret = audio_module_close(&handle);

	zassert_equal(ret, 0, "Close function did not return successfully: ret %d", ret);
	zassert_equal(data_fifo_empty_fake.call_count, empty_call_count,
		      "Failed close, data FIFO empty called %d times",
		      data_fifo_empty_fake.call_count);
	zassert_mem_equal(&mod_description, &test_mod_description,
			  sizeof(struct audio_module_description),
			  "Failed close, modified the modules description");
	zassert_mem_equal(&test_mod_functions, test_fnct, sizeof(struct audio_module_functions),
			  "Failed close, modified the modules functions");
	zassert_mem_equal(&mod_parameters, &test_mod_parameters,
			  sizeof(struct audio_module_parameters),
			  "Failed close, modified the modules parameter settings");
	zassert_mem_equal(&mod_context, &test_mod_context, sizeof(struct mod_context),
			  "Failed close, modified the modules context");
}

/**
 * @brief Test the module's open.
 *
 * @param test_type       [in]  The type of the audio module.
 * @param fifo_rx_set     [in]  Flag to indicate if the audio modules RX data FIFO is to be
 *                              initialised.
 * @param fifo_tx_set     [in]  Flag to indicate if the audio modules TX data FIFO is to be
 *                              initialised.
 * @param test_slab       [in]  The module's data slab. This can be NULL.
 * @param test_data_size  [in]  The size of the data slab items. This can be 0..
 */
static void test_open(enum audio_module_type test_type, bool fifo_rx_set, bool fifo_tx_set,
		      struct k_mem_slab *test_slab, size_t test_data_size)
{
	int ret;
	char *test_inst_name = "TEST instance 1";

	/* Fake internal empty data FIFO success */
	data_fifo_init_fake.custom_fake = fake_data_fifo_init__succeeds;

	mod_description.functions = &ft_pop;

	test_context_set(&test_mod_context, &mod_config);

	if (fifo_rx_set) {
		data_fifo_deinit(&mod_fifo_rx);
		data_fifo_init(&mod_fifo_rx);
		mod_parameters.thread.msg_rx = &mod_fifo_rx;
	} else {
		mod_parameters.thread.msg_rx = NULL;
	}

	if (fifo_tx_set) {
		data_fifo_deinit(&mod_fifo_tx);
		data_fifo_init(&mod_fifo_tx);
		mod_parameters.thread.msg_tx = &mod_fifo_tx;
	} else {
		mod_parameters.thread.msg_tx = NULL;
	}

	mod_parameters.thread.data_slab = test_slab;
	mod_parameters.thread.data_size = test_data_size;

	mod_description.type = test_type;

	memset(&handle, 0, sizeof(struct audio_module_handle));

	handle.state = AUDIO_MODULE_STATE_UNDEFINED;

	ret = audio_module_open(&mod_parameters, (struct audio_module_configuration *)&mod_config,
				test_inst_name, (struct audio_module_context *)&mod_context,
				&handle);

	zassert_equal(ret, 0, "Open function did not return successfully: ret %d", ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Open state not AUDIO_MODULE_STATE_CONFIGURED (%d) rather %d",
		      AUDIO_MODULE_STATE_CONFIGURED, handle.state);
	zassert_mem_equal(&handle.name, test_inst_name, sizeof(*test_inst_name),
			  "Failed open, name should be %s, but is %s", test_inst_name, handle.name);
	zassert_mem_equal(handle.description, &mod_description,
			  sizeof(struct audio_module_description),
			  "Failed open, module descriptions differ");
	zassert_mem_equal(handle.description->functions, &ft_pop,
			  sizeof(struct audio_module_functions),
			  "Failed open, module function pointers differ");
	zassert_mem_equal(&handle.thread, &mod_parameters.thread,
			  sizeof(struct audio_module_thread_configuration),
			  "Failed open, module thread settings differ");
	zassert_mem_equal(handle.context, &test_mod_context, sizeof(struct mod_context),
			  "Failed open, module contexts differ");
	zassert_equal(handle.use_tx_queue, false, "Open failed use_tx_queue is not false: %d",
		      handle.use_tx_queue);
	zassert_equal(handle.dest_count, 0, "Open failed dest_count is not 0: %d",
		      handle.dest_count);

	k_thread_abort(handle.thread_id);
}

/**
 * @brief Test the audio modules connect and disconnect.
 *
 * @param connect     [in]  A flag to signal if this is for testing the connect or disconnect.
 * @param from_type   [in]  The from audio module type.
 * @param to_type     [in]  The to audio module type.
 * @param from_state  [in]  The state of the from audio module.
 * @param to_state    [in]  The state of the to audio module.
 */
static void test_connections(bool connect, enum audio_module_type from_type,
			     enum audio_module_type to_type, enum audio_module_state from_state,
			     enum audio_module_state to_state)
{
	int ret;
	int num_destinations;
	char *test_inst_from_name = "TEST instance from";
	char *test_inst_to_name = "TEST instance to";
	struct audio_module_handle handle_from;
	struct audio_module_handle handles_to[TEST_CONNECTIONS_NUM];

	test_from_description.type = from_type;
	test_to_description.type = to_type;

	for (int k = 0; k < TEST_CONNECTIONS_NUM; k++) {
		test_initialize_handle(&handles_to[k], &test_to_description, NULL, NULL);
		handles_to[k].state = to_state;
		snprintf(handles_to[k].name, CONFIG_AUDIO_MODULE_NAME_SIZE, "%s %d",
			 test_inst_to_name, k);
	}

	test_initialize_handle(&handle_from, &test_from_description, NULL, NULL);
	memcpy(&handle_from.name, test_inst_from_name, CONFIG_AUDIO_MODULE_NAME_SIZE);
	handle_from.state = from_state;
	sys_slist_init(&handle_from.handle_dest_list);
	k_mutex_init(&handle_from.dest_mutex);

	if (connect) {
		for (int k = 0; k < TEST_CONNECTIONS_NUM; k++) {
			ret = audio_module_connect(&handle_from, &handles_to[k], false);

			zassert_equal(ret, 0,
				      "Connect function did not return successfully: "
				      "ret %d (%d)",
				      ret, k);
			zassert_equal(handle_from.dest_count, k + 1,
				      "Destination count is not %d, %d", k + 1,
				      handle_from.dest_count);
		}

		test_list(&handle_from, &handles_to[0], TEST_CONNECTIONS_NUM, false);
	} else {
		num_destinations = test_initialize_connection_list(&handle_from, &handles_to[0],
								   TEST_CONNECTIONS_NUM, false);

		for (int k = 0; k < TEST_CONNECTIONS_NUM; k++) {
			ret = audio_module_disconnect(&handle_from, &handles_to[k], false);

			zassert_equal(ret, 0,
				      "Disconnect function did not return successfully: "
				      "ret %d (%d)",
				      ret, k);

			num_destinations--;

			zassert_equal(handle_from.dest_count, num_destinations,
				      "Destination count should be %d, but is %d", num_destinations,
				      handle_from.dest_count);

			test_list(&handle_from, &handles_to[k + 1], num_destinations, false);
		}
	}
}

/**
 * @brief Test the audio module's start and stop.
 *
 * @param start         [in]      A flag to signal if this is for testing ths start or stop.
 * @param test_fnct     [in]      Pointer to the module's function pointer structure.
 * @param test_state    [in]      The state of the audio module.
 * @param return_state  [in]      The state of the audio module after a start or stop.
 * @param ret_expected  [in/out]  Expected return value from the start or stop.
 */
static void test_start_stop(bool start, const struct audio_module_functions *test_fnct,
			    enum audio_module_state test_state,
			    enum audio_module_state return_state, int ret_expected)
{
	int ret;

	mod_description.functions = test_fnct;
	test_initialize_handle(&handle, &mod_description, &mod_context, NULL);
	handle.state = test_state;

	if (start) {
		ret = audio_module_start(&handle);
	} else {
		ret = audio_module_stop(&handle);
	}

	zassert_equal(ret, ret_expected, "%s function did not return as expected (%d): ret %d",
		      (start ? "Start" : "Stop"), ret_expected, ret);
	zassert_equal(handle.state, return_state, "%s state failed %d rather %d",
		      (start ? "Start" : "Stop"), return_state, handle.state);
}

ZTEST(suite_audio_module_functional, test_number_channels_calculate_fnct)
{
	int ret;
	struct audio_data audio_data;
	uint8_t number_channels;

	audio_data.meta.locations = 0x00000000;
	ret = audio_module_number_channels_calculate(audio_data.meta.locations, &number_channels);
	zassert_equal(ret, 0,
		      "Calculate number of channels function did not return successfully: ret %d",
		      ret);
	zassert_equal(number_channels, 0,
		      "Calculate number of channels function did not return 0 (%d) channel count",
		      number_channels);

	audio_data.meta.locations = 0x00000001;
	ret = audio_module_number_channels_calculate(audio_data.meta.locations, &number_channels);
	zassert_equal(ret, 0,
		      "Calculate number of channels function did not return successfully: ret %d",
		      ret);
	zassert_equal(number_channels, 1,
		      "Calculate number of channels function did not return 1 (%d) channel count",
		      number_channels);

	audio_data.meta.locations = 0x80000000;
	ret = audio_module_number_channels_calculate(audio_data.meta.locations, &number_channels);
	zassert_equal(ret, 0,
		      "Calculate number of channels function did not return successfully: ret %d",
		      ret);
	zassert_equal(number_channels, 1,
		      "Calculate number of channels function did not return 1 (%d) channel count",
		      number_channels);

	audio_data.meta.locations = 0xFFFFFFFF;
	ret = audio_module_number_channels_calculate(audio_data.meta.locations, &number_channels);
	zassert_equal(ret, 0,
		      "Calculate number of channels function did not return successfully: ret %d",
		      ret);
	zassert_equal(number_channels, 32,
		      "Calculate number of channels function did not return 32 (%d) channel count",
		      number_channels);

	audio_data.meta.locations = 0x55555555;
	ret = audio_module_number_channels_calculate(audio_data.meta.locations, &number_channels);
	zassert_equal(ret, 0,
		      "Calculate number of channels function did not return successfully: ret %d",
		      ret);
	zassert_equal(number_channels, 16,
		      "Calculate number of channels function did not return 16 (%d) channel count",
		      number_channels);
}

ZTEST(suite_audio_module_functional, test_state_get_fnct)
{
	int ret;
	struct audio_module_handle handle = {0};
	enum audio_module_state state;

	handle.state = AUDIO_MODULE_STATE_CONFIGURED;
	ret = audio_module_state_get(&handle, &state);
	zassert_equal(ret, 0, "Get state function did not return successfully: ret %d", ret);
	zassert_equal(
		state, AUDIO_MODULE_STATE_CONFIGURED,
		"Get state function did not return AUDIO_MODULE_STATE_CONFIGURED (%d) rather %d",
		AUDIO_MODULE_STATE_CONFIGURED, state);

	handle.state = AUDIO_MODULE_STATE_RUNNING;
	ret = audio_module_state_get(&handle, &state);
	zassert_equal(ret, 0, "Get state function did not return successfully: ret %d", ret);
	zassert_equal(state, AUDIO_MODULE_STATE_RUNNING,
		      "Get state function did not return AUDIO_MODULE_STATE_RUNNING (%d) rather %d",
		      AUDIO_MODULE_STATE_RUNNING, state);

	handle.state = AUDIO_MODULE_STATE_STOPPED;
	ret = audio_module_state_get(&handle, &state);
	zassert_equal(ret, 0, "Get state function did not return successfully: ret %d", ret);
	zassert_equal(state, AUDIO_MODULE_STATE_STOPPED,
		      "Get state function did not return AUDIO_MODULE_STATE_STOPPED (%d) rather %d",
		      AUDIO_MODULE_STATE_STOPPED, state);
}

ZTEST(suite_audio_module_functional, test_names_get_fnct)
{
	int ret;
	char *base_name;
	char instance_name[CONFIG_AUDIO_MODULE_NAME_SIZE] = {0};
	char *test_base_name_empty = "";
	char *test_base_name_long =
		"Test base name that is longer than the size of CONFIG_AUDIO_MODULE_NAME_SIZE";

	test_handle_set(&handle, AUDIO_MODULE_STATE_CONFIGURED, test_base_name_empty,
			&mod_description);
	memset(&handle.name[0], 0, CONFIG_AUDIO_MODULE_NAME_SIZE);

	ret = audio_module_names_get(&handle, &base_name, &instance_name[0]);
	zassert_equal(ret, 0, "Get names function did not return successfully: ret %d", ret);
	zassert_equal_ptr(base_name, handle.description->name, "Failed to get the base name: %s",
			  handle.description->name);
	zassert_mem_equal(instance_name, handle.name, CONFIG_AUDIO_MODULE_NAME_SIZE,
			  "Failed to get the instance name: %s", instance_name);

	test_handle_set(&handle, AUDIO_MODULE_STATE_CONFIGURED, (char *)test_base_name,
			&mod_description);

	memcpy(&handle.name, "Test instance name", sizeof("Test instance name"));
	ret = audio_module_names_get(&handle, &base_name, &instance_name[0]);
	zassert_equal(ret, 0, "Get names function did not return successfully: ret %d", ret);
	zassert_equal_ptr(base_name, handle.description->name,
			  "Failed to get the base name in configured state: %s",
			  handle.description->name);
	zassert_mem_equal(instance_name, handle.name, CONFIG_AUDIO_MODULE_NAME_SIZE,
			  "Failed to get the instance name in configured state: %s", instance_name);

	test_handle_set(&handle, AUDIO_MODULE_STATE_RUNNING, (char *)test_base_name,
			&mod_description);
	memcpy(&handle.name, "Instance name run", sizeof("Instance name run"));

	ret = audio_module_names_get(&handle, &base_name, &instance_name[0]);
	zassert_equal(ret, 0, "Get names function did not return successfully: ret %d", ret);
	zassert_equal_ptr(base_name, handle.description->name,
			  "Failed to correctly get the base name in running state: %s",
			  handle.description->name);
	zassert_mem_equal(instance_name, handle.name, CONFIG_AUDIO_MODULE_NAME_SIZE,
			  "Failed to get the instance name in running state: %s", instance_name);

	test_handle_set(&handle, AUDIO_MODULE_STATE_STOPPED, (char *)test_base_name,
			&mod_description);
	memcpy(&handle.name, "Instance name stop", sizeof("Instance name stop"));

	ret = audio_module_names_get(&handle, &base_name, &instance_name[0]);
	zassert_equal(ret, 0, "Get names function did not return successfully: ret %d", ret);
	zassert_equal_ptr(base_name, handle.description->name,
			  "Failed to correctly get the base name in stopped state: %s",
			  handle.description->name);
	zassert_mem_equal(instance_name, handle.name, CONFIG_AUDIO_MODULE_NAME_SIZE,
			  "Failed to get the instance name in stopped state: %s", instance_name);

	test_handle_set(&handle, AUDIO_MODULE_STATE_CONFIGURED, test_base_name_long,
			&mod_description);
	memcpy(&handle.name, "Test instance name", sizeof("Test instance name"));

	ret = audio_module_names_get(&handle, &base_name, &instance_name[0]);
	zassert_equal(ret, 0, "Get names function did not return successfully: ret %d", ret);
	zassert_equal_ptr(base_name, handle.description->name,
			  "Failed to get the base name in configured state: %s",
			  handle.description->name);
	zassert_mem_equal(instance_name, handle.name, CONFIG_AUDIO_MODULE_NAME_SIZE,
			  "Failed to get the instance name in configured state: %s", instance_name);
}

ZTEST(suite_audio_module_functional, test_reconfigure_fnct)
{
	int ret;

	mod_description.functions = &ft_pop;
	test_initialize_handle(&handle, &mod_description, &mod_context, &mod_config);
	handle_context = (struct mod_context *)handle.context;
	handle.state = AUDIO_MODULE_STATE_CONFIGURED;

	ret = audio_module_reconfigure(&handle, (struct audio_module_configuration *)&mod_config);
	zassert_equal(ret, 0, "Reconfigure function did not return successfully: ret %d", ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Reconfigure state not AUDIO_MODULE_STATE_CONFIGURED (%d) rather %d",
		      AUDIO_MODULE_STATE_CONFIGURED, handle.state);
	zassert_mem_equal(&mod_config, &handle_context->config, sizeof(struct mod_config),
			  "Failed reconfigure");

	test_initialize_handle(&handle, &mod_description, &mod_context, &mod_config);
	handle_context = (struct mod_context *)handle.context;
	handle.state = AUDIO_MODULE_STATE_STOPPED;

	ret = audio_module_reconfigure(&handle, (struct audio_module_configuration *)&mod_config);
	zassert_equal(ret, 0, "Reconfigure function did not return successfully: ret %d", ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Reconfigure state not AUDIO_MODULE_STATE_CONFIGURED (%d) rather %d",
		      AUDIO_MODULE_STATE_CONFIGURED, handle.state);
	zassert_mem_equal(&mod_config, &handle_context->config, sizeof(struct mod_config),
			  "Failed reconfigure");
}

ZTEST(suite_audio_module_functional, test_configuration_get_fnct)
{
	int ret;

	mod_description.functions = &ft_pop;
	test_initialize_handle(&handle, &mod_description, &mod_context, &mod_config);
	handle_context = (struct mod_context *)handle.context;
	handle.state = AUDIO_MODULE_STATE_CONFIGURED;

	ret = audio_module_configuration_get(&handle,
					     (struct audio_module_configuration *)&mod_config);
	zassert_equal(ret, 0, "Reconfigure function did not return successfully: ret %d", ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_CONFIGURED,
		      "Reconfigure state not AUDIO_MODULE_STATE_CONFIGURED (%d) rather %d",
		      AUDIO_MODULE_STATE_CONFIGURED, handle.state);
	zassert_mem_equal(&mod_config, &handle_context->config, sizeof(struct mod_config),
			  "Failed reconfigure");

	test_initialize_handle(&handle, &mod_description, &mod_context, &mod_config);
	handle_context = (struct mod_context *)handle.context;
	handle.state = AUDIO_MODULE_STATE_STOPPED;

	ret = audio_module_configuration_get(&handle,
					     (struct audio_module_configuration *)&mod_config);
	zassert_equal(ret, 0, "Reconfigure function did not return successfully: ret %d", ret);
	zassert_equal(handle.state, AUDIO_MODULE_STATE_STOPPED,
		      "Reconfigure state not AUDIO_MODULE_STATE_STOPPED (%d) rather %d",
		      AUDIO_MODULE_STATE_STOPPED, handle.state);
	zassert_mem_equal(&mod_config, &handle_context->config, sizeof(struct mod_config),
			  "Failed reconfigure");
}

ZTEST(suite_audio_module_functional, test_stop_fnct_null)
{
	test_start_stop(false, &ft_null, AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_STOPPED,
			-EALREADY);
	test_start_stop(false, &ft_null, AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_STOPPED, 0);
	test_start_stop(false, &ft_null, AUDIO_MODULE_STATE_CONFIGURED,
			AUDIO_MODULE_STATE_CONFIGURED, -EALREADY);
}

ZTEST(suite_audio_module_functional, test_stop_fnct)
{
	test_start_stop(false, &ft_pop, AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_STOPPED,
			-EALREADY);
	test_start_stop(false, &ft_pop, AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_STOPPED, 0);
	test_start_stop(false, &ft_pop, AUDIO_MODULE_STATE_CONFIGURED,
			AUDIO_MODULE_STATE_CONFIGURED, -EALREADY);
}

ZTEST(suite_audio_module_functional, test_start_fnct_null)
{
	test_start_stop(true, &ft_null, AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_RUNNING,
			-EALREADY);
	test_start_stop(true, &ft_null, AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_RUNNING, 0);
	test_start_stop(true, &ft_null, AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_RUNNING,
			0);
}

ZTEST(suite_audio_module_functional, test_start_fnct)
{
	test_start_stop(true, &ft_pop, AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_RUNNING,
			-EALREADY);
	test_start_stop(true, &ft_pop, AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_RUNNING, 0);
	test_start_stop(true, &ft_pop, AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_RUNNING,
			0);
}

ZTEST(suite_audio_module_functional, test_connect_disconnect_fnct)
{
	bool connect = false;

	do {
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_INPUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_OUTPUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_CONFIGURED, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_RUNNING, AUDIO_MODULE_STATE_STOPPED);

		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_CONFIGURED);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_RUNNING);
		test_connections(connect, AUDIO_MODULE_TYPE_IN_OUT, AUDIO_MODULE_TYPE_IN_OUT,
				 AUDIO_MODULE_STATE_STOPPED, AUDIO_MODULE_STATE_STOPPED);

		if (connect) {
			break;
		}

		connect = true;
	} while (1);
}

ZTEST(suite_audio_module_functional, test_close_null_fnct)
{
	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, true, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, false, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, true, false, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, false, false, AUDIO_MODULE_STATE_CONFIGURED);

	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, true, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, false, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, true, false, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, false, false, AUDIO_MODULE_STATE_CONFIGURED);

	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, true, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, false, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, true, false, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, false, false, AUDIO_MODULE_STATE_CONFIGURED);

	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, true, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, false, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, true, false, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_INPUT, false, false, AUDIO_MODULE_STATE_STOPPED);

	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, true, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, false, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, true, false, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_OUTPUT, false, false, AUDIO_MODULE_STATE_STOPPED);

	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, true, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, false, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, true, false, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_null, AUDIO_MODULE_TYPE_IN_OUT, false, false, AUDIO_MODULE_STATE_STOPPED);
}

ZTEST(suite_audio_module_functional, test_close_fnct)
{
	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, true, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, false, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, true, false, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, false, false, AUDIO_MODULE_STATE_CONFIGURED);

	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, true, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, false, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, true, false, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, false, false, AUDIO_MODULE_STATE_CONFIGURED);

	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, true, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, false, true, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, true, false, AUDIO_MODULE_STATE_CONFIGURED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, false, false, AUDIO_MODULE_STATE_CONFIGURED);

	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, true, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, false, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, true, false, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_INPUT, false, false, AUDIO_MODULE_STATE_STOPPED);

	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, true, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, false, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, true, false, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_OUTPUT, false, false, AUDIO_MODULE_STATE_STOPPED);

	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, true, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, false, true, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, true, false, AUDIO_MODULE_STATE_STOPPED);
	test_close(&ft_pop, AUDIO_MODULE_TYPE_IN_OUT, false, false, AUDIO_MODULE_STATE_STOPPED);
}

ZTEST(suite_audio_module_functional, test_open_fnct)
{
	test_open(AUDIO_MODULE_TYPE_INPUT, true, true, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_INPUT, false, true, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_INPUT, true, false, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_INPUT, false, false, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_INPUT, true, true, NULL, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_INPUT, true, true, &data_slab, 0);

	test_open(AUDIO_MODULE_TYPE_OUTPUT, true, true, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_OUTPUT, false, true, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_OUTPUT, true, false, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_OUTPUT, false, false, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_OUTPUT, true, true, NULL, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_OUTPUT, true, true, &data_slab, 0);

	test_open(AUDIO_MODULE_TYPE_IN_OUT, true, true, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_IN_OUT, false, true, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_IN_OUT, true, false, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_IN_OUT, false, false, &data_slab, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_OUTPUT, true, true, NULL, TEST_MOD_DATA_SIZE);
	test_open(AUDIO_MODULE_TYPE_OUTPUT, true, true, &data_slab, 0);
}

ZTEST(suite_audio_module_functional, test_data_tx_fnct)
{
	int ret;
	char *test_inst_name = "TEST instance 1";
	size_t size;
	char test_data[TEST_MOD_DATA_SIZE];
	struct audio_data audio_data = {0};
	struct audio_module_message *msg_rx;

	test_context_set(&mod_context, &mod_config);

	/* Fake internal empty data FIFO success */
	data_fifo_init_fake.custom_fake = fake_data_fifo_init__succeeds;
	data_fifo_pointer_first_vacant_get_fake.custom_fake =
		fake_data_fifo_pointer_first_vacant_get__succeeds;
	data_fifo_block_lock_fake.custom_fake = fake_data_fifo_block_lock__succeeds;
	data_fifo_pointer_last_filled_get_fake.custom_fake =
		fake_data_fifo_pointer_last_filled_get__succeeds;
	data_fifo_block_free_fake.custom_fake = fake_data_fifo_block_free__succeeds;

	data_fifo_deinit(&mod_fifo_rx);

	data_fifo_init(&mod_fifo_rx);

	memcpy(&handle.name, test_inst_name, sizeof(*test_inst_name));
	handle.description = &mod_description;
	handle.thread.msg_rx = &mod_fifo_rx;
	handle.thread.msg_tx = NULL;
	handle.thread.data_slab = &data_slab;
	handle.thread.data_size = TEST_MOD_DATA_SIZE;
	handle.state = AUDIO_MODULE_STATE_RUNNING;
	handle.context = (struct audio_module_context *)&mod_context;

	for (int i = 0; i < TEST_MOD_DATA_SIZE; i++) {
		test_data[i] = TEST_MOD_DATA_SIZE - i;
	}

	audio_data.data = &test_data[0];
	audio_data.data_size = TEST_MOD_DATA_SIZE;

	ret = audio_module_data_tx(&handle, &audio_data, NULL);
	zassert_equal(ret, 0, "Data TX function did not return successfully: ret %d", ret);

	ret = data_fifo_pointer_last_filled_get(&mod_fifo_rx, (void **)&msg_rx, &size, K_NO_WAIT);
	zassert_equal(ret, 0, "Data TX function did not return 0: ret %d", 0, ret);
	zassert_mem_equal(msg_rx->audio_data.data, &test_data[0], TEST_MOD_DATA_SIZE,
			  "Failed open, module contexts differ");
	zassert_equal(msg_rx->audio_data.data_size, TEST_MOD_DATA_SIZE,
		      "Failed Data TX function, data sizes differs");
	zassert_equal(data_fifo_pointer_first_vacant_get_fake.call_count, 1,
		      "Data TX failed to get item, data FIFO get called %d times",
		      data_fifo_pointer_first_vacant_get_fake.call_count);
	zassert_equal(data_fifo_block_lock_fake.call_count, 1,
		      "Failed to send item, data FIFO send called %d times",
		      data_fifo_pointer_first_vacant_get_fake.call_count);
}

ZTEST(suite_audio_module_functional, test_data_rx_fnct)
{
	int ret;
	char *test_inst_name = "TEST instance 1";
	char test_data[TEST_MOD_DATA_SIZE];
	char data[TEST_MOD_DATA_SIZE] = {0};
	struct audio_data audio_data_in;
	struct audio_data audio_data_out = {0};
	struct audio_module_message *data_msg_tx;

	test_context_set(&mod_context, &mod_config);

	/* Fake internal empty data FIFO success */
	data_fifo_init_fake.custom_fake = fake_data_fifo_init__succeeds;
	data_fifo_pointer_first_vacant_get_fake.custom_fake =
		fake_data_fifo_pointer_first_vacant_get__succeeds;
	data_fifo_block_lock_fake.custom_fake = fake_data_fifo_block_lock__succeeds;
	data_fifo_pointer_last_filled_get_fake.custom_fake =
		fake_data_fifo_pointer_last_filled_get__succeeds;
	data_fifo_block_free_fake.custom_fake = fake_data_fifo_block_free__succeeds;

	data_fifo_deinit(&mod_fifo_tx);

	data_fifo_init(&mod_fifo_tx);

	memcpy(&handle.name, test_inst_name, sizeof(*test_inst_name));
	handle.description = &mod_description;
	handle.thread.msg_rx = NULL;
	handle.thread.msg_tx = &mod_fifo_tx;
	handle.thread.data_slab = &data_slab;
	handle.thread.data_size = TEST_MOD_DATA_SIZE;
	handle.state = AUDIO_MODULE_STATE_RUNNING;
	handle.context = (struct audio_module_context *)&mod_context;

	ret = data_fifo_pointer_first_vacant_get(handle.thread.msg_tx, (void **)&data_msg_tx,
						 K_NO_WAIT);
	/* fill data */
	for (int i = 0; i < TEST_MOD_DATA_SIZE; i++) {
		test_data[i] = TEST_MOD_DATA_SIZE - i;
	}

	audio_data_in.data = &test_data[0];
	audio_data_in.data_size = TEST_MOD_DATA_SIZE;

	memcpy(&data_msg_tx->audio_data, &audio_data_in, sizeof(struct audio_data));
	data_msg_tx->tx_handle = NULL;
	data_msg_tx->response_cb = NULL;

	ret = data_fifo_block_lock(handle.thread.msg_tx, (void **)&data_msg_tx,
				   sizeof(struct audio_module_message));

	audio_data_out.data = &data[0];
	audio_data_out.data_size = TEST_MOD_DATA_SIZE;

	ret = audio_module_data_rx(&handle, &audio_data_out, K_NO_WAIT);
	zassert_equal(ret, 0, "Data RX function did not return successfully: ret %d", ret);
	zassert_mem_equal(&test_data[0], audio_data_out.data, TEST_MOD_DATA_SIZE,
			  "Failed Data RX function, data differs");
	zassert_equal(audio_data_in.data_size, audio_data_out.data_size,
		      "Failed Data RX function, data sizes differs");
	zassert_equal(data_fifo_pointer_last_filled_get_fake.call_count, 1,
		      "Data RX function failed to get item, data FIFO get called %d times",
		      data_fifo_pointer_last_filled_get_fake.call_count);
	zassert_equal(data_fifo_block_free_fake.call_count, 1,
		      "Data RX function failed to free item, data FIFO free called %d times",
		      data_fifo_block_free_fake.call_count);
}
