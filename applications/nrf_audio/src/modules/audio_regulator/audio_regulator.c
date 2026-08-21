/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_regulator.h"

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(regulator, CONFIG_MODULE_REGULATOR_LOG_LEVEL);

static bool valid_entry(struct audio_regulator_context const *const context, int *ret)
{
	*ret = 0;

	if (context == NULL) {
		LOG_ERR("Regulator context pointer is NULL");
		*ret = -EINVAL;
		return false;
	}

	if (context->state == AUDIO_REGULATOR_STATE_UNINITIALIZED) {
		LOG_ERR("Regulator uninitialized");
		*ret = -EINVAL;
		return false;
	}

	return true;
}

int audio_regulator_configuration_set(
	struct audio_regulator_context *const context,
	struct audio_regulator_configuration const *const configuration)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.configuration_set != NULL) {
		ret = context->cb.configuration_set(context->imp_ctx, configuration);
		if (ret != 0) {
			LOG_ERR("Failed to configure the regulator implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No configuration set callback");
	}

	return 0;
}

int audio_regulator_configuration_get(struct audio_regulator_context const *const context,
				      struct audio_regulator_configuration *configuration)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.configuration_get != NULL) {
		ret = context->cb.configuration_get(context->imp_ctx, configuration);
		if (ret != 0) {
			LOG_ERR("Failed to retrieve the configuration for the regulator "
				"implementation: %d",
				ret);
			return ret;
		}
	} else {
		LOG_DBG("No configuration get callback");
	}

	return 0;
}

int audio_regulator_update_error(struct audio_regulator_context const *const context,
				 void *const pts_ref, void *const pts, void *const error)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if ((pts_ref == NULL) || (pts == NULL) || (error == NULL)) {
		LOG_ERR("Update error failed due to invalid parameter: %d", ret);
		return -EINVAL;
	}

	if (context->cb.update != NULL) {
		ret = context->cb.update(context->imp_ctx, pts_ref, pts, error);
		if (ret != 0) {
			LOG_ERR("Failed to update the error for the regulator implementation: %d",
				ret);
			return ret;
		}
	} else {
		LOG_ERR("No update error callback");
		return -EINVAL;
	}

	return 0;
}

int audio_regulator_error_get(struct audio_regulator_context const *const context,
			      void *const error)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.error_get != NULL) {
		ret = context->cb.error_get(context->imp_ctx, error);
		if (ret != 0) {
			LOG_ERR("Failed to retrieve the last error calculated for "
				"the regulator implementation: %d",
				ret);
			return ret;
		}
	} else {
		LOG_ERR("No last error get callback");
		return -EINVAL;
	}

	return 0;
}

int audio_regulator_reset(struct audio_regulator_context *const context)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.reset != NULL) {
		ret = context->cb.reset(context->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to reset the regulator implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No reset callback");
	}

	return 0;
}

int audio_regulator_uninit(struct audio_regulator_context *const context)
{
	int ret;

	if (!valid_entry(context, &ret)) {
		return ret;
	}

	if (context->cb.uninitialize != NULL) {
		ret = context->cb.uninitialize(context->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to uninitialize the regulator implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No un initialize callback");
	}

	context->imp_ctx = NULL;
	context->state = AUDIO_REGULATOR_STATE_UNINITIALIZED;
	memset(&context->cb, 0, sizeof(struct audio_regulator_ops));

	return 0;
}

int audio_regulator_init(struct audio_regulator_context *const context,
			 struct audio_regulator_implementation_ctx const *const implementation_ctx,
			 struct audio_regulator_ops const *const implementation_cb)
{
	int ret;

	if ((context == NULL) || (implementation_ctx == NULL) || (implementation_cb == NULL)) {
		LOG_ERR("Regulator call parameter error");
		return -EINVAL;
	}

	if (implementation_cb->update == NULL) {
		LOG_ERR("Regulator mandatory callback is not configured");
		return -EINVAL;
	}

	memset(context, 0, sizeof(struct audio_regulator_context));

	context->imp_ctx = (struct audio_regulator_implementation_ctx *)implementation_ctx;

	memcpy(&context->cb, implementation_cb, sizeof(struct audio_regulator_ops));

	if (context->cb.initialize != NULL) {
		ret = context->cb.initialize(context->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to initialize the regulator implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No initialize callback");
	}

	context->state = AUDIO_REGULATOR_STATE_INITIALIZED;

	return 0;
}
