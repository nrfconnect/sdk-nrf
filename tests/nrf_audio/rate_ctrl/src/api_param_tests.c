/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "audio_rate_control.h"

#define TEST_ARRAY_SIZE		      (8)
#define TEST_TYPE_INVALID	      (AUDIO_RATE_CONTROL_TYPE_COUNT)
#define TEST_RATE_CONTROL_INIT_FLAG   false
#define TEST_RATE_CONTROL_INIT_CTRL   (0x12345678)
#define TEST_RATE_CONTROL_RESET_FLAG  false
#define TEST_RATE_CONTROL_RESET_CTRL  (0x56781234)
#define TEST_RATE_CONTROL_SET_FLAG    true
#define TEST_RATE_CONTROL_SET_CTRL    (0x87654321)
#define TEST_RATE_CONTROL_UPDATE_FLAG true
#define TEST_RATE_CONTROL_UPDATE_CTRL (0x43218765)

struct rate_control_imp_config {
	uint16_t array[TEST_ARRAY_SIZE];
	uint32_t data_32;
};

/* The new API no longer passes a per-instance context to the callbacks, so the mock
 * implementation keeps its state the same way a real implementation (e.g. hf_audio_clock) would.
 */
static struct {
	struct rate_control_imp_config config;
	bool flag;
	int ctrl_val_u;
} imp_state;

static struct rate_control_imp_config const config_set_val = {
	.array = {0x10, 0x11, 0x20, 0x21, 0x22, 0x30, 0x31, 0x32}, .data_32 = 0x12345678};
static struct audio_rate_control_cfg const *set_cfg =
	(struct audio_rate_control_cfg const *)&config_set_val;

static void imp_state_reset(void)
{
	memset(&imp_state, 0, sizeof(imp_state));
}

static int init_cb(void)
{
	imp_state.flag = TEST_RATE_CONTROL_INIT_FLAG;
	imp_state.ctrl_val_u = TEST_RATE_CONTROL_INIT_CTRL;

	return 0;
}

static int uninit_cb(void)
{
	return 0;
}

static int reset_cb(void)
{
	imp_state.flag = TEST_RATE_CONTROL_RESET_FLAG;
	imp_state.ctrl_val_u = TEST_RATE_CONTROL_RESET_CTRL;

	return 0;
}

static int config_set_cb(struct audio_rate_control_cfg const *const cfg)
{
	struct rate_control_imp_config const *config = (struct rate_control_imp_config const *)cfg;

	if (config == NULL) {
		return -EINVAL;
	}

	memcpy(&imp_state.config, config, sizeof(struct rate_control_imp_config));

	imp_state.flag = TEST_RATE_CONTROL_SET_FLAG;
	imp_state.ctrl_val_u = TEST_RATE_CONTROL_SET_CTRL;

	return 0;
}

static int config_get_cb(struct audio_rate_control_cfg *cfg)
{
	struct rate_control_imp_config *config = (struct rate_control_imp_config *)cfg;

	if (config == NULL) {
		return -EINVAL;
	}

	memcpy(config, &imp_state.config, sizeof(struct rate_control_imp_config));

	return 0;
}

static int update_cb(void *const control_val_u)
{
	if (control_val_u == NULL) {
		return -EINVAL;
	}

	imp_state.flag = TEST_RATE_CONTROL_UPDATE_FLAG;
	imp_state.ctrl_val_u = TEST_RATE_CONTROL_UPDATE_CTRL;

	return 0;
}

static struct audio_rate_control_ops const imp_ops_full = {.initialize = init_cb,
							   .uninitialize = uninit_cb,
							   .reset = reset_cb,
							   .cfg_set = config_set_cb,
							   .cfg_get = config_get_cb,
							   .update = update_cb};

/* Only the mandatory callback is configured; all optional ones are left out */
static struct audio_rate_control_ops const imp_ops_update_only = {.update = update_cb};

/* NOTE: The tests below rely on their declaration order. Each rate control "type" slot is
 * global and, once registered, cannot be unregistered, so each type is only ever touched by
 * one test (or a fixed, ordered sequence of tests).
 */

ZTEST(suite_audio_rate_control_tests, test_invalid_type)
{
	int ret;
	int ctrl_val_u = 0;
	struct audio_rate_control_cfg *cfg = (struct audio_rate_control_cfg *)&imp_state.config;

	ret = audio_rate_control_register(TEST_TYPE_INVALID, &imp_ops_full);
	zassert_equal(ret, -EINVAL, "Register did not return -EINVAL: ret %d", ret);

	ret = audio_rate_control_init(TEST_TYPE_INVALID);
	zassert_equal(ret, -EINVAL, "Init did not return -EINVAL: ret %d", ret);

	ret = audio_rate_control_uninit(TEST_TYPE_INVALID);
	zassert_equal(ret, -EINVAL, "Uninit did not return -EINVAL: ret %d", ret);

	ret = audio_rate_control_reset(TEST_TYPE_INVALID);
	zassert_equal(ret, -EINVAL, "Reset did not return -EINVAL: ret %d", ret);

	ret = audio_rate_control_cfg_set(TEST_TYPE_INVALID, cfg);
	zassert_equal(ret, -EINVAL, "Set configuration did not return -EINVAL: ret %d", ret);

	ret = audio_rate_control_cfg_get(TEST_TYPE_INVALID, cfg);
	zassert_equal(ret, -EINVAL, "Get configuration did not return -EINVAL: ret %d", ret);

	ret = audio_rate_control_update(TEST_TYPE_INVALID, (void *)&ctrl_val_u);
	zassert_equal(ret, -EINVAL, "Update did not return -EINVAL: ret %d", ret);
}

ZTEST(suite_audio_rate_control_tests, test_unregistered_type)
{
	int ret;
	int ctrl_val_u = 0;
	struct audio_rate_control_cfg *cfg = (struct audio_rate_control_cfg *)&imp_state.config;

	ret = audio_rate_control_register(USB_ASYNC, NULL);
	zassert_equal(ret, -EINVAL, "Register with NULL ops did not return -EINVAL: ret %d", ret);

	/* USB has not been registered by any previous test */
	ret = audio_rate_control_init(USB_ASYNC);
	zassert_equal(ret, -ESRCH, "Init did not return -ESRCH: ret %d", ret);

	ret = audio_rate_control_uninit(USB_ASYNC);
	zassert_equal(ret, -ESRCH, "Uninit did not return -ESRCH: ret %d", ret);

	ret = audio_rate_control_reset(USB_ASYNC);
	zassert_equal(ret, -ESRCH, "Reset did not return -ESRCH: ret %d", ret);

	ret = audio_rate_control_cfg_set(USB_ASYNC, cfg);
	zassert_equal(ret, -ESRCH, "Set configuration did not return -ESRCH: ret %d", ret);

	ret = audio_rate_control_cfg_get(USB_ASYNC, cfg);
	zassert_equal(ret, -ESRCH, "Get configuration did not return -ESRCH: ret %d", ret);

	ret = audio_rate_control_update(USB_ASYNC, (void *)&ctrl_val_u);
	zassert_equal(ret, -ESRCH, "Update did not return -ESRCH: ret %d", ret);
}

ZTEST(suite_audio_rate_control_tests, test_mandatory_update_cb)
{
	int ret;
	struct audio_rate_control_ops const missing_update = {0};

	ret = audio_rate_control_register(I2S, &missing_update);
	zassert_equal(ret, -EINVAL,
		      "Register without mandatory update callback did not return -EINVAL: ret %d",
		      ret);

	/* Registration must not have taken place, so the type is still unregistered */
	ret = audio_rate_control_init(I2S);
	zassert_equal(ret, -ESRCH, "Init did not return -ESRCH: ret %d", ret);
}

ZTEST(suite_audio_rate_control_tests, test_optional_cb_not_supported)
{
	int ret;
	int ctrl_val_u = TEST_RATE_CONTROL_SET_CTRL;
	struct audio_rate_control_cfg *cfg = (struct audio_rate_control_cfg *)&imp_state.config;

	imp_state_reset();

	ret = audio_rate_control_register(I2S, &imp_ops_update_only);
	zassert_equal(ret, 0, "Register did not return 0: ret %d", ret);

	ret = audio_rate_control_register(I2S, &imp_ops_update_only);
	zassert_equal(ret, -EEXIST, "Re-register did not return -EEXIST: ret %d", ret);

	/* Not yet initialized */
	ret = audio_rate_control_cfg_set(I2S, cfg);
	zassert_equal(ret, -EACCES, "Set configuration did not return -EACCES: ret %d", ret);

	ret = audio_rate_control_init(I2S);
	zassert_equal(ret, 0, "Init did not return 0: ret %d", ret);

	ret = audio_rate_control_cfg_set(I2S, cfg);
	zassert_equal(ret, -ENOTSUP, "Set configuration did not return -ENOTSUP: ret %d", ret);

	ret = audio_rate_control_cfg_get(I2S, cfg);
	zassert_equal(ret, -ENOTSUP, "Get configuration did not return -ENOTSUP: ret %d", ret);

	ret = audio_rate_control_reset(I2S);
	zassert_equal(ret, -ENOTSUP, "Reset did not return -ENOTSUP: ret %d", ret);

	ret = audio_rate_control_update(I2S, (void *)&ctrl_val_u);
	zassert_equal(ret, 0, "Update did not return 0: ret %d", ret);
	zassert_equal(imp_state.flag, TEST_RATE_CONTROL_UPDATE_FLAG,
		      "Update callback was not invoked");

	ret = audio_rate_control_update(I2S, NULL);
	zassert_equal(ret, -EINVAL, "Update with NULL control value did not return -EINVAL: ret %d",
		      ret);

	/* No uninitialize callback: uninit still succeeds and clears the initialized state */
	ret = audio_rate_control_uninit(I2S);
	zassert_equal(ret, 0, "Uninit did not return 0: ret %d", ret);

	ret = audio_rate_control_cfg_set(I2S, cfg);
	zassert_equal(ret, -EACCES, "Set configuration did not return -EACCES: ret %d", ret);
}

ZTEST(suite_audio_rate_control_tests, test_full_ops_flow)
{
	int ret;
	int ctrl_val_u = TEST_RATE_CONTROL_UPDATE_CTRL;
	struct rate_control_imp_config get_cfg;

	imp_state_reset();

	ret = audio_rate_control_register(AUDIO_PLL, &imp_ops_full);
	zassert_equal(ret, 0, "Register did not return 0: ret %d", ret);

	ret = audio_rate_control_init(AUDIO_PLL);
	zassert_equal(ret, 0, "Init did not return 0: ret %d", ret);
	zassert_equal(imp_state.flag, TEST_RATE_CONTROL_INIT_FLAG,
		      "Initialize callback was not invoked");
	zassert_equal(imp_state.ctrl_val_u, TEST_RATE_CONTROL_INIT_CTRL,
		      "Initialize callback was not invoked");

	ret = audio_rate_control_cfg_set(AUDIO_PLL, set_cfg);
	zassert_equal(ret, 0, "Set configuration did not return 0: ret %d", ret);
	zassert_mem_equal(&imp_state.config, &config_set_val, sizeof(config_set_val),
			  "Configuration was not applied");
	zassert_equal(imp_state.flag, TEST_RATE_CONTROL_SET_FLAG,
		      "Set configuration callback was not invoked");

	ret = audio_rate_control_cfg_get(AUDIO_PLL, (struct audio_rate_control_cfg *)&get_cfg);
	zassert_equal(ret, 0, "Get configuration did not return 0: ret %d", ret);
	zassert_mem_equal(&get_cfg, &config_set_val, sizeof(config_set_val),
			  "Retrieved configuration did not match applied configuration");

	ret = audio_rate_control_update(AUDIO_PLL, (void *)&ctrl_val_u);
	zassert_equal(ret, 0, "Update did not return 0: ret %d", ret);
	zassert_equal(imp_state.flag, TEST_RATE_CONTROL_UPDATE_FLAG,
		      "Update callback was not invoked");
	zassert_equal(imp_state.ctrl_val_u, TEST_RATE_CONTROL_UPDATE_CTRL,
		      "Update callback was not invoked");

	ret = audio_rate_control_reset(AUDIO_PLL);
	zassert_equal(ret, 0, "Reset did not return 0: ret %d", ret);
	zassert_equal(imp_state.flag, TEST_RATE_CONTROL_RESET_FLAG,
		      "Reset callback was not invoked");
	zassert_equal(imp_state.ctrl_val_u, TEST_RATE_CONTROL_RESET_CTRL,
		      "Reset callback was not invoked");

	ret = audio_rate_control_uninit(AUDIO_PLL);
	zassert_equal(ret, 0, "Uninit did not return 0: ret %d", ret);

	ret = audio_rate_control_cfg_get(AUDIO_PLL, (struct audio_rate_control_cfg *)&get_cfg);
	zassert_equal(ret, -EACCES, "Get configuration did not return -EACCES: ret %d", ret);
}
