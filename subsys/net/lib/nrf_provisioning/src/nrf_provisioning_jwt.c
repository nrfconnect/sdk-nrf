/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/modem_jwt.h>

#include "nrf_provisioning_jwt.h"
#include "nrf_provisioning_at.h"

LOG_MODULE_REGISTER(nrf_provisioning_jwt, CONFIG_NRF_PROVISIONING_LOG_LEVEL);

#define TIME_MAX_LEN 64

int nrf_provisioning_jwt_generate(uint32_t time_valid_s, char *const jwt_buf, size_t jwt_buf_sz)
{
	if (!jwt_buf || !jwt_buf_sz) {
		return -EINVAL;
	}

	int err;
	char buf[TIME_MAX_LEN + 1];
	struct jwt_data jwt = { .audience = NULL,
				.sec_tag = CONFIG_NRF_PROVISIONING_JWT_SEC_TAG,
				.key = JWT_KEY_TYPE_CLIENT_PRIV,
				.alg = JWT_ALG_TYPE_ES256,
				.jwt_buf = jwt_buf,
				.jwt_sz = jwt_buf_sz,
				/* The UUID is present in the iss claim */
				.subject = NULL };

	/* Check if modem time is valid */
	err = nrf_provisioning_at_time_get(buf, sizeof(buf));
	if (err != 0) {
		LOG_ERR("Modem does not have valid date/time, JWT not generated");
		return -ETIME;
	}

	if (time_valid_s > CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S) {
		jwt.exp_delta_s = CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S;
	} else if (time_valid_s == 0) {
		jwt.exp_delta_s = CONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S;
	} else {
		jwt.exp_delta_s = time_valid_s;
	}

	LOG_DBG("Generating JWT");
	err = modem_jwt_generate(&jwt);
	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
	}

	return err;
}
