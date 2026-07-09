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

static int valid_entry(struct audio_regulator_ctx const *const ctx)
{
	if (ctx == NULL) {
		LOG_ERR("Regulator context pointer is NULL");
		return -EINVAL;
	}

	if (ctx->state == AUDIO_REGULATOR_STATE_UNINITIALIZED) {
		LOG_ERR("Regulator uninitialized");
		return -EACCES;
	}

	return 0;
}

int audio_regulator_cfg_set(struct audio_regulator_ctx *const ctx,
			    struct audio_regulator_cfg const *const cfg)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->cb.cfg_set == NULL) {
		LOG_ERR("No configure set callback");
		return -ENOTSUP;
	}

	ret = ctx->cb.cfg_set(ctx->imp_ctx, cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure the regulator implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_regulator_cfg_get(struct audio_regulator_ctx const *const ctx,
			    struct audio_regulator_cfg *cfg)
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
		LOG_ERR("Failed to get the config for the regulator implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_regulator_update_control(struct audio_regulator_ctx const *const ctx,
				   void *const ref_val_r, void *const meas_val_y,
				   void *const control_val_u)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if ((ref_val_r == NULL) || (meas_val_y == NULL) || (control_val_u == NULL)) {
		LOG_ERR("Update error failed due to invalid parameter");
		return -EINVAL;
	}

	if (ctx->cb.update == NULL) {
		LOG_ERR("No update error callback");
		return -ENOTSUP;
	}

	ret = ctx->cb.update(ctx->imp_ctx, ref_val_r, meas_val_y, control_val_u);
	if (ret != 0) {
		LOG_ERR("Failed to update the error for the regulator implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_regulator_control_get(struct audio_regulator_ctx const *const ctx,
				void *const control_val_u)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (control_val_u == NULL) {
		LOG_ERR("Invalid control value pointer");
		return -EINVAL;
	}

	if (ctx->cb.control_get == NULL) {
		LOG_ERR("No last error get callback");
		return -ENOTSUP;
	}

	ret = ctx->cb.control_get(ctx->imp_ctx, control_val_u);
	if (ret != 0) {
		LOG_ERR("Failed to retrieve the last error calculated for "
			"the regulator implementation: %d",
			ret);
		return ret;
	}

	return 0;
}

int audio_regulator_reset(struct audio_regulator_ctx *const ctx)
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
		LOG_ERR("Failed to reset the regulator implementation: %d", ret);
		return ret;
	}

	return 0;
}

int audio_regulator_uninit(struct audio_regulator_ctx *ctx)
{
	int ret;

	ret = valid_entry(ctx);
	if (ret != 0) {
		return ret;
	}

	if (ctx->cb.uninitialize != NULL) {
		ret = ctx->cb.uninitialize(ctx->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to uninitialize the regulator implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_WRN("No uninitialize callback");
	}

	ctx->state = AUDIO_REGULATOR_STATE_UNINITIALIZED;

	return 0;
}

int audio_regulator_init(struct audio_regulator_ctx *const ctx,
			 struct audio_regulator_imp_ctx *const imp_ctx,
			 struct audio_regulator_ops const *const imp_cb)
{
	int ret;

	if ((ctx == NULL) || (imp_ctx == NULL) || (imp_cb == NULL)) {
		LOG_ERR("Regulator call parameter error");
		return -EINVAL;
	}

	if (imp_cb->update == NULL) {
		LOG_ERR("Regulator mandatory callback is not configured");
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(struct audio_regulator_ctx));

	ctx->imp_ctx = (struct audio_regulator_imp_ctx *)imp_ctx;

	memcpy(&ctx->cb, imp_cb, sizeof(struct audio_regulator_ops));

	if (ctx->cb.initialize != NULL) {
		ret = ctx->cb.initialize(ctx->imp_ctx);
		if (ret != 0) {
			LOG_ERR("Failed to initialize the regulator implementation: %d", ret);
			return ret;
		}
	} else {
		LOG_WRN("No initialize callback");
	}

	ctx->state = AUDIO_REGULATOR_STATE_INITIALIZED;

	return 0;
}
