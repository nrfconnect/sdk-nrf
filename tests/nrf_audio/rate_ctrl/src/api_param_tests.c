/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "audio_rate_control.h"

#define TEST_ARRAY_SIZE		       (8)
#define TEST_RATE_CONTROL_INIT_FLAG    false
#define TEST_RATE_CONTROL_INIT_ERROR   (0x12345678)
#define TEST_RATE_CONTROL_RESET_FLAG   false
#define TEST_RATE_CONTROL_RESET_ERROR  (0x56781234)
#define TEST_RATE_CONTROL_SET_FLAG     true
#define TEST_RATE_CONTROL_SET_ERROR    (0x87654321)
#define TEST_RATE_CONTROL_UPDATE_FLAG  true
#define TEST_RATE_CONTROL_UPDATE_ERROR (0x43218765)

struct rate_control_imp_config {
	uint16_t array[TEST_ARRAY_SIZE];
	uint32_t data_32;
};

struct rate_control_imp_ctx {
	struct rate_control_imp_config config;
	bool flag;
	int32_t error;
};

static struct audio_rate_control_context context;

static struct rate_control_imp_ctx imp_ctx_init = {.config = {.array = {0}, .data_32 = 0xdeadbeef},
						   .flag = TEST_RATE_CONTROL_INIT_FLAG,
						   .error = TEST_RATE_CONTROL_INIT_ERROR};

static struct rate_control_imp_ctx imp_ctx_set = {
	.config = {.array = {0x10, 0x11, 0x20, 0x21, 0x22, 0x30, 0x31, 0x32},
		   .data_32 = 0x12345678},
	.flag = TEST_RATE_CONTROL_SET_FLAG,
	.error = TEST_RATE_CONTROL_SET_ERROR};

static struct rate_control_imp_config config;

static int init_cb(struct audio_rate_control_implementation_ctx *const context)
{
	struct rate_control_imp_ctx *ctx = (struct rate_control_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_RATE_CONTROL_INIT_FLAG;
	ctx->error = TEST_RATE_CONTROL_INIT_ERROR;

	return 0;
}

static int uninit_cb(struct audio_rate_control_implementation_ctx *const context)
{
	struct rate_control_imp_ctx *ctx = (struct rate_control_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	memset(&ctx, 0, sizeof(struct rate_control_imp_ctx));

	return 0;
}

static int reset_cb(struct audio_rate_control_implementation_ctx *const context)
{
	struct rate_control_imp_ctx *ctx = (struct rate_control_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_RATE_CONTROL_RESET_FLAG;
	ctx->error = TEST_RATE_CONTROL_RESET_ERROR;

	return 0;
}

static int config_set_cb(struct audio_rate_control_implementation_ctx *const context,
			 struct audio_rate_control_configuration const *const configuration)
{
	struct rate_control_imp_ctx *ctx = (struct rate_control_imp_ctx *)context;
	struct rate_control_imp_config *config = (struct rate_control_imp_config *)configuration;

	if (ctx == NULL || config == NULL) {
		return -EINVAL;
	}

	memcpy(&ctx->config, config, sizeof(struct rate_control_imp_config));

	ctx->flag = TEST_RATE_CONTROL_SET_FLAG;
	ctx->error = TEST_RATE_CONTROL_SET_ERROR;

	return 0;
}

static int config_get_cb(struct audio_rate_control_implementation_ctx const *const context,
			 struct audio_rate_control_configuration *const configuration)
{
	struct rate_control_imp_ctx *ctx = (struct rate_control_imp_ctx *)context;
	struct rate_control_imp_config *config = (struct rate_control_imp_config *)configuration;

	if (ctx == NULL || config == NULL) {
		return -EINVAL;
	}

	memcpy(config, &ctx->config, sizeof(struct rate_control_imp_config));

	return 0;
}

static int update_cb(struct audio_rate_control_implementation_ctx *const context, void *const error)
{
	struct rate_control_imp_ctx *ctx = (struct rate_control_imp_ctx *)context;

	if (ctx == NULL || error == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_RATE_CONTROL_UPDATE_FLAG;
	ctx->error = TEST_RATE_CONTROL_UPDATE_ERROR;

	return 0;
}

struct audio_rate_control_ops imp_cb = {.initialize = init_cb,
					.uninitialize = uninit_cb,
					.reset = reset_cb,
					.configuration_set = config_set_cb,
					.configuration_get = config_get_cb,
					.update = update_cb};

struct audio_rate_control_ops imp_cb_null = {.initialize = NULL,
					     .uninitialize = NULL,
					     .reset = NULL,
					     .configuration_set = NULL,
					     .configuration_get = NULL,
					     .update = NULL};

struct audio_rate_control_ops imp_cb_man = {.initialize = NULL,
					    .uninitialize = NULL,
					    .reset = NULL,
					    .configuration_set = NULL,
					    .configuration_get = NULL,
					    .update = update_cb};

static void test_ctx_ptrs(struct audio_rate_control_context *ctx_test,
			  struct audio_rate_control_context *ctx_ref)
{
	zassert_equal_ptr(ctx_test->imp_ctx, ctx_ref->imp_ctx,
			  "Failed with mismatch of implementation context pointers");
	zassert_mem_equal(&ctx_test->cb, &ctx_ref->cb, sizeof(struct audio_rate_control_ops),
			  "Failed with mismatch callbacks");
}

static void test_imp_ctx(struct rate_control_imp_ctx *imp_ctx_test,
			 struct rate_control_imp_ctx *imp_ctx_ref)
{
	zassert_mem_equal(imp_ctx_test, imp_ctx_ref, sizeof(struct rate_control_imp_ctx),
			  "Failed with mismatch contexts");
}

ZTEST(suite_audio_rate_control_tests, test_null)
{
	int ret;
	int error;

	ret = audio_rate_control_init(
		NULL, (struct audio_rate_control_implementation_ctx *const)&imp_ctx_init, &imp_cb);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_rate_control_init(&context, NULL, &imp_cb);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_rate_control_init(
		&context, (struct audio_rate_control_implementation_ctx *const)&imp_ctx_init, NULL);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_rate_control_uninit(NULL);
	zassert_equal(ret, -EINVAL, "Uninitialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_rate_control_reset(NULL);
	zassert_equal(ret, -EINVAL, "Reset function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_rate_control_configuration_set(
		NULL, (struct audio_rate_control_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Set configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_rate_control_configuration_get(
		NULL, (struct audio_rate_control_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Get configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_rate_control_update(NULL, (void *)&error);
	zassert_equal(ret, -EINVAL,
		      "Rate control update function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_rate_control_update(&context, NULL);
	zassert_equal(ret, -EINVAL,
		      "Rate control update function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
}

ZTEST(suite_audio_rate_control_tests, test_state)
{
	int ret;
	int error;

	context.state = AUDIO_RATE_CONTROL_STATE_INITIALIZED;

	ret = audio_rate_control_init(
		&context, (struct audio_rate_control_implementation_ctx *const)&imp_ctx_init,
		&imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);

	context.state = AUDIO_RATE_CONTROL_STATE_UNINITIALIZED;

	ret = audio_rate_control_init(
		&context, (struct audio_rate_control_implementation_ctx *const)&imp_ctx_init,
		&imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);

	context.state = AUDIO_RATE_CONTROL_STATE_UNINITIALIZED;

	ret = audio_rate_control_uninit(&context);
	zassert_equal(ret, -EINVAL, "Uninitialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_rate_control_reset(&context);
	zassert_equal(ret, -EINVAL, "Reset function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_rate_control_configuration_set(
		&context, (struct audio_rate_control_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Set configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_rate_control_configuration_get(
		&context, (struct audio_rate_control_configuration *)&config);
	zassert_equal(ret, -EINVAL,
		      "Get configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_rate_control_update(&context, (void *)&error);
	zassert_equal(ret, -EINVAL,
		      "Rate control update function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);
}

ZTEST(suite_audio_rate_control_tests, test_null_imp_ctx_cb)
{
	int ret;
	struct audio_rate_control_context context_tmp;
	struct rate_control_imp_ctx imp_ctx_tmp;

	memset(&context, 0, sizeof(struct audio_rate_control_context));
	memset(&context_tmp, 0, sizeof(struct audio_rate_control_context));
	memset(&imp_ctx_tmp, 0, sizeof(struct rate_control_imp_ctx));

	ret = audio_rate_control_init(&context,
				      (struct audio_rate_control_implementation_ctx *)&imp_ctx_tmp,
				      &imp_cb_null);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (-%d): ret %d",
		      -EINVAL, ret);

	context_tmp.imp_ctx = (struct audio_rate_control_implementation_ctx *)&imp_ctx_tmp;
	memcpy(&context_tmp.cb, &imp_cb_man, sizeof(struct audio_rate_control_ops));
	memcpy((void *)context_tmp.imp_ctx, &imp_ctx_init, sizeof(struct rate_control_imp_ctx));
	context_tmp.state = AUDIO_RATE_CONTROL_STATE_INITIALIZED;

	ret = audio_rate_control_init(&context,
				      (struct audio_rate_control_implementation_ctx *)&imp_ctx_tmp,
				      &imp_cb_man);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_rate_control_configuration_set(
		&context, (struct audio_rate_control_configuration *)&imp_ctx_set.config);
	zassert_equal(ret, 0, "Set configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_rate_control_configuration_get(
		&context, (struct audio_rate_control_configuration *)&config);
	zassert_equal(ret, 0, "Get configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_rate_control_reset(&context);
	zassert_equal(ret, 0, "Rate control update function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	context_tmp.imp_ctx = NULL;
	context_tmp.state = AUDIO_RATE_CONTROL_STATE_UNINITIALIZED;
	memset(&context_tmp.cb, 0, sizeof(struct audio_rate_control_ops));

	ret = audio_rate_control_uninit(&context);
	zassert_equal(ret, 0, "Uninitialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
}

ZTEST(suite_audio_rate_control_tests, test_imp_ctx_cb)
{
	int ret;
	struct audio_rate_control_context context_tmp;
	struct rate_control_imp_ctx imp_ctx_tmp;

	memset(&context, 0, sizeof(struct audio_rate_control_context));
	memset(&context_tmp, 0, sizeof(struct audio_rate_control_context));
	memset(&imp_ctx_tmp, 0, sizeof(struct rate_control_imp_ctx));

	context_tmp.imp_ctx = (struct audio_rate_control_implementation_ctx *)&imp_ctx_tmp;
	memcpy(&context_tmp.cb, &imp_cb, sizeof(struct audio_rate_control_ops));
	memcpy((void *)context_tmp.imp_ctx, &imp_ctx_init, sizeof(struct rate_control_imp_ctx));

	ret = audio_rate_control_init(
		&context, (struct audio_rate_control_implementation_ctx *)&imp_ctx_tmp, &imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	memcpy(context_tmp.imp_ctx, &imp_ctx_set, sizeof(struct rate_control_imp_ctx));

	ret = audio_rate_control_configuration_set(
		&context, (struct audio_rate_control_configuration *)&imp_ctx_set.config);
	zassert_equal(ret, 0, "Set configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_rate_control_configuration_get(
		&context, (struct audio_rate_control_configuration *)&config);
	zassert_equal(ret, 0, "Get configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx, &imp_ctx_set);
	zassert_mem_equal(&config, &imp_ctx_set.config, sizeof(struct rate_control_imp_config),
			  "Failed to read back the configuration");

	((struct rate_control_imp_ctx *)context_tmp.imp_ctx)->flag = TEST_RATE_CONTROL_UPDATE_FLAG;
	((struct rate_control_imp_ctx *)context_tmp.imp_ctx)->error =
		TEST_RATE_CONTROL_UPDATE_ERROR;
	((struct rate_control_imp_ctx *)context_tmp.imp_ctx)->config.data_32 =
		TEST_RATE_CONTROL_SET_ERROR;

	ret = audio_rate_control_update(&context, (void *)TEST_RATE_CONTROL_SET_ERROR);
	zassert_equal(ret, 0, "Rate control update function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	((struct rate_control_imp_ctx *)context_tmp.imp_ctx)->flag = TEST_RATE_CONTROL_RESET_FLAG;
	((struct rate_control_imp_ctx *)context_tmp.imp_ctx)->error = TEST_RATE_CONTROL_RESET_ERROR;

	ret = audio_rate_control_reset(&context);
	zassert_equal(ret, 0, "Rate control update function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
	test_imp_ctx((struct rate_control_imp_ctx *)context.imp_ctx,
		     (struct rate_control_imp_ctx *)context_tmp.imp_ctx);

	context_tmp.imp_ctx = NULL;
	context_tmp.state = AUDIO_RATE_CONTROL_STATE_UNINITIALIZED;
	memset(&context_tmp.cb, 0, sizeof(struct audio_rate_control_ops));

	ret = audio_rate_control_uninit(&context);
	zassert_equal(ret, 0, "Uninitialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(&context, &context_tmp);
}
