/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _COMMON_AUDIO_MODULE_TEST_H_
#define _COMMON_AUDIO_MODULE_TEST_H_

#include "audio_module/audio_module.h"

#define FAKE_FIFO_MSG_QUEUE_SIZE      (4)
#define FAKE_FIFO_MSG_QUEUE_DATA_SIZE (sizeof(struct audio_module_message))
#define FAKE_FIFO_NUM		      (4)

#define TEST_MOD_THREAD_STACK_SIZE (2048)
#define TEST_MOD_THREAD_PRIORITY   (4)
#define TEST_CONNECTIONS_NUM	   (5)
#define TEST_MODULES_NUM	   (TEST_CONNECTIONS_NUM - 1)
#define TEST_MOD_DATA_SIZE	   (40)

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

extern const char *TEST_INSTANCE_NAME;
extern const char *TEST_STRING;
extern const uint32_t TEST_UINT32;

/**
 * @brief Test function to set a module's context.
 *
 * @param ctx[out]    The module context to set
 * @param config[in]  Pointer to the module's configuration
 *
 * @return 0 if successful, error otherwise
 */
void test_context_set(struct mod_context *ctx, struct mod_config const *const config);

/**
 * @brief Test function to open a module.
 *
 * @param handle         [in/out]  The handle to the module instance.
 * @param configuration  [in]      Pointer to the module's configuration.
 *
 * @return 0 if successful, error otherwise.
 */
int test_open_function(struct audio_module_handle_private *handle,
		       struct audio_module_configuration const *const configuration);

/**
 * @brief Test function to close a module.
 *
 * @param handle  [in/out]  The handle to the module instance.
 *
 * @return 0 if successful, error otherwise.
 */
int test_close_function(struct audio_module_handle_private *handle);

/**
 * @brief Test function to configure a module.
 *
 * @param handle         [in/out]  The handle to the module instance.
 * @param configuration  [in]      Pointer to the module's configuration to set.
 *
 * @return 0 if successful, error otherwise.
 */
int test_config_set_function(struct audio_module_handle_private *handle,
			     struct audio_module_configuration const *const configuration);

/**
 * @brief Test function to get the configuration of a module.
 *
 * @param handle         [in]   The handle to the module instance.
 * @param configuration  [out]  Pointer to the module's current configuration.
 *
 * @return 0 if successful, error otherwise.
 */
int test_config_get_function(struct audio_module_handle_private const *const handle,
			     struct audio_module_configuration *configuration);

/**
 * @brief Test start function of a module.
 *
 * @param handle  [in/out]  The handle for the module to be started.
 *
 * @return 0 if successful, error otherwise.
 */
int test_start_function(struct audio_module_handle_private *handle);

/**
 * @brief Test stop function of a module.
 *
 * @param handle  [in/out]  The handle for the module to be stopped.
 *
 * @return 0 if successful, error otherwise.
 */
int test_stop_function(struct audio_module_handle_private *handle);

/**
 * @brief Test process data function of a module.
 *
 * @param handle         [in/out]  The handle to the module instance.
 * @param audio_data_rx  [in]      Pointer to the input audio data or NULL for an input module.
 * @param audio_data_tx  [out]     Pointer to the output audio data or NULL for an output module.
 *
 * @return 0 if successful, error otherwise.
 */
int test_data_process_function(struct audio_module_handle_private *handle,
			       struct audio_data const *const audio_data_rx,
			       struct audio_data *audio_data_tx);

#endif /* _COMMON_AUDIO_MODULE_TEST_H_ */
