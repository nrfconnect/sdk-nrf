/*
 * Copyright(c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "audio_module_template.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <errno.h>
#include "audio_defines.h"
#include "audio_module.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_module_template, CONFIG_AUDIO_MODULE_TEMPLATE_LOG_LEVEL);

static int audio_module_template_open(struct audio_module_handle_private *handle,
				      struct audio_module_configuration const *const configuration)

{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct audio_module_template_context *ctx =
		(struct audio_module_template_context *)hdl->context;
	struct audio_module_template_configuration *config =
		(struct audio_module_template_configuration *)configuration;

	/* Perform any other functions required to open the module. */

	/* For example: Clear the module's context.
	 *              Save the initial configuration to the module context.
	 */
	memset(ctx, 0, sizeof(struct audio_module_template_context));
	memcpy(&ctx->config, config, sizeof(struct audio_module_template_configuration));

	LOG_DBG("Open %s module", hdl->name);

	return 0;
}

static int audio_module_template_close(struct audio_module_handle_private *handle)
{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct audio_module_template_context *ctx =
		(struct audio_module_template_context *)hdl->context;

	/* Perform any other functions required to close the module. */

	/* For example: Clear the context data */
	memset(ctx, 0, sizeof(struct audio_module_template_context));

	LOG_DBG("Close %s module", hdl->name);

	return 0;
}

static int audio_module_template_configuration_set(
	struct audio_module_handle_private *handle,
	struct audio_module_configuration const *const configuration)
{
	struct audio_module_template_configuration *config =
		(struct audio_module_template_configuration *)configuration;
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct audio_module_template_context *ctx =
		(struct audio_module_template_context *)hdl->context;

	/* Perform any other functions to configure the module. */

	/* For example: Copy the configuration into the context. */
	memcpy(&ctx->config, config, sizeof(struct audio_module_template_configuration));

	LOG_DBG("Set the configuration for %s module: rate = %d  depth = %d  string = %s",
		hdl->name, ctx->config.rate, ctx->config.depth, ctx->config.some_text);

	return 0;
}

static int
audio_module_template_configuration_get(struct audio_module_handle_private const *const handle,
					struct audio_module_configuration *configuration)
{
	struct audio_module_template_configuration *config =
		(struct audio_module_template_configuration *)configuration;
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct audio_module_template_context *ctx =
		(struct audio_module_template_context *)hdl->context;

	/* Perform any other functions to extract the configuration of the module. */

	/* For example: Copy the configuration from the context into the output configuration. */
	memcpy(config, &ctx->config, sizeof(struct audio_module_template_configuration));

	LOG_DBG("Get the configuration for %s module: rate = %d  depth = %d  string = %s",
		hdl->name, config->rate, config->depth, config->some_text);

	return 0;
}

static int audio_module_template_start(struct audio_module_handle_private *handle)
{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;

	/* Perform any other functions to start the module. */

	LOG_DBG("Start the %s module", hdl->name);

	return 0;
}

static int audio_module_template_stop(struct audio_module_handle_private *handle)
{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;

	/* Perform any other functions to stop the module. */

	LOG_DBG("Stop the %s module", hdl->name);

	return 0;
}

static int audio_module_template_data_process(struct audio_module_handle_private *handle,
					      struct audio_data const *const audio_data_in,
					      struct audio_data *audio_data_out)
{
	struct audio_module_handle *hdl = (struct audio_module_handle *)handle;
	struct audio_module_template_context *ctx =
		(struct audio_module_template_context *)hdl->context;

	/* Perform any other functions to process the data within the module. */

	/* For example: Copy the last input to the output and save the
	 *              AUDIO_MODULE_TEMPLATE_LAST_BYTES bytes to the modules context.
	 */
	{
		uint8_t *data_in, *data_out;
		size_t size = audio_data_in->data_size < audio_data_out->data_size
				      ? audio_data_out->data_size
				      : audio_data_in->data_size;

		data_in = (uint8_t *)audio_data_in->data;
		data_out = (uint8_t *)audio_data_out->data;

		for (size_t i = 0; i < size; i++) {
			*data_out++ = *data_in++;
		}

		size = audio_data_in->data_size < AUDIO_MODULE_TEMPLATE_LAST_BYTES
			       ? audio_data_in->data_size
			       : AUDIO_MODULE_TEMPLATE_LAST_BYTES;

		data_in = (uint8_t *)&audio_data_in->data;
		data_in += audio_data_in->data_size - size;
		data_out = (uint8_t *)&ctx->audio_module_template_data[0];

		for (size_t i = 0; i < size; i++) {
			*data_out++ = *data_in++;
		}
	}

	LOG_DBG("Process the input audio data into the output audio data item for %s module",
		hdl->name);

	return 0;
}

/**
 * @brief Table of the dummy module functions.
 */
const struct audio_module_functions audio_module_template_functions = {
	/**
	 * @brief  Function to an open the dummy module.
	 */
	.open = audio_module_template_open,

	/**
	 * @brief  Function to close the dummy module.
	 */
	.close = audio_module_template_close,

	/**
	 * @brief  Function to set the configuration of the dummy module.
	 */
	.configuration_set = audio_module_template_configuration_set,

	/**
	 * @brief  Function to get the configuration of the dummy module.
	 */
	.configuration_get = audio_module_template_configuration_get,

	/**
	 * @brief Start a module processing data.
	 */
	.start = audio_module_template_start,

	/**
	 * @brief Pause a module processing data.
	 */
	.stop = audio_module_template_stop,

	/**
	 * @brief The core data processing function in the dummy module.
	 */
	.data_process = audio_module_template_data_process,
};

/**
 * @brief The set-up description for the LC3 decoder.
 */
struct audio_module_description audio_module_template_dept = {
	.name = "Audio Module Temp",
	.type = AUDIO_MODULE_TYPE_IN_OUT,
	.functions = &audio_module_template_functions};

/**
 * @brief A private pointer to the LC3 decoder set-up parameters.
 */
struct audio_module_description *audio_module_template_description = &audio_module_template_dept;
