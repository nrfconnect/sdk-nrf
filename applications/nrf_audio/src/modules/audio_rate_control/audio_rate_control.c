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

static int valid_entry(struct audio_rate_control_ctx const *const ctx)
{
	if (ctx == NULL) {
		LOG_ERR("Rate control ctx pointer is NULL");
		return -EINVAL;
	}

	if (ctx->state == AUDIO_RATE_CONTROL_STATE_UNINITIALIZED) {
		LOG_ERR("Rate control uninitialized");
		return -EACCES;
	}

	return 0;
}

int audio_rate_control_cfg_set(struct audio_rate_control_ctx *const ctx,
			       struct audio_rate_control_cfg const *const cfg)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->cb.cfg_set == NULL) {
		LOG_ERR("No configuration set callback");
		return -ENOTSUP;
	}

	ret = ctx->cb.cfg_set(ctx->imp_ctx, cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_cfg_get(struct audio_rate_control_ctx const *const ctx,
			       struct audio_rate_control_cfg *cfg)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->cb.cfg_get == NULL) {
		LOG_ERR("No configuration get callback");
		return -ENOTSUP;
	}

	ret = ctx->cb.cfg_get(ctx->imp_ctx, cfg);
	if (ret != 0) {
		LOG_ERR("Failed to get the config for the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_update(struct audio_rate_control_ctx *const ctx, void *const control_val_u)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (control_val_u == NULL) {
		LOG_ERR("Rate control error pointer NULL");
		return -EINVAL;
	}

	if (ctx->cb.update == NULL) {
		LOG_ERR("No update error callback");
		return -ENOTSUP;
	}

	ret = ctx->cb.update(ctx->imp_ctx, control_val_u);
	if (ret != 0) {
		LOG_ERR("Failed to update the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_reset(struct audio_rate_control_ctx *const ctx)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->cb.reset == NULL) {
		LOG_ERR("No reset callback");
		return -ENOTSUP;
	}

	ret = ctx->cb.reset(ctx->imp_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to reset the rate control implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_rate_control_uninit(struct audio_rate_control_ctx *ctx)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->cb.uninitialize != NULL) {
		ret = ctx->cb.uninitialize(ctx->imp_ctx);
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

int audio_rate_control_init(struct audio_rate_control_ctx *const ctx,
			    struct audio_rate_control_imp_ctx *const imp_ctx,
			    struct audio_rate_control_ops const *const imp_cb)
{
	int ret;

	if ((ctx == NULL) || (imp_ctx == NULL) || (imp_cb == NULL)) {
		LOG_ERR("Rate control call parameter error");
		return -EINVAL;
	}

	if (imp_cb->update == NULL) {
		LOG_ERR("Rate control mandatory callback is not configured");
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(struct audio_rate_control_ctx));

	ctx->imp_ctx = imp_ctx;

	memcpy(&ctx->cb, imp_cb, sizeof(struct audio_rate_control_ops));

	if (ctx->cb.initialize != NULL) {
		ret = ctx->cb.initialize(ctx->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to initialize the rate control implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("No initialize callback");
	}

	ctx->state = AUDIO_RATE_CONTROL_STATE_INITIALIZED;

	return 0;
}
