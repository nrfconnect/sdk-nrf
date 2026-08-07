/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "audio_regulator.h"

#define TEST_ARRAY_SIZE		      (8)
#define TEST_REGULATOR_INIT_FLAG      false
#define TEST_REGULATOR_INIT_ERROR     (0x12345678)
#define TEST_REGULATOR_RESET_FLAG     false
#define TEST_REGULATOR_RESET_ERROR    (0x56781234)
#define TEST_REGULATOR_SET_FLAG	      true
#define TEST_REGULATOR_SET_ERROR      (0x87654321)
#define TEST_REGULATOR_UPDATE_FLAG    true
#define TEST_REGULATOR_UPDATE_ERROR   (0x43218765)
#define TEST_REGULATOR_UPDATE_PTS_REF (0xfffff000)
#define TEST_REGULATOR_UPDATE_PTS     (0xffffffff)
#define TEST_REGULATOR_ERROR_GET      (0x18273645)

struct regulator_imp_config {
	uint16_t array[TEST_ARRAY_SIZE];
	uint32_t data_32;
};

struct regulator_imp_ctx {
	struct regulator_imp_config config;
	bool flag;
	int32_t error;
};

static struct audio_regulator_context context;

static struct regulator_imp_ctx imp_ctx_init = {.config = {.array = {0}, .data_32 = 0xdeadbeef},
						.flag = TEST_REGULATOR_INIT_FLAG,
						.error = TEST_REGULATOR_INIT_ERROR};

static struct regulator_imp_ctx imp_ctx_set = {
	.config = {.array = {0x10, 0x11, 0x20, 0x21, 0x22, 0x30, 0x31, 0x32},
		   .data_32 = 0x12345678},
	.flag = TEST_REGULATOR_SET_FLAG,
	.error = TEST_REGULATOR_SET_ERROR};

static struct regulator_imp_config config;

static int init_cb(struct audio_regulator_implementation_ctx *const context)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_REGULATOR_INIT_FLAG;
	ctx->error = TEST_REGULATOR_INIT_ERROR;

	return 0;
}

static int uninit_cb(struct audio_regulator_implementation_ctx *const context)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	memset(&ctx, 0, sizeof(struct regulator_imp_ctx));

	return 0;
}

static int reset_cb(struct audio_regulator_implementation_ctx *const context)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_REGULATOR_RESET_FLAG;
	ctx->error = TEST_REGULATOR_RESET_ERROR;

	return 0;
}

static int config_set_cb(struct audio_regulator_implementation_ctx *const context,
			 struct audio_regulator_configuration const *const configuration)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;
	struct regulator_imp_config *config = (struct regulator_imp_config *)configuration;

	if (ctx == NULL || config == NULL) {
		return -EINVAL;
	}

	memcpy(&ctx->config, config, sizeof(struct regulator_imp_config));

	ctx->flag = TEST_REGULATOR_SET_FLAG;
	ctx->error = TEST_REGULATOR_SET_ERROR;

	return 0;
}

static int config_get_cb(struct audio_regulator_implementation_ctx const *const context,
			 struct audio_regulator_configuration *const configuration)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;
	struct regulator_imp_config *config = (struct regulator_imp_config *)configuration;

	if (ctx == NULL || config == NULL) {
		return -EINVAL;
	}

	memcpy(config, &ctx->config, sizeof(struct regulator_imp_config));

	return 0;
}

static int update_cb(struct audio_regulator_implementation_ctx *const context, void *pts_ref,
		     void *pts, void *const error)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL || error == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_REGULATOR_UPDATE_FLAG;
	ctx->error = TEST_REGULATOR_UPDATE_ERROR;

	*((int32_t *)error) = ctx->error;

	return 0;
}

static int error_get_cb(struct audio_regulator_implementation_ctx *const context, void *const error)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if ((ctx == NULL) || (error == NULL)) {
		return -EINVAL;
	}

	ctx->error = TEST_REGULATOR_ERROR_GET;

	*((int32_t *)error) = ctx->error;

	return 0;
}

struct audio_regulator_ops imp_cb = {.initialize = init_cb,
				     .uninitialize = uninit_cb,
				     .reset = reset_cb,
				     .configuration_set = config_set_cb,
				     .configuration_get = config_get_cb,
				     .update = update_cb,
				     .error_get = error_get_cb};

struct audio_regulator_ops imp_cb_null = {.initialize = NULL,
					  .uninitialize = NULL,
					  .reset = NULL,
					  .configuration_set = NULL,
					  .configuration_get = NULL,
					  .update = NULL,
					  .error_get = NULL};

struct audio_regulator_ops imp_cb_man = {.initialize = NULL,
					 .uninitialize = NULL,
					 .reset = NULL,
					 .configuration_set = NULL,
					 .configuration_get = NULL,
					 .update = update_cb,
					 .error_get = NULL};

static void test_ctx_ptrs(struct audio_regulator_context *ctx_test,
			  struct audio_regulator_context *ctx_ref)
{
	zassert_equal_ptr(ctx_test->imp_ctx, ctx_ref->imp_ctx,
			  "Failed with mismatch of implementation context pointers");
	zassert_mem_equal(&ctx_test->cb, &ctx_ref->cb, sizeof(struct audio_regulator_ops),
			  "Failed with mismatch callbacks");
}

static void test_imp_ctx(struct regulator_imp_ctx *imp_ctx_test,
			 struct regulator_imp_ctx *imp_ctx_ref)
{
	zassert_mem_equal(imp_ctx_test, imp_ctx_ref, sizeof(struct regulator_imp_ctx),
			  "Failed with mismatch contexts");
}

ZTEST(suite_audio_regulator_tests, test_null)
{
	int ret;
	int error;
	uint32_t pts_ref, pts;

	ret = audio_regulator_init(NULL, (struct audio_regulator_implementation_ctx *)&imp_ctx_init,
				   &imp_cb);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_init(
		&context, (struct audio_regulator_implementation_ctx *)&imp_ctx_init, NULL);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_uninit(NULL);
	zassert_equal(ret, -EINVAL, "Uninitialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_reset(NULL);
	zassert_equal(ret, -EINVAL, "Reset function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_configuration_set(NULL,
						(struct audio_regulator_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Set configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_configuration_get(NULL,
						(struct audio_regulator_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Get configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_update_error(NULL, (void *)&pts_ref, (void *)&pts, (void *)&error);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_update_error(&context, NULL, (void *)&pts, (void *)&error);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_update_error(&context, (void *)&pts_ref, NULL, (void *)&error);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_update_error(&context, (void *)&pts_ref, (void *)&pts, NULL);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_error_get(NULL, (void *)&error);
	zassert_equal(ret, -EINVAL,
		      "Regulator get error function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_error_get(&context, NULL);
	zassert_equal(ret, -EINVAL,
		      "Regulator get error function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
}

ZTEST(suite_audio_regulator_tests, test_state)
{
	int ret;
	int error;
	uint32_t pts_ref, pts;

	context.state = AUDIO_REGULATOR_STATE_INITIALIZED;

	ret = audio_regulator_init(
		&context, (struct audio_regulator_implementation_ctx *const)&imp_ctx_init, &imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);

	context.state = AUDIO_REGULATOR_STATE_UNINITIALIZED;

	ret = audio_regulator_init(
		&context, (struct audio_regulator_implementation_ctx *const)&imp_ctx_init, &imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);

	context.state = AUDIO_REGULATOR_STATE_UNINITIALIZED;

	ret = audio_regulator_uninit(&context);
	zassert_equal(ret, -EINVAL, "Uninitialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_reset(&context);
	zassert_equal(ret, -EINVAL, "Reset function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_configuration_set(&context,
						(struct audio_regulator_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Set configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_configuration_get(&context,
						(struct audio_regulator_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Get configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_update_error(&context, (void *)&pts_ref, (void *)&pts,
					   (void *)&error);
	zassert_equal(ret, -EINVAL,
		      "Rate control update function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_error_get(&context, (void *)&error);
	zassert_equal(ret, -EINVAL,
		      "Rate control update function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
}

ZTEST(suite_audio_regulator_tests, test_null_imp_ctx_cb)
{
	int ret;
	struct audio_regulator_context context_tmp;
	struct regulator_imp_ctx imp_ctx_tmp;

	memset(&context, 0, sizeof(struct audio_regulator_context));
	memset(&context_tmp, 0, sizeof(struct audio_regulator_context));
	memset(&imp_ctx_tmp, 0, sizeof(struct regulator_imp_ctx));

	ret = audio_regulator_init(
		&context, (struct audio_regulator_implementation_ctx *)&imp_ctx_tmp, &imp_cb_null);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (-%d): ret %d",
		      -EINVAL, ret);

	context_tmp.imp_ctx = (struct audio_regulator_implementation_ctx *)&imp_ctx_tmp;
	memcpy(&context_tmp.cb, &imp_cb_man, sizeof(struct audio_regulator_ops));
	memcpy((void *)context_tmp.imp_ctx, &imp_ctx_init, sizeof(struct regulator_imp_ctx));
	context_tmp.state = AUDIO_REGULATOR_STATE_INITIALIZED;

	ret = audio_regulator_init(
		&context, (struct audio_regulator_implementation_ctx *)&imp_ctx_tmp, &imp_cb_man);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_configuration_set(
		&context, (struct audio_regulator_configuration *)&imp_ctx_set.config);
	zassert_equal(ret, 0, "Set configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_configuration_get(&context,
						(struct audio_regulator_configuration *)&config);
	zassert_equal(ret, 0, "Get configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_reset(&context);
	zassert_equal(ret, 0, "Regulator update function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	context_tmp.imp_ctx = NULL;
	context_tmp.state = AUDIO_REGULATOR_STATE_UNINITIALIZED;
	memset(&context_tmp.cb, 0, sizeof(struct audio_regulator_ops));

	ret = audio_regulator_uninit(&context);
	zassert_equal(ret, 0, "Uninitialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
}

ZTEST(suite_audio_regulator_tests, test_imp_ctx_cb)
{
	int ret;
	struct audio_regulator_context context_tmp;
	struct regulator_imp_ctx imp_ctx_tmp;
	uint32_t pts_ref = TEST_REGULATOR_UPDATE_PTS_REF;
	uint32_t pts = TEST_REGULATOR_UPDATE_PTS;
	int error;

	memset(&context, 0, sizeof(struct audio_regulator_context));
	memset(&context_tmp, 0, sizeof(struct audio_regulator_context));

	context_tmp.state = AUDIO_REGULATOR_STATE_INITIALIZED;
	context_tmp.imp_ctx = (struct audio_regulator_implementation_ctx *)&imp_ctx_tmp;
	memcpy(&context_tmp.cb, &imp_cb, sizeof(struct audio_regulator_ops));
	memcpy((void *)context_tmp.imp_ctx, &imp_ctx_init, sizeof(struct regulator_imp_ctx));

	ret = audio_regulator_init(
		&context, (struct audio_regulator_implementation_ctx *)&imp_ctx_tmp, &imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_configuration_set(
		&context, (struct audio_regulator_configuration *)&imp_ctx_set.config);
	zassert_equal(ret, 0, "Set configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_configuration_get(&context,
						(struct audio_regulator_configuration *)&config);
	zassert_equal(ret, 0, "Get configuration function did not return 0: ret %d", ret);
	zassert_mem_equal(&config, &imp_ctx_set.config, sizeof(struct regulator_imp_config),
			  "Failed to read back the configuration");
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx, &imp_ctx_set);

	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->flag = TEST_REGULATOR_UPDATE_FLAG;
	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->error = TEST_REGULATOR_UPDATE_ERROR;

	ret = audio_regulator_update_error(&context, (void *)&pts_ref, (void *)&pts,
					   (void *)&error);
	zassert_equal(ret, 0, "Regulator update function did not return 0: ret %d", ret);
	zassert_equal(error, TEST_REGULATOR_UPDATE_ERROR,
		      "Regulator update function did not return correct error (%d): error %d",
		      TEST_REGULATOR_UPDATE_ERROR, error);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->error = TEST_REGULATOR_ERROR_GET;

	ret = audio_regulator_error_get(&context, (void *)&error);
	zassert_equal(ret, 0, "Regulator update function did not return 0: ret %d", ret);
	zassert_equal(error, TEST_REGULATOR_ERROR_GET,
		      "Regulator get error function did not return correct error (%d): error %d",
		      TEST_REGULATOR_ERROR_GET, error);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->flag = TEST_REGULATOR_RESET_FLAG;
	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->error = TEST_REGULATOR_RESET_ERROR;

	ret = audio_regulator_reset(&context);
	zassert_equal(ret, 0, "Regulator update function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	context_tmp.imp_ctx = NULL;
	context_tmp.state = AUDIO_REGULATOR_STATE_UNINITIALIZED;
	memset(&context_tmp.cb, 0, sizeof(struct audio_regulator_ops));

	ret = audio_regulator_uninit(&context);
	zassert_equal(ret, 0, "Uninitialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
}
