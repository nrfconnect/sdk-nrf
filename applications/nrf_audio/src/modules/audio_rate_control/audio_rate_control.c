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

static struct audio_rate_control_ctx contexts[AUDIO_RATE_CONTROL_TYPE_COUNT] = {
	[0 ...(AUDIO_RATE_CONTROL_TYPE_COUNT - 1)] = {.state = AUDIO_RATE_CONTROL_STATE_EMPTY}};

/**
 * @brief	Get the context for a specific rate control type.
 *
 * @param	type	[in]	The type of the rate control source.
 * @param	ctx	[out]	Pointer to the context pointer to be filled.
 *
 * @retval	-EINVAL		If the type is invalid.
 * @retval	-ESRCH		If the context is empty.
 * @retval	-EACCES		If the context is registered but uninitialized.
 * @retval	0		If the context is valid and returned successfully.
 */
static int ctx_by_type_get(uint8_t type, struct audio_rate_control_ctx **ctx)
{
	if (type >= AUDIO_RATE_CONTROL_TYPE_COUNT) {
		return -EINVAL;
	}

	*ctx = &contexts[type];
	if ((*ctx)->state == AUDIO_RATE_CONTROL_STATE_EMPTY) {
		return -ESRCH;
	}

	if ((*ctx)->state == AUDIO_RATE_CONTROL_STATE_UNINITIALIZED) {
		return -EACCES;
	}

	return 0;
}

int audio_rate_control_cfg_set(uint8_t type, struct audio_rate_control_cfg const *const cfg)
{
	int ret;
	struct audio_rate_control_ctx *ctx = NULL;

	ret = ctx_by_type_get(type, &ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->ops.cfg_set == NULL) {
		LOG_ERR("No configuration set callback");
		return -ENOTSUP;
	}

	ret = ctx->ops.cfg_set(cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_cfg_get(uint8_t type, struct audio_rate_control_cfg *cfg)
{
	int ret;
	struct audio_rate_control_ctx *ctx = NULL;

	ret = ctx_by_type_get(type, &ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->ops.cfg_get == NULL) {
		LOG_ERR("No configuration get callback");
		return -ENOTSUP;
	}

	ret = ctx->ops.cfg_get(cfg);
	if (ret != 0) {
		LOG_ERR("Failed to get the config for the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_update(uint8_t type, void *const control_val_u, bool calibrate)
{
	int ret;
	struct audio_rate_control_ctx *ctx = NULL;

	ret = ctx_by_type_get(type, &ctx);
	if (ret != 0) {
		return ret;
	}

	if (control_val_u == NULL) {
		LOG_ERR("Rate control error pointer NULL");
		return -EINVAL;
	}

	if (ctx->ops.update == NULL) {
		LOG_ERR("No update error callback");
		return -ENOTSUP;
	}

	ret = ctx->ops.update(control_val_u, calibrate);
	if (ret != 0) {
		LOG_DBG("Failed to update the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_reset(uint8_t type)
{
	int ret;
	struct audio_rate_control_ctx *ctx = NULL;

	ret = ctx_by_type_get(type, &ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->ops.reset == NULL) {
		LOG_ERR("No reset callback");
		return -ENOTSUP;
	}

	ret = ctx->ops.reset();
	if (ret != 0) {
		LOG_ERR("Failed to reset the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_register(uint8_t type, struct audio_rate_control_ops const *const imp_ops)
{
	int ret;
	struct audio_rate_control_ctx *ctx = NULL;

	if (imp_ops == NULL) {
		LOG_ERR("Rate control implementation ops is NULL");
		return -EINVAL;
	}

	ret = ctx_by_type_get(type, &ctx);
	if (ret == -EINVAL) {
		LOG_ERR("Invalid type");
		return ret;
	}

	if (ret != -ESRCH) {
		LOG_ERR("Rate control type already registered");
		return -EEXIST;
	}

	if (imp_ops->update == NULL) {
		LOG_ERR("Rate control mandatory callback is not configured");
		return -EINVAL;
	}

	memcpy(&ctx->ops, imp_ops, sizeof(struct audio_rate_control_ops));
	ctx->state = AUDIO_RATE_CONTROL_STATE_UNINITIALIZED;

	return 0;
}

int audio_rate_control_uninit(uint8_t type)
{
	int ret;
	struct audio_rate_control_ctx *ctx = NULL;

	ret = ctx_by_type_get(type, &ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->ops.uninitialize != NULL) {
		ret = ctx->ops.uninitialize();
		if (ret != 0) {
			LOG_ERR("Failed to uninitialize the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_ERR("No uninitialize callback");
	}

	ctx->state = AUDIO_RATE_CONTROL_STATE_UNINITIALIZED;

	return 0;
}

int audio_rate_control_init(uint8_t type)
{
	int ret;
	struct audio_rate_control_ctx *ctx = NULL;

	ret = ctx_by_type_get(type, &ctx);
	if (ret != 0 && ret != -EACCES) {
		return ret;
	}

	if (ctx->ops.initialize != NULL) {
		ret = ctx->ops.initialize();
		if (ret != 0) {
			LOG_ERR("Failed to initialize the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No initialize operation");
	}

	ctx->state = AUDIO_RATE_CONTROL_STATE_INITIALIZED;

	return 0;
}
