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
#define TEST_REGULATOR_INIT_CTRL      (0x12345678)
#define TEST_REGULATOR_RESET_FLAG     false
#define TEST_REGULATOR_RESET_CTRL     (0x56781234)
#define TEST_REGULATOR_SET_FLAG	      true
#define TEST_REGULATOR_SET_CTRL	      (0x87654321)
#define TEST_REGULATOR_UPDATE_FLAG    true
#define TEST_REGULATOR_UPDATE_CTRL    (0x43218765)
#define TEST_REGULATOR_UPDATE_TIMESTAMP_REF (0xfffff000)
#define TEST_REGULATOR_UPDATE_TIMESTAMP	    (0xffffffff)
#define TEST_REGULATOR_CTRL_GET	      (0x18273645)

struct regulator_imp_config {
	uint16_t array[TEST_ARRAY_SIZE];
	uint32_t data_32;
};

struct regulator_imp_ctx {
	struct regulator_imp_config config;
	bool flag;
	int control_val_u;
};

static struct audio_regulator_ctx context;

static struct regulator_imp_ctx imp_ctx_init = {.config = {.array = {0}, .data_32 = 0xdeadbeef},
						.flag = TEST_REGULATOR_INIT_FLAG,
						.control_val_u = TEST_REGULATOR_INIT_CTRL};

static struct regulator_imp_ctx imp_ctx_set = {
	.config = {.array = {0x10, 0x11, 0x20, 0x21, 0x22, 0x30, 0x31, 0x32},
		   .data_32 = 0x12345678},
	.flag = TEST_REGULATOR_SET_FLAG,
	.control_val_u = TEST_REGULATOR_SET_CTRL};
static struct audio_regulator_cfg *set_cfg = (struct audio_regulator_cfg *)&imp_ctx_set.config;

static int init_cb(struct audio_regulator_imp_ctx *const context)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_REGULATOR_INIT_FLAG;
	ctx->control_val_u = TEST_REGULATOR_INIT_CTRL;

	return 0;
}

static int uninit_cb(struct audio_regulator_imp_ctx *const context)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx = NULL;

	return 0;
}

static int reset_cb(struct audio_regulator_imp_ctx *const context)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_REGULATOR_RESET_FLAG;
	ctx->control_val_u = TEST_REGULATOR_RESET_CTRL;

	return 0;
}

static int config_set_cb(struct audio_regulator_imp_ctx *const context,
			 struct audio_regulator_cfg const *const cfg)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;
	struct regulator_imp_config *config = (struct regulator_imp_config *)cfg;

	if (ctx == NULL || config == NULL) {
		return -EINVAL;
	}

	memcpy(&ctx->config, config, sizeof(struct regulator_imp_config));

	ctx->flag = TEST_REGULATOR_SET_FLAG;
	ctx->control_val_u = TEST_REGULATOR_SET_CTRL;

	return 0;
}

static int config_get_cb(struct audio_regulator_imp_ctx const *const context,
			 struct audio_regulator_cfg *const cfg)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;
	struct regulator_imp_config *config = (struct regulator_imp_config *)cfg;

	if (ctx == NULL || config == NULL) {
		return -EINVAL;
	}

	memcpy(config, &ctx->config, sizeof(struct regulator_imp_config));

	return 0;
}

static int update_cb(struct audio_regulator_imp_ctx *const context, void *ref_val_r,
		     void *meas_val_y, void *const control_val_u)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if (ctx == NULL || control_val_u == NULL) {
		return -EINVAL;
	}

	ctx->flag = TEST_REGULATOR_UPDATE_FLAG;
	ctx->control_val_u = TEST_REGULATOR_UPDATE_CTRL;

	*((int32_t *)control_val_u) = ctx->control_val_u;

	return 0;
}

static int control_get_cb(struct audio_regulator_imp_ctx *const context, void *const control_val_u)
{
	struct regulator_imp_ctx *ctx = (struct regulator_imp_ctx *)context;

	if ((ctx == NULL) || (control_val_u == NULL)) {
		return -EINVAL;
	}

	ctx->control_val_u = TEST_REGULATOR_CTRL_GET;

	*((int32_t *)control_val_u) = ctx->control_val_u;

	return 0;
}

struct audio_regulator_ops imp_cb = {.initialize = init_cb,
				     .uninitialize = uninit_cb,
				     .reset = reset_cb,
				     .cfg_set = config_set_cb,
				     .cfg_get = config_get_cb,
				     .update = update_cb,
				     .control_get = control_get_cb};

struct audio_regulator_ops imp_cb_null = {.initialize = NULL,
					  .uninitialize = NULL,
					  .reset = NULL,
					  .cfg_set = NULL,
					  .cfg_get = NULL,
					  .update = NULL,
					  .control_get = NULL};

struct audio_regulator_ops imp_cb_man = {.initialize = NULL,
					 .uninitialize = NULL,
					 .reset = NULL,
					 .cfg_set = NULL,
					 .cfg_get = NULL,
					 .update = update_cb,
					 .control_get = NULL};

static void test_ctx_ptrs(struct audio_regulator_ctx *ctx_test, struct audio_regulator_ctx *ctx_ref)
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

ZTEST(suite_audio_regulator_tests, test_null_params)
{
	int ret;
	int control_val_u;
	uint32_t ref_val_r, meas_val_y;
	struct audio_regulator_ctx *context_test = &context;
	struct audio_regulator_imp_ctx *imp_ctx = (struct audio_regulator_imp_ctx *)&imp_ctx_init;
	struct regulator_imp_config config;
	struct audio_regulator_cfg *cfg = (struct audio_regulator_cfg *)&config;

	ret = audio_regulator_init(NULL, imp_ctx, &imp_cb);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_init(context_test, NULL, &imp_cb);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_init(context_test, imp_ctx, NULL);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	context_test = NULL;

	ret = audio_regulator_uninit(context_test);
	zassert_equal(ret, -EINVAL, "Uninitialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_reset(NULL);
	zassert_equal(ret, -EINVAL, "Reset function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_cfg_set(NULL, cfg);
	zassert_equal(ret, -EINVAL,
		      "Set configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_cfg_get(NULL, cfg);
	zassert_equal(ret, -EINVAL,
		      "Get configuration function did not return -EINVAL (%d): ret %d", -EINVAL,
		      ret);

	ret = audio_regulator_update_control(NULL, (void *)&ref_val_r, (void *)&meas_val_y,
					     (void *)&control_val_u);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_update_control(context_test, NULL, (void *)&meas_val_y,
					     (void *)&control_val_u);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_update_control(context_test, (void *)&ref_val_r, NULL,
					     (void *)&control_val_u);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_update_control(context_test, (void *)&ref_val_r, (void *)&meas_val_y,
					     NULL);
	zassert_equal(ret, -EINVAL, "Regulator update function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_control_get(NULL, (void *)&control_val_u);
	zassert_equal(ret, -EINVAL,
		      "Regulator get control_val_u function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	ret = audio_regulator_control_get(context_test, NULL);
	zassert_equal(ret, -EINVAL,
		      "Regulator get control_val_u function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);
}

ZTEST(suite_audio_regulator_tests, test_state)
{
	int ret;
	int control_val_u;
	uint32_t ref_val_r, meas_val_y;
	struct audio_regulator_ctx *context_test = &context;
	struct audio_regulator_imp_ctx *imp_ctx = (struct audio_regulator_imp_ctx *)&imp_ctx_init;
	struct regulator_imp_config config;
	struct audio_regulator_cfg *cfg = (struct audio_regulator_cfg *)&config;

	context_test->state = AUDIO_REGULATOR_STATE_INITIALIZED;

	ret = audio_regulator_init(context_test, imp_ctx, &imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);

	context_test->state = AUDIO_REGULATOR_STATE_UNINITIALIZED;

	ret = audio_regulator_init(context_test, imp_ctx, &imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);

	context_test->state = AUDIO_REGULATOR_STATE_UNINITIALIZED;

	ret = audio_regulator_uninit(context_test);
	zassert_equal(ret, -EACCES, "Uninitialize function did not return -EACCES (%d): ret %d",
		      -EACCES, ret);
	zassert_equal_ptr(context_test, &context,
			  "Uninitialize function set the context pointer to NULL");

	ret = audio_regulator_reset(context_test);
	zassert_equal(ret, -EACCES, "Reset function did not return -EACCES (%d): ret %d", -EACCES,
		      ret);

	ret = audio_regulator_cfg_set(context_test, cfg);
	zassert_equal(ret, -EACCES,
		      "Set configuration function did not return -EACCES (%d): ret %d", -EACCES,
		      ret);

	ret = audio_regulator_cfg_get(context_test, cfg);
	zassert_equal(ret, -EACCES,
		      "Get configuration function did not return -EACCES (%d): ret %d", -EACCES,
		      ret);

	ret = audio_regulator_update_control(context_test, (void *)&ref_val_r, (void *)&meas_val_y,
					     (void *)&control_val_u);
	zassert_equal(ret, -EACCES,
		      "Rate control update function did not return -EINVAL (%d): ret %d", -EACCES,
		      ret);

	ret = audio_regulator_control_get(context_test, (void *)&control_val_u);
	zassert_equal(ret, -EACCES,
		      "Rate control update function did not return -EACCES (%d): ret %d", -EACCES,
		      ret);
}

ZTEST(suite_audio_regulator_tests, test_null_imp_ctx_cb)
{
	int ret;
	struct audio_regulator_ctx *context_test = &context;
	struct audio_regulator_ctx context_tmp;
	struct regulator_imp_ctx imp_ctx_tmp;
	struct audio_regulator_cfg *cfg = (struct audio_regulator_cfg *)&imp_ctx_tmp.config;

	memset(context_test, 0, sizeof(struct audio_regulator_ctx));
	memset(&context_tmp, 0, sizeof(struct audio_regulator_ctx));
	memset(&imp_ctx_tmp, 0, sizeof(struct regulator_imp_ctx));

	context_tmp.imp_ctx = (struct audio_regulator_imp_ctx *)&imp_ctx_tmp;

	ret = audio_regulator_init(context_test, context_tmp.imp_ctx, &imp_cb_null);
	zassert_equal(ret, -EINVAL, "Initialize function did not return -EINVAL (%d): ret %d",
		      -EINVAL, ret);

	context_tmp.imp_ctx = (struct audio_regulator_imp_ctx *)&imp_ctx_tmp;
	memcpy(&context_tmp.cb, &imp_cb_man, sizeof(struct audio_regulator_ops));
	context_tmp.state = AUDIO_REGULATOR_STATE_INITIALIZED;

	ret = audio_regulator_init(context_test, context_tmp.imp_ctx, &imp_cb_man);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_cfg_set(context_test, set_cfg);
	zassert_equal(ret, -ENOTSUP,
		      "Set configuration function did not return -ENOTSUP (%d): ret %d", -ENOTSUP,
		      ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context_test->imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_cfg_get(context_test, cfg);
	zassert_equal(ret, -ENOTSUP,
		      "Get configuration function did not return -ENOTSUP (%d): ret %d", -ENOTSUP,
		      ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context_test->imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_reset(context_test);
	zassert_equal(ret, -ENOTSUP, "Reset function did not return -ENOTSUP (%d): ret %d",
		      -ENOTSUP, ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context_test->imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_uninit(context_test);
	zassert_equal(ret, 0, "Uninitialize function did not return 0: ret %d", ret);
	zassert_equal_ptr(context_test, &context,
			  "Uninitialize function modified the context pointer");
}

ZTEST(suite_audio_regulator_tests, test_imp_ctx_cb)
{
	int ret;
	struct audio_regulator_ctx *context_test = &context;
	struct audio_regulator_ctx context_tmp;
	struct regulator_imp_ctx imp_ctx_tmp;
	struct audio_regulator_cfg *cfg = (struct audio_regulator_cfg *)&imp_ctx_tmp.config;
	uint32_t ref_val_r = TEST_REGULATOR_UPDATE_TIMESTAMP_REF;
	uint32_t meas_val_y = TEST_REGULATOR_UPDATE_TIMESTAMP;
	int control_val_u;

	memset(context_test, 0, sizeof(struct audio_regulator_ctx));
	memset(&context_tmp, 0, sizeof(struct audio_regulator_ctx));
	memset(&imp_ctx_tmp, 0, sizeof(struct regulator_imp_ctx));

	context_tmp.imp_ctx = (struct audio_regulator_imp_ctx *)&imp_ctx_tmp;
	memcpy(&context_tmp.cb, &imp_cb, sizeof(struct audio_regulator_ops));
	memcpy((void *)context_tmp.imp_ctx, &imp_ctx_init, sizeof(struct regulator_imp_ctx));
	context_tmp.state = AUDIO_REGULATOR_STATE_INITIALIZED;

	ret = audio_regulator_init(context_test, context_tmp.imp_ctx, &imp_cb);
	zassert_equal(ret, 0, "Initialize function did not return 0: ret %d", ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	memcpy((void *)context_tmp.imp_ctx, &imp_ctx_set, sizeof(struct regulator_imp_ctx));

	ret = audio_regulator_cfg_set(context_test, set_cfg);
	zassert_equal(ret, 0, "Set configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	memset((void *)context_tmp.imp_ctx, 0, sizeof(struct regulator_imp_ctx));

	ret = audio_regulator_cfg_get(context_test, cfg);
	zassert_equal(ret, 0, "Get configuration function did not return 0: ret %d", ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context.imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->flag = TEST_REGULATOR_UPDATE_FLAG;
	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->control_val_u =
		TEST_REGULATOR_UPDATE_CTRL;

	ret = audio_regulator_update_control(context_test, (void *)&ref_val_r, (void *)&meas_val_y,
					     (void *)&control_val_u);
	zassert_equal(ret, 0, "Regulator update function did not return 0: ret %d", ret);
	zassert_equal(control_val_u, TEST_REGULATOR_UPDATE_CTRL,
		      "Regulator update function did not return correct control_val_u (%d): "
		      "control_val_u %d",
		      TEST_REGULATOR_UPDATE_CTRL, control_val_u);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context_test->imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->control_val_u = TEST_REGULATOR_CTRL_GET;

	ret = audio_regulator_control_get(context_test, (void *)&control_val_u);
	zassert_equal(ret, 0, "Regulator update function did not return 0: ret %d", ret);
	zassert_equal(control_val_u, TEST_REGULATOR_CTRL_GET,
		      "Regulator get control_val_u function did not return correct control_val_u "
		      "(%d): control_val_u %d",
		      TEST_REGULATOR_CTRL_GET, control_val_u);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context_test->imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->flag = TEST_REGULATOR_RESET_FLAG;
	((struct regulator_imp_ctx *)context_tmp.imp_ctx)->control_val_u =
		TEST_REGULATOR_RESET_CTRL;

	ret = audio_regulator_reset(context_test);
	zassert_equal(ret, 0, "Regulator update function did not return 0: ret %d", ret);
	test_ctx_ptrs(context_test, &context_tmp);
	test_imp_ctx((struct regulator_imp_ctx *)context_test->imp_ctx,
		     (struct regulator_imp_ctx *)context_tmp.imp_ctx);

	ret = audio_regulator_uninit(context_test);
	zassert_equal(ret, 0, "Uninitialize function did not return 0: ret %d", ret);
	zassert_equal_ptr(context_test, &context,
			  "Uninitialize function modified the context pointer");
}
