/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define TEST_PWM_COUNT                      DT_PROP_LEN(DT_PATH(zephyr_user), pwms)
#define TEST_PWM_CONFIG_ENTRY(idx, node_id) PWM_DT_SPEC_GET_BY_IDX(node_id, idx)
#define TEST_PWM_CONFIG_ARRAY(node_id)                                                             \
	{                                                                                          \
		LISTIFY(TEST_PWM_COUNT, TEST_PWM_CONFIG_ENTRY, (,), node_id)                       \
	}

static const struct pwm_dt_spec pwms_dt[] = TEST_PWM_CONFIG_ARRAY(DT_PATH(zephyr_user));


static void test_pwm_set(const struct pwm_dt_spec *pwm_dt, uint8_t duty)
{
	int result;
	uint32_t pulse = (uint32_t)((pwm_dt->period * duty) / 100);
	bool inverted = (pwm_dt->flags & PWM_POLARITY_INVERTED) ? true : false;

	TC_PRINT("Test case: [Channel: %" PRIu32 "] [Period: %" PRIu32 "] [Pulse: %" PRIu32
		 "] [Inverted: %s]\n",
		 pwm_dt->channel, pwm_dt->period, pulse, inverted ? "Yes" : "No");

	result = pwm_set_dt(pwm_dt, pwm_dt->period, pulse);
	zassert_false(result, "Failed on pwm_set() call");
}


ZTEST(pwm_channels, test_pwm_set_cycles)
{
	for (int i = 0; i < TEST_PWM_COUNT; i++) {

		/* Test case: [Duty: 100%] */
		test_pwm_set(&pwms_dt[i], 100);
	}

	for (int i = 0; i < TEST_PWM_COUNT; i++) {

		/* Test case: [Duty: 50%] */
		test_pwm_set(&pwms_dt[i], 50);
	}
}

static void *suite_setup(void)
{
	for (int i = 0; i < TEST_PWM_COUNT; i++) {
		zassert_true(device_is_ready(pwms_dt[i].dev), "PWM device is not ready");
		k_object_access_grant(pwms_dt[i].dev, k_current_get());
	}

	return NULL;
}

ZTEST_SUITE(pwm_channels, NULL, suite_setup, NULL, NULL, NULL);
