/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <app_jwt.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(jwt_sample, CONFIG_JWT_SAMPLE_LOG_LEVEL);

#define JWT_DURATION_S (60 * 10)

#define JWT_AUDIENCE_STR "JSON web token for demonstration"

static int jwt_generate(uint32_t exp_time_s, char *out_buffer, const size_t out_buffer_size)
{
	if (!out_buffer || !out_buffer_size) {
		return -EINVAL;
	}

	int err = -EINVAL;

	struct app_jwt_data jwt = {.sec_tag = 0, /* Irrelevant since JWT will be signed with IAK */
				   .key = 0,	 /* Irrelevant since JWT will be signed with IAK */
				   .alg = JWT_ALG_TYPE_ES256,
				   .audience = JWT_AUDIENCE_STR,
				   .jwt_buf = out_buffer,
				   .jwt_sz = out_buffer_size};

	if (exp_time_s > APP_JWT_VALID_TIME_S_MAX) {
		jwt.validity_s = APP_JWT_VALID_TIME_S_MAX;
	} else if (exp_time_s == 0) {
		jwt.validity_s = APP_JWT_VALID_TIME_S_DEF;
	} else {
		jwt.validity_s = exp_time_s;
	}

	char device_uuid_str[APP_JWT_UUID_V4_STR_LEN] = {0};

	if (0 != app_jwt_get_uuid(device_uuid_str, APP_JWT_UUID_V4_STR_LEN)) {
		return -ENXIO;
	}

	jwt.subject = device_uuid_str;

	err = app_jwt_generate(&jwt);

	return err;
}

int main(void)
{
	LOG_INF("Application JWT sample (%s)", CONFIG_BOARD);

	char jwt_str[APP_JWT_STR_MAX_LEN] = {0};

	int ret = jwt_generate(JWT_DURATION_S, jwt_str, APP_JWT_STR_MAX_LEN);

	if (ret == 0) {
		LOG_INF("JWT(%d) : %s", strlen(jwt_str), jwt_str);
	} else {
		LOG_ERR("jwt_generate error : %d", ret);
	}

	return 0;
}
