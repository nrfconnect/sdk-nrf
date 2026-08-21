/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_rate_control.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rate_control, CONFIG_MODULE_RATE_CONTROL_LOG_LEVEL);

static bool valid_entry(struct audio_rate_control_context const *const context, int *ret)
{
	*ret = 0;

	if (context == NULL) {
		LOG_ERR("Rate control context pointer is NULL");
		*ret = -EINVAL;
		return false;
	}

	if (context->state == AUDIO_RATE_CONTROL_STATE_UNINITIALIZED) {
		LOG_ERR("Rate control uninitialized");
		*ret = -EINVAL;
		return false;
	}

	return true;
}

int audio_rate_control_configuration_set(
	struct audio_rate_control_context *const context,
	struct audio_rate_control_configuration const *const configuration)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.configuration_set != NULL) {
		ret = context->cb.configuration_set(context->imp_ctx, configuration);
		if (ret != 0) {
			LOG_ERR("Failed to configure the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No configuration set callback");
	}

	return 0;
}

int audio_rate_control_configuration_get(struct audio_rate_control_context const *const context,
					 struct audio_rate_control_configuration *configuration)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.configuration_get != NULL) {
		ret = context->cb.configuration_get(context->imp_ctx, configuration);
		if (ret != 0) {
			LOG_ERR("Failed to retrieve the configuration for the rate control "
				"implementation: %d",
				ret);
			return ret;
		}
	} else {
		LOG_DBG("No configuration get callback");
	}

	return 0;
}

int audio_rate_control_update(struct audio_rate_control_context *const context, void *const error)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (error == NULL) {
		LOG_ERR("Rate control error pointer NULL");
		return -EINVAL;
	}

	if (context->cb.update != NULL) {
		ret = context->cb.update(context->imp_ctx, error);
		if (ret != 0) {
			LOG_ERR("Failed to update the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_ERR("No update error callback");
		return -EINVAL;
	}

	return 0;
}

int audio_rate_control_reset(struct audio_rate_control_context *const context)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.reset != NULL) {
		ret = context->cb.reset(context->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to reset the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No reset callback");
	}

	return 0;
}

int audio_rate_control_uninit(struct audio_rate_control_context *const context)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.uninitialize != NULL) {
		ret = context->cb.uninitialize(context->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to uninitialize the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No uninitialize callback");
	}

	context->imp_ctx = NULL;
	context->state = AUDIO_RATE_CONTROL_STATE_UNINITIALIZED;
	memset(&context->cb, 0, sizeof(struct audio_rate_control_ops));

	return 0;
}

int audio_rate_control_init(struct audio_rate_control_context *const context,
			    struct audio_rate_control_implementation_ctx *const implementation_ctx,
			    struct audio_rate_control_ops const *const implementation_cb)
{
	int ret;

	if ((context == NULL) || (implementation_ctx == NULL) || (implementation_cb == NULL)) {
		LOG_ERR("Rate control call parameter error");
		return -EINVAL;
	}

	if (implementation_cb->update == NULL) {
		LOG_ERR("Rate control mandatory callback is not configured");
		return -EINVAL;
	}

	memset(context, 0, sizeof(struct audio_rate_control_context));

	context->imp_ctx = implementation_ctx;

	memcpy(&context->cb, implementation_cb, sizeof(struct audio_rate_control_ops));

	if (context->cb.initialize != NULL) {
		ret = context->cb.initialize(context->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to initialize the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No initialize callback");
	}

	context->state = AUDIO_RATE_CONTROL_STATE_INITIALIZED;

	return 0;
}
